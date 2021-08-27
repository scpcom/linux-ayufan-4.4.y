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
#	define DB(x) x
#else
#	define DB(x) 
#endif

#define MV_PEX_MASK_ABCD              (BIT24 | BIT25 | BIT26 | BIT27)

static int __init mv_map_irq_0(struct pci_dev *dev, u8 slot, u8 pin);
static int __init mv_map_irq_1(struct pci_dev *dev, u8 slot, u8 pin);
static int __init mv_map_irq_2(struct pci_dev *dev, u8 slot, u8 pin);
static int __init mv_map_irq_3(struct pci_dev *dev, u8 slot, u8 pin);
static int __init mv_map_irq_4(struct pci_dev *dev, u8 slot, u8 pin);
static int __init mv_map_irq_5(struct pci_dev *dev, u8 slot, u8 pin);
static int __init mv_map_irq_6(struct pci_dev *dev, u8 slot, u8 pin);
static int __init mv_map_irq_7(struct pci_dev *dev, u8 slot, u8 pin);
static int __init mv_map_irq_8(struct pci_dev *dev, u8 slot, u8 pin);
static int __init mv_map_irq_9(struct pci_dev *dev, u8 slot, u8 pin);

extern u32 mv_pci_mem_size_get(int ifNum);
extern u32 mv_pci_io_base_get(int ifNum);
extern u32 mv_pci_io_size_get(int ifNum);
extern u32 mv_pci_mem_base_get(int ifNum);
extern int mv_is_pci_io_mapped(int ifNum);
extern MV_TARGET mv_pci_io_target_get(int ifNum);

static struct platform_device mv_pex = {
	.name		= "mv_pex",
	.id		= 0,
	.num_resources	= 0,
};

static void* mv_get_irqmap_func[] __initdata =
{
	mv_map_irq_0,
	mv_map_irq_1,
	mv_map_irq_2,
	mv_map_irq_3,
	mv_map_irq_4,
	mv_map_irq_5,
	mv_map_irq_6,
	mv_map_irq_7,
	mv_map_irq_8,
	mv_map_irq_9
};

void __init mv_pex_preinit(void)
{
	static MV_U32 pex0flg = 0;
	unsigned int pciIf, temp;
	MV_ADDR_WIN pciIoRemap;
	MV_BOARD_PEX_INFO* boardPexInfo = mvBoardPexInfoGet();
	MV_U32 pexHWInf = 0;

	for (pciIf = 0; pciIf < boardPexInfo->boardPexIfNum; pciIf++) 
	{
		/* Translate logical interface number to physical */
		pexHWInf = boardPexInfo->pexMapping[pciIf];

		if (MV_FALSE == mvUnitMapIsPexMine(pexHWInf))
			continue;

		printk("PCI-E: Cheking physical bus #%d (controller #%d): ", pciIf, pexHWInf);
		if (MV_FALSE == mvCtrlPwrClckGet(PEX_UNIT_ID, pexHWInf))
		{
			printk("Disabled\n");
			continue;
		}

		/* init the PCI interface */
		temp = mvSysPexInit(pexHWInf, MV_PEX_ROOT_COMPLEX);

		if (MV_NO_SUCH == temp)
		{
			printk("Enabled - No Link\n");
			/* No Link - shutdown interface */
			mvCtrlPwrClckSet(PEX_UNIT_ID, pexHWInf, MV_FALSE);;
			continue;
		}
		else if ((MV_OK != temp) && (MV_NO_SUCH != temp)){
			printk("Init FAILED!!!\n");
			printk("PCI-E %d: Init Failed.\n", pexHWInf);
		}

		printk("Enabled - Link UP\n");
		/* Assign bus number 0 to first active/available bus */
		if (pex0flg == 0) {
	       		mvPexLocalBusNumSet(pexHWInf, 0x0);
	       		pex0flg = 1;
		}

		MV_REG_BIT_SET(PEX_MASK_REG(pexHWInf), MV_PEX_MASK_ABCD);
		if (mv_is_pci_io_mapped(pexHWInf))
		{
			pciIoRemap.baseLow = mv_pci_io_base_get(pexHWInf) - IO_SPACE_REMAP;
			pciIoRemap.baseHigh = 0; 		
			pciIoRemap.size = mv_pci_io_size_get(pexHWInf);
			mvCpuIfPexRemap(mv_pci_io_target_get(pexHWInf), &pciIoRemap);
		}
	}
}

