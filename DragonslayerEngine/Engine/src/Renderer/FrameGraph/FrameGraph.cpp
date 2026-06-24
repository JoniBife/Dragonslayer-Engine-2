#include "Renderer/FrameGraph/FrameGraph.hpp"

#include <execution>
#include <Core/ThreadContext.hpp>
#include <Math/MathAux.hpp>

#include "Core/Assert.hpp"
#include "Core/Containers/ArrayHelper.hpp"

constexpr uint32 INITIAL_PASS_CAPACITY = 16;
constexpr uint32 INITIAL_RESOURCE_CAPACITY = 16;
constexpr uint32 INITIAL_ACCESS_CAPACITY = 4;

constexpr uint32 MAX_COLOR_ATTACHMENTS = 8;

TextureHandle TextureHandle::Invalid = {INVALID_RESOURCE_HANDLE_ID};
BufferHandle BufferHandle::Invalid = {INVALID_RESOURCE_HANDLE_ID};

NO_DISCARD FORCE_INLINE static bool IsSameTextureState(const nri::AccessLayoutStage& a, const nri::AccessLayoutStage& b) {
    return a.access == b.access && a.layout == b.layout;
}

NO_DISCARD FORCE_INLINE static bool IsSameBufferState(const nri::AccessStage& a, const nri::AccessStage& b) {
    return a.access == b.access;
}

// A texture finalState with layout == UNDEFINED means "no closing barrier
// required" - the resource's end-of-frame state is whatever the last pass
// left it in.
NO_DISCARD FORCE_INLINE static bool RequiresClosingBarrier(const nri::AccessLayoutStage& finalState) {
    return finalState.layout != nri::Layout::UNDEFINED;
}

// A buffer finalState with no access bits and no stages means "no closing barrier".
NO_DISCARD FORCE_INLINE static bool RequiresClosingBarrier(const nri::AccessStage& finalState) {
    return finalState.access != nri::AccessBits::NONE || finalState.stages != nri::StageBits::NONE;
}

// -----------------------------------------------------------------------------
// PassDesc

PassDesc::PassDesc(const NameString& name, nri::QueueType queueType, FreeListAllocator* allocator) :
    name(name),
    queue(queueType),
    textureAccesses(INITIAL_ACCESS_CAPACITY, allocator),
    colorAttachments(INITIAL_ACCESS_CAPACITY, allocator),
    bufferAccesses(INITIAL_ACCESS_CAPACITY, allocator),
    compiledTextureBarriers(INITIAL_ACCESS_CAPACITY, allocator),
    compiledBufferBarriers(INITIAL_ACCESS_CAPACITY, allocator)
{}

// -----------------------------------------------------------------------------
// FrameGraph - lifetime

FrameGraph::FrameGraph(const nri::CoreInterface& NRI, nri::Device& device, FreeListAllocator* allocator) :
    NRI(NRI),
    device(device),
    allocator(allocator),
    passes(INITIAL_PASS_CAPACITY, allocator),
    textures(INITIAL_RESOURCE_CAPACITY, allocator),
    buffers(INITIAL_RESOURCE_CAPACITY, allocator),
    compiledClosingTextureBarriers(INITIAL_ACCESS_CAPACITY, allocator),
    compiledClosingBufferBarriers(INITIAL_ACCESS_CAPACITY, allocator)
{
    ASSERT(allocator != nullptr);
}

FrameGraph::~FrameGraph() {
    Invalidate();
}

void FrameGraph::Invalidate() {
    // Phase 0a does not own any temp resources yet; once temp creation lands,
    // this is where the graph tears them down. Imports stay as-is - they are
    // owned externally and outlive the graph.
    passes.Reset();
    textures.Reset();
    buffers.Reset();
    compiledClosingTextureBarriers.Reset();
    compiledClosingBufferBarriers.Reset();
    isCompiled = false;
}

