#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
/*
 *  linux/fs/hfsplus/dir.c
 *
 * Copyright (C) 2001
 * Brad Boyer (flar@allandria.com)
 * (C) 2003 Ardis Technologies <roman@ardistech.com>
 *
 * Handling of directories
 */

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/random.h>

#include "hfsplus_fs.h"
#include "hfsplus_raw.h"
#include "xattr.h"

#ifdef MY_ABC_HERE
extern struct mutex syno_hfsplus_global_mutex;
#endif

static inline void hfsplus_instantiate(struct dentry *dentry,
				       struct inode *inode, u32 cnid)
{
	dentry->d_fsdata = (void *)(unsigned long)cnid;
	d_instantiate(dentry, inode);
}

/* Find the entry inside dir named dentry->d_name */
static struct dentry *hfsplus_lookup(struct inode *dir, struct dentry *dentry,
				     struct nameidata *nd)
{
	struct inode *inode = NULL;
	struct hfs_find_data fd;
	struct super_block *sb;
	hfsplus_cat_entry entry;
	int err;
	u32 cnid, linkid = 0;
	u16 type;

#ifdef MY_ABC_HERE
	mutex_lock(&syno_hfsplus_global_mutex);
#endif
#ifdef MY_ABC_HERE
	if (dentry->d_name.len > NAME_MAX) {
#ifdef MY_ABC_HERE
		mutex_unlock(&syno_hfsplus_global_mutex);
#endif
		return ERR_PTR(-ENAMETOOLONG);
	}
#endif
	sb = dir->i_sb;

	dentry->d_op = &hfsplus_dentry_operations;
	dentry->d_fsdata = NULL;
	hfs_find_init(HFSPLUS_SB(sb).cat_tree, &fd);
	hfsplus_cat_build_key(sb, fd.search_key, dir->i_ino, &dentry->d_name);
again:
	err = hfs_brec_read(&fd, &entry, sizeof(entry));
	if (err) {
		if (err == -ENOENT) {
			hfs_find_exit(&fd);
			/* No such entry */
			inode = NULL;
			goto out;
		}
		goto fail;
	}
	type = be16_to_cpu(entry.type);
	if (type == HFSPLUS_FOLDER) {
		if (fd.entrylength < sizeof(struct hfsplus_cat_folder)) {
			err = -EIO;
			goto fail;
		}
		cnid = be32_to_cpu(entry.folder.id);
		dentry->d_fsdata = (void *)(unsigned long)cnid;
	} else if (type == HFSPLUS_FILE) {
		if (fd.entrylength < sizeof(struct hfsplus_cat_file)) {
			err = -EIO;
			goto fail;
		}
		cnid = be32_to_cpu(entry.file.id);
		if (entry.file.user_info.fdType == cpu_to_be32(HFSP_HARDLINK_TYPE) &&
		    entry.file.user_info.fdCreator == cpu_to_be32(HFSP_HFSPLUS_CREATOR) &&
		    (entry.file.create_date == HFSPLUS_I(HFSPLUS_SB(sb).hidden_dir).create_date ||
		     entry.file.create_date == HFSPLUS_I(sb->s_root->d_inode).create_date) &&
		    HFSPLUS_SB(sb).hidden_dir) {
			struct qstr str;
			char name[32];

			if (dentry->d_fsdata) {
				/*
				 * We found a link pointing to another link,
				 * so ignore it and treat it as regular file.
				 */
				cnid = (unsigned long)dentry->d_fsdata;
				linkid = 0;
			} else {
				dentry->d_fsdata = (void *)(unsigned long)cnid;
				linkid = be32_to_cpu(entry.file.permissions.dev);
				str.len = sprintf(name, "iNode%d", linkid);
				str.name = name;
				hfsplus_cat_build_key(sb, fd.search_key, HFSPLUS_SB(sb).hidden_dir->i_ino, &str);
				goto again;
			}
		} else if (!dentry->d_fsdata)
			dentry->d_fsdata = (void *)(unsigned long)cnid;
	} else {
		printk(KERN_ERR "hfs: invalid catalog entry type in lookup\n");
		err = -EIO;
		goto fail;
	}
	hfs_find_exit(&fd);
	inode = hfsplus_iget(dir->i_sb, cnid);
	if (IS_ERR(inode)) {
#ifdef MY_ABC_HERE
		mutex_unlock(&syno_hfsplus_global_mutex);
#endif
		return ERR_CAST(inode);
	}
	if (S_ISREG(inode->i_mode))
		HFSPLUS_I(inode).dev = linkid;
out:
	d_add(dentry, inode);
#ifdef MY_ABC_HERE
	mutex_unlock(&syno_hfsplus_global_mutex);
#endif
	return NULL;
fail:
	hfs_find_exit(&fd);
#ifdef MY_ABC_HERE
	mutex_unlock(&syno_hfsplus_global_mutex);
#endif
	return ERR_PTR(err);
}

