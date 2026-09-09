#include "momentum/platform/platform_factory.hpp"

#if defined(CR_PLATFORM_WINDOWS)
#include "momentum/platform/impl/platform_windows.hpp"
#elif defined(CR_PLATFORM_POSIX)
#include "momentum/platform/impl/platform_posix.hpp"
#endif

namespace momentum::platform {
    std::unique_ptr<momentum::platform::IPlatform> CreatePlatform() {
#if defined(CR_PLATFORM_WINDOWS)
        return std::make_unique<momentum::platform::impl::PlatformWindows>();
#elif defined(CR_PLATFORM_POSIX)
        return std::make_unique<momentum::platform::impl::PlatformPosix>();
#else
#error "[src/platform/platform_factory.cpp] No platform implementation selected"
#endif
    }
}
