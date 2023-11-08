/**
 * @file 	vaai_xcopy_helper.c
 * @brief	This file contains the 3rd party copy command helper's code for VAAI
 *
 * @author	Adam Hsu
 * @date	2012/06/04
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/unaligned.h>
#include <scsi/scsi.h>
#include <target/target_core_base.h>
#include <target/target_core_backend.h>
#include <target/target_core_fabric.h>
#include "target_core_ua.h"
#include "target_core_iblock.h"

#include "vaai_target_struc.h"
#include "target_general.h"
#include "tpc_general.h"
#include "vaai_helper.h"

#if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_FAST_BLOCK_CLONE)
#include "target_fast_clone.h"
#endif

/* Jonathan Ho, 20140108, monitor VAAI */
#ifdef SHOW_OFFLOAD_STATS
extern u64 vaai_Xtotal;
#endif /* SHOW_OFFLOAD_STATS */

/* This is hard coding currently. Just to match the raid stripe size.
 * It is the same as ODX setting
 */
#define MAX_MEM_SIZE    (PAGE_SIZE * 16)

//
// prototype for xcopy helper function
//
static void * get_dev_from_desc_id(
    IN LIO_SE_CMD *pSeCmd,
    IN u8 *pu8CscdDesc,
    IN u16 u16Id
    );

static void * __get_dev_from_id_cscd_desc(
    IN LIO_SE_CMD *pSeCmd,
    IN u8 *pu8CscdDesc
    );

static void __get_dev_from_dt_03(
    IN LIO_SE_CMD *pSeCmd,
    IN u8 *pu8CscdDescPtr,
    IN void **ppRetDev
    );

static u8 __chk_valid_desc_id(
    IN u16 u16Id
    );

static int __chk_lid1_lid4_desc_data(
    IN LIO_SE_CMD *pSeCmd, 
    IN u8 *pu8CscdDesc, 
    IN u8 *pu8SegDesc, 
    IN u32 u32TotalCscdDescListLen, 
    IN u32 u32TotalSegDescListLen
    );
//
// prototype for main xcopy function
//
#if 0
static int __b2b_img_xcopy(
    IN LIO_SE_CMD *pSeCmd,
    IN u8 *pu8CurrentSegPtr,
    IN XCOPY_INFO *pXcopyInfo
    );

static int __b2b_hold_xcopy(
    IN LIO_SE_CMD *pSeCmd,
    IN u8 *pu8CurrentSegPtr,
    IN XCOPY_INFO *pXcopyInfo
    );

static int __b2b_off_xcopy(
    IN LIO_SE_CMD *pSeCmd,
    IN u8 *pu8CurrentSegPtr,
    IN XCOPY_INFO *pXcopyInfo
    );
#endif

static int __b2b_chk_desc_id(
    IN LIO_SE_CMD *pSeCmd,
    IN u8 *pu8SegDescPtr
    );

static int __b2b_info_collect(
    IN LIO_SE_CMD *pSeCmd,
    IN u8 *pu8CscdDescPtr, 
    IN u8 *pu8SegDescPtr 
    );

static int __b2b_xcopy(
    IN LIO_SE_CMD *pSeCmd,
    IN u8 *pu8CurrentSegPtr
    );

#if 0
static int __xcopy_verify_cscd(
    IN LIO_SE_CMD *pSeCmd,
    IN u8 *pu8CurrentSegPtr,
    IN XCOPY_INFO *pXcopyInfo
    );

static u8 __xcopy_verify_cscd(
    IN LIO_SE_CMD *pSeCmd,
    IN u8 *pu8CurrentSegPtr,
    IN XCOPY_INFO *pXcopyInfo
    );

static u8 __xcopy_verify_cscd_chk_desc_id(
    IN LIO_SE_CMD *pSeCmd,
    IN u8 *pu8SegDescPtr
    );

static u8 __xcopy_verify_cscd_info_collect(
    IN LIO_SE_CMD *pSeCmd,
    IN u8 *pu8CscdDescPtr, 
    IN u8 *pu8SegDescPtr, 
    IN XCOPY_INFO *pXcopyInfo
    );
#endif

static int __b2b_xcopy_do_rw(GEN_RW_TASK *task);
static int __do_core_xcopy(B2B_XCOPY_OBJ *obj);
static int __do_normal_b2b_xcopy(B2B_XCOPY_OBJ *obj, sector_t s_lba,
		sector_t d_lba, u64 *data_bytes);

//
//
//
GET_DEV_FROM_DT_HANDLER gGetDevFromDTFuncTable[] =
{
    { 0x00, NULL                 },  // Vendor specific
    { 0x01, NULL                 },  // t10 vendor ID based
    { 0x02, NULL                 },  // EUI-64 based
    { 0x03, __get_dev_from_dt_03 },  // NAA
    { 0x04, NULL                 },  // Relative target port identifier
    { 0x05, NULL                 },  // Target port group
    { 0x06, NULL                 },  // Logical unit group
    { 0x07, NULL                 },  // MD5 logical unit identifier
    { 0x08, NULL                 },  // SCSI name string
    { 0x09, NULL                 },  // PCIe routing ID
    { 0x0a, NULL                 },  // Reserved
    { 0x0b, NULL                 },  // Reserved
    { 0x0c, NULL                 },  // Reserved
    { 0x0d, NULL                 },  // Reserved
    { 0x0e, NULL                 },  // Reserved
    { 0x0f, NULL                 },  // Reserved
    { 0xff, NULL                 },  // End of table
};

/** 
 * @brief  Function pointer table for extended copy function
 */
