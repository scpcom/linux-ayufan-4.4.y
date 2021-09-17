#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/pci.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/platform_device.h>
#include <pci/pci.h>
#include "mv_prestera_pci.h"
#include "mv_sysmap_pci.h"

#ifdef CONFIG_OF
#include "mach/mvCommon.h"
#else
#include "common/mvCommon.h"
#endif

#ifdef CONFIG_MV_INCLUDE_SERVICECPU
#include "mv_servicecpu/servicecpu.h"
#endif
#ifdef CONFIG_MV_INCLUDE_DRAGONITE_XCAT
#if defined(MY_ABC_HERE)
#include <linux/miscdevice.h>
#endif  
#include "mv_drivers_lsp/mv_dragonite/dragonite_xcat.h"
#endif

#undef MV_PP_PCI_DBG
#undef MV_PP_IDT_DBG

#ifdef MV_PP_PCI_DBG
#define dprintk(a...) printk(a)
#else
#define dprintk(a...)
#endif

#define SWITCH_TARGET_ID		0x3
#define DFX_TARGET_ID			0x8
#define RAM_TARGET_ID			0x0
#define SHARED_RAM_ATTR_ID		0x3E
#define DRAGONITE_TARGET_ID		0xa

#define DRAGONITE_CTRL_REG			0x1c
#define DRAGONITE_POE_CAUSE_IRQ_REG		0x64
#define DRAGONITE_POE_MASK_IRQ_REG		0x68
#define DRAGONITE_HOST2POE_IRQ_REG		0x6c
#define DRAGONITE_DEBUGGER_REG			0xF8290

#ifdef MV_PP_IDT_DBG
#define idtprintk(a...) printk(a)
#else
#define idtprintk(a...)
#endif

static const char prestera_drv_name[] = "mvPP_PCI";
static void __iomem *inter_regs;
static struct idtSwitchConfig idtSwCfg[MAX_NUM_OF_IDT_SWITCH];

static struct pci_decoding_window ac3_pci_sysmap[] = {

	{0,		PXWCR_WIN_BAR_MAP_BAR1,	0x0,		_64M,		0x0,
					SWITCH_TARGET_ID,		0x0,	ENABLE},

	{DFXW,		PXWCR_WIN_BAR_MAP_BAR2,	DFX_BASE,	DFX_SIZE,	0x0,
					DFX_TARGET_ID,			0x0,	ENABLE},

#if defined(MY_ABC_HERE)
	{SCPUW,		PXWCR_WIN_BAR_MAP_BAR2,	SCPU_BASE,	SCPU_SIZE,	0xFFE00000,
#else  
	{SCPUW,		PXWCR_WIN_BAR_MAP_BAR2,	SCPU_BASE,	SCPU_SIZE,	0xFFF80000,
#endif  
					RAM_TARGET_ID,	SHARED_RAM_ATTR_ID,	ENABLE},

	{DITCMW,	PXWCR_WIN_BAR_MAP_BAR2, ITCM_BASE,	ITCM_SIZE,	0x0,
					DRAGONITE_TARGET_ID,		0x0,	ENABLE},

	{DDTCMW,	PXWCR_WIN_BAR_MAP_BAR2,	DTCM_BASE,	DTCM_SIZE,	DRAGONITE_DTCM_OFFSET,
					DRAGONITE_TARGET_ID,		0x0,	ENABLE},

	{0xff,		PXWCR_WIN_BAR_MAP_BAR2,	0x0,		0,		0x0,
					0,				0x0,	TBL_TERM},
};

static struct pci_decoding_window bc2_pci_sysmap[] = {

	{0,		PXWCR_WIN_BAR_MAP_BAR1,	0x0,		_64M,		0x0,
					SWITCH_TARGET_ID,		0x0,	ENABLE},

