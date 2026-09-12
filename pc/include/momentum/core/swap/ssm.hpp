#pragma once

/**
 * @file ssm.hpp
 *
 * Super Smash Bros. Melee "*.ssm" sound effect container.
 *
 *   SSM File Format:
 *     u32 dataOffset       - offset to sample data region
 *     u32 totalSize        - total size of sample data
 *     u32 soundCount       - number of sound effects
 *     SoundHeader[soundCount] - array of sound headers (64 bytes each)
 *     <sample data @ dataOffset>
 *
 *   SoundHeader (64 bytes):
 *     u32 padding          - always 0
 *     u32 channelMode      - 0x01=Mono, 0x00=Stereo
 *     u32 sampleRate       - sample rate in Hz
 *     u32 padding2         - always 0
 *     u32 startOffset      - start offset in sample data
 *     u32 endOffset        - end offset in sample data
 *     u32 loopStart        - loop start offset
 *     u8[44] dspCoeffs     - DSP ADPCM coefficients/metadata
 */

#include <any>
#include <cstdint>
#include <memory>
#include <vector>

#include "esfa/interface/registry.hpp"

namespace momentum::overrides {

inline constexpr const char* kSsmRegistryKey = "ssm";

struct SoundHeader
{
    uint32_t padding = 0;
    uint32_t channelMode = 0;     // 0x01=Mono, 0x00=Stereo
    uint32_t sampleRate = 0;
    uint32_t padding2 = 0;
    uint32_t startOffset = 0;
    uint32_t endOffset = 0;
    uint32_t loopStart = 0;
    uint8_t dspCoeffs[44] = {};   // DSP ADPCM coefficients (big-endian u16 values)
};

struct SsmContainer : public esfa::interface::ParsedAsset
{
    uint32_t dataOffset = 0;
    uint32_t totalSize = 0;
    std::vector<SoundHeader> sounds;
    std::vector<uint8_t> sampleData;
};

std::shared_ptr<esfa::interface::ParsedAsset> ParseSsm(
    esfa::binary::Reader& reader,
    const esfa::interface::AssetMeta& meta,
    std::any& ctx);

void ExportSsm(
    esfa::binary::Writer& writer,
    std::shared_ptr<esfa::interface::ParsedAsset> asset,
    std::any& ctx);

void RegisterSsm(esfa::interface::Registry& registry);

} // namespace momentum::overrides
