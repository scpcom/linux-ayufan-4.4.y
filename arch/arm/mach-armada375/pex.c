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
#include <linux/module.h>
#include <linux/platform_device.h>

#include <mach/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/mach/pci.h>
#include <mach/irqs.h>

#include "ctrlEnv/mvCtrlEnvLib.h"
#include "boardEnv/mvBoardEnvLib.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"
#include "ctrlEnv/mvUnitMap.h"
#include "pex/mvPexRegs.h"
#include "mvSysPexApi.h"

#ifdef MV_DEBUG
#define DB(x) x
#else
#define DB(x)
#endif

#define MV_PEX_MASK_ABCD              (BIT24 | BIT25 | BIT26 | BIT27)

static int __init mv_map_irq_0(const struct pci_dev *dev, u8 slot, u8 pin);
static int __init mv_map_irq_1(const struct pci_dev *dev, u8 slot, u8 pin);

extern u32 mv_pci_mem_size_get(int ifNum);
extern u32 mv_pci_io_base_get(int ifNum);
extern u32 mv_pci_io_size_get(int ifNum);
extern u32 mv_pci_mem_base_get(int ifNum);
extern int mv_is_pci_io_mapped(int ifNum);
extern MV_TARGET mv_pci_io_target_get(int ifNum);

static struct platform_device mv_pex = {
	.name = "mv_pex",
	.id = 0,
	.num_resources = 0,
};

static void *mv_get_irqmap_func[] __initdata = {
	mv_map_irq_0,
	mv_map_irq_1,
};

void __init mv_pex_preinit(void)
{
	static MV_U32 pex0flg;
	unsigned int pci_if;
	MV_ADDR_WIN pciIoRemap;
	MV_BOARD_PEX_INFO *boardPexInfo = mvBoardPexInfoGet();

	for (pci_if = 0; pci_if < boardPexInfo->boardPexIfNum; pci_if++) {
		if (mvUnitMapIsPexMine(pci_if) == MV_FALSE)
			continue;

		pr_info("PCIe: Checking physical bus #%d: ", pci_if);
		if (mvCtrlPwrClckGet(PEX_UNIT_ID, pci_if) == MV_FALSE) {
			pr_info("Disabled\n");
			continue;
		}

		if ((MV_REG_READ(PEX_DBG_STATUS_REG(pci_if)) & 0x7f) != 0x7E) {
			pr_info("no link, disabled\n");
			mvCtrlPwrClckSet(PEX_UNIT_ID, pci_if, MV_FALSE);
			continue;
		}

		if (mvSysPexInit(pci_if, MV_PEX_ROOT_COMPLEX, pci_if) != MV_OK) {
			pr_warn("%s: Error: mvSysPexInit(%d) failed\n",
				__func__, pci_if);
			mvCtrlPwrClckSet(PEX_UNIT_ID, pci_if, MV_FALSE);
			continue;
		}

		pr_info("PCIe %d enabled - link up\n", pci_if);

		/* Assign bus number 0 to first active/available bus */
		if (pex0flg == 0) {
			mvPexLocalBusNumSet(pci_if, 0x0);
			pex0flg = 1;
		}

		MV_REG_BIT_SET(PEX_MASK_REG(pci_if), MV_PEX_MASK_ABCD);

		if (mv_is_pci_io_mapped(pci_if)) {
			pciIoRemap.baseLow =
			    mv_pci_io_base_get(pci_if) - IO_SPACE_REMAP;
			pciIoRemap.baseHigh = 0;
			pciIoRemap.size = mv_pci_io_size_get(pci_if);
			mvCpuIfPexRemap(mv_pci_io_target_get(pci_if),
					&pciIoRemap);
		}
	}
}

