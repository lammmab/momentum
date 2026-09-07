#pragma once

#include "momentum/platform/iplatform.hpp"

// While being simple, it is very handy to make the platform specific implementation available globally
namespace momentum::core::service_locator {
    momentum::platform::IPlatform* GetPlatform();
    void ProvidePlatform(momentum::platform::IPlatform* platform);
}
