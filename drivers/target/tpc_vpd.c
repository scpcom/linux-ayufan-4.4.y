/**
 * @file 	tpc_vpd.c
 * @brief	This file contains the code for 3rd party copy vpd page 
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
#include "tpc_helper.h"


/* */
#if (SUPPORT_ROD_TOKEN_DESC_IN_TPC_VPD == 1)
static u16 __get_rod_dev_type_sepcific_desc_len(IN void);
static int __get_total_rod_token_feature_len(IN OUT u16 *len);
static int __build_rod_token_feature(IN LIO_SE_CMD *cmd, IN TPC_REC_DATA *data);
#endif

static u16 __get_blkdev_rod_limits_desc_len(IN void);
static int __get_total_blkdev_rod_limits_desc_len(IN OUT u16 *len);
static int __get_total_supported_cmds_len(IN OUT u16 *len);

#if 0
static u16 __get_rod_type_desc_len(IN void);
static int __get_total_params_len(IN OUT u16 *len);
static int __get_total_supported_descs_len(IN OUT u16 *len);
static int __get_total_supported_cscd_ids_len(IN OUT u16 *len);
static int __get_total_supported_rod_len(IN OUT u16 *len);
static int __get_total_stream_copy_op_len(IN OUT u16 *len);
static int __get_total_hold_data_len(IN OUT u16 *len);

static int __build_params(IN LIO_SE_CMD *cmd, IN TPC_REC_DATA *data);
static int __build_supported_descs(IN LIO_SE_CMD *cmd, IN TPC_REC_DATA *data);
static int __build_supported_cscd_ids(IN LIO_SE_CMD *cmd, IN TPC_REC_DATA *data);
static int __build_supported_rod(IN LIO_SE_CMD *cmd, IN TPC_REC_DATA *data);
static int __build_stream_copy_op(IN LIO_SE_CMD *cmd, IN TPC_REC_DATA *data);
static int __build_hold_data(IN LIO_SE_CMD *cmd, IN TPC_REC_DATA *data);
#endif

static int __get_total_gen_copy_op_len(IN OUT u16 *len);
static int __build_gen_copy_op(IN LIO_SE_CMD *cmd, IN TPC_REC_DATA *data);
static int __build_blkdev_rod_limits_desc(IN LIO_SE_CMD *cmd, IN TPC_REC_DATA *data);
static int __build_supported_cmds(IN LIO_SE_CMD *cmd, IN TPC_REC_DATA *data);

/** 
 * @brief  Function pointer table to build each 3rd party copy descriptor data
 */
BUILD_TPC_DESC gFuncTable[] = {

    {   
        TPC_DESC_BLOCK_DEV_ROD_LIMITS, 
        __get_total_blkdev_rod_limits_desc_len,
        __build_blkdev_rod_limits_desc 
    },

    /* This function is mandatory (SPC4R36, page 769) */
    {   
        TPC_DESC_SUPPORTED_CMDS , 
        __get_total_supported_cmds_len ,
        __build_supported_cmds 
    },


#if 0 // mark this cause of no way to verify this
    /*
     * This function shall be mandatory if the EXTENDED COPY (LID4) or
     * EXTENDED COPY (LID1) command is supported (SPC4R36, page 769)
     */
    {   
        TPC_DESC_PARAMETER_DATA , 
        __get_total_params_len ,
        __build_params 
    },  

    /*
     * This function shall be mandatory if the EXTENDED COPY (LID4) or
     * EXTENDED COPY (LID1) command is supported (SPC4R36, page 769)
     */
    {   
        TPC_DESC_SUPPORTED_DESCS , 
        __get_total_supported_descs_len ,
        __build_supported_descs 
    },

    /*
     * This function shall be mandatory if the EXTENDED COPY (LID4) or
     * EXTENDED COPY (LID1) command is supported (SPC4R36, page 769)
     */
    {   
        TPC_DESC_SUPPORTED_CSCD_IDS , 
        __get_total_supported_cscd_ids_len ,
        __build_supported_cscd_ids 
    },
#endif

#if (SUPPORT_ROD_TOKEN_DESC_IN_TPC_VPD == 1)
    /*
     * This function shall be mandatory if the extended copy command ROD CSCD
     * descriptor is supported (SPC4R36, page 769)
     */
    {   
        /* Currently, we only support the information for block device */
        TPC_DESC_ROD_TOKEN_FEATURES , 
        __get_total_rod_token_feature_len ,
        __build_rod_token_feature 
    },
#endif

#if 0 // mark this cause of no way to verify this
    /*
     * This function shall be mandatory if the extended copy command ROD CSCD
     * descriptor is supported (SPC4R36, page 769)
     */
    {   
        TPC_DESC_SUPPORTED_ROD , 
        __get_total_supported_rod_len ,
        __build_supported_rod 
    },
#endif

    /* This function is mandatory (SPC4R36, page 769) */
    {   
        TPC_DESC_GENERAL_COPY_OP ,
        __get_total_gen_copy_op_len ,
        __build_gen_copy_op 
    },

#if 0 // mark this cause of no way to verify this
    {
        TPC_DESC_STREAM_COPY_OP ,
        __get_total_stream_copy_op_len ,
        __build_stream_copy_op  
    },

    /*
     * This function shall be mandatory if the RECEIVE COPY DATA (LID4) or
     * RECEIVE COPY DATA (LID1) command is supported (SPC4R36, page 769)
     */
    {   
        TPC_DESC_HOLD_DATA , 
        __get_total_hold_data_len ,
        __build_hold_data 
    },
#endif

    {   
        MAX_TPC_DESC_TYPE , 
        NULL ,
        NULL
    },
};

