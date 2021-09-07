
/*
 * t23xrmMemUtil.c
 *
 * Buffer mapping/conversion utility routines for use by t23x registered
 * interfaces
 *
 *
 * Copyright (c) 2007, Freescale Semiconductor, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither the name of Freescale Semiconductor nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */




/** @file
 * Manages buffer mapping for constructed descriptors to be executed
 * by the xwc driver subsystem for Talitos 2/3, including virtual buffer
 * page mapping for both kernel and user application handling
 */



#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/vmalloc.h>

#include <asm/page.h>
#include <asm/io.h>
#include <asm/scatterlist.h>

#include "../common/xwcRMinterface.h"
#include "t23xrmInternal.h"
#include "../common/t23.h"


/**
 * All functions here in t23xrmMemUtil.c are designed to handle the
 * mapping, lockdown, and construction of fragment lists as required for
 * (a) a single memory buffer specified in a descriptor, and (b)
 * the host operating system in question.
 *
 * There are six calls present, handling the mapping translation and
 * unmapping/freeing of buffers consisting of three different classes
 * of memory as:
 *
 *   (1) Logical addresses
 *   (2) Virtual addresses in a user-process address space
 *   (3) Virtual addresses in the kernel's address space
 *       (unimplemented in this release)
 *
 * Of course, hardware crypto accelerators generally cannot do address
 * translations on their own; this translation to physical addresses
 * must be done for them. Advanced crypto accelerators do generally
 * have the capability to act on a buffers fragmented into varying
 * locations (as buffers in virtual address space may be), so one of
 * the actions that must take place here is the construction of a
 * scatter-gather list for that buffer.
 *
 * Therefore, these functions are all passed:
 * (1) an entity ID for the pointer value in the descriptor, normally
 * the ordinal value of where the pointer element occurs in the
 * descriptor's construction,
 * (2) the descriptor in question (this is core dependent), and
 * (3) a pointer to auxiliary mapping information that the IF will
 * preserve with the descriptor; this holds enough information to "free"
 * any descriptive resources later. These functions will work upon the
 * three classes of memory in this way:
 *
 *   - "Translate" functions will:
 *     + translate a buffer address to physical
 *     + if virtual,
 *       > make sure pages comprising the buffer are swapped in
 *       > lock down those pages
 *       > allocate a scatter-gather list adequate for this buffer
 *       > fill out the scatter-gather list
 *     + put the physical address (the indirect reference to the
 *       scatterlist) in the descriptor pointer entity
 *
 *   - "Free" functions will:
 *     + if a scatter-gather list was constructed (DPD_AUXMAP pointer
 *       is non-NULL):
 *       > unlock memory pages
 *       > free the scatter-gather list
 *
 * So, all these functions have a symmetrical argument set:
 * - uint32_t    entity - descriptor pointer entity ID
 * - void       *desc   - descriptor
 * - void       *auxmap - auxiliary data to maintain with the descriptor
 *                       that tracks the resources used for this pointer.
 *
 */

/**
 * RMstatus xwcMemTranslateLogical(uint32_t    entity,
 *                                 void       *desc,
 *                                 void       *auxmap)
 *
 * where:
 * - entity - pointer element identity in the descriptor
 * - desc   - pointer of the descriptor containing the logical
 *            buffer reference
 * - auxmap - an auxmap entry for tracking resources used to
 *            map this buffer (unlikely to be used with a logical
 *            address)
 *
 * xwcMemTranslateLogical will take a logical buffer pointer
 * in a descriptor, and translate it to a physical pointer to
 * the same buffer. The buffer in question must be resident
 * and contiguous in physical memory
 *
 */

RMstatus xwcMemTranslateLogical(uint32_t    entity,
                                void       *desc,
                                void       *auxmap)
{
    T2DPD   *pd = desc;

#ifdef OBSOLETE
    uint8_t *ud;
    ud = pd->pair[entity].ptr;
#endif

    pd->pair[entity].ptr =
      (void *)virt_to_phys(lowmem_page_address(pd->pair[entity].ptr));

    return RM_OK;
}




