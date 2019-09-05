#include <linux/spinlock.h>
#include <linux/export.h>
#include <asm/atomic.h>

static DEFINE_RAW_SPINLOCK(atomic_lock);

struct virtual_zone arm_dma_zone; /* initialized in arch/arm/mm/mmu.c::map_lowmem() */
EXPORT_SYMBOL(arm_dma_zone);

int comcerto_atomic_add(int i, atomic_t *v)
{
	unsigned long flags;
	int result;

	raw_spin_lock_irqsave(&atomic_lock, flags);
	v->counter += i;
	result = v->counter;
	raw_spin_unlock_irqrestore(&atomic_lock, flags);

	return result;
}
EXPORT_SYMBOL(comcerto_atomic_add);

int comcerto_atomic_add_unless(atomic_t *v, int a, int u)
{
	unsigned long flags;
	int result;

	raw_spin_lock_irqsave(&atomic_lock, flags);
	result = v->counter;
	if (likely(result != u))
	{
		v->counter += a;
		result = v->counter;
	}
	raw_spin_unlock_irqrestore(&atomic_lock, flags);

	return result;
}
EXPORT_SYMBOL(comcerto_atomic_add_unless);

int comcerto_atomic_cmpxchg(atomic_t *v, int old, int new)
{
	unsigned long flags;
	int result;

	raw_spin_lock_irqsave(&atomic_lock, flags);
	result = v->counter;
	if (likely(result == old))
		v->counter = new;
	raw_spin_unlock_irqrestore(&atomic_lock, flags);

	return result;
}
EXPORT_SYMBOL(comcerto_atomic_cmpxchg);

void comcerto_atomic_clear_mask(unsigned long mask, unsigned long *addr)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&atomic_lock, flags);
	*addr &= ~mask;
	raw_spin_unlock_irqrestore(&atomic_lock, flags);
}
EXPORT_SYMBOL(comcerto_atomic_clear_mask);

