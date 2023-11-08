/**
 * @file 	tpc_general.c
 * @brief	(1) This file contains the geneal code for 3rd party copy function
 *		(2) This file may be used by VAAI / ODX function
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

#if defined(SUPPORT_TPC_CMD)
#include "tpc_helper.h"
#endif
#if defined(SUPPORT_VAAI)
#include "vaai_helper.h"
#endif

/**/

/* @brief  3rd party copy command function table */
TPC_SAC gTpc83hSacTable[] = {
#if defined(SUPPORT_VAAI)
    { 0x00 , (void*)&vaai_do_lid1_xcopy_v1      }, // extended xcopy (LID1)
#endif
#if defined(SUPPORT_TPC_CMD)
    { 0x10 , (void*)&tpc_populate_token         }, // populate token
    { 0x11 , (void*)&tpc_write_by_token         }, // write using token
#endif
    { 0xff , NULL }, // end of table
};

/* @brief  3rd party copy command function table */
TPC_SAC gTpc84hSacTable[] = {
#if defined(SUPPORT_VAAI)
    { 0x03 , (void*)&vaai_do_receive_xcopy_op_param_v1  }, // receive xcopy op parameter
#endif
#if defined(SUPPORT_TPC_CMD)
    { 0x07 , (void*)&tpc_receive_rod_token_info         }, // receive rod token information
#endif
    { 0xff , NULL },  // end of table
};

TPC_CMD gTpcTable[MAX_TPC_CMD_INDEX] = {
	{ EXTENDED_COPY        , gTpc83hSacTable},
	{ RECEIVE_COPY_RESULTS , gTpc84hSacTable},
};


/** 
 * @brief  CSCD supported table
 */
CSCD_DESC_TYPE_CODE  gCscdSupportedTable[] ={
    /* The type code should be ordered here */
    ID_DESC,
    MAX_CSCD_DESC_TYPE_CODE, // end of table
};

/** 
 * @brief  SEGMENT supported table
 */
SEG_DESC_TYPE_CODE   gSegSupportedTable[] = {
    /* The type code should be ordered here */
    COPY_BLK_TO_BLK,
    MAX_SEG_DESC_TYPE_CODE, // end of table
};



/**/
u8 __is_my_cscd_desc_type(
  IN u8 *pu8CscdLoc
  )
{
  u8 u8Index = 0;

  for(u8Index = 0;; u8Index++){
    if(gCscdSupportedTable[u8Index] == MAX_CSCD_DESC_TYPE_CODE)
      break; 

    if(gCscdSupportedTable[u8Index] == pu8CscdLoc[0])
      return TRUE;
  }

  return FALSE;
}

/*
 * @fn u32 __get_cscd_desc_len(IN u8 *pu8CscdLoc)
 * @brief Simple function to get the CSCD descriptor size
 *
 * @sa 
 * @param[in] pu8CscdLoc    Pointer to the current CSCD descriptor
 * @retval u32 size for CSCD descriptor size
 */
u32 __get_cscd_desc_len(
    IN u8 *pu8CscdLoc
    )
{
    u32 u32Len = 0;

    /*
     * FIXED ME !! FIXED ME !! FIXED ME !!
     *
     * Shall we use hard-code here ????  How about to use sizeof (STRUCTURE_NAME) ????
     */
    switch(pu8CscdLoc[0]){

    case FC_N_PORT_NAME_DESC:
    case FC_N_PORT_ID_DESC:
    case FC_N_PORT_ID_WITH_N_PORT_NAME_CHECK_CSCD:
    case PARALLEL_INTERFACE_DESC:
    case ID_DESC:
    case IPv4_DESC:
    case ALIAS_DESC:
    case RDMA_DESC:
    case IEEE_1394_DESC:
    case SAS_DESC:
    case ROD_DESC:
        u32Len= 32;
        break;

    case IPv6_DESC:
    case IP_COPY_SERVICE_DESC:
        u32Len= 64;
        break;

    default:
        break;
    }

    return u32Len;
}

/*
 * @fn u8 __is_my_seg_desc_type (IN u8 *pu8SegLoc)
 * @brief Function to check whether current passed SEGMENT descriptor was supported or not
 *
 * @sa 
 * @param[in] pu8SegLoc
 * @retval TRUE or FALSE
 */
