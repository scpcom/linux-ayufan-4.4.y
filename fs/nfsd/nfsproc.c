#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/namei.h>

#include "cache.h"
#include "xdr.h"
#include "vfs.h"

#ifdef MY_ABC_HERE
#include "../synoacl_int.h"
#endif  

typedef struct svc_rqst	svc_rqst;
typedef struct svc_buf	svc_buf;

#define NFSDDBG_FACILITY		NFSDDBG_PROC

static __be32
nfsd_proc_null(struct svc_rqst *rqstp, void *argp, void *resp)
{
	return nfs_ok;
}

static __be32
nfsd_return_attrs(__be32 err, struct nfsd_attrstat *resp)
{
	if (err) return err;
	return fh_getattr(&resp->fh, &resp->stat);
}
static __be32
nfsd_return_dirop(__be32 err, struct nfsd_diropres *resp)
{
	if (err) return err;
	return fh_getattr(&resp->fh, &resp->stat);
}
 
static __be32
nfsd_proc_getattr(struct svc_rqst *rqstp, struct nfsd_fhandle  *argp,
					  struct nfsd_attrstat *resp)
{
	__be32 nfserr;
	dprintk("nfsd: GETATTR  %s\n", SVCFH_fmt(&argp->fh));

	fh_copy(&resp->fh, &argp->fh);
	nfserr = fh_verify(rqstp, &resp->fh, 0,
			NFSD_MAY_NOP | NFSD_MAY_BYPASS_GSS_ON_ROOT);
	return nfsd_return_attrs(nfserr, resp);
}

static __be32
nfsd_proc_setattr(struct svc_rqst *rqstp, struct nfsd_sattrargs *argp,
					  struct nfsd_attrstat  *resp)
{
	struct iattr *iap = &argp->attrs;
	struct svc_fh *fhp;
	__be32 nfserr;

	dprintk("nfsd: SETATTR  %s, valid=%x, size=%ld\n",
		SVCFH_fmt(&argp->fh),
		argp->attrs.ia_valid, (long) argp->attrs.ia_size);

	fhp = fh_copy(&resp->fh, &argp->fh);

#define BOTH_TIME_SET (ATTR_ATIME_SET | ATTR_MTIME_SET)
#define	MAX_TOUCH_TIME_ERROR (30*60)
	if ((iap->ia_valid & BOTH_TIME_SET) == BOTH_TIME_SET &&
	    iap->ia_mtime.tv_sec == iap->ia_atime.tv_sec) {
		 
		time_t delta = iap->ia_atime.tv_sec - get_seconds();
		struct inode *inode;

		nfserr = fh_verify(rqstp, fhp, 0, NFSD_MAY_NOP);
		if (nfserr)
			goto done;
		inode = d_inode(fhp->fh_dentry);

		if (delta < 0)
			delta = -delta;
#ifdef MY_ABC_HERE
		if (delta < MAX_TOUCH_TIME_ERROR) {
			if (IS_SYNOACL(fhp->fh_dentry)) {
				if (0 > synoacl_op_inode_chg_ok(fhp->fh_dentry, iap)) {
					iap->ia_valid &= ~BOTH_TIME_SET;
				}
			} else if (0 > inode_change_ok(inode, iap)){
				iap->ia_valid &= ~BOTH_TIME_SET;
			}
		}
#else
		if (delta < MAX_TOUCH_TIME_ERROR &&
		    inode_change_ok(inode, iap) != 0) {
			 
			iap->ia_valid &= ~BOTH_TIME_SET;
		}
#endif  
	}

	nfserr = nfsd_setattr(rqstp, fhp, iap, 0, (time_t)0);
done:
	return nfsd_return_attrs(nfserr, resp);
}

static __be32
nfsd_proc_lookup(struct svc_rqst *rqstp, struct nfsd_diropargs *argp,
					 struct nfsd_diropres  *resp)
{
	__be32	nfserr;

	dprintk("nfsd: LOOKUP   %s %.*s\n",
		SVCFH_fmt(&argp->fh), argp->len, argp->name);

	fh_init(&resp->fh, NFS_FHSIZE);
	nfserr = nfsd_lookup(rqstp, &argp->fh, argp->name, argp->len,
				 &resp->fh);

	fh_put(&argp->fh);
	return nfsd_return_dirop(nfserr, resp);
}

static __be32
nfsd_proc_readlink(struct svc_rqst *rqstp, struct nfsd_readlinkargs *argp,
					   struct nfsd_readlinkres *resp)
{
	__be32	nfserr;

	dprintk("nfsd: READLINK %s\n", SVCFH_fmt(&argp->fh));

	resp->len = NFS_MAXPATHLEN;
	nfserr = nfsd_readlink(rqstp, &argp->fh, argp->buffer, &resp->len);

