#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/module.h>
#include <linux/mtd/nand.h>
#include <linux/sizes.h>

#define LP_OPTIONS NAND_SAMSUNG_LP_OPTIONS
#define LP_OPTIONS16 (LP_OPTIONS | NAND_BUSWIDTH_16)

#define SP_OPTIONS NAND_NEED_READRDY
#define SP_OPTIONS16 (SP_OPTIONS | NAND_BUSWIDTH_16)

struct nand_flash_dev nand_flash_ids[] = {
	 
	{"TC58NVG2S0F 4G 3.3V 8-bit",
		{ .id = {0x98, 0xdc, 0x90, 0x26, 0x76, 0x15, 0x01, 0x08} },
		  SZ_4K, SZ_512, SZ_256K, 0, 8, 224},
	{"TC58NVG3S0F 8G 3.3V 8-bit",
		{ .id = {0x98, 0xd3, 0x90, 0x26, 0x76, 0x15, 0x02, 0x08} },
		  SZ_4K, SZ_1K, SZ_256K, 0, 8, 232},
	{"TC58NVG5D2 32G 3.3V 8-bit",
		{ .id = {0x98, 0xd7, 0x94, 0x32, 0x76, 0x56, 0x09, 0x00} },
		  SZ_8K, SZ_4K, SZ_1M, 0, 8, 640},
	{"TC58NVG6D2 64G 3.3V 8-bit",
		{ .id = {0x98, 0xde, 0x94, 0x82, 0x76, 0x56, 0x04, 0x20} },
		  SZ_8K, SZ_8K, SZ_2M, 0, 8, 640},
#if defined(CONFIG_SYNO_LSP_HI3536)
#ifdef CONFIG_MTD_NAND_MUSEUM_IDS
	LEGACY_ID_NAND("NAND 1MiB 5V 8-bit",	0x6E, 1, SZ_4K, SP_OPTIONS),
	LEGACY_ID_NAND("NAND 2MiB 5V 8-bit",	0x64, 2, SZ_4K, SP_OPTIONS),
	LEGACY_ID_NAND("NAND 4MiB 5V 8-bit",	0x6B, 4, SZ_8K, SP_OPTIONS),
	LEGACY_ID_NAND("NAND 1MiB 3,3V 8-bit",	0xE8, 1, SZ_4K, SP_OPTIONS),
	LEGACY_ID_NAND("NAND 1MiB 3,3V 8-bit",	0xEC, 1, SZ_4K, SP_OPTIONS),
	LEGACY_ID_NAND("NAND 2MiB 3,3V 8-bit",	0xEA, 2, SZ_4K, SP_OPTIONS),
	LEGACY_ID_NAND("NAND 4MiB 3,3V 8-bit",	0xD5, 4, SZ_8K, SP_OPTIONS),
	LEGACY_ID_NAND("NAND 4MiB 3,3V 8-bit",	0xE3, 4, SZ_8K, SP_OPTIONS),
	LEGACY_ID_NAND("NAND 4MiB 3,3V 8-bit",	0xE5, 4, SZ_8K, SP_OPTIONS),
	LEGACY_ID_NAND("NAND 8MiB 3,3V 8-bit",	0xD6, 8, SZ_8K, SP_OPTIONS),
	LEGACY_ID_NAND("NAND 8MiB 1,8V 8-bit",	0x39, 8, SZ_8K, SP_OPTIONS),
	LEGACY_ID_NAND("NAND 8MiB 3,3V 8-bit",	0xE6, 8, SZ_8K, SP_OPTIONS),
	LEGACY_ID_NAND("NAND 8MiB 1,8V 16-bit",	0x49, 8, SZ_8K, SP_OPTIONS16),
	LEGACY_ID_NAND("NAND 8MiB 3,3V 16-bit",	0x59, 8, SZ_8K, SP_OPTIONS16),
#endif
#else  

