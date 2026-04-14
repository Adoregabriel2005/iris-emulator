#include "SDLInput.h"
#include <QDebug>
#include <QtCore/Qt>

// Map Qt::Key to SDL_Scancode for keyboard input (SDL can't see keys without its own window)
SDL_Scancode SDLInput::qtKeyToSdlScancode(int qtKey)
{
    switch (qtKey) {
    // Letters
    case Qt::Key_A: return SDL_SCANCODE_A;
    case Qt::Key_B: return SDL_SCANCODE_B;
    case Qt::Key_C: return SDL_SCANCODE_C;
    case Qt::Key_D: return SDL_SCANCODE_D;
    case Qt::Key_E: return SDL_SCANCODE_E;
    case Qt::Key_F: return SDL_SCANCODE_F;
    case Qt::Key_G: return SDL_SCANCODE_G;
    case Qt::Key_H: return SDL_SCANCODE_H;
    case Qt::Key_I: return SDL_SCANCODE_I;
    case Qt::Key_J: return SDL_SCANCODE_J;
    case Qt::Key_K: return SDL_SCANCODE_K;
    case Qt::Key_L: return SDL_SCANCODE_L;
    case Qt::Key_M: return SDL_SCANCODE_M;
    case Qt::Key_N: return SDL_SCANCODE_N;
    case Qt::Key_O: return SDL_SCANCODE_O;
    case Qt::Key_P: return SDL_SCANCODE_P;
    case Qt::Key_Q: return SDL_SCANCODE_Q;
    case Qt::Key_R: return SDL_SCANCODE_R;
    case Qt::Key_S: return SDL_SCANCODE_S;
    case Qt::Key_T: return SDL_SCANCODE_T;
    case Qt::Key_U: return SDL_SCANCODE_U;
    case Qt::Key_V: return SDL_SCANCODE_V;
    case Qt::Key_W: return SDL_SCANCODE_W;
    case Qt::Key_X: return SDL_SCANCODE_X;
    case Qt::Key_Y: return SDL_SCANCODE_Y;
    case Qt::Key_Z: return SDL_SCANCODE_Z;
    // Digits
    case Qt::Key_0: return SDL_SCANCODE_0;
    case Qt::Key_1: return SDL_SCANCODE_1;
    case Qt::Key_2: return SDL_SCANCODE_2;
    case Qt::Key_3: return SDL_SCANCODE_3;
    case Qt::Key_4: return SDL_SCANCODE_4;
    case Qt::Key_5: return SDL_SCANCODE_5;
    case Qt::Key_6: return SDL_SCANCODE_6;
    case Qt::Key_7: return SDL_SCANCODE_7;
    case Qt::Key_8: return SDL_SCANCODE_8;
    case Qt::Key_9: return SDL_SCANCODE_9;
    // Function keys
    case Qt::Key_F1:  return SDL_SCANCODE_F1;
    case Qt::Key_F2:  return SDL_SCANCODE_F2;
    case Qt::Key_F3:  return SDL_SCANCODE_F3;
    case Qt::Key_F4:  return SDL_SCANCODE_F4;
    case Qt::Key_F5:  return SDL_SCANCODE_F5;
    case Qt::Key_F6:  return SDL_SCANCODE_F6;
    case Qt::Key_F7:  return SDL_SCANCODE_F7;
    case Qt::Key_F8:  return SDL_SCANCODE_F8;
    case Qt::Key_F9:  return SDL_SCANCODE_F9;
    case Qt::Key_F10: return SDL_SCANCODE_F10;
    case Qt::Key_F11: return SDL_SCANCODE_F11;
    case Qt::Key_F12: return SDL_SCANCODE_F12;
    // Arrow keys
    case Qt::Key_Up:    return SDL_SCANCODE_UP;
    case Qt::Key_Down:  return SDL_SCANCODE_DOWN;
    case Qt::Key_Left:  return SDL_SCANCODE_LEFT;
    case Qt::Key_Right: return SDL_SCANCODE_RIGHT;
    // Common keys
    case Qt::Key_Space:     return SDL_SCANCODE_SPACE;
    case Qt::Key_Return:    return SDL_SCANCODE_RETURN;
    case Qt::Key_Enter:     return SDL_SCANCODE_KP_ENTER;
    case Qt::Key_Escape:    return SDL_SCANCODE_ESCAPE;
    case Qt::Key_Tab:       return SDL_SCANCODE_TAB;
    case Qt::Key_Backspace: return SDL_SCANCODE_BACKSPACE;
    case Qt::Key_Delete:    return SDL_SCANCODE_DELETE;
    case Qt::Key_Insert:    return SDL_SCANCODE_INSERT;
    case Qt::Key_Home:      return SDL_SCANCODE_HOME;
    case Qt::Key_End:       return SDL_SCANCODE_END;
    case Qt::Key_PageUp:    return SDL_SCANCODE_PAGEUP;
    case Qt::Key_PageDown:  return SDL_SCANCODE_PAGEDOWN;
    // Modifiers
    case Qt::Key_Shift:   return SDL_SCANCODE_LSHIFT;
    case Qt::Key_Control: return SDL_SCANCODE_LCTRL;
    case Qt::Key_Alt:     return SDL_SCANCODE_LALT;
    default: return SDL_SCANCODE_UNKNOWN;
    }
}

