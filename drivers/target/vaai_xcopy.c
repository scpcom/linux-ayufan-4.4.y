/**
 * @file 	vaai_xcopy.c
 * @brief	This file contains the 3rd party copy command code for VAAI
 *
 * @author	Adam Hsu
 * @date	2012/06/04
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/unaligned.h>
#include <scsi/scsi.h>
#include "vaai_comp_opt.h"
#include <target/target_core_base.h>
#include <target/target_core_backend.h>
#include <target/target_core_fabric.h>
#include "target_core_ua.h"
#include "target_core_iblock.h"
#include "vaai_target_struc.h"
#include "tpc_general.h"
#include "vaai_helper.h"

/**/
extern int __chk_do_lid1_xcopy(
    IN LIO_SE_CMD *pSeCmd,
    IN LID1_XCOPY_PARAMS *pXcopyParams
    );

extern int __chk_do_lid4_xcopy(
    IN LIO_SE_CMD *pSeCmd,
    IN LID4_XCOPY_PARAMS *pXcopyParams
    );

/*
 * @fn int __go_xcopy_handler (IN LIO_SE_CMD *pSeCmd, IN u8 *pu8Cscd, IN u8 *pu8Seg, IN u32 u32TotalSegListLen)
 *
 * @brief Main function to process Extended Copy (LID1 or LID4) Command
 * @note
 * @param[in] pSeCmd
 * @param[in] pu8Cscd
 * @param[in] pu8Seg
 * @param[in] u32TotalSegListLen
 * @retval 0 - Success
 * @retval Others - Fail
 */
int __go_xcopy_handler(
    IN LIO_SE_CMD *pSeCmd,
    IN u8 *pu8Cscd,
    IN u8 *pu8Seg,
    IN u32 u32TotalSegListLen
    )
{
    u8 u8FoundExecFunc =FALSE, u8SuccessToExec = FALSE;
    u32 u32Index = 0;

    if (pSeCmd == NULL || pu8Cscd == NULL || pu8Seg == NULL || u32TotalSegListLen == 0)
        BUG_ON(TRUE);

    /* let's start to parse the each segment descriptor to do xcopy ... */
    while (u32TotalSegListLen > 0){

        u8FoundExecFunc = FALSE;
        u8SuccessToExec = FALSE;

        /*  To get the suitable xcopy function by segment descriptor type code */
        for (u32Index = 0;; u32Index++){
            if (gXcopyFuncTable[u32Index].SegDescCode == MAX_SEG_DESC_TYPE_CODE)
                break;

            if ((gXcopyFuncTable[u32Index].SegDescCode == pu8Seg[0])
                && (gXcopyFuncTable[u32Index].ChkCscdId != NULL)
                && (gXcopyFuncTable[u32Index].InfoCollect != NULL)
                && (gXcopyFuncTable[u32Index].Exec != NULL)
            )
            {
                u8FoundExecFunc = TRUE;
                break;
            }
        } // end of (u32Index = 0;; u32Index++)

        if (u8FoundExecFunc == FALSE)
            break;

        /*
         * FIXED ME !! FIXED ME !!
         *
         * Before to go to real xocpy function, we should check whether the
         * current SEGMENT type can be mapped to current CSCD type. This should
         * be fixed in the future.
         */



        /* Check whether the CSCD descriptor ID we support */
        if (gXcopyFuncTable[u32Index].ChkCscdId(pSeCmd, pu8Seg) == 0)
            break;

        if (gXcopyFuncTable[u32Index].InfoCollect(pSeCmd, pu8Cscd, pu8Seg) == 0)
            break;

        /* If one of type of xcopy is fail, to exit this loop and report to uppoer layer .. */
        if (gXcopyFuncTable[u32Index].Exec(pSeCmd, pu8Seg) == 0)
            break;

        /*
         * FIXED ME !! FIXED ME !!
         *
         * We may need to record some information after to process xcopy function.
         * This should be fixed in the future.
         */

        /* Go to next segment descriptor ... */
        u8SuccessToExec     = TRUE;
        u32TotalSegListLen  -= __get_seg_desc_len(pu8Seg);
        pu8Seg              += (size_t)__get_seg_desc_len(pu8Seg);
    }

    return u8SuccessToExec;
}

/*
 * @fn int __do_lid1_xcopy (IN LIO_SE_CMD *pSeCmd)
 * @brief Main function to process Extended Copy (LID1) Command
 * @note LID1 means List Id is one byte
 * @param[in] pSeCmd
 * @retval 0 - Success
 * @retval Others - Fail
 */
