/*
 * arch/arm/plat-armada/cpufreq.c
 *
 * Clock scaling for ArmadaXP SoC
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/io.h>
#include <linux/proc_fs.h>
#include <config/mvSysHwConfig.h>
#include "mvCommon.h"
#include "mvOs.h"
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "boardEnv/mvBoardEnvLib.h"
#include "cpu/mvCpu.h"

#undef DEBUG
#ifdef DEBUG
#define DB(x)	x
#else
#define DB(x)
#endif

extern struct proc_dir_entry *mv_pm_proc_entry;

enum armadaxp_cpufreq_range {
	ARMADAXP_CPUFREQ_LOW	= 0,
	ARMADAXP_CPUFREQ_HIGH	= 1
};

static struct cpufreq_frequency_table armadaxp_freqs[] = {
	{ ARMADAXP_CPUFREQ_LOW, 0                  },
	{ ARMADAXP_CPUFREQ_HIGH, 0                  },
	{ 0, CPUFREQ_TABLE_END  }
};

static void armadaxp_set_freq_handler(void *data)
{
	u32 reg;
	u32 cpu = smp_processor_id();
	unsigned long flags;

	local_irq_save(flags);

	/* Configure the *WaitMask fields in the CPUx PM Status & Mask Register	(Set the CPUIdleMask fields) */
	reg = MV_REG_READ(PM_STATUS_AND_MASK_REG(cpu));
	/* set WaitMask fields */
	reg |= PM_STATUS_AND_MASK_CPU_IDLE_WAIT;
	/* Mask interrupts */
	reg |= PM_STATUS_AND_MASK_IRQ_MASK | PM_STATUS_AND_MASK_FIQ_MASK;
	MV_REG_WRITE(PM_STATUS_AND_MASK_REG(cpu), reg);

	/* Configure the MP_PMU: "CPU PM Control and Configuration", UnitDfsReq field to 1. */
	reg = MV_REG_READ(PM_CONTROL_AND_CONFIG_REG(cpu));
	reg |= PM_CONTROL_AND_CONFIG_DFS_REQ;
	MV_REG_WRITE(PM_CONTROL_AND_CONFIG_REG(cpu), reg);

	wfi();

	/* cancel Enable wakeup events */
	reg = MV_REG_READ(PM_STATUS_AND_MASK_REG(cpu));
	reg &= ~PM_STATUS_AND_MASK_CPU_IDLE_WAIT;
	/* UnMask interrupts */
	reg &= ~(PM_STATUS_AND_MASK_IRQ_MASK | PM_STATUS_AND_MASK_FIQ_MASK);
	MV_REG_WRITE(PM_STATUS_AND_MASK_REG(cpu), reg);

	local_irq_restore(flags);
}

/*
 * Power management function: set or unset powersave mode
 */
static inline void armadaxp_set_powersave(unsigned int cpu, u8 on)
{
	void __iomem *addr = (void __iomem *)(AXP_SW_TRIG_IRQ);
	u32 reg, fabric_div, target_div;
	unsigned long timeout;

	DB(printk(KERN_DEBUG "cpufreq: Setting PowerSaveState to %s\n", on ? "on" : "off"));

	fabric_div = ((MV_REG_READ(CPU_DIV_CLK_CTRL2_RATIO_FULL0_REG) &
		(0x3F << CPU_DIV_CLK_CTRL2_NB_RATIO_OFFS)) >> CPU_DIV_CLK_CTRL2_NB_RATIO_OFFS);

	if (on)
		target_div = fabric_div;
	else
		target_div = fabric_div/2;

	if (target_div == 0)
		target_div = 1;

	/* Configure the DEV_PMU: "PMU DFS Control %n" register, DfsRatio%n field, to the future required ratio. */
	reg = MV_REG_READ(PMU_DFS_CTRL_REG(cpu));
	reg &= ~(PMU_DFS_CTRL_RATIO_MASK << PMU_DFS_CTRL_RATIO_OFFS);
	reg |= (target_div << PMU_DFS_CTRL_RATIO_OFFS);
	MV_REG_WRITE(PMU_DFS_CTRL_REG(cpu), reg);

	reg = MV_REG_READ(PM_EVENT_STATUS_AND_MASK_REG(cpu));
	reg &= ~(1 << PM_EVENT_STATUS_AND_MASK_DFS_DONE_OFFS);
	MV_REG_WRITE(PM_EVENT_STATUS_AND_MASK_REG(cpu), reg);

	/* Unmask DFS Done interrupt */
	reg = MV_REG_READ(PM_EVENT_STATUS_AND_MASK_REG(cpu));
	reg |= (1 << PM_EVENT_STATUS_AND_MASK_DFS_DONE_MASK_OFFS);
	MV_REG_WRITE(PM_EVENT_STATUS_AND_MASK_REG(cpu), reg);

	/* Mask reset signals at frequency change */
	reg = MV_REG_READ(CPU_DIV_CLK_CTRL0_REG);
	MV_REG_WRITE(CPU_DIV_CLK_CTRL0_REG, reg | (0xFF << CPU_DIV_CLK_CTRL0_RESET_MASK_OFFS));

	/* Generate IPI CPUFREQ message to the traget cpu */
	smp_call_function_single(cpu, armadaxp_set_freq_handler, NULL, false);

	/* Wait for DFS end indication */
	timeout = jiffies + (10 * HZ);
	while (time_before(jiffies, timeout)) {
		if (MV_REG_READ(PM_EVENT_STATUS_AND_MASK_REG(cpu)) & (1 << PM_EVENT_STATUS_AND_MASK_DFS_DONE_OFFS))
			break;
		udelay(10);
	}

	/* Mask DFS Done interrupt */
	reg = MV_REG_READ(PM_EVENT_STATUS_AND_MASK_REG(cpu));
	reg &= ~(1 << PM_EVENT_STATUS_AND_MASK_DFS_DONE_MASK_OFFS);
	MV_REG_WRITE(PM_EVENT_STATUS_AND_MASK_REG(cpu), reg);

	DB(printk(KERN_DEBUG "cpufreq: CPU %d finished setting CPU %d Frequency to %d \n",
		smp_processor_id(), cpu, target_div));

}

