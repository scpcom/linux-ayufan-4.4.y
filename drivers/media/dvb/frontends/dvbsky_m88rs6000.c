/*
    Montage Technology M88RS6000 
    - DVBS/S2 Satellite demod/tuner driver
    Copyright (C) 2014 Max Nibble <nibble.max@gmail.com>

 */

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/firmware.h>

#include "dvb_frontend.h"
#include "dvbsky_m88rs6000.h"
#include "dvbsky_m88rs6000_priv.h"

static int debug;
module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "Activates frontend debugging (default:0)");

#define dprintk(args...) \
	do { \
		if (debug) \
			printk(KERN_INFO "m88rs6000: " args); \
	} while (0)

/*demod register operations.*/
static int m88rs6000_writereg(struct m88rs6000_state *state, int reg, int data)
{
	u8 buf[] = { reg, data };
	struct i2c_msg msg = { .addr = state->config->demod_address,
		.flags = 0, .buf = buf, .len = 2 };
	int ret;

	if (debug > 1)
		printk("m88rs6000: %s: write reg 0x%02x, value 0x%02x\n",
			__func__, reg, data);

	ret = i2c_transfer(state->i2c, &msg, 1);
	if (ret != 1) {
		printk(KERN_ERR "%s: writereg error(err == %i, reg == 0x%02x,"
			 " value == 0x%02x)\n", __func__, ret, reg, data);
		return -EREMOTEIO;
	}
	return 0;
}

static int m88rs6000_readreg(struct m88rs6000_state *state, u8 reg)
{
	int ret;
	u8 b0[] = { reg };
	u8 b1[] = { 0 };
	struct i2c_msg msg[] = {
		{ .addr = state->config->demod_address, .flags = 0,
			.buf = b0, .len = 1 },
		{ .addr = state->config->demod_address, .flags = I2C_M_RD,
			.buf = b1, .len = 1 }
	};
	ret = i2c_transfer(state->i2c, msg, 2);

	if (ret != 2) {
		printk(KERN_ERR "%s: reg=0x%x (error=%d)\n",
			__func__, reg, ret);
		return ret;
	}

	if (debug > 1)
		printk(KERN_INFO "m88rs6000: read reg 0x%02x, value 0x%02x\n",
			reg, b1[0]);

	return b1[0];
}

/*tuner register operations.*/
static int m88rs6000_tuner_writereg(struct m88rs6000_state *state, int reg, int data)
{
	u8 buf[] = { reg, data };
	struct i2c_msg msg = { .addr = state->tuner_addr,
		.flags = 0, .buf = buf, .len = 2 };
	int ret;

	m88rs6000_writereg(state, 0x03, 0x11);
	ret = i2c_transfer(state->i2c, &msg, 1);
	
	if (ret != 1) {
		printk("%s: writereg error(err == %i, reg == 0x%02x,"
			 " value == 0x%02x)\n", __func__, ret, reg, data);
		return -EREMOTEIO;
	}

	return 0;
}

static int m88rs6000_tuner_readreg(struct m88rs6000_state *state, u8 reg)
{
	int ret;
	u8 b0[] = { reg };
	u8 b1[] = { 0 };
	struct i2c_msg msg[] = {
		{ .addr = state->tuner_addr, .flags = 0,
			.buf = b0, .len = 1 },
		{ .addr = state->tuner_addr, .flags = I2C_M_RD,
			.buf = b1, .len = 1 }
	};

	m88rs6000_writereg(state, 0x03, (0x11 + state->config->tuner_readstops));	
	ret = i2c_transfer(state->i2c, msg, 2);

	if (ret != 2) {
		printk(KERN_ERR "%s: reg=0x%x(error=%d)\n", __func__, reg, ret);
		return ret;
	}

	return b1[0];
}

/* Bulk demod I2C write, for firmware download. */
static int m88rs6000_writeregN(struct m88rs6000_state *state, int reg,
				const u8 *data, u16 len)
{
	int ret = -EREMOTEIO;
	struct i2c_msg msg;
	u8 *buf;

	buf = kmalloc(len + 1, GFP_KERNEL);
	if (buf == NULL) {
		printk("Unable to kmalloc\n");
		ret = -ENOMEM;
		goto error;
	}

	*(buf) = reg;
	memcpy(buf + 1, data, len);

	msg.addr = state->config->demod_address;
	msg.flags = 0;
	msg.buf = buf;
	msg.len = len + 1;

	if (debug > 1)
		printk(KERN_INFO "m88rs6000: %s:  write regN 0x%02x, len = %d\n",
			__func__, reg, len);

	ret = i2c_transfer(state->i2c, &msg, 1);
	if (ret != 1) {
		printk(KERN_ERR "%s: writereg error(err == %i, reg == 0x%02x\n",
			 __func__, ret, reg);
		ret = -EREMOTEIO;
	}
	
error:
	kfree(buf);

	return ret;
}

static int m88rs6000_load_firmware(struct dvb_frontend *fe)
{
	struct m88rs6000_state *state = fe->demodulator_priv;
	const struct firmware *fw;
	int i, ret = 0;

	dprintk("%s()\n", __func__);
		
	if (state->skip_fw_load)
		return 0;
	/* Load firmware */
	/* request the firmware, this will block until someone uploads it */	
	printk(KERN_INFO "%s: Waiting for firmware upload (%s)...\n", __func__,
				RS6000_DEFAULT_FIRMWARE);		
	ret = request_firmware(&fw, RS6000_DEFAULT_FIRMWARE,
				state->i2c->dev.parent);

	printk(KERN_INFO "%s: Waiting for firmware upload(2)...\n", __func__);
	if (ret) {
		printk(KERN_ERR "%s: No firmware uploaded (timeout or file not "
				"found?)\n", __func__);
		return ret;
	}

	/* Make sure we don't recurse back through here during loading */
	state->skip_fw_load = 1;

	dprintk("Firmware is %zu bytes (%02x %02x .. %02x %02x)\n",
			fw->size,
			fw->data[0],
			fw->data[1],
			fw->data[fw->size - 2],
			fw->data[fw->size - 1]);
			
	/* stop internal mcu. */
	m88rs6000_writereg(state, 0xb2, 0x01);
	/* split firmware to download.*/
	for(i = 0; i < FW_DOWN_LOOP; i++){
		ret = m88rs6000_writeregN(state, 0xb0, &(fw->data[FW_DOWN_SIZE*i]), FW_DOWN_SIZE);
		if(ret != 1) break;		
	}
	/* start internal mcu. */
	if(ret == 1)
		m88rs6000_writereg(state, 0xb2, 0x00);
		
	release_firmware(fw);

	dprintk("%s: Firmware upload %s\n", __func__,
			ret == 1 ? "complete" : "failed");

	if(ret == 1) ret = 0;
	
	/* Ensure firmware is always loaded if required */
	state->skip_fw_load = 0;

	return ret;
}


static int m88rs6000_set_voltage(struct dvb_frontend *fe, fe_sec_voltage_t voltage)
{
	struct m88rs6000_state *state = fe->demodulator_priv;
	u8 data;

	dprintk("%s(%d)\n", __func__, voltage);

	dprintk("m88rs6000:pin_ctrl = (%02x)\n", state->config->pin_ctrl);
	
	if(state->config->set_voltage)
		state->config->set_voltage(fe, voltage);
	
	data = m88rs6000_readreg(state, 0xa2);
	
        if(state->config->pin_ctrl & 0x80){ /*If control pin is assigned.*/
	        data &= ~0x03; /* bit0 V/H, bit1 off/on */
	        if(state->config->pin_ctrl & 0x02)
		     data |= 0x02;

	        switch (voltage) {
	        case SEC_VOLTAGE_18:
		     if((state->config->pin_ctrl & 0x01) == 0)
			  data |= 0x01;
		     break;
	        case SEC_VOLTAGE_13:
		     if(state->config->pin_ctrl & 0x01)
			  data |= 0x01;
		     break;
	        case SEC_VOLTAGE_OFF:
		     if(state->config->pin_ctrl & 0x02)
			   data &= ~0x02;			
		     else
			   data |= 0x02;
		     break;
	         }
        }

	m88rs6000_writereg(state, 0xa2, data);

	return 0;
}

