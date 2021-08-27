#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifndef __ASM_ARM_SYSTEM_H
#define __ASM_ARM_SYSTEM_H

#ifdef __KERNEL__

#define CPU_ARCH_UNKNOWN	0
#define CPU_ARCH_ARMv3		1
#define CPU_ARCH_ARMv4		2
#define CPU_ARCH_ARMv4T		3
#define CPU_ARCH_ARMv5		4
#define CPU_ARCH_ARMv5T		5
#define CPU_ARCH_ARMv5TE	6
#define CPU_ARCH_ARMv5TEJ	7
#define CPU_ARCH_ARMv6		8
#define CPU_ARCH_ARMv7		9

#define CR_M	(1 << 0)	 
#define CR_A	(1 << 1)	 
#define CR_C	(1 << 2)	 
#define CR_W	(1 << 3)	 
#define CR_P	(1 << 4)	 
#define CR_D	(1 << 5)	 
#define CR_L	(1 << 6)	 
#define CR_B	(1 << 7)	 
#define CR_S	(1 << 8)	 
#define CR_R	(1 << 9)	 
#define CR_F	(1 << 10)	 
#define CR_Z	(1 << 11)	 
#define CR_I	(1 << 12)	 
#define CR_V	(1 << 13)	 
#define CR_RR	(1 << 14)	 
#define CR_L4	(1 << 15)	 
#define CR_DT	(1 << 16)
#define CR_IT	(1 << 18)
#define CR_ST	(1 << 19)
#define CR_FI	(1 << 21)	 
#define CR_U	(1 << 22)	 
#define CR_XP	(1 << 23)	 
#define CR_VE	(1 << 24)	 
#define CR_EE	(1 << 25)	 
#define CR_TRE	(1 << 28)	 
#define CR_AFE	(1 << 29)	 
#define CR_TE	(1 << 30)	 

#define __asmeq(x, y)  ".ifnc " x "," y " ; .err ; .endif\n\t"

#ifndef __ASSEMBLY__

#include <linux/compiler.h>
#include <linux/linkage.h>
#include <linux/irqflags.h>

#include <asm/outercache.h>

struct thread_info;
struct task_struct;

extern unsigned int system_rev;
extern unsigned int system_serial_low;
extern unsigned int system_serial_high;
extern unsigned int mem_fclk_21285;

struct pt_regs;

void die(const char *msg, struct pt_regs *regs, int err);

struct siginfo;
void arm_notify_die(const char *str, struct pt_regs *regs, struct siginfo *info,
		unsigned long err, unsigned long trap);
#ifdef MY_DEF_HERE
#ifdef CONFIG_ARM_LPAE
#define FAULT_CODE_ALIGNMENT	33
#define FAULT_CODE_DEBUG	34
#else
#define FAULT_CODE_ALIGNMENT	1
#define FAULT_CODE_DEBUG	2
#endif
#endif

void hook_fault_code(int nr, int (*fn)(unsigned long, unsigned int,
				       struct pt_regs *),
		     int sig, int code, const char *name);

void hook_ifault_code(int nr, int (*fn)(unsigned long, unsigned int,
				       struct pt_regs *),
		     int sig, int code, const char *name);

#define xchg(ptr,x) \
	((__typeof__(*(ptr)))__xchg((unsigned long)(x),(ptr),sizeof(*(ptr))))

extern asmlinkage void c_backtrace(unsigned long fp, int pmode);

struct mm_struct;
extern void show_pte(struct mm_struct *mm, unsigned long addr);
extern void __show_regs(struct pt_regs *);

extern int __pure cpu_architecture(void);
extern void cpu_init(void);

void arm_machine_restart(char mode, const char *cmd);
extern void (*arm_pm_restart)(char str, const char *cmd);

#define UDBG_UNDEFINED	(1 << 0)
#define UDBG_SYSCALL	(1 << 1)
#define UDBG_BADABORT	(1 << 2)
#define UDBG_SEGV	(1 << 3)
#define UDBG_BUS	(1 << 4)

extern unsigned int user_debug;

#if __LINUX_ARM_ARCH__ >= 4
#define vectors_high()	(cr_alignment & CR_V)
#else
#define vectors_high()	(0)
#endif

#if __LINUX_ARM_ARCH__ >= 7 ||		\
	(__LINUX_ARM_ARCH__ == 6 && defined(CONFIG_CPU_32v6K))
