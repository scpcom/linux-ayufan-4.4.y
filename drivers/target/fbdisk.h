/**
* $Header:
*
* Copyright (c) 2009 QNAP SYSTEMS, INC.
* All Rights Reserved.
*
* $Id:
*/
/**
 * @file 	fbdisk.h
 * @brief	file backed disk declaration file.
 *
 * @author	Nike Chen
 * @date	2009/08/01
 */

#ifndef _FILE_BACKED_DISK_HDR
#define _FILE_BACKED_DISK_HDR

#define	SUPPORT_FBSNAP_DEVICE                 // Open this definition for supporting snapshot.  
//#define 	ENABLE_FBSNAP_STATISTIC                 // Enable the fbsnap statistic. (For test only.)

/////////////////////////////////////////////////////////////////////////////
#define MAX_FBDISK_NUM		256				// MAX_FBDISK_NUM should be the same as the max_device in  NasUtil/fbutil/fbdisk.sh

#define MAX_FBDISK_NAME		64
#ifdef QNAP_HAL_SUPPORT
#define MAX_FILE_NUM		512
#else
#define MAX_FILE_NUM		64
#endif /* #ifdef QNAP_HAL_SUPPORT */
#define MAX_FILE_NAME		64
#define LUN_NOT_MAPPING     (-1)
// [S] Snapshot feature related constants.
#define	CBMAX_FILE_PREFIX	256                     // The maximum size of snap-file prefix in byte.

#define SECTOR_SIZE     512
#define SECTOR_SHIFT    9
#define SECTOR_MASK     (SECTOR_SIZE - 1)

enum fb_type_num
{
	FB_TYPE_DISK, 
#ifdef SUPPORT_FBSNAP_DEVICE
	FB_TYPE_SNAP,
#endif /* #ifdef SUPPORT_FBSNAP_DEVICE */		
	MAX_FB_TYPE
};

enum {
	IDCMD_FBSNAP_ENABLE             = 1,            // Request command to enable a snapshot.
	IDCMD_FBSNAP_DISABLE            = 2,            // Request command to disable the snapshot.
} IDCMD_FBSNAP;

enum {
	IDSTAT_FBSNAP_READY             = 0,            // An snapshot is ready.
	IDSTAT_FBSNAP_IO_ERROR          = -EIO,         // (5) The snapshot is ceased by I/O error.
	IDSTAT_FBSNAP_NO_SPACE		= -ENOSPC,          // (28) The snapshot is ceased by no-device-space.
	IDSTAT_FBSNAP_NO_MEMORY		= -ENOMEM,          // (12) The snapshot is ceased by out-of-memory.
	IDSTAT_FBSNAP_NOT_READY		= -ENODATA,         // (61) The snapshot is not enabled.
	IDSTAT_FBSNAP_DEV_NOT_READY	= -ENOTBLK,         // (15) The fbdisk device is not available.
} IDSTAT_FBSNAP;
// [E] Snapshot feature related constants.

typedef struct __attribute__((packed))
{
    //char file_name[MAX_FILE_NAME];
    u64	file_lenth; /*!< reserved field right now */
    int file_desc;
} fbdisk_file_info;

typedef struct __attribute__((packed))
{
    /** fbdisk commnad options flag. */
    /** bits in the flags field*/
    enum {
        ADD_DEVICE_NAME = 0x0001,
        ADD_FILE_NAME = 0x0002,
        ADD_LUN_INDEX = 0x0004,
    } flags;
    char dev_name[MAX_FBDISK_NAME];
    int file_count;
    fbdisk_file_info file_info[MAX_FILE_NUM];
    // [S] Benjamin 20110218 for getting the lun-fbdisk mapping 
    int bSnap;     //the lun index is for fbsnap or fbdisk
    int lun_index;
    // [E] Benjamin 20110218 for getting the lun-fbdisk mapping            
} fbdisk_info;

typedef struct __attribute__((packed))
{
    int status;
    // [S] Benjamin 20110218 for getting the lun-fbdisk mapping 
    int lun_index;
    // [E] Benjamin 20110218 for getting the lun-fbdisk mapping    
} fbdisk_status;

// [S] Snapshot feature related data strctures.
// The data structure for the snapshot file path.
typedef struct __attribute__((packed))
{
	int32_t		idCommand;		// The command ID. (Check IDCMD_FBSNAP_XXX)
	int32_t		btBlock;		// The block size in 2 exponentation. (def:12, 12 ~ 20)
	int32_t		btSubmap;		// The submap size in 2 exponentation. (def:10, 9 ~ 12)
	int32_t		cnSubmap;		// The total number of the submap. (def:16, 4 ~ 64)
	char		szfPrefix[CBMAX_FILE_PREFIX];	// The path prefix of the snapshot files.
} fbsnap_info;

// The data structure for the snapshot status.
typedef struct __attribute__((packed))
{
	int32_t		idStatus;		// The snapshot status. (Check IDSTAT_FBSNAP_XXX)
	uint32_t	cnFiles;		// The total number of device files.
	uint32_t 	mbFiles;		// The total size of this device files in megabyte.
	uint32_t 	rymbFile[MAX_FILE_NUM];		// The size of every device file in megabyte.
} fbsnap_status;

