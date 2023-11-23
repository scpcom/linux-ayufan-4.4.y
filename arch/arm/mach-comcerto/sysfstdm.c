 /*
  *  Copyright (C) 2008 Mindspeed Technologies, Inc.
  *
  * This program is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation; either version 2 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program; if not, write to the Free Software
  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <asm/io.h>
#include <asm/delay.h>
#include <mach/comcerto-common.h>
#include <asm/div64.h>
#include <mach/comcerto-2000/clk-rst.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <mach/reset.h>

#define tdm_readl(r) readl((void*)(r))
#define tdm_writel(v,r) writel(v, (void*)(r))

/* Defalult Rate value is in Hz*/
#define TDMNTG_DEFAULT_REF_CLK 500000000

/* since 'ppm' means parts-per-million */
#define MILLION (1000000UL)

/* NTG to TDM clock dividers */
#define COMCERTO_BLOCK_TDM_DIV	1
#define COMCERTO_BLOCK_ZDS_DIV	24
#define COMCERTO_BLOCK_GPIO_DIV	1
#define COMCERTO_BLOCK_MSIF_DIV	12
static int comcerto_block_clk_div [4] = {COMCERTO_BLOCK_TDM_DIV, COMCERTO_BLOCK_ZDS_DIV, COMCERTO_BLOCK_GPIO_DIV, COMCERTO_BLOCK_MSIF_DIV};
static int block_selected;
static int block_sel_cnt = 0;
static spinlock_t block_lock;

/*ntgref clock*/
static struct clk *clk_ntg_ref;
static unsigned long tdmntg_ref_clk;

static unsigned long BaseClock = 0; /* frequency is set by clock_frequency_set() */

static void fsync_output_set(unsigned int fsoutput)
{
	if (fsoutput)
	{
		tdm_writel(tdm_readl(COMCERTO_GPIO_TDM_MUX) | (1 << 0), COMCERTO_GPIO_TDM_MUX);
		tdm_writel(tdm_readl(TDM_FSYNC_GEN_CTRL) | (1 << 0), TDM_FSYNC_GEN_CTRL);
	}
	else
	{
		tdm_writel(tdm_readl(COMCERTO_GPIO_TDM_MUX) & ~(1 << 0), COMCERTO_GPIO_TDM_MUX);
		tdm_writel(tdm_readl(TDM_FSYNC_GEN_CTRL) & ~(1 << 0), TDM_FSYNC_GEN_CTRL);
	}
}

static void fsync_polarity_set(unsigned int fspolarity)
{
	/* 28 FSYNC_FALL(RISE)_EDGE */
	if (fspolarity)
		tdm_writel(tdm_readl(TDM_FSYNC_GEN_CTRL) | FSYNC_FALL_EDGE, TDM_FSYNC_GEN_CTRL);
	else
		tdm_writel(tdm_readl(TDM_FSYNC_GEN_CTRL) & ~FSYNC_FALL_EDGE, TDM_FSYNC_GEN_CTRL);
}

static void fsync_lphase_set(u32 fslwidth)
{
	/* Low_Phase_Width 7ff- maximum */
	if (fslwidth > 0x7FF) {
		printk(KERN_ERR "%s: Low Phase width value is out of range %#x > 0x7FF\n", __func__, fslwidth);
		return;
	}

	tdm_writel(fslwidth, TDM_FSYNC_LOW);
}

static void fsync_hphase_set(u32 fshwidth)
{
	/* High_Phase_Width 7ff- maximum */
        if (fshwidth > 0x7FF) {
		printk(KERN_ERR "%s: High Phase width value is out of range %#x > 0x7FF\n", __func__, fshwidth);
		return;
	}

	tdm_writel(fshwidth, TDM_FSYNC_HIGH);
}

