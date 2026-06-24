#include "Framework/TestFramework.hpp"
#include "Core/String.hpp"

TEST(InlineString, DefaultConstruction) {
    InlineString<32> s;
    TEST_CHECK_EQUAL(s.Size(), 0u);
    TEST_CHECK_EQUAL(s.Capacity(), 31u);
    TEST_CHECK(s == "");
}

TEST(InlineString, ConstructFromCString) {
    InlineString<32> s("Hello");
    TEST_CHECK_EQUAL(s.Size(), 5u);
    TEST_CHECK(s == "Hello");
}

TEST(InlineString, Contains) {
    InlineString<64> s("The quick brown fox");
    TEST_CHECK(s.Contains("quick"));
    TEST_CHECK(s.Contains("fox"));
    TEST_CHECK(s.Contains("The"));
    TEST_CHECK_FALSE(s.Contains("lazy"));
    TEST_CHECK(s.Contains("")); // Empty substring returns true
}

TEST(InlineString, StartsWith) {
    InlineString<32> s("Hello World");
    TEST_CHECK(s.StartsWith("Hello"));
    TEST_CHECK(s.StartsWith("H"));
    TEST_CHECK_FALSE(s.StartsWith("World"));
    TEST_CHECK_FALSE(s.StartsWith("Hello World!!!!")); // Longer than string
}

TEST(InlineString, EndsWith) {
    InlineString<32> s("Hello World");
    TEST_CHECK(s.EndsWith("World"));
    TEST_CHECK(s.EndsWith("d"));
    TEST_CHECK_FALSE(s.EndsWith("Hello"));
}

TEST(InlineString, Concatenation) {
    InlineString<64> a("Hello");
    InlineString<64> b(" World");
    InlineString<64> result = a + b;
    TEST_CHECK(result == "Hello World");
    TEST_CHECK_EQUAL(result.Size(), 11u);
}

TEST(InlineString, Equality) {
    InlineString<32> a("Test");
    InlineString<32> b("Test");
    InlineString<32> c("Other");
    TEST_CHECK(a == b);
    TEST_CHECK(a != c);
    TEST_CHECK(a == "Test");
    TEST_CHECK_NOT_EQUAL(a, c);
}

TEST(InlineString, FromInteger) {
    InlineString<32> pos(42);
    TEST_CHECK(pos == "42");

    InlineString<32> neg(-7);
    TEST_CHECK(neg == "-7");

    InlineString<32> zero(0);
    TEST_CHECK(zero == "0");
}
