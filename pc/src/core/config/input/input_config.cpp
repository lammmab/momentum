#include "cr/core/config/input/input_config.hpp"
#include "cr/core/config/config.hpp"
#include "cr/core/input/platform.hpp"
#include "rainfall/input/local_source.hpp"

#include <rainfall/input/backend/input_backend.h>

#include <cstdio>

static const char* sActionNames[] = {
    "A Button", "B Button", "X Button", "Y Button",
    "L Trigger", "R Trigger", "Z Button", "Start",
    "D-Pad Up", "D-Pad Down", "D-Pad Left", "D-Pad Right",
    "Stick Up", "Stick Down", "Stick Left", "Stick Right",
    "C-Stick Up", "C-Stick Down", "C-Stick Left", "C-Stick Right",
    "Slow Modifier","Turbo Modifier","Rewind",
};

static const char* sActionKeys[] = {
    "a", "b", "x", "y", "l", "r", "z", "start",
    "dup", "ddown", "dleft", "dright",
    "sup", "sdown", "sleft", "sright",
    "cup", "cdown", "cleft", "cright",
    "mslow","mturbo","rewind",
};

static const int sActionTableCount = (int)(sizeof(sActionNames) / sizeof(sActionNames[0]));

void InputConfig::ParseSection(inih::INIReader& r) {
    resetDefaults();
    deadzone_left      = r.Get<float>(header, "deadzone_left",     std::move(float(deadzone_left)));
    deadzone_right     = r.Get<float>(header, "deadzone_right",    std::move(float(deadzone_right)));
    mouse_sensitivity  = r.Get<float>(header, "mouse_sensitivity", std::move(float(mouse_sensitivity)));
    mouse_mode         = r.Get<bool> (header, "mousemode",         std::move(bool(mouse_mode)));
    background_input   = r.Get<bool> (header, "background_input",  std::move(bool(background_input)));
    //virtual_notches    = r.Get<bool> (header, "virtual_notches",   std::move(bool(virtual_notches)));

    gyro_enabled       = r.Get<bool> (header, "gyro_enabled",      std::move(bool(gyro_enabled)));
    //gyro_use_mouse     = r.Get<bool> (header, "gyro_use_mouse",    std::move(bool(gyro_use_mouse)));
    gyro_sensitivity   = r.Get<float>(header, "gyro_sensitivity",  std::move(float(gyro_sensitivity)));
    gyro_invert_x      = r.Get<bool> (header, "gyro_invert_x",     std::move(bool(gyro_invert_x)));
    gyro_invert_y      = r.Get<bool> (header, "gyro_invert_y",     std::move(bool(gyro_invert_y)));
    //gyro_roll_mode     = r.Get<bool> (header, "gyro_roll_mode",    std::move(bool(gyro_roll_mode)));

    invert_x           = r.Get<bool>(header, "invert_x",           std::move(bool(invert_x)));
    invert_y           = r.Get<bool>(header, "invert_y",           std::move(bool(invert_y)));

    invert_x_camera    = r.Get<bool>(header, "invert_x_camera",    std::move(bool(invert_x_camera)));
    invert_y_camera    = r.Get<bool>(header, "invert_y_camera",    std::move(bool(invert_y_camera)));
    invert_x_aiming    = r.Get<bool>(header, "invert_x_aiming",    std::move(bool(invert_x_aiming)));
    invert_y_aiming    = r.Get<bool>(header, "invert_y_aiming",    std::move(bool(invert_y_aiming)));

    for (int a = 0; a < GC_ACTION_COUNT && a < sActionTableCount; a++) {
        for (int s = 0; s < 2; s++) {
            char key[32];
            snprintf(key, sizeof(key), "%s_%d", sActionKeys[a], s);
            std::string val = r.Get<std::string>(header, key, std::string("none"));
            RF_InputBinding parsed;
            if (RF_ParseBinding(val.c_str(), &parsed) && parsed.type != RF_SRC_NONE)
                bindings[a][s] = parsed;
        }
    }
}

