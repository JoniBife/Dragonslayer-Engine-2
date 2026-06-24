
// Lightweight Lock, copied over from NRI

// © 2021 NVIDIA Corporation

#pragma once

#include <atomic>

constexpr size_t SPIN_LOCK_CACHELINE_SIZE = 64;

// Found in sse2neon
#if (defined(__arm__) || defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM))
inline void _mm_pause() {
#    if defined(_MSC_VER)
    __isb(_ARM64_BARRIER_SY);
#    else
    __asm__ __volatile__("isb\n");
#    endif
}
#else
#    include <xmmintrin.h>
#endif

// Very lightweight exclusive lock
struct alignas(SPIN_LOCK_CACHELINE_SIZE) SpinLock {
    inline SpinLock() {
        m_Atomic.store(0, std::memory_order_relaxed);
    }

    inline void Acquire() {
        while (m_Atomic.exchange(1, std::memory_order_acquire))
            _mm_pause();
    }

    inline void Release() {
        m_Atomic.store(0, std::memory_order_release);
    }

private:
    std::atomic_uint32_t m_Atomic;
};

struct ScopeLock {
    inline ScopeLock(SpinLock& lock)
        : spinLock(lock) {
        spinLock.Acquire();
    }

    inline ~ScopeLock() {
        spinLock.Release();
    }

private:
    SpinLock& spinLock;
};
