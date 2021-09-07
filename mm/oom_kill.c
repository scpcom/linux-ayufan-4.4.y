 
#include <linux/oom.h>
#include <linux/mm.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/swap.h>
#include <linux/timex.h>
#include <linux/jiffies.h>
#include <linux/cpuset.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/memcontrol.h>
#include <linux/security.h>

int sysctl_panic_on_oom;
int sysctl_oom_kill_allocating_task;
int sysctl_oom_dump_tasks;
static DEFINE_SPINLOCK(zone_scan_lock);
 
static int has_intersects_mems_allowed(struct task_struct *tsk)
{
	struct task_struct *t;

	t = tsk;
	do {
		if (cpuset_mems_allowed_intersects(current, t))
			return 1;
		t = next_thread(t);
	} while (t != tsk);

	return 0;
}

unsigned long badness(struct task_struct *p, unsigned long uptime)
{
	unsigned long points, cpu_time, run_time;
	struct mm_struct *mm;
	struct task_struct *child;
	int oom_adj = p->signal->oom_adj;
	struct task_cputime task_time;
	unsigned long utime;
	unsigned long stime;

	if (oom_adj == OOM_DISABLE)
		return 0;

	task_lock(p);
	mm = p->mm;
	if (!mm) {
		task_unlock(p);
		return 0;
	}

	points = mm->total_vm;

	task_unlock(p);

	if (p->flags & PF_OOM_ORIGIN)
		return ULONG_MAX;

	list_for_each_entry(child, &p->children, sibling) {
		task_lock(child);
		if (child->mm != mm && child->mm)
			points += child->mm->total_vm/2 + 1;
		task_unlock(child);
	}

	thread_group_cputime(p, &task_time);
	utime = cputime_to_jiffies(task_time.utime);
	stime = cputime_to_jiffies(task_time.stime);
	cpu_time = (utime + stime) >> (SHIFT_HZ + 3);

	if (uptime >= p->start_time.tv_sec)
		run_time = (uptime - p->start_time.tv_sec) >> 10;
	else
		run_time = 0;

	if (cpu_time)
		points /= int_sqrt(cpu_time);
	if (run_time)
		points /= int_sqrt(int_sqrt(run_time));

	if (task_nice(p) > 0)
		points *= 2;

	if (has_capability_noaudit(p, CAP_SYS_ADMIN) ||
	    has_capability_noaudit(p, CAP_SYS_RESOURCE))
		points /= 4;

	if (has_capability_noaudit(p, CAP_SYS_RAWIO))
		points /= 4;

	if (!has_intersects_mems_allowed(p))
		points /= 8;

	if (oom_adj) {
		if (oom_adj > 0) {
			if (!points)
				points = 1;
			points <<= oom_adj;
		} else
			points >>= -(oom_adj);
	}

#ifdef DEBUG
	printk(KERN_DEBUG "OOMkill: task %d (%s) got %lu points\n",
	p->pid, p->comm, points);
#endif
	return points;
}

static inline enum oom_constraint constrained_alloc(struct zonelist *zonelist,
						    gfp_t gfp_mask)
{
#ifdef CONFIG_NUMA
	struct zone *zone;
	struct zoneref *z;
	enum zone_type high_zoneidx = gfp_zone(gfp_mask);
	nodemask_t nodes = node_states[N_HIGH_MEMORY];

	for_each_zone_zonelist(zone, z, zonelist, high_zoneidx)
		if (cpuset_zone_allowed_softwall(zone, gfp_mask))
			node_clear(zone_to_nid(zone), nodes);
		else
			return CONSTRAINT_CPUSET;

	if (!nodes_empty(nodes))
		return CONSTRAINT_MEMORY_POLICY;
#endif

	return CONSTRAINT_NONE;
}

static struct task_struct *select_bad_process(unsigned long *ppoints,
						struct mem_cgroup *mem)
{
	struct task_struct *p;
	struct task_struct *chosen = NULL;
	struct timespec uptime;
	*ppoints = 0;

	do_posix_clock_monotonic_gettime(&uptime);
	for_each_process(p) {
		unsigned long points;

		if (!p->mm)
			continue;
		 
		if (is_global_init(p))
			continue;
		if (mem && !task_in_mem_cgroup(p, mem))
			continue;

		if (test_tsk_thread_flag(p, TIF_MEMDIE))
			return ERR_PTR(-1UL);

		if (p->flags & PF_EXITING) {
			if (p != current)
				return ERR_PTR(-1UL);

			chosen = p;
			*ppoints = ULONG_MAX;
		}

		if (p->signal->oom_adj == OOM_DISABLE)
			continue;

		points = badness(p, uptime.tv_sec);
		if (points > *ppoints || !chosen) {
			chosen = p;
			*ppoints = points;
		}
	}