XCOPY_HANDLER gXcopyFuncTable[] = 
{
    { COPY_BLK_TO_STREAM               , NULL, NULL, NULL},
    { COPY_STREAM_TO_BLK               , NULL, NULL, NULL},
    { COPY_BLK_TO_BLK                  ,
      __b2b_chk_desc_id, 
      __b2b_info_collect, 
      __b2b_xcopy 
    },

    { COPY_STREAM_TO_STREAM            , NULL, NULL, NULL},
    { COPY_INLINE_DATA_TO_STREAM       , NULL, NULL, NULL},
    { COPY_EMB_DATA_TO_STREAM          , NULL, NULL, NULL},
    { READ_STREAM_AND_DISCARD          , NULL, NULL, NULL},
    { VERIFY_CSCD                      , NULL, NULL, NULL},
    { COPY_BLK_OFF_TO_STREAM           , NULL, NULL, NULL},
    { COPY_STREAM_TO_BLK_OFF           , NULL, NULL, NULL},
    { COPY_BLK_OFF_TO_BLK_OFF          , NULL, NULL, NULL},
    { COPY_BLK_TO_STREAM_AND_HOLD      , NULL, NULL, NULL},
    { COPY_STREAM_TO_BLK_AND_HOLD      , NULL, NULL, NULL},
    { COPY_BLK_TO_BLK_AND_HOLD         , NULL, NULL, NULL},
    { COPY_STREAM_TO_STREAM_AND_HOLD   , NULL, NULL, NULL},
    { READ_STREAM_AND_HOLD             , NULL, NULL, NULL},
    { WRITE_FILEMARK_TO_SEQ_ACCESS_DEV , NULL, NULL, NULL},
    { SPACE_OR_FILEMARKS_SEQ_ACCESS_DEV, NULL, NULL, NULL},
    { LOCATE_SEQ_ACCESS_DEV            , NULL, NULL, NULL},
    { TAPE_IMAGE_COPY                  , NULL, NULL, NULL},
    { REG_PR_KEY                       , NULL, NULL, NULL},
    { THIRD_PARTY_PR_SRC               , NULL, NULL, NULL},
    { POPULATE_ROD_FROM_ONE_OR_MORE_BLK_RANGES , NULL, NULL, NULL},
    { POPULATE_ROD_FROM_ONE_BLK_RANGES , NULL, NULL, NULL},
    { BLK_IMAGE_COPY                   , NULL, NULL, NULL},
    { MAX_SEG_DESC_TYPE_CODE           , NULL, NULL, NULL}, // end of the table

};

/*
 * @fn static inline u8 __chk_valid_desc_id(IN u16 u16Id)
 * @brief Simple function to check valid descriptor id we supported
 *
 * @sa 
 * @param[in] u16Id
 * @retval TRUE or FALSE
 */
static inline u8 __chk_valid_desc_id(
    IN u16 u16Id
    )
{
    /*
     * FIXED ME !! FIXED ME !!
     *
     * Please refer the SPC4R36, page 300 and we don't support some CSCD
     * descriptor ID values here ...
     *
     * 0xc000 / 0xc001 : copy src or copy dest is a null logical unit whose 
     *                   device type is 0x00 (block) or 0x01 (stream)
     *
     * 0xffff          : copy src or copy dest is the logical that contains the
     *                   copy manager that is processing thr EXTENDED COPY command
     *                   (i.e. the logical unit to which the EXTENDED COPY command was sent)
     *
     * 0xf800          : copy src or copy dest is the logical unit specified by
     *                   the ROD token specified in the ROD CSCD descriptor that
     *                   has this value in its ROD PRODUCER CSCD DESCRIPTOR ID filed
     *
     * Ohters          : reserved
     */

    if (u16Id <= 0x07ff)
        return TRUE;

    return FALSE;

}

/*
 * @fn static void __get_dev_from_dt_03(IN LIO_SE_CMD *pSeCmd, IN u8 *pu8CscdDescPtr, IN void **ppRetDev)
 * @brief Get the suitable device ptr (struct se_device) from SCD descriptor ID
 *
 * @sa 
 * @param[in] task
 * @param[in] pu8CscdDescPtr    Pointer to the current CSCD descriptor
 * @param[in] ppRetDev  Pointer to be put the device ptr value
 * @retval NA
 */
static void __get_dev_from_dt_03
    (IN LIO_SE_CMD *pSeCmd, IN u8 *pu8CscdDescPtr, IN void **ppRetDev)
{
    LIO_SE_HBA *pHba = NULL, *pTmpHba = NULL;
    LIO_SE_DEVICE *pSeDev = NULL, *pTmpSeDev = NULL;
    ID_CSCD_DESC *pIdCscdDesc = (ID_CSCD_DESC *)pu8CscdDescPtr;
    struct list_head *pHbaList = NULL;
    spinlock_t *pHbaSpinLock = NULL;
    u8 pu8TmpBufPtr[20];
    u32 bFoundDev = FALSE;
    unsigned long Flag;

    if(pSeCmd == NULL || pu8CscdDescPtr == NULL || ppRetDev == NULL)
        BUG_ON(TRUE);

//  DBG_PRINT("XCOPY_DBG: pIdCscdDesc:0x%p\n", pIdCscdDesc);
//  dbg_dump_mem(pu8CscdDescPtr, 0x20);

    /*
     * FIXED ME !!!! FIXED ME !!!! FIXED ME !!!!
     *
     * Here to use hba_lock variable (please refer the target_core_hba.c) 
     */
    pHbaSpinLock = (spinlock_t *)vaai_get_hba_lock_var();
    pHbaList     = vaai_get_hba_list_var();

    spin_lock_irqsave(pHbaSpinLock, Flag);

    list_for_each_entry_safe(pHba, pTmpHba, pHbaList, hba_node){
        if (vaai_get_vritual_lun0_hba_var() == pHba)
            continue;

        spin_lock(&pHba->device_lock);

        list_for_each_entry_safe(pSeDev, pTmpSeDev, &pHba->hba_dev_list, dev_list){

            /*
             * FIXED ME !!
             *
             * Below code was copied from target_emulate_evpd_83(). Actually, 
             * it will be better to be programmed to sub-function.
             *
             * (a) Start NAA IEEE Registered Extended Identifier/Designator
             * (b) Use OpenFabrics IEEE Company ID: 00 14 05
             * (c) Return ConfigFS Unit Serial Number information for
             *     VENDOR_SPECIFIC_IDENTIFIER and VENDOR_SPECIFIC_IDENTIFIER_EXTENTION
             */
            memset(pu8TmpBufPtr, 0, pIdCscdDesc->DesignatorLen);

            if(!strcmp(pSeDev->se_sub_dev->se_dev_naa, "qnap")) {
                /* This code is for new firmware version (3.9.0) */
                pu8TmpBufPtr[0] = (0x6 << 4) | 0x0e;;
                pu8TmpBufPtr[1] = 0x84;
                pu8TmpBufPtr[2] = 0x3b;
                pu8TmpBufPtr[3] = (0x6 << 4);
            }
            else {
                pu8TmpBufPtr[0] = (0x6 << 4);
                pu8TmpBufPtr[1] = 0x01;
      	        pu8TmpBufPtr[2] = 0x40;
      	        pu8TmpBufPtr[3] = (0x5 << 4);
            }
            __get_target_parse_naa_6h_vendor_specific((void*)pSeDev, &pu8TmpBufPtr[3]);

//            dbg_dump_mem(pu8TmpBufPtr, pIdCscdDesc->DesignatorLen);
//            dbg_dump_mem(pIdCscdDesc->Designator, pIdCscdDesc->DesignatorLen);

            if (memcmp(pu8TmpBufPtr, pIdCscdDesc->Designator, pIdCscdDesc->DesignatorLen) == 0){
                bFoundDev = TRUE;
                spin_unlock(&pHba->device_lock);
                spin_unlock_irqrestore(pHbaSpinLock, Flag);
                goto _EXIT_LOOP_;
            }

        } // end of list_for_each_entry_safe(pSeDev, pTmpSeDev, &pHba->hba_dev_list, dev_list)

        spin_unlock(&pHba->device_lock);

    }// end of list_for_each_entry_safe(pHba, pTmpHba, pHbaList, hba_node)

    spin_unlock_irqrestore(pHbaSpinLock, Flag);

_EXIT_LOOP_:
	*ppRetDev = NULL;

	if (likely(bFoundDev == TRUE)) {
		pr_debug("%s: pSeDev:0x%p\n",__func__, pSeDev);
		*ppRetDev = (void*)pSeDev;
	}
	else
        	pr_debug("%s: Not found device!\n", __func__);

	return;
}