static int m88rs6000_read_status(struct dvb_frontend *fe, fe_status_t* status)
{
	struct m88rs6000_state *state = fe->demodulator_priv;
	int lock = 0;
	
	*status = 0;
	
	switch (state->delivery_system){
	case SYS_DVBS:
		lock = m88rs6000_readreg(state, 0xd1);
		dprintk("%s: SYS_DVBS status=%x.\n", __func__, lock);
		
		if ((lock & 0x07) == 0x07){
			/*if((m88rs6000_readreg(state, 0x0d) & 0x07) == 0x07)*/
				*status = FE_HAS_SIGNAL | FE_HAS_CARRIER 
					| FE_HAS_VITERBI | FE_HAS_SYNC | FE_HAS_LOCK;
			
		}
		break;
	case SYS_DVBS2:
		lock = m88rs6000_readreg(state, 0x0d);
		dprintk("%s: SYS_DVBS2 status=%x.\n", __func__, lock);

		if ((lock & 0x8f) == 0x8f)
			*status = FE_HAS_SIGNAL | FE_HAS_CARRIER 
				| FE_HAS_VITERBI | FE_HAS_SYNC | FE_HAS_LOCK;
			
		break;
	default:
		break;
	}

	return 0;
}

static int m88rs6000_read_ber(struct dvb_frontend *fe, u32* ber)
{
	struct m88rs6000_state *state = fe->demodulator_priv;
	u8 tmp1, tmp2, tmp3;
	u32 ldpc_frame_cnt, pre_err_packags, code_rate_fac = 0;

	dprintk("%s()\n", __func__);

	switch (state->delivery_system) {
	case SYS_DVBS:
		m88rs6000_writereg(state, 0xf9, 0x04);
		tmp3 = m88rs6000_readreg(state, 0xf8);
		if ((tmp3&0x10) == 0){
			tmp1 = m88rs6000_readreg(state, 0xf7);
			tmp2 = m88rs6000_readreg(state, 0xf6);
			tmp3 |= 0x10;
			m88rs6000_writereg(state, 0xf8, tmp3);
			state->preBer = (tmp1<<8) | tmp2;
		}
		break;
	case SYS_DVBS2:
		tmp1 = m88rs6000_readreg(state, 0x7e) & 0x0f;
		switch(tmp1){
		case 0:	code_rate_fac = 16008 - 80; break;
		case 1:	code_rate_fac = 21408 - 80; break;
		case 2:	code_rate_fac = 25728 - 80; break;
		case 3:	code_rate_fac = 32208 - 80; break;
		case 4:	code_rate_fac = 38688 - 80; break;
		case 5:	code_rate_fac = 43040 - 80; break;
		case 6:	code_rate_fac = 48408 - 80; break;
		case 7:	code_rate_fac = 51648 - 80; break;
		case 8:	code_rate_fac = 53840 - 80; break;
		case 9:	code_rate_fac = 57472 - 80; break;
		case 10: code_rate_fac = 58192 - 80; break;
		}
		
		tmp1 = m88rs6000_readreg(state, 0xd7) & 0xff;
		tmp2 = m88rs6000_readreg(state, 0xd6) & 0xff;
		tmp3 = m88rs6000_readreg(state, 0xd5) & 0xff;		
		ldpc_frame_cnt = (tmp1 << 16) | (tmp2 << 8) | tmp3;

		tmp1 = m88rs6000_readreg(state, 0xf8) & 0xff;
		tmp2 = m88rs6000_readreg(state, 0xf7) & 0xff;
		pre_err_packags = tmp1<<8 | tmp2;
		
		if (ldpc_frame_cnt > 1000){
			m88rs6000_writereg(state, 0xd1, 0x01);
			m88rs6000_writereg(state, 0xf9, 0x01);
			m88rs6000_writereg(state, 0xf9, 0x00);
			m88rs6000_writereg(state, 0xd1, 0x00);
			state->preBer = pre_err_packags;
		} 				
		break;
	default:
		break;
	}
	*ber = state->preBer;
	
	return 0;
}

static int m88rs6000_read_signal_strength(struct dvb_frontend *fe,
						u16 *signal_strength)
{
	struct m88rs6000_state *state = fe->demodulator_priv;

	int val;

	//u32  RF_GS = 290, IF_GS = 290, BB_GS = 290;
	u32  PGA2_cri_GS = 46, PGA2_crf_GS = 290, TIA_GS = 290;
	u32  RF_GC = 1200, IF_GC = 1100, BB_GC = 300, PGA2_GC = 300, TIA_GC = 300;
	u32  PGA2_cri = 0, PGA2_crf = 0;
	u32  RFG = 0, IFG = 0, BBG = 0, PGA2G = 0, TIAG = 0;

	u32 i = 0;

	u32 RFGS[13] = {0, 245, 266, 268, 270, 285, 298, 295, 283, 285, 285, 300, 300};
	u32 IFGS[12] = {0, 300, 230, 270, 270, 285, 295, 285, 290, 295, 295, 310};
	u32 BBGS[14] = {0, 286, 275, 290, 294, 300, 290, 290, 285, 283, 260, 295, 290, 260};

	dprintk("%s()\n", __func__);
	
	val = m88rs6000_tuner_readreg(state, 0x5A);
	RF_GC = val & 0x0f;
	if(RF_GC >= ARRAY_SIZE(RFGS)) {
		printk(KERN_ERR "%s: Invalid, RFGC=%d\n", __func__, RF_GC);
		return -EINVAL;
	}

	val = m88rs6000_tuner_readreg(state, 0x5F);
	IF_GC = val & 0x0f;
	if(IF_GC >= ARRAY_SIZE(IFGS)) {
		printk(KERN_ERR "%s: Invalid, IFGC=%d\n", __func__, IF_GC);
		return -EINVAL;
	}

	
	val = m88rs6000_tuner_readreg(state, 0x3F);
	TIA_GC = (val >> 4) & 0x07;
	
	val = m88rs6000_tuner_readreg(state, 0x77);
	BB_GC = (val >> 4) & 0x0f;
	if(BB_GC >= ARRAY_SIZE(BBGS)) {
		printk(KERN_ERR "%s: Invalid, BBGC=%d\n", __func__, BB_GC);
		return -EINVAL;
	}

	val = m88rs6000_tuner_readreg(state, 0x76);
	PGA2_GC = val & 0x3f;
	PGA2_cri = PGA2_GC >> 2;
	PGA2_crf = PGA2_GC & 0x03;

	for(i = 0; i <= RF_GC; i++) {
		RFG += RFGS[i];
	}

	if(RF_GC == 0)	RFG += 400;
	if(RF_GC == 1)	RFG += 300;
	if(RF_GC == 2)	RFG += 200;
	if(RF_GC == 3)	RFG += 100;

	for(i = 0; i <= IF_GC; i++) {
		IFG += IFGS[i];
	}

	TIAG = TIA_GC * TIA_GS;

	for(i = 0; i <= BB_GC; i++) {
		BBG += BBGS[i];
	}

	PGA2G = PGA2_cri * PGA2_cri_GS + PGA2_crf * PGA2_crf_GS;

	*signal_strength = RFG + IFG - TIAG + BBG + PGA2G;
	
	return 0;
}


