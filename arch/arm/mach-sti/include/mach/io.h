#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __ASM_ARM_ARCH_IO_H
#define __ASM_ARM_ARCH_IO_H

#define IO_SPACE_LIMIT ((resource_size_t)0)

#define __io(a)		__typesafe_io((a) & IO_SPACE_LIMIT)

#ifdef MY_DEF_HERE
#else  
#ifdef CONFIG_STM_PCIE_TRACKER_BUG

#include <linux/spinlock.h>

extern spinlock_t stm_pcie_io_spinlock;

#define __STM_FROB_BIT (1 << 31)

static inline void __iomem *__stm_frob(const volatile void __iomem *virt)
{
	return (void __iomem *) ((unsigned long) virt & ~__STM_FROB_BIT);
}

static inline void __iomem *__stm_unfrob(const volatile void __iomem *virt)
{
	return (void __iomem *) ((unsigned long) virt | __STM_FROB_BIT);
}

static inline int __stm_is_frobbed(const volatile void __iomem *virt)
{
	return !((unsigned long) virt & __STM_FROB_BIT);
}

#define __readb_relaxed(c) ({ u8  __r = __raw_readb(c); __r; })
#define __readw_relaxed(c) ({ u16 __r = le16_to_cpu((__force __le16) \
					__raw_readw(c)); __r; })
#define __readl_relaxed(c) ({ u32 __r = le32_to_cpu((__force __le32) \
					__raw_readl(c)); __r; })

#define __stm_lock_read_relaxed_fn(__type, __ext) \
static inline __type __stm_lock_read##__ext##_relaxed(const volatile void __iomem *addr)\
{									\
	__type  v;							\
	unsigned long flags;						\
									\
	if (!__stm_is_frobbed(addr)) {					\
		v = __read##__ext##_relaxed(addr);			\
	} else {							\
		addr = __stm_unfrob(addr);				\
		spin_lock_irqsave(&stm_pcie_io_spinlock, flags);	\
		v = __read##__ext##_relaxed(addr);			\
		__iormb();						\
		spin_unlock_irqrestore(&stm_pcie_io_spinlock, flags);	\
	}								\
	return v;							\
}

__stm_lock_read_relaxed_fn(u8, b)
__stm_lock_read_relaxed_fn(u16, w)
__stm_lock_read_relaxed_fn(u32, l)

#define readb_relaxed(c) __stm_lock_readb_relaxed(c)
#define readw_relaxed(c) __stm_lock_readw_relaxed(c)
#define readl_relaxed(c) __stm_lock_readl_relaxed(c)

#define __writeb_relaxed(v, c)	((void)__raw_writeb(v, c))
#define __writew_relaxed(v, c)	((void)__raw_writew((__force u16) \
					cpu_to_le16(v), c))
#define __writel_relaxed(v, c)	((void)__raw_writel((__force u32) \
					cpu_to_le32(v), c))

#define __stm_lock_write_relaxed_fn(__type, __ext)				\
static inline void __stm_lock_write##__ext##_relaxed(__type v,			\
					   volatile void __iomem *addr) \
{									\
	unsigned long flags;						\
									\
	if (!__stm_is_frobbed(addr)) {					\
		__write##__ext##_relaxed(v, addr);			\
	} else {							\
		__iowmb();						\
		addr = __stm_unfrob((void *)addr);			\
		spin_lock_irqsave(&stm_pcie_io_spinlock, flags);	\
		__write##__ext##_relaxed(v, addr);			\
		__iowmb();  	\
		spin_unlock_irqrestore(&stm_pcie_io_spinlock, flags);	\
	}								\
}

__stm_lock_write_relaxed_fn(u8, b)
__stm_lock_write_relaxed_fn(u16, w)
__stm_lock_write_relaxed_fn(u32, l)

#define writeb_relaxed(v, c) __stm_lock_writeb_relaxed(v, c)
#define writew_relaxed(v, c) __stm_lock_writew_relaxed(v, c)
#define writel_relaxed(v, c) __stm_lock_writel_relaxed(v, c)

#define __stm_lock_read_fn(__type, __ext) \
static inline __type __stm_lock_read##__ext(const volatile void __iomem *addr)\
{									\
	__type  v;							\
	unsigned long flags;						\
									\
	if (!__stm_is_frobbed(addr)) {					\
		v = __read##__ext##_relaxed(addr);			\
	} else {							\
		addr = __stm_unfrob(addr);				\
		spin_lock_irqsave(&stm_pcie_io_spinlock, flags);	\
		v = __read##__ext##_relaxed(addr);			\
		__iormb();						\
		spin_unlock_irqrestore(&stm_pcie_io_spinlock, flags);	\
	}								\
	return v;							\
}