void FrameGraph::GetCommandBuffers(TempArray<nri::CommandBuffer*>& commandBuffers) {
    for (PassDesc& pass : passes) {
        ASSERT(pass.commandBuffer != nullptr);
        commandBuffers.Add(pass.commandBuffer);
    }
}

// -----------------------------------------------------------------------------
// FrameGraph - resource declaration

TextureHandle FrameGraph::ImportTexture(
        const NameString& name,
        nri::Texture* texture,
        const TextureViews& views,
        nri::AccessLayoutStage initialState,
        nri::AccessLayoutStage finalState) {

    ASSERT(!isCompiled);
    ASSERT(texture != nullptr);

    textures.Emplace(
        name,
        texture,
        views,
        initialState,
        finalState,
        /*currentState=*/initialState,
        false
    );

    return TextureHandle(textures.GetSize());
}

BufferHandle FrameGraph::ImportBuffer(
        const NameString& name,
        nri::Buffer* buffer,
        const BufferViews& views,
        nri::AccessStage initialState,
        nri::AccessStage finalState) {

    ASSERT(!isCompiled);
    ASSERT(buffer != nullptr);

    buffers.Emplace(
        name,
        buffer,
        views,
        initialState,
        finalState,
        /*currentState=*/initialState,
        false
    );

    return BufferHandle(buffers.GetSize());
}

// TODO This validation should only happen when validation is enabled
static bool IsTextureAccessUnique(const Array<TextureAccessDesc, FreeListAllocator>& accesses, TextureHandle textureHandle) {
    for (const TextureAccessDesc& access : accesses) {
        if (access.textureHandle == textureHandle) {
            return false;
        }
    }
    return true;
}

// TODO This validation should only happen when validation is enabled
static bool IsBufferAccessUnique(const Array<BufferAccessDesc, FreeListAllocator>& accesses, BufferHandle bufferHandle) {
    for (const BufferAccessDesc& access : accesses) {
        if (access.bufferHandle == bufferHandle) {
            return false;
        }
    }
    return true;
}

FrameGraph& FrameGraph::Access(BufferHandle bufferHandle, nri::AccessBits access, nri::StageBits stages) {

    ASSERT(activeDesc != nullptr);
    ASSERT(bufferHandle.IsValid());

    PassDesc& passDesc = *activeDesc;

    // A framegraph pass must only access a resource once
    ASSERT(IsBufferAccessUnique(passDesc.bufferAccesses, bufferHandle));
    passDesc.bufferAccesses.Emplace(bufferHandle, nri::AccessStage(access, stages));

    return *this;
}

FrameGraph& FrameGraph::Access(TextureHandle textureHandle, nri::AccessBits access, nri::Layout layout, nri::StageBits stages) {

    ASSERT(activeDesc != nullptr);
    ASSERT(textureHandle.IsValid());
    ASSERT(layout != nri::Layout::UNDEFINED); // textures must declare a real layout

    PassDesc& passDesc = *activeDesc;

    // A framegraph pass must only access a resource once
    ASSERT(IsTextureAccessUnique(passDesc.textureAccesses, textureHandle));
    passDesc.textureAccesses.Emplace(textureHandle, nri::AccessLayoutStage(access, layout, stages));

    return *this;
}

