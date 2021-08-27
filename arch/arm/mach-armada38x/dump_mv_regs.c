/*
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <asm/hardware/cache-l2x0.h>
#include "ca9x2.h"
#include "ctrlEnv/mvCtrlEnvLib.h"

static int proc_dump_cp15_read(char *page, char **start, off_t off, int count,
			       int *eof, void *data)
{
	char *p = page;
	int len;
	unsigned int value;

	asm volatile ("mrc p15, 0, %0, c0, c0, 0" : "=r" (value));
	p += sprintf(p, "Main ID: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c0, 1" : "=r" (value));
	p += sprintf(p, "Cache Type: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c0, 2" : "=r" (value));
	p += sprintf(p, "TCM Type: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c0, 3" : "=r" (value));
	p += sprintf(p, "TLB Type: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c0, 5" : "=r" (value));
	p += sprintf(p, "Microprocessor Affinity: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c1, 0" : "=r" (value));
	p += sprintf(p, "Processor Feature 0: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c1, 1" : "=r" (value));
	p += sprintf(p, "Processor Feature 1: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c1, 2" : "=r" (value));
	p += sprintf(p, "Debug Feature 0: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c1, 3" : "=r" (value));
	p += sprintf(p, "Auxiliary Feature 0: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c1, 4" : "=r" (value));
	p += sprintf(p, "Memory Model Feature 0: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c1, 5" : "=r" (value));
	p += sprintf(p, "Memory Model Feature 1: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c1, 6" : "=r" (value));
	p += sprintf(p, "Memory Model Feature 2: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c1, 7" : "=r" (value));
	p += sprintf(p, "Memory Model Feature 3: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c2, 0" : "=r" (value));
	p += sprintf(p, "Set Attribute 0: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c2, 1" : "=r" (value));
	p += sprintf(p, "Set Attribute 1: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c2, 2" : "=r" (value));
	p += sprintf(p, "Set Attribute 2: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c2, 3" : "=r" (value));
	p += sprintf(p, "Set Attribute 3: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c2, 4" : "=r" (value));
	p += sprintf(p, "Set Attribute 4: 0x%08x\n", value);

	asm volatile ("mrc p15, 1, %0, c0, c0, 0" : "=r" (value));
	p += sprintf(p, "Current Cache Size ID: 0x%08x\n", value);

	asm volatile ("mrc p15, 1, %0, c0, c0, 1" : "=r" (value));
	p += sprintf(p, "Current Cache Level ID: 0x%08x\n", value);

	asm volatile ("mrc p15, 1, %0, c0, c0, 7" : "=r" (value));
	p += sprintf(p, "Auxiliary ID: 0x%08x\n", value);

	asm volatile ("mrc p15, 2, %0, c0, c0, 0" : "=r" (value));
	p += sprintf(p, "Cache Size Selection: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r" (value));
	p += sprintf(p, "Control : 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c1, c0, 1" : "=r" (value));
	p += sprintf(p, "Auxiliary Control : 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c1, c0, 2" : "=r" (value));
	p += sprintf(p, "Coprocessor Access Control : 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c1, c1, 0" : "=r" (value));
	p += sprintf(p, "Secure Configuration : 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c2, c0, 0" : "=r" (value));
	p += sprintf(p, "Translation Table Base 0 : 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c2, c0, 1" : "=r" (value));
	p += sprintf(p, "Translation Table Base 1 : 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c2, c0, 2" : "=r" (value));
	p += sprintf(p, "Translation Table Control : 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c3, c0, 0" : "=r" (value));
	p += sprintf(p, "Domain Access Control : 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c5, c0, 0" : "=r" (value));
	p += sprintf(p, "Data Fault Status : 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c5, c0, 1" : "=r" (value));
	p += sprintf(p, "Instruction Fault Status : 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c5, c1, 0" : "=r" (value));
	p += sprintf(p, "Auxiliary Data Fault Status : 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c5, c1, 1" : "=r" (value));
	p += sprintf(p, "Auxiliary Instruction Fault Status : 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c6, c0, 0" : "=r" (value));
	p += sprintf(p, "Data Fault Address : 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c6, c0, 2" : "=r" (value));
	p += sprintf(p, "Instruction Fault Address : 0x%08x\n", value);

	asm volatile ("mrc p15, 4, %0, c15, c0, 0" : "=r" (value));
	p += sprintf(p, "Configuration Base Address: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c10, c2, 0" : "=r" (value));
	p += sprintf(p, "Memory Attribute PRRR: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c10, c2, 1" : "=r" (value));
	p += sprintf(p, "Memory Attribute NMRR: 0x%08x\n", value);

	len = (p - page) - off;
	if (len < 0)
		len = 0;

	*eof = (len <= count) ? 1 : 0;
	*start = page + off;

	return len;
}

static int proc_dump_preformance_regs_read(char *page, char **start, off_t off, int count,
			       int *eof, void *data)
{
	char *p = page;
	int len, offset;
	void __iomem *addr;
	unsigned int value;
	addr = (void __iomem *)INTER_REGS_VIRT_BASE;

	/* SCU registers */
	p += sprintf(p, "SCU registers:\n");
	offset = A9_MPCORE_SCU;
	value = readl(addr + offset);
	p += sprintf(p, "MPCA9 SCU Control Register (0x%05x): 0x%08x\n", offset, value);

	/* L2C registers */
	p += sprintf(p, "L2C registers:\n");
	offset = MV_CA9X2_L2CC_OFFSET;
	value = readl(addr + offset);
	p += sprintf(p, "L2C-310  Control Register (0x%05x): 0x%08x\n", offset, value);

	offset = MV_CA9X2_L2CC_OFFSET + L2X0_AUX_CTRL;
	value = readl(addr + offset);
	p += sprintf(p, "L2C-310 Auxiliary Control Register (0x%05x): 0x%08x\n", offset, value);

	offset = MV_CA9X2_L2CC_OFFSET + L2X0_TAG_LATENCY_CTRL;
	value = readl(addr + offset);
	p += sprintf(p, "L2C-310 Tag and Data RAM latency Control Register (0x%05x): 0x%08x\n", offset, value);

	offset = MV_CA9X2_L2CC_OFFSET + L2X0_PREFETCH_CTRL;
	value = readl(addr + offset);
	p += sprintf(p, "L2C-310 Prefetch Control Register (0x%05x): 0x%08x\n", offset, value);

	/* CIB registers */
	p += sprintf(p, "CIB registers:\n");
	for (offset = 0x20280; offset < 0x2029C; offset += 0x4) {
		value = readl(addr + offset);
		p += sprintf(p, "CIB register (0x%05x): 0x%08x\n", offset, value);
	}

	/* MBUS/Fabric */
	p += sprintf(p, "MBUS/Fabric registers:\n");
	for (offset = 0x20420; offset < 0x20430; offset += 0x4) {
		value = readl(addr + offset);
		p += sprintf(p, "MBUS/Fabric register (0x%05x): 0x%08x\n", offset, value);
	}

	/* XOR Outstanding registers */
	p += sprintf(p, "XOR Outstanding registers:\n");
	for (offset = 0x60880; offset < 0x60988; offset += 0x4) {
		if (offset == 0x60888)
			offset = 0x60980;
		value = readl(addr + offset);
		p += sprintf(p, "MBUS/Fabric register (0x%05x): 0x%08x\n", offset, value);
	}

	len = (p - page) - off;
	if (len < 0)
		len = 0;

	*eof = (len <= count) ? 1 : 0;
	*start = page + off;

	return len;
}

