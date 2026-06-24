#pragma once

/**
 * Helper to be create an NRI window without having to include platform specific
 * window management headers on Engine classes which leads to
 * a bunch of type ambiguations (.e.g X11 also declares a Windows type, a Time type etc..)
 */
namespace nri { struct Window; }
nri::Window CreateNRIWindow(struct GLFWwindow* glfwWindow);
