
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

#include <linux/clk.h>
#include <linux/c2k-devfreq.h>

//#define	C2K_DEVFREQ_DEBUG

int devfreq_counters_init(struct c2k_devfreq_data *data)
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
EXPORT_SYMBOL(devfreq_counters_init);

static int c2k_devfreq_pm_notifier_event(struct notifier_block *this,
               unsigned long event, void *ptr)
{
       struct c2k_devfreq_data *data = container_of(&this, struct c2k_devfreq_data,
                                                pm_notifier);

       switch (event) {
       case PM_SUSPEND_PREPARE:
               /* Deactivate DVFS */
               mutex_lock(&data->lock);
               data->disabled = true;
               mutex_unlock(&data->lock);
               return NOTIFY_OK;
       case PM_POST_RESTORE:
       case PM_POST_SUSPEND:
               /* Reactivate */
               mutex_lock(&data->lock);
               data->disabled = false;
               mutex_unlock(&data->lock);
               return NOTIFY_OK;
       }

       return NOTIFY_DONE;
}

/*
 * @devfreq_target  Returns desired operating frequency for the device.
 *                      Basically, get_target_freq will run
 *                      devfreq_dev_profile.get_dev_status() to get the
 *                      status of the device (load = busy_time / total_time).
 */
static int devfreq_target(struct device *dev, unsigned long *freq)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct c2k_devfreq_data *data = platform_get_drvdata(pdev);
	unsigned long old_freq = opp_get_freq(data->curr_opp);
	struct opp *new_opp;
	int err = 0;

	mutex_lock(&data->lock);

#ifdef C2K_DEVFREQ_DEBUG
	c2k_devfreq_debug ("%s:current opp <f=%lu v=%lu>\n", __func__, \
		opp_get_freq(data->curr_opp), opp_get_voltage(data->curr_opp));
#endif
	if (data->disabled)
		goto out;

	/* here...find corresponding OPP w.r.t |_*freq_| */
	new_opp = opp_find_freq_floor(dev, freq);
	if (IS_ERR(new_opp)) {
		printk("%s: Invalid frequency %lu kHz.\n",
			__func__, *freq);
		err = PTR_ERR(new_opp);
		goto out;
	}
#ifdef C2K_DEVFREQ_DEBUG
	c2k_devfreq_debug ("%s:new opp <f=%lu v=%lu>\n", __func__, \
		opp_get_freq(new_opp), opp_get_voltage(new_opp));
#endif
	data->curr_opp = new_opp; 
	*freq = opp_get_freq(new_opp);

	/* now set freq */
	if (old_freq != *freq)
	{
		if (*freq >= UINT_MAX)
			*freq = data->max_freq;
		if (*freq == 0)
			*freq = data->min_freq;

		err = data->set_freq(data, freq); /* uses clk struct */
	}
	else
		goto out;

	if (err)
		goto out;

	/* we could set voltage also */
out:
	mutex_unlock(&data->lock);
	return err;
}

/*
 * @total_time          The total time represented by this instance of
 *                      devfreq_dev_status
 * @busy_time           The time that the device was working among the
 *                      total_time.
 */
static int get_devfreq_status(struct device *dev, struct devfreq_dev_status *stat)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct c2k_devfreq_data *data = platform_get_drvdata(pdev);
	devfreq_counters *dc = data->dpc;

	stat->current_frequency = opp_get_freq(data->curr_opp);

	/* Here goes a list of mechanisms to fetch busy/total time:
	   - Performance counters
	   - Measure the time between "operation start" and "operation end" and
	   accumulate the time (getting busy time. probably by ktime?)
	   - Measure the idle time (CPUIDLE/CPUFREQ does this)
	   - Count the number of operation and calculate the operational time
	   based on the number.
	 */

	stat->busy_time = (unsigned long)module_busy_time(dc);

	stat->total_time = (unsigned long)module_busy_plus_not_busy_time(dc);

	devfreq_reset_counters(dc);

	return 0;
}

