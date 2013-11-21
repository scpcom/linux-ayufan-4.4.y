/*
 * Copyright (C) 2010 Google, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* CONFIG_BOOTLOG_COPY */

#include <linux/bootmem.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/ioport.h>
#include <linux/list.h>
#include <linux/vmalloc.h>
#include <linux/compiler.h>
#include <linux/module.h>

#define BOOTLOG_MAXSIZE		(1024*1024)

static unsigned long bootlog_addr = 0;
static unsigned long bootlog_size = 0;
static int bootlog_is_reserved = 0;

unsigned long bootlog_get_addr(void)
{

	if (!bootlog_addr)
		return 0;

	if (!bootlog_is_reserved) {
		if (reserve_bootmem(bootlog_addr, bootlog_size, BOOTMEM_EXCLUSIVE)) {
			printk(KERN_WARNING "bootlog: reserve_bootmem(0x%lx, 0x%lx) failed\n", bootlog_addr, bootlog_size);
			return 0;
		}
	}
	return bootlog_addr;
}

unsigned long bootlog_get_size(void)
{
	if (!bootlog_addr)
		return 0;

	return bootlog_size;
}

void bootlog_free(void)
{
	if (bootlog_is_reserved) {
	      free_bootmem(bootlog_addr, bootlog_size);
	}
	bootlog_addr = bootlog_size = 0;
}

static int __init bootlog_setup(char *str)
{
	unsigned long addr = 0;
	unsigned long size;

	size = (unsigned long) memparse(str, &str);
	if (*str == '@')
		addr = (unsigned long)memparse(str + 1, &str);

	if (!addr) {
		printk(KERN_WARNING "bootlog: address and size must not be 0,"
		       " ignoring range '%s'\n", str);
		return 0;
	}

	bootlog_addr = addr;
	bootlog_size = size;

	return 0;
}

early_param("bootlog", bootlog_setup);
