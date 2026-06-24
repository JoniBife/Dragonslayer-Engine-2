#pragma once

#include "Core/Assert.hpp"
#include "Core/Types.hpp"
#include "Core/Macros.hpp"

// TODO Decide what todo with ENGINE_API since its missing here

// Every operation directly maps to an OS-Level API call
#if DS_PLATFORM_WINDOWS
class WinFile;
using File = WinFile;
#elif DS_PLATFORM_LINUX
class UnixFile;
using File = UnixFile;
#endif

/**
 * Dead-simple File interface
 * - Supports bytes and any trivially copyable type
 * - DOES not close file on destruction! Close needs to be explicitly called!
 */
template<typename Implementation>
class FileInterface {

public:

    // TODO Flags
    FORCE_INLINE static Implementation OpenOrCreate(const char* path) {
        return Implementation::OpenFile(path);
    }

    FORCE_INLINE bool ReadBytes(uint8* bytes, uint32 bytesToRead, uint32& bytesRead) {
        return Impl().ReadBytes(bytes, bytesToRead, bytesRead);
    }

    FORCE_INLINE bool WriteBytes(const uint8* bytes, uint32 bytesToWrite) {
        return Impl().WriteBytes(bytes, bytesToWrite);
    }

    template<typename T>
    FORCE_INLINE std::enable_if_t<std::is_trivially_copyable_v<T>, bool> Write(const T& obj) {
        return WriteDynamic(&obj, 1);
    }

    template<typename T>
    FORCE_INLINE std::enable_if_t<std::is_trivially_copyable_v<T>, bool> WriteCopy(T obj) {
        return WriteDynamic(&obj, 1);
    }

    template<typename T>
    FORCE_INLINE std::enable_if_t<std::is_trivially_copyable_v<T>, bool> Read(T& obj) {
        return ReadDynamic(&obj, 1);
    }

    template<typename T, size_t N>
    FORCE_INLINE std::enable_if_t<std::is_trivially_copyable_v<T>, bool> Write(const T (&array)[N]) {
        return WriteDynamic(array, N);
    }

    template<typename T, size_t N>
    FORCE_INLINE std::enable_if_t<std::is_trivially_copyable_v<T>, bool> Read(T (&array)[N]) {
        return ReadDynamic(array, N);
    }

    template<typename T>
    FORCE_INLINE std::enable_if_t<std::is_trivially_copyable_v<T>, bool> Write(const T* ptr, size_t count) {
        return WriteDynamic(ptr, count);
    }

    template<typename T>
    FORCE_INLINE std::enable_if_t<std::is_trivially_copyable_v<T>, bool> Read(T* ptr, size_t count) {
        return ReadDynamic(ptr, count);
    }

    template<size_t N>
    FORCE_INLINE bool Write(const char (&array)[N]) {
        return WriteDynamic(array, N - 1); // Removing null terminator from strings
    }

    FORCE_INLINE bool SetFilePointerTo(int64 position) {
        return Impl().SetFilePointerTo(position);
    }

    FORCE_INLINE bool MoveFilePointerBy(int64 offset) {
        return Impl().MoveFilePointerBy(offset);
    }

    FORCE_INLINE bool ResetFilePointer() {
        return SetFilePointerTo(0);
    }

    FORCE_INLINE bool GetFilePointer(int64& pointer) {
        return Impl().GetFilePointer(pointer);
    }

    FORCE_INLINE bool GetFileSize(int64& size) {
        return Impl().GetFileSize(size);
    }

    FORCE_INLINE bool FlushWrites() {
        return Impl().FlushWrites();
    }

    FORCE_INLINE bool ClearContents() {
        return Impl().ClearContents();
    }

    FORCE_INLINE bool Close() {
        return Impl().CloseFile();
    }