#define sev()	__asm__ __volatile__ ("sev" : : : "memory")
#define wfe()	__asm__ __volatile__ ("wfe" : : : "memory")
#define wfi()	__asm__ __volatile__ ("wfi" : : : "memory")
#endif

#if __LINUX_ARM_ARCH__ >= 7
#define isb() __asm__ __volatile__ ("isb" : : : "memory")
#define dsb() __asm__ __volatile__ ("dsb" : : : "memory")
#if (defined(MY_DEF_HERE) || defined(MY_DEF_HERE)) && defined(CONFIG_SHEEVA_ERRATA_ARM_CPU_6075)
#define dmb() __asm__ __volatile__ ("dsb" : : : "memory")
#else
#define dmb() __asm__ __volatile__ ("dmb" : : : "memory")
#endif
#elif defined(CONFIG_CPU_XSC3) || __LINUX_ARM_ARCH__ == 6
#define isb() __asm__ __volatile__ ("mcr p15, 0, %0, c7, c5, 4" \
				    : : "r" (0) : "memory")
#define dsb() __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 4" \
				    : : "r" (0) : "memory")
#define dmb() __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 5" \
				    : : "r" (0) : "memory")
#elif defined(CONFIG_CPU_FA526)
#define isb() __asm__ __volatile__ ("mcr p15, 0, %0, c7, c5, 4" \
				    : : "r" (0) : "memory")
#define dsb() __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 4" \
				    : : "r" (0) : "memory")
#define dmb() __asm__ __volatile__ ("" : : : "memory")
#else
#define isb() __asm__ __volatile__ ("" : : : "memory")
#define dsb() __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 4" \
				    : : "r" (0) : "memory")
#define dmb() __asm__ __volatile__ ("" : : : "memory")
#endif

#ifdef CONFIG_ARCH_HAS_BARRIERS
#include <mach/barriers.h>
#elif defined(CONFIG_ARM_DMA_MEM_BUFFERABLE) || defined(CONFIG_SMP)
#define mb()		do { dsb(); outer_sync(); } while (0)
#define rmb()		dsb()
#define wmb()		mb()
#else
#include <asm/memory.h>
#define mb()	do { if (arch_is_coherent()) dmb(); else barrier(); } while (0)
#define rmb()	do { if (arch_is_coherent()) dmb(); else barrier(); } while (0)
#define wmb()	do { if (arch_is_coherent()) dmb(); else barrier(); } while (0)
#endif

#ifndef CONFIG_SMP
#define smp_mb()	barrier()
#define smp_rmb()	barrier()
#define smp_wmb()	barrier()
#else
#define smp_mb()	dmb()
#define smp_rmb()	dmb()
#define smp_wmb()	dmb()
#endif

#define read_barrier_depends()		do { } while(0)
#define smp_read_barrier_depends()	do { } while(0)

#define set_mb(var, value)	do { var = value; smp_mb(); } while (0)
#define nop() __asm__ __volatile__("mov\tr0,r0\t@ nop\n\t");

extern unsigned long cr_no_alignment;	 
extern unsigned long cr_alignment;	 

static inline unsigned int get_cr(void)
{
	unsigned int val;
	asm("mrc p15, 0, %0, c1, c0, 0	@ get CR" : "=r" (val) : : "cc");
	return val;
}

static inline void set_cr(unsigned int val)
{
	asm volatile("mcr p15, 0, %0, c1, c0, 0	@ set CR"
	  : : "r" (val) : "cc");
	isb();
}

#ifndef CONFIG_SMP
extern void adjust_cr(unsigned long mask, unsigned long set);
#endif

#define CPACC_FULL(n)		(3 << (n * 2))
#define CPACC_SVC(n)		(1 << (n * 2))
#define CPACC_DISABLE(n)	(0 << (n * 2))

static inline unsigned int get_copro_access(void)
{
	unsigned int val;
	asm("mrc p15, 0, %0, c1, c0, 2 @ get copro access"
	  : "=r" (val) : : "cc");
	return val;
}

static inline void set_copro_access(unsigned int val)
{
	asm volatile("mcr p15, 0, %0, c1, c0, 2 @ set copro access"
	  : : "r" (val) : "cc");
	isb();
}

#define __ARCH_WANT_INTERRUPTS_ON_CTXSW

