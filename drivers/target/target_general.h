/*
 *
 *
 *
 *
 */
 
#ifndef _TARGET_GENERAL_H_
#define _TARGET_GENERAL_H_

#include <linux/types.h>
#include <linux/kernel.h>
#include <scsi/scsi.h>
#include <linux/dma-direction.h>
#include "vaai_target_struc.h"

/**/
#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif


/**/
#define COMPILE_ASSERT(Expr) \
    do { \
        char _compile_assert[((Expr) ? 1 : -1)]; \
        (void)_compile_assert; \
    }while (0);

#define IS_TIMEOUT(time) \
    ((time_after(jiffies, time) && (!time)))

/* max # of bios to submit at a time, please refer the target_core_iblock.c */
#define BLOCK_MAX_BIO_PER_TASK		32
#define DEFAULT_IO_TIMEOUT		(2 * 1000)
#define SG_MAX_IO			(PAGE_SIZE)

/**/
#define SCSI_COMPARE_AND_WRITE		0x89

// Indicate whether return zero data wehn read unmapped LBA
#define RET_ZERO_READ_UNMAP_LAB 	1
#define SUPPORT_ANCH_LBA		0

/* 1. 20140626, adamhsu, redmine 8745,8777,8778
 * 2. 2015/04/13, adamhsu, redmine 11438 
 */
#define	MAX_UNMAP_COUNT_SHIFT	10	/* 2^10 */
#define MAX_UNMAP_DESC_COUNT	1
#define MAX_UNMAP_MB_SIZE	(512)

/* 2014/06/26, adamhsu, redmine 8794 (start) */
#if (BITS_PER_LONG == 64)
#elif (BITS_PER_LONG == 32)
#else
#error unsupported long size. neither 64 nor 32 bits. please check the arch
#endif

#define	ARCH_P_LEN	BITS_PER_LONG
#define __IS_32BIT_ARCH(len)	(len == 32)
#define __IS_64BIT_ARCH(len)	(len == 64)
/* 2014/06/26, adamhsu, redmine 8794 (end) */


#define ALIGN_GAP_SIZE_B	(0x100000) /* 512b * 2048 sctors */
#define POOL_BLK_SIZE_512_KB	(512)
#define POOL_BLK_SIZE_1024_KB	(1024)

/* TODO
 * 1. these shall be multiplied by POOL_BLK_SIZE_XXXX_KB 
 * 2. and, MAX_TRANSFER_LEN_MB >= OPTIMAL_TRANSFER_LEN_MB
 */
#define MAX_TRANSFER_LEN_MB	(16)
#define OPTIMAL_TRANSFER_LEN_MB	(16)
/** 
 * @enum      ERR_REASON_INDEX
 * @brief     Error condition index value
 */
