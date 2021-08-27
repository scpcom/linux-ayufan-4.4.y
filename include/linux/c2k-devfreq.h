#ifndef _C2K_DEVFREQ_H
#define _C2K_DEVFREQ_H

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/opp.h>
#include <linux/devfreq.h>

#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/jiffies.h>

#define C2K_DEVFREQ_DEBUG
#ifdef C2K_DEVFREQ_DEBUG
        #define c2k_devfreq_debug(fmt, arg...)   printk(fmt, ##arg)
#else
        #define c2k_devfreq_debug(fmt, arg...)     ;
#endif

#define C2K_DEVFREQ_USE_KTIME
typedef struct devfreq_perf_counters {
#ifdef C2K_DEVFREQ_USE_KTIME
	ktime_t fentry_time; /* ktime at the enrty of a func */
	ktime_t start_time; /* starting ktime for every busy/total \
			     load cal in the last x seconds*/
	ktime_t prev_count; /* prev ktime count */
	s64 busy_time; /* total execution ktime count in the last \
			      x seconds */
#else
	unsigned long fentry_time; /* jiffies at the enrty of a func */
	unsigned long start_time; /* starting jiffies for every busy/total \
					     load cal in the last x seconds*/
	unsigned long prev_count; /* last jiffies count */
	unsigned long busy_time; /* total execution jiffies count in the last \
				      x seconds */
#endif
}devfreq_counters;

struct c2k_devfreq_opp_table {
	unsigned int idx;
	unsigned long freq; /* MHz */
	unsigned long volt; /* uVolt */
};

struct c2k_devfreq_data{
	devfreq_counters *dpc; 
	struct device *dev; /* provided by driver */
	struct regulator *vdd_int; /* if not provided by driver, use default */
	struct opp *curr_opp;
	struct notifier_block *pm_notifier; /* if not provided by driver, use default */
	struct c2k_devfreq_opp_table *opp_table; /* provided by driver */
	struct devfreq_dev_profile *devfreq_profile; /* provided by driver */
	const struct devfreq_governor *gov; /* provided by driver */
	struct clk *clk;
	int (*set_freq)(struct c2k_devfreq_data *data, unsigned long *freq);

	int disabled;
	unsigned long max_freq;
	unsigned long min_freq;
	struct mutex lock;
};

#ifdef C2K_DEVFREQ_USE_KTIME
/* using ktime */
	#define devfreq_func_start(dc) do {\
		(dc)->fentry_time = ktime_get();\
	}while(0)

	#define devfreq_func_end(dc) do {\
		(dc)->prev_count = ktime_get();\
		(dc)->busy_time += ktime_to_ns(ktime_sub((dc)->prev_count, \
					(dc)->fentry_time));\
	}while(0)

	#define module_busy_time(dc)	((dc)->busy_time)

	#define module_busy_plus_not_busy_time(dc) \
		ktime_to_ns(ktime_sub(ktime_get(), (dc)->start_time))

	#define devfreq_reset_counters(dc) do { \
		(dc)->start_time = ktime_get();\
	}while(0)
#else
	#define devfreq_func_start(dc) \
		(dc)->fentry_time = jiffies

	#define devfreq_fun_end(dc) do { \
		(dc)->prev_count = (jiffies)-((dc)->fentry_time);\
		(dc)->busy_time += (dc)->prev_count;\
	}while(0)

	#define module_busy_time(dc) \
		jiffies_to_usecs((dc)->busy_time)

	#define module_busy_plus_not_busy_time(dc) \
		jiffies_to_usecs(((jiffies) - (dc)->start_time))

	#define devfreq_reset_counters(dc) do { \
		(dc)->fentry_time = 0UL; \
		(dc)->prev_count = 0UL; \
		(dc)->busy_time = 0UL; \
		(dc)->start_time = jiffies;\
	}while(0)
#endif

#if 0
static int devfreq_counters_init(struct c2k_devfreq_data *data)
{
	devfreq_counters **dc = &data->dpc;

	*dc = kzalloc(sizeof (devfreq_counters), GFP_KERNEL);
        if (dc == NULL) {
	        printk ("%s: Cannot allocate memory for devfreq_counters.\n"\
			, __func__);
	        return -ENOMEM;
	}

#ifdef C2K_DEVFREQ_USE_KTIME
	(*dc)->start_time = ktime_get();
#else
	(*dc)->fentry_time = 0UL;
	(*dc)->prev_count = 0UL;
	(*dc)->busy_time = 0UL;
	(*dc)->start_time = jiffies;
#endif
	return 0;
}
#endif

extern int c2k_driver_devfreq(struct device *dev, struct c2k_devfreq_data *data);

#endif
