#include <linux/antirebootloop.h>
#include <asm/memory.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

static struct arl_marker *antirebootloop_marker = 0;
static u32 bootloader_version;

static int verify_magic(struct arl_marker *m) {
	int i;
	for (i=0; i < ARRAY_SIZE(m->magic); i++) {
		if (m->magic[i] != ARL_MAGIC + i) return -1;
	}
	return 0;
}

static int arl_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "counter: %u\n", antirebootloop_marker->counter);
	seq_printf(m, "bootloader_version: %u\n",
			bootloader_version);
	seq_printf(m, "kernel_version: %u\n",
			antirebootloop_marker->kernel_version);
	return 0;
}

#define MAX_ARL_WRITE 8
static ssize_t arl_write(struct file *file, const char __user *buffer,
		       size_t count, loff_t *ppos)
{
	char kbuf[MAX_ARL_WRITE + 1];
	u32 counter;

	if (count > MAX_ARL_WRITE)
		return -EINVAL;
	if (copy_from_user(&kbuf, buffer, count))
		return -EFAULT;
	kbuf[MAX_ARL_WRITE] = '\0';

	if (sscanf(kbuf, "%d", &counter) != 1)
		return -EINVAL;

	antirebootloop_marker->counter = counter;
	pr_info("Set antirebootloop counter to %u\n",
			antirebootloop_marker->counter);
	return count;
}

static int arl_proc_open(struct inode *inode, struct  file *file) {
	if (!antirebootloop_marker)
		return -ENOENT;
	return single_open(file, arl_proc_show, NULL);
}

static const struct file_operations arl_proc_fops = {
	.open		= arl_proc_open,
	.read		= seq_read,
	.write		= arl_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init init_marker(void) {
	phys_addr_t a;
	static struct arl_marker *m;
	a = get_antirebootloop_ptr();
	if (!a)
		return 0;
	m = __va(a);
	if (!verify_magic(m)) {
		pr_info("Found antirebootloop marker. "
			"counter %u bootloader_version %u "
			"kernel_version %u\n",
			m->counter, m->bootloader_version, m->kernel_version);
		/* We need to make a copy of bootloader_version and then reset
		 * it to 0. Otherwise, we might read a stale value if someone
		 * downgrades from an ARL to an old, non-ARL bootloader.
		 * */
		bootloader_version = m->bootloader_version;
		m->bootloader_version = 0;
		m->kernel_version = ARL_KERNEL_VERSION;
		antirebootloop_marker = m;
	}
	return 0;
}

static int __init create_procfs_entry(void) {
	pr_info("Creating ARL proc fs entry\n");
	if (antirebootloop_marker)
		proc_create("antirebootloop", 0, NULL, &arl_proc_fops);
	return 0;
}

early_initcall(init_marker);
core_initcall(create_procfs_entry);
