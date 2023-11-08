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
#include "target_core_pr.h"
#include "target_core_extern.h"


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

	/* we may get null at this time if bd didn't be get completely in
	 * iblock_create_virtdevice() so ...
	 */
	if (!bd)
		return;

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


enum {
	aptpl_Opt_initiator_fabric, aptpl_Opt_initiator_node, 
	aptpl_Opt_initiator_sid, aptpl_Opt_sa_res_key, aptpl_Opt_res_holder, 
	aptpl_Opt_res_type, aptpl_Opt_res_scope, aptpl_Opt_res_all_tg_pt, 
	aptpl_Opt_mapped_lun, aptpl_Opt_target_fabric, aptpl_Opt_target_node, 
	aptpl_Opt_tpgt, aptpl_Opt_port_rtpi, aptpl_Opt_target_lun, 
	aptpl_Opt_pr_data_start, aptpl_Opt_pr_data_end, aptpl_Opt_err
};

static match_table_t tokens = {
	{aptpl_Opt_initiator_fabric, "initiator_fabric=%s"},
	{aptpl_Opt_initiator_node, "initiator_node=%s"},
	{aptpl_Opt_initiator_sid, "initiator_sid=%s"},
	{aptpl_Opt_sa_res_key, "sa_res_key=%s"},
	{aptpl_Opt_res_holder, "res_holder=%d"},
	{aptpl_Opt_res_type, "res_type=%d"},
	{aptpl_Opt_res_scope, "res_scope=%d"},
	{aptpl_Opt_res_all_tg_pt, "res_all_tg_pt=%d"},
	{aptpl_Opt_mapped_lun, "mapped_lun=%d"},
	{aptpl_Opt_target_fabric, "target_fabric=%s"},
	{aptpl_Opt_target_node, "target_node=%s"},
	{aptpl_Opt_tpgt, "tpgt=%d"},
	{aptpl_Opt_port_rtpi, "port_rtpi=%d"},
	{aptpl_Opt_target_lun, "target_lun=%d"},
	{aptpl_Opt_pr_data_start, "PR_REG_START: %d"},
	{aptpl_Opt_pr_data_end, "PR_REG_END: %d"},
	{aptpl_Opt_err, NULL}
};

int __qnap_scsi3_parse_aptpl_data(
	struct se_device *se_dev,
	char *data,
	struct node_info *s,
	struct node_info *d
	)
{
	unsigned char *i_fabric = NULL, *i_port = NULL, *isid = NULL;
	unsigned char *t_fabric = NULL, *t_port = NULL;
	char *ptr, *arg_p, *opts;
	substring_t args[MAX_OPT_ARGS];
	unsigned long long tmp_ll;
	u64 sa_res_key = 0;
	u32 mapped_lun = 0, target_lun = 0;
	int ret = -1, res_holder = 0, all_tg_pt = 0, arg, token;
	int token_start = 0, token_end = 0, found_match = 0;
	u16 port_rpti = 0, tpgt = 0;
	u8 type = 0, scope;

	while ((ptr = strsep(&data, ",\n")) != NULL) {
		if (!*ptr)
			continue;
	
		token = match_token(ptr, tokens, args);
		switch (token) {
		case aptpl_Opt_pr_data_end:
			match_int(args, &arg);
			token_end = 1;
			break;
		case aptpl_Opt_pr_data_start:
			match_int(args, &arg);
			token_start = 1;
			break;
		case aptpl_Opt_initiator_fabric:
			i_fabric = match_strdup(&args[0]);
			if (!i_fabric) {
				ret = -ENOMEM;
				goto out;
			}
			break;
		case aptpl_Opt_initiator_node:
			i_port = match_strdup(&args[0]);
			if (!i_port) {
				ret = -ENOMEM;
				goto out;
			}
			if (strlen(i_port) >= PR_APTPL_MAX_IPORT_LEN) {
				pr_err("APTPL metadata initiator_node="
					" exceeds PR_APTPL_MAX_IPORT_LEN: %d\n",
					PR_APTPL_MAX_IPORT_LEN);
				ret = -EINVAL;
				break;
			}
			break;
		case aptpl_Opt_initiator_sid:
			isid = match_strdup(&args[0]);
			if (!isid) {
				ret = -ENOMEM;
				goto out;
			}
			if (strlen(isid) >= PR_REG_ISID_LEN) {
				pr_err("APTPL metadata initiator_isid"
					"= exceeds PR_REG_ISID_LEN: %d\n",
					PR_REG_ISID_LEN);
				ret = -EINVAL;
				break;
			}
			break;
		case aptpl_Opt_sa_res_key:
			arg_p = match_strdup(&args[0]);
			if (!arg_p) {
				ret = -ENOMEM;
				goto out;
			}
			ret = strict_strtoull(arg_p, 0, &tmp_ll);
			if (ret < 0) {
				pr_err("strict_strtoull() failed for"
					" sa_res_key=\n");
				goto out;
			}
			sa_res_key = (u64)tmp_ll;
			break;
		/*
		 * PR APTPL Metadata for Reservation
		 */
		case aptpl_Opt_res_holder:
			match_int(args, &arg);
			res_holder = arg;
			break;
		case aptpl_Opt_res_type:
			match_int(args, &arg);
			type = (u8)arg;
			break;
		case aptpl_Opt_res_scope:
			match_int(args, &arg);
			scope = (u8)arg;
			break;
		case aptpl_Opt_res_all_tg_pt:
			match_int(args, &arg);
			all_tg_pt = (int)arg;
			break;
		case aptpl_Opt_mapped_lun:
			match_int(args, &arg);
			mapped_lun = (u32)arg;
			break;
		/*
		 * PR APTPL Metadata for Target Port
		 */
		case aptpl_Opt_target_fabric:
			t_fabric = match_strdup(&args[0]);
			if (!t_fabric) {
				ret = -ENOMEM;
				goto out;
			}
			break;
		case aptpl_Opt_target_node:
			t_port = match_strdup(&args[0]);
			if (!t_port) {
				ret = -ENOMEM;
				goto out;
			}
			if (strlen(t_port) >= PR_APTPL_MAX_TPORT_LEN) {
				pr_err("APTPL metadata target_node="
					" exceeds PR_APTPL_MAX_TPORT_LEN: %d\n",
					PR_APTPL_MAX_TPORT_LEN);
				ret = -EINVAL;
				break;
			}
			break;
		case aptpl_Opt_tpgt:
			match_int(args, &arg);
			tpgt = (u16)arg;
			break;
		case aptpl_Opt_port_rtpi:
			match_int(args, &arg);
			port_rpti = (u16)arg;
			break;
		case aptpl_Opt_target_lun:
			match_int(args, &arg);
			target_lun = (u32)arg;
			break;
		default:
			break;
		}
	
		if (!token_start) {
			pr_info("%s: not found any information in APTPL "
				"metafile\n", __func__);
			ret = -ENOENT;
			goto out;
		}
	
		if (token_end && token_start) {
			if (!i_port || !t_port || !sa_res_key) {
				pr_err("Illegal parameters for APTPL registration\n");
				ret = -EINVAL;
				goto out;
			}
				
			if (res_holder && !(type)) {
				pr_err("Illegal PR type: 0x%02x for reservation"
						" holder\n", type);
				ret = -EINVAL;
				goto out;
			}
	
			if (!strcmp(s->i_port, i_port) &&
			(!strncmp(s->i_sid, isid, PR_REG_ISID_LEN)) &&
			(!strcmp(s->t_port, t_port)) &&
			(s->tpgt == tpgt) && (s->mapped_lun == mapped_lun) &&
			(s->target_lun == target_lun)
			)
			{

				memcpy(d->i_port, i_port, PR_APTPL_MAX_IPORT_LEN);
				memcpy(d->t_port, t_port, PR_APTPL_MAX_IPORT_LEN);
				memcpy(d->i_sid, isid, PR_REG_ISID_LEN);
				d->tpgt = tpgt;
				d->sa_res_key = sa_res_key;
				d->mapped_lun = mapped_lun;
				d->target_lun = target_lun;
				d->res_holder = res_holder;
				d->all_tg_pt = all_tg_pt;
				d->port_rpti = port_rpti;
				d->type = type;
				d->scope = scope;

				found_match = 1;
			}
	
			token_start = 0;
			token_end = 0;
	
			if (found_match)
				goto out;
		}
	
	}
	
out:
	if (i_fabric)
		kfree(i_fabric);
	if (t_fabric)
		kfree(t_fabric);
	if (i_port)
		kfree(i_port);
	if (isid)
		kfree(isid);
	if (t_port)
		kfree(t_port);

	if (found_match)
		return 0;

	return -ENOENT;

}