extern struct task_struct *__switch_to(struct task_struct *, struct thread_info *, struct thread_info *);

#define switch_to(prev,next,last)					\
do {									\
	last = __switch_to(prev,task_thread_info(prev), task_thread_info(next));	\
} while (0)

#if defined(CONFIG_CPU_SA1100) || defined(CONFIG_CPU_SA110)
 
#define swp_is_buggy
#endif

#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
static inline int atomic_backoff_delay(void)
{
#ifdef CONFIG_SHEEVA_ERRATA_ARM_CPU_ADD_DELAY_FOR_STREX
        unsigned int delay;

        __asm__ __volatile__(
        "       mrc     p15, 0, %0, c0, c0, 5\n"
        "       and     %0, %0, #0xf\n"
        "       mov     %0, %0, lsl #8\n"
        "1:     subs    %0, %0, #1\n"
        "       bpl     1b\n"
        : "=&r" (delay)
        :
        : "cc");
#endif
        return 1;
}
#endif

static inline unsigned long __xchg(unsigned long x, volatile void *ptr, int size)
{
	extern void __bad_xchg(volatile void *, int);
	unsigned long ret;
#ifdef swp_is_buggy
	unsigned long flags;
#endif
#if __LINUX_ARM_ARCH__ >= 6
	unsigned int tmp;
#endif

	smp_mb();

	switch (size) {
#if __LINUX_ARM_ARCH__ >= 6
	case 1:
		asm volatile("@	__xchg1\n"
		"1:	ldrexb	%0, [%3]\n"
		"	strexb	%1, %2, [%3]\n"
		"	teq	%1, #0\n"
		"	bne	1b"
			: "=&r" (ret), "=&r" (tmp)
			: "r" (x), "r" (ptr)
			: "memory", "cc");
		break;
	case 4:
		asm volatile("@	__xchg4\n"
		"1:	ldrex	%0, [%3]\n"
		"	strex	%1, %2, [%3]\n"
		"	teq	%1, #0\n"
		"	bne	1b"
			: "=&r" (ret), "=&r" (tmp)
			: "r" (x), "r" (ptr)
			: "memory", "cc");
		break;
#elif defined(swp_is_buggy)
#ifdef CONFIG_SMP
#error SMP is not supported on this platform
#endif
	case 1:
		raw_local_irq_save(flags);
		ret = *(volatile unsigned char *)ptr;
		*(volatile unsigned char *)ptr = x;
		raw_local_irq_restore(flags);
		break;

	case 4:
		raw_local_irq_save(flags);
		ret = *(volatile unsigned long *)ptr;
		*(volatile unsigned long *)ptr = x;
		raw_local_irq_restore(flags);
		break;
#else
	case 1:
		asm volatile("@	__xchg1\n"
		"	swpb	%0, %1, [%2]"
			: "=&r" (ret)
			: "r" (x), "r" (ptr)
			: "memory", "cc");
		break;
	case 4:
		asm volatile("@	__xchg4\n"
		"	swp	%0, %1, [%2]"
			: "=&r" (ret)
			: "r" (x), "r" (ptr)
			: "memory", "cc");
		break;
#endif
	default:
		__bad_xchg(ptr, size), ret = 0;
		break;
	}
	smp_mb();

	return ret;
}

extern void disable_hlt(void);
extern void enable_hlt(void);

void cpu_idle_wait(void);

#include <asm-generic/cmpxchg-local.h>

#if __LINUX_ARM_ARCH__ < 6
 
#ifdef CONFIG_SMP
#error "SMP is not supported on this platform"
#endif

#define cmpxchg_local(ptr, o, n)				  	       \
	((__typeof__(*(ptr)))__cmpxchg_local_generic((ptr), (unsigned long)(o),\
			(unsigned long)(n), sizeof(*(ptr))))
#define cmpxchg64_local(ptr, o, n) __cmpxchg64_local_generic((ptr), (o), (n))

#ifndef CONFIG_SMP
#include <asm-generic/cmpxchg.h>
#endif

#else	 

extern void __bad_cmpxchg(volatile void *ptr, int size);

