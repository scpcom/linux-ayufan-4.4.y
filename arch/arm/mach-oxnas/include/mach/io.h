/*
 * linux/include/asm-arm/arch-oxnas/io.h
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARM_ARCH_IO_H
#define __ASM_ARM_ARCH_IO_H

#include <mach/hardware.h>

#define IO_SPACE_LIMIT 0xffffffff

#define __mem_pci(a)	(a)

#ifndef CONFIG_PCI

#define	__io(v)		__typesafe_io(v)

#else // CONFIG_PCI

#define	outb(p, v)	__oxnas_outb(p, v)
#define	outw(p, v)	__oxnas_outw(p, v)
#define	outl(p, v)	__oxnas_outl(p, v)
	
#define	outsb(p, v, l)	__oxnas_outsb(p, v, l)
#define	outsw(p, v, l)	__oxnas_outsw(p, v, l)
#define	outsl(p, v, l)	__oxnas_outsl(p, v, l)

#define	inb(p)	__oxnas_inb(p)
#define	inw(p)	__oxnas_inw(p)
#define	inl(p)	__oxnas_inl(p)

#define	insb(p, v, l)	__oxnas_insb(p, v, l)
#define	insw(p, v, l)	__oxnas_insw(p, v, l)
#define	insl(p, v, l)	__oxnas_insl(p, v, l)

extern void pciio_write(unsigned int  data, u32 addr, unsigned int size);
extern unsigned int pciio_read(u32 addr, unsigned int size);

static inline void __iomem *ioport_map(unsigned long port, unsigned int nr) { return __typesafe_io(port); }
static inline void ioport_unmap(void __iomem *addr) {}

static inline void __oxnas_outb(unsigned char  v, u32 p)	{ pciio_write(v,(u32)__typesafe_io(p),             sizeof(char ) );  }
static inline void __oxnas_outw(unsigned short v, u32 p)	{ pciio_write(cpu_to_le16(v),(u32)__typesafe_io(p),sizeof(short) );  }
static inline void __oxnas_outl(unsigned long  v, u32 p)	{ pciio_write(cpu_to_le32(v),(u32)__typesafe_io(p),sizeof(long ) );  }

static inline void __oxnas_outsb(volatile u32 p, unsigned char  * from, u32 len)	{ while (len--) { pciio_write(	         (*from++),(u32)__typesafe_io(p),sizeof(char ) ); } }
static inline void __oxnas_outsw(volatile u32 p, unsigned short * from, u32 len)	{ while (len--) { pciio_write(cpu_to_le16(*from++),(u32)__typesafe_io(p),sizeof(short) ); } }
static inline void __oxnas_outsl(volatile u32 p, unsigned long  * from, u32 len)	{ while (len--) { pciio_write(cpu_to_le32(*from++),(u32)__typesafe_io(p),sizeof(long ) ); } }

static inline unsigned char   __oxnas_inb(u32 p)		{ unsigned int __v =            (pciio_read((u32)__typesafe_io(p),sizeof(char ))); return __v; }
static inline unsigned short  __oxnas_inw(u32 p)		{ unsigned int __v = le16_to_cpu(pciio_read((u32)__typesafe_io(p),sizeof(short))); return __v; }
static inline unsigned long   __oxnas_inl(u32 p)		{ unsigned int __v = le32_to_cpu(pciio_read((u32)__typesafe_io(p),sizeof(long ))); return __v; }
                                         
static inline void __oxnas_insb(volatile u32 p, unsigned char  * to, u32 len)	{ while (len--) { *to++ =            (pciio_read((u32)__typesafe_io(p),sizeof(char ))); } }
static inline void __oxnas_insw(volatile u32 p, unsigned short * to, u32 len)	{ while (len--) { *to++ = le16_to_cpu(pciio_read((u32)__typesafe_io(p),sizeof(short))); } }
static inline void __oxnas_insl(volatile u32 p, unsigned long  * to, u32 len)	{ while (len--) { *to++ = le32_to_cpu(pciio_read((u32)__typesafe_io(p),sizeof(long ))); } }

#endif // CONFIG_PCI

#endif //__ASM_ARM_ARCH_IO_H
