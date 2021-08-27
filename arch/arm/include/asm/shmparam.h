#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifndef _ASMARM_SHMPARAM_H
#define _ASMARM_SHMPARAM_H

/*
 * This should be the size of the virtually indexed cache/ways,
 * or page size, whichever is greater since the cache aliases
 * every size/ways bytes.
 */
#if !defined(CONFIG_SYNO_COMCERTO) || !defined(CONFIG_COMCERTO_64K_PAGES)
#if defined(CONFIG_SYNO_ARMADA_ARCH) && defined(CONFIG_MV_SUPPORT_64KB_PAGE_SIZE)
#define	SHMLBA	(16 << 10)		 /* attach addr a multiple of this */
#else
#define	SHMLBA	(4 * PAGE_SIZE)		 /* attach addr a multiple of this */
#endif
#else
#define	SHMLBA	(PAGE_SIZE)		 /* attach addr a multiple of this */
#endif

/*
 * Enforce SHMLBA in shmat
 */
#define __ARCH_FORCE_SHMLBA

#endif /* _ASMARM_SHMPARAM_H */
