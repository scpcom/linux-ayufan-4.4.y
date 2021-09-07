#ifndef _LINUX_SCHED_H
#define _LINUX_SCHED_H

#define CSIGNAL		0x000000ff	 
#define CLONE_VM	0x00000100	 
#define CLONE_FS	0x00000200	 
#define CLONE_FILES	0x00000400	 
#define CLONE_SIGHAND	0x00000800	 
#define CLONE_PTRACE	0x00002000	 
#define CLONE_VFORK	0x00004000	 
#define CLONE_PARENT	0x00008000	 
#define CLONE_THREAD	0x00010000	 
#define CLONE_NEWNS	0x00020000	 
#define CLONE_SYSVSEM	0x00040000	 
#define CLONE_SETTLS	0x00080000	 
#define CLONE_PARENT_SETTID	0x00100000	 
#define CLONE_CHILD_CLEARTID	0x00200000	 
#define CLONE_DETACHED		0x00400000	 
#define CLONE_UNTRACED		0x00800000	 
#define CLONE_CHILD_SETTID	0x01000000	 
#define CLONE_STOPPED		0x02000000	 
#define CLONE_NEWUTS		0x04000000	 
#define CLONE_NEWIPC		0x08000000	 
#define CLONE_NEWUSER		0x10000000	 
#define CLONE_NEWPID		0x20000000	 
#define CLONE_NEWNET		0x40000000	 
#define CLONE_IO		0x80000000	 

#define SCHED_NORMAL		0
#define SCHED_FIFO		1
#define SCHED_RR		2
#define SCHED_BATCH		3
 
#define SCHED_IDLE		5
 
#define SCHED_RESET_ON_FORK     0x40000000

#ifdef __KERNEL__

struct sched_param {
	int sched_priority;
};

#include <asm/param.h>	 

#include <linux/capability.h>
#include <linux/threads.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/timex.h>
#include <linux/jiffies.h>
#include <linux/rbtree.h>
#include <linux/thread_info.h>
#include <linux/cpumask.h>
#include <linux/errno.h>
#include <linux/nodemask.h>
#include <linux/mm_types.h>

#include <asm/system.h>
#include <asm/page.h>
#include <asm/ptrace.h>
#include <asm/cputime.h>

#include <linux/smp.h>
#include <linux/sem.h>
#include <linux/signal.h>
#include <linux/path.h>
#include <linux/compiler.h>
#include <linux/completion.h>
#include <linux/pid.h>
#include <linux/percpu.h>
#include <linux/topology.h>
#include <linux/proportions.h>
#include <linux/seccomp.h>
#include <linux/rcupdate.h>
#include <linux/rculist.h>
#include <linux/rtmutex.h>

#include <linux/time.h>
#include <linux/param.h>
#include <linux/resource.h>
#include <linux/timer.h>
#include <linux/hrtimer.h>
#include <linux/task_io_accounting.h>
#include <linux/kobject.h>
#include <linux/latencytop.h>
#include <linux/cred.h>

#include <asm/processor.h>

struct exec_domain;
struct futex_pi_state;
struct robust_list_head;
struct bio;
struct fs_struct;
struct bts_context;
struct perf_event_context;

#define CLONE_KERNEL	(CLONE_FS | CLONE_FILES | CLONE_SIGHAND)

extern unsigned long avenrun[];		 
extern void get_avenrun(unsigned long *loads, unsigned long offset, int shift);

#define FSHIFT		11		 
#define FIXED_1		(1<<FSHIFT)	 
#define LOAD_FREQ	(5*HZ+1)	 
#define EXP_1		1884		 
#define EXP_5		2014		 
#define EXP_15		2037		 

#define CALC_LOAD(load,exp,n) \
	load *= exp; \
	load += n*(FIXED_1-exp); \
	load >>= FSHIFT;

extern unsigned long total_forks;
extern int nr_threads;
DECLARE_PER_CPU(unsigned long, process_counts);
extern int nr_processes(void);
extern unsigned long nr_running(void);
extern unsigned long nr_uninterruptible(void);
extern unsigned long nr_iowait(void);
extern unsigned long nr_iowait_cpu(void);
extern unsigned long this_cpu_load(void);

extern void calc_global_load(void);
extern u64 cpu_nr_migrations(int cpu);

extern unsigned long get_parent_ip(unsigned long addr);

struct seq_file;
struct cfs_rq;
struct task_group;
#ifdef CONFIG_SCHED_DEBUG
extern void proc_sched_show_task(struct task_struct *p, struct seq_file *m);
extern void proc_sched_set_task(struct task_struct *p);
extern void
print_cfs_rq(struct seq_file *m, int cpu, struct cfs_rq *cfs_rq);
#else
static inline void
proc_sched_show_task(struct task_struct *p, struct seq_file *m)
{
}
static inline void proc_sched_set_task(struct task_struct *p)
{
}
static inline void
print_cfs_rq(struct seq_file *m, int cpu, struct cfs_rq *cfs_rq)
{
}
#endif

extern unsigned long long time_sync_thresh;

#define TASK_RUNNING		0
#define TASK_INTERRUPTIBLE	1
#define TASK_UNINTERRUPTIBLE	2
#define __TASK_STOPPED		4
#define __TASK_TRACED		8
 
#define EXIT_ZOMBIE		16
#define EXIT_DEAD		32
 
#define TASK_DEAD		64
#define TASK_WAKEKILL		128
#define TASK_WAKING		256

#define TASK_KILLABLE		(TASK_WAKEKILL | TASK_UNINTERRUPTIBLE)
#define TASK_STOPPED		(TASK_WAKEKILL | __TASK_STOPPED)
#define TASK_TRACED		(TASK_WAKEKILL | __TASK_TRACED)

#define TASK_NORMAL		(TASK_INTERRUPTIBLE | TASK_UNINTERRUPTIBLE)
#define TASK_ALL		(TASK_NORMAL | __TASK_STOPPED | __TASK_TRACED)

#define TASK_REPORT		(TASK_RUNNING | TASK_INTERRUPTIBLE | \
				 TASK_UNINTERRUPTIBLE | __TASK_STOPPED | \
				 __TASK_TRACED)

#define task_is_traced(task)	((task->state & __TASK_TRACED) != 0)
#define task_is_stopped(task)	((task->state & __TASK_STOPPED) != 0)
#define task_is_stopped_or_traced(task)	\
			((task->state & (__TASK_STOPPED | __TASK_TRACED)) != 0)
#define task_contributes_to_load(task)	\
				((task->state & TASK_UNINTERRUPTIBLE) != 0 && \
				 (task->flags & PF_FREEZING) == 0)

#define __set_task_state(tsk, state_value)		\
	do { (tsk)->state = (state_value); } while (0)
#define set_task_state(tsk, state_value)		\
	set_mb((tsk)->state, (state_value))

#define __set_current_state(state_value)			\
	do { current->state = (state_value); } while (0)
#define set_current_state(state_value)		\
	set_mb(current->state, (state_value))

#define TASK_COMM_LEN 16

#include <linux/spinlock.h>

extern rwlock_t tasklist_lock;
extern spinlock_t mmlist_lock;

struct task_struct;

extern void sched_init(void);
extern void sched_init_smp(void);
extern asmlinkage void schedule_tail(struct task_struct *prev);
extern void init_idle(struct task_struct *idle, int cpu);
extern void init_idle_bootup_task(struct task_struct *idle);

extern int runqueue_is_locked(int cpu);
extern void task_rq_unlock_wait(struct task_struct *p);

extern cpumask_var_t nohz_cpu_mask;
#if defined(CONFIG_SMP) && defined(CONFIG_NO_HZ)
extern int select_nohz_load_balancer(int cpu);
extern int get_nohz_load_balancer(void);
#else
static inline int select_nohz_load_balancer(int cpu)
{
	return 0;
}
#endif

extern void show_state_filter(unsigned long state_filter);

static inline void show_state(void)
{
	show_state_filter(0);
}

extern void show_regs(struct pt_regs *);

extern void show_stack(struct task_struct *task, unsigned long *sp);

void io_schedule(void);
long io_schedule_timeout(long timeout);

extern void cpu_init (void);
extern void trap_init(void);
extern void update_process_times(int user);
extern void scheduler_tick(void);

extern void sched_show_task(struct task_struct *p);

#ifdef CONFIG_DETECT_SOFTLOCKUP
extern void softlockup_tick(void);
extern void touch_softlockup_watchdog(void);
extern void touch_all_softlockup_watchdogs(void);
extern int proc_dosoftlockup_thresh(struct ctl_table *table, int write,
				    void __user *buffer,
				    size_t *lenp, loff_t *ppos);
