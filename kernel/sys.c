#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/utsname.h>
#include <linux/mman.h>
#include <linux/smp_lock.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/prctl.h>
#include <linux/highuid.h>
#include <linux/fs.h>
#include <linux/perf_event.h>
#include <linux/resource.h>
#include <linux/kernel.h>
#include <linux/kexec.h>
#include <linux/workqueue.h>
#include <linux/capability.h>
#include <linux/device.h>
#include <linux/key.h>
#include <linux/times.h>
#include <linux/posix-timers.h>
#include <linux/security.h>
#include <linux/dcookies.h>
#include <linux/suspend.h>
#include <linux/tty.h>
#include <linux/signal.h>
#include <linux/cn_proc.h>
#include <linux/getcpu.h>
#include <linux/task_io_accounting_ops.h>
#include <linux/seccomp.h>
#include <linux/cpu.h>
#include <linux/ptrace.h>
#include <linux/fs_struct.h>

#include <linux/compat.h>
#include <linux/syscalls.h>
#include <linux/kprobes.h>
#include <linux/user_namespace.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/unistd.h>

#ifndef SET_UNALIGN_CTL
# define SET_UNALIGN_CTL(a,b)	(-EINVAL)
#endif
#ifndef GET_UNALIGN_CTL
# define GET_UNALIGN_CTL(a,b)	(-EINVAL)
#endif
#ifndef SET_FPEMU_CTL
# define SET_FPEMU_CTL(a,b)	(-EINVAL)
#endif
#ifndef GET_FPEMU_CTL
# define GET_FPEMU_CTL(a,b)	(-EINVAL)
#endif
#ifndef SET_FPEXC_CTL
# define SET_FPEXC_CTL(a,b)	(-EINVAL)
#endif
#ifndef GET_FPEXC_CTL
# define GET_FPEXC_CTL(a,b)	(-EINVAL)
#endif
#ifndef GET_ENDIAN
# define GET_ENDIAN(a,b)	(-EINVAL)
#endif
#ifndef SET_ENDIAN
# define SET_ENDIAN(a,b)	(-EINVAL)
#endif
#ifndef GET_TSC_CTL
# define GET_TSC_CTL(a)		(-EINVAL)
#endif
#ifndef SET_TSC_CTL
# define SET_TSC_CTL(a)		(-EINVAL)
#endif

int overflowuid = DEFAULT_OVERFLOWUID;
int overflowgid = DEFAULT_OVERFLOWGID;

#ifdef CONFIG_UID16
EXPORT_SYMBOL(overflowuid);
EXPORT_SYMBOL(overflowgid);
#endif

int fs_overflowuid = DEFAULT_FS_OVERFLOWUID;
int fs_overflowgid = DEFAULT_FS_OVERFLOWUID;

EXPORT_SYMBOL(fs_overflowuid);
EXPORT_SYMBOL(fs_overflowgid);

int C_A_D = 1;
struct pid *cad_pid;
EXPORT_SYMBOL(cad_pid);

void (*pm_power_off_prepare)(void);

static int set_one_prio(struct task_struct *p, int niceval, int error)
{
	const struct cred *cred = current_cred(), *pcred = __task_cred(p);
	int no_nice;

	if (pcred->uid  != cred->euid &&
	    pcred->euid != cred->euid && !capable(CAP_SYS_NICE)) {
		error = -EPERM;
		goto out;
	}
	if (niceval < task_nice(p) && !can_nice(p, niceval)) {
		error = -EACCES;
		goto out;
	}
	no_nice = security_task_setnice(p, niceval);
	if (no_nice) {
		error = no_nice;
		goto out;
	}
	if (error == -ESRCH)
		error = 0;
	set_user_nice(p, niceval);
out:
	return error;
}

SYSCALL_DEFINE3(setpriority, int, which, int, who, int, niceval)
{
	struct task_struct *g, *p;
	struct user_struct *user;
	const struct cred *cred = current_cred();
	int error = -EINVAL;
	struct pid *pgrp;

	if (which > PRIO_USER || which < PRIO_PROCESS)
		goto out;

	error = -ESRCH;
	if (niceval < -20)
		niceval = -20;
	if (niceval > 19)
		niceval = 19;

	read_lock(&tasklist_lock);
	switch (which) {
		case PRIO_PROCESS:
			if (who)
				p = find_task_by_vpid(who);
			else
				p = current;
			if (p)
				error = set_one_prio(p, niceval, error);
			break;
		case PRIO_PGRP:
			if (who)
				pgrp = find_vpid(who);
			else
				pgrp = task_pgrp(current);
			do_each_pid_thread(pgrp, PIDTYPE_PGID, p) {
				error = set_one_prio(p, niceval, error);
			} while_each_pid_thread(pgrp, PIDTYPE_PGID, p);
			break;
		case PRIO_USER:
			user = (struct user_struct *) cred->user;
			if (!who)
				who = cred->uid;
			else if ((who != cred->uid) &&
				 !(user = find_user(who)))
				goto out_unlock;	 

			do_each_thread(g, p)
				if (__task_cred(p)->uid == who)
					error = set_one_prio(p, niceval, error);
			while_each_thread(g, p);
			if (who != cred->uid)
				free_uid(user);		 
			break;
	}
out_unlock:
	read_unlock(&tasklist_lock);
out:
	return error;
}

