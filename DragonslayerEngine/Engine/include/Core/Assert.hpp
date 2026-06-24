#pragma once

#include "Macros.hpp"

static constexpr bool OutputAssert(const char* expression, const char* file, unsigned int line, const char* message = nullptr, ...) {

    // Check if custom message is provided
    //if (message) {
    //    constexpr size_t maxCharacters = 256;
    //    char messageBuffer[maxCharacters];
    //    va_list args;
    //    va_start(args, message);
    //    std::vsnprintf(messageBuffer, maxCharacters, message, args);
    //    va_end(args);
    //    Log::Error(messageBuffer);
    //}
//
    // TODO Implement message properly!

    return true;
}

// TODO Remove from shipping builds!
// Triggers breakpoint if the expression is false and outputs file, line and an optional message to the console
#define ASSERT(expression, ...) \
    do { \
        if (!(expression) && OutputAssert(#expression, __FILE__, __LINE__, ##__VA_ARGS__)) { \
           BREAKPOINT; \
        } \
    } while(false);

#define FAIL(...) ASSERT(false, ##__VA_ARGS__)