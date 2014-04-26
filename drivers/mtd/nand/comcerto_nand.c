/*
 *  linux/drivers/mtd/nand/comcerto-nand.c
 *
 *  Copyright (C) Mindspeed Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Overview:
 *   This is a device driver for the NAND flash device found on the
 *   Comcerto board which utilizes the Toshiba TC58V64AFT part. This is
 *   a 128Mibit (8MiB x 8 bits) NAND flash device.
 */

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/ratelimit.h>
#include <linux/platform_device.h>
#include <mach/ecc.h>

/*
 * MTD structure for Comcerto board
 */
struct comcerto_nand_info {
	struct mtd_partition	*parts;
	struct mtd_info		*mtd;
};

static void __iomem *ecc_base_addr;

/*
 * Define partitions for flash device
 */

/* Partitions coming from command line*/
static const char *part_probes[] = { "cmdlinepart", NULL };

uint32_t COMCERTO_NAND_ALE = 0x00000200;
uint32_t COMCERTO_NAND_CLE = 0x00000400;

#ifdef CONFIG_NAND_COMCERTO_ECC_24_HW_BCH
/*
 * spare area layout for BCH ECC bytes calculated over 512-Bytes ECC block size
 */
static struct nand_ecclayout comcerto_ecc_info_512_bch = {
	.eccbytes = 42,
	.eccpos = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
		  11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
		  21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
		  31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41},
	.oobfree = {
		{.offset = 43, .length = 13}
	}
};

/*
 * spare area layout for BCH ECC bytes calculated over 1024-Bytes ECC block size
 */
static struct nand_ecclayout comcerto_ecc_info_1024_bch = {
	.eccbytes = 42,
	.eccpos = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
		  11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
		  21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
		  31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41},
	.oobfree = {
		{.offset = 43, .length = 13}
	}
};

#elif CONFIG_NAND_COMCERTO_ECC_8_HW_BCH

/*
 * spare area layout for BCH ECC bytes calculated over 512-Bytes ECC block size
 */
static struct nand_ecclayout comcerto_ecc_info_512_bch = {
	.eccbytes = 14,
	.eccpos = {0, 1, 2, 3, 4, 5, 6,
		   7, 8, 9, 10, 11, 12, 13},
	.oobfree = {
		{.offset = 15, .length = 1}
	}
};

/*
 * spare area layout for BCH ECC bytes calculated over 1024-Bytes ECC block size
 */
static struct nand_ecclayout comcerto_ecc_info_1024_bch = {
	.eccbytes = 14,
	.eccpos = {0, 1, 2, 3, 4, 5, 6,
		   7, 8, 9, 10, 11, 12, 13},
	.oobfree = {
		{.offset = 15, .length = 17}
	}
};

#else

/*
 * spare area layout for Hamming ECC bytes calculated over 512-Bytes ECC block
 * size
 */
static struct nand_ecclayout comcerto_ecc_info_512_hamm = {
	.eccbytes = 4,
	.eccpos = {0, 1, 2, 3},
	.oobfree = {
		{.offset = 5, .length = 12}
	}
};

/*
 * spare area layout for Hamming ECC bytes calculated over 1024-Bytes ECC block
 * size
 */
static struct nand_ecclayout comcerto_ecc_info_1024_hamm = {
	.eccbytes = 4,
	.eccpos = {0, 1, 2, 3},
	.oobfree = {
		{.offset = 5, .length = 28}
	}
};
#endif

static uint8_t bbt_pattern[] = { 'B', 'b', 't', '0' };
static uint8_t mirror_pattern[] = { '1', 't', 'b', 'B' };

static struct nand_bbt_descr bbt_main_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
		| NAND_BBT_8BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 44,
	.len = 4,
	.veroffs = 48,
	.maxblocks = 8,
	.pattern = bbt_pattern,
};

static struct nand_bbt_descr bbt_mirror_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
		| NAND_BBT_8BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 44,
	.len = 4,
	.veroffs = 48,
	.maxblocks = 8,
	.pattern = mirror_pattern,
};

static uint8_t scan_ff_pattern[] = { 0xff };

#ifdef CONFIG_NAND_COMCERTO_ECC_24_HW_BCH
static struct nand_bbt_descr c2000_badblock_pattern = {
	.offs = 42,
	.len = 1,
	.pattern = scan_ff_pattern
};
#elif CONFIG_NAND_COMCERTO_ECC_8_HW_BCH
static struct nand_bbt_descr c2000_badblock_pattern = {
	.offs = 14,
	.len = 1,
	.pattern = scan_ff_pattern
};
#endif

