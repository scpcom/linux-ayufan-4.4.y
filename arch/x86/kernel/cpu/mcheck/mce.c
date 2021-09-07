#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/thread_info.h>
#include <linux/capability.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/ratelimit.h>
#include <linux/kallsyms.h>
#include <linux/rcupdate.h>
#include <linux/kobject.h>
#include <linux/uaccess.h>
#include <linux/kdebug.h>
#include <linux/kernel.h>
#include <linux/percpu.h>
#include <linux/string.h>
#include <linux/sysdev.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/sched.h>
#include <linux/sysfs.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kmod.h>
#include <linux/poll.h>
#include <linux/nmi.h>
#include <linux/cpu.h>
#include <linux/smp.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/debugfs.h>
#include <linux/syno.h>

#include <asm/processor.h>
#include <asm/hw_irq.h>
#include <asm/apic.h>
#include <asm/idle.h>
#include <asm/ipi.h>
#include <asm/mce.h>
#include <asm/msr.h>

#include "mce-internal.h"

int mce_disabled __read_mostly;

#define MISC_MCELOG_MINOR	227

#define SPINUNIT 100	 

atomic_t mce_entry;

DEFINE_PER_CPU(unsigned, mce_exception_count);

static int			tolerant		__read_mostly = 1;
static int			banks			__read_mostly;
static int			rip_msr			__read_mostly;
static int			mce_bootlog		__read_mostly = -1;
static int			monarch_timeout		__read_mostly = -1;
static int			mce_panic_timeout	__read_mostly;
static int			mce_dont_log_ce		__read_mostly;
int				mce_cmci_disabled	__read_mostly;
int				mce_ignore_ce		__read_mostly;
int				mce_ser			__read_mostly;

struct mce_bank                *mce_banks		__read_mostly;

static unsigned long		mce_need_notify;
static char			mce_helper[128];
static char			*mce_helper_argv[2] = { mce_helper, NULL };

static DECLARE_WAIT_QUEUE_HEAD(mce_wait);
static DEFINE_PER_CPU(struct mce, mces_seen);
static int			cpu_missing;

#ifdef MY_DEF_HERE
int (*funcSYNOECCNotification)(unsigned int type, unsigned int syndrome, u64 memAddr) = NULL;
EXPORT_SYMBOL(funcSYNOECCNotification);
#endif

static void default_decode_mce(struct mce *m)
{
	pr_emerg("No human readable MCE decoding support on this CPU type.\n");
	pr_emerg("Run the message through 'mcelog --ascii' to decode.\n");
}

void (*x86_mce_decode_callback)(struct mce *m) = default_decode_mce;
EXPORT_SYMBOL(x86_mce_decode_callback);

DEFINE_PER_CPU(mce_banks_t, mce_poll_banks) = {
	[0 ... BITS_TO_LONGS(MAX_NR_BANKS)-1] = ~0UL
};

static DEFINE_PER_CPU(struct work_struct, mce_work);

void mce_setup(struct mce *m)
{
	memset(m, 0, sizeof(struct mce));
	m->cpu = m->extcpu = smp_processor_id();
	rdtscll(m->tsc);
	 
	m->time = get_seconds();
	m->cpuvendor = boot_cpu_data.x86_vendor;
	m->cpuid = cpuid_eax(1);
#ifdef CONFIG_SMP
	m->socketid = cpu_data(m->extcpu).phys_proc_id;
#endif
	m->apicid = cpu_data(m->extcpu).initial_apicid;
	rdmsrl(MSR_IA32_MCG_CAP, m->mcgcap);
}

DEFINE_PER_CPU(struct mce, injectm);
EXPORT_PER_CPU_SYMBOL_GPL(injectm);

static struct mce_log mcelog = {
	.signature	= MCE_LOG_SIGNATURE,
	.len		= MCE_LOG_LEN,
	.recordlen	= sizeof(struct mce),
};

void mce_log(struct mce *mce)
{
	unsigned next, entry;

	mce->finished = 0;
	wmb();
	for (;;) {
		entry = rcu_dereference(mcelog.next);
		for (;;) {
			 
			if (entry >= MCE_LOG_LEN) {
				set_bit(MCE_OVERFLOW,
					(unsigned long *)&mcelog.flags);
				return;
			}
			 
			if (mcelog.entry[entry].finished) {
				entry++;
				continue;
			}
			break;
		}
		smp_rmb();
		next = entry + 1;
		if (cmpxchg(&mcelog.next, entry, next) == entry)
			break;
	}
	memcpy(mcelog.entry + entry, mce, sizeof(struct mce));
	wmb();
	mcelog.entry[entry].finished = 1;
	wmb();

	mce->finished = 1;
	set_bit(0, &mce_need_notify);
}

static void print_mce(struct mce *m)
{
	pr_emerg("CPU %d: Machine Check Exception: %16Lx Bank %d: %016Lx\n",
	       m->extcpu, m->mcgstatus, m->bank, m->status);

	if (m->ip) {
		pr_emerg("RIP%s %02x:<%016Lx> ",
			!(m->mcgstatus & MCG_STATUS_EIPV) ? " !INEXACT!" : "",
				m->cs, m->ip);

		if (m->cs == __KERNEL_CS)
			print_symbol("{%s}", m->ip);
		pr_cont("\n");
	}

	pr_emerg("TSC %llx ", m->tsc);
	if (m->addr)
		pr_cont("ADDR %llx ", m->addr);
	if (m->misc)
		pr_cont("MISC %llx ", m->misc);

	pr_cont("\n");
	pr_emerg("PROCESSOR %u:%x TIME %llu SOCKET %u APIC %x\n",
		m->cpuvendor, m->cpuid, m->time, m->socketid, m->apicid);

	x86_mce_decode_callback(m);
}

static void print_mce_head(void)
{
	pr_emerg("\nHARDWARE ERROR\n");
}

static void print_mce_tail(void)
{
	pr_emerg("This is not a software problem!\n");
}

#define PANIC_TIMEOUT 5  

static atomic_t mce_paniced;