	return chosen;
}

static void dump_tasks(const struct mem_cgroup *mem)
{
	struct task_struct *g, *p;

	printk(KERN_INFO "[ pid ]   uid  tgid total_vm      rss cpu oom_adj "
	       "name\n");
	do_each_thread(g, p) {
		struct mm_struct *mm;

		if (mem && !task_in_mem_cgroup(p, mem))
			continue;
		if (!thread_group_leader(p))
			continue;

		task_lock(p);
		mm = p->mm;
		if (!mm) {
			 
			task_unlock(p);
			continue;
		}
		printk(KERN_INFO "[%5d] %5d %5d %8lu %8lu %3d     %3d %s\n",
		       p->pid, __task_cred(p)->uid, p->tgid, mm->total_vm,
		       get_mm_rss(mm), (int)task_cpu(p), p->signal->oom_adj,
		       p->comm);
		task_unlock(p);
	} while_each_thread(g, p);
}

static void __oom_kill_task(struct task_struct *p, int verbose)
{
	if (is_global_init(p)) {
		WARN_ON(1);
		printk(KERN_WARNING "tried to kill init!\n");
		return;
	}

	if (!p->mm) {
		WARN_ON(1);
		printk(KERN_WARNING "tried to kill an mm-less task!\n");
		return;
	}

	if (verbose)
		printk(KERN_ERR "Killed process %d (%s)\n",
				task_pid_nr(p), p->comm);

	p->rt.time_slice = HZ;
	set_tsk_thread_flag(p, TIF_MEMDIE);

	force_sig(SIGKILL, p);
}

#ifdef SYNO_FORCE_UNMOUNT
void SYNO_stop_task(struct task_struct *p)
{
	if (is_global_init(p) && !p->mm) {
		return;
	}
#ifdef SYNO_DEBUG_FORCE_UNMOUNT
	printk(KERN_ERR "Force umount stop process %d (%s)\n", task_pid_nr(p), p->comm);
#endif

	force_sig(SIGSTOP, p);
}
EXPORT_SYMBOL(SYNO_stop_task);

void SYNO_start_task(struct task_struct *p)
{
	if (is_global_init(p) && !p->mm) {
		return;
	}
#ifdef SYNO_DEBUG_FORCE_UNMOUNT
	printk(KERN_ERR "Force umount start process %d (%s)\n", task_pid_nr(p), p->comm);
#endif

	force_sig(SIGCONT, p);
}
EXPORT_SYMBOL(SYNO_start_task);
#endif

static int oom_kill_task(struct task_struct *p)
{
	 
	if (!p->mm || p->signal->oom_adj == OOM_DISABLE)
		return 1;

	__oom_kill_task(p, 1);

	return 0;
}

static int oom_kill_process(struct task_struct *p, gfp_t gfp_mask, int order,
			    unsigned long points, struct mem_cgroup *mem,
			    const char *message)
{
	struct task_struct *c;

	if (printk_ratelimit()) {
		printk(KERN_WARNING "%s invoked oom-killer: "
			"gfp_mask=0x%x, order=%d, oom_adj=%d\n",
			current->comm, gfp_mask, order,
			current->signal->oom_adj);
		task_lock(current);
		cpuset_print_task_mems_allowed(current);
		task_unlock(current);
		dump_stack();
		mem_cgroup_print_oom_info(mem, p);
		show_mem();
		if (sysctl_oom_dump_tasks)
			dump_tasks(mem);
	}

	if (p->flags & PF_EXITING) {
		__oom_kill_task(p, 0);
		return 0;
	}

	printk(KERN_ERR "%s: kill process %d (%s) score %li or a child\n",
					message, task_pid_nr(p), p->comm, points);

	list_for_each_entry(c, &p->children, sibling) {
		if (c->mm == p->mm)
			continue;
		if (mem && !task_in_mem_cgroup(c, mem))
			continue;
		if (!oom_kill_task(c))
			return 0;
	}
	return oom_kill_task(p);
}

