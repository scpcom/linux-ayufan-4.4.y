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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/capability.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/platform_device.h>
#include "mvCommon.h"
#include "dbg-trace.h"

#define TRACE_ARR_LEN   800
#define STR_LEN         128

static inline int mv_trace_next_idx(int idx)
{
	idx++;
	if (idx == TRACE_ARR_LEN)
		idx = 0;

	return idx;
}

static inline int mv_trace_prev_idx(int idx)
{
	if (idx == 0)
		idx = TRACE_ARR_LEN;

	idx--;
	return idx;
}

struct trace {
	struct timeval tv;
	char str[STR_LEN];
	char valid;
};

struct trace *trc_arr[CONFIG_NR_CPUS];
static int trc_index[CONFIG_NR_CPUS];
static int trc_active;
static int trc_mode;

void TRC_START(void)
{
	trc_active = 1;
}

void TRC_STOP(void)
{
	trc_active = 0;
}

void TRC_MODE(int mode)
{
	trc_mode = mode;
}

int TRC_INIT(void)
{
	struct trace *trc;
	int cpu;

	printk(KERN_INFO "Marvell debug tracing is supported\n");

	for_each_possible_cpu(cpu) {

		trc = kmalloc(TRACE_ARR_LEN * sizeof(struct trace), GFP_KERNEL);
		if (trc == NULL) {
			printk(KERN_ERR "Can't allocate Debug Trace buffer\n");
			return 1;
		}
		memset(trc, 0, TRACE_ARR_LEN * sizeof(struct trace));
		trc_arr[cpu] = trc;
		trc_index[cpu] = 0;
		trc_active = 0;
		trc_mode = 0;
	}
	return 0;
}

void TRC_REC(char *fmt, ...)
{
	va_list args;
	int idx = trc_index[smp_processor_id()];
	struct trace *trc = &trc_arr[smp_processor_id()][idx];

	if (trc_active == 0)
		return;

	if (trc_mode == 1) {
		/* Stop when trace buffer is full */
		if (trc->valid) {
			printk(KERN_ERR "Trace stopped - buffer is full\n");
			TRC_STOP();
			return;
		}
	}
	do_gettimeofday(&trc->tv);
	va_start(args, fmt);
	vsprintf(trc->str, fmt, args);
	va_end(args);
	trc->valid = 1;

	trc_index[smp_processor_id()] = mv_trace_next_idx(idx);
}

/* cpu_mask:  0 - from running CPU only, -1 from all CPUs, 1..(1 << CONFIG_NR_CPUS) - 1 */
/* time_mode: 0 - time stamp normalized to oldest message, 1 - difference from previous message */
void TRC_OUTPUT(int cpu_mask, int time_mode)
{
	int i, last, next, cpu, active;
	struct trace *p;
	struct timeval *tv_base;
	bool   cpu_found = false;

	active = trc_active;
	trc_active = 0;
	if (cpu_mask == 0)
		cpu = smp_processor_id();
	else {
		for_each_possible_cpu(cpu) {
			if (MV_BIT_CHECK(cpu_mask, cpu)) {
				cpu_found = true;
				break;
			}
		}
	}
	if (cpu_found == false) {
		printk(KERN_ERR "%s: Wrong cpu_mask=0x%x\n", __func__, cpu_mask);
		return;
	}

	next = trc_index[cpu];
	last = mv_trace_prev_idx(next);
	p = &trc_arr[cpu][last];
	if (p->valid == 0) {
		printk(KERN_INFO "\nTrace: cpu=%d - No valid entries\n", cpu);
		return;
	}

	/* Find first valid entry */
	i = next;
	while (i != last) {
		p = &trc_arr[cpu][i];
		if (p->valid)
			break;
		i = mv_trace_next_idx(i);
	}

	tv_base = &trc_arr[cpu][i].tv;

	printk(KERN_INFO "\nTrace: cpu=%d, first=%d, last=%d, base time: %lu sec, %lu usec\n",
				cpu, i, last, tv_base->tv_sec, tv_base->tv_usec);
	printk(KERN_INFO "\n No CPU [s : ms : us]  message\n");
	do {
		unsigned int sec, msec, usec;

		p = &trc_arr[cpu][i];
		sec = p->tv.tv_sec - tv_base->tv_sec;
		if (p->tv.tv_usec >= tv_base->tv_usec)
			usec = (p->tv.tv_usec - tv_base->tv_usec);
		else {
			sec--;
			usec = 1000000 - (tv_base->tv_usec - p->tv.tv_usec);
		}
		msec = usec / 1000;
		usec = usec % 1000;
		printk(KERN_INFO "%03d: %d: [%02u:%03u:%03u]: ", i, cpu, sec, msec, usec);
		printk(KERN_CONT "%s", p->str);
		i = mv_trace_next_idx(i);
		if (time_mode == 1)
			tv_base = &p->tv;
	} while (i != next);

	memset(trc_arr[cpu], 0, TRACE_ARR_LEN * sizeof(struct trace));
	trc_index[cpu] = 0;
	trc_active = active;

}

