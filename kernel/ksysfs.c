/*
 * kernel/ksysfs.c - sysfs attributes in /sys/kernel, which
 * 		     are not related to any other subsystem
 *
 * Copyright (C) 2004 Kay Sievers <kay.sievers@vrfy.org>
 * 
 * This file is release under the GPLv2
 *
 */

#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/export.h>
#include <linux/init.h>
#include <linux/kexec.h>
#include <linux/profile.h>
#include <linux/stat.h>
#include <linux/sched.h>
#include <linux/capability.h>

#define KERNEL_ATTR_RO(_name) \
static struct kobj_attribute _name##_attr = __ATTR_RO(_name)

#define KERNEL_ATTR_RW(_name) \
static struct kobj_attribute _name##_attr = \
	__ATTR(_name, 0644, _name##_show, _name##_store)

#if defined(CONFIG_HOTPLUG)
/* current uevent sequence number */
static ssize_t uevent_seqnum_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%llu\n", (unsigned long long)uevent_seqnum);
}
KERNEL_ATTR_RO(uevent_seqnum);

/* uevent helper program, used during early boot */
static ssize_t uevent_helper_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", uevent_helper);
}
static ssize_t uevent_helper_store(struct kobject *kobj,
				   struct kobj_attribute *attr,
				   const char *buf, size_t count)
{
	if (count+1 > UEVENT_HELPER_PATH_LEN)
		return -ENOENT;
	memcpy(uevent_helper, buf, count);
	uevent_helper[count] = '\0';
	if (count && uevent_helper[count-1] == '\n')
		uevent_helper[count-1] = '\0';
	return count;
}
KERNEL_ATTR_RW(uevent_helper);
#endif

#ifdef CONFIG_PROFILING
static ssize_t profiling_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", prof_on);
}
static ssize_t profiling_store(struct kobject *kobj,
				   struct kobj_attribute *attr,
				   const char *buf, size_t count)
{
	int ret;

	if (prof_on)
		return -EEXIST;
	/*
	 * This eventually calls into get_option() which
	 * has a ton of callers and is not const.  It is
	 * easiest to cast it away here.
	 */
	profile_setup((char *)buf);
	ret = profile_init();
	if (ret)
		return ret;
	ret = create_proc_profile();
	if (ret)
		return ret;
	return count;
}
KERNEL_ATTR_RW(profiling);
#endif

#ifdef CONFIG_KEXEC
static ssize_t kexec_loaded_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", !!kexec_image);
}
KERNEL_ATTR_RO(kexec_loaded);

static ssize_t kexec_crash_loaded_show(struct kobject *kobj,
				       struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", !!kexec_crash_image);
}
KERNEL_ATTR_RO(kexec_crash_loaded);

static ssize_t kexec_crash_size_show(struct kobject *kobj,
				       struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%zu\n", crash_get_memory_size());
}
static ssize_t kexec_crash_size_store(struct kobject *kobj,
				   struct kobj_attribute *attr,
				   const char *buf, size_t count)
{
	unsigned long cnt;
	int ret;

	if (strict_strtoul(buf, 0, &cnt))
		return -EINVAL;

	ret = crash_shrink_memory(cnt);
	return ret < 0 ? ret : count;
}
KERNEL_ATTR_RW(kexec_crash_size);

static ssize_t vmcoreinfo_show(struct kobject *kobj,
			       struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%lx %x\n",
		       paddr_vmcoreinfo_note(),
		       (unsigned int)vmcoreinfo_max_size);
}
KERNEL_ATTR_RO(vmcoreinfo);

#endif /* CONFIG_KEXEC */

/* whether file capabilities are enabled */
static ssize_t fscaps_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", file_caps_enabled);
}
KERNEL_ATTR_RO(fscaps);

#if defined(CONFIG_COMCERTO_MDMA_PROF)

#include <mach/stats.h>

static ssize_t comcerto_mdma_prof_enable_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	int n = 0;

	if (mdma_stats_enable)
		n += sprintf(buf + n, "MDMA profiling is enabled\n");
	else
		n += sprintf(buf + n, "MDMA profiling is disabled\n");

	return (n + 1);
}