typedef enum{
    ERR_UNKNOWN_SAM_OPCODE = 0,             // 0 - TCM_UNSUPPORTED_SCSI_OPCODE
    ERR_REQ_TOO_MANY_SECTORS,               // 1 - TCM_SECTOR_COUNT_TOO_MANY
    ERR_INVALID_CDB_FIELD,                  // 2 - TCM_INVALID_CDB_FIELD
    ERR_INVALID_PARAMETER_LIST,             // 3 - TCM_INVALID_PARAMETER_LIST
    ERR_LOGICAL_UNIT_COMMUNICATION_FAILURE, // 4 - TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE
    ERR_UNKNOWN_MODE_PAGE,                  // 5 - TCM_UNKNOWN_MODE_PAGE
    ERR_WRITE_PROTECTEDS,                   // 6 - TCM_WRITE_PROTECTED
    ERR_RESERVATION_CONFLICT,               // 7 - TCM_RESERVATION_CONFLICT
    ERR_ILLEGAL_REQUEST,                    // 8 - TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE
    ERR_CHECK_CONDITION_NOT_READY,          // 9 - TCM_CHECK_CONDITION_NOT_READY
    ERR_LBA_OUT_OF_RANGE,                   // 10 - TCM_ADDRESS_OUT_OF_RANGE
    ERR_MISCOMPARE_DURING_VERIFY_OP,        // 11 - TCM_MISCOMPARE_DURING_VERIFY_OP
    ERR_PARAMETER_LIST_LEN_ERROR,           // 12 - TCM_PARAMETER_LIST_LEN_ERROR
    ERR_UNREACHABLE_COPY_TARGET,            // 13 - TCM_UNREACHABLE_COPY_TARGET
    ERR_3RD_PARTY_DEVICE_FAILURE,           // 14 - TCM_3RD_PARTY_DEVICE_FAILURE
    ERR_INCORRECT_COPY_TARGET_DEV_TYPE,     // 15 - TCM_INCORRECT_COPY_TARGET_DEV_TYPE
    ERR_TOO_MANY_TARGET_DESCRIPTORS,        // 16 - TCM_TOO_MANY_TARGET_DESCRIPTORS
    ERR_TOO_MANY_SEGMENT_DESCRIPTORS,       // 17 - TCM_TOO_MANY_SEGMENT_DESCRIPTORS

    ERR_ILLEGAL_REQ_DATA_OVERRUN_COPY_TARGET,   // 18 - TCM_ILLEGAL_REQ_DATA_OVERRUN_COPY_TARGET
    ERR_ILLEGAL_REQ_DATA_UNDERRUN_COPY_TARGET,  // 19 - TCM_ILLEGAL_REQ_DATA_UNDERRUN_COPY_TARGET
    ERR_COPY_ABORT_DATA_OVERRUN_COPY_TARGET,    // 20 - TCM_COPY_ABORT_DATA_OVERRUN_COPY_TARGET
    ERR_COPY_ABORT_DATA_UNDERRUN_COPY_TARGET,   // 21 - TCM_COPY_ABORT_DATA_UNDERRUN_COPY_TARGET
    ERR_INSUFFICIENT_RESOURCES,                 // 22
    ERR_INSUFFICIENT_RESOURCES_TO_CREATE_ROD,   // 23 - TCM_INSUFFICIENT_RESOURCES_TO_CREATE_ROD
    ERR_INSUFFICIENT_RESOURCES_TO_CREATE_ROD_TOKEN, // 24 - TCM_INSUFFICIENT_RESOURCES_TO_CREATE_ROD_TOKEN
    ERR_OPERATION_IN_PROGRESS,                  // 25 - TCM_OPERATION_IN_PROGRESS
    ERR_INVALID_TOKEN_OP_AND_INVALID_TOKEN_LEN, // 26 - TCM_INVALID_TOKEN_OP_AND_INVALID_TOKEN_LEN
    ERR_INVALID_TOKEN_OP_AND_CAUSE_NOT_REPORTABLE,  // 27 - TCM_INVALID_TOKEN_OP_AND_CAUSE_NOT_REPORTABLE
    ERR_INVALID_TOKEN_OP_AND_REMOTE_ROD_TOKEN_CREATION_NOT_SUPPORTED, // 28 - TCM_INVALID_TOKEN_OP_AND_REMOTE_ROD_TOKEN_CREATION_NOT_SUPPORTED
    ERR_INVALID_TOKEN_OP_AND_REMOTE_ROD_TOKEN_USAGE_NOT_SUPPORTED,  // 29 - TCM_INVALID_TOKEN_OP_AND_REMOTE_ROD_TOKEN_USAGE_NOT_SUPPORTED
    ERR_INVALID_TOKEN_OP_AND_TOKEN_CANCELLED,   // 30 - TCM_INVALID_TOKEN_OP_AND_TOKEN_CANCELLED
    ERR_INVALID_TOKEN_OP_AND_TOKEN_CORRUPT,     // 31 - TCM_INVALID_TOKEN_OP_AND_TOKEN_CORRUPT
    ERR_INVALID_TOKEN_OP_AND_TOKEN_DELETED,     // 32 - TCM_INVALID_TOKEN_OP_AND_TOKEN_DELETED
    ERR_INVALID_TOKEN_OP_AND_TOKEN_EXPIRED,     // 33 - TCM_INVALID_TOKEN_OP_AND_TOKEN_EXPIRED
    ERR_INVALID_TOKEN_OP_AND_TOKEN_REVOKED,     // 34 - TCM_INVALID_TOKEN_OP_AND_TOKEN_REVOKED
    ERR_INVALID_TOKEN_OP_AND_TOKEN_UNKNOWN,     // 35 - TCM_INVALID_TOKEN_OP_AND_TOKEN_UNKNOWN
    ERR_INVALID_TOKEN_OP_AND_UNSUPPORTED_TOKEN_TYPE, // 36 - TCM_INVALID_TOKEN_OP_AND_UNSUPPORTED_TOKEN_TYPE

    ERR_NO_SPACE_WRITE_PROTECT, // 37 - TCM_SPACE_ALLOCATION_FAILED_WRITE_PROTECT
    ERR_OUT_OF_RESOURCES, // 38 - TCM_OUT_OF_RESOURCES

    MAX_ERR_REASON_INDEX,
}ERR_REASON_INDEX;