void mv_pex_reinit(void)
{
	static MV_U32 pex0flg = 0;
	unsigned int pciIf, temp;
	MV_ADDR_WIN pciIoRemap;
	MV_BOARD_PEX_INFO* boardPexInfo = mvBoardPexInfoGet();
	MV_U32 pexHWInf = 0;

	for (pciIf = 0; pciIf < boardPexInfo->boardPexIfNum; pciIf++)
	{
		/* Translate logical interface number to physical */
		pexHWInf = boardPexInfo->pexMapping[pciIf];

		if (MV_FALSE == mvUnitMapIsPexMine(pexHWInf))
			continue;

		if (MV_FALSE == mvCtrlPwrClckGet(PEX_UNIT_ID, pexHWInf))
			continue;

		/* init the PCI interface */
		temp = mvSysPexInit(pexHWInf, MV_PEX_ROOT_COMPLEX);

		if (MV_NO_SUCH == temp)
		{
			/* No Link - shutdown interface */
			mvCtrlPwrClckSet(PEX_UNIT_ID, pexHWInf, MV_FALSE);;
			continue;
		}

		/* Assign bus number 0 to first active/available bus */
		if (pex0flg == 0) {
			mvPexLocalBusNumSet(pexHWInf, 0x0);
			pex0flg = 1;
		}

		MV_REG_BIT_SET(PEX_MASK_REG(pexHWInf), MV_PEX_MASK_ABCD);
		if (mv_is_pci_io_mapped(pexHWInf))
		{
			pciIoRemap.baseLow = mv_pci_io_base_get(pexHWInf) - IO_SPACE_REMAP;
			pciIoRemap.baseHigh = 0;
			pciIoRemap.size = mv_pci_io_size_get(pexHWInf);
			mvCpuIfPexRemap(mv_pci_io_target_get(pexHWInf), &pciIoRemap);
		}
	}
}


/* Currentlly the PCI config read/write are implemented as read modify write
   to 32 bit.
   TBD: adjust it to realy use 1/2/4 byte(partial) read/write, after the pex
	read config WA will be removed.
*/
static int mv_pci_read_config(struct pci_bus *bus,
				  unsigned int devfn, int where,
				  int size, u32 *val)
{
	u32 bus_num,func,regOff,dev_no,temp, localBus;		
	struct pci_sys_data *sysdata = (struct pci_sys_data *)bus->sysdata;
	u32 pciIf = sysdata->mv_controller_num;

	*val = 0xffffffff;

	if (MV_FALSE == mvCtrlPwrClckGet(PEX_UNIT_ID, pciIf))
		return 0;
	bus_num = bus->number;
	dev_no = PCI_SLOT(devfn);

	/* don't return for our device */
	localBus = mvPexLocalBusNumGet(pciIf);
	if ((dev_no == 0) && ( bus_num == localBus))
	{
		DB(printk("PCI %d read from our own dev return 0xffffffff \n", pciIf));
		return 0xffffffff;
	}

	func = PCI_FUNC(devfn);
	regOff = (MV_U32)where & (PXCAR_REG_NUM_MASK | PXCAR_REAL_EXT_REG_NUM_MASK); /* total of 12 bits: 8 legacy + 4 extended */

	DB(printk("PCI %d read: bus = %x dev = %x func = %x regOff = %x ",pciIf, bus_num,dev_no,func,regOff));

	temp = (u32) mvPexConfigRead(pciIf, bus_num, dev_no, func, regOff);
	switch (size) {
		case 1:
			temp = (temp >>  (8*(where & 0x3))) & 0xff;
			break;

		case 2:
			temp = (temp >>  (8*(where & 0x2))) & 0xffff;
			break;

		default:
			break;
	}

	*val = temp;

	DB(printk(" got %x \n",temp));

	return 0;
}

static int mv_pci_write_config(struct pci_bus *bus, unsigned int devfn, int where,
                           int size, u32 val)
{
	u32 bus_num,func,regOff,dev_no,temp, mask , shift;
	struct pci_sys_data *sysdata = (struct pci_sys_data *)bus->sysdata;	
	u32 pciIf = sysdata->mv_controller_num;		

	if (MV_FALSE == mvCtrlPwrClckGet(PEX_UNIT_ID, pciIf))
		return 0xFFFFFFFF;
	bus_num = bus->number;
	dev_no = PCI_SLOT(devfn);
	func = PCI_FUNC(devfn);
	regOff = (MV_U32)where & (PXCAR_REG_NUM_MASK | PXCAR_REAL_EXT_REG_NUM_MASK); /* total of 12 bits: 8 legacy + 4 extended */

	DB(printk("PCI %d: writing data %x size %x to bus %x dev %x func %x offs %x \n",
			  pciIf, val,size,bus_num,dev_no,func,regOff));
	if (size != 4)
	{
		temp = (u32) mvPexConfigRead(pciIf, bus_num, dev_no, func, regOff);
	}
	else
	{
		temp = val;
	}

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
	mvPexConfigWrite(pciIf, bus_num, dev_no, func, regOff, temp);
	return 0;
}

