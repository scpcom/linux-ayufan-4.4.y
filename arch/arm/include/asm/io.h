#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __ASM_ARM_IO_H
#define __ASM_ARM_IO_H

#ifdef __KERNEL__

#include <linux/types.h>
#include <asm/byteorder.h>
#include <asm/memory.h>
#include <asm/system.h>

#define isa_virt_to_bus virt_to_phys
#define isa_page_to_bus page_to_phys
#define isa_bus_to_virt phys_to_virt

extern void __raw_writesb(void __iomem *addr, const void *data, int bytelen);
extern void __raw_writesw(void __iomem *addr, const void *data, int wordlen);
extern void __raw_writesl(void __iomem *addr, const void *data, int longlen);

extern void __raw_readsb(const void __iomem *addr, void *data, int bytelen);
extern void __raw_readsw(const void __iomem *addr, void *data, int wordlen);
extern void __raw_readsl(const void __iomem *addr, void *data, int longlen);

#define __raw_writeb(v,a)	(__chk_io_ptr(a), *(volatile unsigned char __force  *)(a) = (v))
#define __raw_writew(v,a)	(__chk_io_ptr(a), *(volatile unsigned short __force *)(a) = (v))
#define __raw_writel(v,a)	(__chk_io_ptr(a), *(volatile unsigned int __force   *)(a) = (v))

#define __raw_readb(a)		(__chk_io_ptr(a), *(volatile unsigned char __force  *)(a))
#define __raw_readw(a)		(__chk_io_ptr(a), *(volatile unsigned short __force *)(a))
#define __raw_readl(a)		(__chk_io_ptr(a), *(volatile unsigned int __force   *)(a))

#define MT_DEVICE		0
#define MT_DEVICE_NONSHARED	1
#define MT_DEVICE_CACHED	2
#define MT_DEVICE_WC		3
 
extern void __iomem *__arm_ioremap_pfn_caller(unsigned long, unsigned long,
	size_t, unsigned int, void *);
extern void __iomem *__arm_ioremap_caller(unsigned long, size_t, unsigned int,
	void *);

extern void __iomem *__arm_ioremap_pfn(unsigned long, unsigned long, size_t, unsigned int);
extern void __iomem *__arm_ioremap(unsigned long, size_t, unsigned int);
extern void __iomem *__arm_ioremap_exec(unsigned long, size_t, bool cached);
extern void __iounmap(volatile void __iomem *addr);

extern void __readwrite_bug(const char *fn);

static inline void __iomem *__typesafe_io(unsigned long addr)
{
	return (void __iomem *)addr;
}

#ifdef CONFIG_ARM_DMA_MEM_BUFFERABLE
#define __iormb()		rmb()
#define __iowmb()		wmb()
#else
#define __iormb()		do { } while (0)
#define __iowmb()		do { } while (0)
#endif
#ifdef CONFIG_SYNO_ALPINE
#define PCI_IO_VIRT_BASE        0xfee00000

struct resource;
struct device;

extern int pci_ioremap_io(unsigned int offset, phys_addr_t phys_addr);
extern void __iomem *devm_request_and_ioremap(struct device *dev,
                         struct resource *res);
#endif

#include <mach/io.h>

#ifndef IO_SPACE_LIMIT
#if defined(CONFIG_PCMCIA_SOC_COMMON) || defined(CONFIG_PCMCIA_SOC_COMMON_MODULE)
#define IO_SPACE_LIMIT ((resource_size_t)0xffffffff)
#elif defined(CONFIG_PCI) || defined(CONFIG_ISA) || defined(CONFIG_PCCARD)
#define IO_SPACE_LIMIT ((resource_size_t)0xffff)
#else
#define IO_SPACE_LIMIT ((resource_size_t)0)
#endif
#endif

#ifdef __io
#define outb(v,p)	({ __iowmb(); __raw_writeb(v,__io(p)); })
#define outw(v,p)	({ __iowmb(); __raw_writew((__force __u16) \
					cpu_to_le16(v),__io(p)); })
#define outl(v,p)	({ __iowmb(); __raw_writel((__force __u32) \
					cpu_to_le32(v),__io(p)); })

#define inb(p)	({ __u8 __v = __raw_readb(__io(p)); __iormb(); __v; })
#define inw(p)	({ __u16 __v = le16_to_cpu((__force __le16) \
			__raw_readw(__io(p))); __iormb(); __v; })
