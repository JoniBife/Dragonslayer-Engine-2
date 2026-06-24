#include "Runtime/Input.hpp"

#include <Editor/TimeScope.hpp>

#include "Core/Window.hpp"

#if WITH_EDITOR
#include <imgui.h>
#include "Editor/EditorGlobals.hpp"

static ImGuiKey RemapKey(int32 key) {
    switch (key) {
        case GLFW_KEY_SPACE:
            return ImGuiKey_Space;
        case GLFW_KEY_APOSTROPHE:
            return ImGuiKey_Apostrophe;
        case GLFW_KEY_COMMA:
            return ImGuiKey_Comma;
        case GLFW_KEY_MINUS:
            return ImGuiKey_Minus;
        case GLFW_KEY_PERIOD:
            return ImGuiKey_Period;
        case GLFW_KEY_SLASH:
            return ImGuiKey_Slash;
        case GLFW_KEY_0:
            return ImGuiKey_0;
        case GLFW_KEY_1:
            return ImGuiKey_1;
        case GLFW_KEY_2:
            return ImGuiKey_2;
        case GLFW_KEY_3:
            return ImGuiKey_3;
        case GLFW_KEY_4:
            return ImGuiKey_4;
        case GLFW_KEY_5:
            return ImGuiKey_5;
        case GLFW_KEY_6:
            return ImGuiKey_6;
        case GLFW_KEY_7:
            return ImGuiKey_7;
        case GLFW_KEY_8:
            return ImGuiKey_8;
        case GLFW_KEY_9:
            return ImGuiKey_9;
        case GLFW_KEY_SEMICOLON:
            return ImGuiKey_Semicolon;
        case GLFW_KEY_EQUAL:
            return ImGuiKey_Equal;
        case GLFW_KEY_A:
            return ImGuiKey_A;
        case GLFW_KEY_B:
            return ImGuiKey_B;
        case GLFW_KEY_C:
            return ImGuiKey_C;
        case GLFW_KEY_D:
            return ImGuiKey_D;
        case GLFW_KEY_E:
            return ImGuiKey_E;
        case GLFW_KEY_F:
            return ImGuiKey_F;
        case GLFW_KEY_G:
            return ImGuiKey_G;
        case GLFW_KEY_H:
            return ImGuiKey_H;
        case GLFW_KEY_I:
            return ImGuiKey_I;
        case GLFW_KEY_J:
            return ImGuiKey_J;
        case GLFW_KEY_K:
            return ImGuiKey_K;
        case GLFW_KEY_L:
            return ImGuiKey_L;
        case GLFW_KEY_M:
            return ImGuiKey_M;
        case GLFW_KEY_N:
            return ImGuiKey_N;
        case GLFW_KEY_O:
            return ImGuiKey_O;
        case GLFW_KEY_P:
            return ImGuiKey_P;
        case GLFW_KEY_Q:
            return ImGuiKey_Q;
        case GLFW_KEY_R:
            return ImGuiKey_R;
        case GLFW_KEY_S:
            return ImGuiKey_S;
        case GLFW_KEY_T:
            return ImGuiKey_T;
        case GLFW_KEY_U:
            return ImGuiKey_U;
        case GLFW_KEY_V:
            return ImGuiKey_V;
        case GLFW_KEY_W:
            return ImGuiKey_W;
        case GLFW_KEY_X:
            return ImGuiKey_X;
        case GLFW_KEY_Y:
            return ImGuiKey_Y;
        case GLFW_KEY_Z:
            return ImGuiKey_Z;
        case GLFW_KEY_LEFT_BRACKET:
            return ImGuiKey_LeftBracket;
        case GLFW_KEY_BACKSLASH:
            return ImGuiKey_Backslash;
        case GLFW_KEY_RIGHT_BRACKET:
            return ImGuiKey_RightBracket;
        case GLFW_KEY_GRAVE_ACCENT:
            return ImGuiKey_GraveAccent;
        case GLFW_KEY_ESCAPE:
            return ImGuiKey_Escape;
        case GLFW_KEY_ENTER:
            return ImGuiKey_Enter;
        case GLFW_KEY_TAB:
            return ImGuiKey_Tab;
        case GLFW_KEY_BACKSPACE:
            return ImGuiKey_Backspace;
        case GLFW_KEY_INSERT:
            return ImGuiKey_Insert;
        case GLFW_KEY_DELETE:
            return ImGuiKey_Delete;
        case GLFW_KEY_RIGHT:
            return ImGuiKey_RightArrow;
        case GLFW_KEY_LEFT:
            return ImGuiKey_LeftArrow;
        case GLFW_KEY_DOWN:
            return ImGuiKey_DownArrow;
        case GLFW_KEY_UP:
            return ImGuiKey_UpArrow;
        case GLFW_KEY_PAGE_UP:
            return ImGuiKey_PageUp;
        case GLFW_KEY_PAGE_DOWN:
            return ImGuiKey_PageDown;
        case GLFW_KEY_HOME:
            return ImGuiKey_Home;
        case GLFW_KEY_END:
            return ImGuiKey_End;
        case GLFW_KEY_CAPS_LOCK:
            return ImGuiKey_CapsLock;
        case GLFW_KEY_SCROLL_LOCK:
            return ImGuiKey_ScrollLock;
        case GLFW_KEY_NUM_LOCK:
            return ImGuiKey_NumLock;
        case GLFW_KEY_PRINT_SCREEN:
            return ImGuiKey_PrintScreen;
        case GLFW_KEY_PAUSE:
            return ImGuiKey_Pause;
        case GLFW_KEY_F1:
            return ImGuiKey_F1;
        case GLFW_KEY_F2:
            return ImGuiKey_F2;
        case GLFW_KEY_F3:
            return ImGuiKey_F3;
        case GLFW_KEY_F4:
            return ImGuiKey_F4;
        case GLFW_KEY_F5:
            return ImGuiKey_F5;
        case GLFW_KEY_F6:
            return ImGuiKey_F6;
        case GLFW_KEY_F7:
            return ImGuiKey_F7;
        case GLFW_KEY_F8:
            return ImGuiKey_F8;
        case GLFW_KEY_F9:
            return ImGuiKey_F9;
        case GLFW_KEY_F10:
            return ImGuiKey_F10;
        case GLFW_KEY_F11:
            return ImGuiKey_F11;
        case GLFW_KEY_F12:
            return ImGuiKey_F12;
        case GLFW_KEY_KP_0:
            return ImGuiKey_Keypad0;
        case GLFW_KEY_KP_1:
            return ImGuiKey_Keypad1;
        case GLFW_KEY_KP_2:
            return ImGuiKey_Keypad2;
        case GLFW_KEY_KP_3:
            return ImGuiKey_Keypad3;
        case GLFW_KEY_KP_4:
            return ImGuiKey_Keypad4;
        case GLFW_KEY_KP_5:
            return ImGuiKey_Keypad5;
        case GLFW_KEY_KP_6:
            return ImGuiKey_Keypad6;
        case GLFW_KEY_KP_7:
            return ImGuiKey_Keypad7;
        case GLFW_KEY_KP_8:
            return ImGuiKey_Keypad8;
        case GLFW_KEY_KP_9:
            return ImGuiKey_Keypad9;
        case GLFW_KEY_KP_DECIMAL:
            return ImGuiKey_KeypadDecimal;
        case GLFW_KEY_KP_DIVIDE:
            return ImGuiKey_KeypadDivide;
        case GLFW_KEY_KP_MULTIPLY:
            return ImGuiKey_KeypadMultiply;
        case GLFW_KEY_KP_SUBTRACT:
            return ImGuiKey_KeypadSubtract;
        case GLFW_KEY_KP_ADD:
            return ImGuiKey_KeypadAdd;
        case GLFW_KEY_KP_ENTER:
            return ImGuiKey_KeypadEnter;
        case GLFW_KEY_KP_EQUAL:
            return ImGuiKey_KeypadEqual;
        case GLFW_KEY_LEFT_SHIFT:
            return ImGuiKey_LeftShift;
        case GLFW_KEY_LEFT_CONTROL:
            return ImGuiKey_LeftCtrl;
        case GLFW_KEY_LEFT_ALT:
            return ImGuiKey_LeftAlt;
        case GLFW_KEY_LEFT_SUPER:
            return ImGuiKey_LeftSuper;
        case GLFW_KEY_RIGHT_SHIFT:
            return ImGuiKey_RightShift;
        case GLFW_KEY_RIGHT_CONTROL:
            return ImGuiKey_RightCtrl;
        case GLFW_KEY_RIGHT_ALT:
            return ImGuiKey_RightAlt;
        case GLFW_KEY_RIGHT_SUPER:
            return ImGuiKey_RightSuper;
        case GLFW_KEY_MENU:
            return ImGuiKey_Menu;
        default:
            return ImGuiKey_None;
    }
}
#endif