    NO_DISCARD FORCE_INLINE bool IsOpen() const {
        return Impl().IsOpen();
    }

private:
    template<typename T>
    FORCE_INLINE bool WriteDynamic(const T* ptr, size_t count) {
        return WriteBytes(reinterpret_cast<const uint8*>(ptr), sizeof(T) * count);
    }

    template<typename T>
    FORCE_INLINE bool ReadDynamic(T* ptr, size_t count) {
        uint32 bytesRead = 0;
        const bool success = ReadBytes(reinterpret_cast<uint8*>(ptr), sizeof(T) * count, bytesRead);
        return success && bytesRead == sizeof(T) * count;
    }

    Implementation& Impl() {
        return *static_cast<Implementation*>(this);
    }
};

#if DS_PLATFORM_WINDOWS

#undef APIENTRY
#include <Windows.h>

class WinFile : public FileInterface<WinFile> {

protected:
    HANDLE handle = INVALID_HANDLE_VALUE;
    WinFile() = default;

public:

    FORCE_INLINE static WinFile OpenOrCreate(const char* path) {
        WinFile file;
        file.handle = CreateFileA(
            path,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ,
            nullptr,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );
        if (!file.IsOpen()) {
            const DWORD error = GetLastError();
            FAIL("ERROR %d: Failed to open or create file!", error);
        }
        return file;
    }

    FORCE_INLINE bool ReadBytes(uint8* bytes, uint32 bytesToRead, uint32& bytesRead) {
        ASSERT(IsOpen(), "File is not open!");
        DWORD read = 0;
        if (!ReadFile(handle, bytes, bytesToRead, &read, nullptr)) {
            bytesRead = 0;
            return false;
        }
        bytesRead = read;
        return true;
    }

    FORCE_INLINE bool WriteBytes(const uint8* bytes, uint32 bytesToWrite) {
        ASSERT(IsOpen(), "File is not open!");
        DWORD written = 0;
        return WriteFile(handle, bytes, bytesToWrite, &written, nullptr) &&
               written == bytesToWrite;
    }

    FORCE_INLINE bool SetFilePointerTo(int64 position) {
        ASSERT(IsOpen(), "File is not open!");
        LARGE_INTEGER newPointer = {.QuadPart = position};
        return SetFilePointerEx(handle, newPointer, nullptr, FILE_BEGIN);
    }

    FORCE_INLINE bool MoveFilePointerBy(int64 offset) {
        ASSERT(IsOpen(), "File is not open!");
        LARGE_INTEGER newPointer = {.QuadPart = offset};
        return SetFilePointerEx(handle, newPointer, nullptr, FILE_CURRENT);
    }

    FORCE_INLINE bool GetFilePointer(int64& pointer) {
        ASSERT(IsOpen(), "File is not open!");
        LARGE_INTEGER dummyPointer = {.QuadPart = 0};
        LARGE_INTEGER currentPointer = {.QuadPart = 0};
        if (SetFilePointerEx(handle, dummyPointer, &currentPointer, FILE_CURRENT)) {
            pointer = currentPointer.QuadPart;
            return true;
        }
        return false;
    }

    FORCE_INLINE bool GetFileSize(int64& size) {
        ASSERT(IsOpen(), "File is not open!");
        LARGE_INTEGER fileSize;
        if (GetFileSizeEx(handle, &fileSize)) {
            size = fileSize.QuadPart;
            return true;
        }
        return false;
    }

    FORCE_INLINE bool FlushWrites() {
        ASSERT(IsOpen(), "File is not open!");
        return FlushFileBuffers(handle);
    }

    FORCE_INLINE bool ClearContents() {
        ASSERT(IsOpen(), "File is not open!");
        LARGE_INTEGER zero = {};
        zero.QuadPart = 0;
        const bool success = SetFilePointerEx(handle, zero, nullptr, FILE_BEGIN);
        return success ? SetEndOfFile(handle) : false;
    }

