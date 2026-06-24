#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "Core/Allocators/LinearAllocator.hpp"
#include "Core/Containers/Array.hpp"
#include "Core/NotCopyable.hpp"
#include "Core/Types.hpp"
#include "Math/Vec2.hpp"


enum class InputState : uint8 {
	Release = GLFW_RELEASE, // 0
	Press = GLFW_PRESS, // 1
	Repeat = GLFW_REPEAT, // 2
	Hold = 3,
	None = 4,
};

enum class KeyCode : uint16
{
    // Digits (top row)
    Zero  = GLFW_KEY_0,
    One   = GLFW_KEY_1,
    Two   = GLFW_KEY_2,
    Three = GLFW_KEY_3,
    Four  = GLFW_KEY_4,
    Five  = GLFW_KEY_5,
    Six   = GLFW_KEY_6,
    Seven = GLFW_KEY_7,
    Eight = GLFW_KEY_8,
    Nine  = GLFW_KEY_9,

    // Letters
    A = GLFW_KEY_A,
    B = GLFW_KEY_B,
    C = GLFW_KEY_C,
    D = GLFW_KEY_D,
    E = GLFW_KEY_E,
    F = GLFW_KEY_F,
    G = GLFW_KEY_G,
    H = GLFW_KEY_H,
    I = GLFW_KEY_I,
    J = GLFW_KEY_J,
    K = GLFW_KEY_K,
    L = GLFW_KEY_L,
    M = GLFW_KEY_M,
    N = GLFW_KEY_N,
    O = GLFW_KEY_O,
    P = GLFW_KEY_P,
    Q = GLFW_KEY_Q,
    R = GLFW_KEY_R,
    S = GLFW_KEY_S,
    T = GLFW_KEY_T,
    U = GLFW_KEY_U,
    V = GLFW_KEY_V,
    W = GLFW_KEY_W,
    X = GLFW_KEY_X,
    Y = GLFW_KEY_Y,
    Z = GLFW_KEY_Z,

    // Function keys
    Escape = GLFW_KEY_ESCAPE,
    F1  = GLFW_KEY_F1,
    F2  = GLFW_KEY_F2,
    F3  = GLFW_KEY_F3,
    F4  = GLFW_KEY_F4,
    F5  = GLFW_KEY_F5,
    F6  = GLFW_KEY_F6,
    F7  = GLFW_KEY_F7,
    F8  = GLFW_KEY_F8,
    F9  = GLFW_KEY_F9,
    F10 = GLFW_KEY_F10,
    F11 = GLFW_KEY_F11,
    F12 = GLFW_KEY_F12,
    Pause   = GLFW_KEY_PAUSE,
    Scroll  = GLFW_KEY_SCROLL_LOCK,
    Capital = GLFW_KEY_CAPS_LOCK,
    NumLock = GLFW_KEY_NUM_LOCK,

    // Numpad
    Num0 = GLFW_KEY_KP_0,
    Num1 = GLFW_KEY_KP_1,
    Num2 = GLFW_KEY_KP_2,
    Num3 = GLFW_KEY_KP_3,
    Num4 = GLFW_KEY_KP_4,
    Num5 = GLFW_KEY_KP_5,
    Num6 = GLFW_KEY_KP_6,
    Num7 = GLFW_KEY_KP_7,
    Num8 = GLFW_KEY_KP_8,
    Num9 = GLFW_KEY_KP_9,

    NumAdd    = GLFW_KEY_KP_ADD,
    NumSub    = GLFW_KEY_KP_SUBTRACT,
    NumMul    = GLFW_KEY_KP_MULTIPLY,
    NumDiv    = GLFW_KEY_KP_DIVIDE,
    NumDel    = GLFW_KEY_KP_DECIMAL,
    NumEnter  = GLFW_KEY_KP_ENTER,

    // Modifiers
    LControl = GLFW_KEY_LEFT_CONTROL,
    RControl = GLFW_KEY_RIGHT_CONTROL,
    LShift   = GLFW_KEY_LEFT_SHIFT,
    RShift   = GLFW_KEY_RIGHT_SHIFT,
    LAlt     = GLFW_KEY_LEFT_ALT,
    RAlt     = GLFW_KEY_RIGHT_ALT,

