/*
 * xHCI host controller driver
 *
 * Copyright (C) 2008 Intel Corp.
 *
 * Author: Sarah Sharp
 * Some code borrowed from the Linux EHCI driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <asm/unaligned.h>

#include "xhci.h"

static void xhci_hub_descriptor(struct xhci_hcd *xhci,
		struct usb_hub_descriptor *desc)
{
	int ports;
	u16 temp;

	ports = HCS_MAX_PORTS(xhci->hcs_params1);

	/* USB 3.0 hubs have a different descriptor, but we fake this for now */
	desc->bDescriptorType = 0x29;
	desc->bPwrOn2PwrGood = 10;	/* xhci section 5.4.9 says 20ms max */
	desc->bHubContrCurrent = 0;

	desc->bNbrPorts = ports;
	temp = 1 + (ports / 8);
	desc->bDescLength = 7 + 2 * temp;

	/* Why does core/hcd.h define bitmap?  It's just confusing. */
#ifdef MY_ABC_HERE
	memset(&desc->bitmap[0], 0, temp);
	memset(&desc->bitmap[temp], 0xff, temp);
#else
	memset(&desc->DeviceRemovable[0], 0, temp);
	memset(&desc->DeviceRemovable[temp], 0xff, temp);
#endif

	/* Ugh, these should be #defines, FIXME */
	/* Using table 11-13 in USB 2.0 spec. */
	temp = 0;
	/* Bits 1:0 - support port power switching, or power always on */
	if (HCC_PPC(xhci->hcc_params))
		temp |= 0x0001;
	else
		temp |= 0x0002;
	/* Bit  2 - root hubs are not part of a compound device */
	/* Bits 4:3 - individual port over current protection */
	temp |= 0x0008;
	/* Bits 6:5 - no TTs in root ports */
	/* Bit  7 - no port indicators */
	desc->wHubCharacteristics = cpu_to_le16(temp);
}

static unsigned int xhci_port_speed(unsigned int port_status)
{
	if (DEV_LOWSPEED(port_status))
		return 1 << USB_PORT_FEAT_LOWSPEED;
	if (DEV_HIGHSPEED(port_status))
		return 1 << USB_PORT_FEAT_HIGHSPEED;
	if (DEV_SUPERSPEED(port_status))
		return 1 << USB_PORT_FEAT_SUPERSPEED;
	/*
	 * FIXME: Yes, we should check for full speed, but the core uses that as
	 * a default in portspeed() in usb/core/hub.c (which is the only place
	 * USB_PORT_FEAT_*SPEED is used).
	 */
	return 0;
}

/*
 * These bits are Read Only (RO) and should be saved and written to the
 * registers: 0, 3, 10:13, 30
 * connect status, over-current status, port speed, and device removable.
 * connect status and port speed are also sticky - meaning they're in
 * the AUX well and they aren't changed by a hot, warm, or cold reset.
 */
#define	XHCI_PORT_RO	((1<<0) | (1<<3) | (0xf<<10) | (1<<30))
/*
 * These bits are RW; writing a 0 clears the bit, writing a 1 sets the bit:
 * bits 5:8, 9, 14:15, 25:27
 * link state, port power, port indicator state, "wake on" enable state
 */
#define XHCI_PORT_RWS	((0xf<<5) | (1<<9) | (0x3<<14) | (0x7<<25))
/*
 * These bits are RW; writing a 1 sets the bit, writing a 0 has no effect:
 * bit 4 (port reset)
 */
#define	XHCI_PORT_RW1S	((1<<4))
/*
 * These bits are RW; writing a 1 clears the bit, writing a 0 has no effect:
 * bits 1, 17, 18, 19, 20, 21, 22, 23
 * port enable/disable, and
 * change bits: connect, PED, warm port reset changed (reserved zero for USB 2.0 ports),
 * over-current, reset, link state, and L1 change
 */
#define XHCI_PORT_RW1CS	((1<<1) | (0x7f<<17))
/*
 * Bit 16 is RW, and writing a '1' to it causes the link state control to be
 * latched in
 */
#define	XHCI_PORT_RW	((1<<16))
/*
 * These bits are Reserved Zero (RsvdZ) and zero should be written to them:
 * bits 2, 24, 28:31
 */
