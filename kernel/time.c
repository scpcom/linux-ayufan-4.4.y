#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/module.h>
#include <linux/timex.h>
#include <linux/capability.h>
#include <linux/clocksource.h>
#include <linux/errno.h>
#include <linux/syscalls.h>
#include <linux/security.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/math64.h>
#include <linux/ptrace.h>

#include <asm/uaccess.h>
#include <asm/unistd.h>

#include "timeconst.h"

struct timezone sys_tz;

EXPORT_SYMBOL(sys_tz);

#ifdef __ARCH_WANT_SYS_TIME

SYSCALL_DEFINE1(time, time_t __user *, tloc)
{
	time_t i = get_seconds();

	if (tloc) {
		if (put_user(i,tloc))
			return -EFAULT;
	}
	force_successful_syscall_return();
	return i;
}

SYSCALL_DEFINE1(stime, time_t __user *, tptr)
{
	struct timespec tv;
	int err;

	if (get_user(tv.tv_sec, tptr))
		return -EFAULT;

	tv.tv_nsec = 0;

	err = security_settime(&tv, NULL);
	if (err)
		return err;

	do_settimeofday(&tv);
	return 0;
}

#endif  

SYSCALL_DEFINE2(gettimeofday, struct timeval __user *, tv,
		struct timezone __user *, tz)
{
	if (likely(tv != NULL)) {
		struct timeval ktv;
		do_gettimeofday(&ktv);
		if (copy_to_user(tv, &ktv, sizeof(ktv)))
			return -EFAULT;
	}
	if (unlikely(tz != NULL)) {
		if (copy_to_user(tz, &sys_tz, sizeof(sys_tz)))
			return -EFAULT;
	}
	return 0;
}

static inline void warp_clock(void)
{
	write_seqlock_irq(&xtime_lock);
	wall_to_monotonic.tv_sec -= sys_tz.tz_minuteswest * 60;
	xtime.tv_sec += sys_tz.tz_minuteswest * 60;
	update_xtime_cache(0);
	write_sequnlock_irq(&xtime_lock);
	clock_was_set();
}

int do_sys_settimeofday(struct timespec *tv, struct timezone *tz)
{
	static int firsttime = 1;
	int error = 0;

	if (tv && !timespec_valid(tv))
		return -EINVAL;

	error = security_settime(tv, tz);
	if (error)
		return error;

	if (tz) {
		 
		sys_tz = *tz;
		update_vsyscall_tz();
		if (firsttime) {
			firsttime = 0;
#ifndef MY_ABC_HERE
			if (!tv)
				warp_clock();
#endif
		}
	}
	if (tv)
	{
		 
		return do_settimeofday(tv);
	}
	return 0;
}

SYSCALL_DEFINE2(settimeofday, struct timeval __user *, tv,
		struct timezone __user *, tz)
{
	struct timeval user_tv;
	struct timespec	new_ts;
	struct timezone new_tz;

	if (tv) {
		if (copy_from_user(&user_tv, tv, sizeof(*tv)))
			return -EFAULT;
		new_ts.tv_sec = user_tv.tv_sec;
		new_ts.tv_nsec = user_tv.tv_usec * NSEC_PER_USEC;
	}
	if (tz) {
		if (copy_from_user(&new_tz, tz, sizeof(*tz)))
			return -EFAULT;
	}

	return do_sys_settimeofday(tv ? &new_ts : NULL, tz ? &new_tz : NULL);
}

SYSCALL_DEFINE1(adjtimex, struct timex __user *, txc_p)
{
	struct timex txc;		 
	int ret;

	if(copy_from_user(&txc, txc_p, sizeof(struct timex)))
		return -EFAULT;
	ret = do_adjtimex(&txc);
	return copy_to_user(txc_p, &txc, sizeof(struct timex)) ? -EFAULT : ret;
}

struct timespec current_fs_time(struct super_block *sb)
{
	struct timespec now = current_kernel_time();
	return timespec_trunc(now, sb->s_time_gran);
}
EXPORT_SYMBOL(current_fs_time);

unsigned int inline jiffies_to_msecs(const unsigned long j)
{
#if HZ <= MSEC_PER_SEC && !(MSEC_PER_SEC % HZ)
	return (MSEC_PER_SEC / HZ) * j;
#elif HZ > MSEC_PER_SEC && !(HZ % MSEC_PER_SEC)
	return (j + (HZ / MSEC_PER_SEC) - 1)/(HZ / MSEC_PER_SEC);
#else
# if BITS_PER_LONG == 32
	return (HZ_TO_MSEC_MUL32 * j) >> HZ_TO_MSEC_SHR32;
# else
	return (j * HZ_TO_MSEC_NUM) / HZ_TO_MSEC_DEN;
# endif
#endif
}
EXPORT_SYMBOL(jiffies_to_msecs);

