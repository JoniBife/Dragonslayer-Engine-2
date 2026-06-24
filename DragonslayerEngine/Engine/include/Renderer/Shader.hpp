#pragma once

#include <NRI.h>
#include <array>
#include <string>

#include "Core/Allocators/FreeListAllocator.hpp"
#include "Core/Containers/Array.hpp"
#include "RendererCore.hpp"

typedef Array<Array<uint8, FreeListAllocator>, FreeListAllocator> ShaderCodeStorage;

struct Shader {
    const char* ext;
    nri::StageBits stage;
};

constexpr std::array<Shader, 13> gShaderExts = {{
    {"", nri::StageBits::NONE},
    {".vs.", nri::StageBits::VERTEX_SHADER},
    {".tcs.", nri::StageBits::TESS_CONTROL_SHADER},
    {".tes.", nri::StageBits::TESS_EVALUATION_SHADER},
    {".gs.", nri::StageBits::GEOMETRY_SHADER},
    {".fs.", nri::StageBits::FRAGMENT_SHADER},
    {".cs.", nri::StageBits::COMPUTE_SHADER},
    {".rgen.", nri::StageBits::RAYGEN_SHADER},
    {".rmiss.", nri::StageBits::MISS_SHADER},
    {"<noimpl>", nri::StageBits::INTERSECTION_SHADER},
    {".rchit.", nri::StageBits::CLOSEST_HIT_SHADER},
    {".rahit.", nri::StageBits::ANY_HIT_SHADER},
    {"<noimpl>", nri::StageBits::CALLABLE_SHADER},
}};

FORCE_INLINE const char* GetShaderExt(nri::GraphicsAPI graphicsAPI) {
    if (graphicsAPI == nri::GraphicsAPI::D3D11)
        return ".dxbc";
    else if (graphicsAPI == nri::GraphicsAPI::D3D12)
        return ".dxil";

    return ".spirv";
}

static nri::ShaderDesc LoadShader(nri::GraphicsAPI graphicsAPI, const PathString& shaderName, ShaderCodeStorage& storage, const char* entryPointName = nullptr) {
    const char* ext = GetShaderExt(graphicsAPI);
    PathString path = GetFullPath(shaderName + ext, DataFolder::SHADERS);
    nri::ShaderDesc shaderDesc = {};

    size_t i = 1;
    for (; i < gShaderExts.size(); i++) {
        if (path.Contains(gShaderExts[i].ext)) {

            Array<uint8, FreeListAllocator>& code = storage.Emplace(128, storage.GetAllocator());

            if (LoadFileBytes(path, code)) {
                shaderDesc.stage = gShaderExts[i].stage;
                shaderDesc.bytecode = code.GetData();
                shaderDesc.size = code.GetSize();
                shaderDesc.entryPointName = entryPointName;
            }

            break;
        }
    }

    if (i == gShaderExts.size()) {
        printf("ERROR: Shader '%s' has invalid shader extension!\n", shaderName.CStr());

        NRI_ABORT_ON_FALSE(false);
    };

    return shaderDesc;
}
