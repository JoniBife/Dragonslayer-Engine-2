#include "Framework/TestFramework.hpp"

#include <cstdio>

#if DS_PLATFORM_WINDOWS
    #include <windows.h>
#endif

// ---------------------------------------------------------------
// Global test list (zero-initialized before any static ctors)
// ---------------------------------------------------------------

static TestEntry* headEntry = nullptr;

TestEntry*& GetTestListHead() {
    return headEntry;
}

TestEntry::TestEntry(const char* suite, const char* name, void (*fn)(TestContext&))
    : suiteName(suite), testName(name), func(fn), next(headEntry) {
    headEntry = this;
}

// ---------------------------------------------------------------
// Console colors (Windows)
// ---------------------------------------------------------------

#if DS_PLATFORM_WINDOWS

void SetColor(uint16 color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

#else

void SetColor(uint16) {}

#endif

// ---------------------------------------------------------------
// Timing
// ---------------------------------------------------------------

int64 GetTimeMicroseconds() {
#if DS_PLATFORM_WINDOWS
    LARGE_INTEGER freq, now;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);
    return (now.QuadPart * 1000000) / freq.QuadPart;
#else
    return 0;
#endif
}

// ---------------------------------------------------------------
// Failure reporting
// ---------------------------------------------------------------

void ReportFailure(const char* file, int line, const char* expression) {
    SetColor(ColorRed);
    printf("%s(%d): Failure\n", file, line);
    printf("      Expression: %s\n", expression);
    SetColor(ColorDefault);
}

// ---------------------------------------------------------------
// Filter matching (--filter=)
// ---------------------------------------------------------------

bool MatchesFilter(const char* filter, const char* suite) {
    if (!filter) return true;
    return strcmp(filter, suite) == 0;
}
