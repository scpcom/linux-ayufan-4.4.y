#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/module.h>
#if defined(MY_ABC_HERE)
#include <linux/pm_opp.h>
#else  
#include <linux/opp.h>
#endif  
#include <linux/cpu.h>

#include "omap_device.h"

#include "omap_opp_data.h"

static u8 __initdata omap_table_init;

int __init omap_init_opp_table(struct omap_opp_def *opp_def,
		u32 opp_def_size)
{
	int i, r;

	if (!opp_def || !opp_def_size) {
		pr_err("%s: invalid params!\n", __func__);
		return -EINVAL;
	}

	if (omap_table_init)
		return -EEXIST;
	omap_table_init = 1;

	for (i = 0; i < opp_def_size; i++, opp_def++) {
		struct omap_hwmod *oh;
		struct device *dev;

		if (!opp_def->hwmod_name) {
			pr_err("%s: NULL name of omap_hwmod, failing [%d].\n",
				__func__, i);
			return -EINVAL;
		}

		if (!strncmp(opp_def->hwmod_name, "mpu", 3)) {
			 
			dev = get_cpu_device(0);
		} else {
			oh = omap_hwmod_lookup(opp_def->hwmod_name);
			if (!oh || !oh->od) {
				pr_debug("%s: no hwmod or odev for %s, [%d] cannot add OPPs.\n",
					 __func__, opp_def->hwmod_name, i);
				continue;
			}
			dev = &oh->od->pdev->dev;
		}

#if defined(MY_ABC_HERE)
		r = dev_pm_opp_add(dev, opp_def->freq, opp_def->u_volt);
#else  
		r = opp_add(dev, opp_def->freq, opp_def->u_volt);
#endif  
		if (r) {
			dev_err(dev, "%s: add OPP %ld failed for %s [%d] result=%d\n",
				__func__, opp_def->freq,
				opp_def->hwmod_name, i, r);
		} else {
			if (!opp_def->default_available)
#if defined(MY_ABC_HERE)
				r = dev_pm_opp_disable(dev, opp_def->freq);
#else  
				r = opp_disable(dev, opp_def->freq);
#endif  
			if (r)
				dev_err(dev, "%s: disable %ld failed for %s [%d] result=%d\n",
					__func__, opp_def->freq,
					opp_def->hwmod_name, i, r);
		}
	}

	return 0;
}