extern unsigned int  softlockup_panic;
extern int softlockup_thresh;
#else
static inline void softlockup_tick(void)
{
}
static inline void touch_softlockup_watchdog(void)
{
}
static inline void touch_all_softlockup_watchdogs(void)
{
}
#endif

#ifdef CONFIG_DETECT_HUNG_TASK
extern unsigned int  sysctl_hung_task_panic;
extern unsigned long sysctl_hung_task_check_count;
extern unsigned long sysctl_hung_task_timeout_secs;
extern unsigned long sysctl_hung_task_warnings;
extern int proc_dohung_task_timeout_secs(struct ctl_table *table, int write,
					 void __user *buffer,
					 size_t *lenp, loff_t *ppos);
#endif

#define __sched		__attribute__((__section__(".sched.text")))

extern char __sched_text_start[], __sched_text_end[];

extern int in_sched_functions(unsigned long addr);

#define	MAX_SCHEDULE_TIMEOUT	LONG_MAX
extern signed long schedule_timeout(signed long timeout);
extern signed long schedule_timeout_interruptible(signed long timeout);
extern signed long schedule_timeout_killable(signed long timeout);
extern signed long schedule_timeout_uninterruptible(signed long timeout);
asmlinkage void __schedule(void);
asmlinkage void schedule(void);
extern int mutex_spin_on_owner(struct mutex *lock, struct thread_info *owner);

struct nsproxy;
struct user_namespace;

#define MAPCOUNT_ELF_CORE_MARGIN	(5)
#define DEFAULT_MAX_MAP_COUNT	(USHORT_MAX - MAPCOUNT_ELF_CORE_MARGIN)

extern int sysctl_max_map_count;

#include <linux/aio.h>

extern unsigned long
arch_get_unmapped_area(struct file *, unsigned long, unsigned long,
		       unsigned long, unsigned long);
extern unsigned long
arch_get_unmapped_area_topdown(struct file *filp, unsigned long addr,
			  unsigned long len, unsigned long pgoff,
			  unsigned long flags);
extern void arch_unmap_area(struct mm_struct *, unsigned long);
extern void arch_unmap_area_topdown(struct mm_struct *, unsigned long);

#if USE_SPLIT_PTLOCKS
 
#define set_mm_counter(mm, member, value) atomic_long_set(&(mm)->_##member, value)
#define get_mm_counter(mm, member) ((unsigned long)atomic_long_read(&(mm)->_##member))
#define add_mm_counter(mm, member, value) atomic_long_add(value, &(mm)->_##member)
#define inc_mm_counter(mm, member) atomic_long_inc(&(mm)->_##member)
#define dec_mm_counter(mm, member) atomic_long_dec(&(mm)->_##member)

#else   
 
#define set_mm_counter(mm, member, value) (mm)->_##member = (value)
#define get_mm_counter(mm, member) ((mm)->_##member)
#define add_mm_counter(mm, member, value) (mm)->_##member += (value)
#define inc_mm_counter(mm, member) (mm)->_##member++
#define dec_mm_counter(mm, member) (mm)->_##member--

#endif  

#define get_mm_rss(mm)					\
	(get_mm_counter(mm, file_rss) + get_mm_counter(mm, anon_rss))
#define update_hiwater_rss(mm)	do {			\
	unsigned long _rss = get_mm_rss(mm);		\
	if ((mm)->hiwater_rss < _rss)			\
		(mm)->hiwater_rss = _rss;		\
} while (0)
#define update_hiwater_vm(mm)	do {			\
	if ((mm)->hiwater_vm < (mm)->total_vm)		\
		(mm)->hiwater_vm = (mm)->total_vm;	\
} while (0)

static inline unsigned long get_mm_hiwater_rss(struct mm_struct *mm)
{
	return max(mm->hiwater_rss, get_mm_rss(mm));
}

static inline void setmax_mm_hiwater_rss(unsigned long *maxrss,
					 struct mm_struct *mm)
{
	unsigned long hiwater_rss = get_mm_hiwater_rss(mm);

	if (*maxrss < hiwater_rss)
		*maxrss = hiwater_rss;
}

static inline unsigned long get_mm_hiwater_vm(struct mm_struct *mm)
{
	return max(mm->hiwater_vm, mm->total_vm);
}

extern void set_dumpable(struct mm_struct *mm, int value);
extern int get_dumpable(struct mm_struct *mm);

#define MMF_DUMPABLE      0   
#define MMF_DUMP_SECURELY 1   

#define MMF_DUMPABLE_BITS 2
#define MMF_DUMPABLE_MASK ((1 << MMF_DUMPABLE_BITS) - 1)

#define MMF_DUMP_ANON_PRIVATE	2
#define MMF_DUMP_ANON_SHARED	3
#define MMF_DUMP_MAPPED_PRIVATE	4
#define MMF_DUMP_MAPPED_SHARED	5
#define MMF_DUMP_ELF_HEADERS	6
#define MMF_DUMP_HUGETLB_PRIVATE 7
#define MMF_DUMP_HUGETLB_SHARED  8

#define MMF_DUMP_FILTER_SHIFT	MMF_DUMPABLE_BITS
#define MMF_DUMP_FILTER_BITS	7
#define MMF_DUMP_FILTER_MASK \
	(((1 << MMF_DUMP_FILTER_BITS) - 1) << MMF_DUMP_FILTER_SHIFT)
#define MMF_DUMP_FILTER_DEFAULT \
	((1 << MMF_DUMP_ANON_PRIVATE) |	(1 << MMF_DUMP_ANON_SHARED) |\
	 (1 << MMF_DUMP_HUGETLB_PRIVATE) | MMF_DUMP_MASK_DEFAULT_ELF)

#ifdef CONFIG_CORE_DUMP_DEFAULT_ELF_HEADERS
# define MMF_DUMP_MASK_DEFAULT_ELF	(1 << MMF_DUMP_ELF_HEADERS)
#else
# define MMF_DUMP_MASK_DEFAULT_ELF	0
#endif
					 
#define MMF_VM_MERGEABLE	16	 

#define MMF_INIT_MASK		(MMF_DUMPABLE_MASK | MMF_DUMP_FILTER_MASK)

struct sighand_struct {
	atomic_t		count;
	struct k_sigaction	action[_NSIG];
	spinlock_t		siglock;
	wait_queue_head_t	signalfd_wqh;
};

struct pacct_struct {
	int			ac_flag;
	long			ac_exitcode;
	unsigned long		ac_mem;
	cputime_t		ac_utime, ac_stime;
	unsigned long		ac_minflt, ac_majflt;
};

struct cpu_itimer {
	cputime_t expires;
	cputime_t incr;
	u32 error;
	u32 incr_error;
};

struct task_cputime {
	cputime_t utime;
	cputime_t stime;
	unsigned long long sum_exec_runtime;
};
 
#define prof_exp	stime
#define virt_exp	utime
#define sched_exp	sum_exec_runtime

#define INIT_CPUTIME	\
	(struct task_cputime) {					\
		.utime = cputime_zero,				\
		.stime = cputime_zero,				\
		.sum_exec_runtime = 0,				\
	}

#define INIT_PREEMPT_COUNT	(1 + PREEMPT_ACTIVE)

struct thread_group_cputimer {
	struct task_cputime cputime;
	int running;
	spinlock_t lock;
};

struct signal_struct {
	atomic_t		count;
	atomic_t		live;

	wait_queue_head_t	wait_chldexit;	 

	struct task_struct	*curr_target;

	struct sigpending	shared_pending;

	int			group_exit_code;
	 
	int			notify_count;
	struct task_struct	*group_exit_task;

	int			group_stop_count;
	unsigned int		flags;  

	struct list_head posix_timers;

	struct hrtimer real_timer;
	struct pid *leader_pid;
	ktime_t it_real_incr;

	struct cpu_itimer it[2];

	struct thread_group_cputimer cputimer;

	struct task_cputime cputime_expires;

	struct list_head cpu_timers[3];

	struct pid *tty_old_pgrp;

	int leader;

	struct tty_struct *tty;  

	cputime_t utime, stime, cutime, cstime;
	cputime_t gtime;
	cputime_t cgtime;
	unsigned long nvcsw, nivcsw, cnvcsw, cnivcsw;
	unsigned long min_flt, maj_flt, cmin_flt, cmaj_flt;
	unsigned long inblock, oublock, cinblock, coublock;
	unsigned long maxrss, cmaxrss;
	struct task_io_accounting ioac;

