/*
 *
 * @file 	vaai_bp.c
 * @brief	This file contains the SCSI block provisioning management code
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
#include "tp_def.h"


/*
 * @fn int vaai_modesense_lbp (IN LIO_SE_CMD *pSeCmd, IN u8 *pu8Buff)
 * @brief Function to prepare modsense data for logical block provisioning function
 *
 * @param[in] pSeCmd
 * @param[in] pu8Buff
 * @retval Length of parameter data for logical block provisioning modsense command
 */
int vaai_modesense_lbp(
    IN LIO_SE_CMD *pSeCmd, 
    IN u8 *pu8Buff
    )
{
	THRESHOLD_DESC_FORMAT *pTDF = NULL;
	u16 u16Off = 16, u16PageLen = 0;
	struct se_device *dev = pSeCmd->se_dev;
	u32 threshold_count;
	/* get_blocks() api will get the last lba value */
	unsigned long long total_blocks = (dev->transport->get_blocks(dev) + 1);
	sector_t tmp_lba, tmp_blks;
	int err_ret = 0;
	u64 dividend;

	pr_debug("BP_DBG: Go to vaai_modesense_lbp()\n");

	/* FIXED !! If no any enough condition to prepare data, to return lenght to 0 */
	if ((pSeCmd->data_length == 0) || (u16Off > (u16)pSeCmd->data_length))
		return 0;

	if (!dev->se_sub_dev->se_dev_attrib.tp_threshold_set_size)
		return 0;

	tmp_blks = (sector_t)total_blocks;
	tmp_lba = 0; /* don't care tmp_lba value here */

#if defined(SUPPORT_LOGICAL_BLOCK_4KB_FROM_NAS_GUI)
	/* The block unit of kernel block i/o layer is 512b, so here to convert
	 * it if we are in 4KB environment */
	if (dev->se_sub_dev->se_dev_attrib.block_size != 0x200){
		err_ret = __blkio_transfer_task_lba_to_block_lba(
			dev->se_sub_dev->se_dev_attrib.block_size, &tmp_lba);
    
		if (err_ret != 0)
			return 0;
    
		tmp_blks *= (dev->se_sub_dev->se_dev_attrib.block_size >> 9);
	}
#endif

	/* Here to use div_u64() to make 64 bit division to avoid this code
	 * will be fail to build with 32bit compiler environment.
	 *
	 * (1) The unit is (2 ^ tp_threshold_set_size) blocks for threshold_count
	 * (2) tp_threshold_percent will be less than 100
	 */
	dividend = (tmp_blks * dev->se_sub_dev->se_dev_attrib.tp_threshold_percent);
	dividend = div_u64(dividend, 100);

	threshold_count = (u32)div_u64(dividend, 
		(1 << (dev->se_sub_dev->se_dev_attrib.tp_threshold_set_size)));

	dev->se_sub_dev->se_dev_attrib.tp_threshold_count = threshold_count;

	pu8Buff[0] = (0x1c | 0x40); /* set SPF bit (bit 6) to 1 */
	pu8Buff[1] = 0x02;

	/* FIXED ME !! */
	pu8Buff[4] = 0x01;  /* set the SITUA (single initiator threshold unit attention) bit */

	pTDF                        = (THRESHOLD_DESC_FORMAT *)&pu8Buff[u16Off];
	pTDF->u8ThresholdArming     = THRESHOLD_ARM_INC;
	pTDF->u8ThresholdType       = THRESHOLD_TYPE_SOFTWARE;
	pTDF->u8Enabled             = 1;
	pTDF->u8ThresholdResource   = LBP_LOG_PARAMS_USED_LBA_MAP_RES_COUNT; /* should be less than 0100h */

	/*    
	 * THRESHOLD COUNT field is the center of the threshold range for
	 * this threshold expressed as a number of threshold sets
	 */
	pTDF->u8ThresholdCount[0] = (threshold_count >> 24) & 0xff;
	pTDF->u8ThresholdCount[1] = (threshold_count >> 16) & 0xff;
	pTDF->u8ThresholdCount[2] = (threshold_count >> 8) & 0xff;
	pTDF->u8ThresholdCount[3] = threshold_count  & 0xff;

	u16PageLen = (sizeof(THRESHOLD_DESC_FORMAT) + 12);
	put_unaligned_be16(u16PageLen, &pu8Buff[2]);
	return (4 + u16PageLen);
}

/*
 * @fn void vaai_build_pg_desc (IN LIO_SE_CMD *pSeCmd, IN u8 *pu8Buff)
 * @brief Function to build the provisioning group descriptor
 *
 * @param[in] pSeCmd
 * @param[in] pu8Buff
 * @retval None
 */