/**/
#if (SUPPORT_ROD_TOKEN_DESC_IN_TPC_VPD == 1)
static u16 __get_rod_dev_type_sepcific_desc_len()
{
    return ROD_DEV_TYPE_SPECIFIC_DESC_LEN;
}
#endif

#if 0
static u16 __get_rod_type_desc_len()
{
    u16 total_len = 0;
    u8 index = 0;

    for (index = 0 ;; index++){
        if (gRodTypeTable[index].end_table == 1)
            break;

        total_len += ROD_TYPE_DESC_LEN;
    }

    return total_len;
}
#endif

static u16 __get_blkdev_rod_limits_desc_len()
{
    return BLK_DEV_ROD_TOKEN_LIMIT_DESC_LEN;
}

/*
 * @fn static int __get_total_blkdev_rod_limits_desc_len (IN OUT u16 *len)
 * @brief Get the length of descriptor we will build
 *
 * @param[in,out] pu16Len
 * @retval        0 - Success , Others - Fail
 */
static int __get_total_blkdev_rod_limits_desc_len(
    IN OUT u16 *len
    )
{
    u16 desc_len = 0;

    if (len == NULL)
        return 1;

    desc_len = __get_blkdev_rod_limits_desc_len();
    if (desc_len == 0){
        *len = 0;
        return 0;
    }

    pr_debug("%s - total length of descriptor:0x%x\n",__func__, desc_len);
    *len = desc_len;
    return 0;
}

/*
 * @fn static int __get_total_supported_cmds_len (IN OUT u16 *len)
 * @brief Get the length of descriptor we will build
 *
 * @param[in,out] len
 * @retval 0 - Success , Others - Fail
 */
static int __get_total_supported_cmds_len(
    IN OUT u16 *len
    )
{
    TPC_SAC *pTpcSacTable = NULL;
    u16 index0 = 0, index1 = 0;
    u16 total_len = 0, pad_len = 0;

    if (len == NULL)
        return 1;

    for (index0 = 0; index0 < MAX_TPC_CMD_INDEX; index0++){
        pTpcSacTable  = gTpcTable[index0].pSAC;
        BUG_ON(!pTpcSacTable);

        /* to count the u8Index1 value, it is the length for list of supported service actions */
        for (index1 = 0;; index1++){
            if ((pTpcSacTable[index1].u8SAC == 0xff) && (pTpcSacTable[index1].pProc == NULL))
                break;
        }
        index1 += 2;
        total_len += index1;
    }

    if (total_len == 0){
        *len = 0; /* report zero length if not found any supported value */
        return 0;
    }

    if ((total_len + 5) & (0x03)){
        /* Caculate the pad length */
        pad_len = (((((total_len+ 5) + 4) >> 2) << 2) - (total_len+ 5));
        total_len += pad_len;
    }

    pr_debug("%s: total length of descriptor:0x%x\n",__func__, (total_len + 5));
    *len = (total_len + 5);
    return 0;

}

#if 0
/*
 * @fn static int __get_total_params_len (IN OUT u16 *len)
 * @brief Get the length of descriptor we will build
 *
 * @param[in,out] len
 * @retval 0 - Success , Others - Fail
 */
static int __get_total_params_len(
    IN OUT u16 *len
    )
{
    u16 total_len = 32;

    if (len == NULL)
        return 1;

    if (total_len == 0){
        *len = 0; /* report zero length if not found any supported value */
        return 0;
    }

    pr_debug("%s: total length of descriptor:0x%x\n",__func__, total_len);
    *len =  total_len;
    return 0;
}

/*
 * @fn static int __get_total_supported_descs_len (IN OUT u16 *len)
 * @brief Get the length of descriptor we will build
 *
 * @param[in,out] len
 * @retval 0 - Success , Others - Fail
 */
static int __get_total_supported_descs_len(
    IN OUT u16 *len
    )
{
    u16 total_len = 0, pad_len = 0;

    if (len == NULL)
        return 1;

    total_len = __get_max_supported_cscd_desc_count();
    total_len += __get_max_supported_seg_desc_count();

    if (total_len == 0){
        *len = 0;
        return 0;
    }

    if ((total_len + 5) & (0x03)){
        /* Caculate the pad length */
        pad_len = (((((total_len+ 5) + 4) >> 2) << 2) - (total_len+ 5));
        total_len += pad_len;
    }

    pr_debug("%s: total length of descriptor:0x%x\n",__func__, (total_len + 5));
    *len = (total_len + 5);
    return 0;
}

