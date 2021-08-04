/*
 * Driver for Atheros PHYs
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/phy.h>

/* Atheros PHY registers*/
#define AR8x_INTR_EN			0x12
#define AR8x_INTR_STS			0x13
#define AR8x_DEBUG_PORT_ADDR		0x1D
#define AR8x_DEBUG_PORT_DATA		0x1E

/* Interrupt mask */
#define AR8x_INTR_MASK			0xfc03

/* Atheros PHY registers on debug port */
/* RGMII Rx clock control */
#define AR8x_DBG_RGMII_RXCLK_CTRL	0x00
#define AR8x_DBG_RGMII_RXCLK_MASK	0x8000

/*RGMII Tx clock control */
#define AR8x_DBG_RGMII_TXCLK_CTRL	0x05
#define AR8x_DBG_RGMII_TXCLK_MASK	0x0100

#define PHY_ID_AR8035			0x004dd072
#define PHY_ID_AR8033			0x004dd074
#define PHY_ID_AR8327			0x004dd033

MODULE_DESCRIPTION("Atheros PHY driver");
MODULE_LICENSE("GPL");

static int ar8x_phy_dbg_read(struct phy_device *phydev, int reg_addr)
{
	phy_write(phydev, AR8x_DEBUG_PORT_ADDR, reg_addr);
	return  phy_read(phydev, AR8x_DEBUG_PORT_DATA);
}

static void ar8x_phy_dbg_write(struct phy_device *phydev,  int reg_addr, u32 val)
{
	phy_write(phydev, AR8x_DEBUG_PORT_ADDR, reg_addr);
	phy_write(phydev, AR8x_DEBUG_PORT_DATA, val);
}

int ar8x_add_skew(struct phy_device *phydev)
{
	int tmp, err;

	/* Enable Rx delay */
	tmp = ar8x_phy_dbg_read(phydev, AR8x_DBG_RGMII_RXCLK_CTRL);
	tmp |= AR8x_DBG_RGMII_RXCLK_MASK;
	ar8x_phy_dbg_write(phydev, AR8x_DBG_RGMII_RXCLK_CTRL, tmp);
	/* Enable Tx delay */
	tmp = ar8x_phy_dbg_read(phydev, AR8x_DBG_RGMII_TXCLK_CTRL);
	tmp |= AR8x_DBG_RGMII_TXCLK_MASK;
	ar8x_phy_dbg_write(phydev, AR8x_DBG_RGMII_TXCLK_CTRL, tmp);

	err = genphy_restart_aneg(phydev);

	if (err < 0)
		return err;

	return 0;
}
EXPORT_SYMBOL(ar8x_add_skew);

static int ar8x_config_init(struct phy_device *phydev)
{
	int err = 0;

	if (phydev->interface == PHY_INTERFACE_MODE_RGMII)
		err = ar8x_add_skew(phydev);

	return err;

}


static int ar8x_ack_interrupt(struct phy_device *phydev)
{
	int err = 0;

	if (phydev->interrupts == PHY_INTERRUPT_ENABLED)
		err = phy_read(phydev, AR8x_INTR_STS);

	return (err < 0) ? err : 0;
}

static int ar8x_config_intr(struct phy_device *phydev)
{
	int err;

	if (phydev->interrupts == PHY_INTERRUPT_ENABLED)
		err = phy_write(phydev, AR8x_INTR_EN, AR8x_INTR_MASK);
	else {
		/* Clear the interrupts */
		err = phy_read(phydev, AR8x_INTR_STS);

		if (err < 0)
			return err;

		err = phy_write(phydev, AR8x_INTR_EN, 0);
	}

	return err;
}

/* Atheros Ar8035 */
static struct phy_driver ar8035_driver = {
	.phy_id		= PHY_ID_AR8035,
	.name		= "Atheros AR8035",
	.phy_id_mask	= 0xffffff00,
	.features	= PHY_GBIT_FEATURES,
	.flags		= PHY_HAS_INTERRUPT,
	.config_init	= &ar8x_config_init,
	.config_aneg	= &genphy_config_aneg,
	.read_status	= &genphy_read_status,
	.ack_interrupt	= &ar8x_ack_interrupt,
	.config_intr	= &ar8x_config_intr,
#ifdef CONFIG_PM
	.suspend	= &genphy_suspend,
	.resume		= &genphy_resume,
#endif
	.driver 	= { .owner = THIS_MODULE,},
};

/* Atheros Ar8327 */
static struct phy_driver ar8327_driver = {
	.phy_id		= PHY_ID_AR8327,
	.name		= "Atheros AR8327",
	.phy_id_mask	= 0xffffff00,
	.features	= PHY_GBIT_FEATURES,
	.flags		= PHY_HAS_INTERRUPT,
	.config_init	= &ar8x_config_init,
	.config_aneg	= &genphy_config_aneg,
	.read_status	= &genphy_read_status,
	.ack_interrupt	= &ar8x_ack_interrupt,
	.config_intr	= &ar8x_config_intr,
#ifdef CONFIG_PM
	.suspend	= &genphy_suspend,
	.resume		= &genphy_resume,
#endif
	.driver 	= { .owner = THIS_MODULE,},
};

static int __init ar8x_init(void)
{
	int err;

	err = phy_driver_register(&ar8035_driver);
	if (err < 0)
		return err;

	err = phy_driver_register(&ar8327_driver);
	if (err < 0)
		phy_driver_unregister(&ar8035_driver);

	return err;
}

static void __exit ar8x_exit(void)
{
	phy_driver_unregister(&ar8035_driver);
	phy_driver_unregister(&ar8327_driver);
}

module_init(ar8x_init);
module_exit(ar8x_exit);

static struct mdio_device_id __maybe_unused atheros_tbl[] = {
	{ PHY_ID_AR8035, 0xffffff00 },
	{ PHY_ID_AR8327, 0xffffff00 },
	{ }
};

MODULE_DEVICE_TABLE(mdio, atheros_tbl);
