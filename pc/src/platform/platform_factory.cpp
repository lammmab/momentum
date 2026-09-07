#include "cr/platform/platform_factory.hpp"

#if defined(CR_PLATFORM_WINDOWS)
#include "cr/platform/impl/platform_windows.hpp"
#elif defined(CR_PLATFORM_POSIX)
#include "cr/platform/impl/platform_posix.hpp"
#endif

namespace cr::platform {
    std::unique_ptr<cr::platform::IPlatform> CreatePlatform() {
#if defined(CR_PLATFORM_WINDOWS)
        return std::make_unique<cr::platform::impl::PlatformWindows>();
#elif defined(CR_PLATFORM_POSIX)
        return std::make_unique<cr::platform::impl::PlatformPosix>();
#else
#error "[src/platform/platform_factory.cpp] No platform implementation selected"
#endif
    }
}