static void clock_frequency_set(unsigned long clockhz)
{
	unsigned long long ntg_incr ;

	BaseClock = clockhz;

	/* Calculate NTG clock: multiply TDM base clock with block divider */	
	/* clock_frequency_set() is called with block_lock taken */
	ntg_incr =  comcerto_block_clk_div[block_selected] * clockhz;
	/* get frequency resolution on an 32-bit accumulator */
	ntg_incr = ntg_incr * (1ULL << 32) + tdmntg_ref_clk / 2;
	do_div(ntg_incr, tdmntg_ref_clk);
	printk(KERN_INFO "%s: (%d:%d) NTG INCR value is %llu\n", __func__, block_selected, comcerto_block_clk_div[block_selected], ntg_incr);

	/* ntg_incr = 0x10C6F7A for 2.048 MHz */
	/* ntg_incr = 0xC953969 for 24.576 MHz */
	/* ntg_incr = 0x192A7371 for 49.152 MHz */
	tdm_writel(ntg_incr, TDM_NTG_INCR);
}

static unsigned long clock_frequency_get(void)
{
	unsigned long long clc_data;

	/* According to the desired TDM clock output frequency, this field should be configured */
	clc_data = (tdm_readl(TDM_NTG_INCR) & 0x3FFFFFFF);/* get frequency from resolution on an 32-bit accumulator */
	clc_data = (clc_data * tdmntg_ref_clk + (1ULL << 31)) >> 32;
	/* Divide down the Data with TDM block divider */
	do_div(clc_data, comcerto_block_clk_div[block_selected]); /* do_div because of 64 bit operation*/
	return (unsigned long)(clc_data);
}

static void clock_output_set(unsigned long clockout)
{
	switch (clockout) {
	case 0:
		tdm_writel((0x2 << 12) | (tdm_readl(COMCERTO_GPIO_BOOTSTRAP_OVERRIDE) & ~(0x3 << 12)), COMCERTO_GPIO_BOOTSTRAP_OVERRIDE);
		break;
	case 1:
		tdm_writel((0x3 << 12) | (tdm_readl(COMCERTO_GPIO_BOOTSTRAP_OVERRIDE) &	~(0x3 << 12)), COMCERTO_GPIO_BOOTSTRAP_OVERRIDE);
		break;
	case 2:
		tdm_writel((0x0 << 12) | (tdm_readl(COMCERTO_GPIO_BOOTSTRAP_OVERRIDE) &	~(0x3 << 12)), COMCERTO_GPIO_BOOTSTRAP_OVERRIDE);
		break;
	default:
		printk(KERN_ERR "%s: Unknown clock output value\n", __func__);
	}
}

static void clock_ppm_adjust(long ppm)
{
	unsigned long long clc_data;
	unsigned long freq_set = BaseClock;
	int nsign = 0;

	if (!freq_set) {
		printk(KERN_ERR "(%s): Could not adjust frequency: you should set it before\n", __func__);
		return;
	}

	if (ppm < 0) {
		nsign = 1;
		ppm = -ppm;
	}

	if (nsign && (ppm >= MILLION)) {
		/* overflow dangerous */
		printk(KERN_ERR "(%s): This is too much ppm: -%lu\n", __func__, (unsigned long)ppm);
		return;
	}

	clc_data = ppm * (1ULL << 32);

	do_div(clc_data, MILLION);

	if (nsign) {
		clc_data = (1ULL << 32) - clc_data;
	} else {
		clc_data = (1ULL << 32) + clc_data;
	}

	clc_data = clc_data * freq_set + tdmntg_ref_clk / 2; /* with rounding to nearest integer */
	clc_data *= comcerto_block_clk_div[block_selected];
	do_div(clc_data, tdmntg_ref_clk);

	if (clc_data & ~0x3FFFFFFF) {
		/* unaccounted bits dangerous */
		printk(KERN_ERR "(%s): This is too much ppm: %lu\n", __func__, (unsigned long)ppm);
		return;
	}

	tdm_writel((clc_data & 0x3FFFFFFF) | (tdm_readl(TDM_NTG_INCR) & ~0x3FFFFFFF), TDM_NTG_INCR);
}