u8 __is_my_seg_desc_type(
    IN u8 *pu8SegLoc
    )
{
    u8 u8Index = 0;

    for (u8Index = 0;; u8Index++){
        if (gSegSupportedTable[u8Index] == MAX_SEG_DESC_TYPE_CODE)
            break; 

        if (gSegSupportedTable[u8Index] == pu8SegLoc[0])
            return TRUE;
    }
    return FALSE;
}

/*
 * @fn u32 __get_seg_desc_len (IN u8 *pu8SegLoc)
 * @brief Function to get length of specific SEGMENT descriptor
 *
 * @param[in] pu8SegLoc
 * @retval Length of specific SEGMENT descriptor
 */
u32 __get_seg_desc_len(
    IN u8 *pu8SegLoc
    )
{
    u32 u32Len = 0;

    /*
     * FIXED ME !! FIXED ME !!
     * We need to check these again ...
     */
    switch(pu8SegLoc[0]){

    case COPY_BLK_TO_STREAM:
    case COPY_STREAM_TO_BLK:
    case COPY_BLK_TO_STREAM_AND_HOLD:
    case COPY_STREAM_TO_BLK_AND_HOLD:
        u32Len = sizeof(BLK_STREAM_SEG_DESC);   // 24 bytes
        break;

    case COPY_BLK_TO_BLK:
    case COPY_BLK_TO_BLK_AND_HOLD:
        u32Len = sizeof(BLK_TO_BLK_SEG_DESC);   // 28 bytes
        break;

    case COPY_STREAM_TO_STREAM:
    case COPY_STREAM_TO_STREAM_AND_HOLD:
        u32Len =  sizeof(STREAM_TO_STREAM_SEG_DESC);   // 20 bytes
        break;

    case COPY_INLINE_DATA_TO_STREAM:
        u32Len = sizeof(INLINE_DATA_TO_STREAM_SEG_DESC); // 20 bytes;
        break;

    case READ_STREAM_AND_DISCARD:
    case READ_STREAM_AND_HOLD:
        u32Len = sizeof(STREAM_DISCARD_SEG_DESC);   // 16 bytes
        break;

    case VERIFY_CSCD:
        u32Len = sizeof(VERIFY_CSCD_SEG_DESC);   // 12 bytes
        break;

    case COPY_BLK_OFF_TO_STREAM:
    case COPY_STREAM_TO_BLK_OFF:
        u32Len = sizeof(BLK_STREAM_OFF_SEG_DESC);   // 28 bytes
        break;

    case COPY_BLK_OFF_TO_BLK_OFF:
        u32Len = sizeof(BLK_OFF_TO_BLK_OFF_SEG_DESC);   // 32 bytes
        break;

    case WRITE_FILEMARK_TO_SEQ_ACCESS_DEV:
        u32Len = sizeof(WRITE_FILEMARKS_SEG_DESC);   // 12 bytes
        break;

    case SPACE_OR_FILEMARKS_SEQ_ACCESS_DEV:
        u32Len = sizeof(SPACE_SEG_DESC);   // 12 bytes
        break;

    case LOCATE_SEQ_ACCESS_DEV:
        u32Len = sizeof(LOCATE_SEG_DESC);   // 12 bytes
        break;

    case TAPE_IMAGE_COPY:
        u32Len = sizeof(TAPE_IMAGE_COPY_SEG_DESC);   // 12 bytes
        break;

    case REG_PR_KEY:
        u32Len = sizeof(REG_PR_KEY_SEG_DESC);   // 28 bytes
        break;

    case BLK_IMAGE_COPY:
        u32Len = sizeof(BLK_IMAGE_COPY_SEG_DESC);   // 28 bytes
        break;

    case POPULATE_ROD_FROM_ONE_BLK_RANGES:
        u32Len = sizeof(POPULATE_ROD_FROM_ONE_BLK_SEG_DESC);   // 20 bytes
        break;

    case POPULATE_ROD_FROM_ONE_OR_MORE_BLK_RANGES:
    case COPY_EMB_DATA_TO_STREAM:
    case THIRD_PARTY_PR_SRC:
        u32Len = (u32)(get_unaligned_be16(&pu8SegLoc[2]) + 4);
        break;

    default:
        break;
    }

    return u32Len;
}

