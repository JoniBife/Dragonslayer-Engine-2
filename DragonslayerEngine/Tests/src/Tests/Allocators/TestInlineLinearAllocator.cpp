#include <utility>
#include "Framework/TestFramework.hpp"
#include "Core/Allocators/InlineLinearAllocator.hpp"

TEST(InlineLinearAllocator, DefaultConstruction) {
    InlineLinearAllocator<256> alloc;

    TEST_CHECK_EQUAL(alloc.GetTotalMemory(), 256u);
    TEST_CHECK_EQUAL(alloc.GetUsedMemory(), 0u);
    TEST_CHECK_EQUAL(alloc.GetFreeMemory(), 256u);
}

TEST(InlineLinearAllocator, Allocate) {
    InlineLinearAllocator<512> alloc;

    uint8* block = alloc.Allocate(64, alignof(uint8));
    TEST_REQUIRE(block != nullptr);
    TEST_CHECK(alloc.GetUsedMemory() >= 64u);
}

TEST(InlineLinearAllocator, TypedAllocate) {
    InlineLinearAllocator<256> alloc;

    int32* val = alloc.Allocate<int32>(123);
    TEST_REQUIRE(val != nullptr);
    TEST_CHECK_EQUAL(*val, 123);
}

TEST(InlineLinearAllocator, MultipleAllocations) {
    InlineLinearAllocator<1024> alloc;

    uint8* a = alloc.Allocate(100, alignof(uint8));
    uint8* b = alloc.Allocate(100, alignof(uint8));
    uint8* c = alloc.Allocate(100, alignof(uint8));

    TEST_REQUIRE(a != nullptr);
    TEST_REQUIRE(b != nullptr);
    TEST_REQUIRE(c != nullptr);

    // Linear allocator: blocks are sequential
    TEST_CHECK(b >= a + 100);
    TEST_CHECK(c >= b + 100);
}

TEST(InlineLinearAllocator, Clear) {
    InlineLinearAllocator<512> alloc;

    uint8* a = alloc.Allocate(128, alignof(uint8));
    uint8* b = alloc.Allocate(128, alignof(uint8));
    TEST_REQUIRE(a != nullptr);
    TEST_REQUIRE(b != nullptr);
    TEST_CHECK(alloc.GetUsedMemory() >= 256u);

    alloc.Clear();
    TEST_CHECK_EQUAL(alloc.GetUsedMemory(), 0u);
    TEST_CHECK_EQUAL(alloc.GetFreeMemory(), 512u);
}

TEST(InlineLinearAllocator, FreeIsNoOp) {
    InlineLinearAllocator<256> alloc;

    uint8* block = alloc.Allocate(64, alignof(uint8));
    size_t usedBefore = alloc.GetUsedMemory();

    alloc.Free(block);
    TEST_CHECK_EQUAL(alloc.GetUsedMemory(), usedBefore);
}

TEST(InlineLinearAllocator, TryExtendAlwaysFails) {
    InlineLinearAllocator<256> alloc;

    uint8* block = alloc.Allocate(32, alignof(uint8));
    bool extended = alloc.TryExtend(block, 64);
    TEST_CHECK_FALSE(extended);
}

TEST(InlineLinearAllocator, AllocateMultipleTypes) {
    InlineLinearAllocator<512> alloc;

    int32* i = alloc.Allocate<int32>(42);
    float* f = alloc.Allocate<float>(3.14f);
    uint64* u = alloc.Allocate<uint64>(999);

    TEST_REQUIRE(i != nullptr);
    TEST_REQUIRE(f != nullptr);
    TEST_REQUIRE(u != nullptr);

    TEST_CHECK_EQUAL(*i, 42);
    TEST_CHECK_EQUAL(*u, 999u);
}
