#pragma once

#include <NRI.h>
#include <Extensions/NRIHelper.h>

#include <detex.h>
#include <string_view>

#include "Core/Allocators/FreeListAllocator.hpp"
#include "Core/Assert.hpp"
#include "Core/Types.hpp"

typedef void* Mip;

enum class AlphaMode : uint32 {
    OPAQUE_A,
    PREMULTIPLIED_A,
    TRANSPARENT_A,
    OFF // alpha is 0 everywhere
};

static struct FormatMapping {
    uint32_t detexFormat;
    nri::Format nriFormat;
} formatTable[] = {
    // Uncompressed formats.
    {DETEX_PIXEL_FORMAT_RGB8, nri::Format::UNKNOWN},
    {DETEX_PIXEL_FORMAT_RGBA8, nri::Format::RGBA8_UNORM},
    {DETEX_PIXEL_FORMAT_R8, nri::Format::R8_UNORM},
    {DETEX_PIXEL_FORMAT_SIGNED_R8, nri::Format::R8_SNORM},
    {DETEX_PIXEL_FORMAT_RG8, nri::Format::RG8_UNORM},
    {DETEX_PIXEL_FORMAT_SIGNED_RG8, nri::Format::RG8_SNORM},
    {DETEX_PIXEL_FORMAT_R16, nri::Format::R16_UNORM},
    {DETEX_PIXEL_FORMAT_SIGNED_R16, nri::Format::R16_SNORM},
    {DETEX_PIXEL_FORMAT_RG16, nri::Format::RG16_UNORM},
    {DETEX_PIXEL_FORMAT_SIGNED_RG16, nri::Format::RG16_SNORM},
    {DETEX_PIXEL_FORMAT_RGB16, nri::Format::UNKNOWN},
    {DETEX_PIXEL_FORMAT_RGBA16, nri::Format::RGBA16_UNORM},
    {DETEX_PIXEL_FORMAT_FLOAT_R16, nri::Format::R16_SFLOAT},
    {DETEX_PIXEL_FORMAT_FLOAT_RG16, nri::Format::RG16_SFLOAT},
    {DETEX_PIXEL_FORMAT_FLOAT_RGB16, nri::Format::UNKNOWN},
    {DETEX_PIXEL_FORMAT_FLOAT_RGBA16, nri::Format::RGBA16_SFLOAT},
    {DETEX_PIXEL_FORMAT_FLOAT_R32, nri::Format::R32_SFLOAT},
    {DETEX_PIXEL_FORMAT_FLOAT_RG32, nri::Format::RG32_SFLOAT},
    {DETEX_PIXEL_FORMAT_FLOAT_RGB32, nri::Format::RGB32_SFLOAT},
    {DETEX_PIXEL_FORMAT_FLOAT_RGBA32, nri::Format::RGBA32_SFLOAT},
    {DETEX_PIXEL_FORMAT_A8, nri::Format::UNKNOWN},
    // Compressed formats.
    {DETEX_TEXTURE_FORMAT_BC1, nri::Format::BC1_RGBA_UNORM},
    {DETEX_TEXTURE_FORMAT_BC1A, nri::Format::UNKNOWN},
    {DETEX_TEXTURE_FORMAT_BC2, nri::Format::BC2_RGBA_UNORM},
    {DETEX_TEXTURE_FORMAT_BC3, nri::Format::BC3_RGBA_UNORM},
    {DETEX_TEXTURE_FORMAT_RGTC1, nri::Format::BC4_R_UNORM},
    {DETEX_TEXTURE_FORMAT_SIGNED_RGTC1, nri::Format::BC4_R_SNORM},
    {DETEX_TEXTURE_FORMAT_RGTC2, nri::Format::BC5_RG_UNORM},
    {DETEX_TEXTURE_FORMAT_SIGNED_RGTC2, nri::Format::BC5_RG_SNORM},
    {DETEX_TEXTURE_FORMAT_BPTC_FLOAT, nri::Format::BC6H_RGB_UFLOAT},
    {DETEX_TEXTURE_FORMAT_BPTC_SIGNED_FLOAT, nri::Format::BC6H_RGB_SFLOAT},
    {DETEX_TEXTURE_FORMAT_BPTC, nri::Format::BC7_RGBA_UNORM},
    {DETEX_TEXTURE_FORMAT_ETC1, nri::Format::UNKNOWN},
    {DETEX_TEXTURE_FORMAT_ETC2, nri::Format::UNKNOWN},
    {DETEX_TEXTURE_FORMAT_ETC2_PUNCHTHROUGH, nri::Format::UNKNOWN},
    {DETEX_TEXTURE_FORMAT_ETC2_EAC, nri::Format::UNKNOWN},
    {DETEX_TEXTURE_FORMAT_EAC_R11, nri::Format::UNKNOWN},
    {DETEX_TEXTURE_FORMAT_EAC_SIGNED_R11, nri::Format::UNKNOWN},
    {DETEX_TEXTURE_FORMAT_EAC_RG11, nri::Format::UNKNOWN},
    {DETEX_TEXTURE_FORMAT_EAC_SIGNED_RG11, nri::Format::UNKNOWN},
};

static nri::Format GetFormatNRI(uint32_t detexFormat) {
    for (auto& entry : formatTable) {
        if (entry.detexFormat == detexFormat)
            return entry.nriFormat;
    }

    return nri::Format::UNKNOWN;
}

static FORCE_INLINE detexTexture* ToMip(Mip mip) {
    return static_cast<detexTexture*>(mip);
}

static FORCE_INLINE detexTexture** ToTexture(Mip* mips) {
    return reinterpret_cast<detexTexture**>(mips);
}


