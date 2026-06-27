#include "Renderer/RenderingSystem.hpp"

#include <algorithm>
#include <thread>

#include "Core/Containers/ArrayHelper.hpp"
#include "Core/EngineGlobals.hpp"
#include "Core/String.hpp"
#include "Core/Window.hpp"
#include "Renderer/BasicPrimitives.hpp"
#include "Renderer/Components/PrimitiveRenderer.hpp"
#include "Renderer/NRIWindowCreation.hpp"
#include "Runtime/Components/Transform.hpp"

#include "Editor/TimeScope.hpp"
#if WITH_EDITOR
#include <imgui.h>
#include "Editor/EditorGlobals.hpp"
#include "Editor/ImGuiSetup.hpp"
#endif

#include "Physics/Components/CharacterController.hpp"
#include "Physics/Components/DynamicRigidBody.hpp"
#include "Renderer/Shader.hpp"
#include "Renderer/Texture.hpp"
#include "Runtime/Components/PhysicsNPC.hpp"
#include "Runtime/Engine.hpp"

// Settings
constexpr nri::VKBindingOffsets VK_BINDING_OFFSETS = {0, 128, 32, 64}; // see CMake
constexpr bool D3D12_DISABLE_ENHANCED_BARRIERS = false;

constexpr uint32 GLOBAL_DESCRIPTOR_SET = 0;
constexpr uint32 SHADOWS_DESCRIPTOR_SET = 1;
constexpr float CLEAR_DEPTH = 1.0f;
constexpr nri::Color32f CLEAR_COLOR = {0.0f, 0.63f, 1.0f };

constexpr uint32 CONSTANT_BUFFER = 0;
constexpr uint32 READBACK_BUFFER = 1;
constexpr uint32 MESH_INDEX_BUFFER = 2;
constexpr uint32 MESH_VERTEX_BUFFER = 3;
constexpr uint32 MESH_DESC_BUFFER = 4;
constexpr uint32 INSTANCE_BUFFER = 5;       // per-frame Instance[] — streamer dst-copies into this
constexpr uint32 INDIRECT_ARGS_BUFFER = 6;  // per-PSO bucket draw args, GPU-written by GenerateDrawCommandsCS
constexpr uint32 DRAW_COUNT_BUFFER = 7;     // per-PSO bucket draw count, atomic-incremented by GenerateDrawCommandsCS
constexpr uint32 NUM_DEVICE_BUFFERS = 6;    // MESH_INDEX..DRAW_COUNT (all DEVICE-memory)
constexpr uint32 DEPTH_TEXTURE = 0;
constexpr uint32 SHADOW_MAP_TEXTURE = 1;

// GPU-driven draw generation sizing.
constexpr uint32 NUM_PSO_BUCKETS = ShaderBucket::Count;            // opaque, transparent, shadow
constexpr uint32 MAX_DRAWS_PER_BUCKET = MAX_INSTANCES;             // worst case: every instance lands in one bucket
// Indirect-arg stride depends on whether the bindless VS uses NRI's drawParameters emulation:
//   - Native (VK, or D3D12 with SM 6.8+):  nri::DrawIndexedDesc       — 5 uints / 20 bytes.
//   - Emulation (D3D12 with SM < 6.8):     nri::DrawIndexedBaseDesc   — 7 uints / 28 bytes.
// GenerateDrawCommands.cs.hlsl always defines NRI_ENABLE_DRAW_PARAMETERS_EMULATION; NRI.hlsl honors it
// only on DXIL (where it rewrites NRI_FILL_DRAW_INDEXED_DESC to the BASE variant). On SPIRV
// the FILL macro stays at 5 uints regardless, so on VK we must use the 20-byte stride.
constexpr uint32 DRAW_INDEXED_DESC_UINTS_NATIVE    = 5;
constexpr uint32 DRAW_INDEXED_DESC_UINTS_EMULATION = 7;
constexpr uint32 DRAW_INDEXED_DESC_SIZE_NATIVE     = DRAW_INDEXED_DESC_UINTS_NATIVE    * sizeof(uint32);
constexpr uint32 DRAW_INDEXED_DESC_SIZE_EMULATION  = DRAW_INDEXED_DESC_UINTS_EMULATION * sizeof(uint32);
constexpr uint32 DRAW_INDEXED_DESC_SIZE_MAX        = DRAW_INDEXED_DESC_SIZE_EMULATION; // for static buffer sizing
constexpr uint32 BUILD_DRAWS_GROUP_SIZE  = 64;                     // must match [numthreads(64,1,1)] in GenerateDrawCommands.cs.hlsl

// CPU-side mesh archive (offsets into shared VB/IB).
// Index by PrimitiveType. Populated during InitBuffers, consumed by the per-pass draw loop.
static MeshDesc gMeshDescs[MAX_MESHES] = {};

// Compute root constants for GenerateDrawCommandsCS — must mirror the HLSL struct in GenerateDrawCommands.cs.hlsl.
struct BuildDrawsConstants {
    uint32 instanceCount;
    uint32 passShaderID;
    uint32 passFlagMask;       // 0 = accept any instance (bucket filter only)
    uint32 maxDrawsPerBucket;
};

constexpr uint32 VIEW_MASK = 0b11;
constexpr nri::Color32f COLOR_0 = {1.0f, 1.0f, 0.0f, 1.0f};
constexpr nri::Color32f COLOR_1 = {0.46f, 0.72f, 0.0f, 1.0f};

constexpr uint32 OPAQUE_PIPELINE = 0;
constexpr uint32 DISCARD_PIPELINE = 1;
constexpr uint32 TRANSPARENT_PIPELINE = 2;
constexpr uint32 SHADOW_PIPELINE = 3;
constexpr uint32 GENERATE_DRAW_COMMANDS_PIPELINE = 4;

constexpr nri::Dim_t SHADOW_RESOLUTION_WIDTH = 4096;
constexpr float SHADOW_CONSTANT_DEPTH_BIAS = 1.25f;
constexpr float SHADOW_SLOPE_DEPTH_BIAS = 1.75f;
constexpr float SHADOW_PROJECTION_SIZE = 150.f;
constexpr float SHADOW_FAR_PLANE = 100.f;

static uint32 windowWidth, windowHeight;

RenderingSystem::RenderingSystem(const RendererSettings& settings) :
    settings(settings),
    allocator(*gGlobalAllocator, settings.usedMemory),
    perThreadQueuedFrames(1 /* The actual amount of threads will be known during start! */, &allocator),
    pipelines(4, &allocator),
    swapChainTextures(1, &allocator),
    descriptorSets(1, &allocator),
    textures(8, &allocator),
    buffers(8, &allocator),
    descriptors(8, &allocator),
    memoryAllocations(8, &allocator),
    instances(MAX_INSTANCES, &allocator) {

    gCamera = &defaultCamera;
    glfwWindow = gWindow->GetGLFWWindow();
}

void RenderingSystem::Initialize(ThreadContext& threadContext) {
    if (threadContext.IsMainThread()) {
        perThreadQueuedFrames.EmplaceMany(threadContext.count);

        InitNRICallbacks();
        InitDevice();
        InitSwapChain(*gWindow);
        CreateCommandAllocators();
        CreateTextures();
        InitPipelineLayout();
        InitPipelines();
        InitBuffers();
        InitMemory();
        InitDescriptors();
        UploadGPUData();
        InitQueryPool();

#if WITH_EDITOR
        ASSERT(InitImguiRenderer(*device));
#endif

        // FrameGraph must be constructed after InitDevice (needs NRI + device populated)
        // and after InitDescriptors (needs the texture views to import).
        frameGraph = allocator.Allocate<FrameGraph>(static_cast<const nri::CoreInterface&>(NRI), *device, &allocator);

        BuildFrameGraph();
        frameGraph->Compile();

        CreateCommandBuffers(frameGraph->GetPassCount());
    }
    threadContext.Sync();
}

RenderingSystem::~RenderingSystem() {

    if (NRI.HasCore()) {
        NRI.DeviceWaitIdle(device);

        // Destroy the FrameGraph BEFORE tearing down NRI resources it references.
        // The graph itself owns no NRI resources (Phase 0a) but holds raw pointers.
        if (frameGraph) {
            allocator.Free(frameGraph);
            frameGraph = nullptr;
        }

        for (PerThreadQueueFrames& threadContext: perThreadQueuedFrames) {
            for (QueuedFrame& queuedFrame : threadContext.queuedFrames) {
                for (nri::CommandBuffer* commandBuffer: queuedFrame.commandBuffers) {
                    NRI.DestroyCommandBuffer(commandBuffer);
                }
                NRI.DestroyCommandAllocator(queuedFrame.commandAllocator);
            }
        }

        for (SwapChainTexture& swapChainTexture: swapChainTextures) {
            NRI.DestroyFence(swapChainTexture.acquireSemaphore);
            NRI.DestroyFence(swapChainTexture.releaseSemaphore);
            NRI.DestroyDescriptor(swapChainTexture.colorAttachment);
        }

        for (nri::Descriptor* descriptor: descriptors) {
            NRI.DestroyDescriptor(descriptor);
        }

        NRI.DestroyDescriptor(depthAttachment);

        for (nri::Texture* texture: textures) {
            NRI.DestroyTexture(texture);
        }

        for (nri::Buffer* buffer: buffers) {
            NRI.DestroyBuffer(buffer);
        }

        for (nri::Memory* memoryAllocation: memoryAllocations) {
            NRI.FreeMemory(memoryAllocation);
        }

        NRI.FreeMemory(depthTextureMemory);

        for (nri::Pipeline* pipeline: pipelines) {
            NRI.DestroyPipeline(pipeline);
        }

        NRI.DestroyQueryPool(queryPool);
        NRI.DestroyPipelineLayout(generateDrawCommandsLayout);
        NRI.DestroyPipelineLayout(pipelineLayout);
        NRI.DestroyDescriptorPool(descriptorPool);
        NRI.DestroyFence(frameFence);
    }

    if (NRI.HasSwapChain())
        NRI.DestroySwapChain(swapChain);

    if (NRI.HasStreamer()) {
#if WITH_EDITOR
        NRI.DestroyStreamer(imguiStreamer);
#endif
        NRI.DestroyStreamer(gameStreamer);
    }

#if WITH_EDITOR
    DestroyImguiRenderer();
#endif

    nri::nriDestroyDevice(device);
}

void RenderingSystem::InitNRICallbacks() {
    allocationCallbacks.Allocate = NRIAllocate;
    allocationCallbacks.Reallocate = NRIReallocate;
    allocationCallbacks.Free = NRIFree;
    allocationCallbacks.userArg = &allocator;
    allocationCallbacks.disable3rdPartyAllocationCallbacks = false;

    messageCallbacks.MessageCallback = NRIMessage;
    messageCallbacks.AbortExecution = NRIAbortExecution;
    messageCallbacks.userArg = nullptr;
}