/*
 * @fn static int __get_total_supported_cscd_ids_len (IN OUT u16 *len)
 * @brief Get the length of descriptor we will build
 *
 * @param[in,out] len
 * @retval 0 - Success, 1 - Fail
 */
static int __get_total_supported_cscd_ids_len(
    IN OUT u16 *len
    )
{
    u16 total_len = 0, pad_len = 0;
    u8 index = 0;

    if (len == NULL)
        return 1;

    for (index = 0;; index++){
        if(gSupportedCSCDIdsTable[index].u8IsEndTable == TRUE)
            break;
    }

    total_len  = (u16)index;
    if (total_len == 0){
        *len = 0; /* report zero length if not found any supported CSCD ID */
        return 0;
    }

    if ((total_len + 6) & (0x03)){
        /* Caculate the pad length */
        pad_len = (((((total_len+ 6) + 4) >> 2) << 2) - (total_len+ 6));
        total_len += pad_len;
    }

    pr_debug("%s: total length of descriptor:0x%x\n",__func__, (total_len + 6));
    *len = (total_len + 6);
    return 0;
}
#endif

#if (SUPPORT_ROD_TOKEN_DESC_IN_TPC_VPD == 1)

/*
 * @fn static int __get_total_rod_token_feature_len (IN OUT u16 *len)
 * @brief Get the length of descriptor we will build
 *
 * @param[in,out] len
 * @retval 0 - Success, 1 - Fail
 */
static int __get_total_rod_token_feature_len(
    IN OUT u16 *len
    )
{
    u16 total_len = 0, pad_len = 0;

    /* FIXED ME !! FIXED ME !! FIXED ME !!*/
    if (len == NULL)
        return 1;

    total_len = __get_rod_dev_type_sepcific_desc_len();
    if (total_len == 0){
        *len = 0;
        return 0;
    }

    if ((total_len + 48) & (0x03)){
        /* Caculate the pad length */
        pad_len = (((((total_len + 48) + 4) >> 2) << 2) - (total_len + 48));
        total_len += pad_len;
    }

    pr_debug("%s: total length of descriptor:0x%x\n",__func__, (total_len + 48));
    *len = (total_len + 48);
    return 0;
}
#endif

#if 0
/*
 * @fn static int __get_total_supported_rod_len (IN OUT u16 *len)
 * @brief Get the length of descriptor we will build
 *
 * @param[in,out] len
 * @retval 0 - Success, 1 - Fail
 */
static int __get_total_supported_rod_len(
    IN OUT u16 *len
    )
{
    u16 total_len = 0, pad_len = 0;

    if (len == NULL)
        return 1;

    total_len = __get_rod_type_desc_len();
    if (total_len == 0){
        *len = 0;
        return 0;
    }

    if ((total_len + 8) & (0x03)){
        /* Caculate the pad length */
        pad_len = (((((total_len + 8) + 4) >> 2) << 2) - (total_len + 8));
        total_len += pad_len;
    }

    pr_debug("%s: total length of descriptor:0x%x\n",__func__, (total_len + 8));
    *len = (total_len + 8); // (ROD_TYPE_DESC_LEN + 8)
    return 0;
}
#endif

/*
 * @fn static int __get_total_gen_copy_op_len (IN OUT u16 *len)
 * @brief Get the length of descriptor we will build
 *
 * @param[in,out] len
 * @retval 0 - Success, 1 - Fail
 */
static int __get_total_gen_copy_op_len(
    IN OUT u16 *len
    )
{
    u16 total_len = 0;

    if (len == NULL)
        return 1;

    if (total_len == 0){
        *len = 0;
        return 0;
    }

    pr_debug("%s: total length of descriptor:0x%x\n",__func__, total_len);
    *len = total_len;
    return 0;
}

#if 0
/*
 * @fn static int __get_total_stream_copy_op_len (IN OUT u16 *len)
 * @brief Get the length of descriptor we will build
 *
 * @param[in,out] len
 * @retval        0 - Success, 1 - Fail
 */
static int __get_total_stream_copy_op_len(
    IN OUT u16 *len
    )
{
    u16 total_len = 0;

    if (len == NULL)
        return 1;

    /* we don't suppert this */
    if (total_len == 0){
        *len = 0;
        return 0;
    }

    pr_debug("%s: total length of descriptor:0x%x\n",__func__, total_len);
    *len = total_len;
    return 0;
}

/*
 * @fn static int __get_total_hold_data_len (IN OUT u16 *len)
 * @brief Get the length of descriptor we will build
 *
 * @param[in,out] len
 * @retval 0 - Success, 1 - Fail
 */
static int __get_total_hold_data_len(
    IN OUT u16 *len
    )
{
    u16 total_len = 0;

    if (len == NULL)
        return 1;

    /* we don't suppert this */
    if (total_len == 0){
        *len = 0;
        return 0;
    }

    pr_debug("%s: total length of descriptor:0x%x\n",__func__, total_len);
    *len = total_len;
    return 0;
}
#endif


