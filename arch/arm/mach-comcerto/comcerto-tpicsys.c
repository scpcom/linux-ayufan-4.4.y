/*
 *  linux/arch/arm/mach-comcerto/comcerto-tpicsys.c
 *
 *  Copyright (C) 2008 Mindspeed Technologies, Inc.
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

#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/err.h>
#include <linux/module.h>

struct clk *clk_tpi,*clk_csys;

static int tpi_csys_probe(struct platform_device *pdev)
{
	int err=0;
	/* Get the refrence to TPI clk structure */
	clk_tpi = clk_get(NULL,"tpi");

	/* Error Handling , if no TPI clock reference: return error */
	if (IS_ERR(clk_tpi)) {
		pr_err("%s: Unable to obtain  clock: %ld\n",__func__,PTR_ERR(clk_tpi));
		err = PTR_ERR(clk_tpi);
		goto err_tpi_get;
	}

	/* Enable the TPI clock ,required for cortex A9 JTAG */
	err = clk_enable(clk_tpi);
	if (err){
		pr_err("%s: TPI clock failed to enable:\n",__func__);
		goto err_tpi_enable;
	}

	/* Get the CSYS clk structure ,required ofr cortex A9 coresight*/
	clk_csys = clk_get(NULL,"csys");

	/* Error Handling , if no CSYS clock reference: return error 
	 * Disable the TPI clock enabled before. 
	 */
	if (IS_ERR(clk_csys)) {
		pr_err("%s: Unable to obtain  clock: %ld\n",__func__,PTR_ERR(clk_csys));
		err = PTR_ERR(clk_csys);
 		goto err_csys_get;
	}

	/*Enable the  CSYS clock */
	err = clk_enable(clk_csys);
	if (err){
		pr_err("%s: CSYS clock failed to enable:\n",__func__);
		/* Disable the CSYS(A9 coresight clock also */ 
		goto err_csys_enable;
	}

	return 0;

err_csys_enable:
	clk_put(clk_csys);
err_csys_get:
	clk_disable(clk_tpi);
err_tpi_enable:
	clk_put(clk_tpi);
err_tpi_get:
	return err;
}

static int tpi_csys_remove(struct platform_device *pdev)
{

	/* Disable the TPI/CSYS clock */
	clk_disable(clk_tpi);
	clk_put(clk_tpi);

	clk_disable(clk_csys);
	clk_put(clk_csys);

	return 0;
}
	

/* Structure for Device Driver */
static struct platform_driver tpi_csys_platform_driver = {
	.probe = tpi_csys_probe,
	.remove = tpi_csys_remove,
	.driver = {
		.name = "tpi_csys",
	},	
};

static int comcerto_tpi_csys_init(void)
{
	return platform_driver_register(&tpi_csys_platform_driver);
}

static void comcerto_tpi_csys_exit(void)
{
	platform_driver_unregister(&tpi_csys_platform_driver);
}

module_init(comcerto_tpi_csys_init);
module_exit(comcerto_tpi_csys_exit);

MODULE_DESCRIPTION("Comcerto TPI/CSYS");
MODULE_LICENSE("GPL");
