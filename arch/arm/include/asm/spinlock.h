#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifndef __ASM_SPINLOCK_H
#define __ASM_SPINLOCK_H

#if __LINUX_ARM_ARCH__ < 6
#error SMP not supported on pre-ARMv6 CPUs
#endif

#include <asm/processor.h>

#define ALT_SMP(smp, up)					\
	"9998:	" smp "\n"					\
	"	.pushsection \".alt.smp.init\", \"a\"\n"	\
	"	.long	9998b\n"				\
	"	" up "\n"					\
	"	.popsection\n"

#ifdef CONFIG_THUMB2_KERNEL
#define SEV		ALT_SMP("sev.w", "nop.w")
 
#define WFE(cond)	ALT_SMP(		\
	"it " cond "\n\t"			\
	"wfe" cond ".n",			\
						\
	"nop.w"					\
)
#else
#define SEV		ALT_SMP("sev", "nop")
#define WFE(cond)	ALT_SMP("wfe" cond, "nop")
#endif

static inline void dsb_sev(void)
{
#if __LINUX_ARM_ARCH__ >= 7
	__asm__ __volatile__ (
		"dsb\n"
		SEV
	);
#else
	__asm__ __volatile__ (
		"mcr p15, 0, %0, c7, c10, 4\n"
		SEV
		: : "r" (0)
	);
#endif
}

#define arch_spin_is_locked(x)		((x)->lock != 0)
#define arch_spin_unlock_wait(lock) \
	do { while (arch_spin_is_locked(lock)) cpu_relax(); } while (0)

#define arch_spin_lock_flags(lock, flags) arch_spin_lock(lock)

#ifdef CONFIG_SYNO_ALPINE_V2_5_3
extern unsigned int al_spin_lock_wfe_enable;
#endif

static inline void arch_spin_lock(arch_spinlock_t *lock)
{
	unsigned long tmp;

#ifdef CONFIG_SYNO_ALPINE_V2_5_3
	if (unlikely(al_spin_lock_wfe_enable)) {
		__asm__ __volatile__(
	"1:	ldrex	%0, [%1]\n"
	"	teq	%0, #0\n"
		WFE("ne")
	"	strexeq	%0, %2, [%1]\n"
	"	teqeq	%0, #0\n"
	"	bne	1b"
		: "=&r" (tmp)
		: "r" (&lock->lock), "r" (1)
		: "cc");
	} else {
		__asm__ __volatile__(
	"1:	ldrex	%0, [%1]\n"
	"	teq	%0, #0\n"
	"	strexeq	%0, %2, [%1]\n"
	"	teqeq	%0, #0\n"
	"	bne	1b"
		: "=&r" (tmp)
		: "r" (&lock->lock), "r" (1)
		: "cc");
	} 
#else
	__asm__ __volatile__(
"1:	ldrex	%0, [%1]\n"
"	teq	%0, #0\n"
#if (defined(MY_DEF_HERE) || defined(MY_DEF_HERE)) && defined(CONFIG_SHEEVA_ERRATA_ARM_CPU_BTS61)
#else
	WFE("ne")
#endif
"	strexeq	%0, %2, [%1]\n"
"	teqeq	%0, #0\n"
"	bne	1b"
	: "=&r" (tmp)
	: "r" (&lock->lock), "r" (1)
	: "cc");
#endif

	smp_mb();
}

static inline int arch_spin_trylock(arch_spinlock_t *lock)
{
	unsigned long tmp;

#ifdef MY_DEF_HERE
	unsigned long strex_fail = 0;

	do {
		__asm__ __volatile__(
	"	mov	%1, #0\n"
	"	ldrex	%0, [%2]\n"
	"	teq	%0, #0\n"
	"	strexeq	%1, %3, [%2]"
		: "=&r" (tmp), "=&r" (strex_fail)
		: "r" (&lock->lock), "r" (1)
		: "cc");
	} while (strex_fail);
#else
	__asm__ __volatile__(
"	ldrex	%0, [%1]\n"
"	teq	%0, #0\n"
"	strexeq	%0, %2, [%1]"
	: "=&r" (tmp)
	: "r" (&lock->lock), "r" (1)
	: "cc");
#endif
	if (tmp == 0) {
		smp_mb();
		return 1;
	} else {
		return 0;
	}
}