static int fake_panic;
static atomic_t mce_fake_paniced;

static void wait_for_panic(void)
{
	long timeout = PANIC_TIMEOUT*USEC_PER_SEC;

	preempt_disable();
	local_irq_enable();
	while (timeout-- > 0)
		udelay(1);
	if (panic_timeout == 0)
		panic_timeout = mce_panic_timeout;
	panic("Panicing machine check CPU died");
}

static void mce_panic(char *msg, struct mce *final, char *exp)
{
	int i;

	if (!fake_panic) {
		 
		if (atomic_inc_return(&mce_paniced) > 1)
			wait_for_panic();
		barrier();

		bust_spinlocks(1);
		console_verbose();
	} else {
		 
		if (atomic_inc_return(&mce_fake_paniced) > 1)
			return;
	}
	print_mce_head();
	 
	for (i = 0; i < MCE_LOG_LEN; i++) {
		struct mce *m = &mcelog.entry[i];
		if (!(m->status & MCI_STATUS_VAL))
			continue;
		if (!(m->status & MCI_STATUS_UC))
			print_mce(m);
	}
	 
	for (i = 0; i < MCE_LOG_LEN; i++) {
		struct mce *m = &mcelog.entry[i];
		if (!(m->status & MCI_STATUS_VAL))
			continue;
		if (!(m->status & MCI_STATUS_UC))
			continue;
		if (!final || memcmp(m, final, sizeof(struct mce)))
			print_mce(m);
	}
	if (final)
		print_mce(final);
	if (cpu_missing)
		printk(KERN_EMERG "Some CPUs didn't answer in synchronization\n");
	print_mce_tail();
	if (exp)
		printk(KERN_EMERG "Machine check: %s\n", exp);
	if (!fake_panic) {
		if (panic_timeout == 0)
			panic_timeout = mce_panic_timeout;
		panic(msg);
	} else
		printk(KERN_EMERG "Fake kernel panic: %s\n", msg);
}

static int msr_to_offset(u32 msr)
{
	unsigned bank = __get_cpu_var(injectm.bank);

	if (msr == rip_msr)
		return offsetof(struct mce, ip);
	if (msr == MSR_IA32_MCx_STATUS(bank))
		return offsetof(struct mce, status);
	if (msr == MSR_IA32_MCx_ADDR(bank))
		return offsetof(struct mce, addr);
	if (msr == MSR_IA32_MCx_MISC(bank))
		return offsetof(struct mce, misc);
	if (msr == MSR_IA32_MCG_STATUS)
		return offsetof(struct mce, mcgstatus);
	return -1;
}

static u64 mce_rdmsrl(u32 msr)
{
	u64 v;

	if (__get_cpu_var(injectm).finished) {
		int offset = msr_to_offset(msr);

		if (offset < 0)
			return 0;
		return *(u64 *)((char *)&__get_cpu_var(injectm) + offset);
	}

	if (rdmsrl_safe(msr, &v)) {
		WARN_ONCE(1, "mce: Unable to read msr %d!\n", msr);
		 
		v = 0;
	}

	return v;
}

static void mce_wrmsrl(u32 msr, u64 v)
{
	if (__get_cpu_var(injectm).finished) {
		int offset = msr_to_offset(msr);

		if (offset >= 0)
			*(u64 *)((char *)&__get_cpu_var(injectm) + offset) = v;
		return;
	}
	wrmsrl(msr, v);
}

#define MCE_RING_SIZE 16	 

struct mce_ring {
	unsigned short start;
	unsigned short end;
	unsigned long ring[MCE_RING_SIZE];
};
static DEFINE_PER_CPU(struct mce_ring, mce_ring);

static int mce_ring_empty(void)
{
	struct mce_ring *r = &__get_cpu_var(mce_ring);

	return r->start == r->end;
}

static int mce_ring_get(unsigned long *pfn)
{
	struct mce_ring *r;
	int ret = 0;

	*pfn = 0;
	get_cpu();
	r = &__get_cpu_var(mce_ring);
	if (r->start == r->end)
		goto out;
	*pfn = r->ring[r->start];
	r->start = (r->start + 1) % MCE_RING_SIZE;
	ret = 1;
out:
	put_cpu();
	return ret;
}

static int mce_ring_add(unsigned long pfn)
{
	struct mce_ring *r = &__get_cpu_var(mce_ring);
	unsigned next;

	next = (r->end + 1) % MCE_RING_SIZE;
	if (next == r->start)
		return -1;
	r->ring[r->end] = pfn;
	wmb();
	r->end = next;
	return 0;
}

int mce_available(struct cpuinfo_x86 *c)
{
	if (mce_disabled)
		return 0;
	return cpu_has(c, X86_FEATURE_MCE) && cpu_has(c, X86_FEATURE_MCA);
}

static void mce_schedule_work(void)
{
	if (!mce_ring_empty()) {
		struct work_struct *work = &__get_cpu_var(mce_work);
		if (!work_pending(work))
			schedule_work(work);
	}
}

static inline void mce_get_rip(struct mce *m, struct pt_regs *regs)
{

	if (regs && (m->mcgstatus & (MCG_STATUS_RIPV|MCG_STATUS_EIPV))) {
		m->ip = regs->ip;
		m->cs = regs->cs;
	} else {
		m->ip = 0;
		m->cs = 0;
	}
	if (rip_msr)
		m->ip = mce_rdmsrl(rip_msr);
}

#ifdef CONFIG_X86_LOCAL_APIC
 
asmlinkage void smp_mce_self_interrupt(struct pt_regs *regs)
{
	ack_APIC_irq();
	exit_idle();
	irq_enter();
	mce_notify_irq();
	mce_schedule_work();
	irq_exit();
}
#endif

static void mce_report_event(struct pt_regs *regs)
{
	if (regs->flags & (X86_VM_MASK|X86_EFLAGS_IF)) {
		mce_notify_irq();
		 
		mce_schedule_work();
		return;
	}

#ifdef CONFIG_X86_LOCAL_APIC
	 
	if (!cpu_has_apic)
		return;

	apic->send_IPI_self(MCE_SELF_VECTOR);

	apic_wait_icr_idle();
#endif
}

