#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/cpufreq.h>
#include <linux/list.h>
#include <linux/rculist.h>
#include <linux/rcupdate.h>
#include <linux/opp.h>

struct opp {
	struct list_head node;

	bool available;
	unsigned long rate;
	unsigned long u_volt;

	struct device_opp *dev_opp;
};

struct device_opp {
	struct list_head node;

	struct device *dev;
	struct srcu_notifier_head head;
	struct list_head opp_list;
};

static LIST_HEAD(dev_opp_list);
 
static DEFINE_MUTEX(dev_opp_list_lock);

static struct device_opp *find_device_opp(struct device *dev)
{
	struct device_opp *tmp_dev_opp, *dev_opp = ERR_PTR(-ENODEV);

	if (unlikely(IS_ERR_OR_NULL(dev))) {
		pr_err("%s: Invalid parameters\n", __func__);
		return ERR_PTR(-EINVAL);
	}

	list_for_each_entry_rcu(tmp_dev_opp, &dev_opp_list, node) {
		if (tmp_dev_opp->dev == dev) {
			dev_opp = tmp_dev_opp;
			break;
		}
	}

	return dev_opp;
}

unsigned long opp_get_voltage(struct opp *opp)
{
	struct opp *tmp_opp;
	unsigned long v = 0;

	tmp_opp = rcu_dereference(opp);
	if (unlikely(IS_ERR_OR_NULL(tmp_opp)) || !tmp_opp->available)
		pr_err("%s: Invalid parameters\n", __func__);
	else
		v = tmp_opp->u_volt;

	return v;
}

unsigned long opp_get_freq(struct opp *opp)
{
	struct opp *tmp_opp;
	unsigned long f = 0;

	tmp_opp = rcu_dereference(opp);
	if (unlikely(IS_ERR_OR_NULL(tmp_opp)) || !tmp_opp->available)
		pr_err("%s: Invalid parameters\n", __func__);
	else
		f = tmp_opp->rate;

	return f;
}

int opp_get_opp_count(struct device *dev)
{
	struct device_opp *dev_opp;
	struct opp *temp_opp;
	int count = 0;

	dev_opp = find_device_opp(dev);
	if (IS_ERR(dev_opp)) {
		int r = PTR_ERR(dev_opp);
		dev_err(dev, "%s: device OPP not found (%d)\n", __func__, r);
		return r;
	}

	list_for_each_entry_rcu(temp_opp, &dev_opp->opp_list, node) {
		if (temp_opp->available)
			count++;
	}

	return count;
}

struct opp *opp_find_freq_exact(struct device *dev, unsigned long freq,
				bool available)
{
	struct device_opp *dev_opp;
	struct opp *temp_opp, *opp = ERR_PTR(-ENODEV);

	dev_opp = find_device_opp(dev);
	if (IS_ERR(dev_opp)) {
		int r = PTR_ERR(dev_opp);
		dev_err(dev, "%s: device OPP not found (%d)\n", __func__, r);
		return ERR_PTR(r);
	}

	list_for_each_entry_rcu(temp_opp, &dev_opp->opp_list, node) {
		if (temp_opp->available == available &&
				temp_opp->rate == freq) {
			opp = temp_opp;
			break;
		}
	}

	return opp;
}

struct opp *opp_find_freq_ceil(struct device *dev, unsigned long *freq)
{
	struct device_opp *dev_opp;
	struct opp *temp_opp, *opp = ERR_PTR(-ENODEV);

	if (!dev || !freq) {
		dev_err(dev, "%s: Invalid argument freq=%p\n", __func__, freq);
		return ERR_PTR(-EINVAL);
	}

	dev_opp = find_device_opp(dev);
	if (IS_ERR(dev_opp))
		return opp;

	list_for_each_entry_rcu(temp_opp, &dev_opp->opp_list, node) {
		if (temp_opp->available && temp_opp->rate >= *freq) {
			opp = temp_opp;
			*freq = opp->rate;
			break;
		}
	}

	return opp;
}


struct opp *opp_find_freq_floor(struct device *dev, unsigned long *freq)
{
	struct device_opp *dev_opp;
	struct opp *temp_opp, *opp = ERR_PTR(-ENODEV);
#if defined(CONFIG_SYNO_COMCERTO)
	int c = 0;
#endif

	if (!dev || !freq) {
		dev_err(dev, "%s: Invalid argument freq=%p\n", __func__, freq);
		return ERR_PTR(-EINVAL);
	}

	dev_opp = find_device_opp(dev);
	if (IS_ERR(dev_opp))
		return opp;

	list_for_each_entry_rcu(temp_opp, &dev_opp->opp_list, node) {
#if defined(CONFIG_SYNO_COMCERTO)
		++c;
#endif
		if (temp_opp->available) {
			 
			if (temp_opp->rate > *freq)
#if defined(CONFIG_SYNO_COMCERTO)	
			{
				if (c == 1)
					opp = temp_opp;
				break;
			}
#else
				break;
#endif
			else
				opp = temp_opp;
		}
	}
	if (!IS_ERR(opp))
		*freq = opp->rate;

	return opp;
}

