#pragma once

#include <cstring>

#include "Core/EngineGlobals.hpp"
#include "Core/Memory.hpp"
#include "File.hpp"
#include "Core/Allocators/FreeListAllocator.hpp"

/**
 * BufferedFile
 * - Serves to reduce sys calls by buffering reads and writes
 * - Buffer grows in chunks of fixed size
 */
template<typename Allocator = FreeListAllocator>
class BufferedFile : public FileInterface<BufferedFile<Allocator>> {

private:
    // Once buffer reaches max size we grow by this much
    // TODO Give more thought to the size of the chunk
    static constexpr int64 GROW_CHUNK = Kb(256);

    // Arbitrary size to ensure that no files start growing out of control
    // nor that we try to load files that are ridiculously large!
    static constexpr int64 MAX_FILE_SIZE = Mb(16);

    File file;
    Allocator* allocator;
    uint8* buffer = nullptr;
    int64 capacity = 0;
    int64 fileEnd = 0;
    int64 filePointer = 0;
    bool dirty = false;
    bool cleared = false; // If file is cleared before creating buffer then we can skip reading its contents

    explicit BufferedFile(const char* path, Allocator* allocator = gGlobalAllocator) :
        file(File::OpenOrCreate(path)),
        allocator(allocator) {}

    NO_DISCARD FORCE_INLINE static int64 AlignUpToChunk(int64 value) {
        return ((value + GROW_CHUNK - 1) / GROW_CHUNK) * GROW_CHUNK;
    }

    FORCE_INLINE void AllocateBuffer() {
        int64 diskSize = 0;
        file.GetFileSize(diskSize);

        ASSERT(diskSize <= MAX_FILE_SIZE, "Buffered file is too large!");

        capacity = diskSize > GROW_CHUNK ? AlignUpToChunk(diskSize) : GROW_CHUNK;
        buffer = allocator->Allocate(static_cast<size_t>(capacity));

        if (diskSize == 0 || cleared) {
            fileEnd = 0;
        } else {
            file.SetFilePointerTo(0);
            uint32 bytesRead = 0;
            file.ReadBytes(buffer, static_cast<uint32>(diskSize), bytesRead);
            fileEnd = static_cast<int64>(bytesRead);
        }
    }

    FORCE_INLINE void Grow(int64 requiredEnd) {

        const int64 newCapacity = AlignUpToChunk(requiredEnd);

        ASSERT(newCapacity <= MAX_FILE_SIZE, "Buffered file is too large!");

        uint8* newBuffer = allocator->Allocate(static_cast<size_t>(newCapacity));

        // Only copy contents if file actually has anything written to it
        // otherwise pointless. This can happen when writing to an empty file
        // more bytes than its capacity
        if (fileEnd > 0) {
            std::memcpy(newBuffer, buffer, static_cast<size_t>(fileEnd));
        }

        allocator->Free(buffer);
        buffer = newBuffer;
        capacity = newCapacity;
    }

public:
    FORCE_INLINE static BufferedFile OpenOrCreate(const char* path, Allocator* allocator = gGlobalAllocator) {
        return BufferedFile(path, allocator);
    }

    FORCE_INLINE bool ReadBytes(uint8* bytes, uint32 bytesToRead, uint32& bytesRead) {

        if (!buffer) {
            AllocateBuffer();
        }

        const int64 remainingBytes = fileEnd - filePointer;
        if (remainingBytes == 0) {
            bytesRead = 0;
            return false;
        }

        bytesRead = bytesToRead > remainingBytes ? remainingBytes : bytesToRead;
        memcpy(bytes, buffer + filePointer, bytesRead);
        filePointer += bytesRead;
        return true;
    }

    FORCE_INLINE bool WriteBytes(const uint8* bytes, uint32 bytesToWrite) {

        if (!buffer) {
            AllocateBuffer();
        }

        const int64 endWrite = filePointer + static_cast<int64>(bytesToWrite);
        if (endWrite > capacity) {
            Grow(endWrite);
        }

        memcpy(buffer + filePointer, bytes, bytesToWrite);
        filePointer = endWrite;

        if (filePointer > fileEnd) {
            fileEnd = filePointer;
        }

        dirty = true;
        return true;
    }

    FORCE_INLINE bool SetFilePointerTo(int64 position) {
        ASSERT(position >= 0 && position <= fileEnd, "Tried setting file pointer beyond file bounds!");
        filePointer = position;
        return true;
    }

    FORCE_INLINE bool MoveFilePointerBy(int64 offset) {
        const int64 newPointer = filePointer + offset;
        ASSERT(newPointer >= 0 && newPointer <= fileEnd, "Tried offsetting file pointer beyond file bounds!");
        filePointer = newPointer;
        return true;
    }

    FORCE_INLINE bool GetFilePointer(int64& pointer) {
        pointer = filePointer;
        return true;
    }

    FORCE_INLINE bool GetFileSize(int64& outSize) {
        if (buffer) {
            outSize = fileEnd;
            return true;
        }
        return file.GetFileSize(outSize);
    }

    FORCE_INLINE bool FlushWrites() {

        if (!dirty) {
            return true;
        }

        // The file's contents have been copied
        // to the buffer in case they exist, thus we always
        // start by clearing and then write the whole thing at once
        if (!file.ClearContents()) {
            return false;
        }

        if (fileEnd > 0 && !file.WriteBytes(buffer, static_cast<uint32>(fileEnd))) {
            return false;
        }

        if (!file.FlushWrites()) {
            return false;
        }

        dirty = false;
        cleared = false;
        return true;
    }

    FORCE_INLINE void DiscardWrites() {
        if (!dirty) {
            return;
        }
        allocator->Free(buffer);
        buffer = nullptr;
        capacity = 0;
        fileEnd = 0;
        filePointer = 0;
        dirty = false;
        cleared = false;
    }

    FORCE_INLINE bool ClearContents() {
        fileEnd = 0;
        filePointer = 0;
        dirty = true;
        cleared = true;
        return true;
    }

    FORCE_INLINE bool Close() {
        if (dirty) {
            if (!FlushWrites()) {
                return false;
            }
        }
        if (buffer) {
            allocator->Free(buffer);
            buffer = nullptr;
        }
        capacity = 0;
        fileEnd = 0;
        filePointer = 0;
        return file.Close();
    }

    NO_DISCARD FORCE_INLINE bool IsOpen() const {
        return file.IsOpen();
    }
};