// The data structure for the snapshot statistic.
typedef struct __attribute__((packed))
{
	unsigned long long	cnQuery;		///< The total times of querying bit map. (By set/get APIs)
	unsigned long long	cnQueryCache;	///< The total times of querying a submap in cache object. (It happens when switching from one submap to another.)
	unsigned long long	cnSwap;			///< The total times we swap a cached submap with a new submap loaded from hard disk.
	unsigned long long	cnFlush;		///< The total times we flush the data of a swapped-out submap into hard disk.
} fbsnap_statistic;
// [E] Snapshot feature related data strctures.

#define FBDISK_IOC_MAGIC             'F'
#define FBDISK_IOCSETFD              _IOW(FBDISK_IOC_MAGIC, 1, fbdisk_info)
#define FBDISK_IOCCLRFD              _IO(FBDISK_IOC_MAGIC, 2)
#define FBDISK_IOCGETSTATUS          _IOR(FBDISK_IOC_MAGIC, 3, fbdisk_status)
#define FBDISK_IOCSETSTATUS          _IOW(FBDISK_IOC_MAGIC, 4, fbdisk_status)
#define FBDISK_IOCADDFD              _IOW(FBDISK_IOC_MAGIC, 5, fbdisk_info)
#define	FBDISK_IOC_SETSNAP           _IOW(FBDISK_IOC_MAGIC, 6, fbsnap_info)
#define	FBDISK_IOC_GETSNAP           _IOR(FBDISK_IOC_MAGIC, 7, fbsnap_status)
#define	FBDISK_IOC_SNAP_STATISTIC    _IOR(FBDISK_IOC_MAGIC, 8, fbsnap_statistic)

#ifdef __KERNEL__

#include <linux/bio.h>
#include <linux/blkdev.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/ioctl.h>

#define TRACE_ERROR		0x0001
#define TRACE_WARNING		0x0002
#define TRACE_ENTRY		0x0004
#define TRACE_REQ		0x0008
#define TRACE_ALL		0xFFFF

/* Possible states of device */
enum {
    Fb_unbound,
    Fb_bound,
    Fb_rundown,
};

/*
 * fbdisk flags
 */
enum {
    FB_FLAGS_READ_ONLY	= 1,
    FB_FLAGS_USE_AOPS	= 2,
};

struct fbdisk_file {
    sector_t             fb_start_sector;
    sector_t             fb_end_sector;
    // char                         fb_file_name[MAX_FILE_NAME];
    struct file *        fb_backing_file;
    unsigned             fb_blocksize;
    loff_t               fb_file_size;		// file size in sector (512 byte)
    gfp_t                old_gfp_mask;
};

struct fbdisk_device {
    int                  fb_number;
    int                  fb_refcnt;
    loff_t               fb_sizelimit;
    int                  fb_flags;
    int                  (*transfer)(struct fbdisk_device *,
                                   int cmd,
                                   struct page *raw_page,
                                   unsigned raw_off,
                                   struct page *loop_page,
                                   unsigned loop_off, int size);
    __u32                fb_init[2];
    int	                 (*ioctl)(struct fbdisk_device *,
                                 int cmd,
                                 unsigned long arg);
    struct block_device  *fb_device; 
    spinlock_t           fb_lock;               // for bio
    struct bio_list      fb_bio_list;
    int                  fb_state;
    struct mutex         fb_ctl_mutex;          // for ioctl
    struct task_struct   *fb_thread;
    wait_queue_head_t    fb_event;
    struct request_queue *fb_queue[MAX_FB_TYPE];
    struct gendisk       *fb_disk[MAX_FB_TYPE];
    struct list_head     fb_list;
    int                  fb_file_num;
    struct fbdisk_file   fb_backing_files_ary[MAX_FILE_NUM];
    // [S] Benjamin 20110218 for getting the lun-fbdisk mapping 
    int                  fb_lun_index[MAX_FB_TYPE];
    // [E] Benjamin 20110218 for getting the lun-fbdisk mapping        
#ifdef	SUPPORT_FBSNAP_DEVICE
	int                 idStatusSnap;              // The snapshot status. (Check IDSTAT_FBSNAP_XXX)
	int                 btBlock;                   // The block size in 2 exponentation. (def:12, 12 ~ 20)
	int                 cbBlock;                   // The block size in byte.
	int                 btSubmap;                  // The submap size in 2 exponentation. (def:10, 9 ~ 12)
	int                 cnSubmap;                  // The total number of the submap. (def:16, 4 ~ 64)
	loff_t              cbBmpFile;                 // The size of bitmap file in byte.
	loff_t              csCapSnap;                 // The snapshot device capacity. (in sector)
	struct bio_list     tblSnap;                   // The bio list of the snapshot device.
	struct file         *ptfSnapBmp;               // The snapshot bitmap file object.
	void                *ptBitmap;                 // The snapshot bitmap object.
    gfp_t               rygfpSnap[MAX_FILE_NUM];   // The snapshot gfp value.
	struct file         *ryptfSnap[MAX_FILE_NUM];  // The snapshot data file object array.
#endif	//SUPPORT_FBSNAP_DEVICE
};

struct fb_read_data {
    struct fbdisk_device *fb;
    struct page *page;
    unsigned offset;
    int bsize;
};

#endif /* __KERNEL__ */


#endif