	unsigned long long sum_sched_runtime;

	struct rlimit rlim[RLIM_NLIMITS];

#ifdef CONFIG_BSD_PROCESS_ACCT
	struct pacct_struct pacct;	 
#endif
#ifdef CONFIG_TASKSTATS
	struct taskstats *stats;
#endif
#ifdef CONFIG_AUDIT
	unsigned audit_tty;
	struct tty_audit_buf *tty_audit_buf;
#endif

	int oom_adj;	 
};

#ifdef __ARCH_WANT_INTERRUPTS_ON_CTXSW
# define __ARCH_WANT_UNLOCKED_CTXSW
#endif

#define SIGNAL_STOP_STOPPED	0x00000001  
#define SIGNAL_STOP_DEQUEUED	0x00000002  
#define SIGNAL_STOP_CONTINUED	0x00000004  
#define SIGNAL_GROUP_EXIT	0x00000008  
 
#define SIGNAL_CLD_STOPPED	0x00000010
#define SIGNAL_CLD_CONTINUED	0x00000020
#define SIGNAL_CLD_MASK		(SIGNAL_CLD_STOPPED|SIGNAL_CLD_CONTINUED)

#define SIGNAL_UNKILLABLE	0x00000040  

static inline int signal_group_exit(const struct signal_struct *sig)
{
	return	(sig->flags & SIGNAL_GROUP_EXIT) ||
		(sig->group_exit_task != NULL);
}

struct user_struct {
	atomic_t __count;	 
	atomic_t processes;	 
	atomic_t files;		 
	atomic_t sigpending;	 
#ifdef CONFIG_INOTIFY_USER
	atomic_t inotify_watches;  
	atomic_t inotify_devs;	 
#endif
#ifdef CONFIG_EPOLL
	atomic_t epoll_watches;	 
#endif
#ifdef CONFIG_POSIX_MQUEUE
	 
	unsigned long mq_bytes;	 
#endif
	unsigned long locked_shm;  

#ifdef CONFIG_KEYS
	struct key *uid_keyring;	 
	struct key *session_keyring;	 
#endif

	struct hlist_node uidhash_node;
	uid_t uid;
	struct user_namespace *user_ns;

#ifdef CONFIG_USER_SCHED
	struct task_group *tg;
#ifdef CONFIG_SYSFS
	struct kobject kobj;
	struct delayed_work work;
#endif
#endif

#ifdef CONFIG_PERF_EVENTS
	atomic_long_t locked_vm;
#endif
};

extern int uids_sysfs_init(void);

extern struct user_struct *find_user(uid_t);

extern struct user_struct root_user;
#define INIT_USER (&root_user)

struct backing_dev_info;
struct reclaim_state;

#if defined(CONFIG_SCHEDSTATS) || defined(CONFIG_TASK_DELAY_ACCT)
struct sched_info {
	 
	unsigned long pcount;	       
	unsigned long long run_delay;  

	unsigned long long last_arrival, 
			   last_queued;	 
#ifdef CONFIG_SCHEDSTATS
	 
	unsigned int bkl_count;
#endif
};
#endif  

#ifdef CONFIG_TASK_DELAY_ACCT
struct task_delay_info {
	spinlock_t	lock;
	unsigned int	flags;	 

	struct timespec blkio_start, blkio_end;	 
	u64 blkio_delay;	 
	u64 swapin_delay;	 
	u32 blkio_count;	 
				 
	u32 swapin_count;	 
				 
	struct timespec freepages_start, freepages_end;
	u64 freepages_delay;	 
	u32 freepages_count;	 
};
#endif	 

static inline int sched_info_on(void)
{
#ifdef CONFIG_SCHEDSTATS
	return 1;
#elif defined(CONFIG_TASK_DELAY_ACCT)
	extern int delayacct_on;
	return delayacct_on;
#else
	return 0;
#endif
}

enum cpu_idle_type {
	CPU_IDLE,
	CPU_NOT_IDLE,
	CPU_NEWLY_IDLE,
	CPU_MAX_IDLE_TYPES
};

#define SCHED_LOAD_SHIFT	10
#define SCHED_LOAD_SCALE	(1L << SCHED_LOAD_SHIFT)

#define SCHED_LOAD_SCALE_FUZZ	SCHED_LOAD_SCALE

#ifdef CONFIG_SMP
#define SD_LOAD_BALANCE		0x0001	 
#define SD_BALANCE_NEWIDLE	0x0002	 
#define SD_BALANCE_EXEC		0x0004	 
#define SD_BALANCE_FORK		0x0008	 
#define SD_BALANCE_WAKE		0x0010   
#define SD_WAKE_AFFINE		0x0020	 
#define SD_PREFER_LOCAL		0x0040   
#define SD_SHARE_CPUPOWER	0x0080	 
#define SD_POWERSAVINGS_BALANCE	0x0100	 
#define SD_SHARE_PKG_RESOURCES	0x0200	 
#define SD_SERIALIZE		0x0400	 

#define SD_PREFER_SIBLING	0x1000	 

enum powersavings_balance_level {
	POWERSAVINGS_BALANCE_NONE = 0,   
	POWERSAVINGS_BALANCE_BASIC,	 
	POWERSAVINGS_BALANCE_WAKEUP,	 
	MAX_POWERSAVINGS_BALANCE_LEVELS
};

extern int sched_mc_power_savings, sched_smt_power_savings;

static inline int sd_balance_for_mc_power(void)
{
	if (sched_smt_power_savings)
		return SD_POWERSAVINGS_BALANCE;

	if (!sched_mc_power_savings)
		return SD_PREFER_SIBLING;

	return 0;
}

static inline int sd_balance_for_package_power(void)
{
	if (sched_mc_power_savings | sched_smt_power_savings)
		return SD_POWERSAVINGS_BALANCE;

	return SD_PREFER_SIBLING;
}

static inline int sd_power_saving_flags(void)
{
	if (sched_mc_power_savings | sched_smt_power_savings)
		return SD_BALANCE_NEWIDLE;

	return 0;
}

struct sched_group {
	struct sched_group *next;	 

	unsigned int cpu_power;

	unsigned long cpumask[0];
};

static inline struct cpumask *sched_group_cpus(struct sched_group *sg)
{
	return to_cpumask(sg->cpumask);
}

enum sched_domain_level {
	SD_LV_NONE = 0,
	SD_LV_SIBLING,
	SD_LV_MC,
	SD_LV_CPU,
	SD_LV_NODE,
	SD_LV_ALLNODES,
	SD_LV_MAX
};

struct sched_domain_attr {
	int relax_domain_level;
};

#define SD_ATTR_INIT	(struct sched_domain_attr) {	\
	.relax_domain_level = -1,			\
}

struct sched_domain {
	 
	struct sched_domain *parent;	 
	struct sched_domain *child;	 
	struct sched_group *groups;	 
	unsigned long min_interval;	 
	unsigned long max_interval;	 
	unsigned int busy_factor;	 
	unsigned int imbalance_pct;	 
	unsigned int cache_nice_tries;	 
	unsigned int busy_idx;
	unsigned int idle_idx;
	unsigned int newidle_idx;
	unsigned int wake_idx;
	unsigned int forkexec_idx;
	unsigned int smt_gain;
	int flags;			 
	enum sched_domain_level level;

	unsigned long last_balance;	 
	unsigned int balance_interval;	 
	unsigned int nr_balance_failed;  

	u64 last_update;

#ifdef CONFIG_SCHEDSTATS
	 
	unsigned int lb_count[CPU_MAX_IDLE_TYPES];
	unsigned int lb_failed[CPU_MAX_IDLE_TYPES];
	unsigned int lb_balanced[CPU_MAX_IDLE_TYPES];
	unsigned int lb_imbalance[CPU_MAX_IDLE_TYPES];
	unsigned int lb_gained[CPU_MAX_IDLE_TYPES];
	unsigned int lb_hot_gained[CPU_MAX_IDLE_TYPES];
	unsigned int lb_nobusyg[CPU_MAX_IDLE_TYPES];
	unsigned int lb_nobusyq[CPU_MAX_IDLE_TYPES];

	unsigned int alb_count;
	unsigned int alb_failed;
	unsigned int alb_pushed;

	unsigned int sbe_count;
	unsigned int sbe_balanced;
	unsigned int sbe_pushed;

	unsigned int sbf_count;
	unsigned int sbf_balanced;
	unsigned int sbf_pushed;

	unsigned int ttwu_wake_remote;
	unsigned int ttwu_move_affine;
	unsigned int ttwu_move_balance;
#endif
#ifdef CONFIG_SCHED_DEBUG
	char *name;
#endif