/** Disable/Enable shifting of data to parity module
 *
 * @param[in] en_dis_shift  Enable or disable shift to parity module.
 *
 */
static void comcerto_ecc_shift(uint8_t en_dis_shift)
{
	writel_relaxed(en_dis_shift, ecc_base_addr + ECC_SHIFT_EN_CFG);
}

/** Initializes h/w ECC with proper configuration values.
 *
 * @param[in] mtd	MTD device structure
 * @param[in] mode	Select between BCH and Hamming
 *
 */
static void comcerto_enable_hw_ecc(struct mtd_info *mtd, int mode)
{
	struct nand_chip *nand_device = (struct nand_chip *)(mtd->priv);
	uint32_t ecc_gen_cfg_val = 0;


	/* CS4 will have the option for ECC calculation */
	writel_relaxed(ECC_CS4_SEL, ecc_base_addr + ECC_CS_SEL_CFG);

	/* parity calculation for write, syndrome calculation for read.*/
	(mode == NAND_ECC_WRITE) ? (ecc_gen_cfg_val |= PRTY_CALC) : (ecc_gen_cfg_val &= SYNDROME_CALC);

#if defined (CONFIG_NAND_COMCERTO_ECC_8_HW_BCH) || defined (CONFIG_NAND_COMCERTO_ECC_24_HW_BCH)
	ecc_gen_cfg_val &= BCH_MODE;
	ecc_gen_cfg_val = (ecc_gen_cfg_val & ~(ECC_LVL_MASK)) | (ECC_LVL_VAL << ECC_LVL_SHIFT);
#else
	ecc_gen_cfg_val |= HAMM_MODE;
#endif

	ecc_gen_cfg_val = (ecc_gen_cfg_val & ~(BLK_SIZE_MASK)) | nand_device->ecc.size;

	writel_relaxed(ecc_gen_cfg_val, ecc_base_addr + ECC_GEN_CFG);
	/* Reset parity module and latch configured values */
	writel_relaxed(ECC_INIT, ecc_base_addr + ECC_INIT_CFG);
	comcerto_ecc_shift(ECC_SHIFT_ENABLE);
	return;
}

/** writes ECC bytes generated by the parity module into the flash
 *
 * @param[in] mtd	MTD device structure
 * @param[in] dat	raw data
 * @param[in] ecc_code	buffer for ECC
 *
 */
static int comcerto_calculate_ecc(struct mtd_info *mtd,
				  const uint8_t *dat,
				  uint8_t *ecc_code)
{
	struct nand_chip *nand_device = mtd->priv;
	uint32_t ecc_bytes = nand_device->ecc.bytes;
	uint8_t dummy_var = 0xFF;
	unsigned long timeo = jiffies + 2;

	comcerto_ecc_shift(ECC_SHIFT_DISABLE);

	do {
		if ((readl_relaxed(ecc_base_addr + ECC_IDLE_STAT)) & ECC_IDLE)
			break;
		touch_softlockup_watchdog();
	} while (time_before(jiffies, timeo));

	comcerto_ecc_shift(ECC_SHIFT_ENABLE);

	writel_relaxed(ECC_PARITY_OUT_EN, ecc_base_addr + ECC_PRTY_OUT_SEL_CFG);

	/* Even though we do a dummy write to NAND flash, actual ECC bytes are
	 * written to the ECC location in the flash. */
	for ( ; ecc_bytes; ecc_bytes--)
		writeb(dummy_var, nand_device->IO_ADDR_W);

	comcerto_ecc_shift(ECC_SHIFT_DISABLE);
	writel_relaxed(ECC_PARITY_OUT_DISABLE, ecc_base_addr + ECC_PRTY_OUT_SEL_CFG);

	return 0;
}

/** Checks ECC registers for errors and will correct them, if correctable
 *
 * @param[in] mtd	MTD device structure
 * @param[in] dat	raw data
 * @param[in] read_ecc  ECC read out from flash
 * @param[in] calc_ecc	ECC calculated over the raw data
 *
 */
