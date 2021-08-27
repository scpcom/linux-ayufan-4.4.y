/*
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
#include <linux/pci.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/init.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/mach/pci.h>

#include "ctrlEnv/mvCtrlEnvLib.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"
#include "mvSysPci.h"
#include "pci/mvPci.h"

#undef DEBUG
#ifdef DEBUG
#	define DB(x) x
#else
#	define DB(x)
#endif

static int __init mv_map_irq(struct pci_dev *dev, u8 slot, u8 pin);

extern u32 mv_pci_mem_size_get(int ifNum);
extern u32 mv_pci_io_base_get(int ifNum);
extern u32 mv_pci_io_size_get(int ifNum);
extern u32 mv_pci_mem_base_get(int ifNum);

void __init mv_pci_preinit(void)
{
	MV_ADDR_WIN win;

	if (mvCtrlPciMaxIfGet() > 1)
		panic("Single PCI is supported ONLY!");

	mvPciInit(0, MV_PCI_MOD_HOST);

	/* I/O remmap */
	win.baseLow = 0x0;
	win.baseHigh = 0x0;
	mvCpuIfPciRemap(PCI_IF0_IO, &win);
}

/* Currentlly the PCI config read/write are implemented as read modify write
   to 32 bit.
   TBD: adjust it to realy use 1/2/4 byte(partial) read/write, after the pex
	read config WA will be removed.
*/
static int mv_pci0_read_config(struct pci_bus *bus, unsigned int devfn,
			       int where, int size, u32 *val)
{

	MV_U32 bus_num, func, regOff, dev_no, temp;
	MV_U32 localBus;

	*val = 0xffffffff;

	bus_num = bus->number;
	dev_no = PCI_SLOT(devfn);

	/* don't return for our device */
	localBus = mvPciLocalBusNumGet(0);
	if ((dev_no == 0) && (bus_num == localBus)) {
		DB(printk
		   (KERN_INFO "PCI 0 read from our own dev return 0xffffffff\n"));
		return 0xffffffff;
	}

	func = PCI_FUNC(devfn);
	regOff = (MV_U32) where & PCAR_REG_NUM_MASK;

	if ((func == 0) && (dev_no < 2))
		DB(printk
		   ("PCI 0 read: bus = %x dev = %x func = %x regOff = %x ",
		    bus_num, dev_no, func, regOff));

	temp = (u32) mvPciConfigRead(0, bus_num, dev_no, func, regOff);

	switch (size) {
	case 1:
		temp = (temp >> (8 * (where & 0x3))) & 0xff;
		break;

	case 2:
		temp = (temp >> (8 * (where & 0x2))) & 0xffff;
		break;

	default:
		break;
	}

	*val = temp;

	if ((func == 0) && (dev_no < 2))
		DB(printk(KERN_INFO " got %x\n", temp));

	return 0;
}

static int mv_pci0_write_config(struct pci_bus *bus, unsigned int devfn,
				int where, int size, u32 val)
{
	MV_U32 bus_num, func, regOff, dev_no, temp, mask, shift;

	bus_num = bus->number;
	dev_no = PCI_SLOT(devfn);
	func = PCI_FUNC(devfn);
	regOff = (MV_U32) where & PCAR_REG_NUM_MASK;

	DB(printk
	   (KERN_INFO "PCI 0: writing data %x size %x to bus %x dev %x func %x offs %x\n",
	    val, size, bus_num, dev_no, func, regOff));
	if (size != 4)
		temp = (u32) mvPciConfigRead(0, bus_num, dev_no, func, regOff);
	else
		temp = val;

	switch (size) {
	case 1:
		shift = (8 * (where & 0x3));
		mask = 0xff;
		break;

	case 2:
		shift = (8 * (where & 0x2));
		mask = 0xffff;
		break;

	default:
		shift = 0;
		mask = 0xffffffff;
		break;
	}

	temp = (temp & (~(mask << shift))) | ((val & mask) << shift);
	mvPciConfigWrite(0, bus_num, dev_no, func, regOff, temp);

	return 0;
}

static struct pci_ops mv_pci_ops = {
	.read = mv_pci0_read_config,
	.write = mv_pci0_write_config,
};

int __init mv_pci_setup(int nr, struct pci_sys_data *sys)
{
	struct resource *res;

	if (nr)
		panic("Single PCI is supported ONLY!");

	sys->map_irq = mv_map_irq;

	res = kmalloc(sizeof(struct resource) * 2, GFP_KERNEL);
	if (!res)
		panic("PCI: unable to alloc resources");

	memset(res, 0, sizeof(struct resource) * 2);

	res[0].start = mv_pci_io_base_get(0) - IO_SPACE_REMAP;
	res[0].end =
	    mv_pci_io_base_get(0) - IO_SPACE_REMAP + mv_pci_io_size_get(0) - 1;
	res[0].name = "PCI0 IO Primary";
	res[0].flags = IORESOURCE_IO;

	res[1].start = mv_pci_mem_base_get(0);
	res[1].end = mv_pci_mem_base_get(0) + mv_pci_mem_size_get(0) - 1;
	res[1].name = "PCI0 Memory Primary";
	res[1].flags = IORESOURCE_MEM;

	if (request_resource(&ioport_resource, &res[0]))
		printk(KERN_NOTICE "IO Request resource failed - Pci If %x\n", nr);

	if (request_resource(&iomem_resource, &res[1]))
		printk(KERN_NOTICE "Memory Request resource failed - Pci If %x\n", nr);

	sys->resource[0] = &res[0];
	sys->resource[1] = &res[1];
	sys->resource[2] = NULL;
	sys->io_offset = 0x0;

	return 1;

}

struct pci_bus *mv_pci_scan_bus(int nr, struct pci_sys_data *sys)
{
	struct pci_ops *ops;
	struct pci_bus *bus;

	if (nr)
		panic("Single PCI is supported ONLY!");

	ops = &mv_pci_ops;
	bus = pci_scan_bus(sys->busnr, ops, sys);
	return bus;
}

static int __init mv_map_irq(struct pci_dev *dev, u8 slot, u8 pin)
{
	return IRQ_AURORA_PCI0;
}

static struct hw_pci mv_pci __initdata = {
	.swizzle = pci_std_swizzle,
	.map_irq = mv_map_irq,
	.setup = mv_pci_setup,
	.scan = mv_pci_scan_bus,
	.preinit = mv_pci_preinit,
};

static int __init mv_pci_init(void)
{
	MV_U32 ifnum = mvCtrlPciMaxIfGet();
	if (ifnum) {
		mv_pci.nr_controllers = ifnum;
		pci_common_init(&mv_pci);
	}

	return 0;
}

subsys_initcall(mv_pci_init);
