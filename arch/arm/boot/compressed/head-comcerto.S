#include <asm/mach-types.h>

	.section	".start", "ax"
	ldr	r7,mach_type

mach_type:     .word    MACH_TYPE_COMCERTO