void mv_pex_reinit(void)
{
	MV_BOARD_PEX_INFO *boardPexInfo = mvBoardPexInfoGet();
	static MV_U32 pex0flg;
	unsigned int pci_if;

	for (pci_if = 0; pci_if < boardPexInfo->boardPexIfNum; pci_if++) {
		if (mvUnitMapIsPexMine(pci_if) == MV_FALSE)
			continue;

		if (mvCtrlPwrClckGet(PEX_UNIT_ID, pci_if) == MV_FALSE)
			continue;

		if ((MV_REG_READ(PEX_DBG_STATUS_REG(pci_if)) & 0x7f) != 0x7E) {
			pr_info("no link, disabled\n");
			mvCtrlPwrClckSet(PEX_UNIT_ID, pci_if, MV_FALSE);
			continue;
		}

		if (mvSysPexInit(pci_if, MV_PEX_ROOT_COMPLEX, pci_if) != MV_OK) {
			pr_warn("%s: Error: mvSysPexInit(%d) failed\n",
				__func__, pci_if);
			mvCtrlPwrClckSet(PEX_UNIT_ID, pci_if, MV_FALSE);
			continue;
		}

		/* Assign bus number 0 to first active/available bus */
		if (pex0flg == 0) {
			mvPexLocalBusNumSet(pci_if, 0x0);
			pex0flg = 1;
		}

		MV_REG_BIT_SET(PEX_MASK_REG(pci_if), MV_PEX_MASK_ABCD);
	}
}

static int mv_pci_read_config(struct pci_bus *bus, unsigned int devfn,
			      int where, int size, u32 *val)
{
	u32 bus_num, func, regOff, dev_no, temp, localBus;
	struct pci_sys_data *sysdata = (struct pci_sys_data *)bus->sysdata;
	u32 pciIf = sysdata->mv_controller_num;

	*val = 0xffffffff;

	if (MV_FALSE == mvCtrlPwrClckGet(PEX_UNIT_ID, pciIf))
		return 0;
	bus_num = bus->number;
	dev_no = PCI_SLOT(devfn);

	/* don't return for our device */
	localBus = mvPexLocalBusNumGet(pciIf);
	if (dev_no == 0 && bus_num == localBus) {
		DB(pr_info
		   ("PCI %d read from our own dev return 0xffffffff\n", pciIf));
		return 0xffffffff;
	}

	func = PCI_FUNC(devfn);

	/* total of 12 bits: 8 legacy + 4 extended */
	regOff =
	    (MV_U32) where & (PXCAR_REG_NUM_MASK | PXCAR_REAL_EXT_REG_NUM_MASK);

	temp = (u32) mvPexConfigRead(pciIf, bus_num, dev_no, func, regOff);
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

	DB(pr_info
	   ("PCI %2d read : (b.d.f) = (%2d,%2d,%2d); reg = %4d: val = 0x%08x\n",
	    pciIf, bus_num, dev_no, func, regOff, temp));

	return 0;
}

static int mv_pci_write_config(struct pci_bus *bus, unsigned int devfn,
			       int where, int size, u32 val)
{
	u32 bus_num, func, regOff, dev_no, temp, mask, shift;
	struct pci_sys_data *sysdata = (struct pci_sys_data *)bus->sysdata;
	u32 pciIf = sysdata->mv_controller_num;

	if (MV_FALSE == mvCtrlPwrClckGet(PEX_UNIT_ID, pciIf))
		return 0xFFFFFFFF;

	bus_num = bus->number;
	dev_no = PCI_SLOT(devfn);
	func = PCI_FUNC(devfn);

	/* total of 12 bits: 8 legacy + 4 extended */
	regOff =
	    (MV_U32) where & (PXCAR_REG_NUM_MASK | PXCAR_REAL_EXT_REG_NUM_MASK);

	DB(pr_info
	   ("PCI %2d write: (b.d.f) = (%2d,%2d,%2d); reg = %4d: val = 0x%08x\n",
	    pciIf, bus_num, dev_no, func, regOff, val));
	if (size != 4)
		temp =
		    (u32) mvPexConfigRead(pciIf, bus_num, dev_no, func, regOff);
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
	}

	temp = (temp & (~(mask << shift))) | ((val & mask) << shift);
	mvPexConfigWrite(pciIf, bus_num, dev_no, func, regOff, temp);
	return 0;
}

