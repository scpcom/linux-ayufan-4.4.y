#ifndef _PFE_MSPSYNC_H_
#define _PFE_MSPSYNC_H_

struct pfe_info {
	void *ddr_baseaddr;
	void *cbus_baseaddr;
	void *owner;
};

int pfe_msp_sync_init(struct pfe *pfe);
void pfe_msp_sync_exit(struct pfe *pfe);
#endif /* _PFE_MSPSYNC_H_ */