	unsigned long span[0];
};

static inline struct cpumask *sched_domain_span(struct sched_domain *sd)
{
	return to_cpumask(sd->span);
}

extern void partition_sched_domains(int ndoms_new, struct cpumask *doms_new,
				    struct sched_domain_attr *dattr_new);

static inline int test_sd_parent(struct sched_domain *sd, int flag)
{
	if (sd->parent && (sd->parent->flags & flag))
		return 1;

	return 0;
}

unsigned long default_scale_freq_power(struct sched_domain *sd, int cpu);
unsigned long default_scale_smt_power(struct sched_domain *sd, int cpu);

#else  

struct sched_domain_attr;

static inline void
partition_sched_domains(int ndoms_new, struct cpumask *doms_new,
			struct sched_domain_attr *dattr_new)
{
}
#endif	 

struct io_context;			 

#ifdef ARCH_HAS_PREFETCH_SWITCH_STACK
extern void prefetch_stack(struct task_struct *t);
#else
static inline void prefetch_stack(struct task_struct *t) { }
#endif

struct audit_context;		 
struct mempolicy;
struct pipe_inode_info;
struct uts_namespace;

struct rq;
struct sched_domain;

#define WF_SYNC		0x01		 
#define WF_FORK		0x02		 

struct sched_class {
	const struct sched_class *next;

	void (*enqueue_task) (struct rq *rq, struct task_struct *p, int wakeup);
	void (*dequeue_task) (struct rq *rq, struct task_struct *p, int sleep);
	void (*yield_task) (struct rq *rq);

	void (*check_preempt_curr) (struct rq *rq, struct task_struct *p, int flags);

	struct task_struct * (*pick_next_task) (struct rq *rq);
	void (*put_prev_task) (struct rq *rq, struct task_struct *p);

#ifdef CONFIG_SMP
	int  (*select_task_rq)(struct task_struct *p, int sd_flag, int flags);

	unsigned long (*load_balance) (struct rq *this_rq, int this_cpu,
			struct rq *busiest, unsigned long max_load_move,
			struct sched_domain *sd, enum cpu_idle_type idle,
			int *all_pinned, int *this_best_prio);

	int (*move_one_task) (struct rq *this_rq, int this_cpu,
			      struct rq *busiest, struct sched_domain *sd,
			      enum cpu_idle_type idle);
	void (*pre_schedule) (struct rq *this_rq, struct task_struct *task);
	void (*post_schedule) (struct rq *this_rq);
	void (*task_wake_up) (struct rq *this_rq, struct task_struct *task);

	void (*set_cpus_allowed)(struct task_struct *p,
				 const struct cpumask *newmask);

	void (*rq_online)(struct rq *rq);
	void (*rq_offline)(struct rq *rq);
#endif

	void (*set_curr_task) (struct rq *rq);
	void (*task_tick) (struct rq *rq, struct task_struct *p, int queued);
	void (*task_new) (struct rq *rq, struct task_struct *p);

	void (*switched_from) (struct rq *this_rq, struct task_struct *task,
			       int running);
	void (*switched_to) (struct rq *this_rq, struct task_struct *task,
			     int running);
	void (*prio_changed) (struct rq *this_rq, struct task_struct *task,
			     int oldprio, int running);

	unsigned int (*get_rr_interval) (struct task_struct *task);

#ifdef CONFIG_FAIR_GROUP_SCHED
	void (*moved_group) (struct task_struct *p);
#endif
};

struct load_weight {
	unsigned long weight, inv_weight;
};

struct sched_entity {
	struct load_weight	load;		 
	struct rb_node		run_node;
	struct list_head	group_node;
	unsigned int		on_rq;

	u64			exec_start;
	u64			sum_exec_runtime;
	u64			vruntime;
	u64			prev_sum_exec_runtime;

	u64			last_wakeup;
	u64			avg_overlap;

	u64			nr_migrations;

	u64			start_runtime;
	u64			avg_wakeup;

	u64			avg_running;

#ifdef CONFIG_SCHEDSTATS
	u64			wait_start;
	u64			wait_max;
	u64			wait_count;
	u64			wait_sum;
	u64			iowait_count;
	u64			iowait_sum;

	u64			sleep_start;
	u64			sleep_max;
	s64			sum_sleep_runtime;

	u64			block_start;
	u64			block_max;
	u64			exec_max;
	u64			slice_max;

	u64			nr_migrations_cold;
	u64			nr_failed_migrations_affine;
	u64			nr_failed_migrations_running;
	u64			nr_failed_migrations_hot;
	u64			nr_forced_migrations;
	u64			nr_forced2_migrations;

	u64			nr_wakeups;
	u64			nr_wakeups_sync;
	u64			nr_wakeups_migrate;
	u64			nr_wakeups_local;
	u64			nr_wakeups_remote;
	u64			nr_wakeups_affine;
	u64			nr_wakeups_affine_attempts;
	u64			nr_wakeups_passive;
	u64			nr_wakeups_idle;
#endif

#ifdef CONFIG_FAIR_GROUP_SCHED
	struct sched_entity	*parent;
	 
	struct cfs_rq		*cfs_rq;
	 
	struct cfs_rq		*my_q;
#endif
};

struct sched_rt_entity {
	struct list_head run_list;
	unsigned long timeout;
	unsigned int time_slice;
	int nr_cpus_allowed;

	struct sched_rt_entity *back;
#ifdef CONFIG_RT_GROUP_SCHED
	struct sched_rt_entity	*parent;
	 
	struct rt_rq		*rt_rq;
	 
	struct rt_rq		*my_q;
#endif
};

struct rcu_node;

struct task_struct {
	volatile long state;	 
	void *stack;
	atomic_t usage;
	unsigned int flags;	 
	unsigned int ptrace;

	int lock_depth;		 

#ifdef CONFIG_SMP
#ifdef __ARCH_WANT_UNLOCKED_CTXSW
	int oncpu;
#endif
#endif

	int prio, static_prio, normal_prio;
	unsigned int rt_priority;
	const struct sched_class *sched_class;
	struct sched_entity se;
	struct sched_rt_entity rt;

#ifdef CONFIG_PREEMPT_NOTIFIERS
	 
	struct hlist_head preempt_notifiers;
#endif

	unsigned char fpu_counter;
#ifdef CONFIG_BLK_DEV_IO_TRACE
	unsigned int btrace_seq;
#endif

	unsigned int policy;
	cpumask_t cpus_allowed;

#ifdef CONFIG_TREE_PREEMPT_RCU
	int rcu_read_lock_nesting;
	char rcu_read_unlock_special;
	struct rcu_node *rcu_blocked_node;
	struct list_head rcu_node_entry;
#endif  

#if defined(CONFIG_SCHEDSTATS) || defined(CONFIG_TASK_DELAY_ACCT)
	struct sched_info sched_info;
#endif

	struct list_head tasks;
	struct plist_node pushable_tasks;

	struct mm_struct *mm, *active_mm;

	int exit_state;
	int exit_code, exit_signal;
	int pdeath_signal;   
	 
	unsigned int personality;
	unsigned did_exec:1;
	unsigned in_execve:1;	 
	unsigned in_iowait:1;

	unsigned sched_reset_on_fork:1;

	pid_t pid;
	pid_t tgid;

#ifdef CONFIG_CC_STACKPROTECTOR
	 
	unsigned long stack_canary;
#endif

	struct task_struct *real_parent;  
	struct task_struct *parent;  
	 
	struct list_head children;	 
	struct list_head sibling;	 
	struct task_struct *group_leader;	 

	struct list_head ptraced;
	struct list_head ptrace_entry;

	struct bts_context *bts;

	struct pid_link pids[PIDTYPE_MAX];
	struct list_head thread_group;

	struct completion *vfork_done;		 
	int __user *set_child_tid;		 
	int __user *clear_child_tid;		 

	cputime_t utime, stime, utimescaled, stimescaled;
	cputime_t gtime;
	cputime_t prev_utime, prev_stime;
	unsigned long nvcsw, nivcsw;  
	struct timespec start_time; 		 
	struct timespec real_start_time;	 
 
	unsigned long min_flt, maj_flt;

	struct task_cputime cputime_expires;
	struct list_head cpu_timers[3];

	const struct cred *real_cred;	 
	const struct cred *cred;	 
	struct mutex cred_guard_mutex;	 
	struct cred *replacement_session_keyring;  

	char comm[TASK_COMM_LEN];  
 
	int link_count, total_link_count;
#ifdef CONFIG_SYSVIPC
 