static long clock_ppm_get(void)
{
	unsigned long freq_set = BaseClock;
	unsigned long freq = clock_frequency_get();
	unsigned long long ppm;

	if (freq > freq_set) {
		ppm = (freq - freq_set) * MILLION + (freq_set >> 1);
		do_div(ppm, freq_set);
		return (long)ppm;
	} else {
		ppm = (freq_set - freq) * MILLION + (freq_set >> 1);
		do_div(ppm, freq_set);
		return (-1 * (long)ppm);
	}
}

#define TDM_CTRL_SLIC_RESET 	0x80
#define TDM_CTRL_CLK_DIV_BYPASS	0x40
#define TDM_CTRL_DEF_DIV 	0x2
#define TDM_CTLR_RESET_BYPASS TDM_CTRL_SLIC_RESET | TDM_CTRL_CLK_DIV_BYPASS | TDM_CTRL_DEF_DIV

static void tdm_mux_set(u32 tdmmux)
{
	block_selected = tdmmux;
	/* TDM interface Muxing [5:4]
	00	TDM block is selected
	01	ZDS block (Zarlink) is selected
	10	GPIO[63:60] signals are selected
	11	MSIF block (SiLabs) is selected */

	switch (block_selected){
	case 0:
		/* TDM block selected (SiLabs si3227) */
		tdm_writel(TDM_CTLR_RESET_BYPASS, TDM_CLK_CNTRL);    /* bypass TDM divider --> TDM = NTG out, keen ZDS/MSIF Slic reset */
		tdm_writel((0x0 << 4) |(tdm_readl(COMCERTO_GPIO_MISC_PIN_SELECT) & ~(0x3 << 4)), COMCERTO_GPIO_MISC_PIN_SELECT);
		break;

	case 1:
		/* ZDS block selected (Zarlink le88264) */
		tdm_writel(TDM_CTRL_SLIC_RESET | COMCERTO_BLOCK_ZDS_DIV, TDM_CLK_CNTRL); /* TDM = NTG out / 24*/
		tdm_writel((0x1 << 4) |(tdm_readl(COMCERTO_GPIO_MISC_PIN_SELECT) & ~(0x3 << 4)), COMCERTO_GPIO_MISC_PIN_SELECT);
		tdm_writel(COMCERTO_BLOCK_ZDS_DIV, TDM_CLK_CNTRL); /* Remove out of reset */
		break;

	case 2:
		/* GPIO[63:60] signals selected */
		tdm_writel(TDM_CTLR_RESET_BYPASS, TDM_CLK_CNTRL); /* bypass TDM divider --> TDM = NTG out, keen ZDS/MSIF Slic reset */
		tdm_writel((0x2 << 4) |(tdm_readl(COMCERTO_GPIO_MISC_PIN_SELECT) & ~(0x3 << 4)), COMCERTO_GPIO_MISC_PIN_SELECT);
		break;

	case 3:
		/* MSIF block selected (SiLabs si32268) */
		tdm_writel(TDM_CTRL_SLIC_RESET | COMCERTO_BLOCK_MSIF_DIV, TDM_CLK_CNTRL); /* TDM = NTG out / 12 */
		tdm_writel((0x3 << 4) |(tdm_readl(COMCERTO_GPIO_MISC_PIN_SELECT) & ~(0x3 << 4)), COMCERTO_GPIO_MISC_PIN_SELECT);
		tdm_writel(COMCERTO_BLOCK_MSIF_DIV, TDM_CLK_CNTRL); /* Remove out of reset */

		/* Delay 100us after C2k TDM block has been un-reset, before SLIC is un-reset.
		   Ensures MSIF interface clock is active well before SLIC is un-reset (SiLabs spec). 
		*/
		udelay(100);

		tdm_writel(0x1 << 30, COMCERTO_GPIO_63_32_PIN_OUTPUT); /* remove slic out of reset */
		break;

	default:
		printk(KERN_ERR "%s: Unknown TDM MUX value\n", __func__);
	}
}

