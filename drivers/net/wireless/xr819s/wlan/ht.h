/*
 * HT-related code for XRadio drivers
 *
 * Copyright (c) 2013
 * Xradio Technology Co., Ltd. <www.xradiotech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef XRADIO_HT_H_INCLUDED
#define XRADIO_HT_H_INCLUDED

#include <net/mac80211_xr.h>

struct xradio_ht_info {
	struct ieee80211_sta_ht_cap  ht_cap;
	enum nl80211_channel_type    channel_type;
	u16                          operation_mode;
};

static inline int xradio_is_ht(const struct xradio_ht_info *ht_info)
{
	return ht_info->channel_type != NL80211_CHAN_NO_HT;
}

static inline int xradio_ht_greenfield(const struct xradio_ht_info *ht_info)
{
	int ret = (xradio_is_ht(ht_info) &&
		(ht_info->ht_cap.cap & IEEE80211_HT_CAP_GRN_FLD) &&
		!(ht_info->operation_mode & IEEE80211_HT_OP_MODE_NON_GF_STA_PRSNT));
	return ret;
}

static inline int xradio_ht_ampdu_density(const struct xradio_ht_info *ht_info)
{
	if (!xradio_is_ht(ht_info))
		return 0;
	return ht_info->ht_cap.ampdu_density;
}

#endif /* XRADIO_HT_H_INCLUDED */
