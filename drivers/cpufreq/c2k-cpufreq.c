
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/compiler.h>
#include <linux/gfp.h>

#include <asm/system.h>

#include <mach/hardware.h>
#include <linux/netdevice.h>
#include <linux/clk.h>

#define NR_FREQS        15

//#define C2K_CPUFREQ_DEBUG
#ifdef C2K_CPUFREQ_DEBUG
	#define c2k_cpufreq_debug(fmt, arg...)   printk(fmt, ##arg)
#else
	#define c2k_cpufreq_debug(fmt, arg...)     ;
#endif

extern struct cpufreq_governor cpufreq_gov_ondemand;

static struct cpufreq_frequency_table comcerto_clk_frqs[NR_FREQS + 1];

static int comcerto_verify_speed(struct cpufreq_policy *policy)
{
	if (policy->cpu < 0)
		return -EINVAL;

	return cpufreq_frequency_table_verify(policy, comcerto_clk_frqs);
}

static unsigned int comcerto_getspeed(unsigned int cpu)
{
	struct clk *clk_arm;

	if (cpu < 0)
		return -EINVAL;

	c2k_cpufreq_debug ("%s: Get speed for cpu(%d)\n", __func__, cpu);

	clk_arm = clk_get(NULL,"arm");
	if (IS_ERR(clk_arm)) {
		pr_err("cpufreq: Unable to obtain ARMCLK: %ld\n",
				PTR_ERR(clk_arm));
		return PTR_ERR(clk_arm);
	}

	return clk_get_rate(clk_arm); /* we get freq in hz */
}

static int comcerto_set_target(struct cpufreq_policy *policy,
                             unsigned int target_freq,
                             unsigned int relation)
{
	struct cpufreq_freqs freqs;
	unsigned int index;
	struct clk *clk_arm;
	int ret;
	int j = 0;

	ret = cpufreq_frequency_table_target(policy, comcerto_clk_frqs, target_freq, relation, &index);
	if (ret < 0)
		return ret;

	if (policy->cpu >= NR_CPUS) {
		pr_debug("%s: couldn't limit to CPUs in this domain\n", \
				__func__);
		return -EAGAIN;
	}

	clk_arm = clk_get(NULL,"arm");
	if (IS_ERR(clk_arm)) {
		pr_err("%s: cpufreq: Unable to obtain ARMCLK: %ld\n", __func__,\
				PTR_ERR(clk_arm));
		return PTR_ERR(clk_arm);
	}

	freqs.old = comcerto_getspeed(policy->cpu);
	if (freqs.old == target_freq)
	{
		c2k_cpufreq_debug ("%s: old freq (%d) equals new freq(%d).\n", \
			__func__, freqs.old, target_freq);
		return 0;
	}

	freqs.new = comcerto_clk_frqs[index].frequency;

	c2k_cpufreq_debug("%s: Transition(cpu:%d) %d-%dHz\n", __func__,\
				policy->cpu, freqs.old, freqs.new);

	while (j < NR_CPUS)
	{
		if (!cpu_online(j))
		{
			j++;
			continue;
		}
		freqs.cpu = j;
		cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);
		j++;
	}
	j = 0;

	policy->cur = freqs.new;
	ret = clk_set_rate(clk_arm, freqs.new);
	if (ret < 0) {
		pr_debug("cpufreq: Failed to set rate %dHz: %d\n",
				freqs.new, ret);
		j = 0;
		goto err;
	}

	while (j < NR_CPUS)
	{
		if (!cpu_online(j))
		{
			j++;
			continue;
		}
		freqs.cpu = j;
		cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);
		j++;
	}

	c2k_cpufreq_debug("%s:Set actual frequency %luHz\n", __func__, \
			clk_get_rate(clk_arm));

	return 0;
err:
	policy->cur = freqs.old;
	freqs.new = freqs.old;
	while (j < NR_CPUS)
	{
		if (!cpu_online(j))
		{
			j++;
			continue;
		}
		freqs.cpu = j;
		cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);
		j++;
	}

        return ret;
}

static int comcerto_init_table(int cpu)
{
	int i;
	struct clk *clk_arm;
	struct clk *clk_pll;

        if (cpu != 0)
                return 0;

	clk_arm = clk_get(NULL,"arm");
	if (IS_ERR(clk_arm)) {
		pr_err("cpufreq: Unable to obtain ARMCLK: %ld\n",
				PTR_ERR(clk_arm));
		return PTR_ERR(clk_arm);
	}

	clk_pll = clk_get_parent(clk_arm);

	comcerto_clk_frqs[0].frequency = clk_get_rate(clk_arm);
	comcerto_clk_frqs[0].index = clk_get_rate(clk_pll)/clk_get_rate(clk_arm);

	c2k_cpufreq_debug("\n\n ### cpufreq Table ###\n");

	c2k_cpufreq_debug ("comcerto_clk_frqs[0].index = %d, comcerto_clk_frqs[0].frequency = %d\n",\
				comcerto_clk_frqs[0].index, comcerto_clk_frqs[0].frequency);

	for (i = 1; comcerto_clk_frqs[0].index + i <= NR_FREQS; i++) 
	{
		comcerto_clk_frqs[i].frequency = (comcerto_clk_frqs[0].frequency \
			* comcerto_clk_frqs[0].index) / (comcerto_clk_frqs[0].index + i);

		comcerto_clk_frqs[i].index = comcerto_clk_frqs[0].index + i;

		c2k_cpufreq_debug ("comcerto_clk_frqs[%d].index = %d, comcerto_clk_frqs[%d].frequency = %d\n",\
					i, comcerto_clk_frqs[i].index, i, comcerto_clk_frqs[i].frequency);
	}

	comcerto_clk_frqs[i].frequency = CPUFREQ_TABLE_END;

	return 0;
}

static int comcerto_cpu_init(struct cpufreq_policy *policy)
{
	comcerto_init_table(policy->cpu);

	cpufreq_frequency_table_cpuinfo(policy, comcerto_clk_frqs);

	cpufreq_frequency_table_get_attr(comcerto_clk_frqs, policy->cpu);

	/* PLL stabalisation time */
	policy->cpuinfo.transition_latency = 10000; /* 10us */

	/* Current Frequency */
	policy->cur = comcerto_clk_frqs[0].frequency;

	/* Governor used */
	policy->governor = CPUFREQ_DEFAULT_GOVERNOR;

	return 0;
}

static struct freq_attr *comcerto_cpufreq_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,
};

static struct cpufreq_driver comcerto_driver = {
	.name = "comcerto",
	.init = comcerto_cpu_init,
	.verify = comcerto_verify_speed,
	.target = comcerto_set_target,
	.get = comcerto_getspeed,
	.attr = comcerto_cpufreq_attr,
	.flags = CPUFREQ_STICKY,
	.owner = THIS_MODULE,
};

static int __init comcerto_cpufreq_init(void)
{
	printk ("Registering CPUFreq(%s)\n", comcerto_driver.name);

	return  cpufreq_register_driver(&comcerto_driver);
}

static void __exit comcerto_cpufreq_exit(void)
{
	cpufreq_unregister_driver(&comcerto_driver);
}

MODULE_AUTHOR ("Satendra Pratap <satendra.pratap@gmail.com>");
MODULE_DESCRIPTION ("CPUFreq driver for Mindspeed's C2000 SoC");
MODULE_LICENSE ("GPL");

late_initcall(comcerto_cpufreq_init);
module_exit(comcerto_cpufreq_exit);

