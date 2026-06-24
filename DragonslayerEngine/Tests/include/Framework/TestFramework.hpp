#pragma once

#include "Core/Types.hpp"
#include "Core/Macros.hpp"

// ---------------------------------------------------------------
// Lightweight test framework for DragonslayerEngine
// ---------------------------------------------------------------

struct TestContext {
    const char* suiteName;
    const char* testName;
    uint32 checksPassed;
    uint32 checksFailed;
};

struct TestEntry {
    const char* suiteName;
    const char* testName;
    void (*func)(TestContext&);
    TestEntry* next;

    TestEntry(const char* suite, const char* name, void (*fn)(TestContext&));
};

TestEntry*& GetTestListHead();
void ReportFailure(const char* file, int line, const char* expression);

// ---------------------------------------------------------------
// Console colors
// ---------------------------------------------------------------

#if DS_PLATFORM_WINDOWS
enum ConsoleColor : uint16 {
    ColorDefault = 0x0007, // FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
    ColorGreen = 0x000A, // FOREGROUND_GREEN | FOREGROUND_INTENSITY
    ColorRed = 0x000C, // FOREGROUND_RED | FOREGROUND_INTENSITY
    ColorYellow  = 0x000E  // FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY
};
#else
enum ConsoleColor : uint16 { ColorDefault = 0, ColorGreen = 0, ColorRed = 0, ColorYellow = 0 };
#endif

void SetColor(uint16 color);
bool MatchesFilter(const char* filter, const char* suite);
void ListTests();
int64 GetTimeMicroseconds();

// ---------------------------------------------------------------
// Test declaration macro
// ---------------------------------------------------------------

#define TEST(Suite, Name) \
    static void Test_##Suite##_##Name(TestContext& ctx); \
    static TestEntry test_##Suite##_##Name( \
        #Suite, #Name, &Test_##Suite##_##Name); \
    static void Test_##Suite##_##Name( \
        MAYBE_UNUSED TestContext& ctx)

// ---------------------------------------------------------------
// Check macros (non-fatal)
// ---------------------------------------------------------------

#define TEST_CHECK(expr) \
    do { \
        if (!(expr)) { \
            ReportFailure(__FILE__, __LINE__, #expr); \
            ctx.checksFailed++; \
        } else { \
            ctx.checksPassed++; \
        } \
    } while(false)

#define TEST_CHECK_EQUAL(a, b) \
    do { \
        if (!((a) == (b))) { \
            ReportFailure(__FILE__, __LINE__, #a " == " #b); \
            ctx.checksFailed++; \
        } else { \
            ctx.checksPassed++; \
        } \
    } while(false)

#define TEST_CHECK_NOT_EQUAL(a, b) \
    do { \
        if ((a) == (b)) { \
            ReportFailure(__FILE__, __LINE__, #a " != " #b); \
            ctx.checksFailed++; \
        } else { \
            ctx.checksPassed++; \
        } \
    } while(false)

#define TEST_CHECK_FALSE(expr) \
    do { \
        if (expr) { \
            ReportFailure(__FILE__, __LINE__, "!(" #expr ")"); \
            ctx.checksFailed++; \
        } else { \
            ctx.checksPassed++; \
        } \
    } while(false)

// ---------------------------------------------------------------
// Require macro (fatal - returns from test on failure)
// ---------------------------------------------------------------

#define TEST_REQUIRE(expr) \
    do { \
        if (!(expr)) { \
            ReportFailure(__FILE__, __LINE__, #expr); \
            ctx.checksFailed++; \
            return; \
        } else { \
            ctx.checksPassed++; \
        } \
    } while(false)
