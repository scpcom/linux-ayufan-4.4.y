#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef _MV_OS_LNX_H_
#define _MV_OS_LNX_H_

#ifdef __KERNEL__
 
#include <linux/stddef.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/reboot.h>
#include <linux/pci.h>
#include <linux/kdev_t.h>
#include <linux/major.h>
#include <linux/blkdev.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/mm.h>

#include <asm/system.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/hardirq.h>
#include <asm/dma.h>
#include <asm/io.h>
#include <linux/module.h>

#include <linux/random.h>

#define arch_is_coherent()      true

extern void mv_early_printk(char *fmt, ...);

#define MV_ASM              (__asm__ __volatile__)
#define INLINE              inline
#define _INIT               __init
#define MV_TRC_REC(...)
#define mvOsPrintf          printk
#define mvOsEarlyPrintf	    mv_early_printk
#define mvOsOutput          printk
#define mvOsSPrintf         sprintf
#define mvOsSNPrintf        snprintf
#define mvOsMalloc(_size_)  kmalloc(_size_, GFP_ATOMIC)
#define mvOsFree            kfree
#define mvOsMemcpy          memcpy
#define mvOsMemset          memset
#define mvOsSleep(_mils_)   mdelay(_mils_)
#define mvOsTaskLock()
#define mvOsTaskUnlock()
#define strtol              simple_strtoul
#define mvOsDelay(x)        mdelay(x)
#define mvOsUDelay(x)       udelay(x)
#define mvCopyFromOs        copy_from_user
#define mvCopyToOs          copy_to_user
#define mvOsWarning()       WARN_ON(1)
#define mvOsGetTicks()      jiffies
#define mvOsGetTicksFreq()  HZ

#include "mvTypes.h"
#include "mvCommon.h"
#include "../coherency.h"

#ifdef MV_NDEBUG
#define mvOsAssert(cond)
#else
#define mvOsAssert(cond) { do { if (!(cond)) { BUG(); } } while (0); }
#endif  

#else  

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define INLINE inline
#define mvOsPrintf printf
#define mvOsOutput printf
#define mvOsMalloc(_size_) malloc(_size_)
#define mvOsFree free
#define mvOsAssert(cond) assert(cond)

#endif  

#define mvOsIoVirtToPhy(pDev, pVirtAddr)        virt_to_dma((pDev), (pVirtAddr))

#define mvOsIoVirtToPhys(pDev, pVirtAddr)       virt_to_dma((pDev), (pVirtAddr))

#define mvOsCacheFlush(pDev, p, size)                              \
	(COHERENCY_FABRIC_HARD_MODE() ? virt_to_phys((p)) : dma_map_single((pDev), (p), (size), DMA_TO_DEVICE))

#define mvOsCacheInvalidate(pDev, p, size)                          \
	(COHERENCY_FABRIC_HARD_MODE() ? virt_to_phys((p)) : dma_map_single((pDev), (p), (size), DMA_FROM_DEVICE))

#define mvOsCacheUnmap(pDev, phys, size)                          \
	dma_unmap_single((pDev), (dma_addr_t)(phys), (size), DMA_FROM_DEVICE)

#if defined(MY_ABC_HERE)
#define mvOsCachePageFlush(pDev, page, offset, size)              \
	(COHERENCY_FABRIC_HARD_MODE() ? (pfn_to_dma((pDev), page_to_pfn(page)) + (offset)) :             \
	dma_map_page((pDev), (page), (offset), (size), DMA_TO_DEVICE))

#define mvOsCachePageInvalidate(pDev, page, offset, size)         \
	(COHERENCY_FABRIC_HARD_MODE() ? (pfn_to_dma((pDev), page_to_pfn(page)) + (offset)) :             \
	dma_map_page((pDev), (page), (offset), (size), DMA_FROM_DEVICE))
#endif  

#define CPU_PHY_MEM(x)              ((MV_U32)x)
#define CPU_MEMIO_CACHED_ADDR(x)    ((void *)x)
#define CPU_MEMIO_UNCACHED_ADDR(x)  ((void *)x)

