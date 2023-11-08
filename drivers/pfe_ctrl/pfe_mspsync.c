#ifdef __KERNEL__
#include <linux/kernel.h>
#include <linux/module.h>

#else
#include "platform.h"
#endif


#include "pfe_mod.h"

int msp_register_pfe(struct pfe_info *pfe_sync_info);
void msp_unregister_pfe(void);

struct pfe_info pfe_sync_info;

int pfe_msp_sync_init(struct pfe *pfe)
{
	pfe_sync_info.owner = THIS_MODULE;
	pfe_sync_info.cbus_baseaddr = pfe->cbus_baseaddr;
	pfe_sync_info.ddr_baseaddr = pfe->ddr_baseaddr;

	if (msp_register_pfe(&pfe_sync_info)) {
		printk(KERN_ERR "%s: Failed to register with msp\n",__func__);
		return -EIO;
	}

	return 0;
}

void pfe_msp_sync_exit(struct pfe *pfe)
{
	msp_unregister_pfe();
}
