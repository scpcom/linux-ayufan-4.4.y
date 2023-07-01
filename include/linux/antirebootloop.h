#ifndef __ANTIREBOOTLOOP_H
#define __ANTIREBOOTLOOP_H

#ifdef CONFIG_ANTIREBOOTLOOP

#include <linux/types.h>

#define ARL_MAGIC 0x1c93f311
#define ARL_KERNEL_VERSION 1

struct arl_marker {
	u32 magic[16]; /* Use a 64 byte magic value to increase the likelihood
			  of detecting bit flips. magic[i] = ARL_MAGIC + i for
			  0<i<15. */
	u32 counter; /* bootloader increments this counter on every boot
			attempt.  Kernel resets it to 0. */
	/* The term version refers to this anti-reboot-loop mechanism not to
	 * the barebox or kernel version number */
	u32 bootloader_version; /* ARL version supported by bootloader */
	u32 kernel_version; /* ARL version supported by bootloader. Filled in
			       by kernel. */
};

phys_addr_t get_antirebootloop_ptr(void);

#endif

#endif
