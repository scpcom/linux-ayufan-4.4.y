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
//#include <linux/autoconf.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/init.h>

#include <mach/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/mach/pci.h>
#include <mach/irqs.h>

#include "mvCommon.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"
#include "pex/mvPex.h"
#include "pex/mvPexRegs.h"
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "mvSysPexApi.h"
#include "ctrlEnv/sys/mvCpuIf.h"

#undef DEBUG
#ifdef DEBUG
#	define DB(x) x
#else
#	define DB(x)
#endif

#define PCI_ERR_NAME_LEN 12
#define MV_MAX_PEX_IF_NUMBER 2

static int __init mv_map_irq_0(const struct pci_dev *dev, u8 slot, u8 pin);
static int __init mv_map_irq_1(const struct pci_dev *dev, u8 slot, u8 pin);

void mv_pci_error_init(u32 pciIf);

static irqreturn_t pex_error_interrupt(int irq, void *dev_id);

static struct pex_if_error {
	MV_8 irq_name[PCI_ERR_NAME_LEN];
	MV_U32 ifNumber;
} pex_err[MV_MAX_PEX_IF_NUMBER];

void __init mv_pci_preinit(void)
{
	MV_ADDR_WIN win;
	MV_STATUS retval;
	u32 pciIf;
	u32 maxif = mvCtrlPexMaxIfGet();

	/* Check Power State */
	if (MV_FALSE == mvCtrlPwrClckGet(PEX_UNIT_ID, 0))
		return;

	for (pciIf = 0; pciIf < maxif; pciIf++)	{

		retval = mvSysPexInit(pciIf, MV_PEX_ROOT_COMPLEX);

		if (pciIf == 0) {
			mvPexLocalBusNumSet(pciIf, 0);
			mvPexLocalDevNumSet(pciIf, 1);
		}

		/* Clear the secondary bus number */
		MV_REG_WRITE(PEX_SECONDARY_BUS_REG(pciIf), 0);

		if (retval == MV_NO_SUCH) {
			//printk("pci_init:no such calling mvPexInit for PEX-%x\n",pciIf);
			continue;
		}

		if (retval != MV_OK) {
			printk("pci_init:Error calling mvPexInit for PEX-%x\n",pciIf);
			continue;
		}

		/* unmask inter A/B/C/D */
		//printk("writing %x tp %x \n",MV_PCI_MASK_ABCD, MV_PCI_MASK_REG(pciIf) );
		MV_REG_WRITE(MV_PCI_MASK_REG(pciIf), MV_PCI_MASK_ABCD );

		/* init PCI express error handling */
		mv_pci_error_init(pciIf);

		/* remmap IO !! */
		win.baseLow = (pciIf? PEX1_IO_PHYS_BASE : PEX0_IO_PHYS_BASE) - IO_SPACE_REMAP;
		win.baseHigh = 0x0;
		win.size = pciIf? PEX1_IO_SIZE : PEX0_IO_SIZE;
		mvCpuIfPexRemap((pciIf? PEX1_IO : PEX0_IO), &win);
	}
}

/**
* mv_pci_error_init
* DESCRIPTION: init PCI express error handling
* INPUTS:      pciIf - number of pex device
* OUTPUTS:     N/A
* RETURNS:     N/A
**/
void mv_pci_error_init(u32 pciIf)
{
	MV_U32      reg_val;

	/* init pex_err structure per each pex */
	pex_err[pciIf].ifNumber = pciIf;
	snprintf(pex_err[pciIf].irq_name, PCI_ERR_NAME_LEN, "error_pex%d", pciIf);

	/* register interrupt for PCI express error */
	if (request_irq((IRQ_AURORA_ERR_START + INT_ERR_PCIE(pciIf)), pex_error_interrupt, IRQF_DISABLED,
				(const char*)pex_err[pciIf].irq_name, &pex_err[pciIf].ifNumber) < 0) {
		panic("Could not allocate IRQ for PCI express error!");
	}

	/* get current value of Interrupt Mask Register */
	reg_val = MV_REG_READ(MV_PCI_MASK_REG(pciIf));

	/* set relevant mask to Interrupt Mask Register */
	MV_REG_WRITE(MV_PCI_MASK_REG(pciIf), (reg_val | MV_PCI_MASK_ERR));
}

/* Currentlly the PCI config read/write are implemented as read modify write
   to 32 bit.
   TBD: adjust it to realy use 1/2/4 byte(partial) read/write, after the pex
	read config WA will be removed.
*/