/** 
 * @enum      TPC_CMD_INDEX
 * @brief     Declare index value for 3rd party copy command
 */
typedef enum{
    TPC_CMD_83H             = 0,
    TPC_CMD_84H                ,
    MAX_TPC_CMD_INDEX          ,
}TPC_CMD_INDEX;

/** 
 * @struct TPC_SAC
 * @brief
 */
typedef struct tpc_sac{
    u8    u8SAC;
    void  *pProc;
} __attribute__ ((packed)) TPC_SAC;

/** 
 * @struct TPC_CMD
 * @brief
 */
typedef struct tpc_cmd{
    u8        u8OpCode;
    TPC_SAC   *pSAC;
} __attribute__ ((packed)) TPC_CMD;


/** 
 * @enum      SUBSYSTEM_TYPE
 * @brief     Declare backend device type
 */
typedef enum {
    SUBSYSTEM_BLOCK    = 0,
    SUBSYSTEM_FILE        ,
    SUBSYSTEM_PSCSI       ,
    MAX_SUBSYSTEM_TYPE    ,
}SUBSYSTEM_TYPE;


/**/
typedef ssize_t (*VFS_RW)(
	struct file *file,
	const struct iovec __user *vec,
	unsigned long vlen, 
	loff_t *pos
	);

typedef void (*BIO_END_IO)( struct bio *bio, int err);
typedef void (*BIO_DESTRUCTIR)(struct bio *bio);

typedef struct _bio_data {
	BIO_END_IO	bi_end_io;
	BIO_DESTRUCTIR	bi_destructor;
}BIO_DATA;

typedef struct cb_data {
	atomic_t		BioCount;
	atomic_t		BioErrCount;
	struct completion	*wait;
	int			nospc_err;
}CB_DATA;

/* 20140626, adamhsu, redmine 8745,8777,8778 (start) */
typedef struct __align_desc{
	/* in - lba and nr blks to be calculated
	 * out - final result
	 */
	sector_t	lba;
	u32		nr_blks;

	/* helper argument */
	u32		bs_order;
	u32		e_align_bytes;
	bool		is_aligned;
} ALIGN_DESC;
/* 20140626, adamhsu, redmine 8745,8777,8778 (end) */


/**/
#define TASK_FLAG_DO_FUA        0x1
typedef struct gen_rw_task{
	LIO_SE_DEVICE		*se_dev;
	struct scatterlist	*sg_list;
	unsigned long		timeout_jiffies;
	sector_t		lba;
	u32			sg_nents;

	/* 1. (nr_blks << dev_bs_order) = sum of len for all sg elements 
	 * 2. the purpose of sg lists
	 * - for read: Usually, they are i/o buffer
	 * - for write:
	 * (a) Usually, they are buffer for normal write i/o
	 * (b) for special write discard, they are buffer for non-aligned /
	 * non-multipled write data i/o
	 */
	u32			nr_blks;


	/* s_nr_blks is for discard operation */
	u32			s_nr_blks;

	u32			dev_bs_order;
	u32			task_flag;
	enum dma_data_direction	dir;
	bool			is_timeout;
	int			ret_code;
}__attribute__ ((packed)) GEN_RW_TASK;

typedef struct _io_rec{
	struct	list_head	node;
	void			*pIBlockDev;
	CB_DATA			*cb_data;
	u32			nr_blks;
	bool			transfer_done;
} IO_REC;

typedef struct _err_reason_table{
	int			err_reason;
	char			*err_str;
} ERR_REASON_TABLE;

typedef struct _logsense_func_table{
	u8	page_code;
	u8	sub_page_code;
	int	(*logsense_func)(struct se_cmd *cmd, u8 *buf);   
	int	is_end_table;
}__attribute__ ((packed)) LOGSENSE_FUNC_TABLE;

