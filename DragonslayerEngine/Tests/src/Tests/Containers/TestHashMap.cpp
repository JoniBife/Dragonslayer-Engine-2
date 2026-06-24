#include "Framework/TestFramework.hpp"
#include "Core/Allocators/FreeListAllocator.hpp"
#include "Core/Containers/HashMap.hpp"

static constexpr size_t BUFFER_SIZE = Kb(8);

TEST(HashMap, DefaultConstruction) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    HashMap<uint32, int32, FreeListAllocator> map(16, &alloc);
    TEST_CHECK(map.IsEmpty());
    TEST_CHECK_EQUAL(map.GetSize(), 0u);
}

TEST(HashMap, InsertAndFind) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    HashMap<uint32, int32, FreeListAllocator> map(16, &alloc);
    map.Insert(1u, 100);
    map.Insert(2u, 200);
    map.Insert(3u, 300);

    TEST_CHECK_EQUAL(map.GetSize(), 3u);

    int32* val = map.Find(1u);
    TEST_REQUIRE(val != nullptr);
    TEST_CHECK_EQUAL(*val, 100);

    val = map.Find(2u);
    TEST_REQUIRE(val != nullptr);
    TEST_CHECK_EQUAL(*val, 200);

    val = map.Find(3u);
    TEST_REQUIRE(val != nullptr);
    TEST_CHECK_EQUAL(*val, 300);
}

TEST(HashMap, FindMissing) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    HashMap<uint32, int32, FreeListAllocator> map(16, &alloc);
    map.Insert(1u, 100);

    int32* val = map.Find(999u);
    TEST_CHECK(val == nullptr);
}

TEST(HashMap, Contains) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    HashMap<uint32, int32, FreeListAllocator> map(16, &alloc);
    map.Insert(42u, 7);

    TEST_CHECK(map.Contains(42u));
    TEST_CHECK_FALSE(map.Contains(99u));
}

TEST(HashMap, Remove) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    HashMap<uint32, int32, FreeListAllocator> map(16, &alloc);
    map.Insert(1u, 100);
    map.Insert(2u, 200);

    TEST_CHECK(map.Remove(1u));
    TEST_CHECK_EQUAL(map.GetSize(), 1u);
    TEST_CHECK_FALSE(map.Contains(1u));
    TEST_CHECK(map.Contains(2u));
}

TEST(HashMap, RemoveMissing) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    HashMap<uint32, int32, FreeListAllocator> map(16, &alloc);
    map.Insert(1u, 100);

    TEST_CHECK_FALSE(map.Remove(999u));
    TEST_CHECK_EQUAL(map.GetSize(), 1u);
}

TEST(HashMap, SubscriptOperator) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    HashMap<uint32, int32, FreeListAllocator> map(16, &alloc);
    map.Insert(5u, 50);

    TEST_CHECK_EQUAL(map[5u], 50);
}

TEST(HashMap, Emplace) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    HashMap<uint32, int32, FreeListAllocator> map(16, &alloc);
    const int32& ref = map.Emplace(10u, 999);

    TEST_CHECK_EQUAL(ref, 999);
    TEST_CHECK_EQUAL(map.GetSize(), 1u);
    TEST_CHECK(map.Contains(10u));
}

TEST(HashMap, UpdateViaFind) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    HashMap<uint32, int32, FreeListAllocator> map(16, &alloc);
    map.Insert(1u, 100);

    // To update an existing key, use Find and modify directly
    int32* val = map.Find(1u);
    TEST_REQUIRE(val != nullptr);
    *val = 200;

    TEST_CHECK_EQUAL(map.GetSize(), 1u);
    TEST_CHECK_EQUAL(map[1u], 200);
}

TEST(HashMap, Reset) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    HashMap<uint32, int32, FreeListAllocator> map(16, &alloc);
    map.Insert(1u, 10);
    map.Insert(2u, 20);
    map.Reset();

    TEST_CHECK(map.IsEmpty());
    TEST_CHECK_EQUAL(map.GetSize(), 0u);
}

TEST(HashMap, ManyInsertions) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    HashMap<uint32, int32, FreeListAllocator> map(4, &alloc);

    // Insert enough to trigger growth (load factor 0.85)
    for (uint32 i = 0; i < 20; ++i) {
        map.Insert(i, static_cast<int32>(i * 10));
    }

    TEST_CHECK_EQUAL(map.GetSize(), 20u);

    for (uint32 i = 0; i < 20; ++i) {
        int32* val = map.Find(i);
        TEST_REQUIRE(val != nullptr);
        TEST_CHECK_EQUAL(*val, static_cast<int32>(i * 10));
    }
}

TEST(HashMap, CopyConstructor) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    HashMap<uint32, int32, FreeListAllocator> source(16, &alloc);
    source.Insert(1u, 100);
    source.Insert(2u, 200);
    source.Insert(3u, 300);

    HashMap<uint32, int32, FreeListAllocator> copy(source);

    TEST_CHECK_EQUAL(copy.GetSize(), source.GetSize());
    TEST_CHECK_EQUAL(copy.GetCapacity(), source.GetCapacity());

    int32* val = copy.Find(1u);
    TEST_REQUIRE(val != nullptr);
    TEST_CHECK_EQUAL(*val, 100);
    val = copy.Find(2u);
    TEST_REQUIRE(val != nullptr);
    TEST_CHECK_EQUAL(*val, 200);
    val = copy.Find(3u);
    TEST_REQUIRE(val != nullptr);
    TEST_CHECK_EQUAL(*val, 300);
}