unsigned int inline jiffies_to_usecs(const unsigned long j)
{
#if HZ <= USEC_PER_SEC && !(USEC_PER_SEC % HZ)
	return (USEC_PER_SEC / HZ) * j;
#elif HZ > USEC_PER_SEC && !(HZ % USEC_PER_SEC)
	return (j + (HZ / USEC_PER_SEC) - 1)/(HZ / USEC_PER_SEC);
#else
# if BITS_PER_LONG == 32
	return (HZ_TO_USEC_MUL32 * j) >> HZ_TO_USEC_SHR32;
# else
	return (j * HZ_TO_USEC_NUM) / HZ_TO_USEC_DEN;
# endif
#endif
}
EXPORT_SYMBOL(jiffies_to_usecs);

struct timespec timespec_trunc(struct timespec t, unsigned gran)
{
	 
	if (gran <= jiffies_to_usecs(1) * 1000) {
		 
	} else if (gran == 1000000000) {
		t.tv_nsec = 0;
	} else {
		t.tv_nsec -= t.tv_nsec % gran;
	}
	return t;
}
EXPORT_SYMBOL(timespec_trunc);

#ifndef CONFIG_GENERIC_TIME
 
void getnstimeofday(struct timespec *tv)
{
	struct timeval x;

	do_gettimeofday(&x);
	tv->tv_sec = x.tv_sec;
	tv->tv_nsec = x.tv_usec * NSEC_PER_USEC;
}
EXPORT_SYMBOL_GPL(getnstimeofday);
#endif

unsigned long
mktime(const unsigned int year0, const unsigned int mon0,
       const unsigned int day, const unsigned int hour,
       const unsigned int min, const unsigned int sec)
{
	unsigned int mon = mon0, year = year0;

	if (0 >= (int) (mon -= 2)) {
		mon += 12;	 
		year -= 1;
	}

	return ((((unsigned long)
		  (year/4 - year/100 + year/400 + 367*mon/12 + day) +
		  year*365 - 719499
	    )*24 + hour  
	  )*60 + min  
	)*60 + sec;  
}

EXPORT_SYMBOL(mktime);

void set_normalized_timespec(struct timespec *ts, time_t sec, s64 nsec)
{
	while (nsec >= NSEC_PER_SEC) {
		 
		asm("" : "+rm"(nsec));
		nsec -= NSEC_PER_SEC;
		++sec;
	}
	while (nsec < 0) {
		asm("" : "+rm"(nsec));
		nsec += NSEC_PER_SEC;
		--sec;
	}
	ts->tv_sec = sec;
	ts->tv_nsec = nsec;
}
EXPORT_SYMBOL(set_normalized_timespec);

struct timespec ns_to_timespec(const s64 nsec)
{
	struct timespec ts;
	s32 rem;

	if (!nsec)
		return (struct timespec) {0, 0};

	ts.tv_sec = div_s64_rem(nsec, NSEC_PER_SEC, &rem);
	if (unlikely(rem < 0)) {
		ts.tv_sec--;
		rem += NSEC_PER_SEC;
	}
	ts.tv_nsec = rem;

	return ts;
}
EXPORT_SYMBOL(ns_to_timespec);

struct timeval ns_to_timeval(const s64 nsec)
{
	struct timespec ts = ns_to_timespec(nsec);
	struct timeval tv;

	tv.tv_sec = ts.tv_sec;
	tv.tv_usec = (suseconds_t) ts.tv_nsec / 1000;

	return tv;
}
EXPORT_SYMBOL(ns_to_timeval);

unsigned long msecs_to_jiffies(const unsigned int m)
{
	 
	if ((int)m < 0)
		return MAX_JIFFY_OFFSET;

#if HZ <= MSEC_PER_SEC && !(MSEC_PER_SEC % HZ)
	 
	return (m + (MSEC_PER_SEC / HZ) - 1) / (MSEC_PER_SEC / HZ);
#elif HZ > MSEC_PER_SEC && !(HZ % MSEC_PER_SEC)
	 
	if (m > jiffies_to_msecs(MAX_JIFFY_OFFSET))
		return MAX_JIFFY_OFFSET;

	return m * (HZ / MSEC_PER_SEC);
#else
	 
	if (HZ > MSEC_PER_SEC && m > jiffies_to_msecs(MAX_JIFFY_OFFSET))
		return MAX_JIFFY_OFFSET;

	return (MSEC_TO_HZ_MUL32 * m + MSEC_TO_HZ_ADJ32)
		>> MSEC_TO_HZ_SHR32;
#endif
}
EXPORT_SYMBOL(msecs_to_jiffies);

unsigned long usecs_to_jiffies(const unsigned int u)
{
	if (u > jiffies_to_usecs(MAX_JIFFY_OFFSET))
		return MAX_JIFFY_OFFSET;
#if HZ <= USEC_PER_SEC && !(USEC_PER_SEC % HZ)
	return (u + (USEC_PER_SEC / HZ) - 1) / (USEC_PER_SEC / HZ);
#elif HZ > USEC_PER_SEC && !(HZ % USEC_PER_SEC)
	return u * (HZ / USEC_PER_SEC);
#else
	return (USEC_TO_HZ_MUL32 * u + USEC_TO_HZ_ADJ32)
		>> USEC_TO_HZ_SHR32;
#endif
}
EXPORT_SYMBOL(usecs_to_jiffies);