#if 0
static void tdm_dr_set(u32 tdmdr)
{
	if(tdmdr > 0x3F)
	{
		printk(KERN_ERR "%s: TDM_DR value is out of range %#x >	0x3F\n", 	__func__, tdmdr);
		return;
	}

	tdm_writel((tdmdr << 24) |(tdm_readl(COMCERTO_GPIO_PAD_CONFIG0) & ~(0x3F << 24)), COMCERTO_GPIO_PAD_CONFIG0);
}

static void tdm_dx_set(u32 tdmdx)
{
	if(tdmdx > 0x3F)
	{
		printk(KERN_ERR "%s: TDM_DX value is out of range %#x > 0x3F\n", __func__, tdmdx);
		return;
	}

	tdm_writel((tdmdx << 18) |(tdm_readl(COMCERTO_GPIO_PAD_CONFIG0) & ~(0x3F << 18)), COMCERTO_GPIO_PAD_CONFIG0);
}

static void tdm_fs_set(u32 tdmfs)
{
	if(tdmfs > 0x3F)
	{
		printk(KERN_ERR "%s: TDM_FS value is out of range %#x > 0x3F\n", __func__, tdmfs);
		return;
	}

	tdm_writel((tdmfs << 12) |(tdm_readl(COMCERTO_GPIO_PAD_CONFIG0) & ~(0x3F << 12)), COMCERTO_GPIO_PAD_CONFIG0);
}

static void tdm_ck_set(u32 tdmck)
{
	if(tdmck > 0x3F)
	{
		printk(KERN_ERR "%s: TDM_CK value is out of range %#x > 0x3F\n", __func__, tdmck);
		return;
	}

	tdm_writel((tdmck << 6) |(tdm_readl(COMCERTO_GPIO_PAD_CONFIG0) & ~(0x3F << 6)), COMCERTO_GPIO_PAD_CONFIG0);
}
#endif

int tdm_get_block(unsigned int block)
{
	unsigned long flags;
	int rc = 0;

	/* 0x0 - TDM block, 0x1 - ZDS block, 0x2 - GPIO[63:60] signals and 0x3 - MSIF block */
	printk(KERN_INFO "%s: get block %d (selected %d)\n", __func__, block, block_selected);
	if (block > 3)
		return -EINVAL;

	spin_lock_irqsave(&block_lock, flags);

	if(block == block_selected)
	{
		block_sel_cnt++;
	}
	else
	{
		if (block_sel_cnt)
		{
			rc = -EBUSY;
			goto out;
		}

		block_sel_cnt++;
		/* Selected block global variable will be updated */
		tdm_mux_set(block);
		/* Clock to be recalculated accordingly to selected block */
		clock_frequency_set(BaseClock);
	}

out:
	spin_unlock_irqrestore(&block_lock,flags);

	printk(KERN_INFO "%s: get block result %d\n", __func__, rc);
	return rc;
}

void tdm_put_block(unsigned int block)
{
	unsigned long flags;
	spin_lock_irqsave(&block_lock, flags);

	printk(KERN_INFO "%s: put block %d (selected %d, cnt %d)\n", 
		__func__, block, block_selected, block_sel_cnt);

	if (block == block_selected)
	{
		if (block_sel_cnt)
			block_sel_cnt--;
		else
			printk(KERN_ERR "%s: block hasn't selected yet\n", __func__);
	}
	else
		printk(KERN_ERR "%s: Invalid block\n", __func__);

	spin_unlock_irqrestore(&block_lock,flags);
}

