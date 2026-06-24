#pragma once

#include "Types.hpp"
#include "Macros.hpp"
#include "Assert.hpp"
#include "NotCopyable.hpp"

#if DS_PLATFORM_WINDOWS
class WinThread;
using Thread = WinThread;
#elif DS_PLATFORM_LINUX
class UnixThread;
using Thread = UnixThread;
#endif

// User thread entry point. Receives the user-data pointer passed at construction.
using ThreadEntry = void(*)(Thread& thread);

#if DS_PLATFORM_WINDOWS

#undef APIENTRY
#include <Windows.h>

class WinThread : public NotCopyable {

private:
    HANDLE handle = INVALID_HANDLE_VALUE;
    DWORD id = 0;
    ThreadEntry function = nullptr;
    void* userData = nullptr;
    bool joined = false;

public:
    WinThread(ThreadEntry function, void* userData) : function(function), userData(userData) {
        handle = CreateThread(
            nullptr,
            0, /*Default stack size used by the executable*/
            &Trampoline,
            this,
            0, /*Thread runs immediately after creation*/
            &id
        );
        if (handle == nullptr) {
            const DWORD error = GetLastError();
            FAIL("ERROR %d: Failed to create thread!", error);
        }
    }

    ~WinThread() {
        if (handle != INVALID_HANDLE_VALUE) {
            Join();
            CloseHandle(handle);
            handle = INVALID_HANDLE_VALUE;
        }
    }

    FORCE_INLINE void Join() {
        ASSERT(handle != INVALID_HANDLE_VALUE, "Thread is not valid!");
        if (!joined) {
            WaitForSingleObject(handle, INFINITE);
            joined = true;
        }
    }

    NO_DISCARD FORCE_INLINE HANDLE GetNativeHandle() {
        return handle;
    }

    NO_DISCARD FORCE_INLINE uint64 GetID() const {
        return id;
    }

    template<typename T>
    NO_DISCARD FORCE_INLINE T& GetUserData() {
        return *static_cast<T*>(userData);
    }

    template<typename T>
    NO_DISCARD FORCE_INLINE const T& GetUserData() const {
        return *static_cast<T*>(userData);
    }

private:
    static DWORD WINAPI Trampoline(LPVOID data) {
        auto* thread = static_cast<WinThread*>(data);
        thread->function(*thread);
        return 0;
    }
};

#endif

#if DS_PLATFORM_LINUX

#include <pthread.h> // For pthread_create(), pthread_join()

class UnixThread : public NotCopyable {

private:
    pthread_t handle = 0;
    ThreadEntry function = nullptr;
    void* userData = nullptr;
    bool joined = false;

public:
    UnixThread(ThreadEntry function, void* userData)
        : function(function), userData(userData) {
        const int result = pthread_create(
            &handle,
            nullptr,
            &Trampoline,
            this
        );
        if (result != 0) {
            FAIL("ERROR %d: Failed to create thread!", result);
        }
    }

    ~UnixThread() {
        if (handle != 0) {
            Join();
            handle = 0;
        }
    }

    FORCE_INLINE void Join() {
        ASSERT(handle != 0, "Thread is not valid!");
        if (!joined) {
            pthread_join(handle, nullptr);
            joined = true;
        }
    }

    NO_DISCARD FORCE_INLINE pthread_t GetNativeHandle() {
        return handle;
    }

    NO_DISCARD FORCE_INLINE uint64 GetID() const {
        return static_cast<uint64>(handle);
    }

    template<typename T>
    NO_DISCARD FORCE_INLINE T& GetUserData() {
        return *static_cast<T*>(userData);
    }

    template<typename T>
    NO_DISCARD FORCE_INLINE const T& GetUserData() const {
        return *static_cast<T*>(userData);
    }

private:
    static void* Trampoline(void* data) {
        auto* thread = static_cast<UnixThread*>(data);
        thread->function(*thread);
        return nullptr;
    }
};

#endif
