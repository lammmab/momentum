#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

namespace momentum::dol {

bool init(const char* gameDataDir);

void patchMatDLSetImage3(void* matDL, uint32_t matDLSize, const void* texPtr);
void patchMatDLTex0Dims(void* matDL, uint32_t matDLSize, uint16_t width, uint16_t height, uint8_t format);

bool loadDolRange(void* dst, uint32_t vaddr, uint32_t size);
bool loadRelRange(void* dst, const char* relName, uint32_t fileOffset, uint32_t size);

}

#ifdef __cplusplus
extern "C" {
#endif

bool pc_assets_load_dol_range(void* dst, uint32_t vaddr, uint32_t size);
bool pc_assets_load_rel_range(void* dst, const char* relName, uint32_t fileOffset, uint32_t size);

#ifdef __cplusplus
}
#endif

#if defined(_MSC_VER)
  #define PC_ASSET_ALIGN(n) __declspec(align(n))
#elif defined(__GNUC__) || defined(__clang__)
  #define PC_ASSET_ALIGN(n) __attribute__((aligned(n)))
#else
  #define PC_ASSET_ALIGN(n) alignas(n)
#endif

#define PC_DOL_ASSET(name, vaddr, size)                                      \
    static PC_ASSET_ALIGN(32) uint8_t name##_buf[(size)];          \
    static inline uint8_t* name##_get(void) {                          \
        static int loaded = -1;                                               \
        if (loaded < 0)                                                       \
            loaded = pc_assets_load_dol_range(name##_buf, (vaddr), (size));   \
        return name##_buf;                                                    \
    }

#define PC_REL_ASSET(name, relName, fileOffset, size)                         \
    static PC_ASSET_ALIGN(32) uint8_t name##_buf[(size)];           \
    static inline uint8_t* name##_get(void) {                           \
        static int loaded = -1;                                               \
        if (loaded < 0)                                                        \
            loaded = pc_assets_load_rel_range(name##_buf, (relName),          \
                                               (fileOffset), (size));          \
        return name##_buf;                                                     \
    }