#define	XHCI_PORT_RZ	((1<<2) | (1<<24) | (0xf<<28))

/*
 * Given a port state, this function returns a value that would result in the
 * port being in the same state, if the value was written to the port status
 * control register.
 * Save Read Only (RO) bits and save read/write bits where
 * writing a 0 clears the bit and writing a 1 sets the bit (RWS).
 * For all other types (RW1S, RW1CS, RW, and RZ), writing a '0' has no effect.
 */
static u32 xhci_port_state_to_neutral(u32 state)
{
	/* Save read-only status and port state */
	return (state & XHCI_PORT_RO) | (state & XHCI_PORT_RWS);
}

#ifdef MY_ABC_HERE
#include <linux/pci.h>

extern int gSynoFactoryUSB3Disable;
extern unsigned short xhci_vendor;

// NEC
//#define IS_USB2(y) (y == 2 || y == 3) // index from 0 // HCS_MAX_PORTS(xhci->hcs_params1) = 4
//#define IS_USB3(y) (y == 0 || y == 1) // index from 0 // HCS_MAX_PORTS(xhci->hcs_params1) = 4
// Etron
//#define IS_USB3(y) (y == 2 || y == 3) // index from 0 // HCS_MAX_PORTS(xhci->hcs_params1) = 4
//#define IS_USB2(y) (y == 0 || y == 1) // index from 0 // HCS_MAX_PORTS(xhci->hcs_params1) = 4
static bool inline IS_USB3(unsigned int x)
{
	if ((xhci_vendor == PCI_VENDOR_ID_NEC) &&
		(x == 0 || x == 1)) {
		return true;
	} else if ((xhci_vendor == PCI_VENDOR_ID_ETRON) &&
		(x == 2 || x == 3)) {
		return true;
	} else {
		return false;
	}
}

static bool inline IS_USB2(unsigned int x)
{
	return !IS_USB3(x);
}

#endif

static void xhci_disable_port(struct xhci_hcd *xhci, u16 wIndex,
		__le32 __iomem *addr, u32 port_status)
{
#ifdef MY_ABC_HERE
	/* Don't allow the USB core to disable SuperSpeed ports. */
	if (IS_USB3(wIndex)) {
		xhci_dbg(xhci, "Ignoring request to disable "
				"SuperSpeed port.\n");
		return;
	}
#endif

	/* Write 1 to disable the port */
	xhci_writel(xhci, port_status | PORT_PE, addr);
	port_status = xhci_readl(xhci, addr);
	xhci_dbg(xhci, "disable port, actual port %d status  = 0x%x\n",
			wIndex, port_status);
}

#ifdef MY_ABC_HERE
static void xhci_clear_port_change_bit(struct xhci_hcd *xhci, u16 wValue,
		u16 wIndex, __le32 __iomem *addr, __le32 __iomem *addr_map, u32 port_status)
#else
static void xhci_clear_port_change_bit(struct xhci_hcd *xhci, u16 wValue,
		u16 wIndex, __le32 __iomem *addr, u32 port_status)
