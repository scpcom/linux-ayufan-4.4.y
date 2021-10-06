#if defined(MY_DEF_HERE)
/*
 *  phy-rtk-usb.h RTK usb phy header file
 *
 * copyright (c) 2017 realtek semiconductor corporation
 *
 * this program is free software; you can redistribute it and/or modify
 * it under the terms of the gnu general public license as published by
 * the free software foundation; either version 2 of the license, or
 * (at your option) any later version.
 */
#endif /* MY_DEF_HERE */

#ifndef __PHY_RTK_USB_H__
#define __PHY_RTK_USB_H__

struct rtk_usb_phy_s {
	struct usb_phy phy;
	struct device *dev;

#if defined(MY_DEF_HERE)
	enum rtd_chip_id chip_id;
	enum rtd_chip_revision chip_revision;

	int phyN;
#else /* MY_DEF_HERE */
	int portN;
#endif /* MY_DEF_HERE */
	void *reg_addr;
	void *phy_data;

#if defined(MY_DEF_HERE)
#ifdef CONFIG_DYNAMIC_DEBUG
	struct dentry		*debug_dir;
#endif
#endif /* MY_DEF_HERE */
};

struct rtk_usb_phy_data_s {
	char addr;
	char data;
};

#if defined(MY_DEF_HERE)
#define phy_read(addr)			__raw_readl(addr)
#define phy_write(addr, val)	do { smp_wmb(); __raw_writel(val, addr); } while(0)
#define PHY_IO_TIMEOUT_MSEC		(50)

static inline int utmi_wait_register(void __iomem *reg, u32 mask, u32 result)
{
	unsigned long timeout = jiffies + msecs_to_jiffies(PHY_IO_TIMEOUT_MSEC);
	while (time_before(jiffies, timeout)) {
		smp_rmb();
		if ((phy_read(reg) & mask) == result)
			return 0;
		udelay(100);
	}
	pr_err("\033[0;32;31m can't program USB phy \033[m\n");
	return -1;
}
#endif /* MY_DEF_HERE */

#endif /* __PHY_RTK_USB_H__ */
