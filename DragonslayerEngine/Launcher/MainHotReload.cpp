#include <iostream>
#include <Runtime/Engine.hpp>
#include "EngineSetup.hpp"

#if DS_PLATFORM_WINDOWS
#undef APIENTRY
#include <Windows.h>
#elif DS_PLATFORM_LINUX
#include <dlfcn.h>
#include <unistd.h>
#include <sys/stat.h>
#endif

#if WITH_EDITOR
#include "Editor/ImGuiState.hpp"
using SetImGuiStateProc = void (*)(const ImGui::State&);
#endif

/* TODO The whole hot reloading logic should not exists in the shipped build
 * Especially because it means we are duplicating the game code
 */
struct GameEntryPoint {
    char libraryPath[16] = "Game.dll";
    char tempLibraryPath[16] = "GameTemp.dll";

    // TODO Wrap handle and lastWriteTime in IFDEF for each OS (or switch to better types)

#if DS_PLATFORM_WINDOWS
    HMODULE handle = nullptr; // The shared library handle
    FILETIME lastWriteTime = {}; // The last time the library was updated (for hot reloading)
#elif DS_PLATFORM_LINUX
    void* handle = nullptr;              // Shared library handle
    struct timespec lastWriteTime = {};  // Last modification time
#endif

    Engine::RegisterGameSystemsProc startGameProc = nullptr; // The game's entry point
#if WITH_EDITOR
    SetImGuiStateProc setImGuiStateProc = nullptr;
#endif

};

#if DS_PLATFORM_WINDOWS
static bool WinCopyFileWithRetry(const char* src, const char* dst, int maxRetries = 50, int sleepMs = 50) {
    for (uint32 i = 0; i < maxRetries; ++i) {
        if (CopyFileA(src, dst, FALSE)) {
            return true;
        }

        DWORD err = GetLastError();
        if (err != ERROR_SHARING_VIOLATION && err != ERROR_USER_MAPPED_FILE) {
            std::cerr << "[HotReload] CopyFileA failed with error: " << err << std::endl;
            return false;
        }

        Sleep(sleepMs);
    }

    std::cerr << "[HotReload] CopyFileA failed after retries with sharing violation." << std::endl;
    return false;
}

static bool WinLoadGameEntryPoint(GameEntryPoint& gameEntryPoint) {

    // If handle is not null then we are reloading thus
    // we have to free the library first
    if (gameEntryPoint.handle != nullptr) {
        if (!FreeLibrary(gameEntryPoint.handle)) {
            DWORD err = GetLastError();
            std::cout << "[HotReload] FreeLibrary failed with error: " << err << std::endl;
            return false;
        }

        gameEntryPoint.handle = nullptr;

        // The OS may keep the tempLibrary locked for a while even after FreeLibrary is called
        // This gives the OS the necessary breathing room to unlock so we can write to it
        Sleep(100);
    }

    CreateDirectoryA("HotReload", nullptr);

    char tempLibraryPath[64];
    sprintf_s(tempLibraryPath, "HotReload\\GameTemp_%llu.dll", GetTickCount64());

    if (!WinCopyFileWithRetry(gameEntryPoint.libraryPath, tempLibraryPath)) {
        return false;
    }

    HMODULE gameLib = LoadLibraryA(tempLibraryPath);
    if (!gameLib) {
        DWORD err = GetLastError();
        std::cerr << "[HotReload] LoadLibraryA failed with error: " << err << std::endl;
        return false;
    }
    gameEntryPoint.handle = gameLib;

    Engine::RegisterGameSystemsProc startGameProc = reinterpret_cast<Engine::RegisterGameSystemsProc>(GetProcAddress(gameLib, "RegisterSystems"));
    if (!startGameProc) {
        DWORD err = GetLastError();
        std::cerr << "[HotReload] GetProcAddress failed for RegisterSystems with error: " << err << std::endl;
        return false;
    }
    gameEntryPoint.startGameProc = startGameProc;

#if WITH_EDITOR
    SetImGuiStateProc setImGuiStateProc = reinterpret_cast<SetImGuiStateProc>(GetProcAddress(gameLib, "SetImGuiState"));
    if (!setImGuiStateProc) {
        DWORD err = GetLastError();
        std::cerr << "[HotReload] GetProcAddress failed for SetImGuiState with error: " << err << std::endl;
        return false;
    }
    gameEntryPoint.setImGuiStateProc = setImGuiStateProc;
#endif

    return true;
}

static void WinUpdateWriteTimeGameEntryPoint(GameEntryPoint& gameEntryPoint) {

    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!GetFileAttributesExA(gameEntryPoint.libraryPath, GetFileExInfoStandard, &data)) {
        DWORD err = GetLastError();
        std::cerr << "[HotReload] GetFileAttributesExA failed with error: " << err << std::endl;
        return;
    }

    gameEntryPoint.lastWriteTime = data.ftLastWriteTime;
}

static bool WinShouldReloadGameEntryPoint(const GameEntryPoint& gameEntryPoint, FILETIME& lastWriteTime) {

    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!GetFileAttributesExA(gameEntryPoint.libraryPath, GetFileExInfoStandard, &data)) {
        DWORD err = GetLastError();
        std::cerr << "[HotReload] GetFileAttributesExA failed with error: " << err << std::endl;
        return false;
    }

    // -1 means First file time is earlier than second file time.
    if (CompareFileTime(&gameEntryPoint.lastWriteTime, &data.ftLastWriteTime) >= 0) {
        return false;
    }

    // Try to open the file for writing with NO sharing allowed.
    // If the linker is still "touching" the file, this will return INVALID_HANDLE_VALUE.
    HANDLE hFile = CreateFileA(
        gameEntryPoint.libraryPath,
        GENERIC_WRITE, // We want to write (even if we don't actually do it)
        0, // DO NOT SHARE FILE (this is the important part),
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        // The file is still locked by the linker/compiler
        return false;
    }

    // If we got here, the linker is 100% finished and has released the file.
    CloseHandle(hFile);

    lastWriteTime = data.ftLastWriteTime;

    return true;
}
#endif