#endif
{
	char *port_change_bit;
	u32 status;
#ifdef MY_ABC_HERE
	u32 port_status_map;
	int link_state_map;
#endif

#ifdef MY_ABC_HERE
	int link_state;
#endif

	switch (wValue) {
	case USB_PORT_FEAT_C_RESET:
		status = PORT_RC;
		port_change_bit = "reset";
		break;
	case USB_PORT_FEAT_C_CONNECTION:
		status = PORT_CSC;
		port_change_bit = "connect";
		break;
	case USB_PORT_FEAT_C_OVER_CURRENT:
		status = PORT_OCC;
		port_change_bit = "over-current";
		break;
	case USB_PORT_FEAT_C_ENABLE:
		status = PORT_PEC;
		port_change_bit = "enable/disable";
		break;
#ifdef MY_ABC_HERE
	case USB_PORT_FEAT_C_BH_PORT_RESET:
		status = PORT_WRC;
		port_change_bit = "warm(BH) reset";
		break;
	case USB_PORT_FEAT_C_PORT_LINK_STATE:
		status = PORT_PLC;
		port_change_bit = "link state";
		break;
#endif

	default:
		/* Should never happen */
		return;
	}
	/* Change bits are all write 1 to clear */
	xhci_writel(xhci, port_status | status, addr);
	port_status = xhci_readl(xhci, addr);

#ifdef MY_ABC_HERE
	port_status_map = xhci_readl(xhci, addr_map);
	link_state_map = (port_status_map >> 5) & 0xf;
#endif

#ifdef MY_ABC_HERE
	link_state = (port_status >> 5) & 0xf;

	if (status == PORT_CSC) {
		int ori_status = port_status;
		if (port_status & (PORT_PLC | PORT_CEC)) {
			port_status = xhci_port_state_to_neutral(port_status);
			xhci_dbg(xhci, "set PLC/CEC.\n");
			xhci_writel(xhci, port_status | (ori_status & (PORT_PLC | PORT_CEC)), addr);
			port_status = xhci_readl(xhci, addr);
		}
		if (link_state == 0x6 || link_state == 0x4) {
			xhci_dbg(xhci, "set PLS 1.\n");
			xhci_writel(xhci, (0x5 << 5) | PORT_LINK_STROBE, addr);
			port_status = xhci_readl(xhci, addr); /* unblock any posted writes */
		}
		if (link_state_map == 0x6 || link_state_map == 0x4) { // check the mapping port too
			xhci_dbg(xhci, "set PLS 2.\n");
			xhci_writel(xhci, (0x5 << 5) | PORT_LINK_STROBE, addr_map);
			port_status_map = xhci_readl(xhci, addr_map);
		}

		xhci_dbg(xhci, "link:%d. link_map:%d.\n", link_state, link_state_map);
	}

#ifdef MY_ABC_HERE
	if (1 == gSynoFactoryUSB3Disable) {
		goto skip_check_power;
	}
#endif

#ifdef MY_ABC_HERE

	// check port_power again, because "set PLS" may affect it.
	port_status_map = xhci_readl(xhci, addr_map);
	link_state_map = (port_status_map >> 5) & 0xf;
	port_status = xhci_readl(xhci, addr);
	link_state = (port_status >> 5) & 0xf;

	if (link_state == 0x4) {
		xhci_dbg(xhci, "set PP 1 again.\n");
		xhci_writel(xhci, port_status | PORT_POWER, addr);
		port_status = xhci_readl(xhci, addr); /* unblock any posted writes */
	}
	if (link_state_map == 0x4) {
		xhci_dbg(xhci, "set PP 2 again.\n");
		xhci_writel(xhci, port_status_map | PORT_POWER, addr_map);
		port_status_map = xhci_readl(xhci, addr_map); /* unblock any posted writes */
	}
#endif

#ifdef MY_ABC_HERE
skip_check_power:
#endif

	xhci_dbg(xhci, "clear port %s change, actual port %d status  = 0x%x. status_map = 0x%x\n",
			port_change_bit, wIndex, port_status, port_status_map);

#endif

	xhci_dbg(xhci, "clear port %s change, actual port %d status  = 0x%x\n",
			port_change_bit, wIndex, port_status);
}

#ifdef MY_ABC_HERE
extern enum XHCI_SPECIAL_RESET_MODE xhci_special_reset; // from hub.c
#endif

