#pragma once

#include <cstdlib>
#include "Assert.hpp"
#include "Platform.hpp"
#include "Types.hpp"

#define Gb(X) (size_t(X) * 1024ULL * 1024ULL * 1024ULL)

#define Mb(X) (size_t(X) * 1024ULL * 1024ULL)

#define Kb(X) (size_t(X) * 1024ULL)

template <typename T>
constexpr bool IsPowerOf2(T x)
{
    return x > 0 && (x & (x - 1)) == 0;
}

template <typename T>
FORCE_INLINE constexpr T AlignUp(T x, uint64 alignment)
{
    ASSERT(IsPowerOf2(alignment));
    return static_cast<T>((static_cast<uint64>(x) + alignment - 1) & ~(alignment - 1));
}

template <typename T>
FORCE_INLINE constexpr size_t AlignmentOf(T x) {

    const uint64 ptr = reinterpret_cast<uint64>(x);

    size_t alignment = 1;
    while (ptr % (alignment << 1) == 0) {
        alignment <<= 1;
    }

   return alignment;
}

template <typename T1, typename T2>
FORCE_INLINE size_t OffsetOf(T1 T2::*member) {
    // We must ensure 'dummy' is correctly aligned for T2 to avoid Undefined Behavior
    // when we cast it and access its members.
    alignas(T2) char dummy[sizeof(T2)];
    const T2* objPtr = reinterpret_cast<const T2*>(dummy);
    const char* baseAddr = reinterpret_cast<const char*>(objPtr);
    const char* memberAddr = reinterpret_cast<const char*>(&(objPtr->*member));
    return static_cast<size_t>(memberAddr - baseAddr);
}

FORCE_INLINE void* Malloc(size_t size) {
    ASSERT(size > 0);
    return std::malloc(size);
}

FORCE_INLINE void* Realloc(void* blockAddress, MAYBE_UNUSED size_t oldSize, size_t newSize) {
    ASSERT(newSize > 0 && blockAddress != nullptr);
    return std::realloc(blockAddress, newSize);
}

FORCE_INLINE void Free(void* blockAddress) {
    ASSERT(blockAddress != nullptr);
    std::free(blockAddress);
}

FORCE_INLINE void* AlignedMalloc(size_t size, size_t alignment) {
    ASSERT(size > 0 && alignment > 0);
    ASSERT(IsPowerOf2(alignment));
    ASSERT(size % alignment == 0);
#if DS_PLATFORM_WINDOWS
    return _aligned_malloc(size, alignment);
#else
    void* blockAddress = nullptr;
    if (posix_memalign(&blockAddress, alignment, size) == 0) {
        return blockAddress;
    }
    return nullptr;
#endif
}

FORCE_INLINE void AlignedFree(void* blockAddress) {
    ASSERT(blockAddress != nullptr);
#if DS_PLATFORM_WINDOWS
    _aligned_free(blockAddress);
#else
    free(blockAddress);
#endif
}

#define OVERRIDE_NEW_DELETE() \
    FORCE_INLINE void* operator new (size_t count) { return Malloc(count); } \
    FORCE_INLINE void operator delete (void* pointer) noexcept { Free(pointer); } \
    FORCE_INLINE void* operator new[] (size_t count) { return Malloc(count); } \
    FORCE_INLINE void operator delete[] (void* pointer) noexcept { Free(pointer); } \
    FORCE_INLINE void* operator new (size_t count, std::align_val_t alignment) { return AlignedMalloc(count, static_cast<size_t>(alignment)); } \
    FORCE_INLINE void operator delete (void *pointer, UNUSED std::align_val_t alignment) noexcept { AlignedFree(pointer); } \
    FORCE_INLINE void* operator new[] (size_t count, std::align_val_t alignment) { return AlignedMalloc(count, static_cast<size_t>(alignment)); } \
    FORCE_INLINE void operator delete[] (void *pointer, UNUSED std::align_val_t alignment) noexcept { AlignedFree(pointer); }