void SDLInput::setQtKeyState(int qtKey, bool pressed)
{
    SDL_Scancode sc = qtKeyToSdlScancode(qtKey);
    if (sc != SDL_SCANCODE_UNKNOWN)
        m_keystate[sc] = pressed ? 1 : 0;
}

// --- InputBinding ---

QString InputBinding::toDisplayString() const
{
    switch (type) {
    case Type::Keyboard:
        return QString::fromUtf8(SDL_GetScancodeName(static_cast<SDL_Scancode>(code)));
    case Type::GamepadButton: {
        const char* name = SDL_GameControllerGetStringForButton(
            static_cast<SDL_GameControllerButton>(code));
        if (name && name[0])
            return QString("Pad %1").arg(name);
        return QString("Button %1").arg(code);
    }
    case Type::GamepadAxis: {
        const char* name = SDL_GameControllerGetStringForAxis(
            static_cast<SDL_GameControllerAxis>(code));
        if (name && name[0])
            return QString("Pad %1%2").arg(name).arg(direction > 0 ? "+" : "-");
        return QString("Axis %1%2").arg(code).arg(direction > 0 ? "+" : "-");
    }
    default:
        return QStringLiteral("None");
    }
}

QString InputBinding::toSettingsString() const
{
    switch (type) {
    case Type::Keyboard:     return QString("KB:%1").arg(code);
    case Type::GamepadButton: return QString("GB:%1").arg(code);
    case Type::GamepadAxis:  return QString("GA:%1:%2").arg(code).arg(direction);
    default:                  return QStringLiteral("None");
    }
}

InputBinding InputBinding::fromSettingsString(const QString& s)
{
    InputBinding b;
    if (s.startsWith("KB:")) {
        b.type = Type::Keyboard;
        b.code = s.mid(3).toInt();
    } else if (s.startsWith("GB:")) {
        b.type = Type::GamepadButton;
        b.code = s.mid(3).toInt();
    } else if (s.startsWith("GA:")) {
        auto parts = s.mid(3).split(':');
        if (parts.size() >= 2) {
            b.type = Type::GamepadAxis;
            b.code = parts[0].toInt();
            b.direction = parts[1].toInt();
        }
    }
    return b;
}

// --- SDLInput ---

SDLInput::SDLInput()
{
    if (SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO | SDL_INIT_EVENTS) == 0) {
        m_initialized = true;
        openController();
    } else {
        qWarning() << "SDL_Init failed:" << SDL_GetError();
    }
    loadBindings();
}

SDLInput::~SDLInput()
{
    closeController();
    if (m_initialized) {
        SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO | SDL_INIT_EVENTS);
        SDL_Quit();
    }
}

void SDLInput::openController()
{
    if (m_controller || m_joystick) return;

    int numJoysticks = SDL_NumJoysticks();
    qDebug() << "SDL detected" << numJoysticks << "joystick(s)";

    for (int i = 0; i < numJoysticks; i++) {
        if (SDL_IsGameController(i)) {
            m_controller = SDL_GameControllerOpen(i);
            if (m_controller) {
                m_controller_index = i;
                qDebug() << "GameController connected:" << SDL_GameControllerName(m_controller);
                return;
            }
        }
    }

    // Fallback: open first joystick as raw joystick (generic USB controllers)
    if (numJoysticks > 0 && !m_controller) {
        m_joystick = SDL_JoystickOpen(0);
        if (m_joystick) {
            m_controller_index = 0;
            m_joystick_num_buttons = SDL_JoystickNumButtons(m_joystick);
            m_joystick_num_axes = SDL_JoystickNumAxes(m_joystick);
            qDebug() << "Generic joystick connected:" << SDL_JoystickName(m_joystick)
                     << "buttons:" << m_joystick_num_buttons << "axes:" << m_joystick_num_axes;
        }
    }
}

