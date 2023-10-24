/*
 *  arch/arm/mach-comcerto/msp/smi.h
 *
 *  Copyright (C) 2012 Mindspeed Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _COMCERTO_SMI_H
#define _COMCERTO_SMI_H

#include <linux/types.h>
#include <linux/spinlock.h>

#include <asm/io.h>
#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/sema.h>

#ifndef COMCERTO_IRQ_SET
#define DISTR_INT_SET_PENDING (COMCERTO_GIC_DIST_BASE + 0x200)
#define DISTR_CLEAR_PENDING (COMCERTO_GIC_DIST_BASE + 0x280)
#define CPU_END_OF_INT (COMCERTO_GIC_CPU_BASE + 0x10)
#define comcerto_softirq_set(irq)	  \
	(*((volatile unsigned long *)DISTR_INT_SET_PENDING + ((irq) >> 5)) = 1 << ((irq) & 0x1f))
#define comcerto_softirq_check(irq)	  \
	((*((volatile unsigned long *)DISTR_INT_SET_PENDING + ((irq) >> 5)) >> ((irq) & 0x1f)) & 0x01)
#define comcerto_irq_ack(irq)	  \
	(* ((volatile unsigned long *) DISTR_CLEAR_PENDING + ((irq) >> 5)) = 1 << ((irq) % 32))

//#define comcerto_irq_ack(irq) (*(volatile unsigned long *)(CPU_END_OF_INT) = irq)

#endif  /* C2000 */

/* shared structure (MSP uses the same type) */
struct fastqueue {
	u32 *storage;
	u32 sema;
	volatile u16 lock;
	volatile u16 get;
	volatile u16 put;
	u16 size;
};

/* private structure (CSP only) */
struct smiqueue {
	u32 *storage;			  /* virtual address of fpq->storage */
	struct fastqueue *fpq;	  /* queue pointer */
	void (*gen_msp_irq)(void); /* CSP to MSP generate interrupt function */
};

struct fastpart {
        volatile u8 lock;
        u8 reserved1;
        u32 * volatile freeblk;
        u32 *storage;
        u32 blksz;
        u32 blkcnt;
        u32 *end_of_storage;
        u16 reserved2;
        u16 reserved3;
        u16 freecnt;
};

void *smi_alloc_part(struct fastpart *fp);
void smi_free_part(struct fastpart *fp, void *v);


struct fastqueue *smi_queue_init(struct smiqueue *psmiq, unsigned long addr, void (*gen_msp_irq)(void));

static inline void smi_gen_msp_irq(void)
{
	comcerto_softirq_set(IRQ_PTP1);
}

static inline int smi_enqueue(struct smiqueue *psmiq, void *vaddr)
{
	struct fastqueue *fpq = psmiq->fpq;
	unsigned long flags = 0;
	u16 put = 0;
	int rc = 0;

	flags = msp_lock_frqsave();

	put = fpq->put;

	if (++put >= fpq->size) {
		put = 0;
	}

	if (put != fpq->get) {
		psmiq->storage[fpq->put] = (u32)vaddr;
		fpq->put = put;

		if (psmiq->gen_msp_irq) {
			psmiq->gen_msp_irq();
		}

		rc = 1;
	}

	msp_unlock_frqrestore(flags);

	return rc;
}

static inline void *smi_dequeue(struct smiqueue *psmiq)
{
	struct fastqueue *fpq = psmiq->fpq;
	unsigned long flags = 0;
	u32 get = 0;
	void *vaddr = NULL;

	flags = msp_lock_frqsave();

	get = fpq->get;

	if (fpq->put != get) {
		vaddr = (void *)(psmiq->storage[get++]);

		if (get == fpq->size) {
			get = 0;
		}

		fpq->get = get;
	}

	msp_unlock_frqrestore(flags);

	return vaddr;
}

static inline int smi_is_queue_empty(struct fastqueue *fpq)
{
	return (fpq->get == fpq->put);
}

#endif /* _COMCERTO_SMI_H */