static constexpr size_t CalculateRequiredInputMemory() {
    return sizeof(InputState) * (static_cast<size_t>(KeyCode::NUM) + static_cast<size_t>(MouseButtonCode::NUM));
}

#if WITH_EDITOR
static FORCE_INLINE bool ImGuiWantsKeyboardInput() {
    return ImGui::GetIO().WantCaptureKeyboard;
}

static FORCE_INLINE bool ImGuiWantsMouseInput() {
    return ImGui::GetIO().WantCaptureMouse;
}
#endif

static void GLFWKeyCallback(GLFWwindow* glfwWindow, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_UNKNOWN) {
        return; // Do nothing in such case!
    }

#if WITH_EDITOR
    if (!ImGuiWantsKeyboardInput() || !gDetachedFromGame) {
#endif
        GLFWState* glfwState = static_cast<GLFWState*>(glfwGetWindowUserPointer(glfwWindow));
        Input::State& inputState = *static_cast<Input::State*>(glfwState->inputState);

        const InputState newKeyState = static_cast<InputState>(action);
        InputState& cachedKeyState = inputState.keyStates[key];

        if (newKeyState == InputState::Repeat) {
            cachedKeyState = InputState::Hold;
        } else {
            cachedKeyState = newKeyState;
        }
#if WITH_EDITOR
    }
