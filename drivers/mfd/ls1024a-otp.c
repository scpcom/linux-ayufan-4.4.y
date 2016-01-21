/*
 * Freescale LS1024A OTP access
 *
 * (C) Copyright 2014 Google Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/platform_device.h>

#define DRIVER_NAME "ls1024a-otp"

#define OTP_CONFIG_LOCK_0	0x00
#define OTP_CONFIG_LOCK_1	0x04
#define OTP_CEB_INPUT		0x0C
#define OTP_RSTB_INPUT		0x10
#define OTP_ADDR_INPUT		0x14
#define OTP_READEN_INPUT	0x18
#define OTP_DATA_OUT_COUNTER	0x4C
#define OTP_DATA_OUTPUT		0x50

struct ls1024a_otp {
	struct device *dev;
	void __iomem *reg;
	struct clk *clk;
	spinlock_t lock;
};

/* Taken from repo://barebox/mindspeed/drivers/otp/c2k_otp.c */
static void otp_read(struct ls1024a_otp *otp, u32 offset, u8 *read_data, int size)
{
	int i;
	unsigned long flags;
	u32 read_tmp = 0;
	u32 dataout_counter;
	u32 axi_clk;

	if (NULL == read_data)
		return;

	if (size <= 0)
		return;

	axi_clk = clk_get_rate(otp->clk);
	/* 70 nSec */
	dataout_counter = ((axi_clk * 7 + 99999999) / 100000000) & 0x1FF;

	spin_lock_irqsave(&otp->lock, flags);

	/* configure the OTP_DATA_OUT_COUNTER for read operation.
			70 nsec is needed except for blank check test, in which 1.5 usec is needed.*/
	writel(dataout_counter, otp->reg + OTP_DATA_OUT_COUNTER);

	/* Unlock the write protection. */
	writel(0xEBCF0000, otp->reg + OTP_CONFIG_LOCK_0); /* config lock0 */
	writel(0xEBCF1111, otp->reg + OTP_CONFIG_LOCK_1); /* config lock1 */
	writel(0x0, otp->reg + OTP_CEB_INPUT);

	udelay(1);

	/* rstb drive 0 */
	writel(0x0, otp->reg + OTP_RSTB_INPUT);
	/* Wait for at least 20nsec */
	ndelay(20);
	/* rstb drive 1 to have pulse */
	writel(0x1, otp->reg + OTP_RSTB_INPUT);
	/* Wait for at least 1usec */
	udelay(1);

	/* Write the desired address to the ADDR register */
	writel(offset, otp->reg + OTP_ADDR_INPUT);
	/* read_enable drive */
	writel(0x1, otp->reg + OTP_READEN_INPUT);
	/* Wait for at least 70nsec/1.5usec depends on operation type */
	ndelay(70);

	/* Read First Byte */
	read_tmp = readl(otp->reg + OTP_DATA_OUTPUT);
	*read_data = read_tmp & 0xFF;

	/* For consecutive read */
	for(i = 1 ; i < size ; i++)
	{
		offset = offset + 8;

		/* start reading from data out register */
		writel(offset, otp->reg + OTP_ADDR_INPUT);
		/* Wait for at least 70nsec/1.5usec depends on operation type */
		ndelay(70);

		read_tmp = readl(otp->reg + OTP_DATA_OUTPUT);
		*(read_data + i) = read_tmp & 0xFF;
	}

	/* reading is done make the read_enable low */
	writel(0x0, otp->reg + OTP_READEN_INPUT);

	/* lock CEB register, return to standby mode */
	writel(0x1, otp->reg + OTP_CEB_INPUT);

	spin_unlock_irqrestore(&otp->lock, flags);
}

static int islocked_proc_show(struct seq_file *m, void *v)
{
	struct ls1024a_otp *otp = (struct ls1024a_otp *) m->private;
	char status_buf[1];

	otp_read(otp, 8, status_buf, 1);

	seq_printf(m, "%d\n", (*status_buf & 0x2) == 0x2);
	return 0;
}

static int islocked_proc_open(struct inode *inode, struct file *filp)
{
	struct ls1024a_otp *otp = PDE_DATA(inode);
	return single_open(filp, islocked_proc_show, otp);
}

static struct file_operations islocked_proc_fops = {
	.owner = THIS_MODULE,
	.open = islocked_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int ls1024a_otp_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	struct ls1024a_otp *otp;
	int rc;
	struct proc_dir_entry *otp_dir;
	struct proc_dir_entry *islocked_proc_file;

	otp = devm_kzalloc(dev, sizeof(*otp), GFP_KERNEL);
	if (!otp)
		return -ENOMEM;

	otp->dev = &pdev->dev;

	platform_set_drvdata(pdev, otp);

	spin_lock_init(&otp->lock);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	otp->reg = devm_ioremap_resource(dev, res);
	if (IS_ERR(otp->reg)) {
		rc = PTR_ERR(otp->reg);
		goto err1;
	}

	otp->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(otp->clk)) {
		rc = PTR_ERR(otp->clk);
		goto err2;
	}
	rc = clk_prepare_enable(otp->clk);
	if (rc)
		goto err2;

	otp_dir = proc_mkdir("otp", NULL);
	if (otp_dir == NULL) {
		dev_err(otp->dev, "Unable to create /proc/otp directory\n");
		rc = -ENOMEM;
		goto err3;
	}

	islocked_proc_file = proc_create_data("islocked", 0, otp_dir, &islocked_proc_fops, otp);
	if (islocked_proc_file == NULL) {
		dev_err(otp->dev, "Unable to create /proc/otp/islocked\n");
		rc = -ENOMEM;
		goto err4;
	}
	return 0;

err4:
	remove_proc_entry("otp", NULL);
err3:
	clk_disable_unprepare(otp->clk);
err2:
	devm_iounmap(dev, otp->reg);
err1:
	platform_set_drvdata(pdev, NULL);
	devm_kfree(dev, otp);
	return rc;
}

static const struct of_device_id ls1024a_otp_of_match[] = {
		{
			.compatible	= "fsl,ls1024a-otp",
		},
		{},
};
MODULE_DEVICE_TABLE(of, ls1024a_otp_of_match);
static struct platform_driver ls1024a_otp_driver = {
	.probe		= ls1024a_otp_probe,
	.driver		= {
		.name	= DRIVER_NAME,
		.of_match_table = ls1024a_otp_of_match,
	},
};
module_platform_driver(ls1024a_otp_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Stephen McGruer <smcgruer@google.com>");
MODULE_DESCRIPTION("LS1024A OTP driver");
