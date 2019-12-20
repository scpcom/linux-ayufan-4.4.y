/*
 * drivers/net/phy/realtek.c
 *
 * Driver for Realtek PHYs
 *
 * Author: Johnson Leung <r58129@freescale.com>
 *
 * Copyright (c) 2004 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#include <linux/phy.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>

#define RTL821x_PHYSR		0x11
#define RTL821x_PHYSR_DUPLEX	0x2000
#define RTL821x_PHYSR_SPEED	0xc000
#define RTL821x_INER		0x12
#define RTL821x_INER_INIT	0x6400
#define RTL821x_INSR		0x13
#define RTL8211E_INER_LINK_STATUS 0x400

#define RTL8211F_INER_LINK_STATUS 0x0010
#define RTL8211F_INSR		0x1d
#define RTL8211F_PAGE_SELECT	0x1f
#define RTL8211F_TX_DELAY	0x100

#define RTL8211_PAGSEL			0x1f
#define RTL8211_PAGSEL_EXT		0x0007
#define RTL8211_EXTPAGE			0x1e
#define RTL8211_EXTPAGE_110		0x006e
#define RTL8211_EXTPAGE_109		0x006d
#define RTL8211_MAGIC_PACKET_EVT	0x1000

MODULE_DESCRIPTION("Realtek PHY driver");
MODULE_AUTHOR("Johnson Leung");
MODULE_LICENSE("GPL");

struct rtl821x_priv {
    int		wol_enabled;
    u16		addr[3];
};

static int rtl821x_ack_interrupt(struct phy_device *phydev)
{
	int err;

	err = phy_read(phydev, RTL821x_INSR);

	return (err < 0) ? err : 0;
}

static int rtl8211f_ack_interrupt(struct phy_device *phydev)
{
	int err;

	phy_write(phydev, RTL8211F_PAGE_SELECT, 0xa43);
	err = phy_read(phydev, RTL8211F_INSR);
	/* restore to default page 0 */
	phy_write(phydev, RTL8211F_PAGE_SELECT, 0x0);

	return (err < 0) ? err : 0;
}

static int rtl8211b_config_intr(struct phy_device *phydev)
{
	int err;

	if (phydev->interrupts == PHY_INTERRUPT_ENABLED)
		err = phy_write(phydev, RTL821x_INER,
				RTL821x_INER_INIT);
	else
		err = phy_write(phydev, RTL821x_INER, 0);

	return err;
}

static int rtl8211e_config_intr(struct phy_device *phydev)
{
	int err;

	if (phydev->interrupts == PHY_INTERRUPT_ENABLED)
		err = phy_write(phydev, RTL821x_INER,
				RTL8211E_INER_LINK_STATUS);
	else
		err = phy_write(phydev, RTL821x_INER, 0);

	return err;
}

static int rtl8211e_probe(struct phy_device *phydev)
{
	struct device *dev = &phydev->dev;
	struct rtl821x_priv *priv;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	phydev->priv = priv;
	return 0;
}

static void rtl8211e_remove(struct phy_device *phydev)
{
	struct device *dev = &phydev->dev;
	struct rtl821x_priv *priv = phydev->priv;

	if (priv)
		devm_kfree(dev, priv);
}

static int rtl8211e_select_page(struct phy_device *phydev, int page)
{
	int err;

	/* page select external */
	err = phy_write(phydev, RTL8211_PAGSEL, RTL8211_PAGSEL_EXT);
	if (err < 0)
		return err;

	/* page select */
	return phy_write(phydev, RTL8211_EXTPAGE, page);
}

