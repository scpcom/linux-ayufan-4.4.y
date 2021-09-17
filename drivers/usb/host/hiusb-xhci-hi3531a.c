#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/types.h>
#include <mach/hardware.h>

#include "xhci.h"
#include "hiusb.h"

MODULE_LICENSE("Dual MIT/GPL");

#define IO_REG_USB3_CTRL	__io_address(CRG_REG_BASE + REG_USB3_CTRL)
#define USB3_VCC_SRST_REQ2	(1 << 13)
#define USB3_UTMI_CKEN		(1 << 12)
#define USB3_PIPE_CKEN		(1 << 11)
#define USB3_SUSPEND_CKEN	(1 << 10)
#define USB3_REF_CKEN		(1 << 9)
#define USB3_BUS_CKEN		(1 << 8)

#define PERI_CRG72	__io_address(CRG_REG_BASE + 0x120)
#define COMBPHY1_REFCLK1_SEL	(0x3 << 14)
#define COMBPHY1_LANE1_REQ	(1 << 11)
#define COMBPHY1_LANE0_REQ	(1 << 10)

#define USB3_CTRL	__io_address(MISC_REG_BASE + 0x128)
#define HOST_U3_DISABLE        (1 << 3)

#define IO_REG_COMB_PHY1	__io_address(MISC_REG_BASE + REG_COMB_PHY1)
/* write(0x1 << 6) 0x6 to addr 0x4 */
#define CONFIG_CLK		((0x1 << 6) | (0x6 << 0) | (0x4 << 8))

#define IO_REG_USB3_CTRL1	__io_address(MISC_REG_BASE + REG_USB3_CTRL1)

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

	reg = readl(__io_address(0x1205008c));
	if ((reg & (0x1 << 15)) != 0) {
		reg = readl(PERI_CRG72);
		reg |= COMBPHY1_LANE1_REQ;
		writel(reg, PERI_CRG72);
		mdelay(10);

		reg = readl(PERI_CRG72);
		reg &= ~COMBPHY1_LANE1_REQ;
		writel(reg, PERI_CRG72);
		mdelay(10);

	} else {
		reg = readl(USB3_CTRL);
		reg |= HOST_U3_DISABLE;
		writel(reg, USB3_CTRL);
		mdelay(1);

		pr_info("COMBPHY IS NOT USB\n");
	}

	writel(0x1, __io_address(0x1204016c));
	mdelay(2);
	/* de-assert usb3_vcc_srst_req */
	reg = readl(IO_REG_USB3_CTRL);
	reg |= USB3_VCC_SRST_REQ2;
	reg |= USB3_UTMI_CKEN;
	reg |= USB3_PIPE_CKEN;
	reg |= USB3_SUSPEND_CKEN;
	reg |= USB3_REF_CKEN;
	reg |= USB3_BUS_CKEN;
	writel(reg, IO_REG_USB3_CTRL);
	mdelay(100);

	reg = readl(IO_REG_USB3_CTRL);
	reg &= ~(USB3_VCC_SRST_REQ2);
	writel(reg, IO_REG_USB3_CTRL);
	mdelay(10);

	reg = readl(base + REG_GUSB3PIPECTL0);
	reg |= PCS_SSP_SOFT_RESET;
	writel(reg, base + REG_GUSB3PIPECTL0);

	/*step 3: USB2 PHY chose ulpi 8bit interface */
	reg = readl(base + REG_GUSB2PHYCFG0);
	reg &= ~BIT_UTMI_ULPI;
	reg &= ~BIT_UTMI_8_16;
	reg &= ~(0x1 << 6);
	writel(reg, base + REG_GUSB2PHYCFG0);
	wmb();
	mdelay(20);

	/* set host mode. [13:12] 01: Host; 10: Device; 11: OTG */
	reg = readl(base + REG_GCTL);
	reg &= ~PRTCAPDIR_MASK;
	reg |= PRTCAPDIR_HOST;
	writel(reg, base + REG_GCTL);

	/* config u3 u2 eye diagram */
	/* close HS pre-emphasis */
	writel(0x0, IO_REG_USB3_CTRL1);
	udelay(10);
	writel(0x1820, IO_REG_USB3_CTRL1);
	udelay(100);
	/* Icomp = 212.5 */
	writel(0xa, IO_REG_USB3_CTRL1);
	udelay(10);
	writel(0xbb2a, IO_REG_USB3_CTRL1);
	udelay(100);

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

	reg = readl(__io_address(0x1205008c));
	if ((reg & (0x1 << 15)) == 0) {
		pr_info("COMBPHY IS NOT USB\n");
		local_irq_restore(flags);
		return;
	}

	reg = readl(IO_REG_USB3_CTRL);
	writel(reg | (USB3_VCC_SRST_REQ2), IO_REG_USB3_CTRL);
	mdelay(500);
	local_irq_restore(flags);
}
EXPORT_SYMBOL(hiusb3_stop_hcd);
