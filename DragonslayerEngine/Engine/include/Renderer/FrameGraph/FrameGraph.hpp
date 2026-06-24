#pragma once

#include <functional>

#include <Core/Containers/ArrayHelper.hpp>
#include <NRI.h>

#include "Core/Allocators/FreeListAllocator.hpp"
#include "Core/Containers/Array.hpp"
#include "Core/Macros.hpp"
#include "Core/String.hpp"
#include "Core/Types.hpp"

#include "Renderer/FrameGraph/Handles.hpp"
#include "Renderer/RendererCore.hpp"

/**
 * FrameGraph resource handles represented as an uint32
 * 0 is reserved to an invalid handle
 *
 * Stable identity for a resource managed by the graph. The graph owns the
 * registry that maps handles to underlying nri pointers, tracks per-resource
 * last-known state across passes, and (for temp resources) owns lifetime and
 * memory.
 */

#define INVALID_RESOURCE_HANDLE_ID 0

using ResourceID = uint32;

struct TextureHandle {

    ResourceID id = INVALID_RESOURCE_HANDLE_ID;

    static TextureHandle Invalid;

    NO_DISCARD FORCE_INLINE bool IsValid() const { return id != INVALID_RESOURCE_HANDLE_ID; }

    NO_DISCARD FORCE_INLINE bool operator==(TextureHandle textureHandle) const {
        return id == textureHandle.id;
    }

    NO_DISCARD FORCE_INLINE bool operator!=(TextureHandle textureHandle) const {
        return id != textureHandle.id;
    }
};

struct BufferHandle {

    ResourceID id = INVALID_RESOURCE_HANDLE_ID;

    static BufferHandle Invalid;

    NO_DISCARD FORCE_INLINE bool IsValid() const { return id != INVALID_RESOURCE_HANDLE_ID; }

    NO_DISCARD FORCE_INLINE bool operator==(BufferHandle textureHandle) const {
        return id == textureHandle.id;
    }

    NO_DISCARD FORCE_INLINE bool operator!=(BufferHandle textureHandle) const {
        return id != textureHandle.id;
    }
};

/**
 * Texture access on a render pass.
 * Must be unique per pass.
 */
struct TextureAccessDesc {
    TextureHandle textureHandle = TextureHandle::Invalid;
    nri::AccessLayoutStage state = {};
};

/**
 * Color or depth attachment binding for a graphics pass.
 */
struct TextureAttachmentDesc {
    TextureHandle textureHandle = TextureHandle::Invalid;
    nri::AttachmentDesc desc = {};
};


/*
 * Buffer access on a render pass
 * Must be unique per pass
 */
struct BufferAccessDesc {
    BufferHandle bufferHandle = BufferHandle::Invalid;
    nri::AccessStage state = {};
};


/**
 * Per-imported-texture set of views.
 *
 * NRI separates a texture from its views (descriptors). A pass might bind a
 * texture as a shader resource, render target, or depth attachment - each
 * needs a distinct descriptor. The graph carries a small bundle of views per
 * imported texture. Any view not provided is left null and any pass that
 * needs it will assert at execute time.
 *
 */
struct TextureViews {
    nri::Descriptor* shaderResource = nullptr;
    nri::Descriptor* colorAttachment = nullptr;
    nri::Descriptor* depthAttachment = nullptr;
};

/**
 * Per-imported-buffer view bundle. Buffers are simpler than textures in NRI
 * (no layout) but still need a descriptor for shader bindings in some cases.
 */
struct BufferViews {
    nri::Descriptor* shaderResource = nullptr;
    nri::Descriptor* shaderResourceStorage = nullptr;
    nri::Descriptor* constantBuffer = nullptr;
};

/**
 * Compiled barrier records.
 *
 * Compile bakes the access state transitions per pass. The actual nri::Texture*
 * / nri::Buffer* are resolved from the registry at Execute time so imports that
 * rotate per frame (swapchain backbuffer, ringbuffer slices) work correctly
 * with a single static graph.
 */
struct CompiledTextureBarrier {
    TextureHandle textureHandle = TextureHandle::Invalid;
    nri::AccessLayoutStage before = {};
    nri::AccessLayoutStage after = {};
};

