#pragma once

namespace momentum::dol {

bool init(const char* gameDataDir);

void patchMatDLSetImage3(void* matDL, uint32_t matDLSize, const void* texPtr);
void patchMatDLTex0Dims(void* matDL, uint32_t matDLSize, uint16_t width, uint16_t height, uint8_t format);

bool loadDolRange(void* dst, uint32_t vaddr, uint32_t size);
bool loadRelRange(void* dst, const char* relName, uint32_t fileOffset, uint32_t size);

}
