#pragma once

#include "Core/Export.hpp"
#include "ECS/System.hpp"

// NRI: core & common extensions
#include <NRI.h>

#include <Extensions/NRIDeviceCreation.h>
#include <Extensions/NRIHelper.h>
#include <Extensions/NRIImgui.h>
#include <Extensions/NRILowLatency.h>
#include <Extensions/NRIMeshShader.h>
#include <Extensions/NRIRayTracing.h>
#include <Extensions/NRIStreamer.h>
#include <Extensions/NRISwapChain.h>
#include <Extensions/NRIUpscaler.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#if WITH_EDITOR
#include <imgui.h>
#endif

#include "Core/Containers/ArrayHelper.hpp"

#include "Camera.hpp"
#include "Components/PrimitiveRenderer.hpp"
#include "RendererCore.hpp"
#include "FrameGraph/FrameGraph.hpp"

// TODO Don't include everything!
// Include everything for demo purposes
struct NRIInterface
    : public nri::CoreInterface,
      public nri::HelperInterface,
      public nri::LowLatencyInterface,
      public nri::MeshShaderInterface,
      public nri::RayTracingInterface,
      public nri::StreamerInterface,
      public nri::SwapChainInterface,
      public nri::UpscalerInterface {

    NO_DISCARD FORCE_INLINE bool HasCore() const {
        return GetDeviceDesc != nullptr;
    }

    NO_DISCARD FORCE_INLINE bool HasHelper() const {
        return CalculateAllocationNumber != nullptr;
    }

    NO_DISCARD FORCE_INLINE bool HasLowLatency() const {
        return SetLatencySleepMode != nullptr;
    }

    NO_DISCARD FORCE_INLINE bool HasMeshShader() const {
        return CmdDrawMeshTasks != nullptr;
    }

    NO_DISCARD FORCE_INLINE bool HasRayTracing() const {
        return CreateRayTracingPipeline != nullptr;
    }

    NO_DISCARD FORCE_INLINE bool HasStreamer() const {
        return CreateStreamer != nullptr;
    }

    NO_DISCARD FORCE_INLINE bool HasSwapChain() const {
        return CreateSwapChain != nullptr;
    }

    NO_DISCARD FORCE_INLINE bool HasUpscaler() const {
        return CreateUpscaler != nullptr;
    }
};

struct RendererSettings {
    size_t usedMemory;
    nri::GraphicsAPI graphicsAPI;
    bool enableVSync;
    bool debugAPI;
    bool debugNRI;
};

/**
 * TODO Revisit projection matrix and view matrix
 * Currently its mixing opengl and D3D12 conventions including depth
 * As well as the type of matrices, currently mine are rowmajor
 * The whole camera topic is a mess right now
 *
 * TODO Some resources are tracked separately (e.g. command buffer)
 * To avoid leaks we should track them globally, e.g. list of all buffers, textures etc...
 *
 * TODO Global textures and buffers should be imported to framegraph automatically when created
 * thus we could create our own texture creation function to help!
 */
class ENGINE_API RenderingSystem final {

    GENERATE_SYSTEM_BODY(RenderingSystem)

private:
    RendererSettings settings;

    FreeListAllocator allocator;

    nri::AllocationCallbacks allocationCallbacks = {};
    nri::CallbackInterface messageCallbacks = {};

    NRIInterface NRI = {};
    nri::Device* device = nullptr;
    nri::SwapChain* swapChain = nullptr;
    nri::Queue* graphicsQueue = nullptr;
    nri::Fence* frameFence = nullptr;
    nri::DescriptorPool* descriptorPool = nullptr;
    nri::PipelineLayout* pipelineLayout = nullptr;

    nri::Descriptor* depthAttachment = nullptr;
    nri::Descriptor* depthAttachmentShadowsWrite = nullptr;
    nri::Descriptor* depthAttachmentShadowsRead = nullptr;
    nri::Descriptor* shadowMapSampler = nullptr;
    nri::QueryPool* queryPool = nullptr;

    Array<PerThreadQueueFrames, FreeListAllocator> perThreadQueuedFrames;
    Array<nri::Pipeline*, FreeListAllocator> pipelines;
    Array<SwapChainTexture, FreeListAllocator> swapChainTextures;
    Array<nri::DescriptorSet*, FreeListAllocator> descriptorSets;
    Array<nri::Texture*, FreeListAllocator> textures;
    Array<nri::Buffer*, FreeListAllocator> buffers;
    Array<nri::Descriptor*, FreeListAllocator> descriptors;
    Array<nri::Memory*, FreeListAllocator> memoryAllocations;
    nri::Memory* depthTextureMemory = nullptr;

    GLFWwindow* glfwWindow = nullptr;

    nri::Format depthFormat = nri::Format::UNKNOWN;

    uint32_t adapterIndex = 0;

    uint32_t frameIndex = 0;

#if WITH_EDITOR
    nri::ImguiInterface imgui = {};
    nri::Imgui* imguiRenderer = nullptr;
    nri::Streamer* imguiStreamer = nullptr;
    GLFWcursor* mouseCursors[ImGuiMouseCursor_COUNT] = {};
#endif