static int hfsplus_readdir(struct file *filp, void *dirent, filldir_t filldir)
{
#ifdef MY_ABC_HERE
	struct inode *inode = NULL;
	struct super_block *sb = NULL;
#else
	struct inode *inode = filp->f_path.dentry->d_inode;
	struct super_block *sb = inode->i_sb;
#endif
	int len, err;
	char strbuf[HFSPLUS_MAX_STRLEN + 1];
	hfsplus_cat_entry entry;
	struct hfs_find_data fd;
	struct hfsplus_readdir_data *rd;
	u16 type;

#ifdef MY_ABC_HERE
	mutex_lock(&syno_hfsplus_global_mutex);
	inode = filp->f_path.dentry->d_inode;
	sb = inode->i_sb;
#endif
	if (filp->f_pos >= inode->i_size) {
#ifdef MY_ABC_HERE
		mutex_unlock(&syno_hfsplus_global_mutex);
#endif
		return 0;
	}
	hfs_find_init(HFSPLUS_SB(sb).cat_tree, &fd);
	hfsplus_cat_build_key(sb, fd.search_key, inode->i_ino, NULL);
	err = hfs_brec_find(&fd, hfs_find_rec_by_key);
	if (err)
		goto out;

	switch ((u32)filp->f_pos) {
	case 0:
		/* This is completely artificial... */
		if (filldir(dirent, ".", 1, 0, inode->i_ino, DT_DIR))
			goto out;
		filp->f_pos++;
		/* fall through */
	case 1:
		hfs_bnode_read(fd.bnode, &entry, fd.entryoffset, fd.entrylength);
		if (be16_to_cpu(entry.type) != HFSPLUS_FOLDER_THREAD) {
			printk(KERN_ERR "hfs: bad catalog folder thread\n");
			err = -EIO;
			goto out;
		}
		if (fd.entrylength < HFSPLUS_MIN_THREAD_SZ) {
			printk(KERN_ERR "hfs: truncated catalog thread\n");
			err = -EIO;
			goto out;
		}
		if (filldir(dirent, "..", 2, 1,
			    be32_to_cpu(entry.thread.parentID), DT_DIR))
			goto out;
		filp->f_pos++;
		/* fall through */
	default:
		if (filp->f_pos >= inode->i_size)
			goto out;
		err = hfs_brec_goto(&fd, filp->f_pos - 1);
		if (err)
			goto out;
	}

	for (;;) {
		if (be32_to_cpu(fd.key->cat.parent) != inode->i_ino) {
			printk(KERN_ERR "hfs: walked past end of dir\n");
			err = -EIO;
			goto out;
		}
		hfs_bnode_read(fd.bnode, &entry, fd.entryoffset, fd.entrylength);
		type = be16_to_cpu(entry.type);
		len = HFSPLUS_MAX_STRLEN;
		err = hfsplus_uni2asc(sb, &fd.key->cat.name, strbuf, &len);
		if (err)
			goto out;
		if (type == HFSPLUS_FOLDER) {
			if (fd.entrylength < sizeof(struct hfsplus_cat_folder)) {
				printk(KERN_ERR "hfs: small dir entry\n");
				err = -EIO;
				goto out;
			}
			if (HFSPLUS_SB(sb).hidden_dir &&
			    HFSPLUS_SB(sb).hidden_dir->i_ino == be32_to_cpu(entry.folder.id))
				goto next;
			if (filldir(dirent, strbuf, len, filp->f_pos,
				    be32_to_cpu(entry.folder.id), DT_DIR))
				break;
		} else if (type == HFSPLUS_FILE) {
			if (fd.entrylength < sizeof(struct hfsplus_cat_file)) {
				printk(KERN_ERR "hfs: small file entry\n");
				err = -EIO;
				goto out;
			}
			if (filldir(dirent, strbuf, len, filp->f_pos,
				    be32_to_cpu(entry.file.id), DT_REG))
				break;
		} else {
			printk(KERN_ERR "hfs: bad catalog entry type\n");
			err = -EIO;
			goto out;
		}
	next:
		filp->f_pos++;
		if (filp->f_pos >= inode->i_size)
			goto out;
		err = hfs_brec_goto(&fd, 1);
		if (err)
			goto out;
	}
	rd = filp->private_data;
	if (!rd) {
		rd = kmalloc(sizeof(struct hfsplus_readdir_data), GFP_KERNEL);
		if (!rd) {
			err = -ENOMEM;
			goto out;
		}
		filp->private_data = rd;
		rd->file = filp;
		list_add(&rd->list, &HFSPLUS_I(inode).open_dir_list);
	}
	memcpy(&rd->key, fd.key, sizeof(struct hfsplus_cat_key));
out:
	hfs_find_exit(&fd);
#ifdef MY_ABC_HERE
	mutex_unlock(&syno_hfsplus_global_mutex);
#endif
	return err;
}

