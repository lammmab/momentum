#include "momentum/core/swap/sem.hpp"

#include <cstring>

#include "esfa/processing/binary/reader.hpp"
#include "esfa/processing/binary/writer.hpp"

namespace momentum::overrides {

namespace {

inline uint32_t Swap32(uint32_t val) {
    return ((val & 0xFF000000) >> 24) |
           ((val & 0x00FF0000) >> 8)  |
           ((val & 0x0000FF00) << 8)  |
           ((val & 0x000000FF) << 24);
}

inline int32_t Swap32(int32_t val) {
    return static_cast<int32_t>(Swap32(static_cast<uint32_t>(val)));
}

void SwapSemData(uint8_t* data, size_t size) {
    if (size < 4) return;

    size_t offset = 0;

    // SEM file structure from axdriver.c
    // 1. count1 (s32) + array1 (u32[count1]): no pointer relocation
    // 2. count2 (s32) + array2 (u32[count2]): with pointer relocation
    // 3. count3 (s32) + array3 (u32[count3]): no pointer relocation
    // 4. count4 (s32) + array4 (u32[count4]): with pointer relocation
    // 5. count5 (s32) + array5 (u32[count5]): with pointer relocation

    // Array 1
    if (offset + 4 > size) return;
    int32_t count1 = *reinterpret_cast<int32_t*>(data + offset);
    count1 = Swap32(count1);
    *reinterpret_cast<int32_t*>(data + offset) = count1;
    offset += 4;

    if (count1 > 0 && offset + count1 * 4 <= size) {
        for (int32_t i = 0; i < count1; ++i) {
            uint32_t* ptr = reinterpret_cast<uint32_t*>(data + offset + i * 4);
            *ptr = Swap32(*ptr);
        }
        offset += count1 * 4;
    }

    // Array 2
    if (offset + 4 > size) return;
    int32_t count2 = *reinterpret_cast<int32_t*>(data + offset);
    count2 = Swap32(count2);
    *reinterpret_cast<int32_t*>(data + offset) = count2;
    offset += 4;

    if (count2 > 0 && offset + count2 * 4 <= size) {
        for (int32_t i = 0; i < count2; ++i) {
            uint32_t* ptr = reinterpret_cast<uint32_t*>(data + offset + i * 4);
            *ptr = Swap32(*ptr);
        }
        offset += count2 * 4;
    }

    // Array 3
    if (offset + 4 > size) return;
    int32_t count3 = *reinterpret_cast<int32_t*>(data + offset);
    count3 = Swap32(count3);
    *reinterpret_cast<int32_t*>(data + offset) = count3;
    offset += 4;

    if (count3 > 0 && offset + count3 * 4 <= size) {
        for (int32_t i = 0; i < count3; ++i) {
            uint32_t* ptr = reinterpret_cast<uint32_t*>(data + offset + i * 4);
            *ptr = Swap32(*ptr);
        }
        offset += count3 * 4;
    }

    // Array 4
    if (offset + 4 > size) return;
    int32_t count4 = *reinterpret_cast<int32_t*>(data + offset);
    count4 = Swap32(count4);
    *reinterpret_cast<int32_t*>(data + offset) = count4;
    offset += 4;

    if (count4 > 0 && offset + count4 * 4 <= size) {
        for (int32_t i = 0; i < count4; ++i) {
            uint32_t* ptr = reinterpret_cast<uint32_t*>(data + offset + i * 4);
            *ptr = Swap32(*ptr);
        }
        offset += count4 * 4;
    }

    // Array 5
    if (offset + 4 > size) return;
    int32_t count5 = *reinterpret_cast<int32_t*>(data + offset);
    count5 = Swap32(count5);
    *reinterpret_cast<int32_t*>(data + offset) = count5;
    offset += 4;

    if (count5 > 0 && offset + count5 * 4 <= size) {
        for (int32_t i = 0; i < count5; ++i) {
            uint32_t* ptr = reinterpret_cast<uint32_t*>(data + offset + i * 4);
            *ptr = Swap32(*ptr);
        }
        offset += count5 * 4;
    }
}

} // namespace

std::shared_ptr<esfa::interface::ParsedAsset> ParseSem(
    esfa::binary::Reader& reader,
    const esfa::interface::AssetMeta& meta,
    std::any& /*ctx*/)
{
    auto sem = std::make_shared<SemContainer>();

    sem->data.resize(meta.size);
    reader.Seek(0, esfa::stream::SeekOffsetType::Start);
    reader.Read(reinterpret_cast<char*>(sem->data.data()), static_cast<int32_t>(meta.size));

    SwapSemData(sem->data.data(), sem->data.size());

    return sem;
}

void ExportSem(
    esfa::binary::Writer& writer,
    std::shared_ptr<esfa::interface::ParsedAsset> asset,
    std::any& /*ctx*/)
{
    auto sem = std::static_pointer_cast<SemContainer>(asset);

    // Write the little-endian data directly (already swapped in ParseSem)
    writer.Write(reinterpret_cast<char*>(sem->data.data()), sem->data.size());
}

void RegisterSem(esfa::interface::Registry& registry)
{
    registry.RegisterReader(kSemRegistryKey, ParseSem);
    registry.RegisterWriter(kSemRegistryKey, ExportSem);
}

} // namespace momentum::overrides
