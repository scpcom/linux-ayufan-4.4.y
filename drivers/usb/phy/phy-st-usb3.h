/*
 * Copyright (C) 2015 STMicroelectronics
 *
 * Author: Aymen BOUATTAY <aymen.bouattay@st.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 */

#ifndef __PHY_ST_USB3_H
#define __PHY_ST_USB3_H

#define phy_to_priv(x)	container_of((x), struct sti_usb3_miphy, phy)

#define SSC_ON	0x11
#define SSC_OFF	0x01
#define SSC_FREQ_KHZ	31
#define SSC_AMP_PPM	4000
#define USB3FVCO_GHZ	5
#define USB3FVCO_MHZ	(USB3FVCO_GHZ * 1000)
#define USB3FVCO_KHZ	(USB3FVCO_MHZ * 1000)

#define MIPHY_PLL_RATIO(n, m) \
	(1024 * (8 * (USB3FVCO_KHZ + n * USB3FVCO_GHZ) / 125) / m)

#define MIPHY_PLL_SSC_PERIOD(n)		(250 * n / SSC_FREQ_KHZ)

#define MIPHY_PLL_SSC_STEP(n, m) \
	((1024 * SSC_AMP_PPM) / ((n * 2000000) / (1024 * USB3FVCO_MHZ / m)))

/* MIPHY Regs*/
#define MIPHY_CONF_RESET		0x00
/* Reset macro except the CPU registers */
#define RST_APPLI			BIT(0)
/* Reset SerDes CPU registers except miphy_conf_reset */
#define RST_CONF			BIT(1)
/*Reset macro except the oscillator */
#define RST_MACRO			BIT(2)
#define CLEAR_MASK			0x00

#define MIPHY_RESET			0x01
#define RX_CAL_RST_SW			BIT(6)

#define MIPHY_CONTROL			0x04
#define DIS_LINK_RST			BIT(3)
#define MIPHY_90OHM_EN			BIT(0)

#define MIPHY_BOUNDARY_SEL		0x0A
#define GENSEL_SEL			BIT(0)
#define SSC_SEL				BIT(4)

#define MIPHY_BOUNDARY_2		0x0C
#define SSC_EN_SW			BIT(2)

#define MIPHY_PLL_CLKREF_FREQ		0x0D
#define MIPHY_SPEED			0x0E
#define txrx_spdsel(n)			((n & 0x3) | ((n & 0x3) << 2))

#define MIPHY_CONF			0x0F
#define MIPHY_SPARE_1			0x2B
#define RX_AUTO_CAL_EN			BIT(0)

#define MIPHY_SYNCHAR_CONTROL		0x30
/**
 * bit 0: enables symbol alignment on positive synch_char
 * bit 1: enables symbol alignment on negative synch_char.
 * bit 2: '0' performs symbol alignment on 10-bit boundary.
 *	  '1'performs symbol alignment on 20-bit boundary.
 * bit 3: enable symbol alignment.
 */
#define SYNC_CHAR_EN(n)			(n & 0xf)
#define DATA_OUT_DES_LOCKED		BIT(4)

#define MIPHY_COMP_POSTP2		0x39
#define MIPHY_COMP_FSM_1		0x3A
#define COMP_START			BIT(6)
#define MIPHY_COMP_FSM_4		0x3D
#define COMP_RX_AVG_END(n)		(n & 0xf)
#define COMP_RX_TEMPO_END(n)		((n & 0xf) << 4)

#define MIPHY_COMP_FSM_5		0x3E
#define COMP_TX_AVG_END(n)		(n & 0xf)
#define COMP_TX_TEMPO_END(n)		((n & 0xf) << 4)

#define MIPHY_COMP_POSTP		0x42
#define COMP_TX_OFFSET(n)		(n & 0xf)
#define COMP_RX_OFFSET(n)		((n & 0xf) << 4)

#define MIPHY_TX_CAL_MAN		0x4E
#define MIPHY_TST_BIAS_BOOST_2(n)	(0x62 + n)
#define TST_BIAS_BOOST			0x02
#define MIPHY_BIAS_BOOST_1(n)		(0x63 + n)
#define MIPHY_BIAS_BOOST_2(n)		(0x64 + n)
#define MIPHY_RXBUF_EQ_1(n)		(0x65 + n)
#define VTH_BIAS_PROG			(0x3 << 6)

#define MIPHY_RX_K_GAIN			0x78
#define Kp_GAIN(n)			(n & 0xf)
#define Ki_GAIN(n)			((n & 0x7) << 4)
#define AUTO_GAIN_EN			BIT(7)

#define MIPHY_RX_BUFFER_CTRL		0x7A
#define RX_BUFF_CTL_MAN(n)		(n & 0x3f)

#define MIPHY_RX_VGA_GAIN		0x7B
#define VGA_GAIN_MAN			0x05

