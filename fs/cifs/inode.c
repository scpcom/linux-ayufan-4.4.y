#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/fs.h>
#include <linux/stat.h>
#include <linux/pagemap.h>
#include <asm/div64.h>
#include "cifsfs.h"
#include "cifspdu.h"
#include "cifsglob.h"
#include "cifsproto.h"
#include "cifs_debug.h"
#include "cifs_fs_sb.h"

static void cifs_set_ops(struct inode *inode, const bool is_dfs_referral)
{
	struct cifs_sb_info *cifs_sb = CIFS_SB(inode->i_sb);

	switch (inode->i_mode & S_IFMT) {
	case S_IFREG:
		inode->i_op = &cifs_file_inode_ops;
		if (cifs_sb->mnt_cifs_flags & CIFS_MOUNT_DIRECT_IO) {
			if (cifs_sb->mnt_cifs_flags & CIFS_MOUNT_NO_BRL)
				inode->i_fop = &cifs_file_direct_nobrl_ops;
			else
				inode->i_fop = &cifs_file_direct_ops;
		} else if (cifs_sb->mnt_cifs_flags & CIFS_MOUNT_NO_BRL)
			inode->i_fop = &cifs_file_nobrl_ops;
		else {  
			inode->i_fop = &cifs_file_ops;
		}

		if (cifs_sb->tcon->ses->server->maxBuf <
				PAGE_CACHE_SIZE + MAX_CIFS_HDR_SIZE)
			inode->i_data.a_ops = &cifs_addr_ops_smallbuf;
		else
			inode->i_data.a_ops = &cifs_addr_ops;
		break;
	case S_IFDIR:
#ifdef CONFIG_CIFS_DFS_UPCALL
		if (is_dfs_referral) {
			inode->i_op = &cifs_dfs_referral_inode_operations;
		} else {
#else  
		{
#endif
			inode->i_op = &cifs_dir_inode_ops;
			inode->i_fop = &cifs_dir_ops;
		}
		break;
	case S_IFLNK:
		inode->i_op = &cifs_symlink_inode_ops;
		break;
	default:
		init_special_inode(inode, inode->i_mode, inode->i_rdev);
		break;
	}
}

void
cifs_fattr_to_inode(struct inode *inode, struct cifs_fattr *fattr)
{
	struct cifsInodeInfo *cifs_i = CIFS_I(inode);
	struct cifs_sb_info *cifs_sb = CIFS_SB(inode->i_sb);
	unsigned long oldtime = cifs_i->time;

	inode->i_atime = fattr->cf_atime;
	inode->i_mtime = fattr->cf_mtime;
	inode->i_ctime = fattr->cf_ctime;
	inode->i_rdev = fattr->cf_rdev;
	inode->i_nlink = fattr->cf_nlink;
	inode->i_uid = fattr->cf_uid;
	inode->i_gid = fattr->cf_gid;
#ifdef MY_ABC_HERE
	inode->i_CreateTime = cifs_NTtimeToUnix(cpu_to_le64(fattr->cf_createtime));
#endif

	if (inode->i_state & I_NEW ||
	    !(cifs_sb->mnt_cifs_flags & CIFS_MOUNT_DYNPERM))
		inode->i_mode = fattr->cf_mode;

	cifs_i->cifsAttrs = fattr->cf_cifsattrs;
	cifs_i->uniqueid = fattr->cf_uniqueid;

	if (fattr->cf_flags & CIFS_FATTR_NEED_REVAL)
		cifs_i->time = 0;
	else
		cifs_i->time = jiffies;

	cFYI(1, ("inode 0x%p old_time=%ld new_time=%ld", inode,
		 oldtime, cifs_i->time));

	cifs_i->delete_pending = fattr->cf_flags & CIFS_FATTR_DELETE_PENDING;

	spin_lock(&inode->i_lock);
	if (is_size_safe_to_change(cifs_i, fattr->cf_eof)) {
		i_size_write(inode, fattr->cf_eof);

		inode->i_blocks = (512 - 1 + fattr->cf_bytes) >> 9;
	}
	spin_unlock(&inode->i_lock);

	cifs_set_ops(inode, fattr->cf_flags & CIFS_FATTR_DFS_REFERRAL);
}

void
cifs_unix_basic_to_fattr(struct cifs_fattr *fattr, FILE_UNIX_BASIC_INFO *info,
			 struct cifs_sb_info *cifs_sb)
{
	memset(fattr, 0, sizeof(*fattr));
	fattr->cf_uniqueid = le64_to_cpu(info->UniqueId);
	fattr->cf_bytes = le64_to_cpu(info->NumOfBytes);
	fattr->cf_eof = le64_to_cpu(info->EndOfFile);

	fattr->cf_atime = cifs_NTtimeToUnix(info->LastAccessTime);
	fattr->cf_mtime = cifs_NTtimeToUnix(info->LastModificationTime);
	fattr->cf_ctime = cifs_NTtimeToUnix(info->LastStatusChange);
	fattr->cf_mode = le64_to_cpu(info->Permissions);

	fattr->cf_mode &= ~S_IFMT;
	switch (le32_to_cpu(info->Type)) {
	case UNIX_FILE:
		fattr->cf_mode |= S_IFREG;
		fattr->cf_dtype = DT_REG;
		break;
	case UNIX_SYMLINK:
		fattr->cf_mode |= S_IFLNK;
		fattr->cf_dtype = DT_LNK;
		break;
	case UNIX_DIR:
		fattr->cf_mode |= S_IFDIR;
		fattr->cf_dtype = DT_DIR;
		break;
	case UNIX_CHARDEV:
		fattr->cf_mode |= S_IFCHR;
		fattr->cf_dtype = DT_CHR;
		fattr->cf_rdev = MKDEV(le64_to_cpu(info->DevMajor),
				       le64_to_cpu(info->DevMinor) & MINORMASK);
		break;
	case UNIX_BLOCKDEV:
		fattr->cf_mode |= S_IFBLK;
		fattr->cf_dtype = DT_BLK;
		fattr->cf_rdev = MKDEV(le64_to_cpu(info->DevMajor),
				       le64_to_cpu(info->DevMinor) & MINORMASK);
		break;
	case UNIX_FIFO:
		fattr->cf_mode |= S_IFIFO;
		fattr->cf_dtype = DT_FIFO;
		break;
	case UNIX_SOCKET:
		fattr->cf_mode |= S_IFSOCK;
		fattr->cf_dtype = DT_SOCK;
		break;
	default:
		 
		fattr->cf_mode |= S_IFREG;
		fattr->cf_dtype = DT_REG;
		cFYI(1, ("unknown type %d", le32_to_cpu(info->Type)));
		break;
	}

	if (cifs_sb->mnt_cifs_flags & CIFS_MOUNT_OVERR_UID)
		fattr->cf_uid = cifs_sb->mnt_uid;
	else
		fattr->cf_uid = le64_to_cpu(info->Uid);

	if (cifs_sb->mnt_cifs_flags & CIFS_MOUNT_OVERR_GID)
		fattr->cf_gid = cifs_sb->mnt_gid;
	else
		fattr->cf_gid = le64_to_cpu(info->Gid);

	fattr->cf_nlink = le64_to_cpu(info->Nlinks);
}

static void
cifs_create_dfs_fattr(struct cifs_fattr *fattr, struct super_block *sb)
{
	struct cifs_sb_info *cifs_sb = CIFS_SB(sb);

	cFYI(1, ("creating fake fattr for DFS referral"));

	memset(fattr, 0, sizeof(*fattr));
	fattr->cf_mode = S_IFDIR | S_IXUGO | S_IRWXU;
	fattr->cf_uid = cifs_sb->mnt_uid;
	fattr->cf_gid = cifs_sb->mnt_gid;
	fattr->cf_atime = CURRENT_TIME;
	fattr->cf_ctime = CURRENT_TIME;
	fattr->cf_mtime = CURRENT_TIME;
	fattr->cf_nlink = 2;
	fattr->cf_flags |= CIFS_FATTR_DFS_REFERRAL;
}

int cifs_get_inode_info_unix(struct inode **pinode,
			     const unsigned char *full_path,
			     struct super_block *sb, int xid)
{
	int rc;
	FILE_UNIX_BASIC_INFO find_data;
	struct cifs_fattr fattr;
	struct cifsTconInfo *tcon;
	struct cifs_sb_info *cifs_sb = CIFS_SB(sb);

	tcon = cifs_sb->tcon;
	cFYI(1, ("Getting info on %s", full_path));

	rc = CIFSSMBUnixQPathInfo(xid, tcon, full_path, &find_data,
				  cifs_sb->local_nls, cifs_sb->mnt_cifs_flags &
					CIFS_MOUNT_MAP_SPECIAL_CHR);

	if (!rc) {
		cifs_unix_basic_to_fattr(&fattr, &find_data, cifs_sb);
	} else if (rc == -EREMOTE) {
		cifs_create_dfs_fattr(&fattr, sb);
		rc = 0;
	} else {
		return rc;
	}

	if (*pinode == NULL) {
		 
		*pinode = cifs_iget(sb, &fattr);
		if (!*pinode)
			rc = -ENOMEM;
	} else {
		 
		cifs_fattr_to_inode(*pinode, &fattr);
	}

	return rc;
}

static int
cifs_sfu_type(struct cifs_fattr *fattr, const unsigned char *path,
	      struct cifs_sb_info *cifs_sb, int xid)
{
	int rc;
	int oplock = 0;
	__u16 netfid;
	struct cifsTconInfo *pTcon = cifs_sb->tcon;
	char buf[24];
	unsigned int bytes_read;
	char *pbuf;

	pbuf = buf;

	fattr->cf_mode &= ~S_IFMT;

	if (fattr->cf_eof == 0) {
		fattr->cf_mode |= S_IFIFO;
		fattr->cf_dtype = DT_FIFO;
		return 0;
	} else if (fattr->cf_eof < 8) {
		fattr->cf_mode |= S_IFREG;
		fattr->cf_dtype = DT_REG;
		return -EINVAL;	  
	}

	rc = CIFSSMBOpen(xid, pTcon, path, FILE_OPEN, GENERIC_READ,
			 CREATE_NOT_DIR, &netfid, &oplock, NULL,
			 cifs_sb->local_nls,
			 cifs_sb->mnt_cifs_flags &
				CIFS_MOUNT_MAP_SPECIAL_CHR);
	if (rc == 0) {
		int buf_type = CIFS_NO_BUFFER;
			 
		rc = CIFSSMBRead(xid, pTcon, netfid,
				 24  , 0  ,
				 &bytes_read, &pbuf, &buf_type);
		if ((rc == 0) && (bytes_read >= 8)) {
			if (memcmp("IntxBLK", pbuf, 8) == 0) {
				cFYI(1, ("Block device"));
				fattr->cf_mode |= S_IFBLK;
				fattr->cf_dtype = DT_BLK;
				if (bytes_read == 24) {
					 
					__u64 mjr;  
					__u64 mnr;  
					mjr = le64_to_cpu(*(__le64 *)(pbuf+8));
					mnr = le64_to_cpu(*(__le64 *)(pbuf+16));
					fattr->cf_rdev = MKDEV(mjr, mnr);
				}
			} else if (memcmp("IntxCHR", pbuf, 8) == 0) {
				cFYI(1, ("Char device"));
				fattr->cf_mode |= S_IFCHR;
				fattr->cf_dtype = DT_CHR;
				if (bytes_read == 24) {
					 
					__u64 mjr;  
					__u64 mnr;  
					mjr = le64_to_cpu(*(__le64 *)(pbuf+8));
					mnr = le64_to_cpu(*(__le64 *)(pbuf+16));
					fattr->cf_rdev = MKDEV(mjr, mnr);
				}
			} else if (memcmp("IntxLNK", pbuf, 7) == 0) {
				cFYI(1, ("Symlink"));
				fattr->cf_mode |= S_IFLNK;
				fattr->cf_dtype = DT_LNK;
			} else {
				fattr->cf_mode |= S_IFREG;  
				fattr->cf_dtype = DT_REG;
				rc = -EOPNOTSUPP;
			}
		} else {
			fattr->cf_mode |= S_IFREG;  
			fattr->cf_dtype = DT_REG;
			rc = -EOPNOTSUPP;  
		}
		CIFSSMBClose(xid, pTcon, netfid);
	}
	return rc;
}

#define SFBITS_MASK (S_ISVTX | S_ISGID | S_ISUID)   

static int cifs_sfu_mode(struct cifs_fattr *fattr, const unsigned char *path,
			 struct cifs_sb_info *cifs_sb, int xid)
{
#ifdef CONFIG_CIFS_XATTR
	ssize_t rc;
	char ea_value[4];
	__u32 mode;

	rc = CIFSSMBQueryEA(xid, cifs_sb->tcon, path, "SETFILEBITS",
			    ea_value, 4  , cifs_sb->local_nls,
			    cifs_sb->mnt_cifs_flags &
				CIFS_MOUNT_MAP_SPECIAL_CHR);
	if (rc < 0)
		return (int)rc;
	else if (rc > 3) {
		mode = le32_to_cpu(*((__le32 *)ea_value));
		fattr->cf_mode &= ~SFBITS_MASK;
		cFYI(1, ("special bits 0%o org mode 0%o", mode,
			 fattr->cf_mode));
		fattr->cf_mode = (mode & SFBITS_MASK) | fattr->cf_mode;
		cFYI(1, ("special mode bits 0%o", mode));
	}

	return 0;
#else
	return -EOPNOTSUPP;
#endif
}

static void
cifs_all_info_to_fattr(struct cifs_fattr *fattr, FILE_ALL_INFO *info,
		       struct cifs_sb_info *cifs_sb, bool adjust_tz)
{
	memset(fattr, 0, sizeof(*fattr));
	fattr->cf_cifsattrs = le32_to_cpu(info->Attributes);
	if (info->DeletePending)
		fattr->cf_flags |= CIFS_FATTR_DELETE_PENDING;

	if (info->LastAccessTime)
		fattr->cf_atime = cifs_NTtimeToUnix(info->LastAccessTime);
	else
		fattr->cf_atime = CURRENT_TIME;

	fattr->cf_ctime = cifs_NTtimeToUnix(info->ChangeTime);
	fattr->cf_mtime = cifs_NTtimeToUnix(info->LastWriteTime);

	if (adjust_tz) {
		fattr->cf_ctime.tv_sec += cifs_sb->tcon->ses->server->timeAdj;
		fattr->cf_mtime.tv_sec += cifs_sb->tcon->ses->server->timeAdj;
	}

	fattr->cf_eof = le64_to_cpu(info->EndOfFile);
	fattr->cf_bytes = le64_to_cpu(info->AllocationSize);
	fattr->cf_createtime = le64_to_cpu(info->CreationTime);

	if (fattr->cf_cifsattrs & ATTR_DIRECTORY) {
		fattr->cf_mode = S_IFDIR | cifs_sb->mnt_dir_mode;
		fattr->cf_dtype = DT_DIR;
	} else {
		fattr->cf_mode = S_IFREG | cifs_sb->mnt_file_mode;
		fattr->cf_dtype = DT_REG;

		if (fattr->cf_cifsattrs & ATTR_READONLY)
			fattr->cf_mode &= ~(S_IWUGO);
	}

	fattr->cf_nlink = le32_to_cpu(info->NumberOfLinks);

	fattr->cf_uid = cifs_sb->mnt_uid;
	fattr->cf_gid = cifs_sb->mnt_gid;
}

int cifs_get_inode_info(struct inode **pinode,
	const unsigned char *full_path, FILE_ALL_INFO *pfindData,
	struct super_block *sb, int xid, const __u16 *pfid)
{
	int rc = 0, tmprc;
	struct cifsTconInfo *pTcon;
	struct cifs_sb_info *cifs_sb = CIFS_SB(sb);
	char *buf = NULL;
	bool adjustTZ = false;
	struct cifs_fattr fattr;

	pTcon = cifs_sb->tcon;
	cFYI(1, ("Getting info on %s", full_path));

	if ((pfindData == NULL) && (*pinode != NULL)) {
		if (CIFS_I(*pinode)->clientCanCacheRead) {
			cFYI(1, ("No need to revalidate cached inode sizes"));
			return rc;
		}
	}

	if (pfindData == NULL) {
		buf = kmalloc(sizeof(FILE_ALL_INFO), GFP_KERNEL);
		if (buf == NULL)
			return -ENOMEM;
		pfindData = (FILE_ALL_INFO *)buf;

		rc = CIFSSMBQPathInfo(xid, pTcon, full_path, pfindData,
			      0  ,
			      cifs_sb->local_nls, cifs_sb->mnt_cifs_flags &
				CIFS_MOUNT_MAP_SPECIAL_CHR);
		 
		if ((rc == -EOPNOTSUPP) || (rc == -EINVAL)) {
			rc = SMBQueryInformation(xid, pTcon, full_path,
					pfindData, cifs_sb->local_nls,
					cifs_sb->mnt_cifs_flags &
					  CIFS_MOUNT_MAP_SPECIAL_CHR);
			adjustTZ = true;
		}
	}

	if (!rc) {
		cifs_all_info_to_fattr(&fattr, (FILE_ALL_INFO *) pfindData,
				       cifs_sb, adjustTZ);
	} else if (rc == -EREMOTE) {
		cifs_create_dfs_fattr(&fattr, sb);
		rc = 0;
	} else {
		goto cgii_exit;
	}

	if (*pinode == NULL) {
		if (cifs_sb->mnt_cifs_flags & CIFS_MOUNT_SERVER_INUM) {
			int rc1 = 0;

			rc1 = CIFSGetSrvInodeNumber(xid, pTcon,
					full_path, &fattr.cf_uniqueid,
					cifs_sb->local_nls,
					cifs_sb->mnt_cifs_flags &
						CIFS_MOUNT_MAP_SPECIAL_CHR);
			if (rc1 || !fattr.cf_uniqueid) {
				cFYI(1, ("GetSrvInodeNum rc %d", rc1));
				fattr.cf_uniqueid = iunique(sb, ROOT_I);
				cifs_autodisable_serverino(cifs_sb);
			}
		} else {
			fattr.cf_uniqueid = iunique(sb, ROOT_I);
		}
	} else {
		fattr.cf_uniqueid = CIFS_I(*pinode)->uniqueid;
	}

	if (fattr.cf_cifsattrs & ATTR_SYSTEM &&
	    cifs_sb->mnt_cifs_flags & CIFS_MOUNT_UNX_EMUL) {
		tmprc = cifs_sfu_type(&fattr, full_path, cifs_sb, xid);
		if (tmprc)
			cFYI(1, ("cifs_sfu_type failed: %d", tmprc));
	}

#ifdef CONFIG_CIFS_EXPERIMENTAL
	 
	if (cifs_sb->mnt_cifs_flags & CIFS_MOUNT_CIFS_ACL) {
		cFYI(1, ("Getting mode bits from ACL"));
		cifs_acl_to_fattr(cifs_sb, &fattr, *pinode, full_path, pfid);
	}
#endif

	if (cifs_sb->mnt_cifs_flags & CIFS_MOUNT_UNX_EMUL)
		cifs_sfu_mode(&fattr, full_path, cifs_sb, xid);

	if (!*pinode) {
		*pinode = cifs_iget(sb, &fattr);
		if (!*pinode)
			rc = -ENOMEM;
	} else {
		cifs_fattr_to_inode(*pinode, &fattr);
	}

cgii_exit:
	kfree(buf);
	return rc;
}

static const struct inode_operations cifs_ipc_inode_ops = {
	.lookup = cifs_lookup,
};

char *cifs_build_path_to_root(struct cifs_sb_info *cifs_sb)
{
	int pplen = cifs_sb->prepathlen;
	int dfsplen;
	char *full_path = NULL;

	if (pplen == 0) {
		full_path = kmalloc(1, GFP_KERNEL);
		if (full_path)
			full_path[0] = 0;
		return full_path;
	}

	if (cifs_sb->tcon && (cifs_sb->tcon->Flags & SMB_SHARE_IS_IN_DFS))
		dfsplen = strnlen(cifs_sb->tcon->treeName, MAX_TREE_SIZE + 1);
	else
		dfsplen = 0;

	full_path = kmalloc(dfsplen + pplen + 1, GFP_KERNEL);
	if (full_path == NULL)
		return full_path;

	if (dfsplen) {
		strncpy(full_path, cifs_sb->tcon->treeName, dfsplen);
		 
		if (cifs_sb->mnt_cifs_flags & CIFS_MOUNT_POSIX_PATHS) {
			int i;
			for (i = 0; i < dfsplen; i++) {
				if (full_path[i] == '\\')
					full_path[i] = '/';
			}
		}
	}
	strncpy(full_path + dfsplen, cifs_sb->prepath, pplen);
	full_path[dfsplen + pplen] = 0;  
	return full_path;
}

static int
cifs_find_inode(struct inode *inode, void *opaque)
{
	struct cifs_fattr *fattr = (struct cifs_fattr *) opaque;

	if (CIFS_I(inode)->uniqueid != fattr->cf_uniqueid)
		return 0;

	if (CIFS_I(inode)->createtime != fattr->cf_createtime)
		return 0;

	return 1;
}

static int
cifs_init_inode(struct inode *inode, void *opaque)
{
	struct cifs_fattr *fattr = (struct cifs_fattr *) opaque;

	CIFS_I(inode)->uniqueid = fattr->cf_uniqueid;
	CIFS_I(inode)->createtime = fattr->cf_createtime;
	return 0;
}

struct inode *
cifs_iget(struct super_block *sb, struct cifs_fattr *fattr)
{
	unsigned long hash;
	struct inode *inode;

	cFYI(1, ("looking for uniqueid=%llu", fattr->cf_uniqueid));

	hash = cifs_uniqueid_to_ino_t(fattr->cf_uniqueid);

	inode = iget5_locked(sb, hash, cifs_find_inode, cifs_init_inode, fattr);

	if (inode) {
		cifs_fattr_to_inode(inode, fattr);
		if (sb->s_flags & MS_NOATIME)
			inode->i_flags |= S_NOATIME | S_NOCMTIME;
		if (inode->i_state & I_NEW) {
			inode->i_ino = hash;
			unlock_new_inode(inode);
		}
	}

	return inode;
}

struct inode *cifs_root_iget(struct super_block *sb, unsigned long ino)
{
	int xid;
	struct cifs_sb_info *cifs_sb;
	struct inode *inode = NULL;
	long rc;
	char *full_path;

	cifs_sb = CIFS_SB(sb);
	full_path = cifs_build_path_to_root(cifs_sb);
	if (full_path == NULL)
		return ERR_PTR(-ENOMEM);

	xid = GetXid();
	if (cifs_sb->tcon->unix_ext)
		rc = cifs_get_inode_info_unix(&inode, full_path, sb, xid);
	else
		rc = cifs_get_inode_info(&inode, full_path, NULL, sb,
						xid, NULL);

	if (!inode)
		return ERR_PTR(-ENOMEM);

	if (rc && cifs_sb->tcon->ipc) {
		cFYI(1, ("ipc connection - fake read inode"));
		inode->i_mode |= S_IFDIR;
		inode->i_nlink = 2;
		inode->i_op = &cifs_ipc_inode_ops;
		inode->i_fop = &simple_dir_operations;
		inode->i_uid = cifs_sb->mnt_uid;
		inode->i_gid = cifs_sb->mnt_gid;
	} else if (rc) {
		kfree(full_path);
		_FreeXid(xid);
		iget_failed(inode);
		return ERR_PTR(rc);
	}

	kfree(full_path);
	 
	_FreeXid(xid);
	return inode;
}

static int
cifs_set_file_info(struct inode *inode, struct iattr *attrs, int xid,
		    char *full_path, __u32 dosattr)
{
	int rc;
	int oplock = 0;
	__u16 netfid;
	__u32 netpid;
	bool set_time = false;
	struct cifsFileInfo *open_file;
	struct cifsInodeInfo *cifsInode = CIFS_I(inode);
	struct cifs_sb_info *cifs_sb = CIFS_SB(inode->i_sb);
	struct cifsTconInfo *pTcon = cifs_sb->tcon;
	FILE_BASIC_INFO	info_buf;

	if (attrs == NULL)
		return -EINVAL;

	if (attrs->ia_valid & ATTR_ATIME) {
		set_time = true;
		info_buf.LastAccessTime =
			cpu_to_le64(cifs_UnixTimeToNT(attrs->ia_atime));
	} else
		info_buf.LastAccessTime = 0;

	if (attrs->ia_valid & ATTR_MTIME) {
		set_time = true;
		info_buf.LastWriteTime =
		    cpu_to_le64(cifs_UnixTimeToNT(attrs->ia_mtime));
	} else
		info_buf.LastWriteTime = 0;

	if (set_time && (attrs->ia_valid & ATTR_CTIME)) {
		cFYI(1, ("CIFS - CTIME changed"));
		info_buf.ChangeTime =
		    cpu_to_le64(cifs_UnixTimeToNT(attrs->ia_ctime));
	} else
		info_buf.ChangeTime = 0;

	info_buf.CreationTime = 0;	 
	info_buf.Attributes = cpu_to_le32(dosattr);

	open_file = find_writable_file(cifsInode);
	if (open_file) {
		netfid = open_file->netfid;
		netpid = open_file->pid;
		goto set_via_filehandle;
	}

	if (!(pTcon->ses->flags & CIFS_SES_NT4)) {
		rc = CIFSSMBSetPathInfo(xid, pTcon, full_path,
				     &info_buf, cifs_sb->local_nls,
				     cifs_sb->mnt_cifs_flags &
					CIFS_MOUNT_MAP_SPECIAL_CHR);
		if (rc == 0) {
			cifsInode->cifsAttrs = dosattr;
			goto out;
		} else if (rc != -EOPNOTSUPP && rc != -EINVAL)
			goto out;
	}

	cFYI(1, ("calling SetFileInfo since SetPathInfo for "
		 "times not supported by this server"));
	rc = CIFSSMBOpen(xid, pTcon, full_path, FILE_OPEN,
			 SYNCHRONIZE | FILE_WRITE_ATTRIBUTES,
			 CREATE_NOT_DIR, &netfid, &oplock,
			 NULL, cifs_sb->local_nls,
			 cifs_sb->mnt_cifs_flags &
				CIFS_MOUNT_MAP_SPECIAL_CHR);

	if (rc != 0) {
		if (rc == -EIO)
			rc = -EINVAL;
		goto out;
	}

	netpid = current->tgid;

set_via_filehandle:
	rc = CIFSSMBSetFileInfo(xid, pTcon, &info_buf, netfid, netpid);
	if (!rc)
		cifsInode->cifsAttrs = dosattr;

	if (open_file == NULL)
		CIFSSMBClose(xid, pTcon, netfid);
	else
		cifsFileInfo_put(open_file);
out:
	return rc;
}

static int
cifs_rename_pending_delete(char *full_path, struct dentry *dentry, int xid)
{
	int oplock = 0;
	int rc;
	__u16 netfid;
	struct inode *inode = dentry->d_inode;
	struct cifsInodeInfo *cifsInode = CIFS_I(inode);
	struct cifs_sb_info *cifs_sb = CIFS_SB(inode->i_sb);
	struct cifsTconInfo *tcon = cifs_sb->tcon;
	__u32 dosattr, origattr;
	FILE_BASIC_INFO *info_buf = NULL;

	rc = CIFSSMBOpen(xid, tcon, full_path, FILE_OPEN,
			 DELETE|FILE_WRITE_ATTRIBUTES, CREATE_NOT_DIR,
			 &netfid, &oplock, NULL, cifs_sb->local_nls,
			 cifs_sb->mnt_cifs_flags & CIFS_MOUNT_MAP_SPECIAL_CHR);
	if (rc != 0)
		goto out;

	origattr = cifsInode->cifsAttrs;
	if (origattr == 0)
		origattr |= ATTR_NORMAL;

	dosattr = origattr & ~ATTR_READONLY;
	if (dosattr == 0)
		dosattr |= ATTR_NORMAL;
	dosattr |= ATTR_HIDDEN;

	if (dosattr != origattr) {
		info_buf = kzalloc(sizeof(*info_buf), GFP_KERNEL);
		if (info_buf == NULL) {
			rc = -ENOMEM;
			goto out_close;
		}
		info_buf->Attributes = cpu_to_le32(dosattr);
		rc = CIFSSMBSetFileInfo(xid, tcon, info_buf, netfid,
					current->tgid);
		 
		if (rc != 0)
			cifsInode->cifsAttrs = dosattr;
		else
			dosattr = origattr;  
	}

	rc = CIFSSMBRenameOpenFile(xid, tcon, netfid, NULL, cifs_sb->local_nls,
				   cifs_sb->mnt_cifs_flags &
					    CIFS_MOUNT_MAP_SPECIAL_CHR);
	if (rc != 0) {
		rc = -ETXTBSY;
		goto undo_setattr;
	}

	if (!cifsInode->delete_pending) {
		rc = CIFSSMBSetFileDisposition(xid, tcon, true, netfid,
					       current->tgid);
		 
		if (rc == -ENOENT)
			rc = 0;
		else if (rc != 0) {
			rc = -ETXTBSY;
			goto undo_rename;
		}
		cifsInode->delete_pending = true;
	}

out_close:
	CIFSSMBClose(xid, tcon, netfid);
out:
	kfree(info_buf);
	return rc;

undo_rename:
	CIFSSMBRenameOpenFile(xid, tcon, netfid, dentry->d_name.name,
				cifs_sb->local_nls, cifs_sb->mnt_cifs_flags &
					    CIFS_MOUNT_MAP_SPECIAL_CHR);
undo_setattr:
	if (dosattr != origattr) {
		info_buf->Attributes = cpu_to_le32(origattr);
		if (!CIFSSMBSetFileInfo(xid, tcon, info_buf, netfid,
					current->tgid))
			cifsInode->cifsAttrs = origattr;
	}

	goto out_close;
}

int cifs_unlink(struct inode *dir, struct dentry *dentry)
{
	int rc = 0;
	int xid;
	char *full_path = NULL;
	struct inode *inode = dentry->d_inode;
	struct cifsInodeInfo *cifs_inode;
	struct super_block *sb = dir->i_sb;
	struct cifs_sb_info *cifs_sb = CIFS_SB(sb);
	struct cifsTconInfo *tcon = cifs_sb->tcon;
	struct iattr *attrs = NULL;
	__u32 dosattr = 0, origattr = 0;

	cFYI(1, ("cifs_unlink, dir=0x%p, dentry=0x%p", dir, dentry));

	xid = GetXid();

	full_path = build_path_from_dentry(dentry);
	if (full_path == NULL) {
		rc = -ENOMEM;
		FreeXid(xid);
		return rc;
	}

	if ((tcon->ses->capabilities & CAP_UNIX) &&
		(CIFS_UNIX_POSIX_PATH_OPS_CAP &
			le64_to_cpu(tcon->fsUnixInfo.Capability))) {
		rc = CIFSPOSIXDelFile(xid, tcon, full_path,
			SMB_POSIX_UNLINK_FILE_TARGET, cifs_sb->local_nls,
			cifs_sb->mnt_cifs_flags & CIFS_MOUNT_MAP_SPECIAL_CHR);
		cFYI(1, ("posix del rc %d", rc));
		if ((rc == 0) || (rc == -ENOENT))
			goto psx_del_no_retry;
	}

retry_std_delete:
	rc = CIFSSMBDelFile(xid, tcon, full_path, cifs_sb->local_nls,
			cifs_sb->mnt_cifs_flags & CIFS_MOUNT_MAP_SPECIAL_CHR);

psx_del_no_retry:
	if (!rc) {
		if (inode)
			drop_nlink(inode);
	} else if (rc == -ENOENT) {
		d_drop(dentry);
	} else if (rc == -ETXTBSY) {
		rc = cifs_rename_pending_delete(full_path, dentry, xid);
		if (rc == 0)
			drop_nlink(inode);
	} else if ((rc == -EACCES) && (dosattr == 0) && inode) {
		attrs = kzalloc(sizeof(*attrs), GFP_KERNEL);
		if (attrs == NULL) {
			rc = -ENOMEM;
			goto out_reval;
		}

		cifs_inode = CIFS_I(inode);
		origattr = cifs_inode->cifsAttrs;
		if (origattr == 0)
			origattr |= ATTR_NORMAL;
		dosattr = origattr & ~ATTR_READONLY;
		if (dosattr == 0)
			dosattr |= ATTR_NORMAL;
		dosattr |= ATTR_HIDDEN;

		rc = cifs_set_file_info(inode, attrs, xid, full_path, dosattr);
		if (rc != 0)
			goto out_reval;

		goto retry_std_delete;
	}

	if (rc != 0 && dosattr != 0)
		cifs_set_file_info(inode, attrs, xid, full_path, origattr);

out_reval:
	if (inode) {
		cifs_inode = CIFS_I(inode);
		cifs_inode->time = 0;	 
		inode->i_ctime = current_fs_time(sb);
	}
	dir->i_ctime = dir->i_mtime = current_fs_time(sb);
	cifs_inode = CIFS_I(dir);
	CIFS_I(dir)->time = 0;	 

	kfree(full_path);
	kfree(attrs);
	FreeXid(xid);
	return rc;
}

int cifs_mkdir(struct inode *inode, struct dentry *direntry, int mode)
{
	int rc = 0, tmprc;
	int xid;
	struct cifs_sb_info *cifs_sb;
	struct cifsTconInfo *pTcon;
	char *full_path = NULL;
	struct inode *newinode = NULL;
	struct cifs_fattr fattr;

	cFYI(1, ("In cifs_mkdir, mode = 0x%x inode = 0x%p", mode, inode));

	xid = GetXid();

	cifs_sb = CIFS_SB(inode->i_sb);
	pTcon = cifs_sb->tcon;

	full_path = build_path_from_dentry(direntry);
	if (full_path == NULL) {
		rc = -ENOMEM;
		FreeXid(xid);
		return rc;
	}

	if ((pTcon->ses->capabilities & CAP_UNIX) &&
		(CIFS_UNIX_POSIX_PATH_OPS_CAP &
			le64_to_cpu(pTcon->fsUnixInfo.Capability))) {
		u32 oplock = 0;
		FILE_UNIX_BASIC_INFO *pInfo =
			kzalloc(sizeof(FILE_UNIX_BASIC_INFO), GFP_KERNEL);
		if (pInfo == NULL) {
			rc = -ENOMEM;
			goto mkdir_out;
		}

		mode &= ~current_umask();
		rc = CIFSPOSIXCreate(xid, pTcon, SMB_O_DIRECTORY | SMB_O_CREAT,
				mode, NULL  , pInfo, &oplock,
				full_path, cifs_sb->local_nls,
				cifs_sb->mnt_cifs_flags &
					CIFS_MOUNT_MAP_SPECIAL_CHR);
		if (rc == -EOPNOTSUPP) {
			kfree(pInfo);
			goto mkdir_retry_old;
		} else if (rc) {
			cFYI(1, ("posix mkdir returned 0x%x", rc));
			d_drop(direntry);
		} else {
			if (pInfo->Type == cpu_to_le32(-1)) {
				 
				kfree(pInfo);
				goto mkdir_get_info;
			}
 
			inc_nlink(inode);
			if (pTcon->nocase)
				direntry->d_op = &cifs_ci_dentry_ops;
			else
				direntry->d_op = &cifs_dentry_ops;

			cifs_unix_basic_to_fattr(&fattr, pInfo, cifs_sb);
			newinode = cifs_iget(inode->i_sb, &fattr);
			if (!newinode) {
				kfree(pInfo);
				goto mkdir_get_info;
			}

			d_instantiate(direntry, newinode);

#ifdef CONFIG_CIFS_DEBUG2
			cFYI(1, ("instantiated dentry %p %s to inode %p",
				direntry, direntry->d_name.name, newinode));

			if (newinode->i_nlink != 2)
				cFYI(1, ("unexpected number of links %d",
					newinode->i_nlink));
#endif
		}
		kfree(pInfo);
		goto mkdir_out;
	}
mkdir_retry_old:
	 
	rc = CIFSSMBMkDir(xid, pTcon, full_path, cifs_sb->local_nls,
			  cifs_sb->mnt_cifs_flags & CIFS_MOUNT_MAP_SPECIAL_CHR);
	if (rc) {
		cFYI(1, ("cifs_mkdir returned 0x%x", rc));
		d_drop(direntry);
	} else {
mkdir_get_info:
		inc_nlink(inode);
		if (pTcon->unix_ext)
			rc = cifs_get_inode_info_unix(&newinode, full_path,
						      inode->i_sb, xid);
		else
			rc = cifs_get_inode_info(&newinode, full_path, NULL,
						 inode->i_sb, xid, NULL);

		if (pTcon->nocase)
			direntry->d_op = &cifs_ci_dentry_ops;
		else
			direntry->d_op = &cifs_dentry_ops;
		d_instantiate(direntry, newinode);
		  
		if ((direntry->d_inode) && (direntry->d_inode->i_nlink < 2))
				direntry->d_inode->i_nlink = 2;

		mode &= ~current_umask();
		 
		if (inode->i_mode & S_ISGID)
			mode |= S_ISGID;

		if (pTcon->unix_ext) {
			struct cifs_unix_set_info_args args = {
				.mode	= mode,
				.ctime	= NO_CHANGE_64,
				.atime	= NO_CHANGE_64,
				.mtime	= NO_CHANGE_64,
				.device	= 0,
			};
			if (cifs_sb->mnt_cifs_flags & CIFS_MOUNT_SET_UID) {
				args.uid = (__u64)current_fsuid();
				if (inode->i_mode & S_ISGID)
					args.gid = (__u64)inode->i_gid;
				else
					args.gid = (__u64)current_fsgid();
			} else {
				args.uid = NO_CHANGE_64;
				args.gid = NO_CHANGE_64;
			}
			CIFSSMBUnixSetPathInfo(xid, pTcon, full_path, &args,
					       cifs_sb->local_nls,
					       cifs_sb->mnt_cifs_flags &
						CIFS_MOUNT_MAP_SPECIAL_CHR);
		} else {
			if (!(cifs_sb->mnt_cifs_flags & CIFS_MOUNT_CIFS_ACL) &&
			    (mode & S_IWUGO) == 0) {
				FILE_BASIC_INFO pInfo;
				struct cifsInodeInfo *cifsInode;
				u32 dosattrs;

				memset(&pInfo, 0, sizeof(pInfo));
				cifsInode = CIFS_I(newinode);
				dosattrs = cifsInode->cifsAttrs|ATTR_READONLY;
				pInfo.Attributes = cpu_to_le32(dosattrs);
				tmprc = CIFSSMBSetPathInfo(xid, pTcon,
						full_path, &pInfo,
						cifs_sb->local_nls,
						cifs_sb->mnt_cifs_flags &
						CIFS_MOUNT_MAP_SPECIAL_CHR);
				if (tmprc == 0)
					cifsInode->cifsAttrs = dosattrs;
			}
			if (direntry->d_inode) {
				if (cifs_sb->mnt_cifs_flags &
				     CIFS_MOUNT_DYNPERM)
					direntry->d_inode->i_mode =
						(mode | S_IFDIR);

				if (cifs_sb->mnt_cifs_flags &
				     CIFS_MOUNT_SET_UID) {
					direntry->d_inode->i_uid =
						current_fsuid();
					if (inode->i_mode & S_ISGID)
						direntry->d_inode->i_gid =
							inode->i_gid;
					else
						direntry->d_inode->i_gid =
							current_fsgid();
				}
			}
		}
	}
mkdir_out:
	kfree(full_path);
	FreeXid(xid);
	return rc;
}

int cifs_rmdir(struct inode *inode, struct dentry *direntry)
{
	int rc = 0;
	int xid;
	struct cifs_sb_info *cifs_sb;
	struct cifsTconInfo *pTcon;
	char *full_path = NULL;
	struct cifsInodeInfo *cifsInode;

	cFYI(1, ("cifs_rmdir, inode = 0x%p", inode));

	xid = GetXid();

	cifs_sb = CIFS_SB(inode->i_sb);
	pTcon = cifs_sb->tcon;

	full_path = build_path_from_dentry(direntry);
	if (full_path == NULL) {
		rc = -ENOMEM;
		FreeXid(xid);
		return rc;
	}

	rc = CIFSSMBRmDir(xid, pTcon, full_path, cifs_sb->local_nls,
			  cifs_sb->mnt_cifs_flags & CIFS_MOUNT_MAP_SPECIAL_CHR);

	if (!rc) {
		drop_nlink(inode);
		spin_lock(&direntry->d_inode->i_lock);
		i_size_write(direntry->d_inode, 0);
		clear_nlink(direntry->d_inode);
		spin_unlock(&direntry->d_inode->i_lock);
	}

	cifsInode = CIFS_I(direntry->d_inode);
	cifsInode->time = 0;	 

	cifsInode = CIFS_I(inode);
	cifsInode->time = 0;	 

	direntry->d_inode->i_ctime = inode->i_ctime = inode->i_mtime =
		current_fs_time(inode->i_sb);

	kfree(full_path);
	FreeXid(xid);
	return rc;
}

static int
cifs_do_rename(int xid, struct dentry *from_dentry, const char *fromPath,
		struct dentry *to_dentry, const char *toPath)
{
	struct cifs_sb_info *cifs_sb = CIFS_SB(from_dentry->d_sb);
	struct cifsTconInfo *pTcon = cifs_sb->tcon;
	__u16 srcfid;
	int oplock, rc;

	rc = CIFSSMBRename(xid, pTcon, fromPath, toPath, cifs_sb->local_nls,
			   cifs_sb->mnt_cifs_flags &
				CIFS_MOUNT_MAP_SPECIAL_CHR);

	if (rc == 0 || rc != -ETXTBSY)
		return rc;

	rc = CIFSSMBOpen(xid, pTcon, fromPath, FILE_OPEN, DELETE,
			 CREATE_NOT_DIR, &srcfid, &oplock, NULL,
			 cifs_sb->local_nls, cifs_sb->mnt_cifs_flags &
				CIFS_MOUNT_MAP_SPECIAL_CHR);

	if (rc == 0) {
		rc = CIFSSMBRenameOpenFile(xid, pTcon, srcfid,
				(const char *) to_dentry->d_name.name,
				cifs_sb->local_nls, cifs_sb->mnt_cifs_flags &
					CIFS_MOUNT_MAP_SPECIAL_CHR);

		CIFSSMBClose(xid, pTcon, srcfid);
	}

	return rc;
}

int cifs_rename(struct inode *source_dir, struct dentry *source_dentry,
	struct inode *target_dir, struct dentry *target_dentry)
{
	char *fromName = NULL;
	char *toName = NULL;
	struct cifs_sb_info *cifs_sb_source;
	struct cifs_sb_info *cifs_sb_target;
	struct cifsTconInfo *tcon;
	FILE_UNIX_BASIC_INFO *info_buf_source = NULL;
	FILE_UNIX_BASIC_INFO *info_buf_target;
	int xid, rc, tmprc;

	cifs_sb_target = CIFS_SB(target_dir->i_sb);
	cifs_sb_source = CIFS_SB(source_dir->i_sb);
	tcon = cifs_sb_source->tcon;

	xid = GetXid();

	if (tcon != cifs_sb_target->tcon) {
		rc = -EXDEV;
		goto cifs_rename_exit;
	}

	fromName = build_path_from_dentry(source_dentry);
	if (fromName == NULL) {
		rc = -ENOMEM;
		goto cifs_rename_exit;
	}

	toName = build_path_from_dentry(target_dentry);
	if (toName == NULL) {
		rc = -ENOMEM;
		goto cifs_rename_exit;
	}

	rc = cifs_do_rename(xid, source_dentry, fromName,
			    target_dentry, toName);

	if (rc == -EEXIST && tcon->unix_ext) {
		 
		info_buf_source =
			kmalloc(2 * sizeof(FILE_UNIX_BASIC_INFO),
					GFP_KERNEL);
		if (info_buf_source == NULL) {
			rc = -ENOMEM;
			goto cifs_rename_exit;
		}

		info_buf_target = info_buf_source + 1;
		tmprc = CIFSSMBUnixQPathInfo(xid, tcon, fromName,
					info_buf_source,
					cifs_sb_source->local_nls,
					cifs_sb_source->mnt_cifs_flags &
					CIFS_MOUNT_MAP_SPECIAL_CHR);
		if (tmprc != 0)
			goto unlink_target;

		tmprc = CIFSSMBUnixQPathInfo(xid, tcon,
					toName, info_buf_target,
					cifs_sb_target->local_nls,
					 
					cifs_sb_source->mnt_cifs_flags &
					CIFS_MOUNT_MAP_SPECIAL_CHR);

		if (tmprc == 0 && (info_buf_source->UniqueId ==
				   info_buf_target->UniqueId)) {
			 
			rc = 0;
			goto cifs_rename_exit;
		}
	}  

unlink_target:
	 
	if (target_dentry->d_inode && (rc == -EACCES || rc == -EEXIST)) {
		tmprc = cifs_unlink(target_dir, target_dentry);
		if (tmprc)
			goto cifs_rename_exit;

		rc = cifs_do_rename(xid, source_dentry, fromName,
				    target_dentry, toName);
	}

cifs_rename_exit:
	kfree(info_buf_source);
	kfree(fromName);
	kfree(toName);
	FreeXid(xid);
	return rc;
}

int cifs_revalidate(struct dentry *direntry)
{
	int xid;
	int rc = 0, wbrc = 0;
	char *full_path;
	struct cifs_sb_info *cifs_sb;
	struct cifsInodeInfo *cifsInode;
	loff_t local_size;
	struct timespec local_mtime;
	bool invalidate_inode = false;

	if (direntry->d_inode == NULL)
		return -ENOENT;

	cifsInode = CIFS_I(direntry->d_inode);

	if (cifsInode == NULL)
		return -ENOENT;

	if (CIFS_I(direntry->d_inode)->clientCanCacheRead)
		return rc;

	xid = GetXid();

	cifs_sb = CIFS_SB(direntry->d_sb);

	full_path = build_path_from_dentry(direntry);
	if (full_path == NULL) {
		rc = -ENOMEM;
		FreeXid(xid);
		return rc;
	}
	cFYI(1, ("Revalidate: %s inode 0x%p count %d dentry: 0x%p d_time %ld "
		 "jiffies %ld", full_path, direntry->d_inode,
		 direntry->d_inode->i_count.counter, direntry,
		 direntry->d_time, jiffies));

	if (cifsInode->time == 0) {
		 
	} else if (time_before(jiffies, cifsInode->time + HZ) &&
		   lookupCacheEnabled) {
		if ((S_ISREG(direntry->d_inode->i_mode) == 0) ||
		    (direntry->d_inode->i_nlink == 1)) {
			kfree(full_path);
			FreeXid(xid);
			return rc;
		} else {
			cFYI(1, ("Have to revalidate file due to hardlinks"));
		}
	}

	local_mtime = direntry->d_inode->i_mtime;
	local_size = direntry->d_inode->i_size;

	if (cifs_sb->tcon->unix_ext) {
		rc = cifs_get_inode_info_unix(&direntry->d_inode, full_path,
					      direntry->d_sb, xid);
		if (rc) {
			cFYI(1, ("error on getting revalidate info %d", rc));
 	 
		}
	} else {
		rc = cifs_get_inode_info(&direntry->d_inode, full_path, NULL,
					 direntry->d_sb, xid, NULL);
		if (rc) {
			cFYI(1, ("error on getting revalidate info %d", rc));
 	 
		}
	}
	 
	if (timespec_equal(&local_mtime, &direntry->d_inode->i_mtime) &&
	    (local_size == direntry->d_inode->i_size)) {
		cFYI(1, ("cifs_revalidate - inode unchanged"));
	} else {
		 
		if (cifsInode->clientCanCacheRead) {
			 
		} else {
			invalidate_inode = true;
		}
	}

	if (direntry->d_inode->i_mapping) {
		 
		wbrc = filemap_fdatawrite(direntry->d_inode->i_mapping);
		if (wbrc)
			CIFS_I(direntry->d_inode)->write_behind_rc = wbrc;
	}
	if (invalidate_inode) {
	 
		if (S_ISREG(direntry->d_inode->i_mode)) {
			if (direntry->d_inode->i_mapping) {
				wbrc = filemap_fdatawait(direntry->d_inode->i_mapping);
				if (wbrc)
					CIFS_I(direntry->d_inode)->write_behind_rc = wbrc;
			}
			 
			if (list_empty(&(cifsInode->openFileList))) {
				 
				cFYI(1, ("Invalidating read ahead data on "
					 "closed file"));
				invalidate_remote_inode(direntry->d_inode);
			}
		}
	}
 
	kfree(full_path);
	FreeXid(xid);
	return rc;
}

int cifs_getattr(struct vfsmount *mnt, struct dentry *dentry,
	struct kstat *stat)
{
	int err = cifs_revalidate(dentry);
	if (!err) {
		generic_fillattr(dentry->d_inode, stat);
		stat->blksize = CIFS_MAX_MSGSIZE;
		stat->ino = CIFS_I(dentry->d_inode)->uniqueid;
	}
	return err;
}

static int cifs_truncate_page(struct address_space *mapping, loff_t from)
{
	pgoff_t index = from >> PAGE_CACHE_SHIFT;
	unsigned offset = from & (PAGE_CACHE_SIZE - 1);
	struct page *page;
	int rc = 0;

	page = grab_cache_page(mapping, index);
	if (!page)
		return -ENOMEM;

	zero_user_segment(page, offset, PAGE_CACHE_SIZE);
	unlock_page(page);
	page_cache_release(page);
	return rc;
}

static int cifs_vmtruncate(struct inode *inode, loff_t offset)
{
	loff_t oldsize;
	int err;

	spin_lock(&inode->i_lock);
	err = inode_newsize_ok(inode, offset);
	if (err) {
		spin_unlock(&inode->i_lock);
		goto out;
	}

	oldsize = inode->i_size;
	i_size_write(inode, offset);
	spin_unlock(&inode->i_lock);
	truncate_pagecache(inode, oldsize, offset);
	if (inode->i_op->truncate)
		inode->i_op->truncate(inode);
out:
	return err;
}

static int
cifs_set_file_size(struct inode *inode, struct iattr *attrs,
		   int xid, char *full_path)
{
	int rc;
	struct cifsFileInfo *open_file;
	struct cifsInodeInfo *cifsInode = CIFS_I(inode);
	struct cifs_sb_info *cifs_sb = CIFS_SB(inode->i_sb);
	struct cifsTconInfo *pTcon = cifs_sb->tcon;

	open_file = find_writable_file(cifsInode);
	if (open_file) {
		__u16 nfid = open_file->netfid;
		__u32 npid = open_file->pid;
		rc = CIFSSMBSetFileSize(xid, pTcon, attrs->ia_size, nfid,
					npid, false);
		cifsFileInfo_put(open_file);
		cFYI(1, ("SetFSize for attrs rc = %d", rc));
		if ((rc == -EINVAL) || (rc == -EOPNOTSUPP)) {
			unsigned int bytes_written;
			rc = CIFSSMBWrite(xid, pTcon, nfid, 0, attrs->ia_size,
					  &bytes_written, NULL, NULL, 1);
			cFYI(1, ("Wrt seteof rc %d", rc));
		}
	} else
		rc = -EINVAL;

	if (rc != 0) {
		 
		rc = CIFSSMBSetEOF(xid, pTcon, full_path, attrs->ia_size,
				   false, cifs_sb->local_nls,
				   cifs_sb->mnt_cifs_flags &
					CIFS_MOUNT_MAP_SPECIAL_CHR);
		cFYI(1, ("SetEOF by path (setattrs) rc = %d", rc));
		if ((rc == -EINVAL) || (rc == -EOPNOTSUPP)) {
			__u16 netfid;
			int oplock = 0;

			rc = SMBLegacyOpen(xid, pTcon, full_path,
				FILE_OPEN, GENERIC_WRITE,
				CREATE_NOT_DIR, &netfid, &oplock, NULL,
				cifs_sb->local_nls,
				cifs_sb->mnt_cifs_flags &
					CIFS_MOUNT_MAP_SPECIAL_CHR);
			if (rc == 0) {
				unsigned int bytes_written;
				rc = CIFSSMBWrite(xid, pTcon, netfid, 0,
						  attrs->ia_size,
						  &bytes_written, NULL,
						  NULL, 1);
				cFYI(1, ("wrt seteof rc %d", rc));
				CIFSSMBClose(xid, pTcon, netfid);
			}
		}
	}

	if (rc == 0) {
		cifsInode->server_eof = attrs->ia_size;
		rc = cifs_vmtruncate(inode, attrs->ia_size);
		cifs_truncate_page(inode->i_mapping, inode->i_size);
	}

	return rc;
}

static int
cifs_setattr_unix(struct dentry *direntry, struct iattr *attrs)
{
	int rc;
	int xid;
	char *full_path = NULL;
	struct inode *inode = direntry->d_inode;
	struct cifsInodeInfo *cifsInode = CIFS_I(inode);
	struct cifs_sb_info *cifs_sb = CIFS_SB(inode->i_sb);
	struct cifsTconInfo *pTcon = cifs_sb->tcon;
	struct cifs_unix_set_info_args *args = NULL;
	struct cifsFileInfo *open_file;

	cFYI(1, ("setattr_unix on file %s attrs->ia_valid=0x%x",
		 direntry->d_name.name, attrs->ia_valid));

	xid = GetXid();

	if ((cifs_sb->mnt_cifs_flags & CIFS_MOUNT_NO_PERM) == 0) {
		 
		rc = inode_change_ok(inode, attrs);
		if (rc < 0)
			goto out;
		else
			rc = 0;
	}

	full_path = build_path_from_dentry(direntry);
	if (full_path == NULL) {
		rc = -ENOMEM;
		goto out;
	}

	rc = filemap_write_and_wait(inode->i_mapping);
	if (rc != 0) {
		cifsInode->write_behind_rc = rc;
		rc = 0;
	}

	if (attrs->ia_valid & ATTR_SIZE) {
		rc = cifs_set_file_size(inode, attrs, xid, full_path);
		if (rc != 0)
			goto out;
	}

	if (attrs->ia_valid & (ATTR_KILL_SUID|ATTR_KILL_SGID))
		attrs->ia_valid &= ~ATTR_MODE;

	args = kmalloc(sizeof(*args), GFP_KERNEL);
	if (args == NULL) {
		rc = -ENOMEM;
		goto out;
	}

	if (attrs->ia_valid & ATTR_MODE)
		args->mode = attrs->ia_mode;
	else
		args->mode = NO_CHANGE_64;

	if (attrs->ia_valid & ATTR_UID)
		args->uid = attrs->ia_uid;
	else
		args->uid = NO_CHANGE_64;

	if (attrs->ia_valid & ATTR_GID)
		args->gid = attrs->ia_gid;
	else
		args->gid = NO_CHANGE_64;

	if (attrs->ia_valid & ATTR_ATIME)
		args->atime = cifs_UnixTimeToNT(attrs->ia_atime);
	else
		args->atime = NO_CHANGE_64;

	if (attrs->ia_valid & ATTR_MTIME)
		args->mtime = cifs_UnixTimeToNT(attrs->ia_mtime);
	else
		args->mtime = NO_CHANGE_64;

	if (attrs->ia_valid & ATTR_CTIME)
		args->ctime = cifs_UnixTimeToNT(attrs->ia_ctime);
	else
		args->ctime = NO_CHANGE_64;

	args->device = 0;
	open_file = find_writable_file(cifsInode);
	if (open_file) {
		u16 nfid = open_file->netfid;
		u32 npid = open_file->pid;
		rc = CIFSSMBUnixSetFileInfo(xid, pTcon, args, nfid, npid);
		cifsFileInfo_put(open_file);
	} else {
		rc = CIFSSMBUnixSetPathInfo(xid, pTcon, full_path, args,
				    cifs_sb->local_nls,
				    cifs_sb->mnt_cifs_flags &
					CIFS_MOUNT_MAP_SPECIAL_CHR);
	}

	if (!rc)
		rc = inode_setattr(inode, attrs);
out:
	kfree(args);
	kfree(full_path);
	FreeXid(xid);
	return rc;
}

static int
cifs_setattr_nounix(struct dentry *direntry, struct iattr *attrs)
{
	int xid;
	struct inode *inode = direntry->d_inode;
	struct cifs_sb_info *cifs_sb = CIFS_SB(inode->i_sb);
	struct cifsInodeInfo *cifsInode = CIFS_I(inode);
	char *full_path = NULL;
	int rc = -EACCES;
	__u32 dosattr = 0;
	__u64 mode = NO_CHANGE_64;

	xid = GetXid();

	cFYI(1, ("setattr on file %s attrs->iavalid 0x%x",
		 direntry->d_name.name, attrs->ia_valid));

	if ((cifs_sb->mnt_cifs_flags & CIFS_MOUNT_NO_PERM) == 0) {
		 
		rc = inode_change_ok(inode, attrs);
		if (rc < 0) {
			FreeXid(xid);
			return rc;
		} else
			rc = 0;
	}

	full_path = build_path_from_dentry(direntry);
	if (full_path == NULL) {
		rc = -ENOMEM;
		FreeXid(xid);
		return rc;
	}

	rc = filemap_write_and_wait(inode->i_mapping);
	if (rc != 0) {
		cifsInode->write_behind_rc = rc;
		rc = 0;
	}

	if (attrs->ia_valid & ATTR_SIZE) {
		rc = cifs_set_file_size(inode, attrs, xid, full_path);
		if (rc != 0)
			goto cifs_setattr_exit;
	}

	if (!(cifs_sb->mnt_cifs_flags & CIFS_MOUNT_SET_UID))
		attrs->ia_valid &= ~(ATTR_UID | ATTR_GID);

	if (attrs->ia_valid & (ATTR_KILL_SUID|ATTR_KILL_SGID))
		attrs->ia_valid &= ~ATTR_MODE;

	if (attrs->ia_valid & ATTR_MODE) {
		cFYI(1, ("Mode changed to 0%o", attrs->ia_mode));
		mode = attrs->ia_mode;
	}

	if (attrs->ia_valid & ATTR_MODE) {
		rc = 0;
#ifdef CONFIG_CIFS_EXPERIMENTAL
		if (cifs_sb->mnt_cifs_flags & CIFS_MOUNT_CIFS_ACL)
			rc = mode_to_acl(inode, full_path, mode);
		else
#endif
		if (((mode & S_IWUGO) == 0) &&
		    (cifsInode->cifsAttrs & ATTR_READONLY) == 0) {

			dosattr = cifsInode->cifsAttrs | ATTR_READONLY;

			if ((cifs_sb->mnt_cifs_flags & CIFS_MOUNT_DYNPERM) == 0)
				attrs->ia_mode = inode->i_mode & ~S_IWUGO;
		} else if ((mode & S_IWUGO) &&
			   (cifsInode->cifsAttrs & ATTR_READONLY)) {

			dosattr = cifsInode->cifsAttrs & ~ATTR_READONLY;
			 
			if (dosattr == 0)
				dosattr |= ATTR_NORMAL;

			if (!(cifs_sb->mnt_cifs_flags & CIFS_MOUNT_DYNPERM)) {
				attrs->ia_mode &= ~(S_IALLUGO);
				if (S_ISDIR(inode->i_mode))
					attrs->ia_mode |=
						cifs_sb->mnt_dir_mode;
				else
					attrs->ia_mode |=
						cifs_sb->mnt_file_mode;
			}
		} else if (!(cifs_sb->mnt_cifs_flags & CIFS_MOUNT_DYNPERM)) {
			 
			attrs->ia_valid &= ~ATTR_MODE;
		}
	}

	if (attrs->ia_valid & (ATTR_MTIME|ATTR_ATIME|ATTR_CTIME) ||
	    ((attrs->ia_valid & ATTR_MODE) && dosattr)) {
		rc = cifs_set_file_info(inode, attrs, xid, full_path, dosattr);
		 
		if ((rc) && (attrs->ia_valid &
				(ATTR_MODE | ATTR_GID | ATTR_UID | ATTR_SIZE)))
			rc = 0;
	}

	if (!rc)
		rc = inode_setattr(inode, attrs);
cifs_setattr_exit:
	kfree(full_path);
	FreeXid(xid);
	return rc;
}

int
cifs_setattr(struct dentry *direntry, struct iattr *attrs)
{
	struct inode *inode = direntry->d_inode;
	struct cifs_sb_info *cifs_sb = CIFS_SB(inode->i_sb);
	struct cifsTconInfo *pTcon = cifs_sb->tcon;

	if (pTcon->unix_ext)
		return cifs_setattr_unix(direntry, attrs);

	return cifs_setattr_nounix(direntry, attrs);

}

#if 0
void cifs_delete_inode(struct inode *inode)
{
	cFYI(1, ("In cifs_delete_inode, inode = 0x%p", inode));
	 
}
#endif