static ssize_t comcerto_mdma_prof_enable_store(struct kobject *kobj,
				  struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int enable;

	if (kstrtouint(buf, 0, &enable))
		return -EINVAL;

	if (enable > 0)
		mdma_stats_enable = 1;
	else
		mdma_stats_enable = 0;

	return count;
}
KERNEL_ATTR_RW(comcerto_mdma_prof_enable);

static ssize_t comcerto_mdma_reqtiming_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	struct mdma_stats *stats = &mdma_stats;
	int i;
	int n = 0;

	n += sprintf(buf + n, "Histogram of mdma request time (us)\n");

	for (i = 0; i < MAX_BINS - 1; i++) {
		if (stats->reqtime_counter[i]) {
			n += sprintf(buf + n, "%8u in [%5d-%5d]\n", stats->reqtime_counter[i], i << US_SHIFT, (i + 1) << US_SHIFT);
			stats->reqtime_counter[i] = 0;
		}
	}

	if (stats->reqtime_counter[i]) {
		n += sprintf(buf + n, "%8u in [%5d-.....]\n", stats->reqtime_counter[i], i << US_SHIFT);
		stats->reqtime_counter[i] = 0;
	}

	return (n + 1);
}
KERNEL_ATTR_RO(comcerto_mdma_reqtiming);

static ssize_t comcerto_mdma_timing_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	struct mdma_stats *stats = &mdma_stats;
	int i;
	int n = 0;

	stats->init = 0;

	n += sprintf(buf + n, "Histogram of inter mdma request time (us)\n");

	for (i = 0; i < MAX_BINS - 1; i++) {
		if (stats->time_counter[i]) {
			n += sprintf(buf + n, "%8u in [%5d-%5d]\n", stats->time_counter[i], i << US_SHIFT, (i + 1) << US_SHIFT);
			stats->time_counter[i] = 0;
		}
	}

	if (stats->time_counter[i]) {
		n += sprintf(buf + n, "%8u in [%5d-.....]\n", stats->time_counter[i], i << US_SHIFT);
		stats->time_counter[i] = 0;
	}

	return (n + 1);
}
KERNEL_ATTR_RO(comcerto_mdma_timing);

static ssize_t comcerto_mdma_data_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	struct mdma_stats *stats = &mdma_stats;
	int i;
	int n = 0;

	n += sprintf(buf + n, "Histogram of mdma data length (KiB)\n");

	for (i = 0; i < MAX_BINS; i++) {
		if (stats->data_counter[i]) {
			n += sprintf(buf + n, "%8u in [%4d-%4d]\n", stats->data_counter[i], (i << BYTE_SHIFT) / 1024, ((i + 1) << BYTE_SHIFT) / 1024);
			stats->data_counter[i] = 0;
		}
	}

	return (n + 1);
}
KERNEL_ATTR_RO(comcerto_mdma_data);
#endif

#if defined(CONFIG_COMCERTO_SPLICE_PROF)

#include <mach/stats.h>

static ssize_t comcerto_splice_prof_enable_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	int n = 0;

	if (splice_stats_enable)
		n += sprintf(buf + n, "Splice profiling is enabled\n");
	else
		n += sprintf(buf + n, "Splice profiling is disabled\n");

	return (n + 1);
}

static ssize_t comcerto_splice_prof_enable_store(struct kobject *kobj,
				  struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int enable;

	if (kstrtouint(buf, 0, &enable))
		return -EINVAL;

	if (enable > 0)
		splice_stats_enable = 1;
	else
		splice_stats_enable = 0;

	return count;
}
KERNEL_ATTR_RW(comcerto_splice_prof_enable);

static ssize_t comcerto_splicew_reqtiming_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	struct splicew_stats *stats = &splicew_stats;
	int i;
	int n = 0;

	n += sprintf(buf + n, "Histogram of splice write time (us)\n");

	for (i = 0; i < MAX_BINS - 1; i++) {
		if (stats->reqtime_counter[i]) {
			n += sprintf(buf + n, "%8u in [%5d-%5d]\n", stats->reqtime_counter[i], i << US_SHIFT, (i + 1) << US_SHIFT);
			stats->reqtime_counter[i] = 0;
		}
	}

	if (stats->reqtime_counter[i]) {
		n += sprintf(buf + n, "%8u in [%5d-.....]\n", stats->reqtime_counter[i], i << US_SHIFT);
		stats->reqtime_counter[i] = 0;
	}

	return (n + 1);
}
KERNEL_ATTR_RO(comcerto_splicew_reqtiming);