int __qnap_scsi3_check_aptpl_metadata_file_exists(
	struct se_device *dev,
	struct file **fp
	)
{
	struct t10_wwn *wwn = &dev->se_sub_dev->t10_wwn;
	struct file *file;
	mm_segment_t old_fs;
	int flags = O_RDONLY;
	char path[512];

	/* check aptpl meta file path */
	if (strlen(&wwn->unit_serial[0]) >= 512) {
		pr_err("%s: WWN value for struct se_device does not fit"
			" into path buffer\n", __func__);
		return -EMSGSIZE;
	}

	memset(path, 0, 512);
	snprintf(path, 512, "/var/target/pr/aptpl_%s", &wwn->unit_serial[0]);

	file = filp_open(path, flags, 0600);
	if (IS_ERR(file) || !file || !file->f_dentry) {
		pr_debug("%s: filp_open(%s) for APTPL metadata"
			" failed\n", __func__, path);
		return IS_ERR(file) ? PTR_ERR(file) : -ENOENT;
	}

	*fp = file;
	return 0;
}

int qnap_transport_check_aptpl_registration(
	struct se_session *se_sess,
	struct se_node_acl *nacl,
	struct se_portal_group *tpg
	)
{
	int i = 0;
	u32 lun_access = 0;
	struct se_lun *lun;
	struct se_dev_entry *deve;
	unsigned long flags;

	spin_lock_irqsave(&nacl->device_list_lock, flags);
	    
	for (i = 0; i < TRANSPORT_MAX_LUNS_PER_TPG; i++) {
		deve = nacl->device_list[i];
		if (!deve)
			continue;
	
		lun = deve->se_lun;
		if (lun) {
			if (lun->lun_status != TRANSPORT_LUN_STATUS_ACTIVE)
				continue;
	
			lun_access = (deve->lun_flags & TRANSPORT_LUNFLAGS_READ_WRITE) ?
				TRANSPORT_LUNFLAGS_READ_WRITE :
				TRANSPORT_LUNFLAGS_READ_ONLY;

			spin_unlock_irqrestore(&nacl->device_list_lock, flags);

			qnap_transport_scsi3_check_aptpl_registration(
				lun->lun_se_dev, tpg, lun, 
				se_sess, nacl, lun->unpacked_lun
				);

			spin_lock_irqsave(&nacl->device_list_lock, flags);
		}
	}    
	spin_unlock_irqrestore(&nacl->device_list_lock, flags);
	return 0;
}
EXPORT_SYMBOL(qnap_transport_check_aptpl_registration);
