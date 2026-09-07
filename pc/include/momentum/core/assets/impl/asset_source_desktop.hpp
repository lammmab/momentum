#pragma once

#include "momentum/boot/assets/iasset_source.hpp"
#include "momentum/platform/iplatform.hpp"

#include <string>

namespace momentum::boot::assets::impl {
    class AssetSourceDesktop : public momentum::boot::assets::IAssetSource {
    public:
        explicit AssetSourceDesktop(momentum::platform::IPlatform& platform);
        virtual ~AssetSourceDesktop() = default;

        bool EnsureDataDirectory() override;
        bool AcquireDiscImage(std::string& outPath) override;

    private:
        momentum::platform::IPlatform& mPlatform;
    };
}
