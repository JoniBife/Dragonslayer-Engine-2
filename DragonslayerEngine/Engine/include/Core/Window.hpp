#pragma once

#include "Core/Allocators/FreeListAllocator.hpp"
#include "Core/Allocators/LinearAllocator.hpp"
#include "Core/Export.hpp"
#include "Core/Macros.hpp"
#include "Core/NotCopyable.hpp"
#include "Core/Types.hpp"

struct GLFWState {
    void* windowState;
    void* inputState;
};

struct ENGINE_API WindowSettings {

    // The title of the window up to 64 characters
    char title[64];

    // Maximum memory used by the Window System and GLFW (subset of maxMemory)
    size_t maxMemory;

    // The initial width of the window in pixels
    uint32 width;

    // The initial height of the window in pixels
    uint32 height;
};

/*
 * Simple GLFW Window wrapper, handles setup and destruction
 * TODO Reconsider window destruction in destructor
 * TODO Decide what to do with monitor data (DPI, resolution etc..)
 */
class ENGINE_API Window final : public NotCopyable {

public:
    struct State {
        // The current window width
        uint32 width;

        // The current window height
        uint32 height;

        // Whether the window is focused or not
        bool isFocused;
    };

private:
    struct GLFWwindow* glfwWindow;

    GLFWState glfwState;

    // Needs to be Freelist (or general purpose) due to GLFW's expectation of a realloc function
    FreeListAllocator allocator;
    struct GLFWallocator* glfwAllocator;

    State state;

public:
    Window(const WindowSettings& windowSettings, FreeListAllocator& parentAllocator);
    ~Window();

    NO_DISCARD bool ShouldWindowClose() const;

    NO_DISCARD bool IsInFocus() const;

    NO_DISCARD bool IsMinimized() const;

    NO_DISCARD static double GetTimeFromCreation();

    void GetSize(uint32& width, uint32& height) const;

    NO_DISCARD GLFWwindow* GetGLFWWindow() const;

    NO_DISCARD size_t GetUsedMemory() const;
};