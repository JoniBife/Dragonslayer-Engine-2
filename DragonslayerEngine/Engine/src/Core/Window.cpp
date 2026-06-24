
#include "Core/Window.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <iostream>

//#if DS_PLATFORM_WINDOWS
//#define GLFW_EXPOSE_NATIVE_WIN32
//#include <GLFW/glfw3native.h>
//#include <dwmapi.h>
//#endif

#include "Core/Log.hpp"
#include "Core/Containers/Array.hpp"

// TODO Is this really necessary after glfw and why??
#if DS_PLATFORM_WINDOWS
    #undef APIENTRY // defined in GLFW
#endif

static void* GLFWAllocate(size_t size, void* user)
{
    FreeListAllocator* allocator = static_cast<FreeListAllocator*>(user);
    return allocator->Allocate(size);
}

static void* GLFWReallocate(void* block, size_t size, void* user)
{
    FreeListAllocator* allocator = static_cast<FreeListAllocator*>(user);
    return allocator->Reallocate(static_cast<uint8*>(block), size);
}

static void GLFWFree(void* block, void* user)
{
    FreeListAllocator* allocator = static_cast<FreeListAllocator*>(user);
    allocator->Free(static_cast<uint8*>(block));
}

static void GLFWErrorCallback(int error, const char* description)
{
    char errorString[MAX_LOG_LENGTH];
    sprintf(errorString, "[GLFW] %d : %s\n", error, description);
    Log::Error(errorString);
}

static void GLFWWindowSizeCallBack(GLFWwindow* glfwWindow, int width, int height)
{
    GLFWState* glfwState = static_cast<GLFWState*>(glfwGetWindowUserPointer(glfwWindow));

    Window::State* windowState = static_cast<Window::State*>(glfwState->windowState);
    windowState->width = width;
    windowState->height = height;
}

static void GLFWWindowFocusCallback(GLFWwindow* glfwWindow, int focused)
{
    GLFWState* glfwState = static_cast<GLFWState*>(glfwGetWindowUserPointer(glfwWindow));

    Window::State* windowState = static_cast<Window::State*>(glfwState->windowState);
    windowState->isFocused = focused;
}

Window::Window(const WindowSettings& windowSettings, FreeListAllocator& parentAllocator) :
    glfwState(nullptr, nullptr),
    allocator(parentAllocator, windowSettings.maxMemory),
    glfwAllocator(allocator.Allocate<GLFWallocator>()),
    state() {

    glfwAllocator->allocate = GLFWAllocate;
    glfwAllocator->reallocate = GLFWReallocate;
    glfwAllocator->deallocate = GLFWFree;
    glfwAllocator->user = &this->allocator;

    glfwInitAllocator(glfwAllocator);

    glfwSetErrorCallback(GLFWErrorCallback);

    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    // TODO Decide how we handle the starting resolution
    //GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    //float contentScale = 1.0f;
    //
    //float unused;
    //glfwGetMonitorContentScale(monitor, &contentScale, &unused);
    //printf("DPI scale %.1f%% (%s)\n", contentScale * 100.0f, "quality");
    //
    //const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);
    //const uint32 screenW = static_cast<uint32>(videoMode->width);
    //const uint32 screenH = static_cast<uint32>(videoMode->height);
    //
    //std::cout << "Monitor Resolution: " << screenW << " x " << screenH << std::endl;

    // The default is a focused, visible, resizable window with decorations
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

    GLFWwindow* window =
            glfwCreateWindow(static_cast<int>(windowSettings.width), static_cast<int>(windowSettings.height),
                             windowSettings.title, nullptr, nullptr);

    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    state.width = windowSettings.width;
    state.height = windowSettings.height;
    state.isFocused = true; // TODO Does a window start focused??

    glfwState.windowState = &state;

    glfwSetWindowUserPointer(window, &glfwState);

    glfwSetWindowSizeCallback(window, GLFWWindowSizeCallBack);
    glfwSetWindowFocusCallback(window, GLFWWindowFocusCallback);
    // TODO Add other window callbacks for stuff like monitor change, DPI change etc...

    //HWND hwnd = glfwGetWin32Window(window); // Get the native Win32 handle
    //BOOL useDarkMode = TRUE;
    //DwmSetWindowAttribute(
    //    hwnd,
    //    20, // DWMWA_USE_IMMERSIVE_DARK_MODE (use 19 for older Windows 10 versions)
    //    &useDarkMode,
    //    sizeof(useDarkMode)
    //);

    glfwWindow = window;
}

Window::~Window() {
    glfwDestroyWindow(glfwWindow);
    glfwTerminate();
}

bool Window::ShouldWindowClose() const { return glfwWindowShouldClose(glfwWindow); }

bool Window::IsInFocus() const { return glfwGetWindowAttrib(glfwWindow, GLFW_FOCUSED); }

bool Window::IsMinimized() const { return glfwGetWindowAttrib(glfwWindow, GLFW_ICONIFIED); }

double Window::GetTimeFromCreation() { return glfwGetTime(); }

void Window::GetSize(uint32& width, uint32& height) const {
    int windowWidth, windowHeight;
    glfwGetWindowSize(glfwWindow, &windowWidth, &windowHeight);
    width = windowWidth;
    height = windowHeight;
}

GLFWwindow* Window::GetGLFWWindow() const { return glfwWindow; }

size_t Window::GetUsedMemory() const {
    return allocator.GetAllocatedMemory();
}