/*
 * @fn void *__get_dev_from_id_cscd_desc(IN LIO_SE_CMD *pSeCmd, IN u8 *pu8CscdDesc)
 * @brief Get the suitable device ptr (struct se_device) from SCD descriptor ID
 *
 * @sa 
 * @param[in] task
 * @param[in] pu8CscdDesc    Pointer to the current CSCD descriptor
 * @retval NULL or suitable device ptr
 */
static void *__get_dev_from_id_cscd_desc(IN LIO_SE_CMD *pSeCmd, IN u8 *pu8CscdDesc)
{
    ID_CSCD_DESC *pIdCscdDesc = (ID_CSCD_DESC *)pu8CscdDesc;
    void *pRetDev = NULL;
    u8 u8Index = 0;

    /* 
     * FIXED ME !!
     *
     * The ASSOCIATION field was defined in SPC4R36, page 733
     */
    switch(pIdCscdDesc->Assoc){
    case 0x0:
        /*
         * FIXED ME !!
         * There are ten designator type ... so here to parse which one we will use ..
         */
        for (u8Index = 0; 0xff != gGetDevFromDTFuncTable[u8Index].u8Type; u8Index++) {
            if ((gGetDevFromDTFuncTable[u8Index].u8Type == pIdCscdDesc->DesignatorType)
                && (gGetDevFromDTFuncTable[u8Index].GetDevExecFunc)) {
                gGetDevFromDTFuncTable[u8Index].GetDevExecFunc(pSeCmd, pu8CscdDesc, &pRetDev);
                return pRetDev;
            }
        }

        pr_err("[VAAI]%s: Cannot find execute function!\n", __func__);            
        break;

    case 0x1:
    case 0x2:
    default:
        pr_err("[VAAI]%s: ASSOCIATION (%u) is not supported!\n", 
                __func__, (unsigned int)pIdCscdDesc->Assoc);         
        break;
    }

    return NULL;
}

/*
 * @fn void *get_dev_from_desc_id(IN LIO_SE_CMD *pSeCmd, IN u8 *pu8CscdDesc, IN u16 u16Id)
 * @brief Get the suitable device ptr (struct se_device) from CSCD descriptor ID
 *
 * @sa 
 * @param[in] pSeCmd
 * @param[in] pu8CscdDesc    Pointer to the current CSCD descriptor
 * @param[in] u16Id D value
 * @retval NULL or suitable device ptr
 */
static void *get_dev_from_desc_id(IN LIO_SE_CMD *pSeCmd, IN u8 *pu8CscdDesc, IN u16 u16Id)
{
    u8 *p = pu8CscdDesc;
    void *pRetDev = NULL;

    /* Get real location for segment descriptor */
    while (u16Id) {
        p += (size_t)__get_cscd_desc_len(p);
        u16Id--;
    }
    /* Parse which cscd descriptor we will use ... */
    switch(p[0]){

    case ID_DESC:
        pRetDev = __get_dev_from_id_cscd_desc(pSeCmd, p);
        break;
    case FC_N_PORT_NAME_DESC:
    case FC_N_PORT_ID_DESC:
    case FC_N_PORT_ID_WITH_N_PORT_NAME_CHECK_CSCD:
    case PARALLEL_INTERFACE_DESC:
    case IPv4_DESC:
    case ALIAS_DESC:
    case RDMA_DESC:
    case IEEE_1394_DESC:
    case SAS_DESC:
    case IPv6_DESC:
    case IP_COPY_SERVICE_DESC:
    case ROD_DESC:
    default:
        pr_err("[XCOPY] Unsupported CSCD descriptor(%u)\n", (unsigned int)p[0]);
        break;
    }

    return pRetDev;

}

/*
 * @fn int __b2b_info_collect
 *              (IN LIO_SE_CMD *pSeCmd,IN u8 *pu8CscdDescPtr, IN u8 *pu8SegDescPtr)
 * @brief Collect the necessary information about xcopy function of block-to-block
 *
 * @sa 
 * @param[in] pSeCmd
 * @param[in] pu8CscdDescPtr    Pointer to the current CSCD descriptor
 * @param[in] pu8SegDescPtr    Pointer to the current segment descriptor
 * @retval  1- Success / Others - Fail
 */