static int m88rs6000_read_snr(struct dvb_frontend *fe, u16 *p_snr)
{
	struct m88rs6000_state *state = fe->demodulator_priv;
	u8 val, npow1, npow2, spow1, cnt;
	u16 tmp, snr;
	u32 npow, spow, snr_total;	
	static const u16 mes_log10[] ={
		0,	3010,	4771,	6021, 	6990,	7781,	8451,	9031,	9542,	10000,
		10414,	10792,	11139,	11461,	11761,	12041,	12304,	12553,	12788,	13010,
		13222,	13424,	13617,	13802,	13979,	14150,	14314,	14472,	14624,	14771,
		14914,	15052,	15185,	15315,	15441,	15563,	15682,	15798,	15911,	16021,
		16128,	16232,	16335,	16435,	16532,	16628,	16721,	16812,	16902,	16990,
		17076,	17160,	17243,	17324,	17404,	17482,	17559,	17634,	17709,	17782,
		17853,	17924,	17993,	18062,	18129,	18195,	18261,	18325,	18388,	18451,
		18513,	18573,	18633,	18692,	18751,	18808,	18865,	18921,	18976,	19031
	};
	static const u16 mes_loge[] ={
		0,	6931,	10986,	13863, 	16094,	17918,	19459,	20794,	21972,	23026,
		23979,	24849,	25649,	26391,	27081,	27726,	28332,	28904,	29444,	29957,
		30445,	30910,	31355,	31781,	32189,	32581,	32958,	33322,	33673,	34012,
		34340,	34657,
	};

	dprintk("%s()\n", __func__);

	snr = 0;
	
	switch (state->delivery_system){
	case SYS_DVBS:
		cnt = 10; snr_total = 0;
		while(cnt > 0){
			val = m88rs6000_readreg(state, 0xff);
			snr_total += val;
			cnt--;
		}
		/* TODO(kedong): The following code is based on the formula
		 * from data sheet. The formula is basically
		 * 10*ln(snr/8)/ln(10). The result SNR seems very low though
		 * more reasonable at magnitude level compared to the original
		 * setting. Need to double check with vendor on this.
		 */
		tmp = (u16)(snr_total/80);
		if(tmp > 0){
			if (tmp > 32) tmp = 32;
			snr = (mes_loge[tmp - 1] * 10) / 23026;
		}else{
			snr = 0;
		}
		break;
	case SYS_DVBS2:
		cnt  = 10; npow = 0; spow = 0;
		while(cnt >0){
			npow1 = m88rs6000_readreg(state, 0x8c) & 0xff;
			npow2 = m88rs6000_readreg(state, 0x8d) & 0xff;
			npow += (((npow1 & 0x3f) + (u16)(npow2 << 6)) >> 2);

			spow1 = m88rs6000_readreg(state, 0x8e) & 0xff;
			spow += ((spow1 * spow1) >> 1);
			cnt--;
		}
		npow /= 10; spow /= 10;
		if(spow == 0){
			snr = 0;
		}else if(npow == 0){
			snr = 19;
		}else{
			if(spow > npow){
				tmp = (u16)(spow / npow);
				if (tmp > 80) tmp = 80;
				snr = mes_log10[tmp - 1]*3;
			}else{
				tmp = (u16)(npow / spow);
				if (tmp > 80) tmp = 80;
				snr = -(mes_log10[tmp - 1] / 1000);
			}
		}			
		break;
	default:
		break;
	}
	*p_snr = snr;

	return 0;
}


static int m88rs6000_read_ucblocks(struct dvb_frontend *fe, u32 *ucblocks)
{
	struct m88rs6000_state *state = fe->demodulator_priv;
	u8 tmp1, tmp2, tmp3, data;

	dprintk("%s()\n", __func__);

	switch (state->delivery_system) {
	case SYS_DVBS:
		data = m88rs6000_readreg(state, 0xf8);
		data |= 0x40;
		m88rs6000_writereg(state, 0xf8, data);		
		tmp1 = m88rs6000_readreg(state, 0xf5);
		tmp2 = m88rs6000_readreg(state, 0xf4);
		*ucblocks = (tmp1 <<8) | tmp2;		
		data &= ~0x20;
		m88rs6000_writereg(state, 0xf8, data);
		data |= 0x20;
		m88rs6000_writereg(state, 0xf8, data);
		data &= ~0x40;
		m88rs6000_writereg(state, 0xf8, data);
		break;
	case SYS_DVBS2:
		tmp1 = m88rs6000_readreg(state, 0xda);
		tmp2 = m88rs6000_readreg(state, 0xd9);
		tmp3 = m88rs6000_readreg(state, 0xd8);
		*ucblocks = (tmp1 <<16)|(tmp2 <<8)|tmp3;
		data = m88rs6000_readreg(state, 0xd1);
		data |= 0x01;
		m88rs6000_writereg(state, 0xd1, data);
		data &= ~0x01;
		m88rs6000_writereg(state, 0xd1, data);
		break;
	default:
		break;
	}
	return 0;
}

static int m88rs6000_set_tone(struct dvb_frontend *fe, fe_sec_tone_mode_t tone)
{
	struct m88rs6000_state *state = fe->demodulator_priv;
	u8 data_a1, data_a2;

	dprintk("%s(%d)\n", __func__, tone);
	if ((tone != SEC_TONE_ON) && (tone != SEC_TONE_OFF)) {
		printk(KERN_ERR "%s: Invalid, tone=%d\n", __func__, tone);
		return -EINVAL;
	}

	data_a1 = m88rs6000_readreg(state, 0xa1);
	data_a2 = m88rs6000_readreg(state, 0xa2);

	data_a2 &= 0xdf; /* Normal mode */
	switch (tone) {
	case SEC_TONE_ON:
		dprintk("%s: SEC_TONE_ON\n", __func__);
		data_a1 |= 0x04;
		data_a1 &= ~0x03;
		data_a1 &= ~0x40;
		data_a2 &= ~0xc0;
		break;
	case SEC_TONE_OFF:
		dprintk("%s: SEC_TONE_OFF\n", __func__);
		data_a2 &= ~0xc0;
		data_a2 |= 0x80;
		break;
	}
	m88rs6000_writereg(state, 0xa2, data_a2);
	m88rs6000_writereg(state, 0xa1, data_a1);
	return 0;
}

static int m88rs6000_send_diseqc_msg(struct dvb_frontend *fe,
				struct dvb_diseqc_master_cmd *d)
{
	struct m88rs6000_state *state = fe->demodulator_priv;
	int i, ret = 0;
	u8 tmp, time_out;

	/* Dump DiSEqC message */
	if (debug) {
		printk(KERN_INFO "m88rs6000: %s(", __func__);
		for (i = 0 ; i < d->msg_len ;) {
			printk(KERN_INFO "0x%02x", d->msg[i]);
			if (++i < d->msg_len)
				printk(KERN_INFO ", ");
		}
	}

	tmp = m88rs6000_readreg(state, 0xa2);
	tmp &= ~0xc0;
	tmp &= ~0x20;
	m88rs6000_writereg(state, 0xa2, tmp);
	
	for (i = 0; i < d->msg_len; i ++)
		m88rs6000_writereg(state, (0xa3+i), d->msg[i]);

	tmp = m88rs6000_readreg(state, 0xa1);	
	tmp &= ~0x38;
	tmp &= ~0x40;
	tmp |= ((d->msg_len-1) << 3) | 0x07;
	tmp &= ~0x80;
	m88rs6000_writereg(state, 0xa1, tmp);
	/*	1.5 * 9 * 8	= 108ms	*/
	time_out = 150;
	while (time_out > 0){
		msleep(10);
		time_out -= 10;
		tmp = m88rs6000_readreg(state, 0xa1);		
		if ((tmp & 0x40) == 0)
			break;
	}
	if (time_out == 0){
		tmp = m88rs6000_readreg(state, 0xa1);
		tmp &= ~0x80;
		tmp |= 0x40;
		m88rs6000_writereg(state, 0xa1, tmp);
		ret = 1;
	}
	tmp = m88rs6000_readreg(state, 0xa2);
	tmp &= ~0xc0;
	tmp |= 0x80;
	m88rs6000_writereg(state, 0xa2, tmp);	
	return ret;
}