struct CompiledBufferBarrier {
    BufferHandle bufferHandle = BufferHandle::Invalid;
    nri::AccessStage before = {};
    nri::AccessStage after = {};
};

/**
 * Represents a render pass
 */
struct PassDesc {
    NameString name;
    nri::QueueType queue;

    Array<TextureAccessDesc, FreeListAllocator> textureAccesses;
    Array<BufferAccessDesc, FreeListAllocator> bufferAccesses;

    Array<TextureAttachmentDesc, FreeListAllocator> colorAttachments;
    TextureAttachmentDesc depthAttachment;
    bool hasDepthAttachment = false; // TODO Consider implementing optional type

    // TODO Replace with something else that is not an std::function
    std::function<void(nri::CommandBuffer&, const nri::RenderingDesc&)> execute;

    Array<CompiledTextureBarrier, FreeListAllocator> compiledTextureBarriers;
    Array<CompiledBufferBarrier, FreeListAllocator> compiledBufferBarriers;

    nri::CommandBuffer* commandBuffer = nullptr;

    PassDesc(const NameString& name, nri::QueueType queueType, FreeListAllocator* allocator);
};

/**
 * Resource registry entries.
 *
 * Imported entries hold pointers owned externally; the graph never destroys
 * them. Temp entries are owned by the graph and destroyed in Invalidate().
 *
 * initialState: state assumed at the start of every frame. The graph emits a
 *   barrier from this state to whatever the first pass that touches the
 *   resource requires. Set to {NONE, UNDEFINED, NONE} for a "discard previous
 *   contents" semantic, which works regardless of actual GPU state.
 *
 * finalState: state required at the end of every frame. If non-default, the
 *   graph emits a closing barrier from currentState to finalState after all
 *   passes have run. For textures, layout == UNDEFINED means "no closing
 *   barrier required". For buffers, access == NONE && stages == NONE means
 *   the same.
 */
struct TextureRegistryEntry {
    NameString name; // TODO Should probably not be included in the release build
    nri::Texture* texture = nullptr;
    TextureViews views;
    nri::AccessLayoutStage initialState = {};
    nri::AccessLayoutStage finalState = {};
    nri::AccessLayoutStage currentState = {};
    bool isTemp = false;
};

struct BufferRegistryEntry {
    NameString name; // TODO Should probably not be included in the release build
    nri::Buffer* buffer = nullptr;
    BufferViews views;
    nri::AccessStage initialState = {};
    nri::AccessStage finalState = {};
    nri::AccessStage currentState = {};
    bool isTemp = false;
};

/**
 * The render frame graph.
 *
 * Lifetime model:
 * - Build once at startup (passes + temp resource declarations).
 * - Compile once: allocates temp memory, simulates execution forward, bakes
 *   per-pass barrier lists, and bakes a final list of closing barriers needed
 *   to bring imports back to their declared finalState.
 * - Execute every frame: walks the baked passes, emits barriers, runs lambdas,
 *   then emits the closing barriers.
 * - Invalidate on resize: destroys temps, clears passes; user rebuilds + recompiles.
 *
 * The graph does NOT do DAG (directed acyclic graph) resolution or scheduling; declaration order is
 * execution order. This is sufficient for the engine's fixed pipeline and
 * keeps the implementation small. Upgrading to a real DAG later does not
 * change the pass-authoring API.
 */
class FrameGraph {

private:
    const nri::CoreInterface& NRI;
    nri::Device& device;
    FreeListAllocator* allocator;

    Array<PassDesc, FreeListAllocator> passes;
    Array<TextureRegistryEntry, FreeListAllocator> textures;
    Array<BufferRegistryEntry, FreeListAllocator> buffers;

    // Closing barriers emitted once per frame after all passes have run.
    Array<CompiledTextureBarrier, FreeListAllocator> compiledClosingTextureBarriers;
    Array<CompiledBufferBarrier, FreeListAllocator> compiledClosingBufferBarriers;

    // The description we are currently modifying (during pass building)
    PassDesc* activeDesc = nullptr;

    bool isCompiled = false;

public:
    FrameGraph(const nri::CoreInterface& NRI, nri::Device& device, FreeListAllocator* allocator);
    ~FrameGraph();