SYSCALL_DEFINE2(getpriority, int, which, int, who)
{
	struct task_struct *g, *p;
	struct user_struct *user;
	const struct cred *cred = current_cred();
	long niceval, retval = -ESRCH;
	struct pid *pgrp;

	if (which > PRIO_USER || which < PRIO_PROCESS)
		return -EINVAL;

	read_lock(&tasklist_lock);
	switch (which) {
		case PRIO_PROCESS:
			if (who)
				p = find_task_by_vpid(who);
			else
				p = current;
			if (p) {
				niceval = 20 - task_nice(p);
				if (niceval > retval)
					retval = niceval;
			}
			break;
		case PRIO_PGRP:
			if (who)
				pgrp = find_vpid(who);
			else
				pgrp = task_pgrp(current);
			do_each_pid_thread(pgrp, PIDTYPE_PGID, p) {
				niceval = 20 - task_nice(p);
				if (niceval > retval)
					retval = niceval;
			} while_each_pid_thread(pgrp, PIDTYPE_PGID, p);
			break;
		case PRIO_USER:
			user = (struct user_struct *) cred->user;
			if (!who)
				who = cred->uid;
			else if ((who != cred->uid) &&
				 !(user = find_user(who)))
				goto out_unlock;	 

			do_each_thread(g, p)
				if (__task_cred(p)->uid == who) {
					niceval = 20 - task_nice(p);
					if (niceval > retval)
						retval = niceval;
				}
			while_each_thread(g, p);
			if (who != cred->uid)
				free_uid(user);		 
			break;
	}
out_unlock:
	read_unlock(&tasklist_lock);

	return retval;
}

void emergency_restart(void)
{
	machine_emergency_restart();
}
EXPORT_SYMBOL_GPL(emergency_restart);

void kernel_restart_prepare(char *cmd)
{
	blocking_notifier_call_chain(&reboot_notifier_list, SYS_RESTART, cmd);
	system_state = SYSTEM_RESTART;
	device_shutdown();
	sysdev_shutdown();
}

void kernel_restart(char *cmd)
{
	kernel_restart_prepare(cmd);
	if (!cmd)
		printk(KERN_EMERG "Restarting system.\n");
	else
		printk(KERN_EMERG "Restarting system with command '%s'.\n", cmd);
	machine_restart(cmd);
}
EXPORT_SYMBOL_GPL(kernel_restart);

#ifdef MY_ABC_HERE 
extern void scsi_host_poweroff_all(void);
#endif
static void kernel_shutdown_prepare(enum system_states state)
{
	blocking_notifier_call_chain(&reboot_notifier_list,
		(state == SYSTEM_HALT)?SYS_HALT:SYS_POWER_OFF, NULL);
	system_state = state;
	device_shutdown();
#ifdef MY_ABC_HERE
	scsi_host_poweroff_all();
#endif
}
 
void kernel_halt(void)
{
	kernel_shutdown_prepare(SYSTEM_HALT);
	sysdev_shutdown();
	printk(KERN_EMERG "System halted.\n");
	machine_halt();
}

EXPORT_SYMBOL_GPL(kernel_halt);

void kernel_power_off(void)
{
#ifdef SYNO_X86_AUTO_POWER_ON 
	extern int syno_schedule_power_on_prepare(void);
#endif
	kernel_shutdown_prepare(SYSTEM_POWER_OFF);
#ifdef SYNO_X86_AUTO_POWER_ON 
	syno_schedule_power_on_prepare();
#endif
	if (pm_power_off_prepare)
		pm_power_off_prepare();
#ifdef CONFIG_SYNO_QORIQ_NO_DISABLE_NON_BOOT_CPU
#else
	disable_nonboot_cpus();
#endif
	sysdev_shutdown();
	printk(KERN_EMERG "Power down.\n");
	machine_power_off();
}
EXPORT_SYMBOL_GPL(kernel_power_off);

#ifdef SYNO_X86_MICROP_CTRL
#ifdef CONFIG_SYNO_CEDARVIEW
#define UART_PORT1_IOBASE   0x2F8
#else
#define UART_PORT1_IOBASE   0x3F8
#endif
#define UART_CMD_PREFIX 45  
#define UART_CMD_REBOOT 67  
#define UART_CMD_POWEROFF   49  
 
#define UART_TX     0    
#define UART_IER    1    
#define UART_IER_THRI       0x02  
#define UART_START_TX   UART_IER_THRI
#define UART_STOP_TX    0x0
#endif