void RenderingSystem::InitDevice() {
    // Adapters
    nri::AdapterDesc adapterDesc[2] = {};
    uint32 adapterDescsNum = helper::GetCountOf(adapterDesc);
    NRI_ABORT_ON_FAILURE(nri::nriEnumerateAdapters(adapterDesc, adapterDescsNum));

    // Device
    nri::DeviceCreationDesc deviceCreationDesc = {
        .graphicsAPI = settings.graphicsAPI,
        .adapterDesc = &adapterDesc[std::min(adapterIndex, adapterDescsNum - 1)],
        .callbackInterface = messageCallbacks,
        .allocationCallbacks = allocationCallbacks,
        .vkBindingOffsets = VK_BINDING_OFFSETS,
        .enableNRIValidation = settings.debugNRI,
        .enableGraphicsAPIValidation = settings.debugAPI,
        .disableD3D12EnhancedBarriers = D3D12_DISABLE_ENHANCED_BARRIERS,
    };
    NRI_ABORT_ON_FAILURE(nri::nriCreateDevice(deviceCreationDesc, device));

    // NRI
    NRI_ABORT_ON_FAILURE(nri::nriGetInterface(*device, NRI_INTERFACE(nri::CoreInterface), (nri::CoreInterface*) &NRI));
    NRI_ABORT_ON_FAILURE(nri::nriGetInterface(*device, NRI_INTERFACE(nri::HelperInterface), (nri::HelperInterface*) &NRI));
    NRI_ABORT_ON_FAILURE(nri::nriGetInterface(*device, NRI_INTERFACE(nri::StreamerInterface), (nri::StreamerInterface*) &NRI));
    NRI_ABORT_ON_FAILURE(nri::nriGetInterface(*device, NRI_INTERFACE(nri::SwapChainInterface), (nri::SwapChainInterface*) &NRI));

    const nri::DeviceDesc& deviceDesc = NRI.GetDeviceDesc(*device);
    if (!deviceDesc.tiers.bindless) {
        FAIL("Bindless is not supported!");
    }

    if (!deviceDesc.shaderFeatures.drawParameters && !deviceDesc.shaderFeatures.drawParametersEmulation) {
        FAIL("Draw parameters are not supported!");
    }

    // Create imgui streamer
#if WITH_EDITOR
    nri::StreamerDesc imguiStreamerDesc = {
        .constantBufferMemoryLocation = nri::MemoryLocation::HOST_UPLOAD,
        .dynamicBufferMemoryLocation = nri::MemoryLocation::HOST_UPLOAD,
        .dynamicBufferDesc = {0, 0, nri::BufferUsageBits::VERTEX_BUFFER | nri::BufferUsageBits::INDEX_BUFFER},
        .queuedFrameNum = GetQueuedFrameNum(),
    };
    NRI_ABORT_ON_FAILURE(NRI.CreateStreamer(*device, imguiStreamerDesc, imguiStreamer));
#endif


    // Device upload will be slightly faster here and will default to host upload
    // anyway when not supported
    nri::StreamerDesc gameStreamerDesc = {
        .constantBufferMemoryLocation = nri::MemoryLocation::DEVICE_UPLOAD,
        .dynamicBufferMemoryLocation = nri::MemoryLocation::DEVICE_UPLOAD,
        .dynamicBufferDesc = {0, 0, nri::BufferUsageBits::SHADER_RESOURCE},
        .queuedFrameNum = GetQueuedFrameNum(),
    };
    NRI_ABORT_ON_FAILURE(NRI.CreateStreamer(*device, gameStreamerDesc, gameStreamer));

    // Command queue
    NRI_ABORT_ON_FAILURE(NRI.GetQueue(*device, nri::QueueType::GRAPHICS, 0, graphicsQueue));

    // Fences
    NRI_ABORT_ON_FAILURE(NRI.CreateFence(*device, 0, frameFence));

    depthFormat = nri::GetSupportedDepthFormat(NRI, *device, 32, false);
}

void RenderingSystem::InitSwapChain(const Window& window) {
    window.GetSize(windowWidth, windowHeight);

    const nri::SwapChainDesc swapChainDesc = {
        .window = CreateNRIWindow(gWindow->GetGLFWWindow()),
        .queue = graphicsQueue,
        .width = static_cast<uint16>(windowWidth),
        .height = static_cast<uint16>(windowHeight),
        .textureNum = GetOptimalSwapChainTextureNum(),
        .format = nri::SwapChainFormat::BT709_G22_10BIT,
        .flags = (settings.enableVSync ? nri::SwapChainBits::VSYNC : nri::SwapChainBits::NONE) | nri::SwapChainBits::ALLOW_TEARING,
        .queuedFrameNum = GetQueuedFrameNum(),
    };
    NRI_ABORT_ON_FAILURE(NRI.CreateSwapChain(*device, swapChainDesc, swapChain));

    uint32 swapChainTextureNum;
    nri::Texture* const* localSwapChainTextures = NRI.GetSwapChainTextures(*swapChain, swapChainTextureNum);
    nri::Format swapChainFormat = NRI.GetTextureDesc(*localSwapChainTextures[0]).format;

    CreateSwapChainTextures(localSwapChainTextures, swapChainTextureNum, swapChainFormat);
}

void RenderingSystem::CreateCommandAllocators() {

    for (uint32 i = 0; i < perThreadQueuedFrames.GetSize(); ++i) {
        PerThreadQueueFrames& threadContext = perThreadQueuedFrames[i];
        threadContext.queuedFrames.EmplaceMany(GetQueuedFrameNum());
        for (QueuedFrame& queuedFrame : threadContext.queuedFrames) {
            NRI_ABORT_ON_FAILURE(NRI.CreateCommandAllocator(*graphicsQueue, queuedFrame.commandAllocator));
        }
    }
}

void RenderingSystem::CreateCommandBuffers(uint32 maxPassesPerThread) {

    ASSERT(maxPassesPerThread <= MAX_COMMAND_BUFFERS_PER_QUEUED_FRAME);
    ASSERT(maxPassesPerThread <= perThreadQueuedFrames.GetSize());

    for (PerThreadQueueFrames& threadContext : perThreadQueuedFrames) {
        for (QueuedFrame& queuedFrame : threadContext.queuedFrames) {
            queuedFrame.commandBuffers.EmplaceMany(maxPassesPerThread);
            for (uint32 i = 0; i < maxPassesPerThread; ++i) {
                NRI_ABORT_ON_FAILURE(NRI.CreateCommandBuffer(*queuedFrame.commandAllocator, queuedFrame.commandBuffers[i]));
            }
        }
    }
}

void RenderingSystem::CreateTextures() {

    // Depth attachment
    {
        const nri::TextureDesc textureDesc = {
            .type = nri::TextureType::TEXTURE_2D,
            .usage = nri::TextureUsageBits::DEPTH_STENCIL_ATTACHMENT,
            .format = depthFormat,
            .width = static_cast<nri::Dim_t>(windowWidth),
            .height = static_cast<nri::Dim_t>(windowHeight),
            .mipNum = 1,
        };
        nri::Texture* texture;
        NRI_ABORT_ON_FAILURE(NRI.CreateTexture(*device, textureDesc, texture));
        textures.Add(texture);
    }

    // Shadow Map
    {
        const nri::TextureDesc textureDesc = {
            .type = nri::TextureType::TEXTURE_2D,
            .usage = nri::TextureUsageBits::DEPTH_STENCIL_ATTACHMENT | nri::TextureUsageBits::SHADER_RESOURCE,
            .format = depthFormat,
            .width = SHADOW_RESOLUTION_WIDTH,
            .height = SHADOW_RESOLUTION_WIDTH,
            .mipNum = 1,
        };
        nri::Texture* texture;
        NRI_ABORT_ON_FAILURE(NRI.CreateTexture(*device, textureDesc, texture));
        textures.Add(texture);
    }
}

void RenderingSystem::InitPipelineLayout() {
    const nri::DeviceDesc& deviceDesc = NRI.GetDeviceDesc(*device);


    // -------- Graphics layout (Shadow / Forward / Transparent) --------
    // Set 0 (global): CBV at b0 + bindless SRVs at t0..t1 (Instance[] and MeshDesc[]).
    // Set 1 (shadow): sampler + shadow-map texture.
    // No per-draw root constants — transforms / color come from the Instance SRV.
    {
        nri::DescriptorRangeDesc globalDescriptorRange[] = {
            {0, 1, nri::DescriptorType::CONSTANT_BUFFER,   nri::StageBits::ALL},
            {0, 2, nri::DescriptorType::STRUCTURED_BUFFER, nri::StageBits::VERTEX_SHADER | nri::StageBits::FRAGMENT_SHADER}
        };

        nri::DescriptorRangeDesc shadowDescriptorRange[] = {
            {0, 1, nri::DescriptorType::SAMPLER, nri::StageBits::FRAGMENT_SHADER},
            {0, 1, nri::DescriptorType::TEXTURE, nri::StageBits::FRAGMENT_SHADER}
        };

        nri::DescriptorSetDesc descriptorSetDescs[] = {
            {0, globalDescriptorRange, helper::GetCountOf(globalDescriptorRange)},
            {1, shadowDescriptorRange, helper::GetCountOf(shadowDescriptorRange)},
        };

        nri::PipelineLayoutDesc pipelineLayoutDesc = {
            .rootRegisterSpace = 0,
            .descriptorSets = descriptorSetDescs,
            .descriptorSetNum = helper::GetCountOf(descriptorSetDescs),
            .shaderStages = nri::StageBits::VERTEX_SHADER | nri::StageBits::FRAGMENT_SHADER,
        };

        // VK / SM 6.8+ D3D12: native drawParameters, thus no flag, shader uses SV_StartInstanceLocation directly
        // D3D12 SM < 6.8: emulation, thus NRI injects a hidden root constant per draw
        if (deviceDesc.shaderFeatures.drawParametersEmulation) {
            pipelineLayoutDesc.flags = nri::PipelineLayoutBits::ENABLE_DRAW_PARAMETERS_EMULATION;
        }

        NRI_ABORT_ON_FAILURE(NRI.CreatePipelineLayout(*device, pipelineLayoutDesc, pipelineLayout));
    }

    // -------- Compute layout (GenerateDrawCommandsCS) --------
    // Set 0 layout matches GenerateDrawCommands.cs.hlsl:
    //   t0: StructuredBuffer<Instance>     (gInstances)
    //   t1: StructuredBuffer<MeshDesc>     (gMeshDescs)
    //   u0: RWByteAddressBuffer            (gIndirectArgs)
    //   u1: RWStructuredBuffer<uint>       (gDrawCount)
    // Root constants at b0: BuildDrawsConstants.
    // Note: RWByteAddressBuffer and RWStructuredBuffer share DescriptorType
    // STORAGE_STRUCTURED_BUFFER per the NRI BufferView enum docs in NRIDescs.h.
    {
        nri::DescriptorRangeDesc descriptorRange[] = {
            {0, 2, nri::DescriptorType::STRUCTURED_BUFFER, nri::StageBits::COMPUTE_SHADER},
            {0, 2, nri::DescriptorType::STORAGE_STRUCTURED_BUFFER, nri::StageBits::COMPUTE_SHADER},
        };
        nri::DescriptorSetDesc descriptorSetDescs[] = {
            {0, descriptorRange, helper::GetCountOf(descriptorRange)},
        };
        nri::RootConstantDesc rootConstants[] = {
            {0, sizeof(BuildDrawsConstants), nri::StageBits::COMPUTE_SHADER},
        };
        nri::PipelineLayoutDesc layoutDesc = {
            .rootRegisterSpace = 0,
            .rootConstants = rootConstants,
            .rootConstantNum = helper::GetCountOf(rootConstants),
            .descriptorSets = descriptorSetDescs,
            .descriptorSetNum = helper::GetCountOf(descriptorSetDescs),
            .shaderStages = nri::StageBits::COMPUTE_SHADER,
        };
        NRI_ABORT_ON_FAILURE(NRI.CreatePipelineLayout(*device, layoutDesc, generateDrawCommandsLayout));
    }
}