/**
 * RMstatus xwcMemTranslateUserVirtual(uint32_t    entity,
 *                                     void       *desc,
 *                                     void       *auxmap)
 *
 * where:
 * - entity - pointer element identity in the descriptor
 * - desc   - pointer of the descriptor containing the virtual
 *            buffer reference in the user process' address
 *            space
 * - auxmap - an auxmap entry for tracking resources used to
 *            map this buffer
 *
 * xwcMemTranslateUserVirtual(), with a user buffer pointer in
 * a descriptor (as was passed in from the user process), will
 * identify the pages used for this buffer, lock them in memory,
 * allocate a contiguous block for a scatterlist, populate the
 * scatterlist, and set the descriptor pointer to that scatterlist.
 *
 */

RMstatus xwcMemTranslateUserVirtual(uint32_t    entity,
                                    void       *desc,
                                    void       *auxmap)
{
    unsigned       i;
    linkEntry     *hwsgl;
    size_t         count;
    int            pageEst;
    T2DPD         *pd = desc;
    T2DESC_AUXMAP *pdmap = auxmap;

    /* Estimate the number of needed page pointers */
    pageEst = (((unsigned long)pd->pair[entity].ptr & ~PAGE_MASK) +
               pd->pair[entity].size + ~PAGE_MASK) >> PAGE_SHIFT;

    /* Allocate list of pointers to pages for this user buffer reference */
    pdmap->pair[entity].pages = vmalloc(pageEst * sizeof(pdmap->pair[entity].pages));
    if (pdmap->pair[entity].pages == NULL)
        return RM_NO_MEMORY;

    /* Lock this process' pages and map them. The descriptor pair pointer */
    /* still references the user's buffer at this point                   */
    down_read(&current->mm->mmap_sem);
    pdmap->pair[entity].pageCt =
        get_user_pages(current,
                       current->mm,
                       (unsigned long)pd->pair[entity].ptr,
                       pageEst,
                       WRITE, 1,
                       pdmap->pair[entity].pages,
                       NULL);
    up_read(&current->mm->mmap_sem);

    /* here for development, remove once stabilized */
    if (pageEst != pdmap->pair[entity].pageCt)
        printk("t23xwc: user page estimate = %d, actual = %d\n", pageEst, pdmap->pair[entity].pageCt);

    if (pdmap->pair[entity].pageCt > pageEst)
        panic("t23xwc - user pages mapped exceeds estimate\n");

    /* Needed user pages are now mapped. If data element fits in 1 page, then */
    /* we can just do a physical pointer, no scatterlist is needed. If it     */
    /* exceeds one page, we must have a scatterlist                           */

    if (pdmap->pair[entity].pageCt > 1) /* Does entry span pages? */
    {
        /* Allocate "hardware" scatterlist */
        hwsgl = kmalloc(pageEst * sizeof(linkEntry), GFP_KERNEL | GFP_DMA);
        if (hwsgl == NULL)
        {
            /* Out of kmalloc() space, gotta bail. Release mapped pages */
            for (i = 0; i < pdmap->pair[entity].pageCt; i++)
                page_cache_release(pdmap->pair[entity].pages[i]);

            /* Free allocated page list */
            vfree(pdmap->pair[entity].pages);

            return RM_NO_MEMORY;
        }

        count = pd->pair[entity].size;

        hwsgl[0].segAddr =
            (unsigned char *)virt_to_phys(lowmem_page_address(pdmap->pair[entity].pages[0]) +
                             ((unsigned long)pd->pair[entity].ptr & ~PAGE_MASK));

        hwsgl[0].chainCtrl = 0;
        hwsgl[0].extAddr   = 0;

        if (pdmap->pair[entity].pageCt > 1)
        {
            hwsgl[0].segLen = PAGE_SIZE - ((unsigned long)pd->pair[entity].ptr & ~PAGE_MASK);
            count -= hwsgl[0].segLen;
            for (i = 1; i < pdmap->pair[entity].pageCt; i++)
            {
                hwsgl[i].segLen    = count < PAGE_SIZE ? count : PAGE_SIZE;
                hwsgl[i].segAddr   = (unsigned char *)
                    virt_to_phys(lowmem_page_address(pdmap->pair[entity].pages[i]));
                hwsgl[i].extAddr   = 0;
                hwsgl[i].chainCtrl = 0;
                count -= PAGE_SIZE;
            }
        }
        else
            hwsgl[0].segLen = pd->pair[entity].size;

        /* mark the last entry in the Talitos scatterlist */
        hwsgl[pdmap->pair[entity].pageCt - 1].chainCtrl = LAST_ENTRY;

        /* Point to descriptor pair to the Talitos scatterlist */
        pd->pair[entity].ptr     = (unsigned char *)virt_to_phys(hwsgl);
        pd->pair[entity].extent |= JUMPTABLE;
    }
    else
        pd->pair[entity].ptr =
            (unsigned char *)virt_to_phys(lowmem_page_address(pdmap->pair[entity].pages[0]) +
                                          ((unsigned long)pd->pair[entity].ptr & ~PAGE_MASK));

    return RM_OK;
}




