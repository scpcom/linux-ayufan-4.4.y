/* This program is free software; you can redistribute it and/or modify
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