static ssize_t tdm_data_read(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	if (strcmp("fs_polarity", attr->attr.name) == 0) /* FSYNC_FALL(RISE)_EDGE */
		return sprintf(buf, "%d\n", (tdm_readl(TDM_FSYNC_GEN_CTRL) >> 1) & 0x1);
	else if (strcmp("fs_lwidth", attr->attr.name) == 0) /* Low_Phase_Width */
		return sprintf(buf, "%x\n", (tdm_readl(TDM_FSYNC_LOW) & 0x3FF));
	else if (strcmp("fs_hwidth", attr->attr.name) == 0) /* High_Phase_Width */
		return sprintf(buf, "%x\n", (tdm_readl(TDM_FSYNC_HIGH) & 0x3FF));
	else if (strcmp("fs_output", attr->attr.name) == 0) /* Generic Pad Control and Version ID Register[2] */
		return sprintf(buf, "%d\n", tdm_readl(COMCERTO_GPIO_TDM_MUX) & 0x1);
	else if (strcmp("clock_ppmshift", attr->attr.name) == 0)
		return sprintf(buf, "%ld\n", clock_ppm_get());
	else if (strcmp("clock_output", attr->attr.name) == 0)
		return sprintf(buf, "%d\n", (tdm_readl(COMCERTO_GPIO_SYSTEM_CONFIG)	>> 3) & 0x1);
	else if (strcmp("clock_hz", attr->attr.name) == 0)
		return sprintf(buf, "%lu\n", clock_frequency_get());
	else if (strcmp("tdm_mux", attr->attr.name) == 0)
                return sprintf(buf, "%u\n", (tdm_readl(COMCERTO_GPIO_MISC_PIN_SELECT) >> 4) & 0x3);
	else
	{
		printk(KERN_ERR "%s: Unknown file attribute\n", __func__);
		return -1;
	}
}

static ssize_t tdm_data_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long tdm_data = simple_strtoul(buf, NULL, 0);

	if (strcmp("fs_polarity", attr->attr.name) == 0)
		fsync_polarity_set(tdm_data);
	else if (strcmp("fs_lwidth", attr->attr.name) == 0)
		fsync_lphase_set(tdm_data);
	else if (strcmp("fs_hwidth", attr->attr.name) == 0)
		fsync_hphase_set(tdm_data);
	else if (strcmp("fs_output", attr->attr.name) == 0)
		fsync_output_set(tdm_data);
	else if (strcmp("clock_hz", attr->attr.name) == 0)
		clock_frequency_set(tdm_data);
	else if (strcmp("clock_output", attr->attr.name) == 0)
		clock_output_set(tdm_data);
	else if (strcmp("clock_ppmshift", attr->attr.name) == 0)
		clock_ppm_adjust(simple_strtol(buf, NULL, 0));
	else if (strcmp("tdm_mux", attr->attr.name) == 0) {
		if (tdm_data != block_selected)
		{
			tdm_put_block(block_selected);
			tdm_get_block(tdm_data);
		}
		else
			printk(KERN_INFO "%s: TDM block %d is already selected\n", __func__, block_selected);
	}
	else
		printk(KERN_ERR "%s: Unknown file attribute \n", __func__);

	return count;
}

static struct device_attribute fsoutput_attr = __ATTR(fs_output, 0644, tdm_data_read, tdm_data_write);
static struct device_attribute fspolarity_attr = __ATTR(fs_polarity, 0644, tdm_data_read, tdm_data_write);
static struct device_attribute fshwidth_attr = __ATTR(fs_hwidth, 0644, tdm_data_read, tdm_data_write);
static struct device_attribute fslwidth_attr = __ATTR(fs_lwidth, 0644, tdm_data_read, tdm_data_write);
static struct device_attribute clockhz_attr = __ATTR(clock_hz, 0644, tdm_data_read, tdm_data_write);
static struct device_attribute clockout_attr = __ATTR(clock_output, 0644, tdm_data_read, tdm_data_write);
static struct device_attribute clockppm_attr = __ATTR(clock_ppmshift, 0644, tdm_data_read, tdm_data_write);
static struct device_attribute tdmmux_attr = __ATTR(tdm_mux, 0644, tdm_data_read, tdm_data_write);