	LEGACY_ID_NAND("NAND 4MiB 5V 8-bit",   0x6B, 4, SZ_8K, SP_OPTIONS),
	LEGACY_ID_NAND("NAND 4MiB 3,3V 8-bit", 0xE3, 4, SZ_8K, SP_OPTIONS),
	LEGACY_ID_NAND("NAND 4MiB 3,3V 8-bit", 0xE5, 4, SZ_8K, SP_OPTIONS),
	LEGACY_ID_NAND("NAND 8MiB 3,3V 8-bit", 0xD6, 8, SZ_8K, SP_OPTIONS),
	LEGACY_ID_NAND("NAND 8MiB 3,3V 8-bit", 0xE6, 8, SZ_8K, SP_OPTIONS),

#endif  
	LEGACY_ID_NAND("NAND 16MiB 1,8V 8-bit",  0x33, 16, SZ_16K, SP_OPTIONS),
	LEGACY_ID_NAND("NAND 16MiB 3,3V 8-bit",  0x73, 16, SZ_16K, SP_OPTIONS),
	LEGACY_ID_NAND("NAND 16MiB 1,8V 16-bit", 0x43, 16, SZ_16K, SP_OPTIONS16),
	LEGACY_ID_NAND("NAND 16MiB 3,3V 16-bit", 0x53, 16, SZ_16K, SP_OPTIONS16),

	LEGACY_ID_NAND("NAND 32MiB 1,8V 8-bit",  0x35, 32, SZ_16K, SP_OPTIONS),
	LEGACY_ID_NAND("NAND 32MiB 3,3V 8-bit",  0x75, 32, SZ_16K, SP_OPTIONS),
	LEGACY_ID_NAND("NAND 32MiB 1,8V 16-bit", 0x45, 32, SZ_16K, SP_OPTIONS16),
	LEGACY_ID_NAND("NAND 32MiB 3,3V 16-bit", 0x55, 32, SZ_16K, SP_OPTIONS16),

	LEGACY_ID_NAND("NAND 64MiB 1,8V 8-bit",  0x36, 64, SZ_16K, SP_OPTIONS),
	LEGACY_ID_NAND("NAND 64MiB 3,3V 8-bit",  0x76, 64, SZ_16K, SP_OPTIONS),
	LEGACY_ID_NAND("NAND 64MiB 1,8V 16-bit", 0x46, 64, SZ_16K, SP_OPTIONS16),
	LEGACY_ID_NAND("NAND 64MiB 3,3V 16-bit", 0x56, 64, SZ_16K, SP_OPTIONS16),

	LEGACY_ID_NAND("NAND 128MiB 1,8V 8-bit",  0x78, 128, SZ_16K, SP_OPTIONS),
	LEGACY_ID_NAND("NAND 128MiB 1,8V 8-bit",  0x39, 128, SZ_16K, SP_OPTIONS),
	LEGACY_ID_NAND("NAND 128MiB 3,3V 8-bit",  0x79, 128, SZ_16K, SP_OPTIONS),
	LEGACY_ID_NAND("NAND 128MiB 1,8V 16-bit", 0x72, 128, SZ_16K, SP_OPTIONS16),
	LEGACY_ID_NAND("NAND 128MiB 1,8V 16-bit", 0x49, 128, SZ_16K, SP_OPTIONS16),
	LEGACY_ID_NAND("NAND 128MiB 3,3V 16-bit", 0x74, 128, SZ_16K, SP_OPTIONS16),
	LEGACY_ID_NAND("NAND 128MiB 3,3V 16-bit", 0x59, 128, SZ_16K, SP_OPTIONS16),

	LEGACY_ID_NAND("NAND 256MiB 3,3V 8-bit", 0x71, 256, SZ_16K, SP_OPTIONS),

	EXTENDED_ID_NAND("NAND 64MiB 1,8V 8-bit",  0xA2,  64, LP_OPTIONS),
	EXTENDED_ID_NAND("NAND 64MiB 1,8V 8-bit",  0xA0,  64, LP_OPTIONS),
	EXTENDED_ID_NAND("NAND 64MiB 3,3V 8-bit",  0xF2,  64, LP_OPTIONS),
	EXTENDED_ID_NAND("NAND 64MiB 3,3V 8-bit",  0xD0,  64, LP_OPTIONS),
	EXTENDED_ID_NAND("NAND 64MiB 3,3V 8-bit",  0xF0,  64, LP_OPTIONS),
	EXTENDED_ID_NAND("NAND 64MiB 1,8V 16-bit", 0xB2,  64, LP_OPTIONS16),
	EXTENDED_ID_NAND("NAND 64MiB 1,8V 16-bit", 0xB0,  64, LP_OPTIONS16),
	EXTENDED_ID_NAND("NAND 64MiB 3,3V 16-bit", 0xC2,  64, LP_OPTIONS16),
	EXTENDED_ID_NAND("NAND 64MiB 3,3V 16-bit", 0xC0,  64, LP_OPTIONS16),

