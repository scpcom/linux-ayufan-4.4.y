 
#ifndef _ASMARM_PGTABLE_HWDEF_H
#define _ASMARM_PGTABLE_HWDEF_H

#if (defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2)) && defined(CONFIG_ARM_LPAE)
#include <asm/pgtable-3level-hwdef.h>
#elif defined(CONFIG_SYNO_ALPINE) && defined(CONFIG_ARM_LPAE)
#include <asm/pgtable-3level-hwdef.h>
#else
#include <asm/pgtable-2level-hwdef.h>
#endif

#endif