static int armadaxp_cpufreq_verify(struct cpufreq_policy *policy)
{
	if (unlikely(!cpu_online(policy->cpu)))
		return -ENODEV;

	return cpufreq_frequency_table_verify(policy, armadaxp_freqs);
}

/*
 * Get the current frequency for a given cpu.
 */
static unsigned int armadaxp_cpufreq_get(unsigned int cpu)
{
	unsigned int freq;
	u32 cur_div_val, fabric_div;

	if (unlikely(!cpu_online(cpu)))
		return -ENODEV;

	/* To get the current frequency, we have to check if
	* the powersave mode is set. */
	cur_div_val = (((MV_REG_READ(CPU_DIV_CLK_CTRL3_RATIO_FULL1_REG) &
		(0x3F << (cpu * CPU_DIV_CLK_CTRL3_CPU_RATIO_OFFS))) >> (cpu * CPU_DIV_CLK_CTRL3_CPU_RATIO_OFFS)) & 0x3F);
	fabric_div = ((MV_REG_READ(CPU_DIV_CLK_CTRL2_RATIO_FULL0_REG) &
		(0x3F << CPU_DIV_CLK_CTRL2_NB_RATIO_OFFS)) >> CPU_DIV_CLK_CTRL2_NB_RATIO_OFFS);

	if (cur_div_val == fabric_div)
		freq = armadaxp_freqs[ARMADAXP_CPUFREQ_LOW].frequency;
	else
		freq = armadaxp_freqs[ARMADAXP_CPUFREQ_HIGH].frequency;

	DB(printk(KERN_DEBUG "armadaxp_cpufreq_get: CPU %d freq is %u MHz\n", cpu, freq));

	return freq;
}

/*
 * Set the frequency for a given cpu.
 */
static int armadaxp_cpufreq_target(struct cpufreq_policy *policy,
		unsigned int target_freq, unsigned int relation)
{
	unsigned int index;
	struct cpufreq_freqs freqs;

	if (unlikely(!cpu_online(policy->cpu)))
		return -ENODEV;

	/* Lookup the next frequency */
	if (unlikely(cpufreq_frequency_table_target(policy,
		armadaxp_freqs, target_freq, relation, &index)))
		return -EINVAL;

	freqs.old = policy->cur;
	freqs.new = armadaxp_freqs[index].frequency;
	freqs.cpu = policy->cpu;

	DB(printk(KERN_DEBUG "cpufreq: CPU %d is setting CPU %d Frequency to %u KHz\n", smp_processor_id(),
			policy->cpu, freqs.new));

	cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);

	if (freqs.new == armadaxp_freqs[ARMADAXP_CPUFREQ_LOW].frequency)
		armadaxp_set_powersave(policy->cpu, 1);
	else
		armadaxp_set_powersave(policy->cpu, 0);

	cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);

	return 0;
}

