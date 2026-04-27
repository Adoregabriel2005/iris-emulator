#include "SDLInput.h"

JaguarInputState SDLInput::pollJaguar()
{
    JaguarInputState s;
    if (!m_initialized) return s;

    SDL_PumpEvents();
    const Uint8* keys = m_keystate;

    s.up     = isActionActive(m_bindings[static_cast<int>(InputAction::JagUp)],     keys);
    s.down   = isActionActive(m_bindings[static_cast<int>(InputAction::JagDown)],   keys);
    s.left   = isActionActive(m_bindings[static_cast<int>(InputAction::JagLeft)],   keys);
    s.right  = isActionActive(m_bindings[static_cast<int>(InputAction::JagRight)],  keys);
    s.a      = isActionActive(m_bindings[static_cast<int>(InputAction::JagA)],      keys);
    s.b      = isActionActive(m_bindings[static_cast<int>(InputAction::JagB)],      keys);
    s.c      = isActionActive(m_bindings[static_cast<int>(InputAction::JagC)],      keys);
    s.option = isActionActive(m_bindings[static_cast<int>(InputAction::JagOption)], keys);
    s.pause  = isActionActive(m_bindings[static_cast<int>(InputAction::JagPause)],  keys);
    s.n1     = isActionActive(m_bindings[static_cast<int>(InputAction::Jag1)],      keys);
    s.n2     = isActionActive(m_bindings[static_cast<int>(InputAction::Jag2)],      keys);
    s.n3     = isActionActive(m_bindings[static_cast<int>(InputAction::Jag3)],      keys);
    s.n4     = isActionActive(m_bindings[static_cast<int>(InputAction::Jag4)],      keys);
    s.n5     = isActionActive(m_bindings[static_cast<int>(InputAction::Jag5)],      keys);
    s.n6     = isActionActive(m_bindings[static_cast<int>(InputAction::Jag6)],      keys);
    s.n7     = isActionActive(m_bindings[static_cast<int>(InputAction::Jag7)],      keys);
    s.n8     = isActionActive(m_bindings[static_cast<int>(InputAction::Jag8)],      keys);
    s.n9     = isActionActive(m_bindings[static_cast<int>(InputAction::Jag9)],      keys);
    s.star   = isActionActive(m_bindings[static_cast<int>(InputAction::JagStar)],   keys);
    s.n0     = isActionActive(m_bindings[static_cast<int>(InputAction::Jag0)],      keys);
    s.hash   = isActionActive(m_bindings[static_cast<int>(InputAction::JagHash)],   keys);

    if (m_controller) {
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_DPAD_UP))    s.up = true;
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN))  s.down = true;
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT))  s.left = true;
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) s.right = true;
        Sint16 lx = SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_LEFTX);
        Sint16 ly = SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_LEFTY);
        if (lx < -AXIS_DEADZONE) s.left = true;
        if (lx >  AXIS_DEADZONE) s.right = true;
        if (ly < -AXIS_DEADZONE) s.up = true;
        if (ly >  AXIS_DEADZONE) s.down = true;
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_A)) s.a = true;
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_B)) s.b = true;
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_X)) s.c = true;
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER))  s.option = true;
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_START)) s.pause = true;
    }

    return s;
}
