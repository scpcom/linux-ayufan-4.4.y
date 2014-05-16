/*
 * Comcerto OTP access
 *
 * (C) Copyright 2014 Google Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

extern int otp_read(u32 offset, u8* read_data, int size);

static int islocked_proc_show(struct seq_file *m, void *v)
{
	char status_buf[1];

	if (!otp_read(8, status_buf, 1)) {
		printk(KERN_ERR "comcerto_otp: Unable to read from OTP!\n");
		return -1;
	}

	seq_printf(m, "%d\n", (*status_buf & 0x2) == 0x2);
	return 0;
}

static int islocked_proc_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, islocked_proc_show, NULL);
}

static struct file_operations islocked_proc_fops = {
	.owner = THIS_MODULE,
	.open = islocked_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static struct proc_dir_entry *otp_dir;

static int __init comcerto_otp_init(void)
{
	struct proc_dir_entry *islocked_proc_file;

	otp_dir = proc_mkdir("otp", NULL);
	if (otp_dir == NULL) {
		printk(KERN_ERR "comcerto_otp: Unable to create/proc/otp directory\n");
		return -ENOMEM;
	}

	islocked_proc_file = proc_create("islocked", 0, otp_dir, &islocked_proc_fops);
	if (islocked_proc_file == NULL) {
		printk(KERN_ERR "comcerto_otp: Unable to create /proc/otp/islocked\n");
		remove_proc_entry("otp", NULL);
		return -ENOMEM;
	}

	printk(KERN_INFO "comcerto_otp: Created /proc/otp/islocked\n");
	return 0;
}

static void __exit comcerto_otp_exit(void)
{
	remove_proc_entry("islocked", otp_dir);
	remove_proc_entry("otp", NULL);
}

module_init(comcerto_otp_init);
module_exit(comcerto_otp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Stephen McGruer <smcgruer@google.com>");
MODULE_DESCRIPTION("Comcerto OTP driver");