__stm_lock_read_fn(u8, b)
__stm_lock_read_fn(u16, w)
__stm_lock_read_fn(u32, l)

#define readw(c) __stm_lock_readw(c)
#define readb(c) __stm_lock_readb(c)
#define readl(c) __stm_lock_readl(c)

#define __stm_lock_write_fn(__type, __ext)				\
static inline void __stm_lock_write##__ext(__type v,			\
					   volatile void __iomem *addr) \
{									\
	unsigned long flags;						\
									\
	__iowmb();							\
	if (!__stm_is_frobbed(addr)) {					\
		__write##__ext##_relaxed(v, addr);			\
	} else {							\
		addr = __stm_unfrob((void *)addr);			\
		spin_lock_irqsave(&stm_pcie_io_spinlock, flags);	\
		__write##__ext##_relaxed(v, addr);			\
		__iowmb();  	\
		spin_unlock_irqrestore(&stm_pcie_io_spinlock, flags);	\
	}								\
}

__stm_lock_write_fn(u8, b)
__stm_lock_write_fn(u16, w)
__stm_lock_write_fn(u32, l)

#define writeb(v, c) __stm_lock_writeb(v, c)
#define writew(v, c) __stm_lock_writew(v, c)
#define writel(v, c) __stm_lock_writel(v, c)

#define __stm_lock_reads_fn(__ext)					\
static inline void __stm_lock_reads##__ext(const void __iomem *addr,	\
					   void *buf, int len)		\
{									\
	unsigned long flags;						\
									\
	if (!__stm_is_frobbed(addr)) {					\
		__raw_reads##__ext(addr, buf, len);			\
	} else {							\
		addr = __stm_unfrob((void *)addr);			\
		spin_lock_irqsave(&stm_pcie_io_spinlock, flags);	\
		__raw_reads##__ext(addr, buf, len);			\
		__iormb();						\
		spin_unlock_irqrestore(&stm_pcie_io_spinlock, flags);	\
	}								\
}

__stm_lock_reads_fn(b)
__stm_lock_reads_fn(w)
__stm_lock_reads_fn(l)

#define readsb(p, d, l) __stm_lock_readsb(p, d, l)
#define readsw(p, d, l) __stm_lock_readsw(p, d, l)
#define readsl(p, d, l) __stm_lock_readsl(p, d, l)

#define __stm_lock_writes_fn(__ext) \
static inline void __stm_lock_writes##__ext(void __iomem *addr,		\
					    const void *buf, int len)	\
{									\
	unsigned long flags;						\
									\
	if (!__stm_is_frobbed(addr)) {					\
		__raw_writes##__ext(addr, buf, len);			\
	} else {							\
		addr = __stm_unfrob((void *)addr);			\
		spin_lock_irqsave(&stm_pcie_io_spinlock, flags);	\
		__raw_writes##__ext(addr, buf, len);			\
		__iowmb();						\
		spin_unlock_irqrestore(&stm_pcie_io_spinlock, flags);	\
	}								\
}

__stm_lock_writes_fn(b)
__stm_lock_writes_fn(w)
__stm_lock_writes_fn(l)

#define writesb(p, d, l) __stm_lock_writesb(p, d, l)
#define writesw(p, d, l) __stm_lock_writesw(p, d, l)
#define writesl(p, d, l) __stm_lock_writesl(p, d, l)

#define memset_io(c, v, l)	_memset_io(c, (v), (l))
#define memcpy_fromio(a, c, l)	_memcpy_fromio((a), c, (l))
#define memcpy_toio(c, a, l)	_memcpy_toio(c, (a), (l))

#define ioread8(p)	readb(p)
#define ioread16(p)	readw(p)
#define ioread32(p)	readl(p)

#define ioread16be(p)	be16_to_cpu(readw(p))
#define ioread32be(p)	be32_to_cpu(readl(p))

#define iowrite8(v, p)	writeb(v, p)
#define iowrite16(v, p) writew(v, p)
#define iowrite32(v, p)	writel(v, p)

#define iowrite16be(v, p) writew(cpu_to_be16(v), p)
#define iowrite32be(v, p) writel(cpu_to_be32(v), p)

#define ioread8_rep(p, d, c)	readsb(p, d, c)
#define ioread16_rep(p, d, c)	readsw(p, d, c)
#define ioread32_rep(p, d, c)	readsl(p, d, c)

#define iowrite8_rep(p, s, c)	writesb(p, s, c)
#define iowrite16_rep(p, s, c)	writesw(p, s, c)
#define iowrite32_rep(p, s, c)	writesl(p, s, c)

extern void __iomem *ioport_map(unsigned long port, unsigned int nr);
extern void ioport_unmap(void __iomem *addr);

#endif   
#endif  

#endif