struct bio_rec {
	struct	list_head node;
	struct	bio *bio;
	void	*se_task;
};

/* tricky method like PageAnon(struct page *page) */
#define BI_PRIVATE_BREC	(1)

static inline int qnap_bi_private_is_brec(
	void *bi_private
	)
{
	return (((unsigned long)bi_private & BI_PRIVATE_BREC) != 0);
}

static inline void *qnap_bi_private_set_brec_bit(
	void *bi_private
	) 
{
	unsigned long tmp = (unsigned long)bi_private;
	return (void *)(tmp + BI_PRIVATE_BREC);
}

static inline void *qnap_bi_private_clear_brec_bit(
	void *bi_private
	) 
{
	unsigned long tmp = (unsigned long)bi_private;
	return (void *)(tmp - BI_PRIVATE_BREC);
}

/**/
extern u32 __call_transport_get_size(
	IN u32 sectors,
	IN u8 *cdb,
	IN LIO_SE_CMD *pSeCmd
	);

extern unsigned long long __call_transport_lba_64(IN u8 *cdb);
extern unsigned long long __call_transport_lba_64_ext(IN u8 *cdb);
extern u32 __call_transport_lba_21(IN u8 *cdb);
extern u32 __call_transport_lba_32(IN u8 *cdb);

extern u32 __call_transport_get_sectors_6(
	IN u8 *cdb, 
	IN LIO_SE_CMD *se_cmd, 
	IN OUT int *ret
	);

extern u32 __call_transport_get_sectors_10(
	IN u8 *cdb, 
	IN LIO_SE_CMD *se_cmd, 
	IN OUT int *ret
	);

extern u32 __call_transport_get_sectors_12(
	IN u8 *cdb, 
	IN LIO_SE_CMD *se_cmd, 
	IN OUT int *ret
	);

extern u32 __call_transport_get_sectors_16(
	IN u8 *cdb, 
	IN LIO_SE_CMD *se_cmd, 
	IN OUT int *ret
	);

extern u32 __call_transport_get_sectors_32(
	IN u8 *cdb, 
	IN LIO_SE_CMD *se_cmd, 
	IN OUT int *ret
	);

extern void __get_target_parse_naa_6h_vendor_specific(
	IN LIO_SE_DEVICE *pSeDev,
	IN u8 *pu8Buf
	);

extern void __make_target_naa_6h_hdr(
	LIO_SE_DEVICE *se_dev, 
	u8 *buf
	);

/**/
int is_thin_lun(struct se_device *dev);

int __get_lba_and_nr_blks_ws_cdb(
	IN LIO_SE_CMD *pSeCmd,
	IN OUT sector_t *pLba,
	IN OUT u32 *pu32NumBlocks
	);

int __isntall_tpc_proc(
	IN u8 *pu8Cdb,
	IN void **ppHook
	);

void __set_err_reason(
	IN ERR_REASON_INDEX ErrCode,
	IN OUT u8 *pOutReason
	);

int __do_b_rw(GEN_RW_TASK *task);
int __do_f_rw(GEN_RW_TASK *task);


void __init_cb_data(
	CB_DATA *pData,
	void *pContects
	);

void __make_rw_task(
	GEN_RW_TASK *task,
	LIO_SE_DEVICE *se_dev,
	sector_t lba,
	u32 nr_blks,
	unsigned long timeout,
	enum dma_data_direction dir
	);


void __generic_free_sg_list(
	struct scatterlist *sg_list,
	u32 sg_nent
	);

int __generic_alloc_sg_list(
	u64 *data_size,
	struct scatterlist **sg_list,
	u32 *sg_nent
	);

int  __submit_bio_wait(
	struct bio_list *bio_lists,
	u8 cmd,
	unsigned long timeout
	);

void __do_pop_submit_bio(
	IN struct block_device *pBD,
	IN struct bio_list *pBioList,
	IN u8 u8Cmd
	);

void __do_pop_put_bio(
	IN struct bio_list *biolist
	);

struct bio *__get_one_bio(
	GEN_RW_TASK *task,
	BIO_DATA *bio_data,
	sector_t block_lba
	);

void __add_page_to_one_bio(
	struct bio *bio,
	struct page *page,
	unsigned int len,
	unsigned int off
	);

