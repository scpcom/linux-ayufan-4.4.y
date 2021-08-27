#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/phy.h>
#include <linux/module.h>

#define RTL821x_PHYSR		0x11
#define RTL821x_PHYSR_DUPLEX	0x2000
#define RTL821x_PHYSR_SPEED	0xc000
#define RTL821x_INER		0x12
#define RTL821x_INER_INIT	0x6400
#define RTL821x_INSR		0x13

#if defined(CONFIG_ARCH_GEN3) || defined(CONFIG_SYNO_ALPINE)
#define	RTL8211E_INER_LINK_STAT	0x10
#endif

MODULE_DESCRIPTION("Realtek PHY driver");
MODULE_AUTHOR("Johnson Leung");
MODULE_LICENSE("GPL");

static int rtl821x_ack_interrupt(struct phy_device *phydev)
{
	int err;

	err = phy_read(phydev, RTL821x_INSR);

	return (err < 0) ? err : 0;
}

#if defined(CONFIG_ARCH_GEN3) || defined(CONFIG_SYNO_ALPINE)
static int rtl8211b_config_intr(struct phy_device *phydev)
#else
static int rtl821x_config_intr(struct phy_device *phydev)
#endif
{
	int err;

	if (phydev->interrupts == PHY_INTERRUPT_ENABLED)
		err = phy_write(phydev, RTL821x_INER,
				RTL821x_INER_INIT);
	else
		err = phy_write(phydev, RTL821x_INER, 0);

	return err;
}

#if defined(CONFIG_ARCH_GEN3) || defined(CONFIG_SYNO_ALPINE)
static int rtl8211e_config_intr(struct phy_device *phydev)
{
	int err;

	if (phydev->interrupts == PHY_INTERRUPT_ENABLED)
		err = phy_write(phydev, RTL821x_INER,
				RTL8211E_INER_LINK_STAT);
	else
		err = phy_write(phydev, RTL821x_INER, 0);

	return err;
}
#endif


#if defined(CONFIG_ARCH_GEN3) || defined(CONFIG_SYNO_ALPINE)
static struct phy_driver rtl8211b_driver = {
#else
static struct phy_driver rtl821x_driver = {
#endif
#ifdef CONFIG_SYNO_ALPINE
	.phy_id         = 0x001cc914,
#else
	.phy_id		= 0x001cc912,
#endif
#if defined(CONFIG_ARCH_GEN3) || defined(CONFIG_SYNO_ALPINE)
	.name		= "RTL8211B Gigabit Ethernet",
#else
	.name		= "RTL821x Gigabit Ethernet",
#endif
	.phy_id_mask	= 0x001fffff,
	.features	= PHY_GBIT_FEATURES,
	.flags		= PHY_HAS_INTERRUPT,
	.config_aneg	= &genphy_config_aneg,
	.read_status	= &genphy_read_status,
	.ack_interrupt	= &rtl821x_ack_interrupt,
#if defined(CONFIG_ARCH_GEN3) || defined(CONFIG_SYNO_ALPINE)
	.config_intr	= &rtl8211b_config_intr,
#else
	.config_intr	= &rtl821x_config_intr,
#endif
	.driver		= { .owner = THIS_MODULE,},
};

#if defined(CONFIG_ARCH_GEN3) || defined(CONFIG_SYNO_ALPINE)
 
static struct phy_driver rtl8211e_driver = {
	.phy_id		= 0x001cc915,
	.name		= "RTL8211E Gigabit Ethernet",
	.phy_id_mask	= 0x001fffff,
	.features	= PHY_GBIT_FEATURES,
	.flags		= PHY_HAS_INTERRUPT,
	.config_aneg	= &genphy_config_aneg,
	.read_status	= &genphy_read_status,
	.ack_interrupt	= &rtl821x_ack_interrupt,
	.config_intr	= &rtl8211e_config_intr,
	.suspend	= genphy_suspend,
	.resume		= genphy_resume,
 	.driver		= { .owner = THIS_MODULE,},
 };
#endif

static int __init realtek_init(void)
{
	int ret;

#if defined(CONFIG_ARCH_GEN3) || defined(CONFIG_SYNO_ALPINE)
	ret = phy_driver_register(&rtl8211b_driver);
	if (ret < 0)
		return -ENODEV;
	return phy_driver_register(&rtl8211e_driver);
#else
	ret = phy_driver_register(&rtl821x_driver);

	return ret;
#endif
}

static void __exit realtek_exit(void)
{
#if defined(CONFIG_ARCH_GEN3) || defined(CONFIG_SYNO_ALPINE)
	phy_driver_unregister(&rtl8211b_driver);
	phy_driver_unregister(&rtl8211e_driver);
#else
	phy_driver_unregister(&rtl821x_driver);
#endif
}

module_init(realtek_init);
module_exit(realtek_exit);

static struct mdio_device_id __maybe_unused realtek_tbl[] = {
#ifdef CONFIG_SYNO_ALPINE
	{ 0x001cc914, 0x001fffff },
#else
	{ 0x001cc912, 0x001fffff },
#endif
#if defined(CONFIG_ARCH_GEN3) || defined(CONFIG_SYNO_ALPINE)
	{ 0x001cc915, 0x001fffff },
#endif
	{ }
};

MODULE_DEVICE_TABLE(mdio, realtek_tbl);