	fh_put(&argp->fh);
	return nfserr;
}

static __be32
nfsd_proc_read(struct svc_rqst *rqstp, struct nfsd_readargs *argp,
				       struct nfsd_readres  *resp)
{
	__be32	nfserr;

	dprintk("nfsd: READ    %s %d bytes at %d\n",
		SVCFH_fmt(&argp->fh),
		argp->count, argp->offset);

	if (NFSSVC_MAXBLKSIZE_V2 < argp->count) {
		char buf[RPC_MAX_ADDRBUFLEN];
		printk(KERN_NOTICE
			"oversized read request from %s (%d bytes)\n",
				svc_print_addr(rqstp, buf, sizeof(buf)),
				argp->count);
		argp->count = NFSSVC_MAXBLKSIZE_V2;
	}
	svc_reserve_auth(rqstp, (19<<2) + argp->count + 4);

	resp->count = argp->count;
	nfserr = nfsd_read(rqstp, fh_copy(&resp->fh, &argp->fh),
				  argp->offset,
			   	  rqstp->rq_vec, argp->vlen,
				  &resp->count);

	if (nfserr) return nfserr;
	return fh_getattr(&resp->fh, &resp->stat);
}

static __be32
nfsd_proc_write(struct svc_rqst *rqstp, struct nfsd_writeargs *argp,
					struct nfsd_attrstat  *resp)
{
	__be32	nfserr;
	int	stable = 1;
	unsigned long cnt = argp->len;

	dprintk("nfsd: WRITE    %s %d bytes at %d\n",
		SVCFH_fmt(&argp->fh),
		argp->len, argp->offset);

	nfserr = nfsd_write(rqstp, fh_copy(&resp->fh, &argp->fh), NULL,
				   argp->offset,
				   rqstp->rq_vec, argp->vlen,
			           &cnt,
				   &stable);
	return nfsd_return_attrs(nfserr, resp);
}

static __be32
nfsd_proc_create(struct svc_rqst *rqstp, struct nfsd_createargs *argp,
					 struct nfsd_diropres   *resp)
{
	svc_fh		*dirfhp = &argp->fh;
	svc_fh		*newfhp = &resp->fh;
	struct iattr	*attr = &argp->attrs;
	struct inode	*inode;
	struct dentry	*dchild;
	int		type, mode;
	__be32		nfserr;
	int		hosterr;
	dev_t		rdev = 0, wanted = new_decode_dev(attr->ia_size);

	dprintk("nfsd: CREATE   %s %.*s\n",
		SVCFH_fmt(dirfhp), argp->len, argp->name);

	nfserr = fh_verify(rqstp, dirfhp, S_IFDIR, NFSD_MAY_EXEC);
	if (nfserr)
		goto done;  

	nfserr = nfserr_acces;
	if (!argp->len)
		goto done;
	nfserr = nfserr_exist;
	if (isdotent(argp->name, argp->len))
		goto done;
	hosterr = fh_want_write(dirfhp);
	if (hosterr) {
		nfserr = nfserrno(hosterr);
		goto done;
	}

	fh_lock_nested(dirfhp, I_MUTEX_PARENT);
	dchild = lookup_one_len(argp->name, dirfhp->fh_dentry, argp->len);
	if (IS_ERR(dchild)) {
		nfserr = nfserrno(PTR_ERR(dchild));
		goto out_unlock;
	}
	fh_init(newfhp, NFS_FHSIZE);
	nfserr = fh_compose(newfhp, dirfhp->fh_export, dchild, dirfhp);
	if (!nfserr && d_really_is_negative(dchild))
		nfserr = nfserr_noent;
	dput(dchild);
	if (nfserr) {
		if (nfserr != nfserr_noent)
			goto out_unlock;
		 
		nfserr = nfserr_acces;
		if (!newfhp->fh_dentry) {
			printk(KERN_WARNING
				"nfsd_proc_create: file handle not verified\n");
			goto out_unlock;
		}
	}

	inode = d_inode(newfhp->fh_dentry);

	if (attr->ia_valid & ATTR_MODE) {
		type = attr->ia_mode & S_IFMT;
		mode = attr->ia_mode & ~S_IFMT;
		if (!type) {
			 
			if (inode) {
				type = inode->i_mode & S_IFMT;
				switch(type) {
				case S_IFCHR:
				case S_IFBLK:
					 
					rdev = inode->i_rdev;
					attr->ia_valid |= ATTR_SIZE;

				case S_IFIFO:
					 
					nfserr = nfsd_permission(rqstp,
								 newfhp->fh_export,
								 newfhp->fh_dentry,
								 NFSD_MAY_WRITE|NFSD_MAY_LOCAL_ACCESS);
					if (nfserr && nfserr != nfserr_rofs)
						goto out_unlock;
				}
			} else
				type = S_IFREG;
		}
	} else if (inode) {
		type = inode->i_mode & S_IFMT;
		mode = inode->i_mode & ~S_IFMT;
	} else {
		type = S_IFREG;
		mode = 0;	 
	}