#endif

#if WITH_EDITOR
    if (gDetachedFromGame) {
        ImGuiIO& io = ImGui::GetIO();
        ImGuiKey remappedKey = RemapKey(key);
        io.AddKeyEvent(remappedKey, action == GLFW_PRESS);
    }
#endif
}

static void GLFWMouseButtonCallback(GLFWwindow* glfwWindow, int button, int action, int mods) {

#if WITH_EDITOR
    if (!ImGuiWantsMouseInput() || !gDetachedFromGame) {
#endif
        GLFWState* glfwState = static_cast<GLFWState*>(glfwGetWindowUserPointer(glfwWindow));
        Input::State& inputState = *static_cast<Input::State*>(glfwState->inputState);

        const InputState newButtonState = static_cast<InputState>(action);
        InputState& cachedButtonState = inputState.keyStates[button];

        if (newButtonState == InputState::Repeat) {
            cachedButtonState = InputState::Hold;
        } else {
            cachedButtonState = newButtonState;
        }
#if WITH_EDITOR
    }
#endif

#if WITH_EDITOR
    if (gDetachedFromGame) {
        ImGuiIO& io = ImGui::GetIO();
        io.AddMouseButtonEvent(button, action == GLFW_PRESS);
    }
#endif
}

static void GLFWCharCallback(GLFWwindow* glfwWindow, uint32_t codepoint) {
#if WITH_EDITOR
    if (gDetachedFromGame) {
        ImGuiIO& io = ImGui::GetIO();
        io.AddInputCharacter(codepoint);
    }
#endif
}

