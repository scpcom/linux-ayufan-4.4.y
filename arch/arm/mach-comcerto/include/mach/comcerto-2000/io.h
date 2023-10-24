/*
 *  linux/include/asm-arm/arch-comcerto/comcerto-1000/io.h
 *
 *  Copyright (C) 2004,2005 Mindspeed Technologies, Inc.
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
#ifndef __ASM_ARCH_COMCERTO1000_IO_H
#define __ASM_ARCH_COMCERTO1000_IO_H

#include <asm/io.h>
#include <mach/hardware.h>

#if !defined(CONFIG_PCI)

#define __io(a)			((void __iomem *)(a))
#define __mem_pci(a)	(a)

#else

#define __mem_pci(a)	(a)

/* IO ports are not supported */
#define outb(v,p)	__readwrite_bug("outb")
#define outw(v,p)	__readwrite_bug("outw")
#define outl(v,p)	__readwrite_bug("outl")

#define inb(p)		(__readwrite_bug("inb"), 0)
#define inw(p)		(__readwrite_bug("inw"), 0)
#define inl(p)		(__readwrite_bug("inl"), 0)

#define outsb(p,d,l)	__readwrite_bug("outsb")
#define outsw(p,d,l)	__readwrite_bug("outsw")
#define outsl(p,d,l)	__readwrite_bug("outsl")

#define insb(p,d,l)	(__readwrite_bug("insb"), 0)
#define insw(p,d,l)	(__readwrite_bug("insw"), 0)
#define insl(p,d,l)	(__readwrite_bug("insl"), 0)

/*
 * io{read,write}{8,16,32} macros
 */

#define ioread8(p)	({ unsigned int __v = __raw_readb(p); __v; })
#define ioread16(p)	({ unsigned int __v = le16_to_cpu(__raw_readw(p)); __v; })
#define ioread32(p)	({ unsigned int __v = le32_to_cpu(__raw_readl(p)); __v; })

#define iowrite8(v,p)	__raw_writeb(v, p)
#define iowrite16(v,p)	__raw_writew(cpu_to_le16(v), p)
#define iowrite32(v,p)	__raw_writel(cpu_to_le32(v), p)

#define ioread8_rep(p,d,c)	__raw_readsb(p,d,c)
#define ioread16_rep(p,d,c)	__raw_readsw(p,d,c)
#define ioread32_rep(p,d,c)	__raw_readsl(p,d,c)

#define iowrite8_rep(p,s,c)	__raw_writesb(p,s,c)
#define iowrite16_rep(p,s,c)	__raw_writesw(p,s,c)
#define iowrite32_rep(p,s,c)	__raw_writesl(p,s,c)

#define ioport_map(c,s)		(__readwrite_bug("ioport_map"), NULL)
#define ioport_unmap(addr)	__readwrite_bug("ioport_unmap")

#endif /* !defined(CONFIG_PCI) */

#endif /* __ASM_ARCH_COMCERTO1000_IO_H */