static int comcerto_correct_ecc(struct mtd_info *mtd, uint8_t *dat,
		uint8_t *read_ecc, uint8_t *calc_ecc)
{
#if !defined (CONFIG_NAND_COMCERTO_ECC_8_HW_BCH) && !defined (CONFIG_NAND_COMCERTO_ECC_24_HW_BCH)
	struct nand_chip *nand_device = mtd->priv;
#else
	uint8_t err_count = 0;
	uint32_t err_corr_data_prev;
#endif
	uint32_t err_corr_data;
	uint16_t mask, index;
	uint32_t temp_nand_ecc_errors[4];
	unsigned long timeo = jiffies + 2;

	 /* Wait for syndrome calculation to complete */
	do {
		if ((readl_relaxed(ecc_base_addr + ECC_IDLE_STAT)) & ECC_IDLE)
			break;
		touch_softlockup_watchdog();
	} while (time_before(jiffies, timeo));

	 /* If no correction is required */
	if (likely(!((readl_relaxed(ecc_base_addr + ECC_POLY_STAT)) & ECC_CORR_REQ))) {
		return 0;
	}
 
	/* Error found! Correction required */
#if defined (CONFIG_NAND_COMCERTO_ECC_8_HW_BCH) || defined (CONFIG_NAND_COMCERTO_ECC_24_HW_BCH)
	/* Initiate correction operation */
	writel_relaxed(ECC_POLY_START, ecc_base_addr + ECC_POLY_START_CFG);

	udelay(25);

	err_corr_data_prev = 0;
	/* Read Correction data status register till header is 0x7FD */
	do {
		err_corr_data_prev = readl_relaxed(ecc_base_addr + ECC_CORR_DATA_STAT);
		if ((err_corr_data_prev >> ECC_BCH_INDEX_SHIFT) == 0x87FD)
			break;

		touch_softlockup_watchdog();
	} while (time_before(jiffies, timeo));

	udelay(25);
	err_corr_data = 0x0;
	/* start reading error locations */
	while (((err_corr_data >> 16) !=  0x87FE)) {
		err_corr_data = readl_relaxed(ecc_base_addr + ECC_CORR_DATA_STAT);
		if ((err_corr_data >> 16) ==  0x87FE)
			break;
		if (err_corr_data == err_corr_data_prev)
			continue;
		err_corr_data_prev = err_corr_data;
		index = (err_corr_data >> 16) & 0x7FF;
		mask = err_corr_data & 0xFFFF;
		*((uint16_t *)(dat + (index * 2))) ^= mask;
		while (mask) {
			if (mask & 1)
				err_count++;
			mask >>= 1;
		}
	}

	if (!((readl_relaxed(ecc_base_addr + ECC_CORR_DONE_STAT)) & ECC_DONE)) {
		temp_nand_ecc_errors[0] += 1 ;
		printk_ratelimited(KERN_WARNING "ECC: uncorrectable error 1 !!!\n");
		return -1;
	}

	/* Check if the block has uncorrectable number of errors */
	if ((readl_relaxed(ecc_base_addr + ECC_CORR_STAT)) & ECC_UNCORR) {
		printk_ratelimited(KERN_WARNING "ECC: uncorrectable error  2 !!!\n");
		temp_nand_ecc_errors[1] += 1 ;
		return -EIO;
	}

	temp_nand_ecc_errors[3] += err_count;

#else		/* Hamming Mode */
		if (readl_relaxed(ecc_base_addr + ECC_POLY_STAT) == ECC_UNCORR_ERR_HAMM) {
			/* 2 or more errors detected and hence cannot
			be corrected */
			return -1; /* uncorrectable */
		} else {  /* 1-bit correctable error */
			err_corr_data = readl_relaxed(ecc_base_addr + ECC_CORR_DATA_STAT);
			index = (err_corr_data >> 16) & 0x1FF;

			if (nand_device->options & NAND_BUSWIDTH_16) {
				mask = 1 << (err_corr_data & 0xF);
				*((uint16_t *)(dat + index)) ^= mask;
			} else {
				mask = 1 << (err_corr_data & 0x7);
				*(dat + index) ^= mask;
			}
			return 1;

		}
#endif
	return err_count;
}

/** writes single page to the NAND device along with the ECC bytes
 *
 * @param[in] mtd	MTD device structure
 * @param[in] chip      nand chip info structure
 * @param[in] buf	data buffer
 *
 */
static void comcerto_nand_write_page_hwecc(struct mtd_info *mtd,
					struct nand_chip *chip,
					const uint8_t *buf)
{
	int i, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	const uint8_t *p = buf;
	uint8_t *oob = chip->oob_poi;

	/* CS4 will have the option for ECC calculation */
	writel_relaxed(ECC_CS4_SEL, ecc_base_addr + ECC_CS_SEL_CFG);

	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {

		chip->ecc.hwctl(mtd, NAND_ECC_WRITE);
		chip->write_buf(mtd, p, eccsize);

		chip->ecc.calculate(mtd, p, oob);
		oob += eccbytes;

		if (chip->ecc.postpad) {
			chip->write_buf(mtd, oob, chip->ecc.postpad);
			oob += chip->ecc.postpad;
		}
	}

	/* Calculate remaining oob bytes */
	i = mtd->oobsize - (oob - chip->oob_poi);
	if (i)
		chip->write_buf(mtd, oob, i);
}

