#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifndef _ASMARM_SHMPARAM_H
#define _ASMARM_SHMPARAM_H


#if !defined(CONFIG_SYNO_COMCERTO) || !defined(CONFIG_COMCERTO_64K_PAGES)
#if (defined(CONFIG_SYNO_ARMADA_ARCH) && defined(CONFIG_MV_SUPPORT_64KB_PAGE_SIZE)) ||\
     (defined(CONFIG_SYNO_ARMADA_ARCH_V2) && defined(CONFIG_MV_LARGE_PAGE_SUPPORT))
#define	SHMLBA	(16 << 10)		  
#elif defined(CONFIG_SYNO_ALPINE) && defined(CONFIG_ARM_PAGE_SIZE_LARGE)
#define SHMLBA  (16 << 10)
#else
#define	SHMLBA	(4 * PAGE_SIZE)		  
#endif
#else
#define	SHMLBA	(PAGE_SIZE)		  
#endif

#define __ARCH_FORCE_SHMLBA

#endif  
