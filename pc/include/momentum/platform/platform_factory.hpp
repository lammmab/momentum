#pragma once

#include <memory>
#include "momentum/platform/iplatform.hpp"

namespace momentum::platform {
    std::unique_ptr<IPlatform> CreatePlatform();
}