void RenderingSystem::InitPipelines() {
    const nri::DeviceDesc& deviceDesc = NRI.GetDeviceDesc(*device);
    const nri::Format swapChainFormat = swapChainTextures[0].attachmentFormat;
    ShaderCodeStorage shaderCodeStorage = ShaderCodeStorage(128, &allocator);

    nri::VertexStreamDesc vertexStreamDesc = {.bindingSlot = 0};

    nri::VertexAttributeDesc vertexAttributeDesc[2] = {};
    {
        vertexAttributeDesc[0].format = nri::Format::RGB32_SFLOAT;
        vertexAttributeDesc[0].streamIndex = 0;
        vertexAttributeDesc[0].offset = helper::GetOffsetOf(&VertexNormal::position);;
        vertexAttributeDesc[0].d3d = {"POSITION", 0};
        vertexAttributeDesc[0].vk.location = {0};

        vertexAttributeDesc[1].format = nri::Format::RGB32_SFLOAT;
        vertexAttributeDesc[1].streamIndex = 0;
        vertexAttributeDesc[1].offset = helper::GetOffsetOf(&VertexNormal::normal);
        vertexAttributeDesc[1].d3d = {"NORMAL", 0};
        vertexAttributeDesc[1].vk.location = {1};
    }

    nri::VertexInputDesc vertexInputDesc = {
        .attributes = vertexAttributeDesc,
        .attributeNum = static_cast<uint8>(helper::GetCountOf(vertexAttributeDesc)),
        .streams = &vertexStreamDesc,
        .streamNum = 1,
    };

    nri::InputAssemblyDesc inputAssemblyDesc = {
        .topology = nri::Topology::TRIANGLE_LIST
    };

    nri::RasterizationDesc rasterizationDesc = {
        .fillMode = nri::FillMode::SOLID,
        .cullMode = nri::CullMode::BACK,
        .frontCounterClockwise = true,
    };

    //nri::MultisampleDesc multisampleDesc = {
    //    .sampleMask = nri::ALL,
    //    .sampleNum = 1,
    //    .sampleLocations = deviceDesc.tiers.sampleLocations >= 2,
    //};

    nri::ColorAttachmentDesc colorAttachmentDesc = {
        .format = swapChainFormat,
        .colorWriteMask = nri::ColorWriteBits::RGBA,
    };

    nri::OutputMergerDesc outputMergerDesc = {
        .colors = &colorAttachmentDesc,
        .colorNum = 1,
        .depth = {
            .compareOp = CLEAR_DEPTH == 1.0f ? nri::CompareOp::LESS : nri::CompareOp::GREATER,
            .write = true
        },
        .depthStencilFormat = depthFormat,
    };

    nri::ShaderDesc shaderStages[] = {
        LoadShader(deviceDesc.graphicsAPI, "Forward.vs", shaderCodeStorage),
        LoadShader(deviceDesc.graphicsAPI, "Forward.fs", shaderCodeStorage),
    };

    nri::GraphicsPipelineDesc graphicsPipelineDesc = {
        .pipelineLayout = pipelineLayout,
        .vertexInput = &vertexInputDesc,
        .inputAssembly = inputAssemblyDesc,
        .rasterization = rasterizationDesc,
        //.multisample = &multisampleDesc,
        .outputMerger = outputMergerDesc,
        .shaders = shaderStages,
        .shaderNum = helper::GetCountOf(shaderStages),
    };

    nri::Pipeline* pipeline;

    // Opaque
    {
        NRI_ABORT_ON_FAILURE(NRI.CreateGraphicsPipeline(*device, graphicsPipelineDesc, pipeline));
        pipelines.Add(pipeline);
    }

    // Alpha opaque
    {
        shaderStages[1] = LoadShader(deviceDesc.graphicsAPI, "ForwardDiscard.fs", shaderCodeStorage);

        rasterizationDesc.cullMode = nri::CullMode::NONE;
        outputMergerDesc.depth.write = true;
        colorAttachmentDesc.blendEnabled = false;
        NRI_ABORT_ON_FAILURE(NRI.CreateGraphicsPipeline(*device, graphicsPipelineDesc, pipeline));
        pipelines.Add(pipeline);
    }

    // Transparent
    {
        shaderStages[1] = LoadShader(deviceDesc.graphicsAPI, "ForwardTransparent.fs", shaderCodeStorage);
        rasterizationDesc.cullMode = nri::CullMode::NONE;
        outputMergerDesc.depth.write = false;
        colorAttachmentDesc.blendEnabled = true;
        colorAttachmentDesc.colorBlend = {nri::BlendFactor::SRC_ALPHA, nri::BlendFactor::ONE_MINUS_SRC_ALPHA, nri::BlendOp::ADD};
        NRI_ABORT_ON_FAILURE(NRI.CreateGraphicsPipeline(*device, graphicsPipelineDesc, pipeline));
        pipelines.Add(pipeline);
    }

    // Shadow
    {
        nri::ShaderDesc shadowShaderStages[] = {
            LoadShader(deviceDesc.graphicsAPI, "ForwardShadow.vs", shaderCodeStorage),
        };
        graphicsPipelineDesc.shaders = shadowShaderStages;
        graphicsPipelineDesc.shaderNum = 1;

        nri::VertexAttributeDesc shadowVertexAttributeDesc[1] = {};
        {
            shadowVertexAttributeDesc[0].format = nri::Format::RGB32_SFLOAT;
            shadowVertexAttributeDesc[0].streamIndex = 0;
            shadowVertexAttributeDesc[0].offset = helper::GetOffsetOf(&VertexNormal::position);;
            shadowVertexAttributeDesc[0].d3d = {"POSITION", 0};
            shadowVertexAttributeDesc[0].vk.location = {0};
        }
        nri::VertexInputDesc shadowVertexInputDesc = {
            .attributes = shadowVertexAttributeDesc,
            .attributeNum = static_cast<uint8>(helper::GetCountOf(shadowVertexAttributeDesc)),
            .streams = &vertexStreamDesc,
            .streamNum = 1,
        };
        graphicsPipelineDesc.vertexInput = &shadowVertexInputDesc;

        rasterizationDesc.depthBias = {.constant = SHADOW_CONSTANT_DEPTH_BIAS, .slope = SHADOW_SLOPE_DEPTH_BIAS};
        rasterizationDesc.cullMode = nri::CullMode::NONE;
        graphicsPipelineDesc.rasterization = rasterizationDesc;

        outputMergerDesc.depth.write = true;
        outputMergerDesc.colors = nullptr;
        outputMergerDesc.colorNum = 0;
        graphicsPipelineDesc.outputMerger = outputMergerDesc;

        colorAttachmentDesc.colorWriteMask = nri::ColorWriteBits::NONE;

        NRI_ABORT_ON_FAILURE(NRI.CreateGraphicsPipeline(*device, graphicsPipelineDesc, pipeline));
        pipelines.Add(pipeline);
    }

    // Generate draw commands
    {
        nri::ComputePipelineDesc pipelineDesc = {
            .pipelineLayout = generateDrawCommandsLayout,
            .shader = LoadShader(deviceDesc.graphicsAPI, "GenerateDrawCommands.cs", shaderCodeStorage),
        };
        NRI_ABORT_ON_FAILURE(NRI.CreateComputePipeline(*device, pipelineDesc, pipeline));
        pipelines.Add(pipeline);
    }
}

// Concatenated mesh archive sizes — must match the order written in UploadGPUData
struct MeshSource {
    const VertexNormal* vertices;
    uint32 vertexCount;
    const uint32* indices;
    uint32 indexCount;
};

static void GatherMeshSources(MeshSource (&out)[(uint32)PrimitiveType::Capsule + 1]) {
    out[(uint32)PrimitiveType::Plane] = {gPlaneVertexData,         helper::GetCountOf(gPlaneVertexData),         gPlaneIndexData,         helper::GetCountOf(gPlaneIndexData)};
    out[(uint32)PrimitiveType::Cube] = {gCubeVertexData,          helper::GetCountOf(gCubeVertexData),          gCubeIndexData,          helper::GetCountOf(gCubeIndexData)};
    out[(uint32)PrimitiveType::Sphere] = {gSphereMesh.vertices,     helper::GetCountOf(gSphereMesh.vertices),     gSphereMesh.indices,     helper::GetCountOf(gSphereMesh.indices)};
    out[(uint32)PrimitiveType::Cylinder] = {gCylinderMesh.vertices,   helper::GetCountOf(gCylinderMesh.vertices),   gCylinderMesh.indices,   helper::GetCountOf(gCylinderMesh.indices)};
    out[(uint32)PrimitiveType::Capsule] = {gCapsuleMesh.vertices,    helper::GetCountOf(gCapsuleMesh.vertices),    gCapsuleMesh.indices,    helper::GetCountOf(gCapsuleMesh.indices)};
}

void RenderingSystem::InitBuffers() {
    const nri::DeviceDesc& deviceDesc = NRI.GetDeviceDesc(*device);
    const uint32 constantBufferSize = helper::Align(static_cast<uint32>(sizeof(GlobalConstantBufferLayout)), deviceDesc.memoryAlignment.constantBufferOffset);

    nri::BufferDesc bufferDesc = {};
    nri::Buffer* buffer;

    // CONSTANT_BUFFER
    bufferDesc.size = constantBufferSize * GetQueuedFrameNum();
    bufferDesc.usage = nri::BufferUsageBits::CONSTANT_BUFFER;
    NRI_ABORT_ON_FAILURE(NRI.CreateBuffer(*device, bufferDesc, buffer));
    buffers.Add(buffer);

    // READBACK_BUFFER
    bufferDesc.size = sizeof(nri::PipelineStatisticsDesc);
    bufferDesc.usage = nri::BufferUsageBits::NONE;
    NRI_ABORT_ON_FAILURE(NRI.CreateBuffer(*device, bufferDesc, buffer));
    buffers.Add(buffer);

    // Shared mesh archive: walk all primitives once to compute offsets and totals.
    MeshSource sources[(uint32)PrimitiveType::Capsule + 1];
    GatherMeshSources(sources);

    uint32 totalVertices = 0;
    uint32 totalIndices = 0;
    for (uint32 i = 0; i < helper::GetCountOf(sources); ++i) {
        gMeshDescs[i].baseVertex = (int32)totalVertices;
        gMeshDescs[i].firstIndex = totalIndices;
        gMeshDescs[i].indexCount = sources[i].indexCount;
        gMeshDescs[i].padding = 0;
        totalVertices += sources[i].vertexCount;
        totalIndices  += sources[i].indexCount;
    }

    // MESH_INDEX_BUFFER
    bufferDesc.size = totalIndices * sizeof(uint32);
    bufferDesc.usage = nri::BufferUsageBits::INDEX_BUFFER;
    NRI_ABORT_ON_FAILURE(NRI.CreateBuffer(*device, bufferDesc, buffer));
    buffers.Add(buffer);

    // MESH_VERTEX_BUFFER
    bufferDesc.size = totalVertices * sizeof(VertexNormal);
    bufferDesc.usage = nri::BufferUsageBits::VERTEX_BUFFER;
    NRI_ABORT_ON_FAILURE(NRI.CreateBuffer(*device, bufferDesc, buffer));
    buffers.Add(buffer);

    // MESH_DESC_BUFFER — structured buffer of MeshDesc, read by bindless VS / GenerateDrawCommandsCS.
    bufferDesc.size = MAX_MESHES * sizeof(MeshDesc);
    bufferDesc.structureStride = sizeof(MeshDesc);
    bufferDesc.usage = nri::BufferUsageBits::SHADER_RESOURCE;
    NRI_ABORT_ON_FAILURE(NRI.CreateBuffer(*device, bufferDesc, buffer));
    buffers.Add(buffer);

    // INSTANCE_BUFFER — persistent DEVICE buffer that the game streamer copies into each frame
    // via CmdCopyStreamedData. Kept persistent so the SRV is stable (one descriptor-set update
    // at init, not per frame). Read by GenerateDrawCommandsCS and the bindless VS.
    bufferDesc.size = MAX_INSTANCES * sizeof(Instance);
    bufferDesc.structureStride = sizeof(Instance);
    bufferDesc.usage = nri::BufferUsageBits::SHADER_RESOURCE;
    NRI_ABORT_ON_FAILURE(NRI.CreateBuffer(*device, bufferDesc, buffer));
    buffers.Add(buffer);

    // INDIRECT_ARGS_BUFFER — DrawIndexed(Base)Desc per-PSO-bucket region, written by GenerateDrawCommandsCS.
    // Consumed by CmdDrawIndexedIndirect as ARGUMENT_BUFFER. Sized for the worst-case (emulated)
    // 28-byte stride so the same buffer works on either backend.
    // structureStride = 4 is NRI's "byte-address" contract on VK: it requests both
    // STORAGE_BUFFER_BIT (for the RWByteAddressBuffer view used by GenerateDrawCommandsCS) and the texel
    // flag — necessary to satisfy VkWriteDescriptorSet's STORAGE_BUFFER expectation.
    bufferDesc.size = NUM_PSO_BUCKETS * MAX_DRAWS_PER_BUCKET * DRAW_INDEXED_DESC_SIZE_MAX;
    bufferDesc.structureStride = 4;
    bufferDesc.usage = nri::BufferUsageBits::ARGUMENT_BUFFER | nri::BufferUsageBits::SHADER_RESOURCE_STORAGE;
    NRI_ABORT_ON_FAILURE(NRI.CreateBuffer(*device, bufferDesc, buffer));
    buffers.Add(buffer);

    // DRAW_COUNT_BUFFER — one uint per PSO bucket, atomic-counter target for GenerateDrawCommandsCS.
    // Cleared to 0 each frame, then consumed by CmdDrawIndexedIndirect's countBuffer slot.
    bufferDesc.size = NUM_PSO_BUCKETS * sizeof(uint32);
    bufferDesc.structureStride = sizeof(uint32);
    bufferDesc.usage = nri::BufferUsageBits::ARGUMENT_BUFFER | nri::BufferUsageBits::SHADER_RESOURCE_STORAGE;
    NRI_ABORT_ON_FAILURE(NRI.CreateBuffer(*device, bufferDesc, buffer));
    buffers.Add(buffer);
}

