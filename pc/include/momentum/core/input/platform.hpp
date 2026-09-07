#pragma once

#ifndef PC_INPUT_PLATFORM_H
#define PC_INPUT_PLATFORM_H

#include <rainfall/input/input.h>
#include <rainfall/input/hooks.h>

namespace momentum::input::platform {

void resetDefaults(RF_InputBinding bindings[GC_ACTION_COUNT][2]);
const char* getDisplayName(RF_InputBinding binding);
bool isListenCandidate(int keyCode);
void applyRuntimeOptions(bool allowBackgroundInput, bool gyro_enabled, bool gyro_use_mouse, float gyro_sensitivity, bool gyro_invert_x, bool gyro_invert_y, bool gyro_roll_mode);

}

#endif