static int __rtl8211e_set_wol(struct phy_device *phydev, int enable)
{
	struct rtl821x_priv *priv = phydev->priv;
	int err;

	mutex_lock(&phydev->lock);

	if (enable) {
		err = rtl8211e_select_page(phydev, RTL8211_EXTPAGE_110);
		if (err < 0)
			goto restore_page;

		/* setting unicast MAC address */
		err = phy_write(phydev, 0x15, priv->addr[0]);
		if (err < 0)
			goto restore_page;
		err = phy_write(phydev, 0x16, priv->addr[1]);
		if (err < 0)
			goto restore_page;
		err = phy_write(phydev, 0x17, priv->addr[2]);
		if (err < 0)
			goto restore_page;

		err = rtl8211e_select_page(phydev, RTL8211_EXTPAGE_109);
		if (err < 0)
			goto restore_page;

		/* set max packet length */
		err = phy_write(phydev, 0x16, 0x1fff);
		if (err < 0)
			goto restore_page;

		/* enable all wol event */
		err = phy_write(phydev, 0x15, RTL8211_MAGIC_PACKET_EVT);

	} else {
		err = rtl8211e_select_page(phydev, RTL8211_EXTPAGE_109);
		if (err < 0)
			goto restore_page;

		/* disable WOL events */
		err = phy_write(phydev, 0x15, 0x0);
	}

restore_page:
	phy_write(phydev, RTL8211_PAGSEL, 0x0);

	mutex_unlock(&phydev->lock);
	return err;
}

static int rtl8211e_set_wol(struct phy_device *phydev,
			    struct ethtool_wolinfo *wol)
{
	struct net_device *ndev = phydev->attached_dev;
	struct rtl821x_priv *priv = phydev->priv;

	if (!wol->wolopts && priv->wol_enabled) {
		priv->wol_enabled = 0;

	} else if (wol->wolopts & WAKE_MAGIC) {
		if (!ndev || !is_valid_ether_addr(ndev->dev_addr))
			return -EINVAL;

		pr_debug("rtl8211e: setting wol\n");
		priv->wol_enabled = 1;
		priv->addr[0] = *(const u16 *)(ndev->dev_addr + 0);
		priv->addr[1] = *(const u16 *)(ndev->dev_addr + 2);
		priv->addr[2] = *(const u16 *)(ndev->dev_addr + 4);

	} else {
		pr_debug("rtl8211e: invalid wolopts %x\n", wol->wolopts);
		return -EOPNOTSUPP;
	}

	return __rtl8211e_set_wol(phydev, priv->wol_enabled);
}

static void rtl8211e_get_wol(struct phy_device *phydev,
			   struct ethtool_wolinfo *wol)
{
	wol->supported = WAKE_MAGIC;
	wol->wolopts = 0;
}

static int rtl8211e_suspend(struct phy_device *phydev)
{
	struct rtl821x_priv *priv = phydev->priv;

	/* do not power down PHY when WOL is enabled */
	if (!priv->wol_enabled)
		genphy_suspend(phydev);

	return 0;
}

static int rtl8211e_resume(struct phy_device *phydev)
{
	struct rtl821x_priv *priv = phydev->priv;
	int err = 0;

	mutex_lock(&phydev->lock);

	if (priv->wol_enabled) {
		err = rtl8211e_select_page(phydev, RTL8211_EXTPAGE_109);
		if (err < 0)
			goto restore_page;

		/* reset WOL event */
		err = phy_write(phydev, 0x16, 0x8000);

restore_page:
		phy_write(phydev, RTL8211_PAGSEL, 0x0);
	} else {
		int value;
		value = phy_read(phydev, MII_BMCR);
		phy_write(phydev, MII_BMCR, value & ~BMCR_PDOWN);
	}

	mutex_unlock(&phydev->lock);
	return err;
}

static int rtl8211f_config_intr(struct phy_device *phydev)
{
	int err;

	if (phydev->interrupts == PHY_INTERRUPT_ENABLED)
		err = phy_write(phydev, RTL821x_INER,
				RTL8211F_INER_LINK_STATUS);
	else
		err = phy_write(phydev, RTL821x_INER, 0);

	return err;
}

