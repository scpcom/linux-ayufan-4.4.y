
/*
 *    This file has definition data structure and interfaces for
 *    hisilicon multicpu communication.
 *
 *    Copyright (C) 2008 hisilicon , chanjinn@hauwei.com
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 *    IPCM is short for Multy-Cpu-Communication
 *
 *    create by chanjinn, 2008.11.20
 */

#include <linux/ioctl.h>

#ifndef __HI_IPCM_USERDEV_H__
#define __HI_IPCM_USERDEV_H__

/* HI_IPCM_PORT_NR : max ports supported
 * HI_IPCM_RESERVE_PORT_NR : count of reserve ports
 */
#define HI_IPCM_PORT_NR 1024
#define HI_IPCM_RESERVE_PORT_NR 32

/* HI_IPCM_PORT_NR : max target id supported
* it depends on hardware pci device number
*/
#define HI_IPCM_TARGET_ID_IPCM_START	1
#define HISI_MAX_MAP_DEV		2

struct hi_ipcm_handle_attr {
	unsigned int target_id;
	unsigned int port;
	unsigned int priority;

	/*function  hios_ipcm_getremoteids to get remoteids.*/
	unsigned int remote_id[HISI_MAX_MAP_DEV - 1];
};

#define	HI_IOC_IPCM_BASE  'M'

/* Create a new ipcm handle. A file descriptor is only used*
 * once for one ipcm handle. */
#define HI_IPCM_IOC_CONNECT  \
	_IOW(HI_IOC_IPCM_BASE, 1, struct hi_ipcm_handle_attr)
#define HI_IPCM_IOC_CHECK  \
	_IOW(HI_IOC_IPCM_BASE, 2, struct hi_ipcm_handle_attr)
#define HI_IPCM_IOC_DISCONNECT  \
	_IOW(HI_IOC_IPCM_BASE, 3, unsigned long)
#define HI_IPCM_IOC_GET_LOCAL_ID \
	_IOW(HI_IOC_IPCM_BASE, 4, struct hi_ipcm_handle_attr)
#define HI_IPCM_IOC_GET_REMOTE_ID \
	_IOW(HI_IOC_IPCM_BASE, 5, struct hi_ipcm_handle_attr)
#define HI_IPCM_IOC_ATTR_INIT \
	_IOW(HI_IOC_IPCM_BASE, 6, struct hi_ipcm_handle_attr)

/* Bind a handle to a port */
#define HI_IPCM_IOC_SETOPTION  \
	_IOW(HI_IOC_IPCM_BASE, 3, unsigned int)

#ifdef __KERNEL__
#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/ioctl.h>
#include <linux/wait.h>
#define HI_IPCM_DBG_LEVEL 0x3
#define hi_ipcm_trace(level, s, params...) do {\
	if (level >= HI_IPCM_DBG_LEVEL)\
		pr_info("[%s, %d]: " s "\n",\
			__func__, __LINE__, params);\
	} while (0)

#define IPCM_PORT_ISVALID(s) (s < HI_IPCM_PORT_NR)
struct hios_ipcm_handle {
	unsigned long ipcm_handle;  /* ipcm handle_sem */
	struct list_head mem_list;  /* mem list for */
	wait_queue_head_t wait;
};

typedef struct hios_ipcm_handle  hios_ipcm_handle_t;

/* recvfrom_notify sample
*int myrecv_notify(void *handle ,void *buf, unsigned int data_len)
*{		        ~~~~~~~
*	struct hios_ipcm_handle hios_handle;
*	hios_ipcm_handle_opt opt;
*	unsigned long cus_data;
*	hios_handle.handle = (unsigned long) handle
*					     ~~~~~~
*	hios_ipcm_getopt(&hios_handle, &opt);
*	cus_data = opt.data;
*	...
*}
*/

struct hios_ipcm_handle_opt {
	/* Remaind:input parameter "void *handle"*
	 * is not the hios_ipcm_open return value */
	/* see recvfrom_notify the sample if use *
	 *want to get the value hide in opt->data */
	int (*recvfrom_notify)(void *handle, void *buf,
				unsigned int data_len);
	unsigned long data;
};

typedef struct hios_ipcm_handle_opt hios_ipcm_handle_opt_t;

hios_ipcm_handle_t *hios_ipcm_open(struct hi_ipcm_handle_attr *attr);
void hios_ipcm_handle_attr_init(struct hi_ipcm_handle_attr *attr);
int hios_ipcm_sendto(hios_ipcm_handle_t *handle,
			const void *buf, unsigned int len);
int hios_ipcm_close(hios_ipcm_handle_t *handle);
int hios_ipcm_getopt(hios_ipcm_handle_t *handle, hios_ipcm_handle_opt_t *opt);
int hios_ipcm_setopt(hios_ipcm_handle_t *handle,
			const hios_ipcm_handle_opt_t *opt);
int hios_ipcm_getlocalid(hios_ipcm_handle_t *handle);
int hios_ipcm_getremoteids(int ids[], hios_ipcm_handle_t *handle);
int hios_ipcm_check_remote(int remote_id, hios_ipcm_handle_t *handle);

#endif  /* __KERNEL__ */

#endif  /* __HI_IPCM_H__ */