void SDLInput::closeController()
{
    if (m_controller) {
        SDL_GameControllerClose(m_controller);
        m_controller = nullptr;
    }
    if (m_joystick) {
        SDL_JoystickClose(m_joystick);
        m_joystick = nullptr;
    }
    m_controller_index = -1;
    m_joystick_num_buttons = 0;
    m_joystick_num_axes = 0;
}

const char* SDLInput::actionName(InputAction action)
{
    switch (action) {
    case InputAction::Up:           return "Up";
    case InputAction::Down:         return "Down";
    case InputAction::Left:         return "Left";
    case InputAction::Right:        return "Right";
    case InputAction::Fire:         return "Fire";
    case InputAction::P1Up:         return "P2 Up";
    case InputAction::P1Down:       return "P2 Down";
    case InputAction::P1Left:       return "P2 Left";
    case InputAction::P1Right:      return "P2 Right";
    case InputAction::P1Fire:       return "P2 Fire";
    case InputAction::Select:       return "Select";
    case InputAction::Reset:        return "Reset";
    case InputAction::P0DiffToggle: return "P0 Difficulty";
    case InputAction::ColorBWToggle:return "Color/B&W";
    case InputAction::LynxUp:       return "Lynx Up";
    case InputAction::LynxDown:     return "Lynx Down";
    case InputAction::LynxLeft:     return "Lynx Left";
    case InputAction::LynxRight:    return "Lynx Right";
    case InputAction::LynxA:        return "Lynx A";
    case InputAction::LynxB:        return "Lynx B";
    case InputAction::LynxLeftShoulder:  return "Lynx L Shoulder";
    case InputAction::LynxRightShoulder: return "Lynx R Shoulder";
    case InputAction::LynxStart:         return "Lynx Start";
    default: return "Unknown";
    }
}

InputBinding SDLInput::defaultBinding(InputAction action)
{
    InputBinding b;
    b.type = InputBinding::Type::Keyboard;
    switch (action) {
    // Atari 2600 defaults
    case InputAction::Up:    b.code = SDL_SCANCODE_UP;     break;
    case InputAction::Down:  b.code = SDL_SCANCODE_DOWN;   break;
    case InputAction::Left:  b.code = SDL_SCANCODE_LEFT;   break;
    case InputAction::Right: b.code = SDL_SCANCODE_RIGHT;  break;
    case InputAction::Fire:  b.code = SDL_SCANCODE_SPACE;  break;
    case InputAction::P1Up:    b.code = SDL_SCANCODE_KP_8;  break;
    case InputAction::P1Down:  b.code = SDL_SCANCODE_KP_2;  break;
    case InputAction::P1Left:  b.code = SDL_SCANCODE_KP_4;  break;
    case InputAction::P1Right: b.code = SDL_SCANCODE_KP_6;  break;
    case InputAction::P1Fire:  b.code = SDL_SCANCODE_KP_0;  break;
    case InputAction::Select: b.code = SDL_SCANCODE_F1;    break;
    case InputAction::Reset:  b.code = SDL_SCANCODE_F2;    break;
    case InputAction::P0DiffToggle: b.code = SDL_SCANCODE_F3; break;
    case InputAction::ColorBWToggle: b.code = SDL_SCANCODE_F4; break;
    // Atari Lynx defaults
    case InputAction::LynxUp:      b.code = SDL_SCANCODE_UP;     break;
    case InputAction::LynxDown:    b.code = SDL_SCANCODE_DOWN;   break;
    case InputAction::LynxLeft:    b.code = SDL_SCANCODE_LEFT;   break;
    case InputAction::LynxRight:   b.code = SDL_SCANCODE_RIGHT;  break;
    case InputAction::LynxA:              b.code = SDL_SCANCODE_Z;      break;
    case InputAction::LynxB:              b.code = SDL_SCANCODE_X;      break;
    case InputAction::LynxLeftShoulder:   b.code = SDL_SCANCODE_A;      break;
    case InputAction::LynxRightShoulder:  b.code = SDL_SCANCODE_S;      break;
    case InputAction::LynxStart:          b.code = SDL_SCANCODE_RETURN; break;
    default: b.type = InputBinding::Type::None; break;
    }
    return b;
}

