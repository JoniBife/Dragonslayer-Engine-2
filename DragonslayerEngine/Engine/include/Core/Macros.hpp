#pragma once

#include "Platform.hpp"

// Debug breakpoint
#if DS_PLATFORM_WINDOWS
    #define BREAKPOINT __debugbreak()
#elif DS_PLATFORM_LINUX
    #define BREAKPOINT __asm volatile ("int $0x3")
#endif

// Define force inline macro
#if DS_COMPILER_MSVC
    #define FORCE_INLINE __forceinline
#elif DS_COMPILER_CLANG || DS_COMPILER_GCC
    #define FORCE_INLINE __inline__ __attribute__((always_inline))
#endif

// No discard
#define NO_DISCARD [[nodiscard]]

// Unused to skip "unused" compiler warnings
#define MAYBE_UNUSED [[maybe_unused]]

// Indicates parameter or variable is unused
#define UNUSED(x) void(x)

// Returns size of container cast to unsigned integer
#define SIZE(container) static_cast<uint_32>(std::size(container))