	EXTENDED_ID_NAND("NAND 128MiB 1,8V 8-bit",  0xA1, 128, LP_OPTIONS),
	EXTENDED_ID_NAND("NAND 128MiB 3,3V 8-bit",  0xF1, 128, LP_OPTIONS),
	EXTENDED_ID_NAND("NAND 128MiB 3,3V 8-bit",  0xD1, 128, LP_OPTIONS),
	EXTENDED_ID_NAND("NAND 128MiB 1,8V 16-bit", 0xB1, 128, LP_OPTIONS16),
	EXTENDED_ID_NAND("NAND 128MiB 3,3V 16-bit", 0xC1, 128, LP_OPTIONS16),
	EXTENDED_ID_NAND("NAND 128MiB 1,8V 16-bit", 0xAD, 128, LP_OPTIONS16),

	EXTENDED_ID_NAND("NAND 256MiB 1,8V 8-bit",  0xAA, 256, LP_OPTIONS),
	EXTENDED_ID_NAND("NAND 256MiB 3,3V 8-bit",  0xDA, 256, LP_OPTIONS),
	EXTENDED_ID_NAND("NAND 256MiB 1,8V 16-bit", 0xBA, 256, LP_OPTIONS16),
	EXTENDED_ID_NAND("NAND 256MiB 3,3V 16-bit", 0xCA, 256, LP_OPTIONS16),

	EXTENDED_ID_NAND("NAND 512MiB 1,8V 8-bit",  0xAC, 512, LP_OPTIONS),
	EXTENDED_ID_NAND("NAND 512MiB 3,3V 8-bit",  0xDC, 512, LP_OPTIONS),
	EXTENDED_ID_NAND("NAND 512MiB 1,8V 16-bit", 0xBC, 512, LP_OPTIONS16),
	EXTENDED_ID_NAND("NAND 512MiB 3,3V 16-bit", 0xCC, 512, LP_OPTIONS16),

	EXTENDED_ID_NAND("NAND 1GiB 1,8V 8-bit",  0xA3, 1024, LP_OPTIONS),
	EXTENDED_ID_NAND("NAND 1GiB 3,3V 8-bit",  0xD3, 1024, LP_OPTIONS),
#if defined(MY_ABC_HERE)
	{"NAND 1GiB 3,3V 8-bit", { { .dev_id = 0x38 } },
		  4096, 1024, 524288, LP_OPTIONS},
#endif  
	EXTENDED_ID_NAND("NAND 1GiB 1,8V 16-bit", 0xB3, 1024, LP_OPTIONS16),
	EXTENDED_ID_NAND("NAND 1GiB 3,3V 16-bit", 0xC3, 1024, LP_OPTIONS16),

	EXTENDED_ID_NAND("NAND 2GiB 1,8V 8-bit",  0xA5, 2048, LP_OPTIONS),
	EXTENDED_ID_NAND("NAND 2GiB 3,3V 8-bit",  0xD5, 2048, LP_OPTIONS),
	EXTENDED_ID_NAND("NAND 2GiB 1,8V 16-bit", 0xB5, 2048, LP_OPTIONS16),
	EXTENDED_ID_NAND("NAND 2GiB 3,3V 16-bit", 0xC5, 2048, LP_OPTIONS16),

	EXTENDED_ID_NAND("NAND 4GiB 1,8V 8-bit",  0xA7, 4096, LP_OPTIONS),
#if defined(MY_ABC_HERE) && defined(CONFIG_MTD_NAND_NFC_MLC_SUPPORT)
	 
	{"NAND 4GiB 3,3V 8-bit", { { .dev_id = 0xD7 } },
		4096, 4096, 524288, LP_OPTIONS},
	 
	{"NAND 8GiB 3,3V 8-bit", { { .dev_id = 0x88 } },
		8192, 8192, 2097152, LP_OPTIONS},
#else
	EXTENDED_ID_NAND("NAND 4GiB 3,3V 8-bit",  0xD7, 4096, LP_OPTIONS),
#endif
	EXTENDED_ID_NAND("NAND 4GiB 1,8V 16-bit", 0xB7, 4096, LP_OPTIONS16),
	EXTENDED_ID_NAND("NAND 4GiB 3,3V 16-bit", 0xC7, 4096, LP_OPTIONS16),