void RenderingSystem::InitMemory() {
    nri::ResourceGroupDesc resourceGroupDesc = {
        .memoryLocation = nri::MemoryLocation::HOST_UPLOAD,
        .buffers = &buffers[CONSTANT_BUFFER],
        .bufferNum = 1,
    };
    NRI_ABORT_ON_FAILURE(NRI.AllocateAndBindMemory(*device, resourceGroupDesc, &memoryAllocations.Emplace(nullptr)));

    resourceGroupDesc = {
        .memoryLocation = nri::MemoryLocation::HOST_READBACK,
        .buffers = &buffers[READBACK_BUFFER],
        .bufferNum = 1,
    };
    NRI_ABORT_ON_FAILURE(NRI.AllocateAndBindMemory(*device, resourceGroupDesc, &memoryAllocations.Emplace(nullptr)));

    resourceGroupDesc = {
        .memoryLocation = nri::MemoryLocation::DEVICE,
        .buffers = &buffers[MESH_INDEX_BUFFER],
        .bufferNum = NUM_DEVICE_BUFFERS, // shared mesh IB + VB + mesh-desc SRV
    };
    uint32 allocationNum = NRI.CalculateAllocationNumber(*device, resourceGroupDesc);
    NRI_ABORT_ON_FAILURE(NRI.AllocateAndBindMemory(*device, resourceGroupDesc, memoryAllocations.EmplaceMany(allocationNum, nullptr)));

    // Depth texture gets its own allocation so it can be freed/recreated on resize
    resourceGroupDesc = {
        .memoryLocation = nri::MemoryLocation::DEVICE,
        .textures = &textures[DEPTH_TEXTURE],
        .textureNum = 1,
    };
    NRI_ABORT_ON_FAILURE(NRI.AllocateAndBindMemory(*device, resourceGroupDesc, &depthTextureMemory));

    // Remaining textures (shadow map, etc.)
    resourceGroupDesc = {
        .memoryLocation = nri::MemoryLocation::DEVICE,
        .textures = &textures[SHADOW_MAP_TEXTURE],
        .textureNum = textures.GetSize() - 1,
    };
    allocationNum = NRI.CalculateAllocationNumber(*device, resourceGroupDesc);
    NRI_ABORT_ON_FAILURE(NRI.AllocateAndBindMemory(*device, resourceGroupDesc, memoryAllocations.EmplaceMany(allocationNum, nullptr)));
}

void RenderingSystem::InitDescriptors() {
    const nri::DeviceDesc& deviceDesc = NRI.GetDeviceDesc(*device);
    const uint32 constantBufferSize = helper::Align(static_cast<uint32>(sizeof(GlobalConstantBufferLayout)), deviceDesc.memoryAlignment.constantBufferOffset);

    nri::Descriptor* constantBufferViews[8] = {};

    // Constant buffer views
    for (uint32 i = 0; i < GetQueuedFrameNum(); i++) {
        
        for (PerThreadQueueFrames& threadContext : perThreadQueuedFrames) {
            threadContext.queuedFrames[i].globalConstantBufferViewOffsets = i * constantBufferSize;
        }

        nri::BufferViewDesc bufferViewDesc = {
            .buffer = buffers[CONSTANT_BUFFER],
            .type = nri::BufferView::CONSTANT_BUFFER,
            .offset = i * constantBufferSize,
            .size = constantBufferSize,
        };
        NRI_ABORT_ON_FAILURE(NRI.CreateBufferView(bufferViewDesc, constantBufferViews[i]));
        descriptors.Add(constantBufferViews[i]);
    }

    // Depth buffer
    {
        nri::TextureViewDesc textureViewDesc = {textures[DEPTH_TEXTURE], nri::TextureView::DEPTH_STENCIL_ATTACHMENT, depthFormat};
        NRI_ABORT_ON_FAILURE(NRI.CreateTextureView(textureViewDesc, depthAttachment));
        //descriptors.Add(depthAttachment); This one gets tracked by the depthAttachment field instead
    }

    // Shadow Map
    {
        nri::TextureViewDesc textureViewDesc = {textures[SHADOW_MAP_TEXTURE], nri::TextureView::DEPTH_STENCIL_ATTACHMENT, depthFormat};
        NRI_ABORT_ON_FAILURE(NRI.CreateTextureView(textureViewDesc, depthAttachmentShadowsWrite));

        textureViewDesc.type = nri::TextureView::TEXTURE;
        NRI_ABORT_ON_FAILURE(NRI.CreateTextureView(textureViewDesc, depthAttachmentShadowsRead));

        descriptors.Add(depthAttachmentShadowsWrite);
        descriptors.Add(depthAttachmentShadowsRead);
    }

    // Shadow Map sampler
    {
        nri::SamplerDesc viewDesc = {
            .filters = {.min = nri::Filter::LINEAR, .mag = nri::Filter::LINEAR},
            .addressModes = {.u = nri::AddressMode::CLAMP_TO_BORDER, .v = nri::AddressMode::CLAMP_TO_BORDER},
            .borderColor = {1.f, 1.f, 1.f, 1.f},
        };
        NRI_ABORT_ON_FAILURE(NRI.CreateSampler(*device, viewDesc, shadowMapSampler));
        descriptors.Add(shadowMapSampler);
    }

    // Bindless buffer views — shared by the graphics pipeline (bindless VS) and the
    // GenerateDrawCommandsCS compute pipeline.
    {
        // INSTANCE_BUFFER as a structured SRV.
        nri::BufferViewDesc viewDesc = {
            .buffer = buffers[INSTANCE_BUFFER],
            .type = nri::BufferView::STRUCTURED_BUFFER,
            .offset = 0,
            .size = MAX_INSTANCES * sizeof(Instance),
        };
        NRI_ABORT_ON_FAILURE(NRI.CreateBufferView(viewDesc, instancesView));
        descriptors.Add(instancesView);

        // MESH_DESC_BUFFER as a structured SRV.
        viewDesc.buffer = buffers[MESH_DESC_BUFFER];
        viewDesc.size = MAX_MESHES * sizeof(MeshDesc);
        NRI_ABORT_ON_FAILURE(NRI.CreateBufferView(viewDesc, meshDescsView));
        descriptors.Add(meshDescsView);

        // INDIRECT_ARGS_BUFFER as a raw RW byte-address view (HLSL RWByteAddressBuffer).
        // Descriptor type at the layout level is STORAGE_STRUCTURED_BUFFER (NRI groups them).
        viewDesc = {
            .buffer = buffers[INDIRECT_ARGS_BUFFER],
            .type = nri::BufferView::STORAGE_BYTE_ADDRESS_BUFFER,
            .offset = 0,
            .size = NUM_PSO_BUCKETS * MAX_DRAWS_PER_BUCKET * DRAW_INDEXED_DESC_SIZE_MAX,
        };
        NRI_ABORT_ON_FAILURE(NRI.CreateBufferView(viewDesc, indirectArgsView));
        descriptors.Add(indirectArgsView);

        // DRAW_COUNT_BUFFER as a storage structured view of uint.
        viewDesc = {
            .buffer = buffers[DRAW_COUNT_BUFFER],
            .type = nri::BufferView::STORAGE_STRUCTURED_BUFFER,
            .offset = 0,
            .size = NUM_PSO_BUCKETS * sizeof(uint32),
            .structureStride = sizeof(uint32),
        };
        NRI_ABORT_ON_FAILURE(NRI.CreateBufferView(viewDesc, drawCountView));
        descriptors.Add(drawCountView);
    }

    // Descriptor pool
    {
        // Graphics global set: 1 CBV + 2 STRUCTURED_BUFFER SRVs (instances, meshDescs) — multiplied
        //                      by GetQueuedFrameNum() since one set is allocated per queued frame.
        // Graphics shadow set: 1 sampler + 1 texture.
        // GenerateDrawCommandsCS compute set (Phase 4): 2 STRUCTURED_BUFFER SRVs + 2 STORAGE_STRUCTURED_BUFFER UAVs.
        const uint32 queuedN = GetQueuedFrameNum();
        nri::DescriptorPoolDesc descriptorPoolDesc = {
            .descriptorSetMaxNum = queuedN + 1 + 1,
            .samplerMaxNum = queuedN,
            .constantBufferMaxNum = queuedN,
            .textureMaxNum = queuedN,
            .structuredBufferMaxNum = 2u * queuedN + 2u,
            .storageStructuredBufferMaxNum = 2,
        };
        NRI_ABORT_ON_FAILURE(NRI.CreateDescriptorPool(*device, descriptorPoolDesc, descriptorPool));
    }

    // Descriptor sets
    {
        descriptorSets.EmplaceMany(GetQueuedFrameNum() + 1);

        // Global
        NRI_ABORT_ON_FAILURE(NRI.AllocateDescriptorSets(*descriptorPool, *pipelineLayout, GLOBAL_DESCRIPTOR_SET, &descriptorSets[0], GetQueuedFrameNum(), 0));

        // instancesView / meshDescsView are persistent SRVs shared across all per-frame
        // graphics sets and the compute descriptor set below.
        nri::Descriptor* bindlessSrvs[2] = {instancesView, meshDescsView};
        for (uint32 i = 0; i < GetQueuedFrameNum(); i++) {
            nri::UpdateDescriptorRangeDesc updateDescriptorRangeDescs[2] = {
                {descriptorSets[i], 0, 0, &constantBufferViews[i], 1},
                {descriptorSets[i], 1, 0, bindlessSrvs,            2},
            };
            NRI.UpdateDescriptorRanges(updateDescriptorRangeDescs, helper::GetCountOf(updateDescriptorRangeDescs));
        }

        // Shadows
        NRI_ABORT_ON_FAILURE(NRI.AllocateDescriptorSets(*descriptorPool, *pipelineLayout, SHADOWS_DESCRIPTOR_SET, &descriptorSets[GetQueuedFrameNum()], 1, 0));

        nri::UpdateDescriptorRangeDesc updateDescriptorRangeDescs[2] = {
            {descriptorSets[GetQueuedFrameNum()], 0, 0, &shadowMapSampler, 1},
            {descriptorSets[GetQueuedFrameNum()], 1, 0, &depthAttachmentShadowsRead, 1}
        };
        NRI.UpdateDescriptorRanges(updateDescriptorRangeDescs, helper::GetCountOf(updateDescriptorRangeDescs));

        // GenerateDrawCommandsCS compute set: 2 structured SRVs (instances, meshDescs) + 2 storage views (indirect args, drawCount).
        NRI_ABORT_ON_FAILURE(NRI.AllocateDescriptorSets(*descriptorPool, *generateDrawCommandsLayout, 0, &generateDrawCommandsDescriptorSet, 1, 0));

        nri::Descriptor* computeSrvs[2] = {instancesView, meshDescsView};
        nri::Descriptor* computeUavs[2] = {indirectArgsView, drawCountView};
        nri::UpdateDescriptorRangeDesc computeUpdates[] = {
            {generateDrawCommandsDescriptorSet, 0, 0, computeSrvs, 2},
            {generateDrawCommandsDescriptorSet, 1, 0, computeUavs, 2},
        };
        NRI.UpdateDescriptorRanges(computeUpdates, helper::GetCountOf(computeUpdates));
    }
}

void RenderingSystem::UploadGPUData() {

    MeshSource sources[(uint32)PrimitiveType::Capsule + 1];
    GatherMeshSources(sources);

    uint32 totalVertices = 0;
    uint32 totalIndices = 0;
    for (uint32 i = 0; i < helper::GetCountOf(sources); ++i) {
        totalVertices += sources[i].vertexCount;
        totalIndices  += sources[i].indexCount;
    }

    // Build flat staging blobs concatenating every primitive in PrimitiveType order.
    // Allocated through the engine allocator (not on the stack — capsule alone is several KB).
    VertexNormal* vbStaging = reinterpret_cast<VertexNormal*>(allocator.Allocate(totalVertices * sizeof(VertexNormal), alignof(VertexNormal)));
    uint32* ibStaging = reinterpret_cast<uint32*>(allocator.Allocate(totalIndices  * sizeof(uint32),       alignof(uint32)));

    {
        uint32 vCursor = 0;
        uint32 iCursor = 0;
        for (uint32 i = 0; i < helper::GetCountOf(sources); ++i) {
            const MeshSource& src = sources[i];
            memcpy(vbStaging + vCursor, src.vertices, src.vertexCount * sizeof(VertexNormal));
            memcpy(ibStaging + iCursor, src.indices,  src.indexCount  * sizeof(uint32));
            vCursor += src.vertexCount;
            iCursor += src.indexCount;
        }
    }

    const nri::BufferUploadDesc bufferData[] = {
        {ibStaging,    buffers[MESH_INDEX_BUFFER],  {nri::AccessBits::INDEX_BUFFER}},
        {vbStaging,    buffers[MESH_VERTEX_BUFFER], {nri::AccessBits::VERTEX_BUFFER}},
        {gMeshDescs,   buffers[MESH_DESC_BUFFER],   {nri::AccessBits::SHADER_RESOURCE, nri::StageBits::VERTEX_SHADER | nri::StageBits::COMPUTE_SHADER}},
    };

    NRI_ABORT_ON_FAILURE(NRI.UploadData(*graphicsQueue, nullptr, 0, bufferData, helper::GetCountOf(bufferData)));

    allocator.Free(reinterpret_cast<uint8*>(vbStaging));
    allocator.Free(reinterpret_cast<uint8*>(ibStaging));
}

