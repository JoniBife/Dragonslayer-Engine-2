 #pragma once

#include <cstdint>

#include "Platform.hpp"

// Fundamental types
using int8 = std::int8_t;
using int16 = std::int16_t;
using int32 = std::int32_t;
using int64 = std::int64_t;

using uint8 = std::uint8_t;
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;

using uintptr = std::uintptr_t;

#if DS_PLATFORM_LINUX
using size_t = std::size_t;
#endif

static_assert(sizeof(int8) == 1, "Invalid size of int8");
static_assert(sizeof(int16) == 2, "Invalid size of int16");
static_assert(sizeof(int32) == 4, "Invalid size of int32");
static_assert(sizeof(int64) == 8, "Invalid size of int64");
static_assert(sizeof(uint8) == 1, "Invalid size of uint8");
static_assert(sizeof(uint16) == 2, "Invalid size of uint16");
static_assert(sizeof(uint32) == 4, "Invalid size of uint32");
static_assert(sizeof(uint64) == 8, "Invalid size of uint64");
static_assert(sizeof(void*) == (DS_CPU_64_BITS ? 8 : 4), "Invalid size of pointer");