static void GLFWScrollCallback(GLFWwindow* glfwWindow, double xoffset, double yoffset) {

#if WITH_EDITOR
    if (!ImGuiWantsMouseInput() || !gDetachedFromGame) {
#endif
        GLFWState* glfwState = static_cast<GLFWState*>(glfwGetWindowUserPointer(glfwWindow));
        Input::State& inputState = *static_cast<Input::State*>(glfwState->inputState);
        inputState.newScrollEvent = true;
        inputState.scrollYOffset = static_cast<float>(yoffset);
#if WITH_EDITOR
    }
#endif

#if WITH_EDITOR
    if (gDetachedFromGame) {
        ImGuiIO& io = ImGui::GetIO();
        io.AddMouseWheelEvent(static_cast<float>(xoffset), static_cast<float>(yoffset));
    }
#endif
}

static void GLFWCursorPosCallback(GLFWwindow* glfwWindow, double xpos, double ypos) {

#if WITH_EDITOR
    if (!ImGuiWantsMouseInput() || !gDetachedFromGame) {
#endif
        GLFWState* glfwState = static_cast<GLFWState*>(glfwGetWindowUserPointer(glfwWindow));
        Input::State& inputState = *static_cast<Input::State*>(glfwState->inputState);
        inputState.cursorPosition.x = static_cast<float>(xpos);
        inputState.cursorPosition.y = static_cast<float>(ypos);
#if WITH_EDITOR
    }
#endif

#if WITH_EDITOR
    if (gDetachedFromGame) {
        ImGuiIO& io = ImGui::GetIO();
        io.AddMousePosEvent(static_cast<float>(xpos), static_cast<float>(ypos));
    }
#endif
}

Input* Input::instance = nullptr;

Input::Input(GLFWwindow* window, FreeListAllocator& parentAllocator) :
    glfwWindow(window),
    allocator(parentAllocator, CalculateRequiredInputMemory()),
    state({static_cast<size_t>(KeyCode::NUM), &allocator, InputState::None},
          {static_cast<size_t>(MouseButtonCode::NUM), &allocator, InputState::None}) {

    ASSERT(instance == nullptr, "There is more than one instance of Input");

    // This assumes the window has been created at this point
    GLFWState* glfwState = static_cast<GLFWState*>(glfwGetWindowUserPointer(glfwWindow));
    glfwState->inputState = &state;

    glfwSetKeyCallback(glfwWindow, GLFWKeyCallback);
    glfwSetMouseButtonCallback(glfwWindow, GLFWMouseButtonCallback);
    glfwSetScrollCallback(glfwWindow, GLFWScrollCallback);
    glfwSetCursorPosCallback(glfwWindow, GLFWCursorPosCallback);
    glfwSetCharCallback(glfwWindow, GLFWCharCallback);
    /* When sticky keys mode is enabled, the pollable state of a key will remain GLFW_PRESS until the state of that key
    is polled with glfwGetKey. Once it has been polled, if a key release event had been processed in the meantime, the
    state will reset to GLFW_RELEASE, otherwise it will remain GLFW_PRESS.*/
    // glfwSetInputMode(glfwWindow, GLFW_STICKY_KEYS, GLFW_TRUE);

    instance = this;
}

Input::~Input() {
    instance = nullptr;
}

void Input::PollInputEvents() {

    TIME_SCOPE("Poll Input");

    // Update states from last frame

    for (InputState& keyState : state.keyStates) {

        if (keyState == InputState::Press) {
            keyState = InputState::Hold;
        } else if (keyState == InputState::Release) {
            keyState = InputState::None;
        }
    }

    for (InputState& buttonState : state.mouseButtonStates) {

        if (buttonState == InputState::Press) {
            buttonState = InputState::Hold;
        } else if (buttonState == InputState::Release) {
            buttonState = InputState::None;
        }
    }

    glfwPollEvents();

    if (state.suppressNextDelta) {
        state.mouseDelta = Vec2(0.f, 0.f);
        state.suppressNextDelta = false;
    } else {
        state.mouseDelta = state.cursorPosition - state.previousCursorPosition;
    }
    state.previousCursorPosition = state.cursorPosition;

    if (!state.newScrollEvent) {
        state.scrollYOffset = 0.f;
    } else {
        state.newScrollEvent = false;
    }
}
#if WITH_EDITOR
    #define RETURN_X_IF_INPUT_DISABLED(X) \
        if (!instance->state.isEnabled) { \
            return X; \
        }
    #define RETURN_IF_INPUT_DISABLED() \
        if (!instance->state.isEnabled) { \
            return; \
        }