void RenderingSystem::InitQueryPool() {
    const nri::DeviceDesc& deviceDesc = NRI.GetDeviceDesc(*device);
    if (deviceDesc.features.pipelineStatistics) {
        nri::QueryPoolDesc queryPoolDesc = {
            .queryType = nri::QueryType::PIPELINE_STATISTICS,
            .capacity = GetQueuedFrameNum(),
        };
        NRI_ABORT_ON_FAILURE(NRI.CreateQueryPool(*device, queryPoolDesc, queryPool));
    }
}
void RenderingSystem::LatencySleep() {

    const uint32 queuedFrameIndex = frameIndex % GetQueuedFrameNum();

    NRI.Wait(*frameFence, frameIndex >= GetQueuedFrameNum() ? 1 + frameIndex - GetQueuedFrameNum() : 0);

    for (PerThreadQueueFrames& threadContext : perThreadQueuedFrames) {
        NRI.ResetCommandAllocator(*threadContext.queuedFrames[queuedFrameIndex].commandAllocator);
    }
}

void RenderingSystem::CreateSwapChainTextures(nri::Texture* const* nriTextures, uint32 count, nri::Format format) {
    for (uint32 i = 0; i < count; i++) {
        nri::TextureViewDesc textureViewDesc = {nriTextures[i], nri::TextureView::COLOR_ATTACHMENT, format};

        nri::Descriptor* colorAttachment = nullptr;
        NRI_ABORT_ON_FAILURE(NRI.CreateTextureView(textureViewDesc, colorAttachment));

        nri::Fence* acquireSemaphore = nullptr;
        NRI_ABORT_ON_FAILURE(NRI.CreateFence(*device, nri::SWAPCHAIN_SEMAPHORE, acquireSemaphore));

        nri::Fence* releaseSemaphore = nullptr;
        NRI_ABORT_ON_FAILURE(NRI.CreateFence(*device, nri::SWAPCHAIN_SEMAPHORE, releaseSemaphore));

        swapChainTextures.Emplace() = {
            .acquireSemaphore = acquireSemaphore,
            .releaseSemaphore = releaseSemaphore,
            .texture = nriTextures[i],
            .colorAttachment = colorAttachment,
            .attachmentFormat = format,
        };
    }
}

#if WITH_EDITOR
bool RenderingSystem::InitImguiRenderer(nri::Device& device) {

    SetupImGui(&allocator);

    mouseCursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    mouseCursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    mouseCursors[ImGuiMouseCursor_Hand] = glfwCreateStandardCursor(GLFW_POINTING_HAND_CURSOR);
    mouseCursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_RESIZE_EW_CURSOR);
    mouseCursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_RESIZE_NS_CURSOR);
    mouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_RESIZE_ALL_CURSOR);

#if DS_PLATFORM_WINDOWS
    mouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);
    mouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
#endif

    nri::Result result = nri::nriGetInterface(device, NRI_INTERFACE(nri::ImguiInterface), &imgui);
    if (result != nri::Result::SUCCESS)
        return false;

    nri::ImguiDesc imguiDesc = {};

    result = imgui.CreateImgui(device, imguiDesc, imguiRenderer);
    if (result != nri::Result::SUCCESS)
        return false;

    return true;
}
void RenderingSystem::DestroyImguiRenderer() {

    if (!imguiRenderer)
        return;

    imgui.DestroyImgui(imguiRenderer);
    ImGui::DestroyContext();

    imguiRenderer = nullptr;
}
void RenderingSystem::CmdCopyImguiData(nri::CommandBuffer& commandBuffer, nri::Streamer& streamer) {

    if (!imguiRenderer)
        return;

    const ImDrawData& drawData = *ImGui::GetDrawData();

    nri::CopyImguiDataDesc copyImguiDataDesc = {
        .drawLists = drawData.CmdLists.Data,
        .drawListNum = static_cast<uint32>(drawData.CmdLists.Size),
        .textures = drawData.Textures->Data,
        .textureNum = static_cast<uint32>(drawData.Textures->Size),
    };
    imgui.CmdCopyImguiData(commandBuffer, streamer, *imguiRenderer, copyImguiDataDesc);
}

void RenderingSystem::CmdDrawImgui(
    nri::CommandBuffer& commandBuffer,
    nri::Format attachmentFormat,
    float sdrScale,
    bool isSrgb) {

    if (!imguiRenderer)
        return;

    const ImDrawData& drawData = *ImGui::GetDrawData();

    nri::DrawImguiDesc drawImguiDesc = {
        .drawLists = drawData.CmdLists.Data,
        .drawListNum = static_cast<uint32>(drawData.CmdLists.Size),
        .displaySize = {static_cast<nri::Dim_t>(drawData.DisplaySize.x), static_cast<nri::Dim_t>(drawData.DisplaySize.y)},
        .hdrScale = sdrScale,
        .attachmentFormat = attachmentFormat,
        .linearColor = !isSrgb,
    };
    imgui.CmdDrawImgui(commandBuffer, *imguiRenderer, drawImguiDesc);
}
#endif

bool RenderingSystem::TryResizeSwapChain() {

    uint32 newWindowWidth, newWindowHeight;
    gWindow->GetSize(newWindowWidth, newWindowHeight);

    if (newWindowWidth == windowWidth && newWindowHeight == windowHeight) {
        return false;
    }

    windowWidth = newWindowWidth;
    windowHeight = newWindowHeight;

    // Wait for idle
    NRI.QueueWaitIdle(graphicsQueue);

    // Destroy old swapchain
    for (SwapChainTexture& swapChainTexture : swapChainTextures) {
        NRI.DestroyFence(swapChainTexture.acquireSemaphore);
        NRI.DestroyFence(swapChainTexture.releaseSemaphore);
        NRI.DestroyDescriptor(swapChainTexture.colorAttachment);
    }

    NRI.DestroySwapChain(swapChain);

    nri::SwapChainDesc swapChainDesc = {
        .window = CreateNRIWindow(gWindow->GetGLFWWindow()),
        .queue = graphicsQueue,
        .width = static_cast<uint16>(windowWidth),
        .height = static_cast<uint16>(windowHeight),
        .textureNum = GetOptimalSwapChainTextureNum(),
        .format = nri::SwapChainFormat::BT709_G22_10BIT,
        .flags = (settings.enableVSync ? nri::SwapChainBits::VSYNC : nri::SwapChainBits::NONE) | nri::SwapChainBits::ALLOW_TEARING,
        .queuedFrameNum = GetQueuedFrameNum(),
    };
    NRI_ABORT_ON_FAILURE(NRI.CreateSwapChain(*device, swapChainDesc, swapChain));

    uint32 swapChainTextureNum;
    nri::Texture* const* localSwapChainTextures = NRI.GetSwapChainTextures(*swapChain, swapChainTextureNum);
    nri::Format swapChainFormat = NRI.GetTextureDesc(*localSwapChainTextures[0]).format;

    swapChainTextures.Reset();
    CreateSwapChainTextures(localSwapChainTextures, swapChainTextureNum, swapChainFormat);

    // Recreate depth texture at new resolution
    NRI.DestroyDescriptor(depthAttachment);
    NRI.DestroyTexture(textures[DEPTH_TEXTURE]);
    NRI.FreeMemory(depthTextureMemory);

    {
        const nri::TextureDesc textureDesc = {
            .type = nri::TextureType::TEXTURE_2D,
            .usage = nri::TextureUsageBits::DEPTH_STENCIL_ATTACHMENT,
            .format = depthFormat,
            .width = static_cast<nri::Dim_t>(windowWidth),
            .height = static_cast<nri::Dim_t>(windowHeight),
            .mipNum = 1,
        };
        NRI_ABORT_ON_FAILURE(NRI.CreateTexture(*device, textureDesc, textures[DEPTH_TEXTURE]));

        nri::ResourceGroupDesc resourceGroupDesc = {
            .memoryLocation = nri::MemoryLocation::DEVICE,
            .textures = &textures[DEPTH_TEXTURE],
            .textureNum = 1,
        };
        NRI_ABORT_ON_FAILURE(NRI.AllocateAndBindMemory(*device, resourceGroupDesc, &depthTextureMemory));

        nri::TextureViewDesc textureViewDesc = {textures[DEPTH_TEXTURE], nri::TextureView::DEPTH_STENCIL_ATTACHMENT, depthFormat};
        NRI_ABORT_ON_FAILURE(NRI.CreateTextureView(textureViewDesc, depthAttachment));
    }

    // Rebuild the FrameGraph with the new swapchain views + new depth texture/view.
    if (frameGraph) {
        frameGraph->Invalidate();
        BuildFrameGraph();
        frameGraph->Compile();
    }

    return true;
}


static void CursorMode(GLFWwindow* glfwWindow, int32 mode) {
    if (mode == GLFW_CURSOR_NORMAL) {
        glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
#if DS_PLATFORM_WINDOWS
        // GLFW works with cursor visibility incorrectly
        for (uint32 n = 0; ::ShowCursor(1) < 0 && n < 256; n++)
            ;
#endif
    } else {
        glfwSetInputMode(glfwWindow, GLFW_CURSOR, mode);
#if DS_PLATFORM_WINDOWS
        // GLFW works with cursor visibility incorrectly
        for (uint32 n = 0; ::ShowCursor(0) >= 0 && n < 256; n++)
            ;
#endif
    }
}

#if WITH_EDITOR
static void OnMainMenu() {
    // FPS text
    const InlineString<16> fpsText = InlineString<16>(static_cast<uint32>(ImGui::GetIO().Framerate)) + " FPS";
    const ImVec2 textSize = ImGui::CalcTextSize(fpsText.CStr());

    // Get menu bar full width and current cursor position
    const float menuBarWidth = ImGui::GetWindowWidth();
    const float padding = ImGui::GetStyle().WindowPadding.x;

    // Position cursor to right-aligned position
    const float prevCursorPos = ImGui::GetCursorPosX();
    ImGui::SetCursorPosX(menuBarWidth - textSize.x - padding);
    ImGui::Text(fpsText.CStr());
    ImGui::SetCursorPosX(prevCursorPos);
}
#endif

static Mat4 ComputeModelMatrix(const Transform& transform, const DynamicRigidBody* body, bool interpolate) {
    if (interpolate && body) {
        // Interpolate between last and current physics update since physics runs at a different frequency than the renderer
        const Vec3 interpolatedPosition = body->GetInterpolatedPosition(transform.location);
        const Quat interpolatedRotation = body->GetInterpolatedRotation(transform.rotation);
        return Mat4::Translation(interpolatedPosition) * interpolatedRotation.ToRotationMatrix() * Mat4::Scaling(transform.scale);
    }
    return transform.ToMat4();
}

static Mat4 ComputeNormalMatrix(const Transform& transform, const Mat4& modelMatrix) {

    Mat4 normalMatrix;

    if (transform.HasUniformScale()) {
        // Ref: https://lxjk.github.io/2017/10/01/Stop-Using-Normal-Matrix.html

        normalMatrix = modelMatrix;
        const Vec3 scale = transform.scale;
        ASSERT(scale.x > 0.f && scale.y > 0.f && scale.z > 0.f);
        for (uint32 row = 0; row < 3; row++) {
            normalMatrix.m[row][0] *= 1.f/scale.x;
            normalMatrix.m[row][1] *= 1.f/scale.y;
            normalMatrix.m[row][2] *= 1.f/scale.z;
            normalMatrix.m[row][3] = 0.f;
        }
    } else {
        Mat3 rotationMatrix = modelMatrix.ToMat3();
        Mat3 normalMatrixMat3;
        rotationMatrix.Inverse(normalMatrixMat3);
        normalMatrixMat3 = normalMatrixMat3.Transpose();
        normalMatrix = Mat4(
            normalMatrixMat3.m[0][0], normalMatrixMat3.m[0][1], normalMatrixMat3.m[0][2], 0.f,
            normalMatrixMat3.m[1][0], normalMatrixMat3.m[1][1], normalMatrixMat3.m[1][2], 0.f,
            normalMatrixMat3.m[2][0], normalMatrixMat3.m[2][1], normalMatrixMat3.m[2][2], 0.f,
            0.f, 0.f, 0.f, 0.f
        );
        //const Mat4 rotation = transform.rotation.ToRotationMatrix();
        //const Vec3 scale = { 1.f / transform.scale.x, 1.f / transform.scale.y, 1.f / transform.scale.z };
        //normalMatrix = rotation * Mat4::Scaling(scale);
    }

    return normalMatrix;
}