FrameGraph& FrameGraph::SetColorAttachment(uint32 slot, TextureHandle textureHandle, nri::LoadOp loadOp, nri::StoreOp storeOp, const nri::Color32f& clearColor) {

    ASSERT(activeDesc != nullptr);
    ASSERT(activeDesc->queue == nri::QueueType::GRAPHICS)
    ASSERT(textureHandle.IsValid());

    PassDesc& passDesc = *activeDesc;

    // A framegraph pass must only access a resource once
    ASSERT(IsTextureAccessUnique(passDesc.textureAccesses, textureHandle));
    ASSERT(passDesc.colorAttachments.GetSize() < MAX_COLOR_ATTACHMENTS);

    TextureAttachmentDesc attachmentDesc = {};
    attachmentDesc.textureHandle = textureHandle;
    attachmentDesc.desc.loadOp = loadOp;
    attachmentDesc.desc.storeOp = storeOp;
    attachmentDesc.desc.clearValue.color.f = clearColor;
    passDesc.colorAttachments.Add(attachmentDesc);

    // Register the equivalent access so barrier resolution treats attachments
    // consistently with other resources.
    passDesc.textureAccesses.Emplace(
        textureHandle,
        nri::AccessLayoutStage(
            nri::AccessBits::COLOR_ATTACHMENT,
            nri::Layout::COLOR_ATTACHMENT,
            nri::StageBits::COLOR_ATTACHMENT));

    return *this;

}
FrameGraph& FrameGraph::SetDepthAttachment(TextureHandle textureHandle, nri::LoadOp loadOp, nri::StoreOp storeOp, float clearDepth, uint8 clearStencil) {

    ASSERT(activeDesc != nullptr);
    ASSERT(activeDesc->queue == nri::QueueType::GRAPHICS)
    ASSERT(textureHandle.IsValid());

    PassDesc& passDesc = *activeDesc;

    // A framegraph pass must only access a resource once
    ASSERT(IsTextureAccessUnique(passDesc.textureAccesses, textureHandle));
    ASSERT(!passDesc.hasDepthAttachment);

    passDesc.hasDepthAttachment = true;

    passDesc.depthAttachment.textureHandle = textureHandle;
    passDesc.depthAttachment.desc.loadOp = loadOp;
    passDesc.depthAttachment.desc.storeOp = storeOp;
    passDesc.depthAttachment.desc.clearValue.depthStencil.depth = clearDepth;
    passDesc.depthAttachment.desc.clearValue.depthStencil.stencil = clearStencil;

    // Register the equivalent access so barrier resolution treats attachments
    // consistently with other resources.
    passDesc.textureAccesses.Emplace(
        textureHandle,
        nri::AccessLayoutStage(
            nri::AccessBits::DEPTH_STENCIL_ATTACHMENT,
            nri::Layout::DEPTH_STENCIL_ATTACHMENT,
            nri::StageBits::DEPTH_STENCIL_ATTACHMENT));

    return *this;
}

void FrameGraph::SetImport(TextureHandle textureHandle, nri::Texture* texture, const TextureViews& views) {
    ASSERT(texture != nullptr);
    TextureRegistryEntry& entry = GetTexture(textureHandle);
    ASSERT(!entry.isTemp);
    entry.texture = texture;
    entry.views = views;
    // Initial state is unchanged; the caller guarantees the resource enters the
    // frame in the originally declared state.
}

void FrameGraph::SetImport(BufferHandle bufferHandle, nri::Buffer* buffer, const BufferViews& views) {
    ASSERT(buffer != nullptr);
    BufferRegistryEntry& entry = GetBuffer(bufferHandle);
    ASSERT(!entry.isTemp);
    entry.buffer = buffer;
    entry.views = views;
}

