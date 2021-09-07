
/*
 * t23xrmModule.c
 *
 * Linux specific driver module initialization,
 *
 * Current version is 100% dependent on a flat device
 * tree entry for each core, it does not auto-detect
 * hardware capability
 *
 * Copyright (c) 2007-2010 Freescale Semiconductor, Inc.
 * All Rights Reserved
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
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the  GNU General Public License along
 * with this program; if not, write  to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 2.1.0   2009_05_04 sec - remove /proc entries, simplify registration
 */

#include <linux/of_platform.h>
#include <linux/interrupt.h>

#include "../common/t23.h"
#include "../common/xwcRMinterface.h"
#include "t23xrmInternal.h"



/* FIXME: This should be picked up from a header... */
irqreturn_t t23RMintDoneHandler(int32_t, void *);

extern RMinterfaceCtx ifCtx;



/**
 * Basic initializer, called from RM module installation
 *
 * Note that this only maps in 1 instance of a Talitos core,
 * all that is possible at the present time. The OS dependent
 * parts of initialization happen here; the portable core
 * initialization code happens in t23RMdevInit()
 *
 * @return
 */
static int t23x_probe(struct of_device *ofdev,
                      const struct of_device_id *match)
{
    int         stat;
    uint32_t    devirq = 0;

    uint32_t   *channels = NULL;
    uint32_t   *fifod    = NULL;
    uint32_t   *eu_mask  = NULL;
    uint32_t   *typ_mask = NULL;
    const char* mode = NULL;
    T2CoreInstance *devinst;
    struct device_node *dn;
    struct device *dev;


    devinst = kzalloc(sizeof(T2CoreInstance), GFP_KERNEL);
    if (!devinst)
	return -ENOMEM;

    dev = &ofdev->dev;
    dev_set_drvdata(dev, devinst);
    dn = ofdev->node;

    devinst->regs = of_iomap(dn, 0);
    if (devinst->regs == NULL) {
        dev_err(dev, "t23x: can't map device register space\n");
	return -ENOMEM;
    }

    mode = of_get_property(dn, "fsl,multi-host-mode", NULL);
    if (mode && !strcmp(mode, "secondary")) {
	printk("t23x: can't work on CAMP secondary mode\n");
	return -1;
    }

    devinst->doneIRQid = of_irq_to_resource(dn, 0, NULL);

    /* Need to fetch capability bits from the dev node here */
    channels = (uint32_t *)of_get_property(dn, "fsl,num-channels", NULL);
    fifod    = (uint32_t *)of_get_property(dn, "fsl,channel-fifo-len", NULL);
    eu_mask  = (uint32_t *)of_get_property(dn, "fsl,exec-units-mask", NULL);
    typ_mask = (uint32_t *)of_get_property(dn, "fsl,descriptor-types-mask", NULL);

    if ((channels == NULL) ||
        (fifod    == NULL) ||
        (eu_mask  == NULL) ||
        (typ_mask == NULL))
    {
        printk("t23xrm: can't get a required property from device tree\n");
        return -1;
    }

    /*
     * Now go call the standard driver initialization
     * It's primary argument is the base address of the security block
     * in the address space of the chip
     */

    stat = t23RMdevInit(devinst, *channels, *fifod, *eu_mask, *typ_mask);

    if (stat == -1)
    {
/*         iounmap(devBase); */
        return stat;
    }


    /* connect the primary "done" handler. All Talitos devs use this */
    stat = request_irq(devinst->doneIRQid,
                       t23RMintDoneHandler,
                       0,
                       "t23x-done",
                       devinst);

    if (stat)
    {
        printk("t23xrm: can't connect 'done' interrupt %d\n", devirq);
        return -1;
    }



    ifCtx.dev    = dev;
    ifCtx.devctx = devinst;

    return stat;
}


/**
 * Device shutdown and removed, from RM module removal
 */
static int t23x_remove(struct of_device *ofdev)
{
    T2CoreInstance *devinst;


    devinst = dev_get_drvdata(&ofdev->dev);

    /*
     * Go call the "portable" remove function before releasing
     * OS resources. It will shut off the core-level interrupt,
     * we need to shut off the handlers
     */
    t23RMdevRemove(devinst);

    /* Now disconnect interrupts */
    free_irq(devinst->doneIRQid, devinst);

    /* Unmap the register region, and unregister the driver */
/*    iounmap(devinst->regs); */

    kfree(devinst);

    return 0;
}


static struct of_device_id t23x_match[] = {
	{
		.compatible = "fsl,sec3.1",
	},
	{},
};
MODULE_DEVICE_TABLE(of, t23x_match);

static struct of_platform_driver t23x_driver = {
	.name        = "t32x",
	.match_table = t23x_match,
	.probe       = t23x_probe,
	.remove      = __devexit_p(t23x_remove),
};

static int __init t23x_init(void)
{
	return of_register_platform_driver(&t23x_driver);
}

static void __exit t23x_exit(void)
{
	return of_unregister_platform_driver(&t23x_driver);
}

EXPORT_SYMBOL (xwcRMregisterInterface);
EXPORT_SYMBOL (xwcRMderegisterInterface);
EXPORT_SYMBOL (xwcRMqueueRequest);
EXPORT_SYMBOL (xwcRMcancelRequest);

EXPORT_SYMBOL (xwcMemTranslateLogical);
EXPORT_SYMBOL (xwcMemTranslateUserVirtual);
EXPORT_SYMBOL (xwcMemTranslateKernelVirtual);
EXPORT_SYMBOL (xwcMemReleaseLogical);
EXPORT_SYMBOL (xwcMemReleaseUserVirtual);
EXPORT_SYMBOL (xwcMemReleaseKernelVirtual);


module_init(t23x_init);
module_exit(t23x_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Extensible Crypto Driver " \
                    "- Resource Manager (SEC 2/3)");
MODULE_AUTHOR("Freescale Semiconductor - NMG/STC");
