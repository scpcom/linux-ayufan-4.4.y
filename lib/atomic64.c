#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/types.h>
#include <linux/cache.h>
#include <linux/spinlock.h>
#include <linux/init.h>
#include <linux/module.h>
#include <asm/atomic.h>

#ifdef MY_ABC_HERE
#include <asm-generic/atomic64.h>
#endif

#define NR_LOCKS	16

static union {
	spinlock_t lock;
	char pad[L1_CACHE_BYTES];
} atomic64_lock[NR_LOCKS] __cacheline_aligned_in_smp;

static inline spinlock_t *lock_addr(const atomic64_t *v)
{
	unsigned long addr = (unsigned long) v;

	addr >>= L1_CACHE_SHIFT;
	addr ^= (addr >> 8) ^ (addr >> 16);
	return &atomic64_lock[addr & (NR_LOCKS - 1)].lock;
}

long long atomic64_read(const atomic64_t *v)
{
	unsigned long flags;
	spinlock_t *lock = lock_addr(v);
	long long val;

	spin_lock_irqsave(lock, flags);
	val = v->counter;
	spin_unlock_irqrestore(lock, flags);
	return val;
}
EXPORT_SYMBOL(atomic64_read);

void atomic64_set(atomic64_t *v, long long i)
{
	unsigned long flags;
	spinlock_t *lock = lock_addr(v);

	spin_lock_irqsave(lock, flags);
	v->counter = i;
	spin_unlock_irqrestore(lock, flags);
}
EXPORT_SYMBOL(atomic64_set);

void atomic64_add(long long a, atomic64_t *v)
{
	unsigned long flags;
	spinlock_t *lock = lock_addr(v);

	spin_lock_irqsave(lock, flags);
	v->counter += a;
	spin_unlock_irqrestore(lock, flags);
}
EXPORT_SYMBOL(atomic64_add);

long long atomic64_add_return(long long a, atomic64_t *v)
{
	unsigned long flags;
	spinlock_t *lock = lock_addr(v);
	long long val;

	spin_lock_irqsave(lock, flags);
	val = v->counter += a;
	spin_unlock_irqrestore(lock, flags);
	return val;
}
EXPORT_SYMBOL(atomic64_add_return);

void atomic64_sub(long long a, atomic64_t *v)
{
	unsigned long flags;
	spinlock_t *lock = lock_addr(v);

	spin_lock_irqsave(lock, flags);
	v->counter -= a;
	spin_unlock_irqrestore(lock, flags);
}
EXPORT_SYMBOL(atomic64_sub);

long long atomic64_sub_return(long long a, atomic64_t *v)
{
	unsigned long flags;
	spinlock_t *lock = lock_addr(v);
	long long val;

	spin_lock_irqsave(lock, flags);
	val = v->counter -= a;
	spin_unlock_irqrestore(lock, flags);
	return val;
}
EXPORT_SYMBOL(atomic64_sub_return);

long long atomic64_dec_if_positive(atomic64_t *v)
{
	unsigned long flags;
	spinlock_t *lock = lock_addr(v);
	long long val;

	spin_lock_irqsave(lock, flags);
	val = v->counter - 1;
	if (val >= 0)
		v->counter = val;
	spin_unlock_irqrestore(lock, flags);
	return val;
}
EXPORT_SYMBOL(atomic64_dec_if_positive);

long long atomic64_cmpxchg(atomic64_t *v, long long o, long long n)
{
	unsigned long flags;
	spinlock_t *lock = lock_addr(v);
	long long val;

	spin_lock_irqsave(lock, flags);
	val = v->counter;
	if (val == o)
		v->counter = n;
	spin_unlock_irqrestore(lock, flags);
	return val;
}
EXPORT_SYMBOL(atomic64_cmpxchg);

long long atomic64_xchg(atomic64_t *v, long long new)
{
	unsigned long flags;
	spinlock_t *lock = lock_addr(v);
	long long val;

	spin_lock_irqsave(lock, flags);
	val = v->counter;
	v->counter = new;
	spin_unlock_irqrestore(lock, flags);
	return val;
}
EXPORT_SYMBOL(atomic64_xchg);

int atomic64_add_unless(atomic64_t *v, long long a, long long u)
{
	unsigned long flags;
	spinlock_t *lock = lock_addr(v);
	int ret = 1;

	spin_lock_irqsave(lock, flags);
	if (v->counter != u) {
		v->counter += a;
		ret = 0;
	}
	spin_unlock_irqrestore(lock, flags);
	return ret;
}
EXPORT_SYMBOL(atomic64_add_unless);

static int init_atomic64_lock(void)
{
	int i;

	for (i = 0; i < NR_LOCKS; ++i)
		spin_lock_init(&atomic64_lock[i].lock);
	return 0;
}

pure_initcall(init_atomic64_lock);
