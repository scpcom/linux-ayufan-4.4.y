// Copyright (c) 2000-2009 Synology Inc. All rights reserved.

#include <linux/syno.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/errno.h>  /* error codes */
#include <linux/delay.h>
#include <linux/synobios.h>
#include <linux/fs.h>
#include <linux/rtc.h>
#include <linux/mc146818rtc.h>
#include <linux/bcd.h>
#include <linux/cpumask.h>
#include "bandon_common.h"

int SetUart(const char* cmd)
{
	int err = -1;

	if (NULL == cmd) {
		goto ERR;
	}

	// write cmd
	if (0 > syno_ttys_write(UART_TTYS_INDEX, cmd)) {
		goto ERR;
	}

	err = 0;
ERR:
	return err;
}