static int m88rs6000_diseqc_send_burst(struct dvb_frontend *fe,
					fe_sec_mini_cmd_t burst)
{
	struct m88rs6000_state *state = fe->demodulator_priv;
	u8	val, time_out;
	
	dprintk("%s()\n", __func__);

	val = m88rs6000_readreg(state, 0xa2);
	val &= ~0xc0;
	val &= 0xdf; /* Normal mode */
	m88rs6000_writereg(state, 0xa2, val);
	/* DiSEqC burst */
	if (burst == SEC_MINI_B)
		m88rs6000_writereg(state, 0xa1, 0x01);
	else
		m88rs6000_writereg(state, 0xa1, 0x02);

	msleep(13);

	time_out = 5;
	do{
		val = m88rs6000_readreg(state, 0xa1);
		if ((val & 0x40) == 0)
			break;
		msleep(1);
		time_out --;
	} while (time_out > 0);

	val = m88rs6000_readreg(state, 0xa2);
	val &= ~0xc0;
	val |= 0x80;
	m88rs6000_writereg(state, 0xa2, val);
	
	return 0;
}

static void m88rs6000_release(struct dvb_frontend *fe)
{
	struct m88rs6000_state *state = fe->demodulator_priv;

	dprintk("%s\n", __func__);
	kfree(state);
}

static int m88rs6000_check_id(struct m88rs6000_state *state)
{
	int val_00, val_01, val_02;
	
	/*check demod id*/
	val_00 = m88rs6000_readreg(state, 0x00);
	val_01 = m88rs6000_readreg(state, 0x01);
	val_02 = m88rs6000_readreg(state, 0x02);
	printk(KERN_INFO "RS6000 chip, demod id=%x, version=%x.\n", val_00, (val_02 << 8 | val_01));

	val_01 = m88rs6000_tuner_readreg(state, 0x01);
	printk(KERN_INFO "RS6000 chip, tuner id=%x.\n", val_01);
				
	state->demod_id = 0;
	if(val_00 == 0xE8) {
		state->demod_id = RS6000_ID;
	}					
	
	return state->demod_id;	
}

static struct dvb_frontend_ops m88rs6000_ops;
static int m88rs6000_initilaze(struct dvb_frontend *fe);

struct dvb_frontend *dvbsky_m88rs6000_attach(const struct dvbsky_m88rs6000_config *config,
				    struct i2c_adapter *i2c)
{
	struct m88rs6000_state *state = NULL;

	dprintk("%s\n", __func__);

	/* allocate memory for the internal state */
	state = kzalloc(sizeof(struct m88rs6000_state), GFP_KERNEL);
	if (state == NULL) {
		printk(KERN_ERR "Unable to kmalloc\n");
		goto error2;
	}

	state->config = config;
	state->i2c = i2c;
	state->preBer = 0x0;
	state->delivery_system = SYS_DVBS; /*Default to DVB-S.*/
	state->iMclkKHz = 96000;
	
	memcpy(&state->frontend.ops, &m88rs6000_ops,
			sizeof(struct dvb_frontend_ops));
	state->frontend.demodulator_priv = state;

	/* check demod id */
	if(m88rs6000_initilaze(&state->frontend)){
		printk(KERN_ERR "Unable to find Montage RS6000.\n");
		goto error3;
	}
			
	return &state->frontend;

error3:
	kfree(state);
error2:
	return NULL;
}
EXPORT_SYMBOL(dvbsky_m88rs6000_attach);

static int m88rs6000_tuner_set_pll_freq(struct m88rs6000_state *state, u32 tuner_freq_MHz)
{
	u32 fcry_KHz, ulNDiv1, ulNDiv2, ulNDiv;
	u8  refDiv, ucLoDiv1, ucLomod1, ucLoDiv2, ucLomod2, ucLoDiv, ucLomod;
	u8 reg27, reg29, reg2d, reg2e, reg36, reg42, reg42buf, reg83;
	
	fcry_KHz = MT_FE_CRYSTAL_KHZ;
	refDiv = 27;
	reg36 = refDiv - 8;

	m88rs6000_tuner_writereg(state, 0x36, reg36);
	m88rs6000_tuner_writereg(state, 0x31, 0x00);

	if(reg36 == 19) {
		m88rs6000_tuner_writereg(state, 0x2c, 0x02);
	} else {
		m88rs6000_tuner_writereg(state, 0x2c, 0x00);
	}

	if(tuner_freq_MHz >= 1550) {
		ucLoDiv1 = 2;
		ucLomod1 = 0;
		ucLoDiv2 = 2;
		ucLomod2 = 0;
	} else if(tuner_freq_MHz >= 1380) {
		ucLoDiv1 = 3;
		ucLomod1 = 16;
		ucLoDiv2 = 2;
		ucLomod2 = 0;
	} else if(tuner_freq_MHz >= 1070) {
		ucLoDiv1 = 3;
		ucLomod1 = 16;
		ucLoDiv2 = 3;
		ucLomod2 = 16;
	} else if(tuner_freq_MHz >= 1000) {
		ucLoDiv1 = 3;
		ucLomod1 = 16;
		ucLoDiv2 = 4;
		ucLomod2 = 64;
	} else if(tuner_freq_MHz >= 775) {
		ucLoDiv1 = 4;
		ucLomod1 = 64;
		ucLoDiv2 = 4;
		ucLomod2 = 64;
	} else if(tuner_freq_MHz >= 700) {
		ucLoDiv1 = 6;
		ucLomod1 = 48;
		ucLoDiv2 = 4;
		ucLomod2 = 64;
	} else if(tuner_freq_MHz >= 520) {
		ucLoDiv1 = 6;
		ucLomod1 = 48;
		ucLoDiv2 = 6;
		ucLomod2 = 48;
	} else {
		ucLoDiv1 = 8;
		ucLomod1 = 96;
		ucLoDiv2 = 8;
		ucLomod2 = 96;
	}

	ulNDiv1 = ((tuner_freq_MHz * ucLoDiv1 * 1000) * refDiv / fcry_KHz - 1024) / 2;
	ulNDiv2 = ((tuner_freq_MHz * ucLoDiv2 * 1000) * refDiv / fcry_KHz - 1024) / 2;

	reg27 = (((ulNDiv1 >> 8) & 0x0F) + ucLomod1) & 0x7F;
	m88rs6000_tuner_writereg(state, 0x27, reg27);
	m88rs6000_tuner_writereg(state, 0x28, (u8)(ulNDiv1 & 0xFF));
	reg29 = (((ulNDiv2 >> 8) & 0x0F) + ucLomod2) & 0x7f;
	m88rs6000_tuner_writereg(state, 0x29, reg29);
	m88rs6000_tuner_writereg(state, 0x2a, (u8)(ulNDiv2 & 0xFF));

	m88rs6000_tuner_writereg(state, 0x2F, 0xf5);
	m88rs6000_tuner_writereg(state, 0x30, 0x05);

	m88rs6000_tuner_writereg(state, 0x08, 0x1f);
	m88rs6000_tuner_writereg(state, 0x08, 0x3f);
	m88rs6000_tuner_writereg(state, 0x09, 0x20);
	m88rs6000_tuner_writereg(state, 0x09, 0x00);

	m88rs6000_tuner_writereg(state, 0x3e, 0x11);

	m88rs6000_tuner_writereg(state, 0x08, 0x2f);
	m88rs6000_tuner_writereg(state, 0x08, 0x3f);
	m88rs6000_tuner_writereg(state, 0x09, 0x10);
	m88rs6000_tuner_writereg(state, 0x09, 0x00);
	msleep(2);

	reg42 = m88rs6000_tuner_readreg(state, 0x42);

	m88rs6000_tuner_writereg(state, 0x3e, 0x10);
	m88rs6000_tuner_writereg(state, 0x08, 0x2f);
	m88rs6000_tuner_writereg(state, 0x08, 0x3f);
	m88rs6000_tuner_writereg(state, 0x09, 0x10);
	m88rs6000_tuner_writereg(state, 0x09, 0x00);
	msleep(2);
	reg42buf = m88rs6000_tuner_readreg(state, 0x42);
	if(reg42buf < reg42)
		m88rs6000_tuner_writereg(state, 0x3e, 0x11);
	msleep(5);

	reg2d = m88rs6000_tuner_readreg(state, 0x2d);
	m88rs6000_tuner_writereg(state, 0x2d, reg2d);

	reg2e = m88rs6000_tuner_readreg(state, 0x2e);
	m88rs6000_tuner_writereg(state, 0x2e, reg2e);
		
	reg27 = m88rs6000_tuner_readreg(state, 0x27);		
	reg27 = reg27 & 0x70;
	reg83 = m88rs6000_tuner_readreg(state, 0x83);			
	reg83 = reg83 & 0x70;

	if(reg27 == reg83) {
		ucLoDiv	= ucLoDiv1;
		ulNDiv = ulNDiv1;
		ucLomod = ucLomod1 / 16;
	} else {
		ucLoDiv	= ucLoDiv2;
		ulNDiv = ulNDiv2;
		ucLomod = ucLomod2 / 16;
	}

	if ((ucLoDiv == 3) || (ucLoDiv == 6)) {
		refDiv = 18;
		reg36 = refDiv - 8;
		m88rs6000_tuner_writereg(state, 0x36, reg36);
		ulNDiv = ((tuner_freq_MHz * ucLoDiv * 1000) * refDiv / fcry_KHz - 1024) / 2;
	}

	reg27 = (0x80 + ((ucLomod << 4) & 0x70) + ((ulNDiv >> 8) & 0x0F)) & 0xFF;
	m88rs6000_tuner_writereg(state, 0x27, reg27);
	m88rs6000_tuner_writereg(state, 0x28, (u8)(ulNDiv & 0xFF));
	m88rs6000_tuner_writereg(state, 0x29, 0x80);
	m88rs6000_tuner_writereg(state, 0x31, 0x03);

	if (ucLoDiv == 3) {
		m88rs6000_tuner_writereg(state, 0x3b, 0xCE);
	} else {
		m88rs6000_tuner_writereg(state, 0x3b, 0x8A);
	}

	//tuner_lo_freq_KHz = fcry_KHz* (ulNDiv * 2 + 1024) / refDiv / ucLoDiv;
	return 0;
}