static int __b2b_info_collect(
	LIO_SE_CMD *pSeCmd,
	u8 *pu8CscdDescPtr, 
	u8 *pu8SegDescPtr
	)
{
	LIO_SE_SUBSYSTEM_API *pSrcApi = NULL, *pDestApi = NULL;
	BLK_TO_BLK_SEG_DESC *pB2BSeg = NULL;
	GEN_CSCD_HDR *pGenCscdHdr = NULL;
	u8 *pCscdPtr = NULL;
	u8 u8Index= 0;
	u16 u16SrcId = 0, u16DestId = 0, u16Id = 0;
	u32 u32Size = 0;
	XCOPY_INFO *xcopy_info = NULL;
	B2B_REC_INFO *b2b_info = NULL;

	pB2BSeg   = (BLK_TO_BLK_SEG_DESC *)pu8SegDescPtr;
	pCscdPtr  = pu8CscdDescPtr;
	u16SrcId  = get_unaligned_be16(&pB2BSeg->SegDescHdr.SrcCscdDescId[0]);
	u16DestId = get_unaligned_be16(&pB2BSeg->SegDescHdr.DestCscdDescId[0]);

	xcopy_info = &pSeCmd->xcopy_info;
	xcopy_info->SegDescTypeCode = (u32)pB2BSeg->SegDescHdr.DescTypeCode;
	xcopy_info->pSrcSeDev = get_dev_from_desc_id(pSeCmd, pCscdPtr, u16SrcId);
	xcopy_info->pDestSeDev = get_dev_from_desc_id(pSeCmd, pCscdPtr, u16DestId);

	/*
	 * SPC4R36, section 6.4.6.1, page 300,
	 *
	 * To report CHECK CONDITION, SK = COPY ABORTED, ASC = UNREACHABLE COPY TARGET
	 * to upper layer if CSCD specificed by src, dest CSCD desc id field is not
	 * accessible to the copy manager ...
	 */
	if (!xcopy_info->pSrcSeDev || !xcopy_info->pDestSeDev) {
		if (!xcopy_info->pSrcSeDev || !xcopy_info->pDestSeDev){
			pr_debug("%s: either SrcSeDev(id:%u) or"
				" DestSeDev(id:%u) could not be found\n",
				__func__, (unsigned int)u16SrcId, 
				(unsigned int)u16DestId);
		} 
		__set_err_reason(ERR_UNREACHABLE_COPY_TARGET, 
			&pSeCmd->scsi_sense_reason);
		return 0;
	}

	if ((__do_get_subsystem_dev_type(xcopy_info->pSrcSeDev, 
		&xcopy_info->SrcSubSysType) != 0)
	|| (__do_get_subsystem_dev_type(xcopy_info->pDestSeDev, 
		&xcopy_info->DestSubSysType) != 0)
	)
	{
		pr_err("[XCOPY]: error!! coundn't find subsystem dev type "
			"for SRC or DEST\n");
		__set_err_reason(ERR_UNREACHABLE_COPY_TARGET, 
			&pSeCmd->scsi_sense_reason);
		return 0;
	}

	/*
	 * SPC4R36, section 5.17.7.4, page 262
	 *
	 * To report CHECK CONDITION, SK = COPY ABORTED, ASC = INCORRECT COPY 
	 * TARGET DEVICE TYPE if device type return by INQUIRY cmd doesn't match
	 * to the type in CSCD desc data
	 */
	pGenCscdHdr = (GEN_CSCD_HDR *)pu8CscdDescPtr;
	pSrcApi     = xcopy_info->pSrcSeDev->transport;
	pDestApi    = xcopy_info->pDestSeDev->transport;
	if ((pSrcApi->get_device_type(xcopy_info->pSrcSeDev) != (u32)pGenCscdHdr->DevType)
	|| (pDestApi->get_device_type(xcopy_info->pDestSeDev) != (u32)pGenCscdHdr->DevType)
	)
	{
		pr_err("[XCOPY]: error!! invalid device type for SRC or DEST\n");
		__set_err_reason(ERR_INCORRECT_COPY_TARGET_DEV_TYPE, 
			&pSeCmd->scsi_sense_reason);
		return 0;
	}

	/* create b2b_info */
	b2b_info = &xcopy_info->xcopy_main_data.b2b_rec_info;
	b2b_info->dc = pB2BSeg->SegDescHdr.DC;
	b2b_info->s_lba = get_unaligned_be64(&pB2BSeg->SrcBlkDevLba[0]);
	b2b_info->d_lba = get_unaligned_be64(&pB2BSeg->DestBlkDevLba[0]); 
	b2b_info->num_blks = (u32)get_unaligned_be16(&pB2BSeg->BlkDevNumBlks[0]);

	/* DEV_TYPE_PARAM always starts from byte 28 ~ byte 31 */
	for (u8Index = 0; u8Index < 2; u8Index++){
		pCscdPtr = pu8CscdDescPtr;
		if (u8Index == 0)
			u16Id = u16SrcId;
		else
			u16Id = u16DestId;

		while (u16Id){
			if (u16Id--){
				pCscdPtr += (size_t)__get_cscd_desc_len(pCscdPtr);
			}
		}

		u32Size = get_unaligned_be32(&pCscdPtr[28]);
		u32Size &= 0x00ffffff;  // data from byte 29 to byte 31 is we want

		if (u8Index == 0)
			b2b_info->s_bs_order = ilog2(u32Size);
		else
			b2b_info->d_bs_order = ilog2(u32Size);
	}// end of (u8Index = 0; u8Index < 2; u8Index++)

	return 1;

}

static int __b2b_xcopy_do_rw(
	GEN_RW_TASK *task
	)
{
	int ret = -1;
	SUBSYSTEM_TYPE subsys_type;
	
	if (!task)
	    return ret;
	
	/* try get the device backend type for this taks */
	if (__do_get_subsystem_dev_type(task->se_dev, &subsys_type))
	    return ret;
	
	if (subsys_type == SUBSYSTEM_BLOCK)
	    return __do_b_rw(task);
	else if (subsys_type == SUBSYSTEM_FILE)
	    return __do_f_rw(task);
	
	return -1;
}