SYSCALL_DEFINE4(reboot, int, magic1, int, magic2, unsigned int, cmd,
		void __user *, arg)
{
	char buffer[256];
	int ret = 0;

	if (!capable(CAP_SYS_BOOT))
		return -EPERM;

	if (magic1 != LINUX_REBOOT_MAGIC1 ||
	    (magic2 != LINUX_REBOOT_MAGIC2 &&
	                magic2 != LINUX_REBOOT_MAGIC2A &&
			magic2 != LINUX_REBOOT_MAGIC2B &&
	                magic2 != LINUX_REBOOT_MAGIC2C))
		return -EINVAL;

	if ((cmd == LINUX_REBOOT_CMD_POWER_OFF) && !pm_power_off)
		cmd = LINUX_REBOOT_CMD_HALT;

	lock_kernel();
	switch (cmd) {
	case LINUX_REBOOT_CMD_RESTART:
#ifdef SYNO_X86_MICROP_CTRL
        outb(UART_START_TX, UART_PORT1_IOBASE + UART_IER);
        outb(UART_CMD_PREFIX, UART_PORT1_IOBASE + UART_TX);
        outb(UART_CMD_REBOOT, UART_PORT1_IOBASE + UART_TX);
        outb(13, UART_PORT1_IOBASE + UART_TX);
        outb(10, UART_PORT1_IOBASE + UART_TX);
        outb(UART_STOP_TX, UART_PORT1_IOBASE + UART_IER);
#endif
		kernel_restart(NULL);
		break;

	case LINUX_REBOOT_CMD_CAD_ON:
		C_A_D = 1;
		break;

	case LINUX_REBOOT_CMD_CAD_OFF:
		C_A_D = 0;
		break;

	case LINUX_REBOOT_CMD_HALT:
		kernel_halt();
		unlock_kernel();
		do_exit(0);
		panic("cannot halt");

	case LINUX_REBOOT_CMD_POWER_OFF:
#ifdef SYNO_X86_MICROP_CTRL
        outb(UART_START_TX, UART_PORT1_IOBASE + UART_IER);
        outb(UART_CMD_PREFIX, UART_PORT1_IOBASE + UART_TX);
        outb(UART_CMD_POWEROFF, UART_PORT1_IOBASE + UART_TX);
        outb(13, UART_PORT1_IOBASE + UART_TX);
        outb(10, UART_PORT1_IOBASE + UART_TX);
        outb(UART_STOP_TX, UART_PORT1_IOBASE + UART_IER);
#endif
		kernel_power_off();
		unlock_kernel();
		do_exit(0);
		break;

	case LINUX_REBOOT_CMD_RESTART2:
		if (strncpy_from_user(&buffer[0], arg, sizeof(buffer) - 1) < 0) {
			unlock_kernel();
			return -EFAULT;
		}
		buffer[sizeof(buffer) - 1] = '\0';

		kernel_restart(buffer);
		break;

#ifdef CONFIG_KEXEC
	case LINUX_REBOOT_CMD_KEXEC:
		ret = kernel_kexec();
		break;
#endif

#ifdef CONFIG_HIBERNATION
	case LINUX_REBOOT_CMD_SW_SUSPEND:
		ret = hibernate();
		break;
#endif

	default:
		ret = -EINVAL;
		break;
	}
	unlock_kernel();
	return ret;
}

static void deferred_cad(struct work_struct *dummy)
{
	kernel_restart(NULL);
}

void ctrl_alt_del(void)
{
	static DECLARE_WORK(cad_work, deferred_cad);

	if (C_A_D)
		schedule_work(&cad_work);
	else
		kill_cad_pid(SIGINT, 1);
}
	
SYSCALL_DEFINE2(setregid, gid_t, rgid, gid_t, egid)
{
	const struct cred *old;
	struct cred *new;
	int retval;

	new = prepare_creds();
	if (!new)
		return -ENOMEM;
	old = current_cred();

	retval = security_task_setgid(rgid, egid, (gid_t)-1, LSM_SETID_RE);
	if (retval)
		goto error;

	retval = -EPERM;
	if (rgid != (gid_t) -1) {
		if (old->gid == rgid ||
		    old->egid == rgid ||
		    capable(CAP_SETGID))
			new->gid = rgid;
		else
			goto error;
	}
	if (egid != (gid_t) -1) {
		if (old->gid == egid ||
		    old->egid == egid ||
		    old->sgid == egid ||
		    capable(CAP_SETGID))
			new->egid = egid;
		else
			goto error;
	}

	if (rgid != (gid_t) -1 ||
	    (egid != (gid_t) -1 && egid != old->gid))
		new->sgid = new->egid;
	new->fsgid = new->egid;

	return commit_creds(new);

error:
	abort_creds(new);
	return retval;
}

SYSCALL_DEFINE1(setgid, gid_t, gid)
{
	const struct cred *old;
	struct cred *new;
	int retval;

	new = prepare_creds();
	if (!new)
		return -ENOMEM;
	old = current_cred();

	retval = security_task_setgid(gid, (gid_t)-1, (gid_t)-1, LSM_SETID_ID);
	if (retval)
		goto error;

	retval = -EPERM;
	if (capable(CAP_SETGID))
		new->gid = new->egid = new->sgid = new->fsgid = gid;
	else if (gid == old->gid || gid == old->sgid)
		new->egid = new->fsgid = gid;
	else
		goto error;

	return commit_creds(new);

error:
	abort_creds(new);
	return retval;
}

static int set_user(struct cred *new)
{
	struct user_struct *new_user;

	new_user = alloc_uid(current_user_ns(), new->uid);
	if (!new_user)
		return -EAGAIN;

	if (!task_can_switch_user(new_user, current)) {
		free_uid(new_user);
		return -EINVAL;
	}

	if (atomic_read(&new_user->processes) >=
				current->signal->rlim[RLIMIT_NPROC].rlim_cur &&
			new_user != INIT_USER) {
		free_uid(new_user);
		return -EAGAIN;
	}

	free_uid(new->user);
	new->user = new_user;
	return 0;
}