	EXTENDED_ID_NAND("NAND 8GiB 1,8V 8-bit",  0xAE, 8192, LP_OPTIONS),
	EXTENDED_ID_NAND("NAND 8GiB 3,3V 8-bit",  0xDE, 8192, LP_OPTIONS),
	EXTENDED_ID_NAND("NAND 8GiB 1,8V 16-bit", 0xBE, 8192, LP_OPTIONS16),
	EXTENDED_ID_NAND("NAND 8GiB 3,3V 16-bit", 0xCE, 8192, LP_OPTIONS16),

	EXTENDED_ID_NAND("NAND 16GiB 1,8V 8-bit",  0x1A, 16384, LP_OPTIONS),
	EXTENDED_ID_NAND("NAND 16GiB 3,3V 8-bit",  0x3A, 16384, LP_OPTIONS),
	EXTENDED_ID_NAND("NAND 16GiB 1,8V 16-bit", 0x2A, 16384, LP_OPTIONS16),
	EXTENDED_ID_NAND("NAND 16GiB 3,3V 16-bit", 0x4A, 16384, LP_OPTIONS16),

	EXTENDED_ID_NAND("NAND 32GiB 1,8V 8-bit",  0x1C, 32768, LP_OPTIONS),
	EXTENDED_ID_NAND("NAND 32GiB 3,3V 8-bit",  0x3C, 32768, LP_OPTIONS),
	EXTENDED_ID_NAND("NAND 32GiB 1,8V 16-bit", 0x2C, 32768, LP_OPTIONS16),
	EXTENDED_ID_NAND("NAND 32GiB 3,3V 16-bit", 0x4C, 32768, LP_OPTIONS16),

	EXTENDED_ID_NAND("NAND 64GiB 1,8V 8-bit",  0x1E, 65536, LP_OPTIONS),
	EXTENDED_ID_NAND("NAND 64GiB 3,3V 8-bit",  0x3E, 65536, LP_OPTIONS),
	EXTENDED_ID_NAND("NAND 64GiB 1,8V 16-bit", 0x2E, 65536, LP_OPTIONS16),
	EXTENDED_ID_NAND("NAND 64GiB 3,3V 16-bit", 0x4E, 65536, LP_OPTIONS16),

#if defined(CONFIG_SYNO_LSP_HI3536)
	 
	{"AND 128MiB 3,3V 8-bit", {.id = {0x01} }, 2048, 128, 0x4000,
		NAND_IS_AND | NAND_NO_AUTOINCR | NAND_NEED_READRDY
		| NAND_4PAGE_ARRAY | BBT_AUTO_REFRESH},

	{NULL,}
#else  
	{NULL}
#endif  
};

struct nand_manufacturers nand_manuf_ids[] = {
#if defined(CONFIG_SYNO_LSP_HI3536)
	{NAND_MFR_TOSHIBA,	"Toshiba"},
	{NAND_MFR_SAMSUNG,	"Samsung"},
	{NAND_MFR_FUJITSU,	"Fujitsu"},
	{NAND_MFR_NATIONAL,	"National"},
	{NAND_MFR_RENESAS,	"Renesas"},
	{NAND_MFR_STMICRO,	"ST Micro"},
	{NAND_MFR_HYNIX,	"Hynix"},
	{NAND_MFR_MICRON,	"Micron"},
	{NAND_MFR_AMD,		"AMD/Spansion"},
	{NAND_MFR_MACRONIX,	"Macronix"},
	{NAND_MFR_GD,		"GD"},
	{NAND_MFR_EON,		"Eon"},
	{NAND_MFR_ESMT,		"ESMT"},
	{NAND_MFR_WINBOND,	"Winbond"},
	{NAND_MFR_ATO,		"ATO"},
	{0x0,			"Unknown"}
#else  
	{NAND_MFR_TOSHIBA, "Toshiba"},
	{NAND_MFR_SAMSUNG, "Samsung"},
	{NAND_MFR_FUJITSU, "Fujitsu"},
	{NAND_MFR_NATIONAL, "National"},
	{NAND_MFR_RENESAS, "Renesas"},
	{NAND_MFR_STMICRO, "ST Micro"},
	{NAND_MFR_HYNIX, "Hynix"},
	{NAND_MFR_MICRON, "Micron"},
	{NAND_MFR_AMD, "AMD/Spansion"},
	{NAND_MFR_MACRONIX, "Macronix"},
	{NAND_MFR_EON, "Eon"},
	{0x0, "Unknown"}
#endif  
};

EXPORT_SYMBOL(nand_manuf_ids);
EXPORT_SYMBOL(nand_flash_ids);