struct Texture {
    PathString name;
    Mip* mips = nullptr;
    AlphaMode alphaMode = AlphaMode::OPAQUE_A;
    nri::Format format = nri::Format::UNKNOWN;
    uint16_t width = 0;
    uint16_t height = 0;
    uint16_t depth = 0;
    uint8_t mipNum = 0;
    uint16_t layerNum = 0;

    ~Texture() {
        detexFreeTexture(ToTexture(mips), mipNum);
    }

    bool IsBlockCompressed() const {
        return detexFormatIsCompressed(ToMip(mips[0])->format);
    }

    void GetSubresource(nri::TextureSubresourceUploadDesc& subresource, uint32_t mipIndex, uint32_t arrayIndex = 0) const {
        // TODO: 3D images are not supported, "subresource.slices" needs to be allocated to store pointers to all slices of the current mipmap
        ASSERT(GetDepth() == 1);
        (void)(arrayIndex); // TODO: unused

        detexTexture* mip = ToMip(mips[mipIndex]);

        int rowPitch, slicePitch;
        detexComputePitch(mip->format, mip->width, mip->height, &rowPitch, &slicePitch);

        subresource.slices = mip->data;
        subresource.sliceNum = 1;
        subresource.rowPitch = static_cast<uint32_t>(rowPitch);
        subresource.slicePitch = static_cast<uint32_t>(slicePitch);
    }

    inline void OverrideFormat(nri::Format fmt) {
        this->format = fmt;
    }

    inline uint16_t GetArraySize() const {
        return layerNum;
    }

    inline uint8_t GetMipNum() const {
        return mipNum;
    }

    inline uint16_t GetWidth() const {
        return width;
    }

    inline uint16_t GetHeight() const {
        return height;
    }

    inline uint16_t GetDepth() const {
        return depth;
    }

    inline nri::Format GetFormat() const {
        return format;
    }
};


static void PostProcessTexture(FreeListAllocator* allocator, const PathString& name, Texture& texture, bool computeAvgColorAndAlphaMode, detexTexture** dTexture, int mipNum) {

    texture.mips = (Mip*)dTexture;
    texture.name = name;
    texture.format = GetFormatNRI(dTexture[0]->format);
    texture.width = (uint16_t)dTexture[0]->width;
    texture.height = (uint16_t)dTexture[0]->height;
    texture.mipNum = (uint8_t)mipNum;

    // TODO: detex doesn't support cubemaps and 3D textures
    texture.layerNum = 1;
    texture.depth = 1;

    texture.alphaMode = AlphaMode::OPAQUE_A;
    if (computeAvgColorAndAlphaMode) {
        // Alpha mode
        if (texture.format == nri::Format::BC1_RGBA_UNORM || texture.format == nri::Format::BC1_RGBA_SRGB) {
            bool hasTransparency = false;
            for (int i = mipNum - 1; i >= 0 && !hasTransparency; i--) {
                const size_t size = detexTextureSize(dTexture[i]->width_in_blocks, dTexture[i]->height_in_blocks, dTexture[i]->format);
                const uint8_t* bc1 = dTexture[i]->data;

                for (size_t j = 0; j < size && !hasTransparency; j += 8) {
                    const uint16_t* c = (uint16_t*)bc1;
                    if (c[0] <= c[1]) {
                        const uint32_t bits = *(uint32_t*)(bc1 + 4);
                        for (uint32_t k = 0; k < 32 && !hasTransparency; k += 2)
                            hasTransparency = ((bits >> k) & 0x3) == 0x3;
                    }
                    bc1 += 8;
                }
            }

            if (hasTransparency)
                texture.alphaMode = AlphaMode::PREMULTIPLIED_A;
        }

        // Decompress last mip
        detexTexture* lastMip = dTexture[mipNum - 1];
        uint8* rgba8 = lastMip->data;

        if (lastMip->format != DETEX_PIXEL_FORMAT_RGBA8) {
            Array<uint8, FreeListAllocator> image = Array<uint8, FreeListAllocator>(lastMip->width * lastMip->height * detexGetPixelSize(DETEX_PIXEL_FORMAT_RGBA8), allocator);
            // TODO this is not resizing
            detexDecompressTextureLinear(lastMip, image.GetData(), DETEX_PIXEL_FORMAT_RGBA8);
            rgba8 =image.GetData();
        }

        // Average color
        Vec4 avgColor = Vec4::ZERO;
        const size_t pixelNum = lastMip->width * lastMip->height;
        for (size_t i = 0; i < pixelNum; i++)
            avgColor += Vec4(static_cast<float>(*reinterpret_cast<uint32*>(rgba8 + i * 4)));
        avgColor /= static_cast<float>(pixelNum);

        if (texture.alphaMode != AlphaMode::PREMULTIPLIED_A && avgColor.w < 254.0f / 255.0f)
            texture.alphaMode = AlphaMode::TRANSPARENT_A;

        if (texture.alphaMode == AlphaMode::TRANSPARENT_A && avgColor.w == 0.0f) {
            printf("WARNING: Texture '%s' is fully transparent!\n", name.CStr());
            texture.alphaMode = AlphaMode::OFF;
        }
    }
}

static bool LoadTexture(FreeListAllocator* allocator, const PathString& path, Texture& texture, bool computeAvgColorAndAlphaMode = false) {
    printf("Loading texture '%s'...\n", GetFileName(path));

    detexTexture** dTexture = nullptr;
    int mipNum = 0;

    if (!detexLoadTextureFileWithMipmaps(path.CStr(), 32, &dTexture, &mipNum)) {
        printf("ERROR: Can't load texture '%s'\n", path.CStr());

        return false;
    }

    PostProcessTexture(allocator, path, texture, computeAvgColorAndAlphaMode, dTexture, mipNum);

    return true;
}