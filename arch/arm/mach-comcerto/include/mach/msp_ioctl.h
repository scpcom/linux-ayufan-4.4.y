/*
 *  arch/arm/mach-comcerto/include/mach/msp_ioctl.h
 *
 *  Copyright (C) 2004,2005 Mindspeed Technologies, Inc.
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

#ifndef _MSP_IOCTL_H
#define _MSP_IOCTL_H

struct MSP_IOCTL_IMAGE
{
	void *buf;
	unsigned long len;
};

struct MSP_IOCTL_MEM_DUMP
{
	void *buf;
	unsigned long addr;
	unsigned long len;
};

#ifdef __KERNEL__
int msp_ioctl_from_csp(unsigned int cmd, unsigned long arg);

/* Only valid when called from the CSP */
#define MSP_IOCTL_RESET_MSP_LOAD_FROM_KERNELBUF		_IOR(MSP_IOC_TYPE, 5, struct MSP_IOCTL_IMAGE)
#define MSP_IOCTL_RESET_MSP_DUMP_TO_KERNELBUF		_IOWR(MSP_IOC_TYPE, 6, struct MSP_IOCTL_IMAGE)

#endif

#define MSP_IOC_TYPE					'm'
#define MSP_IOCTL_DISPLAY_MSP_FROM_NORFLASH		_IO(MSP_IOC_TYPE, 1)
#define MSP_IOCTL_RESET_MSP_LOAD_FROM_NORFLASH		_IO(MSP_IOC_TYPE, 2)
#define MSP_IOCTL_RESET_MSP_LOAD_FROM_BUF		_IOR(MSP_IOC_TYPE, 3, struct MSP_IOCTL_IMAGE)
#define MSP_IOCTL_RESET_MSP_DUMP_TO_BUF			_IOR(MSP_IOC_TYPE, 4, struct MSP_IOCTL_MEM_DUMP)

#define MSP_DEVICE_NAME		"/dev/msp"

#define MSP_DEVICE_MAJOR_NUM	237         /* this could be whatever you want, as long as it does not conflict with other modules */

#endif /* _MSP_IOCTL_H */