DEFINE_PER_CPU(unsigned, mce_poll_count);

void machine_check_poll(enum mcp_flags flags, mce_banks_t *b)
{
	struct mce m;
#ifdef MY_DEF_HERE
	u64 mstatus, eccsyndrome;
#endif
	int i;

	__get_cpu_var(mce_poll_count)++;

	mce_setup(&m);

	m.mcgstatus = mce_rdmsrl(MSR_IA32_MCG_STATUS);
	for (i = 0; i < banks; i++) {
		if (!mce_banks[i].ctl || !test_bit(i, *b))
			continue;

		m.misc = 0;
		m.addr = 0;
		m.bank = i;
		m.tsc = 0;

		barrier();
		m.status = mce_rdmsrl(MSR_IA32_MCx_STATUS(i));
		if (!(m.status & MCI_STATUS_VAL))
			continue;

		if (!(flags & MCP_UC) &&
		    (m.status & (mce_ser ? MCI_STATUS_S : MCI_STATUS_UC)))
			continue;

		if (m.status & MCI_STATUS_MISCV)
			m.misc = mce_rdmsrl(MSR_IA32_MCx_MISC(i));
		if (m.status & MCI_STATUS_ADDRV)
			m.addr = mce_rdmsrl(MSR_IA32_MCx_ADDR(i));

#ifdef MY_DEF_HERE
		mstatus = ((m.status & SYNO_MCI_STATUS_ECC) >> SYNO_MCI_STATUS_UECC_SHIFT);
		eccsyndrome = ((m.status & SYNO_MCI_STATUS_ECC_SYNDROME) >> SYNO_MCI_STATUS_ECC_SYNDROME_SHIFT);
		if (funcSYNOECCNotification &&
			((m.status & SYNO_MCI_STATUS_ECC))) {
			funcSYNOECCNotification(((unsigned int *)(void *)&mstatus)[0], 
					((unsigned int *)(void *)&eccsyndrome)[0], m.addr);
		}
#endif

		if (!(flags & MCP_TIMESTAMP))
			m.tsc = 0;
		 
		if (!(flags & MCP_DONTLOG) && !mce_dont_log_ce) {
			mce_log(&m);
			add_taint(TAINT_MACHINE_CHECK);
		}

		mce_wrmsrl(MSR_IA32_MCx_STATUS(i), 0);
	}

	sync_core();
}
EXPORT_SYMBOL_GPL(machine_check_poll);

static int mce_no_way_out(struct mce *m, char **msg)
{
	int i;

	for (i = 0; i < banks; i++) {
		m->status = mce_rdmsrl(MSR_IA32_MCx_STATUS(i));
		if (mce_severity(m, tolerant, msg) >= MCE_PANIC_SEVERITY)
			return 1;
	}
	return 0;
}

static atomic_t mce_executing;

static atomic_t mce_callin;

static int mce_timed_out(u64 *t)
{
	 
	rmb();
	if (atomic_read(&mce_paniced))
		wait_for_panic();
	if (!monarch_timeout)
		goto out;
	if ((s64)*t < SPINUNIT) {
		 
		if (tolerant < 1)
			mce_panic("Timeout synchronizing machine check over CPUs",
				  NULL, NULL);
		cpu_missing = 1;
		return 1;
	}
	*t -= SPINUNIT;
out:
	touch_nmi_watchdog();
	return 0;
}

static void mce_reign(void)
{
	int cpu;
	struct mce *m = NULL;
	int global_worst = 0;
	char *msg = NULL;
	char *nmsg = NULL;

	for_each_possible_cpu(cpu) {
		int severity = mce_severity(&per_cpu(mces_seen, cpu), tolerant,
					    &nmsg);
		if (severity > global_worst) {
			msg = nmsg;
			global_worst = severity;
			m = &per_cpu(mces_seen, cpu);
		}
	}

	if (m && global_worst >= MCE_PANIC_SEVERITY && tolerant < 3)
		mce_panic("Fatal Machine check", m, msg);

	if (global_worst <= MCE_KEEP_SEVERITY && tolerant < 3)
		mce_panic("Machine check from unknown source", NULL, NULL);

	for_each_possible_cpu(cpu)
		memset(&per_cpu(mces_seen, cpu), 0, sizeof(struct mce));
}

static atomic_t global_nwo;

static int mce_start(int *no_way_out)
{
	int order;
	int cpus = num_online_cpus();
	u64 timeout = (u64)monarch_timeout * NSEC_PER_USEC;

	if (!timeout)
		return -1;

	atomic_add(*no_way_out, &global_nwo);
	 
	smp_wmb();
	order = atomic_inc_return(&mce_callin);

	while (atomic_read(&mce_callin) != cpus) {
		if (mce_timed_out(&timeout)) {
			atomic_set(&global_nwo, 0);
			return -1;
		}
		ndelay(SPINUNIT);
	}

	smp_rmb();

	if (order == 1) {
		 
		atomic_set(&mce_executing, 1);
	} else {
		 
		while (atomic_read(&mce_executing) < order) {
			if (mce_timed_out(&timeout)) {
				atomic_set(&global_nwo, 0);
				return -1;
			}
			ndelay(SPINUNIT);
		}
	}

	*no_way_out = atomic_read(&global_nwo);

	return order;
}

static int mce_end(int order)
{
	int ret = -1;
	u64 timeout = (u64)monarch_timeout * NSEC_PER_USEC;

	if (!timeout)
		goto reset;
	if (order < 0)
		goto reset;

	atomic_inc(&mce_executing);

	if (order == 1) {
		 
		int cpus = num_online_cpus();

		while (atomic_read(&mce_executing) <= cpus) {
			if (mce_timed_out(&timeout))
				goto reset;
			ndelay(SPINUNIT);
		}

		mce_reign();
		barrier();
		ret = 0;
	} else {
		 
		while (atomic_read(&mce_executing) != 0) {
			if (mce_timed_out(&timeout))
				goto reset;
			ndelay(SPINUNIT);
		}

		return 0;
	}

reset:
	atomic_set(&global_nwo, 0);
	atomic_set(&mce_callin, 0);
	barrier();

	atomic_set(&mce_executing, 0);
	return ret;
}