#define MV_MEMIO32_WRITE(addr, data)    \
	((*((volatile unsigned int *)(addr))) = ((unsigned int)(data)))

#define MV_MEMIO32_READ(addr)           \
	((*((volatile unsigned int *)(addr))))

#define MV_MEMIO16_WRITE(addr, data)    \
	((*((volatile unsigned short *)(addr))) = ((unsigned short)(data)))

#define MV_MEMIO16_READ(addr)           \
	((*((volatile unsigned short *)(addr))))

#define MV_MEMIO8_WRITE(addr, data)     \
	((*((volatile unsigned char *)(addr))) = ((unsigned char)(data)))

#define MV_MEMIO8_READ(addr)            \
	((*((volatile unsigned char *)(addr))))

#define MV_32BIT_LE_FAST(val)            MV_32BIT_LE(val)
#define MV_16BIT_LE_FAST(val)            MV_16BIT_LE(val)
#define MV_32BIT_BE_FAST(val)            MV_32BIT_BE(val)
#define MV_16BIT_BE_FAST(val)            MV_16BIT_BE(val)

#define MV_MEMIO_LE16_WRITE(addr, data) \
	MV_MEMIO16_WRITE(addr, MV_16BIT_LE_FAST(data))

static inline MV_U16 MV_MEMIO_LE16_READ(MV_U32 addr)
{
	MV_U16 data;

	data = (MV_U16)MV_MEMIO16_READ(addr);

	return (MV_U16)MV_16BIT_LE_FAST(data);
}

#define MV_MEMIO_LE32_WRITE(addr, data) \
	MV_MEMIO32_WRITE(addr, MV_32BIT_LE_FAST(data))

static inline MV_U32 MV_MEMIO_LE32_READ(MV_U32 addr)
{
	MV_U32 data;

	data = (MV_U32)MV_MEMIO32_READ(addr);

	return (MV_U32)MV_32BIT_LE_FAST(data);
}

static inline void mvOsBCopy(char *srcAddr, char *dstAddr, int byteCount)
{
	while (byteCount != 0) {
		*dstAddr = *srcAddr;
		dstAddr++;
		srcAddr++;
		byteCount--;
	}
}

static INLINE MV_U64 mvOsDivMod64(MV_U64 divided, MV_U64 divisor, MV_U64 *modulu)
{
	MV_U64  division = 0;

	if (divisor == 1)
		return divided;

	while (divided >= divisor) {
		division++;
		divided -= divisor;
	}
	if (modulu != NULL)
		*modulu = divided;

	return division;
}

#define MV_FL_8_READ            MV_MEMIO8_READ
#define MV_FL_16_READ           MV_MEMIO_LE16_READ
#define MV_FL_32_READ           MV_MEMIO_LE32_READ
#define MV_FL_8_DATA_READ       MV_MEMIO8_READ
#define MV_FL_16_DATA_READ      MV_MEMIO16_READ
#define MV_FL_32_DATA_READ      MV_MEMIO32_READ
#define MV_FL_8_WRITE           MV_MEMIO8_WRITE
#define MV_FL_16_WRITE          MV_MEMIO_LE16_WRITE
#define MV_FL_32_WRITE          MV_MEMIO_LE32_WRITE
#define MV_FL_8_DATA_WRITE      MV_MEMIO8_WRITE
#define MV_FL_16_DATA_WRITE     MV_MEMIO16_WRITE
#define MV_FL_32_DATA_WRITE     MV_MEMIO32_WRITE

#define CPU_I_CACHE_LINE_SIZE   32     
#define CPU_D_CACHE_LINE_SIZE   32     

#if defined(CONFIG_SHEEVA_ERRATA_ARM_CPU_4413)
#define	 DSBWA_4413(x)	dmb() 		 
#else
#define  DSBWA_4413(x)
#endif

#if defined(CONFIG_SHEEVA_ERRATA_ARM_CPU_4611)
#define	 DSBWA_4611(x)	dmb()		 
#else
#define  DSBWA_4611(x)
#endif