void SDLInput::resetToDefaults()
{
    for (int i = 0; i < static_cast<int>(InputAction::Count); i++)
        m_bindings[i] = defaultBinding(static_cast<InputAction>(i));
}

void SDLInput::loadBindings()
{
    resetToDefaults();
    QSettings settings;
    settings.beginGroup("Input");
    for (int i = 0; i < static_cast<int>(InputAction::Count); i++) {
        auto action = static_cast<InputAction>(i);
        QString key = QString("Binding_%1").arg(actionName(action));
        if (settings.contains(key))
            m_bindings[i] = InputBinding::fromSettingsString(settings.value(key).toString());
    }
    settings.endGroup();
}

void SDLInput::saveBindings()
{
    QSettings settings;
    settings.beginGroup("Input");
    for (int i = 0; i < static_cast<int>(InputAction::Count); i++) {
        auto action = static_cast<InputAction>(i);
        QString key = QString("Binding_%1").arg(actionName(action));
        settings.setValue(key, m_bindings[i].toSettingsString());
    }
    settings.endGroup();
}

InputBinding SDLInput::getBinding(InputAction action) const
{
    int idx = static_cast<int>(action);
    if (idx >= 0 && idx < static_cast<int>(InputAction::Count))
        return m_bindings[idx];
    return {};
}

void SDLInput::setBinding(InputAction action, const InputBinding& binding)
{
    int idx = static_cast<int>(action);
    if (idx >= 0 && idx < static_cast<int>(InputAction::Count))
        m_bindings[idx] = binding;
}

QString SDLInput::gamepadName() const
{
    if (m_controller)
        return QString::fromUtf8(SDL_GameControllerName(m_controller));
    if (m_joystick)
        return QString::fromUtf8(SDL_JoystickName(m_joystick));
    return {};
}

bool SDLInput::isActionActive(const InputBinding& binding, const Uint8* keys) const
{
    switch (binding.type) {
    case InputBinding::Type::Keyboard:
        return keys[binding.code];
    case InputBinding::Type::GamepadButton:
        if (m_controller)
            return SDL_GameControllerGetButton(m_controller,
                static_cast<SDL_GameControllerButton>(binding.code));
        // Fallback: raw joystick button
        if (m_joystick && binding.code < m_joystick_num_buttons)
            return SDL_JoystickGetButton(m_joystick, binding.code);
        return false;
    case InputBinding::Type::GamepadAxis: {
        Sint16 val = 0;
        if (m_controller)
            val = SDL_GameControllerGetAxis(m_controller,
                static_cast<SDL_GameControllerAxis>(binding.code));
        else if (m_joystick && binding.code < m_joystick_num_axes)
            val = SDL_JoystickGetAxis(m_joystick, binding.code);
        return (binding.direction < 0) ? (val < -AXIS_DEADZONE)
                                       : (val > AXIS_DEADZONE);
    }
    default:
        return false;
    }
}

