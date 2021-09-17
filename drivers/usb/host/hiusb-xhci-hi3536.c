#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/types.h>
#include <mach/hardware.h>

#include "xhci.h"
#include "hiusb.h"

MODULE_LICENSE("Dual MIT/GPL");

#define PERI_CRG46		__io_address(0x12040000 + 0xb8)
#define USB3_VCC_SRST_REQ2	(1 << 13)
#define USB3_UTMI_CKEN          (1 << 12)
#define USB3_PIPE_CKEN          (1 << 11)
#define USB3_SUSPEND_CKEN       (1 << 10)
#define USB3_REF_CKEN           (1 << 9)
#define USB3_BUS_CKEN           (1 << 8)
#define USB3_PHY_SRST_REQ2	(1 << 3)
#define USB3_PHY_REF_CKEN	(1 << 0)

#define PERI_USB2               __io_address(0x12120000 + 0x88)
#define PERI_USB3               __io_address(0x12120000 + 0x8c)

#define GTXTHRCFG		0xC108
#define USB_TXPKTCNT_SEL	(1 << 29)
#define USB_TXPKTCNT		(3 << 24)
#define USB_MAX_TXBUTST_SIZE	(0x10 << 16)
#define GRXTHRCFG		0xC10C
#define USB_RXPKTCNT_SEL	(1 << 29)
#define USB_RXPKTCNT		(3 << 24)
#define USB_MAX_RXBUTST_SIZE	(0x10 << 16)

#define REG_GCTL		0xC110
#define PRTCAPDIR_HOST		(1 << 12)
#define PRTCAPDIR_MASK		((1 << 12) | (1 << 13))

#define REG_GUSB2PHYCFG0	0xC200
#define BIT_UTMI_ULPI		(1 << 4)
#define BIT_UTMI_8_16		(1 << 3)

#define REG_GUSB3PIPECTL0	0xC2C0
#define PCS_SSP_SOFT_RESET	(1 << 31)
#define USB3_PHY_SUSPEND_EN	(1 << 17)
#define USB3_SELECT_VOLT	(1 << 3)
#define USB3_DEEMPHASIS	(1 << 1)

