/*
 * Copyright (C) 2010 Felix Fietkau <nbd@openwrt.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/netdevice.h>
#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/debugfs.h>
#include <linux/ieee80211.h>
#include <linux/export.h>
#include <net/mac80211_xr.h>
#include "rc80211_minstrel.h"
#include "rc80211_minstrel_ht.h"

#ifdef CONFIG_XRMAC_DEBUGFS

static int
minstrel_ht_stats_open(struct inode *inode, struct file *file)
{
	struct minstrel_ht_sta_priv *msp = inode->i_private;
	struct minstrel_ht_sta *mi = &msp->ht;
	struct minstrel_debugfs_info *ms;
	unsigned int i, j, tp, prob, eprob;
	char *p;
	int ret;

	if (!msp->is_ht) {
		inode->i_private = &msp->legacy;
		ret = xrmac_minstrel_stats_open(inode, file);
		inode->i_private = msp;
		return ret;
	}

	ms = kmalloc(sizeof(*ms) + 8192, GFP_KERNEL);
	if (!ms)
		return -ENOMEM;

	file->private_data = ms;
	p = ms->buf;
	p += sprintf(p, "type      rate     throughput  ewma prob   this prob  "
			"this succ/attempt   success    attempts\n");
	for (i = 0; i < MINSTREL_MAX_STREAMS * MINSTREL_STREAM_GROUPS; i++) {
		char htmode = '2';
		char gimode = 'L';

		if (!mi->groups[i].supported)
			continue;

		if (xrmac_minstrel_mcs_groups[i].flags & IEEE80211_TX_RC_40_MHZ_WIDTH)
			htmode = '4';
		if (xrmac_minstrel_mcs_groups[i].flags & IEEE80211_TX_RC_SHORT_GI)
			gimode = 'S';

		for (j = 0; j < MCS_GROUP_RATES; j++) {
			struct minstrel_rate_stats *mr = &mi->groups[i].rates[j];
			int idx = i * MCS_GROUP_RATES + j;

			if (!(mi->groups[i].supported & BIT(j)))
				continue;

			p += sprintf(p, "HT%c0/%cGI ", htmode, gimode);

			*(p++) = (idx == mi->max_tp_rate) ? 'T' : ' ';
			*(p++) = (idx == mi->max_tp_rate2) ? 't' : ' ';
			*(p++) = (idx == mi->max_prob_rate) ? 'P' : ' ';
			p += sprintf(p, "MCS%-2u", (xrmac_minstrel_mcs_groups[i].streams - 1) *
					MCS_GROUP_RATES + j);

			tp = mr->cur_tp / 10;
			prob = MINSTREL_TRUNC(mr->cur_prob * 1000);
			eprob = MINSTREL_TRUNC(mr->probability * 1000);

			p += sprintf(p, "  %6u.%1u   %6u.%1u   %6u.%1u        "
					"%3u(%3u)   %8llu    %8llu\n",
					tp / 10, tp % 10,
					eprob / 10, eprob % 10,
					prob / 10, prob % 10,
					mr->last_success,
					mr->last_attempts,
					(unsigned long long)mr->succ_hist,
					(unsigned long long)mr->att_hist);
		}
	}
	p += sprintf(p, "\nTotal packet count::    ideal %d      "
			"lookaround %d\n",
			max(0, (int) mi->total_packets - (int) mi->sample_packets),
			mi->sample_packets);
	p += sprintf(p, "Average A-MPDU length: %d.%d\n",
		MINSTREL_TRUNC(mi->avg_ampdu_len),
		MINSTREL_TRUNC(mi->avg_ampdu_len * 10) % 10);
	ms->len = p - ms->buf;

	return nonseekable_open(inode, file);
}

static const struct file_operations minstrel_ht_stat_fops = {
	.owner = THIS_MODULE,
	.open = minstrel_ht_stats_open,
	.read = xrmac_minstrel_stats_read,
	.release = xrmac_minstrel_stats_release,
	.llseek = no_llseek,
};

static int minstrel_ctrl_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}
static ssize_t minstrel_get_param(struct file *file,
	char __user *user_buf, size_t count, loff_t *ppos)
{
	struct minstrel_priv *mp_ctrl = file->private_data;
	char buf[200];
	size_t size = 0;
	sprintf(buf, "ewma_level=%d, update_interval=%dms\n" \
			"has_mrr=%d, lookaround=%d, lookaround_mrr=%d\n",
			mp_ctrl->ewma_level, mp_ctrl->update_interval,
			mp_ctrl->has_mrr, mp_ctrl->lookaround_rate, mp_ctrl->lookaround_rate_mrr);

	size = strlen(buf);

	return simple_read_from_buffer(user_buf, count, ppos,
					buf, size);
}

static ssize_t minstrel_set_param(struct file *file,
	const char __user *user_buf, size_t count, loff_t *ppos)
{
	struct minstrel_priv *mp_ctrl = file->private_data;
	char buf[50] = {0};
	char *start  = &buf[0];
	char *endptr = NULL;
	unsigned int set_param = 0;

	if (!count)
		return -EINVAL;
	if (copy_from_user(buf, user_buf, count > 49 ? 49 : count))
		return -EFAULT;


	set_param = simple_strtoul(start, &endptr, 10);
	if (set_param <= 100)
		mp_ctrl->ewma_level = set_param;

	start = endptr+1;
	if (start < buf+49) {
		set_param = simple_strtoul(start, &endptr, 10);
		mp_ctrl->update_interval = set_param;
	}

	start = endptr+1;
	if (start < buf+49) {
		set_param = simple_strtoul(start, &endptr, 10);
		if (set_param <= 100)
			mp_ctrl->lookaround_rate = set_param;
	}

	start = endptr+1;
	if (start < buf+49) {
		set_param = simple_strtoul(start, &endptr, 10);
		if (set_param <= 100)
			mp_ctrl->lookaround_rate_mrr = set_param;
	}
	return count;
}

static const struct file_operations fops_param_ctrl = {
	.open   = minstrel_ctrl_open,
	.write  = minstrel_set_param,
	.read   = minstrel_get_param,
	.llseek = default_llseek,
};

void
xrmac_minstrel_ht_add_sta_debugfs(void *priv, void *priv_sta, struct dentry *dir)
{
	struct minstrel_ht_sta_priv *msp = priv_sta;

	msp->dbg_stats = debugfs_create_file("rc_stats", S_IRUGO, dir, msp,
			&minstrel_ht_stat_fops);

	if (priv)
		msp->dbg_stats = debugfs_create_file("param_ctrl", S_IRUSR|S_IWUSR, dir,
											priv, &fops_param_ctrl);
}

void
xrmac_minstrel_ht_remove_sta_debugfs(void *priv, void *priv_sta)
{
	struct minstrel_ht_sta_priv *msp = priv_sta;

	debugfs_remove(msp->dbg_stats);
}
#endif /* CONFIG_XRMAC_DEBUGFS */