static int  pci_read_cfg(u32 pciIf, u32 bus_num, u32 dev_no,
		u32 func, u32 where)
{
	u32 cfgCmd;
	u32 regOff = (MV_U32)where & (PXCAR_REG_NUM_MASK
			| PXCAR_REAL_EXT_REG_NUM_MASK);

	/* Creating PEX address to be passed */
	cfgCmd  = (bus_num << PXCAR_BUS_NUM_OFFS);
	cfgCmd |= (dev_no << PXCAR_DEVICE_NUM_OFFS);
	cfgCmd |= (func << PXCAR_FUNC_NUM_OFFS);
	/* lgacy register space */
	cfgCmd |= (regOff & PXCAR_REG_NUM_MASK);
	/* extended register space */
	cfgCmd |= (((regOff & PXCAR_REAL_EXT_REG_NUM_MASK) >>
		     PXCAR_REAL_EXT_REG_NUM_OFFS) << PXCAR_EXT_REG_NUM_OFFS);
	cfgCmd |= PXCAR_CONFIG_EN;

	/* Write the address to the PEX configuration address register */
	MV_REG_WRITE(PEX_CFG_ADDR_REG(pciIf), cfgCmd);

	/*
	 * In order to let the PEX controller absorbed the address of the
	 * read transaction we perform a validity check that the address
	 * was written
	 * */
	if (cfgCmd != MV_REG_READ(PEX_CFG_ADDR_REG(pciIf)))
		return 0xFFFFFFFF;

	/* cleaning Master Abort */
	MV_REG_BIT_SET(PEX_CFG_DIRECT_ACCESS(pciIf, PEX_STATUS_AND_COMMAND),
			PXSAC_MABORT);

	/* Read the Data returned in the PEX Data register */
	return  MV_REG_READ(PEX_CFG_DATA_REG(pciIf));

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
	u32 bus_num, func, dev_no, temp, localBus;
	struct pci_sys_data *sysdata = (struct pci_sys_data *)bus->sysdata;
	u32 pciIf = sysdata->mv_controller_num;

	*val = 0xffffffff;

	if (MV_FALSE == mvCtrlPwrClckGet(PEX_UNIT_ID, pciIf))
		return 0;

	bus_num = bus->number;
	dev_no = PCI_SLOT(devfn);

	/* Our local bus is PEX so enable reading only device 0 */
	localBus = mvPexLocalBusNumGet(pciIf);
	if ((dev_no !=  0) && (bus_num == localBus)) {
		DB(pr_info("PCI %d device %d illegal on local bus\n", pciIf,
					dev_no));
		return 0xffffffff;
	}

	func = PCI_FUNC(devfn);
	temp = pci_read_cfg(pciIf, bus_num, dev_no, func, where);

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

	DB(pr_info("PCI %d read: bus = %x dev = %x func = %x regOff = %x"
		   "val = 0x%08x\n", pciIf, bus_num, dev_no, func,
		   where, temp));

	return 0;
}

static int mv_pci_write_config(struct pci_bus *bus, unsigned int devfn,
		int where, int size, u32 val)
{
	u32 bus_num, func, regOff, dev_no, temp, mask, shift;
	struct pci_sys_data *sysdata = (struct pci_sys_data *)bus->sysdata;
	u32 pciIf = sysdata->mv_controller_num;
	u32 cfgCmd;

	if (MV_FALSE == mvCtrlPwrClckGet(PEX_UNIT_ID, pciIf))
		return 0xFFFFFFFF;

	bus_num = bus->number;
	dev_no = PCI_SLOT(devfn);
	func = PCI_FUNC(devfn);
	/* total of 12 bits: 8 legacy + 4 extended */
	regOff = (MV_U32)where & (PXCAR_REG_NUM_MASK |
			PXCAR_REAL_EXT_REG_NUM_MASK);

	if (size != 4)
		temp = pci_read_cfg(pciIf, bus_num, dev_no, func, where);
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

	/* Creating PEX address to be passed */
	cfgCmd  = (bus_num << PXCAR_BUS_NUM_OFFS);
	cfgCmd |= (dev_no << PXCAR_DEVICE_NUM_OFFS);
	cfgCmd |= (func << PXCAR_FUNC_NUM_OFFS);
	/* lgacy register space */
	cfgCmd |= (regOff & PXCAR_REG_NUM_MASK);
	/* extended register space */
	cfgCmd |= (((regOff & PXCAR_REAL_EXT_REG_NUM_MASK) >>
		     PXCAR_REAL_EXT_REG_NUM_OFFS) << PXCAR_EXT_REG_NUM_OFFS);
	cfgCmd |= PXCAR_CONFIG_EN;

	DB(pr_info("PCI %d: write data 0x%08x size %x to bus %x dev %x func %x"
		   "offs %x\n", pciIf, temp, size, bus_num, dev_no,
		   func, regOff));

	/* Write the address to the PEX configuration address register */
	MV_REG_WRITE(PEX_CFG_ADDR_REG(pciIf), cfgCmd);

	/*
	 * In order to let the PEX controller absorbed the address of the read
	 * transaction we perform a validity check that the address was written
	 */
	if (cfgCmd != MV_REG_READ(PEX_CFG_ADDR_REG(pciIf))) {
		pr_info("Error: mv_pci_write_config failed to write\n");
		return 1;
	}

	/* Write the Data passed to the PEX Data register */
	MV_REG_WRITE(PEX_CFG_DATA_REG(pciIf), temp);

	return 0;
}