static void ComputeModelAndNormalMatrices(
    const Transform& transform,
    const DynamicRigidBody* body,
    const CharacterController* characterController,
    Mat4& modelMatrix,
    Mat4& normalMatrix,
    bool interpolate) {

    ASSERT(transform.scale.x > 0.f && transform.scale.y > 0.f && transform.scale.z > 0.f);

    Vec3 translation;
    Mat3 rotationMatrix;
    Mat3 scalingMatrix = Mat3::Scaling(transform.scale);

    if (interpolate && body) {
        // Interpolate between last and current physics update since physics runs at a different frequency than the renderer
        const Vec3 interpolatedPosition = body->GetInterpolatedPosition(transform.location);
        const Quat interpolatedRotation = body->GetInterpolatedRotation(transform.rotation);
        translation = interpolatedPosition;
        rotationMatrix = interpolatedRotation.ToRotationMatrix3x3();
    } else if (interpolate && characterController) {
        // Interpolate between last and current physics update since physics runs at a different frequency than the renderer
        const Vec3 interpolatedPosition = characterController->GetInterpolatedPosition(transform.location);
        const Quat interpolatedRotation = characterController->GetInterpolatedRotation(transform.rotation);
        translation = interpolatedPosition;
        rotationMatrix = interpolatedRotation.ToRotationMatrix3x3();
    } else {
        translation = transform.location;
        rotationMatrix = transform.rotation.ToRotationMatrix3x3();
    }

    Mat3 rotationScaling = rotationMatrix * scalingMatrix;
    Mat4 rotationScaling4x4 = {
        rotationScaling.m[0][0], rotationScaling.m[0][1], rotationScaling.m[0][2], 0.f,
        rotationScaling.m[1][0], rotationScaling.m[1][1], rotationScaling.m[1][2], 0.f,
        rotationScaling.m[2][0], rotationScaling.m[2][1], rotationScaling.m[2][2], 0.f,
        0.f, 0.f, 0.f, 1.f
    };

    modelMatrix = Mat4::Translation(translation) * rotationScaling4x4;

    // Now invert scaling for normal matrix
    // since we have access to the transform
    // and not just the model matrix we can
    // do this instead of full 3x3 matrix inversion
    // N = (M^-1)^T and M = R*S
    // N = ((R*S)^-1)^T
    // N = (R^-1)^T * (S^-1)^T
    // R is orthogonal thus (R^-1)^T = R
    // S is diagonal thus (S^-1)^T = S^-1
    // Thus, N = R*S^-1

    scalingMatrix[0][0] = 1.f / scalingMatrix[0][0];
    scalingMatrix[1][1] = 1.f / scalingMatrix[1][1];
    scalingMatrix[2][2] = 1.f / scalingMatrix[2][2];

    rotationScaling = rotationMatrix * scalingMatrix;

    normalMatrix = {
        rotationScaling.m[0][0], rotationScaling.m[0][1], rotationScaling.m[0][2], 0.f,
        rotationScaling.m[1][0], rotationScaling.m[1][1], rotationScaling.m[1][2], 0.f,
        rotationScaling.m[2][0], rotationScaling.m[2][1], rotationScaling.m[2][2], 0.f,
        0.f, 0.f, 0.f, 1.f
    };
}

static void CreateInstance(
    const Transform& transform,
    const PrimitiveRenderer& renderer,
    const DynamicRigidBody* rigidBody,
    const CharacterController* characterController,
    const FrameContext& frameContext,
    Instance& instance) {

    Mat4 modelMatrix, normalMatrix;
    ComputeModelAndNormalMatrices(transform, rigidBody, characterController, modelMatrix, normalMatrix, frameContext.interpolate);

    const bool isOpaque = renderer.alpha >= 1.f;

    instance = {
        .modelMatrix = modelMatrix,
        .normalMatrix = normalMatrix,
        .color = {renderer.color.x, renderer.color.y, renderer.color.z, renderer.alpha},
        .meshID = static_cast<uint32>(renderer.primitiveType),
        .materialID = 0, // reserved, single-material renderer for now
        .shaderID = isOpaque ? ShaderBucket::Opaque : ShaderBucket::Transparent,
        .flags = isOpaque ? InstanceFlag::IsOpaque : InstanceFlag::IsTransparent
    };

    if (renderer.isVisible) {
        instance.flags |= InstanceFlag::IsVisible;
    }

    if (renderer.castShadow) {
        instance.flags |= InstanceFlag::CastsShadow;
    }
}

void RenderingSystem::CollectInstances(ThreadContext& threadContext, Vault& vault) {

    TIME_SCOPE("Renderer_CollectInstances");

    static uint32 numPrimitiveRenderers = 0;
    static uint32 numPhysicsNPCs = 0;

    if (threadContext.IsMainThread())
    {
        numPrimitiveRenderers = vault.GetNumberOfComponents<PrimitiveRenderer>();
        numPhysicsNPCs = vault.GetNumberOfComponents<PhysicsNPC>();

        const uint32 numTotalInstances = numPrimitiveRenderers + numPhysicsNPCs;
        const uint32 numExistingInstances = instances.GetSize();

        ASSERT(numTotalInstances < MAX_INSTANCES);

        const int32 diff = numTotalInstances - numExistingInstances;
        if (diff > 0)
        {
            instances.EmplaceMany(diff);
        }
        else
        {
            for (int32 i = diff; i < 0; ++i)
            {
                instances.RemoveLast();
            }
        }
    }
    threadContext.Sync();

    const Range primitiveRenderersRange = threadContext.UniformRange(numPrimitiveRenderers);
    ContainerView<ComponentEntityPair<PrimitiveRenderer>> primitiveRenderers = vault.GetComponents<PrimitiveRenderer>();
    for (uint32 i = primitiveRenderersRange.min; i < primitiveRenderersRange.max; i++) {

        auto& [primitiveRenderer, entity] = primitiveRenderers[i];

        CreateInstance(
            vault.GetComponent<Transform>(entity),
            primitiveRenderer,
            vault.TryGetComponent<DynamicRigidBody>(entity),
            vault.TryGetComponent<CharacterController>(entity),
            frameContext,
            instances[i]
        );
    }

    const Range physicsNPCsRange = threadContext.UniformRange(numPhysicsNPCs);
    ContainerView<ComponentEntityPair<PhysicsNPC>> physicsNPCs = vault.GetComponents<PhysicsNPC>();
    for (uint32 i = physicsNPCsRange.min; i < physicsNPCsRange.max; i++) {

        auto& [physicsNPC, entity] = physicsNPCs[i];

        CreateInstance(
            physicsNPC.transform,
            physicsNPC.primitiveRenderer,
            &physicsNPC.dynamicRigidBody,
            nullptr,
            frameContext,
            instances[i + numPrimitiveRenderers]
        );
    }
    threadContext.Sync();

    if (threadContext.IsMainThread())
    {
        if (!useGpuDrawGeneration) {

            TIME_SCOPE("Renderer_SortInstances");

            // Sort by (shaderID, meshID) so the CPU draw path can merge consecutive same-mesh
            // instances within a bucket into a single CmdDrawIndexed (instanceCount = N).
            // GenerateDrawCommandsCS is order-agnostic; the GPU path is unaffected.
            if (instances.GetSize() > 1) {
                std::sort(instances.GetData(), instances.GetData() + instances.GetSize(),
                    [](const Instance& a, const Instance& b) {
                        if (a.shaderID != b.shaderID) return a.shaderID < b.shaderID;
                        return a.meshID < b.meshID;
                    });
            }
        }

        // Stream the per-frame instance array into the game streamer's ring, with the persistent
        // INSTANCE_BUFFER as the copy destination. CmdCopyStreamedData (issued in Update()) does
        // the actual transfer on the GPU. The descriptor view (instancesView) points at
        // INSTANCE_BUFFER, so it never has to be re-created across frames.
        if (!instances.IsEmpty()) {
            const nri::DataSize chunk = {instances.GetData(), instances.GetSize() * sizeof(Instance)};
            const nri::StreamBufferDataDesc streamDesc = {
                .dataChunks = &chunk,
                .dataChunkNum = 1,
                .placementAlignment = static_cast<uint32>(alignof(Instance)),
                .dstBuffer = buffers[INSTANCE_BUFFER],
                .dstOffset = 0,
            };

            instanceBufferOffset = NRI.StreamBufferData(*gameStreamer, streamDesc);
        } else {
            instanceBufferOffset = {};
        }
    }
    threadContext.Sync();
}

void RenderingSystem::UpdateConstantBuffer(uint32 queuedFrameIndex, const Camera* camera, float shadowProjectionSize) {

    const uint64_t rangeOffset = perThreadQueuedFrames[0].queuedFrames[queuedFrameIndex].globalConstantBufferViewOffsets;
    GlobalConstantBufferLayout* constants = static_cast<GlobalConstantBufferLayout*>(
        NRI.MapBuffer(*buffers[CONSTANT_BUFFER], rangeOffset, sizeof(GlobalConstantBufferLayout)));

    if (!constants)
        return;

    constants->gViewToClip = camera->GetProjection() * camera->GetView();
    constants->gCameraPos = camera->GetPosition();

    const Vec3 lightDir = Vec3(-0.6f, -1.0f, -0.6f).Normalize();

    // Center shadow volume around visible camera region
    const Vec3 shadowCenter = camera->GetPosition() + camera->GetForward() * shadowProjectionSize * .5f;

    // Position virtual light camera
    const Vec3 lightEye = shadowCenter - lightDir * shadowProjectionSize;

    // Stable up vector (avoid singularity)
    Vec3 up = Vec3::Y;
    if (fabs(Dot(lightDir, up)) > 0.99f)
        up = Vec3::Z;

    const Mat4 lightView = Mat4::LookAt(lightEye, shadowCenter, up);

    // Symmetric box around frustum center in light space
    const float size = shadowProjectionSize;
    constexpr float nearPlane = 1.0f;
    const float farPlane = shadowProjectionSize * 2.0f;

    const Mat4 lightProj = Mat4::OrthographicVulkan(-size, size, -size, size, nearPlane, farPlane);

    constants->gLightViewToClip = lightProj * lightView;

    NRI.UnmapBuffer(*buffers[CONSTANT_BUFFER]);
}