static int __do_core_xcopy(
	B2B_XCOPY_OBJ *obj
	)
{
	
	u64 data_bytes = obj->data_bytes, e_bytes;
	sector_t s_lba = obj->s_lba, d_lba = obj->d_lba;
	int ret;

#if defined(CONFIG_MACH_QNAPTS)
#if defined(SUPPORT_FAST_BLOCK_CLONE)
	int is_create = 0, do_fbc = 0;
	FC_OBJ fc_obj;
	TBC_DESC_DATA tbc_desc_data;
#endif
#endif
	
	/**/
	while (data_bytes){

		e_bytes = data_bytes;

#if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_FAST_BLOCK_CLONE)


		if (!obj->s_se_dev->fast_blk_clone)
			goto _NORMAL_RW_;

		if (!obj->d_se_dev->fast_blk_clone)
			goto _NORMAL_RW_;


		if (!is_create){
			__create_fbc_obj(&fc_obj, obj->s_se_dev, 
				obj->d_se_dev, s_lba, d_lba, e_bytes);
			
			if (__check_s_d_lba_before_fbc(&fc_obj) == 0){
				if (__do_check_support_fbc(&fc_obj, 
					&tbc_desc_data) == 0)
					do_fbc = __do_update_fbc_data_bytes(
						&fc_obj, &tbc_desc_data,	
						d_lba, &e_bytes);
			}
			is_create = 1;
		} else 
			/* go further step since we created already */
			do_fbc = __do_update_fbc_data_bytes(
				&fc_obj, &tbc_desc_data, d_lba, &e_bytes);

		if (do_fbc){
			/* now, the s_lba, d_lba and e_bytes were aligned already */
			__create_fbc_obj(&fc_obj, obj->s_se_dev, obj->d_se_dev, 
				s_lba, d_lba, e_bytes);
			
			ret = __do_b2b_xcopy_by_fbc(obj, &fc_obj, &e_bytes);

			/* if it is successfull*/
			if (ret == 0)
				goto _GO_NEXT_;

			/* if found error try to use general read / write
			 * to do copy operation except the no-space event 
			 */
			if (ERR_NO_SPACE_WRITE_PROTECT == obj->err){
				ret = 0;
				goto _EXIT_;
			}

			pr_debug("[XCOPY] fail to execute "
				"fast-block-clone, try rollback to "
				"normal read/write, src lba:0x%llx, "
				"dest lba:0x%llx, bytes:0x%llx\n",
				(unsigned long long)s_lba, 
				(unsigned long long)d_lba, 
				(unsigned long long)e_bytes);

			goto _NORMAL_RW_;
		}

_NORMAL_RW_:
#endif
		ret = __do_normal_b2b_xcopy(obj, s_lba, d_lba, &e_bytes);
		if (ret != 0){
			/* exit if fail */
			ret = 0;
			goto _EXIT_;
		}
_GO_NEXT_:
		s_lba += (sector_t)(e_bytes >> obj->s_bs_order);
		d_lba += (sector_t)(e_bytes >> obj->d_bs_order);
		data_bytes -= e_bytes;
/* Jonathan Ho, 20140108, monitor VAAI */
#ifdef SHOW_OFFLOAD_STATS
		vaai_Xtotal += e_bytes >> 10; /* bytes to KB */
#endif /* SHOW_OFFLOAD_STATS */

	}

	/* everythin is fine */
	ret = 1;
_EXIT_:
	return ret;
}

static int __do_normal_b2b_xcopy(
	B2B_XCOPY_OBJ *obj,
	sector_t s_lba,
	sector_t d_lba,
	u64 *data_bytes
	)
{
	int r_done_blks = 0, w_done_blks = 0, exit_loop = 0, ret = 1;
	GEN_RW_TASK r_task, w_task;
	u64 e_bytes = 0;

	/* Prepare the read task */
	e_bytes = min_t(u64, *data_bytes, (u64)obj->sg_total_bytes);

	memset((void *)&r_task, 0, sizeof(GEN_RW_TASK));
	r_task.sg_list = obj->sg_list;
	r_task.sg_nents = obj->sg_nents;
	__make_rw_task(&r_task, obj->s_se_dev, s_lba,
		(e_bytes >> obj->s_bs_order), obj->timeout, DMA_FROM_DEVICE
		);

	/* To submit read */
	r_done_blks = __b2b_xcopy_do_rw(&r_task);
#if 0
	if (r_done_blks != (e_bytes >> obj->s_bs_order)){
		printk("err !! r_done_blks:0x%x, eb:0x%llx\n", r_done_blks,
			(unsigned long long)e_bytes);
	}
#endif

	if (r_done_blks <= 0 || r_task.is_timeout || r_task.ret_code != 0){
		pr_err("XCOPY: fail to read from copy source\n");
		obj->err = ERR_3RD_PARTY_DEVICE_FAILURE;
		exit_loop = 1;
		goto _RW_ERR_;
	}

	/* Prepare the write task */
	memset((void *)&w_task, 0, sizeof(GEN_RW_TASK));
	w_task.sg_list	 = r_task.sg_list;
	w_task.sg_nents  = r_task.sg_nents;
	__make_rw_task(&w_task, obj->d_se_dev, d_lba,
		(e_bytes >> obj->d_bs_order), obj->timeout, DMA_TO_DEVICE
		);

	/* To submit write */
	w_done_blks = __b2b_xcopy_do_rw(&w_task);

#if 0
	if (w_done_blks != (e_bytes >> obj->d_bs_order)){
		printk("err !! w_done_blks:0x%x, eb:0x%llx\n", w_done_blks,
			(unsigned long long)e_bytes);
	}
#endif

	if((w_done_blks <= 0) || w_task.is_timeout || w_task.ret_code != 0){
		pr_err("XCOPY: fail to write to copy destination\n");
		if (w_task.ret_code == -ENOSPC)
			obj->err = ERR_NO_SPACE_WRITE_PROTECT;
		else
			obj->err = ERR_3RD_PARTY_DEVICE_FAILURE;
		exit_loop = 1;
		goto _RW_ERR_;
	}
_RW_ERR_:
	
	/* To exit this loop if hit any error */
	if (r_task.is_timeout || w_task.is_timeout || exit_loop)
		goto _EXIT_;

	/* everythin is fine */
	ret = 0;
_EXIT_:
	*data_bytes = ((u64)w_done_blks << obj->d_bs_order);
	return ret;	
}



/*
 * @fn int __b2b_xcopy(IN LIO_SE_CMD *pSeCmd, IN u8 *pu8CurrentSegPtr)
 * @brief Do the xcopy function
 *
 * @sa 
 * @param[in] se_cmd
 * @param[in] curr_seg - Pointer to the current segment descriptor
 * @retval 1 - Success; Others - Fail
 */