int c2k_driver_devfreq(struct device *dev, struct c2k_devfreq_data *data)
{
	struct devfreq *ldevfreq;
	struct opp *opp;
	int i, err = 0;

	BUG_ON(data==NULL);

#ifdef C2K_DEVFREQ_DEBUG
	c2k_devfreq_debug("%s: initializing for dev %s\n", __func__, dev_name(dev));
#endif
	if (!(data->vdd_int))
	{
		data->vdd_int = regulator_get(dev, "c2k_default_vcc");

		if (IS_ERR(data->vdd_int)) {
			printk ("%s: Cannot get the regulator \"vcc_c2k\"\n", __func__);
			err = PTR_ERR(data->vdd_int);
			goto err_regulator;
		}
	}

	data->dev = dev;
	mutex_init(&data->lock);

	/* OPP Entries assumed to be setup by now */
	if (data->opp_table)
	{
		for (i = 0; data->opp_table[i].idx != 0; i++) {
		#ifdef C2K_DEVFREQ_DEBUG
			c2k_devfreq_debug("%s: Adding opp entry: %ld Hz, %ld uvolt.\n",\
				__func__, data->opp_table[i].freq, data->opp_table[i].volt);
		#endif
			err = opp_add(dev, data->opp_table[i].freq, data->opp_table[i].volt);
	
			if (err) {
				printk("%s: Cannot add opp entries: err = %d\n",__func__, err);
				goto err_opp_add;
			}
		}
	}
	else
	{
		printk ("%s: Not using OPP framework.\n", __func__);
	}

	if (data->devfreq_profile->initial_freq > 0)
	{
		opp = opp_find_freq_floor(dev, &(data->devfreq_profile->initial_freq));
		if (IS_ERR(opp)) {
			printk("%s: Invalid initial frequency %lu kHz.\n",
				__func__, data->devfreq_profile->initial_freq);
			err = PTR_ERR(opp);
			goto err_opp_add;
		}
		data->curr_opp = opp; 
	}
	else
		return -EINVAL;

	if (!data->devfreq_profile->target)
		data->devfreq_profile->target = devfreq_target; /* using default */

	if (!data->devfreq_profile->get_dev_status)
		data->devfreq_profile->get_dev_status = get_devfreq_status; /* using default */

	if (!data->gov)
		data->gov = &devfreq_simple_ondemand; /* using default governer */

	dev_set_drvdata(dev, data);

	ldevfreq = devfreq_add_device(dev, data->devfreq_profile, data->gov, NULL);
	if (!ldevfreq)
	{
		printk ("%s: devfreq_add_device failed with err = %d\n", __func__, err);
		goto err_opp_add;
	}

	if (data->pm_notifier)
	{
		err = register_pm_notifier(data->pm_notifier);
		if (err) {
			printk ("%s: Failed to setup pm notifier.\n", __func__);
			goto err_notifier;
		}
	}
	else
	{
		data->pm_notifier = (struct notifier_block*)kzalloc\
				(sizeof(struct notifier_block), GFP_KERNEL);

	        if (data->pm_notifier == NULL) {
        	        printk ("%s: Cannot allocate memory for pm_notifier.\n",\
				__func__);
        	        return -ENOMEM;
        	}

		data->pm_notifier->notifier_call = c2k_devfreq_pm_notifier_event; /* default pm notifier */

		err = register_pm_notifier(data->pm_notifier);
		if (err) {
			printk ("%s: Failed to setup pm notifier.\n", __func__);
			goto err_notifier;
		}
	}

	err = devfreq_counters_init(data);

	return err;

err_notifier:
	devfreq_remove_device(ldevfreq);
err_opp_add:
	regulator_put(data->vdd_int);
err_regulator:
	kfree(data);
	return err;
}
EXPORT_SYMBOL(c2k_driver_devfreq);