#if defined (MY_DEF_HERE)
 
struct nand_timing_spec nand_onfi_timing_specs[] = {
	 
	[0] = {
		.tCLS	= 50,
		.tCS	= 70,
		.tALS	= 50,
		.tDS	= 40,
		.tWP	= 50,
		.tCLH	= 20,
		.tCH	= 20,
		.tALH	= 20,
		.tDH	= 20,
		.tWB	= 200,
		.tWH	= 30,
		.tWC	= 100,
		.tRP	= 50,
		.tREH	= 30,
		.tRC	= 100,
		.tREA	= 40,
		.tRHOH	= 0,
		.tCEA	= 100,
		.tCOH	= 0,
		.tCHZ	= 100,
	},

	[1] = {
		.tCLS	= 25,
		.tCS	= 35,
		.tALS	= 25,
		.tDS	= 20,
		.tWP	= 25,
		.tCLH	= 10,
		.tCH	= 10,
		.tALH	= 10,
		.tDH	= 10,
		.tWB	= 100,
		.tWH	= 15,
		.tWC	= 45,
		.tRP	= 25,
		.tREH	= 15,
		.tRC	= 50,
		.tREA	= 30,
		.tRHOH	= 15,
		.tCEA	= 45,
		.tCOH	= 15,
		.tCHZ	= 50,
	},

	[2] = {
		.tCLS	= 15,
		.tCS	= 25,
		.tALS	= 15,
		.tDS	= 15,
		.tWP	= 17,
		.tCLH	= 10,
		.tCH	= 10,
		.tALH	= 10,
		.tDH	= 5,
		.tWB	= 100,
		.tWH	= 15,
		.tWC	= 35,
		.tRP	= 17,
		.tREH	= 16,
		.tRC	= 35,
		.tREA	= 25,
		.tRHOH	= 15,
		.tCEA	= 30,
		.tCOH	= 15,
		.tCHZ	= 50,
	},

	[3] = {
		.tCLS	= 10,
		.tCS	= 25,
		.tALS	= 10,
		.tDS	= 10,
		.tWP	= 15,
		.tCLH	= 5,
		.tCH	= 5,
		.tALH	= 5,
		.tDH	= 5,
		.tWB	= 100,
		.tWH	= 10,
		.tWC	= 30,
		.tRP	= 15,
		.tREH	= 10,
		.tRC	= 30,
		.tREA	= 20,
		.tRHOH	= 15,
		.tCEA	= 25,
		.tCOH	= 15,
		.tCHZ	= 50,
	},

	[4] = {
		.tCLS	= 10,
		.tCS	= 20,
		.tALS	= 10,
		.tDS	= 10,
		.tWP	= 12,
		.tCLH	= 5,
		.tCH	= 5,
		.tALH	= 5,
		.tDH	= 5,
		.tWB	= 100,
		.tWH	= 10,
		.tWC	= 25,
		.tRP	= 12,
		.tREH	= 10,
		.tRC	= 25,
		.tREA	= 20,
		.tRHOH	= 15,
		.tCEA	= 25,
		.tCOH	= 15,
		.tCHZ	= 30,
	},

	[5] = {
		.tCLS	= 10,
		.tCS	= 15,
		.tALS	= 10,
		.tDS	= 7,
		.tWP	= 10,
		.tCLH	= 5,
		.tCH	= 5,
		.tALH	= 5,
		.tDH	= 5,
		.tWB	= 100,
		.tWH	= 7,
		.tWC	= 20,
		.tRP	= 10,
		.tREH	= 7,
		.tRC	= 20,
		.tREA	= 16,
		.tRHOH	= 15,
		.tCEA	= 25,
		.tCOH	= 15,
		.tCHZ	= 30,
	}
};
EXPORT_SYMBOL(nand_onfi_timing_specs);

static int nand_decode_id_2(struct mtd_info *mtd,
			    struct nand_chip *chip,
			    struct nand_flash_dev *type,
			    uint8_t *id, int id_len)
{
	mtd->writesize = type->pagesize;
	mtd->oobsize = type->pagesize / 32;
	chip->chipsize = ((uint64_t)type->chipsize) << 20;

	if (id[0] == NAND_MFR_AMD)
		mtd->erasesize = 512 * 1024;
	else
		mtd->erasesize = type->erasesize;

	chip->options |= type->options;
	chip->options |= NAND_NO_AUTOINCR;
	if (mtd->writesize > 512)
		chip->options |= NAND_NEED_READRDY;

	chip->cellinfo = 0;
	chip->planes_per_chip = 1;
	chip->planes_per_chip = 1;
	chip->luns_per_chip = 1;

	return 0;
}