SYSCALL_DEFINE2(setreuid, uid_t, ruid, uid_t, euid)
{
	const struct cred *old;
	struct cred *new;
	int retval;

	new = prepare_creds();
	if (!new)
		return -ENOMEM;
	old = current_cred();

	retval = security_task_setuid(ruid, euid, (uid_t)-1, LSM_SETID_RE);
	if (retval)
		goto error;

	retval = -EPERM;
	if (ruid != (uid_t) -1) {
		new->uid = ruid;
		if (old->uid != ruid &&
		    old->euid != ruid &&
		    !capable(CAP_SETUID))
			goto error;
	}

	if (euid != (uid_t) -1) {
		new->euid = euid;
		if (old->uid != euid &&
		    old->euid != euid &&
		    old->suid != euid &&
		    !capable(CAP_SETUID))
			goto error;
	}

	if (new->uid != old->uid) {
		retval = set_user(new);
		if (retval < 0)
			goto error;
	}
	if (ruid != (uid_t) -1 ||
	    (euid != (uid_t) -1 && euid != old->uid))
		new->suid = new->euid;
	new->fsuid = new->euid;

	retval = security_task_fix_setuid(new, old, LSM_SETID_RE);
	if (retval < 0)
		goto error;

	return commit_creds(new);

error:
	abort_creds(new);
	return retval;
}
		
SYSCALL_DEFINE1(setuid, uid_t, uid)
{
	const struct cred *old;
	struct cred *new;
	int retval;

	new = prepare_creds();
	if (!new)
		return -ENOMEM;
	old = current_cred();

	retval = security_task_setuid(uid, (uid_t)-1, (uid_t)-1, LSM_SETID_ID);
	if (retval)
		goto error;

	retval = -EPERM;
	if (capable(CAP_SETUID)) {
		new->suid = new->uid = uid;
		if (uid != old->uid) {
			retval = set_user(new);
			if (retval < 0)
				goto error;
		}
	} else if (uid != old->uid && uid != new->suid) {
		goto error;
	}

	new->fsuid = new->euid = uid;

	retval = security_task_fix_setuid(new, old, LSM_SETID_ID);
	if (retval < 0)
		goto error;

	return commit_creds(new);

error:
	abort_creds(new);
	return retval;
}

SYSCALL_DEFINE3(setresuid, uid_t, ruid, uid_t, euid, uid_t, suid)
{
	const struct cred *old;
	struct cred *new;
	int retval;

	new = prepare_creds();
	if (!new)
		return -ENOMEM;

	retval = security_task_setuid(ruid, euid, suid, LSM_SETID_RES);
	if (retval)
		goto error;
	old = current_cred();

	retval = -EPERM;
	if (!capable(CAP_SETUID)) {
		if (ruid != (uid_t) -1 && ruid != old->uid &&
		    ruid != old->euid  && ruid != old->suid)
			goto error;
		if (euid != (uid_t) -1 && euid != old->uid &&
		    euid != old->euid  && euid != old->suid)
			goto error;
		if (suid != (uid_t) -1 && suid != old->uid &&
		    suid != old->euid  && suid != old->suid)
			goto error;
	}

	if (ruid != (uid_t) -1) {
		new->uid = ruid;
		if (ruid != old->uid) {
			retval = set_user(new);
			if (retval < 0)
				goto error;
		}
	}
	if (euid != (uid_t) -1)
		new->euid = euid;
	if (suid != (uid_t) -1)
		new->suid = suid;
	new->fsuid = new->euid;

	retval = security_task_fix_setuid(new, old, LSM_SETID_RES);
	if (retval < 0)
		goto error;

	return commit_creds(new);

error:
	abort_creds(new);
	return retval;
}

SYSCALL_DEFINE3(getresuid, uid_t __user *, ruid, uid_t __user *, euid, uid_t __user *, suid)
{
	const struct cred *cred = current_cred();
	int retval;

	if (!(retval   = put_user(cred->uid,  ruid)) &&
	    !(retval   = put_user(cred->euid, euid)))
		retval = put_user(cred->suid, suid);

	return retval;
}

SYSCALL_DEFINE3(setresgid, gid_t, rgid, gid_t, egid, gid_t, sgid)
{
	const struct cred *old;
	struct cred *new;
	int retval;

	new = prepare_creds();
	if (!new)
		return -ENOMEM;
	old = current_cred();

	retval = security_task_setgid(rgid, egid, sgid, LSM_SETID_RES);
	if (retval)
		goto error;

	retval = -EPERM;
	if (!capable(CAP_SETGID)) {
		if (rgid != (gid_t) -1 && rgid != old->gid &&
		    rgid != old->egid  && rgid != old->sgid)
			goto error;
		if (egid != (gid_t) -1 && egid != old->gid &&
		    egid != old->egid  && egid != old->sgid)
			goto error;
		if (sgid != (gid_t) -1 && sgid != old->gid &&
		    sgid != old->egid  && sgid != old->sgid)
			goto error;
	}

	if (rgid != (gid_t) -1)
		new->gid = rgid;
	if (egid != (gid_t) -1)
		new->egid = egid;
	if (sgid != (gid_t) -1)
		new->sgid = sgid;
	new->fsgid = new->egid;

	return commit_creds(new);

error:
	abort_creds(new);
	return retval;
}

SYSCALL_DEFINE3(getresgid, gid_t __user *, rgid, gid_t __user *, egid, gid_t __user *, sgid)
{
	const struct cred *cred = current_cred();
	int retval;

	if (!(retval   = put_user(cred->gid,  rgid)) &&
	    !(retval   = put_user(cred->egid, egid)))
		retval = put_user(cred->sgid, sgid);

	return retval;
}

