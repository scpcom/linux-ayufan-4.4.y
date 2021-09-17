/******************************************************************************
 *    COPYRIGHT (C) 2013 Czyong. Hisilicon
 *    All rights reserved.
 * ***
 *    Create by Czyong 2013-02-06
 *
******************************************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <asm/setup.h>

#include "spi_ids.h"

#define SPI_DRV_VERSION       "1.22"
/*****************************************************************************/

/*****************************************************************************/
static char *int_to_size(unsigned long long size)
{
	int ix;
	static char buffer[20];
	char *fmt[] = {"%u", "%uK", "%uM", "%uG", "%uT", "%uT"};

	for (ix = 0; (ix < 5) && !(size & 0x3FF) && size; ix++)
		size = (size >> 10);

	sprintf(buffer, fmt[ix], size);
	return buffer;
}
/*****************************************************************************/

struct spi_info *spi_serach_ids(unsigned char ids[8])
{
	struct spi_info *info;
	struct spi_info *fit_info = NULL;

	for (info = spi_info_table; info->name; info++) {
		if (memcmp(info->id, ids, info->id_len))
			continue;

		if ((fit_info == NULL) || (fit_info->id_len < info->id_len))
			fit_info = info;
	}
	return fit_info;
}
/*****************************************************************************/

void spi_search_rw(struct spi_info *spiinfo, struct spi_operation *spiop_rw,
	unsigned int iftype, unsigned int max_dummy, int is_read)
{
	int ix = 0;
	struct spi_operation **spiop, **fitspiop;

	for (fitspiop = spiop = (is_read ? spiinfo->read : spiinfo->write);
		(*spiop) && ix < MAX_SPI_OP; spiop++, ix++) {
		if (((*spiop)->iftype & iftype)
			&& ((*spiop)->dummy <= max_dummy)
			&& (*fitspiop)->iftype < (*spiop)->iftype)
			fitspiop = spiop;
	}
	memcpy(spiop_rw, (*fitspiop), sizeof(struct spi_operation));
}
/*****************************************************************************/
#ifndef CONFIG_MTD_HISFC300
void spi_get_erase(struct spi_info *spiinfo, struct spi_operation *spiop_erase)
{
	int ix;

	spiop_erase->size = 0;
	for (ix = 0; ix < MAX_SPI_OP; ix++) {
		if (spiinfo->erase[ix] == NULL)
			break;
		if (spiinfo->erasesize == spiinfo->erase[ix]->size) {
			memcpy(&spiop_erase[ix], spiinfo->erase[ix],
			sizeof(struct spi_operation));

			break;
		}
	}
}
#else
void spi_get_erase(struct spi_info *spiinfo, struct spi_operation *spiop_erase,
	unsigned int *erasesize)
{
	int ix;

	(*erasesize) = spiinfo->erasesize;
	for (ix = 0; ix < MAX_SPI_OP; ix++) {
		if (spiinfo->erase[ix] == NULL)
			break;

		memcpy(&spiop_erase[ix], spiinfo->erase[ix],
			sizeof(struct spi_operation));

		switch (spiop_erase[ix].size) {
		case SPI_IF_ERASE_SECTOR:
			spiop_erase[ix].size = spiinfo->erasesize;
			break;
		case SPI_IF_ERASE_CHIP:
			spiop_erase[ix].size = spiinfo->chipsize;
			break;
		}

		if ((int)(spiop_erase[ix].size) < _2K) {
			DBG_BUG("block size mistaken spi->erase[%d].size:%s\n",
				ix, int_to_size(spiop_erase[ix].size));
		}

		if (spiop_erase[ix].size < (*erasesize))
			(*erasesize) = spiop_erase[ix].size;
	}
}
#endif
/*****************************************************************************/

struct spi_tag {
	char name[16];

	unsigned char  id[8];
	unsigned int   id_len;

	unsigned long  chipsize;
	unsigned int   erasesize;
	unsigned int   addrcycle;

	struct spi_operation read[MAX_SPI_OP];
	struct spi_operation write[MAX_SPI_OP];
	struct spi_operation erase[MAX_SPI_OP];
};
/*****************************************************************************/