/*
 * @fn static int __build_blkdev_rod_limits_desc (IN LIO_SE_CMD *cmd, IN TPC_REC_DATA *data)
 * @brief Build block device ROD limits descriptor
 *
 * @param[in] cmd
 * @param[in] data
 * @retval 0 - Success , Others - Fail
 */
static int __build_blkdev_rod_limits_desc(
    IN LIO_SE_CMD *cmd,
    IN TPC_REC_DATA *data
    )
{
    u8 *pu8TpcDesc = NULL;
    u32 d_bs_order = 0;

    pr_debug("go to %s\n",__func__);

    if (cmd == NULL || data== NULL)
        return 1;

    d_bs_order = ilog2(cmd->se_dev->se_sub_dev->se_dev_attrib.block_size);
    pu8TpcDesc = data->pu8TpcDesc;

    put_unaligned_be16((u16)data->DescCode, &pu8TpcDesc[0]);
    put_unaligned_be16(0x0020, &pu8TpcDesc[2]);

    put_unaligned_be16(
                __tpc_get_max_supported_blk_dev_range(cmd),
                &pu8TpcDesc[10]
                );

    put_unaligned_be32(MAX_INACTIVITY_TIMEOUT, &pu8TpcDesc[12]);
    put_unaligned_be32(D4_INACTIVITY_TIMEOUT, &pu8TpcDesc[16]);

    /*
     * SBC3-R31, page-282
     *
     * MAX TOKEN TRANSFER SIZE indicates the max size in blocks that may be
     * specified by the sum of the NUMBER OF LOGICAL BLOCKS fileds in all block
     * device range descriptors of the POPULATE TOKEN command or 
     * WRITE USING TOKEN command
     *
     * a). If the MAX BYTES IN BLOCK ROD field in block ROD device type feature
     *     descriptor was reported, the MAX TOKEN TRANSFER SIZE fiedl shall be
     *     set to MAX BYTES IN BLOCK ROD field.
     *
     * b). If the OPTIMAL BYTES IN BLOCK ROD TRANSFER field in block ROD device
     *     type feature descriptor was reported, the OPTIMAL TRANSFER COUNT field 
     *     shall be set to OPTIMAL BYTES IN BLOCK ROD TRANSFER field.
     *
     */

    /* set the max transfer size and optimal transfer size */
    put_unaligned_be64((MAX_TRANSFER_SIZE_IN_BYTES >> d_bs_order), &pu8TpcDesc[20]);
    put_unaligned_be64((MAX_TRANSFER_SIZE_IN_BYTES >> d_bs_order), &pu8TpcDesc[28]);
    return 0;
}

/*
 * @fn static int __build_supported_cmds (IN LIO_SE_CMD *cmd, IN TPC_REC_DATA *data)
 * @brief Build supported command descriptor
 *
 * @param[in] cmd
 * @param[in] data
 * @retval 0 - Success , Others - Fail
 */
static int __build_supported_cmds(
    IN LIO_SE_CMD *cmd,
    IN TPC_REC_DATA *data
    )
{
    u8 *pu8CmdDesc = NULL, *pu8SAC = NULL;
    TPC_SAC *pTpcSacTable = NULL;
    u8 u8Index0 = 0, u8Index1 = 0;
    u16 u16Len = 0, u16PadLen = 0;
    u8 *pu8TpcDesc = NULL;

    pr_debug("go to %s\n",__func__);

    if (cmd == NULL || data== NULL)
        return 1;

    pu8TpcDesc = data->pu8TpcDesc;
    put_unaligned_be16((u16)data->DescCode, &pu8TpcDesc[0]);
    pu8CmdDesc = &pu8TpcDesc[5];
    pu8SAC     = &pu8CmdDesc[2];
    u16Len     = 0;

    for (u8Index0 = 0; u8Index0 < MAX_TPC_CMD_INDEX; u8Index0++){
        pu8CmdDesc[0] = gTpcTable[u8Index0].u8OpCode;
        pTpcSacTable  = gTpcTable[u8Index0].pSAC;
        BUG_ON(!pTpcSacTable);

        for (u8Index1 = 0;; u8Index1++){
            if ((pTpcSacTable[u8Index1].u8SAC == 0xff) && (pTpcSacTable[u8Index1].pProc == NULL))
                break;

            pu8SAC[u8Index1] = pTpcSacTable[u8Index1].u8SAC;
        }

        /*
         * (1) Fill the cmd supported service action code list length
         * (2) Record the current cmd supported desc length
         */
        pu8CmdDesc[1] = u8Index1;
        u16Len        += (u16)(pu8CmdDesc[1] + 2);

        /*
         * (1) Go to next cmd supported desc pos
         * (2) Go to next service action code list pos
         */
        pu8CmdDesc    += (size_t)(pu8CmdDesc[1] + 2);
        pu8SAC        = &pu8CmdDesc[2];
    }// end of (u8Index0 = 0; u8Index0 < MAX_TPC_CMD_INDEX; u8Index0++)

    pu8TpcDesc[4] = (u8)u16Len;  // fill the total cmd supported desc list length

    /* The supported commands for third-party descriptor should be a multiple of four */
    if ((u16Len + 5) & (0x03)){
        /* Caculate the pad length */
        u16PadLen = (((((u16Len+ 5) + 4) >> 2) << 2) - (u16Len+ 5));
        u16Len    += u16PadLen; /* u16Len is length for commands supported list plus pad length */
    }

    /* one byte for command supported list length */
    put_unaligned_be16((u16)(u16Len+1), &pu8TpcDesc[2]);
    return 0;
}

