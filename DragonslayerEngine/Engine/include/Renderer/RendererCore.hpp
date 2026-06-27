#pragma once

#include <detex.h>

// NRI: core & common extensions
#include <NRI.h>

#include "Core/Log.hpp"
#include "Core/Allocators/FreeListAllocator.hpp"
#include "Core/Containers/Array.hpp"
#include "Core/Lock.hpp"
#include "Extensions/NRIDeviceCreation.h"
#include "Math/Mat4.hpp"
#include "Math/Vec3.hpp"

#define NRI_ABORT_ON_FAILURE(result) \
    if ((result) != nri::Result::SUCCESS) { \
        exit(1); \
    }

#define NRI_ABORT_ON_FALSE(result) \
    if (!(result)) { \
        exit(1); \
    }

namespace nri {
enum class Message : uint8_t;
}
struct SwapChainTexture {
    nri::Fence* acquireSemaphore;
    nri::Fence* releaseSemaphore;
    nri::Texture* texture;
    nri::Descriptor* colorAttachment;
    nri::Format attachmentFormat;
};

constexpr uint32 MAX_COMMAND_BUFFERS_PER_QUEUED_FRAME = 8;

struct QueuedFrame {
    InlineArray<nri::CommandBuffer*, MAX_COMMAND_BUFFERS_PER_QUEUED_FRAME> commandBuffers = {};
    nri::CommandAllocator* commandAllocator = nullptr;
    uint32_t globalConstantBufferViewOffsets = 0;
};

constexpr size_t QUEUED_FRAME_MAX_NUM = 4;
struct PerThreadQueueFrames {
    InlineArray<QueuedFrame, QUEUED_FRAME_MAX_NUM> queuedFrames = {};
};

struct FrameContext {
    uint32 queuedFrameIndex = 0;
    bool interpolate = true;
    float constantBias = 0.f;
    float slopeBias = 0.f;
};

struct GlobalConstantBufferLayout {
    Mat4 gViewToClip;
    Mat4 gLightViewToClip;
    Vec3 gCameraPos;
    float gPadding;
};

struct ConstantBufferLayout {
    float color[3];
    float scale;
};

struct Vertex {
    float position[2];
    float uv[2];
};

struct VertexNormal {
    float position[3];
    float normal[3];
};

struct RootConstantData {
    Mat4 modelMatrix;
    Mat4 normalMatrix;
    Vec4 color;
};

// Per-mesh entry into the shared vertex/index buffers.
// Mirrors what gets baked into indirect draw args (baseVertex/firstIndex).
struct MeshDesc {
    uint32 firstIndex;   // offset into shared IB, in indices
    uint32 indexCount;
    int32 baseVertex;   // offset into shared VB, in vertices
    uint32 padding;
};
static_assert(sizeof(MeshDesc) == 16, "MeshDesc must be 16 bytes (matches HLSL StructuredBuffer<MeshDesc> layout)");

// Per-instance data streamed every frame, read by the bindless VS via SV_StartInstanceLocation.
// HLSL struct must mirror this exact layout (StructuredBuffer<Instance>).
struct Instance {
    Mat4 modelMatrix;
    Mat4 normalMatrix;
    Vec4 color;
    uint32 meshID;
    uint32 materialID; // reserved for bindless material (always 0 for now)
    uint32 shaderID; // PSO bucket (0=opaque, 1=transparent, 2=shadow)
    uint32 flags; // bit 0: isVisible, bit 1: castsShadow, bit 2: isOpaque, bit 3: isTransparent
};
static_assert(sizeof(Instance) == 160, "Instance must be 160 bytes (must match HLSL StructuredBuffer<Instance>)");

// Instance flag bits.
namespace InstanceFlag {
    constexpr uint32 IsVisible = 1u << 0;
    constexpr uint32 CastsShadow = 1u << 1;
    constexpr uint32 IsOpaque = 1u << 2;
    constexpr uint32 IsTransparent = 1u << 3;
}

// PSO bucket index, matches the per-pass PSO enum in RenderingSystem.cpp.
namespace ShaderBucket {
    constexpr uint32 Opaque = 0;
    constexpr uint32 Transparent = 1;
    constexpr uint32 Shadow = 2;
    constexpr uint32 Count = 3;
}

// Maximum number of mesh entries in the shared archive.
// Matches PrimitiveType count today (Plane, Cube, Sphere, Cylinder, Capsule).
constexpr uint32 MAX_MESHES = 16;

// Maximum per-frame instance count. Drives game streamer sizing.
constexpr uint32 MAX_INSTANCES = 65536;

enum class DataFolder : uint8_t {
    ROOT,
    SHADERS,
    TEXTURES,
    SCENES,
    TESTS
};

