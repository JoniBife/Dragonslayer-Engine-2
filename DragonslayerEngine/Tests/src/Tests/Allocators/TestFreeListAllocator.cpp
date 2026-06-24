#include "Framework/TestFramework.hpp"
#include "Core/Allocators/FreeListAllocator.hpp"
#include "Core/Memory.hpp"

static constexpr size_t BUFFER_SIZE = 4096;

TEST(FreeListAllocator, ConstructFromBuffer) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(static_cast<uint8*>(buffer), BUFFER_SIZE);

    TEST_CHECK_EQUAL(alloc.GetTotalMemory(), BUFFER_SIZE);
    TEST_CHECK_EQUAL(alloc.GetAllocatedMemory(), 0u);
}

TEST(FreeListAllocator, Allocate) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(static_cast<uint8*>(buffer), BUFFER_SIZE);

    uint8* block = alloc.Allocate(64);
    TEST_REQUIRE(block != nullptr);
    TEST_CHECK(alloc.GetAllocatedMemory() >= 64u);
}

TEST(FreeListAllocator, AllocateAndFree) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(static_cast<uint8*>(buffer), BUFFER_SIZE);

    uint8* a = alloc.Allocate(128);
    uint8* b = alloc.Allocate(128);
    TEST_REQUIRE(a != nullptr);
    TEST_REQUIRE(b != nullptr);

    size_t usedAfterAlloc = alloc.GetAllocatedMemory();

    alloc.Free(a);
    TEST_CHECK(alloc.GetAllocatedMemory() < usedAfterAlloc);

    alloc.Free(b);
    TEST_CHECK_EQUAL(alloc.GetAllocatedMemory(), 0u);
}

TEST(FreeListAllocator, TypedAllocate) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(static_cast<uint8*>(buffer), BUFFER_SIZE);

    int32* val = alloc.Allocate<int32>(42);
    TEST_REQUIRE(val != nullptr);
    TEST_CHECK_EQUAL(*val, 42);

    alloc.Free(val);
}

TEST(FreeListAllocator, ReuseFreedMemory) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(static_cast<uint8*>(buffer), BUFFER_SIZE);

    uint8* a = alloc.Allocate(128);
    alloc.Free(a);

    // Should be able to reuse freed memory
    uint8* b = alloc.Allocate(128);
    TEST_REQUIRE(b != nullptr);

    alloc.Free(b);
}

TEST(FreeListAllocator, Clear) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(static_cast<uint8*>(buffer), BUFFER_SIZE);

    uint8* a = alloc.Allocate(128);
    uint8* b = alloc.Allocate(256);
    TEST_REQUIRE(a != nullptr);
    TEST_REQUIRE(b != nullptr);
    TEST_CHECK(alloc.GetAllocatedMemory() > 0u);

    alloc.Clear();
    TEST_CHECK_EQUAL(alloc.GetAllocatedMemory(), 0u);
}

TEST(FreeListAllocator, MultipleAllocationsNoOverlap) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(static_cast<uint8*>(buffer), BUFFER_SIZE);

    uint8* a = alloc.Allocate(64);
    uint8* b = alloc.Allocate(64);
    uint8* c = alloc.Allocate(64);

    TEST_REQUIRE(a != nullptr);
    TEST_REQUIRE(b != nullptr);
    TEST_REQUIRE(c != nullptr);

    // Blocks should not overlap (accounting for possible headers)
    TEST_CHECK(b < a || b >= a + 64);
    TEST_CHECK(c < b || c >= b + 64);
    TEST_CHECK(c < a || c >= a + 64);
}

TEST(FreeListAllocator, BestFitStrategy) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(static_cast<uint8*>(buffer), BUFFER_SIZE, FreeListStrategy::BestFit);

    uint8* a = alloc.Allocate(64);
    uint8* b = alloc.Allocate(256);
    uint8* c = alloc.Allocate(64);

    TEST_REQUIRE(a != nullptr);
    TEST_REQUIRE(b != nullptr);
    TEST_REQUIRE(c != nullptr);

    // Free the middle block, creating a 256-byte gap
    alloc.Free(b);

    // Allocate something small; best fit should find an appropriate spot
    uint8* d = alloc.Allocate(32);
    TEST_REQUIRE(d != nullptr);

    alloc.Free(a);
    alloc.Free(c);
    alloc.Free(d);
}

TEST(FreeListAllocator, Reallocate) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(static_cast<uint8*>(buffer), BUFFER_SIZE);

    uint8* block = alloc.Allocate(64);
    TEST_REQUIRE(block != nullptr);

    // Write some data
    for (int i = 0; i < 64; ++i) {
        block[i] = static_cast<uint8>(i);
    }

    uint8* grown = alloc.Reallocate(block, 128);
    TEST_REQUIRE(grown != nullptr);

    // Original data should be preserved
    for (int i = 0; i < 64; ++i) {
        TEST_CHECK_EQUAL(grown[i], static_cast<uint8>(i));
    }

    alloc.Free(grown);
}