static int hfsplus_dir_release(struct inode *inode, struct file *file)
{
#ifdef MY_ABC_HERE
	struct hfsplus_readdir_data *rd = NULL;
	mutex_lock(&syno_hfsplus_global_mutex);
	rd = file->private_data;
#else
	struct hfsplus_readdir_data *rd = file->private_data;
#endif
	if (rd) {
		list_del(&rd->list);
		kfree(rd);
	}
#ifdef MY_ABC_HERE
	mutex_unlock(&syno_hfsplus_global_mutex);
#endif
	return 0;
}

static int hfsplus_create(struct inode *dir, struct dentry *dentry, int mode,
			  struct nameidata *nd)
{
	struct inode *inode;
	int res;

#ifdef MY_ABC_HERE
	mutex_lock(&syno_hfsplus_global_mutex);
#endif
	inode = hfsplus_new_inode(dir->i_sb, mode);
	if (!inode) {
#ifdef MY_ABC_HERE
		mutex_unlock(&syno_hfsplus_global_mutex);
#endif
		return -ENOSPC;
	}

	res = hfsplus_create_cat(inode->i_ino, dir, &dentry->d_name, inode);
	if (res) {
		inode->i_nlink = 0;
		hfsplus_delete_inode(inode);
		iput(inode);
#ifdef MY_ABC_HERE
		mutex_unlock(&syno_hfsplus_global_mutex);
#endif
		return res;
	}
	hfsplus_instantiate(dentry, inode, inode->i_ino);
	mark_inode_dirty(inode);
#ifdef MY_ABC_HERE
	mutex_unlock(&syno_hfsplus_global_mutex);
#endif
	return 0;
}

