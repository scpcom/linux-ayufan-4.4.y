/*
 *  drivers/char/comcerto_otp.c
 *
 *  Copyright (C) Mindspeed Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Overview:
 *
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <mtd/mtd-abi.h>

#include <asm/io.h>
#include <mach/otp.h>
#include <asm/uaccess.h>
#include <mach/comcerto-2000/clock.h>

#define otp_readl(r) readl((void*)(r))
#define otp_writel(v,r) writel(v, (void*)(r))


static DEFINE_MUTEX(comcerto_otp_lock);

#if defined (CONFIG_COMCERTO_OTP_WR_EN)
#define NP1	4		/* Number of initial programming pulses */
#define NP2	12		/* Maximum number of additional programming pulses */
static int ops_bit_byte = COMCERTO_OTP_OPS_BIT;	   /* Default is bit wise operation */
int otp_write_en = COMCERTO_OTP_WR_DISABLE; /* By default Write is disabled */
EXPORT_SYMBOL(otp_write_en);
#endif /* CONFIG_COMCERTO_OTP_WR_EN */

/*
 ***********************************************
 * write_protect_unlock()
 *
 * To avoid accidental programming of the OTP
 * memory, this hardware lock has to be unlocked.
 ***********************************************
 */
static inline void write_protect_unlock(void)
{
	otp_writel(0xEBCF0000, COMCERTO_OTP_CONFIG_LOCK_0);  /* config lock0 */
	otp_writel(0xEBCF1111, COMCERTO_OTP_CONFIG_LOCK_1);  /* config lock1 */
	otp_writel(0x0, COMCERTO_OTP_CEB_INPUT);
}

#if defined (CONFIG_COMCERTO_OTP_WR_EN)

int otp_smart_write_sequence(u32 offset, u8 write_data)
{
	unsigned long i;

	/* Drive the address now */
	otp_writel(offset, COMCERTO_OTP_ADDR_INPUT);
	/* Write data to the DATA_IN register */
	otp_writel(write_data, COMCERTO_OTP_DATA_INPUT);

	/* DLE drive  "1" */
	otp_writel(0x1, COMCERTO_OTP_DLE_INPUT);
	/* Wait for at least 20nsec */
	ndelay(20);

	/* WEB drive  "0" */
	otp_writel(0x0, COMCERTO_OTP_WEB_INPUT);
	/* Wait for at least 20nsec */
	ndelay(20);

	/* WEB drive  "1" */
	otp_writel(0x1, COMCERTO_OTP_WEB_INPUT);
	/* Wait for at least 20nsec */
	ndelay(20);

	/* DLE drive  "0" */
	otp_writel(0x0, COMCERTO_OTP_DLE_INPUT);

	/* Write '1' to PGMEN to trigger the whole write and verify operation until PGMEN will be deasserted by HW */
	otp_writel(0x1, COMCERTO_OTP_PGMEN_INPUT);

	/* Wait for PGMEN to go low for 11.2 u sec */
    for (i = 0 ; i < 12 ; i++) {
        if (!(otp_readl(COMCERTO_OTP_PGMEN_INPUT) & 1))
            break;
        udelay(1);
    }

	if (otp_readl(COMCERTO_OTP_PGMEN_INPUT) & 1) {
		printk("Timeout waiting for PGMEN "
				"to be deasserted\n");
	}

	return 0;
}

/*
 ***********************************************
 *   comcerto_otp_write_bits()
 *
 * PARAMETERS:
 *	bit_offset -- Starting bit Address
 *	write_data -- Source Data Buffer
 *	no_bits	   -- Number of bits to program
 ***********************************************
 */