static int nand_decode_id_ext(struct mtd_info *mtd,
			      struct nand_chip *chip,
			      struct nand_flash_dev *type,
			      uint8_t *id, int id_len) {
	uint8_t data;

	if (id_len < 3 || id_len > 5) {
		pr_err("[MTD][NAND]: %s: invalid ID length [%d]\n",
		       __func__, id_len);
		return 1;
	}

	if (id[0] == NAND_MFR_HYNIX && id_len == 5 && id[4] == 0 &&
	    (id[1] == 0xDA || id[1] == 0xCA)) {
		 
		chip->planes_per_chip = 2;
		chip->chipsize = (128 * 1024 * 1024) * chip->planes_per_chip;
	} else if (id[0] == NAND_MFR_HYNIX && id_len == 5 &&
		   id[1] == 0xD5 && id[4] == 0x44) {
		 
		chip->planes_per_chip = 2;
		chip->chipsize = (1024UL * 1024 * 1024) * chip->planes_per_chip;
	} else if (id_len == 5) {
		 
		data = (id[4] >> 2) & 0x3;
		chip->planes_per_chip = 1 << data;

		if (id[0] != NAND_MFR_TOSHIBA) {
			 
			data = (id[4] >> 4) & 0x7;
			chip->chipsize = (8 * 1024 * 1024) << data;
			chip->chipsize *= chip->planes_per_chip;
		} else {
			 
			chip->chipsize = (((uint64_t)type->chipsize) << 20);
		}
	} else {
		 
		chip->planes_per_chip = 1;
		chip->chipsize = (((uint64_t)type->chipsize) << 20);
	}

	if (id_len >= 4) {
		 
		data = id[3] & 0x3;
		mtd->writesize = 1024 << data;  

		data = (id[3] >> 2) & 0x1;
		mtd->oobsize = 8 << data;		 
		mtd->oobsize *= mtd->writesize / 512;	 

		if (id[0] == NAND_MFR_TOSHIBA && id[1] == 0xD3 &&
		    id[2] == 0x90 && id[3] == 0x26 && id[4] == 0x76)
			mtd->oobsize = 232;

		data = (id[3] >> 4) & 0x3;
		mtd->erasesize = (64 * 1024) << data;  

		if ((id[3] >> 6) & 0x1)
			chip->options |= NAND_BUSWIDTH_16;
	} else {
		 
		mtd->writesize = type->pagesize;
		mtd->oobsize = type->pagesize / 32;
		if (type->options & NAND_BUSWIDTH_16)
			chip->options |= NAND_BUSWIDTH_16;
	}

	chip->options |= NAND_NO_AUTOINCR;
	if (chip->planes_per_chip > 1)
		chip->options |= NAND_MULTIPLANE_READ;

	if (mtd->writesize > 512)
		chip->options |= NAND_NEED_READRDY;

	if (id[0] == NAND_MFR_SAMSUNG && mtd->writesize > 512)
		chip->options |= NAND_SAMSUNG_LP_OPTIONS;

	if (id_len == 4 && id[0] == NAND_MFR_HYNIX &&
	    (id[1] == 0xF1 || id[1] == 0xC1 || id[1] == 0xA1 || id[1] == 0xAD ||
	     id[1] == 0xDA || id[1] == 0xCA)) {
		 
		chip->luns_per_chip = 1;
		chip->cellinfo = 0;
		chip->options |= (NAND_CACHEPRG |
				  NAND_CACHERD |
				  NAND_COPYBACK);
	} else if (id_len == 4 && id[0] == NAND_MFR_MICRON &&
		   (id[1] == 0xDA || id[1] == 0xCA || id[1] == 0xDC ||
		    id[1] == 0xCC || id[1] == 0xAA || id[1] == 0xBA)) {
		 
		chip->luns_per_chip = 1;
		chip->cellinfo = 0;
		chip->options |= (NAND_CACHEPRG |
				  NAND_CACHERD |
				  NAND_COPYBACK);
	} else if (id_len == 4 && id[0] == NAND_MFR_SAMSUNG &&
		   (id[1] == 0xF1 || id[1] == 0xA1)) {
		 
		chip->luns_per_chip = 1;
		chip->cellinfo = 0;
		chip->options |= (NAND_CACHEPRG |
				  NAND_CACHERD |
				  NAND_COPYBACK);
	} else {
		 
		data = id[2] & 0x3;
		chip->luns_per_chip = 0x1 << data;

		if ((id[2] >> 6) & 0x1)
			chip->options |= NAND_MULTILUN;

		if ((id[2] >> 7) & 0x1)
			chip->options |= NAND_CACHEPRG;

		chip->cellinfo = id[2];
	}

	return 0;
}

