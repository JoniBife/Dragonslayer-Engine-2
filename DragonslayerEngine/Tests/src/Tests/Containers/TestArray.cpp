#include "Framework/TestFramework.hpp"
#include "Core/Allocators/FreeListAllocator.hpp"
#include "Core/Containers/Array.hpp"

// Array requires a heap-capable allocator; use FreeListAllocator with stack memory
static constexpr size_t BUFFER_SIZE = Kb(4);

TEST(Array, DefaultConstruction) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    Array<int32, FreeListAllocator> arr(8, &alloc);
    TEST_CHECK_EQUAL(arr.GetSize(), 0u);
    TEST_CHECK_EQUAL(arr.GetCapacity(), 8u);
    TEST_CHECK(arr.IsEmpty());
}

TEST(Array, AddAndAccess) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    Array<int32, FreeListAllocator> arr(4, &alloc);
    arr.Add(100);
    arr.Add(200);
    arr.Add(300);

    TEST_CHECK_EQUAL(arr.GetSize(), 3u);
    TEST_CHECK_EQUAL(arr[0], 100);
    TEST_CHECK_EQUAL(arr[1], 200);
    TEST_CHECK_EQUAL(arr[2], 300);
}

TEST(Array, InitializerList) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    Array<int32, FreeListAllocator> arr({10, 20, 30, 40}, &alloc);
    TEST_CHECK_EQUAL(arr.GetSize(), 4u);
    TEST_CHECK_EQUAL(arr[0], 10);
    TEST_CHECK_EQUAL(arr[3], 40);
}

TEST(Array, GrowBeyondCapacity) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    Array<int32, FreeListAllocator> arr(2, &alloc);
    arr.Add(1);
    arr.Add(2);
    arr.Add(3); // Should trigger growth

    TEST_CHECK_EQUAL(arr.GetSize(), 3u);
    TEST_CHECK(arr.GetCapacity() > 2u);
    TEST_CHECK_EQUAL(arr[0], 1);
    TEST_CHECK_EQUAL(arr[1], 2);
    TEST_CHECK_EQUAL(arr[2], 3);
}

TEST(Array, RemoveAt) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    Array<int32, FreeListAllocator> arr({10, 20, 30, 40}, &alloc);
    TEST_CHECK(arr.RemoveAt(1)); // Remove 20
    TEST_CHECK_EQUAL(arr.GetSize(), 3u);
    TEST_CHECK_EQUAL(arr[0], 10);
    TEST_CHECK_EQUAL(arr[1], 30);
    TEST_CHECK_EQUAL(arr[2], 40);
}

TEST(Array, RemoveAndSwapAt) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    Array<int32, FreeListAllocator> arr({10, 20, 30, 40}, &alloc);
    TEST_CHECK(arr.RemoveAndSwapAt(0)); // Remove 10, last fills gap
    TEST_CHECK_EQUAL(arr.GetSize(), 3u);
    TEST_CHECK_EQUAL(arr[0], 40);
}

TEST(Array, GetLast) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    Array<int32, FreeListAllocator> arr(4, &alloc);
    arr.Add(5);
    arr.Add(10);
    arr.Add(15);
    TEST_CHECK_EQUAL(arr.GetLast(), 15);
}

TEST(Array, Reset) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    Array<int32, FreeListAllocator> arr({1, 2, 3}, &alloc);
    arr.Reset();
    TEST_CHECK_EQUAL(arr.GetSize(), 0u);
    TEST_CHECK(arr.IsEmpty());
}

TEST(Array, Emplace) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    Array<int32, FreeListAllocator> arr(4, &alloc);
    int32& ref = arr.Emplace(42);
    TEST_CHECK_EQUAL(ref, 42);
    TEST_CHECK_EQUAL(arr.GetSize(), 1u);
    TEST_CHECK_EQUAL(arr[0], 42);
}

TEST(Array, RangeBasedFor) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    Array<int32, FreeListAllocator> arr({10, 20, 30}, &alloc);
    int32 sum = 0;
    for (int32 val : arr) {
        sum += val;
    }
    TEST_CHECK_EQUAL(sum, 60);
}

TEST(Array, MoveConstruction) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    Array<int32, FreeListAllocator> a({1, 2, 3}, &alloc);
    Array<int32, FreeListAllocator> b(static_cast<Array<int32, FreeListAllocator>&&>(a));

    TEST_CHECK_EQUAL(b.GetSize(), 3u);
    TEST_CHECK_EQUAL(b[0], 1);
    TEST_CHECK_EQUAL(b[1], 2);
    TEST_CHECK_EQUAL(b[2], 3);
}

