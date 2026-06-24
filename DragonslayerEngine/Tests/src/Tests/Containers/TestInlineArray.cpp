#include "Framework/TestFramework.hpp"
#include "Core/Containers/Array.hpp"

TEST(InlineArray, DefaultConstruction) {
    InlineArray<int32, 16> arr;
    TEST_CHECK_EQUAL(arr.GetSize(), 0u);
    TEST_CHECK_EQUAL(arr.GetCapacity(), 16u);
    TEST_CHECK(arr.IsEmpty());
}

TEST(InlineArray, Add) {
    InlineArray<int32, 8> arr;
    arr.Add(10);
    arr.Add(20);
    arr.Add(30);

    TEST_CHECK_EQUAL(arr.GetSize(), 3u);
    TEST_CHECK_EQUAL(arr[0], 10);
    TEST_CHECK_EQUAL(arr[1], 20);
    TEST_CHECK_EQUAL(arr[2], 30);
    TEST_CHECK_FALSE(arr.IsEmpty());
}

TEST(InlineArray, InitializerList) {
    InlineArray<int32, 8> arr = {1, 2, 3, 4, 5};
    TEST_CHECK_EQUAL(arr.GetSize(), 5u);
    TEST_CHECK_EQUAL(arr[0], 1);
    TEST_CHECK_EQUAL(arr[4], 5);
}

TEST(InlineArray, GetLast) {
    InlineArray<int32, 8> arr;
    arr.Add(10);
    arr.Add(20);
    arr.Add(30);
    TEST_CHECK_EQUAL(arr.GetLast(), 30);
}

TEST(InlineArray, RemoveAt) {
    InlineArray<int32, 8> arr = {10, 20, 30, 40};
    TEST_CHECK(arr.RemoveAt(1)); // Remove 20
    TEST_CHECK_EQUAL(arr.GetSize(), 3u);
    TEST_CHECK_EQUAL(arr[0], 10);
    TEST_CHECK_EQUAL(arr[1], 30);
    TEST_CHECK_EQUAL(arr[2], 40);
}

TEST(InlineArray, RemoveAndSwap) {
    InlineArray<int32, 8> arr = {10, 20, 30, 40};
    TEST_CHECK(arr.RemoveAndSwapAt(0)); // Remove 10, last element fills the gap
    TEST_CHECK_EQUAL(arr.GetSize(), 3u);
    TEST_CHECK_EQUAL(arr[0], 40); // 40 swapped into position 0
    TEST_CHECK_EQUAL(arr[1], 20);
    TEST_CHECK_EQUAL(arr[2], 30);
}

TEST(InlineArray, RemoveLast) {
    InlineArray<int32, 8> arr = {10, 20, 30};
    TEST_CHECK(arr.RemoveLast());
    TEST_CHECK_EQUAL(arr.GetSize(), 2u);
    TEST_CHECK_EQUAL(arr.GetLast(), 20);
}

TEST(InlineArray, RemoveAll) {
    InlineArray<int32, 8> arr = {1, 2, 3};
    TEST_CHECK(arr.RemoveLast());
    TEST_CHECK(arr.RemoveLast());
    TEST_CHECK(arr.RemoveLast());
    TEST_CHECK_EQUAL(arr.GetSize(), 0u);
    TEST_CHECK(arr.IsEmpty());
}

TEST(InlineArray, RangeBasedFor) {
    InlineArray<int32, 8> arr = {10, 20, 30};
    int32 sum = 0;
    for (int32 val : arr) {
        sum += val;
    }
    TEST_CHECK_EQUAL(sum, 60);
}