	{DFXW,		PXWCR_WIN_BAR_MAP_BAR2,	DFX_BASE,	DFX_SIZE,	0x0,
					DFX_TARGET_ID,			0x0,	ENABLE},

#if defined(MY_ABC_HERE)
	{SCPUW,		PXWCR_WIN_BAR_MAP_BAR2,	SCPU_BASE,	SCPU_SIZE,	0xFFE00000,
#else  
	{SCPUW,		PXWCR_WIN_BAR_MAP_BAR2,	SCPU_BASE,	SCPU_SIZE,	0xFFF80000,
#endif  
					RAM_TARGET_ID,	SHARED_RAM_ATTR_ID,	ENABLE},

	{0xff,		PXWCR_WIN_BAR_MAP_BAR2,	0x0,		0,		0x0,
					0,				0x0,	TBL_TERM},
};

static struct pci_decoding_window *get_pci_sysmap(struct pci_dev *pdev)
{
	 
	switch (pdev->device & ~MV_DEV_FLAVOUR_MASK) {

	case MV_BOBCAT2_DEV_ID:
		return bc2_pci_sysmap;
	case MV_ALLEYCAT3_DEV_ID:
		return ac3_pci_sysmap;
	default:
		return NULL;
	}
}

static void mv_set_pex_bars(uint8_t pex_nr, uint8_t bar_nr, int enable)
{
	int val;

	dprintk("%s: pex_nr %d, bar_nr %d, enable:%d\n", __func__, pex_nr,
		bar_nr, enable);

	val = readl(inter_regs + PEX_BAR_CTRL_REG(pex_nr, bar_nr));

	if (enable == ENABLE)
		writel(val | PXBCR_BAR_EN,
			inter_regs + PEX_BAR_CTRL_REG(pex_nr, bar_nr));
	else
		writel(val & ~(PXBCR_BAR_EN),
			inter_regs + PEX_BAR_CTRL_REG(pex_nr, bar_nr));
}

static void mv_resize_bar(uint8_t pex_nr, uint8_t bar_nr, uint32_t bar_size)
{
	 
	mv_set_pex_bars(pex_nr, bar_nr, DISABLE);

	writel(bar_size, inter_regs + PEX_BAR_CTRL_REG(pex_nr, bar_nr));
	dprintk("PEX_BAR_CTRL_REG(%d, %d) = 0x%x\n", pex_nr, bar_nr,
		readl(inter_regs + PEX_BAR_CTRL_REG(pex_nr, bar_nr)));

	mv_set_pex_bars(pex_nr, bar_nr, ENABLE);
}

static int mv_read_and_assign_bars(struct pci_dev *pdev, int resno)
{
	struct resource *res = pdev->resource + resno;
	int reg, err;

	dprintk("before reassign: r_start 0x%x, r_end: 0x%x, r_flags 0x%lx\n",
		res->start, res->end, res->flags);

	reg = PCI_BASE_ADDRESS_0 + (resno << 2);
	__pci_read_base(pdev, pci_bar_unknown, res, reg);
	err = pci_assign_resource(pdev, resno);

	dprintk("after reassign: r_start 0x%x, r_end: 0x%x, r_flags 0x%lx\n",
		res->start, res->end, res->flags);

	return err;
}

static int mv_configure_win_bar(struct pci_decoding_window *win_map, struct pci_dev *pdev)
{
	uint8_t target, bar_nr;
	int io_base_bar[2], base_addr, val, win_ctrl_reg, win_base_reg, win_remap_reg;

	io_base_bar[0] = pci_resource_start(pdev, MV_PCI_BAR_1);
	io_base_bar[1] = pci_resource_start(pdev, MV_PCI_BAR_2);

	for (target = 0; win_map[target].enable != TBL_TERM; target++) {
		if (win_map[target].enable != ENABLE)
			continue;

		val = (SIZE_TO_BAR_REG(win_map[target].size) |
		      (win_map[target].target_id << PXWCR_TARGET_OFFS) |
		      (win_map[target].attr << PXWCR_ATTRIB_OFFS) |
		      win_map[target].win_bar_map | PXWCR_WIN_EN);

		dprintk("targ size 0x%x, size reg 0x%x, val 0x%x\n", win_map[target].size,
			SIZE_TO_BAR_REG(win_map[target].size), val);

		bar_nr = win_map[target].win_bar_map >> PXWCR_WIN_BAR_MAP_OFFS;
		base_addr = io_base_bar[bar_nr] + win_map[target].base_offset;

		switch (win_map[target].win_num) {
		case 0:
		case 1:
		case 2:
		case 3:
			win_ctrl_reg = PEX_WIN0_3_CTRL_REG(PEX_0, win_map[target].win_num);
			win_base_reg = PEX_WIN0_3_BASE_REG(PEX_0, win_map[target].win_num);
			win_remap_reg = PEX_WIN0_3_REMAP_REG(PEX_0, win_map[target].win_num);
			break;
		case 4:
		case 5:
			win_ctrl_reg = PEX_WIN4_5_CTRL_REG(PEX_0, win_map[target].win_num);
			win_base_reg = PEX_WIN4_5_BASE_REG(PEX_0, win_map[target].win_num);
			win_remap_reg = PEX_WIN4_5_REMAP_REG(PEX_0, win_map[target].win_num);
			break;
		default:
			dev_err(&pdev->dev, "Not supported decoding window\n");
			return -ENODEV;
		}

		dprintk("pex_win %d for bar%d = 0x%x\n", win_map[target].win_num,
			bar_nr+1, readl(inter_regs + win_ctrl_reg));

		writel(base_addr, inter_regs + win_base_reg);
		writel(win_map[target].remap | PXWRR_REMAP_EN, inter_regs + win_remap_reg);
		writel(val, inter_regs + win_ctrl_reg);

		dprintk("BAR%d: pex_win_ctrl = 0x%x, pex_win_base 0 = 0x%x\n",
			bar_nr+1, readl(inter_regs + win_ctrl_reg), readl(inter_regs + win_base_reg));
	}

	return 0;
}

static void mv_release_pci_resources(struct pci_dev *pdev)
{
	int i;

	for (i = 0; i < PCI_STD_RESOURCE_END; i++) {
		struct resource *res = pdev->resource + i;
		if (res->parent)
			release_resource(res);
	}
}

static int mv_calc_bar_size(struct pci_decoding_window *win_map, uint8_t bar)
{
	uint8_t target, bar_nr;
	int size = 0;

	for (target = 0; win_map[target].enable != TBL_TERM; target++) {
		if (win_map[target].enable != ENABLE)
			continue;

		bar_nr = (win_map[target].win_bar_map >> PXWCR_WIN_BAR_MAP_OFFS) + 1;
		if (bar_nr != bar)
			continue;

		size += win_map[target].size;
	}

	if (!MV_IS_POWER_OF_2(size))
		size = (1 << (mvLog2(size) + 1));

	dprintk("%s: calculated bar size %d\n", __func__, size);
	return size;

}

static int mv_reconfig_bars(struct pci_dev *pdev, struct pci_decoding_window *prestera_sysmap_bar)
{
	int i, err, size;

	if (!pdev->resource->parent) {
		mv_release_pci_resources(pdev);
		err = mv_read_and_assign_bars(pdev, MV_PCI_BAR_INTER_REGS);
		if (err != 0)
			return err;
	}

	inter_regs = pci_iomap(pdev, MV_PCI_BAR_INTER_REGS, _1M);
	if (inter_regs == NULL) {
		dev_err(&pdev->dev, "failed to remap registers\n");
		return -ENODEV;
	}
	dprintk("%s: inter_regs 0x%p\n", __func__, inter_regs);

	size = mv_calc_bar_size(prestera_sysmap_bar, BAR_1);
	mv_resize_bar(PEX_0, BAR_1, SIZE_TO_BAR_REG(size));

	size = mv_calc_bar_size(prestera_sysmap_bar, BAR_2);
	mv_resize_bar(PEX_0, BAR_2, SIZE_TO_BAR_REG(size));

	iounmap(inter_regs);

	mv_release_pci_resources(pdev);

	err = mv_read_and_assign_bars(pdev, MV_PCI_BAR_1);
	if (err != 0)
		return err;

	err = mv_read_and_assign_bars(pdev, MV_PCI_BAR_2);
	if (err != 0)
		return err;

	err = mv_read_and_assign_bars(pdev, MV_PCI_BAR_INTER_REGS);

	inter_regs = pci_iomap(pdev, MV_PCI_BAR_INTER_REGS, _1M);
	if (inter_regs == NULL) {
		dev_err(&pdev->dev, "failed to remap registers\n");
		return -ENODEV;
	}
	dprintk("%s: inter_regs 0x%p\n", __func__, inter_regs);

	for (i = 0; i < 6; i++) {
		if (i < 4) {
			writel(0, inter_regs + PEX_WIN0_3_CTRL_REG(PEX_0, i));
			writel(0, inter_regs + PEX_WIN0_3_BASE_REG(PEX_0, i));
			writel(0, inter_regs + PEX_WIN0_3_REMAP_REG(PEX_0, i));
		} else {
			writel(0, inter_regs + PEX_WIN4_5_CTRL_REG(PEX_0, i));
			writel(0, inter_regs + PEX_WIN4_5_BASE_REG(PEX_0, i));
			writel(0, inter_regs + PEX_WIN4_5_REMAP_REG(PEX_0, i));
		}
	}

	err = mv_configure_win_bar(prestera_sysmap_bar, pdev);
	if (err)
		return err;

	dprintk("%s: decoding win for BAR1 and BAR2 configured\n", __func__);

	return err;
}

static void mv_calc_device_address_range(struct pci_dev *dev,  unsigned int devInstance, int index)
{
	unsigned int i = 0;
	unsigned long startAddr, endAddr;
	unsigned long tempStartAddr, tempEndAddr;

	startAddr = pci_resource_start(dev, i);
	endAddr = pci_resource_end(dev, i);

	for (i = 2; i < PCI_STD_RESOURCE_END; i += 2) {
		tempStartAddr = pci_resource_start(dev, i);
		tempEndAddr = pci_resource_end(dev, i);

		if ((tempStartAddr != 0) && (tempEndAddr != 0)) {
			if (tempStartAddr < startAddr)
				startAddr = tempStartAddr;
			if (tempEndAddr > endAddr)
				endAddr = tempEndAddr;
		}
	}

	idtSwCfg[index].idtSwPortCfg.startAddr[devInstance] = startAddr;
	idtSwCfg[index].idtSwPortCfg.endAddr[devInstance] = endAddr;
}

static void mv_find_pci_dev_pp_instances(void)
{
	struct pci_dev *dev = NULL;
	int index;

	memset(&(idtSwCfg[0]), 0, ((sizeof(struct idtSwitchConfig)) * MAX_NUM_OF_IDT_SWITCH));
	for (index = 0; index < MAX_NUM_OF_IDT_SWITCH; index++) {
		idtSwCfg[index].ppIdtSwitchUsBusNum = 0xFF;
		idtSwCfg[index].ppIdtSwitchDsBusNum = 0xFF;
	}

	while ((dev = pci_get_device(PCI_ANY_ID, PCI_ANY_ID, dev)) != NULL) {
		if (dev->vendor == PCI_VENDOR_ID_IDT_SWITCH) {
			for (index = 0; index < MAX_NUM_OF_IDT_SWITCH; index++) {
				if (idtSwCfg[index].ppIdtSwitchUsBusNum == 0xFF) {
					idtSwCfg[index].ppIdtSwitchUsBusNum = dev->bus->number;
					idtprintk("idt -%d- us bus number %d\n", index, dev->bus->number);
					break;
				}

				if (idtSwCfg[index].ppIdtSwitchDsBusNum == 0xFF) {
					idtSwCfg[index].ppIdtSwitchDsBusNum = dev->bus->number;
					idtprintk("idt -%d- ds bus number %d\n", index, dev->bus->number);
					break;
				}

				if ((idtSwCfg[index].ppIdtSwitchUsBusNum == dev->bus->number) ||
					(idtSwCfg[index].ppIdtSwitchDsBusNum == dev->bus->number))
					break;
			}
		} else if (dev->vendor == MARVELL_VEN_ID) {
			if (dev->bus->parent != NULL) {
				idtprintk("mrvl dev, bus number %d, parent %d\n",
					dev->bus->number,  dev->bus->parent->number);

				for (index = 0; index < MAX_NUM_OF_IDT_SWITCH; index++)
					if (idtSwCfg[index].ppIdtSwitchDsBusNum == dev->bus->parent->number)
						break;

				mv_calc_device_address_range(dev, idtSwCfg[index].numOfPpInstances, index);
				idtSwCfg[index].idtSwPortCfg.ppBusNumArray[idtSwCfg[index].numOfPpInstances] =
												dev->bus->number;
				idtSwCfg[index].numOfPpInstances++;
			}
		}
	}
}

static void mv_discover_active_pp_instances(void)
{
	int i, num;
	struct pci_dev *dev = NULL;
	unsigned short pciLinkStatusRegVal;
	int index;

	for (index = 0; index < MAX_NUM_OF_IDT_SWITCH; index++) {
		for (num = 0, i = 0; num < idtSwCfg[index].numOfPpInstances; i++) {
			dev = pci_get_bus_and_slot(idtSwCfg[index].ppIdtSwitchDsBusNum, PCI_DEVFN(i + 2, 0));
			if (dev != NULL) {
				pci_read_config_word(dev, (int)MV_IDT_SWITCH_PCI_LINK_STATUS_REG, &pciLinkStatusRegVal);
				if (pciLinkStatusRegVal & MV_IDT_SWITCH_PCI_LINK_STATUS_REG_ACTIVE_LINK)
					idtSwCfg[index].idtSwPortCfg.ppIdtSwitchDsPpDevices[num++] = i + 2;
			}
		}
	}
}

static void mv_configure_idt_switch_addr_range(void)
{
	int i;
	struct pci_dev *dev = NULL;
	int index;

	for (index = 0; index < MAX_NUM_OF_IDT_SWITCH; index++) {
		for (i = 0; i < idtSwCfg[index].numOfPpInstances; i++) {
			dev = pci_get_bus_and_slot(idtSwCfg[index].ppIdtSwitchDsBusNum,
						  PCI_DEVFN(idtSwCfg[index].idtSwPortCfg.ppIdtSwitchDsPpDevices[i], 0));
			if (dev != NULL) {
				pci_write_config_dword(dev, MV_IDT_SWITCH_PCI_MEM_BASE_REG,
							((idtSwCfg[index].idtSwPortCfg.endAddr[i] &
							MV_IDT_SWITCH_PCI_MEM_BASE_REG_MASK) |
							(((idtSwCfg[index].idtSwPortCfg.startAddr[i]) &
							MV_IDT_SWITCH_PCI_MEM_BASE_REG_MASK) >> 16)));
			}
		}

		if (idtSwCfg[index].numOfPpInstances > 0) {
			dev = pci_get_bus_and_slot(idtSwCfg[index].ppIdtSwitchUsBusNum, PCI_DEVFN(0, 0));
			pci_write_config_dword(dev, MV_IDT_SWITCH_PCI_MEM_BASE_REG,
				((idtSwCfg[index].idtSwPortCfg.endAddr[idtSwCfg[index].numOfPpInstances - 1] &
				MV_IDT_SWITCH_PCI_MEM_BASE_REG_MASK) |
				(((idtSwCfg[index].idtSwPortCfg.startAddr[0]) &
				MV_IDT_SWITCH_PCI_MEM_BASE_REG_MASK) >> MV_IDT_SWITCH_PCI_MEM_BASE_REG_SHIFT)));
		}
	}
}

static void mv_print_idt_switch_configuration(void)
{
	int i;
	unsigned int pciBaseAddr;
	struct pci_dev *dev = NULL;
	int index;

	for (index = 0; index < MAX_NUM_OF_IDT_SWITCH; index++) {
		if (idtSwCfg[index].numOfPpInstances > 0) {
			idtprintk("\n");
			idtprintk("PEX IDT Switch Configuration\n");
			idtprintk("============================\n");
			idtprintk("                  %02x:00.0 (IDT Uplink port)\n",
				idtSwCfg[index].ppIdtSwitchUsBusNum);
			idtprintk("                     |\n");
			idtprintk("---------------------|---------------------\n");
			idtprintk("%02x:%02x.0     %02x:%02x.0     %02x:%02x.0     %02x:%02x.0 (IDT Downlink ports)\n",
				idtSwCfg[index].ppIdtSwitchDsBusNum,
				idtSwCfg[index].idtSwPortCfg.ppIdtSwitchDsPpDevices[0],
				idtSwCfg[index].ppIdtSwitchDsBusNum,
				idtSwCfg[index].idtSwPortCfg.ppIdtSwitchDsPpDevices[1],
				idtSwCfg[index].ppIdtSwitchDsBusNum,
				idtSwCfg[index].idtSwPortCfg.ppIdtSwitchDsPpDevices[2],
				idtSwCfg[index].ppIdtSwitchDsBusNum,
				idtSwCfg[index].idtSwPortCfg.ppIdtSwitchDsPpDevices[3]);
			idtprintk("%02x:00.0     %02x:00.0     %02x:00.0     %02x:00.0 (Marvell Devices)\n",
				idtSwCfg[index].idtSwPortCfg.ppBusNumArray[0],
				idtSwCfg[index].idtSwPortCfg.ppBusNumArray[1],
				idtSwCfg[index].idtSwPortCfg.ppBusNumArray[2],
				idtSwCfg[index].idtSwPortCfg.ppBusNumArray[3]);
			idtprintk("\n");
		}
	}

	for (index = 0; index < MAX_NUM_OF_IDT_SWITCH; index++) {
		for (i = 0; i < idtSwCfg[index].numOfPpInstances; i++) {
			dev = pci_get_bus_and_slot(idtSwCfg[index].ppIdtSwitchDsBusNum,
				PCI_DEVFN(idtSwCfg[index].idtSwPortCfg.ppIdtSwitchDsPpDevices[i], 0));
			if (dev != NULL) {
				pci_read_config_dword(dev, (int)MV_IDT_SWITCH_PCI_MEM_BASE_REG, &pciBaseAddr);
				idtprintk("PEX IDT Downlink port [bus 0x%x device 0x%x] Base address 0x%08x\n",
					idtSwCfg[index].ppIdtSwitchDsBusNum,
					idtSwCfg[index].idtSwPortCfg.ppIdtSwitchDsPpDevices[i], pciBaseAddr);
			}
		}

		if (idtSwCfg[index].numOfPpInstances > 0) {
			dev = pci_get_bus_and_slot(idtSwCfg[index].ppIdtSwitchUsBusNum, PCI_DEVFN(0, 0));
			pci_read_config_dword(dev, (int)MV_IDT_SWITCH_PCI_MEM_BASE_REG, &pciBaseAddr);
			idtprintk("PEX IDT Uplink port   [bus 0x%x device 0x%x] Base address 0x%08x\n\n",
				idtSwCfg[index].ppIdtSwitchUsBusNum, 0, pciBaseAddr);
		}
	}
	idtprintk("\n");
}

void mv_reconfig_idt_switch(void)
{
	 
	mv_find_pci_dev_pp_instances();

	mv_discover_active_pp_instances();

	mv_configure_idt_switch_addr_range();

	mv_print_idt_switch_configuration();
}

#ifdef CONFIG_MV_INCLUDE_DRAGONITE_XCAT
int __init mv_msys_dragonite_init(struct pci_dev *pdev, struct pci_decoding_window *prestera_sysmap_bar)
{
	void __iomem * const *iomap = NULL;
	void __iomem *switch_reg, *dfx_reg;
	struct resource *dragonite_resources;
	struct dragonite_info *dragonite_pci_data;
	struct platform_device *mv_dragonite_dev;
	const int msys_interregs_phys_base =
				pci_resource_start(pdev, MV_PCI_BAR_INTER_REGS);
	const int itcm_phys = pci_resource_start(pdev, MV_PCI_BAR_2) +
					prestera_sysmap_bar[DITCMW].base_offset;
	const int dtcm_phys = pci_resource_start(pdev, MV_PCI_BAR_2) +
					prestera_sysmap_bar[DDTCMW].base_offset;

	dragonite_resources = devm_kzalloc(&pdev->dev, 4 * sizeof(struct resource), GFP_KERNEL);
	if (!dragonite_resources)
		return -ENOMEM;

	dragonite_pci_data = devm_kzalloc(&pdev->dev, sizeof(struct dragonite_info), GFP_KERNEL);
	if (!dragonite_pci_data)
		return -ENOMEM;

	mv_dragonite_dev =  devm_kzalloc(&pdev->dev, sizeof(struct platform_device), GFP_KERNEL);
	if (!mv_dragonite_dev)
		return -ENOMEM;

	dev_dbg(&pdev->dev, "Dragonite init...\n");

	iomap = pcim_iomap_table(pdev);
	inter_regs = iomap[MV_PCI_BAR_INTER_REGS];
	switch_reg = iomap[MV_PCI_BAR_1];
	dfx_reg = iomap[MV_PCI_BAR_2];

	dragonite_resources[0].start	= itcm_phys;
	dragonite_resources[0].end	= itcm_phys + ITCM_SIZE - 1;
	dragonite_resources[0].flags	= IORESOURCE_MEM;

	dragonite_resources[1].start	= dtcm_phys;
	dragonite_resources[1].end	= dtcm_phys + DTCM_SIZE - 1;
	dragonite_resources[1].flags	= IORESOURCE_MEM;

	dragonite_resources[2].start	= msys_interregs_phys_base;
	dragonite_resources[2].end	= msys_interregs_phys_base + _1M - 1;
	dragonite_resources[2].flags	= IORESOURCE_MEM;

	dragonite_resources[3].start	= pdev->irq;
	dragonite_resources[3].end	= pdev->irq;
	dragonite_resources[3].flags	= IORESOURCE_IRQ;

	dragonite_pci_data->ctrl_reg	= (void *)(switch_reg + DRAGONITE_CTRL_REG);
	dragonite_pci_data->jtag_reg	= (void *)(dfx_reg + DRAGONITE_DEBUGGER_REG);
	dragonite_pci_data->poe_cause_irq_reg = (void *)(switch_reg + DRAGONITE_POE_CAUSE_IRQ_REG);
	dragonite_pci_data->poe_mask_irq_reg = (void *)(switch_reg + DRAGONITE_POE_MASK_IRQ_REG);
	dragonite_pci_data->host2poe_irq_reg = (void *)(switch_reg + DRAGONITE_HOST2POE_IRQ_REG);

	mv_dragonite_dev->name		= "dragonite_xcat";
#if defined(MY_ABC_HERE)
	mv_dragonite_dev->id		= 1;
#else  
	mv_dragonite_dev->id		= -1;
#endif  
	mv_dragonite_dev->dev.platform_data = dragonite_pci_data;
	mv_dragonite_dev->num_resources	= 4;
	mv_dragonite_dev->resource	= dragonite_resources;

	platform_device_register(mv_dragonite_dev);

	return 0;
}
#endif

static int prestera_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	int err;
	static int pexSwitchConfigure;
	struct pci_decoding_window *prestera_sysmap_bar;
	unsigned short tmpDevId = pdev->device;
#ifdef CONFIG_MV_INCLUDE_SERVICECPU
	void __iomem * const *iomap;
#endif

