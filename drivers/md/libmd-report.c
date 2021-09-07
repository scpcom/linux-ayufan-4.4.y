#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifdef MY_ABC_HERE
#include <linux/bio.h>
#include <linux/synobios.h>
#include <linux/synolib.h>

#include <linux/raid/libmd-report.h>
#include "md.h"

int (*funcSYNOSendRaidEvent)(unsigned int, unsigned int, unsigned int, unsigned int) = NULL;

void SynoReportBadSector(sector_t sector, unsigned long rw, 
						 int md_minor, struct block_device *bdev, const char *szFuncName)
{
	char b[BDEVNAME_SIZE];
	int index = SynoSCSIGetDeviceIndex(bdev);

	bdevname(bdev,b);

	if (printk_ratelimit()) {
		printk("%s error, md%d, %s index [%d], sector %llu [%s]\n",
					   rw ? "write" : "read", md_minor, b, index, (unsigned long long)sector, szFuncName);
	}

	if (funcSYNOSendRaidEvent) {
		funcSYNOSendRaidEvent(
			(rw == WRITE) ? MD_SECTOR_WRITE_ERROR : MD_SECTOR_READ_ERROR, 
			md_minor, index, sector);
	}
}

EXPORT_SYMBOL(SynoReportBadSector);

void SynoReportCorrectBadSector(sector_t sector, int md_minor, 
								struct block_device *bdev, const char *szFuncName)
{
	char b[BDEVNAME_SIZE];
	int index = SynoSCSIGetDeviceIndex(bdev);

	bdevname(bdev,b);

	printk("read error corrected, md%d, %s index [%d], sector %llu [%s]\n",
				   md_minor, b, index, (unsigned long long)sector, szFuncName);

	if (funcSYNOSendRaidEvent) {
		funcSYNOSendRaidEvent(MD_SECTOR_REWRITE_OK, md_minor, 
							  index, sector);
	}
}
EXPORT_SYMBOL(SynoReportCorrectBadSector);
EXPORT_SYMBOL(funcSYNOSendRaidEvent);
#endif  