#if 0
/*
 * @fn static int __build_params (IN LIO_SE_CMD *cmd, IN TPC_REC_DATA *data)
 * @brief Build supported parameter data descriptor
 *
 * @param[in] cmd
 * @param[in] data
 * @retval 0 - Success , Others - Fail
 */
static int __build_params(
    IN LIO_SE_CMD *cmd,
    IN TPC_REC_DATA *data
    )
{
    u16 u16CscdDescCount = 0, u16SegDescCount = 0;
    u8 *pu8TpcDesc = NULL;

    pr_debug("go to %s\n",__func__);

    if (cmd == NULL || data== NULL)
        return 1;

    u16CscdDescCount = __get_max_supported_cscd_desc_count();
    if (u16CscdDescCount == 0)
        return 1;

    u16SegDescCount  = __get_max_supported_seg_desc_count();
    if (u16SegDescCount == 0)
        return 1;

    pu8TpcDesc = data->pu8TpcDesc;
    put_unaligned_be16((u16)data->DescCode, &pu8TpcDesc[0]);
    put_unaligned_be16(0x001c, &pu8TpcDesc[2]);
    put_unaligned_be16(u16CscdDescCount, &pu8TpcDesc[8]);
    put_unaligned_be16(u16SegDescCount, &pu8TpcDesc[10]);
    put_unaligned_be32(__get_total_supported_desc_len(), &pu8TpcDesc[12]);
    put_unaligned_be32(0x0, &pu8TpcDesc[16]);
    return 0;
}

/*
 * @fn static int __build_supported_descs (IN LIO_SE_CMD *cmd, IN TPC_REC_DATA *data)
 * @brief Build supported descriptors data
 *
 * @param[in] cmd
 * @param[in] data
 * @retval 0 - Success , Others - Fail
 */
static int __build_supported_descs(
    IN LIO_SE_CMD *cmd,
    IN TPC_REC_DATA *data
    )
{
    u16 u16Len = 0, u16PadLen = 0;
    u8 *pu8TpcDesc = NULL;

    pr_debug("go to %s\n",__func__);

    if (cmd == NULL || data== NULL)
        return 1;

    pu8TpcDesc = data->pu8TpcDesc;
    put_unaligned_be16((u16)data->DescCode, &pu8TpcDesc[0]);
    u16Len        = __fill_supported_desc_type_codes(&pu8TpcDesc[5]);
    pu8TpcDesc[4] = (u8)u16Len;

    /* The length of supported third-party descriptor should be a multiple of four */
    if ((u16Len + 5) & (0x03)){
        /* Caculate the pad length */
        u16PadLen = (((((u16Len+ 5) + 4) >> 2) << 2) - (u16Len+ 5));
        u16Len    += u16PadLen; /* u16Len is length for supported descriptor list plus pad length */
    }
    pr_debug("u16PadLen:0x%x, u16Len:0x%x\n",u16PadLen, u16Len);

    /* one byte for supported descriptor list length field */
    put_unaligned_be16((u16)(u16Len+1), &pu8TpcDesc[2]);
    return 0;
}

/*
 * @fn static int __build_supported_cscd_ids (IN LIO_SE_CMD *cmd, IN TPC_REC_DATA *data)
 * @brief Build supported CSCD ID data other than 0000h to 07ffh (table 133 in 6.4.5.1)
 *
 * @param[in] cmd
 * @param[in] data
 * @retval 0 - Success , Others - Fail
 */
static int __build_supported_cscd_ids(
    IN LIO_SE_CMD *cmd,
    IN TPC_REC_DATA *data
    )
{
    u16 u16Len = 0, u16PadLen = 0;
    u8 *pu8TpcDesc = NULL;

    pr_debug("go to %s\n",__func__);

    if (cmd == NULL || data== NULL)
        BUG_ON(TRUE);

    pu8TpcDesc = data->pu8TpcDesc;
    put_unaligned_be16((u16)data->DescCode, &pu8TpcDesc[0]);
    put_unaligned_be16((u16)u16Len, &pu8TpcDesc[4]);

    /* The length of supported third-party descriptor should be a multiple of four */
    if ((u16Len + 6) & (0x03)){
        /* Caculate the pad length */
        u16PadLen = (((((u16Len+ 6) + 4) >> 2) << 2) - (u16Len+ 6));
        u16Len    += u16PadLen; /* u16Len is length for supported descriptor list plus pad length */
    }

    /* two bytes for supported CSCD IDs list length field */
    put_unaligned_be16((u16)(u16Len + 2), &pu8TpcDesc[2]);
    return 0;
}
#endif

