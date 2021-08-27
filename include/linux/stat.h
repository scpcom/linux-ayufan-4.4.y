#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifndef _LINUX_STAT_H
#define _LINUX_STAT_H

#ifdef __KERNEL__

#include <asm/stat.h>

#endif

#if defined(__KERNEL__) || !defined(__GLIBC__) || (__GLIBC__ < 2)

#define S_IFMT  00170000
#define S_IFSOCK 0140000
#define S_IFLNK	 0120000
#define S_IFREG  0100000
#define S_IFBLK  0060000
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_IFIFO  0010000
#define S_ISUID  0004000
#define S_ISGID  0002000
#define S_ISVTX  0001000

#define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m)	(((m) & S_IFMT) == S_IFSOCK)

#define S_IRWXU 00700
#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100

#define S_IRWXG 00070
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010

#define S_IRWXO 00007
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001

#endif

#ifdef __KERNEL__
#define S_IRWXUGO	(S_IRWXU|S_IRWXG|S_IRWXO)
#define S_IALLUGO	(S_ISUID|S_ISGID|S_ISVTX|S_IRWXUGO)
#define S_IRUGO		(S_IRUSR|S_IRGRP|S_IROTH)
#define S_IWUGO		(S_IWUSR|S_IWGRP|S_IWOTH)
#define S_IXUGO		(S_IXUSR|S_IXGRP|S_IXOTH)

#define UTIME_NOW	((1l << 30) - 1l)
#define UTIME_OMIT	((1l << 30) - 2l)

#include <linux/types.h>
#include <linux/time.h>

struct kstat {
	u64		ino;
	dev_t		dev;
	umode_t		mode;
#ifdef MY_ABC_HERE
	__u32		syno_archive_bit;
#endif
#ifdef MY_ABC_HERE
	__u32		syno_archive_version;
#endif
	unsigned int	nlink;
	uid_t		uid;
	gid_t		gid;
	dev_t		rdev;
	loff_t		size;
	struct timespec  atime;
	struct timespec	mtime;
	struct timespec	ctime;
#ifdef MY_ABC_HERE
	struct timespec syno_create_time;
#endif
	unsigned long	blksize;
	unsigned long long	blocks;
};

#endif

#ifdef MY_ABC_HERE
#ifdef __KERNEL__

struct SYNOSTAT_EXTRA {
	struct timespec create_time;   
	unsigned int archive_version;  		 
	unsigned int archive_bit; 		 
};
struct SYNOSTAT {
	struct stat st;
	struct SYNOSTAT_EXTRA ext;
};

#define SYNOST_STAT			0x00000001	 	
#define SYNOST_ARCHIVE_BIT		0x00000002	 
#define SYNOST_ARCHIVE_VER		0x00000004	 
#define SYNOST_CREATE_TIME		0x00000008	 

#define SYNOST_ALL			SYNOST_STAT|SYNOST_ARCHIVE_BIT|SYNOST_ARCHIVE_VER|SYNOST_CREATE_TIME
#define SYNOST_IS_CASELESS	0x10000000	 

#endif  
#endif  

#ifdef MY_ABC_HERE
#define S2_IARCHIVE    (1<<0)	 
#define S2_SMB_ARCHIVE (1<<1)	 
#define S2_SMB_HIDDEN  (1<<2)	 
#define S2_SMB_SYSTEM  (1<<3)	 
#define S3_IARCHIVE    (1<<4)	 
#if defined(MY_ABC_HERE) || defined(CONFIG_FS_SYNO_ACL)
#define S2_SMB_READONLY    					(1<<5)	 
#define S2_SYNO_ACL_INHERIT				    (1<<6)	 
#define S2_SYNO_ACL_IS_OWNER_GROUP			(1<<7)	 
#define S2_SYNO_ACL_EXIST					(1<<8)	 
#define S2_SYNO_ACL_SUPPORT  				(1<<9)	 
#define ALL_SYNO_ACL_ARCHIVE	(S2_SMB_READONLY|S2_SYNO_ACL_INHERIT|S2_SYNO_ACL_IS_OWNER_GROUP|S2_SYNO_ACL_EXIST|S2_SYNO_ACL_SUPPORT)
#endif
#define ALL_IARCHIVE (S2_IARCHIVE|S3_IARCHIVE)	 
#define ALL_SYNO_ARCHIVE (S2_IARCHIVE|S2_SMB_ARCHIVE|S3_IARCHIVE)	 
#if defined(MY_ABC_HERE) || defined(CONFIG_FS_SYNO_ACL)
#define ALL_ARCHIVE_BIT (S2_IARCHIVE|S2_SMB_ARCHIVE|S2_SMB_HIDDEN|S2_SMB_SYSTEM|S3_IARCHIVE|ALL_SYNO_ACL_ARCHIVE)
#else
#define ALL_ARCHIVE_BIT (S2_IARCHIVE|S2_SMB_ARCHIVE|S2_SMB_HIDDEN|S2_SMB_SYSTEM|S3_IARCHIVE)
#endif
#endif

#endif