static int comcerto_tdm_probe(struct platform_device *pdev)
{
	struct comcerto_tdm_data *pdata = (struct comcerto_tdm_data *)pdev->dev.platform_data;
	
	int ret = 0;

	spin_lock_init(&block_lock);
	
	/* Get the reference to ntgref clock structure*/
	clk_ntg_ref = clk_get(NULL,"ntgref");
	
	/* Error Handling , if no ntgref clock reference: return error */
	if (IS_ERR(clk_ntg_ref)) {
		pr_err("%s: Unable to obtain ntgref clock: %ld\n",__func__,PTR_ERR(clk_ntg_ref));
		return PTR_ERR(clk_ntg_ref);
	}

	/* Take TDM NTG out of reset*/
	c2000_block_reset(COMPONENT_TDMNTG,0);

	/*Enable the TDMNTG_REF  clock*/ 
        ret = clk_enable(clk_ntg_ref);
	
	if (ret){
		pr_err("%s: Unable to enable ntgref clock \n",__func__); 
		return ret;
	}

	/* Set the rate value to 500000000 Hz for the ref clock */ 
	clk_set_rate(clk_ntg_ref,TDMNTG_DEFAULT_REF_CLK);

 	/*Initialize the tdmntgref clock rate value */ 	
	tdmntg_ref_clk = clk_get_rate(clk_ntg_ref);

	tdm_writel((NTG_DIV_RST_N | NTG_EN), TDM_NTG_CLK_CTRL);

	/* Inital configuration of tdm bus */
	tdm_mux_set(pdata->tdmmux);
	fsync_polarity_set(pdata->fspolarity);
	fsync_lphase_set(pdata->fslwidth);
	fsync_hphase_set(pdata->fshwidth);
	clock_frequency_set(pdata->clockhz);
	clock_output_set(pdata->clockout);
	fsync_output_set(pdata->fsoutput);

#if 0 // The default paramters are good
	tdm_dr_set(pdata->tdmdr);
	tdm_dx_set(pdata->tdmdx);
	tdm_fs_set(pdata->tdmfs);
	tdm_ck_set(pdata->tdmck);
#endif

	/* Creating sysfs files */
	ret |= device_create_file(&pdev->dev, &fsoutput_attr);
	ret |= device_create_file(&pdev->dev, &fspolarity_attr);
	ret |= device_create_file(&pdev->dev, &fshwidth_attr);
	ret |= device_create_file(&pdev->dev, &fslwidth_attr);
	ret |= device_create_file(&pdev->dev, &clockhz_attr);
	ret |= device_create_file(&pdev->dev, &clockout_attr);
	ret |= device_create_file(&pdev->dev, &clockppm_attr);
	ret |= device_create_file(&pdev->dev, &tdmmux_attr);

	return ret;
}

static int comcerto_tdm_remove(struct platform_device *pdev)
{
	/* Disable the TDMNTG_REFclock */
	clk_disable(clk_ntg_ref);
	clk_put(clk_ntg_ref);
	
	/* Puuting TDMNTG is reset mode */
	c2000_block_reset(COMPONENT_TDMNTG,1);

	/* Remove the Device  File  */
	device_remove_file(&pdev->dev, &fsoutput_attr);
	device_remove_file(&pdev->dev, &fspolarity_attr);
	device_remove_file(&pdev->dev, &fshwidth_attr);
	device_remove_file(&pdev->dev, &fslwidth_attr);
	device_remove_file(&pdev->dev, &clockhz_attr);
	device_remove_file(&pdev->dev, &clockout_attr);
	device_remove_file(&pdev->dev, &clockppm_attr);
	device_remove_file(&pdev->dev, &tdmmux_attr);

	return 0;
}

/* Structure for a device driver */
static struct platform_driver comcerto_tdm_driver = {
	.probe = comcerto_tdm_probe,
	.remove = comcerto_tdm_remove,
	.driver	= {
		.name = "comcerto-tdm",
	},
};

static int  comcerto_tdm_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&comcerto_tdm_driver);

	return ret;
}

static void  comcerto_tdm_exit(void)
{
	platform_driver_unregister(&comcerto_tdm_driver);
}

module_init(comcerto_tdm_init);
module_exit(comcerto_tdm_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Comcerto TDM Driver");

