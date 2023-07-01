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

#include <linux/kernel.h>
#include <asm/io.h>

#include "smi.h"
#include "msp.h"

void *smi_alloc_part(struct fastpart *fp)
{
	unsigned long flags = 0;
	void *vaddr = NULL;

	flags = msp_lock_frqsave();

	if (fp->freeblk) {
		vaddr = fp->freeblk;
		fp->freeblk = (u32 * volatile)__raw_readl(vaddr);
	}

	msp_unlock_frqrestore(flags);

	return vaddr;
}

void smi_free_part(struct fastpart *fp, void *vaddr)
{
	unsigned long flags = 0;

	flags = msp_lock_frqsave();

	__raw_writel((unsigned long)fp->freeblk, (unsigned long)vaddr);
	fp->freeblk = vaddr;

	msp_unlock_frqrestore(flags);
}

struct fastqueue *smi_queue_init(struct smiqueue *psmiq, unsigned long addr, void (*gen_msp_irq)(void))
{
        struct fastqueue *fpq = (struct fastqueue *)addr;

        if (fpq) {
                psmiq->fpq = fpq;
                psmiq->storage = fpq->storage;
                psmiq->gen_msp_irq = gen_msp_irq;
        }

        return fpq;
}
