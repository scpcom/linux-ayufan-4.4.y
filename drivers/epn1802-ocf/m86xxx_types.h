/*
 * Copyright (c) 2012 Elliptic Technologies Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
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


#ifndef _ELPTYPES_H_
#define _ELPTYPES_H_

#include <linux/kernel.h>       /* printk() */
#include <linux/slab.h>         /* kmalloc() */
#include <linux/fs.h>           /* everything... */
#include <linux/errno.h>        /* error codes */
#include <linux/types.h>        /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>        /* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/delay.h>        /* udelay */
#include <linux/sched.h>
#include <linux/kdev_t.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/poll.h>
#include <linux/wait.h>         /* wait queue */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#ifndef NO_PCI_H
#include <linux/pci.h>
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,15,0)
#include <asm/system.h>         /* cli(), *_flags, mb() */
#endif
#include <asm/uaccess.h>        /* copy_*_user */
#include <asm/io.h>             /* memcpy_fromio */


#define MEM_FREE_MAP(d,s,v,r)	dma_free_coherent(d,s,v,(dma_addr_t)(r))
#define MEM_ALLOC_MAP(d,s,r)	dma_alloc_coherent(d,s,(dma_addr_t *)(r), GFP_KERNEL)
#define MEM_UNMAP(d,m,s)	dma_unmap_single(d,(dma_addr_t)(m),(size_t)(s),DMA_BIDIRECTIONAL)
#define MEM_MAP(d,m,s)		(U32*)dma_map_single(d,m,(size_t)(s),DMA_BIDIRECTIONAL)



#define USE_IT(x) ((x)=(x))

typedef unsigned int   UINT;
typedef char C8;                /* just normal char for compatability with sysytem functions */
typedef unsigned char U8;       /* unsigned 8-bit  integer */
typedef unsigned short U16;     /* unsigned 16-bit integer */
typedef unsigned int U32;       /* unsigned 32-bit integer */
typedef unsigned long long U64; /* unsigned 64-bit integer */
typedef signed char S8;         /* 8-bit  signed integer */
typedef signed short S16;       /* 16-bit signed integer */
typedef signed int S32;         /* 32-bit signed integer */
typedef signed long long S64;   /* 64 bit signed integer */

//#define ELP_BOARD_VENDOR_ID   0xE117

typedef struct elp_id_s
{

  U16 vendor_id;
  U16 device_id;
  struct pci_dev *data;

} elp_id;


// This structure defines control and access registers and memory regions
// mapped into host virtual and physical memory
// it also holds a bus control and config information
typedef struct elphw_if_s
{
  // Virtual address mapping
  volatile void *pmem;          //  Bar0
  volatile void *preg;          //  Bar1
  volatile char *preg2;         //  Bar1
  volatile char *pbar2;         //  Bar2
  volatile char *pbar3;         //  Bar3
  volatile char *pbar4;         //  Bar4

} elphw_if;

void elppci_get_vendor(elphw_if *tif);
void elppci_set_little_endian(elphw_if *tif);
void elppci_interrupt_enabled(elphw_if *tif);
void elppci_set_little_endian(elphw_if *tif);
void elppci_set_big_endian(elphw_if *tif);
void elppci_reset (elphw_if * tif);
int elppci_init (elp_id * tid, elphw_if * tif, U32 flags);
void elppci_cleanup (elphw_if * tif);
void elppci_info (elphw_if * tif);

#endif