static int m88rs6000_tuner_set_bb(struct m88rs6000_state *state, u32 symbol_rate_KSs, s32 lpf_offset_KHz)
{
	u32 f3dB;
	u8  reg40;

	f3dB = symbol_rate_KSs * 9 / 14 + 2000;
	f3dB += lpf_offset_KHz;
	if(f3dB < 6000) f3dB = 6000;
	if(f3dB > 43000) f3dB = 43000;
	reg40 = f3dB / 1000;
	m88rs6000_tuner_writereg(state, 0x40, reg40);
	return 0;
}

static int m88rs6000_set_carrier_offset(struct dvb_frontend *fe,
					s32 carrier_offset_khz)
{
	struct m88rs6000_state *state = fe->demodulator_priv;
	s32 tmp;

	tmp = carrier_offset_khz;
	tmp *= 65536;
	
	//tmp = (2*tmp + MT_FE_MCLK_KHZ) / (2*MT_FE_MCLK_KHZ);
	tmp = (2*tmp + state->iMclkKHz) / (2*state->iMclkKHz);

	if (tmp < 0)
		tmp += 65536;

	m88rs6000_writereg(state, 0x5f, tmp >> 8);
	m88rs6000_writereg(state, 0x5e, tmp & 0xff);

	return 0;
}

static int m88rs6000_set_symrate(struct dvb_frontend *fe)
{
	struct m88rs6000_state *state = fe->demodulator_priv;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	u16 value;
	
	//value = (((c->symbol_rate / 1000) << 15) + (MT_FE_MCLK_KHZ / 4)) / (MT_FE_MCLK_KHZ / 2);
	value = (((c->symbol_rate / 1000) << 15) + (state->iMclkKHz / 4)) / (state->iMclkKHz / 2);
	m88rs6000_writereg(state, 0x61, value & 0x00ff);
	m88rs6000_writereg(state, 0x62, (value & 0xff00) >> 8);

	return 0;
}

static int m88rs6000_set_CCI(struct dvb_frontend *fe)
{
	struct m88rs6000_state *state = fe->demodulator_priv;
	u8 tmp;

	tmp = m88rs6000_readreg(state, 0x56);
	tmp &= ~0x01;
	m88rs6000_writereg(state, 0x56, tmp);

	tmp = m88rs6000_readreg(state, 0x76);
	tmp &= ~0x80;
	m88rs6000_writereg(state, 0x76, tmp);

	return 0;
}

static int m88rs6000_init_reg(struct m88rs6000_state *state, const u8 *p_reg_tab, u32 size)
{
	u32 i;
	
	for(i = 0; i < size; i+=2)
		m88rs6000_writereg(state, p_reg_tab[i], p_reg_tab[i+1]);
		
	return 0;
}

static int  m88rs6000_get_ts_mclk(struct m88rs6000_state *state, u32 *p_MCLK_KHz)
{
	u8 reg15, reg16, reg1D, reg1E, reg1F;
	u8 sm, f0, f1, f2, f3, pll_ldpc_mode;
	u16 pll_div_fb, N;
	u32 MCLK_KHz;

	*p_MCLK_KHz = MT_FE_MCLK_KHZ;

	reg15 = m88rs6000_tuner_readreg(state, 0x15);
	reg16 = m88rs6000_tuner_readreg(state, 0x16);
	reg1D = m88rs6000_tuner_readreg(state, 0x1D);
	reg1E = m88rs6000_tuner_readreg(state, 0x1E);
	reg1F = m88rs6000_tuner_readreg(state, 0x1F);
	
	pll_ldpc_mode = (reg15 >> 1) & 0x01;

	MCLK_KHz = 9000;

	pll_div_fb = reg15 & 0x01;
	pll_div_fb <<= 8;
	pll_div_fb += reg16;

	MCLK_KHz *= (pll_div_fb + 32);

	sm = reg1D & 0x03;

	f3 = (reg1E >> 4) & 0x0F;
	f2 = reg1E & 0x0F;
	f1 = (reg1F >> 4) & 0x0F;
	f0 = reg1F & 0x0F;

	if(f3 == 0) f3 = 16;
	if(f2 == 0) f2 = 16;
	if(f1 == 0) f1 = 16;
	if(f0 == 0) f0 = 16;

	N = f2 + f1;

	switch(sm) {
		case 3:
			N = f3 + f2 + f1 + f0;
			break;
		case 2:
			N = f2 + f1 + f0;
			break;
		case 1:
		case 0:
		default:
			N = f2 + f1;
			break;
	}

	MCLK_KHz *= 4;
	MCLK_KHz /= N;
	*p_MCLK_KHz = MCLK_KHz;
	
	dprintk("%s(), mclk=%d.\n", __func__, MCLK_KHz);

	return 0;
}