	if (((pdev->device & ~MV_DEV_FLAVOUR_MASK) == MV_BOBCAT2_DEV_ID) ||
		((pdev->device & ~MV_DEV_FLAVOUR_MASK) == MV_ALLEYCAT3_DEV_ID))
		tmpDevId &= ~MV_DEV_FLAVOUR_MASK;

	switch (tmpDevId) {

	case MV_IDT_SWITCH_DEV_ID_808E:
	case MV_IDT_SWITCH_DEV_ID_802B:
		if (pexSwitchConfigure == 0) {
			mv_reconfig_idt_switch();
			pexSwitchConfigure++;
		}
		return 0;

	case MV_BOBCAT2_DEV_ID:
	case MV_ALLEYCAT3_DEV_ID:

		prestera_sysmap_bar = get_pci_sysmap(pdev);
		if (!prestera_sysmap_bar)
			return -ENXIO;

		err = pcim_enable_device(pdev);
		if (err)
			return err;

		err = mv_reconfig_bars(pdev, prestera_sysmap_bar);
		if (err != 0)
			return err;

		err = pcim_iomap_regions(pdev, (1 << MV_PCI_BAR_INTER_REGS) |
					(1 << MV_PCI_BAR_1) |
					(1 << MV_PCI_BAR_2), prestera_drv_name);
		if (err)
			return err;
#ifdef CONFIG_MV_INCLUDE_DRAGONITE_XCAT
		if (tmpDevId == MV_ALLEYCAT3_DEV_ID)
			err = mv_msys_dragonite_init(pdev, prestera_sysmap_bar);
			if (err)
				return err;
#endif

#ifdef CONFIG_MV_INCLUDE_SERVICECPU
		iomap = pcim_iomap_table(pdev);
		servicecpu_data.inter_regs_base = iomap[MV_PCI_BAR_INTER_REGS];
		servicecpu_data.pci_win_size = prestera_sysmap_bar[SCPUW].size;
		servicecpu_data.pci_win_virt_base =
			iomap[MV_PCI_BAR_2] + prestera_sysmap_bar[SCPUW].base_offset;
		servicecpu_data.pci_win_phys_base = (void *)(pci_resource_start(pdev,
			MV_PCI_BAR_2) + prestera_sysmap_bar[SCPUW].base_offset);
		servicecpu_init();
#endif
		break;

	case MV_LION2_DEV_ID:

		err = pcim_enable_device(pdev);
		if (err)
			return err;

		err = pcim_iomap_regions(pdev, ((1 << MV_PCI_BAR_INTER_REGS) |
						(1 << MV_PCI_BAR_1)),
						prestera_drv_name);
		if (err)
			return err;

		break;

	default:
		 dprintk("%s: unsupported device\n", __func__);
	}