TEST(Array, CopyConstructor) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    Array<int32, FreeListAllocator> source({10, 20, 30, 40}, &alloc);
    Array<int32, FreeListAllocator> copy(source);

    TEST_CHECK_EQUAL(copy.GetSize(), source.GetSize());
    TEST_CHECK_EQUAL(copy[0], 10);
    TEST_CHECK_EQUAL(copy[1], 20);
    TEST_CHECK_EQUAL(copy[2], 30);
    TEST_CHECK_EQUAL(copy[3], 40);
}

TEST(Array, CopyIsIndependent) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    Array<int32, FreeListAllocator> source({1, 2, 3}, &alloc);
    Array<int32, FreeListAllocator> copy(source);

    // Mutate the copy
    copy[0] = 999;
    copy.Add(4);

    // Source must be untouched
    TEST_CHECK_EQUAL(source.GetSize(), 3u);
    TEST_CHECK_EQUAL(source[0], 1);
    TEST_CHECK_EQUAL(source[1], 2);
    TEST_CHECK_EQUAL(source[2], 3);

    // Copy reflects mutations
    TEST_CHECK_EQUAL(copy.GetSize(), 4u);
    TEST_CHECK_EQUAL(copy[0], 999);
    TEST_CHECK_EQUAL(copy[3], 4);
}

TEST(Array, CopyAssignmentNoGrow) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    Array<int32, FreeListAllocator> source({10, 20, 30}, &alloc);
    Array<int32, FreeListAllocator> dest(8, &alloc);
    dest.Add(77);
    dest.Add(88);
    const uint32 destCapacityBefore = dest.GetCapacity();

    dest = source;

    TEST_CHECK_EQUAL(dest.GetSize(), 3u);
    TEST_CHECK_EQUAL(dest.GetCapacity(), destCapacityBefore); // Capacity preserved when sufficient
    TEST_CHECK_EQUAL(dest[0], 10);
    TEST_CHECK_EQUAL(dest[1], 20);
    TEST_CHECK_EQUAL(dest[2], 30);
}

TEST(Array, CopyAssignmentGrows) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    Array<int32, FreeListAllocator> source(16, &alloc);
    for (int32 i = 0; i < 10; ++i) {
        source.Add(i * 5);
    }

    Array<int32, FreeListAllocator> dest(2, &alloc);
    dest.Add(1);

    dest = source;

    TEST_CHECK_EQUAL(dest.GetSize(), 10u);
    TEST_CHECK(dest.GetCapacity() >= 10u);
    for (uint32 i = 0; i < 10; ++i) {
        TEST_CHECK_EQUAL(dest[i], static_cast<int32>(i) * 5);
    }
}

TEST(Array, CopyAssignmentSelfIsNoOp) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    Array<int32, FreeListAllocator> arr({10, 20, 30}, &alloc);
    Array<int32, FreeListAllocator>& ref = arr;
    arr = ref;

    TEST_CHECK_EQUAL(arr.GetSize(), 3u);
    TEST_CHECK_EQUAL(arr[0], 10);
    TEST_CHECK_EQUAL(arr[1], 20);
    TEST_CHECK_EQUAL(arr[2], 30);
}

TEST(Array, CopyAssignmentFromEmpty) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    Array<int32, FreeListAllocator> empty(4, &alloc);
    Array<int32, FreeListAllocator> dest({1, 2, 3}, &alloc);

    dest = empty;

    TEST_CHECK_EQUAL(dest.GetSize(), 0u);
    TEST_CHECK(dest.IsEmpty());
}

TEST(Array, MoveAssignment) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    Array<int32, FreeListAllocator> source({100, 200, 300}, &alloc);
    Array<int32, FreeListAllocator> dest(4, &alloc);
    dest.Add(7);
    dest.Add(8);

    dest = static_cast<Array<int32, FreeListAllocator>&&>(source);

    TEST_CHECK_EQUAL(dest.GetSize(), 3u);
    TEST_CHECK_EQUAL(dest[0], 100);
    TEST_CHECK_EQUAL(dest[1], 200);
    TEST_CHECK_EQUAL(dest[2], 300);

    TEST_CHECK_EQUAL(source.GetSize(), 0u);
    TEST_CHECK_EQUAL(source.GetCapacity(), 0u);
}