u16 __get_max_supported_cscd_desc_count(void)
{
	return MAX_CSCD_DESC_COUNT;
}


u16 __get_max_supported_seg_desc_count(void)
{
	return MAX_SEG_DESC_COUNT;
}


/*
 * @fn u32 __get_total_supported_desc_len(void)
 * @brief Function to get total length of supported CSCD / SEGMENT descriptors
 * @param[in] None
 * @retval Total length of supported CSCD / SEGMENT descriptors
 */
u32 __get_total_supported_desc_len(void)
{
    u16 u16CscdDescCount = 0, u16SegDescCount = 0;

    /* FIXED ME */
    u16CscdDescCount = __get_max_supported_cscd_desc_count();
    u16SegDescCount  = __get_max_supported_seg_desc_count();

    return ((u32)u16CscdDescCount * sizeof(GEN_CSCD_DESC) + \
                (u32)u16SegDescCount * sizeof(GEN_SEG_DESC));
}


/*
 * @fn u16 __fill_supported_desc_type_codes (IN u8 *pu8StartLoc)
 * @brief Function to fill the supported descriptor type code into specific location
 *
 * @note This function will be called in the __do_receive_xcopy_op_param() currently
 * @param[in] pu8StartLoc
 * @retval Total counts to be filled successfully
 */
u16 __fill_supported_desc_type_codes(
    IN u8 *pu8StartLoc
    )
{
    u16 u16Count = 0, u16Index = 0;

    /* 
     * SPC4R36, page 773,
     *
     * The unique supported value in each byte shall appear in the list in
     * ascending numerical order
     */
    for (u16Index = 0;; u16Index++){
        if (gSegSupportedTable[u16Index] == MAX_SEG_DESC_TYPE_CODE)
            break;
        pu8StartLoc[u16Count++] = gSegSupportedTable[u16Index];
    }

    for (u16Index = 0;; u16Index++){
        if (gCscdSupportedTable[u16Index] == MAX_CSCD_DESC_TYPE_CODE)
            break;
        pu8StartLoc[u16Count++] = gCscdSupportedTable[u16Index];
    }

    return u16Count;
}


/*
 * @fn u8 __chk_supported_cscd_type(IN u8  *pu8CscdLoc, IN u32 u32CscdDescListLen)
 * @brief Check supported CSCD type code
 *
 * @sa 
 * @param[in] pu8CscdLoc    Pointer to the current CSCD location
 * @param[in] u32CscdDescListLen    Total CSCD descriptior list length
 * @retval TRUE or FALSE
 */
u8 __chk_supported_cscd_type(IN u8  *pu8CscdLoc, IN u32 u32CscdDescListLen)
{
    u8 *p = NULL;

    if ((pu8CscdLoc == NULL) || (u32CscdDescListLen == 0))
        return FALSE;

    /*  To check all CSCD type code here ... */
    p = pu8CscdLoc;
    while (u32CscdDescListLen > 0){
        if (__is_my_cscd_desc_type(p) == FALSE){
            return FALSE;
        }
        u32CscdDescListLen -= __get_cscd_desc_len(p);
        p += (size_t)__get_cscd_desc_len(p);
    }
    return TRUE;
}

/*
 * @fn u8 __chk_supported_seg_type(IN u8  *pu8SegLoc, IN u32 u32SegDescListLen)
 * @brief Check supported CSCD type code
 *
 * @sa 
 * @param[in] pu8SegLoc    Pointer to the current segment location
 * @param[in] u32SegDescListLen    Total segment descriptior list length
 * @retval TRUE or FALSE
 */
u8 __chk_supported_seg_type(IN u8  *pu8SegLoc, IN u32 u32SegDescListLen)
{
    u8 *p = NULL;

    if ((pu8SegLoc == NULL) || (u32SegDescListLen == 0))
        return FALSE;

    /*  To check all segment type code here ... */
    p = pu8SegLoc;
    while (u32SegDescListLen > 0){
        if (__is_my_seg_desc_type(p) == FALSE){
            return FALSE;
        }
        u32SegDescListLen -= __get_seg_desc_len(p);
        p += (size_t)__get_seg_desc_len(p);
    }
    return TRUE;
}