SYSCALL_DEFINE1(setfsuid, uid_t, uid)
{
	const struct cred *old;
	struct cred *new;
	uid_t old_fsuid;

	new = prepare_creds();
	if (!new)
		return current_fsuid();
	old = current_cred();
	old_fsuid = old->fsuid;

	if (security_task_setuid(uid, (uid_t)-1, (uid_t)-1, LSM_SETID_FS) < 0)
		goto error;

	if (uid == old->uid  || uid == old->euid  ||
	    uid == old->suid || uid == old->fsuid ||
	    capable(CAP_SETUID)) {
		if (uid != old_fsuid) {
			new->fsuid = uid;
			if (security_task_fix_setuid(new, old, LSM_SETID_FS) == 0)
				goto change_okay;
		}
	}

error:
	abort_creds(new);
	return old_fsuid;

change_okay:
	commit_creds(new);
	return old_fsuid;
}

SYSCALL_DEFINE1(setfsgid, gid_t, gid)
{
	const struct cred *old;
	struct cred *new;
	gid_t old_fsgid;

	new = prepare_creds();
	if (!new)
		return current_fsgid();
	old = current_cred();
	old_fsgid = old->fsgid;

	if (security_task_setgid(gid, (gid_t)-1, (gid_t)-1, LSM_SETID_FS))
		goto error;

	if (gid == old->gid  || gid == old->egid  ||
	    gid == old->sgid || gid == old->fsgid ||
	    capable(CAP_SETGID)) {
		if (gid != old_fsgid) {
			new->fsgid = gid;
			goto change_okay;
		}
	}

error:
	abort_creds(new);
	return old_fsgid;

change_okay:
	commit_creds(new);
	return old_fsgid;
}

void do_sys_times(struct tms *tms)
{
	struct task_cputime cputime;
	cputime_t cutime, cstime;

	thread_group_cputime(current, &cputime);
	spin_lock_irq(&current->sighand->siglock);
	cutime = current->signal->cutime;
	cstime = current->signal->cstime;
	spin_unlock_irq(&current->sighand->siglock);
	tms->tms_utime = cputime_to_clock_t(cputime.utime);
	tms->tms_stime = cputime_to_clock_t(cputime.stime);
	tms->tms_cutime = cputime_to_clock_t(cutime);
	tms->tms_cstime = cputime_to_clock_t(cstime);
}

SYSCALL_DEFINE1(times, struct tms __user *, tbuf)
{
	if (tbuf) {
		struct tms tmp;

		do_sys_times(&tmp);
		if (copy_to_user(tbuf, &tmp, sizeof(struct tms)))
			return -EFAULT;
	}
	force_successful_syscall_return();
	return (long) jiffies_64_to_clock_t(get_jiffies_64());
}

SYSCALL_DEFINE2(setpgid, pid_t, pid, pid_t, pgid)
{
	struct task_struct *p;
	struct task_struct *group_leader = current->group_leader;
	struct pid *pgrp;
	int err;

	if (!pid)
		pid = task_pid_vnr(group_leader);
	if (!pgid)
		pgid = pid;
	if (pgid < 0)
		return -EINVAL;

	write_lock_irq(&tasklist_lock);

	err = -ESRCH;
	p = find_task_by_vpid(pid);
	if (!p)
		goto out;

	err = -EINVAL;
	if (!thread_group_leader(p))
		goto out;

	if (same_thread_group(p->real_parent, group_leader)) {
		err = -EPERM;
		if (task_session(p) != task_session(group_leader))
			goto out;
		err = -EACCES;
		if (p->did_exec)
			goto out;
	} else {
		err = -ESRCH;
		if (p != group_leader)
			goto out;
	}

	err = -EPERM;
	if (p->signal->leader)
		goto out;

	pgrp = task_pid(p);
	if (pgid != pid) {
		struct task_struct *g;

		pgrp = find_vpid(pgid);
		g = pid_task(pgrp, PIDTYPE_PGID);
		if (!g || task_session(g) != task_session(group_leader))
			goto out;
	}

	err = security_task_setpgid(p, pgid);
	if (err)
		goto out;

	if (task_pgrp(p) != pgrp)
		change_pid(p, PIDTYPE_PGID, pgrp);

	err = 0;
out:
	 
	write_unlock_irq(&tasklist_lock);
	return err;
}

SYSCALL_DEFINE1(getpgid, pid_t, pid)
{
	struct task_struct *p;
	struct pid *grp;
	int retval;

	rcu_read_lock();
	if (!pid)
		grp = task_pgrp(current);
	else {
		retval = -ESRCH;
		p = find_task_by_vpid(pid);
		if (!p)
			goto out;
		grp = task_pgrp(p);
		if (!grp)
			goto out;

		retval = security_task_getpgid(p);
		if (retval)
			goto out;
	}
	retval = pid_vnr(grp);
out:
	rcu_read_unlock();
	return retval;
}

#ifdef __ARCH_WANT_SYS_GETPGRP

SYSCALL_DEFINE0(getpgrp)
{
	return sys_getpgid(0);
}

#endif