void FrameGraph::Compile() {

    ASSERT(!isCompiled);

    // Phase 0a: temp resource allocation goes here when temp creation is
    // implemented. For now temp arrays are empty so there is nothing to allocate.

    // Start by resetting resources back to their initial state
    for (TextureRegistryEntry& texture : textures) {
        texture.currentState = texture.initialState;
    }
    for (BufferRegistryEntry& buffer : buffers) {
        buffer.currentState = buffer.initialState;
    }

    // Compile all barriers except closing
    for (PassDesc& pass: passes) {

        // Compile texture barriers
        pass.compiledTextureBarriers.Reset();
        for (const TextureAccessDesc& accessDesc : pass.textureAccesses) {

            TextureRegistryEntry& texture = GetTexture(accessDesc.textureHandle);

            const nri::AccessLayoutStage before = texture.currentState;
            const nri::AccessLayoutStage after = accessDesc.state;

            if (!IsSameTextureState(before, after)) {
                pass.compiledTextureBarriers.Emplace(
                    accessDesc.textureHandle,
                    before,
                    after
                );

                texture.currentState = after;
            }
        }

        // Compile buffer barriers
        pass.compiledBufferBarriers.Reset();
        for (const BufferAccessDesc& accessDesc : pass.bufferAccesses) {

            BufferRegistryEntry& buffer = GetBuffer(accessDesc.bufferHandle);

            const nri::AccessStage before = buffer.currentState;
            const nri::AccessStage after = { accessDesc.state.access, accessDesc.state.stages };

            if (!IsSameBufferState(before, after)) {
                pass.compiledBufferBarriers.Emplace(
                    accessDesc.bufferHandle,
                    before,
                    after
                );

                buffer.currentState = after;
            }
        }
    }


    // Compile closing barriers
    compiledClosingTextureBarriers.Reset();
    compiledClosingBufferBarriers.Reset();
    for (uint32 i = 0; i < textures.GetSize(); ++i) {
        const TextureRegistryEntry& tex = textures[i];

        if (!RequiresClosingBarrier(tex.finalState)) {
            continue;
        }

        if (IsSameTextureState(tex.currentState, tex.finalState)) {
            continue;
        }

        const CompiledTextureBarrier barrier = {
            .textureHandle = TextureHandle(i + 1),
            .before = tex.currentState,
            .after = tex.finalState
        };
        compiledClosingTextureBarriers.Add(barrier);
    }

    for (uint32 i = 0; i < buffers.GetSize(); ++i) {
        const BufferRegistryEntry& buf = buffers[i];

        if (!RequiresClosingBarrier(buf.finalState)) {
            continue;
        }

        if (IsSameBufferState(buf.currentState, buf.finalState)) {
            continue;
        }

        const CompiledBufferBarrier barrier = {
            .bufferHandle = BufferHandle(i + 1),
            .before = buf.currentState,
            .after = buf.finalState
        };
        compiledClosingBufferBarriers.Add(barrier);
    }

    isCompiled = true;
}

void FrameGraph::EmitBarriers(
        nri::CommandBuffer& commandBuffer,
        const Array<CompiledTextureBarrier, FreeListAllocator>& compiledTextureBarriers,
        const Array<CompiledBufferBarrier, FreeListAllocator>& compiledBufferBarriers) {

    const uint32 numTextureBarriers = compiledTextureBarriers.GetSize();
    const uint32 numBufferBarriers = compiledBufferBarriers.GetSize();
    if (numTextureBarriers == 0 && numBufferBarriers == 0) {
        return;
    }

    TempArray<nri::TextureBarrierDesc> textureBarriers(std::max(numTextureBarriers, 1u));
    for (const CompiledTextureBarrier& compiledBarrier: compiledTextureBarriers) {

        const TextureRegistryEntry& texture = GetTexture(compiledBarrier.textureHandle);
        textureBarriers.Emplace(
            texture.texture,
            compiledBarrier.before,
            compiledBarrier.after
        );
        // mipNum / layerNum left at 0 == REMAINING (apply to all mips and layers).
    }

   TempArray<nri::BufferBarrierDesc> bufferBarriers(std::max(numBufferBarriers, 1u));
    for (const CompiledBufferBarrier& compiledBarrier: compiledBufferBarriers) {
        const BufferRegistryEntry& buffer = GetBuffer(compiledBarrier.bufferHandle);
        bufferBarriers.Emplace(
            buffer.buffer,
            compiledBarrier.before,
            compiledBarrier.after
        );
    }

    const nri::BarrierDesc desc = {
        .buffers = bufferBarriers.GetData(),
        .bufferNum = bufferBarriers.GetSize(),
        .textures = textureBarriers.GetData(),
        .textureNum = textureBarriers.GetSize(),
    };
    NRI.CmdBarrier(commandBuffer, desc);
}