	struct sysv_sem sysvsem;
#endif
#ifdef CONFIG_DETECT_HUNG_TASK
 
	unsigned long last_switch_count;
#endif
 
	struct thread_struct thread;
 
	struct fs_struct *fs;
 
	struct files_struct *files;
 
	struct nsproxy *nsproxy;
 
	struct signal_struct *signal;
	struct sighand_struct *sighand;

	sigset_t blocked, real_blocked;
	sigset_t saved_sigmask;	 
	struct sigpending pending;

	unsigned long sas_ss_sp;
	size_t sas_ss_size;
	int (*notifier)(void *priv);
	void *notifier_data;
	sigset_t *notifier_mask;
	struct audit_context *audit_context;
#ifdef CONFIG_AUDITSYSCALL
	uid_t loginuid;
	unsigned int sessionid;
#endif
	seccomp_t seccomp;

   	u32 parent_exec_id;
   	u32 self_exec_id;
 
	spinlock_t alloc_lock;

#ifdef CONFIG_GENERIC_HARDIRQS
	 
	struct irqaction *irqaction;
#endif

	spinlock_t pi_lock;

#ifdef CONFIG_RT_MUTEXES
	 
	struct plist_head pi_waiters;
	 
	struct rt_mutex_waiter *pi_blocked_on;
#endif

#ifdef CONFIG_DEBUG_MUTEXES
	 
	struct mutex_waiter *blocked_on;
#endif
#ifdef CONFIG_TRACE_IRQFLAGS
	unsigned int irq_events;
	int hardirqs_enabled;
	unsigned long hardirq_enable_ip;
	unsigned int hardirq_enable_event;
	unsigned long hardirq_disable_ip;
	unsigned int hardirq_disable_event;
	int softirqs_enabled;
	unsigned long softirq_disable_ip;
	unsigned int softirq_disable_event;
	unsigned long softirq_enable_ip;
	unsigned int softirq_enable_event;
	int hardirq_context;
	int softirq_context;
#endif
#ifdef CONFIG_LOCKDEP
# define MAX_LOCK_DEPTH 48UL
	u64 curr_chain_key;
	int lockdep_depth;
	unsigned int lockdep_recursion;
	struct held_lock held_locks[MAX_LOCK_DEPTH];
	gfp_t lockdep_reclaim_gfp;
#endif

	void *journal_info;

	struct bio *bio_list, **bio_tail;

	struct reclaim_state *reclaim_state;

	struct backing_dev_info *backing_dev_info;

	struct io_context *io_context;

	unsigned long ptrace_message;
	siginfo_t *last_siginfo;  
	struct task_io_accounting ioac;
#if defined(CONFIG_TASK_XACCT)
	u64 acct_rss_mem1;	 
	u64 acct_vm_mem1;	 
	cputime_t acct_timexpd;	 
#endif
#ifdef CONFIG_CPUSETS
	nodemask_t mems_allowed;	 
	int cpuset_mem_spread_rotor;
#endif
#ifdef CONFIG_CGROUPS
	 
	struct css_set *cgroups;
	 
	struct list_head cg_list;
#endif
#ifdef CONFIG_FUTEX
	struct robust_list_head __user *robust_list;
#ifdef CONFIG_COMPAT
	struct compat_robust_list_head __user *compat_robust_list;
#endif
	struct list_head pi_state_list;
	struct futex_pi_state *pi_state_cache;
#endif
#ifdef CONFIG_PERF_EVENTS
	struct perf_event_context *perf_event_ctxp;
	struct mutex perf_event_mutex;
	struct list_head perf_event_list;
#endif
#ifdef CONFIG_NUMA
	struct mempolicy *mempolicy;	 
	short il_next;
#endif
	atomic_t fs_excl;	 
	struct rcu_head rcu;

	struct pipe_inode_info *splice_pipe;
#ifdef	CONFIG_TASK_DELAY_ACCT
	struct task_delay_info *delays;
#endif
#ifdef CONFIG_FAULT_INJECTION
	int make_it_fail;
#endif
	struct prop_local_single dirties;
#ifdef CONFIG_LATENCYTOP
	int latency_record_count;
	struct latency_record latency_record[LT_SAVECOUNT];
#endif
	 
	unsigned long timer_slack_ns;
	unsigned long default_timer_slack_ns;

	struct list_head	*scm_work_list;
#ifdef CONFIG_FUNCTION_GRAPH_TRACER
	 
	int curr_ret_stack;
	 
	struct ftrace_ret_stack	*ret_stack;
	 
	unsigned long long ftrace_timestamp;
	 
	atomic_t trace_overrun;
	 
	atomic_t tracing_graph_pause;
#endif
#ifdef CONFIG_TRACING
	 
	unsigned long trace;
	 
	unsigned long trace_recursion;
#endif  
	unsigned long stack_start;
};

#define tsk_cpumask(tsk) (&(tsk)->cpus_allowed)

#define MAX_USER_RT_PRIO	100
#define MAX_RT_PRIO		MAX_USER_RT_PRIO

#define MAX_PRIO		(MAX_RT_PRIO + 40)
#define DEFAULT_PRIO		(MAX_RT_PRIO + 20)

static inline int rt_prio(int prio)
{
	if (unlikely(prio < MAX_RT_PRIO))
		return 1;
	return 0;
}

static inline int rt_task(struct task_struct *p)
{
	return rt_prio(p->prio);
}

static inline struct pid *task_pid(struct task_struct *task)
{
	return task->pids[PIDTYPE_PID].pid;
}

static inline struct pid *task_tgid(struct task_struct *task)
{
	return task->group_leader->pids[PIDTYPE_PID].pid;
}

static inline struct pid *task_pgrp(struct task_struct *task)
{
	return task->group_leader->pids[PIDTYPE_PGID].pid;
}

static inline struct pid *task_session(struct task_struct *task)
{
	return task->group_leader->pids[PIDTYPE_SID].pid;
}

struct pid_namespace;

pid_t __task_pid_nr_ns(struct task_struct *task, enum pid_type type,
			struct pid_namespace *ns);

static inline pid_t task_pid_nr(struct task_struct *tsk)
{
	return tsk->pid;
}

static inline pid_t task_pid_nr_ns(struct task_struct *tsk,
					struct pid_namespace *ns)
{
	return __task_pid_nr_ns(tsk, PIDTYPE_PID, ns);
}

static inline pid_t task_pid_vnr(struct task_struct *tsk)
{
	return __task_pid_nr_ns(tsk, PIDTYPE_PID, NULL);
}

static inline pid_t task_tgid_nr(struct task_struct *tsk)
{
	return tsk->tgid;
}

pid_t task_tgid_nr_ns(struct task_struct *tsk, struct pid_namespace *ns);

static inline pid_t task_tgid_vnr(struct task_struct *tsk)
{
	return pid_vnr(task_tgid(tsk));
}

static inline pid_t task_pgrp_nr_ns(struct task_struct *tsk,
					struct pid_namespace *ns)
{
	return __task_pid_nr_ns(tsk, PIDTYPE_PGID, ns);
}

static inline pid_t task_pgrp_vnr(struct task_struct *tsk)
{
	return __task_pid_nr_ns(tsk, PIDTYPE_PGID, NULL);
}

static inline pid_t task_session_nr_ns(struct task_struct *tsk,
					struct pid_namespace *ns)
{
	return __task_pid_nr_ns(tsk, PIDTYPE_SID, ns);
}

static inline pid_t task_session_vnr(struct task_struct *tsk)
{
	return __task_pid_nr_ns(tsk, PIDTYPE_SID, NULL);
}

static inline pid_t task_pgrp_nr(struct task_struct *tsk)
{
	return task_pgrp_nr_ns(tsk, &init_pid_ns);
}

static inline int pid_alive(struct task_struct *p)
{
	return p->pids[PIDTYPE_PID].pid != NULL;
}

static inline int is_global_init(struct task_struct *tsk)
{
	return tsk->pid == 1;
}

extern int is_container_init(struct task_struct *tsk);

extern struct pid *cad_pid;

extern void free_task(struct task_struct *tsk);
#define get_task_struct(tsk) do { atomic_inc(&(tsk)->usage); } while(0)

extern void __put_task_struct(struct task_struct *t);

static inline void put_task_struct(struct task_struct *t)
{
	if (atomic_dec_and_test(&t->usage))
		__put_task_struct(t);
}

extern cputime_t task_utime(struct task_struct *p);
extern cputime_t task_stime(struct task_struct *p);
extern cputime_t task_gtime(struct task_struct *p);