	attr->ia_valid |= ATTR_MODE;
	attr->ia_mode = mode;

	if (type != S_IFREG) {
		if (type != S_IFBLK && type != S_IFCHR) {
			rdev = 0;
		} else if (type == S_IFCHR && !(attr->ia_valid & ATTR_SIZE)) {
			 
			type = S_IFIFO;
		} else {
			 
			if (!rdev)
				rdev = wanted;
		}

		attr->ia_valid &= ~ATTR_SIZE;

		nfserr = nfserr_exist;
		if (inode && type != (inode->i_mode & S_IFMT))
			goto out_unlock;
	}

	nfserr = 0;
	if (!inode) {
		 
		nfserr = nfsd_create(rqstp, dirfhp, argp->name, argp->len,
					attr, type, rdev, newfhp);
	} else if (type == S_IFREG) {
		dprintk("nfsd:   existing %s, valid=%x, size=%ld\n",
			argp->name, attr->ia_valid, (long) attr->ia_size);
		 
		attr->ia_valid &= ATTR_SIZE;
		if (attr->ia_valid)
			nfserr = nfsd_setattr(rqstp, newfhp, attr, 0, (time_t)0);
	}

out_unlock:
	 
	fh_unlock(dirfhp);
	fh_drop_write(dirfhp);
done:
	fh_put(dirfhp);
	return nfsd_return_dirop(nfserr, resp);
}

static __be32
nfsd_proc_remove(struct svc_rqst *rqstp, struct nfsd_diropargs *argp,
					 void		       *resp)
{
	__be32	nfserr;

	dprintk("nfsd: REMOVE   %s %.*s\n", SVCFH_fmt(&argp->fh),
		argp->len, argp->name);

	nfserr = nfsd_unlink(rqstp, &argp->fh, -S_IFDIR, argp->name, argp->len);
	fh_put(&argp->fh);
	return nfserr;
}

static __be32
nfsd_proc_rename(struct svc_rqst *rqstp, struct nfsd_renameargs *argp,
				  	 void		        *resp)
{
	__be32	nfserr;

	dprintk("nfsd: RENAME   %s %.*s -> \n",
		SVCFH_fmt(&argp->ffh), argp->flen, argp->fname);
	dprintk("nfsd:        ->  %s %.*s\n",
		SVCFH_fmt(&argp->tfh), argp->tlen, argp->tname);

	nfserr = nfsd_rename(rqstp, &argp->ffh, argp->fname, argp->flen,
				    &argp->tfh, argp->tname, argp->tlen);
	fh_put(&argp->ffh);
	fh_put(&argp->tfh);
	return nfserr;
}

static __be32
nfsd_proc_link(struct svc_rqst *rqstp, struct nfsd_linkargs *argp,
				void			    *resp)
{
	__be32	nfserr;

	dprintk("nfsd: LINK     %s ->\n",
		SVCFH_fmt(&argp->ffh));
	dprintk("nfsd:    %s %.*s\n",
		SVCFH_fmt(&argp->tfh),
		argp->tlen,
		argp->tname);

	nfserr = nfsd_link(rqstp, &argp->tfh, argp->tname, argp->tlen,
				  &argp->ffh);
	fh_put(&argp->ffh);
	fh_put(&argp->tfh);
	return nfserr;
}

static __be32
nfsd_proc_symlink(struct svc_rqst *rqstp, struct nfsd_symlinkargs *argp,
				          void			  *resp)
{
	struct svc_fh	newfh;
	__be32		nfserr;

	dprintk("nfsd: SYMLINK  %s %.*s -> %.*s\n",
		SVCFH_fmt(&argp->ffh), argp->flen, argp->fname,
		argp->tlen, argp->tname);

	fh_init(&newfh, NFS_FHSIZE);
	 
	argp->tname[argp->tlen] = '\0';
	nfserr = nfsd_symlink(rqstp, &argp->ffh, argp->fname, argp->flen,
						 argp->tname, &newfh);

	fh_put(&argp->ffh);
	fh_put(&newfh);
	return nfserr;
}

static __be32
nfsd_proc_mkdir(struct svc_rqst *rqstp, struct nfsd_createargs *argp,
					struct nfsd_diropres   *resp)
{
	__be32	nfserr;

	dprintk("nfsd: MKDIR    %s %.*s\n", SVCFH_fmt(&argp->fh), argp->len, argp->name);

	if (resp->fh.fh_dentry) {
		printk(KERN_WARNING
			"nfsd_proc_mkdir: response already verified??\n");
	}

