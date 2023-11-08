/**
 *
 * @file	tp_def.h
 * @brief	Basic strcuture declaration header file for thin-provisioning command
 * @author
 * @date
 *
 */
#ifndef __TP_DEF_H__
#define __TP_DEF_H__

#include <linux/types.h>
#include <linux/kernel.h>
#include <scsi/scsi.h>


/** 
 * @enum      VPD_B2h_PROVISION_TYPE
 * @brief     VPD_B2h_PROVISION_TYPE defines which provisioning type we will support in VPD B2h 
 */
typedef enum{
    VPD_B2h_PROVISION_TYPE_NONE = 0x0 , // not report provisioning type
    VPD_B2h_PROVISION_TYPE_RP         , // provisioning type is resource provisioning
    VPD_B2h_PROVISION_TYPE_TP         , // provisioning type is thin provisioning
    MAX_VPD_B2h_PROVISION_TYPE        ,
}VPD_B2h_PROVISION_TYPE;


#define VPD_B2h_CURRENT_PROVISION_TYPE	VPD_B2h_PROVISION_TYPE_TP


/** 
 * @enum      THRESHOLD_TYPE
 * @brief     Define threshold type value
 */
typedef enum{
    THRESHOLD_TYPE_SOFTWARE = 0x0 ,
    MAX_THRESHOLD_TYPE            ,
}THRESHOLD_TYPE;

/** 
 * @enum      THRESHOLD_ARM_TYPE
 * @brief     THRESHOLD_ARM_XXX defines which type will be triggerred when resource was changed
 */
typedef enum{
    THRESHOLD_ARM_DESC = 0x0,
    THRESHOLD_ARM_INC       ,
    MAX_THRESHOLD_ARM_TYPE  ,
}THRESHOLD_ARM_TYPE;

/** 
 * @enum      LBP_LOG_PARAMS_TYPE
 * @brief     Define the logical block provisioning parameter type
 */
typedef enum{
    LBP_LOG_PARAMS_AVAILABLE_LBA_MAP_RES_COUNT      = 0x001,
    LBP_LOG_PARAMS_USED_LBA_MAP_RES_COUNT           = 0x002,
    LBP_LOG_PARAMS_DEDUPLICATED_LBA_MAP_RES_COUNT   = 0x100,
    LBP_LOG_PARAMS_COMPRESSED_LBA_MAP_RES_COUNT     = 0x101,
    LBP_LOG_PARAMS_TOTAL_LBA_MAP_RES_COUNT          = 0x102,
    MAX_LBP_LOG_PARAMS_TYPE,
}LBP_LOG_PARAMS_TYPE;


/** 
 * @struct THRESHOLD_DESC_FORMAT
 * @brief  Structure for threshold descriptor format
 */
typedef struct threshold_desc_format{
    u8  u8ThresholdArming:3;  // byte 0
    u8  u8ThresholdType:3;
    u8  u8Reserved0:1;
    u8  u8Enabled:1;
    u8  u8ThresholdResource;  // byte 1
    u8  u8Reserved1[2];       // byte 2 ~ byte 3
    u8  u8ThresholdCount[4];  // byte 4 ~ byte 7
} __attribute__ ((packed)) THRESHOLD_DESC_FORMAT;

// KENNY, 20121211
/** 
 * @struct tp_log_parameter_format
 * @brief  Structure for threshold descriptor format
 */
typedef struct lbp_log_parameter_format{
    u8  ParameterCode[2];  // byte 0~1
    u8  FormatAndLinking: 2;  // byte 2
    u8  Tmc: 2;
    u8  Etc: 1;
    u8  Tsd: 1;
    u8  Obsolete: 1;
    u8  Du:1;
    u8  ParameterLength;   	    // byte 3
    u8  ResourceCount[4];     // byte 4~7
    u8  Scope: 2;  		   // byte 8
    u8  Reserved0: 6;
    u8  Reserved1[3]; 	   // byte 9~11
}LBP_LOG_PARAMETER_FORMAT;

/* 2014/06/14, adamhsu, redmine 8530 (start) */
/* sbc3r35j, page 116 */
typedef struct lba_status_desc{
	u8	lba[8];
	u8	nr_blks[4];
	u8	provisioning_status:4;
	u8	reserved0:4;
	u8	reserved1[3];
} __attribute__ ((packed)) LBA_STATUS_DESC;
/* 2014/06/14, adamhsu, redmine 8530 (end) */


#endif /* __TP_DEF_H__ */