int comcerto_otp_write_bits(loff_t bit_offset, uint8_t *write_data, size_t no_bits)
{
	int i, k;
	u32 pgm2cpump_counter, cpump2web_counter, web_counter, web2cpump_counter, cpump2pgm_counter, dataout_counter;
	u32 read_data;
	struct clk *clk_axi;
	u32 axi_clk;
	
	/* Clock divider configuration, get the AXI clock first
	 * AXI clock will be used for refernce count , as exp bus
         * also have a dependancy with AXI.
	*/
        clk_axi = clk_get(NULL,"axi");

	if (IS_ERR(clk_axi)) {
		pr_err("comcerto_Device_init: Unable to obtain axi clock: %ld\n",PTR_ERR(clk_axi));
	}
        /* Get the AXI clock rate in HZ */
        axi_clk = clk_get_rate(clk_axi)/1000000;

#if defined (CONFIG_COMCERTO_OTP_USER)
	if (otp_write_en == COMCERTO_OTP_WR_DISABLE) {
		pr_err("%s : %d No Write permissions \n", __func__, __LINE__);
		return -EPERM;
	}
#endif /* CONFIG_COMCERTO_OTP_USER */

	if (NULL == write_data)
		return -EINVAL;

	if (no_bits <= 0)
		return -EINVAL;

	if (mutex_lock_interruptible(&comcerto_otp_lock))
		return -EINVAL;

	/* Setting up counters to program */
	pgm2cpump_counter = axi_clk & 0x7FF ; 				/* 1 uSec */
	cpump2web_counter = (axi_clk*3) & 0x7FF ;			/* 3 uSec */
	web_counter = (axi_clk*5) & 0x7FF ;					/* 5 uSec */
	web2cpump_counter = (axi_clk*2) & 0x7FF ;			/* 2 uSec */
	cpump2pgm_counter = axi_clk & 0x7FF ;				/* 1 uSec */
	dataout_counter = ((axi_clk * 7 + 99) / 100) & 0x1FF ;	/* 70 nSec */

	/* program the counters */
	otp_writel(pgm2cpump_counter, COMCERTO_OTP_PGM2CPUMP_COUNTER);
	otp_writel(cpump2web_counter, COMCERTO_OTP_CPUMP2WEB_COUNTER);
	otp_writel(web_counter, COMCERTO_OTP_WEB_COUNTER);
	otp_writel(web2cpump_counter, COMCERTO_OTP_WEB2CPUMP_COUNTER);
	otp_writel(cpump2pgm_counter, COMCERTO_OTP_CPUMP2PGM_COUNTER);
	otp_writel(dataout_counter, COMCERTO_OTP_DATA_OUT_COUNTER);

	write_protect_unlock();

	udelay(1);

	/* rstb drive 0 */
	otp_writel(0x0, COMCERTO_OTP_RSTB_INPUT);
	/* Wait for at least 20nsec */
	ndelay(20);

	/* rstb drive 1 to have pulse  */
	otp_writel(0x1, COMCERTO_OTP_RSTB_INPUT);
	/* Wait for at least 1usec */
	udelay(1);

	for(i = 0 ; i < no_bits; i++) {

		/* Skip bits that are 0 because 0 is the default value */
		if (!write_data[i])
			continue;

		for(k = 0 ; k < NP1-1 ; k++) 
			otp_smart_write_sequence(bit_offset + i, write_data[i]);

		for(k = 0 ; k < NP2+1 ; k++) {
			ndelay(100);
			otp_smart_write_sequence(bit_offset + i, write_data[i]);

			/* Verify Data */
			read_data = otp_readl(COMCERTO_OTP_DATA_OUTPUT);

			/* Adjust bit offset */
			read_data = ((read_data >> ((bit_offset+i) & 0x7)) & 0x1);

			if(read_data)
				break;
		}
		if(!read_data) {
			printk(" Warning : OTP Write/read mismatch, OTP write FAIL !");
			return -1;
		}
	}

	mutex_unlock(&comcerto_otp_lock);

	return 0;
}
EXPORT_SYMBOL(comcerto_otp_write_bits);
#endif /* CONFIG_COMCERTO_OTP_WR_EN */

/*
 ****************************************
 *   comcerto_otp_read()
 *
 * PARAMETERS:
 *	bit_offset -- Starting bit Address
 *	read_data  -- Destination Data Buffer
 *	no_bytes   -- No of Bytes to read
 ****************************************
 */
