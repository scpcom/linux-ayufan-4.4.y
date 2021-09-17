#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#include <linux/export.h>

#define GLOBAL(name)	\
	.globl VMLINUX_SYMBOL(name);	\
	VMLINUX_SYMBOL(name):

	.section ".init.data","aw"

GLOBAL(modsign_certificate_list)
	.incbin "signing_key.x509"
	.incbin "extra_certificates"
GLOBAL(modsign_certificate_list_end)
