/*
 *  arch/arm/mach-comcerto/msp/msp.h
 *
 *  Copyright (C) 2006,2012 Mindspeed Technologies, Inc.
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

#ifndef _COMCERTO_MSP_H
#define _COMCERTO_MSP_H

#include <linux/platform_device.h>
#include <linux/firmware.h>
#include <linux/elf.h>
#include <linux/netdevice.h>
#include <asm/pgtable.h>
#include <mach/irqs.h>

#define SHF_MIPS_ADDR (0x40000000)

enum msp_state {
	MSP_RESETTING = 0,
	MSP_RESET,
	MSP_STARTING,
	MSP_RUNNING,
	MSP_CRASHED
};

struct msp_stack_frame {
	unsigned int r0;
	unsigned int r1;
	unsigned int r2;
	unsigned int r3;
	unsigned int r4;
	unsigned int r5;
	unsigned int r6;
	unsigned int r7;
	unsigned int r8;
	unsigned int r9;
	unsigned int r10;
	unsigned int r11;
	unsigned int r12;
	unsigned int sp;
	unsigned int lr;
	unsigned int pc;
};

struct alert_type {
	unsigned short type;
	unsigned short channel;
	unsigned short unique_id;
	unsigned short action;
	unsigned int abort_lr;
	unsigned int val1;
	unsigned int val2;
	unsigned int localtime;
};

struct _code_info {
	const unsigned char *code;
	unsigned long size;

	unsigned long checksum_program_addr;
	unsigned long program_addr;
	unsigned long proc_addr;
	unsigned long sym_addr;
	unsigned long checksum;
};

struct msp_info {
	unsigned long abi_rev;
	volatile unsigned long lock;
	unsigned long device;
	unsigned long revision;
	char msp_version[32];
	char spu_version[16];
	/*  */
	unsigned long CSPtoMSPQueuePhyaddr;
	unsigned long MSPtoCSPQueuePhyaddr;
	unsigned long SMRXCSPhyaddr;
	unsigned long SMTXCSPhyaddr;
	unsigned long SPDRV_ACP_MSP_Phyaddr;
	/*  */
	unsigned long ERAMsize;
	unsigned long ARAMsize;
	unsigned long IRAMsize;
	unsigned short ARMfreq;
	unsigned short AMBAfreq;
	unsigned short SPUfreq;
	unsigned long voip_ipaddr;
	volatile unsigned long IPoffload;
	volatile unsigned long ready;
	volatile unsigned long heartbeat;
	volatile unsigned long alert_number;
	volatile unsigned long alert_frame;
	volatile unsigned long save_alert;
	unsigned long pfe_ready;
	unsigned long pfe_virt;
	unsigned long pfe_phys;
	unsigned long axi_virt;
	unsigned long axi_phys;
	unsigned long msp_virt;
	unsigned long msp_phys;
};

struct voip_sym {
	void (*rtxc_handler)(void);
	void (*voip_entry)(void);
	void (*voip_exit)(void);
	void (*voip_crash)(void *);
};

struct comcerto_msp {
	unsigned long alert_seen;
	unsigned long last_tick;
	int state;

	volatile struct msp_info *info;
	struct _code_info code_info;
	struct sk_buff_head msp_list;
	struct timer_list timer_expire;
	struct ctl_table_header *sysctl_header;
	void* __iomem vectors_base;
};

int comcerto_download_to_msp(struct comcerto_msp *msp);
int comcerto_start_msp(struct comcerto_msp *msp);
void comcerto_stop_msp(struct comcerto_msp *msp);
int msp_init_sysfs(struct platform_device *pdev);

#endif  /* _COMCERTO_MSP_H */