int xhci_hub_control(struct usb_hcd *hcd, u16 typeReq, u16 wValue,
		u16 wIndex, char *buf, u16 wLength)
{
	struct xhci_hcd	*xhci = hcd_to_xhci(hcd);
	int ports;
	unsigned long flags;
	u32 temp, status;
#ifdef MY_ABC_HERE
	u32 temp_map;
#endif
	int retval = 0;
	__le32 __iomem *addr;
#ifdef MY_ABC_HERE
	__le32 __iomem *addr_map;
#endif

#ifdef MY_ABC_HERE
	xhci_dbg(xhci, "xhci_hub_control.type:0x%x.wvalue:%d.\n", typeReq, wValue);
#endif

	ports = HCS_MAX_PORTS(xhci->hcs_params1);

	spin_lock_irqsave(&xhci->lock, flags);
	switch (typeReq) {
	case GetHubStatus:
		/* No power source, over-current reported per port */
		memset(buf, 0, 4);
		break;
	case GetHubDescriptor:
		xhci_hub_descriptor(xhci, (struct usb_hub_descriptor *) buf);
		break;
	case GetPortStatus:
		if (!wIndex || wIndex > ports)
			goto error;
		wIndex--;
		status = 0;
		addr = &xhci->op_regs->port_status_base + NUM_PORT_REGS*(wIndex & 0xff);
		temp = xhci_readl(xhci, addr);
		xhci_dbg(xhci, "get port status, actual port %d status = 0x%x\n", wIndex, temp);

#ifdef MY_ABC_HERE
		addr_map = &xhci->op_regs->port_status_base + NUM_PORT_REGS*((wIndex+1)%ports & 0xff);
		temp_map = xhci_readl(xhci, addr_map);
		xhci_dbg(xhci, "get port status, actual port %d status = 0x%x\n", (wIndex+1)%ports, temp_map);
		addr_map = &xhci->op_regs->port_status_base + NUM_PORT_REGS*((wIndex+2)%ports & 0xff);
		temp_map = xhci_readl(xhci, addr_map);
		xhci_dbg(xhci, "get port status, actual port %d status = 0x%x\n", (wIndex+2)%ports, temp_map);
		addr_map = &xhci->op_regs->port_status_base + NUM_PORT_REGS*((wIndex+3)%ports & 0xff);
		temp_map = xhci_readl(xhci, addr_map);
		xhci_dbg(xhci, "get port status, actual port %d status = 0x%x\n", (wIndex+3)%ports, temp_map);
#endif

#ifdef MY_ABC_HERE
		/* wPortChange bits */
		if (temp & PORT_CSC)
			status |= USB_PORT_STAT_C_CONNECTION << 16;
		if (temp & PORT_PEC)
			status |= USB_PORT_STAT_C_ENABLE << 16;
		if ((temp & PORT_OCC))
			status |= USB_PORT_STAT_C_OVERCURRENT << 16;
		if ((temp & PORT_RC))
			status |= USB_PORT_STAT_C_RESET << 16;
		/* USB3.0 only */
		if (IS_USB3(wIndex)) {
			if ((temp & PORT_PLC))
				status |= USB_PORT_STAT_C_LINK_STATE << 16;
			if ((temp & PORT_WRC))
				status |= USB_PORT_STAT_C_BH_RESET << 16;
		}

		/*
		 * FIXME ignoring suspend, reset, and USB 2.1/3.0 specific
		 * changes
		 */
		if (temp & PORT_CONNECT) {
			status |= USB_PORT_STAT_CONNECTION;
			status |= xhci_port_speed(temp);
		}
		if (temp & PORT_PE)
			status |= USB_PORT_STAT_ENABLE;
		if (temp & PORT_OC)
			status |= USB_PORT_STAT_OVERCURRENT;
		if (temp & PORT_RESET)
			status |= USB_PORT_STAT_RESET;
		if (temp & PORT_POWER){
			if (IS_USB3(wIndex))
				status |= USB_SS_PORT_STAT_POWER;
			else
				status |= USB_PORT_STAT_POWER;
		}
		/* Port Link State */
		if (IS_USB3(wIndex)) {
			/* resume state is a xHCI internal state.
			 * Do not report it to usb core.
			 */
			if ((temp & PORT_PLS_MASK) != XDEV_RESUME)
				status |= (temp & PORT_PLS_MASK);
		}
#endif

#ifdef MY_ABC_HERE
		if (((temp & USB_PORT_STAT_LINK_STATE) == USB_SS_PORT_LS_COMP_MOD ||
				(temp & USB_PORT_STAT_LINK_STATE) == USB_SS_PORT_LS_LOOPBACK) &&
				(temp & PORT_POWER))
			status |= USB_PORT_STAT_TEST_MODE;
#endif

		xhci_dbg(xhci, "Get port status returned 0x%x\n", status);
		put_unaligned(cpu_to_le32(status), (__le32 *) buf);
		break;
	case SetPortFeature:
		wIndex &= 0xff;
		if (!wIndex || wIndex > ports)
			goto error;
		wIndex--;
		addr = &xhci->op_regs->port_status_base + NUM_PORT_REGS*(wIndex & 0xff);
		temp = xhci_readl(xhci, addr);
		temp = xhci_port_state_to_neutral(temp);
#ifdef MY_ABC_HERE
		addr_map = &xhci->op_regs->port_status_base + NUM_PORT_REGS*((wIndex+ports/2)%ports & 0xff);
		temp_map = xhci_readl(xhci, addr_map);
		temp_map = xhci_port_state_to_neutral(temp_map);
#endif
		switch (wValue) {
		case USB_PORT_FEAT_POWER:
			/*
			 * Turn on ports, even if there isn't per-port switching.
			 * HC will report connect events even before this is set.
			 * However, khubd will ignore the roothub events until
			 * the roothub is registered.
			 */
#ifdef MY_ABC_HERE
			xhci_dbg(xhci, "set port power. usb%d.\n", wIndex);
			if (1 == gSynoFactoryUSB3Disable && IS_USB3(wIndex))
				xhci_writel(xhci, temp & ~PORT_POWER, addr);
			else {
				// set power on usb3 port before usb2 port
				if((0 == gSynoFactoryUSB3Disable) && IS_USB2(wIndex) && !(temp_map & PORT_POWER)) {
					xhci_writel(xhci, temp_map | PORT_POWER, addr_map);
					temp_map = xhci_readl(xhci, addr_map);
					mdelay(100);
				}
				xhci_writel(xhci, temp | PORT_POWER, addr);
			}
#else
			xhci_writel(xhci, temp | PORT_POWER, addr);
#endif

			temp = xhci_readl(xhci, addr);
			xhci_dbg(xhci, "set port power, actual port %d status  = 0x%x\n", wIndex, temp);
			break;
		case USB_PORT_FEAT_RESET:
			temp = (temp | PORT_RESET);

#ifdef MY_ABC_HERE
			// reset if connected, or mapping port is in test mode
			if ((temp & PORT_CONNECT) || (temp_map & PORT_CONNECT) || // reset when connected // check this to prevent syno_nh trying to reset during disconnected
				(IS_USB2(wIndex) && // no connection but
				((temp_map & USB_PORT_STAT_LINK_STATE) == USB_SS_PORT_LS_COMP_MOD || //compliance mode
				(temp_map & USB_PORT_STAT_LINK_STATE) == USB_SS_PORT_LS_LOOPBACK))) // test mode
				xhci_writel(xhci, temp, addr);
			else
				xhci_dbg(xhci, "skip set port reset.\n");
#else
			xhci_writel(xhci, temp, addr);
#endif

			temp = xhci_readl(xhci, addr);
			xhci_dbg(xhci, "set port reset, actual port %d status  = 0x%x\n", wIndex, temp);

#ifdef MY_ABC_HERE
			xhci_dbg(xhci, "set port reset map, actual port %d status  = 0x%x\n", (wIndex+ports/2)%ports, temp_map);

			temp_map = xhci_readl(xhci, addr_map);
			// reset if mapping port is in test mode
			if (IS_USB2(wIndex) &&
				((temp_map & USB_PORT_STAT_LINK_STATE) == USB_SS_PORT_LS_COMP_MOD ||
				(temp_map & USB_PORT_STAT_LINK_STATE) == USB_SS_PORT_LS_LOOPBACK)) {
				xhci_err(xhci, "set port reset for test mode.\n");
				xhci_writel(xhci, temp_map | PORT_RESET, addr_map);
				temp_map = xhci_readl(xhci, addr_map);
			}

			// if host controller reported usb2 connected, try to reset to check if it can morph into SuperSpeed
			// the sequence and delay time are critical, test before modifying.
			else if((XHCI_SPECIAL_RESET_RUN == xhci_special_reset) && IS_USB2(wIndex) &&
							(temp & PORT_CONNECT) &&
							!(temp_map & PORT_CONNECT)) { // if temp_map connected already, no need to do special reset (ex. external hub)
				xhci_dbg(xhci, "set port special reset.\n");
				xhci_writel(xhci, temp_map | PORT_RESET, addr_map);
				mdelay(200); // 100 is ok for 6281, but not for cedarview. 200 is ok for both

				xhci_writel(xhci, temp | PORT_WR, addr);
				xhci_writel(xhci, temp_map | PORT_WR, addr_map);
				mdelay(200);
			}

			xhci_dbg(xhci, "set port reset, actual port %d status  = 0x%x\n", wIndex, temp);
			xhci_dbg(xhci, "set port reset map, actual port %d status  = 0x%x\n", (wIndex+ports/2)%ports, temp_map);
#endif
			break;
#ifdef MY_ABC_HERE
		case USB_PORT_FEAT_BH_PORT_RESET:
#ifdef MY_ABC_HERE
			xhci_dbg(xhci, "set USB_PORT_FEAT_BH_PORT_RESET.\n");
#endif
			temp |= PORT_WR;
			xhci_writel(xhci, temp, addr);

			temp = xhci_readl(xhci, addr);
			break;
#endif
		default:
			goto error;
		}
		temp = xhci_readl(xhci, addr); /* unblock any posted writes */
		break;
	case ClearPortFeature:
		if (!wIndex || wIndex > ports)
			goto error;
		wIndex--;
		addr = &xhci->op_regs->port_status_base +
			NUM_PORT_REGS*(wIndex & 0xff);
		temp = xhci_readl(xhci, addr);
		temp = xhci_port_state_to_neutral(temp);
#ifdef MY_ABC_HERE
		addr_map = &xhci->op_regs->port_status_base + NUM_PORT_REGS*((wIndex+ports/2)%ports & 0xff);
		temp_map = xhci_readl(xhci, addr_map);
		temp_map = xhci_port_state_to_neutral(temp_map);
#endif
		switch (wValue) {
		case USB_PORT_FEAT_C_RESET:
		case USB_PORT_FEAT_C_CONNECTION:
		case USB_PORT_FEAT_C_OVER_CURRENT:
		case USB_PORT_FEAT_C_ENABLE:
#ifdef MY_ABC_HERE
		case USB_PORT_FEAT_C_BH_PORT_RESET:
		case USB_PORT_FEAT_C_PORT_LINK_STATE:
#endif
#ifdef MY_ABC_HERE
			xhci_clear_port_change_bit(xhci, wValue, wIndex,
					addr, addr_map, temp);
#else
			xhci_clear_port_change_bit(xhci, wValue, wIndex,
					addr, temp);
#endif
			break;

#ifdef MY_ABC_HERE
		case USB_PORT_FEAT_POWER:
			xhci_dbg(xhci, "clear USB_PORT_FEAT_POWER.\n");
			xhci_writel(xhci, temp & ~PORT_POWER, addr);
			temp = xhci_readl(xhci, addr);
			break;
#endif

		case USB_PORT_FEAT_ENABLE:
			xhci_disable_port(xhci, wIndex, addr, temp);
			break;
		default:
			goto error;
		}
		break;
	default:
error:
		/* "stall" on error */
		retval = -EPIPE;
	}
	spin_unlock_irqrestore(&xhci->lock, flags);
	return retval;
}