	argp->attrs.ia_valid &= ~ATTR_SIZE;
	fh_init(&resp->fh, NFS_FHSIZE);
	nfserr = nfsd_create(rqstp, &argp->fh, argp->name, argp->len,
				    &argp->attrs, S_IFDIR, 0, &resp->fh);
	fh_put(&argp->fh);
	return nfsd_return_dirop(nfserr, resp);
}

static __be32
nfsd_proc_rmdir(struct svc_rqst *rqstp, struct nfsd_diropargs *argp,
				 	void		      *resp)
{
	__be32	nfserr;

	dprintk("nfsd: RMDIR    %s %.*s\n", SVCFH_fmt(&argp->fh), argp->len, argp->name);

	nfserr = nfsd_unlink(rqstp, &argp->fh, S_IFDIR, argp->name, argp->len);
	fh_put(&argp->fh);
	return nfserr;
}

static __be32
nfsd_proc_readdir(struct svc_rqst *rqstp, struct nfsd_readdirargs *argp,
					  struct nfsd_readdirres  *resp)
{
	int		count;
	__be32		nfserr;
	loff_t		offset;

	dprintk("nfsd: READDIR  %s %d bytes at %d\n",
		SVCFH_fmt(&argp->fh),
		argp->count, argp->cookie);

	count = (argp->count >> 2) - 2;

	count -= 2;
	if (count < 0)
		count = 0;

	resp->buffer = argp->buffer;
	resp->offset = NULL;
	resp->buflen = count;
	resp->common.err = nfs_ok;
	 
	offset = argp->cookie;
	nfserr = nfsd_readdir(rqstp, &argp->fh, &offset,
			      &resp->common, nfssvc_encode_entry);

	resp->count = resp->buffer - argp->buffer;
	if (resp->offset)
		*resp->offset = htonl(offset);

	fh_put(&argp->fh);
	return nfserr;
}

static __be32
nfsd_proc_statfs(struct svc_rqst * rqstp, struct nfsd_fhandle   *argp,
					  struct nfsd_statfsres *resp)
{
	__be32	nfserr;

	dprintk("nfsd: STATFS   %s\n", SVCFH_fmt(&argp->fh));

	nfserr = nfsd_statfs(rqstp, &argp->fh, &resp->stats,
			NFSD_MAY_BYPASS_GSS_ON_ROOT);
	fh_put(&argp->fh);
	return nfserr;
}

#ifdef MY_ABC_HERE
static __be32
nfsd_proc_writezero(struct svc_rqst *rqstp, struct nfsd_writeargs *argp,
										struct nfsd_attrstat  *resp)
{
	__be32  nfserr, beCnt;
	unsigned long cnt;
	loff_t offset = argp->offset;

	offset *= NFS2_MAXZEROEDSIZE;

	memcpy(&beCnt, rqstp->rq_vec[0].iov_base, sizeof(beCnt));

	cnt = ntohl(beCnt);

	if (cnt > NFS2_MAXZEROEDSIZE) {
		dprintk("nfsd: ERROR WRITEZERO    zeroed byte %lu too large\n", cnt);

		cnt = NFS2_MAXZEROEDSIZE;
	}

	dprintk("nfsd: WRITEZERO    %s %lu zero bytes at %llu\n",
			SVCFH_fmt(&argp->fh),
			cnt, offset);

	nfserr = nfsd_writezero(rqstp, fh_copy(&resp->fh, &argp->fh),
							   offset, &cnt);

	nfserr = nfsd_return_attrs(nfserr, resp);

	dprintk("nfsd: WRITEZERO block count:%llu\n", resp->stat.blocks);

	return nfserr;
}

static __be32
nfsd_proc_xlookup(struct svc_rqst *rqstp, struct nfsd_diropargs *argp,
										 struct nfsd_diropres  *resp)
{
	__be32  nfserr;

	dprintk("nfsd: XLOOKUP   %s %.*s\n",
			SVCFH_fmt(&argp->fh), argp->len, argp->name);

	fh_init(&resp->fh, NFS_FHSIZE);
	nfserr = nfsd_lookup(rqstp, &argp->fh, argp->name, argp->len,
							 &resp->fh);

	fh_put(&argp->fh);

	nfserr = nfsd_return_dirop(nfserr, resp);
	if (nfserr) {
		goto end;
	}

	dprintk("nfsd: XLOOKUP  file block count %lld\n", resp->stat.blocks);

	

