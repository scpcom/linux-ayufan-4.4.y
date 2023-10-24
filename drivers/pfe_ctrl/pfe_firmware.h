#ifndef _PFE_FIRMWARE_H_
#define _PFE_FIRMWARE_H_

#define CLASS_FIRMWARE_FILENAME		"class_c2000.elf"
#define TMU_FIRMWARE_FILENAME		"tmu_c2000.elf"
#define UTIL_FIRMWARE_FILENAME		"util_c2000.elf"
#define UTIL_REVA0_FIRMWARE_FILENAME	"util_c2000_revA0.elf"

#define PFE_FW_CHECK_PASS		0
#define PFE_FW_CHECK_FAIL		1
#define NUM_PFE_FW				3

int pfe_firmware_init(struct pfe *pfe);
void pfe_firmware_exit(struct pfe *pfe);

#endif /* _PFE_FIRMWARE_H_ */