void RenderingSystem::BuildFrameGraph() {

    TIME_SCOPE("Renderer_BuildFrameGraph");

    // Imports use UNDEFINED initial state: every frame the graph re-emits an
    // UNDEFINED -> {required state} barrier, which is valid in NRI / Vulkan / D3D12
    // (hardware discards old contents). Avoids tracking actual end-of-frame state
    // across resize / first frame.
    constexpr nri::AccessLayoutStage initialState = {
        nri::AccessBits::NONE,
        nri::Layout::UNDEFINED,
        nri::StageBits::ALL,
    };

    constexpr nri::AccessStage initialBufferState = {
        nri::AccessBits::NONE,
        nri::StageBits::ALL
    };

    // INSTANCE_BUFFER: cb0 (recorded each frame in Update) copies the streamer data in and
    // leaves the buffer in SHADER_RESOURCE state. The graph picks up from there for the
    // BuildDrawsPass and the bindless VS reads in Shadow/MainPass.
    {
        const BufferViews views = {
            .shaderResource = instancesView,
        };
        instanceBufferHandle = frameGraph->ImportBuffer(
            "InstancesBuffer",
            buffers[INSTANCE_BUFFER],
            views,
            initialBufferState);
    }

    // MESH_DESC_BUFFER: uploaded once at init via UploadGPUData (left in SHADER_RESOURCE).
    // The graph never writes to it; declare it as steady SHADER_RESOURCE.
    {
        const BufferViews views = {
            .shaderResource = meshDescsView,
        };
        meshDescBufferHandle = frameGraph->ImportBuffer(
            "MeshDescBuffer",
            buffers[MESH_DESC_BUFFER],
            views,
            initialBufferState);
    }

    // INDIRECT_ARGS_BUFFER / DRAW_COUNT_BUFFER: written by GenerateDrawCommandsCS each frame and
    // consumed by CmdDrawIndexedIndirect in ShadowPass + MainPass. Both end every frame in
    // ARGUMENT_BUFFER; the graph re-walks from initialState each frame, so declaring it here
    // is necessary for D3D12 enhanced barriers to sync correctly against the previous frame's
    // still-pending indirect reads. First-frame state is COMMON, which enhanced barriers
    // accept loosely on the source side.
    {
        const BufferViews views = {
            .shaderResourceStorage = indirectArgsView,
        };
        indirectArgsBufferHandle = frameGraph->ImportBuffer(
            "IndirectArgsBuffer",
            buffers[INDIRECT_ARGS_BUFFER],
            views,
            initialBufferState);
    }
    {
        const BufferViews views = {
            .shaderResourceStorage = drawCountView,
        };
        drawCountBufferHandle = frameGraph->ImportBuffer(
            "DrawCountBuffer",
            buffers[DRAW_COUNT_BUFFER],
            views,
            initialBufferState);
    }

    // Swapchain backbuffer (rotates per frame; updated via SetImport in Update).
    // finalState = PRESENT so the graph emits the closing barrier automatically.
    {
        constexpr nri::AccessLayoutStage finalState = {
            nri::AccessBits::NONE,
            nri::Layout::PRESENT,
            nri::StageBits::NONE,
        };
        swapchainHandle = frameGraph->ImportTexture(
            "SwapchainBackbuffer",
            swapChainTextures[0].texture,
            {}, // placeholder; overwritten by SetImport each frame
            initialState,
            finalState);
    }

    // Main pass depth (recreated on resize). No closing barrier needed.
    {
        const TextureViews views = {
            .depthAttachment = depthAttachment
        };
        depthHandle = frameGraph->ImportTexture(
            "SceneDepth",
            textures[DEPTH_TEXTURE],
            views,
            initialState);
    }

    // Shadow map: written by shadow pass, read by main pass. No closing barrier.
    {
        const TextureViews views = {
            .shaderResource  = depthAttachmentShadowsRead,
            .depthAttachment = depthAttachmentShadowsWrite,
        };
        shadowMapHandle = frameGraph->ImportTexture(
            "ShadowMap",
            textures[SHADOW_MAP_TEXTURE],
            views,
            initialState);
    }

    // StreamInstances: copies the per-frame Instance[] from the gameStreamer's HOST_UPLOAD ring
    // into the persistent InstanceBuffer.
    frameGraph->BeginCopyPass("StreamInstances")
        .Access(instanceBufferHandle, nri::AccessBits::COPY_DESTINATION, nri::StageBits::COPY)
        .Execute([this](nri::CommandBuffer& commandBuffer, const nri::RenderingDesc&) {
            // CollectInstances(); Currently not working
            if (instances.GetSize() > 0) {
                NRI.CmdCopyStreamedData(commandBuffer, *gameStreamer);
            }
        })
        .EndPass();

    // ClearDrawCount: zero the per-bucket counters before GenerateDrawCommandsCS atomic-adds into them.
    // Must run every frame even when there are 0 instances, otherwise CmdDrawIndexedIndirect
    // would re-use stale counts from the previous frame.
    frameGraph->BeginCopyPass("ClearDrawCount")
        .Access(drawCountBufferHandle, nri::AccessBits::COPY_DESTINATION, nri::StageBits::COPY)
        .Execute([this](nri::CommandBuffer& commandBuffer, const nri::RenderingDesc&) {
            NRI.CmdZeroBuffer(commandBuffer, *buffers[DRAW_COUNT_BUFFER], 0, NUM_PSO_BUCKETS * sizeof(uint32));
        })
        .EndPass();

    // BuildDrawsPass: compute that consumes the per-frame instances + persistent mesh descriptions
    frameGraph->BeginComputePass("BuildDrawsPass")
        .Access(instanceBufferHandle, nri::AccessBits::SHADER_RESOURCE, nri::StageBits::COMPUTE_SHADER)
        .Access(meshDescBufferHandle, nri::AccessBits::SHADER_RESOURCE, nri::StageBits::COMPUTE_SHADER)
        .WritesStorage(indirectArgsBufferHandle)
        .WritesStorage(drawCountBufferHandle)
        .Execute([this](nri::CommandBuffer& commandBuffer, const nri::RenderingDesc&) {
            RecordBuildDrawCommandsPass(commandBuffer);
        })
        .EndPass();

    frameGraph->BeginGraphicsPass("ShadowPass")
        .Access(instanceBufferHandle, nri::AccessBits::SHADER_RESOURCE, nri::StageBits::VERTEX_SHADER)
        .ReadsArgumentBuffer(indirectArgsBufferHandle)
        .ReadsArgumentBuffer(drawCountBufferHandle)
        .SetDepthAttachment(shadowMapHandle, nri::LoadOp::CLEAR, nri::StoreOp::STORE, CLEAR_DEPTH, 0)
        .Execute([this](nri::CommandBuffer& commandBuffer, const nri::RenderingDesc& renderingDesc) {
            NRI.CmdBeginRendering(commandBuffer, renderingDesc);
            RecordShadowPassBody(commandBuffer);
            NRI.CmdEndRendering(commandBuffer);
        })
        .EndPass();

    frameGraph->BeginGraphicsPass("MainPass")
        .Access(instanceBufferHandle, nri::AccessBits::SHADER_RESOURCE, nri::StageBits::VERTEX_SHADER)
        .ReadsArgumentBuffer(indirectArgsBufferHandle)
        .ReadsArgumentBuffer(drawCountBufferHandle)
        .ReadsTexture(shadowMapHandle) // shadow map sampled in fragment shader via descriptor set
        .SetColorAttachment(0, swapchainHandle, nri::LoadOp::CLEAR, nri::StoreOp::STORE, CLEAR_COLOR)
        .SetDepthAttachment(depthHandle, nri::LoadOp::CLEAR, nri::StoreOp::STORE, CLEAR_DEPTH, 0)
        .Execute([this](nri::CommandBuffer& commandBuffer, const nri::RenderingDesc& renderingDesc) {
            NRI.CmdBeginRendering(commandBuffer, renderingDesc);
            RecordMainPassBody(commandBuffer);
            NRI.CmdEndRendering(commandBuffer);
        })
        .EndPass();

#if WITH_EDITOR
    // ImGui draw: writes over the main pass output
    frameGraph->BeginGraphicsPass("ImGuiPass")
        .SetColorAttachment(0, swapchainHandle, nri::LoadOp::LOAD, nri::StoreOp::STORE)
        .Execute([this](nri::CommandBuffer& commandBuffer, const nri::RenderingDesc& renderingDesc) {

            CmdCopyImguiData(commandBuffer, *imguiStreamer);

            NRI.CmdBeginRendering(commandBuffer, renderingDesc);
            // attachmentFormat is the same across the swapchain ring; index 0 is fine.
            CmdDrawImgui(commandBuffer, swapChainTextures[0].attachmentFormat, 1.0f, true);
            NRI.CmdEndRendering(commandBuffer);
        })
        .EndPass();
#endif
}

void RenderingSystem::RecordBuildDrawCommandsPass(nri::CommandBuffer& commandBuffer) {

    const uint32 numInstances = instances.GetSize();

    if (numInstances == 0) {
        return; // drawCount was just cleared to zero; indirect draws will issue 0 draws
    }

    // Bind compute pipeline + descriptor set (instances + meshDescs + indirectArgs + drawCount).
    NRI.CmdSetPipelineLayout(commandBuffer, nri::BindPoint::COMPUTE, *generateDrawCommandsLayout);
    nri::SetDescriptorSetDesc dsDesc = {0, generateDrawCommandsDescriptorSet};
    NRI.CmdSetDescriptorSet(commandBuffer, dsDesc);
    NRI.CmdSetPipeline(commandBuffer, *pipelines[GENERATE_DRAW_COMMANDS_PIPELINE]);

    auto dispatchBucket = [&](uint32 bucketID, uint32 flagMask) {
        BuildDrawsConstants consts = {
            .instanceCount = numInstances,
            .passShaderID = bucketID,
            .passFlagMask = flagMask,
            .maxDrawsPerBucket = MAX_DRAWS_PER_BUCKET,
        };
        nri::SetRootConstantsDesc rc = {0, &consts, sizeof(consts)};
        NRI.CmdSetRootConstants(commandBuffer, rc);

        const uint32 groups = (numInstances + BUILD_DRAWS_GROUP_SIZE - 1) / BUILD_DRAWS_GROUP_SIZE;
        NRI.CmdDispatch(commandBuffer, {groups, 1, 1});
    };

    // Opaque + Transparent buckets filter by inst.shaderID (passFlagMask = 1).
    dispatchBucket(ShaderBucket::Opaque,      /*flagMask=*/InstanceFlag::IsVisible);
    dispatchBucket(ShaderBucket::Transparent, /*flagMask=*/InstanceFlag::IsVisible);
    // Shadow bucket: instance.shaderID won't match Shadow; the GenerateDrawCommandsCS branch keys on
    // the non-zero flagMask to short-circuit the shaderID filter.
    dispatchBucket(ShaderBucket::Shadow,      /*flagMask=*/InstanceFlag::CastsShadow);
}

// Walk `instances` (sorted by shaderID, meshID in CollectInstances) and merge consecutive
// eligible same-mesh entries into one CmdDrawIndexed with instanceCount = run length.
// firstInstance = run start; the bindless VS reads gInstances[NRI_INSTANCE_ID_OFFSET] which
// resolves to firstInstance + SV_InstanceID, so each draw covers instances [start..start+N).
// Used by both pass bodies in the CPU-driven path (useGpuDrawGeneration == false).
static void CmdDrawInstanceBucket(
        const NRIInterface& NRI,
        nri::CommandBuffer& commandBuffer,
        const Array<Instance, FreeListAllocator>& instances,
        uint32 shaderBucket,
        bool shadowPass) {
    auto eligible = [&](const Instance& inst) {
        return shadowPass
            ? (inst.flags & InstanceFlag::CastsShadow) != 0
            : inst.shaderID == shaderBucket;
    };

    const uint32 count = instances.GetSize();
    uint32 i = 0;
    while (i < count) {
        if (!eligible(instances[i])) {
            ++i;
            continue;
        }

        const uint32 runStart = i;
        const uint32 runMeshID = instances[i].meshID;
        uint32 j = i + 1;
        while (j < count && eligible(instances[j]) && instances[j].meshID == runMeshID) {
            ++j;
        }

        const MeshDesc& m = gMeshDescs[runMeshID];
        NRI.CmdDrawIndexed(commandBuffer, {m.indexCount, j - runStart, m.firstIndex, m.baseVertex, runStart});
        i = j;
    }
}

void RenderingSystem::RecordShadowPassBody(nri::CommandBuffer& commandBuffer) {

    constexpr nri::Viewport viewport = {0.0f, 0.0f, static_cast<float>(SHADOW_RESOLUTION_WIDTH), static_cast<float>(SHADOW_RESOLUTION_WIDTH), 0.0f, 1.0f};
    NRI.CmdSetViewports(commandBuffer, &viewport, 1);

    constexpr nri::Rect scissor = {0, 0, SHADOW_RESOLUTION_WIDTH, SHADOW_RESOLUTION_WIDTH};
    NRI.CmdSetScissors(commandBuffer, &scissor, 1);

    NRI.CmdSetPipelineLayout(commandBuffer, nri::BindPoint::GRAPHICS, *pipelineLayout);

    nri::SetDescriptorSetDesc globalSet = {GLOBAL_DESCRIPTOR_SET, descriptorSets[frameContext.queuedFrameIndex]};
    NRI.CmdSetDescriptorSet(commandBuffer, globalSet);

    NRI.CmdSetPipeline(commandBuffer, *pipelines[SHADOW_PIPELINE]);

    if (settings.graphicsAPI == nri::GraphicsAPI::D3D12) {
        NRI.CmdSetDepthBias(commandBuffer, {frameContext.constantBias, 0.f, frameContext.slopeBias});
    }

    NRI.CmdSetIndexBuffer(commandBuffer, *buffers[MESH_INDEX_BUFFER], 0, nri::IndexType::UINT32);
    nri::VertexBufferDesc sharedVB = {.buffer = buffers[MESH_VERTEX_BUFFER], .offset = 0, .stride = sizeof(VertexNormal)};
    NRI.CmdSetVertexBuffers(commandBuffer, 0, &sharedVB, 1);

    if (useGpuDrawGeneration) {
        NRI.CmdDrawIndexedIndirect(
            commandBuffer,
            *buffers[INDIRECT_ARGS_BUFFER],
            ShaderBucket::Shadow * MAX_DRAWS_PER_BUCKET * GetDrawIndexedCommandSize(),
            MAX_DRAWS_PER_BUCKET,
            GetDrawIndexedCommandSize(),
            buffers[DRAW_COUNT_BUFFER],
            ShaderBucket::Shadow * sizeof(uint32));
    } else {
        // Shadow pass: draw every instance that casts a shadow, regardless of opaque/transparent bucket.
        CmdDrawInstanceBucket(NRI, commandBuffer, instances, /*shaderBucket=*/0, /*shadowPass=*/true);
    }
}