static int  m88rs6000_set_ts_mclk(struct m88rs6000_state *state, u32 MCLK_KHz, u32 iSymRateKSs)
{
	u8 reg15, reg16, reg1D, reg1E, reg1F, tmp;
	u8 sm, f0 = 0, f1 = 0, f2 = 0, f3 = 0;
	u16 pll_div_fb, N;
	u32 div;

	dprintk("%s(), mclk=%d, symbol rate=%d KSs.\n", __func__, MCLK_KHz, iSymRateKSs);
	
	reg15 = m88rs6000_tuner_readreg(state, 0x15);
	reg16 = m88rs6000_tuner_readreg(state, 0x16);
	reg1D = m88rs6000_tuner_readreg(state, 0x1D);

	if(state->config->ts_mode == 0) {
		if(reg16 == 92)
			tmp = 93;
		else if (reg16 == 100)
			tmp = 99;
		else
			tmp = 96;
		MCLK_KHz *= tmp;
		MCLK_KHz /= 96;
	}

	pll_div_fb = (reg15 & 0x01) << 8;
	pll_div_fb += reg16;
	pll_div_fb += 32;

	div = 9000 * pll_div_fb * 4;
	div /= MCLK_KHz;

	if(div <= 32) {
		N = 2;
		f0 = 0;
		f1 = div / N;
		f2 = div - f1;
		f3 = 0;
	} else if (div <= 34) {
		N = 3;
		f0 = div / N;
		f1 = (div - f0) / (N - 1);
		f2 = div - f0 - f1;
		f3 = 0;
	} else if (div <= 64) {
		N = 4;
		f0 = div / N;
		f1 = (div - f0) / (N - 1);
		f2 = (div - f0 - f1) / (N - 2);
		f3 = div - f0 - f1 - f2;
	} else {
		N = 4;
		f0 = 16;
		f1 = 16;
		f2 = 16;
		f3 = 16;
	}

	if(state->config->ts_mode == 1) {
		if(f0 == 16)
			f0 = 0;
		else if((f0 < 8) && (f0 != 0))
			f0 = 8;

		if(f1 == 16)
			f1 = 0;
		else if((f1 < 8) && (f1 != 0))
			f1 = 8;

		if(f2 == 16)
			f2 = 0;
		else if((f2 < 8) && (f2 != 0))
			f2 = 8;

		if(f3 == 16)
			f3 = 0;
		else if((f3 < 8) && (f3 != 0))
			f3 = 8;
	} else {
		if(f0 == 16)
			f0 = 0;
		else if((f0 < 9) && (f0 != 0))
			f0 = 9;

		if(f1 == 16)
			f1 = 0;
		else if((f1 < 9) && (f1 != 0))
			f1 = 9;

		if(f2 == 16)
			f2 = 0;
		else if((f2 < 9) && (f2 != 0))
			f2 = 9;

		if(f3 == 16)
			f3 = 0;
		else if((f3 < 9) && (f3 != 0))
			f3 = 9;
	}

	sm = N - 1;
	reg1D &= ~0x03;
	reg1D |= sm;
	reg1E = ((f3 << 4) + f2) & 0xFF;
	reg1F = ((f1 << 4) + f0) & 0xFF;

	m88rs6000_tuner_writereg(state, 0x05, 0x40);
	m88rs6000_tuner_writereg(state, 0x11, 0x08);

	m88rs6000_tuner_writereg(state, 0x1D, reg1D);
	m88rs6000_tuner_writereg(state, 0x1E, reg1E);
	m88rs6000_tuner_writereg(state, 0x1F, reg1F);
	m88rs6000_tuner_writereg(state, 0x17, 0xc1);
	m88rs6000_tuner_writereg(state, 0x17, 0x81);
	msleep(5);

	m88rs6000_tuner_writereg(state, 0x05, 0x00);
	m88rs6000_tuner_writereg(state, 0x11, (iSymRateKSs > 45010) ? 0x0E : 0x0A);
	msleep(5);

	return 0;
}

static int  m88rs6000_set_ts_divide_ratio(struct m88rs6000_state *state, u8 dr_high, u8 dr_low)
{
	u8 val, tmp1, tmp2;

	tmp1 = dr_high;
	tmp2 = dr_low;

	tmp1 -= 1;
	tmp2 -= 1;

	tmp1 &= 0x3f;
	tmp2 &= 0x3f;

	val = m88rs6000_readreg(state, 0xfe); 
	val &= 0xF0;
	val |= (tmp1 >> 2) & 0x0f;
	m88rs6000_writereg(state, 0xfe, val);

	val = (u8)((tmp1 & 0x03) << 6);
	val |= tmp2;
	m88rs6000_writereg(state, 0xea, val);

	return 0;
}

static int m88rs6000_demod_connect(struct dvb_frontend *fe, s32 carrier_offset_khz) 
{
	struct m88rs6000_state *state = fe->demodulator_priv;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	
	u8 tmp, tmp1, tmp2;
	u16 divide_ratio;
	u32 target_mclk = MT_FE_MCLK_KHZ, ts_clk;
	
	dprintk("connect delivery system = %d\n", state->delivery_system);
	
	/* rs6000 build-in uC reset */
	m88rs6000_writereg(state, 0xb2, 0x01);
	/* rs6000 software reset */
	m88rs6000_writereg(state, 0x00, 0x01);

	switch (state->delivery_system) {
	case SYS_DVBS:
		/* initialise the demod in DVB-S mode */
		m88rs6000_init_reg(state, rs6000_dvbs_init_tab, sizeof(rs6000_dvbs_init_tab));
		m88rs6000_writereg(state, 0x4d, 0xfd & m88rs6000_readreg(state, 0x4d));
		m88rs6000_writereg(state, 0x30, 0xef & m88rs6000_readreg(state, 0x30));
		m88rs6000_writereg(state, 0x29, 0x10 | m88rs6000_readreg(state, 0x29));
		
		target_mclk = 96000;
		break;
	case SYS_DVBS2:
		m88rs6000_init_reg(state, rs6000_dvbs2_init_tab, sizeof(rs6000_dvbs2_init_tab));
		m88rs6000_writereg(state, 0x4d, 0xfd & m88rs6000_readreg(state, 0x4d));
		m88rs6000_writereg(state, 0x30, 0xef & m88rs6000_readreg(state, 0x30));
		m88rs6000_writereg(state, 0x29, 0xef & m88rs6000_readreg(state, 0x29));
		
		if(state->config->ts_mode == 1) {
			target_mclk = 96000;
		} else {
			target_mclk = 144000;
		}
		
		if((c->symbol_rate / 1000 ) <= 5000) {
			m88rs6000_writereg(state, 0xc0, 0x04);
			m88rs6000_writereg(state, 0x8a, 0x09);
			m88rs6000_writereg(state, 0x8b, 0x22);
			m88rs6000_writereg(state, 0x8c, 0x88);
		}		
		break;
	default:
		return 1;
	}
		
	/* set ts clock */
	if(state->config->ci_mode == 0)
		ts_clk = 16000;
	else
		ts_clk = 8000;
		
	m88rs6000_writereg(state, 0x06, 0xe0);
	m88rs6000_set_ts_mclk(state, target_mclk, c->symbol_rate / 1000);
	m88rs6000_writereg(state, 0x06, 0x00);

	m88rs6000_writereg(state, 0x9d, 0x08 | m88rs6000_readreg(state, 0x9d));	
	m88rs6000_writereg(state, 0x30, 0x80 | m88rs6000_readreg(state, 0x30));
	
	m88rs6000_get_ts_mclk(state, &target_mclk);
	
	divide_ratio = (target_mclk + ts_clk - 1) / ts_clk;
	if(divide_ratio > 128)
		divide_ratio = 128;
	if(divide_ratio < 2)
		divide_ratio = 2;
	tmp1 = (u8)(divide_ratio / 2);
	tmp2 = (u8)(divide_ratio / 2);
	if((divide_ratio % 2) != 0)
		tmp2 += 1;
	
	m88rs6000_set_ts_divide_ratio(state, tmp1, tmp2);	
		
	/* set ts pins.*/
	if(state->config->ci_mode){
		if(state->config->ci_mode == 2)
			tmp = 0x43;
		else
			tmp = 0x03;
	}else if(state->config->ts_mode)
		tmp = 0x06;
	else
		tmp = 0x02;
	m88rs6000_writereg(state, 0xfd, tmp);
	
	/* set others.*/
	tmp = m88rs6000_readreg(state, 0xca);
	tmp &= 0xFE;
	tmp |= (m88rs6000_readreg(state, 0xca) >> 3) & 0x01;
	m88rs6000_writereg(state, 0xca, tmp);
	
	m88rs6000_writereg(state, 0x33, 0x99);
	
	/* enable ac coupling */
	m88rs6000_writereg(state, 0x25, 0x08 | m88rs6000_readreg(state, 0x25));
	m88rs6000_writereg(state, 0xC9, 0x08 | m88rs6000_readreg(state, 0xC9));

	if ((c->symbol_rate / 1000) <= 3000){
		m88rs6000_writereg(state, 0xc3, 0x08); /* 8 * 32 * 100 / 64 = 400*/
		m88rs6000_writereg(state, 0xc8, 0x20);
		m88rs6000_writereg(state, 0xc4, 0x08); /* 8 * 0 * 100 / 128 = 0*/
		m88rs6000_writereg(state, 0xc7, 0x00);
	}else if((c->symbol_rate / 1000) <= 10000){
		m88rs6000_writereg(state, 0xc3, 0x08); /* 8 * 16 * 100 / 64 = 200*/
		m88rs6000_writereg(state, 0xc8, 0x10);
		m88rs6000_writereg(state, 0xc4, 0x08); /* 8 * 0 * 100 / 128 = 0*/
		m88rs6000_writereg(state, 0xc7, 0x00);
	}else{
		m88rs6000_writereg(state, 0xc3, 0x08); /* 8 * 6 * 100 / 64 = 75*/
		m88rs6000_writereg(state, 0xc8, 0x06);
		m88rs6000_writereg(state, 0xc4, 0x08); /* 8 * 0 * 100 / 128 = 0*/
		m88rs6000_writereg(state, 0xc7, 0x00);
	}
	
	m88rs6000_set_symrate(fe);
	
	m88rs6000_set_CCI(fe);

	m88rs6000_set_carrier_offset(fe, carrier_offset_khz);
		
	/* rs6000 out of software reset */
	m88rs6000_writereg(state, 0x00, 0x00);
	/* start rs6000 build-in uC */
	m88rs6000_writereg(state, 0xb2, 0x00);	
	
	return 0;
}

