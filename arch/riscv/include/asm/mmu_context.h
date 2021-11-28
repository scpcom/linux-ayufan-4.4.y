/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 * Copyright (C) 2017 SiFive
 */

#ifndef _ASM_RISCV_MMU_CONTEXT_H
#define _ASM_RISCV_MMU_CONTEXT_H

#include <linux/mm_types.h>
#include <asm-generic/mm_hooks.h>

#include <linux/mm.h>
#include <linux/sched.h>
#include <asm/tlbflush.h>
#include <asm/cacheflush.h>
#include <asm/asid.h>

#define ASID_MASK		((1 << SATP_ASID_BITS) - 1)
#define cpu_asid(mm)		(atomic64_read(&mm->context.asid) & ASID_MASK)

void switch_mm(struct mm_struct *prev, struct mm_struct *next,
	struct task_struct *task);

#ifdef CONFIG_RISCV_XUANTIE
void check_and_switch_context(struct mm_struct *mm, unsigned int cpu);
#endif

#define activate_mm activate_mm
static inline void activate_mm(struct mm_struct *prev,
			       struct mm_struct *next)
{
	switch_mm(prev, next, NULL);
}

#define init_new_context init_new_context
static inline int init_new_context(struct task_struct *tsk,
			struct mm_struct *mm)
{
#ifdef CONFIG_MMU
#ifdef CONFIG_RISCV_XUANTIE
	atomic64_set(&(mm)->context.asid, 0);
#else
	atomic_long_set(&mm->context.id, 0);
#endif
#endif
	return 0;
}

#ifndef CONFIG_RISCV_XUANTIE
DECLARE_STATIC_KEY_FALSE(use_asid_allocator);
#endif

#include <asm-generic/mmu_context.h>

#endif /* _ASM_RISCV_MMU_CONTEXT_H */