void InputConfig::WriteSection(inih::INIReader& r) {
    r.InsertEntry(header, "background_input",  background_input);
    //r.InsertEntry(header, "virtual_notches",   virtual_notches);
    r.InsertEntry(header, "deadzone_left",     deadzone_left);
    r.InsertEntry(header, "deadzone_right",    deadzone_right);
    r.InsertEntry(header, "mouse_sensitivity", mouse_sensitivity);
    r.InsertEntry(header, "mousemode",         mouse_mode);

    r.InsertEntry(header, "invert_x",          invert_x);
    r.InsertEntry(header, "invert_y",          invert_y);

    r.InsertEntry(header, "invert_x_camera",   invert_x_camera);
    r.InsertEntry(header, "invert_y_camera",   invert_y_camera);
    r.InsertEntry(header, "invert_x_aiming",   invert_x_aiming);
    r.InsertEntry(header, "invert_y_aiming",   invert_y_aiming);

    r.InsertEntry(header, "gyro_enabled",      gyro_enabled);
    //r.InsertEntry(header, "gyro_use_mouse",    gyro_use_mouse);
    r.InsertEntry(header, "gyro_sensitivity",  gyro_sensitivity);
    r.InsertEntry(header, "gyro_invert_x",     gyro_invert_x);
    r.InsertEntry(header, "gyro_invert_y",     gyro_invert_y);
    //r.InsertEntry(header, "gyro_roll_mode",    gyro_roll_mode);

    for (int a = 0; a < GC_ACTION_COUNT && a < sActionTableCount; a++) {
        for (int s = 0; s < 2; s++) {
            char key[32], val[32];
            snprintf(key, sizeof(key), "%s_%d", sActionKeys[a], s);
            RF_FormatBinding(bindings[a][s], val, sizeof(val));
            r.InsertEntry(header, std::string(key), std::string(val));
        }
    }
}

void InputConfig::resetDefaults() {
    background_input = false;
    cr::input::platform::resetDefaults(bindings);
}

void InputConfig::registerWithRainfall() const {
    rfInputSetMouseModeEnabled(mouse_mode);

    for (int ch = 0; ch < RF_INPUT_MAX_GAMEPADS; ch++) {
        rainfall::pad::LocalSource* src = rainfall::pad::GetLocalSource(ch);
        if (!src) continue;

        src->SetBindings(const_cast<RF_InputBinding*>(&bindings[0][0]), GC_ACTION_COUNT);
        src->SetDeadzones(deadzone_left, deadzone_right);
        src->SetMouseSensitivity(mouse_sensitivity);
        //src->SetVirtualNotches(virtual_notches);
    }
}

const char* InputConfig::getActionName(int action) {
    if (action < 0 || action >= GC_ACTION_COUNT || action >= sActionTableCount) return "???";
    return sActionNames[action] ? sActionNames[action] : "???";
}

const char* InputConfig::getBindingDisplayName(RF_InputBinding binding) {
    return cr::input::platform::getDisplayName(binding);
}

bool InputConfig::isListenCandidate(int scancode) {
    return cr::input::platform::isListenCandidate(scancode);
}

void InputConfig::setBinding(int action, int slot, RF_InputBinding binding) {
    if (action < 0 || action >= GC_ACTION_COUNT || slot < 0 || slot > 1) return;
    bindings[action][slot] = binding;
}

void InputConfig::clearBinding(int action, int slot) {
    if (action < 0 || action >= GC_ACTION_COUNT || slot < 0 || slot > 1) return;
    bindings[action][slot] = RF_BindingNone();
}

static bool bindingsEqual(RF_InputBinding a, RF_InputBinding b) {
    return a.type == b.type && a.code == b.code && a.type != RF_SRC_NONE;
}

void InputConfig::applyRebind(int action, int slot, RF_InputBinding binding) {
    RF_InputBinding displaced = bindings[action][slot];
    for (int a = 0; a < GC_ACTION_COUNT; a++) {
        for (int s = 0; s < 2; s++) {
            if (a == action && s == slot) continue;
            if (bindingsEqual(bindings[a][s], binding)) {
                bindings[a][s] = displaced;
                break;
            }
        }
    }
    setBinding(action, slot, binding);
    registerWithRainfall();
}