static int mce_usable_address(struct mce *m)
{
	if (!(m->status & MCI_STATUS_MISCV) || !(m->status & MCI_STATUS_ADDRV))
		return 0;
	if ((m->misc & 0x3f) > PAGE_SHIFT)
		return 0;
	if (((m->misc >> 6) & 7) != MCM_ADDR_PHYS)
		return 0;
	return 1;
}

static void mce_clear_state(unsigned long *toclear)
{
	int i;

	for (i = 0; i < banks; i++) {
		if (test_bit(i, toclear))
			mce_wrmsrl(MSR_IA32_MCx_STATUS(i), 0);
	}
}

void do_machine_check(struct pt_regs *regs, long error_code)
{
	struct mce m, *final;
	int i;
	int worst = 0;
	int severity;
	 
	int order;
	 
	int no_way_out = 0;
	 
	int kill_it = 0;
	DECLARE_BITMAP(toclear, MAX_NR_BANKS);
	char *msg = "Unknown";

	atomic_inc(&mce_entry);

	__get_cpu_var(mce_exception_count)++;

	if (notify_die(DIE_NMI, "machine check", regs, error_code,
			   18, SIGKILL) == NOTIFY_STOP)
		goto out;
	if (!banks)
		goto out;

	mce_setup(&m);

	m.mcgstatus = mce_rdmsrl(MSR_IA32_MCG_STATUS);
	final = &__get_cpu_var(mces_seen);
	*final = m;

	no_way_out = mce_no_way_out(&m, &msg);

	barrier();

	if (!(m.mcgstatus & MCG_STATUS_RIPV))
		kill_it = 1;

	order = mce_start(&no_way_out);
	for (i = 0; i < banks; i++) {
		__clear_bit(i, toclear);
		if (!mce_banks[i].ctl)
			continue;

		m.misc = 0;
		m.addr = 0;
		m.bank = i;

		m.status = mce_rdmsrl(MSR_IA32_MCx_STATUS(i));
		if ((m.status & MCI_STATUS_VAL) == 0)
			continue;

		if (!(m.status & (mce_ser ? MCI_STATUS_S : MCI_STATUS_UC)) &&
			!no_way_out)
			continue;

		add_taint(TAINT_MACHINE_CHECK);

		severity = mce_severity(&m, tolerant, NULL);

		if (severity == MCE_KEEP_SEVERITY && !no_way_out)
			continue;
		__set_bit(i, toclear);
		if (severity == MCE_NO_SEVERITY) {
			 
			continue;
		}

		if (severity == MCE_AR_SEVERITY)
			kill_it = 1;

		if (m.status & MCI_STATUS_MISCV)
			m.misc = mce_rdmsrl(MSR_IA32_MCx_MISC(i));
		if (m.status & MCI_STATUS_ADDRV)
			m.addr = mce_rdmsrl(MSR_IA32_MCx_ADDR(i));

		if (severity == MCE_AO_SEVERITY && mce_usable_address(&m))
			mce_ring_add(m.addr >> PAGE_SHIFT);

		mce_get_rip(&m, regs);
		mce_log(&m);

		if (severity > worst) {
			*final = m;
			worst = severity;
		}
	}

	if (!no_way_out)
		mce_clear_state(toclear);

	if (mce_end(order) < 0)
		no_way_out = worst >= MCE_PANIC_SEVERITY;

	if (no_way_out && tolerant < 3)
		mce_panic("Fatal machine check on current CPU", final, msg);

	if (kill_it && tolerant < 3)
		force_sig(SIGBUS, current);

	set_thread_flag(TIF_MCE_NOTIFY);

	if (worst > 0)
		mce_report_event(regs);
	mce_wrmsrl(MSR_IA32_MCG_STATUS, 0);
out:
	atomic_dec(&mce_entry);
	sync_core();
}
EXPORT_SYMBOL_GPL(do_machine_check);

void __attribute__((weak)) memory_failure(unsigned long pfn, int vector)
{
	printk(KERN_ERR "Action optional memory failure at %lx ignored\n", pfn);
}

void mce_notify_process(void)
{
	unsigned long pfn;
	mce_notify_irq();
	while (mce_ring_get(&pfn))
		memory_failure(pfn, MCE_VECTOR);
}

static void mce_process_work(struct work_struct *dummy)
{
	mce_notify_process();
}

#ifdef CONFIG_X86_MCE_INTEL
 
void mce_log_therm_throt_event(__u64 status)
{
	struct mce m;

	mce_setup(&m);
	m.bank = MCE_THERMAL_BANK;
	m.status = status;
	mce_log(&m);
}
#endif  

static int check_interval = 5 * 60;  

static DEFINE_PER_CPU(int, mce_next_interval);  
static DEFINE_PER_CPU(struct timer_list, mce_timer);

static void mcheck_timer(unsigned long data)
{
	struct timer_list *t = &per_cpu(mce_timer, data);
	int *n;

	WARN_ON(smp_processor_id() != data);

	if (mce_available(&current_cpu_data)) {
		machine_check_poll(MCP_TIMESTAMP,
				&__get_cpu_var(mce_poll_banks));
	}

	n = &__get_cpu_var(mce_next_interval);
	if (mce_notify_irq())
		*n = max(*n/2, HZ/100);
	else
		*n = min(*n*2, (int)round_jiffies_relative(check_interval*HZ));

	t->expires = jiffies + *n;
	add_timer_on(t, smp_processor_id());
}

static void mce_do_trigger(struct work_struct *work)
{
	call_usermodehelper(mce_helper, mce_helper_argv, NULL, UMH_NO_WAIT);
}

static DECLARE_WORK(mce_trigger_work, mce_do_trigger);

