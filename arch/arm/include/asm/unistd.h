#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __ASM_ARM_UNISTD_H
#define __ASM_ARM_UNISTD_H

#include <uapi/asm/unistd.h>

#ifdef MY_ABC_HERE
#include <asm-generic/bitsperlong.h>
#define __NR_syscalls  (380 + 48)
#else  
#define __NR_syscalls  (380)
#endif  

#define __ARM_NR_cmpxchg		(__ARM_NR_BASE+0x00fff0)

#define __ARCH_WANT_STAT64
#define __ARCH_WANT_SYS_GETHOSTNAME
#define __ARCH_WANT_SYS_PAUSE
#define __ARCH_WANT_SYS_GETPGRP
#define __ARCH_WANT_SYS_LLSEEK
#define __ARCH_WANT_SYS_NICE
#define __ARCH_WANT_SYS_SIGPENDING
#define __ARCH_WANT_SYS_SIGPROCMASK
#define __ARCH_WANT_SYS_OLD_MMAP
#define __ARCH_WANT_SYS_OLD_SELECT

#if !defined(CONFIG_AEABI) || defined(CONFIG_OABI_COMPAT)
#define __ARCH_WANT_SYS_TIME
#define __ARCH_WANT_SYS_IPC
#define __ARCH_WANT_SYS_OLDUMOUNT
#define __ARCH_WANT_SYS_ALARM
#define __ARCH_WANT_SYS_UTIME
#define __ARCH_WANT_SYS_OLD_GETRLIMIT
#define __ARCH_WANT_OLD_READDIR
#define __ARCH_WANT_SYS_SOCKETCALL
#endif
#define __ARCH_WANT_SYS_FORK
#define __ARCH_WANT_SYS_VFORK
#define __ARCH_WANT_SYS_CLONE

#define __IGNORE_fadvise64_64
#define __IGNORE_migrate_pages

#ifdef MY_ABC_HERE
#ifdef MY_ABC_HERE
#if BITS_PER_LONG == 32
#define __IGNORE_SYNOCaselessStat
#define __IGNORE_SYNOCaselessLStat
#endif  
#endif  

#ifdef MY_ABC_HERE
#if BITS_PER_LONG == 32
#define __IGNORE_SYNOStat
#define __IGNORE_SYNOFStat
#define __IGNORE_SYNOLStat
#endif  
#endif  
#endif  

#endif  
