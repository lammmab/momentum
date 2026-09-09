#include <cstdint>
#include <climits>

typedef uint64_t u64;
typedef int64_t s64;
typedef uint32_t u32;
typedef int32_t s32;

extern "C" {

u64 __div2u(u64 dividend, u64 divisor)
{
    if (divisor == 0) return 0;
    return dividend / divisor;
}

s64 __div2i(s64 dividend, s64 divisor)
{
    if (divisor == 0) return 0;
    return dividend / divisor;
}

u64 __mod2u(u64 dividend, u64 divisor)
{
    if (divisor == 0) return dividend;
    return dividend % divisor;
}

s64 __mod2i(s64 dividend, s64 divisor)
{
    if (divisor == 0) return dividend;
    return dividend % divisor;
}

u64 __shl2i(u64 value, u32 shift)
{
    if (shift == 0) return value;
    if (shift >= 64) return 0;
    return value << shift;
}

u64 __shr2u(u64 value, u32 shift)
{
    if (shift == 0) return value;
    if (shift >= 64) return 0;
    return value >> shift;
}

s64 __shr2i(s64 value, u32 shift)
{
    if (shift == 0) return value;
    if (shift >= 64) return (value < 0) ? -1 : 0;
    return value >> shift;
}

double __cvt_sll_dbl(s64 x)
{
    return (double)x;
}

double __cvt_ull_dbl(u64 x)
{
    return (double)x;
}

float __cvt_sll_flt(s64 x)
{
    return (float)x;
}

float __cvt_ull_flt(u64 x)
{
    return (float)x;
}

u64 __cvt_dbl_usll(double x)
{
    if (x < 0.0) {
        s64 r = (s64)x;
        return (u64)r;
    }
    if (x >= 18446744073709551615.0) { // UINT64_MAX as double
        return UINT64_MAX;
    }
    return (u64)x;
}

u64 __cvt_dbl_ull(double x)
{
    return __cvt_dbl_usll(x);
}

unsigned long __cvt_fp2unsigned(double d)
{
    if (d < 0.0) return 0;
    if (d >= 4294967295.0) return 0xFFFFFFFFUL; // UINT32_MAX as double
    return (unsigned long)d;
}

} // extern "C"