int __do_lid1_xcopy(
    IN LIO_SE_CMD *pSeCmd
    )
{
    LID1_XCOPY_PARAMS *pParams = NULL;
    u8 *pu8SegDesc = NULL, *pu8CscdDesc = NULL;
    u8 u8SuccessToExec = FALSE;
    u32 u32TotalSegDescListLen = 0;
    int Ret;

	DBG_PRINT("XCOPY_DBG: vaai_do_lid1_xcopy()\n");

    //
    //
    BUG_ON(!pSeCmd);  
    pParams = (LID1_XCOPY_PARAMS *)transport_kmap_data_sg(pSeCmd);

    Ret = __chk_do_lid1_xcopy(pSeCmd, pParams);
    if (Ret == XCOPY_RET_NO_XFS_PASS){
        Ret = 0;
        goto _SUCCESS_EXIT_;
    }

    if (Ret == XCOPY_RET_FAIL){
        Ret = -EOPNOTSUPP;
        goto _ERROR_EXIT_;
    }

    pu8CscdDesc             = (u8*)((size_t)pParams + sizeof(LID1_XCOPY_PARAMS));  
    pu8SegDesc              = (u8*)((size_t)pu8CscdDesc + get_unaligned_be16(&pParams->CscdDescListLen[0]));
    u32TotalSegDescListLen  = get_unaligned_be32(&pParams->SegDescListLen[0]);

    /*
     * All error reason (suitable sense data) was set in internal xcopy function
     * already so here won't do anything ...
     */
    u8SuccessToExec = __go_xcopy_handler(
                            pSeCmd, 
                            pu8CscdDesc, 
                            pu8SegDesc, 
                            u32TotalSegDescListLen
                            );

    Ret = 0;
    if(u8SuccessToExec == FALSE){
        pr_debug("%s: fail\n", __func__);
        Ret = -EIO;
        goto _ERROR_EXIT_;
    }

_SUCCESS_EXIT_:
    pr_debug("%s: pass\n", __func__);

_ERROR_EXIT_:
    transport_kunmap_data_sg(pSeCmd);
    return Ret;
}


/*
 * @fn int vaai_do_lid1_xcopy_v1 (IN LIO_SE_TASK *pSeTask)
 * @brief Wrapper function to process Extended Copy (LID1) Command passed by target module
 * @note LID1 means List Id is one byte
 * @param[in] pSeTask
 * @retval 0 - Success
 * @retval Others - Fail
 */
int vaai_do_lid1_xcopy_v1(
    IN LIO_SE_TASK *pSeTask
    )
{
    int CmdResult;

    BUG_ON(!pSeTask);
    BUG_ON(!pSeTask->task_se_cmd);

    CmdResult =  __do_lid1_xcopy(pSeTask->task_se_cmd);
    return vaai_chk_and_complete(CmdResult, pSeTask);
}

/*
 * @fn int __do_receive_xcopy_op_param (IN LIO_SE_CMD *pSeCmd)
 * @brief Main function to process Receive Copy Parameter command
 *
 * @param[in] pSeCmd
 * @retval 0 - Success
 * @retval 1 - Fail
 */