#define PF_ALIGNWARN	0x00000001	 
					 
#define PF_STARTING	0x00000002	 
#define PF_EXITING	0x00000004	 
#define PF_EXITPIDONE	0x00000008	 
#define PF_VCPU		0x00000010	 
#define PF_FORKNOEXEC	0x00000040	 
#define PF_MCE_PROCESS  0x00000080       
#define PF_SUPERPRIV	0x00000100	 
#define PF_DUMPCORE	0x00000200	 
#define PF_SIGNALED	0x00000400	 
#define PF_MEMALLOC	0x00000800	 
#define PF_FLUSHER	0x00001000	 
#define PF_USED_MATH	0x00002000	 
#define PF_FREEZING	0x00004000	 
#define PF_NOFREEZE	0x00008000	 
#define PF_FROZEN	0x00010000	 
#define PF_FSTRANS	0x00020000	 
#define PF_KSWAPD	0x00040000	 
#define PF_OOM_ORIGIN	0x00080000	 
#define PF_LESS_THROTTLE 0x00100000	 
#define PF_KTHREAD	0x00200000	 
#define PF_RANDOMIZE	0x00400000	 
#define PF_SWAPWRITE	0x00800000	 
#define PF_SPREAD_PAGE	0x01000000	 
#define PF_SPREAD_SLAB	0x02000000	 
#define PF_THREAD_BOUND	0x04000000	 
#define PF_MCE_EARLY    0x08000000       
#define PF_MEMPOLICY	0x10000000	 
#define PF_MUTEX_TESTER	0x20000000	 
#define PF_FREEZER_SKIP	0x40000000	 
#define PF_FREEZER_NOSIG 0x80000000	 

#ifdef SYNO_FORCE_UNMOUNT
#define PF_FORCE_UMOUNT_TASK   0x00000020
#endif
 
#define clear_stopped_child_used_math(child) do { (child)->flags &= ~PF_USED_MATH; } while (0)
#define set_stopped_child_used_math(child) do { (child)->flags |= PF_USED_MATH; } while (0)
#define clear_used_math() clear_stopped_child_used_math(current)
#define set_used_math() set_stopped_child_used_math(current)
#define conditional_stopped_child_used_math(condition, child) \
	do { (child)->flags &= ~PF_USED_MATH, (child)->flags |= (condition) ? PF_USED_MATH : 0; } while (0)
#define conditional_used_math(condition) \
	conditional_stopped_child_used_math(condition, current)
#define copy_to_stopped_child_used_math(child) \
	do { (child)->flags &= ~PF_USED_MATH, (child)->flags |= current->flags & PF_USED_MATH; } while (0)
 
#define tsk_used_math(p) ((p)->flags & PF_USED_MATH)
#define used_math() tsk_used_math(current)

#ifdef CONFIG_TREE_PREEMPT_RCU

#define RCU_READ_UNLOCK_BLOCKED (1 << 0)  
#define RCU_READ_UNLOCK_NEED_QS (1 << 1)  

static inline void rcu_copy_process(struct task_struct *p)
{
	p->rcu_read_lock_nesting = 0;
	p->rcu_read_unlock_special = 0;
	p->rcu_blocked_node = NULL;
	INIT_LIST_HEAD(&p->rcu_node_entry);
}

#else

static inline void rcu_copy_process(struct task_struct *p)
{
}

#endif

#ifdef CONFIG_SMP
extern int set_cpus_allowed_ptr(struct task_struct *p,
				const struct cpumask *new_mask);
#else
static inline int set_cpus_allowed_ptr(struct task_struct *p,
				       const struct cpumask *new_mask)
{
	if (!cpumask_test_cpu(0, new_mask))
		return -EINVAL;
	return 0;
}
#endif

#ifndef CONFIG_CPUMASK_OFFSTACK
static inline int set_cpus_allowed(struct task_struct *p, cpumask_t new_mask)
{
	return set_cpus_allowed_ptr(p, &new_mask);
}
#endif

#ifdef CONFIG_HAVE_UNSTABLE_SCHED_CLOCK
extern int sched_clock_stable;
#endif

extern unsigned long long sched_clock(void);

extern void sched_clock_init(void);
extern u64 sched_clock_cpu(int cpu);

#ifndef CONFIG_HAVE_UNSTABLE_SCHED_CLOCK
static inline void sched_clock_tick(void)
{
}

static inline void sched_clock_idle_sleep_event(void)
{
}

static inline void sched_clock_idle_wakeup_event(u64 delta_ns)
{
}
#else
extern void sched_clock_tick(void);
extern void sched_clock_idle_sleep_event(void);
extern void sched_clock_idle_wakeup_event(u64 delta_ns);
#endif

extern unsigned long long cpu_clock(int cpu);

extern unsigned long long
task_sched_runtime(struct task_struct *task);
extern unsigned long long thread_group_sched_runtime(struct task_struct *task);

#ifdef CONFIG_SMP
extern void sched_exec(void);
#else
#define sched_exec()   {}
#endif

extern void sched_clock_idle_sleep_event(void);
extern void sched_clock_idle_wakeup_event(u64 delta_ns);

#ifdef CONFIG_HOTPLUG_CPU
extern void idle_task_exit(void);
#else
static inline void idle_task_exit(void) {}
#endif

extern void sched_idle_next(void);

#if defined(CONFIG_NO_HZ) && defined(CONFIG_SMP)
extern void wake_up_idle_cpu(int cpu);
#else
static inline void wake_up_idle_cpu(int cpu) { }
#endif

extern unsigned int sysctl_sched_latency;
extern unsigned int sysctl_sched_min_granularity;
extern unsigned int sysctl_sched_wakeup_granularity;
extern unsigned int sysctl_sched_shares_ratelimit;
extern unsigned int sysctl_sched_shares_thresh;
extern unsigned int sysctl_sched_child_runs_first;
#ifdef CONFIG_SCHED_DEBUG
extern unsigned int sysctl_sched_features;
extern unsigned int sysctl_sched_migration_cost;
extern unsigned int sysctl_sched_nr_migrate;
extern unsigned int sysctl_sched_time_avg;
extern unsigned int sysctl_timer_migration;

int sched_nr_latency_handler(struct ctl_table *table, int write,
		void __user *buffer, size_t *length,
		loff_t *ppos);
#endif
#ifdef CONFIG_SCHED_DEBUG
static inline unsigned int get_sysctl_timer_migration(void)
{
	return sysctl_timer_migration;
}
#else
static inline unsigned int get_sysctl_timer_migration(void)
{
	return 1;
}
#endif
extern unsigned int sysctl_sched_rt_period;
extern int sysctl_sched_rt_runtime;

int sched_rt_handler(struct ctl_table *table, int write,
		void __user *buffer, size_t *lenp,
		loff_t *ppos);

extern unsigned int sysctl_sched_compat_yield;

#ifdef CONFIG_RT_MUTEXES
extern int rt_mutex_getprio(struct task_struct *p);
extern void rt_mutex_setprio(struct task_struct *p, int prio);
extern void rt_mutex_adjust_pi(struct task_struct *p);
#else
static inline int rt_mutex_getprio(struct task_struct *p)
{
	return p->normal_prio;
}
# define rt_mutex_adjust_pi(p)		do { } while (0)
#endif

extern void set_user_nice(struct task_struct *p, long nice);
extern int task_prio(const struct task_struct *p);
extern int task_nice(const struct task_struct *p);
extern int can_nice(const struct task_struct *p, const int nice);
extern int task_curr(const struct task_struct *p);
extern int idle_cpu(int cpu);
extern int sched_setscheduler(struct task_struct *, int, struct sched_param *);
extern int sched_setscheduler_nocheck(struct task_struct *, int,
				      struct sched_param *);
extern struct task_struct *idle_task(int cpu);
extern struct task_struct *curr_task(int cpu);
extern void set_curr_task(int cpu, struct task_struct *p);

void yield(void);

extern struct exec_domain	default_exec_domain;

union thread_union {
	struct thread_info thread_info;
	unsigned long stack[THREAD_SIZE/sizeof(long)];
};

#ifndef __HAVE_ARCH_KSTACK_END
static inline int kstack_end(void *addr)
{
	 
	return !(((unsigned long)addr+sizeof(void*)-1) & (THREAD_SIZE-sizeof(void*)));
}
#endif

extern union thread_union init_thread_union;
extern struct task_struct init_task;

extern struct   mm_struct init_mm;

extern struct pid_namespace init_pid_ns;

extern struct task_struct *find_task_by_vpid(pid_t nr);
extern struct task_struct *find_task_by_pid_ns(pid_t nr,
		struct pid_namespace *ns);

