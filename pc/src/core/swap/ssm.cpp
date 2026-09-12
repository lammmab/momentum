#include "momentum/core/swap/ssm.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>

#include "esfa/processing/binary/reader.hpp"
#include "esfa/processing/binary/writer.hpp"

#include "gcesfa/logger.hpp"

namespace momentum::overrides {

namespace {

constexpr size_t kSsmHeaderSize = 12;       // dataOffset + totalSize + soundCount
constexpr size_t kSoundHeaderSize = 64;     // each sound header is 64 bytes
constexpr size_t kDspCoeffsOffset = 28;     // offset to DSP coeffs within sound header
constexpr size_t kDspCoeffsSize = 44;       // DSP ADPCM coefficients size

} // namespace

std::shared_ptr<esfa::interface::ParsedAsset> ParseSsm(
    esfa::binary::Reader& reader,
    const esfa::interface::AssetMeta& meta,
    std::any& /*ctx*/)
{
    auto ssm = std::make_shared<SsmContainer>();

    // Read main header
    ssm->dataOffset = reader.ReadUInt32();
    ssm->totalSize = reader.ReadUInt32();
    uint32_t soundCount = reader.ReadUInt32();

    // Validate header
    // Allow some tolerance for edge cases where sound headers may be slightly truncated
    uint32_t minDataOffset = kSsmHeaderSize + soundCount * kSoundHeaderSize;
    if (ssm->dataOffset + 4 < minDataOffset)  // Allow up to 4 bytes of overlap
    {
        throw std::runtime_error("SSM: invalid dataOffset");
    }
    if (ssm->dataOffset > meta.size)
    {
        throw std::runtime_error("SSM: dataOffset exceeds file size");
    }

    // Read sound headers
    ssm->sounds.resize(soundCount);
    for (uint32_t i = 0; i < soundCount; ++i)
    {
        uint32_t headerStart = kSsmHeaderSize + i * kSoundHeaderSize;
        uint32_t headerEnd = headerStart + kSoundHeaderSize;

        // Handle edge case where last header may overlap with data section
        uint32_t maxRead = (headerEnd > ssm->dataOffset) ?
                          (ssm->dataOffset - headerStart) : kSoundHeaderSize;

        reader.Seek(headerStart, esfa::stream::SeekOffsetType::Start);

        SoundHeader& sound = ssm->sounds[i];

        if (maxRead >= 28) {  // Can read at least up to loopStart
            sound.padding = reader.ReadUInt32();
            sound.channelMode = reader.ReadUInt32();
            sound.sampleRate = reader.ReadUInt32();
            sound.padding2 = reader.ReadUInt32();
            sound.startOffset = reader.ReadUInt32();
            sound.endOffset = reader.ReadUInt32();
            sound.loopStart = reader.ReadUInt32();

            // Read DSP ADPCM coefficients if there's room
            uint32_t coeffsAvailable = (maxRead > 28) ? (maxRead - 28) : 0;
            uint32_t coeffsToRead = std::min(coeffsAvailable, static_cast<uint32_t>(kDspCoeffsSize));

            if (coeffsToRead > 0) {
                reader.Read(reinterpret_cast<char*>(sound.dspCoeffs), coeffsToRead);

                // Byte-swap the DSP coefficients
                for (size_t j = 0; j < coeffsToRead; j += 2)
                {
                    std::swap(sound.dspCoeffs[j], sound.dspCoeffs[j + 1]);
                }
            }
        }
    }

    // Read sample data
    uint32_t sampleDataSize = static_cast<uint32_t>(meta.size) - ssm->dataOffset;
    ssm->sampleData.resize(sampleDataSize);
    if (sampleDataSize > 0)
    {
        reader.Seek(ssm->dataOffset, esfa::stream::SeekOffsetType::Start);
        reader.Read(reinterpret_cast<char*>(ssm->sampleData.data()),
                    static_cast<int32_t>(sampleDataSize));
    }

    return ssm;
}

void ExportSsm(
    esfa::binary::Writer& writer,
    std::shared_ptr<esfa::interface::ParsedAsset> asset,
    std::any& /*ctx*/)
{
    auto ssm = std::static_pointer_cast<SsmContainer>(asset);
    uint32_t soundCount = static_cast<uint32_t>(ssm->sounds.size());

    // Write main header
    writer.Write(ssm->dataOffset);
    writer.Write(ssm->totalSize);
    writer.Write(soundCount);

    // Write sound headers
    for (uint32_t i = 0; i < soundCount; ++i)
    {
        const SoundHeader& sound = ssm->sounds[i];

        writer.Write(sound.padding);
        writer.Write(sound.channelMode);
        writer.Write(sound.sampleRate);
        writer.Write(sound.padding2);
        writer.Write(sound.startOffset);
        writer.Write(sound.endOffset);
        writer.Write(sound.loopStart);

        // Write DSP ADPCM coefficients
        uint8_t swappedCoeffs[kDspCoeffsSize];
        std::memcpy(swappedCoeffs, sound.dspCoeffs, kDspCoeffsSize);

        for (size_t j = 0; j < kDspCoeffsSize; j += 2)
        {
            std::swap(swappedCoeffs[j], swappedCoeffs[j + 1]);
        }

        writer.Write(reinterpret_cast<char*>(swappedCoeffs), kDspCoeffsSize);
    }

    // Write sample data
    if (!ssm->sampleData.empty())
    {
        writer.Seek(static_cast<int64_t>(ssm->dataOffset),
                    esfa::stream::SeekOffsetType::Start);
        writer.Write(reinterpret_cast<char*>(const_cast<uint8_t*>(ssm->sampleData.data())),
                      ssm->sampleData.size());
    }
}

void RegisterSsm(esfa::interface::Registry& registry)
{
    registry.RegisterReader(kSsmRegistryKey, ParseSsm);
    registry.RegisterWriter(kSsmRegistryKey, ExportSsm);
}

} // namespace momentum::overrides
