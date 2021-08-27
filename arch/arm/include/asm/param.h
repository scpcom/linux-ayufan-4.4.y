#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __ASM_PARAM_H
#define __ASM_PARAM_H

#ifdef __KERNEL__
# define HZ		CONFIG_HZ	 
# define USER_HZ	100		 
# define CLOCKS_PER_SEC	(USER_HZ)	 
#else
# define HZ		100
#endif

#if !defined(MY_ABC_HERE) || !defined(CONFIG_COMCERTO_64K_PAGES)
#define EXEC_PAGESIZE	4096
#else
#define EXEC_PAGESIZE	65536
#endif

#ifndef NOGROUP
#define NOGROUP         (-1)
#endif

#define MAXHOSTNAMELEN  64

#endif