JoystickState SDLInput::poll()
{
    JoystickState s;
    if (!m_initialized) return s;

    SDL_PumpEvents();

    // Handle controller hotplug
    SDL_Event ev;
    while (SDL_PeepEvents(&ev, 1, SDL_GETEVENT, SDL_CONTROLLERDEVICEADDED, SDL_JOYDEVICEREMOVED) > 0) {
        if ((ev.type == SDL_CONTROLLERDEVICEADDED || ev.type == SDL_JOYDEVICEADDED)
            && !m_controller && !m_joystick)
            openController();
        else if ((ev.type == SDL_CONTROLLERDEVICEREMOVED || ev.type == SDL_JOYDEVICEREMOVED)
                 && (m_controller || m_joystick))
            closeController();
    }

    const Uint8* keys = m_keystate;

    // Also support WASD as secondary keyboard bindings (always active)
    bool wasd_up    = keys[SDL_SCANCODE_W];
    bool wasd_down  = keys[SDL_SCANCODE_S];
    bool wasd_left  = keys[SDL_SCANCODE_A];
    bool wasd_right = keys[SDL_SCANCODE_D];
    bool wasd_fire  = keys[SDL_SCANCODE_RETURN];

    s.up    = isActionActive(m_bindings[static_cast<int>(InputAction::Up)], keys) || wasd_up;
    s.down  = isActionActive(m_bindings[static_cast<int>(InputAction::Down)], keys) || wasd_down;
    s.left  = isActionActive(m_bindings[static_cast<int>(InputAction::Left)], keys) || wasd_left;
    s.right = isActionActive(m_bindings[static_cast<int>(InputAction::Right)], keys) || wasd_right;
    s.fire  = isActionActive(m_bindings[static_cast<int>(InputAction::Fire)], keys) || wasd_fire;

    s.p1up    = isActionActive(m_bindings[static_cast<int>(InputAction::P1Up)], keys);
    s.p1down  = isActionActive(m_bindings[static_cast<int>(InputAction::P1Down)], keys);
    s.p1left  = isActionActive(m_bindings[static_cast<int>(InputAction::P1Left)], keys);
    s.p1right = isActionActive(m_bindings[static_cast<int>(InputAction::P1Right)], keys);
    s.p1fire  = isActionActive(m_bindings[static_cast<int>(InputAction::P1Fire)], keys);

    s.select       = isActionActive(m_bindings[static_cast<int>(InputAction::Select)], keys);
    s.reset        = isActionActive(m_bindings[static_cast<int>(InputAction::Reset)], keys);
    s.p0DiffToggle = isActionActive(m_bindings[static_cast<int>(InputAction::P0DiffToggle)], keys);
    s.colorBWToggle= isActionActive(m_bindings[static_cast<int>(InputAction::ColorBWToggle)], keys);

    // D-pad on recognized GameController as secondary direction input
    if (m_controller) {
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_DPAD_UP))    s.up = true;
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN))  s.down = true;
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT))  s.left = true;
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) s.right = true;
        // Left stick
        Sint16 lx = SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_LEFTX);
        Sint16 ly = SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_LEFTY);
        if (lx < -AXIS_DEADZONE) s.left = true;
        if (lx > AXIS_DEADZONE)  s.right = true;
        if (ly < -AXIS_DEADZONE) s.up = true;
        if (ly > AXIS_DEADZONE)  s.down = true;
        // A button as fire
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_A)) s.fire = true;
        // Back/Start as Select/Reset
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_BACK))  s.select = true;
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_START)) s.reset = true;
    }
    // Generic joystick fallback: axes 0/1 = stick, buttons 0=fire, 6=select, 7=start
    else if (m_joystick) {
        SDL_JoystickUpdate();
        if (m_joystick_num_axes >= 2) {
            Sint16 ax0 = SDL_JoystickGetAxis(m_joystick, 0);
            Sint16 ax1 = SDL_JoystickGetAxis(m_joystick, 1);
            if (ax0 < -AXIS_DEADZONE) s.left = true;
            if (ax0 > AXIS_DEADZONE)  s.right = true;
            if (ax1 < -AXIS_DEADZONE) s.up = true;
            if (ax1 > AXIS_DEADZONE)  s.down = true;
        }
        // D-pad via hat (POV)
        int numHats = SDL_JoystickNumHats(m_joystick);
        if (numHats > 0) {
            Uint8 hat = SDL_JoystickGetHat(m_joystick, 0);
            if (hat & SDL_HAT_UP)    s.up = true;
            if (hat & SDL_HAT_DOWN)  s.down = true;
            if (hat & SDL_HAT_LEFT)  s.left = true;
            if (hat & SDL_HAT_RIGHT) s.right = true;
        }
        // Common generic button mappings
        if (m_joystick_num_buttons > 0 && SDL_JoystickGetButton(m_joystick, 0)) s.fire = true;
        if (m_joystick_num_buttons > 1 && SDL_JoystickGetButton(m_joystick, 1)) s.fire = true;
        if (m_joystick_num_buttons > 6 && SDL_JoystickGetButton(m_joystick, 6)) s.select = true;
        if (m_joystick_num_buttons > 7 && SDL_JoystickGetButton(m_joystick, 7)) s.reset = true;
    }

    return s;
}