static inline int comcerto_otp_read(loff_t bit_offset, uint8_t *read_data, size_t no_bytes)
{
	int i = 0;
	u32 read_tmp = 0, dataout_counter;
	struct clk *clk_axi;
	u32 axi_clk;

	/* Clock divider configuration, get the AXI clock first
	 * AXI clock will be used for refernce count , as exp bus
         * also have a dependancy with AXI.
	*/
        clk_axi = clk_get(NULL,"axi");

	if (IS_ERR(clk_axi)) {
		pr_err("comcerto_Device_init: Unable to obtain axi clock: %ld\n",PTR_ERR(clk_axi));
	}
        /* Get the AXI clock rate in HZ */
        axi_clk = clk_get_rate(clk_axi)/1000000;

	if (NULL == read_data)
		return -EINVAL;

	if (no_bytes <= 0)
		return -EINVAL;

	if (mutex_lock_interruptible(&comcerto_otp_lock))
		return -EINVAL;

	dataout_counter = ((axi_clk * 7 + 99) / 100) & 0x1FF ;	/* 70 nSec */

	/* configure the COMCERTO_OTP_DATA_OUT_COUNTER for read operation.
	    70 nsec is needed except for blank check test, in which 1.5 usec is needed.*/
	otp_writel(dataout_counter, COMCERTO_OTP_DATA_OUT_COUNTER);

	write_protect_unlock();
	udelay(1);

	/* rstb drive 0 */
	otp_writel(0x0, COMCERTO_OTP_RSTB_INPUT);
	/* Wait for at least 20nsec */
	ndelay(20);
	/* rstb drive 1 to have pulse  */
	otp_writel(0x1, COMCERTO_OTP_RSTB_INPUT);
	/* Wait for at least 1usec */
	udelay(1);

	/* read_enable drive */
	otp_writel(0x1, COMCERTO_OTP_READEN_INPUT);

	do {
		/* Write the desired address to the ADDR register */
		otp_writel(bit_offset, COMCERTO_OTP_ADDR_INPUT);

		/* Wait for at least 70nsec/1.5usec depends on operation type */
		ndelay(70);

		read_tmp = otp_readl(COMCERTO_OTP_DATA_OUTPUT);
		*(read_data + i++) = read_tmp & 0xFF;

		bit_offset += 8;

	} while (no_bytes--);

	/* reading is done make the read_enable low */
	otp_writel(0x0, COMCERTO_OTP_READEN_INPUT);

	/* lock CEB register, return to standby mode */
	otp_writel(0x1, COMCERTO_OTP_CEB_INPUT);

	mutex_unlock(&comcerto_otp_lock);

	return 0;
}

static int check_otp_range(loff_t offset, size_t size)
{
	if ((size <= 0) || (size > COMCERTO_OTP_SIZE_BYTES))
		return -EPERM;

	if ((offset < 0) || (offset >= COMCERTO_OTP_SIZE_BYTES))
		return -ENXIO;

	if ((offset + size) > COMCERTO_OTP_SIZE_BYTES)
		return -ENXIO;

	return 0;
}

#if defined (CONFIG_COMCERTO_OTP_WR_EN)
/*
 ****************************************
 *   comcerto_otp_write_bytes()
 *
 * PARAMETERS:
 *	byte_offset -- Starting bit Address
 *	write_data  -- Source Data Buffer
 *	no_bits     -- No of Bytes to read
 ****************************************
 */
int comcerto_otp_write_bytes(loff_t byte_offset, uint8_t *write_data, size_t no_bytes)
{
	int count, bit_count;
	unsigned char bit_val;
	int ret_val = 0;

	pr_debug("OTP Write request at byte offset: %lld no of bytes: %zu :\n", byte_offset, no_bytes);

	ret_val = check_otp_range(byte_offset, no_bytes);
	if (ret_val) {
		pr_err("%s : wrong otp range\n", __func__);
		return ret_val;
	}

	for (count = 0; count < no_bytes; count++) {
		for (bit_count = 0; bit_count < 8; bit_count++) {
			bit_val = (write_data[count] & (1 << bit_count)) ? 1 : 0;
			if (bit_val) {
				ret_val = comcerto_otp_write_bits(((byte_offset * 8) + (count * 8) + bit_count), &bit_val, 1);
				if (ret_val)
					return ret_val;
			}
		}
	}

	pr_debug("\nOTP Write request done\n");

	return ret_val;
}
EXPORT_SYMBOL(comcerto_otp_write_bytes);
#endif /* CONFIG_COMCERTO_OTP_WR_EN */

int comcerto_otp_read_bytes(loff_t byte_offset, uint8_t *read_data, size_t no_bytes)
{
	int ret_val = 0;

	pr_debug("OTP read request at byte offset: %lld no of bytes: %zu :\n", byte_offset, no_bytes);

	ret_val = check_otp_range(byte_offset, no_bytes);
	if (ret_val) {
		pr_err("%s : wrong otp range\n", __func__);
		return ret_val;
	}

	ret_val = comcerto_otp_read(byte_offset * 8 , (uint8_t *)read_data, no_bytes);
	if (ret_val) {
		pr_err("%s : %d error reading OTP\n", __func__, __LINE__);
		return ret_val;
	}

	pr_debug("\nOTP Read request done\n");

	return ret_val;
}
EXPORT_SYMBOL(comcerto_otp_read_bytes);