static ssize_t comcerto_splicew_timing_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	struct splicew_stats *stats = &splicew_stats;
	int i;
	int n = 0;

	stats->init = 0;

	n += sprintf(buf + n, "Histogram of inter splice write time (us)\n");

	for (i = 0; i < MAX_BINS - 1; i++) {
		if (stats->time_counter[i]) {
			n += sprintf(buf + n, "%8u in [%5d-%5d]\n", stats->time_counter[i], i << US_SHIFT, (i + 1) << US_SHIFT);
			stats->time_counter[i] = 0;
		}
	}

	if (stats->time_counter[i]) {
		n += sprintf(buf + n, "%8u in [%5d-.....]\n", stats->time_counter[i], i << US_SHIFT);
		stats->time_counter[i] = 0;
	}

	return (n + 1);
}
KERNEL_ATTR_RO(comcerto_splicew_timing);

static ssize_t comcerto_splicew_data_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	struct splicew_stats *stats = &splicew_stats;
	int i;
	int n = 0;

	n += sprintf(buf + n, "Histogram of splice write data length (KiB)\n");

	for (i = 0; i < MAX_BINS; i++) {
		if (stats->data_counter[i]) {
			n += sprintf(buf + n, "%8u in [%4d-%4d]\n", stats->data_counter[i], (i << BYTE_SHIFT) / 1024, ((i + 1) << BYTE_SHIFT) / 1024);
			stats->data_counter[i] = 0;
		}
	}

	return (n + 1);
}
KERNEL_ATTR_RO(comcerto_splicew_data);

static ssize_t comcerto_splicew_rate_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	struct splicew_stats *stats = &splicew_stats;
	unsigned long total_mb = 0;
	int i;
	int n = 0;

	n += sprintf(buf + n, "Histogram of splice write rate (MiB/s)\n");

	for (i = 0; i < MAX_BINS; i++) {
		if (stats->rate_counter[i]) {
			n += sprintf(buf + n, "%8u in [%3d-%3d]\n", stats->rate_counter[i], i << RATE_SHIFT, (i + 1) << RATE_SHIFT);

			total_mb += stats->rate_counter[i] >> 10;
			stats->rate_counter[i] = 0;
		}
	}

	if (total_mb) {
		if (stats->active_us) {
			n += sprintf(buf + n, "Idle: %lu us, Active: %lu us\n", stats->idle_us, stats->active_us);
			n += sprintf(buf + n, "Global rate: %lu MiB/s, Adjusted rate: %lu MiB/s for %lu MiB\n", (total_mb * 1000) / (stats->active_us / 1000), (total_mb * 1000) / ((stats->active_us + stats->idle_us) / 1000), total_mb);

			stats->idle_us = 0;
			stats->active_us = 0;
		}
	}

	return (n + 1);
}
KERNEL_ATTR_RO(comcerto_splicew_rate);

static ssize_t comcerto_splicer_reqtiming_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	struct splicer_stats *stats = &splicer_stats;
	int i;
	int n = 0;

	n += sprintf(buf + n, "Histogram of splice read time (us)\n");

	for (i = 0; i < MAX_BINS - 1; i++) {
		if (stats->reqtime_counter[i]) {
			n += sprintf(buf + n, "%8u in [%5d-%5d]\n", stats->reqtime_counter[i], i << US_SHIFT, (i + 1) << US_SHIFT);
			stats->reqtime_counter[i] = 0;
		}
	}

	if (stats->reqtime_counter[i]) {
		n += sprintf(buf + n, "%8u in [%5d-....]\n", stats->reqtime_counter[i], i << US_SHIFT);
		stats->reqtime_counter[i] = 0;
	}

	return (n + 1);
}
KERNEL_ATTR_RO(comcerto_splicer_reqtiming);