#if DS_PLATFORM_LINUX
bool WinLoadGameEntryPoint(GameEntryPoint& gameEntryPoint) {

    // If handle is not null then we are reloading thus
    // we have to free the library first
    if (gameEntryPoint.handle != nullptr) {

        if (!FreeLibrary(gameEntryPoint.handle)) {
            DWORD err = GetLastError();
            std::cout << "FreeLibrary failed with error: " << err << std::endl;
            return false;
        }

        gameEntryPoint.handle = nullptr;

        // The OS may keep the tempLibrary locked for a while even after FreeLibrary is called
        // This gives the OS the necessary breathing room to unlock so we can write to it
        Sleep(100);
    }

    if (!CopyFileA(gameEntryPoint.libraryPath, gameEntryPoint.tempLibraryPath, FALSE)) {
        DWORD err = GetLastError();
        std::cerr << "CopyFileA failed with error: " << err << std::endl;
        return false;
    }

    HMODULE gameLib = LoadLibraryA(gameEntryPoint.tempLibraryPath);
    if (!gameLib) {
        DWORD err = GetLastError();
        std::cerr << "LoadLibraryA failed with error: " << err << std::endl;
        return false;
    }
    gameEntryPoint.handle = gameLib;

    RegisterGameSystemsProc startGameProc = reinterpret_cast<RegisterGameSystemsProc>(GetProcAddress(gameLib, "RegisterSystems"));
    if (!startGameProc) {
        DWORD err = GetLastError();
        std::cerr << "GetProcAddress failed with error: " << err << std::endl;
        return false;
    }
    gameEntryPoint.startGameProc = startGameProc;

    return true;
}

bool WinShouldReloadGameEntryPoint(const GameEntryPoint & gameEntryPoint, FILETIME& lastWriteTime) {

    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!GetFileAttributesExA(gameEntryPoint.libraryPath, GetFileExInfoStandard, &data)) {
        DWORD err = GetLastError();
        std::cerr << "GetFileAttributesExA failed with error: " << err << std::endl;
        return false;
    }

    // -1 means First file time is earlier than second file time.
    if (CompareFileTime(&gameEntryPoint.lastWriteTime, &data.ftLastWriteTime) >= 0) {
        return false;
    }

    lastWriteTime = data.ftLastWriteTime;

    return true;
}
#endif

// Helper function to recursively delete a directory's contents
static bool DeleteDirectoryRecursively(const PathString& directoryPath) {

    const PathString searchPath = directoryPath + "\\*";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath.CStr(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return false; // Directory doesn't exist or we don't have permission
    }

    do {
        PathString fileName = findData.cFileName;

        // Skip the current and parent directory markers
        if (fileName != "." && fileName != "..") {
            PathString filePath = directoryPath + "\\" + fileName;

            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                // It's a folder, recurse into it
                DeleteDirectoryRecursively(filePath);
            } else {
                // It's a file, delete it
                DeleteFileA(filePath.CStr());
            }
        }
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);

    // Now that it's empty, remove the directory itself
    return RemoveDirectoryA(directoryPath.CStr());
}

static void CleanupHotReloadDir() {
    const char* dirName = "HotReload";

    // 1. Check if it exists and delete it if it does (Equivalent to fs::exists + fs::remove_all)
    DWORD attrib = GetFileAttributesA(dirName);
    if (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY)) {
        if (!DeleteDirectoryRecursively(dirName)) {
            std::cerr << "[HotReload] Failed to clean directory. Error Code: " << GetLastError() << std::endl;
        }
    }

    // 2. Create the fresh directory (Equivalent to fs::create_directory)
    if (!CreateDirectoryA(dirName, nullptr)) {
        DWORD err = GetLastError();
        // Ignore the error if the directory somehow already exists
        if (err != ERROR_ALREADY_EXISTS) {
            std::cerr << "[HotReload] Failed to create directory. Error Code: " << err << std::endl;
        }
    }
}

int main(int argc, char *argv[]) {

    CleanupHotReloadDir();

    GameEntryPoint gameEntryPoint;

    if (!WinLoadGameEntryPoint(gameEntryPoint)) {
        std::cerr << "[HotReload] Initial load failed." << std::endl;
        return -1;
    }

    WinUpdateWriteTimeGameEntryPoint(gameEntryPoint);

    Engine engine = SetupEngine(argc, argv);

#if WITH_EDITOR
    ImGui::State imGuiState{};
    ImGui::GetState(imGuiState);
    gameEntryPoint.setImGuiStateProc(imGuiState);
#endif

    engine.SetRegisterGameSystemProc(gameEntryPoint.startGameProc);
    engine.Start();

    while (engine.Update(0)) {

        FILETIME newWriteTime = {};
        if (WinShouldReloadGameEntryPoint(gameEntryPoint, newWriteTime)) {

            Log::Info("[HotReload] HotReload triggered, attempting to load new Game.dll...");

            if (!WinLoadGameEntryPoint(gameEntryPoint)) {
                Log::Error("[HotReload] Failed to reload Game.dll. Retrying...");
                continue;
            }

            Log::Info("[HotReload] HotReload was successful!");

            gameEntryPoint.lastWriteTime = newWriteTime;

#if WITH_EDITOR
            gameEntryPoint.setImGuiStateProc(imGuiState);
#endif
            engine.SetRegisterGameSystemProc(gameEntryPoint.startGameProc);
        }
    }

    engine.End();

    return 0;
}