/*
    Montage Technology M88RS6000 
    - DVBS/S2 Satellite demod/tuner driver
    Copyright (C) 2014 Max Nibble <nibble.max@gmail.com>

 */

#ifndef DVBSKY_M88RS6000_H
#define DVBSKY_M88RS6000_H

#include <linux/dvb/frontend.h>

struct dvbsky_m88rs6000_config {
	u8 demod_address; /* the demodulator's i2c address */
	u8 pin_ctrl; /* LNB pin control.*/
	u8 ci_mode; /*0: no ci, others: ci mode.*/
	u8 ts_mode; /* 0: Parallel, 1: Serial */
	u8 tuner_readstops;
	/* Set device param to start dma */
	int (*set_ts_params)(struct dvb_frontend *fe, int is_punctured);
	/* Set LNB voltage */
	int (*set_voltage)(struct dvb_frontend* fe, fe_sec_voltage_t voltage);
};

#if defined(CONFIG_DVB_DVBSKY_M88RS6000) || \
	(defined(CONFIG_DVB_DVBSKY_M88RS6000_MODULE) && defined(MODULE))
extern struct dvb_frontend *dvbsky_m88rs6000_attach(
       const struct dvbsky_m88rs6000_config *config,
       struct i2c_adapter *i2c);
#else
static inline struct dvb_frontend *dvbsky_m88rs6000_attach(
       const struct dvbsky_m88rs6000_config *config,
       struct i2c_adapter *i2c)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif /* CONFIG_DVB_DVBSKY_M88RS6000 */
#endif /* DVBSKY_M88RS6000_H */
