#include "pfe_mod.h"
#include "pfe_pcap.h"

/* PFE packet capture:
  - uses HIF functions to receive packets
  - uses ctrl function to control packet capture
 */

int pfe_pcap_init(struct pfe *pfe)
{
	printk(KERN_INFO "%s\n", __func__);

	return 0;
}

void pfe_pcap_exit(struct pfe *pfe)
{
	printk(KERN_INFO "%s\n", __func__);
}