static int proc_dump_dram_regs_read(char *page, char **start, off_t off, int count,
					    int *eof, void *data)
{
	char *p = page;
	int len, offset;
	void __iomem *addr;
	unsigned int value;
	addr = (void __iomem *)INTER_REGS_VIRT_BASE;

	/* SDRAM registers */
	p += sprintf(p, "SDRAM registers:\n");
	for (offset = 0x1400; offset < 0x1890; offset += 0x4) {
		if (offset == 0x143C)
			offset = 0x1470;
		if (offset == 0x14AC)
			offset = 0x14F4;
		if (offset == 0x14FC)
			offset = 0x1538;
		if (offset == 0x153D)
			offset = 0x15D0;
		if (offset == 0x15E4)
			offset = 0x16D4;
		if (offset == 0x16D8)
			offset = 0x1700;
		if (offset == 0x1718)
			offset = 0x1870;
		value = readl(addr + offset);
		p += sprintf(p, "SDRAM reg (0x%05x): 0x%08x\n", offset, value);
	}

	/* SDRAM Windows registers */
	p += sprintf(p, "DRAM windows registers:\n");
	for (offset = 0x20180; offset < 0x201a0; offset += 0x4) {
		value = readl(addr + offset);
		p += sprintf(p, "SDRAM windows (0x%05x): 0x%08x\n", offset, value);
	}

	len = (p - page) - off;
	if (len < 0)
		len = 0;

	*eof = (len <= count) ? 1 : 0;
	*start = page + off;

	return len;
}

int dump_init_module(void)
{
#ifdef CONFIG_PROC_FS
	struct proc_dir_entry *res_cp15, *res_preformance_regs, *res_dram_regs;
	res_cp15 = create_proc_entry("mv_dump_cp15", S_IRUSR, NULL);
	if (!res_cp15)
		return -ENOMEM;
	res_cp15->read_proc = proc_dump_cp15_read;

	res_preformance_regs = create_proc_entry("mv_dump_performance_regs", S_IRUSR, NULL);
	if (!res_preformance_regs)
		return -ENOMEM;
	res_preformance_regs->read_proc = proc_dump_preformance_regs_read;

	res_dram_regs = create_proc_entry("mv_dump_dram_regs", S_IRUSR, NULL);
	if (!res_dram_regs)
		return -ENOMEM;
	res_dram_regs->read_proc = proc_dump_dram_regs_read;
#endif

	return 0;
}

void dump_cleanup_module(void)
{
	remove_proc_entry("mv_dump_cp15", NULL);
	remove_proc_entry("mv_dump_dram_regs", NULL);
	remove_proc_entry("mv_dump_performance_regs", NULL);
}

module_init(dump_init_module);
module_exit(dump_cleanup_module);

MODULE_AUTHOR("Nadav Haklai");
MODULE_LICENSE("GPL");
