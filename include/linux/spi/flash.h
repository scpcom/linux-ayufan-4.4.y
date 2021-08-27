#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifndef LINUX_SPI_FLASH_H
#define LINUX_SPI_FLASH_H

#if defined(CONFIG_SYNO_COMCERTO)
#include <linux/ioport.h>
#endif

struct mtd_partition;

#ifdef CONFIG_ARCH_GEN3
 
struct flash_cs_info{
	unsigned int cs0_size;		 
	unsigned int cs1_size;		 
};
#endif
 
struct flash_platform_data {
	char		*name;
	struct mtd_partition *parts;
	unsigned int	nr_parts;

	char		*type;

#if defined(CONFIG_SYNO_COMCERTO)
	u32             num_resources;
	struct resource * resource;
#endif

};

#endif