InputBinding SDLInput::pollForBinding(bool allowKeyboard, bool allowGamepad)
{
    if (!m_initialized) return {};

    SDL_PumpEvents();

    if (allowKeyboard) {
        // Use internal key state (fed from Qt key events)
        for (int i = 0; i < SDL_NUM_SCANCODES; i++) {
            if (m_keystate[i] && i != SDL_SCANCODE_ESCAPE) {
                InputBinding b;
                b.type = InputBinding::Type::Keyboard;
                b.code = i;
                return b;
            }
        }
    }

    if (allowGamepad && m_controller) {
        // Check buttons
        for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++) {
            if (SDL_GameControllerGetButton(m_controller, static_cast<SDL_GameControllerButton>(i))) {
                InputBinding b;
                b.type = InputBinding::Type::GamepadButton;
                b.code = i;
                return b;
            }
        }
        // Check axes
        for (int i = 0; i < SDL_CONTROLLER_AXIS_MAX; i++) {
            Sint16 val = SDL_GameControllerGetAxis(m_controller, static_cast<SDL_GameControllerAxis>(i));
            if (val > AXIS_DEADZONE) {
                InputBinding b;
                b.type = InputBinding::Type::GamepadAxis;
                b.code = i;
                b.direction = 1;
                return b;
            }
            if (val < -AXIS_DEADZONE) {
                InputBinding b;
                b.type = InputBinding::Type::GamepadAxis;
                b.code = i;
                b.direction = -1;
                return b;
            }
        }
    }

    // Fallback: raw joystick buttons and axes
    if (allowGamepad && m_joystick) {
        SDL_JoystickUpdate();
        for (int i = 0; i < m_joystick_num_buttons; i++) {
            if (SDL_JoystickGetButton(m_joystick, i)) {
                InputBinding b;
                b.type = InputBinding::Type::GamepadButton;
                b.code = i;
                return b;
            }
        }
        for (int i = 0; i < m_joystick_num_axes; i++) {
            Sint16 val = SDL_JoystickGetAxis(m_joystick, i);
            if (val > AXIS_DEADZONE) {
                InputBinding b;
                b.type = InputBinding::Type::GamepadAxis;
                b.code = i;
                b.direction = 1;
                return b;
            }
            if (val < -AXIS_DEADZONE) {
                InputBinding b;
                b.type = InputBinding::Type::GamepadAxis;
                b.code = i;
                b.direction = -1;
                return b;
            }
        }
    }

    return {};
}

LynxInputState SDLInput::pollLynx()
{
    LynxInputState s;
    if (!m_initialized) return s;

    SDL_PumpEvents();

    const Uint8* keys = m_keystate;

    s.up      = isActionActive(m_bindings[static_cast<int>(InputAction::LynxUp)], keys);
    s.down    = isActionActive(m_bindings[static_cast<int>(InputAction::LynxDown)], keys);
    s.left    = isActionActive(m_bindings[static_cast<int>(InputAction::LynxLeft)], keys);
    s.right   = isActionActive(m_bindings[static_cast<int>(InputAction::LynxRight)], keys);
    s.a       = isActionActive(m_bindings[static_cast<int>(InputAction::LynxA)], keys);
    s.b       = isActionActive(m_bindings[static_cast<int>(InputAction::LynxB)], keys);
    s.leftShoulder  = isActionActive(m_bindings[static_cast<int>(InputAction::LynxLeftShoulder)], keys);
    s.rightShoulder = isActionActive(m_bindings[static_cast<int>(InputAction::LynxRightShoulder)], keys);
    s.start         = isActionActive(m_bindings[static_cast<int>(InputAction::LynxStart)], keys);

    // Gamepad secondary input for Lynx
    if (m_controller) {
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_DPAD_UP))    s.up = true;
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN))  s.down = true;
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT))  s.left = true;
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) s.right = true;
        Sint16 lx = SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_LEFTX);
        Sint16 ly = SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_LEFTY);
        if (lx < -AXIS_DEADZONE) s.left = true;
        if (lx > AXIS_DEADZONE)  s.right = true;
        if (ly < -AXIS_DEADZONE) s.up = true;
        if (ly > AXIS_DEADZONE)  s.down = true;
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_A)) s.a = true;
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_B)) s.b = true;
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER))  s.leftShoulder = true;
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) s.rightShoulder = true;
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_START)) s.start = true;
    }

    return s;
}