/** reads single page from the NAND device and will read ECC bytes from flash. A
 * function call to comcerto_correct_ecc() will be used to validate the data.
 *
 * @param[in] mtd	MTD device structure
 * @param[in] chip      nand chip info structure
 * @param[in] buf	data buffer
 *
 */
static int comcerto_nand_read_page_hwecc(struct mtd_info *mtd,
		struct nand_chip *chip, uint8_t *buf, int page)
{
	struct nand_chip *nand_device = mtd->priv;
	int i, eccsize = nand_device->ecc.size;
	int eccbytes = nand_device->ecc.bytes;
	int eccsteps = nand_device->ecc.steps;
	uint8_t *p = buf;
	uint8_t *ecc_code = nand_device->buffers->ecccode;
	uint8_t ecc_bytes = nand_device->ecc.bytes;
	uint8_t stat;
	uint8_t *oob = nand_device->oob_poi;

	for (; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {

		chip->ecc.hwctl(mtd, NAND_ECC_READ);
		chip->read_buf(mtd, p, eccsize);
		chip->read_buf(mtd, ecc_code, ecc_bytes);

		stat = chip->ecc.correct(mtd, p, oob, NULL);
		if (stat < 0)
			mtd->ecc_stats.failed++;
		else {
			int idx = eccsteps;
			if (idx >= MTD_ECC_STAT_SUBPAGES) {
				idx = MTD_ECC_STAT_SUBPAGES - 1;
			}

			mtd->ecc_stats.corrected += stat;
			mtd->ecc_subpage_stats.subpage_corrected[idx] += stat;
		}

		comcerto_ecc_shift(ECC_SHIFT_DISABLE);

		if (chip->ecc.postpad) {
			chip->read_buf(mtd, oob, chip->ecc.postpad);
			oob += chip->ecc.postpad;
		}
	}
	/* Calculate remaining oob bytes */
	i = mtd->oobsize - (oob - chip->oob_poi);
	if (i)
		chip->read_buf(mtd, oob, i);

	return 0;
}

/*********************************************************************
 * NAND Hardware functions
 *
 *********************************************************************/

/*
 *	hardware specific access to control-lines
*/
void comcerto_nand_hwcontrol(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	struct nand_chip *chip = mtd->priv;


	if (ctrl & NAND_CTRL_CHANGE) {
		if (ctrl & NAND_NCE)
			comcerto_gpio_set_0(COMCERTO_NAND_CE);
		else
			comcerto_gpio_set_1(COMCERTO_NAND_CE);
	}

	if (cmd == NAND_CMD_NONE)
		return;

	 if (ctrl & NAND_CLE)
		 writeb(cmd, chip->IO_ADDR_W + COMCERTO_NAND_CLE);
	 else if (ctrl & NAND_ALE)
		writeb(cmd, chip->IO_ADDR_W + COMCERTO_NAND_ALE);
	 else
		return;

}

int comcerto_nand_ready(struct mtd_info *mtd)
{
	return comcerto_gpio_read(COMCERTO_NAND_BR) ? 1 : 0;
}

/*********************************************************************
 * NAND Probe
 *
 *********************************************************************/
static int comcerto_nand_probe(struct platform_device *pdev)
{
	struct comcerto_nand_info *info;
	struct mtd_info *mtd;
	struct nand_chip *nand_device;
	int err = 0;

	/* Allocate memory for info structure */
	info = kmalloc(sizeof(struct comcerto_nand_info), GFP_KERNEL);
	if (!info) {
		printk(KERN_ERR
		       "comcerto nand: unable to allocate info structure\n");
		err = -ENOMEM;
		goto out;
	}
	memset(info, 0, sizeof(struct comcerto_nand_info));

	/* Allocate memory for MTD device structure */
	mtd = kmalloc(sizeof(struct mtd_info), GFP_KERNEL);
	if (!mtd) {
		printk(KERN_ERR
		       "comcerto nand: unable to allocate mtd info structure\n");
		err = -ENOMEM;
		goto out_info;
	}
	memset(mtd, 0, sizeof(struct mtd_info));

	/* Link the private data with the MTD structure */
	info->mtd = mtd;
	mtd->owner = THIS_MODULE;

	/* Allocate pointer to nand_device data */
	nand_device = kmalloc(sizeof(struct nand_chip), GFP_KERNEL);
	if (!nand_device) {
		printk(KERN_ERR
		       "comcerto nand: unable to allocate nand chip structure\n");
		err = -ENOMEM;
		goto out_mtd;
	}
	memset(nand_device, 0, sizeof(struct nand_chip));

	/* Link the private data with the MTD structure */
	mtd->priv = nand_device;

	printk(KERN_INFO "pdev->resource->start = %x, pdev->resource->end = %x\n", pdev->resource->start, pdev->resource->end);

	/*Map physical address of nand into virtual space */
	nand_device->IO_ADDR_R = ioremap_nocache(pdev->resource->start, pdev->resource->end - pdev->resource->start + 1);
	if (nand_device->IO_ADDR_R == NULL) {
		printk(KERN_ERR "comcerto nand: cannot map nand memory\n");
		err = -EIO;
		goto out_ior;
	}

	ecc_base_addr = ioremap(COMCERTO_AXI_EXP_ECC_BASE, 0xFFFF);
	if (!ecc_base_addr) {
		printk(KERN_ERR "comcerto nand: cannot map ecc config\n");
		err = -EIO;
		goto out_ior;
	}

	/* This is the same address to read and write */
	nand_device->IO_ADDR_W = nand_device->IO_ADDR_R;

	printk(KERN_INFO "nand_probe: %s base: 0x%08x \n", pdev->name, (resource_size_t) nand_device->IO_ADDR_R);

	/* Set address of hardware control function */
	nand_device->cmd_ctrl = comcerto_nand_hwcontrol;
	nand_device->dev_ready = comcerto_nand_ready;

	/* 20 us command delay time */
	nand_device->chip_delay = 20;

	nand_device->ecc.mode = NAND_ECC_HW_SYNDROME;
//	nand_device->ecc.mode = NAND_ECC_SOFT_BCH;

#if defined(CONFIG_C2K_ASIC) && defined(CONFIG_NAND_TYPE_SLC)
	nand_device->options = NAND_BUSWIDTH_16;
#else
	nand_device->options = 0;
#endif

	/* Scan to find existence of the device */
	if (nand_scan_ident(mtd, 1, NULL)) {
		err = -ENXIO;
		goto out_nand;
	}

#if 1
	if (nand_device->ecc.mode == NAND_ECC_HW_SYNDROME) {
		nand_device->ecc.hwctl = comcerto_enable_hw_ecc;
		nand_device->ecc.write_page = comcerto_nand_write_page_hwecc;
		nand_device->ecc.read_page = comcerto_nand_read_page_hwecc;
		nand_device->ecc.calculate = comcerto_calculate_ecc;
		nand_device->ecc.correct = comcerto_correct_ecc;
		printk("hw_syndrome correction %d.\n", mtd->writesize);

		switch (mtd->writesize) {
		case 512:
			nand_device->ecc.size = mtd->writesize;
#if defined (CONFIG_NAND_COMCERTO_ECC_24_HW_BCH)
			nand_device->ecc.layout = &comcerto_ecc_info_512_bch;
			nand_device->ecc.bytes = 42;
			nand_device->ecc.prepad = 0;
			nand_device->ecc.postpad = 14;
#elif defined (CONFIG_NAND_COMCERTO_ECC_8_HW_BCH)
			nand_device->ecc.layout = &comcerto_ecc_info_512_bch;
			nand_device->ecc.bytes = 14;
			nand_device->ecc.prepad = 0;
			nand_device->ecc.postpad = 2;
#else
			nand_device->ecc.layout = &comcerto_ecc_info_512_hamm;
			nand_device->ecc.bytes = 4;
			nand_device->ecc.prepad = 0;
			nand_device->ecc.postpad = 2;
#endif
			break;
		case 1024:
			nand_device->ecc.size = mtd->writesize;
#ifdef CONFIG_NAND_COMCERTO_ECC_24_HW_BCH
			nand_device->ecc.layout = &comcerto_ecc_info_1024_bch;
			nand_device->ecc.bytes = 42;
			nand_device->ecc.prepad = 0;
			nand_device->ecc.postpad = 14;
#elif CONFIG_NAND_COMCERTO_ECC_8_HW_BCH
			nand_device->ecc.layout = &comcerto_ecc_info_1024_bch;
			nand_device->ecc.bytes = 14;
			nand_device->ecc.prepad = 0;
			nand_device->ecc.postpad = 18;
#else
			nand_device->ecc.layout = &comcerto_ecc_info_1024_hamm;
			nand_device->ecc.bytes = 4;
			nand_device->ecc.prepad = 0;
			nand_device->ecc.postpad = 18;
#endif
			break;
		default:
			printk(KERN_ERR "Using default values for hw ecc\n");
			nand_device->ecc.size =  1024;
#ifdef CONFIG_NAND_COMCERTO_ECC_24_HW_BCH
			nand_device->ecc.layout = &comcerto_ecc_info_1024_bch;
			nand_device->ecc.bytes = 42;
			nand_device->ecc.prepad = 0;
			nand_device->ecc.postpad = 14;
#elif CONFIG_NAND_COMCERTO_ECC_8_HW_BCH
			nand_device->ecc.layout = &comcerto_ecc_info_1024_bch;
			nand_device->ecc.bytes = 14;
			nand_device->ecc.prepad = 0;
			nand_device->ecc.postpad = 18;
#else
			nand_device->ecc.layout = &comcerto_ecc_info_1024_hamm;
			nand_device->ecc.bytes = 4;
			nand_device->ecc.prepad = 0;
			nand_device->ecc.postpad = 18;
#endif
			break;
		}
	nand_device->ecc.steps = mtd->writesize / nand_device->ecc.size;
	if(nand_device->ecc.steps * nand_device->ecc.size != mtd->writesize) {
		printk(KERN_ERR "Invalid ecc parameters\n");
		BUG();
	}
	nand_device->ecc.total = nand_device->ecc.steps * nand_device->ecc.bytes;


	nand_device->bbt_td = &bbt_main_descr;
	nand_device->bbt_md = &bbt_mirror_descr;
	nand_device->badblock_pattern = &c2000_badblock_pattern;
	nand_device->bbt_options |= NAND_BBT_USE_FLASH;

	} else {
		printk("using soft ecc.\n");
		nand_device->ecc.mode = NAND_ECC_SOFT_BCH;
	}

#endif

	nand_device->options |= NAND_NO_SUBPAGE_WRITE;

	if(nand_scan_tail(mtd)) {
		printk(KERN_ERR "nand_scan_tail returned error\n");
		err = -ENXIO;
		goto out_ior;
	}

	/*Name of the mtd device */
	mtd->name = dev_name(&pdev->dev);

	/* Link the info stucture with platform_device */
	platform_set_drvdata(pdev, info);

	err = mtd_device_parse_register(mtd, part_probes, NULL, NULL, 4);

	if (err) {
                printk(KERN_ERR "Could not parse partitions\n");
		return err;
	}


	goto out;

      out_ior:
	iounmap(nand_device->IO_ADDR_R);
	iounmap(ecc_base_addr);
      out_nand:
	kfree(nand_device);
      out_mtd:
	kfree(mtd);
      out_info:
	kfree(info);
      out:
	return err;
}

/*********************************************************************
 * NAND Remove
 *
 *********************************************************************/
static int comcerto_nand_remove(struct platform_device *pdev)
{
	struct comcerto_nand_info *info =
	    (struct comcerto_nand_info *)platform_get_drvdata(pdev);
	struct mtd_info *mtd = info->mtd;
	struct nand_chip *nand_device = (struct nand_chip *)mtd->priv;

	platform_set_drvdata(pdev, NULL);

	/* Release resources, unregister device */
	nand_release(info->mtd);

	/*Deregister virtual address */
	iounmap(nand_device->IO_ADDR_R);
	iounmap(ecc_base_addr);

	kfree(nand_device);

	/* Free the MTD device structure */
	kfree(mtd);

	kfree(info);

	return 0;
}

/*********************************************************************
 * Driver Registration
 *
 *********************************************************************/

static struct platform_driver comcerto_nand_driver = {
	.probe = comcerto_nand_probe,
	.remove = __devexit_p(comcerto_nand_remove),
	.driver = {
		   .name = "comcertonand",
		   },
};

int __init comcerto_nand_init(void)
{
	return platform_driver_register(&comcerto_nand_driver);
}

static void __exit comcerto_nand_exit(void)
{
	platform_driver_unregister(&comcerto_nand_driver);
}

module_init(comcerto_nand_init);
module_exit(comcerto_nand_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Board-specific glue layer for NAND flash on Comcerto board");
