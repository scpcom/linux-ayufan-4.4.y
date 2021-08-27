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
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

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
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
	    this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer in the
	documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
	used to endorse or promote products derived from this software without
	specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <asm/irq_regs.h>

#include "mvTypes.h"
#include "mvOs.h"
#include "mvDebug.h"
#include "mvCommon.h"
#include "mvIpc.h"
#include "mv_ipc.h"
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "cpu/mvCpu.h"
#include "include/mach/smp.h"

extern unsigned int sh_mem_base, sh_mem_size;

void   *sh_virt_base;
void   *sh_mem_stack;
unsigned int sh_mem_remain;
unsigned int virt_phys_offset;
bool   ipcInitialized = 0;

MV_IPC_CHN ipc_drv_channels[MAX_IPC_CHANNELS];

#define IPC_RX_MAX_MSGS_PER_ISR		50

//#define IPC_DRV_DEBUG
#ifdef IPC_DRV_DEBUG
#define ipc_debug	printk
#else
#define ipc_debug(x...)
#endif

/****************************************************************************************
 * ipc_sh_malloc()                                 				        		*
 *   Allocate memory on AMP shared space
 ***************************************************************************************/
void* ipc_sh_malloc(unsigned int size)
{
	void *ptr;

	if(size > sh_mem_remain)
		return NULL;

	ptr = sh_mem_stack;

	sh_mem_stack  += size;
	sh_mem_remain -= size;

	return ptr;
}

/****************************************************************************************
 * ipc_virt_to_phys()                                 				        		*
 *   address translation for shared stack
 ***************************************************************************************/
void* ipc_virt_to_phys(void *virt_addr)
{
	void *phys_addr = 0;

	if((virt_addr >= sh_virt_base) && (virt_addr < (sh_virt_base + sh_mem_size)))
		phys_addr = (void *)((unsigned int)virt_addr - virt_phys_offset);

	return phys_addr;
}

/****************************************************************************************
 * ipc_phys_to_virt()                                 				        		*
 *   address translation for shared stack
 ***************************************************************************************/
void* ipc_phys_to_virt(void *phys_addr)
{
	void *virt_addr = 0;

	if(((int)phys_addr >= sh_mem_base) && ((int)phys_addr < (sh_mem_base + sh_mem_size)))
		virt_addr = (void *)((unsigned int)phys_addr + virt_phys_offset);

	return virt_addr;
}

/****************************************************************************************
 * ipc_init_shared_stack()                                 				        		*
 *   Initialize the shared stack used for communication
 ***************************************************************************************/
static int __init ipc_init_shared_stack(unsigned int sh_phys_base, unsigned int sh_mem_size,
										unsigned int reserved, unsigned int baseIdx)
{
	if(sh_mem_size < reserved) {
		printk(KERN_ERR "IPC: Shared mem size %d smaller then reserved %d\n", sh_mem_size, reserved);
		return 0;
	}

	/* Map shared memory and initialize shared stack */
	sh_virt_base  = ioremap(sh_phys_base, sh_mem_size);
	if(!sh_virt_base) {
		printk(KERN_ERR "IPC: Unable to map physical shared mem block (%#010x - %#010x)\n",
			sh_phys_base, sh_phys_base + sh_mem_size );
		return 0;
	}

	virt_phys_offset = (unsigned int)sh_virt_base - sh_phys_base;

	/* Reserve space shared by both amp groups */
	sh_mem_stack   = sh_virt_base + reserved;
	sh_mem_remain  = sh_mem_size  - reserved;

	/* Each group receives half of remaining memory */
	sh_mem_stack  += ((sh_mem_remain >> 1) * baseIdx);
	sh_mem_remain -= (sh_mem_remain >> 1);

	ipc_debug(KERN_INFO "IPC: Remaped Shared memory PA %#010x to VA %#010x\n",
	          (unsigned int) sh_phys_base, (unsigned int) sh_virt_base);

	ipc_debug(KERN_INFO "IPC: Based shared stack %#010x\n", (unsigned int) sh_mem_stack);

	return 1;
}

/****************************************************************************************
 * ipc_open_chn()                                 				        		*
 *   Initialize and register IPC network interface 										*
 ***************************************************************************************/
int ipc_open_chn(int chnId, IPC_RX_CLBK rx_clbk)
{
	MV_STATUS ret;

	ret = mvIpcOpenChannel(chnId);
	if(ret != MV_OK)
		return -1;

	ipc_drv_channels[chnId].rxCallback = rx_clbk;
	return 0;
}

/****************************************************************************************
 * do_ipc_rx_irq()                                 				        		*
 *  rx interrupt service routine 												*
 ***************************************************************************************/
void do_ipc_rx_irq(int irq, struct pt_regs *regs)
{
	int chnId = 0;
	MV_IPC_MSG *msg;
	int read_msgs = IPC_RX_MAX_MSGS_PER_ISR;
	struct pt_regs *old_regs = set_irq_regs(regs);

	ipc_debug(KERN_INFO "IPC: RX callback. got irq no = %d\n", irq);

	irq_enter();
	mvIpcDisableChnRx(irq);

	/* Pull msg from IPC HAL until no more msgs*/
	while (read_msgs)
	{
		if(mvIpcRxMsg(&chnId, &msg, irq) == MV_FALSE)
			break;

		if(ipc_drv_channels[chnId].rxCallback != 0)
			ipc_drv_channels[chnId].rxCallback(msg);

		read_msgs--;
	}

	mvIpcEnableChnRx(irq);
	irq_exit();

	if(read_msgs == IPC_RX_MAX_MSGS_PER_ISR)
		ipc_debug(KERN_WARNING "IPC: Received interrupt with no messages\n");

	set_irq_regs(old_regs);
}

/****************************************************************************************
 * ipc_init_module()                                 				        		*
 *   intialize and register IPC driver interface 										*
 ***************************************************************************************/
static int __init ipc_init_module(void)
{
	unsigned int cpuId = whoAmI();
	MV_STATUS status;
	int chnId;

	/* Initialize shared memory - Reserve space for ipc queues */
	ipc_init_shared_stack(sh_mem_base, sh_mem_size, MV_IPC_QUEUE_MEM, (cpuId != 0));

	status = mvIpcInit(sh_virt_base, (master_cpu_id == 0));
	if(status != MV_OK) {
		printk(KERN_ERR "IPC: IPC HAL initialization failed\n");
	}

	/* Reset Rx callback pointers */
	for(chnId = 0; chnId < MAX_IPC_CHANNELS; chnId++)
		ipc_drv_channels[chnId].rxCallback = 0;

	ipcInitialized = 1;

	printk(KERN_INFO "IPC: Driver initialized successfully\n");

	return 0;
}

/****************************************************************************************
 * ipc_cleanup_module()                                 				        	*
 *   close IPC driver 																*
 ***************************************************************************************/
static void __exit ipc_cleanup_module(void)
{
	ipcInitialized = 0;

	mvIpcClose();

	/* Unmap shared memory space */
	iounmap(sh_virt_base);
}

module_init(ipc_init_module);
module_exit(ipc_cleanup_module);
MODULE_DESCRIPTION("Marvell Inter Processor Communication (IPC) Driver");
MODULE_AUTHOR("Yehuda Yitschak <yehuday@marvell.com>");
MODULE_LICENSE("GPL");
