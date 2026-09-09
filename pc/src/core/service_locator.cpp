#include "momentum/core/service_locator.hpp"
#include "momentum/utility/log.hpp"

CR_FILENAME_LOGGER();

namespace {
    momentum::platform::IPlatform* sPlatform = nullptr;
}

namespace momentum::core::service_locator {
    momentum::platform::IPlatform* GetPlatform() {
        if(sPlatform == nullptr) {
            LOG_ERROR("Platform has not been provided to service locator yet!");
        }
        
        return sPlatform;
    }

    void ProvidePlatform(momentum::platform::IPlatform* platform) {
        sPlatform = platform;
    }
}
