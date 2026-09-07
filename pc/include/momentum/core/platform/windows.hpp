#ifndef PC_WINDOWS_H
#define PC_WINDOWS_H

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#undef near
#undef far
#undef IN
#undef OUT
#undef OPTIONAL
#endif

#endif