static int hfsplus_link(struct dentry *src_dentry, struct inode *dst_dir,
			struct dentry *dst_dentry)
{
#ifdef MY_ABC_HERE
	struct super_block *sb = NULL;
	struct inode *inode = NULL;
	struct inode *src_dir = NULL;
#else
	struct super_block *sb = dst_dir->i_sb;
	struct inode *inode = src_dentry->d_inode;
	struct inode *src_dir = src_dentry->d_parent->d_inode;
#endif
	struct qstr str;
	char name[32];
	u32 cnid, id;
	int res;

#ifdef MY_ABC_HERE
	mutex_lock(&syno_hfsplus_global_mutex);
	sb = dst_dir->i_sb;
	inode = src_dentry->d_inode;
	src_dir = src_dentry->d_parent->d_inode;
#endif
	if (HFSPLUS_IS_RSRC(inode)) {
#ifdef MY_ABC_HERE
		mutex_unlock(&syno_hfsplus_global_mutex);
#endif
		return -EPERM;
	}

	if (inode->i_ino == (u32)(unsigned long)src_dentry->d_fsdata) {
		for (;;) {
			get_random_bytes(&id, sizeof(cnid));
			id &= 0x3fffffff;
			str.name = name;
			str.len = sprintf(name, "iNode%d", id);
			res = hfsplus_rename_cat(inode->i_ino,
						 src_dir, &src_dentry->d_name,
						 HFSPLUS_SB(sb).hidden_dir, &str);
			if (!res)
				break;
			if (res != -EEXIST) {
#ifdef MY_ABC_HERE
				mutex_unlock(&syno_hfsplus_global_mutex);
#endif
				return res;
			}
		}
		HFSPLUS_I(inode).dev = id;
		cnid = HFSPLUS_SB(sb).next_cnid++;
		src_dentry->d_fsdata = (void *)(unsigned long)cnid;
		res = hfsplus_create_cat(cnid, src_dir, &src_dentry->d_name, inode);
		if (res) {
			/* panic? */
#ifdef MY_ABC_HERE
			mutex_unlock(&syno_hfsplus_global_mutex);
#endif
			return res;
		}
		HFSPLUS_SB(sb).file_count++;
	}
	cnid = HFSPLUS_SB(sb).next_cnid++;
	res = hfsplus_create_cat(cnid, dst_dir, &dst_dentry->d_name, inode);
	if (res) {
#ifdef MY_ABC_HERE
		mutex_unlock(&syno_hfsplus_global_mutex);
#endif
		return res;
	}

	inc_nlink(inode);
	hfsplus_instantiate(dst_dentry, inode, cnid);
	atomic_inc(&inode->i_count);
	inode->i_ctime = CURRENT_TIME_SEC;
	mark_inode_dirty(inode);
	HFSPLUS_SB(sb).file_count++;
	sb->s_dirt = 1;
#ifdef MY_ABC_HERE
	mutex_unlock(&syno_hfsplus_global_mutex);
#endif

	return 0;
}

static int hfsplus_unlink(struct inode *dir, struct dentry *dentry)
{
#ifdef MY_ABC_HERE
	struct super_block *sb = NULL;
	struct inode *inode = NULL;
#else
	struct super_block *sb = dir->i_sb;
	struct inode *inode = dentry->d_inode;
#endif
	struct qstr str;
	char name[32];
	u32 cnid;
	int res;

#ifdef MY_ABC_HERE
	mutex_lock(&syno_hfsplus_global_mutex);
	sb = dir->i_sb;
	inode = dentry->d_inode;
#endif
	if (HFSPLUS_IS_RSRC(inode)) {
#ifdef MY_ABC_HERE
		mutex_unlock(&syno_hfsplus_global_mutex);
#endif
		return -EPERM;
	}

	cnid = (u32)(unsigned long)dentry->d_fsdata;
	if (inode->i_ino == cnid &&
	    atomic_read(&HFSPLUS_I(inode).opencnt)) {
		str.name = name;
		str.len = sprintf(name, "temp%lu", inode->i_ino);
		res = hfsplus_rename_cat(inode->i_ino,
					 dir, &dentry->d_name,
					 HFSPLUS_SB(sb).hidden_dir, &str);
		if (!res)
			inode->i_flags |= S_DEAD;
#ifdef MY_ABC_HERE
		mutex_unlock(&syno_hfsplus_global_mutex);
#endif
		return res;
	}
	res = hfsplus_delete_cat(cnid, dir, &dentry->d_name);
	if (res) {
#ifdef MY_ABC_HERE
		mutex_unlock(&syno_hfsplus_global_mutex);
#endif
		return res;
	}

	if (inode->i_nlink > 0)
		drop_nlink(inode);
	if (inode->i_ino == cnid)
		clear_nlink(inode);
	if (!inode->i_nlink) {
		if (inode->i_ino != cnid) {
			HFSPLUS_SB(sb).file_count--;
			if (!atomic_read(&HFSPLUS_I(inode).opencnt)) {
				res = hfsplus_delete_cat(inode->i_ino,
							 HFSPLUS_SB(sb).hidden_dir,
							 NULL);
				if (!res)
					hfsplus_delete_inode(inode);
			} else
				inode->i_flags |= S_DEAD;
		} else
			hfsplus_delete_inode(inode);
	} else
		HFSPLUS_SB(sb).file_count--;
	inode->i_ctime = CURRENT_TIME_SEC;
	mark_inode_dirty(inode);

#ifdef MY_ABC_HERE
	mutex_unlock(&syno_hfsplus_global_mutex);
#endif
	return res;
}