int __do_receive_xcopy_op_param(
    IN LIO_SE_CMD *pSeCmd
    )
{
	RECEIVE_COPY_OP_PARAM *pParams = NULL;
	u8 *pu8Buf = NULL;
	u16 u16Len = 0, u16CscdDescCount = 0, u16SegDescCount = 0;
	int ret = 1;

	if (pSeCmd->data_length == 0)
		return ret;

	pParams = (RECEIVE_COPY_OP_PARAM *)transport_kmap_data_sg(pSeCmd);
	if (!pParams)
		return ret;

	memset(pParams, 0, pSeCmd->data_length);
	pu8Buf = (u8 *)pParams;

	/*
	 * FIXED ME !! FIXED ME !! FIXED ME !! FIXED ME !!
	 *
	 * VAAI Specification version 7.1 for ESX 4.1 requests SNLID
	 * (Support No List IDentifier) bit is 1 and it indicates the copy manager 
	 * supports an extended copy command parameter list in which the LIST ID USAGE
	 * is set to 11b and LIST IDENTIFIER is set to zero as described in 6.4.3.2
	 *
	 * In the other words, we can't use LIST IDENTIFIER to identify extended copy
	 * command. And, it MAY conflict with other 3rd party software ...
	 *
	 */
	pParams->SNLID  = 1;

	u16CscdDescCount = __get_max_supported_cscd_desc_count();
	if (u16CscdDescCount == 0)
		goto _EXIT_;

	put_unaligned_be16(u16CscdDescCount, &pParams->MaxCscdDescCount[0]);

	u16SegDescCount  = __get_max_supported_seg_desc_count();
	if (u16SegDescCount == 0)
		goto _EXIT_;

	put_unaligned_be16(u16SegDescCount, &pParams->MaxSegDescCount[0]);
	put_unaligned_be32(__get_total_supported_desc_len(), &pParams->MaxDescListLen[0]);

	/* FIXED ME, need to check each element here ... */
	put_unaligned_be32(0x0, &pParams->MaxSegLen[0]);
	put_unaligned_be32(0x0, &pParams->MaxInlineDataLen[0]);
	put_unaligned_be32(0x0, &pParams->HeldDataLimit[0]);
	put_unaligned_be32(0x0, &pParams->MaxStreamDevXfsSize[0]);
	put_unaligned_be16(0x0, &pParams->TotalConCurrentCpSize[0]);  

	pParams->MaxConCurrentCp            = 0;
	pParams->DataSegGranularityLog2     = 10; // 2 ^ 10
	pParams->InlineDataGranularityLog2  = 0;
	pParams->HeldDataGranularityLog2    = 0;  // 2 ^ 10

	u16Len = __fill_supported_desc_type_codes(&pu8Buf[44]);
	pParams->ImplementedDescListLen = (u8)u16Len;  // byte 43 (N - 43)
	put_unaligned_be32((u32)(43 + u16Len -3), &pParams->DataLen[0]);


	pr_debug("XCOPY_DBG: %s\n", __func__);
	pr_debug("XCOPY_DBG: DataLen:0x%08x\n",*(u32*)&pParams->DataLen[0]);
	pr_debug("XCOPY_DBG: MaxCscdDescCount:0x%x\n",*(u16*)&pParams->MaxCscdDescCount[0]);
	pr_debug("XCOPY_DBG: MaxSegDescCount:0x%x\n",*(u16*)&pParams->MaxSegDescCount[0]);
	pr_debug("XCOPY_DBG: MaxDescListLen:0x%08x\n",*(u32*)&pParams->MaxDescListLen[0]);
	pr_debug("XCOPY_DBG: MaxSegLen:0x08%x\n",*(u32*)&pParams->MaxSegLen[0]);
	pr_debug("XCOPY_DBG: MaxInlineDataLen:0x%08x\n",*(u32*)&pParams->MaxInlineDataLen[0]);
	pr_debug("XCOPY_DBG: HeldDataLimit:0x%08x\n",*(u32*)&pParams->HeldDataLimit[0]);
	pr_debug("XCOPY_DBG: MaxStreamDevXfsSize:0x%08x\n",*(u32*)&pParams->MaxStreamDevXfsSize[0]);
	pr_debug("XCOPY_DBG: TotalConCurrentCpSize:0x%x\n",*(u16*)&pParams->TotalConCurrentCpSize[0]);
	pr_debug("XCOPY_DBG: MaxConCurrentCp:0x%x\n",pParams->MaxConCurrentCp);
	pr_debug("XCOPY_DBG: DataSegGranularityLog2:0x%x\n",pParams->DataSegGranularityLog2);
	pr_debug("XCOPY_DBG: InlineDataGranularityLog2:0x%x\n",pParams->InlineDataGranularityLog2);
	pr_debug("XCOPY_DBG: HeldDataGranularityLog2:0x%x\n",pParams->HeldDataGranularityLog2);
	pr_debug("XCOPY_DBG: ImplementedDescListLen:0x%x\n",pParams->ImplementedDescListLen);

	ret = 0;
_EXIT_:
	transport_kunmap_data_sg(pSeCmd);
	return ret;
}

/*
 * @fn int vaai_do_receive_xcopy_op_param_v1 (IN LIO_SE_TASK *pSeTask)
 * @brief Wrapper function to process Receive Copy Parameter command passed by target module
 *
 * @param[in] pSeTask
 * @retval 0 - Success
 * @retval Others - Fail
 */
int vaai_do_receive_xcopy_op_param_v1(
    IN LIO_SE_TASK *pSeTask
    )
{
    int CmdResult;

    BUG_ON(!pSeTask);
    BUG_ON(!pSeTask->task_se_cmd);

    CmdResult =  __do_receive_xcopy_op_param(pSeTask->task_se_cmd);
    return vaai_chk_and_complete(CmdResult, pSeTask);
}