SYSCALL_DEFINE1(getsid, pid_t, pid)
{
	struct task_struct *p;
	struct pid *sid;
	int retval;

	rcu_read_lock();
	if (!pid)
		sid = task_session(current);
	else {
		retval = -ESRCH;
		p = find_task_by_vpid(pid);
		if (!p)
			goto out;
		sid = task_session(p);
		if (!sid)
			goto out;

		retval = security_task_getsid(p);
		if (retval)
			goto out;
	}
	retval = pid_vnr(sid);
out:
	rcu_read_unlock();
	return retval;
}

SYSCALL_DEFINE0(setsid)
{
	struct task_struct *group_leader = current->group_leader;
	struct pid *sid = task_pid(group_leader);
	pid_t session = pid_vnr(sid);
	int err = -EPERM;

	write_lock_irq(&tasklist_lock);
	 
	if (group_leader->signal->leader)
		goto out;

	if (pid_task(sid, PIDTYPE_PGID))
		goto out;

	group_leader->signal->leader = 1;
	__set_special_pids(sid);

	proc_clear_tty(group_leader);

	err = session;
out:
	write_unlock_irq(&tasklist_lock);
	if (err > 0)
		proc_sid_connector(group_leader);
	return err;
}

DECLARE_RWSEM(uts_sem);

SYSCALL_DEFINE1(newuname, struct new_utsname __user *, name)
{
	int errno = 0;

	down_read(&uts_sem);
	if (copy_to_user(name, utsname(), sizeof *name))
		errno = -EFAULT;
	up_read(&uts_sem);
	return errno;
}

SYSCALL_DEFINE2(sethostname, char __user *, name, int, len)
{
	int errno;
	char tmp[__NEW_UTS_LEN];

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	if (len < 0 || len > __NEW_UTS_LEN)
		return -EINVAL;
	down_write(&uts_sem);
	errno = -EFAULT;
	if (!copy_from_user(tmp, name, len)) {
		struct new_utsname *u = utsname();

		memcpy(u->nodename, tmp, len);
		memset(u->nodename + len, 0, sizeof(u->nodename) - len);
		errno = 0;
	}
	up_write(&uts_sem);
	return errno;
}

#ifdef __ARCH_WANT_SYS_GETHOSTNAME

SYSCALL_DEFINE2(gethostname, char __user *, name, int, len)
{
	int i, errno;
	struct new_utsname *u;

	if (len < 0)
		return -EINVAL;
	down_read(&uts_sem);
	u = utsname();
	i = 1 + strlen(u->nodename);
	if (i > len)
		i = len;
	errno = 0;
	if (copy_to_user(name, u->nodename, i))
		errno = -EFAULT;
	up_read(&uts_sem);
	return errno;
}

#endif

SYSCALL_DEFINE2(setdomainname, char __user *, name, int, len)
{
	int errno;
	char tmp[__NEW_UTS_LEN];

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	if (len < 0 || len > __NEW_UTS_LEN)
		return -EINVAL;

	down_write(&uts_sem);
	errno = -EFAULT;
	if (!copy_from_user(tmp, name, len)) {
		struct new_utsname *u = utsname();

		memcpy(u->domainname, tmp, len);
		memset(u->domainname + len, 0, sizeof(u->domainname) - len);
		errno = 0;
	}
	up_write(&uts_sem);
	return errno;
}

SYSCALL_DEFINE2(getrlimit, unsigned int, resource, struct rlimit __user *, rlim)
{
	if (resource >= RLIM_NLIMITS)
		return -EINVAL;
	else {
		struct rlimit value;
		task_lock(current->group_leader);
		value = current->signal->rlim[resource];
		task_unlock(current->group_leader);
		return copy_to_user(rlim, &value, sizeof(*rlim)) ? -EFAULT : 0;
	}
}

#ifdef __ARCH_WANT_SYS_OLD_GETRLIMIT

SYSCALL_DEFINE2(old_getrlimit, unsigned int, resource,
		struct rlimit __user *, rlim)
{
	struct rlimit x;
	if (resource >= RLIM_NLIMITS)
		return -EINVAL;

	task_lock(current->group_leader);
	x = current->signal->rlim[resource];
	task_unlock(current->group_leader);
	if (x.rlim_cur > 0x7FFFFFFF)
		x.rlim_cur = 0x7FFFFFFF;
	if (x.rlim_max > 0x7FFFFFFF)
		x.rlim_max = 0x7FFFFFFF;
	return copy_to_user(rlim, &x, sizeof(x))?-EFAULT:0;
}

#endif

SYSCALL_DEFINE2(setrlimit, unsigned int, resource, struct rlimit __user *, rlim)
{
	struct rlimit new_rlim, *old_rlim;
	int retval;

	if (resource >= RLIM_NLIMITS)
		return -EINVAL;
	if (copy_from_user(&new_rlim, rlim, sizeof(*rlim)))
		return -EFAULT;
	if (new_rlim.rlim_cur > new_rlim.rlim_max)
		return -EINVAL;
	old_rlim = current->signal->rlim + resource;
	if ((new_rlim.rlim_max > old_rlim->rlim_max) &&
	    !capable(CAP_SYS_RESOURCE))
		return -EPERM;
	if (resource == RLIMIT_NOFILE && new_rlim.rlim_max > sysctl_nr_open)
		return -EPERM;

	retval = security_task_setrlimit(resource, &new_rlim);
	if (retval)
		return retval;

	if (resource == RLIMIT_CPU && new_rlim.rlim_cur == 0) {
		 
		new_rlim.rlim_cur = 1;
	}

	task_lock(current->group_leader);
	*old_rlim = new_rlim;
	task_unlock(current->group_leader);

	if (resource != RLIMIT_CPU)
		goto out;

	if (new_rlim.rlim_cur == RLIM_INFINITY)
		goto out;

	update_rlimit_cpu(new_rlim.rlim_cur);
out:
	return 0;
}