#if (SUPPORT_ROD_TOKEN_DESC_IN_TPC_VPD == 1)
/*
 * @fn static int __build_rod_token_feature (IN LIO_SE_CMD *cmd, IN TPC_REC_DATA *data)
 * @brief Build supported ROD token features data that indicates the limits of ROD tokens by copy operation
 *
 * @param[in] cmd
 * @param[in] data
 * @retval 0 - Success , Others - Fail
 */
static int __build_rod_token_feature(
    IN LIO_SE_CMD *cmd,
    IN TPC_REC_DATA *data
    )
{
	u16 u16Len = ROD_DEV_TYPE_SPECIFIC_DESC_LEN, u16PadLen = 0;
	ROD_DEV_TYPE_SPECIFIC_DESC *pDesc = NULL;
	LIO_SE_DEVICE *pSeDev = NULL;
	u32 d_bs_order = 0;
	u8 *pu8TpcDesc = NULL;

	COMPILE_ASSERT(sizeof(ROD_DEV_TYPE_SPECIFIC_DESC) == \
		ROD_DEV_TYPE_SPECIFIC_DESC_LEN);

	if (cmd == NULL || data== NULL)
		return 1;

	pu8TpcDesc = data->pu8TpcDesc;
	put_unaligned_be16((u16)data->DescCode, &pu8TpcDesc[0]);
	put_unaligned_be32(D4_INACTIVITY_TIMEOUT, &pu8TpcDesc[16]);
	put_unaligned_be32(MAX_INACTIVITY_TIMEOUT, &pu8TpcDesc[20]);
	put_unaligned_be32(MAX_INACTIVITY_TIMEOUT, &pu8TpcDesc[24]);
	put_unaligned_be16(u16Len, &pu8TpcDesc[46]);

	/*
	 * FIXED ME !! FIXED ME !!
	 *
	 * The remote_tokens filed indicates the level of support the copy
	 * manager provides for ROD tokens that are NOT created by the copy
	 * manager that is processing the copy operation
	 */
#if (R_TOKENS_CODE_6 == 1)
	pu8TpcDesc[4] |= 6;
#elif (R_TOKENS_CODE_4 == 1)
	pu8TpcDesc[4] |= 4;
#elif (R_TOKENS_CODE_0 == 1)
	pu8TpcDesc[4] |= 0;
#else
#error what are you doing !!??
#endif

	/* 
	 * FIXED ME !!
	 *
	 * start to build the block rod device type specific feature descriptor
	 * and these values shall be changed in the future ...
	 */
	pDesc = (ROD_DEV_TYPE_SPECIFIC_DESC *)&pu8TpcDesc[48];
	pSeDev = cmd->se_dev;
	d_bs_order = ilog2(pSeDev->se_sub_dev->se_dev_attrib.block_size);

	pDesc->u8DevType = pSeDev->transport->get_device_type(pSeDev);
	pDesc->u8DescFormat = 0;
	put_unaligned_be16(0x002c, &pDesc->u8DescLen[0]);

	put_unaligned_be16((OPTIMAL_BLK_ROD_LEN_GRANULARITY_IN_BYTES >> d_bs_order), 
		&pDesc->u8Byte4_7[2]);

	put_unaligned_be64(MAX_TRANSFER_SIZE_IN_BYTES, &pDesc->u8Byte8_15[0]);
	put_unaligned_be64(MAX_TRANSFER_SIZE_IN_BYTES, &pDesc->u8Byte16_23[0]);

	/* FIXED ME !! SPC4R36, page 779
	 *
	 * The SEGMENT means a single segment descriptor or single block device
	 * range descriptor
	 */
	put_unaligned_be64(OPTIMAL_TRANSFER_SIZE_IN_BYTES, &pDesc->u8Byte24_47[0]);
	put_unaligned_be64(OPTIMAL_TRANSFER_SIZE_IN_BYTES, &pDesc->u8Byte24_47[8]);

	/* The length should be a multiple of four */
	if ((u16Len + 48) & (0x03)){
		/* Caculate the pad length */
		u16PadLen = (((((u16Len + 48) + 4) >> 2) << 2) - (u16Len + 48));
		u16Len += u16PadLen;
	}

	put_unaligned_be16((u16)(u16Len + 44), &pu8TpcDesc[2]);
	return 0;
}
#endif

#if 0
/*
 * @fn static int __build_supported_rod (IN LIO_SE_CMD *cmd, IN TPC_REC_DATA *data)
 * @brief Build supported ROD token and ROD types
 *
 * @param[in] cmd
 * @param[in] data
 * @retval 0 - Success , Others - Fail
 */
