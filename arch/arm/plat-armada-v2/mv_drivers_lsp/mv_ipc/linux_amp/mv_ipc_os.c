/*******************************************************************************
   Copyright (C) Marvell International Ltd. and its affiliates

   This software file (the "File") is owned and distributed by Marvell
   International Ltd. and/or its affiliates ("Marvell") under the following
   alternative licensing terms.  Once you have made an election to distribute the
   File under one of the following license alternatives, please (i) delete this
   introductory statement regarding license alternatives, (ii) delete the two
   license alternatives that you have not elected to use and (iii) preserve the
   Marvell copyright notice above.

********************************************************************************
   Marvell GPL License Option

   If you received this File from Marvell, you may opt to use, redistribute and/or
   modify this File in accordance with the terms and conditions of the General
   Public License Version 2, June 1991 (the "GPL License"), a copy of which is
   available along with the File in the license.txt file or by writing to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
   on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

   THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
   WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
   DISCLAIMED.  The GPL License provides additional details about this warranty
   disclaimer.
*******************************************************************************/

#include "mvTypes.h"
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include "cpu/mvCpu.h"
#include "mvIpc.h"
#include "mvOs.h"

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <asm/irq_regs.h>

#include "mvDebug.h"
#include "mvCommon.h"
#include "mvIpc.h"
#include "mv_ipc_os.h"
#include "mv_ipc_common.h"
#include "mv_ipc_node.h"
#include "mv_ipc_doorbell.h"
#include "mv_ipc_poll.h"
#include "include/mach/smp.h"

unsigned int mv_ipc_sh_mem_base[MV_IPC_LINKS_NUM], mv_ipc_sh_mem_size[MV_IPC_LINKS_NUM];

void   *sh_virt_base[MV_IPC_LINKS_NUM];
unsigned int virt_phys_offset[MV_IPC_LINKS_NUM];

/*#define IPC_DRV_DEBUG*/
#ifdef IPC_DRV_DEBUG
#define ipc_debug       printk
#else
#define ipc_debug(x ...)
#endif

/*******************************************************************************
 * ipc_init_shared_stack()                                                     *
 *   Initialize the shared stack used for communication
 ******************************************************************************/
int mvIpcOsSharedStack(unsigned int link, unsigned int sh_phys_base,
		       unsigned int sh_mem_size)
{
	mv_ipc_sh_mem_base[link] = sh_phys_base;
	mv_ipc_sh_mem_size[link] = sh_mem_size;

	/* Map shared memory and initialize shared stack */
	sh_virt_base[link]  = ioremap(sh_phys_base, sh_mem_size);
	if (!sh_virt_base[link]) {
		printk(KERN_ERR "IPC: Unable to map physical shared mem block (%#010x - %#010x)\n",
		       sh_phys_base, sh_phys_base + sh_mem_size);
		return 0;
	}

	virt_phys_offset[link] = (unsigned int)sh_virt_base[link] - sh_phys_base;

	ipc_debug(KERN_INFO "IPC: Remaped Shared memory PA %#010x to VA %#010x\n",
		  (unsigned int)sh_phys_base, (unsigned int)sh_virt_base[link]);

	return 1;
}

/*******************************************************************************
 * ipc_virt_to_phys()                                                          *
 *   address translation for shared stack
 ******************************************************************************/
void *mvIpcOsVirt2Phys(unsigned int link, void *virt_addr)
{
	void *phys_addr = 0;

	if ((virt_addr >= sh_virt_base[link]) && (virt_addr < (sh_virt_base[link] +
							       mv_ipc_sh_mem_size[link])))
		phys_addr = (void *)((unsigned int)virt_addr - virt_phys_offset[link]);

	return phys_addr;
}

/*******************************************************************************
 * ipc_phys_to_virt()                                                          *
 *   address translation for shared stack
 ******************************************************************************/
void *mvIpcOsPhys2Virt(unsigned int link, void *phys_addr)
{
	void *virt_addr = 0;

	if (((int)phys_addr >= mv_ipc_sh_mem_base[link]) &&
		((int)phys_addr < (mv_ipc_sh_mem_base[link] + mv_ipc_sh_mem_size[link])))
		virt_addr = (void *)((unsigned int)phys_addr + virt_phys_offset[link]);

	return virt_addr;
}

/*******************************************************************************
 * ipc_get_virt_base()                                                         *
 *   address translation for base of shared stack
 ******************************************************************************/
void *mvIpcOsGetVirtBase(unsigned int link)
{
	return sh_virt_base[link];
}

/*******************************************************************************
 * mvOsSync()                                                                  *
 *   Barrier/ cache invalidate function
 ******************************************************************************/
void mvOsSync(void)
{
	dmb();
}

/*******************************************************************************
 * ipc_init_module()                                                           *
 *   intialize and register IPC driver interface                               *
 ******************************************************************************/
static int __init ipc_init_module(void)
{
	MV_STATUS status;

	status = mvIpcCommonInit();

	printk(KERN_INFO "IPC: Driver initialized successfully\n");

	return 0;
}

/*******************************************************************************
 * ipc_cleanup_module()                                                        *
 *   close IPC driver                                                          *
 ******************************************************************************/
static void __exit ipc_cleanup_module(void)
{
	mvIpcClose(0);

	/* Unmap shared memory space */
	iounmap(sh_virt_base[0]);
}

module_init(ipc_init_module);
module_exit(ipc_cleanup_module);
MODULE_DESCRIPTION("Marvell Inter Processor Communication (IPC) Driver");
MODULE_AUTHOR("Yehuda Yitschak <yehuday@marvell.com>");
MODULE_LICENSE("GPL");
