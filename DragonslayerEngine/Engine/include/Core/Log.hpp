#pragma once

#include "Core/Allocators/LinearAllocator.hpp"
#include "Core/Containers/Array.hpp"
#include "Core/NotCopyable.hpp"
#include "Core/String.hpp"

#define MAX_LOG_ENTRIES 16384 // After this value we start deleting older ones!
#define MAX_LOG_LENGTH 4096

using LogString = InlineString<MAX_LOG_LENGTH>;

enum class LogType : uint8 {
    Info,
    Warning,
    Error
};

struct LogEntry {
    LogString text;
    LogType type = LogType::Info;
    int64 threadIndex; // Index of -1 means untracked thread
};

/**
 * Dead-simple thread-safe Logging system
 * - TODO Refactor this, switch to per-thread log entries!
 */
class ENGINE_API Log final : public NotCopyable {

private:
    LinearAllocator allocator;
    Array<LogEntry, LinearAllocator> frameLogEntries; // New log entries added this frame
    Array<LogEntry, LinearAllocator> logEntries;
    std::mutex mutex;

    static Log* instance;

public:
    explicit Log(class FreeListAllocator& parentAllocator);
    ~Log() = default; // TODO Consider outputting all remaining logs on destruction!

    static void Info(const LogString& text);
    static void Warning(const LogString& text);
    static void Error(const LogString& text);
    static void Any(LogType type, const LogString& text);

    static void PrintLogsCurrentFrame();
    static const Array<LogEntry, LinearAllocator>& GetLogsCurrentFrame();
    static const Array<LogEntry, LinearAllocator>& GetLogs();

private:
    void AddLogEntry(LogType type, const LogString& text);
    static void ClearLogsCurrentFrame();
    static void ClearLogs();

    friend class LogSystem;
#if WITH_EDITOR
    friend class OutputLogSystem;
#endif
};