#define inl(p)	({ __u32 __v = le32_to_cpu((__force __le32) \
			__raw_readl(__io(p))); __iormb(); __v; })

#define outsb(p,d,l)		__raw_writesb(__io(p),d,l)
#define outsw(p,d,l)		__raw_writesw(__io(p),d,l)
#define outsl(p,d,l)		__raw_writesl(__io(p),d,l)

#define insb(p,d,l)		__raw_readsb(__io(p),d,l)
#define insw(p,d,l)		__raw_readsw(__io(p),d,l)
#define insl(p,d,l)		__raw_readsl(__io(p),d,l)
#endif

#define outb_p(val,port)	outb((val),(port))
#define outw_p(val,port)	outw((val),(port))
#define outl_p(val,port)	outl((val),(port))
#define inb_p(port)		inb((port))
#define inw_p(port)		inw((port))
#define inl_p(port)		inl((port))

#define outsb_p(port,from,len)	outsb(port,from,len)
#define outsw_p(port,from,len)	outsw(port,from,len)
#define outsl_p(port,from,len)	outsl(port,from,len)
#define insb_p(port,to,len)	insb(port,to,len)
#define insw_p(port,to,len)	insw(port,to,len)
#define insl_p(port,to,len)	insl(port,to,len)

extern void _memcpy_fromio(void *, const volatile void __iomem *, size_t);
extern void _memcpy_toio(volatile void __iomem *, const void *, size_t);
extern void _memset_io(volatile void __iomem *, int, size_t);

#define mmiowb()

#ifdef __mem_pci
#if defined(AL_PCIE_RMN_1010) && defined(CONFIG_SYNO_ALPINE_V2_5_3)
uint32_t al_dma_read_reg32(const volatile void __iomem *address);
uint16_t al_dma_read_reg16(const volatile void __iomem *address);
uint8_t al_dma_read_reg8(const volatile void __iomem *address);

#define readb_relaxed(c)	(al_dma_read_reg8(c))
#define readw_relaxed(c)	(al_dma_read_reg16(c))
#define readl_relaxed(c)	(al_dma_read_reg32(c))

#else
#define readb_relaxed(c) ({ u8  __r = __raw_readb(__mem_pci(c)); __r; })
#define readw_relaxed(c) ({ u16 __r = le16_to_cpu((__force __le16) \
					__raw_readw(__mem_pci(c))); __r; })
#define readl_relaxed(c) ({ u32 __r = le32_to_cpu((__force __le32) \
					__raw_readl(__mem_pci(c))); __r; })
#endif

#define writeb_relaxed(v,c)	((void)__raw_writeb(v,__mem_pci(c)))
#define writew_relaxed(v,c)	((void)__raw_writew((__force u16) \
					cpu_to_le16(v),__mem_pci(c)))

#if defined(AL_PCIE_RMN_1010) && defined(CONFIG_SYNO_ALPINE_V2_5_3)
void al_dma_write_reg32(volatile void __iomem *address, u32 val);
#define writel_relaxed(v,c)	(al_dma_write_reg32(c,v))
#else
#define writel_relaxed(v,c)	((void)__raw_writel((__force u32) \
					cpu_to_le32(v),__mem_pci(c)))
#endif

#define readb(c)		({ u8  __v = readb_relaxed(c); __iormb(); __v; })
#define readw(c)		({ u16 __v = readw_relaxed(c); __iormb(); __v; })
#define readl(c)		({ u32 __v = readl_relaxed(c); __iormb(); __v; })

#define writeb(v,c)		({ __iowmb(); writeb_relaxed(v,c); })
#define writew(v,c)		({ __iowmb(); writew_relaxed(v,c); })
#define writel(v,c)		({ __iowmb(); writel_relaxed(v,c); })

#define readsb(p,d,l)		__raw_readsb(__mem_pci(p),d,l)
#define readsw(p,d,l)		__raw_readsw(__mem_pci(p),d,l)
#define readsl(p,d,l)		__raw_readsl(__mem_pci(p),d,l)

#define writesb(p,d,l)		__raw_writesb(__mem_pci(p),d,l)
#define writesw(p,d,l)		__raw_writesw(__mem_pci(p),d,l)
#define writesl(p,d,l)		__raw_writesl(__mem_pci(p),d,l)