static ssize_t comcerto_splicer_timing_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	struct splicer_stats *stats = &splicer_stats;
	int i;
	int n = 0;

	stats->init = 0;

	n += sprintf(buf + n, "Histogram of inter splice read time (us)\n");

	for (i = 0; i < MAX_BINS - 1; i++) {
		if (stats->time_counter[i]) {
			n += sprintf(buf + n, "%8u in [%5d-%5d]\n", stats->time_counter[i], i << US_SHIFT, (i + 1) << US_SHIFT);
			stats->time_counter[i] = 0;
		}
	}

	if (stats->time_counter[i]) {
		n += sprintf(buf + n, "%8u in [%5d-.....]\n", stats->time_counter[i], i << US_SHIFT);
		stats->time_counter[i] = 0;
	}

	return (n + 1);
}
KERNEL_ATTR_RO(comcerto_splicer_timing);

static ssize_t comcerto_splicer_data_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	struct splicer_stats *stats = &splicer_stats;
	int i;
	int n = 0;

	n += sprintf(buf + n, "Histogram of splice read data length (KiB)\n");

	for (i = 0; i < MAX_BINS; i++) {
		if (stats->data_counter[i]) {
			n += sprintf(buf + n, "%8u in [%4d-%4d]\n", stats->data_counter[i], (i << BYTE_SHIFT) / 1024, ((i + 1) << BYTE_SHIFT) / 1024);
			stats->data_counter[i] = 0;
		}
	}

	return (n + 1);
}
KERNEL_ATTR_RO(comcerto_splicer_data);

static ssize_t comcerto_splicer_rate_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	struct splicer_stats *stats = &splicer_stats;
	unsigned long total_mb = 0;
	int i;
	int n = 0;

	n += sprintf(buf + n, "Histogram of splice read rate (MiB/s)\n");

	for (i = 0; i < MAX_BINS; i++) {
		if (stats->rate_counter[i]) {
			n += sprintf(buf + n, "%8u in [%3d-%3d]\n", stats->rate_counter[i], i << RATE_SHIFT, (i + 1) << RATE_SHIFT);

			total_mb += stats->rate_counter[i] >> 10;
			stats->rate_counter[i] = 0;
		}
	}

	if (total_mb) {
		if (stats->active_us) {
			n += sprintf(buf + n, "Idle: %lu us, Active: %lu us\n", stats->idle_us, stats->active_us);
			n += sprintf(buf + n, "Global rate: %lu MiB/s, Adjusted rate: %lu MiB/s for %lu MiB\n", (total_mb * 1000) / (stats->active_us / 1000), (total_mb * 1000) / ((stats->active_us + stats->idle_us) / 1000), total_mb);

			stats->idle_us = 0;
			stats->active_us = 0;
		}
	}

	return (n + 1);
}
KERNEL_ATTR_RO(comcerto_splicer_rate);

static ssize_t comcerto_splicer_tcp_rsock_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	struct splicer_stats *stats = &splicer_stats;
	int i;
	int n = 0;

	n += sprintf(buf + n, "Histogram of TCP receive queue size when splice read is performed (KiB)\n");

	for (i = 0; i < MAX_BINS - 1; i++) {
		if (stats->tcp_rsock_counter[i]) {
			n += sprintf(buf + n, "%8u in [%4d-%4d]\n", stats->tcp_rsock_counter[i], (i << BYTE_SHIFT) / 1024, ((i + 1) << BYTE_SHIFT) / 1024);
			stats->tcp_rsock_counter[i] = 0;
		}
	}

	if (stats->tcp_rsock_counter[i]) {
		n += sprintf(buf + n, "%8u in [%4d-...]\n", stats->tcp_rsock_counter[i], (i << BYTE_SHIFT) / 1024);
		stats->tcp_rsock_counter[i] = 0;
	}

	return (n + 1);
}
KERNEL_ATTR_RO(comcerto_splicer_tcp_rsock);
#endif

#if defined(CONFIG_COMCERTO_AHCI_PROF)

#include <mach/stats.h>

static ssize_t comcerto_ahci_prof_enable_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	int n = 0;

	if (ahci_stats_enable)
		n += sprintf(buf + n, "AHCI profiling is enabled\n");
	else
		n += sprintf(buf + n, "AHCI profiling is disabled\n");

	return (n + 1);
}

static ssize_t comcerto_ahci_prof_enable_store(struct kobject *kobj,
				  struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int enable;

	if (kstrtouint(buf, 0, &enable))
		return -EINVAL;

	if (enable > 0)
		ahci_stats_enable = 1;
	else
		ahci_stats_enable = 0;

	return count;
}
KERNEL_ATTR_RW(comcerto_ahci_prof_enable);

