#include "cr/core/input/setup.hpp"

#include "rainfall/input/backend/input_backend.h"
#include "rainfall/input/gamepad.h"
#include "rainfall/input/local_source.hpp"

#include "cr/core/config/config.hpp"

void cr::input::ProcessEvent(void* event) {
    rfInputProcessEvent(event);
}

void cr::input::Initialize() {
    rfGamepadInitAttached();
    rainfall::pad::InitLocalSources();
    rainfall::pad::UpdateGamepadAssignments();

    Config::input.resetDefaults();
    Config::input.registerWithRainfall();
}

void cr::input::Tick() {
    rainfall::pad::TickAll();
    rainfall::pad::UpdateGamepadAssignments();
}
