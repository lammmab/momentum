#pragma once

/**
 * @file sem.hpp
 *
 * Super Smash Bros. Melee "*.sem" audio metadata container.
 *
 *   SEM File Format:
 *     The file consists of a sequence of counted arrays:
 *       s32 count1
 *       u32 array1[count1]
 *       s32 count2
 *       u32 array2[count2]  (pointers requiring relocation)
 *       s32 count3
 *       u32 array3[count3]
 *       s32 count4
 *       u32 array4[count4]  (pointers requiring relocation)
 *       s32 count5
 *       u32 array5[count5]  (pointers requiring relocation)
 */

#include <any>
#include <cstdint>
#include <memory>
#include <vector>

#include "esfa/interface/registry.hpp"

namespace momentum::overrides {

inline constexpr const char* kSemRegistryKey = "sem";

struct SemContainer : public esfa::interface::ParsedAsset
{
    std::vector<uint8_t> data;
};

std::shared_ptr<esfa::interface::ParsedAsset> ParseSem(
    esfa::binary::Reader& reader,
    const esfa::interface::AssetMeta& meta,
    std::any& ctx);

void ExportSem(
    esfa::binary::Writer& writer,
    std::shared_ptr<esfa::interface::ParsedAsset> asset,
    std::any& ctx);

void RegisterSem(esfa::interface::Registry& registry);

} // namespace momentum::overrides
