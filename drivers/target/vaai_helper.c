/*
 *
 * @file 	vaai_helper.c
 * @brief	This file contains the basic helper function for VAAI code
 *
 * @author	Adam Hsu
 * @date	2012/06/04
 */

#include <linux/net.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/slab.h>
#include <linux/blkdev.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/in.h>
#include <linux/cdrom.h>
#include <linux/module.h>
#include <asm/unaligned.h>
#include <net/sock.h>
#include <net/tcp.h>
#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_tcq.h>

#include <target/target_core_base.h>
#include <target/target_core_backend.h>
#include <target/target_core_fabric.h>
#include <target/target_core_configfs.h>
#include "target_core_alua.h"
#include "target_core_pr.h"
#include "target_core_ua.h"
#include "target_core_iblock.h"
#include "target_core_file.h"

#include "vaai_target_struc.h"
#include "target_general.h"
#include "vaai_helper.h"

/* max value for logical blocks in ATS command */
u8 u8MaxATSLen                  = 0x80; 

/*
 * bit0 is 1 :  The device server doesn't support the value of zero in NUMBER OF LOGICAL 
 *              BLOCKS field for WRITE SAME command cdb 
 *
 * bit0 is 0 :  The device server MAY or MAY NOT support the value of zero in NUMBER OF
 *              LOGICAL BLOCKS field for WRITE SAME command cdb 
 */
u8 u8WriteSameNoZero            = 0x1;

/*
 * The max number of logical block for write same(10) is 0xffffU and the 0xffff_ffffU
 * is for write same(16/32) so I set the u64MaxWSLen to 0xffff_ffffU
 */
sector_t u64MaxWSLen            = UINT_MAX;




