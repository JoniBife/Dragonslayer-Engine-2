#pragma once

// Determine platform
#define DS_PLATFORM_WINDOWS 0
#define DS_PLATFORM_LINUX 0

#if defined(_WIN32) || defined(_WIN64)
    #undef DS_PLATFORM_WINDOWS
    #define DS_PLATFORM_WINDOWS 1
#elif defined(__linux__)
    #undef DS_PLATFORM_LINUX
    #define DS_PLATFORM_LINUX 1
#else
    #error Unsupported platform
#endif

// Determine CPU architecture
#define DS_CPU_64_BITS 0
#define DS_CPU_32_BITS 0

#if defined(__x86_64__) || defined(_M_X64)
    #undef DS_CPU_64_BITS
    #define DS_CPU_64_BITS 1
#elif defined(__i386__) || defined(_M_IX86)
    #undef DS_CPU_32_BITS
    #define DS_CPU_32_BITS 1
#else
    #error Unsupported CPU architecture
#endif

// TODO Determine SIMD Instruction set

// Determine compiler
#define DS_COMPILER_MSVC  0
#define DS_COMPILER_CLANG 0
#define DS_COMPILER_GCC   0

#if defined(_MSC_VER)
    #undef DS_COMPILER_MSVC
    #define DS_COMPILER_MSVC 1
#elif defined(__clang__)
    #undef DS_COMPILER_CLANG
    #define DS_COMPILER_CLANG 1
#elif defined(__GNUC__)
    #undef DS_COMPILER_GCC
    #define DS_COMPILER_GCC 1
#else
    #error Unsupported compiler
#endif

#include <bit>
static_assert(std::endian::native == std::endian::little, "Dragonslayer Engine currently only supports Little Endian systems!");




