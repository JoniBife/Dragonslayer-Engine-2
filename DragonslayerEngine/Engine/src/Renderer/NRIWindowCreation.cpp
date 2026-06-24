#include "Renderer/NRIWindowCreation.hpp"
#include "Core/Allocators/FreeListAllocator.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#if DS_PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#if DS_PLATFORM_LINUX
#define GLFW_EXPOSE_NATIVE_X11
#endif

#include <GLFW/glfw3native.h>
#include <NRI.h>
#include <Extensions/NRISwapChain.h>

nri::Window CreateNRIWindow(GLFWwindow* glfwWindow) {
    nri::Window window = {};

#if DS_PLATFORM_WINDOWS
    window.windows.hwnd = glfwGetWin32Window(glfwWindow);
#elif DS_PLATFORM_LINUX
    window.x11.dpy = glfwGetX11Display();
    window.x11.window = glfwGetX11Window(glfwWindow);
#endif

    // TODO Support other platforms
    /*#elif (NRIF_PLATFORM == NRIF_COCOA)
        out->metal.caMetalLayer = GetMetalLayer(glfwWindow);
    #elif (NRIF_PLATFORM == NRIF_WAYLAND)
        out->wayland.display = glfwGetWaylandDisplay();
        out->wayland.surface = glfwGetWaylandWindow(glfwWindow);
    #endif*/

    return window;
}
