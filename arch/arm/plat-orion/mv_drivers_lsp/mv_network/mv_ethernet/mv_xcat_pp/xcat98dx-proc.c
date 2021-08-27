/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
*******************************************************************************/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>

#include "xcat98dx-core.h"

static struct proc_dir_entry *xcat98dx_proc;

int xcat98dx_proc_read(char *buffer, char **buffer_location, off_t offset,
		       int buffer_length, int *zero, void *ptr)
{
	if (offset > 0)
		return 0;
	return sprintf(buffer, "%d\n", mvPpMruGet(MV_PP_DEV0, 0));
}

int __init xcat98dx_proc_init(void)
{
	xcat98dx_proc = create_proc_entry("xcat98dx", 0666, NULL);
	xcat98dx_proc->read_proc = xcat98dx_proc_read;
	xcat98dx_proc->write_proc = NULL ; /* xcat98dx_proc_write; */
	xcat98dx_proc->nlink = 1;
	return 0;
}

module_init(xcat98dx_proc_init);