static inline void arch_spin_unlock(arch_spinlock_t *lock)
{
	smp_mb();

	__asm__ __volatile__(
"	str	%1, [%0]\n"
	:
	: "r" (&lock->lock), "r" (0)
	: "cc");
#ifdef CONFIG_SYNO_ALPINE_V2_5_3
	if (unlikely(al_spin_lock_wfe_enable))
#endif
		dsb_sev();
}

static inline void arch_write_lock(arch_rwlock_t *rw)
{
	unsigned long tmp;

#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
	do{
#endif
	__asm__ __volatile__(
"1:	ldrex	%0, [%1]\n"
"	teq	%0, #0\n"
#if (defined(MY_DEF_HERE) || defined(MY_DEF_HERE)) && defined(CONFIG_SHEEVA_ERRATA_ARM_CPU_BTS61)
#else
	WFE("ne")
#endif
"	strexeq	%0, %2, [%1]\n"
"	teq	%0, #0\n"
"	bne	1b"
	: "=&r" (tmp)
	: "r" (&rw->lock), "r" (0x80000000)
	: "cc");

#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
	} while (tmp && atomic_backoff_delay());
#endif
	smp_mb();
}

static inline int arch_write_trylock(arch_rwlock_t *rw)
{
	unsigned long tmp;

	__asm__ __volatile__(
"1:	ldrex	%0, [%1]\n"
"	teq	%0, #0\n"
"	strexeq	%0, %2, [%1]"
	: "=&r" (tmp)
	: "r" (&rw->lock), "r" (0x80000000)
	: "cc");

	if (tmp == 0) {
		smp_mb();
		return 1;
	} else {
#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
		atomic_backoff_delay();
#endif
		return 0;
	}
}

static inline void arch_write_unlock(arch_rwlock_t *rw)
{
	smp_mb();

	__asm__ __volatile__(
	"str	%1, [%0]\n"
	:
	: "r" (&rw->lock), "r" (0)
	: "cc");

	dsb_sev();
}

#define arch_write_can_lock(x)		((x)->lock == 0)

static inline void arch_read_lock(arch_rwlock_t *rw)
{
	unsigned long tmp, tmp2;

	__asm__ __volatile__(
"1:	ldrex	%0, [%2]\n"
"	adds	%0, %0, #1\n"
"	strexpl	%1, %0, [%2]\n"
#if (defined(MY_DEF_HERE) || defined(MY_DEF_HERE)) && defined(CONFIG_SHEEVA_ERRATA_ARM_CPU_BTS61)
#else
	WFE("mi")
#endif
"	rsbpls	%0, %1, #0\n"
"	bmi	1b"
	: "=&r" (tmp), "=&r" (tmp2)
	: "r" (&rw->lock)
	: "cc");

	smp_mb();
}

static inline void arch_read_unlock(arch_rwlock_t *rw)
{
	unsigned long tmp, tmp2;

	smp_mb();

	__asm__ __volatile__(
"1:	ldrex	%0, [%2]\n"
"	sub	%0, %0, #1\n"
"	strex	%1, %0, [%2]\n"
"	teq	%1, #0\n"
"	bne	1b"
	: "=&r" (tmp), "=&r" (tmp2)
	: "r" (&rw->lock)
	: "cc");

	if (tmp == 0)
		dsb_sev();
}

static inline int arch_read_trylock(arch_rwlock_t *rw)
{
	unsigned long tmp, tmp2 = 1;

	__asm__ __volatile__(
"1:	ldrex	%0, [%2]\n"
"	adds	%0, %0, #1\n"
"	strexpl	%1, %0, [%2]\n"
	: "=&r" (tmp), "+r" (tmp2)
	: "r" (&rw->lock)
	: "cc");

	smp_mb();
	return tmp2 == 0;
}

#define arch_read_can_lock(x)		((x)->lock < 0x80000000)

#define arch_read_lock_flags(lock, flags) arch_read_lock(lock)
#define arch_write_lock_flags(lock, flags) arch_write_lock(lock)

#define arch_spin_relax(lock)	cpu_relax()
#define arch_read_relax(lock)	cpu_relax()
#define arch_write_relax(lock)	cpu_relax()

#endif  