static struct pci_ops mv_pci_ops = {
	.read = mv_pci_read_config,
	.write = mv_pci_write_config,
};

int __init mv_pex_setup(int nr, struct pci_sys_data *sys)
{
	struct resource *res;
	u32 mem_base, mem_size, iobase, index = 0;

	if (mvUnitMapIsPexMine(nr) == MV_FALSE)
		return 0;

	if (mvCtrlPwrClckGet(PEX_UNIT_ID, nr) == MV_FALSE)
		return 0;

	res = kcalloc(1, sizeof(struct resource) * 2, GFP_KERNEL);
	if (!res) {
		panic("%s: memory alloc failed\n", __func__);
		return 0;
	}

	memset(res, 0, sizeof(struct resource) * 2);

	/* Save the HW iface number for this PEX bus */
	sys->mv_controller_num = nr;
	sys->map_irq = mv_get_irqmap_func[nr];

	if (mv_is_pci_io_mapped(nr)) {
		iobase = mv_pci_io_base_get(nr);
		res[index].start = iobase - IO_SPACE_REMAP;
		res[index].end = iobase - IO_SPACE_REMAP + mv_pci_io_size_get(nr) - 1;
		res[index].name = "PCIx IO Primary";
		res[index].flags = IORESOURCE_IO;
		if (request_resource(&ioport_resource, &res[index]))
			printk(KERN_ERR "IO Request resource failed - "
				"Pci If %x\n", nr);
		else
			index++;
	}

	mem_base = mv_pci_mem_base_get(nr);
	mem_size = mv_pci_mem_size_get(nr);

	res[index].start = mem_base;
	res[index].end   = mem_base + mem_size - 1;
	res[index].name  = "PCIx Memory Primary";
	res[index].flags = IORESOURCE_MEM;

	if (request_resource(&iomem_resource, &res[index]))
		printk(KERN_ERR "Memory Request resource failed - Pci If %x\n", nr);

	sys->resource[0] = &res[0];
	if (index > 0) {
		sys->resource[1] = &res[1];
		sys->resource[2] = NULL;
	} else
		sys->resource[1] = NULL;

	sys->io_offset   = 0x0;

	return 1;
}

struct pci_bus *mv_pex_scan_bus(int nr, struct pci_sys_data *sys)
{
	struct pci_bus *bus;
	bus = pci_scan_bus(sys->busnr, &mv_pci_ops, sys);
	return bus;
}

static int __init mv_map_irq_0(const struct pci_dev *dev, u8 slot, u8 pin)
{
	return IRQ_GLOBAL_PCIE0;
}

static int __init mv_map_irq_1(const struct pci_dev *dev, u8 slot, u8 pin)
{
	return IRQ_GLOBAL_PCIE1;
}

static struct hw_pci mv_pci __initdata = {
	.swizzle = pci_std_swizzle,
	.setup = mv_pex_setup,
	.scan = mv_pex_scan_bus,
	.preinit = mv_pex_preinit,
};

static int mv_pex_probe(struct platform_device *dev)
{
	return 0;
}

static struct platform_driver mv_pex_driver = {
	.probe = mv_pex_probe,
	.driver = {
		   .name = "mv_pex",
		   },
};

static int __init mv_pex_init_module(void)
{
	mv_pci.nr_controllers = mvBoardPexInfoGet()->boardPexIfNum;
	mv_pci.swizzle = pci_std_swizzle;
	mv_pci.map_irq = mv_map_irq_0;
	mv_pci.setup = mv_pex_setup;
	mv_pci.scan = mv_pex_scan_bus;
	mv_pci.preinit = mv_pex_preinit;
	pci_common_init(&mv_pci);
	platform_device_register(&mv_pex);

	return platform_driver_register(&mv_pex_driver);
}

module_init(mv_pex_init_module);
MODULE_DESCRIPTION("Marvell PCIe driver");
MODULE_LICENSE("GPL");