#define MV_OS_CACHE_MULTI_THRESH	256

static inline void mvOsCacheIoSync(void *handle)
{
	if (likely(COHERENCY_FABRIC_HARD_MODE()))
		dma_sync_single_for_cpu(handle, (dma_addr_t) NULL,
					CPU_D_CACHE_LINE_SIZE, DMA_FROM_DEVICE);
}

static inline void mvOsCacheLineFlush(void *handle, void *addr)
{
	if (unlikely(!COHERENCY_FABRIC_HARD_MODE()))
		dma_sync_single_for_device(handle, virt_to_dma(handle, addr), CPU_D_CACHE_LINE_SIZE, DMA_TO_DEVICE);
}

static inline void mvOsCacheLineInv(void *handle, void *addr)
{
	if (unlikely(!COHERENCY_FABRIC_HARD_MODE()))
		dma_sync_single_for_device(handle, virt_to_dma(handle, addr), CPU_D_CACHE_LINE_SIZE, DMA_FROM_DEVICE);
}

static inline void mvOsCacheMultiLineFlush(void *handle, void *addr, int size)
{
	if (unlikely(!COHERENCY_FABRIC_HARD_MODE()))
		dma_map_single(handle, addr, size, DMA_TO_DEVICE);
}

static inline void mvOsCacheMultiLineInv(void *handle, void *addr, int size)
{
	if (unlikely(!COHERENCY_FABRIC_HARD_MODE()))
		dma_map_single(handle, addr, size, DMA_FROM_DEVICE);
}

static inline void mvOsCacheMultiLineFlushInv(void *handle, void *addr, int size)
{
	if (unlikely(!COHERENCY_FABRIC_HARD_MODE()))
		dma_map_single(handle, addr, size, DMA_BIDIRECTIONAL);
}

#if defined(REG_DEBUG)
extern int reg_arry[2048][2];
extern int reg_arry_index;
#endif

#define MV_REG_VALUE(offset)          \
	(MV_MEMIO32_READ((INTER_REGS_VIRT_BASE | (offset))))

#ifdef CONFIG_OF
#define MV_PP2_CPU0_REG_READ(offset)             \
	(MV_MEMIO_LE32_READ(offset))
#define MV_PP2_CPU0_REG_WRITE(offset, val)    \
	MV_MEMIO_LE32_WRITE((offset), (val))

#define MV_PP2_CPU1_REG_READ(offset)             \
	(MV_MEMIO_LE32_READ(offset))
#define MV_PP2_CPU1_REG_WRITE(offset, val)    \
	MV_MEMIO_LE32_WRITE((offset), (val))
#else
#define MV_PP2_CPU0_REG_READ(offset)             \
	(MV_MEMIO_LE32_READ(PP2_CPU0_VIRT_BASE | (offset & 0xffff)))
#define MV_PP2_CPU0_REG_WRITE(offset, val)    \
	MV_MEMIO_LE32_WRITE((PP2_CPU0_VIRT_BASE | (offset & 0xffff)), (val))

#define MV_PP2_CPU1_REG_READ(offset)             \
	(MV_MEMIO_LE32_READ(PP2_CPU1_VIRT_BASE | (offset & 0xffff)))
#define MV_PP2_CPU1_REG_WRITE(offset, val)    \
	MV_MEMIO_LE32_WRITE((PP2_CPU1_VIRT_BASE | (offset & 0xffff)), (val))
#endif

#ifdef CONFIG_SMP
#define MV_PP2_REG_READ(offset)	\
	((smp_processor_id() == 0) ? MV_PP2_CPU0_REG_READ(offset) : MV_PP2_CPU1_REG_READ(offset))

#define MV_PP2_REG_WRITE(offset, val)	\
	((smp_processor_id() == 0) ? MV_PP2_CPU0_REG_WRITE(offset, val) : MV_PP2_CPU1_REG_WRITE(offset, val))
#else
#define MV_PP2_REG_READ(offset)	\
	MV_PP2_CPU0_REG_READ(offset)

#define MV_PP2_REG_WRITE(offset, val)	\
	MV_PP2_CPU0_REG_WRITE(offset, val)
