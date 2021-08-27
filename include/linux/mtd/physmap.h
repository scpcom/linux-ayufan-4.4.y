#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 


#ifndef __LINUX_MTD_PHYSMAP__
#define __LINUX_MTD_PHYSMAP__

#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#if defined(CONFIG_SYNO_COMCERTO)
#include <linux/platform_device.h>
#endif

struct map_info;
struct platform_device;

struct physmap_flash_data {
	unsigned int		width;
	int			(*init)(struct platform_device *);
	void			(*exit)(struct platform_device *);
	void			(*set_vpp)(struct platform_device *, int);
	unsigned int		nr_parts;
	unsigned int		pfow_base;
	char                    *probe_type;
	struct mtd_partition	*parts;
};

#endif  