#else
    #define RETURN_X_IF_INPUT_DISABLED(X)
    #define RETURN_IF_INPUT_DISABLED()
#endif

bool Input::IsKeyPressed(const KeyCode keyCode) {
    ASSERT(instance != nullptr);
    RETURN_X_IF_INPUT_DISABLED(false)
    return instance->state.keyStates[static_cast<uint32>(keyCode)] == InputState::Press;
}

bool Input::IsKeyHeld(const KeyCode keyCode) {
    ASSERT(instance != nullptr);
    RETURN_X_IF_INPUT_DISABLED(false)
    return instance->state.keyStates[static_cast<uint32>(keyCode)] == InputState::Hold;
}

bool Input::IsKeyDown(KeyCode keyCode) {
    return IsKeyPressed(keyCode) || IsKeyHeld(keyCode);
}

bool Input::IsKeyReleased(const KeyCode keyCode) {
    ASSERT(instance != nullptr);
    RETURN_X_IF_INPUT_DISABLED(false)
    return instance->state.keyStates[static_cast<uint32>(keyCode)] == InputState::Release;
}

bool Input::IsMouseButtonPressed(const MouseButtonCode mouseButtonCode) {
    ASSERT(instance != nullptr);
    RETURN_X_IF_INPUT_DISABLED(false)
    return instance->state.keyStates[static_cast<uint32>(mouseButtonCode)] == InputState::Press;
}

bool Input::IsMouseButtonHeld(const MouseButtonCode mouseButtonCode) {
    ASSERT(instance != nullptr);
    RETURN_X_IF_INPUT_DISABLED(false)
    return instance->state.keyStates[static_cast<uint32>(mouseButtonCode)] == InputState::Hold;
}

bool Input::IsMouseButtonDown(MouseButtonCode mouseButtonCode) {
    return IsMouseButtonPressed(mouseButtonCode) || IsMouseButtonHeld(mouseButtonCode);
}

bool Input::IsMouseButtonReleased(const MouseButtonCode mouseButtonCode) {
    ASSERT(instance != nullptr);
    RETURN_X_IF_INPUT_DISABLED(false)
    return instance->state.keyStates[static_cast<uint32>(mouseButtonCode)] == InputState::Release;
}
bool Input::IsCursorVisible() {
    ASSERT(instance != nullptr);
    RETURN_X_IF_INPUT_DISABLED(false)
    return instance->state.cursorVisible;
}

void Input::SetCursorVisibility(bool visible) {
    ASSERT(instance != nullptr);
    RETURN_IF_INPUT_DISABLED()
    Input& input = *instance;
    if (input.state.cursorVisible != visible) {
        input.state.cursorVisible = visible;
        if (!visible) {
            glfwSetInputMode(input.glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwSetInputMode(input.glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

#if WITH_EDITOR
void Input::ToggleInputEnabled(bool enabled) {
    instance->state.isEnabled = enabled;
}
#endif

Vec2 Input::GetCursorPosition() {
    ASSERT(instance != nullptr);
    RETURN_X_IF_INPUT_DISABLED(Vec2(0, 0));
    return instance->state.cursorPosition;
}

void Input::SetCursorPosition(const Vec2& position) {
    ASSERT(instance != nullptr);
    RETURN_IF_INPUT_DISABLED()
    //	TODO Ensure cursor is within screen limits
    glfwSetCursorPos(instance->glfwWindow, position.x, position.y);
}

Vec2 Input::GetCursorDelta() {
    ASSERT(instance != nullptr);
    RETURN_X_IF_INPUT_DISABLED(Vec2(0, 0))
    return instance->state.mouseDelta;
}

void Input::SuppressNextCursorDelta() {
    ASSERT(instance != nullptr);
    instance->state.suppressNextDelta = true;
}

double Input::GetMouseScroll() {
    ASSERT(instance != nullptr);
    RETURN_X_IF_INPUT_DISABLED(0)
    return instance->state.scrollYOffset;
}