static ssize_t comcerto_ahci_timing_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	int i, n = 0, p;

	n += sprintf(buf + n, "Histogram of ahci inter request time (us)\n");

	for (p = 0; p < MAX_AHCI_PORTS; p++) {
		struct ahci_port_stats *stats = &ahci_port_stats[p];

		n += sprintf(buf + n, "AHCI Port %d\n", p);

		stats->init = 0;

		for (i = 0; i < MAX_BINS - 1; i++) {
			if (stats->time_counter[i]) {
				n += sprintf(buf + n, "%8u in [%5d-%5d]\n", stats->time_counter[i], i << US_SHIFT, (i + 1) << US_SHIFT);
				stats->time_counter[i] = 0;
			}
		}

		if (stats->time_counter[i]) {
			n += sprintf(buf + n, "%8u in [%5d-...]\n", stats->time_counter[i], i << US_SHIFT);
			stats->time_counter[i] = 0;
		}
	}

	return (n + 1);
}
KERNEL_ATTR_RO(comcerto_ahci_timing);

static ssize_t comcerto_ahci_data_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	int i, n = 0, p;

	n += sprintf(buf + n, "Histogram of ahci requests data length (KiB)\n");

	for (p = 0; p < MAX_AHCI_PORTS; p++) {
		struct ahci_port_stats *stats = &ahci_port_stats[p];

		n += sprintf(buf + n, "AHCI Port %d\n", p);

		for (i = 0; i < MAX_BINS; i++) {
			if (stats->data_counter[i]) {
				n += sprintf(buf + n, "%8u in [%3d-%3d]\n", stats->data_counter[i], (i << BYTE_SHIFT) / 1024, ((i + 1) << BYTE_SHIFT) / 1024);
				stats->data_counter[i] = 0;
			}
		}
	}

	return (n + 1);
}
KERNEL_ATTR_RO(comcerto_ahci_data);

static ssize_t comcerto_ahci_qc_rate_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	int i, n = 0, p;
	unsigned long mean_rate, total_mb;

	for (p = 0; p < MAX_AHCI_PORTS; p++) {
		struct ahci_port_stats *stats = &ahci_port_stats[p];

		n += sprintf(buf + n, "AHCI Port %d\n", p);
		total_mb = 0;
		mean_rate = 0;

		n += sprintf(buf + n, "Histogram of bytes transfered (MiB) at rate (MiB/s):\n");

		for (i = 0; i < MAX_BINS; i++) {
			if (stats->rate_counter[i]) {
				n += sprintf(buf + n, "%8u in [%3d-%3d]\n", stats->rate_counter[i] >> 10, i << IO_RATE_SHIFT, (i + 1) << IO_RATE_SHIFT);
				mean_rate += (stats->rate_counter[i] >> 10) * (((2 * i + 1) << IO_RATE_SHIFT) / 2);
				total_mb += stats->rate_counter[i] >> 10;
				stats->rate_counter[i] = 0;
			}
		}

		n += sprintf(buf + n, "\nHistogram of requests pending (when new request is issued):\n");

		for (i = 0; i < MAX_AHCI_SLOTS; i++) {
			if (stats->pending_counter[i]) {
				n += sprintf(buf + n, "%8u in [%2d]\n", stats->pending_counter[i], i);
				stats->pending_counter[i] = 0;
			}
		}

		if (total_mb) {
			n += sprintf(buf + n, "Mean: %lu MiB/s for %lu MiB\n", mean_rate / total_mb, total_mb);
			n += sprintf(buf + n, "Max issues in a row : %u \n\n", stats->nb_pending_max);

			if (stats->active_us) {
				n += sprintf(buf + n, "Idle: %lu us, Active: %lu us\n", stats->idle_us, stats->active_us);
				n += sprintf(buf + n, "Adjusted mean: %lu MiB/s\n", ((mean_rate / total_mb) * (stats->active_us / ((stats->idle_us + stats->active_us) >> 10))) >> 10);

				stats->idle_us = 0;
				stats->active_us = 0;
			}
		}

		if (stats->write_total) {
			n += sprintf(buf + n, "Writes: %8lu, Bytes: %8lu MiB\n", stats->write_total, stats->write_kbytes_total >> 10);

			stats->write_total = 0;
			stats->write_kbytes_total = 0;
		}

		if (stats->read_total) {
			n += sprintf(buf + n, "Reads:  %8lu, Bytes: %8lu MiB\n", stats->read_total, stats->read_kbytes_total >> 10);

			stats->read_total = 0;
			stats->read_kbytes_total = 0;
		}

		if (stats->other_total) {
			n += sprintf(buf + n, "Other:  %8lu, Bytes: %8lu MiB\n", stats->other_total, stats->other_kbytes_total >> 10);

			stats->other_total = 0;
			stats->other_kbytes_total = 0;
		}

		n += sprintf(buf + n, "\n");

		stats->nb_pending_max = 0;
	}

	return (n + 1);
}
KERNEL_ATTR_RO(comcerto_ahci_qc_rate);
#endif

