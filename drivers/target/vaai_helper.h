/**
 * @file 	vaai_helper.h
 * @brief	Basic declaration for structure / function for VAAI
 *
 * @author	Adam Hsu
 * @date	2012/06/04
 */

#ifndef VAAI_HELPER_H
#define VAAI_HELPER_H

//
//
//
#include <linux/types.h>
#include "vaai_target_struc.h"
#include "target_general.h"
#include "vaai_def.h"


/** 
 * @enum      ALLOC_MEM_TYPE
 * @brief     Declare memory allocation type
 */
typedef enum{
    MEM_KMALLOC             = 0,
    MEM_VMALLOC                ,
    MAX_MEM_TYPE               ,
}ALLOC_MEM_TYPE;

/** 
 * @enum      XCOPY_DIR
 * @brief     Declare direction for src and dest device when do extended copy
 */
typedef enum{
    XCOPY_DIR_FF            = 0,  // file i/o  -> file i/o
    XCOPY_DIR_FB               ,  // file i/o  -> block i/o
    XCOPY_DIR_BB               ,  // block i/o -> block i/o
    XCOPY_DIR_BF               ,  // block i/o -> file i/o
    MAX_COPY_DIR               ,
}XCOPY_DIR;


/**/
struct my_bio_batch {
    atomic_t          Pending;
    unsigned long     Flags;
    void              *pIBlockDev;
    struct completion	*wait;
};

#define BUILD_BB(pBB, Flag, IBDPtr, pWait) \
    do{ \
        pBB->Flags      = Flag; \
        pBB->pIBlockDev = IBDPtr; \
        pBB->wait       = pWait; \
    }while(0)

   
/**/
typedef struct __b2b_xcopy_obj{
	LIO_SE_DEVICE		*s_se_dev;
	LIO_SE_DEVICE		*d_se_dev;
	sector_t		s_lba;
	sector_t		d_lba;
	u64			data_bytes;
	unsigned long 		timeout;
	u32			s_bs_order;
	u32			d_bs_order;
	ERR_REASON_INDEX	err;

	/**/
	struct scatterlist	*sg_list;
	u32			sg_nents;
	u32			sg_total_bytes;
} B2B_XCOPY_OBJ;


typedef struct _ws_r_desc{
	/* in - lba and nr blks to be calculated
	 * out - final result
	 */
	sector_t	lba;
	u32		nr_blks;

	/* helper argument */
	u32		bs_order;
	u32		e_align_bytes;
	int		is_aligned;
} WS_R_DESC;

//
// prototype for write-same function table
//
typedef int
(*VAAI_WS_FUNC)(
    IN LIO_SE_CMD *pSeCmd,
    IN u8 u8WSAction,
    IN SUBSYSTEM_TYPE SubSysType
    );

typedef struct _vaai_ws_func_set{
    VAAI_WS_FUNC   VaaiWsFunc;
}VAAI_WS_FUNC_SET;

//
// Function prototype to dispatch suitable xcopy sub-function by segment desc type code
//
typedef int
(*XCOPY_CHK_ID_FUNC)(
    IN LIO_SE_CMD *pSeCmd,
    IN u8 *pu8SegDescPtr
    );

typedef int
(*XCOPY_INFO_COLLECT_FUNC)(
    IN LIO_SE_CMD *pSeCmd,
    IN u8 *pu8CscdDescPtr, 
    IN u8 *pu8SegDescPtr
    );

typedef int
(*XCOPY_FUNC)(
    IN LIO_SE_CMD *pSeCmd,
    IN u8 *pu8CurrentSegPtr
    );

/** 
 * @struct XCOPY_HANDLER
 * @brief Structure for each extened copy command handler
 */
typedef struct xcopy_handler{
    SEG_DESC_TYPE_CODE        SegDescCode;
    XCOPY_CHK_ID_FUNC         ChkCscdId;
    XCOPY_INFO_COLLECT_FUNC   InfoCollect;
    XCOPY_FUNC                Exec;
}XCOPY_HANDLER;

//
// Function prototype to dispatch function to do xcopy by direction
//
typedef int
(*VAAI_DEV_FUNC)(
    IN LIO_SE_CMD *pSeCmd,
    IN void *pSubSysDev,
    IN u8 u8FuncType,
    IN u8 *pu8Buffer,
    IN sector_t Lba,
    IN u32 u32ByteCounts
    );


#if 0
typedef struct xcopy_dir_do{
  XCOPY_DIR       Dir;
  VAAI_DEV_FUNC   VaaiSrcDevXcopyFunc;
  VAAI_DEV_FUNC   VaaiDestDevXcopyFunc;
}XCOPY_DIR_DO;
#endif