static struct pci_ops mv_pci_ops = {
        .read   = mv_pci_read_config,
        .write  = mv_pci_write_config,
};

int __init mv_pci_setup(int nr, struct pci_sys_data *sys)
{
        struct resource *res;

        switch (nr) {
        case 0:
                sys->map_irq = mv_map_irq_0;
                break;
        case 1:
                sys->map_irq = mv_map_irq_1;
                break;
        default:
		printk("mv_pci_setup: nr(%d) out of scope\n",nr);
                return 0;
        }

	res = kmalloc(sizeof(struct resource) * 2, GFP_KERNEL);
        if (!res)
                panic("PCI: unable to alloc resources");

        memset(res, 0, sizeof(struct resource) * 2);

	if(!nr) {
		res[0].start = PEX0_IO_PHYS_BASE - IO_SPACE_REMAP;
		res[0].end   = PEX0_IO_PHYS_BASE - IO_SPACE_REMAP + PEX0_IO_SIZE - 1;
		res[0].name  = "PEX0 IO";
		res[0].flags = IORESOURCE_IO;

		res[1].start = PEX0_MEM_PHYS_BASE;
		res[1].end   = PEX0_MEM_PHYS_BASE + PEX0_MEM_SIZE - 1;
		res[1].name  = "PEX0 Memory";
		res[1].flags = IORESOURCE_MEM;
	} else {
		res[0].start = PEX1_IO_PHYS_BASE - IO_SPACE_REMAP;
		res[0].end   = PEX1_IO_PHYS_BASE - IO_SPACE_REMAP + PEX1_IO_SIZE - 1;
		res[0].name  = "PEX1 IO";
		res[0].flags = IORESOURCE_IO;

		res[1].start = PEX1_MEM_PHYS_BASE;
		res[1].end   = PEX1_MEM_PHYS_BASE + PEX1_MEM_SIZE - 1;
		res[1].name  = "PEX1 Memory";
		res[1].flags = IORESOURCE_MEM;
	}

        if (request_resource(&ioport_resource, &res[0]))
	{
		printk ("IO Request resource failed - Pci If %x\n",nr);
	}
	if (request_resource(&iomem_resource, &res[1]))
	{
		printk ("Memory Request resource failed - Pci If %x\n",nr);
	}

        sys->resource[0] = &res[0];
        sys->resource[1] = &res[1];
        sys->resource[2] = NULL;
        sys->io_offset   = 0x0;
	sys->mv_controller_num = nr;

        return 1;

}

struct pci_bus *mv_pci_scan_bus(int nr, struct pci_sys_data *sys)
{
	struct pci_ops *ops = &mv_pci_ops;
	struct pci_bus *bus;

	bus = pci_scan_bus(sys->busnr, ops, sys);

	if (sys->mv_controller_num == 0) {
		mvPexLocalBusNumSet(1, bus->subordinate + 1);
		mvPexLocalDevNumSet(1, 1);
	}

	return bus;
}

/**
* pex_error_interrupt
* DESCRIPTION: PCI express error interrupt  routine
* INPUTS:      @irq: irq number
               @dev_id: device id - ignored
* OUTPUTS:     kernel error message
* RETURNS:     IRQ_HANDLED
**/
static irqreturn_t pex_error_interrupt(int irq, void *dev_id)
{
	MV_U32  reg_val;
	MV_U32 ifPexNumber=*(MV_U32 *)dev_id;

	/* get current value of Interrupt Cause Register */
	reg_val = MV_REG_READ(MV_PCI_IRQ_CAUSE_REG(ifPexNumber));
	printk(KERN_ERR "PCI express error: irq - %d, Pex number: %d, "
			"Interrupt Cause Register value: %x  \n", irq, ifPexNumber, reg_val);

	return IRQ_HANDLED;
}

static int __init mv_map_irq_0(const struct pci_dev *dev, u8 slot, u8 pin)
{
        return IRQ_AURORA_PCIE0;
}

static int __init mv_map_irq_1(const struct pci_dev *dev, u8 slot, u8 pin)
{
        return IRQ_AURORA_PCIE1;
}

static struct hw_pci mv_pci __initdata = {
	.swizzle        	= pci_std_swizzle,
        .map_irq                = mv_map_irq_0,
        .setup                  = mv_pci_setup,
        .scan                   = mv_pci_scan_bus,
        .preinit                = mv_pci_preinit,
};

static int __init mv_pci_init(void)
{
#if defined(MV_INCLUDE_CLK_PWR_CNTRL)
	/* Check pex power state */
	MV_U32 pexPower;
	pexPower = mvCtrlPwrClckGet(PEX_UNIT_ID,0);
	if (pexPower == MV_FALSE)
	{
		printk("\nWarning PCI-E is Powered Off\n");
		return 0;
	}
#endif

    mv_pci.nr_controllers = mvCtrlPexMaxIfGet();
    pci_common_init(&mv_pci);

    return 0;
}

subsys_initcall(mv_pci_init);