	resp->stat.ino = resp->stat.size >> 32;
	resp->stat.size &= (NFS2_4G - 1);
	resp->stat.ino |= (resp->stat.blocks >> 32) << 16;
	resp->stat.blocks &= (NFS2_4G - 1);

	
	if (resp->fh.fh_dentry && resp->fh.fh_dentry->d_inode) {
		resp->stat.nlink = resp->fh.fh_dentry->d_inode->i_sb->s_magic;
	} else {
		resp->stat.nlink = 0;
	}

end:
	if (!(resp->fh.fh_dentry)) {
		printk(KERN_WARNING "nfsd: XLOOKUP   resp->fh.fh_dentery is null\n");
	} else if (!(resp->fh.fh_dentry->d_inode)) {
		printk(KERN_WARNING "nfsd: XLOOKUP   resp->fh.fh_dentry->d_inode is null\n");
	}
	return nfserr;
}

static __be32
nfsd_proc_synocopy(struct svc_rqst *rqstp, struct nfsd_writeargs *argp,
										struct nfsd_attrstat  *resp)
{
	__be32  nfserr = 0, beCnt;
	unsigned long cnt;
	loff_t offset = argp->offset;
	int fnOffset;
	bool skipZero;
	char zeroBuf[sizeof(__be32)] = {0};

	offset *= NFS2_SYNOCOPYSIZE;

	memcpy(&beCnt, rqstp->rq_vec[0].iov_base, sizeof(__be32));

	cnt = ntohl(beCnt);

	if (cnt > NFS2_SYNOCOPYSIZE) {
		dprintk("nfsd: ERROR SYNOCOPY    copyed byte %lu too large\n", cnt);

		cnt = NFS2_SYNOCOPYSIZE;
	}

	skipZero = (0 != memcmp(rqstp->rq_vec[0].iov_base + sizeof(__be32), zeroBuf, sizeof(__be32)));

	fnOffset = sizeof(__be32) + sizeof(__be32);

	dprintk("nfsd: SYNOCOPY    from %s %lu bytes at %llu\n",
			(char *)(rqstp->rq_vec[0].iov_base + fnOffset),
			cnt, offset);

	nfserr = nfsd_synocopy((const char *)(rqstp->rq_vec[0].iov_base + fnOffset), rqstp, fh_copy(&resp->fh, &argp->fh),
							   offset, &cnt, skipZero);

	nfserr = nfsd_return_attrs(nfserr, resp);

	return nfserr;
}

static __be32
nfsd_proc_synosupport(struct svc_rqst *rqstp, void *argp, void *resp)
{
	return nfs_ok;
}

#ifdef MY_ABC_HERE
static __be32
nfsd_proc_synoclone(struct svc_rqst *rqstp, struct nfsd_writeargs *argp,
										struct nfsd_attrstat  *resp)
{
	__be32  nfserr = 0;
	int fnOffset;

	fnOffset = sizeof(__be32) + sizeof(__be32);

	dprintk("nfsd: SYNOCLONE    from %s \n",
			(char *)(rqstp->rq_vec[0].iov_base + fnOffset));

	nfserr = nfsd_synoclone((const char *)(rqstp->rq_vec[0].iov_base + fnOffset), rqstp, fh_copy(&resp->fh, &argp->fh));

	nfserr = nfsd_return_attrs(nfserr, resp);

	return nfserr;
}
#endif 
#endif


struct nfsd_void { int dummy; };

#define ST 1		
#define FH 8		
#define	AT 18		

