#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/export.h>
#include <asm/div64.h>
#include <sound/core.h>
#define COMPONENT clock
#include "common.h"
#include <linux/atomic.h>

extern int snd_stm_debug_level;

struct snd_stm_clk {
	struct clk *clk;

	unsigned long nominal_rate;
	int adjustment;  
	atomic_t enabled;

	snd_stm_magic_field;
};

static int snd_stm_clk_adjustment_info(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = -999999;
	uinfo->value.integer.max = 1000000;

	return 0;
}

static int snd_stm_clk_adjustment_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_stm_clk *snd_stm_clk = snd_kcontrol_chip(kcontrol);

	snd_stm_printd(1, "%s(kcontrol=0x%p, ucontrol=0x%p)\n",
			__func__, kcontrol, ucontrol);

	BUG_ON(!snd_stm_clk);
	BUG_ON(!snd_stm_magic_valid(snd_stm_clk));

	ucontrol->value.integer.value[0] = snd_stm_clk->adjustment;

	return 0;
}

static int snd_stm_clk_adjustment_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_stm_clk *snd_stm_clk = snd_kcontrol_chip(kcontrol);
	int old_adjustement;

	snd_stm_printd(1, "%s(kcontrol=0x%p, ucontrol=0x%p)\n",
			__func__, kcontrol, ucontrol);

	BUG_ON(!snd_stm_clk);
	BUG_ON(!snd_stm_magic_valid(snd_stm_clk));

	BUG_ON(ucontrol->value.integer.value[0] < -999999);
	BUG_ON(ucontrol->value.integer.value[0] > 1000000);

	old_adjustement = snd_stm_clk->adjustment;
	snd_stm_clk->adjustment = ucontrol->value.integer.value[0];

	if (atomic_read(&snd_stm_clk->enabled) &&
	   snd_stm_clk_set_rate(snd_stm_clk, snd_stm_clk->nominal_rate) < 0)
		return -EINVAL;

	return old_adjustement != snd_stm_clk->adjustment;
}

static struct snd_kcontrol_new snd_stm_clk_adjustment_ctl = {
	.iface = SNDRV_CTL_ELEM_IFACE_PCM,
	.name = "PCM Playback Oversampling Freq. Adjustment",
	.info = snd_stm_clk_adjustment_info,
	.get = snd_stm_clk_adjustment_get,
	.put = snd_stm_clk_adjustment_put,
};

#ifdef MY_DEF_HERE
int snd_stm_clk_prepare_enable(struct snd_stm_clk *clk)
{
	int result = 0;

	snd_stm_printd(1, "%s(clk=%p)\n", __func__, clk);

	BUG_ON(!clk);
	BUG_ON(!snd_stm_magic_valid(clk));

	if (atomic_cmpxchg(&clk->enabled, 0, 1))
		return 0;

	result = clk_prepare_enable(clk->clk);
	if (result) {
		atomic_set(&clk->enabled, 0);
	}

	return result;
}
EXPORT_SYMBOL(snd_stm_clk_prepare_enable);

int snd_stm_clk_prepare(struct snd_stm_clk *clk)
{
	int result = 0;

	snd_stm_printd(1, "%s(clk=%p)\n", __func__, clk);

	BUG_ON(!clk);
	BUG_ON(!snd_stm_magic_valid(clk));

	result = clk_prepare(clk->clk);

	return result;
}
EXPORT_SYMBOL(snd_stm_clk_prepare);
#endif  

int snd_stm_clk_enable(struct snd_stm_clk *clk)
{
	int result = 0;

	snd_stm_printd(1, "%s(clk=%p)\n", __func__, clk);

	BUG_ON(!clk);
	BUG_ON(!snd_stm_magic_valid(clk));

	if (atomic_cmpxchg(&clk->enabled, 0, 1))
		return 0;

#ifdef MY_DEF_HERE
	result = clk_enable(clk->clk);
	if (result)
		atomic_set(&clk->enabled, 0);
#else  
	result = clk_prepare_enable(clk->clk);
	if (result) {
		atomic_set(&clk->enabled, 0);
	}
#endif  

	return result;
}
EXPORT_SYMBOL(snd_stm_clk_enable);

#ifdef MY_DEF_HERE
int snd_stm_clk_unprepare(struct snd_stm_clk *clk)
{
	snd_stm_printd(1, "%s(clk=%p)\n", __func__, clk);

	BUG_ON(!clk);
	BUG_ON(!snd_stm_magic_valid(clk));

	clk_unprepare(clk->clk);

	return 0;
}
EXPORT_SYMBOL(snd_stm_clk_unprepare);
#endif  

