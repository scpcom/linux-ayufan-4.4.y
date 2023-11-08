/**
 * @file 	tpc_general.h
 * @brief	(1) This header file contains the geneal code for 3rd party copy function
 *		(2) This file may be used by VAAI / ODX function
 *
 * @author	Adam Hsu
 * @date	2012/06/04
 */
#ifndef _TPC_GENERAL_H_
#define _TPC_GENERAL_H_

#include <linux/types.h>
#include <linux/kernel.h>
#include <scsi/scsi.h>
#include "vaai_target_struc.h"
#include "target_general.h"
#include "tpc_def.h"

/**/

/* one cp src and one cp target per one copy command */
#define	MAX_CSCD_DESC_COUNT	2
/* one action per one copy command */
#define	MAX_SEG_DESC_COUNT	1

/**/
u8 __is_my_cscd_desc_type(IN u8 *pu8CscdLoc);
u8 __is_my_seg_desc_type(IN u8 *pu8SegLoc);
u32 __get_cscd_desc_len(IN u8 *pu8CscdLoc);
u32 __get_seg_desc_len(IN u8 *pu8SegLoc);
u32 __get_total_supported_desc_len(void);
u16 __fill_supported_desc_type_codes(IN u8 *pu8StartLoc);
u16 __get_max_supported_cscd_desc_count(void);
u16 __get_max_supported_seg_desc_count(void);
u8 __chk_supported_cscd_type(IN u8  *pu8CscdLoc, IN u32 u32CscdDescListLen);
u8 __chk_supported_seg_type(IN u8  *pu8SegLoc, IN u32 u32SegDescListLen);


#endif /* _TPC_GENERAL_H_ */

