#ifndef RUNTIME_RUNTIME_H
#define RUNTIME_RUNTIME_H

#include <Runtime/platform.h>

#ifdef PLATFORM_PC

#ifdef __cplusplus
extern "C" {
#endif

u64 __div2u(u64 dividend, u64 divisor);
s64 __div2i(s64 dividend, s64 divisor);
u64 __mod2u(u64 dividend, u64 divisor);
s64 __mod2i(s64 dividend, s64 divisor);
u64 __shl2i(u64 value, u32 shift);
u64 __shr2u(u64 value, u32 shift);
s64 __shr2i(s64 value, u32 shift);
double __cvt_sll_dbl(s64 x);
double __cvt_ull_dbl(u64 x);
float __cvt_sll_flt(s64 x);
float __cvt_ull_flt(u64 x);
u64 __cvt_dbl_usll(double x);
u64 __cvt_dbl_ull(double x);
unsigned long __cvt_fp2unsigned(double d);

#ifdef __cplusplus
}
#endif

#else
ASM void __div2u(void);
ASM void __div2i(void);
ASM void __mod2u(void);
ASM void __mod2i(void);
ASM void __shl2i(void);
ASM void __shr2u(void);
ASM void __shr2i(void);
ASM void __cvt_sll_dbl(void);
ASM void __cvt_ull_dbl(void);
ASM void __cvt_sll_flt(void);
ASM void __cvt_ull_flt(void);
ASM u64 __cvt_dbl_usll(double);
ASM void __cvt_dbl_ull(void);
ASM unsigned long __cvt_fp2unsigned(register double d);
#endif

#endif