typedef int
(*XCOPY_BI_DO_FUNC)(
    IN LIO_SE_CMD *pSeCmd,
    IN u8 u8Dir
    );

/** 
 * @struct XCOPY_BI_DO
 * @brief Structure for extened copy command function
 */
typedef struct xcopy_bi_do{
    XCOPY_BI_DO_FUNC  ExecFunc;
}XCOPY_BI_DO;


//
// Function prototype to get the suitable device pointer by designator type
//
typedef void
(*GET_DEV_FROM_DT_FUNC)(
    IN LIO_SE_CMD *pSeCmd,
    IN u8 *pu8CscdDescPtr,
    IN void **ppRetDev
    );

/** 
 * @struct GET_DEV_FROM_DT_HANDLER
 * @brief
 */
typedef struct get_dev_from_dt_handler{
    u8                     u8Type;
    GET_DEV_FROM_DT_FUNC   GetDevExecFunc;
}GET_DEV_FROM_DT_HANDLER;

//
//
//
int do_bi_file_file(
    IN LIO_SE_CMD *pSeCmd,
    IN u8 u8Dir
    );

int do_bi_block_block(
    IN LIO_SE_CMD *pSeCmd,
    IN u8 u8Dir
    );

int do_bi_file_block(
    IN LIO_SE_CMD *pSeCmd,
    IN u8 u8Dir
    );

int do_bi_block_file(
    IN LIO_SE_CMD *pSeCmd,
    IN u8 u8Dir
    );

XCOPY_DIR vaai_get_xcopy_dir(
    IN XCOPY_INFO *pXcopyInfo
    );

void *vaai_safe_alloc_m(
  IN u32 *pu32Size,
  IN ALLOC_MEM_TYPE MemType,
  IN gfp_t GfpFlag
  );

void vaai_safe_free_m(
    IN void *p,
    IN ALLOC_MEM_TYPE MemType
    );

//
//
//
int vaai_do_lid1_xcopy_v1(
    IN LIO_SE_TASK *pSeTask
    );

int vaai_copy_op_abort_v1(
    IN LIO_SE_TASK *pSeTask
    );
 
//
//
//
int vaai_do_receive_xcopy_op_param_v1(
    IN LIO_SE_TASK *pSeTask
    );

int vaai_do_ws_v1(
    IN LIO_SE_TASK *pSeTask
    );

int vaai_do_ats_v1(
    IN LIO_SE_TASK *pSeTask
    );

int vaai_check_do_ws(
    IN u8 *pu8Flags,
    IN void *pDev
    );

//
//
//
u8 vaai_check_is_thin_lun(
    IN LIO_SE_DEVICE *pSeDev
    );

struct page * vaai_safe_get_page(
    IN gfp_t GfpFlag,
    IN u32 *pu32XfsBCs
    );


struct bio * vaai_get_bio(
    IN sector_t Lba,
    IN u32 u32BioVecs,
    IN LIO_IBLOCK_DEV *pIBlockDev
    );

int vaai_do_file_rw(
	IN LIO_SE_DEVICE *pSeDev,
	IN void *pSubSysDev,
	IN u8 u8FuncType,
	IN u8 *pu8Buffer,
	IN sector_t Lba,
	IN u32 u32ByteCounts
	);

int vaai_do_pscsi_rw(
	IN LIO_SE_CMD *pSeCmd,
	IN void *pSubSysDev,
	IN u8 u8FuncType,
	IN u8 *ppu8Buffer,
	IN sector_t Lba,
	IN u32 pu32ByteCounts
	);

//
//
//
void dbg_dump_mem(
    IN u8 *pu8Buf, 
    IN size_t DumpSize
    );

struct scatterlist *vaai_get_task_sg_from_se_cmd(
    IN LIO_SE_CMD *pSeCmd
    );

int vaai_chk_and_complete(
    IN int CmdResult,
    IN void *pContext
    );

int vaai_block_do_specific_fast_ws(
    IN LIO_SE_CMD *pSeCmd,
    IN bool special_ws
    );

int vaai_file_do_specific_fast_ws(
    IN LIO_SE_CMD *pSeCmd,
    IN bool special_ws
    );

//
extern LIO_SE_HBA* vaai_get_vritual_lun0_hba_var(void);
extern struct list_head *vaai_get_hba_list_var(void);
extern void *vaai_get_hba_lock_var(void);

//
//
extern sector_t               u64MaxWSLen;
extern u8                     u8MaxATSLen;
extern u8                     u8WriteSameNoZero;
extern CSCD_DESC_TYPE_CODE    gCscdSupportedTable[];
extern SEG_DESC_TYPE_CODE     gSegSupportedTable[];
extern XCOPY_HANDLER          gXcopyFuncTable[];

#endif /* VAAI_HELPER_H */