static int nand_decode_id_6(struct mtd_info *mtd,
			    struct nand_chip *chip,
			    struct nand_flash_dev *type,
			    uint8_t *id, int id_len) {
	uint8_t data;

	if (id_len != 6) {
		pr_err("[MTD][NAND]: %s: invalid ID length [%d]\n",
		       __func__, id_len);
		return 1;
	}

	chip->chipsize = (((uint64_t)type->chipsize) << 20);

	data = (id[4] >> 2) & 0x3;
	chip->planes_per_chip = 1 << data;

	data = id[3] & 0x3;
	mtd->writesize = 2048 << data;  

	data = ((id[3] >> 4) & 0x4) | ((id[3] >> 2) & 0x3);
	if (id[0] == NAND_MFR_SAMSUNG) {
		switch (data) {
		case 1:
			mtd->oobsize = 128;
			break;
		case 2:
			mtd->oobsize = 218;
			break;
		case 3:
			mtd->oobsize = 400;
			break;
		case 4:
			mtd->oobsize = 436;
			break;
		case 5:
			mtd->oobsize = 640;
			break;
		default:
			pr_err("[MTD][NAND]: %s: unknown OOB size\n",
			       __func__);
			return 1;
			break;
		}
	} else {
		switch (data) {
		case 0:
			mtd->oobsize = 128;
			break;
		case 1:
			mtd->oobsize = 224;
			break;
		case 2:
			mtd->oobsize = 448;
			break;
		default:
			pr_err("[MTD][NAND]: %s: unknown OOB size\n",
			       __func__);
			break;
		}
	}

	data = ((id[3] >> 5) & 0x4) | ((id[3] >> 4) & 0x3);
	switch (data) {
	case 0:
	case 1:
	case 2:
		mtd->erasesize = (128 * 1024) << data;
		break;
	case 3:
		if (id[0] == NAND_MFR_SAMSUNG)
			mtd->erasesize = (1024 * 1024);
		else
			mtd->erasesize = (768 * 1024);
		break;
	case 4:
	case 5:
		mtd->erasesize = (1024 * 1024) << (data - 4);
		break;
	default:
		pr_err("[MTD][NAND]: %s: unknown block size\n",
		       __func__);
		return 1;
		break;
	}

	chip->options |= NAND_NO_AUTOINCR;
	if (chip->planes_per_chip > 1)
		chip->options |= NAND_MULTIPLANE_READ;

	if (mtd->writesize > 512)
		chip->options |= NAND_NEED_READRDY;

	if (id[0] == NAND_MFR_SAMSUNG && mtd->writesize > 512)
		chip->options |= NAND_SAMSUNG_LP_OPTIONS;

	data = id[2] & 0x3;
	chip->luns_per_chip = 0x1 << data;

	if ((id[2] >> 6) & 0x1)
		chip->options |= NAND_MULTILUN;

	if ((id[2] >> 7) & 0x1)
		chip->options |= NAND_CACHEPRG;

	chip->cellinfo = id[2];

	chip->options |= (type->options & NAND_BUSWIDTH_16);

	return 0;
}

