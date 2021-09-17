/******************************************************************************
 *    COPYRIGHT (C) 2013 llui. Hisilicon
 *    All rights reserved.
 * ***
 *    Create by llui 2013-09-03
 *
******************************************************************************/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/spinlock.h>
#include <asm/byteorder.h>
#include <linux/io.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <linux/kthread.h>
#include <mach/hardware.h>

#define HIUSBOTG_AUTHOR                "hisilicon LiuHui"
#define HIUSBOTG_DESC                  "USB 2.0 OTG Driver"

static char *usbotg_name = "usbotg";

extern int do_usbotg(void);

static struct task_struct *kusbotg_task = NULL;

/******************************************************************************/
#define SWITCH_HOST_DEVICE_REG	__io_address(CONFIG_HIUSB_SWITCH_HOST_DEVICE)
#define DWC_OTG_SWITCH_BIT	(1 << CONFIG_SWITCH_HIUSB_HOST_DEVICE_BIT)

static int usbdev_connect;
static int usbhost_connect;

/*  CPU is in host status, check if there is device connectted. */
void set_usbhost_connect(int index, int online)
{
	if (index == 0)
		usbhost_connect = online;
}
EXPORT_SYMBOL(set_usbhost_connect);

/*  CPU is in device status, check if there is host connectted. */
void set_usbdev_connect(int index, int online)
{
	if (index == 0)
		usbdev_connect = online;
}
EXPORT_SYMBOL(set_usbdev_connect);

static void device_to_host(void)
{
	int reg;

	reg = readl(SWITCH_HOST_DEVICE_REG);
	reg &= ~DWC_OTG_SWITCH_BIT;
	writel(reg, SWITCH_HOST_DEVICE_REG);
}

static void host_to_device(void)
{
	int reg;

	reg = readl(SWITCH_HOST_DEVICE_REG);
	reg |= DWC_OTG_SWITCH_BIT;
	writel(reg, SWITCH_HOST_DEVICE_REG);
}

int do_usbotg(void)
{
	int reg;

	reg = readl(SWITCH_HOST_DEVICE_REG);

	if (reg & DWC_OTG_SWITCH_BIT) {
		/* CPU is in device status */
		if (usbdev_connect)
			return 0;
		device_to_host();
	} else {
		/* CPU is in host status */
		if (usbhost_connect)
			return 0;
		host_to_device();
	}

	return 0;
}
EXPORT_SYMBOL(do_usbotg);

/******************************************************************************/
static int usbotg_thread(void *__unused)
{
	pr_info("%s: usb otg driver registered", usbotg_name);

#ifdef CONFIG_HIUSB_DEVICE2_0
#ifdef CONFIG_HIUSB_HOST
	while (!kthread_should_stop()) {
		do_usbotg();
		msleep(CONFIG_HIUSB_OTG_SWITCH_TIME);
	}
#else
	host_to_device();
#endif
#endif
	return 0;
}

static int __init usb_otg_init(void)
{
	kusbotg_task = kthread_run(usbotg_thread, NULL, "usb-otg");
	if (IS_ERR(kusbotg_task)) {
		pr_err("%s: creating kthread failed\n", usbotg_name);
		kusbotg_task = NULL;
		return -1;
	}
	return 0;
}

static void __exit usb_otg_exit(void)
{
	if (kusbotg_task) {
		kthread_stop(kusbotg_task);
		kusbotg_task = NULL;
	}
	pr_info("%s: usb otg driver exit", usbotg_name);
}

module_init(usb_otg_init);
module_exit(usb_otg_exit);

MODULE_DESCRIPTION(HIUSBOTG_DESC);
MODULE_AUTHOR(HIUSBOTG_AUTHOR);
MODULE_LICENSE("GPL");
