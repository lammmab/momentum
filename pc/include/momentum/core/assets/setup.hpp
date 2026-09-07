#pragma once

#include "esfa/interface/registry.hpp"

#include <string>

namespace momentum::assets {
    extern esfa::interface::Registry registry;

    bool setup(const std::string& disc_location);
}

namespace momentum::boot::assets {
    bool IsValidDiscImage(const char* path);
}