    FORCE_INLINE bool Close() {
        ASSERT(IsOpen(), "File is not open!");
        const bool result = CloseHandle(handle);
        handle = INVALID_HANDLE_VALUE;
        return result;
    }

    NO_DISCARD FORCE_INLINE bool IsOpen() const {
        return handle != INVALID_HANDLE_VALUE;
    }
};
#endif

#if DS_PLATFORM_LINUX
#include <fcntl.h>      // For open(), O_RDWR, O_CREAT
#include <unistd.h>     // For read(), write(), lseek(), close(), ftruncate(), fsync()
#include <sys/stat.h>   // For fstat()
#include <cerrno>       // For errno

class UnixFile : public FileInterface<UnixFile> {

protected:
    int fd = -1;
    UnixFile() = default;

public:

    FORCE_INLINE static UnixFile OpenOrCreate(const char* path) {
        UnixFile file;
        // O_RDWR: Generic Read/Write
        // O_CREAT: Open always (creates if missing)
        // 0644: Standard file attributes (rw-r--r--)
        file.fd = open(path, O_RDWR | O_CREAT, 0644);

        if (!file.IsOpen()) {
            const int error = errno;
            FAIL("ERROR %d: Failed to open or create file!", error);
        }
        return file;
    }

    FORCE_INLINE bool ReadBytes(uint8* bytes, uint32 bytesToRead, uint32& bytesRead) {
        ASSERT(IsOpen(), "File is not open!");
        ssize_t readResult = read(fd, bytes, bytesToRead);

        if (readResult < 0) {
            bytesRead = 0;
            return false;
        }

        bytesRead = static_cast<uint32>(readResult);
        return true;
    }

    FORCE_INLINE bool WriteBytes(const uint8* bytes, uint32 bytesToWrite) {
        ASSERT(IsOpen(), "File is not open!");
        ssize_t written = write(fd, bytes, bytesToWrite);

        // Ensure write was successful and wrote the exact requested amount
        return written >= 0 && static_cast<uint32>(written) == bytesToWrite;
    }

    FORCE_INLINE bool SetFilePointerTo(int64 position) {
        ASSERT(IsOpen(), "File is not open!");
        return lseek(fd, static_cast<off_t>(position), SEEK_SET) != (off_t)-1;
    }

    FORCE_INLINE bool MoveFilePointerBy(int64 offset) {
        ASSERT(IsOpen(), "File is not open!");
        return lseek(fd, static_cast<off_t>(offset), SEEK_CUR) != (off_t)-1;
    }

    FORCE_INLINE bool GetFilePointer(int64& pointer) {
        ASSERT(IsOpen(), "File is not open!");
        off_t currentPointer = lseek(fd, 0, SEEK_CUR);

        if (currentPointer != (off_t)-1) {
            pointer = static_cast<int64>(currentPointer);
            return true;
        }
        return false;
    }

    FORCE_INLINE bool GetFileSize(int64& size) {
        ASSERT(IsOpen(), "File is not open!");
        struct stat fileStat;

        if (fstat(fd, &fileStat) == 0) {
            size = static_cast<int64>(fileStat.st_size);
            return true;
        }
        return false;
    }

    FORCE_INLINE bool FlushWrites() {
        ASSERT(IsOpen(), "File is not open!");
        return fsync(fd) == 0;
    }

    FORCE_INLINE bool ClearContents() {
        ASSERT(IsOpen(), "File is not open!");
        // Equivalent to moving pointer to beginning and calling SetEndOfFile()
        if (lseek(fd, 0, SEEK_SET) == (off_t)-1) {
            return false;
        }
        return ftruncate(fd, 0) == 0;
    }

    FORCE_INLINE bool Close() {
        ASSERT(IsOpen(), "File is not open!");
        const bool result = (close(fd) == 0);
        fd = -1;
        return result;
    }

    NO_DISCARD FORCE_INLINE bool IsOpen() const {
        return fd != -1;
    }
};
#endif







