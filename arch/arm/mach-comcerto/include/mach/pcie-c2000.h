
/*
 * linux/arch/arm/mach-comcerto/pcie-comcerto2000.h
 *
 * Copyright (C) 2004,2005 Mindspeed Technologies, Inc.
 *
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


#ifndef __PCIE_C2000__
#define __PCIE_C2000__

#define NUM_PCIE_PORTS	CONFIG_COMCERTO_NUM_PCIES
#define MAX_PCIE_PORTS  2

struct pcie_port {
	u8			port;
	u8			root_bus_nr;
	u16			reset;
	unsigned long		base;
	unsigned long		remote_mem_baseaddr;
	unsigned long		app_base;
	void __iomem		*va_app_base;
	void __iomem		*va_dbi_base;
	unsigned long		cfg0_base;
	void __iomem		*va_cfg0_base;
	unsigned long		cfg1_base;
	void __iomem		*va_cfg1_base;
	unsigned int		cfg0_prev_taddr;
	unsigned int		cfg1_prev_taddr;
	unsigned int		cmd_reg_val;
	spinlock_t		conf_lock;
	spinlock_t		intr_lock;
	spinlock_t		msi_map_lock;
	char			mem_space_name[16];
	char			io_space_name[16];
	int 			port_mode;
	int 			link_state;
	int 			intx_base;
	int			msi_base;
	int			irq;
	dma_addr_t 		msi_mbox_handle;
	void 			*msi_mbox_baseaddr;
	struct pcie_app_reg	*app_regs;
	struct resource		res[2];
	struct clk		*ref_clock;
#if defined(CONFIG_COMCERTO_PCIE_OCC_CLOCK)
	struct clk              *occ_clock;
#endif
};

#define PCIE_PORT_MODE_NONE 	-1
#define PCIE_PORT_MODE_EP 	CFG0_DEV_TYPE_EP
#define PCIE_PORT_MODE_RC 	CFG0_DEV_TYPE_RC

/* PCIe PM related defines */
#define PCI_CAP_PM 0x40


/* The following register definitions are as per "DWC_regs_rev04.doc" document */

struct pcie_app_reg {
	u32	cfg0;
	u32	cfg1;
	u32	cfg2;
	u32	cfg3;
	u32	cfg4;
	u32	cfg5;
	u32	cfg6;
	u32	sts0;
	u32	sts1;
	u32	sts2;
	u32	sts3;
	u32	pwr_cfg_bdgt_data;
	u32	pwr_cfg_bdgt_fn;
	u32	radm_sts;
	u32	pwr_sts_bdgt;
	u32	intr_sts;
	u32	intr_en;
	u32	intr_msi_sts;
	u32	intr_msi_en;
};

#endif