void nand_derive_bbm(struct mtd_info *mtd, struct nand_chip *chip, uint8_t *id)
{
	int bits_per_cell = ((chip->cellinfo >> 2) & 0x3) + 1;

	if (id[0] == NAND_MFR_HYNIX && id[1] == 0x56) {
		chip->bbm = (NAND_BBM_PAGE_0 |
			     NAND_BBM_PAGE_1 |
			     NAND_BBM_BYTE_OOB_5);
		goto set_bbt_options;
	}

	if (id[0] == NAND_MFR_HYNIX && bits_per_cell == 2 &&
	    mtd->writesize == 4096) {
		chip->bbm = (NAND_BBM_PAGE_LAST |
			     NAND_BBM_PAGE_LMIN2 |
			     NAND_BBM_BYTE_OOB_0);
		goto set_bbt_options;
	}

	if (!(chip->options & NAND_BUSWIDTH_16) &&
	    mtd->writesize >= 2048 && id[0] == NAND_MFR_STMICRO) {
		chip->bbm =  (NAND_BBM_PAGE_0 |
			      NAND_BBM_BYTE_OOB_0 |
			      NAND_BBM_BYTE_OOB_5);
		goto set_bbt_options;
	}

	if ((id[0] == NAND_MFR_SAMSUNG || id[0] == NAND_MFR_HYNIX) &&
	    bits_per_cell == 2) {
		chip->bbm = NAND_BBM_PAGE_LAST | NAND_BBM_BYTE_OOB_0;
		if (mtd->writesize == 8192)
			chip->bbm |= NAND_BBM_PAGE_0;
		goto set_bbt_options;
	}

	if (id[0] == NAND_MFR_MICRON && mtd->writesize == 2048) {
		chip->bbm = NAND_BBM_PAGE_0 | NAND_BBM_PAGE_1 |
			NAND_BBM_BYTE_OOB_0;
		goto set_bbt_options;
	}

	chip->bbm = NAND_BBM_PAGE_0;
	 
	if (bits_per_cell == 1 &&
	    (id[0] == NAND_MFR_SAMSUNG ||
	     id[0] == NAND_MFR_HYNIX ||
	     id[0] == NAND_MFR_AMD ||
	     id[0] == NAND_MFR_MACRONIX ||
	     (id[0] == NAND_MFR_TOSHIBA && mtd->writesize > 512)))
		chip->bbm |= NAND_BBM_PAGE_1;

	if (mtd->writesize == 512 && !(chip->options & NAND_BUSWIDTH_16))
		chip->bbm |= NAND_BBM_BYTE_OOB_5;
	else
		chip->bbm |= NAND_BBM_BYTE_OOB_0;

 set_bbt_options:
	 
	if (chip->bbm & NAND_BBM_PAGE_ALL)
		chip->bbt_options |= NAND_BBT_SCANALLPAGES;

	if (chip->bbm & NAND_BBM_PAGE_1)
		chip->bbt_options |= NAND_BBT_SCAN2NDPAGE;

	if (chip->bbm & NAND_BBM_PAGE_LAST)
		chip->bbt_options |= NAND_BBT_SCANLASTPAGE;

	chip->badblockbits = 8;

	if (mtd->writesize > 512 || (chip->options & NAND_BUSWIDTH_16))
		chip->badblockpos = NAND_LARGE_BADBLOCK_POS;
	else
		chip->badblockpos = NAND_SMALL_BADBLOCK_POS;

	return;
}
EXPORT_SYMBOL(nand_derive_bbm);

static int nand_get_id_len(uint8_t *id, int max_id_len)
{
	int i, len;

	for (len = 2; len < max_id_len; len++) {
		for (i = len; i < max_id_len; i++)
			if (id[i] != id[i % len])
				break;

		if (i == max_id_len)
			break;
	}

	if (len == max_id_len) {
		while (len > 2 && id[len - 1] == 0x00)
			len--;
	}

	if (id[0] == NAND_MFR_TOSHIBA && len > 5)
		len = 5;

	if (id[0] == NAND_MFR_SAMSUNG && len == 6 &&
	    id[5] == NAND_MFR_SAMSUNG && id[6] == NAND_MFR_SAMSUNG)
		len = 5;

	return len;
}

int nand_decode_readid(struct mtd_info *mtd,
		   struct nand_chip *chip,
		   struct nand_flash_dev *type,
		   uint8_t *id, int max_id_len)
{
	int id_len;
	int ret;

	id_len = nand_get_id_len(id, max_id_len);
	if (id_len == 0) {
		pr_err("[MTD][NAND]: %s: failed to read device ID\n",
		       __func__);
		return 1;
	}

	if (id_len == 2 || type->pagesize)
		ret = nand_decode_id_2(mtd, chip, type, id, id_len);
	else if (id_len <= 5)
		ret = nand_decode_id_ext(mtd, chip, type, id, id_len);
	else if (id_len == 6)
		ret = nand_decode_id_6(mtd, chip, type, id, id_len);
	else
		ret = 1;

	if (ret) {
		pr_err("[MTD][NAND]: %s: failed to decode NAND "
		       "device ID\n", __func__);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(nand_decode_readid);

#endif  
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Thomas Gleixner <tglx@linutronix.de>");
MODULE_DESCRIPTION("Nand device & manufacturer IDs");