int snd_stm_clk_disable(struct snd_stm_clk *clk)
{
	snd_stm_printd(1, "%s(clk=%p)\n", __func__, clk);

	BUG_ON(!clk);
	BUG_ON(!snd_stm_magic_valid(clk));

	if (!atomic_read(&clk->enabled))
		return 0;

#ifdef MY_DEF_HERE
	clk_disable(clk->clk);
#else  
	clk_disable_unprepare(clk->clk);
#endif  

	atomic_set(&clk->enabled, 0);

	return 0;
}
EXPORT_SYMBOL(snd_stm_clk_disable);
#ifdef MY_DEF_HERE
int snd_stm_clk_disable_unprepare(struct snd_stm_clk *clk)
{
	snd_stm_printd(1, "%s(clk=%p)\n", __func__, clk);

	BUG_ON(!clk);
	BUG_ON(!snd_stm_magic_valid(clk));

	if (!atomic_read(&clk->enabled))
		return 0;

	clk_disable_unprepare(clk->clk);

	atomic_set(&clk->enabled, 0);

	return 0;
}
EXPORT_SYMBOL(snd_stm_clk_disable_unprepare);
#endif  

int snd_stm_clk_set_rate(struct snd_stm_clk *clk, unsigned long rate)
{
	int rate_adjusted, rate_achieved;
	int delta;

	snd_stm_printd(1, "%s(clk=%p, rate=%lu)\n", __func__, clk, rate);

	BUG_ON(!clk);
	BUG_ON(!snd_stm_magic_valid(clk));

	if (!atomic_read(&clk->enabled))
		return -EAGAIN;

	if (clk->adjustment < 0) {
		 
		delta = -1;
		clk->adjustment = -clk->adjustment;
	} else {
		delta = 1;
	}
	 
	delta *= (int)div64_u64((uint64_t)rate *
			(uint64_t)clk->adjustment + 500000,
			1000000);
	rate_adjusted = rate + delta;

	BUG_ON(rate_adjusted == 0);

	if (likely(clk_set_rate(clk->clk, rate_adjusted)) < 0) {
		snd_stm_printe("Failed to clk set rate %d !\n",
				rate_adjusted);
		return -EINVAL;
	}

	rate_achieved = clk_get_rate(clk->clk);
	BUG_ON(rate_achieved == 0);

	BUG_ON(rate_achieved > 2*rate);
	delta = rate_achieved - rate;
	if (delta < 0) {
		 
		delta = -delta;
		clk->adjustment = -1;
	} else {
		clk->adjustment = 1;
	}
	 
	clk->adjustment *= (int)div64_u64((uint64_t)delta * 1000000 +
			rate / 2, rate);

	clk->nominal_rate = rate;

	return 0;
}
EXPORT_SYMBOL(snd_stm_clk_set_rate);

struct snd_stm_clk *snd_stm_clk_get(struct device *dev, const char *id,
		struct snd_card *card, int card_device)
{
	struct snd_stm_clk *snd_stm_clk;

	snd_stm_printd(0, "%s(dev=%p('%s'), id='%s')\n",
			__func__, dev, dev_name(dev), id);

	snd_stm_clk = kzalloc(sizeof(*snd_stm_clk), GFP_KERNEL);
	if (!snd_stm_clk) {
		snd_stm_printe("No memory for '%s'('%s') clock!\n",
				dev_name(dev), id);
		goto error_kzalloc_clk;
	}

	atomic_set((&snd_stm_clk->enabled), 0);

	snd_stm_magic_set(snd_stm_clk);

	snd_stm_clk->clk = clk_get(dev, id);
	if  (!snd_stm_clk->clk || IS_ERR(snd_stm_clk->clk)) {
		snd_stm_printe("Can't get '%s' clock ('%s's parent)!\n",
				id, dev_name(dev));
		goto error_clk_get;
	}

	snd_stm_clk_adjustment_ctl.device = card_device;
	if (snd_ctl_add(card, snd_ctl_new1(&snd_stm_clk_adjustment_ctl,
			snd_stm_clk)) < 0) {
		snd_stm_printe("Failed to add '%s'('%s') clock ALSA control!\n",
				dev_name(dev), id);
		goto error_clk_get;
	}
	snd_stm_clk_adjustment_ctl.index++;

	return snd_stm_clk;

error_clk_get:
	snd_stm_magic_clear(snd_stm_clk);
	kfree(snd_stm_clk);

error_kzalloc_clk:
	return NULL;
}
EXPORT_SYMBOL(snd_stm_clk_get);

void snd_stm_clk_put(struct snd_stm_clk *clk)
{
	snd_stm_printd(0, "%s(clk=%p)\n", __func__, clk);

	BUG_ON(!clk);

	BUG_ON(!snd_stm_magic_valid(clk));

	clk_put(clk->clk);

	snd_stm_magic_clear(clk);

	kfree(clk);
}
EXPORT_SYMBOL(snd_stm_clk_put);
