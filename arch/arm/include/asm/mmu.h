#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifndef __ARM_MMU_H
#define __ARM_MMU_H

#ifdef CONFIG_MMU

typedef struct {
#ifdef CONFIG_CPU_HAS_ASID
	atomic64_t	id;
#else
	int		switch_pending;
#endif
	unsigned int	vmalloc_seq;
	unsigned long	sigpage;
} mm_context_t;

#ifdef CONFIG_CPU_HAS_ASID
#define ASID_BITS	8
#define ASID_MASK	((~0ULL) << ASID_BITS)
#if defined(MY_ABC_HERE)
#define ASID(mm)	((unsigned int)((mm)->context.id.counter & ~ASID_MASK))
#else  
#define ASID(mm)	((mm)->context.id.counter & ~ASID_MASK)
#endif  
#else
#define ASID(mm)	(0)
#endif

#else

typedef struct {
	unsigned long	end_brk;
} mm_context_t;

#endif

#endif