static int __build_supported_rod(
    IN LIO_SE_CMD *cmd,
    IN TPC_REC_DATA *data
    )
{
    u16 u16Len = 0, u16PadLen = 0, u16Index = 0;
    ROD_TYPE_DESC *pDesc = NULL;
    u8 *p = NULL;

    pr_debug("go to %s\n",__func__);

    if (cmd == NULL || data== NULL)
        return 1;

    put_unaligned_be16((u16)data->DescCode, &data->pu8TpcDesc[0]);

    /* start to build the ROD type descriptor lists data */
    p = &data->pu8TpcDesc[8];
    for (u16Index = 0;; u16Index++){
        if (gRodTypeTable[u16Index].end_table == 1)
            break;

        pDesc = (ROD_TYPE_DESC *)p;

        put_unaligned_be32(gRodTypeTable[u16Index].rod_type, &pDesc->u8RodType[0]);
        pDesc->u8TokenOut               = (u8)gRodTypeTable[u16Index].token_out;
        pDesc->u8TokenIn                = (u8)gRodTypeTable[u16Index].token_in;
        pDesc->u8EcpyInt                = (u8)gRodTypeTable[u16Index].ecpy_int_bit;
        pDesc->u16PreferenceIndication  = (u16)gRodTypeTable[u16Index].preference_indication;

        u16Len += ROD_TYPE_DESC_LEN;
        p      += ROD_TYPE_DESC_LEN;
    }

    put_unaligned_be16(u16Len, &data->pu8TpcDesc[6]);

    /* The length should be a multiple of four */
    if ((u16Len + 8) & (0x03)){
        /* Caculate the pad length */
        u16PadLen = (((((u16Len + 8) + 4) >> 2) << 2) - (u16Len + 8));
        u16Len    += u16PadLen;
    }

    put_unaligned_be16((u16)(u16Len + 4), &data->pu8TpcDesc[2]);
    return 0;
}
#endif

/*
 * @fn static int __build_gen_copy_op (IN LIO_SE_CMD *cmd, IN TPC_REC_DATA *data) 
 * @brief Build general copy operations data
 *
 * @param[in] cmd
 * @param[in] data
 * @retval 0 - Success , Others - Fail
 */
static int __build_gen_copy_op(
    IN LIO_SE_CMD *cmd,
    IN TPC_REC_DATA *data
    )
{
    u8 *pu8TpcDesc = NULL;

    pr_debug("go to %s\n",__func__);

    if (cmd == NULL || data== NULL)
        BUG_ON(TRUE);

    pu8TpcDesc   = data->pu8TpcDesc;
    put_unaligned_be16((u16)data->DescCode, &pu8TpcDesc[0]);
    put_unaligned_be16(0x0020, &pu8TpcDesc[2]);

    /* FIXED ME !! These settings shall be for EXTENDED COPY command (Need to check) */

    /* Concurrent Copies - 
     *
     * Max number of 3rd-party copy commands that are supported for concurrent
     * processing by the copy manager
     */
    put_unaligned_be32(0x1, &pu8TpcDesc[4]);    // total concurrent copies

    /* Max Identified Concurrent Copies - 
     *
     * Max number of 3rd-party copy commands that are not an EXTENDED COPY
     * command with LIST ID USAGE (6.4.3.2) set to 11b that are supported for 
     * concurrent processing by the copy manager
     */
    put_unaligned_be32(0x1, &pu8TpcDesc[8]);    // max identified concurrent copies

    /* max segment length */
    put_unaligned_be32(OPTIMAL_TRANSFER_SIZE_IN_BYTES, &pu8TpcDesc[12]);
    pu8TpcDesc[16] = 22; // data segment granularity (log 2) (2 ^ 22 = 4MB)
    pu8TpcDesc[17] = 0x0; // inline data granularity (log 2)
    return 0;
}

#if 0
/*
 * @fn static int __build_stream_copy_op (IN LIO_SE_CMD *cmd, IN TPC_REC_DATA *data)  
 * @brief Build stream copy operations data
 *
 * @param[in] cmd
 * @param[in] data
 * @retval 0 - Success , Others - Fail
 */
static int __build_stream_copy_op(
    IN LIO_SE_CMD *cmd,
    IN TPC_REC_DATA *data
    )
{
    pr_debug("go to %s\n",__func__);

    if (cmd == NULL || data== NULL)
        return 1;

    /* we are not support stream copy operation */
    return 0;
}

/*
 * @fn static int __build_hold_data (IN LIO_SE_CMD *cmd, IN TPC_REC_DATA *data)   
 * @brief Build hold data features
 *
 * @param[in] cmd
 * @param[in] data
 * @retval 0 - Success , Others - Fail
 */
static int __build_hold_data(
    IN LIO_SE_CMD *cmd,
    IN TPC_REC_DATA *data
    )
{
    pr_debug("go to %s\n",__func__);

    if (cmd == NULL || data== NULL)
        return 1;

    return 0;
}
#endif