static void accumulate_thread_rusage(struct task_struct *t, struct rusage *r)
{
	r->ru_nvcsw += t->nvcsw;
	r->ru_nivcsw += t->nivcsw;
	r->ru_minflt += t->min_flt;
	r->ru_majflt += t->maj_flt;
	r->ru_inblock += task_io_get_inblock(t);
	r->ru_oublock += task_io_get_oublock(t);
}

static void k_getrusage(struct task_struct *p, int who, struct rusage *r)
{
	struct task_struct *t;
	unsigned long flags;
	cputime_t utime, stime;
	struct task_cputime cputime;
	unsigned long maxrss = 0;

	memset((char *) r, 0, sizeof *r);
	utime = stime = cputime_zero;

	if (who == RUSAGE_THREAD) {
		utime = task_utime(current);
		stime = task_stime(current);
		accumulate_thread_rusage(p, r);
		maxrss = p->signal->maxrss;
		goto out;
	}

	if (!lock_task_sighand(p, &flags))
		return;

	switch (who) {
		case RUSAGE_BOTH:
		case RUSAGE_CHILDREN:
			utime = p->signal->cutime;
			stime = p->signal->cstime;
			r->ru_nvcsw = p->signal->cnvcsw;
			r->ru_nivcsw = p->signal->cnivcsw;
			r->ru_minflt = p->signal->cmin_flt;
			r->ru_majflt = p->signal->cmaj_flt;
			r->ru_inblock = p->signal->cinblock;
			r->ru_oublock = p->signal->coublock;
			maxrss = p->signal->cmaxrss;

			if (who == RUSAGE_CHILDREN)
				break;

		case RUSAGE_SELF:
			thread_group_cputime(p, &cputime);
			utime = cputime_add(utime, cputime.utime);
			stime = cputime_add(stime, cputime.stime);
			r->ru_nvcsw += p->signal->nvcsw;
			r->ru_nivcsw += p->signal->nivcsw;
			r->ru_minflt += p->signal->min_flt;
			r->ru_majflt += p->signal->maj_flt;
			r->ru_inblock += p->signal->inblock;
			r->ru_oublock += p->signal->oublock;
			if (maxrss < p->signal->maxrss)
				maxrss = p->signal->maxrss;
			t = p;
			do {
				accumulate_thread_rusage(t, r);
				t = next_thread(t);
			} while (t != p);
			break;

		default:
			BUG();
	}
	unlock_task_sighand(p, &flags);

out:
	cputime_to_timeval(utime, &r->ru_utime);
	cputime_to_timeval(stime, &r->ru_stime);

	if (who != RUSAGE_CHILDREN) {
		struct mm_struct *mm = get_task_mm(p);
		if (mm) {
			setmax_mm_hiwater_rss(&maxrss, mm);
			mmput(mm);
		}
	}
	r->ru_maxrss = maxrss * (PAGE_SIZE / 1024);  
}

int getrusage(struct task_struct *p, int who, struct rusage __user *ru)
{
	struct rusage r;
	k_getrusage(p, who, &r);
	return copy_to_user(ru, &r, sizeof(r)) ? -EFAULT : 0;
}

SYSCALL_DEFINE2(getrusage, int, who, struct rusage __user *, ru)
{
	if (who != RUSAGE_SELF && who != RUSAGE_CHILDREN &&
	    who != RUSAGE_THREAD)
		return -EINVAL;
	return getrusage(current, who, ru);
}

SYSCALL_DEFINE1(umask, int, mask)
{
	mask = xchg(&current->fs->umask, mask & S_IRWXUGO);
	return mask;
}

