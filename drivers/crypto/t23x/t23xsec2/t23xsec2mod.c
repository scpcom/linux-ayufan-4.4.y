
/*
 * t23xsec2mod.c
 *
 * Linux specific driver module initialization for SEC2.x legacy
 * compatibility module
 *
 * Copyright (c) 2007, 2008 Freescale Semiconductor, Inc.
 *
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither the name of Freescale Semiconductor nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */


#include <linux/module.h>
#include <linux/fs.h>

/* Includes for SEC2 legacy interfaces */
#include "Sec2.h"
#include "Sec2local.h"

extern int SEC2xOpen(struct inode *nd, struct file *fil);
extern int SEC2xClose(struct inode *nd, struct file *fil);
extern int SEC2xIoctl(struct inode  *nd,
                      struct file   *fil,
                      unsigned int   code,
                      unsigned long  param);
extern int SEC2_ioctl(struct inode  *nd,
                      struct file   *fil,
                      unsigned int   code,
                      unsigned long  param);
extern int SEC2xDrvInit(void);
extern int SEC2xShutdown(void);


dev_t           sec2x_devmajor;



/** fopts struct from device registration */
struct file_operations SEC2x_fopts = {
    .open    = SEC2xOpen,
    .release = SEC2xClose,
    .ioctl   = SEC2xIoctl,
};

static int __init SEC2xInit(void)
{
    int rtstat;


    /* Notice that this registers a device major number dynamically, but it */
    /* leaves the node to the user to create...                             */

    rtstat = register_chrdev(SEC2X_DEVMAJOR, SEC2X_DEVNAME, &SEC2x_fopts);

    if (rtstat < 0)
    {
        printk(KERN_WARNING "t23xsec2: can't register driver, fatal\n");
        return rtstat;
    }

    if (SEC2X_DEVMAJOR == 0) /* if no specific major requested, and there was no error above */
        sec2x_devmajor = rtstat; /* set the dynamic number */

    /* Now go init */
    rtstat = SEC2xDrvInit();
    if (rtstat < 0)
        unregister_chrdev(sec2x_devmajor, SEC2X_DEVNAME);

    return rtstat;
}


static void __exit SEC2xExit(void)
{

    SEC2xShutdown();

    unregister_chrdev(sec2x_devmajor, SEC2X_DEVNAME);
}


EXPORT_SYMBOL (SEC2xIoctl);
EXPORT_SYMBOL (SEC2_ioctl);
EXPORT_SYMBOL (MarkScatterBuffer);

module_init(SEC2xInit);
module_exit(SEC2xExit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Extensible Crypto Driver - SEC 2.x Legacy Interface");
MODULE_AUTHOR("Freescale Semiconductor - NMG/STC");
