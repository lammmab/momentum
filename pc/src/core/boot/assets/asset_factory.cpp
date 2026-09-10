#include "momentum/core/assets/asset_factory.hpp"

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_POSIX)
#include "momentum/core/assets/impl/asset_source_desktop.hpp"
#endif

namespace momentum::boot::assets {
    std::unique_ptr<IAssetSource> CreateAssetSource(momentum::platform::IPlatform& platform) {
#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_POSIX)
        return std::make_unique<impl::AssetSourceDesktop>(platform);
#else // Later we can use this to add support for other platforms, like consoles or mobile that dont use the typical desktop asset source
#error "[src/core/boot/assets/asset_factory.cpp] No asset source implementation selected"
#endif
    }
}