/*
 * Make /sys/kernel/notes give the raw contents of our kernel .notes section.
 */
extern const void __start_notes __attribute__((weak));
extern const void __stop_notes __attribute__((weak));
#define	notes_size (&__stop_notes - &__start_notes)

static ssize_t notes_read(struct file *filp, struct kobject *kobj,
			  struct bin_attribute *bin_attr,
			  char *buf, loff_t off, size_t count)
{
	memcpy(buf, &__start_notes + off, count);
	return count;
}

static struct bin_attribute notes_attr = {
	.attr = {
		.name = "notes",
		.mode = S_IRUGO,
	},
	.read = &notes_read,
};

struct kobject *kernel_kobj;
EXPORT_SYMBOL_GPL(kernel_kobj);

static struct attribute * kernel_attrs[] = {
	&fscaps_attr.attr,
#if defined(CONFIG_HOTPLUG)
	&uevent_seqnum_attr.attr,
	&uevent_helper_attr.attr,
#endif
#ifdef CONFIG_PROFILING
	&profiling_attr.attr,
#endif
#ifdef CONFIG_KEXEC
	&kexec_loaded_attr.attr,
	&kexec_crash_loaded_attr.attr,
	&kexec_crash_size_attr.attr,
	&vmcoreinfo_attr.attr,
#endif
#if defined(CONFIG_COMCERTO_MDMA_PROF)
	&comcerto_mdma_prof_enable_attr.attr,
	&comcerto_mdma_timing_attr.attr,
	&comcerto_mdma_reqtiming_attr.attr,
	&comcerto_mdma_data_attr.attr,
#endif
#if defined(CONFIG_COMCERTO_SPLICE_PROF)
	&comcerto_splice_prof_enable_attr.attr,
	&comcerto_splicew_timing_attr.attr,
	&comcerto_splicew_reqtiming_attr.attr,
	&comcerto_splicew_data_attr.attr,
	&comcerto_splicew_rate_attr.attr,
	&comcerto_splicer_timing_attr.attr,
	&comcerto_splicer_reqtiming_attr.attr,
	&comcerto_splicer_data_attr.attr,
	&comcerto_splicer_rate_attr.attr,
	&comcerto_splicer_tcp_rsock_attr.attr,
#endif
#if defined(CONFIG_COMCERTO_AHCI_PROF)
	&comcerto_ahci_prof_enable_attr.attr,
	&comcerto_ahci_timing_attr.attr,
	&comcerto_ahci_data_attr.attr,
	&comcerto_ahci_qc_rate_attr.attr,
#endif
	NULL
};

static struct attribute_group kernel_attr_group = {
	.attrs = kernel_attrs,
};

static int __init ksysfs_init(void)
{
	int error;

	kernel_kobj = kobject_create_and_add("kernel", NULL);
	if (!kernel_kobj) {
		error = -ENOMEM;
		goto exit;
	}
	error = sysfs_create_group(kernel_kobj, &kernel_attr_group);
	if (error)
		goto kset_exit;

	if (notes_size > 0) {
		notes_attr.size = notes_size;
		error = sysfs_create_bin_file(kernel_kobj, &notes_attr);
		if (error)
			goto group_exit;
	}

	return 0;

group_exit:
	sysfs_remove_group(kernel_kobj, &kernel_attr_group);
kset_exit:
	kobject_put(kernel_kobj);
exit:
	return error;
}

core_initcall(ksysfs_init);