static struct pci_ops mv_pci_ops = {
	.read   = mv_pci_read_config,
	.write  = mv_pci_write_config,
};

int __init mv_pex_setup(int nr, struct pci_sys_data *sys)
{
	struct resource *res;
	u32 membase, iobase, index = 0;	
	MV_BOARD_PEX_INFO* boardPexInfo = mvBoardPexInfoGet();
	MV_U32 pexHWInf = 0;

	/* Translate logical interface number to physical */
	pexHWInf = boardPexInfo->pexMapping[nr];

	if (MV_FALSE == mvUnitMapIsPexMine(pexHWInf))
		return 0;

	/* Check if this interface is used or not */
	if (MV_FALSE == mvCtrlPwrClckGet(PEX_UNIT_ID, pexHWInf))
		return 0;

	/* Allocate resources memory */	
	res = kmalloc(sizeof(struct resource) * 2, GFP_KERNEL);
	if (!res)
	{
		panic("PCI: unable to alloc resources");
		return 0;
	}

	memset(res, 0, sizeof(struct resource) * 2);

	/* Save the H/W if number for this PEX bus */
	sys->mv_controller_num = pexHWInf;
	sys->map_irq = mv_get_irqmap_func[sys->mv_controller_num];
	
	membase = mv_pci_mem_base_get(sys->mv_controller_num);
	if (mv_is_pci_io_mapped(sys->mv_controller_num))
	{
	
		iobase = mv_pci_io_base_get(sys->mv_controller_num);
		res[index].start = iobase - IO_SPACE_REMAP;
		res[index].end   = iobase - IO_SPACE_REMAP + mv_pci_io_size_get(sys->mv_controller_num)-1;
		res[index].name  = "PCIx IO Primary";
		res[index].flags = IORESOURCE_IO;		
		if (request_resource(&ioport_resource, &res[index]))
		{	
			printk ("IO Request resource failed - Pci If %x\n",nr);
		}
		else
			index++;
	}
	res[index].start = membase;
	res[index].end   = membase + mv_pci_mem_size_get(sys->mv_controller_num)-1;
	res[index].name  = "PCIx Memory Primary";
	res[index].flags = IORESOURCE_MEM;

	if (request_resource(&iomem_resource, &res[index]))
	{	
		printk ("Memory Request resource failed - Pci If %x\n",nr);
	}
 
	sys->resource[0] = &res[0];
	if (index > 0) 
	{
		sys->resource[1] = &res[1];
		sys->resource[2] = NULL;
	}
	else
		sys->resource[1] = NULL;
	sys->io_offset   = 0x0;

	return 1;
}

struct pci_bus *mv_pex_scan_bus(int nr, struct pci_sys_data *sys)
{
	struct pci_ops *ops = &mv_pci_ops;	
	struct pci_bus *bus;
	MV_BOARD_PEX_INFO* boardPexInfo = mvBoardPexInfoGet();
	MV_U32 pexNextHWInf, ifnum;

	bus = pci_scan_bus(sys->busnr, ops, sys);

	/* Set the bus number in the following controller */
	for (ifnum = (nr+1); ifnum < boardPexInfo->boardPexIfNum; ifnum++) {

		pexNextHWInf = boardPexInfo->pexMapping[ifnum];

		if (MV_FALSE == mvUnitMapIsPexMine(pexNextHWInf))
			continue;

		if (MV_TRUE == mvCtrlPwrClckGet(PEX_UNIT_ID, pexNextHWInf)) {
			mvPexLocalBusNumSet(pexNextHWInf, (bus->subordinate + 1));
			break;
		}
	}

	return bus;
}


static int __init mv_map_irq_0(struct pci_dev *dev, u8 slot, u8 pin)
{	
	return IRQ_AURORA_PCIE0;
}

static int __init mv_map_irq_1(struct pci_dev *dev, u8 slot, u8 pin)
{
	return IRQ_AURORA_PCIE1;
}

static int __init mv_map_irq_2(struct pci_dev *dev, u8 slot, u8 pin)
{
	return IRQ_AURORA_PCIE2;
}

