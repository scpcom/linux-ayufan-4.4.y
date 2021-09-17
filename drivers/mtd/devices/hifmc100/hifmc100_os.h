/*
 * The Flash Memory Controller v100 Device Driver for hisilicon
 *
 * Copyright (c) 2016 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __HIFMC100_OS_H__
#define __HIFMC100_OS_H__

/*****************************************************************************/
#ifndef CONFIG_SPI_NOR_MAX_CHIP_NUM
#define CONFIG_SPI_NOR_MAX_CHIP_NUM			2
	#warning NOT config CONFIG_SPI_NOR_MAX_CHIP_NUM, \
	used default value, maybe invalid.
#endif

/*****************************************************************************/
#define SPI_NOR_BUFFER_LEN			(0x2000000) /* 32MB */

/*****************************************************************************/
extern int start_up_addr_mode;

/*****************************************************************************/
extern int hifmc_ip_ver_check(void);

/*****************************************************************************/
extern int hifmc100_spi_nor_init(struct hifmc_host *host);

extern int hifmc100_spi_nor_scan(struct hifmc_host *host);

#endif /* End of __HIFMC100_OS_H__ */
