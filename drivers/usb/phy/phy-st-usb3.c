#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/clk.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/delay.h>
#if defined(MY_DEF_HERE)
#else  
#include <linux/mfd/syscon.h>
#endif  
#include <linux/usb/phy.h>

#if defined(MY_DEF_HERE)
#include "phy-st-usb3.h"
#endif  

#if defined(MY_DEF_HERE)
#else  
#define phy_to_priv(x)	container_of((x), struct sti_usb3_miphy, phy)

#define SSC_ON	0x11
#define SSC_OFF	0x01
#endif  

#ifdef MY_DEF_HERE
#if defined(MY_DEF_HERE)
 
static int miphy_ssc_off;
module_param(miphy_ssc_off, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(miphy_ssc_off, "turn off miphy ssc");
#endif  
 
#define MIPHY_DEFAULT_TIMER	2000
static int miphy_timer_msecs = MIPHY_DEFAULT_TIMER;
module_param(miphy_timer_msecs, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(miphy_timer_msecs, "miphy timer init in msecs");
#define MIPHY_TIMER(x)	(jiffies + msecs_to_jiffies(x))

#if defined(MY_DEF_HERE)
#else  
 
#define MIPHY2_RX_CAL_COMPLETED		BIT(0)
#define MIPHY2_RX_OFFSET		BIT(1)

#define MIPHY2_RX_CAL_STS		0xA0

#define MiPHY2_STATUS_1			0x2
#define MIPHY2_PHY_READY		BIT(0)
#endif  
#endif  
#if defined(MY_DEF_HERE)
 
static struct usb_device_id usb_blacklist[] = {
	{USB_DEVICE(0x07ab, 0xfc9f)},	 
	{USB_DEVICE(0x1058, 0x1148)},	 
	{}			 
};

static int sti_usb3_miphy_on_connect(struct usb_phy *phy_dev,
				     struct usb_device *udev)
{
	struct sti_usb3_miphy *miphy = phy_to_priv(phy_dev);
	int i;

	if (miphy_ssc_off)
		writeb_relaxed(SSC_OFF, miphy->usb3_base + MIPHY_BOUNDARY_SEL);

	for (i = 0; i < ARRAY_SIZE(usb_blacklist); ++i) {
		if ((udev->descriptor.idVendor == usb_blacklist[i].idVendor) &&
		    (udev->descriptor.idProduct ==
		     usb_blacklist[i].idProduct)) {
			dev_warn(miphy->dev,
				 "miphy disable SSC for known buggy device\n");
			writeb_relaxed(SSC_OFF,
				       miphy->usb3_base + MIPHY_BOUNDARY_SEL);
		}
	}

	return 0;
}

static int sti_usb3_miphy_on_disconnect(struct usb_phy *phy_dev,
					struct usb_device *udev)
{
	struct sti_usb3_miphy *miphy = phy_to_priv(phy_dev);
	u8 val;

	if (miphy->sw_auto_calib) {
		val = readb_relaxed(miphy->usb3_base + MIPHY_RESET);
		val |= RX_CAL_RST_SW;
		writeb_relaxed(val, miphy->usb3_base + MIPHY_RESET);
		writeb_relaxed(CLEAR_MASK, miphy->usb3_base + MIPHY_RESET);
		dev_dbg(miphy->dev, "miphy autocalibration done\n");

		usleep_range(10, 20);

		if ((readb_relaxed(miphy->usb3_base + MIPHY_RX_EQU_GAIN_FDB_2))
		    || (readb_relaxed(miphy->usb3_base +
				      MIPHY_RX_EQU_GAIN_FDB_3)))
			dev_err(miphy->dev, "miphy autocalibration failed!\n");
	}

	if (!miphy->no_ssc && !miphy_ssc_off)
		writeb_relaxed(SSC_ON, miphy->usb3_base + MIPHY_BOUNDARY_SEL);

	return 0;
}
#else  
 
#define	SYSCFG5071			0x11c
#define MIPHY2_PX_TX_POLARITY		BIT(0)
#define MIPHY2_PX_RX_POLARITY		BIT(1)
#define MIPHY2_PX_SYNC_DETECT_ENABLE	BIT(2)
#define MIPHY2_CTRL_MASK		0x7

struct sti_usb3_cfg {
	u32 syscfg;
	u32 cfg_mask;
	u32 cfg;
};

struct sti_usb3_miphy {
	struct usb_phy phy;
	struct device *dev;
	struct regmap *regmap;
	const struct sti_usb3_cfg *cfg;
	struct reset_control *rstc;
	void __iomem *usb3_base;
	void __iomem *pipe_base;
#ifdef MY_DEF_HERE
	struct timer_list miphy_timer;
	struct workqueue_struct *miphy_queue;
	struct work_struct miphy_work;
	spinlock_t lock;
#endif  
};

static struct sti_usb3_cfg sti_usb3_miphy_cfg = {
	.syscfg = SYSCFG5071,
	.cfg_mask = MIPHY2_CTRL_MASK,
	.cfg = MIPHY2_PX_SYNC_DETECT_ENABLE,
};

struct miphy_initval {
	u16 reg;
	u16 val;
};

static const struct miphy_initval initvals[] = {
	 
	{0x00, 0x01}, {0x00, 0x03},
	 
	{0x00, 0x01}, {0x04, 0x1C},
	 
	{0xEB, 0x1D}, {0x0D, 0x1E}, {0x0F, 0x00}, {0xC4, 0x70},
#ifdef MY_DEF_HERE
	{0xC9, 0x22}, {0xCA, 0x22}, {0xCB, 0x22}, {0xCC, 0x2A},
#else  
	{0xC9, 0x02}, {0xCA, 0x02}, {0xCB, 0x02}, {0xCC, 0x0A},
#endif  
	 
	{0xD4, 0xA6}, {0xD5, 0xAA}, {0xD6, 0xAA}, {0xD7, 0x04},
	{0xD3, 0x00},
	 
	{0x0F, 0x00}, {0x0E, 0x0A},
	 
	{0xC2, 0x1C}, {0x97, 0x51}, {0x98, 0x70}, {0x99, 0x5F},
	{0x9A, 0x22}, {0x9F, 0x0E},

#ifdef MY_DEF_HERE
	{0x7A, 0x05}, {0x7B, 0x05}, {0x7F, 0x78}, {0x30, 0x1B},
#else  
	{0x7A, 0x05}, {0x7F, 0x78}, {0x30, 0x1B},
#endif  
	 
	{0x0A, SSC_ON},
	 
	{0x63, 0x00}, {0x64, 0xA7},
	 
	{0x42, 0x02},
	 
	{0x0C, 0x04},
	 
	{0x2B, 0x01},
	 
	{0x0F, 0x00}, {0xE5, 0x5A}, {0xE6, 0xA0}, {0xE4, 0x3C},
	{0xE6, 0xA1}, {0xE3, 0x00}, {0xE3, 0x02}, {0xE3, 0x00},
	 
	{0x78, 0xCA},
	 
	{0xCD, 0x21}, {0xCD, 0x29}, {0xCE, 0x1A},
	 
	{0x00, 0x01}, {0x00, 0x00}, {0x01, 0x04}, {0x01, 0x05},
	{0xE9, 0x00}, {0x0D, 0x1E}, {0x3A, 0x40}, {0x01, 0x01},
	{0x01, 0x00}, {0xE9, 0x40}, {0x0F, 0x00}, {0x0B, 0x00},
	{0x62, 0x00}, {0x0F, 0x00}, {0xE3, 0x02}, {0x26, 0xA5},
	{0x0F, 0x00},
};

static int sti_usb3_miphy_autocalibration(struct usb_phy *phy_dev,
					enum usb_device_speed speed)
{
	struct sti_usb3_miphy *miphy = phy_to_priv(phy_dev);

	writeb_relaxed(0x40, miphy->usb3_base + 0x01);
	writeb_relaxed(0x00, miphy->usb3_base + 0x01);
	dev_dbg(miphy->dev, "miphy autocalibration done\n");

	usleep_range(10, 20);

	if ((readb_relaxed(miphy->usb3_base + 0x83)) ||
	    (readb_relaxed(miphy->usb3_base + 0x84)))
		dev_err(miphy->dev, "miphy autocalibration failed!\n");

	return 0;
}
#endif  
#ifdef MY_DEF_HERE
static void sti_usb3_miphy_work(struct work_struct *work)
{
	struct sti_usb3_miphy *miphy =
	    container_of(work, struct sti_usb3_miphy, miphy_work);
	u8 status;
	u8 reg;

	spin_lock(&miphy->lock);

	status = readb_relaxed(miphy->usb3_base + MiPHY2_STATUS_1);
	if (status & MIPHY2_PHY_READY) {
		dev_dbg(miphy->dev, "MiPHY: phy is ready\n");
		 
		spin_unlock(&miphy->lock);
		mdelay(1);
		spin_lock(&miphy->lock);
		reg = readb_relaxed(miphy->usb3_base + MIPHY2_RX_CAL_STS);
		if (!(reg & MIPHY2_RX_CAL_COMPLETED)
		    && !(reg & MIPHY2_RX_OFFSET))
			dev_warn(miphy->dev,
				"fail RX calibration, unplug/plug the cable\n");
	}

	spin_unlock(&miphy->lock);
}

static void sti_usb3_miphy_timer(unsigned long data)
{
	struct sti_usb3_miphy *miphy = (void *)data;

	schedule_work(&miphy->miphy_work);
	 
	if (miphy_timer_msecs <= MIPHY_DEFAULT_TIMER)
		miphy_timer_msecs = MIPHY_DEFAULT_TIMER;
	miphy->miphy_timer.expires = MIPHY_TIMER(miphy_timer_msecs);
	mod_timer(&miphy->miphy_timer, miphy->miphy_timer.expires);
}

static void sti_usb3_miphy_timer_init(struct sti_usb3_miphy *phy_dev)
{
	init_timer(&phy_dev->miphy_timer);

	phy_dev->miphy_timer.data = (unsigned long) phy_dev;
	phy_dev->miphy_timer.function = sti_usb3_miphy_timer;
	phy_dev->miphy_timer.expires = MIPHY_TIMER(miphy_timer_msecs);

	add_timer(&phy_dev->miphy_timer);
	dev_info(phy_dev->dev, "USB3 MIPHY28LP timer initialized\n");

}
#endif  

#if defined(MY_DEF_HERE)
static void sti_miphy_reset(struct sti_usb3_miphy *phy_dev, bool do_reset)
{
	if (do_reset) {
		writeb_relaxed(RST_APPLI | RST_CONF,
				phy_dev->usb3_base + MIPHY_CONF_RESET);
		writeb_relaxed(RST_APPLI,
				phy_dev->usb3_base + MIPHY_CONF_RESET);
	} else
		writeb_relaxed(CLEAR_MASK,
				phy_dev->usb3_base + MIPHY_CONF_RESET);

	usleep_range(10, 20);
}

static void sti_miphy_ctrl(struct sti_usb3_miphy *phy_dev)
{
	u8 val;

	val = readb_relaxed(phy_dev->usb3_base + MIPHY_CONTROL);
	val |= DIS_LINK_RST;
	if (phy_dev->release > MIPHY_CUT_250)
		val |= MIPHY_90OHM_EN;
	writeb_relaxed(val, phy_dev->usb3_base + MIPHY_CONTROL);

	writeb_relaxed(CLEAR_MASK, phy_dev->usb3_base + MIPHY_CONF);

	writeb_relaxed(txrx_spdsel(2), phy_dev->usb3_base + MIPHY_SPEED);

	val = SYNC_CHAR_EN(0xb);
	writeb_relaxed(val, phy_dev->usb3_base + MIPHY_SYNCHAR_CONTROL);
}

static void sti_miphy_pll_config(struct sti_usb3_miphy *phy_dev)
{
	struct miphy_pll *pll = phy_dev->pll;
	u8 val;

	if (phy_dev->release > MIPHY_CUT_250) {
		val = PLL_IVCO_MAN_EN | PLL_IVCO_MAN(0xb) | PLL_ACT_FILT_EN;
		writeb_relaxed(val,
			       phy_dev->usb3_base + MIPHY_PLL_COMMON_MISC_2);
		writeb_relaxed(PLL_CAL_AN_TARGET_LSB(4),
			       phy_dev->usb3_base + MIPHY_PLL_VCODIV_1);
		writeb_relaxed(PLL_CAL_AN_TARGET_MSB,
			       phy_dev->usb3_base + MIPHY_PLL_VCODIV_2);
		writeb_relaxed(PLL_CAL_TIME_MSB,
			       phy_dev->usb3_base + MIPHY_PLL_VCODIV_4);
	}

	val = readb_relaxed(phy_dev->usb3_base + MIPHY_PLL_SPAREIN);
	val |= I_BIAS_REF;
	writeb_relaxed(val, phy_dev->usb3_base + MIPHY_PLL_SPAREIN);

	writeb_relaxed(pll->refclk, phy_dev->usb3_base + MIPHY_PLL_CLKREF_FREQ);

	pll->ratio = MIPHY_PLL_RATIO(pll->Fvco_ppm, pll->refclk);
	writeb_relaxed(CALSET_1(pll->ratio),
			phy_dev->usb3_base + MIPHY_PLL_CALSET_1);
	writeb_relaxed(CALSET_2(pll->ratio),
			phy_dev->usb3_base + MIPHY_PLL_CALSET_2);
	writeb_relaxed(CALSET_3(pll->ratio),
			phy_dev->usb3_base + MIPHY_PLL_CALSET_3);

	if (phy_dev->release < MIPHY_CUT_240)
		writeb_relaxed(PLL_DRIVEBOOST_EN,
				phy_dev->usb3_base + MIPHY_PLL_CALSET_4);

	writeb_relaxed(GENSEL_SEL | SSC_SEL,
			phy_dev->usb3_base + MIPHY_BOUNDARY_SEL);
	writeb_relaxed(SSC_EN_SW, phy_dev->usb3_base + MIPHY_BOUNDARY_2);

	pll->ssc_period = MIPHY_PLL_SSC_PERIOD(pll->refclk);
	pll->ssc_step = MIPHY_PLL_SSC_STEP(pll->ssc_period, pll->refclk);
	writeb_relaxed(SBR_2(pll->ssc_period),
		       phy_dev->usb3_base + MIPHY_PLL_SBR_2);
	writeb_relaxed(SBR_3(pll->ssc_step),
		       phy_dev->usb3_base + MIPHY_PLL_SBR_3);
	writeb_relaxed(SBR_4(pll->ssc_period, pll->ssc_step),
		       phy_dev->usb3_base + MIPHY_PLL_SBR_4);

	val = readb_relaxed(phy_dev->usb3_base + MIPHY_PLL_SBR_1);
	val ^= PLL_CHANGE_SW;
	writeb_relaxed(val, phy_dev->usb3_base + MIPHY_PLL_SBR_1);
}

static void sti_miphy_rx_config(struct sti_usb3_miphy *phy_dev)
{
	u8 val;
	u8 offset;

	val = readb_relaxed(phy_dev->usb3_base + MIPHY_RX_LOCK_CTRL_1);
	val |= ERR_8B10B_RST;
	writeb_relaxed(val, phy_dev->usb3_base + MIPHY_RX_LOCK_CTRL_1);

	val = rx_setting_to_optimize(1, 3, 0);
	writeb_relaxed(val, phy_dev->usb3_base + MIPHY_RX_LOCK_SETTINGS_OPT);
	val = STEP_CDR_DRIFT(7);
	writeb_relaxed(val, phy_dev->usb3_base + MIPHY_RX_LOCK_STEP);

	val = STEP_VGA_GAIN(2) | STEP_EQU_BOOST(2);
	writeb_relaxed(val, phy_dev->usb3_base + MIPHY_RX_CAL_VGA_STEP);
	val = RX_BUFF_CTL_MAN(5);
	writeb_relaxed(val, phy_dev->usb3_base + MIPHY_RX_BUFFER_CTRL);
	val = EQU_BOOST_MAN(0) | EQU_GAIN_MAN(0x1E);
	writeb_relaxed(val, phy_dev->usb3_base + MIPHY_RX_EQU_GAIN_1);

	val = FULL_CONST_EN | EQU_ADPT_EN | RX_ALGO_CAL_EN;
	writeb_relaxed(val, phy_dev->usb3_base + MIPHY_RX_CAL_CTRL_1);
	val = readb_relaxed(phy_dev->usb3_base + MIPHY_RX_CAL_CTRL_2);
	val &= ~RX_CAL_FREEZE_EN;
	writeb_relaxed(val, phy_dev->usb3_base + MIPHY_RX_CAL_CTRL_2);

	val = CAL_OFFSET_VGA_LEN(3) | CAL_OFFSET_THR_LEN(3) |
		OFFSET_COMP_EN | VGA_OFFSET_POL;
	writeb_relaxed(val, phy_dev->usb3_base + MIPHY_RX_CAL_OFFSET_CTRL);
	val = CAL_EYE_CONV_LEN(2) | CAL_EYE_AVG_LEN(3);
	writeb_relaxed(val, phy_dev->usb3_base + MIPHY_RX_CAL_OPT_LENGTH);

	val = INPUT_BRIDGE_EN(1) | PWR_CTL_MAN(9);
	writeb_relaxed(val, phy_dev->usb3_base + MIPHY_RX_POWER_CTRL_1);
	val = RX_CLK_EN_MAN(1) | VTH_THRESHOLD_EN_MAN(0xA);
	writeb_relaxed(val, phy_dev->usb3_base + MIPHY_RX_POWER_CTRL_2);

	if (phy_dev->release > MIPHY_CUT_250)
		val = Kp_GAIN(0xb) | Ki_GAIN(4) | AUTO_GAIN_EN;
	else
		val = Kp_GAIN(0xa) | Ki_GAIN(4) | AUTO_GAIN_EN;
	writeb_relaxed(val, phy_dev->usb3_base + MIPHY_RX_K_GAIN);

	if (phy_dev->release > MIPHY_CUT_250)
		offset = MIPHY_BIAS_BOOST_1(1);
	else
		offset = MIPHY_BIAS_BOOST_1(0);
	writeb_relaxed(CLEAR_MASK, phy_dev->usb3_base + offset);

	if (phy_dev->release > MIPHY_CUT_250) {
		val = 0x20;
		offset = MIPHY_BIAS_BOOST_2(1);
	} else {
		val = 0xA7;
		offset = MIPHY_BIAS_BOOST_2(0);
	}
	writeb_relaxed(val, phy_dev->usb3_base + offset);

	if (phy_dev->release > MIPHY_CUT_250) {
		val = EYE_MIN_TARGET(5) | PATTERN_LENGTH(5);
		writeb_relaxed(val, phy_dev->usb3_base + MIPHY_RX_CAL_EYE_MIN);
	}

	if (phy_dev->release < MIPHY_CUT_240) {
		writeb_relaxed(VGA_GAIN_MAN,
				phy_dev->usb3_base + MIPHY_RX_VGA_GAIN);

		writeb_relaxed(RX_AUTO_CAL_EN,
				phy_dev->usb3_base + MIPHY_SPARE_1);

		writeb_relaxed(TST_BIAS_BOOST,
				phy_dev->usb3_base + MIPHY_TST_BIAS_BOOST_2(0));
		writeb_relaxed(VTH_BIAS_PROG,
				phy_dev->usb3_base + MIPHY_RXBUF_EQ_1(0));
	}
}

static void sti_miphy_comp(struct sti_usb3_miphy *phy_dev)
{
	u8 val;

	writeb_relaxed(CLEAR_MASK, phy_dev->usb3_base + MIPHY_TX_CAL_MAN);

	if (phy_dev->release > MIPHY_CUT_250) {
		writeb_relaxed(CLEAR_MASK,
			       phy_dev->usb3_base + MIPHY_COMP_POSTP);
		writeb_relaxed(CLEAR_MASK,
			       phy_dev->usb3_base + MIPHY_COMP_POSTP2);

		val = COMP_RX_AVG_END(8) | COMP_RX_TEMPO_END(8);
		writeb_relaxed(val, phy_dev->usb3_base + MIPHY_COMP_FSM_4);
		val = COMP_TX_AVG_END(8) | COMP_TX_TEMPO_END(8);
		writeb_relaxed(val, phy_dev->usb3_base + MIPHY_COMP_FSM_5);
	} else {
		 
		val = COMP_TX_OFFSET(2) | COMP_RX_OFFSET(0);
		writeb_relaxed(val, phy_dev->usb3_base + MIPHY_COMP_POSTP);

		val = readb_relaxed(phy_dev->usb3_base + MIPHY_COMP_FSM_1);
		val |= COMP_START;
		writeb_relaxed(val, phy_dev->usb3_base + MIPHY_COMP_FSM_1);
	}
}

static void sti_pipew_cfg(struct sti_usb3_miphy *phy_dev)
{
	struct miphy_pll *pll = phy_dev->pll;
	unsigned int val;

	writeb_relaxed(PIPEW_DELAY_0(PIPEW_P2_DELAY),
			phy_dev->pipe_base + PIPEW_DELAY_P2_USB_COM_RISE_0);
	writeb_relaxed(PIPEW_DELAY_1(PIPEW_P2_DELAY),
			phy_dev->pipe_base + PIPEW_DELAY_P2_USB_COM_RISE_1);
	writeb_relaxed(PIPEW_DELAY_2(PIPEW_P2_DELAY),
			phy_dev->pipe_base + PIPEW_DELAY_P2_USB_COM_RISE_2);

	writeb_relaxed(PIPEW_DELAY_0(PIPEW_P2_DELAY),
			phy_dev->pipe_base + PIPEW_DELAY_P2_USB_COM_FALL_0);
	writeb_relaxed(PIPEW_DELAY_1(PIPEW_P2_DELAY),
			phy_dev->pipe_base + PIPEW_DELAY_P2_USB_COM_FALL_1);
	writeb_relaxed(PIPEW_DELAY_2(PIPEW_P2_DELAY),
			phy_dev->pipe_base + PIPEW_DELAY_P2_USB_COM_FALL_2);

	writeb_relaxed(PIPEW_DETECT_0(PIPEW_P2_DETECT),
			phy_dev->pipe_base + PIPEW_DETECT_P2_USB_RISE_THR_0);
	writeb_relaxed(PIPEW_DETECT_1(PIPEW_P2_DETECT),
			phy_dev->pipe_base + PIPEW_DETECT_P2_USB_RISE_THR_1);
	writeb_relaxed(PIPEW_DETECT_2(PIPEW_P2_DETECT),
			phy_dev->pipe_base + PIPEW_DETECT_P2_USB_RISE_THR_2);

	val = PIPEW_P3_DELAY(pll->refclk);
	writeb_relaxed(PIPEW_DELAY_0(val),
		       phy_dev->pipe_base + PIPEW_DELAY_P3_USB_COM_RISE_0);
	writeb_relaxed(PIPEW_DELAY_1(val),
		       phy_dev->pipe_base + PIPEW_DELAY_P3_USB_COM_RISE_1);
	writeb_relaxed(PIPEW_DELAY_2(val),
		       phy_dev->pipe_base + PIPEW_DELAY_P3_USB_COM_RISE_2);

	writeb_relaxed(PIPEW_DELAY_0(val),
		       phy_dev->pipe_base + PIPEW_DELAY_P3_USB_COM_FALL_0);
	writeb_relaxed(PIPEW_DELAY_1(val),
		       phy_dev->pipe_base + PIPEW_DELAY_P3_USB_COM_FALL_1);
	writeb_relaxed(PIPEW_DELAY_2(val),
		       phy_dev->pipe_base + PIPEW_DELAY_P3_USB_COM_FALL_2);

	val = PIPEW_P3_DETECT(pll->refclk);
	writeb_relaxed(PIPEW_DETECT_0(val),
		       phy_dev->pipe_base + PIPEW_DETECT_P3_USB_RISE_THR_0);
	writeb_relaxed(PIPEW_DETECT_1(val),
		       phy_dev->pipe_base + PIPEW_DETECT_P3_USB_RISE_THR_1);
	writeb_relaxed(PIPEW_DETECT_2(val),
		       phy_dev->pipe_base + PIPEW_DETECT_P3_USB_RISE_THR_2);

	val = TX_SWING(0x7) | TX_PREEMPH(0x6);
	writeb_relaxed(val, phy_dev->pipe_base + PIPEW_USB3_MARG_0);
	writeb_relaxed(val, phy_dev->pipe_base + PIPEW_USB3_MARG_2);
	writeb_relaxed(val, phy_dev->pipe_base + PIPEW_USB3_MARG_4);
	writeb_relaxed(val, phy_dev->pipe_base + PIPEW_USB3_MARG_6);

	val = TX_MARG_UPDATE;
	writeb_relaxed(val, phy_dev->pipe_base + PIPEW_USB3_MARG_1);
	writeb_relaxed(val, phy_dev->pipe_base + PIPEW_USB3_MARG_3);
	writeb_relaxed(val, phy_dev->pipe_base + PIPEW_USB3_MARG_5);
	writeb_relaxed(val, phy_dev->pipe_base + PIPEW_USB3_MARG_7);
}

static void sti_usb3_miphy28lp(struct sti_usb3_miphy *phy_dev)
{
	dev_info(phy_dev->dev, "MiPHY28LP setup\n");

	phy_dev->release = readb_relaxed(phy_dev->usb3_base + MIPHY_REVISION);
	phy_dev->release |= MIPHY_VER(readb_relaxed(phy_dev->usb3_base +
						    MIPHY_VERSION));

	dev_info(phy_dev->dev, "MiPHY28LP release %#x found\n",
		 phy_dev->release);

	sti_miphy_reset(phy_dev, true);

	sti_miphy_ctrl(phy_dev);
	sti_miphy_pll_config(phy_dev);
	sti_miphy_rx_config(phy_dev);
	sti_miphy_comp(phy_dev);

	sti_miphy_reset(phy_dev, false);

	if (phy_dev->no_ssc || miphy_ssc_off)
		writeb_relaxed(SSC_OFF,
			       phy_dev->usb3_base + MIPHY_BOUNDARY_SEL);

	sti_pipew_cfg(phy_dev);
}
#else  
static void sti_usb3_miphy28lp(struct sti_usb3_miphy *phy_dev)
{
	int i;
	struct device_node *np = phy_dev->dev->of_node;

	dev_info(phy_dev->dev, "MiPHY28LP setup\n");

	for (i = 0; i < ARRAY_SIZE(initvals); i++) {
		dev_dbg(phy_dev->dev, "reg: 0x%x=0x%x\n", initvals[i].reg,
			initvals[i].val);
		writeb_relaxed(initvals[i].val,
			       phy_dev->usb3_base + initvals[i].reg);
	}

	if (of_property_read_bool(np, "st,no-ssc"))
		writeb_relaxed(SSC_OFF, phy_dev->usb3_base + 0x0A);

	writeb_relaxed(0X68, phy_dev->pipe_base + 0x23);
	writeb_relaxed(0X61, phy_dev->pipe_base + 0x24);
	writeb_relaxed(0X68, phy_dev->pipe_base + 0x26);
	writeb_relaxed(0X61, phy_dev->pipe_base + 0x27);
	writeb_relaxed(0X18, phy_dev->pipe_base + 0x29);
	writeb_relaxed(0X61, phy_dev->pipe_base + 0x2A);

	writeb_relaxed(0X67, phy_dev->pipe_base + 0x68);
	writeb_relaxed(0X0D, phy_dev->pipe_base + 0x69);
	writeb_relaxed(0X67, phy_dev->pipe_base + 0x6A);
	writeb_relaxed(0X0D, phy_dev->pipe_base + 0x6B);
	writeb_relaxed(0X67, phy_dev->pipe_base + 0x6C);
	writeb_relaxed(0X0D, phy_dev->pipe_base + 0x6D);
	writeb_relaxed(0X67, phy_dev->pipe_base + 0x6E);
	writeb_relaxed(0X0D, phy_dev->pipe_base + 0x6F);
}
#endif  

static int sti_usb3_miphy_init(struct usb_phy *phy)
{
	struct sti_usb3_miphy *phy_dev = phy_to_priv(phy);
#if defined(MY_DEF_HERE)
	if (!IS_ERR(phy_dev->rstc))
		reset_control_deassert(phy_dev->rstc);
#else  
	int ret;

	ret = regmap_update_bits(phy_dev->regmap, phy_dev->cfg->syscfg,
				 phy_dev->cfg->cfg_mask, phy_dev->cfg->cfg);
	if (ret)
		return ret;

	reset_control_deassert(phy_dev->rstc);
#endif  

	sti_usb3_miphy28lp(phy_dev);
#ifdef MY_DEF_HERE
	 
	sti_usb3_miphy_timer_init(phy_dev);
#endif  

	return 0;
}

static void sti_usb3_miphy_shutdown(struct usb_phy *phy)
{
	struct sti_usb3_miphy *phy_dev = phy_to_priv(phy);

#if defined(MY_DEF_HERE)
	if (!IS_ERR(phy_dev->rstc))
		reset_control_assert(phy_dev->rstc);
#else  
	reset_control_assert(phy_dev->rstc);
#endif  
#ifdef MY_DEF_HERE
	del_timer(&phy_dev->miphy_timer);
#endif  
}

static const struct of_device_id sti_usb3_miphy_of_match[];

static int sti_usb3_miphy_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	const struct of_device_id *match;
	struct sti_usb3_miphy *phy_dev;
#if defined(MY_DEF_HERE)
	struct miphy_pll *phy_pll;
#endif  
	struct device *dev = &pdev->dev;
	struct usb_phy *phy;
	struct resource *res;
#if defined(MY_DEF_HERE)
	struct clk *clk;
	unsigned long rate;
	u32 ret;

	if (!np)
		return -ENODEV;
#endif  

	phy_dev = devm_kzalloc(dev, sizeof(*phy_dev), GFP_KERNEL);
	if (!phy_dev)
		return -ENOMEM;

#if defined(MY_DEF_HERE)
	phy_pll = devm_kzalloc(dev, sizeof(*phy_pll), GFP_KERNEL);
	if (!phy_pll)
		return -ENOMEM;
#endif  

	match = of_match_device(sti_usb3_miphy_of_match, &pdev->dev);
	if (!match)
		return -ENODEV;

#if defined(MY_DEF_HERE)
	clk = devm_clk_get(dev, "miphy_osc");
	if (IS_ERR(clk)) {
		dev_err(dev, "miphy_osc clk not found\n");
		return PTR_ERR(clk);
	}

	ret = clk_prepare_enable(clk);
	if (ret) {
		dev_err(phy_dev->dev, "Failed to enable miphy osc clock\n");
		return ret;
#else  
	phy_dev->cfg = match->data;
	phy_dev->dev = dev;

	phy_dev->rstc = devm_reset_control_get(dev, NULL);
	if (IS_ERR(phy_dev->rstc)) {
		dev_err(dev, "failed to ctrl MiPHY2 USB3 reset\n");
		return PTR_ERR(phy_dev->rstc);
#endif  
	}

#if defined(MY_DEF_HERE)
	rate = clk_get_rate(clk);
	phy_pll->refclk = rate / 1000000;

	dev_info(dev, "USB3 MiPHY Ref clk %d MHz enabled\n", phy_pll->refclk);

	if (of_property_read_u32(np, "st,fvco-ppm", &phy_pll->Fvco_ppm)) {
		dev_warn(dev, "phy fvco ppm not set use 0 ppm by default\n");
		phy_pll->Fvco_ppm = 0;
	}

	phy_dev->pll = phy_pll;
	phy_dev->dev = dev;

	phy_dev->rstc = devm_reset_control_get(dev, NULL);
	if (IS_ERR(phy_dev->rstc))
		dev_warn(dev, "MiPHY2 USB3 reset is missing...\n");
	else {
		dev_dbg(dev, "MiPHY2 USB3 reset!\n");
		reset_control_deassert(phy_dev->rstc);
#else  
	dev_info(dev, "reset MiPHY\n");
	reset_control_deassert(phy_dev->rstc);

	phy_dev->regmap = syscon_regmap_lookup_by_phandle(np, "st,syscfg");
	if (IS_ERR(phy_dev->regmap)) {
		dev_err(dev, "No syscfg phandle specified\n");
		return PTR_ERR(phy_dev->regmap);
#endif  
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "usb3-uport");
	if (res) {
#if defined(MY_DEF_HERE)
		phy_dev->usb3_base = devm_ioremap_resource(&pdev->dev, res);
#else  
		phy_dev->usb3_base = devm_request_and_ioremap(&pdev->dev, res);
#endif  
		if (!phy_dev->usb3_base) {
			dev_err(&pdev->dev, "Unable to map base registers\n");
			return -ENOMEM;
		}
	}
	 
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "pipew");
	if (res) {
#if defined(MY_DEF_HERE)
		phy_dev->pipe_base = devm_ioremap_resource(&pdev->dev, res);
#else  
		phy_dev->pipe_base = devm_request_and_ioremap(&pdev->dev, res);
#endif  
		if (!phy_dev->pipe_base) {
			dev_err(&pdev->dev, "Unable to map PIPE registers\n");
			return -ENOMEM;
		}
	}

	dev_info(dev, "usb3 ioaddr 0x%p, pipew ioaddr 0x%p\n",
		 phy_dev->usb3_base, phy_dev->pipe_base);
#ifdef MY_DEF_HERE
	spin_lock_init(&phy_dev->lock);

	phy_dev->miphy_queue =
	    create_singlethread_workqueue("usb3_miphy_queue");
	if (!phy_dev->miphy_queue) {
		dev_err(phy_dev->dev, "couldn't create workqueue\n");
		return -ENOMEM;
	}

	INIT_WORK(&phy_dev->miphy_work, sti_usb3_miphy_work);
#endif  
#if defined(MY_DEF_HERE)
	phy_dev->sw_auto_calib =
		of_property_read_bool(np, "st,auto-calibration");

	phy_dev->no_ssc = of_property_read_bool(np, "st,no-ssc");
#endif  

	phy = &phy_dev->phy;
	phy->dev = dev;
	phy->label = "USB3 MiPHY2 (LP28)";
	phy->init = sti_usb3_miphy_init;
	phy->type = USB_PHY_TYPE_USB3;
	phy->shutdown = sti_usb3_miphy_shutdown;
#if defined(MY_DEF_HERE)
	phy->notify_connect = sti_usb3_miphy_on_connect;
	phy->notify_disconnect = sti_usb3_miphy_on_disconnect;
#else  
	if (of_property_read_bool(np, "st,auto-calibration"))
		phy->notify_disconnect = sti_usb3_miphy_autocalibration;
#endif  

	usb_add_phy_dev(phy);

	platform_set_drvdata(pdev, phy_dev);

	dev_info(dev, "USB3 MiPHY2 probed\n");

	return 0;
}

static int sti_usb3_miphy_remove(struct platform_device *pdev)
{
	struct sti_usb3_miphy *phy_dev = platform_get_drvdata(pdev);

#if defined(MY_DEF_HERE)
	if (!IS_ERR(phy_dev->rstc))
		reset_control_assert(phy_dev->rstc);
#else  
	reset_control_assert(phy_dev->rstc);
#endif  

	usb_remove_phy(&phy_dev->phy);
#ifdef MY_DEF_HERE
	del_timer(&phy_dev->miphy_timer);

	if (phy_dev->miphy_queue)
		destroy_workqueue(phy_dev->miphy_queue);
#endif  

	return 0;
}

static const struct of_device_id sti_usb3_miphy_of_match[] = {
	{
#if defined(MY_DEF_HERE)
	 .compatible = "st,sti-usb3phy"
	},
#else  
	 .compatible = "st,sti-usb3phy",
	 .data = &sti_usb3_miphy_cfg},
#endif  
	{},
};

MODULE_DEVICE_TABLE(of, sti_usb3_miphy_of_match);

static struct platform_driver sti_usb3_miphy_driver = {
	.probe = sti_usb3_miphy_probe,
	.remove = sti_usb3_miphy_remove,
	.driver = {
		   .name = "sti-usb3-phy",
		   .owner = THIS_MODULE,
		   .of_match_table = sti_usb3_miphy_of_match,
		   }
};

module_platform_driver(sti_usb3_miphy_driver);

#ifdef MY_DEF_HERE
#ifndef MODULE
static int __init miphy_cmdline_opt(char *str)
{
	char *opt;

	if (!str || !*str)
		return -EINVAL;
	while ((opt = strsep(&str, ",")) != NULL) {
		if (!strncmp(opt, "miphy_timer_msecs:", 18)) {
			if (kstrtoint(opt + 18, 0, &miphy_timer_msecs))
				goto err;
		}
#if defined(MY_DEF_HERE)
		if (!strncmp(opt, "miphy_ssc_off:", 14)) {
			if (kstrtoint(opt + 14, 0, &miphy_ssc_off))
				goto err;
		}
#endif  
	}
	return 0;

err:
	pr_err("%s: ERROR broken module parameter\n", __func__);
	return -EINVAL;
}

__setup("miphy_st=", miphy_cmdline_opt);
#endif  
#endif  

MODULE_AUTHOR("Giuseppe Cavallaro <peppe.cavallaro@st.com>");
MODULE_DESCRIPTION("STMicroelectronics USB3 MiPHY for STiH407/STi8416 SoC");
MODULE_LICENSE("GPL v2");