static inline unsigned long __cmpxchg(volatile void *ptr, unsigned long old,
				      unsigned long new, int size)
{
	unsigned long oldval, res;

	switch (size) {
#ifndef CONFIG_CPU_V6	 
	case 1:
		do {
			asm volatile("@ __cmpxchg1\n"
			"	ldrexb	%1, [%2]\n"
			"	mov	%0, #0\n"
			"	teq	%1, %3\n"
			"	strexbeq %0, %4, [%2]\n"
				: "=&r" (res), "=&r" (oldval)
				: "r" (ptr), "Ir" (old), "r" (new)
				: "memory", "cc");
		} while (res);
		break;
	case 2:
		do {
			asm volatile("@ __cmpxchg1\n"
			"	ldrexh	%1, [%2]\n"
			"	mov	%0, #0\n"
			"	teq	%1, %3\n"
			"	strexheq %0, %4, [%2]\n"
				: "=&r" (res), "=&r" (oldval)
				: "r" (ptr), "Ir" (old), "r" (new)
				: "memory", "cc");
		} while (res);
		break;
#endif
	case 4:
		do {
			asm volatile("@ __cmpxchg4\n"
			"	ldrex	%1, [%2]\n"
			"	mov	%0, #0\n"
			"	teq	%1, %3\n"
			"	strexeq %0, %4, [%2]\n"
				: "=&r" (res), "=&r" (oldval)
				: "r" (ptr), "Ir" (old), "r" (new)
				: "memory", "cc");
		} while (res);
		break;
	default:
		__bad_cmpxchg(ptr, size);
		oldval = 0;
	}

	return oldval;
}

static inline unsigned long __cmpxchg_mb(volatile void *ptr, unsigned long old,
					 unsigned long new, int size)
{
	unsigned long ret;

	smp_mb();
	ret = __cmpxchg(ptr, old, new, size);
	smp_mb();

	return ret;
}

#define cmpxchg(ptr,o,n)						\
	((__typeof__(*(ptr)))__cmpxchg_mb((ptr),			\
					  (unsigned long)(o),		\
					  (unsigned long)(n),		\
					  sizeof(*(ptr))))

static inline unsigned long __cmpxchg_local(volatile void *ptr,
					    unsigned long old,
					    unsigned long new, int size)
{
	unsigned long ret;

	switch (size) {
#ifdef CONFIG_CPU_V6	 
	case 1:
	case 2:
		ret = __cmpxchg_local_generic(ptr, old, new, size);
		break;
#endif
	default:
		ret = __cmpxchg(ptr, old, new, size);
	}

	return ret;
}

#define cmpxchg_local(ptr,o,n)						\
	((__typeof__(*(ptr)))__cmpxchg_local((ptr),			\
				       (unsigned long)(o),		\
				       (unsigned long)(n),		\
				       sizeof(*(ptr))))

#ifndef CONFIG_CPU_V6	 

static inline unsigned long long __cmpxchg64(volatile void *ptr,
					     unsigned long long old,
					     unsigned long long new)
{
	register unsigned long long oldval asm("r0");
	register unsigned long long __old asm("r2") = old;
	register unsigned long long __new asm("r4") = new;
	unsigned long res;

	do {
		asm volatile(
		"	@ __cmpxchg8\n"
		"	ldrexd	%1, %H1, [%2]\n"
		"	mov	%0, #0\n"
		"	teq	%1, %3\n"
		"	teqeq	%H1, %H3\n"
		"	strexdeq %0, %4, %H4, [%2]\n"
			: "=&r" (res), "=&r" (oldval)
			: "r" (ptr), "Ir" (__old), "r" (__new)
			: "memory", "cc");
	} while (res);

	return oldval;
}

static inline unsigned long long __cmpxchg64_mb(volatile void *ptr,
						unsigned long long old,
						unsigned long long new)
{
	unsigned long long ret;

	smp_mb();
	ret = __cmpxchg64(ptr, old, new);
	smp_mb();

	return ret;
}

#define cmpxchg64(ptr,o,n)						\
	((__typeof__(*(ptr)))__cmpxchg64_mb((ptr),			\
					    (unsigned long long)(o),	\
					    (unsigned long long)(n)))

#define cmpxchg64_local(ptr,o,n)					\
	((__typeof__(*(ptr)))__cmpxchg64((ptr),				\
					 (unsigned long long)(o),	\
					 (unsigned long long)(n)))

#else  

#define cmpxchg64_local(ptr, o, n) __cmpxchg64_local_generic((ptr), (o), (n))

#endif

#endif	 

#endif  

#define arch_align_stack(x) (x)

#endif  

#endif