static int rtl8211f_config_init(struct phy_device *phydev)
{
	int ret;
	u16 reg;

	ret = genphy_config_init(phydev);
	if (ret < 0)
		return ret;

	if (phydev->interface == PHY_INTERFACE_MODE_RGMII) {
		/* enable TXDLY */
		phy_write(phydev, RTL8211F_PAGE_SELECT, 0xd08);
		reg = phy_read(phydev, 0x11);
		reg |= RTL8211F_TX_DELAY;
		phy_write(phydev, 0x11, reg);
		/* restore to default page 0 */
		phy_write(phydev, RTL8211F_PAGE_SELECT, 0x0);
	}

	return 0;
}

static struct phy_driver realtek_drvs[] = {
	{
		.phy_id         = 0x00008201,
		.name           = "RTL8201CP Ethernet",
		.phy_id_mask    = 0x0000ffff,
		.features       = PHY_BASIC_FEATURES,
		.flags          = PHY_HAS_INTERRUPT,
		.config_aneg    = &genphy_config_aneg,
		.read_status    = &genphy_read_status,
		.driver         = { .owner = THIS_MODULE,},
	}, {
		.phy_id		= 0x001cc912,
		.name		= "RTL8211B Gigabit Ethernet",
		.phy_id_mask	= 0x001fffff,
		.features	= PHY_GBIT_FEATURES,
		.flags		= PHY_HAS_INTERRUPT,
		.config_aneg	= &genphy_config_aneg,
		.read_status	= &genphy_read_status,
		.ack_interrupt	= &rtl821x_ack_interrupt,
		.config_intr	= &rtl8211b_config_intr,
		.driver		= { .owner = THIS_MODULE,},
	}, {
		.phy_id		= 0x001cc914,
		.name		= "RTL8211DN Gigabit Ethernet",
		.phy_id_mask	= 0x001fffff,
		.features	= PHY_GBIT_FEATURES,
		.flags		= PHY_HAS_INTERRUPT,
		.config_aneg	= genphy_config_aneg,
		.read_status	= genphy_read_status,
		.ack_interrupt	= rtl821x_ack_interrupt,
		.config_intr	= rtl8211e_config_intr,
		.suspend	= genphy_suspend,
		.resume		= genphy_resume,
		.driver		= { .owner = THIS_MODULE,},
	}, {
		.phy_id		= 0x001cc915,
		.name		= "RTL8211E Gigabit Ethernet",
		.phy_id_mask	= 0x001fffff,
		.features	= PHY_GBIT_FEATURES,
		.flags		= PHY_HAS_INTERRUPT,
		.config_aneg	= &genphy_config_aneg,
		.read_status	= &genphy_read_status,
		.ack_interrupt	= &rtl821x_ack_interrupt,
		.config_intr	= &rtl8211e_config_intr,
		.set_wol	= rtl8211e_set_wol,
		.get_wol	= rtl8211e_get_wol,
		.probe		= rtl8211e_probe,
		.remove		= rtl8211e_remove,
		.suspend	= rtl8211e_suspend,
		.resume		= rtl8211e_resume,
		.driver		= { .owner = THIS_MODULE,},
	}, {
		.phy_id		= 0x001cc916,
		.name		= "RTL8211F Gigabit Ethernet",
		.phy_id_mask	= 0x001fffff,
		.features	= PHY_GBIT_FEATURES,
		.flags		= PHY_HAS_INTERRUPT,
		.config_aneg	= &genphy_config_aneg,
		.config_init	= &rtl8211f_config_init,
		.read_status	= &genphy_read_status,
		.ack_interrupt	= &rtl8211f_ack_interrupt,
		.config_intr	= &rtl8211f_config_intr,
		.suspend	= genphy_suspend,
		.resume		= genphy_resume,
		.driver		= { .owner = THIS_MODULE },
	},
};

module_phy_driver(realtek_drvs);

static struct mdio_device_id __maybe_unused realtek_tbl[] = {
	{ 0x001cc912, 0x001fffff },
	{ 0x001cc914, 0x001fffff },
	{ 0x001cc915, 0x001fffff },
	{ 0x001cc916, 0x001fffff },
	{ }
};

MODULE_DEVICE_TABLE(mdio, realtek_tbl);