#define MIPHY_RX_EQU_GAIN_1		0x7F
#define EQU_BOOST_MAN(n)		(n & 0x3)
#define EQU_GAIN_MAN(n)			((n & 0x1f) << 2)

#define MIPHY_RX_EQU_GAIN_FDB_2		0x83
#define MIPHY_RX_EQU_GAIN_FDB_3		0x84

#define MIPHY_RX_CAL_CTRL_1		0x97
#define FULL_CONST_EN			BIT(0)
#define EQU_ADPT_EN			BIT(4)
#define RX_ALGO_CAL_EN			BIT(6)

#define MIPHY_RX_CAL_CTRL_2		0x98
#define RX_CAL_FREEZE_EN		BIT(3)

#define MIPHY_RX_CAL_OFFSET_CTRL	0x99
#define VGA_OFFSET_POL			BIT(4)
#define OFFSET_COMP_EN			BIT(6)
#define CAL_OFFSET_VGA_LEN(n)		(n & 0x3)
#define CAL_OFFSET_THR_LEN(n)		((n & 0x3) << 2)

#define MIPHY_RX_CAL_VGA_STEP		0x9A
#define STEP_VGA_GAIN(n)		(n & 0xf)
#define STEP_EQU_BOOST(n)		((n & 0xf) << 4)

#define MIPHY_RX_CAL_EYE_MIN		0x9D
#define EYE_MIN_TARGET(n)		(n & 0x1f)
#define PATTERN_LENGTH(n)		((n & 0x7) << 5)

#define MIPHY_RX_CAL_OPT_LENGTH		0x9F
#define CAL_EYE_CONV_LEN(n)		(n & 0x3)
#define CAL_EYE_AVG_LEN(n)		((n & 0x3) << 2)

#define MIPHY_RX_LOCK_CTRL_1		0xC1
#define ERR_8B10B_RST			BIT(4)

#define MIPHY_RX_LOCK_SETTINGS_OPT	0xC2
/**
 * setting to optimize by calibration algorithm
 * 00: none
 * 01: frequency Offset
 * 10: VGA
 * 11: equalizer
 */
#define rx_setting_to_optimize(n, m, v) \
	(((n & 0x3) << 4) | ((m & 0x3) << 2) | ((v & 0x3) << 0))

#define MIPHY_RX_LOCK_STEP		0xC4
#define STEP_CDR_DRIFT(n)		((n & 0x7) << 4)
#define MIPHY_RX_SIGDET_SLEEP_OA	0xC9
#define MIPHY_RX_SIGDET_SLEEP_SEL	0xCA
#define MIPHY_RX_SIGDET_WAIT_SEL	0xCB
#define MIPHY_RX_SIGDET_DATA_SEL	0xCC
#define MIPHY_RX_POWER_CTRL_1		0xCD
#define PWR_CTL_MAN(n)			(n & 0x1f)
#define INPUT_BRIDGE_EN(n)		((n & 0x7) << 5)

#define MIPHY_RX_POWER_CTRL_2		0xCE
#define VTH_THRESHOLD_EN_MAN(n)		(n & 0xf)
#define RX_CLK_EN_MAN(n)		((n & 0x7) << 4)

#define MIPHY_PLL_CALSET_1		0xD4
#define MIPHY_PLL_CALSET_2		0xD5
#define MIPHY_PLL_CALSET_3		0xD6
#define MIPHY_PLL_CALSET_4		0xD7
#define CALSET_1(n)			((n & (0xff << 16)) >> 16)
#define CALSET_2(n)			((n & (0xff << 8)) >> 8)
#define CALSET_3(n)			(n & 0xff)
#define PLL_DRIVEBOOST_EN		BIT(2)

#define MIPHY_PLL_SBR_1			0xE3
#define PLL_CHANGE_SW			BIT(1)

#define MIPHY_PLL_SBR_2			0xE4
#define MIPHY_PLL_SBR_3			0xE5
#define MIPHY_PLL_SBR_4			0xE6
#define SBR_2(n)			((n & (0xff << 2)) >> 2)
#define SBR_3(n)			((n & (0xff << 4)) >> 4)
#define SBR_4(n, m)			((n & 0x3) | ((m & 0xf) << 4))

#define MIPHY_PLL_COMMON_MISC_2		0xE9
#define PLL_IVCO_MAN_EN			BIT(0)
#define PLL_IVCO_MAN(n)			((n & 0x1f) << 1)
#define PLL_ACT_FILT_EN			BIT(6)

#define MIPHY_PLL_SPAREIN		0xEB
#define I_BIAS_REF			BIT(4)
#define MIPHY_PLL_VCODIV_1		0xF8
#define PLL_CAL_AN_TARGET_LSB(n)	((n & 0x1f) << 3)

#define MIPHY_PLL_VCODIV_2		0xF9
#define PLL_CAL_AN_TARGET_MSB		0x80

#define MIPHY_PLL_VCODIV_4		0xFB
#define PLL_CAL_TIME_MSB		0x80