extern void __set_special_pids(struct pid *pid);

extern struct user_struct * alloc_uid(struct user_namespace *, uid_t);
static inline struct user_struct *get_uid(struct user_struct *u)
{
	atomic_inc(&u->__count);
	return u;
}
extern void free_uid(struct user_struct *);
extern void release_uids(struct user_namespace *ns);

#include <asm/current.h>

extern void do_timer(unsigned long ticks);

extern int wake_up_state(struct task_struct *tsk, unsigned int state);
extern int wake_up_process(struct task_struct *tsk);
extern void wake_up_new_task(struct task_struct *tsk,
				unsigned long clone_flags);
#ifdef CONFIG_SMP
 extern void kick_process(struct task_struct *tsk);
#else
 static inline void kick_process(struct task_struct *tsk) { }
#endif
extern void sched_fork(struct task_struct *p, int clone_flags);
extern void sched_dead(struct task_struct *p);

extern void proc_caches_init(void);
extern void flush_signals(struct task_struct *);
extern void __flush_signals(struct task_struct *);
extern void ignore_signals(struct task_struct *);
extern void flush_signal_handlers(struct task_struct *, int force_default);
extern int dequeue_signal(struct task_struct *tsk, sigset_t *mask, siginfo_t *info);

static inline int dequeue_signal_lock(struct task_struct *tsk, sigset_t *mask, siginfo_t *info)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&tsk->sighand->siglock, flags);
	ret = dequeue_signal(tsk, mask, info);
	spin_unlock_irqrestore(&tsk->sighand->siglock, flags);

	return ret;
}	

extern void block_all_signals(int (*notifier)(void *priv), void *priv,
			      sigset_t *mask);
extern void unblock_all_signals(void);
extern void release_task(struct task_struct * p);
extern int send_sig_info(int, struct siginfo *, struct task_struct *);
extern int force_sigsegv(int, struct task_struct *);
extern int force_sig_info(int, struct siginfo *, struct task_struct *);
extern int __kill_pgrp_info(int sig, struct siginfo *info, struct pid *pgrp);
extern int kill_pid_info(int sig, struct siginfo *info, struct pid *pid);
extern int kill_pid_info_as_uid(int, struct siginfo *, struct pid *, uid_t, uid_t, u32);
extern int kill_pgrp(struct pid *pid, int sig, int priv);
extern int kill_pid(struct pid *pid, int sig, int priv);
extern int kill_proc_info(int, struct siginfo *, pid_t);
extern int do_notify_parent(struct task_struct *, int);
extern void __wake_up_parent(struct task_struct *p, struct task_struct *parent);
extern void force_sig(int, struct task_struct *);
extern void force_sig_specific(int, struct task_struct *);
extern int send_sig(int, struct task_struct *, int);
extern void zap_other_threads(struct task_struct *p);
extern struct sigqueue *sigqueue_alloc(void);
extern void sigqueue_free(struct sigqueue *);
extern int send_sigqueue(struct sigqueue *,  struct task_struct *, int group);
extern int do_sigaction(int, struct k_sigaction *, struct k_sigaction *);
extern int do_sigaltstack(const stack_t __user *, stack_t __user *, unsigned long);

static inline int kill_cad_pid(int sig, int priv)
{
	return kill_pid(cad_pid, sig, priv);
}

#define SEND_SIG_NOINFO ((struct siginfo *) 0)
#define SEND_SIG_PRIV	((struct siginfo *) 1)
#define SEND_SIG_FORCED	((struct siginfo *) 2)

static inline int is_si_special(const struct siginfo *info)
{
	return info <= SEND_SIG_FORCED;
}

static inline int on_sig_stack(unsigned long sp)
{
#ifdef CONFIG_STACK_GROWSUP
	return sp >= current->sas_ss_sp &&
		sp - current->sas_ss_sp < current->sas_ss_size;
#else
	return sp > current->sas_ss_sp &&
		sp - current->sas_ss_sp <= current->sas_ss_size;
#endif
}

static inline int sas_ss_flags(unsigned long sp)
{
	return (current->sas_ss_size == 0 ? SS_DISABLE
		: on_sig_stack(sp) ? SS_ONSTACK : 0);
}

extern struct mm_struct * mm_alloc(void);

extern void __mmdrop(struct mm_struct *);
static inline void mmdrop(struct mm_struct * mm)
{
	if (unlikely(atomic_dec_and_test(&mm->mm_count)))
		__mmdrop(mm);
}

extern void mmput(struct mm_struct *);
 
extern struct mm_struct *get_task_mm(struct task_struct *task);
 
extern void mm_release(struct task_struct *, struct mm_struct *);
 
extern struct mm_struct *dup_mm(struct task_struct *tsk);

extern int copy_thread(unsigned long, unsigned long, unsigned long,
			struct task_struct *, struct pt_regs *);
extern void flush_thread(void);
extern void exit_thread(void);

extern void exit_files(struct task_struct *);
extern void __cleanup_signal(struct signal_struct *);
extern void __cleanup_sighand(struct sighand_struct *);

extern void exit_itimers(struct signal_struct *);
extern void flush_itimer_signals(void);

extern NORET_TYPE void do_group_exit(int);

extern void daemonize(const char *, ...);
extern int allow_signal(int);
extern int disallow_signal(int);

extern int do_execve(char *, char __user * __user *, char __user * __user *, struct pt_regs *);
extern long do_fork(unsigned long, unsigned long, struct pt_regs *, unsigned long, int __user *, int __user *);
struct task_struct *fork_idle(int);

extern void set_task_comm(struct task_struct *tsk, char *from);
extern char *get_task_comm(char *to, struct task_struct *tsk);

#ifdef CONFIG_SMP
extern void wait_task_context_switch(struct task_struct *p);
extern unsigned long wait_task_inactive(struct task_struct *, long match_state);
#else
static inline void wait_task_context_switch(struct task_struct *p) {}
static inline unsigned long wait_task_inactive(struct task_struct *p,
					       long match_state)
{
	return 1;
}
#endif

#define next_task(p) \
	list_entry_rcu((p)->tasks.next, struct task_struct, tasks)

#define for_each_process(p) \
	for (p = &init_task ; (p = next_task(p)) != &init_task ; )

extern bool current_is_single_threaded(void);

#define do_each_thread(g, t) \
	for (g = t = &init_task ; (g = t = next_task(g)) != &init_task ; ) do

#define while_each_thread(g, t) \
	while ((t = next_thread(t)) != g)

#define thread_group_leader(p)	(p == p->group_leader)

static inline int has_group_leader_pid(struct task_struct *p)
{
	return p->pid == p->tgid;
}

static inline
int same_thread_group(struct task_struct *p1, struct task_struct *p2)
{
	return p1->tgid == p2->tgid;
}

static inline struct task_struct *next_thread(const struct task_struct *p)
{
	return list_entry_rcu(p->thread_group.next,
			      struct task_struct, thread_group);
}

static inline int thread_group_empty(struct task_struct *p)
{
	return list_empty(&p->thread_group);
}

#define delay_group_leader(p) \
		(thread_group_leader(p) && !thread_group_empty(p))

static inline int task_detached(struct task_struct *p)
{
	return p->exit_signal == -1;
}

static inline void task_lock(struct task_struct *p)
{
	spin_lock(&p->alloc_lock);
}

static inline void task_unlock(struct task_struct *p)
{
	spin_unlock(&p->alloc_lock);
}

extern struct sighand_struct *lock_task_sighand(struct task_struct *tsk,
							unsigned long *flags);

static inline void unlock_task_sighand(struct task_struct *tsk,
						unsigned long *flags)
{
	spin_unlock_irqrestore(&tsk->sighand->siglock, *flags);
}

#ifndef __HAVE_THREAD_FUNCTIONS

#define task_thread_info(task)	((struct thread_info *)(task)->stack)
#define task_stack_page(task)	((task)->stack)

static inline void setup_thread_stack(struct task_struct *p, struct task_struct *org)
{
	*task_thread_info(p) = *task_thread_info(org);
	task_thread_info(p)->task = p;
}

static inline unsigned long *end_of_stack(struct task_struct *p)
{
	return (unsigned long *)(task_thread_info(p) + 1);
}

#endif

static inline int object_is_on_stack(void *obj)
{
	void *stack = task_stack_page(current);

	return (obj >= stack) && (obj < (stack + THREAD_SIZE));
}

extern void thread_info_cache_init(void);