u16 __tpc_get_max_supported_blk_dev_range(
    IN LIO_SE_CMD *se_cmd
    )
{
    u32 count = (MAX_TRANSFER_SIZE_IN_BYTES / OPTIMAL_TRANSFER_SIZE_IN_BYTES);

    /* FIXED ME !!
     *
     * Here, The value of max supported blk device range can not be larger than
     * 0x400. The reason is the some testing items in Offload SCSI Compliance 
     * Test (LOGO) in WINDOWS HCK (ver: 8.59.29757) will be fail. Actually, we
     * can not find any information (or document) about the limitation of this.
     * Therefore, we limit it to 256MB / 4MB = 64 or 16MB / 4MB = 4. Please refer
     * the tpc_def.h file.
     */
    DBG_ROD_PRINT("max supported blk dev range:0x%x\n", count);
    return count;
}

/*
 * @fn int tpc_emulate_evpd_8f (IN LIO_SE_CMD *cmd, IN u8 *buffer)
 * @brief Function to prepare 3rd party copy vpd page data
 *
 * @param[in] cmd
 * @param[in] buffer
 * @retval 0 - Success , Others - Fail
 */
int tpc_emulate_evpd_8f(
    IN LIO_SE_CMD *cmd,
    IN u8 *buffer
    )
{
    u8 index = 0, *tpc_ptr = NULL;
    u16 total_len = 0, cur_len = 0, tmp = 0;
    TPC_REC_DATA data;
    int ret = 1;

    BUG_ON(!cmd);
    BUG_ON(!buffer);

    /* For the offload scsicompliance (LOGO) test in HCK, the allocation len will
     * be 0x1000 but the iscsi initiator only will give 0xff length. Actually,
     * the 0xff size is too small to return suitable data for third-party copy
     * command vpd 0x8f
     */ 
    pr_debug("%s: allocation len field:0x%x in inquiry cmd\n", 
        __func__, get_unaligned_be16(&cmd->t_task_cdb[3]));

    /* SPC4R36, page 767 */
    if (get_unaligned_be16(&cmd->t_task_cdb[3]) < 4)
        goto _ERR_INVALID_CDB_;
    
    buffer[0]  = cmd->se_dev->transport->get_device_type(cmd->se_dev);
    tpc_ptr    = &buffer[4];  // go to first third-party copy descriptor

    /* subtract 4 bytes first and remain bytes is for 3rd-party copy descriptor lists data */
    tmp        = get_unaligned_be16(&cmd->t_task_cdb[3]) - 4;

    /* 
     * (1) To caculate the total length we will build ... 
     */
    for (index = 0 ;; index++){
        if ((gFuncTable[index].DescCode == MAX_TPC_DESC_TYPE) && (gFuncTable[index].GetDescLen == NULL))
            break;

        cur_len = 0;
        ret      = gFuncTable[index].GetDescLen(&cur_len);
        if (ret == 1){
            printk(KERN_DEBUG "error to exec GetDescLen function:0x%x\n", index);
            goto _ERR_INVALID_CDB_;
        }

        total_len += cur_len;
    } // end of (u8Index = 0 ;; u8Index++)

    pr_debug("total len:0x%x\n", total_len);

    if (total_len > tmp){
        printk(KERN_DEBUG "%s: the amount of information length may execeed "
                "the ALLOCATION_LENGTH field. cdb alloc len:0x%x\n",
                __func__, get_unaligned_be16(&cmd->t_task_cdb[3])
                );
        put_unaligned_be16(total_len, &buffer[2]);
        goto _ERR_INVALID_CDB_;
    }


    /* 
     * (2) To start to build the data ... 
     */
    cur_len         = 0;
    total_len       = 0;
    data.DescCode   = MAX_TPC_DESC_TYPE;
    data.pu8TpcDesc = NULL;

    for (index = 0 ;; index++){
        if ((gFuncTable[index].DescCode == MAX_TPC_DESC_TYPE) && (gFuncTable[index].BuildDesc == NULL))
            break;

        /*
         * We did some error-condition checking in step 1 already. So here to
         * get length of descriptor directly. If the value is zero, not build the 
         * descriptor table.
         */
        cur_len = 0;
        gFuncTable[index].GetDescLen(&cur_len);

        if(cur_len == 0)
            continue;

        data.DescCode   = gFuncTable[index].DescCode;
        data.pu8TpcDesc = tpc_ptr;

        ret = gFuncTable[index].BuildDesc(cmd, &data);
        if (ret == 1){
            printk(KERN_DEBUG "error to exec BuildDesc function:0x%x\n", index);
            goto _ERR_INVALID_CDB_;
        }

        total_len += cur_len;
        tpc_ptr   += (size_t)cur_len;

        pr_debug("index:0x%x, tpc_ptr:0x%p, cur_len:0x%x\n",
                        index, tpc_ptr, cur_len);

    } // end of (u8Index = 0 ;; u8Index++)

    pr_debug("total_len:0x%x, cmd->data_length:0x%x\n", total_len, cmd->data_length);

    put_unaligned_be16(total_len, &buffer[2]);
//    dbg_dump_mem(buffer, cmd->data_length);
    return ret;


_ERR_INVALID_CDB_:
    __set_err_reason(ERR_INVALID_CDB_FIELD, &cmd->scsi_sense_reason);
    return 1;

}

