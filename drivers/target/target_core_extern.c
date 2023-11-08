#include <linux/module.h>
#include <linux/moduleparam.h>
#include <generated/utsrelease.h>
#include <linux/utsname.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/namei.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/unistd.h>
#include <linux/string.h>
#include <linux/parser.h>
#include <linux/syscalls.h>
#include <linux/configfs.h>
#include <linux/spinlock.h>

#include <target/target_core_base.h>
#include <target/target_core_backend.h>
#include <target/target_core_fabric.h>
#include <target/target_core_fabric_configfs.h>
#include <target/target_core_configfs.h>
#include <target/configfs_macros.h>

#include "fbdisk.h"
#include "target_core_iblock.h"
#include "target_core_file.h"


#if defined(CONFIG_MACH_QNAPTS)
void iblock_update_allocated(struct se_device *dev)
{
	struct iblock_dev *ibd = dev->dev_ptr;
	struct block_device *bd = ibd->ibd_bd;
	struct fbdisk_device *pfbdev = NULL;
	struct fbdisk_file *pfbfile = NULL;
	struct inode *pInode = NULL;
	loff_t total = 0, used = 0;
	u32 i;
	pfbdev = bd->bd_disk->private_data;

	for ( i = 0; i < pfbdev->fb_file_num; i++ ){ // link all fbdisk files
		pfbfile = &pfbdev->fb_backing_files_ary[i];
		pInode = pfbfile->fb_backing_file->f_mapping->host;

		total += pInode->i_size;
		used += pInode->i_blocks;
	}
	/* calculate used and available resource count */ 
	dev->se_sub_dev->se_dev_attrib.allocated = used * 512;
}

#endif

/* 20140422, adamhsu, JS chen, redmine 8042 */
void __update_allocated_attr(
	struct se_device *dev
	)
{
#if defined(CONFIG_MACH_QNAPTS)

	struct fd_dev *fd_dev = NULL;
	struct inode *inode = NULL;

	/* here is the blkio + file-backend path */
	if(!strcmp(dev->transport->name, "iblock")){
		iblock_update_allocated(dev);
		return;
	}

	if(!strcmp(dev->transport->name, "fileio")){

		/* here is the fileio path */
		fd_dev = (struct fd_dev *)dev->dev_ptr;
		inode = fd_dev->fd_file->f_mapping->host;
		dev->se_sub_dev->se_dev_attrib.allocated = 0;

		/* fileio + blk-backend */
		if (S_ISBLK(inode->i_mode))
			return;

#if defined(SUPPORT_FILEIO_ON_FILE)
		/* 20140422, adamhsu, redmine 8131 
		 * fileio + file-backend (for static volume)
		 */
		dev->se_sub_dev->se_dev_attrib.allocated = 
			((uint64_t)inode->i_blocks << 9);
#endif
	}

#endif
	return;
}