static int hfsplus_mkdir(struct inode *dir, struct dentry *dentry, int mode)
{
	struct inode *inode;
	int res;

#ifdef MY_ABC_HERE
	mutex_lock(&syno_hfsplus_global_mutex);
#endif
	inode = hfsplus_new_inode(dir->i_sb, S_IFDIR | mode);
	if (!inode) {
#ifdef MY_ABC_HERE
		mutex_unlock(&syno_hfsplus_global_mutex);
#endif
		return -ENOSPC;
	}

	res = hfsplus_create_cat(inode->i_ino, dir, &dentry->d_name, inode);
	if (res) {
		inode->i_nlink = 0;
		hfsplus_delete_inode(inode);
		iput(inode);
#ifdef MY_ABC_HERE
		mutex_unlock(&syno_hfsplus_global_mutex);
#endif
		return res;
	}
	hfsplus_instantiate(dentry, inode, inode->i_ino);
	mark_inode_dirty(inode);
#ifdef MY_ABC_HERE
	mutex_unlock(&syno_hfsplus_global_mutex);
#endif
	return 0;
}

static int hfsplus_rmdir(struct inode *dir, struct dentry *dentry)
{
	struct inode *inode;
	int res;

#ifdef MY_ABC_HERE
	mutex_lock(&syno_hfsplus_global_mutex);
#endif
	inode = dentry->d_inode;
	if (inode->i_size != 2) {
#ifdef MY_ABC_HERE
		mutex_unlock(&syno_hfsplus_global_mutex);
#endif
		return -ENOTEMPTY;
	}
	res = hfsplus_delete_cat(inode->i_ino, dir, &dentry->d_name);
	if (res) {
#ifdef MY_ABC_HERE
		mutex_unlock(&syno_hfsplus_global_mutex);
#endif
		return res;
	}
	clear_nlink(inode);
	inode->i_ctime = CURRENT_TIME_SEC;
	hfsplus_delete_inode(inode);
	mark_inode_dirty(inode);
#ifdef MY_ABC_HERE
	mutex_unlock(&syno_hfsplus_global_mutex);
#endif
	return 0;
}