//
//
//
static u8 Hex[] = {
  '0', '1', '2', '3', '4', '5', '6', '7',
  '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

//
//
//
int vaai_do_file_rw(
	IN LIO_SE_DEVICE *pSeDev,
	IN void *pSubSysDev,
	IN u8 u8FuncType,
	IN u8 *pu8Buffer,
	IN sector_t Lba,
	IN u32 u32ByteCounts
	)
{
	struct iovec Iov;
	loff_t Position = 0;
	mm_segment_t Old_FS;
	ssize_t Ret = 0;

	BUG_ON(!pSeDev);
	BUG_ON(!pSubSysDev);
	BUG_ON(!pu8Buffer);
	BUG_ON(!u32ByteCounts);

	if((u8FuncType != 0) && (u8FuncType != DO_WRITE))
		BUG_ON(TRUE);

	Position = (Lba *  pSeDev->se_sub_dev->se_dev_attrib.block_size);
	Iov.iov_base = (void *)pu8Buffer;
	Iov.iov_len = (__kernel_size_t)u32ByteCounts;

	Old_FS = get_fs();
	set_fs(get_ds());

	pr_debug("before call %s, u8FuncType:0x%x, u32ByteCounts:0x%x, "
		"Position:0x%llu\n", __func__, u8FuncType, u32ByteCounts, Position);

	if(0 == u8FuncType)
		Ret = vfs_readv((struct file *)pSubSysDev, &Iov, 1, &Position);
	else
		Ret = vfs_writev((struct file *)pSubSysDev, &Iov, 1, &Position);

	set_fs(Old_FS);

	pr_debug("after call %s, u32ByteCounts:0x%x, Ret:0x%zx, Position:0x%llu\n",
                        __func__, u32ByteCounts, Ret, Position);

	/* FIXED ME */
	if ((Ret < 0) && (Ret != -EIO))
		return Ret;

	if (Ret != u32ByteCounts)
		return -EIO;

	return 0;

}

int
vaai_do_pscsi_rw(
	IN LIO_SE_CMD *pSeCmd,
	IN void *pSubSysDev,
	IN u8 u8FuncType,
	IN u8 *ppu8Buffer,
	IN sector_t Lba,
	IN u32 pu32ByteCounts
	)
{
	return -EOPNOTSUPP;
}

/*
 * @fn XCOPY_DIR vaai_get_xcopy_dir (IN XCOPY_INFO *pXcopyInfo)
 * @brief Simple function to get the copy direction for extended copy operation
 *
 * @sa 
 * @param[in] pXcopyInfo
 * @retval XCOPY_DIR_FF or XCOPY_DIR_BB or XCOPY_DIR_BF or XCOPY_DIR_FB
 */
XCOPY_DIR vaai_get_xcopy_dir(
    IN XCOPY_INFO *pXcopyInfo
    )
{
    XCOPY_DIR Dir;
    SUBSYSTEM_TYPE SrcType, DestType;

    BUG_ON(!pXcopyInfo);

    SrcType   = pXcopyInfo->SrcSubSysType;
    DestType  = pXcopyInfo->DestSubSysType;

    if ((SrcType == SUBSYSTEM_FILE) && (DestType == SUBSYSTEM_FILE)){
        Dir = XCOPY_DIR_FF;
    }else if ((SrcType == SUBSYSTEM_BLOCK) && (DestType == SUBSYSTEM_BLOCK)){
        Dir = XCOPY_DIR_BB;
    }else if ((SrcType == SUBSYSTEM_BLOCK)  && (DestType == SUBSYSTEM_FILE)){
        Dir = XCOPY_DIR_BF;
    }else if ((SrcType == SUBSYSTEM_FILE)  && (DestType == SUBSYSTEM_BLOCK)){
        Dir = XCOPY_DIR_FB;
    }else{
        Dir = MAX_COPY_DIR; // error case
    }
    return Dir;
}



void * vaai_safe_alloc_m(
    IN u32 *pu32Size,
    IN ALLOC_MEM_TYPE MemType,
    IN gfp_t GfpFlag
    )
{
    void *p = NULL;
    u32 u32BC = *pu32Size;

    if(MemType >= MAX_MEM_TYPE)
        return NULL;

    do{   
        p = ((MemType == MEM_KMALLOC) ? (kmalloc(u32BC, GfpFlag)) : (vmalloc(u32BC)));
        DBG_PRINT("vaai_safe_alloc_m(), type:0x%x, p:0x%p, u32BC:0x%x\n",MemType, p ,u32BC);

        if(p)
            break;
    
        if(u32BC <= PAGE_SIZE)
            break;  // If fail to allocate and remain size <= PAGE_SIZE , to treat it as error

        u32BC -= PAGE_SIZE;
    }while(TRUE); 

    *pu32Size = u32BC;
    return p;

}

void vaai_safe_free_m(
    IN void *p,
    IN ALLOC_MEM_TYPE MemType
    )
{
    if(!p)
        return;

    if(MemType < MAX_MEM_TYPE) {
        if ((MemType == MEM_KMALLOC))
            kfree(p);
        else
            vfree(p);
    }
    return;
}

/*
 * @fn int vaai_chk_and_complete(IN int CmdResult, IN void *pContext)
 * @brief Wrapper function to call transport_complete_task() if command is successful
 *
 * @sa 
 * @param[in] CmdResult
 * @param[in] pContext
 * @retval 0 - command is successful , 1 - command is fail
 */
int vaai_chk_and_complete(
    IN int CmdResult,
    IN void *pContext
    )
{
    LIO_SE_TASK *pSeTask = NULL;

    BUG_ON(!pContext);
    pSeTask = (LIO_SE_TASK *)pContext;

    if (CmdResult == 0){
        /* To complete the task if command result is successful */
        pSeTask->task_scsi_status = GOOD;
        transport_complete_task(pSeTask, 1);
    }
    return CmdResult;
}

//
//
//
void
dbg_dump_mem(
  IN u8 *pu8Buf,
  IN size_t DumpSize
  )
/*++

  Description:

    Simple function to dump the memory

  Input:

  Output:

--*/
{
#if 0
    u8 *Data;
    u8 Val[50];
    u8 Str[20];
    u8 c;
    size_t Size;
    size_t Index;

    Data = pu8Buf;

    while (DumpSize) {
        Size = 16;
        if (Size > DumpSize) {
            Size = DumpSize;
        }

        for (Index = 0; Index < Size; Index += 1) {
            c                   = Data[Index];
            Val[Index * 3 + 0]  = Hex[c >> 4];
            Val[Index * 3 + 1]  = Hex[c & 0xF];
            Val[Index * 3 + 2]  = (u8) ((Index == 7) ? '-' : ' ');
            Str[Index]          = (u8) ((c < ' ' || c > 'z') ? '.' : c);
        }

        Val[Index * 3]  = 0;
        Str[Index]      = 0;
        pr_err("addr-0x%p: %s *%s*\n",Data, Val, Str);
        Data += Size;
        DumpSize -= Size;
    }
#endif
}

struct page *
vaai_safe_get_page(
    IN gfp_t GfpFlag,
    IN u32 *pu32XfsBCs
    )
{
    struct page *pPage = NULL;
    u32 u32BCs = 0;
    u8 u8Result = FALSE;

    BUG_ON(!pu32XfsBCs);
    u32BCs = *pu32XfsBCs;

    do{
        pPage = alloc_pages(GfpFlag, get_order(u32BCs));
        if(pPage){
            u8Result = TRUE;
            break;
        }
        //
        // If fail to allocate memory, we will do ...
        //
        if(u32BCs <= PAGE_SIZE)
            break;

        u32BCs -= PAGE_SIZE;
    }while(TRUE);

    if(u8Result == TRUE){
        *pu32XfsBCs = u32BCs;
        return pPage;
    }

  return NULL;

}

struct scatterlist *
vaai_get_task_sg_from_se_cmd(
  IN LIO_SE_CMD *pSeCmd
  )
{
	BUG_ON(!pSeCmd);
    return pSeCmd->t_data_sg;
}