static int __b2b_xcopy(
	LIO_SE_CMD *se_cmd, 
	u8 *curr_seg
	)
{
#define DEFAULT_ALLOC_SIZE	(1 << 20)

	u64 e_bytes;
	XCOPY_INFO *xcopy_info;
	B2B_REC_INFO *b2b_info;
	B2B_XCOPY_OBJ obj;
	int ret;

	/**/
	xcopy_info = &se_cmd->xcopy_info;
	b2b_info = &xcopy_info->xcopy_main_data.b2b_rec_info;

	obj.data_bytes = ((b2b_info->dc) ? 
		((u64)b2b_info->num_blks << b2b_info->d_bs_order) : 
		((u64)b2b_info->num_blks << b2b_info->s_bs_order));

	/* pre-allocate the mem */
	e_bytes = DEFAULT_ALLOC_SIZE;
	ret = __generic_alloc_sg_list(&e_bytes, &obj.sg_list, &obj.sg_nents);
	obj.sg_total_bytes = (u32)e_bytes;

	if (ret != 0){
		if (ret == -ENOMEM)
			pr_err("[b2b xcopy] fail to alloc sg list\n");
		if (ret == -EINVAL)
			pr_err("[b2b xcopy] invalid arg during to alloc sg list\n");
		__set_err_reason(ERR_3RD_PARTY_DEVICE_FAILURE, 
			&se_cmd->scsi_sense_reason);

		return 0;
	}

	/* create obj */
	obj.s_se_dev = xcopy_info->pSrcSeDev;
	obj.d_se_dev = xcopy_info->pDestSeDev;
	obj.s_lba = b2b_info->s_lba;
	obj.d_lba = b2b_info->d_lba;
	obj.s_bs_order = b2b_info->s_bs_order;
	obj.d_bs_order = b2b_info->d_bs_order;
	obj.err = MAX_ERR_REASON_INDEX;
	obj.timeout = jiffies + msecs_to_jiffies(XCOPY_TIMEOUT*1000);

#if 0
	pr_err("[xcopy] s_lba:0x%llx, d_lba:0x%llx, nr_blks:0x%x\n",
		(unsigned long long)b2b_info->s_lba,
		(unsigned long long)b2b_info->d_lba,
		b2b_info->num_blks
		);
#endif

	ret = __do_core_xcopy(&obj);



	__generic_free_sg_list(obj.sg_list, obj.sg_nents);

	if (ret == 0){
		if (obj.err != MAX_ERR_REASON_INDEX)
			__set_err_reason(obj.err, 
				&se_cmd->scsi_sense_reason);
	} else
		ret = 1;


_EXIT_:
	return ret;
}


#if 0
/*
 * @fn int __b2b_img_xcopy(IN LIO_SE_CMD *pSeCmd, IN u8 *pu8CurrentSegPtr, IN XCOPY_INFO *pXcopyInfo)
 * @brief Do the xcopy function of image-to-image
 *
 * @sa 
 * @param[in] pSeCmd
 * @param[in] pu8CurrentSegPtr    Pointer to the current segment descriptor
 * @param[in] pXcopyInfo Pointer to the XCOPY_INFO structure
 * @retval 0 - success; 1 - fail
 */
static int __b2b_img_xcopy
    (IN LIO_SE_CMD *pSeCmd, IN u8 *pu8CurrentSegPtr, IN XCOPY_INFO *pXcopyInfo)
{
    /* FIXED ME (not implemented yet) */
    DBG_PRINT("XCOPY_DBG: __b2b_img_xcopy()\n");
    return 1;
}
/*
 * @fn int __b2b_hold_xcopy(IN LIO_SE_CMD *pSeCmd, IN u8 *pu8CurrentSegPtr, IN XCOPY_INFO *pXcopyInfo)
 * @brief Do the xcopy function of block-to-block and hold data.
 *
 * @sa 
 * @param[in] pSeCmd
 * @param[in] pu8CurrentSegPtr    Pointer to the current segment descriptor
 * @param[in] pXcopyInfo Pointer to the XCOPY_INFO structure
 * @retval 0 - success; 1 - fail
 */
static int __b2b_hold_xcopy(IN LIO_SE_CMD *pSeCmd, IN u8 *pu8CurrentSegPtr, IN XCOPY_INFO *pXcopyInfo)
{
    /* FIXED ME (not implemented yet) */
    DBG_PRINT("XCOPY_DBG: xcopy_blk2blk_hold()\n");
    return 1;
}

/*
 * @fn static int __xcopy_verify_cscd(IN LIO_SE_CMD *pSeCmd, IN u8 *pu8CurrentSegPtr, IN XCOPY_INFO *pXcopyInfo)
 * @brief Do the xcopy function of verifying the CSCD
 *
 * @sa 
 * @param[in] pSeCmd
 * @param[in] pu8CurrentSegPtr    Pointer to the current segment descriptor
 * @param[in] pXcopyInfo Pointer to the XCOPY_INFO structure
 * @retval 0 - success; 1 - fail
 */
static int __xcopy_verify_cscd
        (IN LIO_SE_CMD *pSeCmd, IN u8 *pu8CurrentSegPtr, IN XCOPY_INFO *pXcopyInfo)
{
    /* FIXED ME (not implemented yet) */
    DBG_PRINT("XCOPY_DBG: __xcopy_verify_cscd()\n");
    return 1;
}

/*
 * @fn static int __b2b_off_xcopy(IN LIO_SE_CMD *pSeCmd, IN u8 *pu8CurrentSegPtr, IN XCOPY_INFO *pXcopyInfo)
 * @brief Do the xcopy function of block-offset to block-offset
 *
 * @sa 
 * @param[in] pSeCmd
 * @param[in] pu8CurrentSegPtr    Pointer to the current segment descriptor
 * @param[in] pXcopyInfo Pointer to the XCOPY_INFO structure
 * @retval 0 - success; 1 - fail
 */
static int __b2b_off_xcopy
        (IN LIO_SE_CMD *pSeCmd, IN u8 *pu8CurrentSegPtr, IN XCOPY_INFO *pXcopyInfo)
{
    /* FIXED ME (not implemented yet) */
    DBG_PRINT("XCOPY_DBG: xcopy_blk_off_2_blk_off()\n");
    return 1;
}
#endif

/*
 * @fn static int __b2b_chk_desc_id(IN LIO_SE_CMD *pSeCmd, IN u8 *pu8SegDescPtr)
 * @brief Check supported CSCD desc ID value in segment descriptor
 *
 * @sa 
 * @param[in] pSeCmd
 * @param[in] pu8SegDescPtr
 * @retval 1 - Success / Others - Fail
 */
static int __b2b_chk_desc_id(IN LIO_SE_CMD *pSeCmd, IN u8 *pu8SegDescPtr)
{
    GEN_SEG_DESC_HDR *p = NULL;
    u16 u16SrcId = 0, u16DestId = 0;

    /**/
    p         = (GEN_SEG_DESC_HDR *)pu8SegDescPtr;
    u16SrcId  = get_unaligned_be16(&p->SrcCscdDescId[0]);
    u16DestId = get_unaligned_be16(&p->DestCscdDescId[0]);

    /* Please refer the SPC4R36, page 300 */
    if ((__chk_valid_desc_id(u16SrcId) == TRUE) && (__chk_valid_desc_id(u16DestId) == TRUE))
        return 1;

    /*
     * SPC4R36, page 300,
     *
     * To report CHECK CONDITION, SK = COPY ABORTED, ASC = UNREACHABLE COPY TARGET
     * to upper layer if CSCD specificed by src, dest CSCD desc id field is not
     * accessible to the copy manager ...
     */
    pr_err("[VAAI] XCOPY_DBG: error !! %s:"
                      "found invalid CSCD desc id for SRC or DEST !!\n", __func__);
    __set_err_reason(ERR_UNREACHABLE_COPY_TARGET, &pSeCmd->scsi_sense_reason);
    return 0;

}


