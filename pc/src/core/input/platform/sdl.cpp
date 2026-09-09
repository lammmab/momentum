#include <cstdio>

#include <SDL3/SDL.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_hints.h>

#include "rainfall/input/backend/input_backend.h"

#include "momentum/core/input/platform.hpp"

#define BIND(action, slot, src, code) bindings[action][slot] = { src, code }

namespace momentum::input::platform {

void resetDefaults(RF_InputBinding bindings[GC_ACTION_COUNT][2]) {
    for (int i = 0; i < GC_ACTION_COUNT; i++) {
        bindings[i][0] = bindings[i][1] = RF_BindingNone();
    }

    BIND(GC_A,           0, RF_SRC_KEYBOARD,         SDL_SCANCODE_SPACE);
    BIND(GC_A,           1, RF_SRC_GAMEPAD_BUTTON,   SDL_GAMEPAD_BUTTON_SOUTH);
    BIND(GC_B,           0, RF_SRC_KEYBOARD,         SDL_SCANCODE_X);
    BIND(GC_B,           1, RF_SRC_GAMEPAD_BUTTON,   SDL_GAMEPAD_BUTTON_EAST);
    BIND(GC_X,           0, RF_SRC_KEYBOARD,         SDL_SCANCODE_E);
    BIND(GC_X,           1, RF_SRC_GAMEPAD_BUTTON,   SDL_GAMEPAD_BUTTON_WEST);
    BIND(GC_Y,           0, RF_SRC_KEYBOARD,         SDL_SCANCODE_R);
    BIND(GC_Y,           1, RF_SRC_GAMEPAD_BUTTON,   SDL_GAMEPAD_BUTTON_NORTH);
    BIND(GC_L,           0, RF_SRC_KEYBOARD,         SDL_SCANCODE_LSHIFT);
    BIND(GC_L,           1, RF_SRC_GAMEPAD_AXIS_POS, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
    BIND(GC_R,           0, RF_SRC_KEYBOARD,         SDL_SCANCODE_C);
    BIND(GC_R,           1, RF_SRC_GAMEPAD_AXIS_POS, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
    BIND(GC_Z,           0, RF_SRC_KEYBOARD,         SDL_SCANCODE_TAB);
    BIND(GC_Z,           1, RF_SRC_GAMEPAD_BUTTON,   SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
    BIND(GC_START,       0, RF_SRC_KEYBOARD,         SDL_SCANCODE_RETURN);
    BIND(GC_START,       1, RF_SRC_GAMEPAD_BUTTON,   SDL_GAMEPAD_BUTTON_START);
    BIND(GC_DPAD_UP,     0, RF_SRC_KEYBOARD,         SDL_SCANCODE_UP);
    BIND(GC_DPAD_UP,     1, RF_SRC_GAMEPAD_BUTTON,   SDL_GAMEPAD_BUTTON_DPAD_UP);
    BIND(GC_DPAD_DOWN,   0, RF_SRC_KEYBOARD,         SDL_SCANCODE_DOWN);
    BIND(GC_DPAD_DOWN,   1, RF_SRC_GAMEPAD_BUTTON,   SDL_GAMEPAD_BUTTON_DPAD_DOWN);
    BIND(GC_DPAD_LEFT,   0, RF_SRC_KEYBOARD,         SDL_SCANCODE_LEFT);
    BIND(GC_DPAD_LEFT,   1, RF_SRC_GAMEPAD_BUTTON,   SDL_GAMEPAD_BUTTON_DPAD_LEFT);
    BIND(GC_DPAD_RIGHT,  0, RF_SRC_KEYBOARD,         SDL_SCANCODE_RIGHT);
    BIND(GC_DPAD_RIGHT,  1, RF_SRC_GAMEPAD_BUTTON,   SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
    BIND(GC_STICK_UP,    0, RF_SRC_KEYBOARD,         SDL_SCANCODE_W);
    BIND(GC_STICK_UP,    1, RF_SRC_GAMEPAD_AXIS_NEG, SDL_GAMEPAD_AXIS_LEFTY);
    BIND(GC_STICK_DOWN,  0, RF_SRC_KEYBOARD,         SDL_SCANCODE_S);
    BIND(GC_STICK_DOWN,  1, RF_SRC_GAMEPAD_AXIS_POS, SDL_GAMEPAD_AXIS_LEFTY);
    BIND(GC_STICK_LEFT,  0, RF_SRC_KEYBOARD,         SDL_SCANCODE_A);
    BIND(GC_STICK_LEFT,  1, RF_SRC_GAMEPAD_AXIS_NEG, SDL_GAMEPAD_AXIS_LEFTX);
    BIND(GC_STICK_RIGHT, 0, RF_SRC_KEYBOARD,         SDL_SCANCODE_D);
    BIND(GC_STICK_RIGHT, 1, RF_SRC_GAMEPAD_AXIS_POS, SDL_GAMEPAD_AXIS_LEFTX);
    BIND(GC_CSTICK_UP,   0, RF_SRC_KEYBOARD,         SDL_SCANCODE_I);
    BIND(GC_CSTICK_UP,   1, RF_SRC_GAMEPAD_AXIS_NEG, SDL_GAMEPAD_AXIS_RIGHTY);
    BIND(GC_CSTICK_DOWN, 0, RF_SRC_KEYBOARD,         SDL_SCANCODE_K);
    BIND(GC_CSTICK_DOWN, 1, RF_SRC_GAMEPAD_AXIS_POS, SDL_GAMEPAD_AXIS_RIGHTY);
    BIND(GC_CSTICK_LEFT, 0, RF_SRC_KEYBOARD,         SDL_SCANCODE_J);
    BIND(GC_CSTICK_LEFT, 1, RF_SRC_GAMEPAD_AXIS_NEG, SDL_GAMEPAD_AXIS_RIGHTX);
    BIND(GC_CSTICK_RIGHT,0, RF_SRC_KEYBOARD,         SDL_SCANCODE_L);
    BIND(GC_CSTICK_RIGHT,1, RF_SRC_GAMEPAD_AXIS_POS, SDL_GAMEPAD_AXIS_RIGHTX);

#undef BIND
}

const char* getDisplayName(RF_InputBinding binding) {
    static char buf[64];
    switch (binding.type) {
        case RF_SRC_NONE:
            return "---";
        case RF_SRC_KEYBOARD: {
            const char* name = SDL_GetScancodeName((SDL_Scancode)binding.code);
            if (name && name[0]) return name;
            snprintf(buf, sizeof(buf), "Key %d", binding.code);
            return buf;
        }
        case RF_SRC_GAMEPAD_BUTTON: {
            const char* name = SDL_GetGamepadStringForButton((SDL_GamepadButton)binding.code);
            if (name && name[0]) return name;
            snprintf(buf, sizeof(buf), "Btn %d", binding.code);
            return buf;
        }
        case RF_SRC_GAMEPAD_AXIS_POS:
        case RF_SRC_GAMEPAD_AXIS_NEG: {
            const char* base = SDL_GetGamepadStringForAxis((SDL_GamepadAxis)binding.code);
            snprintf(buf, sizeof(buf), "%s%s",
                base ? base : "Axis",
                binding.type == RF_SRC_GAMEPAD_AXIS_POS ? "+" : "-");
            return buf;
        }
        default:
            return "???";
    }
}

bool isListenCandidate(int keyCode) {
    if (keyCode == RF_INPUT_KEY_UNKNOWN) return false;
    if (keyCode == SDL_SCANCODE_ESCAPE) return false;
    if (keyCode >= SDL_SCANCODE_F1 && keyCode <= SDL_SCANCODE_F12) return false;
    return true;
}

void applyRuntimeOptions(bool allowBackgroundInput, bool gyro_enabled, bool gyro_use_mouse, float gyro_sensitivity, bool gyro_invert_x, bool gyro_invert_y, bool gyro_roll_mode) {
    bool effectiveInvertX = gyro_invert_x ^ true;
    rfInput_RegisterGyroConfig(gyro_enabled, gyro_sensitivity, effectiveInvertX, gyro_invert_y, gyro_roll_mode);
    rfInput_SetGyroUseMouse(gyro_use_mouse);
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, allowBackgroundInput ? "1" : "0");
}

}
