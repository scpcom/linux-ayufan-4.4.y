#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifndef _SCSI_DISK_H
#define _SCSI_DISK_H

#define SD_MAJORS	16

#define SD_MAX_DISKS	(((26 * 26) + 26 + 1) * 26)

#ifdef MY_ABC_HERE
#define SD_TIMEOUT		(60 * HZ)
#else
#define SD_TIMEOUT		(30 * HZ)
#endif
#define SD_MOD_TIMEOUT		(75 * HZ)

#define SD_MAX_RETRIES		5
#define SD_PASSTHROUGH_RETRIES	1

#define SD_BUF_SIZE		512

#define SD_LAST_BUGGY_SECTORS	8

enum {
	SD_EXT_CDB_SIZE = 32,	 
	SD_MEMPOOL_SIZE = 2,	 
};

#ifdef MY_ABC_HERE
 
typedef enum __syno_disk_type {
	SYNO_DISK_UNKNOWN = 0,
	SYNO_DISK_SATA,
	SYNO_DISK_USB,
	SYNO_DISK_SYNOBOOT,
	SYNO_DISK_ISCSI,
	SYNO_DISK_SAS,
	SYNO_DISK_END,  
}SYNO_DISK_TYPE;
#endif

struct scsi_disk {
	struct scsi_driver *driver;	 
	struct scsi_device *device;
	struct device	dev;
	struct gendisk	*disk;
	unsigned int	openers;	 
	sector_t	capacity;	 
	u32		index;
#ifdef MY_ABC_HERE
	SYNO_DISK_TYPE	synodisktype;
#endif
#ifdef SYNO_SAS_DISK_NAME
	u32		synoindex;
#endif
	unsigned short	hw_sector_size;
	u8		media_present;
	u8		write_prot;
	u8		protection_type; 
	unsigned	previous_state : 1;
	unsigned	ATO : 1;	 
	unsigned	WCE : 1;	 
	unsigned	RCD : 1;	 
	unsigned	DPOFUA : 1;	 
	unsigned	first_scan : 1;
};
#define to_scsi_disk(obj) container_of(obj,struct scsi_disk,dev)

static inline struct scsi_disk *scsi_disk(struct gendisk *disk)
{
	return container_of(disk->private_data, struct scsi_disk, driver);
}

#define sd_printk(prefix, sdsk, fmt, a...)				\
        (sdsk)->disk ?							\
	sdev_printk(prefix, (sdsk)->device, "[%s] " fmt,		\
		    (sdsk)->disk->disk_name, ##a) :			\
	sdev_printk(prefix, (sdsk)->device, fmt, ##a)

enum sd_dif_target_protection_types {
	SD_DIF_TYPE0_PROTECTION = 0x0,
	SD_DIF_TYPE1_PROTECTION = 0x1,
	SD_DIF_TYPE2_PROTECTION = 0x2,
	SD_DIF_TYPE3_PROTECTION = 0x3,
};

struct sd_dif_tuple {
       __be16 guard_tag;	 
       __be16 app_tag;		 
       __be32 ref_tag;		 
};

#ifdef CONFIG_BLK_DEV_INTEGRITY

extern void sd_dif_config_host(struct scsi_disk *);
extern int sd_dif_prepare(struct request *rq, sector_t, unsigned int);
extern void sd_dif_complete(struct scsi_cmnd *, unsigned int);

#else  

static inline void sd_dif_config_host(struct scsi_disk *disk)
{
}
static inline int sd_dif_prepare(struct request *rq, sector_t s, unsigned int a)
{
	return 0;
}
static inline void sd_dif_complete(struct scsi_cmnd *cmd, unsigned int a)
{
}

#endif  

#endif  