/*
 * Returns 0 if the status hasn't changed, or the number of bytes in buf.
 * Ports are 0-indexed from the HCD point of view,
 * and 1-indexed from the USB core pointer of view.
 * xHCI instances can have up to 127 ports, so FIXME if you see more than 15.
 *
 * Note that the status change bits will be cleared as soon as a port status
 * change event is generated, so we use the saved status from that event.
 */
int xhci_hub_status_data(struct usb_hcd *hcd, char *buf)
{
	unsigned long flags;
	u32 temp, status;
	int i, retval;
	struct xhci_hcd	*xhci = hcd_to_xhci(hcd);
	int ports;
	__le32 __iomem *addr;

	ports = HCS_MAX_PORTS(xhci->hcs_params1);

	/* Initial status is no changes */
	buf[0] = 0;
	status = 0;
	if (ports > 7) {
		buf[1] = 0;
		retval = 2;
	} else {
		retval = 1;
	}

	spin_lock_irqsave(&xhci->lock, flags);
	/* For each port, did anything change?  If so, set that bit in buf. */
	for (i = 0; i < ports; i++) {
		addr = &xhci->op_regs->port_status_base +
			NUM_PORT_REGS*i;
		temp = xhci_readl(xhci, addr);
		if (temp & (PORT_CSC | PORT_PEC | PORT_OCC)) {
			if (i < 7)
				buf[0] |= 1 << (i + 1);
			else
				buf[1] |= 1 << (i - 7);
			status = 1;
		}
	}
	spin_unlock_irqrestore(&xhci->lock, flags);
	return status ? retval : 0;
}