#define memset_io(c,v,l)	_memset_io(__mem_pci(c),(v),(l))
#define memcpy_fromio(a,c,l)	_memcpy_fromio((a),__mem_pci(c),(l))
#define memcpy_toio(c,a,l)	_memcpy_toio(__mem_pci(c),(a),(l))

#elif !defined(readb)

#define readb(c)			(__readwrite_bug("readb"),0)
#define readw(c)			(__readwrite_bug("readw"),0)
#define readl(c)			(__readwrite_bug("readl"),0)
#define writeb(v,c)			__readwrite_bug("writeb")
#define writew(v,c)			__readwrite_bug("writew")
#define writel(v,c)			__readwrite_bug("writel")

#define check_signature(io,sig,len)	(0)

#endif	 

#ifndef __arch_ioremap
#define __arch_ioremap			__arm_ioremap
#define __arch_iounmap			__iounmap
#endif

#define ioremap(cookie,size)		__arch_ioremap((cookie), (size), MT_DEVICE)
#define ioremap_nocache(cookie,size)	__arch_ioremap((cookie), (size), MT_DEVICE)
#define ioremap_cached(cookie,size)	__arch_ioremap((cookie), (size), MT_DEVICE_CACHED)
#define ioremap_wc(cookie,size)		__arch_ioremap((cookie), (size), MT_DEVICE_WC)
#define iounmap				__arch_iounmap

#ifndef ioread8
#define ioread8(p)	({ unsigned int __v = __raw_readb(p); __iormb(); __v; })
#define ioread16(p)	({ unsigned int __v = le16_to_cpu((__force __le16)__raw_readw(p)); __iormb(); __v; })
#define ioread32(p)	({ unsigned int __v = le32_to_cpu((__force __le32)__raw_readl(p)); __iormb(); __v; })

#define ioread16be(p)	({ unsigned int __v = be16_to_cpu((__force __be16)__raw_readw(p)); __iormb(); __v; })
#define ioread32be(p)	({ unsigned int __v = be32_to_cpu((__force __be32)__raw_readl(p)); __iormb(); __v; })

#define iowrite8(v,p)	({ __iowmb(); (void)__raw_writeb(v, p); })
#define iowrite16(v,p)	({ __iowmb(); (void)__raw_writew((__force __u16)cpu_to_le16(v), p); })
#define iowrite32(v,p)	({ __iowmb(); (void)__raw_writel((__force __u32)cpu_to_le32(v), p); })

#define iowrite16be(v,p) ({ __iowmb(); (void)__raw_writew((__force __u16)cpu_to_be16(v), p); })
#define iowrite32be(v,p) ({ __iowmb(); (void)__raw_writel((__force __u32)cpu_to_be32(v), p); })

#define ioread8_rep(p,d,c)	__raw_readsb(p,d,c)
#define ioread16_rep(p,d,c)	__raw_readsw(p,d,c)
#define ioread32_rep(p,d,c)	__raw_readsl(p,d,c)

#define iowrite8_rep(p,s,c)	__raw_writesb(p,s,c)
#define iowrite16_rep(p,s,c)	__raw_writesw(p,s,c)
#define iowrite32_rep(p,s,c)	__raw_writesl(p,s,c)

extern void __iomem *ioport_map(unsigned long port, unsigned int nr);
extern void ioport_unmap(void __iomem *addr);
#endif

struct pci_dev;

extern void __iomem *pci_iomap(struct pci_dev *dev, int bar, unsigned long maxlen);
extern void pci_iounmap(struct pci_dev *dev, void __iomem *addr);

#define BIOVEC_MERGEABLE(vec1, vec2)	\
	((bvec_to_phys((vec1)) + (vec1)->bv_len) == bvec_to_phys((vec2)))

#ifdef CONFIG_MMU
#define ARCH_HAS_VALID_PHYS_ADDR_RANGE
extern int valid_phys_addr_range(unsigned long addr, size_t size);
extern int valid_mmap_phys_addr_range(unsigned long pfn, size_t size);
extern int devmem_is_allowed(unsigned long pfn);
#endif

#define xlate_dev_mem_ptr(p)	__va(p)

#define xlate_dev_kmem_ptr(p)	p

extern void register_isa_ports(unsigned int mmio, unsigned int io,
			       unsigned int io_shift);

#endif	 
#endif	 