static PathString GetFullPath(const PathString& localPath, DataFolder dataFolder) {
    PathString path;
    if (dataFolder == DataFolder::SHADERS)
        path = "Shaders/"; // special folder with generated files
    else if (dataFolder == DataFolder::TEXTURES)
        path += "Textures/";
    else if (dataFolder == DataFolder::SCENES)
        path += "Scenes/";
    else if (dataFolder == DataFolder::TESTS)
        path = "Tests/"; // special folder stored in Git

    return path + localPath;
}

static const char* GetFileName(const PathString& path) {
    //const size_t slashPos = path.find_last_of("\\/");
    //if (slashPos != std::string::npos)
    //    return path.c_str() + slashPos + 1;
//
    return "";
}

template<typename Allocator>
static bool LoadFileBytes(const PathString& path, Array<uint8, Allocator>& data) {

    File file = File::OpenOrCreate(path.CStr());

    int64 fileSize;
    if (!file.GetFileSize(fileSize)) {
        return false;
    }

    // TODO The file can technically be larger than 32bits, very unlikely tho! (Would need to be larger than 4gb)
    const uint32 fileSize32bits = static_cast<uint32>(fileSize);

    data.EmplaceMany(fileSize);

    uint32 bytesRead;
    if (!file.ReadBytes(data.GetData(), fileSize32bits, bytesRead)) {
        return false;
    }

    file.Close();

    return !data.IsEmpty() && bytesRead == fileSize32bits;
}

static SpinLock lock = SpinLock();

#define UseCustomAllocator 1

static void* NRIAllocate(void* userArg, size_t size, size_t alignment) {

#if UseCustomAllocator
    ScopeLock scopeLock(lock);
    FreeListAllocator* allocator = static_cast<FreeListAllocator*>(userArg);
    uint8* allocation = allocator->Allocate(size, alignment);
    return allocation;
#else
    return Malloc(size);
#endif

}

static void* NRIReallocate(void* userArg, void* memory, size_t size, size_t alignment) {

#if UseCustomAllocator
    ScopeLock scopeLock(lock);
    FreeListAllocator* allocator = static_cast<FreeListAllocator*>(userArg);
    uint8* allocation = allocator->Reallocate(static_cast<uint8*>(memory), size /* TODO Missing alignment*/);
    return allocation;
#else
    return Realloc(memory, size, size);
#endif
}

static void NRIFree(void* userArg, void* memory) {

    if (memory == nullptr) {
        return; // TODO Apparently this is how its handled in the C / C++ standard and NRI relies on it :(
    }

#if UseCustomAllocator
    ScopeLock scopeLock(lock);
    FreeListAllocator* allocator = static_cast<FreeListAllocator*>(userArg);
    allocator->Free(static_cast<uint8*>(memory));
#else
    return Free(memory);
#endif

}

static void NRIMessage(nri::Message messageType, const char* file, uint32_t line, const char* message, void* userArg) {

    char logString[MAX_LOG_LENGTH];
    sprintf(logString, "[NRI] %s (%d) : %s\n", file, line, message);

    switch (messageType) {
        case nri::Message::INFO: {
            Log::Info(logString);
            break;
        }
        case nri::Message::WARNING: {
            Log::Warning(logString);
            break;
        }
        case nri::Message::ERROR: {
            Log::Error(logString);
            break;
        }
        case nri::Message::MAX_NUM: {
            Log::Error(logString);
            break;
        }
    }
}

static void NRIAbortExecution(void* userArg) {
    Log::PrintLogsCurrentFrame();
    std::abort();
}

// TODO Move some of these to a separate helper file
namespace helper {
    template <typename T, typename A>
    constexpr T Align(T x, A alignment) {
        return (T)((size_t(x) + (size_t)alignment - 1) & ~((size_t)alignment - 1));
    }

    template <typename T, uint32 N>
    constexpr uint32 GetCountOf(T const (&)[N]) {
        return N;
    }

    template <typename T, typename A>
    constexpr uint32 GetCountOf(const Array<T, A>& v) {
        return v.GetSize();
    }

    template <typename T, size_t N>
    constexpr uint32 GetCountOf(const InlineArray<T, N>& v) {
        return v.GetSize();
    }

    template <typename T, typename U>
    constexpr uint32 GetOffsetOf(U T::* member) {
        return (uint32_t)((char*)&((T*)nullptr->*member) - (char*)nullptr);
    }
}

struct Annotation {
    const nri::CoreInterface& m_NRI;
    nri::CommandBuffer& m_CommandBuffer;

    inline Annotation(const nri::CoreInterface& NRI, nri::CommandBuffer& commandBuffer, const char* name)
        : m_NRI(NRI), m_CommandBuffer(commandBuffer) {
        m_NRI.CmdBeginAnnotation(m_CommandBuffer, name, nri::BGRA_UNUSED);
    }

    inline ~Annotation() {
        m_NRI.CmdEndAnnotation(m_CommandBuffer);
    }
};




