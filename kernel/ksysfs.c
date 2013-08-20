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
extern unsigned int mdma_time_counter[256]; // 16 -> 4000 us
extern unsigned int mdma_reqtime_counter[256]; // 16 -> 4000 us
extern unsigned int mdma_data_counter[256];
extern unsigned int init_mdma_prof;
extern unsigned int enable_mdma_prof;

static ssize_t comcerto_mdma_prof_enable_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	int n;
	buf[0] = '\0';
	n = 0;
	if (enable_mdma_prof)
		n += sprintf(buf, "MDMA profiling is enabled\n");
	else
		n += sprintf(buf, "MDMA profiling is disabled\n");

	return (n + 1);
}

static ssize_t comcerto_mdma_prof_enable_store(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf, size_t count)
{
	unsigned int enable;

	if (kstrtouint(buf, 0, &enable))
		return -EINVAL;

	if (enable > 0)
		enable_mdma_prof = 1;
	else
		enable_mdma_prof = 0;

	return count;
}
KERNEL_ATTR_RW(comcerto_mdma_prof_enable);

static ssize_t comcerto_mdma_reqtiming_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	int i;
	int n;

	buf[0] = '\0';
	n = 0;
	n += sprintf(buf, "Histogram of mdma request time\n");

	for (i = 0; i < 255; i++)
	{
		if (mdma_reqtime_counter[i]) {
			n += sprintf(buf + n, "%d in [%d-%d] us\n", mdma_reqtime_counter[i], i << 4, (i + 1) << 4);
			mdma_reqtime_counter[i] = 0;
		}
	}
	if (mdma_reqtime_counter[255]) {
		n += sprintf(buf + n, "%d >= %d us\n", mdma_reqtime_counter[255], 255 << 4);
		mdma_reqtime_counter[255] = 0;
	}
	return (n + 1);
}
KERNEL_ATTR_RO(comcerto_mdma_reqtiming);

static ssize_t comcerto_mdma_timing_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	int i;
	int n;

	init_mdma_prof = 0;
	buf[0] = '\0';
	n = 0;
	n += sprintf(buf, "Histogram of inter mdma request time\n");

	for (i = 0; i < 255; i++)
	{
		if (mdma_time_counter[i]) {
			n += sprintf(buf + n, "%d in [%d-%d] us\n", mdma_time_counter[i], i << 4, (i + 1) << 4);
			mdma_time_counter[i] = 0;
		}
	}
	if (mdma_time_counter[255]) {
		n += sprintf(buf + n, "%d >= %d us\n", mdma_time_counter[255], 255 << 4);
		mdma_time_counter[255] = 0;
	}
	return (n + 1);
}
KERNEL_ATTR_RO(comcerto_mdma_timing);

static ssize_t comcerto_mdma_data_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	int i;
	int n;

	buf[0] = '\0';
	n = 0;
	n += sprintf(buf, "Histogram of mdma data length (up to 1M)\n");
	for (i = 0; i < 256; i++)
	{
		if (mdma_data_counter[i]) {
			n += sprintf(buf + n, "%d in [%d-%d] KB\n", mdma_data_counter[i], i << (13 - 10), (i + 1) << (13 - 10));
			mdma_data_counter[i] = 0;
		}
	}
	return (n + 1);
}
KERNEL_ATTR_RO(comcerto_mdma_data);
#endif