static int hfsplus_symlink(struct inode *dir, struct dentry *dentry,
			   const char *symname)
{
	struct super_block *sb;
	struct inode *inode;
	int res;

#ifdef MY_ABC_HERE
	mutex_lock(&syno_hfsplus_global_mutex);
#endif
	sb = dir->i_sb;
	inode = hfsplus_new_inode(sb, S_IFLNK | S_IRWXUGO);
	if (!inode) {
#ifdef MY_ABC_HERE
		mutex_unlock(&syno_hfsplus_global_mutex);
#endif
		return -ENOSPC;
	}

	res = page_symlink(inode, symname, strlen(symname) + 1);
	if (res) {
		inode->i_nlink = 0;
		hfsplus_delete_inode(inode);
		iput(inode);
#ifdef MY_ABC_HERE
		mutex_unlock(&syno_hfsplus_global_mutex);
#endif
		return res;
	}

	mark_inode_dirty(inode);
	res = hfsplus_create_cat(inode->i_ino, dir, &dentry->d_name, inode);

	if (!res) {
		hfsplus_instantiate(dentry, inode, inode->i_ino);
		mark_inode_dirty(inode);
	}
#ifdef MY_ABC_HERE
	mutex_unlock(&syno_hfsplus_global_mutex);
#endif

	return res;
}

static int hfsplus_mknod(struct inode *dir, struct dentry *dentry,
			 int mode, dev_t rdev)
{
	struct super_block *sb;
	struct inode *inode;
	int res;

#ifdef MY_ABC_HERE
	mutex_lock(&syno_hfsplus_global_mutex);
#endif
	sb = dir->i_sb;
	inode = hfsplus_new_inode(sb, mode);
	if (!inode) {
#ifdef MY_ABC_HERE
		mutex_unlock(&syno_hfsplus_global_mutex);
#endif
		return -ENOSPC;
	}

	res = hfsplus_create_cat(inode->i_ino, dir, &dentry->d_name, inode);
	if (res) {
		inode->i_nlink = 0;
		hfsplus_delete_inode(inode);
		iput(inode);
#ifdef MY_ABC_HERE
		mutex_unlock(&syno_hfsplus_global_mutex);
#endif
		return res;
	}
	init_special_inode(inode, mode, rdev);
	hfsplus_instantiate(dentry, inode, inode->i_ino);
	mark_inode_dirty(inode);
#ifdef MY_ABC_HERE
	mutex_unlock(&syno_hfsplus_global_mutex);
#endif

	return 0;
}

static int hfsplus_rename(struct inode *old_dir, struct dentry *old_dentry,
			  struct inode *new_dir, struct dentry *new_dentry)
{
	int res;

#ifdef MY_ABC_HERE
	mutex_lock(&syno_hfsplus_global_mutex);
#endif
	/* Unlink destination if it already exists */
	if (new_dentry->d_inode) {
		res = hfsplus_unlink(new_dir, new_dentry);
		if (res) {
#ifdef MY_ABC_HERE
			mutex_unlock(&syno_hfsplus_global_mutex);
#endif
			return res;
		}
	}

	res = hfsplus_rename_cat((u32)(unsigned long)old_dentry->d_fsdata,
				 old_dir, &old_dentry->d_name,
				 new_dir, &new_dentry->d_name);
	if (!res)
		new_dentry->d_fsdata = old_dentry->d_fsdata;
#ifdef MY_ABC_HERE
	mutex_unlock(&syno_hfsplus_global_mutex);
#endif
	return res;
}

const struct inode_operations hfsplus_dir_inode_operations = {
	.lookup		= hfsplus_lookup,
	.create		= hfsplus_create,
	.link		= hfsplus_link,
	.unlink		= hfsplus_unlink,
	.mkdir		= hfsplus_mkdir,
	.rmdir		= hfsplus_rmdir,
	.symlink	= hfsplus_symlink,
	.mknod		= hfsplus_mknod,
	.rename		= hfsplus_rename,
#ifdef MY_ABC_HERE
	.setxattr		= hfsplus_syno_setxattr,
	.getxattr		= hfsplus_syno_getxattr,
#else
	.setxattr		= hfsplus_setxattr,
	.getxattr		= hfsplus_getxattr,
#endif
	.listxattr		= hfsplus_listxattr,
	.removexattr		= hfsplus_removexattr,
};

const struct file_operations hfsplus_dir_operations = {
	.read		= generic_read_dir,
	.readdir	= hfsplus_readdir,
	.ioctl          = hfsplus_ioctl,
	.llseek		= generic_file_llseek,
	.release	= hfsplus_dir_release,
};