#endif

#define MV_REG_READ(offset)			MV_MEMIO_LE32_READ(offset)

#if defined(REG_DEBUG)
#define MV_REG_WRITE(offset, val)    \
	MV_MEMIO_LE32_WRITE(((offset)), (val)); \
	{ \
		reg_arry[reg_arry_index][0] = (offset);\
		reg_arry[reg_arry_index][1] = (val);\
		reg_arry_index++;\
	}
#else
#define MV_REG_WRITE(offset, val)	MV_MEMIO_LE32_WRITE((offset), (val))
#endif

#define MV_REG_BYTE_READ(offset)	MV_MEMIO8_READ((offset))

#if defined(REG_DEBUG)
#define MV_REG_BYTE_WRITE(offset, val)  \
	MV_MEMIO8_WRITE((offset), (val)); \
	{ \
		reg_arry[reg_arry_index][0] = (offset);\
		reg_arry[reg_arry_index][1] = (val);\
		reg_arry_index++;\
	}
#else
#define MV_REG_BYTE_WRITE(offset, val)  \
	MV_MEMIO8_WRITE((offset), (val))
#endif

#if defined(REG_DEBUG)
#define MV_REG_BIT_SET(offset, bitMask)                 \
	(MV_MEMIO32_WRITE((offset), \
	(MV_MEMIO32_READ(offset) | \
	MV_32BIT_LE_FAST(bitMask)))); \
	{ \
		reg_arry[reg_arry_index][0] = (offset);\
		reg_arry[reg_arry_index][1] = (MV_MEMIO32_READ(offset));\
		reg_arry_index++;\
	}
#else
#define MV_REG_BIT_SET(offset, bitMask)                 \
	(MV_MEMIO32_WRITE((offset), \
	(MV_MEMIO32_READ(offset) | \
	MV_32BIT_LE_FAST(bitMask))))
#endif

#if defined(REG_DEBUG)
#define MV_REG_BIT_RESET(offset, bitMask)                \
	(MV_MEMIO32_WRITE((offset), \
	(MV_MEMIO32_READ(offset) & \
	MV_32BIT_LE_FAST(~bitMask)))); \
	{ \
		reg_arry[reg_arry_index][0] = (offset);\
		reg_arry[reg_arry_index][1] = (MV_MEMIO32_READ(offset));\
		reg_arry_index++;\
	}
#else
#define MV_REG_BIT_RESET(offset, bitMask)                \
	(MV_MEMIO32_WRITE((offset), \
	(MV_MEMIO32_READ(offset) & \
	MV_32BIT_LE_FAST(~bitMask))))
#endif

#define MV_ASM_READ_EXTRA_FEATURES(x) __asm__ volatile("mrc  p15, 1, %0, c15, c1, 0" : "=r" (x));

#define MV_ASM_WAIT_FOR_INTERRUPT      __asm__ volatile("mcr  p15, 0, r0, c7, c0, 4");

MV_U32  mvOsCpuRevGet(MV_VOID);
MV_U32  mvOsCpuPartGet(MV_VOID);
MV_U32  mvOsCpuArchGet(MV_VOID);
MV_U32  mvOsCpuVarGet(MV_VOID);
MV_U32  mvOsCpuAsciiGet(MV_VOID);
MV_U32  mvOsCpuThumbEEGet(MV_VOID);

void *mvOsIoCachedMalloc(void *osHandle, MV_U32 size, MV_ULONG *pPhyAddr, MV_U32 *memHandle);
void *mvOsIoUncachedMalloc(void *osHandle, MV_U32 size, MV_ULONG *pPhyAddr, MV_U32 *memHandle);
void mvOsIoUncachedFree(void *osHandle, MV_U32 size, MV_ULONG phyAddr, void *pVirtAddr, MV_U32 memHandle);
void mvOsIoCachedFree(void *osHandle, MV_U32 size, MV_ULONG phyAddr, void *pVirtAddr, MV_U32 memHandle);
int  mvOsRand(void);

#endif  
