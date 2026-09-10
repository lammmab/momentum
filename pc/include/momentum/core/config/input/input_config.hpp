#pragma once

#ifndef PC_INPUT_CONFIG_H
#define PC_INPUT_CONFIG_H

#include <rainfall/input/input.h>
#include <rainfall/input/hooks.h>
#include <rainfall/config/ini.h>

struct InputConfig {
    const char* header = "input";

    RF_InputBinding bindings[GC_ACTION_COUNT][2];
    float deadzone_left      = 0.1f;
    float deadzone_right     = 0.1f;
    float mouse_sensitivity  = 0.1f;
    bool  mouse_mode         = false;
    bool  gyro_enabled       = false;
    bool  gyro_use_mouse     = false;
    float gyro_sensitivity   = 10.0f;
    bool  gyro_invert_x      = false;
    bool  gyro_invert_y      = false;
    bool  gyro_roll_mode     = false;
    bool  background_input   = false;
    bool  virtual_notches    = false;

    bool  invert_x           = false;
    bool  invert_y           = false;

    bool  invert_x_camera    = false;
    bool  invert_y_camera    = false;
    bool  invert_x_aiming    = false;
    bool  invert_y_aiming    = false;

    void resetDefaults();
    void registerWithRainfall() const;
    
    void setBinding(int action, int slot, RF_InputBinding binding);
    void clearBinding(int action, int slot);
    void applyRebind(int action, int slot, RF_InputBinding binding);

    void ParseSection(inih::INIReader& r);
    void WriteSection(inih::INIReader& r);
    static bool isListenCandidate(int scancode);
    static const char* getBindingDisplayName(RF_InputBinding binding);
    static const char* getActionName(int action);
};

#endif