    // Resource declaration ----------------------------------------------------

    TextureHandle ImportTexture(
        const NameString& name,
        nri::Texture* texture,
        const TextureViews& views,
        nri::AccessLayoutStage initialState,
        nri::AccessLayoutStage finalState = {});

    BufferHandle ImportBuffer(
        const NameString& name,
        nri::Buffer* buffer,
        const BufferViews& views,
        nri::AccessStage initialState,
        nri::AccessStage finalState = {});

    // TODO Temp resources
    // TextureHandle CreateTempTexture(const NameString& name, const nri::TextureDesc& desc, const TextureViews& views);
    // BufferHandle CreateTempBuffer(const NameString& name, const nri::BufferDesc& desc, const BufferViews& views);

    // Pass declaration --------------------------------------------------------

    NO_DISCARD FORCE_INLINE FrameGraph& BeginGraphicsPass(const NameString& name) {
        ASSERT(!isCompiled);
        ASSERT(activeDesc == nullptr, "Called begin pass without having called end!");
        activeDesc = &passes.Emplace(name, nri::QueueType::GRAPHICS, allocator);
        return *this;
    }
    NO_DISCARD FORCE_INLINE FrameGraph& BeginComputePass(const NameString& name) {
        ASSERT(!isCompiled);
        ASSERT(activeDesc == nullptr, "Called begin pass without having called end!");
        activeDesc = &passes.Emplace(name, nri::QueueType::COMPUTE, allocator);
        return *this;
    }
    NO_DISCARD FORCE_INLINE FrameGraph&  BeginCopyPass(const NameString& name) {
        ASSERT(!isCompiled);
        ASSERT(activeDesc == nullptr, "Called begin pass without having called end!");
        activeDesc = &passes.Emplace(name, nri::QueueType::COPY, allocator);
        return *this;
    }

    FORCE_INLINE void EndPass() {
        activeDesc = nullptr;
    }

    NO_DISCARD FrameGraph& Access(BufferHandle bufferHandle, nri::AccessBits access, nri::StageBits stages);
    NO_DISCARD FrameGraph& Access(TextureHandle textureHandle, nri::AccessBits access, nri::Layout layout, nri::StageBits stages);

    NO_DISCARD FrameGraph& SetColorAttachment(
        uint32 slot,
        TextureHandle textureHandle,
        nri::LoadOp loadOp,
        nri::StoreOp storeOp,
        const nri::Color32f& clearColor = nri::Color32f{});

    NO_DISCARD FrameGraph& SetDepthAttachment(
        TextureHandle textureHandle,
        nri::LoadOp loadOp,
        nri::StoreOp storeOp,
        float clearDepth = 1.f,
        uint8 clearStencil = 0.f);

    NO_DISCARD FORCE_INLINE FrameGraph& ReadsConstantBuffer(BufferHandle bufferHandle) {
        return Access(bufferHandle, nri::AccessBits::CONSTANT_BUFFER, nri::StageBits::ALL_SHADERS);
    }
    NO_DISCARD FORCE_INLINE FrameGraph& ReadsVertexBuffer(BufferHandle bufferHandle) {
        return Access(bufferHandle, nri::AccessBits::VERTEX_BUFFER, nri::StageBits::VERTEX_SHADER);
    }
    NO_DISCARD FORCE_INLINE FrameGraph& ReadsIndexBuffer(BufferHandle bufferHandle) {
        return Access(bufferHandle, nri::AccessBits::INDEX_BUFFER, nri::StageBits::INDEX_INPUT);
    }
    NO_DISCARD FORCE_INLINE FrameGraph& ReadsArgumentBuffer(BufferHandle bufferHandle) {
        return Access(bufferHandle, nri::AccessBits::ARGUMENT_BUFFER, nri::StageBits::INDIRECT);
    }
    NO_DISCARD FORCE_INLINE FrameGraph& ReadsTexture(TextureHandle textureHandle) {
        return Access(textureHandle, nri::AccessBits::SHADER_RESOURCE, nri::Layout::SHADER_RESOURCE, nri::StageBits::ALL_SHADERS);
    }
    NO_DISCARD FORCE_INLINE FrameGraph& ReadsStorage(TextureHandle textureHandle) {
        return Access(textureHandle, nri::AccessBits::SHADER_RESOURCE_STORAGE, nri::Layout::SHADER_RESOURCE_STORAGE, nri::StageBits::ALL_SHADERS);
    }
    NO_DISCARD FORCE_INLINE FrameGraph& ReadsStorage(BufferHandle bufferHandle) {
        return Access(bufferHandle, nri::AccessBits::SHADER_RESOURCE_STORAGE, nri::StageBits::ALL_SHADERS);
    }
    NO_DISCARD FORCE_INLINE FrameGraph& WritesStorage(TextureHandle textureHandle) {
        return Access(textureHandle, nri::AccessBits::SHADER_RESOURCE_STORAGE, nri::Layout::SHADER_RESOURCE_STORAGE, nri::StageBits::ALL_SHADERS);
    }
    NO_DISCARD FORCE_INLINE FrameGraph& WritesStorage(BufferHandle bufferHandle) {
        return Access(bufferHandle, nri::AccessBits::SHADER_RESOURCE_STORAGE, nri::StageBits::ALL_SHADERS);
    }