void hiusb3_start_hcd(void __iomem *base)
{
	unsigned long flags;
	unsigned int reg;

	local_irq_save(flags);

	/* enable u2 1p phy reset  */
	writel(0x301, __io_address(0x12040194));
	mdelay(1);
	/* cancel u2 1p phy POR */
	writel(0x201, __io_address(0x12040194));
	mdelay(1);
	/* config u2 1p phy clock */
	writel(0x0c26, __io_address(0x12120088));
	udelay(100);
	writel(0x0, __io_address(0x12120088));
	mdelay(5);

	/* config u2 eye diagram */
	writel(0x1820, PERI_USB2);
	udelay(10);
	writel(0x0, PERI_USB2);
	udelay(100);
	/* squelchg=175mV，1port PHY */
	/*
	writel(0x6e21, PERI_USB2);
	udelay(10);
	writel(0x1, PERI_USB2);
	udelay(100);
	*/
	writel(0xb225, PERI_USB2);
	udelay(10);
	writel(0x5, PERI_USB2);
	udelay(100);
	writel(0x0327, PERI_USB2);
	udelay(10);
	writel(0x7, PERI_USB2);
	udelay(100);
	/* cancel u2 1p phy utmi reset */
	writel(0x1, __io_address(0x12040194));
	mdelay(1);

	/* open COMBPHY ref clk */
	reg = readl(PERI_CRG46);
	reg |= USB3_PHY_REF_CKEN;
	writel(reg, PERI_CRG46);
	mdelay(100);
	/* cancel u3 combphy POR */
	reg = readl(PERI_CRG46);
	reg |= USB3_PHY_SRST_REQ2;
	writel(reg, PERI_CRG46);
	mdelay(100);

	/* de-assert usb3_vcc_srst_req */
	reg = readl(PERI_CRG46);
	reg |= USB3_VCC_SRST_REQ2;
	reg |= USB3_UTMI_CKEN;
	reg |= USB3_PIPE_CKEN;
	reg |= USB3_SUSPEND_CKEN;
	reg |= USB3_REF_CKEN;
	reg |= USB3_BUS_CKEN;
	reg |= USB3_PHY_SRST_REQ2;
	reg |= USB3_PHY_REF_CKEN;
	writel(reg, PERI_CRG46);
	mdelay(100);

	reg = readl(PERI_CRG46);
	reg &= ~(USB3_VCC_SRST_REQ2);
	writel(reg, PERI_CRG46);
	mdelay(100);

	reg = readl(base + REG_GUSB3PIPECTL0);
	reg |= PCS_SSP_SOFT_RESET;
	writel(reg, base + REG_GUSB3PIPECTL0);

	/*step 3: USB2 PHY chose ulpi 8bit interface */
	reg = readl(base + REG_GUSB2PHYCFG0);
	reg &= ~BIT_UTMI_ULPI;
	reg &= ~BIT_UTMI_8_16;
	writel(reg, base + REG_GUSB2PHYCFG0);
	wmb();
	mdelay(20);

	/* set host mode. [13:12] 01: Host; 10: Device; 11: OTG */
	reg = readl(base + REG_GCTL);
	reg &= ~PRTCAPDIR_MASK;
	reg |= PRTCAPDIR_HOST;
	writel(reg, base + REG_GCTL);

	/* de-assert usb3phy hard-macro por */
	reg = readl(PERI_CRG46);
	reg &= ~USB3_PHY_SRST_REQ2;
	writel(reg, PERI_CRG46);
	mdelay(100);

	/* config u3 eye diagram */
	writel(0xd10, PERI_USB3);
	writel(0xd30, PERI_USB3);
	writel(0xd10, PERI_USB3);/* USB3_TX_TERMINATION_TRIM = 1101 */
	writel(0x0, PERI_USB3);
	mdelay(10);
	writel(0xd1b, PERI_USB3);
	writel(0xd3b, PERI_USB3);
	writel(0xd1b, PERI_USB3); /* TX_SWING_COMP = 1101 */
	writel(0x0, PERI_USB3);
	mdelay(10);

	writel(0x802, PERI_USB3);
	writel(0x822, PERI_USB3);
	writel(0x802, PERI_USB3); /* SSC enable */
	writel(0x0, PERI_USB3);
	mdelay(10);
	reg = readl(base + REG_GUSB3PIPECTL0);
	reg &= ~PCS_SSP_SOFT_RESET;
	reg &= ~USB3_PHY_SUSPEND_EN;	/* disable suspend */
	reg &= ~(0x7 << 3);
	reg |= USB3_SELECT_VOLT;	/* select volt is 1100mv */
	reg &= ~(0x3 << 1);
	reg |= USB3_DEEMPHASIS;		/* -3.5db de-emphasis */
	writel(reg, base + REG_GUSB3PIPECTL0);
	mdelay(100);

	reg = USB_TXPKTCNT_SEL | USB_TXPKTCNT | USB_MAX_TXBUTST_SIZE;
	writel(reg, base + GTXTHRCFG);

	reg = USB_RXPKTCNT_SEL | USB_RXPKTCNT | USB_MAX_RXBUTST_SIZE;
	writel(reg, base + GRXTHRCFG);
	mdelay(20);

	local_irq_restore(flags);
}
EXPORT_SYMBOL(hiusb3_start_hcd);

void hiusb3_stop_hcd(void)
{
	unsigned long flags;
	unsigned int reg;

	local_irq_save(flags);

	reg = readl(PERI_CRG46);
	writel(reg | (USB3_VCC_SRST_REQ2), PERI_CRG46);
	mdelay(500);
	local_irq_restore(flags);
}
EXPORT_SYMBOL(hiusb3_stop_hcd);
