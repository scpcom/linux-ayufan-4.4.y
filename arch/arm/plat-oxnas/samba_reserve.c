/*
 * arch/arm/plat-oxnas/samba_reserve.c
 *
 * Copyright (C) 2008, 2009 Oxford Semiconductor Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/errno.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/security.h>
#include <linux/syscalls.h>
#include <asm/uaccess.h>

static int do_preallocate(struct file *file, loff_t offset, loff_t len)
{
	int ret;

	if (!file->f_op->preallocate)
		return -EOPNOTSUPP;

	if (offset < 0 || len <= 0)
		return -EINVAL;

	if (!(file->f_mode & FMODE_WRITE))
		return -EBADF;

	ret = security_file_permission(file, MAY_WRITE);
	if (ret)
		return ret;

//printk("do_preallocate() Call preallocate() %p, %lld, %lld\n", file, offset, len);
	ret = file->f_op->preallocate(file, offset, len);
	
	if(ret >= 0) { /* set the inode flag */
		struct dentry *dentry;
		struct inode	*inode;

		dentry = file->f_path.dentry;
		inode = dentry->d_inode;
		
		inode->space_reserve = 1;
	}
	
	return ret;
}

SYSCALL_DEFINE3(samba_reserve, int, fd, loff_t __user*, start, loff_t __user*, length)
{
	struct file *file;
	long         ret = -EBADF;
	loff_t       st = 0, len = 0;

	file = fget(fd);
	if (!file)
		goto no_file_out;

	if (!start) {
		ret = -EINVAL;
		goto out;
	}

	if (!length) {
		ret = -EINVAL;
		goto out;
	}

	if (unlikely(copy_from_user(&st, start, sizeof(loff_t)))) {
		ret = -EFAULT;
		goto out;
	}

	if (unlikely(copy_from_user(&len, length, sizeof(loff_t)))) {
		ret = -EFAULT;
		goto out;
	}

//printk("samba_reserve() %p, %lld, %lld\n", file, st, len);
	ret = do_preallocate(file, st, len);

out:
	fput(file);
no_file_out:
	return ret;
}
