#ifndef _UAPI__ASM_GENERIC_PARAM_H
#define _UAPI__ASM_GENERIC_PARAM_H

#ifndef HZ
#define HZ 100
#endif

#ifndef EXEC_PAGESIZE
#if !defined(CONFIG_COMCERTO_64K_PAGES)
#define EXEC_PAGESIZE	4096
#else
#define EXEC_PAGESIZE  65536
#endif
#endif

#ifndef NOGROUP
#define NOGROUP		(-1)
#endif

#define MAXHOSTNAMELEN	64	/* max length of hostname */


#endif /* _UAPI__ASM_GENERIC_PARAM_H */
