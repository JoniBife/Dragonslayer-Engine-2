#include "Framework/TestFramework.hpp"
#include "Core/Allocators/LinearAllocator.hpp"

TEST(LinearAllocator, ConstructFromBuffer) {
    alignas(16) uint8 buffer[1024];
    LinearAllocator alloc(buffer, 1024);

    TEST_CHECK_EQUAL(alloc.GetTotalMemory(), 1024u);
    TEST_CHECK_EQUAL(alloc.GetUsedMemory(), 0u);
    TEST_CHECK_EQUAL(alloc.GetFreeMemory(), 1024u);
}

TEST(LinearAllocator, Allocate) {
    alignas(16) uint8 buffer[1024];
    LinearAllocator alloc(buffer, 1024);

    uint8* block = alloc.Allocate(64);
    TEST_REQUIRE(block != nullptr);
    TEST_CHECK(alloc.GetUsedMemory() >= 64u);
    TEST_CHECK(alloc.GetFreeMemory() <= 1024u - 64u);
}

TEST(LinearAllocator, MultipleAllocations) {
    alignas(16) uint8 buffer[1024];
    LinearAllocator alloc(buffer, 1024);

    uint8* a = alloc.Allocate(128);
    uint8* b = alloc.Allocate(128);
    uint8* c = alloc.Allocate(128);

    TEST_REQUIRE(a != nullptr);
    TEST_REQUIRE(b != nullptr);
    TEST_REQUIRE(c != nullptr);

    // Blocks should not overlap
    TEST_CHECK(b >= a + 128);
    TEST_CHECK(c >= b + 128);
}

TEST(LinearAllocator, TypedAllocate) {
    alignas(16) uint8 buffer[1024];
    LinearAllocator alloc(buffer, 1024);

    int32* val = alloc.Allocate<int32>(42);
    TEST_REQUIRE(val != nullptr);
    TEST_CHECK_EQUAL(*val, 42);
}

TEST(LinearAllocator, Reset) {
    alignas(16) uint8 buffer[1024];
    LinearAllocator alloc(buffer, 1024);

    uint8* a = alloc.Allocate(256);
    uint8* b = alloc.Allocate(256);
    TEST_REQUIRE(a != nullptr);
    TEST_REQUIRE(b != nullptr);
    TEST_CHECK(alloc.GetUsedMemory() >= 512u);

    alloc.Clear();
    TEST_CHECK_EQUAL(alloc.GetUsedMemory(), 0u);
    TEST_CHECK_EQUAL(alloc.GetFreeMemory(), 1024u);
}

TEST(LinearAllocator, AllocateUntilFull) {
    alignas(16) uint8 buffer[256];
    LinearAllocator alloc(buffer, 256);

    // Fill the allocator
    uint8* block = alloc.Allocate(256);
    TEST_REQUIRE(block != nullptr);
    TEST_CHECK_EQUAL(alloc.GetFreeMemory(), 0u);
}

TEST(LinearAllocator, FreeIsNoOp) {
    alignas(16) uint8 buffer[1024];
    LinearAllocator alloc(buffer, 1024);

    uint8* block = alloc.Allocate(128);
    size_t usedBefore = alloc.GetUsedMemory();

    alloc.Free(block);
    // Free on a linear allocator is a no-op; memory stays used
    TEST_CHECK_EQUAL(alloc.GetUsedMemory(), usedBefore);
}

TEST(LinearAllocator, TryExtendAlwaysFails) {
    alignas(16) uint8 buffer[1024];
    LinearAllocator alloc(buffer, 1024);

    uint8* block = alloc.Allocate(64);
    bool extended = alloc.TryExtend(block, 128);
    TEST_CHECK_FALSE(extended);
}