/*
 * @fn int __do_copy_op_abort (IN LIO_SE_CMD *pSeCmd)
 * @brief Main function to process Copy Abort Command
 * @param[in] pSeCmd
 * @retval 0 - Success
 * @retval Others - Fail
 */
int __do_copy_op_abort(
    IN LIO_SE_CMD *pSeCmd
    )
{

    BUG_ON(!pSeCmd);

    /* FIXED !! FIXED !!
     *
     * (1) To create the copy cmd obj data
     * (2) To find the command we want to abort from copy obj root node list
     * (3) To send cmd action code to receiver 
     * (4) To check whether we got the response from receiver
     * (5) To wait the response for receiver ...
     */
    return 1;
}

/*
 * @fn int vaai_copy_op_abort_v1 (IN LIO_SE_TASK *pSeTask)
 * @brief Wrapper function to process Copy Abort Command passed by target module
 * @note LID1 means List Id is one byte
 * @param[in] pSeTask
 * @retval 0 - Success
 * @retval Others - Fail
 */
int vaai_copy_op_abort_v1(
    IN LIO_SE_TASK *pSeTask
    )
{
    int CmdResult;

    BUG_ON(!pSeTask);
    BUG_ON(!pSeTask->task_se_cmd);

    CmdResult =  __do_copy_op_abort(pSeTask->task_se_cmd);
    return vaai_chk_and_complete(CmdResult, pSeTask);
}

/*
 * @fn int __do_lid4_xcopy (IN LIO_SE_CMD *pSeCmd)
 * @brief Main function to process Extended Copy (LID4) Command
 * @note LID1 means List Id is 4 bytes
 * @param[in] pSeCmd
 * @retval 0 - Success
 * @retval Others - Fail
 */
int __do_lid4_xcopy(
    IN LIO_SE_CMD *pSeCmd
    )
{
    LID4_XCOPY_PARAMS *pParams = NULL;
    u8 *pu8SegDesc = NULL, *pu8CscdDesc = NULL;
    u8 u8SuccessToExec = FALSE;
    u32 u32TotalSegDescListLen = 0;
    int Ret;

//    DBG_PRINT("XCOPY_DBG: __do_lid4_xcopy()\n");

    BUG_ON(!pSeCmd);  
    pParams = (LID4_XCOPY_PARAMS *)transport_kmap_data_sg(pSeCmd);

    Ret = __chk_do_lid4_xcopy(pSeCmd, pParams);
    if (Ret == XCOPY_RET_NO_XFS_PASS){
        Ret = 0;
        goto _SUCCESS_EXIT_;
    }

    if (Ret == XCOPY_RET_FAIL){
        Ret = -EOPNOTSUPP;
        goto _ERROR_EXIT_;
    }

    pu8CscdDesc = (u8*)((size_t)pParams + sizeof(LID4_XCOPY_PARAMS));  
    pu8SegDesc  = (u8*)((size_t)pu8CscdDesc + get_unaligned_be16(&pParams->CscdDescListLen[0]));
    u32TotalSegDescListLen  = get_unaligned_be32(&pParams->SegDescListLen[0]);

    /* 
     * All error reason (suitable sense data) was set in internal xcopy function
     * already so here won't do anything ...
     */
    u8SuccessToExec = __go_xcopy_handler(
                            pSeCmd, 
                            pu8CscdDesc, 
                            pu8SegDesc, 
                            u32TotalSegDescListLen
                            );

    Ret = 0;
    if (u8SuccessToExec == FALSE){
        printk(KERN_DEBUG "[VAAI_DBG] XCOPY_DBG: %s - fail\n", __func__);
        Ret = -EIO;
        goto _ERROR_EXIT_;
    }

    /*
     * SPC4R36, page 276
     *
     * If G_SENSE is set to 1 and the command was completed with GOOD status, then
     * the copy manager should associate sense data with the GOOD status in which
     * SK is set to COMPLETED, the ASC is set to EXTENDED COPY INFORMATION AVAILABLE
     * and the COMMAND-SPECIFIC INFORMATION field is set to number of segment
     * descriptors the copy manager has processed
     */
    if (pParams->G_Sense == 0x1){
        /* Not implemented yet ... */
    }


_SUCCESS_EXIT_:
    DBG_PRINT("XCOPY_DBG: %s - pass\n", __func__);

_ERROR_EXIT_:
    transport_kunmap_data_sg(pSeCmd);
    return Ret;

}