void RenderingSystem::RecordMainPassBody(nri::CommandBuffer& commandBuffer) {

    const nri::Viewport viewport = {0.0f, 0.0f, static_cast<float>(windowWidth), static_cast<float>(windowHeight), 0.0f, 1.0f};
    NRI.CmdSetViewports(commandBuffer, &viewport, 1);

    const nri::Rect scissor = {0, 0, static_cast<nri::Dim_t>(windowWidth), static_cast<nri::Dim_t>(windowHeight)};
    NRI.CmdSetScissors(commandBuffer, &scissor, 1);

    NRI.CmdSetPipelineLayout(commandBuffer, nri::BindPoint::GRAPHICS, *pipelineLayout);

    nri::SetDescriptorSetDesc globalSet = {GLOBAL_DESCRIPTOR_SET, descriptorSets[frameContext.queuedFrameIndex]};
    NRI.CmdSetDescriptorSet(commandBuffer, globalSet);

    nri::SetDescriptorSetDesc shadowsSet = {SHADOWS_DESCRIPTOR_SET, descriptorSets[GetQueuedFrameNum()]};
    NRI.CmdSetDescriptorSet(commandBuffer, shadowsSet);

    NRI.CmdSetIndexBuffer(commandBuffer, *buffers[MESH_INDEX_BUFFER], 0, nri::IndexType::UINT32);
    nri::VertexBufferDesc sharedVB = {.buffer = buffers[MESH_VERTEX_BUFFER], .offset = 0, .stride = sizeof(VertexNormal)};
    NRI.CmdSetVertexBuffers(commandBuffer, 0, &sharedVB, 1);

    auto drawBucketIndirect = [&](uint32 bucket) {
        NRI.CmdDrawIndexedIndirect(
            commandBuffer,
            *buffers[INDIRECT_ARGS_BUFFER],
            bucket * MAX_DRAWS_PER_BUCKET * GetDrawIndexedCommandSize(),
            MAX_DRAWS_PER_BUCKET,
            GetDrawIndexedCommandSize(),
            buffers[DRAW_COUNT_BUFFER],
            bucket * sizeof(uint32));
    };

    // Opaque
    NRI.CmdSetPipeline(commandBuffer, *pipelines[OPAQUE_PIPELINE]);
    if (useGpuDrawGeneration) {
        drawBucketIndirect(ShaderBucket::Opaque);
    } else {
        CmdDrawInstanceBucket(NRI, commandBuffer, instances, ShaderBucket::Opaque, /*shadowPass=*/false);
    }

    // Transparent
    NRI.CmdSetPipeline(commandBuffer, *pipelines[TRANSPARENT_PIPELINE]);
    if (useGpuDrawGeneration) {
        drawBucketIndirect(ShaderBucket::Transparent);
    } else {
        CmdDrawInstanceBucket(NRI, commandBuffer, instances, ShaderBucket::Transparent, /*shadowPass=*/false);
    }
}

void RenderingSystem::SubmitFrame(
    TempArray<nri::CommandBuffer*>& commandBuffers,
    const SwapChainTexture& swapChainTexture,
    nri::Fence* acquireSemaphore) {

    {
        TIME_SCOPE(NameString("Renderer_Submit"));

        nri::FenceSubmitDesc textureAcquiredFence = {
            .fence = acquireSemaphore,
            .stages = nri::StageBits::COLOR_ATTACHMENT,
        };
        nri::FenceSubmitDesc renderingFinishedFence = {.fence = swapChainTexture.releaseSemaphore};

        nri::QueueSubmitDesc queueSubmitDesc = {
            .waitFences = &textureAcquiredFence,
            .waitFenceNum = 1,
            .commandBuffers = commandBuffers.GetData(),
            .commandBufferNum = commandBuffers.GetSize(),
            .signalFences = &renderingFinishedFence,
            .signalFenceNum = 1,
        };
        NRI.QueueSubmit(*graphicsQueue, queueSubmitDesc);
    }

#if WITH_EDITOR
    NRI.EndStreamerFrame(*imguiStreamer);
#endif
    NRI.EndStreamerFrame(*gameStreamer);

    // Present
    {
        TIME_SCOPE(NameString("Renderer_Present"));
        NRI.QueuePresent(*swapChain, *swapChainTexture.releaseSemaphore);
    }

    { // Signaling after "Present" improves D3D11 performance a bit
        TIME_SCOPE(NameString("Renderer_Signaling_Present"));

        nri::FenceSubmitDesc signalFence = {
            .fence = frameFence,
            .value = 1 + frameIndex,
        };
        nri::QueueSubmitDesc queueSubmitDesc = {
            .signalFences = &signalFence,
            .signalFenceNum = 1,
        };
        NRI.QueueSubmit(*graphicsQueue, queueSubmitDesc);
    }
}

void RenderingSystem::Update(ThreadContext& threadContext, Vault& vault) {

    static float constantBias = SHADOW_CONSTANT_DEPTH_BIAS;
    static float slopeBias = SHADOW_SLOPE_DEPTH_BIAS;
    static float shadowProjectionSize = SHADOW_PROJECTION_SIZE;
    static bool multihreadedRecording = true;

    static nri::Fence* swapChainAcquireSemaphore = nullptr;
    static SwapChainTexture* swapChainTexture = nullptr;

    if (threadContext.IsMainThread()) {
        TryResizeSwapChain();

#if WITH_EDITOR
        if (ImGui::BeginMainMenuBar()) {
            OnMainMenu();
            ImGui::EndMainMenuBar();
        }

        static bool showDemoWindow = false;
        if (ImGui::MainMenuBarItem("Help", "ImGuiDemo", "", &showDemoWindow)) {
            ImGui::ShowDemoWindow(&showDemoWindow);
        }

        static bool showRenderingData = false;
        if (Input::IsKeyHeld(KeyCode::LControl) && Input::IsKeyPressed(KeyCode::Three)) {
            showRenderingData = !showRenderingData;
        }

        if (ImGui::MainMenuBarItem("Stats", "Rendering", "CTRL+7", &showRenderingData))
        {
            nri::PipelineStatisticsDesc* pipelineStats = static_cast<nri::PipelineStatisticsDesc*>(
                    NRI.MapBuffer(*buffers[READBACK_BUFFER], 0, sizeof(nri::PipelineStatisticsDesc)));

            ImGui::Begin("Rendering Stats", &showRenderingData);
            {
                const char* apiNames[3] = {
                    "D3D11",
                    "D3D12",
                    "Vulkan"
                };
                ImGui::Text("GAPI: %s", apiNames[static_cast<size_t>(settings.graphicsAPI) - 1]);
                ImGui::Text("Input vertices               : %llu", pipelineStats->inputVertexNum);
                ImGui::Text("Input primitives             : %llu", pipelineStats->inputPrimitiveNum);
                ImGui::Text("Vertex shader invocations    : %llu", pipelineStats->vertexShaderInvocationNum);
                ImGui::Text("Rasterizer input primitives  : %llu", pipelineStats->rasterizerInPrimitiveNum);
                ImGui::Text("Rasterizer output primitives : %llu", pipelineStats->rasterizerOutPrimitiveNum);
                ImGui::Text("Fragment shader invocations  : %llu", pipelineStats->fragmentShaderInvocationNum);
                ImGui::Text("Memory Used: %d Kb", allocator.GetUsedMemory() / 1024);
                ImGui::Text("Memory Free: %d kb", allocator.GetFreeMemory() / 1024);
                ImGui::Text("Instances this frame: %u", instances.GetSize());
                ImGui::Checkbox("Use GPU draw generation", &useGpuDrawGeneration);
                ImGui::SliderFloat("Constant Bias", &frameContext.constantBias, 0.f, 30.f);
                ImGui::SliderFloat("Slope Bias", &frameContext.slopeBias, 0.f, 10.f);
                ImGui::SliderFloat("Shadow Projection Size", &shadowProjectionSize, 10.f, 1000.f);

                ImGui::Checkbox("Interpolate With Physics", &frameContext.interpolate);
                ImGui::Checkbox("Multithreaded Command Recording", &multihreadedRecording);
            }
            ImGui::End();

            NRI.UnmapBuffer(*buffers[READBACK_BUFFER]);
        }
#endif

         Camera* activeCamera = gCamera;

        if (!activeCamera) {
            activeCamera = &defaultCamera;
            gCamera = activeCamera;
        }

#if WITH_EDITOR
        if (gDetachedFromGame && gEditorCamera) {
            activeCamera = &gEditorCamera->camera;
        }
#endif

        activeCamera->SetViewportSize(windowWidth, windowHeight);
        activeCamera->Update();

#if WITH_EDITOR
        ImGui::EndFrame();
        ImGui::Render();
#endif

        LatencySleep();

        const uint32 queuedFrameIndex = frameIndex % GetQueuedFrameNum();

        // Acquire a swap chain texture
        const uint32 recycledSemaphoreIndex = frameIndex % swapChainTextures.GetSize();
        swapChainAcquireSemaphore = swapChainTextures[recycledSemaphoreIndex].acquireSemaphore;

        uint32 currentSwapChainTextureIndex = 0;
        NRI.AcquireNextTexture(*swapChain, *swapChainAcquireSemaphore, currentSwapChainTextureIndex);

        swapChainTexture = &swapChainTextures[currentSwapChainTextureIndex];

        frameContext.queuedFrameIndex = queuedFrameIndex;
        frameContext.constantBias = constantBias;
        frameContext.slopeBias = slopeBias;

        // Host-side constant-buffer update — pure Map/memcpy/Unmap on a HOST_UPLOAD buffer, no
        // GPU commands. Must run before the graph executes so passes see this frame's view/proj.
        UpdateConstantBuffer(queuedFrameIndex, activeCamera, shadowProjectionSize);

        // Hand the rotating swapchain backbuffer to the FrameGraph and execute the baked passes.
        TextureViews swapchainViews = {};
        swapchainViews.colorAttachment = swapChainTexture->colorAttachment;
        frameGraph->SetImport(swapchainHandle, swapChainTexture->texture, swapchainViews);
    }
    threadContext.Sync();

    CollectInstances(threadContext, vault);

    frameGraph->RecordPasses(threadContext, perThreadQueuedFrames, descriptorPool, frameContext.queuedFrameIndex, multihreadedRecording);

    if (threadContext.IsMainThread()) {

        TempArray<nri::CommandBuffer*> frameCommandBuffers(frameGraph->GetPassCount());

        if (multihreadedRecording) {
            frameGraph->GetCommandBuffers(frameCommandBuffers);
        } else {
            frameCommandBuffers.Add(perThreadQueuedFrames[0].queuedFrames[frameContext.queuedFrameIndex].commandBuffers[0]);
        }

        //if (queryPool) {
        //    NRI.CmdEndQuery(commandBuffer, *queryPool, queuedFrameIndex);
        //    if (frameIndex >= GetQueuedFrameNum())
        //        NRI.CmdCopyQueries(commandBuffer, *queryPool, nextQueuedFrameIndex, 1, *buffers[READBACK_BUFFER], 0);
        //}

        SubmitFrame(frameCommandBuffers, *swapChainTexture, swapChainAcquireSemaphore);

        ++frameIndex;
    }
    threadContext.Sync();
}

#if WITH_EDITOR
void RenderingSystem::BeginImGuiFrame() {

    ASSERT(imguiRenderer != nullptr, "Failed to begin editor frame, ImGuiRenderer missing!")

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float) windowWidth, (float) windowHeight);

    static double imguiTimeStampPrev = 0;
    io.DeltaTime = (float) (glfwGetTime() - imguiTimeStampPrev);
    imguiTimeStampPrev = glfwGetTime();

    // Update key modifiers
    io.AddKeyEvent(ImGuiMod_Ctrl, Input::IsKeyPressed(KeyCode::LControl) || Input::IsKeyPressed(KeyCode::RControl));
    io.AddKeyEvent(ImGuiMod_Shift, Input::IsKeyPressed(KeyCode::LShift) || Input::IsKeyPressed(KeyCode::RShift));
    io.AddKeyEvent(ImGuiMod_Alt, Input::IsKeyPressed(KeyCode::LAlt) || Input::IsKeyPressed(KeyCode::RAlt));

    // Update mouse cursor
    if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) == 0 &&
        glfwGetInputMode(glfwWindow, GLFW_CURSOR) == GLFW_CURSOR_NORMAL) {
        ImGuiMouseCursor cursor = ImGui::GetMouseCursor();
        if (cursor == ImGuiMouseCursor_None || io.MouseDrawCursor) {
            // Hide OS mouse cursor if Imgui is drawing it or wants no cursor
            CursorMode(glfwWindow, GLFW_CURSOR_HIDDEN);
        } else {
            // Show OS mouse cursor
            glfwSetCursor(glfwWindow,
                          mouseCursors[cursor] ? mouseCursors[cursor] : mouseCursors[ImGuiMouseCursor_Arrow]);
            CursorMode(glfwWindow, GLFW_CURSOR_NORMAL);
        }
    }

    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
}

#endif