#if defined(CONFIG_COMCERTO_SPLICE_PROF)
extern unsigned int splicew_time_counter[256]; // 4 ms -> 1S
extern unsigned int splicew_reqtime_counter[256]; // 4 ms -> 1S
extern unsigned int splicew_data_counter[256]; 
extern unsigned int init_splicew_prof; 
extern unsigned int enable_splice_prof;
static ssize_t comcerto_splice_prof_enable_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	int n;
	buf[0] = '\0';
	n = 0;
	if (enable_splice_prof)
		n += sprintf(buf, "Splice profiling is enabled\n");
	else
		n += sprintf(buf, "Splice profiling is disabled\n");

	return (n + 1);
}
static ssize_t comcerto_splice_prof_enable_store(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf, size_t count)
{
	unsigned int enable;

	if (kstrtouint(buf, 0, &enable))
		return -EINVAL;

	if (enable > 0)
		enable_splice_prof = 1;
	else
		enable_splice_prof = 0;

	return count;
}
KERNEL_ATTR_RW(comcerto_splice_prof_enable);
static ssize_t comcerto_splicew_reqtiming_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	int i;
	int n;

	buf[0] = '\0';
	n = 0;
	n += sprintf(buf, "Histogram of splice write time (up to 1 sec otherwise date is discarded)\n");

	for (i = 0; i < 255; i++)
	{
		if (splicew_reqtime_counter[i]) {
			n += sprintf(buf + n, "%d in [%d-%d] ms\n", splicew_reqtime_counter[i], (i * 8), (i * 8) + 8);
			splicew_reqtime_counter[i] = 0;
		}
	}
	if (splicew_reqtime_counter[255]) {
		n += sprintf(buf + n, "%d > 1 second\n", splicew_reqtime_counter[255]);
		splicew_reqtime_counter[255] = 0;
	}
	return (n + 1);
}
KERNEL_ATTR_RO(comcerto_splicew_reqtiming);
static ssize_t comcerto_splicew_timing_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	int i;
	int n;

	init_splicew_prof = 0;
	buf[0] = '\0';
	n = 0;
	n += sprintf(buf, "Histogram of inter splice write time (up to 1 sec otherwise date is discarded)\n");

	for (i = 0; i < 255; i++)
	{
		if (splicew_time_counter[i]) {
			n += sprintf(buf + n, "%d in [%d-%d] ms\n", splicew_time_counter[i], (i * 8), (i * 8) + 8);
			splicew_time_counter[i] = 0;
		}
	}
	if (splicew_time_counter[255]) {
 		n += sprintf(buf + n, "%d > 1 second\n", splicew_time_counter[255]);
		splicew_time_counter[255] = 0;
	}
	return (n + 1);
}
KERNEL_ATTR_RO(comcerto_splicew_timing);
static ssize_t comcerto_splicew_data_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	int i;
	int n;

	buf[0] = '\0';
	n = 0;
	n += sprintf(buf, "Histogram of splice write data length (up to 1M)\n");
	for (i = 0; i < 256; i++)
	{
		if (splicew_data_counter[i]) {
			n += sprintf(buf + n, "%d in [%d-%d] KB\n", splicew_data_counter[i], (i * 8), (i * 8) + 8);
			splicew_data_counter[i] = 0;
		}
	}
	return (n + 1);
}
KERNEL_ATTR_RO(comcerto_splicew_data);


extern unsigned int splicer_time_counter[256]; // 4 ms -> 1S
extern unsigned int splicer_reqtime_counter[256]; // 4 ms -> 1S
extern unsigned int splicer_data_counter[256]; 
extern unsigned int splicer_tcp_rsock_counter[64];
extern unsigned int init_splicer_prof; 
static ssize_t comcerto_splicer_reqtiming_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	int i;
	int n;

	buf[0] = '\0';
	n = 0;
	n += sprintf(buf, "Histogram of splice read time\n");

	for (i = 0; i < 255; i++)
	{
		if (splicer_reqtime_counter[i]) {
			n += sprintf(buf + n, "%d in [%d-%d] ms\n", splicer_reqtime_counter[i], (i * 8), (i * 8) + 8);
			splicer_reqtime_counter[i] = 0;
		}
	}
	if (splicer_reqtime_counter[255]) {
	 	n += sprintf(buf + n, "%d > 1 second\n", splicer_reqtime_counter[255]);
		splicer_reqtime_counter[255] = 0;
	}
	return (n + 1);
}
KERNEL_ATTR_RO(comcerto_splicer_reqtiming);
static ssize_t comcerto_splicer_timing_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	int i;
	int n;

	init_splicer_prof = 0;
	buf[0] = '\0';
	n = 0;
	n += sprintf(buf, "Histogram of inter splice read time\n");

	for (i = 0; i < 255; i++)
	{
		if (splicer_time_counter[i]) {
			n += sprintf(buf + n, "%d in [%d-%d] ms\n", splicer_time_counter[i], (i * 8), (i * 8) + 8);
			splicer_time_counter[i] = 0;
		}
	}
	if (splicer_time_counter[255]) {
 		n += sprintf(buf + n, "%d > 1 second\n", splicer_time_counter[255]);
		splicer_time_counter[255] = 0;
	}
	return (n + 1);
}
KERNEL_ATTR_RO(comcerto_splicer_timing);
static ssize_t comcerto_splicer_data_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	int i;
	int n;

	buf[0] = '\0';
	n = 0;
	n += sprintf(buf, "Histogram of splice read data length (up to 1M)\n");
	for (i = 0; i < 256; i++)
	{
		if (splicer_data_counter[i]) {
			n += sprintf(buf + n, "%d in [%d-%d] KB\n", splicer_data_counter[i], (i * 8), (i * 8) + 8);
			splicer_data_counter[i] = 0;
		}
	}
	return (n + 1);
}
KERNEL_ATTR_RO(comcerto_splicer_data);
static ssize_t comcerto_splicer_tcp_rsock_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	int i;
	int n;

	buf[0] = '\0';
	n = 0;
	n += sprintf(buf, "Histogram of TCP receive queue size when splice read is performed\n");
	for (i = 0; i < 63; i++)
	{
		if (splicer_tcp_rsock_counter[i]) {
			n += sprintf(buf + n, "%d in [%d-%d] KB\n", splicer_tcp_rsock_counter[i], (i * 64), (i * 64) + 64);
			splicer_tcp_rsock_counter[i] = 0;
		}
	}
	if (splicer_tcp_rsock_counter[i]) {
			n += sprintf(buf + n, "%d >= %d KB\n", splicer_tcp_rsock_counter[i], (i * 64));
			splicer_tcp_rsock_counter[i] = 0;
	}
	return (n + 1);
}
KERNEL_ATTR_RO(comcerto_splicer_tcp_rsock);
#endif