static int armadaxp_cpufreq_cpu_init(struct cpufreq_policy *policy)
{
	if (unlikely(!cpu_online(policy->cpu)))
		return -ENODEV;

	armadaxp_freqs[ARMADAXP_CPUFREQ_HIGH].frequency = mvCpuPclkGet()/1000000;
	/* policy->cpu low frequency is the DDR frequency. */
	armadaxp_freqs[ARMADAXP_CPUFREQ_LOW].frequency  = mvBoardSysClkGet()/1000000;

	DB(printk(KERN_DEBUG
			"cpufreq: High frequency: %uKHz - Low frequency: %uMHz\n",
			armadaxp_freqs[ARMADAXP_CPUFREQ_HIGH].frequency,
			armadaxp_freqs[ARMADAXP_CPUFREQ_LOW].frequency));

	policy->cpuinfo.transition_latency = 5000;
	policy->cur = armadaxp_cpufreq_get(0);
	policy->governor = CPUFREQ_DEFAULT_GOVERNOR;

	cpufreq_frequency_table_get_attr(armadaxp_freqs, policy->cpu);

	return cpufreq_frequency_table_cpuinfo(policy, armadaxp_freqs);
}

static int armadaxp_cpufreq_cpu_exit(struct cpufreq_policy *policy)
{
	cpufreq_frequency_table_put_attr(policy->cpu);
	return 0;
}

static struct freq_attr *armadaxp_freq_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,
};

static struct cpufreq_driver armadaxp_freq_driver = {
	.name           = "armadaxp_cpufreq",
	.init           = armadaxp_cpufreq_cpu_init,
	.verify         = armadaxp_cpufreq_verify,
	.exit			= armadaxp_cpufreq_cpu_exit,
	.target         = armadaxp_cpufreq_target,
	.get            = armadaxp_cpufreq_get,
	.attr           = armadaxp_freq_attr,
};

#ifdef CONFIG_MV_PMU_PROC
static int mv_cpu_freq_write(struct file *file, const char *buffer,
		unsigned long count, void *data)
{
	struct cpufreq_policy policy;
	int cpu;

	/* Reading / Writing from system controller internal registers */
	if (!strncmp (buffer, "enable", strlen("enable"))) {
		cpufreq_register_driver(&armadaxp_freq_driver);
	} else if (!strncmp (buffer, "disable", strlen("disable"))) {
		cpufreq_get_policy(&policy, smp_processor_id());
		armadaxp_cpufreq_target(&policy, armadaxp_freqs[ARMADAXP_CPUFREQ_HIGH].frequency, CPUFREQ_RELATION_H);
		cpufreq_unregister_driver(&armadaxp_freq_driver);
	} else if (!strncmp (buffer, "fast", strlen("fast"))) {
		for_each_online_cpu(cpu)
			armadaxp_set_powersave(cpu, 0);
	} else if (!strncmp (buffer, "slow", strlen("slow"))) {
		for_each_online_cpu(cpu)
			armadaxp_set_powersave(cpu, 1);
	}

	return count;
}

static int mv_cpu_freq_read(char *buffer, char **buffer_location, off_t offset,
		int buffer_length, int *zero, void *ptr)
{
	if (offset > 0)
		return 0;
	return sprintf(buffer, "enable - Enable policy->cpu-Freq framework.\n"
				"disable - Disable policy->cpu-Freq framework.\n"
				"fast - Manually set the policy->cpu to fast frequency mode (in Disable mode).\n"
				"slow - Manually set the policy->cpu to slow frequency mode (in Disable mode).\n");
}

#endif /* CONFIG_MV_PMU_PROC */

static int __init armadaxp_cpufreq_init(void)
{
#ifdef CONFIG_MV_PMU_PROC
	struct proc_dir_entry *cpu_freq_proc;
#endif /* CONFIG_MV_PMU_PROC */

	printk(KERN_INFO "cpufreq: Init ArmadaXP cpufreq driver\n");

#ifdef CONFIG_MV_PMU_PROC
	/* Create proc entry. */
	cpu_freq_proc = create_proc_entry("cpu_freq", 0666, NULL);
	cpu_freq_proc->read_proc = mv_cpu_freq_read;
	cpu_freq_proc->write_proc = mv_cpu_freq_write;
	cpu_freq_proc->nlink = 1;
#endif /* CONFIG_MV_PMU_PROC */

	return cpufreq_register_driver(&armadaxp_freq_driver);
}

static void __exit armadaxp_cpufreq_exit(void)
{
	cpufreq_unregister_driver(&armadaxp_freq_driver);
}

device_initcall(armadaxp_cpufreq_init);