#define MIPHY_VERSION			0xFE
#define MIPHY_VER(n)			((n & 0xf) << 8)

#define MIPHY_REVISION			0xFF
#define MIPHY_CUT_230	0x0230
#define MIPHY_CUT_240	0x0240
#define MIPHY_CUT_250	0x0250
#define MIPHY_CUT_260	0x0260

/* MiPHY_2 RX status */
#define MIPHY2_RX_CAL_COMPLETED		BIT(0)
#define MIPHY2_RX_OFFSET		BIT(1)

#define MIPHY2_RX_CAL_STS		0xA0

/* MiPHY2 Status */
#define MiPHY2_STATUS_1			0x2
#define MIPHY2_PHY_READY		BIT(0)

/*pipe Wrapper registers */
#define PIPEW_DELAY_P2_USB_COM_RISE_0	0x23
#define PIPEW_DELAY_P2_USB_COM_RISE_1	0x24
#define PIPEW_DELAY_P2_USB_COM_RISE_2	0x25
#define PIPEW_DELAY_P2_USB_COM_FALL_0	0x26
#define PIPEW_DELAY_P2_USB_COM_FALL_1	0x27
#define PIPEW_DELAY_P2_USB_COM_FALL_2	0x28
#define PIPEW_DETECT_P2_USB_RISE_THR_0	0x29
#define PIPEW_DETECT_P2_USB_RISE_THR_1	0x2a
#define PIPEW_DETECT_P2_USB_RISE_THR_2	0x2b
#define PIPEW_DELAY_P3_USB_COM_RISE_0	0x2c
#define PIPEW_DELAY_P3_USB_COM_RISE_1	0x2d
#define PIPEW_DELAY_P3_USB_COM_RISE_2	0x2e
#define PIPEW_DELAY_P3_USB_COM_FALL_0	0x2f
#define PIPEW_DELAY_P3_USB_COM_FALL_1	0x30
#define PIPEW_DELAY_P3_USB_COM_FALL_2	0x31
#define PIPEW_DETECT_P3_USB_RISE_THR_0	0x32
#define PIPEW_DETECT_P3_USB_RISE_THR_1	0x33
#define PIPEW_DETECT_P3_USB_RISE_THR_2	0x34

#define PIPEW_CLK_P2_MHZ	250
#define PIPEW_DELAY_USECS	200
#define PIPEW_DELAY_NSECS	(1000 * PIPEW_DELAY_USECS)
#define PIPEW_DETECT_NSECS	320

#define PIPEW_P2_DELAY	\
	(PIPEW_DELAY_USECS * PIPEW_CLK_P2_MHZ)
#define PIPEW_P2_DETECT \
	((PIPEW_CLK_P2_MHZ * (PIPEW_DELAY_NSECS - PIPEW_DETECT_NSECS)) / 1000)

#define PIPEW_P3_DELAY(n)	(n * PIPEW_DELAY_USECS)
#define PIPEW_P3_DETECT(n) \
	((n * (PIPEW_DELAY_NSECS - PIPEW_DETECT_NSECS)) / 1000)

#define PIPEW_DELAY_0(n)	(n & 0xff)
#define PIPEW_DELAY_1(n)	((n & (0xff << 8)) >> 8)
#define PIPEW_DELAY_2(n)	((n & (0xff << 16)) >> 16)

#define PIPEW_DETECT_0(n)	(n & 0xff)
#define PIPEW_DETECT_1(n)	((n & (0xff << 8)) >> 8)
#define PIPEW_DETECT_2(n)	((n & (0xff << 16)) >> 16)

#define PIPEW_USB3_MARG_0	0x68
#define PIPEW_USB3_MARG_1	0x69
#define PIPEW_USB3_MARG_2	0x6A
#define PIPEW_USB3_MARG_3	0x6B
#define PIPEW_USB3_MARG_4	0x6C
#define PIPEW_USB3_MARG_5	0x6D
#define PIPEW_USB3_MARG_6	0x6E
#define PIPEW_USB3_MARG_7	0x6F

#define TX_SWING(n)		((n) & 0xF)
#define TX_PREEMPH(n)		(((n) & 0xF) << 4)
#define TX_MARG_UPDATE		0x0D

struct miphy_pll {
	u32 refclk;
	u32 ratio;
	u32 ssc_period;
	u32 ssc_step;
	int Fvco_ppm;
};

struct sti_usb3_miphy {
	struct usb_phy phy;
	struct device *dev;
	struct regmap *regmap;
	struct miphy_pll *pll;
	struct reset_control *rstc;
	void __iomem *usb3_base;
	void __iomem *pipe_base;
	struct timer_list miphy_timer;
	struct workqueue_struct *miphy_queue;
	struct work_struct miphy_work;
	spinlock_t lock;
	bool sw_auto_calib;
	bool no_ssc;
	u16 release;
};

#endif /* __PHY_ST_USB3_H */