/*
 * @fn int __chk_lid1_lid4_desc_data(IN LIO_SE_CMD *pSeCmd, IN LID1_XCOPY_PARAMS *pXcopyParams)
 * @brief Check the descriptor data in parameter list of LID1 or LID4 copy command is valid or not
 *
 * @sa 
 * @param[in] pSeCmd
 * @param[in] pu8CscdDesc
 * @param[in] pu8SegDesc
 * @param[in] u32TotalCscdDescListLen
 * @param[in] u32TotalSegDescListLen
 * @retval XCOPY_RET_PASS or XCOPY_RET_FAIL
 */
static int __chk_lid1_lid4_desc_data(
	IN LIO_SE_CMD *pSeCmd, 
	IN u8 *pu8CscdDesc, 
	IN u8 *pu8SegDesc, 
	IN u32 u32TotalCscdDescListLen, 
	IN u32 u32TotalSegDescListLen
	)
{
	u8 *pu8TmpPtr = NULL;
	u32 u32Tmp = 0;
	u16 u16CscdDescCount = 0, u16SegDescCount = 0;
	int Ret = XCOPY_RET_FAIL;

	if(pSeCmd == NULL || pu8CscdDesc == NULL || pu8SegDesc == NULL
	|| u32TotalCscdDescListLen == 0 || u32TotalSegDescListLen == 0
	)
	{
		__set_err_reason(ERR_INVALID_PARAMETER_LIST, 
			&pSeCmd->scsi_sense_reason);
		goto _EXIT_;
	}

	/*
	 * SPC4R36, section 6.4.3.4 , 6.4.3.5 
	 *
	 * To report the CHECK CONDITION status with SK to ILLEGAL REQUEST and
	 * ASC to TOO MANY TARGET DESCRIPTORS / TOO MANY SEGMENT DESCRIPTORS / 
	 * PARAMETER LIST LENGTH ERROR if max value of CSCD desc count, 
	 * SEG desc count and max combined length of CDCD desc and SEG desc
	 * exceeds we support in this code
	 */

	/* To check CSCD desc count ... */
	pu8TmpPtr   = pu8CscdDesc;
	u32Tmp      = u32TotalCscdDescListLen;

	while (u32Tmp > 0){
		u16CscdDescCount++;
		u32Tmp -= __get_cscd_desc_len(pu8TmpPtr);
		pu8TmpPtr += (size_t)__get_cscd_desc_len(pu8TmpPtr);
	}

	if (u16CscdDescCount > __get_max_supported_cscd_desc_count()){
		pr_err("[VAAI_DBG] XCOPY_DBG: error !! total CSCD desc "
			"counts exceeds max supported counts !!\n");
		__set_err_reason(ERR_TOO_MANY_TARGET_DESCRIPTORS, 
			&pSeCmd->scsi_sense_reason);
		goto _EXIT_;
	}

	/* To check SEG desc count ... */
	pu8TmpPtr = pu8SegDesc;
	u32Tmp    = u32TotalSegDescListLen;
	while (u32Tmp > 0){
		u16SegDescCount++;
		u32Tmp -= __get_seg_desc_len(pu8TmpPtr);
		pu8TmpPtr += (size_t)__get_seg_desc_len(pu8TmpPtr);
	}

	if (u16SegDescCount > __get_max_supported_seg_desc_count()){
		pr_err("[VAAI_DBG] XCOPY_DBG: error !! total SEG desc counts "
			"exceeds max supported counts !!\n");
		__set_err_reason(ERR_TOO_MANY_SEGMENT_DESCRIPTORS, &pSeCmd->scsi_sense_reason);
		goto _EXIT_;
	}

	/* To check the total combined desc length for CSCD and SEG ... */
	if ((u32TotalCscdDescListLen + u32TotalSegDescListLen) > __get_total_supported_desc_len()){
		pr_err("[VAAI_DBG] XCOPY_DBG: error !! total combined desc "
			"length of CSCD and SEG is valid !!\n");
		__set_err_reason(ERR_PARAMETER_LIST_LEN_ERROR, &pSeCmd->scsi_sense_reason);
		goto _EXIT_;
	}

	/* To check the CSCD desc type ... */
	if (__chk_supported_cscd_type(pu8CscdDesc, u32TotalCscdDescListLen) == FALSE){
		pr_err("[VAAI_DBG] XCOPY_DBG: error !! found invalid CSCD desc type !!\n");
		__set_err_reason(ERR_INVALID_PARAMETER_LIST, &pSeCmd->scsi_sense_reason);
		goto _EXIT_;
	}

	/* To check the SEG desc type ... */
	if (__chk_supported_seg_type(pu8SegDesc, u32TotalSegDescListLen) == FALSE){
		pr_err("[VAAI_DBG] XCOPY_DBG: error !! found invalid SEG desc type !!\n");
		__set_err_reason(ERR_INVALID_PARAMETER_LIST, &pSeCmd->scsi_sense_reason);
		goto _EXIT_;
	}

	Ret = XCOPY_RET_PASS;

_EXIT_:
	return Ret;

}

/*
 * @fn int __chk_do_lid4_xcopy(IN LIO_SE_CMD *pSeCmd, IN LID4_XCOPY_PARAMS *pXcopyParams)
 * @brief Check whether the Extended Copy LID4 Command can be executed or not.
 *
 * @sa 
 * @param[in] pSeCmd    
 * @param[in] pLid4XcopyParams    
 * @retval XCOPY_RET_PASS or XCOPY_RET_NO_XFS_PASS or XCOPY_RET_FAIL
 */
