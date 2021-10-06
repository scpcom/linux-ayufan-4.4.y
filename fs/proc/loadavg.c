#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/pid_namespace.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/seqlock.h>
#include <linux/time.h>

#define LOAD_INT(x) ((x) >> FSHIFT)
#define LOAD_FRAC(x) LOAD_INT(((x) & (FIXED_1-1)) * 100)

static int loadavg_proc_show(struct seq_file *m, void *v)
{
	unsigned long avnrun[3];

	get_avenrun(avnrun, FIXED_1/200, 0);

	seq_printf(m, "%lu.%02lu %lu.%02lu %lu.%02lu %ld/%d %d\n",
		LOAD_INT(avnrun[0]), LOAD_FRAC(avnrun[0]),
		LOAD_INT(avnrun[1]), LOAD_FRAC(avnrun[1]),
		LOAD_INT(avnrun[2]), LOAD_FRAC(avnrun[2]),
		nr_running(), nr_threads,
		task_active_pid_ns(current)->last_pid);
	return 0;
}

static int loadavg_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, loadavg_proc_show, NULL);
}

static const struct file_operations loadavg_proc_fops = {
	.open		= loadavg_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

#ifdef MY_ABC_HERE
static int syno_loadavg_proc_show(struct seq_file *m, void *v)
{
	unsigned long avnrun_io[3];
	unsigned long avnrun_cpu[3];

	get_avenrun_split(avnrun_io, avnrun_cpu, FIXED_1/200, 0);

	seq_printf(m, "%lu.%02lu %lu.%02lu %lu.%02lu %lu.%02lu %lu.%02lu %lu.%02lu\n",
		LOAD_INT(avnrun_io[0]), LOAD_FRAC(avnrun_io[0]),
		LOAD_INT(avnrun_io[1]), LOAD_FRAC(avnrun_io[1]),
		LOAD_INT(avnrun_io[2]), LOAD_FRAC(avnrun_io[2]),
		LOAD_INT(avnrun_cpu[0]), LOAD_FRAC(avnrun_cpu[0]),
		LOAD_INT(avnrun_cpu[1]), LOAD_FRAC(avnrun_cpu[1]),
		LOAD_INT(avnrun_cpu[2]), LOAD_FRAC(avnrun_cpu[2]));
	return 0;
}

static int syno_loadavg_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, syno_loadavg_proc_show, NULL);
}

static const struct file_operations syno_loadavg_proc_fops = {
	.open		= syno_loadavg_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};
#endif /* MY_ABC_HERE */

static int __init proc_loadavg_init(void)
{
	proc_create("loadavg", 0, NULL, &loadavg_proc_fops);
#ifdef MY_ABC_HERE
	proc_create("syno_loadavg", 0, NULL, &syno_loadavg_proc_fops);
#endif /* MY_ABC_HERE */
	return 0;
}
fs_initcall(proc_loadavg_init);