#ifdef CONFIG_CGROUP_MEM_RES_CTLR
void mem_cgroup_out_of_memory(struct mem_cgroup *mem, gfp_t gfp_mask)
{
	unsigned long points = 0;
	struct task_struct *p;

	read_lock(&tasklist_lock);
retry:
	p = select_bad_process(&points, mem);
	if (PTR_ERR(p) == -1UL)
		goto out;

	if (!p)
		p = current;

	if (oom_kill_process(p, gfp_mask, 0, points, mem,
				"Memory cgroup out of memory"))
		goto retry;
out:
	read_unlock(&tasklist_lock);
}
#endif

static BLOCKING_NOTIFIER_HEAD(oom_notify_list);

int register_oom_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&oom_notify_list, nb);
}
EXPORT_SYMBOL_GPL(register_oom_notifier);

int unregister_oom_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&oom_notify_list, nb);
}
EXPORT_SYMBOL_GPL(unregister_oom_notifier);

int try_set_zone_oom(struct zonelist *zonelist, gfp_t gfp_mask)
{
	struct zoneref *z;
	struct zone *zone;
	int ret = 1;

	spin_lock(&zone_scan_lock);
	for_each_zone_zonelist(zone, z, zonelist, gfp_zone(gfp_mask)) {
		if (zone_is_oom_locked(zone)) {
			ret = 0;
			goto out;
		}
	}

	for_each_zone_zonelist(zone, z, zonelist, gfp_zone(gfp_mask)) {
		 
		zone_set_flag(zone, ZONE_OOM_LOCKED);
	}

out:
	spin_unlock(&zone_scan_lock);
	return ret;
}

void clear_zonelist_oom(struct zonelist *zonelist, gfp_t gfp_mask)
{
	struct zoneref *z;
	struct zone *zone;

	spin_lock(&zone_scan_lock);
	for_each_zone_zonelist(zone, z, zonelist, gfp_zone(gfp_mask)) {
		zone_clear_flag(zone, ZONE_OOM_LOCKED);
	}
	spin_unlock(&zone_scan_lock);
}

static void __out_of_memory(gfp_t gfp_mask, int order)
{
	struct task_struct *p;
	unsigned long points;

	if (sysctl_oom_kill_allocating_task)
		if (!oom_kill_process(current, gfp_mask, order, 0, NULL,
				"Out of memory (oom_kill_allocating_task)"))
			return;
retry:
	 
	p = select_bad_process(&points, NULL);

	if (PTR_ERR(p) == -1UL)
		return;

	if (!p) {
		read_unlock(&tasklist_lock);
		panic("Out of memory and no killable processes...\n");
	}

	if (oom_kill_process(p, gfp_mask, order, points, NULL,
			     "Out of memory"))
		goto retry;
}

void pagefault_out_of_memory(void)
{
	unsigned long freed = 0;

	blocking_notifier_call_chain(&oom_notify_list, 0, &freed);
	if (freed > 0)
		 
		return;

	if (mem_cgroup_oom_called(current))
		goto rest_and_return;

	if (sysctl_panic_on_oom)
		panic("out of memory from page fault. panic_on_oom is selected.\n");

	read_lock(&tasklist_lock);
	__out_of_memory(0, 0);  
	read_unlock(&tasklist_lock);

rest_and_return:
	if (!test_thread_flag(TIF_MEMDIE))
		schedule_timeout_uninterruptible(1);
}

void out_of_memory(struct zonelist *zonelist, gfp_t gfp_mask, int order)
{
	unsigned long freed = 0;
	enum oom_constraint constraint;

	blocking_notifier_call_chain(&oom_notify_list, 0, &freed);
	if (freed > 0)
		 
		return;

	if (sysctl_panic_on_oom == 2)
		panic("out of memory. Compulsory panic_on_oom is selected.\n");

	constraint = constrained_alloc(zonelist, gfp_mask);
	read_lock(&tasklist_lock);

	switch (constraint) {
	case CONSTRAINT_MEMORY_POLICY:
		oom_kill_process(current, gfp_mask, order, 0, NULL,
				"No available memory (MPOL_BIND)");
		break;

	case CONSTRAINT_NONE:
		if (sysctl_panic_on_oom)
			panic("out of memory. panic_on_oom is selected\n");
		 
	case CONSTRAINT_CPUSET:
		__out_of_memory(gfp_mask, order);
		break;
	}

	read_unlock(&tasklist_lock);

	if (!test_thread_flag(TIF_MEMDIE))
		schedule_timeout_uninterruptible(1);
}