    NO_DISCARD FORCE_INLINE FrameGraph& Execute(std::function<void(nri::CommandBuffer&, const nri::RenderingDesc&)> passFunction) {
        ASSERT(activeDesc != nullptr);
        activeDesc->execute = std::move(passFunction);
        return *this;
    }

    // Replace the underlying pointer of an imported resource (e.g. swapchain
    // backbuffer rotation, ringbuffer slice). The declared initial state is
    // assumed to match the new pointer; the user must guarantee that.
    void SetImport(TextureHandle textureHandle, nri::Texture* texture, const TextureViews& views);
    void SetImport(BufferHandle bufferHandle, nri::Buffer* buffer, const BufferViews& views);

    void Compile();

    void RecordPasses(
        ThreadContext& threadContext,
        Array<PerThreadQueueFrames, FreeListAllocator>& perPassThreadContexts,
        nri::DescriptorPool* descriptorPool,
        uint32 activeFrame,
        bool multithreadedRecording);

    void Invalidate();

    void GetCommandBuffers(TempArray<nri::CommandBuffer*>& commandBuffers);

    NO_DISCARD FORCE_INLINE bool IsCompiled() const { return isCompiled; }
    NO_DISCARD FORCE_INLINE uint32 GetPassCount() const { return passes.GetSize(); }

private:
    NO_DISCARD FORCE_INLINE TextureRegistryEntry& GetTexture(TextureHandle textureHandle) {
        ASSERT(textureHandle.IsValid());
        ASSERT(textureHandle.id <= textures.GetSize());
        return textures[textureHandle.id - 1];
    }
    NO_DISCARD FORCE_INLINE BufferRegistryEntry& GetBuffer(BufferHandle bufferHandle) {
        ASSERT(bufferHandle.IsValid());
        ASSERT(bufferHandle.id <= buffers.GetSize());
        return buffers[bufferHandle.id - 1];
    }
    NO_DISCARD FORCE_INLINE const TextureRegistryEntry& GetTexture(TextureHandle textureHandle) const {
        ASSERT(textureHandle.IsValid());
        ASSERT(textureHandle.id <= textures.GetSize());
        return textures[textureHandle.id - 1];
    }
    NO_DISCARD FORCE_INLINE const BufferRegistryEntry& GetBuffer(BufferHandle bufferHandle) const {
        ASSERT(bufferHandle.IsValid());
        ASSERT(bufferHandle.id <= buffers.GetSize());
        return buffers[bufferHandle.id - 1];
    }

    void ExecutePass(nri::CommandBuffer& commandBuffer, const PassDesc& pass);

    void EmitBarriers(
        nri::CommandBuffer& commandBuffer,
        const Array<CompiledTextureBarrier, FreeListAllocator>& compiledTextureBarriers,
        const Array<CompiledBufferBarrier, FreeListAllocator>& compiledBufferBarriers);
    FORCE_INLINE void EmitFinalBarriers(nri::CommandBuffer& commandBuffer) {
        EmitBarriers(commandBuffer, compiledClosingTextureBarriers, compiledClosingBufferBarriers);
    }
};
