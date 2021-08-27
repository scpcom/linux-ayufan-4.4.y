#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef _ASMARM_PGTABLE_HWDEF_H
#define _ASMARM_PGTABLE_HWDEF_H

#if (defined(MY_DEF_HERE) || defined(MY_DEF_HERE)) && defined(CONFIG_ARM_LPAE)
#include <asm/pgtable-3level-hwdef.h>
#elif defined(MY_DEF_HERE) && defined(CONFIG_ARM_LPAE)
#include <asm/pgtable-3level-hwdef.h>
#else
#include <asm/pgtable-2level-hwdef.h>
#endif

#endif