static int __init parse_spi_id(const struct tag *tag)
{
	int ix;
	static struct spi_tag spitag[1];
	struct spi_info *spiinfo = spi_info_table;

	if (tag->hdr.size < ((sizeof(struct tag_header) +
					sizeof(struct spi_tag)) >> 2)) {
		pr_err("%s(%d):tag->hdr.size(%d) too small.\n",
			__func__, __LINE__, tag->hdr.size);
		return 0;
	}
	memset(spiinfo, 0, sizeof(struct spi_info));
	memcpy(spitag, &tag->u, sizeof(struct spi_tag));

	spiinfo->name = spitag->name;

	memcpy(spiinfo->id, spitag->id, 8);
	spiinfo->id_len = spitag->id_len;

	spiinfo->chipsize = spitag->chipsize;
	spiinfo->erasesize = spitag->erasesize;
	spiinfo->addrcycle = spitag->addrcycle;

	for (ix = 0; ix < MAX_SPI_OP; ix++) {
		if (spitag->read[ix].iftype)
			spiinfo->read[ix] = &spitag->read[ix];
	}
	for (ix = 0; ix < MAX_SPI_OP; ix++) {
		if (spitag->write[ix].iftype)
			spiinfo->write[ix] = &spitag->write[ix];
	}
	for (ix = 0; ix < MAX_SPI_OP; ix++) {
		if (spitag->erase[ix].iftype)
			spiinfo->erase[ix] = &spitag->erase[ix];
	}

	pr_notice("SPI TAG: hdr.tag: 0x%08X, hdr.size: %d\n",
		tag->hdr.tag, tag->hdr.size);
	pr_notice("(%dByte): 0x%02X 0x%02X 0x%02X 0x%02X ",
			spitag->id_len, spitag->id[0],
			spitag->id[1], spitag->id[2],
			spitag->id[3]);
	pr_notice("0x%02X 0x%02X 0x%02X 0x%02X\n",
			spitag->id[4], spitag->id[5],
			spitag->id[6], spitag->id[7]);
	pr_notice("Block:%sB ",     int_to_size(spitag->erasesize));
	pr_notice("Chip:%sB ",      int_to_size(spitag->chipsize));
	pr_notice("AddrCycle:%d ",  spitag->addrcycle);
	pr_notice("Name:(%s)",      spitag->name);
	pr_notice("\n");
	for (ix = 0; ix < MAX_SPI_OP; ix++) {
		if (spitag->read[ix].iftype) {
			pr_notice("R %d: ", ix + 1);
			pr_notice("IF Type:0x%02X ",
					spitag->read[ix].iftype);
			pr_notice("CMD:0x%02X ",
					spitag->read[ix].cmd);
			pr_notice("Dummy:%d ",
					spitag->read[ix].dummy);
			if (spitag->read[ix].size == INFINITE)
				pr_notice("Size:-1      ");
			else
				pr_notice("Size:%6sB ",
					int_to_size(spitag->read[ix].size));
			pr_notice("Clock:%dMHz ",
					spitag->read[ix].clock);
			pr_notice("\n");
		}
	}
	for (ix = 0; ix < MAX_SPI_OP; ix++) {
		if (spitag->write[ix].iftype) {
			pr_notice("W %d: ", ix + 1);
			pr_notice("IF Type:0x%02X ",
					spitag->write[ix].iftype);
			pr_notice("CMD:0x%02X ",
					spitag->write[ix].cmd);
			pr_notice("Dummy:%d ",
					spitag->write[ix].dummy);
			pr_notice("Size:%6sB ",
					int_to_size(spitag->write[ix].size));
			pr_notice("Clock:%dMHz ",
					spitag->write[ix].clock);
			pr_notice("\n");
		}
	}
	for (ix = 0; ix < MAX_SPI_OP; ix++) {
		if (spitag->erase[ix].iftype) {
			pr_notice("E %d: ", ix + 1);
			pr_notice("IF Type:0x%02X ",
					spitag->erase[ix].iftype);
			pr_notice("CMD:0x%02X ",
					spitag->erase[ix].cmd);
			pr_notice("Dummy:%d ",
					spitag->erase[ix].dummy);
			pr_notice("Size:0x%02X ",
					spitag->erase[ix].size);
			pr_notice("Clock:%dMHz ",
					spitag->erase[ix].clock);
			pr_notice("\n");
		}
	}

	return 0;
}

/* turn to ascii is "S_ID" */
__tagtable(0x535F4944, parse_spi_id);
/*****************************************************************************/

static int __init spi_ids_init(void)
{
	pr_info("Spi id table Version %s\n", SPI_DRV_VERSION);
	return 0;
}
/*****************************************************************************/

static void __exit spi_ids_exit(void)
{
}
/*****************************************************************************/

module_init(spi_ids_init);
module_exit(spi_ids_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Spi id table");