int mce_notify_irq(void)
{
	 
	static DEFINE_RATELIMIT_STATE(ratelimit, 60*HZ, 2);

	clear_thread_flag(TIF_MCE_NOTIFY);

	if (test_and_clear_bit(0, &mce_need_notify)) {
		wake_up_interruptible(&mce_wait);

		if (mce_helper[0] && !work_pending(&mce_trigger_work))
			schedule_work(&mce_trigger_work);

		if (__ratelimit(&ratelimit))
			printk(KERN_INFO "Machine check events logged\n");

		return 1;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(mce_notify_irq);

static int mce_banks_init(void)
{
	int i;

	mce_banks = kzalloc(banks * sizeof(struct mce_bank), GFP_KERNEL);
	if (!mce_banks)
		return -ENOMEM;
	for (i = 0; i < banks; i++) {
		struct mce_bank *b = &mce_banks[i];

		b->ctl = -1ULL;
		b->init = 1;
	}
	return 0;
}

static int __cpuinit mce_cap_init(void)
{
	unsigned b;
	u64 cap;

	rdmsrl(MSR_IA32_MCG_CAP, cap);

	b = cap & MCG_BANKCNT_MASK;
	if (!banks)
		printk(KERN_INFO "mce: CPU supports %d MCE banks\n", b);

	if (b > MAX_NR_BANKS) {
		printk(KERN_WARNING
		       "MCE: Using only %u machine check banks out of %u\n",
			MAX_NR_BANKS, b);
		b = MAX_NR_BANKS;
	}

	WARN_ON(banks != 0 && b != banks);
	banks = b;
	if (!mce_banks) {
		int err = mce_banks_init();

		if (err)
			return err;
	}

	if ((cap & MCG_EXT_P) && MCG_EXT_CNT(cap) >= 9)
		rip_msr = MSR_IA32_MCG_EIP;

	if (cap & MCG_SER_P)
		mce_ser = 1;

	return 0;
}

static void mce_init(void)
{
	mce_banks_t all_banks;
	u64 cap;
	int i;

	bitmap_fill(all_banks, MAX_NR_BANKS);
	machine_check_poll(MCP_UC|(!mce_bootlog ? MCP_DONTLOG : 0), &all_banks);

	set_in_cr4(X86_CR4_MCE);

	rdmsrl(MSR_IA32_MCG_CAP, cap);
	if (cap & MCG_CTL_P)
		wrmsr(MSR_IA32_MCG_CTL, 0xffffffff, 0xffffffff);

	for (i = 0; i < banks; i++) {
		struct mce_bank *b = &mce_banks[i];

		if (!b->init)
			continue;
		wrmsrl(MSR_IA32_MCx_CTL(i), b->ctl);
		wrmsrl(MSR_IA32_MCx_STATUS(i), 0);
	}
}

static int __cpuinit mce_cpu_quirks(struct cpuinfo_x86 *c)
{
	if (c->x86_vendor == X86_VENDOR_UNKNOWN) {
		pr_info("MCE: unknown CPU type - not enabling MCE support.\n");
		return -EOPNOTSUPP;
	}

	if (c->x86_vendor == X86_VENDOR_AMD) {
		if (c->x86 == 15 && banks > 4) {
			 
			clear_bit(10, (unsigned long *)&mce_banks[4].ctl);
		}
		if (c->x86 <= 17 && mce_bootlog < 0) {
			 
			mce_bootlog = 0;
		}
		 
		 if (c->x86 == 6 && banks > 0)
			mce_banks[0].ctl = 0;
	}

	if (c->x86_vendor == X86_VENDOR_INTEL) {
		 
		if (c->x86 == 6 && c->x86_model < 0x1A && banks > 0)
			mce_banks[0].init = 0;

		if ((c->x86 > 6 || (c->x86 == 6 && c->x86_model >= 0xe)) &&
			monarch_timeout < 0)
			monarch_timeout = USEC_PER_SEC;

		if (c->x86 == 6 && c->x86_model <= 13 && mce_bootlog < 0)
			mce_bootlog = 0;
	}
	if (monarch_timeout < 0)
		monarch_timeout = 0;
	if (mce_bootlog != 0)
		mce_panic_timeout = 30;

	return 0;
}

static void __cpuinit mce_ancient_init(struct cpuinfo_x86 *c)
{
	if (c->x86 != 5)
		return;
	switch (c->x86_vendor) {
	case X86_VENDOR_INTEL:
		intel_p5_mcheck_init(c);
		break;
	case X86_VENDOR_CENTAUR:
		winchip_mcheck_init(c);
		break;
	}
}

static void mce_cpu_features(struct cpuinfo_x86 *c)
{
	switch (c->x86_vendor) {
	case X86_VENDOR_INTEL:
		mce_intel_feature_init(c);
		break;
	case X86_VENDOR_AMD:
		mce_amd_feature_init(c);
		break;
	default:
		break;
	}
}

static void mce_init_timer(void)
{
	struct timer_list *t = &__get_cpu_var(mce_timer);
	int *n = &__get_cpu_var(mce_next_interval);

	setup_timer(t, mcheck_timer, smp_processor_id());

	if (mce_ignore_ce)
		return;

	*n = check_interval * HZ;
	if (!*n)
		return;
	t->expires = round_jiffies(jiffies + *n);
	add_timer_on(t, smp_processor_id());
}

static void unexpected_machine_check(struct pt_regs *regs, long error_code)
{
	printk(KERN_ERR "CPU#%d: Unexpected int18 (Machine Check).\n",
	       smp_processor_id());
}

void (*machine_check_vector)(struct pt_regs *, long error_code) =
						unexpected_machine_check;

void __cpuinit mcheck_init(struct cpuinfo_x86 *c)
{
	if (mce_disabled)
		return;

	mce_ancient_init(c);

	if (!mce_available(c))
		return;

	if (mce_cap_init() < 0 || mce_cpu_quirks(c) < 0) {
		mce_disabled = 1;
		return;
	}

	machine_check_vector = do_machine_check;

	mce_init();
	mce_cpu_features(c);
	mce_init_timer();
	INIT_WORK(&__get_cpu_var(mce_work), mce_process_work);
}

static DEFINE_SPINLOCK(mce_state_lock);
static int		open_count;		 
static int		open_exclu;		 

static int mce_open(struct inode *inode, struct file *file)
{
	spin_lock(&mce_state_lock);

	if (open_exclu || (open_count && (file->f_flags & O_EXCL))) {
		spin_unlock(&mce_state_lock);

		return -EBUSY;
	}

	if (file->f_flags & O_EXCL)
		open_exclu = 1;
	open_count++;

	spin_unlock(&mce_state_lock);

	return nonseekable_open(inode, file);
}

static int mce_release(struct inode *inode, struct file *file)
{
	spin_lock(&mce_state_lock);

	open_count--;
	open_exclu = 0;

	spin_unlock(&mce_state_lock);

	return 0;
}

static void collect_tscs(void *data)
{
	unsigned long *cpu_tsc = (unsigned long *)data;

	rdtscll(cpu_tsc[smp_processor_id()]);
}

static DEFINE_MUTEX(mce_read_mutex);

static ssize_t mce_read(struct file *filp, char __user *ubuf, size_t usize,
			loff_t *off)
{
	char __user *buf = ubuf;
	unsigned long *cpu_tsc;
	unsigned prev, next;
	int i, err;

	cpu_tsc = kmalloc(nr_cpu_ids * sizeof(long), GFP_KERNEL);
	if (!cpu_tsc)
		return -ENOMEM;

	mutex_lock(&mce_read_mutex);
	next = rcu_dereference(mcelog.next);

	if (*off != 0 || usize < MCE_LOG_LEN*sizeof(struct mce)) {
		mutex_unlock(&mce_read_mutex);
		kfree(cpu_tsc);

		return -EINVAL;
	}

	err = 0;
	prev = 0;
	do {
		for (i = prev; i < next; i++) {
			unsigned long start = jiffies;

			while (!mcelog.entry[i].finished) {
				if (time_after_eq(jiffies, start + 2)) {
					memset(mcelog.entry + i, 0,
					       sizeof(struct mce));
					goto timeout;
				}
				cpu_relax();
			}
			smp_rmb();
			err |= copy_to_user(buf, mcelog.entry + i,
					    sizeof(struct mce));
			buf += sizeof(struct mce);
timeout:
			;
		}

		memset(mcelog.entry + prev, 0,
		       (next - prev) * sizeof(struct mce));
		prev = next;
		next = cmpxchg(&mcelog.next, prev, 0);
	} while (next != prev);

	synchronize_sched();

	on_each_cpu(collect_tscs, cpu_tsc, 1);

	for (i = next; i < MCE_LOG_LEN; i++) {
		if (mcelog.entry[i].finished &&
		    mcelog.entry[i].tsc < cpu_tsc[mcelog.entry[i].cpu]) {
			err |= copy_to_user(buf, mcelog.entry+i,
					    sizeof(struct mce));
			smp_rmb();
			buf += sizeof(struct mce);
			memset(&mcelog.entry[i], 0, sizeof(struct mce));
		}
	}
	mutex_unlock(&mce_read_mutex);
	kfree(cpu_tsc);

	return err ? -EFAULT : buf - ubuf;
}

static unsigned int mce_poll(struct file *file, poll_table *wait)
{
	poll_wait(file, &mce_wait, wait);
	if (rcu_dereference(mcelog.next))
		return POLLIN | POLLRDNORM;
	return 0;
}

static long mce_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	int __user *p = (int __user *)arg;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	switch (cmd) {
	case MCE_GET_RECORD_LEN:
		return put_user(sizeof(struct mce), p);
	case MCE_GET_LOG_LEN:
		return put_user(MCE_LOG_LEN, p);
	case MCE_GETCLEAR_FLAGS: {
		unsigned flags;

		do {
			flags = mcelog.flags;
		} while (cmpxchg(&mcelog.flags, flags, 0) != flags);

		return put_user(flags, p);
	}
	default:
		return -ENOTTY;
	}
}

struct file_operations mce_chrdev_ops = {
	.open			= mce_open,
	.release		= mce_release,
	.read			= mce_read,
	.poll			= mce_poll,
	.unlocked_ioctl		= mce_ioctl,
};
EXPORT_SYMBOL_GPL(mce_chrdev_ops);

static struct miscdevice mce_log_device = {
	MISC_MCELOG_MINOR,
	"mcelog",
	&mce_chrdev_ops,
};

static int __init mcheck_enable(char *str)
{
	if (*str == 0) {
		enable_p5_mce();
		return 1;
	}
	if (*str == '=')
		str++;
	if (!strcmp(str, "off"))
		mce_disabled = 1;
	else if (!strcmp(str, "no_cmci"))
		mce_cmci_disabled = 1;
	else if (!strcmp(str, "dont_log_ce"))
		mce_dont_log_ce = 1;
	else if (!strcmp(str, "ignore_ce"))
		mce_ignore_ce = 1;
	else if (!strcmp(str, "bootlog") || !strcmp(str, "nobootlog"))
		mce_bootlog = (str[0] == 'b');
	else if (isdigit(str[0])) {
		get_option(&str, &tolerant);
		if (*str == ',') {
			++str;
			get_option(&str, &monarch_timeout);
		}
	} else {
		printk(KERN_INFO "mce argument %s ignored. Please use /sys\n",
		       str);
		return 0;
	}
	return 1;
}
__setup("mce", mcheck_enable);

static int mce_disable(void)
{
	int i;

	for (i = 0; i < banks; i++) {
		struct mce_bank *b = &mce_banks[i];

		if (b->init)
			wrmsrl(MSR_IA32_MCx_CTL(i), 0);
	}
	return 0;
}

static int mce_suspend(struct sys_device *dev, pm_message_t state)
{
	return mce_disable();
}

static int mce_shutdown(struct sys_device *dev)
{
	return mce_disable();
}

static int mce_resume(struct sys_device *dev)
{
	mce_init();
	mce_cpu_features(&current_cpu_data);

	return 0;
}

static void mce_cpu_restart(void *data)
{
	del_timer_sync(&__get_cpu_var(mce_timer));
	if (!mce_available(&current_cpu_data))
		return;
	mce_init();
	mce_init_timer();
}

static void mce_restart(void)
{
	on_each_cpu(mce_cpu_restart, NULL, 1);
}

static void mce_disable_ce(void *all)
{
	if (!mce_available(&current_cpu_data))
		return;
	if (all)
		del_timer_sync(&__get_cpu_var(mce_timer));
	cmci_clear();
}

static void mce_enable_ce(void *all)
{
	if (!mce_available(&current_cpu_data))
		return;
	cmci_reenable();
	cmci_recheck();
	if (all)
		mce_init_timer();
}

static struct sysdev_class mce_sysclass = {
	.suspend	= mce_suspend,
	.shutdown	= mce_shutdown,
	.resume		= mce_resume,
	.name		= "machinecheck",
};

DEFINE_PER_CPU(struct sys_device, mce_dev);

__cpuinitdata
void (*threshold_cpu_callback)(unsigned long action, unsigned int cpu);

static inline struct mce_bank *attr_to_bank(struct sysdev_attribute *attr)
{
	return container_of(attr, struct mce_bank, attr);
}

static ssize_t show_bank(struct sys_device *s, struct sysdev_attribute *attr,
			 char *buf)
{
	return sprintf(buf, "%llx\n", attr_to_bank(attr)->ctl);
}

static ssize_t set_bank(struct sys_device *s, struct sysdev_attribute *attr,
			const char *buf, size_t size)
{
	u64 new;

	if (strict_strtoull(buf, 0, &new) < 0)
		return -EINVAL;

	attr_to_bank(attr)->ctl = new;
	mce_restart();

	return size;
}

static ssize_t
show_trigger(struct sys_device *s, struct sysdev_attribute *attr, char *buf)
{
	strcpy(buf, mce_helper);
	strcat(buf, "\n");
	return strlen(mce_helper) + 1;
}

static ssize_t set_trigger(struct sys_device *s, struct sysdev_attribute *attr,
				const char *buf, size_t siz)
{
	char *p;

	strncpy(mce_helper, buf, sizeof(mce_helper));
	mce_helper[sizeof(mce_helper)-1] = 0;
	p = strchr(mce_helper, '\n');

	if (p)
		*p = 0;

	return strlen(mce_helper) + !!p;
}

static ssize_t set_ignore_ce(struct sys_device *s,
			     struct sysdev_attribute *attr,
			     const char *buf, size_t size)
{
	u64 new;

	if (strict_strtoull(buf, 0, &new) < 0)
		return -EINVAL;

	if (mce_ignore_ce ^ !!new) {
		if (new) {
			 
			on_each_cpu(mce_disable_ce, (void *)1, 1);
			mce_ignore_ce = 1;
		} else {
			 
			mce_ignore_ce = 0;
			on_each_cpu(mce_enable_ce, (void *)1, 1);
		}
	}
	return size;
}

static ssize_t set_cmci_disabled(struct sys_device *s,
				 struct sysdev_attribute *attr,
				 const char *buf, size_t size)
{
	u64 new;

	if (strict_strtoull(buf, 0, &new) < 0)
		return -EINVAL;

	if (mce_cmci_disabled ^ !!new) {
		if (new) {
			 
			on_each_cpu(mce_disable_ce, NULL, 1);
			mce_cmci_disabled = 1;
		} else {
			 
			mce_cmci_disabled = 0;
			on_each_cpu(mce_enable_ce, NULL, 1);
		}
	}
	return size;
}

static ssize_t store_int_with_restart(struct sys_device *s,
				      struct sysdev_attribute *attr,
				      const char *buf, size_t size)
{
	ssize_t ret = sysdev_store_int(s, attr, buf, size);
	mce_restart();
	return ret;
}

static SYSDEV_ATTR(trigger, 0644, show_trigger, set_trigger);
static SYSDEV_INT_ATTR(tolerant, 0644, tolerant);
static SYSDEV_INT_ATTR(monarch_timeout, 0644, monarch_timeout);
static SYSDEV_INT_ATTR(dont_log_ce, 0644, mce_dont_log_ce);

static struct sysdev_ext_attribute attr_check_interval = {
	_SYSDEV_ATTR(check_interval, 0644, sysdev_show_int,
		     store_int_with_restart),
	&check_interval
};

static struct sysdev_ext_attribute attr_ignore_ce = {
	_SYSDEV_ATTR(ignore_ce, 0644, sysdev_show_int, set_ignore_ce),
	&mce_ignore_ce
};

static struct sysdev_ext_attribute attr_cmci_disabled = {
	_SYSDEV_ATTR(cmci_disabled, 0644, sysdev_show_int, set_cmci_disabled),
	&mce_cmci_disabled
};

static struct sysdev_attribute *mce_attrs[] = {
	&attr_tolerant.attr,
	&attr_check_interval.attr,
	&attr_trigger,
	&attr_monarch_timeout.attr,
	&attr_dont_log_ce.attr,
	&attr_ignore_ce.attr,
	&attr_cmci_disabled.attr,
	NULL
};

static cpumask_var_t mce_dev_initialized;

static __cpuinit int mce_create_device(unsigned int cpu)
{
	int err;
	int i, j;

	if (!mce_available(&boot_cpu_data))
		return -EIO;

	memset(&per_cpu(mce_dev, cpu).kobj, 0, sizeof(struct kobject));
	per_cpu(mce_dev, cpu).id	= cpu;
	per_cpu(mce_dev, cpu).cls	= &mce_sysclass;

	err = sysdev_register(&per_cpu(mce_dev, cpu));
	if (err)
		return err;

	for (i = 0; mce_attrs[i]; i++) {
		err = sysdev_create_file(&per_cpu(mce_dev, cpu), mce_attrs[i]);
		if (err)
			goto error;
	}
	for (j = 0; j < banks; j++) {
		err = sysdev_create_file(&per_cpu(mce_dev, cpu),
					&mce_banks[j].attr);
		if (err)
			goto error2;
	}
	cpumask_set_cpu(cpu, mce_dev_initialized);

	return 0;
error2:
	while (--j >= 0)
		sysdev_remove_file(&per_cpu(mce_dev, cpu), &mce_banks[j].attr);
error:
	while (--i >= 0)
		sysdev_remove_file(&per_cpu(mce_dev, cpu), &mce_banks[i].attr);

	sysdev_unregister(&per_cpu(mce_dev, cpu));

	return err;
}

static __cpuinit void mce_remove_device(unsigned int cpu)
{
	int i;

	if (!cpumask_test_cpu(cpu, mce_dev_initialized))
		return;

	for (i = 0; mce_attrs[i]; i++)
		sysdev_remove_file(&per_cpu(mce_dev, cpu), mce_attrs[i]);

	for (i = 0; i < banks; i++)
		sysdev_remove_file(&per_cpu(mce_dev, cpu), &mce_banks[i].attr);

	sysdev_unregister(&per_cpu(mce_dev, cpu));
	cpumask_clear_cpu(cpu, mce_dev_initialized);
}

static void mce_disable_cpu(void *h)
{
	unsigned long action = *(unsigned long *)h;
	int i;

	if (!mce_available(&current_cpu_data))
		return;
	if (!(action & CPU_TASKS_FROZEN))
		cmci_clear();
	for (i = 0; i < banks; i++) {
		struct mce_bank *b = &mce_banks[i];

		if (b->init)
			wrmsrl(MSR_IA32_MCx_CTL(i), 0);
	}
}

static void mce_reenable_cpu(void *h)
{
	unsigned long action = *(unsigned long *)h;
	int i;

	if (!mce_available(&current_cpu_data))
		return;

	if (!(action & CPU_TASKS_FROZEN))
		cmci_reenable();
	for (i = 0; i < banks; i++) {
		struct mce_bank *b = &mce_banks[i];

		if (b->init)
			wrmsrl(MSR_IA32_MCx_CTL(i), b->ctl);
	}
}

static int __cpuinit
mce_cpu_callback(struct notifier_block *nfb, unsigned long action, void *hcpu)
{
	unsigned int cpu = (unsigned long)hcpu;
	struct timer_list *t = &per_cpu(mce_timer, cpu);

	switch (action) {
	case CPU_ONLINE:
	case CPU_ONLINE_FROZEN:
		mce_create_device(cpu);
		if (threshold_cpu_callback)
			threshold_cpu_callback(action, cpu);
		break;
	case CPU_DEAD:
	case CPU_DEAD_FROZEN:
		if (threshold_cpu_callback)
			threshold_cpu_callback(action, cpu);
		mce_remove_device(cpu);
		break;
	case CPU_DOWN_PREPARE:
	case CPU_DOWN_PREPARE_FROZEN:
		del_timer_sync(t);
		smp_call_function_single(cpu, mce_disable_cpu, &action, 1);
		break;
	case CPU_DOWN_FAILED:
	case CPU_DOWN_FAILED_FROZEN:
		if (!mce_ignore_ce && check_interval) {
			t->expires = round_jiffies(jiffies +
					   __get_cpu_var(mce_next_interval));
			add_timer_on(t, cpu);
		}
		smp_call_function_single(cpu, mce_reenable_cpu, &action, 1);
		break;
	case CPU_POST_DEAD:
		 
		cmci_rediscover(cpu);
		break;
	}
	return NOTIFY_OK;
}

static struct notifier_block mce_cpu_notifier __cpuinitdata = {
	.notifier_call = mce_cpu_callback,
};

static __init void mce_init_banks(void)
{
	int i;

	for (i = 0; i < banks; i++) {
		struct mce_bank *b = &mce_banks[i];
		struct sysdev_attribute *a = &b->attr;

		a->attr.name	= b->attrname;
		snprintf(b->attrname, ATTR_LEN, "bank%d", i);

		a->attr.mode	= 0644;
		a->show		= show_bank;
		a->store	= set_bank;
	}
}

static __init int mce_init_device(void)
{
	int err;
	int i = 0;

	if (!mce_available(&boot_cpu_data))
		return -EIO;

	zalloc_cpumask_var(&mce_dev_initialized, GFP_KERNEL);

	mce_init_banks();

	err = sysdev_class_register(&mce_sysclass);
	if (err)
		return err;

	for_each_online_cpu(i) {
		err = mce_create_device(i);
		if (err)
			return err;
	}

	register_hotcpu_notifier(&mce_cpu_notifier);
	misc_register(&mce_log_device);

	return err;
}

device_initcall(mce_init_device);

static int __init mcheck_disable(char *str)
{
	mce_disabled = 1;
	return 1;
}
__setup("nomce", mcheck_disable);

#ifdef CONFIG_DEBUG_FS
struct dentry *mce_get_debugfs_dir(void)
{
	static struct dentry *dmce;

	if (!dmce)
		dmce = debugfs_create_dir("mce", NULL);

	return dmce;
}

static void mce_reset(void)
{
	cpu_missing = 0;
	atomic_set(&mce_fake_paniced, 0);
	atomic_set(&mce_executing, 0);
	atomic_set(&mce_callin, 0);
	atomic_set(&global_nwo, 0);
}

static int fake_panic_get(void *data, u64 *val)
{
	*val = fake_panic;
	return 0;
}

static int fake_panic_set(void *data, u64 val)
{
	mce_reset();
	fake_panic = val;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(fake_panic_fops, fake_panic_get,
			fake_panic_set, "%llu\n");

static int __init mce_debugfs_init(void)
{
	struct dentry *dmce, *ffake_panic;

	dmce = mce_get_debugfs_dir();
	if (!dmce)
		return -ENOMEM;
	ffake_panic = debugfs_create_file("fake_panic", 0444, dmce, NULL,
					  &fake_panic_fops);
	if (!ffake_panic)
		return -ENOMEM;

	return 0;
}
late_initcall(mce_debugfs_init);
#endif
