#pragma once

#include <string_view>

#include "Export.hpp"
#include "Macros.hpp"
#include "Types.hpp"

#define FNV_1_OFFSET 2166136261
#define FNV_1_PRIME 16777619

// FNV-1a hash (Fowler–Noll–Vo hash function version 1a)
template<typename Byte>
NO_DISCARD static constexpr uint32 FNV1aHash(const Byte* bytes, const size_t length) {

    static_assert(sizeof(Byte) == 1, "Byte must be exactly 8 bits");

    uint32 hash = FNV_1_OFFSET;

    for(size_t i = 0; i < length; ++i) {
        hash ^= static_cast<uint32>(bytes[i]);
        hash *= FNV_1_PRIME;
    }

    return hash;
}

// Compile-time FNV-1a hash (Fowler–Noll–Vo hash function version 1a)
template<typename Byte, size_t N>
NO_DISCARD static constexpr uint32 FNV1aHash(const Byte (&bytes)[N]) {

    static_assert(sizeof(Byte) == 1, "Byte must be exactly 8 bits");

    uint32 hash = FNV_1_OFFSET;

    for(size_t i = 0; i < N; ++i) {
        hash ^= static_cast<uint32>(bytes[i]);
        hash *= FNV_1_PRIME;
    }

    return hash;
}

// Hashed string
NO_DISCARD static constexpr uint32 operator ""_hs(const char* str, size_t len) {
    return FNV1aHash(str, len);
}



