#include "cr/platform/common/emergency_exit.hpp"

#include <cstdlib>
#include <csignal>

namespace cr::platform::common {
    void emergency_exit::HandleEmergencyExit(int signal) {
        (void)signal;

        std::_Exit(130);
    }
}