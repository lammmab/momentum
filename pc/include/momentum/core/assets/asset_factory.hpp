#pragma once

#include <memory>
#include "momentum/boot/assets/iasset_source.hpp"

namespace momentum::platform {
    class IPlatform;
}

namespace momentum::boot::assets {
    std::unique_ptr<IAssetSource> CreateAssetSource(momentum::platform::IPlatform& platform);
}
