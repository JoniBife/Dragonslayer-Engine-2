#include "Core/Log.hpp"

#include <iostream>
#include <Core/EngineGlobals.hpp>
#include <Core/ThreadContext.hpp>

#include "Core/Assert.hpp"
#include "Core/Allocators/FreeListAllocator.hpp"

Log* Log::instance = nullptr;

Log::Log(FreeListAllocator& parentAllocator) :
    allocator(parentAllocator, sizeof(LogEntry) * (MAX_LOG_ENTRIES*2)),
    frameLogEntries(MAX_LOG_ENTRIES, &allocator),
    logEntries(MAX_LOG_ENTRIES, &allocator) {

    ASSERT(instance == nullptr, "There is more than one instance of Log!");
    instance = this;
}

void Log::Info(const LogString& text) {
    ASSERT(instance != nullptr);
    instance->AddLogEntry(LogType::Info, text);
}

void Log::Warning(const LogString& text) {
    ASSERT(instance != nullptr);
    instance->AddLogEntry(LogType::Warning, text);
}

void Log::Error(const LogString& text) {
    ASSERT(instance != nullptr);
    instance->AddLogEntry(LogType::Error, text);
}
void Log::Any(LogType type, const LogString& text) {
    ASSERT(instance != nullptr);
    instance->AddLogEntry(type, text);
}

void Log::PrintLogsCurrentFrame() {

#define YELLOW "\033[33m"
#define RED "\033[31m"
#define END_COLOR "\033[m"

    for (const LogEntry& logEntry : GetLogsCurrentFrame()) {
        switch (logEntry.type) {
            case LogType::Info: {
                std::cout << "Thread(" << logEntry.threadIndex << ") -> " << "[INFO] " << logEntry.text.CStr() << std::endl;
                break;
            }
            case LogType::Warning: {
                std::cout << YELLOW << "Thread(" << logEntry.threadIndex << ") -> " << "[WARNING] " << logEntry.text.CStr() << END_COLOR << std::endl;
                break;
            }
            case LogType::Error: {
                std::cout << RED << "Thread(" << logEntry.threadIndex << ") -> " << "[ERROR] " << logEntry.text.CStr() << END_COLOR << std::endl;
                break;
            }
        }
    }
}

const Array<LogEntry, LinearAllocator>& Log::GetLogs() {
    ASSERT(instance != nullptr);
    return instance->logEntries;
}

void Log::AddLogEntry(LogType type, const LogString& text) {
    instance->mutex.lock();
    if (instance->frameLogEntries.GetSize() == MAX_LOG_ENTRIES) {
        instance->frameLogEntries.RemoveAt(0);
    }
    if (instance->logEntries.GetSize() == MAX_LOG_ENTRIES) {
        instance->logEntries.RemoveAt(0);
    }

    const int64 threadIndex = gThreadContext ? gThreadContext->index : -1;
    instance->frameLogEntries.Emplace(text, type, threadIndex);
    instance->logEntries.Emplace(text, type, threadIndex);
    instance->mutex.unlock();
}

const Array<LogEntry, LinearAllocator>& Log::GetLogsCurrentFrame() {
    ASSERT(instance != nullptr);
    return instance->frameLogEntries;
}

void Log::ClearLogsCurrentFrame() {
    ASSERT(instance != nullptr);
    instance->frameLogEntries.Reset();
}

void Log::ClearLogs() {
    ASSERT(instance != nullptr);
    instance->logEntries.Reset();
}