void FrameGraph::ExecutePass(nri::CommandBuffer& commandBuffer, const PassDesc& pass) {

    EmitBarriers(commandBuffer, pass.compiledTextureBarriers, pass.compiledBufferBarriers);

    const Annotation annotation(NRI, commandBuffer, pass.name.CStr());

    InlineArray<nri::AttachmentDesc, MAX_COLOR_ATTACHMENTS> colorAttachments;
    nri::RenderingDesc renderingDesc = {};

    if (pass.queue == nri::QueueType::GRAPHICS) {

        if (!pass.colorAttachments.IsEmpty()) {
            ASSERT(pass.queue == nri::QueueType::GRAPHICS);

            for (const TextureAttachmentDesc& attachment : pass.colorAttachments) {

                const TextureRegistryEntry& texture = GetTexture(attachment.textureHandle);

                nri::AttachmentDesc& attachmentDesc = colorAttachments.Emplace(attachment.desc);
                attachmentDesc.descriptor = texture.views.colorAttachment;
            }

            renderingDesc.colors = colorAttachments.GetData();
            renderingDesc.colorNum = colorAttachments.GetSize();
        }

        if (pass.hasDepthAttachment) {
            const TextureRegistryEntry& texture = GetTexture(pass.depthAttachment.textureHandle);
            nri::AttachmentDesc depthAttachmentDesc = pass.depthAttachment.desc;
            depthAttachmentDesc.descriptor = texture.views.depthAttachment;
            renderingDesc.depth = depthAttachmentDesc;
        }

        // TODO Stencil

        // TODO Consider if this should be placed automatically or not
        //NRI.CmdBeginRendering(commandBuffer, renderingDesc);
    }

    if (pass.execute) {
        pass.execute(commandBuffer, renderingDesc);
    }

    //if (hasAttachments) {
    //    NRI.CmdEndRendering(commandBuffer);
    //}
}

void FrameGraph::RecordPasses(
    ThreadContext& threadContext,
    Array<PerThreadQueueFrames, FreeListAllocator>& perPassThreadContexts,
    nri::DescriptorPool* descriptorPool,
    uint32 activeFrame,
    bool multithreadedRecording) {

    ASSERT(isCompiled);

    if (multithreadedRecording) {

        PerThreadQueueFrames& passThreadContext = perPassThreadContexts[threadContext.index];

        // TODO Switch to a nicer heuristic to determine how many passes each thread will take
        // perhaps the number should not be equal among all but instead be based on some workload
        // metric
        Range passRange = threadContext.UniformRange(passes.GetSize());

        for (uint32 i = passRange.min; i < passRange.max; ++i) {

            nri::CommandBuffer& commandBuffer = *passThreadContext.queuedFrames[activeFrame].commandBuffers[i - passRange.min];

            NRI.BeginCommandBuffer(commandBuffer, descriptorPool);

            PassDesc& pass = passes[i];

            // Store command buffer so it can be submitted later in the correct order
            // passes are already stored in the correct order, this simply ensures the commands are as well
            pass.commandBuffer = &commandBuffer;
            ExecutePass(commandBuffer, pass);

            // If this is the last pass emit final barriers!
            if (i == passes.GetSize() - 1) {
                EmitFinalBarriers(commandBuffer);
            }

            NRI.EndCommandBuffer(commandBuffer);
        }
    } else if (threadContext.IsMainThread()) {

        nri::CommandBuffer& commandBuffer = *perPassThreadContexts[0].queuedFrames[activeFrame].commandBuffers[0];

        NRI.BeginCommandBuffer(commandBuffer, descriptorPool);

        for (PassDesc& pass : passes) {
            ExecutePass(commandBuffer, pass);
        }

        EmitFinalBarriers(commandBuffer);

        NRI.EndCommandBuffer(commandBuffer);
    }
    threadContext.Sync();
}
