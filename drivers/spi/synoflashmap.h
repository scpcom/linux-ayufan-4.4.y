#ifndef SYNO_FLASH_MAP_H 
#define SYNO_FLASH_MAP_H 

#include <linux/mtd/partitions.h>
#include <linux/spi/flash.h>

#ifdef CONFIG_ARCH_GEN3
static struct mtd_partition synomtd_partitions[] = {
	{
		.name	= "RedBoot",		/* u-boot		*/
		.offset	= 0x00000000,
		.size	= 0x000D0000,		/* 832KB		*/
	},
	{
		.name	= "zImage",		/* linux kernel image	*/
		.offset	= 0x000D0000,
		.size	= 0x00300000,		/* 3 MB 		*/
	},
	{
		.name	= "rd.gz",			/* ramdisk image*/
		.offset	= 0x003D0000,
		.size	= 0x00400000,		/* 4352 KB		*/
	},
	{
		.name	= "vendor",
		.offset	= 0x007D0000,
		.size	= 0x00010000,		/* 64KB		*/
	},
	{
		.name	= "RedBoot Config",
		.offset	= 0x007E0000,
		.size	= 0x00010000,		/* 64KB		*/
	},
	{
		.name	= "FIS directory",
		.offset	= 0x007F0000,
		.size	= 0x00010000,		/* 64KB		*/
	},
};
#else
#error Partition Table Not Defined
#endif /* CONFIG_ARCH_GEN3 */

static struct flash_platform_data spi_flashdata = { 
	.parts=synomtd_partitions,
	.nr_parts=ARRAY_SIZE(synomtd_partitions),
}; 
#endif /* SYNO_FLASH_MAP_H */
