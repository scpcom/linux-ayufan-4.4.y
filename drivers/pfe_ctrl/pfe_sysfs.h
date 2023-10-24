#ifndef _PFE_SYSFS_H_
#define _PFE_SYSFS_H_

#include <linux/proc_fs.h>

#define	PESTATUS_ADDR_CLASS	0x800
#define	PESTATUS_ADDR_TMU	0x80
#define	PESTATUS_ADDR_UTIL	0x0

#define TMU_CONTEXT_ADDR 	0x3c8
#define IPSEC_CNTRS_ADDR 	0x840

int pfe_sysfs_init(struct pfe *pfe);
void pfe_sysfs_exit(struct pfe *pfe);
#endif /* _PFE_SYSFS_H_ */