#if defined (CONFIG_COMCERTO_OTP_USER)
#if defined (CONFIG_COMCERTO_OTP_WR_EN)
static long comcerto_otp_ioctl_user(struct file *filptr, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {

	case COMCERTO_OTPIOC_OPS_BIT:
		ops_bit_byte = COMCERTO_OTP_OPS_BIT;
		return 0;

	case COMCERTO_OTPIOC_OPS_BYTE:
		ops_bit_byte = COMCERTO_OTP_OPS_BYTE;
		return 0;

	case COMCERTO_OTPIOC_OPS_MODE:
		if (copy_to_user((void __user *)arg, &ops_bit_byte, sizeof(ops_bit_byte)))
			return -EFAULT;
		return 0;

	case COMCERTO_OTPIOC_WR_ENABLE:
		otp_write_en = COMCERTO_OTP_WR_ENABLE;
		return 0;

	case COMCERTO_OTPIOC_WR_DISABLE:
		otp_write_en = COMCERTO_OTP_WR_DISABLE;
		return 0;

	}

	return 0;
}

static ssize_t comcerto_otp_write_user(struct file *fileptr, const char __user *buff, size_t count, loff_t *pos)
{

	uint8_t *content;
	ssize_t ret_val = 0;

	if (otp_write_en == COMCERTO_OTP_WR_DISABLE) {
		pr_err("%s : %d Writing to OTP memory is disabled \n", __func__, __LINE__);
		return -EPERM;
	}

	if (count == 0)
		return -EPERM;

	if (ops_bit_byte == COMCERTO_OTP_OPS_BYTE) {
		if (count > COMCERTO_OTP_SIZE_BYTES)
			return -EPERM;
	} else {
		if (count > COMCERTO_OTP_SIZE_BITS)
			return -EPERM;
	}

	content = kmalloc(count, GFP_KERNEL);
	if (!content)
		return -ENOMEM;

	ret_val = copy_from_user(content, buff, count);
	if (ret_val)
		goto out;

	if (ops_bit_byte == COMCERTO_OTP_OPS_BYTE) {
		ret_val = comcerto_otp_write_bytes(*pos, (uint8_t *) content, count);
		if (ret_val)
			goto out;
	} else if (ops_bit_byte == COMCERTO_OTP_OPS_BIT) {
		ret_val = comcerto_otp_write_bits(*pos, (uint8_t *) content, count);
		if (ret_val)
			goto out;
	}

out:
	kfree(content);
	return ret_val;

}
#endif /* CONFIG_COMCERTO_OTP_WR_EN */

static ssize_t comcerto_otp_read_user(struct file *fileptr, char __user *buff, size_t count, loff_t *pos)
{
	uint8_t *content;
	ssize_t ret_val = 0;

	if (count == 0 || count > COMCERTO_OTP_SIZE_BYTES)
		return -EPERM;

	content = kmalloc(count, GFP_KERNEL);
	if (!content)
		return -ENOMEM;

	ret_val = comcerto_otp_read_bytes(*pos, (uint8_t *) content, count);
	if (ret_val)
		goto out;

	ret_val = copy_to_user(buff, content, count);

out:
	kfree(content);
	return ret_val;
}

static const struct file_operations comcerto_otp_fops = {
	.owner          = THIS_MODULE,
	.read           = comcerto_otp_read_user,
#if defined (CONFIG_COMCERTO_OTP_WR_EN)
	.write		= comcerto_otp_write_user,
	.unlocked_ioctl = comcerto_otp_ioctl_user,
#endif /* CONFIG_COMCERTO_OTP_WR_EN */
	.llseek         = default_llseek,
};

static struct miscdevice comcerto_otp_misc_device = {
	.minor    = MISC_DYNAMIC_MINOR,
	.name     = "comcerto_otp",
	.fops     = &comcerto_otp_fops,
};
#endif /* CONFIG_COMCERTO_OTP_USER */



static int comcerto_otp_probe(struct platform_device *pdev)
{
#if defined (CONFIG_COMCERTO_OTP_USER)
	misc_register(&comcerto_otp_misc_device);
#endif /* CONFIG_COMCERTO_OTP_USER */

	printk(KERN_NOTICE "comcerto_otp_probe.\n");
	return 0;
}

static int comcerto_otp_remove(struct platform_device *pdev)
{
#if defined (CONFIG_COMCERTO_OTP_USER)
	misc_deregister(&comcerto_otp_misc_device);
#endif /* CONFIG_COMCERTO_OTP_USER */
	printk("comcerto_otp_remove.\n");
	return 0;
}

static struct platform_driver comcerto_otp_driver = {
	.probe        = comcerto_otp_probe,
	.remove       = comcerto_otp_remove,
	.driver       = {
		.owner = THIS_MODULE,
		.name  = "comcerto_otp",
	},
};


static int __init comcerto_otp_init(void)
{
	return platform_driver_register(&comcerto_otp_driver);
}
module_init(comcerto_otp_init);

static void __exit comcerto_otp_exit(void)
{
	platform_driver_unregister(&comcerto_otp_driver);
}
module_exit(comcerto_otp_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Comcerto OTP Driver");
