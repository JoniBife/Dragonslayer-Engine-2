#include "Framework/TestFramework.hpp"

#include <cstdio>
#include <cstring>

int main(int argc, char** argv) {

    // Parse arguments
    const char* filter = nullptr;

    for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "--filter=", 9) == 0) {
            filter = argv[i] + 9;
        }
    }

    // Count tests matching filter
    uint32 totalTests = 0;
    uint32 totalSuites = 0;
    for (TestEntry* e = GetTestListHead(); e; e = e->next) {
        if (MatchesFilter(filter, e->suiteName)) {
            totalTests++;
        }
    }

    // Count unique suites
    {
        const char* seen[256] = {};
        uint32 seenCount = 0;
        for (TestEntry* e = GetTestListHead(); e; e = e->next) {
            if (!MatchesFilter(filter, e->suiteName)) continue;
            bool found = false;
            for (uint32 i = 0; i < seenCount; ++i) {
                if (strcmp(seen[i], e->suiteName) == 0) { found = true; break; }
            }
            if (!found && seenCount < 256) {
                seen[seenCount++] = e->suiteName;
            }
        }
        totalSuites = seenCount;
    }

    SetColor(ColorGreen);
    printf("[==========] ");
    SetColor(ColorDefault);
    printf("Running %u tests from %u test suites.\n", totalTests, totalSuites);

    // Run tests
    uint32 passed = 0;
    uint32 failed = 0;
    TestEntry* failedEntries[256] = {};
    uint32 failedCount = 0;

    for (TestEntry* e = GetTestListHead(); e; e = e->next) {
        if (!MatchesFilter(filter, e->suiteName)) continue;

        SetColor(ColorYellow);
        printf("[  RUNNING ] ");
        SetColor(ColorDefault);
        printf("%s.%s\n", e->suiteName, e->testName);

        TestContext ctx = {};
        ctx.suiteName = e->suiteName;
        ctx.testName = e->testName;

        int64 start = GetTimeMicroseconds();
        e->func(ctx);
        int64 elapsed = GetTimeMicroseconds() - start; // us

        if (ctx.checksFailed == 0) {
            SetColor(ColorGreen);
            printf("[    OK    ] ");
            SetColor(ColorDefault);
            printf("%s.%s (%lld us)\n", e->suiteName, e->testName, elapsed);
            passed++;
        } else {
            SetColor(ColorRed);
            printf("[  FAILED  ] ");
            SetColor(ColorDefault);
            printf("%s.%s (%lld us)\n", e->suiteName, e->testName, elapsed);
            failed++;
            if (failedCount < 256) {
                failedEntries[failedCount++] = e;
            }
        }
    }

    // Summary
    printf("\n");
    SetColor(ColorGreen);
    printf("[==========] ");
    SetColor(ColorDefault);
    printf("%u tests ran.\n", passed + failed);

    if (passed > 0) {
        SetColor(ColorGreen);
        printf("[  PASSED  ] ");
        SetColor(ColorDefault);
        printf("%u tests.\n", passed);
    }

    if (failed > 0) {
        SetColor(ColorRed);
        printf("[  FAILED  ] ");
        SetColor(ColorDefault);
        printf("%u tests, listed below:\n", failed);

        for (uint32 i = 0; i < failedCount; ++i) {
            SetColor(ColorRed);
            printf("[  FAILED  ] ");
            SetColor(ColorDefault);
            printf("%s.%s\n", failedEntries[i]->suiteName, failedEntries[i]->testName);
        }
    }

    return failed > 0 ? 1 : 0;
}