static int  m88rs6000_select_mclk(struct m88rs6000_state *state, u32 tuner_freq_MHz, u32 iSymRateKSs)
{
	u32 adc_Freq_MHz[3] = {96, 93, 99};
	u8  reg16_list[3] = {96, 92, 100}, reg16, reg15;
	u32 offset_MHz[3];
	u32 max_offset = 0;
	u8 i;
	u8 big_symbol = (iSymRateKSs > 45010) ? 1 : 0;

	if(big_symbol) {
		reg16 = 115;
		state->iMclkKHz = 110250;
	} else {
		reg16 = 96;
		for(i = 0; i < 3; i++) {
			offset_MHz[i] = tuner_freq_MHz % adc_Freq_MHz[i];

			if(offset_MHz[i] > (adc_Freq_MHz[i] / 2))
				offset_MHz[i] = adc_Freq_MHz[i] - offset_MHz[i];

			if(offset_MHz[i] > max_offset) {
				max_offset = offset_MHz[i];
				reg16 = reg16_list[i];
				state->iMclkKHz = adc_Freq_MHz[i] * 1000;
			}
		}
	}
	switch(state->iMclkKHz) {
		case 93000:
			m88rs6000_writereg(state, 0xa0, 0x42);
			break;
		case 96000:
			m88rs6000_writereg(state, 0xa0, 0x44);
			break;
		case 99000:
			m88rs6000_writereg(state, 0xa0, 0x46);
			break;
		case 110250:
			m88rs6000_writereg(state, 0xa0, 0x4e);
			break;
		default:
			m88rs6000_writereg(state, 0xa0, 0x44);
			break;
	}
	reg15 = m88rs6000_tuner_readreg(state, 0x15);
	m88rs6000_tuner_writereg(state, 0x05, 0x40);
	m88rs6000_tuner_writereg(state, 0x11, 0x08);
	if(big_symbol)
		reg15 |= 0x02;
	else
		reg15 &= ~0x02;
	m88rs6000_tuner_writereg(state, 0x15, reg15);
	m88rs6000_tuner_writereg(state, 0x16, reg16);
	msleep(5);
	m88rs6000_tuner_writereg(state, 0x05, 0x00);
	m88rs6000_tuner_writereg(state, 0x11, (big_symbol) ? 0x0E : 0x0A);
	msleep(5);
	return 0;
}

static int m88rs6000_set_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters* params)
{
	struct m88rs6000_state *state = fe->demodulator_priv;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;

	int i;	
	u32 target_mclk = 144000;
	s32 lpf_offset_KHz;
	u32 realFreq, freq_MHz;
	fe_status_t status;

	dprintk("%s() ", __func__);
	dprintk("c frequency = %d KHz\n", c->frequency);
	dprintk("symbol rate = %d\n", c->symbol_rate);
	dprintk("delivery system = %d\n", c->delivery_system);
	
	state->delivery_system = c->delivery_system;
	if( state->delivery_system == SYS_DVBS )
		target_mclk = 96000;
	
	realFreq = c->frequency;
	lpf_offset_KHz = 0;
	if(c->symbol_rate < 5000000){
		lpf_offset_KHz = FREQ_OFFSET_AT_SMALL_SYM_RATE_KHz;
		realFreq += FREQ_OFFSET_AT_SMALL_SYM_RATE_KHz;
	}
	
	/* set mclk.*/
	m88rs6000_writereg(state, 0x06, 0xe0);
	m88rs6000_select_mclk(state, realFreq / 1000, c->symbol_rate / 1000);
	m88rs6000_set_ts_mclk(state, target_mclk, c->symbol_rate / 1000);
	m88rs6000_writereg(state, 0x06, 0x00);
	msleep(10);
	
	/* set tuner pll.*/
	freq_MHz = (realFreq + 500) / 1000;
	m88rs6000_tuner_set_pll_freq(state, freq_MHz);
	m88rs6000_tuner_set_bb(state, c->symbol_rate / 1000, lpf_offset_KHz);
	m88rs6000_tuner_writereg(state, 0x00, 0x01);
	m88rs6000_tuner_writereg(state, 0x00, 0x00);	
	
	/* start demod to lock */	
	m88rs6000_demod_connect(fe, lpf_offset_KHz);	
		
	/* check lock status.*/
	for (i = 0; i < 30 ; i++) {
		m88rs6000_read_status(fe, &status);
		if (status & FE_HAS_LOCK)
			break;
		msleep(20);
	}
	
	if (status & FE_HAS_LOCK){
		if (state->config->set_ts_params)
			state->config->set_ts_params(fe, 0);
	}
		
	return 0;
}

static int m88rs6000_tune(struct dvb_frontend *fe,
			struct dvb_frontend_parameters* params,
			unsigned int mode_flags,
			unsigned int *delay,
			fe_status_t *status)
{	
	*delay = HZ / 5;
	
	dprintk("%s() ", __func__);
	dprintk("re_tune = %d\n", params ? 1 : 0);
	
	if (params) {
		int ret = m88rs6000_set_frontend(fe, params);
		if (ret)
			return ret;
	}
	
	return m88rs6000_read_status(fe, status);
}

static enum dvbfe_algo m88rs6000_get_algo(struct dvb_frontend *fe)
{
	return DVBFE_ALGO_HW;
}

/*
 * Initialise or wake up device
 */
static int m88rs6000_initfe(struct dvb_frontend *fe)
{
	struct m88rs6000_state *state = fe->demodulator_priv;

	dprintk("%s()\n", __func__);

	/* 1st step to wake up demod */
	m88rs6000_writereg(state, 0x04, 0xfe & m88rs6000_readreg(state, 0x04));
	m88rs6000_writereg(state, 0x23, 0xef & m88rs6000_readreg(state, 0x23));
	
	/* 2nd step to wake up tuner */	
	m88rs6000_tuner_writereg(state, 0x11, 0x08 | m88rs6000_tuner_readreg(state, 0x11));
	msleep(5);
	m88rs6000_tuner_writereg(state, 0x10, 0x01 | m88rs6000_tuner_readreg(state, 0x10));
	msleep(10);
	m88rs6000_tuner_writereg(state, 0x07, 0x7d);
	
	m88rs6000_writereg(state, 0x08, 0x01 | m88rs6000_readreg(state, 0x08));
	m88rs6000_writereg(state, 0x29, 0x01 | m88rs6000_readreg(state, 0x29));
		
	return 0;	
}