    // Navigation / editing
    Tab       = GLFW_KEY_TAB,
    Return    = GLFW_KEY_ENTER,
    Backspace = GLFW_KEY_BACKSPACE,
    Insert    = GLFW_KEY_INSERT,
    Del       = GLFW_KEY_DELETE,
    Home      = GLFW_KEY_HOME,
    End       = GLFW_KEY_END,
    PageUp    = GLFW_KEY_PAGE_UP,
    PageDown  = GLFW_KEY_PAGE_DOWN,

    // Arrows
    Left  = GLFW_KEY_LEFT,
    Right = GLFW_KEY_RIGHT,
    Up    = GLFW_KEY_UP,
    Down  = GLFW_KEY_DOWN,

    // Symbols
    Minus      = GLFW_KEY_MINUS,
    Equals     = GLFW_KEY_EQUAL,
    LBracket   = GLFW_KEY_LEFT_BRACKET,
    RBracket   = GLFW_KEY_RIGHT_BRACKET,
    Tilda      = GLFW_KEY_GRAVE_ACCENT,
    Semicolon  = GLFW_KEY_SEMICOLON,
    Apostrophe = GLFW_KEY_APOSTROPHE,
    BackSlash  = GLFW_KEY_BACKSLASH,
    Comma      = GLFW_KEY_COMMA,
    Period     = GLFW_KEY_PERIOD,
    Slash      = GLFW_KEY_SLASH,
    Space      = GLFW_KEY_SPACE,

    // End marker
    NUM = GLFW_KEY_LAST
};

enum class MouseButtonCode : uint8 {

    Left = GLFW_MOUSE_BUTTON_LEFT,
    Right = GLFW_MOUSE_BUTTON_RIGHT,
    Middle = GLFW_MOUSE_BUTTON_MIDDLE,
    Button4 = GLFW_MOUSE_BUTTON_4,
    Button5 = GLFW_MOUSE_BUTTON_5,
    Button6 = GLFW_MOUSE_BUTTON_6,
    Button7 = GLFW_MOUSE_BUTTON_7,
    Button8 = GLFW_MOUSE_BUTTON_8,

    NUM = GLFW_MOUSE_BUTTON_LAST
};

class ENGINE_API Input : public NotCopyable {

public:
    struct State {
        Array<InputState, LinearAllocator> keyStates;
        Array<InputState, LinearAllocator> mouseButtonStates;
        Vec2 cursorPosition;
        Vec2 previousCursorPosition;
        Vec2 mouseDelta;
        float scrollYOffset = 0.;
        bool newScrollEvent = false;
        bool suppressNextDelta = true;
        bool ignoreInputs = false;
        bool cursorVisible = true;
#if WITH_EDITOR
        // Special state used to disable game input when editor is in focus
        bool isEnabled = true;
#endif
    };

private:
	GLFWwindow* glfwWindow; // The window class owns the glfwWindow not Input

    LinearAllocator allocator;

    State state;

    static Input* instance;

public:
    Input(GLFWwindow* window, class FreeListAllocator& parentAllocator);
    ~Input();

	void PollInputEvents();

	NO_DISCARD static bool IsKeyPressed(KeyCode keyCode);
	NO_DISCARD static bool IsKeyHeld(KeyCode keyCode);
	NO_DISCARD static bool IsKeyDown(KeyCode keyCode);
	NO_DISCARD static bool IsKeyReleased(KeyCode keyCode);

	NO_DISCARD static bool IsMouseButtonPressed(MouseButtonCode mouseButtonCode);
	NO_DISCARD static bool IsMouseButtonHeld(MouseButtonCode mouseButtonCode);
	NO_DISCARD static bool IsMouseButtonDown(MouseButtonCode mouseButtonCode);
	NO_DISCARD static bool IsMouseButtonReleased(MouseButtonCode mouseButtonCode);


    static bool IsCursorVisible();
	static void SetCursorVisibility(bool visible);

#if WITH_EDITOR
    static void ToggleInputEnabled(bool enabled);
#endif

	/* Position is relative to the upper left corner of the main glfw window*/
	NO_DISCARD static Vec2 GetCursorPosition();
	static void SetCursorPosition(const Vec2& position);

	NO_DISCARD static Vec2 GetCursorDelta();
	static void SuppressNextCursorDelta();

	NO_DISCARD static double GetMouseScroll();


};