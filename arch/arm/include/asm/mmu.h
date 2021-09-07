#ifndef __ARM_MMU_H
#define __ARM_MMU_H

#ifdef CONFIG_MMU

typedef struct {
#ifdef CONFIG_CPU_HAS_ASID
	unsigned int id;
#ifdef CONFIG_SYNO_PLX_PORTING
	spinlock_t id_lock;
#endif
#endif
	unsigned int kvm_seq;
} mm_context_t;

#ifdef CONFIG_CPU_HAS_ASID
#define ASID(mm)	((mm)->context.id & 255)
#else
#define ASID(mm)	(0)
#endif

#else

typedef struct {
	unsigned long		end_brk;
} mm_context_t;

#endif

#endif