int __chk_do_lid4_xcopy(
    IN LIO_SE_CMD *pSeCmd, 
    IN LID4_XCOPY_PARAMS *pXcopyParams
    )
{
    u16 u16TotalCscdDescListLen = 0, u16TotalSegDescListLen = 0, u16InlineDataLen = 0;
    u32 u32ParamLen = 0;
    u8 *pu8CscdDesc = NULL, *pu8SegDesc = NULL, *pu8InlineData = NULL;

    if (pSeCmd == NULL || pXcopyParams == NULL)
        BUG_ON(TRUE);

    u32ParamLen = get_unaligned_be32(&CMD_TO_TASK_CDB(pSeCmd)->t_task_cdb[10]);
    if (u32ParamLen == 0)
        return XCOPY_RET_NO_XFS_PASS; // don't need to treat as error if parameter len = 0

//    DBG_PRINT("XCOPY_DBG: %s, dump parameter list data ...\n", __func__);
//    dbg_dump_mem((u8 *)pLid4XcopyParams, (size_t)pSeCmd->data_length);

    /*
     * SPC4R36, page 277 (FIXED ME !!)
     *
     * The EXTENDED COPY(LID4) command shall be terminated with CHECK CONDITION
     * status with the SK is set to ILLEGAL REQUEST and the ASC is set to
     * INVALID FIELD IN CDB if the IMMED bit is set to 1 and
     *
     * a) the G_SENSE bit is set to 1 or
     * b) the LIST ID USAGE field is set to 11b
     *
     * Note:
     *
     * Immed bit is set to 1 means the copy operation should be a background behavior
     * so it should be treat as error if G_SENSE bit is 1.
     *
     */
    if ((pXcopyParams->Immed == 0x1)
    &&  ((pXcopyParams->G_Sense ==  0x1) || (pXcopyParams->ListIdUsage == 0x11))
    )
    {
        printk(KERN_DEBUG "[VAAI_DBG] XCOPY_DBG: "
                      "error !! G_SENSE and Immed are set to 0x1b and ListIdUsage is 0x11b\n");
        __set_err_reason(ERR_INVALID_CDB_FIELD, &pSeCmd->scsi_sense_reason);
        return XCOPY_RET_FAIL;
    }

    /* SPC4R36, page 276 */
    if ((pXcopyParams->ParamListFormat != 0x1) 
    ||  (get_unaligned_be16(&pXcopyParams->HdrCscdDescListLen[0]) != 0x0020)
    ||  (pXcopyParams->HdrCscdDescTypeCode != 0xff)    
    )
    {
        printk(KERN_DEBUG "[VAAI_DBG] XCOPY_DBG: error !! ParamListFormat is not 1"
                " or HdrCscdDescListLen is not 0x20 or HdrCscdDescTypeCode is not 0xff !!\n");
        __set_err_reason(ERR_INVALID_PARAMETER_LIST, &pSeCmd->scsi_sense_reason);
        return XCOPY_RET_FAIL;
    }

    u16TotalCscdDescListLen = get_unaligned_be16(&pXcopyParams->CscdDescListLen[0]);
    u16TotalSegDescListLen  = get_unaligned_be16(&pXcopyParams->SegDescListLen[0]);
    u16InlineDataLen        = get_unaligned_be16(&pXcopyParams->InlineDataLen[0]);

    if (u16TotalCscdDescListLen == 0 || u16TotalSegDescListLen == 0) {
        printk(KERN_DEBUG "[VAAI_DBG] XCOPY_DBG: "
                      "error !! total desc len of CSCD or SEG is zero !!\n");
        __set_err_reason(ERR_INVALID_PARAMETER_LIST, &pSeCmd->scsi_sense_reason);
        return XCOPY_RET_FAIL;
    }

    pu8CscdDesc = (u8*)((size_t)pXcopyParams + (size_t)(sizeof(LID4_XCOPY_PARAMS)));
    pu8SegDesc  = (u8*)((size_t)pu8CscdDesc + (size_t)u16TotalCscdDescListLen);

    if (u16InlineDataLen != 0){
        pu8InlineData = (u8*)((size_t)pu8SegDesc + (size_t)u16TotalSegDescListLen);
    }

    return __chk_lid1_lid4_desc_data(
            pSeCmd,
            pu8CscdDesc, 
            pu8SegDesc,
            (u32)u16TotalCscdDescListLen, 
            (u32)u16TotalSegDescListLen
            );

}

/*
 * @fn int __chk_do_lid1_xcopy(IN LIO_SE_CMD *pSeCmd, IN LID1_XCOPY_PARAMS *pLid1XcopyParams)
 * @brief Check whether the Extended Copy LID1 Command can be executed or not.
 *
 * @sa 
 * @param[in] pSeCmd    
 * @param[in] pLid1XcopyParams    
 * @retval XCOPY_RET_PASS or XCOPY_RET_NO_XFS_PASS or XCOPY_RET_FAIL
 */
int __chk_do_lid1_xcopy(IN LIO_SE_CMD *pSeCmd, IN LID1_XCOPY_PARAMS *pLid1XcopyParams)
{
	u16 u16TotalCscdDescListLen = 0;
	u32 u32TotalSegDescListLen = 0, u32InlineDataLen = 0, u32ParamLen = 0;
	u8 *pu8CscdDesc = NULL, *pu8SegDesc = NULL, *pu8InlineData = NULL;

	/**/
	u32ParamLen = get_unaligned_be32(&CMD_TO_TASK_CDB(pSeCmd)->t_task_cdb[10]);

	// don't need to treat as error if parameter len = 0
	if (u32ParamLen == 0)
		return XCOPY_RET_NO_XFS_PASS;

	u16TotalCscdDescListLen = get_unaligned_be16(&pLid1XcopyParams->CscdDescListLen[0]);
	u32TotalSegDescListLen  = get_unaligned_be32(&pLid1XcopyParams->SegDescListLen[0]);
	u32InlineDataLen        = get_unaligned_be32(&pLid1XcopyParams->InlineDataLen[0]);

	if (u16TotalCscdDescListLen == 0 || u32TotalSegDescListLen == 0) {
		pr_err("[VAAI_DBG] XCOPY_DBG: "
			"error !! total desc len of CSCD or SEG is zero !!\n");
		__set_err_reason(ERR_INVALID_PARAMETER_LIST, 
			&pSeCmd->scsi_sense_reason);
		return XCOPY_RET_FAIL;
	}

	pu8CscdDesc = (u8*)((size_t)pLid1XcopyParams + (size_t)(sizeof(LID1_XCOPY_PARAMS)));
	pu8SegDesc  = (u8*)((size_t)pu8CscdDesc + (size_t)u16TotalCscdDescListLen);

	if (u32InlineDataLen != 0)
		pu8InlineData = (u8*)((size_t)pu8SegDesc + (size_t)u32TotalSegDescListLen);


	return __chk_lid1_lid4_desc_data(pSeCmd, pu8CscdDesc, pu8SegDesc,
			(u32)u16TotalCscdDescListLen, u32TotalSegDescListLen);

}