SYSCALL_DEFINE5(prctl, int, option, unsigned long, arg2, unsigned long, arg3,
		unsigned long, arg4, unsigned long, arg5)
{
	struct task_struct *me = current;
	unsigned char comm[sizeof(me->comm)];
	long error;

	error = security_task_prctl(option, arg2, arg3, arg4, arg5);
	if (error != -ENOSYS)
		return error;

	error = 0;
	switch (option) {
		case PR_SET_PDEATHSIG:
			if (!valid_signal(arg2)) {
				error = -EINVAL;
				break;
			}
			me->pdeath_signal = arg2;
			error = 0;
			break;
		case PR_GET_PDEATHSIG:
			error = put_user(me->pdeath_signal, (int __user *)arg2);
			break;
		case PR_GET_DUMPABLE:
			error = get_dumpable(me->mm);
			break;
		case PR_SET_DUMPABLE:
			if (arg2 < 0 || arg2 > 1) {
				error = -EINVAL;
				break;
			}
			set_dumpable(me->mm, arg2);
			error = 0;
			break;

		case PR_SET_UNALIGN:
			error = SET_UNALIGN_CTL(me, arg2);
			break;
		case PR_GET_UNALIGN:
			error = GET_UNALIGN_CTL(me, arg2);
			break;
		case PR_SET_FPEMU:
			error = SET_FPEMU_CTL(me, arg2);
			break;
		case PR_GET_FPEMU:
			error = GET_FPEMU_CTL(me, arg2);
			break;
		case PR_SET_FPEXC:
			error = SET_FPEXC_CTL(me, arg2);
			break;
		case PR_GET_FPEXC:
			error = GET_FPEXC_CTL(me, arg2);
			break;
		case PR_GET_TIMING:
			error = PR_TIMING_STATISTICAL;
			break;
		case PR_SET_TIMING:
			if (arg2 != PR_TIMING_STATISTICAL)
				error = -EINVAL;
			else
				error = 0;
			break;

		case PR_SET_NAME:
			comm[sizeof(me->comm)-1] = 0;
			if (strncpy_from_user(comm, (char __user *)arg2,
					      sizeof(me->comm) - 1) < 0)
				return -EFAULT;
			set_task_comm(me, comm);
			return 0;
		case PR_GET_NAME:
			get_task_comm(comm, me);
			if (copy_to_user((char __user *)arg2, comm,
					 sizeof(comm)))
				return -EFAULT;
			return 0;
		case PR_GET_ENDIAN:
			error = GET_ENDIAN(me, arg2);
			break;
		case PR_SET_ENDIAN:
			error = SET_ENDIAN(me, arg2);
			break;

		case PR_GET_SECCOMP:
			error = prctl_get_seccomp();
			break;
		case PR_SET_SECCOMP:
			error = prctl_set_seccomp(arg2);
			break;
		case PR_GET_TSC:
			error = GET_TSC_CTL(arg2);
			break;
		case PR_SET_TSC:
			error = SET_TSC_CTL(arg2);
			break;
		case PR_TASK_PERF_EVENTS_DISABLE:
			error = perf_event_task_disable();
			break;
		case PR_TASK_PERF_EVENTS_ENABLE:
			error = perf_event_task_enable();
			break;
		case PR_GET_TIMERSLACK:
			error = current->timer_slack_ns;
			break;
		case PR_SET_TIMERSLACK:
			if (arg2 <= 0)
				current->timer_slack_ns =
					current->default_timer_slack_ns;
			else
				current->timer_slack_ns = arg2;
			error = 0;
			break;
		case PR_MCE_KILL:
			if (arg4 | arg5)
				return -EINVAL;
			switch (arg2) {
			case PR_MCE_KILL_CLEAR:
				if (arg3 != 0)
					return -EINVAL;
				current->flags &= ~PF_MCE_PROCESS;
				break;
			case PR_MCE_KILL_SET:
				current->flags |= PF_MCE_PROCESS;
				if (arg3 == PR_MCE_KILL_EARLY)
					current->flags |= PF_MCE_EARLY;
				else if (arg3 == PR_MCE_KILL_LATE)
					current->flags &= ~PF_MCE_EARLY;
				else if (arg3 == PR_MCE_KILL_DEFAULT)
					current->flags &=
						~(PF_MCE_EARLY|PF_MCE_PROCESS);
				else
					return -EINVAL;
				break;
			default:
				return -EINVAL;
			}
			error = 0;
			break;
		case PR_MCE_KILL_GET:
			if (arg2 | arg3 | arg4 | arg5)
				return -EINVAL;
			if (current->flags & PF_MCE_PROCESS)
				error = (current->flags & PF_MCE_EARLY) ?
					PR_MCE_KILL_EARLY : PR_MCE_KILL_LATE;
			else
				error = PR_MCE_KILL_DEFAULT;
			break;
		default:
			error = -EINVAL;
			break;
	}
	return error;
}

SYSCALL_DEFINE3(getcpu, unsigned __user *, cpup, unsigned __user *, nodep,
		struct getcpu_cache __user *, unused)
{
	int err = 0;
	int cpu = raw_smp_processor_id();
	if (cpup)
		err |= put_user(cpu, cpup);
	if (nodep)
		err |= put_user(cpu_to_node(cpu), nodep);
	return err ? -EFAULT : 0;
}

char poweroff_cmd[POWEROFF_CMD_PATH_LEN] = "/sbin/poweroff";

static void argv_cleanup(char **argv, char **envp)
{
	argv_free(argv);
}

int orderly_poweroff(bool force)
{
	int argc;
	char **argv = argv_split(GFP_ATOMIC, poweroff_cmd, &argc);
	static char *envp[] = {
		"HOME=/",
		"PATH=/sbin:/bin:/usr/sbin:/usr/bin",
		NULL
	};
	int ret = -ENOMEM;
	struct subprocess_info *info;

	if (argv == NULL) {
		printk(KERN_WARNING "%s failed to allocate memory for \"%s\"\n",
		       __func__, poweroff_cmd);
		goto out;
	}

	info = call_usermodehelper_setup(argv[0], argv, envp, GFP_ATOMIC);
	if (info == NULL) {
		argv_free(argv);
		goto out;
	}

	call_usermodehelper_setcleanup(info, argv_cleanup);

	ret = call_usermodehelper_exec(info, UMH_NO_WAIT);

  out:
	if (ret && force) {
		printk(KERN_WARNING "Failed to start orderly shutdown: "
		       "forcing the issue\n");

		emergency_sync();
		kernel_power_off();
	}

	return ret;
}
EXPORT_SYMBOL_GPL(orderly_poweroff);