void TRC_RELEASE(void)
{
	int cpu;

	for_each_possible_cpu(cpu) {

		kfree(trc_arr[cpu]);
		trc_index[cpu] = 0;
	}
}

void mv_trace_status(void)
{
	int cpu;

	printk(KERN_INFO "TRACE: strlen=%d, entries=%d, mode=%d, active=%d\n",
			STR_LEN, TRACE_ARR_LEN, trc_mode, trc_active);
	for_each_possible_cpu(cpu) {
		printk(KERN_INFO "cpu=%d, trc_index=%4d, trc_buffer=%p\n", cpu, trc_index[cpu], trc_arr[cpu]);
	}
}

static ssize_t mv_trace_help(char *buf)
{
	int off = 0;

	off += sprintf(buf+off, "cat           help   	- show this help\n");
	off += sprintf(buf+off, "cat           status 	- show trace buffer status\n");
	off += sprintf(buf+off, "echo [0|1]  > start	- stop/start trace record\n");
	off += sprintf(buf+off, "echo m      > mode     - set record mode: 0-overwrite, 1-stop on full \n");
	off += sprintf(buf+off, "echo c t    > dump     - dump trace buffer: <c>-cpu_mask, <t>-time mode\n");

	return off;
}

static ssize_t mv_trace_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	const char      *name = attr->attr.name;
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "status"))
		mv_trace_status();
	else
		off = mv_trace_help(buf);

	return off;
}

static ssize_t mv_trace_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err = 0;
	unsigned int    a, b;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read arguments */
	sscanf(buf, "%x %x", &a, &b);

	if (!strcmp(name, "start")) {
		if (a)
			TRC_START();
		else
			TRC_STOP();
	} else if (!strcmp(name, "mode"))
			TRC_MODE(a);
	else if (!strcmp(name, "dump"))
		TRC_OUTPUT(a, b);
	else {
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
		err = -EINVAL;
	}
	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help,        S_IRUSR, mv_trace_show, NULL);
static DEVICE_ATTR(status,      S_IRUSR, mv_trace_show, NULL);
static DEVICE_ATTR(start,       S_IWUSR, mv_trace_show, mv_trace_store);
static DEVICE_ATTR(mode,        S_IWUSR, mv_trace_show, mv_trace_store);
static DEVICE_ATTR(dump,        S_IWUSR, mv_trace_show, mv_trace_store);

static struct attribute *mv_trace_attrs[] = {

	&dev_attr_help.attr,
	&dev_attr_status.attr,
	&dev_attr_start.attr,
	&dev_attr_mode.attr,
	&dev_attr_dump.attr,
	NULL
};

static struct attribute_group mv_trace_group = {
	.name = "trace",
	.attrs = mv_trace_attrs,
};

int __devinit mv_trace_init(void)
{
	int err;
	struct device *pd;

	err = TRC_INIT();
	if (err) {
		printk(KERN_INFO "sysfs group failed %d\n", err);
		goto out;
	}

	pd = bus_find_device_by_name(&platform_bus_type, NULL, "neta");
	if (!pd) {
		platform_device_register_simple("neta", -1, NULL, 0);
		pd = bus_find_device_by_name(&platform_bus_type, NULL, "neta");
	}

	if (!pd) {
		printk(KERN_ERR"%s: cannot find neta device\n", __func__);
		pd = &platform_bus;
	}

	err = sysfs_create_group(&pd->kobj, &mv_trace_group);
	if (err) {
		printk(KERN_INFO "sysfs group failed %d\n", err);
		goto out;
	}
out:

	return err;
}

module_init(mv_trace_init);

MODULE_AUTHOR("Dima Epshtein");
MODULE_DESCRIPTION("Trace message support");
MODULE_LICENSE("GPL");