static int __init mv_map_irq_3(struct pci_dev *dev, u8 slot, u8 pin)
{
	return IRQ_AURORA_PCIE3;
}

static int __init mv_map_irq_4(struct pci_dev *dev, u8 slot, u8 pin)
{
	return IRQ_AURORA_PCIE4;
}

static int __init mv_map_irq_5(struct pci_dev *dev, u8 slot, u8 pin)
{
	return IRQ_AURORA_PCIE5;
}

static int __init mv_map_irq_6(struct pci_dev *dev, u8 slot, u8 pin)
{
	return IRQ_AURORA_PCIE6;
}

static int __init mv_map_irq_7(struct pci_dev *dev, u8 slot, u8 pin)
{
	return IRQ_AURORA_PCIE7;
}

static int __init mv_map_irq_8(struct pci_dev *dev, u8 slot, u8 pin)
{
	return IRQ_AURORA_PCIE8;
}

static int __init mv_map_irq_9(struct pci_dev *dev, u8 slot, u8 pin)
{
	return IRQ_AURORA_PCIE9;
}

static struct hw_pci mv_pci __initdata = {
	.swizzle        	= pci_std_swizzle,
        .setup                  = mv_pex_setup,
        .scan                   = mv_pex_scan_bus,
        .preinit                = mv_pex_preinit,
};

static int mv_pex_probe(struct platform_device *dev)
{
	return 0;
}

static int pex_status[MV_PEX_MAX_IF];
static int pex_ifnum;

static int mv_pex_suspend(struct platform_device *dev, pm_message_t state)
{
	unsigned int pciIf;
	MV_U32 pexHWInf = 0;
	MV_BOARD_PEX_INFO* boardPexInfo = mvBoardPexInfoGet();

	/* Save PCI Express status Register */
	for (pciIf = 0; pciIf < pex_ifnum; pciIf++) {
		pexHWInf = boardPexInfo->pexMapping[pciIf];

		if (MV_FALSE == mvUnitMapIsPexMine(pexHWInf))
			continue;
		if (MV_FALSE == mvCtrlPwrClckGet(PEX_UNIT_ID, pexHWInf))
			continue;

		pex_status[pexHWInf] = MV_REG_READ(PEX_STATUS_REG(pexHWInf));
	}

	return 0;
}

static int mv_pex_resume(struct platform_device *dev)
{
	unsigned int pciIf;
	MV_U32 pexHWInf = 0;
	MV_BOARD_PEX_INFO* boardPexInfo = mvBoardPexInfoGet();

	mv_pex_reinit();

	/* Restore PCI Express status Register */
	for (pciIf = 0; pciIf < pex_ifnum; pciIf++) {
		pexHWInf = boardPexInfo->pexMapping[pciIf];

		if (MV_FALSE == mvUnitMapIsPexMine(pexHWInf))
			continue;
		if (MV_FALSE == mvCtrlPwrClckGet(PEX_UNIT_ID, pexHWInf))
			continue;

		MV_REG_WRITE(PEX_STATUS_REG(pexHWInf), pex_status[pexHWInf]);
	}

	return 0;
}

static struct platform_driver mv_pex_driver = {
	.probe    = mv_pex_probe,
#ifdef CONFIG_PM
	.suspend = mv_pex_suspend,
	.resume  = mv_pex_resume,
#endif /* CONFIG_PM */
	.driver = {
		.name = "mv_pex",
	},
};

static int __init mv_pex_init_module(void)
{
	MV_BOARD_PEX_INFO* boardPexInfo = mvBoardPexInfoGet();

	/* WA - Disable PEX on RD-SERVER board */
	if (mvBoardIdGet() == RD_78460_SERVER_ID)
		return 0;

	mv_pci.nr_controllers = (mvBoardPexInfoGet())->boardPexIfNum;
	mv_pci.swizzle        = pci_std_swizzle;
	mv_pci.map_irq         = mv_map_irq_0;
	mv_pci.setup           = mv_pex_setup;
	mv_pci.scan            = mv_pex_scan_bus;
	mv_pci.preinit         = mv_pex_preinit;
	pci_common_init(&mv_pci);
	platform_device_register(&mv_pex);

	pex_ifnum = boardPexInfo->boardPexIfNum;

	return platform_driver_register(&mv_pex_driver);
}

module_init(mv_pex_init_module);
MODULE_DESCRIPTION("Marvell PEX Driver");
MODULE_LICENSE("GPL");
