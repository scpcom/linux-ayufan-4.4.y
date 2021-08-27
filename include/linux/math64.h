#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifndef _LINUX_MATH64_H
#define _LINUX_MATH64_H

#include <linux/types.h>
#include <asm/div64.h>

#if BITS_PER_LONG == 64

#define div64_long(x,y) div64_s64((x),(y))

static inline u64 div_u64_rem(u64 dividend, u32 divisor, u32 *remainder)
{
	*remainder = dividend % divisor;
	return dividend / divisor;
}

static inline s64 div_s64_rem(s64 dividend, s32 divisor, s32 *remainder)
{
	*remainder = dividend % divisor;
	return dividend / divisor;
}

static inline u64 div64_u64(u64 dividend, u64 divisor)
{
	return dividend / divisor;
}

static inline s64 div64_s64(s64 dividend, s64 divisor)
{
	return dividend / divisor;
}

#ifdef MY_ABC_HERE
static inline u64 mod_u64_rem64(u64 dividend, u64 divisor)
{
        return dividend % divisor;
}
#endif

#elif BITS_PER_LONG == 32

#define div64_long(x,y) div_s64((x),(y))

#ifndef div_u64_rem
static inline u64 div_u64_rem(u64 dividend, u32 divisor, u32 *remainder)
{
	*remainder = do_div(dividend, divisor);
	return dividend;
}
#endif

#ifndef div_s64_rem
extern s64 div_s64_rem(s64 dividend, s32 divisor, s32 *remainder);
#endif

#ifndef div64_u64
extern u64 div64_u64(u64 dividend, u64 divisor);
#endif

#ifndef div64_s64
extern s64 div64_s64(s64 dividend, s64 divisor);
#endif

#ifdef MY_ABC_HERE
#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE) || defined(MY_ABC_HERE) || (defined(MY_DEF_HERE) && defined(CONFIG_ARCH_GEN3)) || defined(MY_DEF_HERE)
static inline u64 mod_u64_rem64(u64 dividend, u64 divisor)
{
        if (dividend < divisor) {
                return dividend;
        } else if (dividend == divisor) {
                return (u64)0;
        }

        return dividend - (div64_u64(dividend, divisor) * divisor);
}
#elif !(defined(MY_DEF_HERE) || defined(CONFIG_SYNO_MPC8533) || defined(CONFIG_SYNO_MPC854X) || defined(MY_DEF_HERE))
#error "WARNING: possible lack of rem64 ksymbol"
#endif
#endif

#endif  

#ifndef div_u64
static inline u64 div_u64(u64 dividend, u32 divisor)
{
	u32 remainder;
	return div_u64_rem(dividend, divisor, &remainder);
}
#endif

#ifndef div_s64
static inline s64 div_s64(s64 dividend, s32 divisor)
{
	s32 remainder;
	return div_s64_rem(dividend, divisor, &remainder);
}
#endif

u32 iter_div_u64_rem(u64 dividend, u32 divisor, u64 *remainder);

static __always_inline u32
__iter_div_u64_rem(u64 dividend, u32 divisor, u64 *remainder)
{
	u32 ret = 0;

	while (dividend >= divisor) {
		 
		asm("" : "+rm"(dividend));

		dividend -= divisor;
		ret++;
	}

	*remainder = dividend;

	return ret;
}

#endif  
