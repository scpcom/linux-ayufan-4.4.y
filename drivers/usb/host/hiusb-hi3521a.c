#include <linux/init.h>
#include <linux/timer.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/spinlock.h>
#include <asm/byteorder.h>
#include <linux/io.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <mach/hardware.h>

#define PERI_CRG28		(__io_address(0x12040000 + 0x70))
#define USB_CKEN		(1 << 7)
#define USB_CTRL_UTMI1_REG	(1 << 6)
#define USB_CTRL_UTMI0_REG	(1 << 5)
#define USB_CTRL_HUB_REG	(1 << 4)
#define USBPHY_PORT1_TREQ	(1 << 3)
#define USBPHY_PORT0_TREQ	(1 << 2)
#define USBPHY_REQ		(1 << 1)
#define USB_AHB_SRST_REQ	(1 << 0)

#define PERI_USB		(__io_address(0x12120000 + 0x50))
#define WORDINTERFACE		(1 << 0)
#define SS_BURST4_EN		(1 << 7)
#define SS_BURST8_EN		(1 << 8)
#define SS_BURST16_EN		(1 << 9)
#define USBOVR_P_CTRL		(1 << 17)
#define MISC_USB		(__io_address(0x12120000 + 0x54))
static atomic_t dev_open_cnt = {
	.counter = 0,
};

void hiusb_start_hcd(void)
{
	unsigned long flags;

	local_irq_save(flags);
	if (atomic_add_return(1, &dev_open_cnt) == 1) {

		int reg;
		/* enable phy ref clk to enable phy */
		reg = readl(PERI_CRG28);
		reg |= USB_CKEN;
		writel(reg, PERI_CRG28);
		udelay(100);

		/* config controller */
		reg = readl(PERI_USB);
		reg &= ~(WORDINTERFACE); /* 8bit */
		/* disable ehci burst16 mode*/
		reg &= ~(SS_BURST16_EN);
		reg |= USBOVR_P_CTRL;
		writel(reg, PERI_USB);
		udelay(100);

		/* de-assert phy port */
		reg = readl(PERI_CRG28);
		reg &= ~(USBPHY_REQ);
		writel(reg, PERI_CRG28);
		udelay(300);

		/* open phy clk */
		writel(0x406, MISC_USB);
		udelay(10);
		writel(0x426, MISC_USB);
		mdelay(8);

		/* usb2.0 phy eye pattern */
		/* Icomp = 212.5 RCOMP = 212.5 */
		writel(0xa, MISC_USB);
		udelay(10);
		writel(0xbb2a, MISC_USB);
		mdelay(5);

		writel(0x5, MISC_USB);
		udelay(10);
		writel(0x9225, MISC_USB);
		mdelay(5);

		writel(0x6, MISC_USB);
		udelay(10);
		writel(0x626, MISC_USB);
		mdelay(5);

		/*close eop pre-emphasis*/
		writel(0x0, MISC_USB);
		udelay(10);
		writel(0x1820, MISC_USB);
		mdelay(5);

		writel(0x10, MISC_USB);
		udelay(10);
		writel(0x1830, MISC_USB);
		mdelay(5);

		/* open port1 pre-emphasis */
		writel(0x10, MISC_USB);
		udelay(10);
		writel(0x1c30, MISC_USB);
		mdelay(5);

		/* cancel phy utmi reset */
		reg = readl(PERI_CRG28);
		reg &= ~(USBPHY_PORT0_TREQ);
		reg &= ~(USBPHY_PORT1_TREQ);
		writel(reg, PERI_CRG28);
		udelay(300);

		/* de-assert all the rsts of ctrl */
		reg = readl(PERI_CRG28);
		reg &= ~(USB_CTRL_UTMI0_REG);
		reg &= ~(USB_CTRL_UTMI1_REG);
		reg &= ~(USB_CTRL_HUB_REG);
		reg &= ~(USB_AHB_SRST_REQ);
		writel(reg, PERI_CRG28);
		udelay(200);

	}
	local_irq_restore(flags);
}
EXPORT_SYMBOL(hiusb_start_hcd);

void hiusb_stop_hcd(void)
{
	unsigned long flags;

	local_irq_save(flags);
	if (atomic_sub_return(1, &dev_open_cnt) == 0) {

		int reg;

		/* Disable EHCI clock.
		If the HS PHY is unused disable it too. */

		reg = readl(PERI_CRG28);
		reg &= ~(USB_CKEN);
		reg |= (USB_CTRL_UTMI0_REG);
		reg |= (USB_CTRL_UTMI1_REG);
		reg |= (USB_CTRL_HUB_REG);
		reg |= (USBPHY_PORT0_TREQ);
		reg |= (USBPHY_PORT1_TREQ);
		reg |= (USBPHY_REQ);
		reg |= (USB_AHB_SRST_REQ);
		writel(reg, PERI_CRG28);
		udelay(100);

		/* enable phy */
		reg = readl(PERI_USB);
		reg |= (WORDINTERFACE);
		reg |= (SS_BURST16_EN);
		reg |= (USBOVR_P_CTRL);
		writel(reg, PERI_USB);
		udelay(100);
	}
	local_irq_restore(flags);
}
EXPORT_SYMBOL(hiusb_stop_hcd);