/**
 * RMstatus xwcMemTranslateKernelVirtual(uint32_t    entity,
 *                                       void       *desc,
 *                                       void       *auxmap)
 *
 * where:
 * - entity - pointer element identity in the descriptor
 * - desc   - pointer of the descriptor containing the virtual
 *            buffer reference in the kernel process' address
 *            space
 * - auxmap - an auxmap entry for tracking resources used to
 *            map this buffer
 *
 * xwcMemTranslateKernelVirtual() is intended to translate
 * a kernel-virtual buffer pointer into a scatter/gather
 * fragment list referencing possibly non-contiguous data
 * in kernel-owned address space. It is not implemented
 * at this time, largely because it is assumed that there
 * is little value to referencing virtual buffers in kernel
 * mode.
 */

RMstatus xwcMemTranslateKernelVirtual(uint32_t    entity,
                                      void       *desc,
                                      void       *auxmap)
{

    return RM_UNIMPLEMENTED;
}






/**
 * RMstatus xwcMemReleaseLogical(uint32_t    entity,
 *                               void       *desc,
 *                               void       *auxmap)
 *
 * where:
 * - entity - pointer element identity in the descriptor
 * - desc   - pointer of the descriptor containing the physical
 *            buffer reference in the kernel's address space
 * - auxmap - an auxmap entry for tracking resources used to
 *            map this buffer
 *
 * xwcMemReleaseLogical() as the counterpart of xwcMemTranslateLogical()
 * is effectively a NULL function, as there are no resources to free
 * from a logical pointer. It is here only for symmetry.
 */

RMstatus xwcMemReleaseLogical(uint32_t    entity,
                              void       *desc,
                              void       *auxmap)
{
    return RM_OK;
}



/**
 * RMstatus xwcMemReleaseUserVirtual(uint32_t    entity,
 *                                   void       *desc,
 *                                   void       *auxmap)
 *
 * where:
 * - entity - pointer element identity in the descriptor
 * - desc   - pointer of the descriptor containing the physical
 *            address of a scatter-gather map of the user's
 *            buffer
 * - auxmap  - an auxmap entry for tracking resources used to
 *            map this buffer
 *
 * xwcMemReleaseUserVirtual() as the counterpart of
 * xwcMemTranslateUserVirtual(), exists to free resources used
 * to describe the physical structure of the user's buffer.
 * Effectively, it frees the scatterlist, and unlocks/unmaps
 * the pages comprising the buffer.
 */

RMstatus xwcMemReleaseUserVirtual(uint32_t    entity,
                                  void       *desc,
                                  void       *auxmap)
{
    int            i;
    T2DPD         *pd = desc;
    T2DESC_AUXMAP *pdmap = auxmap;

    /* If element is in real, live scatterlist, free it */
    if (pd->pair[entity].extent & JUMPTABLE)
        kfree(phys_to_virt((unsigned long)pd->pair[entity].ptr));

    /* walk through segment buffer and unlock pages */
    for (i = 0; i < pdmap->pair[entity].pageCt; i++)
        page_cache_release(pdmap->pair[entity].pages[i]);

    /* Now free page list */
    vfree(pdmap->pair[entity].pages);

    return RM_OK;
}



/**
 * RMstatus xwcMemReleaseKernelVirtual(uint32_t    entity,
 *                                     void       *desc,
 *                                     void       *auxmap)
 *
 * where:
 * - entity - pointer element identity in the descriptor
 * - desc   - pointer of the descriptor containing the physical
 *            address of a scatter-gather map of the kernel
 *            buffer
 * - auxmap - an auxmap entry for tracking resources used to
 *            map this buffer
 *
 * xwcMemReleaseKernelVirtual() is present as a placeholder
 * at this time, it is not implemented.
 */

RMstatus xwcMemReleaseKernelVirtual(uint32_t    entity,
                                    void       *desc,
                                    void       *auxmap)
{

    return RM_UNIMPLEMENTED;
}