    // Game-data streamer: per-frame Instance[] uploads (and any future transient game data).
    // Separate from imguiStreamer so the editor's bursty allocations don't share a ring with
    // the steady per-frame game data
    nri::Streamer* gameStreamer = nullptr;
    nri::BufferOffset instanceBufferOffset = {};

    // Per-frame instance data (CPU-side, rebuilt every frame by CollectInstances).
    // Streamed into gameStreamer's ring with dstBuffer=INSTANCE_BUFFER; the StreamInstances
    // FrameGraph pass records CmdCopyStreamedData which performs the actual copy.
    // BuildDrawsCS + bindless VS read INSTANCE_BUFFER's SRV (instancesView).
    Array<Instance, FreeListAllocator> instances;
    uint32 numInstances = 0;

    // GPU-driven draw generation
    nri::PipelineLayout* generateDrawCommandsLayout = nullptr;
    nri::DescriptorSet* generateDrawCommandsDescriptorSet = nullptr; // single set; resources are persistent
    nri::Descriptor* instancesView = nullptr;     // SRV of INSTANCE_BUFFER
    nri::Descriptor* meshDescsView = nullptr;     // SRV of MESH_DESC_BUFFER
    nri::Descriptor* indirectArgsView = nullptr;  // STORAGE_BYTE_ADDRESS view of INDIRECT_ARGS_BUFFER
    nri::Descriptor* drawCountView = nullptr;     // STORAGE_STRUCTURED view of DRAW_COUNT_BUFFER

    // Phase 7 toggle: when false, the CPU walks `instances` and issues per-draw CmdDrawIndexed
    // (with firstInstance = instance index → bindless VS reads via SV_StartInstanceLocation).
    // When true (Phase 8), BuildDrawsCS generates indirect args + we use CmdDrawIndexedIndirect.
    bool useGpuDrawGeneration = true;

    Camera defaultCamera;

    FrameGraph* frameGraph = nullptr;
    BufferHandle instanceBufferHandle;
    BufferHandle meshDescBufferHandle;
    BufferHandle indirectArgsBufferHandle;
    BufferHandle drawCountBufferHandle;
    TextureHandle swapchainHandle;
    TextureHandle depthHandle;
    TextureHandle shadowMapHandle;

    FrameContext frameContext;

public:
    RenderingSystem(const RendererSettings& settings);
    // Initialization is separated from constructor where it can be performed by multiple threads
    void Initialize(ThreadContext& threadContext);
    ~RenderingSystem();

    void Update(ThreadContext& threadContext, Vault& vault);

#if WITH_EDITOR
    void BeginImGuiFrame();
#endif

private:

    void InitNRICallbacks();
    void InitDevice();
    void InitSwapChain(const class Window& window);
    void CreateCommandAllocators();
    void CreateCommandBuffers(uint32 maxPassesPerThread);
    void CreateTextures();
    void InitPipelineLayout();
    void InitPipelines();
    void InitBuffers();
    void InitMemory();
    void InitDescriptors();
    void UploadGPUData();
    void InitQueryPool();

    void LatencySleep();

    bool TryResizeSwapChain();
    void CreateSwapChainTextures(nri::Texture* const* nriTextures, uint32_t count, nri::Format format);
    void UpdateConstantBuffer(uint32_t queuedFrameIndex, const Camera* camera, float shadowProjectionSize);

    void BuildFrameGraph();

    void CountInstances(Vault& vault);
    void CollectInstances(Vault& vault);

    void RecordBuildDrawCommandsPass(nri::CommandBuffer& commandBuffer);
    void RecordShadowPassBody(nri::CommandBuffer& commandBuffer);
    void RecordMainPassBody(nri::CommandBuffer& commandBuffer);

    void SubmitFrame(
        TempArray<nri::CommandBuffer*>& commandBuffers,
        const SwapChainTexture& swapChainTexture,
        nri::Fence* acquireSemaphore);

#if WITH_EDITOR
    bool InitImguiRenderer(nri::Device& device);
    void DestroyImguiRenderer();
    void CmdCopyImguiData(nri::CommandBuffer& commandBuffer, nri::Streamer& streamer);
    void CmdDrawImgui(nri::CommandBuffer& commandBuffer, nri::Format attachmentFormat, float sdrScale, bool isSrgb);
#endif

    NO_DISCARD FORCE_INLINE uint8 GetQueuedFrameNum() const {
        return settings.enableVSync ? 2 : 3;
    }

    NO_DISCARD FORCE_INLINE uint8 GetOptimalSwapChainTextureNum() const {
        return GetQueuedFrameNum() + 1;
    }

    NO_DISCARD FORCE_INLINE uint32 GetDrawIndexedCommandSize() const {
        return settings.graphicsAPI == nri::GraphicsAPI::VK ? sizeof(nri::DrawIndexedDesc) : sizeof(nri::DrawIndexedBaseDesc); // sizeof(nri::DrawIndexedDesc) can be used if VS is compiled with SM 6.8
    }
};