TEST(HashMap, CopyIsIndependent) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    HashMap<uint32, int32, FreeListAllocator> source(16, &alloc);
    source.Insert(1u, 100);
    source.Insert(2u, 200);

    HashMap<uint32, int32, FreeListAllocator> copy(source);

    // Mutate the copy: change a value, add a new key, remove an existing key
    *copy.Find(1u) = 999;
    copy.Insert(42u, 4242);
    copy.Remove(2u);

    // Source must be untouched
    TEST_CHECK_EQUAL(source.GetSize(), 2u);
    TEST_CHECK_EQUAL(*source.Find(1u), 100);
    TEST_CHECK_EQUAL(*source.Find(2u), 200);
    TEST_CHECK_FALSE(source.Contains(42u));

    // Copy reflects mutations
    TEST_CHECK_EQUAL(copy.GetSize(), 2u);
    TEST_CHECK_EQUAL(*copy.Find(1u), 999);
    TEST_CHECK(copy.Contains(42u));
    TEST_CHECK_FALSE(copy.Contains(2u));
}

TEST(HashMap, CopyAssignmentSameCapacity) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    HashMap<uint32, int32, FreeListAllocator> source(16, &alloc);
    source.Insert(1u, 100);
    source.Insert(2u, 200);

    HashMap<uint32, int32, FreeListAllocator> dest(16, &alloc);
    dest.Insert(7u, 70);
    dest.Insert(8u, 80);
    dest.Insert(9u, 90);

    dest = source;

    TEST_CHECK_EQUAL(dest.GetSize(), 2u);
    TEST_CHECK_EQUAL(*dest.Find(1u), 100);
    TEST_CHECK_EQUAL(*dest.Find(2u), 200);
    TEST_CHECK_FALSE(dest.Contains(7u));
    TEST_CHECK_FALSE(dest.Contains(8u));
    TEST_CHECK_FALSE(dest.Contains(9u));
}

TEST(HashMap, CopyAssignmentDifferentCapacity) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    HashMap<uint32, int32, FreeListAllocator> source(32, &alloc);
    for (uint32 i = 0; i < 10; ++i) {
        source.Insert(i, static_cast<int32>(i * 10));
    }

    HashMap<uint32, int32, FreeListAllocator> dest(8, &alloc);
    dest.Insert(100u, 1000);

    dest = source;

    TEST_CHECK_EQUAL(dest.GetSize(), 10u);
    TEST_CHECK_EQUAL(dest.GetCapacity(), source.GetCapacity());
    TEST_CHECK_FALSE(dest.Contains(100u));
    for (uint32 i = 0; i < 10; ++i) {
        int32* val = dest.Find(i);
        TEST_REQUIRE(val != nullptr);
        TEST_CHECK_EQUAL(*val, static_cast<int32>(i * 10));
    }
}

TEST(HashMap, CopyAssignmentSelfIsNoOp) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    HashMap<uint32, int32, FreeListAllocator> map(16, &alloc);
    map.Insert(1u, 100);
    map.Insert(2u, 200);

    HashMap<uint32, int32, FreeListAllocator>& ref = map;
    map = ref;

    TEST_CHECK_EQUAL(map.GetSize(), 2u);
    TEST_CHECK_EQUAL(*map.Find(1u), 100);
    TEST_CHECK_EQUAL(*map.Find(2u), 200);
}

TEST(HashMap, MoveConstructor) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    HashMap<uint32, int32, FreeListAllocator> source(16, &alloc);
    source.Insert(1u, 100);
    source.Insert(2u, 200);

    HashMap<uint32, int32, FreeListAllocator> moved(std::move(source));

    TEST_CHECK_EQUAL(moved.GetSize(), 2u);
    TEST_CHECK_EQUAL(*moved.Find(1u), 100);
    TEST_CHECK_EQUAL(*moved.Find(2u), 200);

    TEST_CHECK_EQUAL(source.GetSize(), 0u);
    TEST_CHECK_EQUAL(source.GetCapacity(), 0u);
}

TEST(HashMap, MoveAssignment) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    HashMap<uint32, int32, FreeListAllocator> source(16, &alloc);
    source.Insert(1u, 100);
    source.Insert(2u, 200);

    HashMap<uint32, int32, FreeListAllocator> dest(8, &alloc);
    dest.Insert(99u, 999);

    dest = std::move(source);

    TEST_CHECK_EQUAL(dest.GetSize(), 2u);
    TEST_CHECK_EQUAL(*dest.Find(1u), 100);
    TEST_CHECK_EQUAL(*dest.Find(2u), 200);
    TEST_CHECK_FALSE(dest.Contains(99u));

    TEST_CHECK_EQUAL(source.GetSize(), 0u);
    TEST_CHECK_EQUAL(source.GetCapacity(), 0u);
}

TEST(HashMap, Iteration) {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    FreeListAllocator alloc(buffer, BUFFER_SIZE);

    HashMap<uint32, int32, FreeListAllocator> map(16, &alloc);
    map.Insert(1u, 10);
    map.Insert(2u, 20);
    map.Insert(3u, 30);

    int32 sum = 0;
    uint32 count = 0;
    for (auto& kvp : map) {
        sum += kvp.value;
        count++;
    }
    TEST_CHECK_EQUAL(count, 3u);
    TEST_CHECK_EQUAL(sum, 60);
}
