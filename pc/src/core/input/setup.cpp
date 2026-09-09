#include "momentum/core/input/setup.hpp"

#include "rainfall/input/backend/input_backend.h"
#include "rainfall/input/gamepad.h"
#include "rainfall/input/local_source.hpp"

#include "momentum/core/config/config.hpp"

void momentum::input::ProcessEvent(void* event) {
    rfInputProcessEvent(event);
}

void momentum::input::Initialize() {
    rfGamepadInitAttached();
    rainfall::pad::InitLocalSources();
    rainfall::pad::UpdateGamepadAssignments();

    Config::input.resetDefaults();
    Config::input.registerWithRainfall();
}

void momentum::input::Tick() {
    rainfall::pad::TickAll();
    rainfall::pad::UpdateGamepadAssignments();
}