/**/
int __do_get_subsystem_dev_type(
	LIO_SE_DEVICE *pSeDev,
	SUBSYSTEM_TYPE *pType
	);

int __do_get_subsystem_dev_ptr_by_type( 
	LIO_SE_DEVICE *pSeDev,
	SUBSYSTEM_TYPE Type,
	void **ppSubSysDev
	);

/* 20140626, adamhsu, redmine 8745,8777,8778 (start) */
void __create_aligned_range_desc(ALIGN_DESC *desc, sector_t start_lba,
		u32 nr_blks_range, u32 block_size_order, u32 aligned_size);

int blkdev_issue_special_discard(
	IN struct block_device *bdev, 
	IN sector_t sector,
	IN sector_t nr_sects, 
	IN gfp_t gfp_mask, 
	IN unsigned long flags
	);

#if defined(SUPPORT_LOGICAL_BLOCK_4KB_FROM_NAS_GUI)
int __blkio_transfer_task_lba_to_block_lba(
	IN u32 logical_block_size,
	IN OUT sector_t *out_lba
	);
#endif

#if defined(SUPPORT_TP)
int check_dm_thin_cond(struct block_device *bd);
int __do_sync_cache_range(
	struct file *file,
	loff_t start_byte,
	loff_t end_byte	
	);


int check_backend_thinpool(struct block_device *bd);
#endif

int transport_check_sectors_exceeds_max_limits_blks(
	LIO_SE_CMD *se_cmd,
	u32 sectors
	);

int transport_set_pool_blk_sectors(
	LIO_SE_DEVICE *se_dev,
	struct se_dev_limits *dev_limits
	);


int transport_get_pool_blk_size_kb(void);

int transport_get_max_hw_transfer_sectors(
	LIO_SE_DEVICE *se_dev
	);


int transport_get_max_transfer_sectors(
	LIO_SE_DEVICE *se_dev
	);

int transport_get_opt_transfer_sectors(
	LIO_SE_DEVICE *se_dev
	);

void transport_setup_support_fbc(
	LIO_SE_DEVICE *se_dev
	);


#if (LINUX_VERSION_CODE == KERNEL_VERSION(3,12,6))
void transport_init_tag_pool(
	struct se_session *se_sess
	);

static int transport_free_extra_tag_pool(
	struct se_session *se_sess
	);
#endif

int qnap_transport_is_iblock_fbdisk(struct se_device *se_dev);
int qnap_transport_is_fio_blk_backend(struct se_device *se_dev);
int qnap_transport_drop_bb_cmd(struct se_cmd *se_cmd, int type);

int qnap_transport_blkdev_issue_discard(struct se_cmd *se_cmd, 
	struct block_device *bdev, sector_t sector, sector_t nr_sects, 
	gfp_t gfp_mask, unsigned long flags);

bool qnap_transport_is_dropped_by_release_conn(struct se_cmd *se_cmd);
bool qnap_transport_is_dropped_by_tmr(struct se_cmd *se_cmd);

int qnap_target_execute_sync_cache(struct se_task *task);
int qnap_target_execute_discard(struct se_task *se_task);

int qnap_transport_drop_fb_cmd(struct se_cmd *se_cmd, int type);
void qnap_transport_create_fb_bio_rec_kmem(struct se_device *se_dev);
void qnap_transport_destroy_fb_bio_rec_kmem(struct se_device *se_dev);
int qnap_transport_alloc_bio_rec(struct se_task *se_task, struct bio *bio);
int qnap_transport_free_bio_rec_lists(struct se_cmd *se_cmd);
void qnap_transport_set_bio_rec_null(struct se_cmd *se_cmd, struct bio_rec *brec);
void qnap_transport_init_bio_rec_val(struct se_cmd *se_cmd);
int qnap_transport_check_report_lun_changed(struct se_cmd *se_cmd);


/**/
extern TPC_CMD                gTpcTable[MAX_TPC_CMD_INDEX];
extern TPC_SAC                gTpc83hSacTable[];
extern TPC_SAC                gTpc84hSacTable[];
extern u8                     bSupportArchorLba;
extern u8                     bReturnZeroWhenReadUnmapLba;


#endif /* _TARGET_GENERAL_H_ */

