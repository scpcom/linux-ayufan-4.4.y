/*
file name	hisi_ipcm_proto.h
author		chendazheng
date:		2014-08-06
ver:		0.0.1
copyright:   hisi
*/
#ifndef __HISI_IPCM_PROTO_H__
#define __HISI_IPCM_PROTO_H__

#include <linux/kernel.h>
#include <linux/init.h>
#include "hios_type.h"
#include <linux/list.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/timer.h>
#include <mach/platform.h>
#include <linux/semaphore.h>

/* max cpu num ipc support */
#define HISI_IPCM_CPU_MAX	(2)

/* the max number of port */
#define HISI_MAX_IPCM_PORT	(1024)

/* message head type */
#define HISI_IPC_HEAD_MAGIC_INITIAL	(0x299) /* 1010011001b */
#define HISI_IPC_HEAD_MAGIC_JUMP	(0x166) /* 0101100110b*/

/* message priority */
#define HISI_IPCM_NORMAL_MSG	     (1 << 0)
#define HISI_IPCM_PRIORITY_MSG	     (1 << 1)

/* message type */
#define HISI_IPCM_INITIAL_DATA		(1 << 2)
#define HISI_IPCM_NORMAL_DATA		(1 << 3)

/* the threshold of timeout */
#define IPCM_TIMEOUT_IN_THREAD_AREA (20)

/* the max core number */
#define HISI_IPC_MAX_CORE_NUM  (0x5)

/* the start address and total size of shared memory */
#define SHARE_MEM_START_ADDR   CONFIG_HIIPCM_SHARE_MEM_ADDR
#define SHARE_MEM_TOTAL_SIZE   CONFIG_HIIPCM_SHARE_MEM_SIZE

/* shared memory size for the core of A17 */
#define A17_CORE_IRQ_SHARED_MEM_SIZE       (0x400)
#define A17_CORE_THR_SHARED_MEM_SIZE       (0xc00)

/* shared memory size of the core of A7 */
#define A7_CORE_IRQ_SHARED_MEM_SIZE        (0x800)
#define A7_CORE_THR_SHARED_MEM_SIZE        (0x1800)

/* share memory flag */
#define  IS_IRQ_SHARED_MEM         (0x01)
#define  IS_THR_SHARED_MEM         (0x10)

/* the mask for mem flag registers*/
#define MEM_FLG_BUSY_MASK	(0x1UL << 31)

/* gic hardware address */
#define CFG_GIC_CPU_BASE    (IO_ADDRESS(REG_BASE_A17_PERI)\
				+ REG_A17_PERI_GIC_CPU)
#define CFG_GIC_DIST_BASE   (IO_ADDRESS(REG_BASE_A17_PERI)\
				+ REG_A17_PERI_GIC_DIST)

/* the offset addr of GICD_SGIR */
#define GIC_DIST_SOFTINT	(0xf00)

/* the gic irq for ipc */
#define IPC_GIC_IRQ     CONFIG_HIIPCM_IRQNUM

/* gic cpu interface map */
#define GIC_CPU_INTR4_MAP       ((0x1UL << 4) << 16)
#define GIC_CPU_INTR0_MAP       (0x1UL << 16)

/* the gic cpu interface allocated to A7 */
#define A7_CPU_INTRF   (0x4)

/* ipc data register base address */
#define IPC_DATA_REG_BASE  (IO_ADDRESS(CONFIG_HIIPCM_DATA_REG_BASE) + 0x200)

/* get the offset of data register */
#define GET_DATA_REG_OFFSET(cpu_intrf, reg_index)\
	((cpu_intrf) * 4 * sizeof(int) + (reg_index) * sizeof(int))

/* ipc irq handle description */
struct gic_sgi_handle {
	unsigned int irq;
	void (*handle)(unsigned int cpu_intrf,
			unsigned int irq_num, struct pt_regs *regs);
};

/* ipc device description */
typedef struct hisi_ipcm_dev {
	/* gic distibutor  base address */
	unsigned int  gic_dist_base;

	/* gic cpu interface base address */
	unsigned int  gic_cpu_inf_base;

	/* data register base address */
	unsigned int  data_reg_base;

	/* if remote_map_flag is not zero*
	 * means remote mcc is running. */
	unsigned int  remote_map_flag;

	unsigned int  irq;		/* irq index */
	unsigned int  local_cpu_id;	/* local cpu index */
	unsigned int sended_irqs;	/* the number of sended irqs */
	unsigned int received_irqs;	/* the number of received irqs */

	struct timer_list timer;	/* timer list for send data */

	void *data;			/* keep other data reserved */
} hisi_ipc_dev_t;

/* the prototype of user receive function */
typedef int (*ipcm_vdd_notifier_recvfrom)(void *handle,
		void *buf, unsigned int length);

/* the opt description */
typedef struct ipcm_vdd_opt {
	/* call-back receive function in interrupt */
	ipcm_vdd_notifier_recvfrom  recvfrom_notify;

	unsigned long data;	/* the user data */
} ipcm_vdd_opt_t;

/* ipc tarnsfer handle description */
typedef struct hisi_ipc_transfer_handle {
	unsigned int  target_id;	/* record target */
	unsigned int  port;	        /* record port */
	unsigned int  priority;		/* record priority */
	ipcm_vdd_opt_t ipc_opt;		/* recv option */
} hisi_ipc_transfer_handle_t;

/* ipc shared memory  description */
struct hisi_ipcm_memory_info {
	/* shared memory base address */
	unsigned long base_addr;

	/* shared memory end address */
	unsigned long end_addr;

	/* the length of buffer in the shared memory */
	unsigned long buf_len;

	/* the remain length of buffer in the shared memory */
	unsigned long buf_remain;

	/* the address of read pointer */
	unsigned long *rp_addr;

	/* the address of write pointer */
	unsigned long *wp_addr;

	/* the address of memory flag */
	unsigned long *flag_addr;

	/* semaphore for write shared memory */
	struct semaphore shared_handle_sem;
	unsigned int last_recv_w_offset;
};

/* ipc shared memory allocation description */
struct hisi_ipcm_shared_mem_alloc_info {
	/* shared memory start address */
	unsigned int  share_mem_start_addr;

	/* shared memory for irq environment */
	struct hisi_ipcm_memory_info irq_mem[HISI_IPC_MAX_CORE_NUM];

	/* shared memory for thread environment */
	struct hisi_ipcm_memory_info thread_mem[HISI_IPC_MAX_CORE_NUM];
};

/* ipc message head description */
struct hisi_ipcm_transfer_head {
	unsigned int target_id:6;	/* target_id */
	unsigned int src:6;		/* scr */
	unsigned int port:10;		/* port */
	unsigned int magic:10;		/* magic number for recv */
	unsigned int length:20;		/* data length */
	unsigned int rsv:12;		/* for byte align */
};

/* APIs */
void *ipcm_vdd_open(int target_id, int port, int priority);
void ipcm_vdd_close(void *handle);
int ipcm_vdd_sendto(void *handle, const void *buf,
			unsigned int len, unsigned int flag);
int ipcm_vdd_getopt(void *handle, ipcm_vdd_opt_t *opt);
int ipcm_vdd_setopt(void *handle, ipcm_vdd_opt_t *opt);
int ipcm_vdd_localid(void);
int ipcm_vdd_remoteid(int *target_id, void *handle);
int ipcm_vdd_check_remote(void);

#endif
/* end of file */