int opp_add(struct device *dev, unsigned long freq, unsigned long u_volt)
{
	struct device_opp *dev_opp = NULL;
	struct opp *opp, *new_opp;
	struct list_head *head;

	new_opp = kzalloc(sizeof(struct opp), GFP_KERNEL);
	if (!new_opp) {
		dev_warn(dev, "%s: Unable to create new OPP node\n", __func__);
		return -ENOMEM;
	}

	mutex_lock(&dev_opp_list_lock);

	dev_opp = find_device_opp(dev);
	if (IS_ERR(dev_opp)) {
		 
		dev_opp = kzalloc(sizeof(struct device_opp), GFP_KERNEL);
		if (!dev_opp) {
			mutex_unlock(&dev_opp_list_lock);
			kfree(new_opp);
			dev_warn(dev,
				"%s: Unable to create device OPP structure\n",
				__func__);
			return -ENOMEM;
		}

		dev_opp->dev = dev;
		srcu_init_notifier_head(&dev_opp->head);
		INIT_LIST_HEAD(&dev_opp->opp_list);

		list_add_rcu(&dev_opp->node, &dev_opp_list);
	}

	new_opp->dev_opp = dev_opp;
	new_opp->rate = freq;
	new_opp->u_volt = u_volt;
	new_opp->available = true;

	head = &dev_opp->opp_list;
	list_for_each_entry_rcu(opp, &dev_opp->opp_list, node) {
		if (new_opp->rate < opp->rate)
			break;
		else
			head = &opp->node;
	}

	list_add_rcu(&new_opp->node, head);
	mutex_unlock(&dev_opp_list_lock);

	srcu_notifier_call_chain(&dev_opp->head, OPP_EVENT_ADD, new_opp);
	return 0;
}

static int opp_set_availability(struct device *dev, unsigned long freq,
		bool availability_req)
{
	struct device_opp *tmp_dev_opp, *dev_opp = ERR_PTR(-ENODEV);
	struct opp *new_opp, *tmp_opp, *opp = ERR_PTR(-ENODEV);
	int r = 0;

	new_opp = kmalloc(sizeof(struct opp), GFP_KERNEL);
	if (!new_opp) {
		dev_warn(dev, "%s: Unable to create OPP\n", __func__);
		return -ENOMEM;
	}

	mutex_lock(&dev_opp_list_lock);

	list_for_each_entry(tmp_dev_opp, &dev_opp_list, node) {
		if (dev == tmp_dev_opp->dev) {
			dev_opp = tmp_dev_opp;
			break;
		}
	}
	if (IS_ERR(dev_opp)) {
		r = PTR_ERR(dev_opp);
		dev_warn(dev, "%s: Device OPP not found (%d)\n", __func__, r);
		goto unlock;
	}

	list_for_each_entry(tmp_opp, &dev_opp->opp_list, node) {
		if (tmp_opp->rate == freq) {
			opp = tmp_opp;
			break;
		}
	}
	if (IS_ERR(opp)) {
		r = PTR_ERR(opp);
		goto unlock;
	}

	if (opp->available == availability_req)
		goto unlock;
	 
	*new_opp = *opp;

	new_opp->available = availability_req;

	list_replace_rcu(&opp->node, &new_opp->node);
	mutex_unlock(&dev_opp_list_lock);
	synchronize_rcu();

	if (availability_req)
		srcu_notifier_call_chain(&dev_opp->head, OPP_EVENT_ENABLE,
					 new_opp);
	else
		srcu_notifier_call_chain(&dev_opp->head, OPP_EVENT_DISABLE,
					 new_opp);

	new_opp = opp;
	goto out;

unlock:
	mutex_unlock(&dev_opp_list_lock);
out:
	kfree(new_opp);
	return r;
}

int opp_enable(struct device *dev, unsigned long freq)
{
	return opp_set_availability(dev, freq, true);
}

int opp_disable(struct device *dev, unsigned long freq)
{
	return opp_set_availability(dev, freq, false);
}

#ifdef CONFIG_CPU_FREQ
 
int opp_init_cpufreq_table(struct device *dev,
			    struct cpufreq_frequency_table **table)
{
	struct device_opp *dev_opp;
	struct opp *opp;
	struct cpufreq_frequency_table *freq_table;
	int i = 0;

	mutex_lock(&dev_opp_list_lock);

	dev_opp = find_device_opp(dev);
	if (IS_ERR(dev_opp)) {
		int r = PTR_ERR(dev_opp);
		mutex_unlock(&dev_opp_list_lock);
		dev_err(dev, "%s: Device OPP not found (%d)\n", __func__, r);
		return r;
	}

	freq_table = kzalloc(sizeof(struct cpufreq_frequency_table) *
			     (opp_get_opp_count(dev) + 1), GFP_KERNEL);
	if (!freq_table) {
		mutex_unlock(&dev_opp_list_lock);
		dev_warn(dev, "%s: Unable to allocate frequency table\n",
			__func__);
		return -ENOMEM;
	}

	list_for_each_entry(opp, &dev_opp->opp_list, node) {
		if (opp->available) {
			freq_table[i].index = i;
			freq_table[i].frequency = opp->rate / 1000;
			i++;
		}
	}
	mutex_unlock(&dev_opp_list_lock);

	freq_table[i].index = i;
	freq_table[i].frequency = CPUFREQ_TABLE_END;

	*table = &freq_table[0];

	return 0;
}

void opp_free_cpufreq_table(struct device *dev,
				struct cpufreq_frequency_table **table)
{
	if (!table)
		return;

	kfree(*table);
	*table = NULL;
}
#endif		 

struct srcu_notifier_head *opp_get_notifier(struct device *dev)
{
	struct device_opp *dev_opp = find_device_opp(dev);

	if (IS_ERR(dev_opp))
		return ERR_CAST(dev_opp);  

	return &dev_opp->head;
}
