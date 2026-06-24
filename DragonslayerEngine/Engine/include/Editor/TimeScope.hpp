#pragma once

// TODO Move out of editor!

#include <chrono>

#include "TimeCapturesManager.hpp"
#include "Core/NotCopyable.hpp"
#include "Core/String.hpp"
#include "Core/ThreadContext.hpp"

#if WITH_EDITOR
    #define TIME_SCOPE_CONCAT_IMPL(x, y) x##y
    #define TIME_SCOPE_CONCAT(x, y) TIME_SCOPE_CONCAT_IMPL(x, y)
    #define TIME_SCOPE(X) const TimeScope TIME_SCOPE_CONCAT(timeScope_, __LINE__) = TimeScope(X)
    #define TIME_SCOPE_PRINT(X) const TimeScope TIME_SCOPE_CONCAT(timeScope_, __LINE__) = TimeScope(X, true)
#else
    #define TIME_SCOPE(X)
    #define TIME_SCOPE_PRINT(X)
#endif


/*
 * TODO Improvement: We can grab things like function/method name
 */
class TimeScope final : public NotCopyable {

private:
    NameString name;
    std::chrono::time_point<std::chrono::high_resolution_clock> timeStart;
    bool printResult;

public:
    explicit TimeScope(const NameString& name, bool printResult = false) :
        name(name), timeStart(std::chrono::high_resolution_clock::now()), printResult(printResult) {}

    ~TimeScope() {

        const std::chrono::time_point<std::chrono::high_resolution_clock> timeEnd = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<float, std::milli> duration = timeEnd - timeStart;

        if (printResult) {
            printf("%s took %f s \n", name.CStr(), duration.count() / 1000.f );
        } else {
            TimeCapturesManager::Instance().AddCapture(name, duration.count());
        }
    }
};