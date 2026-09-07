#pragma once

#include <string>

namespace momentum::platform {
    class IPlatform;
}

namespace momentum::boot::assets {
    class IAssetSource {
    public:
        virtual ~IAssetSource() = default;

        virtual bool EnsureDataDirectory() = 0;
        virtual bool AcquireDiscImage(std::string& outPath) = 0;
    };
}
