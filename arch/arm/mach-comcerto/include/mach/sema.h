/*
 *  arch/arm/mach-comcerto/include/mach/sema.h
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

#ifndef __ASM_ARCH_SEMA_H__
#define __ASM_ARCH_SEMA_H__

#ifdef CONFIG_COMCERTO_MSP

#define COMCERTO_SEMA_COUNT (64)

enum {
	COMCERTO_SEMA_MSP,
};

/* get status and disable FIQ & IRQ */
static inline unsigned long arch_local_frq_save(void)
{
	unsigned long flags;

	asm volatile (
		"mrs %0, cpsr @ arch_local_frq_save\n"
		"cpsid if"
		: "=r" (flags) : : "memory", "cc");

	return flags;
}

#define local_frq_save() arch_local_frq_save()
#define local_frq_restore(flags) local_irq_restore(flags)

/**
 * comcerto_sema_lock()
 * Spin to lock. Always succeeds.
 *
 * In order to acquire the semaphore, the processor needs to read the
 * semaphore value. If zero, the processor acquired the semaphore and
 * HW will set it to one, to mark that the semaphore is taken, if one
 * is read, the semaphore is already taken by another processor and
 * the current processor needs to keep polling the semaphore until
 * value of zero is received.
 *
 * C2K has COMCERTO_SEMA_COUNT=64, 1-bit semaphores.
 */
static inline void comcerto_sema_lock(volatile unsigned long lock_id)
{
	volatile unsigned long *p = (unsigned long *)COMCERTO_SEMA_VADDR;
	unsigned long x;

	if (lock_id >= COMCERTO_SEMA_COUNT) {
		BUG();
	}

	p += lock_id;
	x = *p;

	if (x) {
		while (*p) {
			/* wait for lock */
			;
		}
	}
}

/**
 * comcerto_sema_unlock()
 * Unlock. Always succeeds.
 *
 * After acquiring a semaphore the processor needs to write zero to it
 * in order to release it.
 */
static inline void comcerto_sema_unlock(volatile unsigned long lock_id)
{
	volatile unsigned long *p = (unsigned long *)COMCERTO_SEMA_VADDR;

	if (lock_id >= COMCERTO_SEMA_COUNT) {
		BUG();
	}

	p += lock_id;
	*p = 0;
}

static inline unsigned long msp_lock_frqsave(void)
{
	unsigned long flags;

	flags = local_frq_save();
	comcerto_sema_lock(COMCERTO_SEMA_MSP);

	return flags;
}

static inline void msp_unlock_frqrestore(unsigned long flags)
{
	comcerto_sema_unlock(COMCERTO_SEMA_MSP);
	local_frq_restore(flags);
}

#else  /* !CONFIG_COMCERTO_MSP */
static inline unsigned long msp_lock_frqsave(void)
{
	return 0;
}

static inline void msp_unlock_frqrestore(unsigned long flags)
{
	return;
}
#endif /* CONFIG_COMCERTO_MSP */

#endif  /* __ASM_ARCH_SEMA_H__ */
