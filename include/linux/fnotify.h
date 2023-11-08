/*
	Copyright (c) 2009  QNAP Systems, Inc.  All Rights Reserved.
	FILE:
		syscall.c
	Abstract:
		Intercept the file I/O systme calls by modifying the system call table.
*/
#ifndef _LINUX_FNOTIFY_H_
#define _LINUX_FNOTIFY_H_


#define	FN_CHMOD		0x00000001
#define	FN_CHOWN		0x00000002
#define	FN_MKDIR		0x00000004
#define	FN_RMDIR		0x00000008
#define	FN_UNLINK		0x00000010
#define	FN_SYMLINK		0x00000020
#define	FN_LINK			0x00000040
#define	FN_RENAME		0x00000080
#define	FN_OPEN			0x00000100
#define	FN_CLOSE		0x00000200
#define	FN_READ			0x00000400
#define	FN_WRITE		0x00000800
#define	FN_TRUNCATE		0x00001000
#define	FN_CHTIME		0x00002000
#define	FN_XATTR		0x00004000

#define	MARG_0			0
#define	MARG_1xI32		0x14
#define	MARG_2xI32		0x24
#define	MARG_3xI32		0x34
#define	MARG_4xI32		0x44
#define	MARG_1xI64		0x18
#define	MARG_2xI64		0x28

typedef struct _T_FILE_STATUS_
{
	uint64_t			i_size;
	uint32_t			i_mtsec;
	uint32_t			i_mtnsec;
	uint32_t			i_mode;
	uint32_t			i_uid;
	uint32_t			i_gid;
	uint32_t			i_padding;
} T_FILE_STATUS, *PT_FILE_STATUS;

#define	FILE_STATUS_BY_INODE(pinode, tfsBuf)	do {  if (!pinode) break;  tfsBuf.i_size = (uint64_t)pinode->i_size;  tfsBuf.i_mtsec = (uint32_t)pinode->i_mtime.tv_sec;  tfsBuf.i_mtnsec = (uint32_t)pinode->i_mtime.tv_nsec;  tfsBuf.i_mode = (uint32_t)pinode->i_mode;  tfsBuf.i_uid = (uint32_t)pinode->i_uid;  tfsBuf.i_gid = (uint32_t)pinode->i_gid;  tfsBuf.i_padding = 0;  } while (0)

extern uint32_t  msys_nodify;

extern void (*pfn_sys_file_notify)(int idcode, int margs, const struct path *ppath, const char *pstname, int cbname, PT_FILE_STATUS pfsOrg, int64_t iarg1, int64_t iarg2, uint32_t iarg3, uint32_t iarg4);
extern void (*pfn_sys_files_notify)(int idcode, const struct path *ppath1, const char *pstname1, int cbname1, const struct path *ppath2, const char *pstname2, int cbname2, PT_FILE_STATUS pfsOrg, PT_FILE_STATUS pfsExt);

#ifdef	_LINUX_NFSD_FH_H
extern void (*pfn_nfs_file_notify)(int idcode, int nbargs, struct svc_fh *ptsffile, const char *pszname, int cbname, PT_FILE_STATUS pfsOrg, int64_t iarg1, int64_t iarg2, uint32_t iarg3, uint32_t iarg4);
extern void (*pfn_nfs_files_notify)(int idcode, struct svc_fh *ptsfold, const char *psznold, int cbnold, struct svc_fh *ptsfnew, const char *psznnew, int cbnnew, PT_FILE_STATUS pfsOrg, PT_FILE_STATUS pfsExt);
#endif	//_LINUX_NFSD_FH_H

#endif