void vaai_build_pg_desc(
    IN LIO_SE_CMD *pSeCmd,
    IN u8 *pu8buf
    )
{
//#define PG_DESC_LEN    (20)

    u8 u8Off = 8; // pg_desc information starts from offset 8

    pr_debug("BP_DBG: Go to vaai_build_pg_desc()\n");

//adam 20130123

    /* FIXED ME !!
     *
     * This code shall be modified in the future.
     */
    pu8buf[u8Off++] |= 0x01;        // CODE SET == Binary
    pu8buf[u8Off]   |= 0x00;        // ASSOCIATION == addressed logical unit: 0)b
    pu8buf[u8Off++] |= 0x03;        // Identifier/Designator type == NAA identifier
    u8Off++;
    pu8buf[u8Off++] = 0x10;         // Identifier/Designator length

    if(!strcmp(pSeCmd->se_dev->se_sub_dev->se_dev_naa, "qnap")) {
        pu8buf[u8Off++] = (0x6 << 4)| 0x0e;

        /* Use QNAP IEEE Company ID: */
        pu8buf[u8Off++] = 0x84;
        pu8buf[u8Off++] = 0x3b;
        pu8buf[u8Off] = (0x6 << 4);
    }    
    else {
        pu8buf[u8Off++] |= (0x6 << 4);  // NAA IEEE Registered Extended Identifier/Designator
        pu8buf[u8Off++] = 0x01;         // OpenFabrics IEEE Company ID: 00 14 05
        pu8buf[u8Off++] = 0x40;
        pu8buf[u8Off]   = (0x5 << 4);
    }
//adam 20130123

    __get_target_parse_naa_6h_vendor_specific(pSeCmd->se_dev, &pu8buf[u8Off]);
    return;
}

/*
 * @fn int vaai_logsense_lbp (IN LIO_SE_CMD *pSeCmd, IN u8 *pu8Buff)
 * @brief Function to prepare modsense data for logical block provisioning function
 *
 * @param[in] pSeCmd
 * @param[in] pu8Buff
 * @retval Length of parameter data for logical block provisioning modsense command
 */
int vaai_logsense_lbp(
    IN LIO_SE_CMD *pSeCmd, 
    IN u8 *pu8Buff
    )
{

    LBP_LOG_PARAMETER_FORMAT *pLLPF = NULL;
    u16 u16Off = 4, u16PageLen = 0;
    u32 avail = 0;
    u32 used = 0;

    /* 4bytes + 12bytes */
    if (pSeCmd->data_length < 16)
        return 0;

    used = pSeCmd->se_dev->se_sub_dev->se_dev_attrib.used;
    avail = pSeCmd->se_dev->se_sub_dev->se_dev_attrib.avail;

    /* Logical Block Provisioning log page Header (4-bytes) */
    pu8Buff[0] = (0x0c | 0x80); /* set SPF bit (bit 6) to 0, DS bit (bit 7) to 1 */
    pu8Buff[1] = 0x00;

    /* Available LBA Mapping Resource count log parameter format */
    pLLPF = (LBP_LOG_PARAMETER_FORMAT *)&pu8Buff[u16Off];
    pLLPF->ParameterCode[0] = (0x0001 >> 8) & 0xff;
    pLLPF->ParameterCode[1] = 0x0001 & 0xff;
    pLLPF->Du = 0;
    pLLPF->Tsd = 1;
    pLLPF->Etc = 0;
    pLLPF->Tmc = 0;
    pLLPF->FormatAndLinking = 3;
    pLLPF->ParameterLength = 0x8;
    pLLPF->ResourceCount[0] = (avail >> 24 ) & 0xff;
    pLLPF->ResourceCount[1] = (avail >> 16 ) & 0xff;
    pLLPF->ResourceCount[2] = (avail >> 8 ) & 0xff;
    pLLPF->ResourceCount[3] = avail  & 0xff;
    pLLPF->Scope = 1;

	/* Used LBA Mapping Resource count log parameter */
    u16Off += 12;
    pLLPF  = (LBP_LOG_PARAMETER_FORMAT *)&pu8Buff[u16Off];
    pLLPF->ParameterCode[0] = (0x0002 >> 8) & 0xff;
    pLLPF->ParameterCode[1] = 0x0002 & 0xff;
    pLLPF->Du = 0;
    pLLPF->Tsd = 1;
    pLLPF->Etc = 0;
    pLLPF->Tmc = 0;
    pLLPF->FormatAndLinking = 3;
    pLLPF->ParameterLength = 0x8;
    pLLPF->ResourceCount[0] = (used >> 24 ) & 0xff;
    pLLPF->ResourceCount[1] = (used >> 16 ) & 0xff;
    pLLPF->ResourceCount[2] = (used >> 8 ) & 0xff;
    pLLPF->ResourceCount[3] = used  & 0xff;
    pLLPF->Scope = 1;

    u16PageLen = (2 * sizeof(LBP_LOG_PARAMETER_FORMAT));
    put_unaligned_be16(u16PageLen, &pu8Buff[2]);
		
    return (4 + u16PageLen);
}


