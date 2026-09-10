#pragma once

#ifndef PC_INPUT_PLATFORM_H
#define PC_INPUT_PLATFORM_H

#include <rainfall/input/input.h>
#include <rainfall/input/hooks.h>

#ifdef __cplusplus
extern "C" {
#endif

void pc_input_reset_default_bindings(RF_InputBinding bindings[GC_ACTION_COUNT][2]);
const char* pc_input_get_binding_display_name(RF_InputBinding binding);
bool pc_input_is_listen_candidate(int keyCode);
void pc_input_apply_runtime_options(bool allowBackgroundInput, bool gyro_enabled, bool gyro_use_mouse, float gyro_sensitivity, bool gyro_invert_x, bool gyro_invert_y, bool gyro_roll_mode);

#ifdef __cplusplus
}
#endif

#endif