unsigned long
timespec_to_jiffies(const struct timespec *value)
{
	unsigned long sec = value->tv_sec;
	long nsec = value->tv_nsec + TICK_NSEC - 1;

	if (sec >= MAX_SEC_IN_JIFFIES){
		sec = MAX_SEC_IN_JIFFIES;
		nsec = 0;
	}
	return (((u64)sec * SEC_CONVERSION) +
		(((u64)nsec * NSEC_CONVERSION) >>
		 (NSEC_JIFFIE_SC - SEC_JIFFIE_SC))) >> SEC_JIFFIE_SC;

}
EXPORT_SYMBOL(timespec_to_jiffies);

void
jiffies_to_timespec(const unsigned long jiffies, struct timespec *value)
{
	 
	u32 rem;
	value->tv_sec = div_u64_rem((u64)jiffies * TICK_NSEC,
				    NSEC_PER_SEC, &rem);
	value->tv_nsec = rem;
}
EXPORT_SYMBOL(jiffies_to_timespec);

unsigned long
timeval_to_jiffies(const struct timeval *value)
{
	unsigned long sec = value->tv_sec;
	long usec = value->tv_usec;

	if (sec >= MAX_SEC_IN_JIFFIES){
		sec = MAX_SEC_IN_JIFFIES;
		usec = 0;
	}
	return (((u64)sec * SEC_CONVERSION) +
		(((u64)usec * USEC_CONVERSION + USEC_ROUND) >>
		 (USEC_JIFFIE_SC - SEC_JIFFIE_SC))) >> SEC_JIFFIE_SC;
}
EXPORT_SYMBOL(timeval_to_jiffies);

void jiffies_to_timeval(const unsigned long jiffies, struct timeval *value)
{
	 
	u32 rem;

	value->tv_sec = div_u64_rem((u64)jiffies * TICK_NSEC,
				    NSEC_PER_SEC, &rem);
	value->tv_usec = rem / NSEC_PER_USEC;
}
EXPORT_SYMBOL(jiffies_to_timeval);

clock_t jiffies_to_clock_t(long x)
{
#if (TICK_NSEC % (NSEC_PER_SEC / USER_HZ)) == 0
# if HZ < USER_HZ
	return x * (USER_HZ / HZ);
# else
	return x / (HZ / USER_HZ);
# endif
#else
	return div_u64((u64)x * TICK_NSEC, NSEC_PER_SEC / USER_HZ);
#endif
}
EXPORT_SYMBOL(jiffies_to_clock_t);

unsigned long clock_t_to_jiffies(unsigned long x)
{
#if (HZ % USER_HZ)==0
	if (x >= ~0UL / (HZ / USER_HZ))
		return ~0UL;
	return x * (HZ / USER_HZ);
#else
	 
	if (x >= ~0UL / HZ * USER_HZ)
		return ~0UL;

	return div_u64((u64)x * HZ, USER_HZ);
#endif
}
EXPORT_SYMBOL(clock_t_to_jiffies);

u64 jiffies_64_to_clock_t(u64 x)
{
#if (TICK_NSEC % (NSEC_PER_SEC / USER_HZ)) == 0
# if HZ < USER_HZ
	x = div_u64(x * USER_HZ, HZ);
# elif HZ > USER_HZ
	x = div_u64(x, HZ / USER_HZ);
# else
	 
# endif
#else
	 
	x = div_u64(x * TICK_NSEC, (NSEC_PER_SEC / USER_HZ));
#endif
	return x;
}
EXPORT_SYMBOL(jiffies_64_to_clock_t);

u64 nsec_to_clock_t(u64 x)
{
#if (NSEC_PER_SEC % USER_HZ) == 0
	return div_u64(x, NSEC_PER_SEC / USER_HZ);
#elif (USER_HZ % 512) == 0
	return div_u64(x * USER_HZ / 512, NSEC_PER_SEC / 512);
#else
	 
	return div_u64(x * 9, (9ull * NSEC_PER_SEC + (USER_HZ / 2)) / USER_HZ);
#endif
}

#if (BITS_PER_LONG < 64)
u64 get_jiffies_64(void)
{
	unsigned long seq;
	u64 ret;

	do {
		seq = read_seqbegin(&xtime_lock);
		ret = jiffies_64;
	} while (read_seqretry(&xtime_lock, seq));
	return ret;
}
EXPORT_SYMBOL(get_jiffies_64);
#endif

EXPORT_SYMBOL(jiffies);

struct timespec timespec_add_safe(const struct timespec lhs,
				  const struct timespec rhs)
{
	struct timespec res;

	set_normalized_timespec(&res, lhs.tv_sec + rhs.tv_sec,
				lhs.tv_nsec + rhs.tv_nsec);

	if (res.tv_sec < lhs.tv_sec || res.tv_sec < rhs.tv_sec)
		res.tv_sec = TIME_T_MAX;

	return res;
}