#if defined(CONFIG_COMCERTO_AHCI_PROF)
extern unsigned int ahci_time_counter[256]; // 4 ms -> 1S
extern unsigned int ahci_data_counter[256]; 
extern unsigned int init_ahci_prof;
extern unsigned int enable_ahci_prof;
static ssize_t comcerto_ahci_prof_enable_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	int n;
	buf[0] = '\0';
	n = 0;
	if (enable_ahci_prof)
		n += sprintf(buf, "AHCI profiling is enabled\n");
	else
		n += sprintf(buf, "AHCI profiling is disabled\n");

	return (n + 1);
}
static ssize_t comcerto_ahci_prof_enable_store(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf, size_t count)
{
	unsigned int enable;

	if (kstrtouint(buf, 0, &enable))
		return -EINVAL;

	if (enable > 0)
		enable_ahci_prof = 1;
	else
		enable_ahci_prof = 0;

	return count;
}
KERNEL_ATTR_RW(comcerto_ahci_prof_enable);
static ssize_t comcerto_ahci_timing_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	int i;
	int n;

	buf[0] = '\0';
	n = 0;
	n += sprintf(buf, "Histogram of inter ahci write time (up to 1 sec otherwise date is discarded)\n");
	init_ahci_prof = 0;
	for (i = 0; i < 255; i++)
	{
		if (ahci_time_counter[i]) {
			n += sprintf(buf + n, "%d in [%d-%d] ms\n", ahci_time_counter[i], (i * 8), (i * 8) + 8);
			ahci_time_counter[i] = 0;
		}
	}
	if (ahci_time_counter[255]) {
	 	n += sprintf(buf + n, "%d > 1 second\n", ahci_time_counter[255]);
		ahci_time_counter[255] = 0;
	}
	return (n + 1);
}
KERNEL_ATTR_RO(comcerto_ahci_timing);
static ssize_t comcerto_ahci_data_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	int i;
	int n;

	buf[0] = '\0';
	n = 0;
	n += sprintf(buf, "Histogram of ahci write data length (up to 1M)\n");

	for (i = 0; i < 256; i++)
	{
		if (ahci_data_counter[i]) {
			n += sprintf(buf + n, "%d in [%d-%d] KB\n", ahci_data_counter[i], (i * 8), (i * 8) + 8);
			ahci_data_counter[i] = 0;
		}
	}
	return (n + 1);
}
KERNEL_ATTR_RO(comcerto_ahci_data);


extern unsigned int ahci_qc_comp_counter[33];
static ssize_t comcerto_ahci_qc_comp_timing_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	int i;
	int n;

	buf[0] = '\0';
	n = 0;
	sprintf(buf, "Histogram of AHCI qc_complete time (in ms):\n");
	n = strlen(buf);
	for (i = 0; i < 32; i++)
	{
		if (ahci_qc_comp_counter[i]) {
			sprintf(buf + n, "%d, in [%d-%d]ms\n",ahci_qc_comp_counter[i], (i * 16), (i * 16) + 16);
			n = strlen(buf);
			ahci_qc_comp_counter[i] = 0;
		}
	}
	if (ahci_qc_comp_counter[i]) {
		sprintf(buf + n, "%d, in [> 512]ms\n",ahci_qc_comp_counter[i]);
		n = strlen(buf);
		ahci_qc_comp_counter[i] = 0;
	}
	return (n + 1);
}
KERNEL_ATTR_RO(comcerto_ahci_qc_comp_timing);


extern unsigned int ahci_qc_no_free_slot;
static ssize_t comcerto_ahci_qc_no_free_slot_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	int n;

	buf[0] = '\0';
	n = 0;
	sprintf(buf, "AHCI qc_no_free_slot count: %d\n", ahci_qc_no_free_slot);
	ahci_qc_no_free_slot = 0;

	n = strlen(buf);

	return (n + 1);
}
KERNEL_ATTR_RO(comcerto_ahci_qc_no_free_slot);
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
	&comcerto_splicer_timing_attr.attr,
	&comcerto_splicer_reqtiming_attr.attr,
	&comcerto_splicer_data_attr.attr,
	&comcerto_splicer_tcp_rsock_attr.attr,
#endif
#if defined(CONFIG_COMCERTO_AHCI_PROF)
	&comcerto_ahci_prof_enable_attr.attr,
	&comcerto_ahci_timing_attr.attr,
	&comcerto_ahci_data_attr.attr,
	&comcerto_ahci_qc_comp_timing_attr.attr,
	&comcerto_ahci_qc_no_free_slot_attr.attr,
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