/* Put device to sleep */
static int m88rs6000_sleep(struct dvb_frontend *fe)
{
	struct m88rs6000_state *state = fe->demodulator_priv;

	dprintk("%s()\n", __func__);
	
	m88rs6000_writereg(state, 0x29, 0xfe & m88rs6000_readreg(state, 0x29));
	m88rs6000_writereg(state, 0x08, 0xfe & m88rs6000_readreg(state, 0x08));
	
	/* 1st step to sleep tuner */
	m88rs6000_tuner_writereg(state, 0x07, 0x6d);
	m88rs6000_tuner_writereg(state, 0x10, 0xfe & m88rs6000_tuner_readreg(state, 0x10));
	m88rs6000_tuner_writereg(state, 0x11, 0xf7 & m88rs6000_tuner_readreg(state, 0x11));
	msleep(5);
	
	/* 2nd step to sleep demod */	
	m88rs6000_writereg(state, 0x04, 0x01 | m88rs6000_readreg(state, 0x04));
	m88rs6000_writereg(state, 0x23, 0x10 | m88rs6000_readreg(state, 0x23));
	
	return 0;
}

 
 /*
 * Power config will reset and load initial firmware if required
 */
static int m88rs6000_initilaze(struct dvb_frontend *fe)
{
	struct m88rs6000_state *state = fe->demodulator_priv;
	int ret;
	u8 val;

	dprintk("%s()\n", __func__);
	
	/* Use 0x21 for tuner address since 0x20 is used by TPM. */
	m88rs6000_writereg(state, 0x29, 0x7f & m88rs6000_readreg(state, 0x29));
	state->tuner_addr = 0x21;
	
	m88rs6000_initfe(fe);
	
	m88rs6000_tuner_writereg(state, 0x04, 0x01);
	
	if(m88rs6000_check_id(state) != RS6000_ID)
		return 1;
	
	/* hard reset */	
	val = m88rs6000_readreg(state, 0x08);
	val &= 0xfe;
	m88rs6000_writereg(state, 0x08, val);
	m88rs6000_writereg(state, 0x07, 0x80);
	m88rs6000_writereg(state, 0x07, 0x00);
	m88rs6000_writereg(state, 0xb2, 0x00);
	val |= 0x01;
	m88rs6000_writereg(state, 0x08, val);
	msleep(1);	
	m88rs6000_writereg(state, 0x08, 0x01 | m88rs6000_readreg(state, 0x08));	
	msleep(1);

	/* tuner init. */
	m88rs6000_tuner_writereg(state, 0x05, 0x40);
	m88rs6000_tuner_writereg(state, 0x11, 0x08);
	m88rs6000_tuner_writereg(state, 0x15, 0x6c);
	m88rs6000_tuner_writereg(state, 0x17, 0xc1);
	m88rs6000_tuner_writereg(state, 0x17, 0x81);
	msleep(10);
	m88rs6000_tuner_writereg(state, 0x05, 0x00);
	m88rs6000_tuner_writereg(state, 0x11, 0x0a);
	
	/* set tuner to normal state.*/
	m88rs6000_tuner_writereg(state, 0x11, 0x08 | m88rs6000_tuner_readreg(state, 0x11));
	msleep(5);
	m88rs6000_tuner_writereg(state, 0x10, 0x01 | m88rs6000_tuner_readreg(state, 0x10));
	msleep(10);
	m88rs6000_tuner_writereg(state, 0x07, 0x7d);
	
	/*disable tuner clock output.*/
	m88rs6000_tuner_writereg(state, 0x10, 0xfb);
	m88rs6000_tuner_writereg(state, 0x24, 0x38);
		
	m88rs6000_tuner_writereg(state, 0x11, 0x0a);
	m88rs6000_tuner_writereg(state, 0x12, 0x00);
	m88rs6000_tuner_writereg(state, 0x2b, 0x1c);
	m88rs6000_tuner_writereg(state, 0x44, 0x48);
	m88rs6000_tuner_writereg(state, 0x54, 0x24);
	m88rs6000_tuner_writereg(state, 0x55, 0x06);
	m88rs6000_tuner_writereg(state, 0x59, 0x00);
	m88rs6000_tuner_writereg(state, 0x5b, 0x4c);
	m88rs6000_tuner_writereg(state, 0x60, 0x8b);
	m88rs6000_tuner_writereg(state, 0x61, 0xf4);
	m88rs6000_tuner_writereg(state, 0x65, 0x07);
	m88rs6000_tuner_writereg(state, 0x6d, 0x6f);
	m88rs6000_tuner_writereg(state, 0x6e, 0x31);
		
	m88rs6000_tuner_writereg(state, 0x3c, 0xf3);
	m88rs6000_tuner_writereg(state, 0x37, 0x0f);

	m88rs6000_tuner_writereg(state, 0x48, 0x28);
	m88rs6000_tuner_writereg(state, 0x49, 0xd8);

	m88rs6000_tuner_writereg(state, 0x70, 0x66);
	m88rs6000_tuner_writereg(state, 0x71, 0xCF);
	m88rs6000_tuner_writereg(state, 0x72, 0x81);
	m88rs6000_tuner_writereg(state, 0x73, 0xA7);
	m88rs6000_tuner_writereg(state, 0x74, 0x4F);
	m88rs6000_tuner_writereg(state, 0x75, 0xFC);
	
	/* demod reset.*/
	m88rs6000_writereg(state, 0x07, 0xE0);
	m88rs6000_writereg(state, 0x07, 0x00);
	
	/* Load the firmware if required */
	ret = m88rs6000_load_firmware(fe);
	if (ret != 0){
		printk(KERN_ERR "%s: Unable download firmware\n", __func__);
		return ret;
	}
	
	m88rs6000_writereg(state, 0x4d, 0xfd & m88rs6000_readreg(state, 0x4d));
	m88rs6000_writereg(state, 0x30, 0xef & m88rs6000_readreg(state, 0x30));

	m88rs6000_writereg(state, 0xf1, 0x01);

	m88rs6000_writereg(state, 0x29, 0xbf & m88rs6000_readreg(state, 0x29));
	m88rs6000_writereg(state, 0x9d, 0x08 | m88rs6000_readreg(state, 0x9d));
	
	return 0;
}

static struct dvb_frontend_ops m88rs6000_ops = {
	.info = {
		.name = "Montage RS6000(DVBSky)",
		.type = FE_QPSK,
		.frequency_min = 950000,
		.frequency_max = 2150000,
		.frequency_stepsize = 1011, /* kHz for QPSK frontends */
		.frequency_tolerance = 5000,
		.symbol_rate_min = 1000000,
		.symbol_rate_max = 45000000,
		.caps = FE_CAN_INVERSION_AUTO |
			FE_CAN_FEC_1_2 | FE_CAN_FEC_2_3 | FE_CAN_FEC_3_4 |
			FE_CAN_FEC_4_5 | FE_CAN_FEC_5_6 | FE_CAN_FEC_6_7 |
			FE_CAN_FEC_7_8 | FE_CAN_FEC_AUTO |
			FE_CAN_2G_MODULATION |
			FE_CAN_QPSK | FE_CAN_RECOVER
	},

	.release = m88rs6000_release,
	.init = m88rs6000_initfe,
	.sleep = m88rs6000_sleep,
	.read_status = m88rs6000_read_status,
	.read_ber = m88rs6000_read_ber,
	.read_signal_strength = m88rs6000_read_signal_strength,
	.read_snr = m88rs6000_read_snr,
	.read_ucblocks = m88rs6000_read_ucblocks,
	.set_tone = m88rs6000_set_tone,
	.set_voltage = m88rs6000_set_voltage,
	.diseqc_send_master_cmd = m88rs6000_send_diseqc_msg,
	.diseqc_send_burst = m88rs6000_diseqc_send_burst,
	.get_frontend_algo = m88rs6000_get_algo,
	.tune = m88rs6000_tune,
	.set_frontend = m88rs6000_set_frontend,
};

MODULE_DESCRIPTION("DVB Frontend module for Montage M88RS6000");
MODULE_AUTHOR("Max nibble");
MODULE_LICENSE("GPL");
