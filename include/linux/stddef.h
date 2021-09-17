#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifndef _LINUX_STDDEF_H
#define _LINUX_STDDEF_H

#include <uapi/linux/stddef.h>

#ifdef MY_DEF_HERE
#if !defined(__cplusplus)
#undef NULL
#define NULL ((void *)0)

enum {
	false	= 0,
	true	= 1
};
#endif
#else
#undef NULL
#define NULL ((void *)0)

enum {
	false	= 0,
	true	= 1
};
#endif  

#undef offsetof
#ifdef __compiler_offsetof
#define offsetof(TYPE,MEMBER) __compiler_offsetof(TYPE,MEMBER)
#else
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif
#endif