#ifdef CONFIG_DEBUG_STACK_USAGE
static inline unsigned long stack_not_used(struct task_struct *p)
{
	unsigned long *n = end_of_stack(p);

	do { 	 
		n++;
	} while (!*n);

	return (unsigned long)n - (unsigned long)end_of_stack(p);
}
#endif

static inline void set_tsk_thread_flag(struct task_struct *tsk, int flag)
{
	set_ti_thread_flag(task_thread_info(tsk), flag);
}

static inline void clear_tsk_thread_flag(struct task_struct *tsk, int flag)
{
	clear_ti_thread_flag(task_thread_info(tsk), flag);
}

static inline int test_and_set_tsk_thread_flag(struct task_struct *tsk, int flag)
{
	return test_and_set_ti_thread_flag(task_thread_info(tsk), flag);
}

static inline int test_and_clear_tsk_thread_flag(struct task_struct *tsk, int flag)
{
	return test_and_clear_ti_thread_flag(task_thread_info(tsk), flag);
}

static inline int test_tsk_thread_flag(struct task_struct *tsk, int flag)
{
	return test_ti_thread_flag(task_thread_info(tsk), flag);
}

static inline void set_tsk_need_resched(struct task_struct *tsk)
{
	set_tsk_thread_flag(tsk,TIF_NEED_RESCHED);
}

static inline void clear_tsk_need_resched(struct task_struct *tsk)
{
	clear_tsk_thread_flag(tsk,TIF_NEED_RESCHED);
}

static inline int test_tsk_need_resched(struct task_struct *tsk)
{
	return unlikely(test_tsk_thread_flag(tsk,TIF_NEED_RESCHED));
}

static inline int restart_syscall(void)
{
	set_tsk_thread_flag(current, TIF_SIGPENDING);
	return -ERESTARTNOINTR;
}

static inline int signal_pending(struct task_struct *p)
{
	return unlikely(test_tsk_thread_flag(p,TIF_SIGPENDING));
}

static inline int __fatal_signal_pending(struct task_struct *p)
{
	return unlikely(sigismember(&p->pending.signal, SIGKILL));
}

static inline int fatal_signal_pending(struct task_struct *p)
{
	return signal_pending(p) && __fatal_signal_pending(p);
}

static inline int signal_pending_state(long state, struct task_struct *p)
{
	if (!(state & (TASK_INTERRUPTIBLE | TASK_WAKEKILL)))
		return 0;
	if (!signal_pending(p))
		return 0;

	return (state & TASK_INTERRUPTIBLE) || __fatal_signal_pending(p);
}

static inline int need_resched(void)
{
	return unlikely(test_thread_flag(TIF_NEED_RESCHED));
}

extern int _cond_resched(void);

#define cond_resched() ({			\
	__might_sleep(__FILE__, __LINE__, 0);	\
	_cond_resched();			\
})

extern int __cond_resched_lock(spinlock_t *lock);

#ifdef CONFIG_PREEMPT
#define PREEMPT_LOCK_OFFSET	PREEMPT_OFFSET
#else
#define PREEMPT_LOCK_OFFSET	0
#endif

#define cond_resched_lock(lock) ({				\
	__might_sleep(__FILE__, __LINE__, PREEMPT_LOCK_OFFSET);	\
	__cond_resched_lock(lock);				\
})

extern int __cond_resched_softirq(void);

#define cond_resched_softirq() ({				\
	__might_sleep(__FILE__, __LINE__, SOFTIRQ_OFFSET);	\
	__cond_resched_softirq();				\
})

static inline int spin_needbreak(spinlock_t *lock)
{
#ifdef CONFIG_PREEMPT
	return spin_is_contended(lock);
#else
	return 0;
#endif
}

void thread_group_cputime(struct task_struct *tsk, struct task_cputime *times);
void thread_group_cputimer(struct task_struct *tsk, struct task_cputime *times);

static inline void thread_group_cputime_init(struct signal_struct *sig)
{
	sig->cputimer.cputime = INIT_CPUTIME;
	spin_lock_init(&sig->cputimer.lock);
	sig->cputimer.running = 0;
}

static inline void thread_group_cputime_free(struct signal_struct *sig)
{
}

extern void recalc_sigpending_and_wake(struct task_struct *t);
extern void recalc_sigpending(void);

extern void signal_wake_up(struct task_struct *t, int resume_stopped);

#ifdef CONFIG_SMP

static inline unsigned int task_cpu(const struct task_struct *p)
{
	return task_thread_info(p)->cpu;
}

extern void set_task_cpu(struct task_struct *p, unsigned int cpu);

#else

static inline unsigned int task_cpu(const struct task_struct *p)
{
	return 0;
}

static inline void set_task_cpu(struct task_struct *p, unsigned int cpu)
{
}

#endif  

extern void arch_pick_mmap_layout(struct mm_struct *mm);

#ifdef CONFIG_TRACING
extern void
__trace_special(void *__tr, void *__data,
		unsigned long arg1, unsigned long arg2, unsigned long arg3);
#else
static inline void
__trace_special(void *__tr, void *__data,
		unsigned long arg1, unsigned long arg2, unsigned long arg3)
{
}
#endif

extern long sched_setaffinity(pid_t pid, const struct cpumask *new_mask);
extern long sched_getaffinity(pid_t pid, struct cpumask *mask);

extern void normalize_rt_tasks(void);

#ifdef CONFIG_GROUP_SCHED

extern struct task_group init_task_group;
#ifdef CONFIG_USER_SCHED
extern struct task_group root_task_group;
extern void set_tg_uid(struct user_struct *user);
#endif

extern struct task_group *sched_create_group(struct task_group *parent);
extern void sched_destroy_group(struct task_group *tg);
extern void sched_move_task(struct task_struct *tsk);
#ifdef CONFIG_FAIR_GROUP_SCHED
extern int sched_group_set_shares(struct task_group *tg, unsigned long shares);
extern unsigned long sched_group_shares(struct task_group *tg);
#endif
#ifdef CONFIG_RT_GROUP_SCHED
extern int sched_group_set_rt_runtime(struct task_group *tg,
				      long rt_runtime_us);
extern long sched_group_rt_runtime(struct task_group *tg);
extern int sched_group_set_rt_period(struct task_group *tg,
				      long rt_period_us);
extern long sched_group_rt_period(struct task_group *tg);
extern int sched_rt_can_attach(struct task_group *tg, struct task_struct *tsk);
#endif
#endif

extern int task_can_switch_user(struct user_struct *up,
					struct task_struct *tsk);

#ifdef CONFIG_TASK_XACCT
static inline void add_rchar(struct task_struct *tsk, ssize_t amt)
{
	tsk->ioac.rchar += amt;
}

static inline void add_wchar(struct task_struct *tsk, ssize_t amt)
{
	tsk->ioac.wchar += amt;
}

static inline void inc_syscr(struct task_struct *tsk)
{
	tsk->ioac.syscr++;
}

static inline void inc_syscw(struct task_struct *tsk)
{
	tsk->ioac.syscw++;
}
#else
static inline void add_rchar(struct task_struct *tsk, ssize_t amt)
{
}

static inline void add_wchar(struct task_struct *tsk, ssize_t amt)
{
}

static inline void inc_syscr(struct task_struct *tsk)
{
}

static inline void inc_syscw(struct task_struct *tsk)
{
}
#endif

#ifndef TASK_SIZE_OF
#define TASK_SIZE_OF(tsk)	TASK_SIZE
#endif

extern void task_oncpu_function_call(struct task_struct *p,
				     void (*func) (void *info), void *info);

#ifdef CONFIG_MM_OWNER
extern void mm_update_next_owner(struct mm_struct *mm);
extern void mm_init_owner(struct mm_struct *mm, struct task_struct *p);
#else
static inline void mm_update_next_owner(struct mm_struct *mm)
{
}

static inline void mm_init_owner(struct mm_struct *mm, struct task_struct *p)
{
}
#endif  

#define TASK_STATE_TO_CHAR_STR "RSDTtZX"

static inline unsigned long task_rlimit(const struct task_struct *tsk,
		unsigned int limit)
{
	return ACCESS_ONCE(tsk->signal->rlim[limit].rlim_cur);
}

static inline unsigned long task_rlimit_max(const struct task_struct *tsk,
		unsigned int limit)
{
	return ACCESS_ONCE(tsk->signal->rlim[limit].rlim_max);
}

static inline unsigned long rlimit(unsigned int limit)
{
	return task_rlimit(current, limit);
}

static inline unsigned long rlimit_max(unsigned int limit)
{
	return task_rlimit_max(current, limit);
}

#endif  

#endif
