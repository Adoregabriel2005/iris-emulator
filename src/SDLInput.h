#pragma once

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <QtCore/QSettings>
#include <QtCore/QString>
#include <cstdint>
#include <cstring>

// Actions that can be bound to keys/buttons
enum class InputAction : int {
    // Atari 2600 actions
    Up = 0,
    Down,
    Left,
    Right,
    Fire,
    // Player 2 actions
    P1Up,
    P1Down,
    P1Left,
    P1Right,
    P1Fire,
    Select,
    Reset,
    P0DiffToggle,
    ColorBWToggle,
    // Atari Lynx actions
    LynxUp,
    LynxDown,
    LynxLeft,
    LynxRight,
    LynxA,
    LynxB,
    LynxLeftShoulder,
    LynxRightShoulder,
    LynxStart,
    Count
};

// Helpers to identify action groups
inline bool isLynxAction(InputAction a) {
    return static_cast<int>(a) >= static_cast<int>(InputAction::LynxUp);
}
inline bool is2600Action(InputAction a) {
    return static_cast<int>(a) < static_cast<int>(InputAction::LynxUp);
}

struct JoystickState {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool fire = false;
    // Player 2
    bool p1up = false;
    bool p1down = false;
    bool p1left = false;
    bool p1right = false;
    bool p1fire = false;
    // Console switches
    bool select = false;
    bool reset = false;
    bool p0DiffToggle = false;
    bool colorBWToggle = false;
};

struct LynxInputState {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool a = false;
    bool b = false;
    bool leftShoulder = false;
    bool rightShoulder = false;
    bool start = false;
};

// A binding is either a keyboard scancode or a gamepad button/axis
struct InputBinding {
    enum class Type { None, Keyboard, GamepadButton, GamepadAxis };
    Type type = Type::None;
    int code = 0;      // SDL_Scancode for keyboard, button index for gamepad
    int direction = 0;  // for axis: -1 or +1

    bool isValid() const { return type != Type::None; }
    QString toDisplayString() const;
    QString toSettingsString() const;
    static InputBinding fromSettingsString(const QString& s);
};

class SDLInput {
public:
    SDLInput();
    ~SDLInput();

    JoystickState poll();
    LynxInputState pollLynx();

    // Qt keyboard integration (SDL_GetKeyboardState doesn't work without an SDL window)
    void setQtKeyState(int qtKey, bool pressed);
    void clearAllKeyStates() { memset(m_keystate, 0, sizeof(m_keystate)); }

    // Binding management
    InputBinding getBinding(InputAction action) const;
    void setBinding(InputAction action, const InputBinding& binding);
    void loadBindings();
    void saveBindings();
    void resetToDefaults();

    // Gamepad info
    bool hasGamepad() const { return m_controller != nullptr || m_joystick != nullptr; }
    QString gamepadName() const;

    // For input capture in settings dialog
    InputBinding pollForBinding(bool allowKeyboard, bool allowGamepad);

    static const char* actionName(InputAction action);
    static InputBinding defaultBinding(InputAction action);

private:
    void openController();
    void closeController();
    bool isActionActive(const InputBinding& binding, const Uint8* keys) const;
    static SDL_Scancode qtKeyToSdlScancode(int qtKey);

    bool m_initialized = false;
    Uint8 m_keystate[SDL_NUM_SCANCODES]{};
    SDL_GameController* m_controller = nullptr;
    SDL_Joystick* m_joystick = nullptr;       // fallback for generic/unrecognized controllers
    int m_controller_index = -1;
    int m_joystick_num_buttons = 0;
    int m_joystick_num_axes = 0;

    InputBinding m_bindings[static_cast<int>(InputAction::Count)];

    // Deadzone for gamepad axes
    static constexpr int AXIS_DEADZONE = 8000;
};