	dev_info(&pdev->dev, "%s init completed\n", prestera_drv_name);
	return 0;
}

static void prestera_pci_remove(struct pci_dev *pdev)
{
	dprintk("%s\n", __func__);

#ifdef CONFIG_MV_INCLUDE_SERVICECPU
		servicecpu_deinit();
#endif
}

static DEFINE_PCI_DEVICE_TABLE(prestera_pci_tbl) = {
	 
	{ PCI_DEVICE(PCI_VENDOR_ID_IDT_SWITCH, MV_IDT_SWITCH_DEV_ID_808E)},
	{ PCI_DEVICE(PCI_VENDOR_ID_IDT_SWITCH, MV_IDT_SWITCH_DEV_ID_802B)},
	MV_PCI_DEVICE_FLAVOUR_TABLE_ENTRIES(MV_BOBCAT2_DEV_ID),
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, MV_LION2_DEV_ID)},
	MV_PCI_DEVICE_FLAVOUR_TABLE_ENTRIES(MV_ALLEYCAT3_DEV_ID),
	{}
};

static struct pci_driver prestera_pci_driver = {
	.name			= prestera_drv_name,
	.id_table		= prestera_pci_tbl,
	.probe			= prestera_pci_probe,
	.remove			= prestera_pci_remove,
};

static int __init prestera_pci_init(void)
{
	return pci_register_driver(&prestera_pci_driver);
}

static void __exit prestera_pci_cleanup(void)
{
	pci_unregister_driver(&prestera_pci_driver);
}

MODULE_AUTHOR("Grzegorz Jaszczyk <jaz@semihalf.com>");
MODULE_DESCRIPTION("pci device driver for Marvell Prestera family switches");
MODULE_LICENSE("GPL");

module_init(prestera_pci_init);
module_exit(prestera_pci_cleanup);