#ifndef MY_ABC_HERE
static struct svc_procedure		nfsd_procedures2[18] = {
#else
#ifdef MY_ABC_HERE
static struct svc_procedure             nfsd_procedures2[33] = {
#else
static struct svc_procedure             nfsd_procedures2[32] = {
#endif 
#endif
	[NFSPROC_NULL] = {
		.pc_func = (svc_procfunc) nfsd_proc_null,
		.pc_decode = (kxdrproc_t) nfssvc_decode_void,
		.pc_encode = (kxdrproc_t) nfssvc_encode_void,
		.pc_argsize = sizeof(struct nfsd_void),
		.pc_ressize = sizeof(struct nfsd_void),
		.pc_cachetype = RC_NOCACHE,
		.pc_xdrressize = ST,
	},
	[NFSPROC_GETATTR] = {
		.pc_func = (svc_procfunc) nfsd_proc_getattr,
		.pc_decode = (kxdrproc_t) nfssvc_decode_fhandle,
		.pc_encode = (kxdrproc_t) nfssvc_encode_attrstat,
		.pc_release = (kxdrproc_t) nfssvc_release_fhandle,
		.pc_argsize = sizeof(struct nfsd_fhandle),
		.pc_ressize = sizeof(struct nfsd_attrstat),
		.pc_cachetype = RC_NOCACHE,
		.pc_xdrressize = ST+AT,
	},
	[NFSPROC_SETATTR] = {
		.pc_func = (svc_procfunc) nfsd_proc_setattr,
		.pc_decode = (kxdrproc_t) nfssvc_decode_sattrargs,
		.pc_encode = (kxdrproc_t) nfssvc_encode_attrstat,
		.pc_release = (kxdrproc_t) nfssvc_release_fhandle,
		.pc_argsize = sizeof(struct nfsd_sattrargs),
		.pc_ressize = sizeof(struct nfsd_attrstat),
		.pc_cachetype = RC_REPLBUFF,
		.pc_xdrressize = ST+AT,
	},
	[NFSPROC_ROOT] = {
		.pc_decode = (kxdrproc_t) nfssvc_decode_void,
		.pc_encode = (kxdrproc_t) nfssvc_encode_void,
		.pc_argsize = sizeof(struct nfsd_void),
		.pc_ressize = sizeof(struct nfsd_void),
		.pc_cachetype = RC_NOCACHE,
		.pc_xdrressize = ST,
	},
	[NFSPROC_LOOKUP] = {
		.pc_func = (svc_procfunc) nfsd_proc_lookup,
		.pc_decode = (kxdrproc_t) nfssvc_decode_diropargs,
		.pc_encode = (kxdrproc_t) nfssvc_encode_diropres,
		.pc_release = (kxdrproc_t) nfssvc_release_fhandle,
		.pc_argsize = sizeof(struct nfsd_diropargs),
		.pc_ressize = sizeof(struct nfsd_diropres),
		.pc_cachetype = RC_NOCACHE,
		.pc_xdrressize = ST+FH+AT,
	},
	[NFSPROC_READLINK] = {
		.pc_func = (svc_procfunc) nfsd_proc_readlink,
		.pc_decode = (kxdrproc_t) nfssvc_decode_readlinkargs,
		.pc_encode = (kxdrproc_t) nfssvc_encode_readlinkres,
		.pc_argsize = sizeof(struct nfsd_readlinkargs),
		.pc_ressize = sizeof(struct nfsd_readlinkres),
		.pc_cachetype = RC_NOCACHE,
		.pc_xdrressize = ST+1+NFS_MAXPATHLEN/4,
	},
	[NFSPROC_READ] = {
		.pc_func = (svc_procfunc) nfsd_proc_read,
		.pc_decode = (kxdrproc_t) nfssvc_decode_readargs,
		.pc_encode = (kxdrproc_t) nfssvc_encode_readres,
		.pc_release = (kxdrproc_t) nfssvc_release_fhandle,
		.pc_argsize = sizeof(struct nfsd_readargs),
		.pc_ressize = sizeof(struct nfsd_readres),
		.pc_cachetype = RC_NOCACHE,
		.pc_xdrressize = ST+AT+1+NFSSVC_MAXBLKSIZE_V2/4,
	},
	[NFSPROC_WRITECACHE] = {
		.pc_decode = (kxdrproc_t) nfssvc_decode_void,
		.pc_encode = (kxdrproc_t) nfssvc_encode_void,
		.pc_argsize = sizeof(struct nfsd_void),
		.pc_ressize = sizeof(struct nfsd_void),
		.pc_cachetype = RC_NOCACHE,
		.pc_xdrressize = ST,
	},
	[NFSPROC_WRITE] = {
		.pc_func = (svc_procfunc) nfsd_proc_write,
		.pc_decode = (kxdrproc_t) nfssvc_decode_writeargs,
		.pc_encode = (kxdrproc_t) nfssvc_encode_attrstat,
		.pc_release = (kxdrproc_t) nfssvc_release_fhandle,
		.pc_argsize = sizeof(struct nfsd_writeargs),
		.pc_ressize = sizeof(struct nfsd_attrstat),
		.pc_cachetype = RC_REPLBUFF,
		.pc_xdrressize = ST+AT,
	},
	[NFSPROC_CREATE] = {
		.pc_func = (svc_procfunc) nfsd_proc_create,
		.pc_decode = (kxdrproc_t) nfssvc_decode_createargs,
		.pc_encode = (kxdrproc_t) nfssvc_encode_diropres,
		.pc_release = (kxdrproc_t) nfssvc_release_fhandle,
		.pc_argsize = sizeof(struct nfsd_createargs),
		.pc_ressize = sizeof(struct nfsd_diropres),
		.pc_cachetype = RC_REPLBUFF,
		.pc_xdrressize = ST+FH+AT,
	},
	[NFSPROC_REMOVE] = {
		.pc_func = (svc_procfunc) nfsd_proc_remove,
		.pc_decode = (kxdrproc_t) nfssvc_decode_diropargs,
		.pc_encode = (kxdrproc_t) nfssvc_encode_void,
		.pc_argsize = sizeof(struct nfsd_diropargs),
		.pc_ressize = sizeof(struct nfsd_void),
		.pc_cachetype = RC_REPLSTAT,
		.pc_xdrressize = ST,
	},
	[NFSPROC_RENAME] = {
		.pc_func = (svc_procfunc) nfsd_proc_rename,
		.pc_decode = (kxdrproc_t) nfssvc_decode_renameargs,
		.pc_encode = (kxdrproc_t) nfssvc_encode_void,
		.pc_argsize = sizeof(struct nfsd_renameargs),
		.pc_ressize = sizeof(struct nfsd_void),
		.pc_cachetype = RC_REPLSTAT,
		.pc_xdrressize = ST,
	},
	[NFSPROC_LINK] = {
		.pc_func = (svc_procfunc) nfsd_proc_link,
		.pc_decode = (kxdrproc_t) nfssvc_decode_linkargs,
		.pc_encode = (kxdrproc_t) nfssvc_encode_void,
		.pc_argsize = sizeof(struct nfsd_linkargs),
		.pc_ressize = sizeof(struct nfsd_void),
		.pc_cachetype = RC_REPLSTAT,
		.pc_xdrressize = ST,
	},
	[NFSPROC_SYMLINK] = {
		.pc_func = (svc_procfunc) nfsd_proc_symlink,
		.pc_decode = (kxdrproc_t) nfssvc_decode_symlinkargs,
		.pc_encode = (kxdrproc_t) nfssvc_encode_void,
		.pc_argsize = sizeof(struct nfsd_symlinkargs),
		.pc_ressize = sizeof(struct nfsd_void),
		.pc_cachetype = RC_REPLSTAT,
		.pc_xdrressize = ST,
	},
	[NFSPROC_MKDIR] = {
		.pc_func = (svc_procfunc) nfsd_proc_mkdir,
		.pc_decode = (kxdrproc_t) nfssvc_decode_createargs,
		.pc_encode = (kxdrproc_t) nfssvc_encode_diropres,
		.pc_release = (kxdrproc_t) nfssvc_release_fhandle,
		.pc_argsize = sizeof(struct nfsd_createargs),
		.pc_ressize = sizeof(struct nfsd_diropres),
		.pc_cachetype = RC_REPLBUFF,
		.pc_xdrressize = ST+FH+AT,
	},
	[NFSPROC_RMDIR] = {
		.pc_func = (svc_procfunc) nfsd_proc_rmdir,
		.pc_decode = (kxdrproc_t) nfssvc_decode_diropargs,
		.pc_encode = (kxdrproc_t) nfssvc_encode_void,
		.pc_argsize = sizeof(struct nfsd_diropargs),
		.pc_ressize = sizeof(struct nfsd_void),
		.pc_cachetype = RC_REPLSTAT,
		.pc_xdrressize = ST,
	},
	[NFSPROC_READDIR] = {
		.pc_func = (svc_procfunc) nfsd_proc_readdir,
		.pc_decode = (kxdrproc_t) nfssvc_decode_readdirargs,
		.pc_encode = (kxdrproc_t) nfssvc_encode_readdirres,
		.pc_argsize = sizeof(struct nfsd_readdirargs),
		.pc_ressize = sizeof(struct nfsd_readdirres),
		.pc_cachetype = RC_NOCACHE,
	},
	[NFSPROC_STATFS] = {
		.pc_func = (svc_procfunc) nfsd_proc_statfs,
		.pc_decode = (kxdrproc_t) nfssvc_decode_fhandle,
		.pc_encode = (kxdrproc_t) nfssvc_encode_statfsres,
		.pc_argsize = sizeof(struct nfsd_fhandle),
		.pc_ressize = sizeof(struct nfsd_statfsres),
		.pc_cachetype = RC_NOCACHE,
		.pc_xdrressize = ST+5,
	},
#ifdef MY_ABC_HERE
		{}, 
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		[NFSPROC_SYNO_WRITEZERO] = {
		.pc_func = (svc_procfunc) nfsd_proc_writezero,
		.pc_decode = (kxdrproc_t) nfssvc_decode_writeargs,
		.pc_encode = (kxdrproc_t) nfssvc_encode_attrstat,
		.pc_release = (kxdrproc_t) nfssvc_release_fhandle,
		.pc_argsize = sizeof(struct nfsd_writeargs),
		.pc_ressize = sizeof(struct nfsd_attrstat),
		.pc_cachetype = RC_REPLBUFF,
		.pc_xdrressize = ST+AT,
		},
		[NFSPROC_SYNO_XLOOKUP] = {
		.pc_func = (svc_procfunc) nfsd_proc_xlookup,
		.pc_decode = (kxdrproc_t) nfssvc_decode_diropargs,
		.pc_encode = (kxdrproc_t) nfssvc_encode_diropres,
		.pc_release = (kxdrproc_t) nfssvc_release_fhandle,
		.pc_argsize = sizeof(struct nfsd_diropargs),
		.pc_ressize = sizeof(struct nfsd_diropres),
		.pc_cachetype = RC_NOCACHE,
		.pc_xdrressize = ST+FH+AT,
		},
		[NFSPROC_SYNO_COPY] = {
		.pc_func = (svc_procfunc) nfsd_proc_synocopy,
		.pc_decode = (kxdrproc_t) nfssvc_decode_writeargs,
		.pc_encode = (kxdrproc_t) nfssvc_encode_attrstat,
		.pc_release = (kxdrproc_t) nfssvc_release_fhandle,
		.pc_argsize = sizeof(struct nfsd_writeargs),
		.pc_ressize = sizeof(struct nfsd_attrstat),
		.pc_cachetype = RC_REPLBUFF,
		.pc_xdrressize = ST+AT,
		},
		[NFSPROC_SYNO_SUPPORT] = {
		.pc_func = (svc_procfunc) nfsd_proc_synosupport,
		.pc_decode = (kxdrproc_t) nfssvc_decode_void,
		.pc_encode = (kxdrproc_t) nfssvc_encode_void,
		.pc_argsize = sizeof(struct nfsd_void),
		.pc_ressize = sizeof(struct nfsd_void),
		.pc_cachetype = RC_NOCACHE,
		.pc_xdrressize = ST,
		},
#ifdef MY_ABC_HERE
		[NFSPROC_SYNO_CLONE] = {
		.pc_func = (svc_procfunc) nfsd_proc_synoclone,
		.pc_decode = (kxdrproc_t) nfssvc_decode_writeargs,
		.pc_encode = (kxdrproc_t) nfssvc_encode_attrstat,
		.pc_release = (kxdrproc_t) nfssvc_release_fhandle,
		.pc_argsize = sizeof(struct nfsd_writeargs),
		.pc_ressize = sizeof(struct nfsd_attrstat),
		.pc_cachetype = RC_REPLBUFF,
		.pc_xdrressize = ST+AT,
		},
#endif 
#endif
};

struct svc_version	nfsd_version2 = {
		.vs_vers	= 2,
#ifndef MY_ABC_HERE
		.vs_nproc	= 18,
#else
#ifdef MY_ABC_HERE
		.vs_nproc       = 33,
#else
		.vs_nproc       = 32,
#endif 
#endif
		.vs_proc	= nfsd_procedures2,
		.vs_dispatch	= nfsd_dispatch,
		.vs_xdrsize	= NFS2_SVC_XDRSIZE,
};


__be32
nfserrno (int errno)
{
	static struct {
		__be32	nfserr;
		int	syserr;
	} nfs_errtbl[] = {
		{ nfs_ok, 0 },
		{ nfserr_perm, -EPERM },
		{ nfserr_noent, -ENOENT },
		{ nfserr_io, -EIO },
		{ nfserr_nxio, -ENXIO },
		{ nfserr_fbig, -E2BIG },
		{ nfserr_acces, -EACCES },
		{ nfserr_exist, -EEXIST },
		{ nfserr_xdev, -EXDEV },
		{ nfserr_mlink, -EMLINK },
		{ nfserr_nodev, -ENODEV },
		{ nfserr_notdir, -ENOTDIR },
		{ nfserr_isdir, -EISDIR },
		{ nfserr_inval, -EINVAL },
		{ nfserr_fbig, -EFBIG },
		{ nfserr_nospc, -ENOSPC },
		{ nfserr_rofs, -EROFS },
		{ nfserr_mlink, -EMLINK },
		{ nfserr_nametoolong, -ENAMETOOLONG },
		{ nfserr_notempty, -ENOTEMPTY },
#ifdef EDQUOT
		{ nfserr_dquot, -EDQUOT },
#endif
		{ nfserr_stale, -ESTALE },
		{ nfserr_jukebox, -ETIMEDOUT },
		{ nfserr_jukebox, -ERESTARTSYS },
		{ nfserr_jukebox, -EAGAIN },
		{ nfserr_jukebox, -EWOULDBLOCK },
		{ nfserr_jukebox, -ENOMEM },
		{ nfserr_io, -ETXTBSY },
		{ nfserr_notsupp, -EOPNOTSUPP },
		{ nfserr_toosmall, -ETOOSMALL },
		{ nfserr_serverfault, -ESERVERFAULT },
		{ nfserr_serverfault, -ENFILE },
	};
	int	i;

	for (i = 0; i < ARRAY_SIZE(nfs_errtbl); i++) {
		if (nfs_errtbl[i].syserr == errno)
			return nfs_errtbl[i].nfserr;
	}
	WARN(1, "nfsd: non-standard errno: %d\n", errno);
	return nfserr_io;
}
