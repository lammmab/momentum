#include "cr/core/service_locator.hpp"
#include "cr/utility/log.hpp"

CR_FILENAME_LOGGER();

namespace {
    cr::platform::IPlatform* sPlatform = nullptr;
}

namespace cr::core::service_locator {
    cr::platform::IPlatform* GetPlatform() {
        if(sPlatform == nullptr) {
            LOG_ERROR("Platform has not been provided to service locator yet!");
        }
        
        return sPlatform;
    }

    void ProvidePlatform(cr::platform::IPlatform* platform) {
        sPlatform = platform;
    }
}
