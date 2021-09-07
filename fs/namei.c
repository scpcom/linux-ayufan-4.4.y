#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/quotaops.h>
#include <linux/pagemap.h>
#include <linux/fsnotify.h>
#include <linux/personality.h>
#include <linux/security.h>
#include <linux/ima.h>
#include <linux/syscalls.h>
#include <linux/mount.h>
#include <linux/audit.h>
#include <linux/capability.h>
#include <linux/file.h>
#include <linux/fcntl.h>
#include <linux/device_cgroup.h>
#include <linux/fs_struct.h>
#include <asm/uaccess.h>

#ifdef CONFIG_FS_SYNO_ACL
#include "synoacl_int.h"
#endif

#define ACC_MODE(x) ("\000\004\002\006"[(x)&O_ACCMODE])

#ifdef MY_ABC_HERE
int SYNOUnicodeUTF8ChrToUTF16Chr(u_int16_t *p, const u_int8_t *s, int n);
int SYNOUnicodeUTF16ChrToUTF8Chr(u_int8_t *s, u_int16_t wc, int maxlen);
u_int16_t *SYNOUnicodeGenerateDefaultUpcaseTable(void);
u_int16_t *DefUpcaseTable(void);

struct utf8_table {
	int     cmask;
	int     cval;
	int     shift;
	long    lmask;
	long    lval;
};

static struct utf8_table utf8_table[] =
{
    {0x80,  0x00,   0*6,    0x7F,           0,          },
    {0xE0,  0xC0,   1*6,    0x7FF,          0x80,       },
    {0xF0,  0xE0,   2*6,    0xFFFF,         0x800,      },
    {0xF8,  0xF0,   3*6,    0x1FFFFF,       0x10000,    },
    {0xFC,  0xF8,   4*6,    0x3FFFFFF,      0x200000,   },
    {0xFE,  0xFC,   5*6,    0x7FFFFFFF,     0x4000000,  },
    {0,						        }
};

int SYNOUnicodeUTF8ChrToUTF16Chr(u_int16_t *p, const u_int8_t *s, int n)
{
	long l;
	int c0, c, nc;
	struct utf8_table *t;

	nc = 0;
	c0 = *s;
	l = c0;
	for (t = utf8_table; t->cmask; t++) {
		nc++;
		if ((c0 & t->cmask) == t->cval) {
			l &= t->lmask;
			if (l < t->lval)
				return -1;
			*p = l;
			return nc;
		}
		if (n <= nc)
			return -1;
		s++;
		c = (*s ^ 0x80) & 0xFF;
		if (c & 0xC0)
			return -1;
		l = (l << 6) | c;
	}
	return -1;
}

int SYNOUnicodeUTF16ChrToUTF8Chr(u_int8_t *s, u_int16_t wc, int maxlen)
{
	long l;
	int c, nc;
	struct utf8_table *t;

	if (s == 0)
		return 0;

	l = wc;
	nc = 0;
	for (t = utf8_table; t->cmask && maxlen; t++, maxlen--) {
		nc++;
		if (l <= t->lmask) {
			c = t->shift;
			*s = t->cval | (l >> c);
			while (c > 0) {
				c -= 6;
				s++;
				*s = 0x80 | ((l >> c) & 0x3F);
			}
			return nc;
		}
	}
	return -1;
}

static u_int16_t gUC[UTF16_UPCASE_TABLE_SIZE];

u_int16_t *SYNOUnicodeGenerateDefaultUpcaseTable(void)
{
	const int uc_run_table[][3] = {  
	{0x0061, 0x007B,  -32}, {0x0451, 0x045D, -80}, {0x1F70, 0x1F72,  74},
	{0x00E0, 0x00F7,  -32}, {0x045E, 0x0460, -80}, {0x1F72, 0x1F76,  86},
	{0x00F8, 0x00FF,  -32}, {0x0561, 0x0587, -48}, {0x1F76, 0x1F78, 100},
	{0x0256, 0x0258, -205}, {0x1F00, 0x1F08,   8}, {0x1F78, 0x1F7A, 128},
	{0x028A, 0x028C, -217}, {0x1F10, 0x1F16,   8}, {0x1F7A, 0x1F7C, 112},
	{0x03AC, 0x03AD,  -38}, {0x1F20, 0x1F28,   8}, {0x1F7C, 0x1F7E, 126},
	{0x03AD, 0x03B0,  -37}, {0x1F30, 0x1F38,   8}, {0x1FB0, 0x1FB2,   8},
	{0x03B1, 0x03C2,  -32}, {0x1F40, 0x1F46,   8}, {0x1FD0, 0x1FD2,   8},
	{0x03C2, 0x03C3,  -31}, {0x1F51, 0x1F52,   8}, {0x1FE0, 0x1FE2,   8},
	{0x03C3, 0x03CC,  -32}, {0x1F53, 0x1F54,   8}, {0x1FE5, 0x1FE6,   7},
	{0x03CC, 0x03CD,  -64}, {0x1F55, 0x1F56,   8}, {0x2170, 0x2180, -16},
	{0x03CD, 0x03CF,  -63}, {0x1F57, 0x1F58,   8}, {0x24D0, 0x24EA, -26},
	{0x0430, 0x0450,  -32}, {0x1F60, 0x1F68,   8}, {0xFF41, 0xFF5B, -32},
	{0}
	};

	const int uc_dup_table[][2] = {  
	{0x0100, 0x012F}, {0x01A0, 0x01A6}, {0x03E2, 0x03EF}, {0x04CB, 0x04CC},
	{0x0132, 0x0137}, {0x01B3, 0x01B7}, {0x0460, 0x0481}, {0x04D0, 0x04EB},
	{0x0139, 0x0149}, {0x01CD, 0x01DD}, {0x0490, 0x04BF}, {0x04EE, 0x04F5},
	{0x014A, 0x0178}, {0x01DE, 0x01EF}, {0x04BF, 0x04BF}, {0x04F8, 0x04F9},
	{0x0179, 0x017E}, {0x01F4, 0x01F5}, {0x04C1, 0x04C4}, {0x1E00, 0x1E95},
	{0x018B, 0x018B}, {0x01FA, 0x0218}, {0x04C7, 0x04C8}, {0x1EA0, 0x1EF9},
	{0}
	};

	const int uc_word_table[][2] = {  
	{0x00FF, 0x0178}, {0x01AD, 0x01AC}, {0x01F3, 0x01F1}, {0x0269, 0x0196},
	{0x0183, 0x0182}, {0x01B0, 0x01AF}, {0x0253, 0x0181}, {0x026F, 0x019C},
	{0x0185, 0x0184}, {0x01B9, 0x01B8}, {0x0254, 0x0186}, {0x0272, 0x019D},
	{0x0188, 0x0187}, {0x01BD, 0x01BC}, {0x0259, 0x018F}, {0x0275, 0x019F},
	{0x018C, 0x018B}, {0x01C6, 0x01C4}, {0x025B, 0x0190}, {0x0283, 0x01A9},
	{0x0192, 0x0191}, {0x01C9, 0x01C7}, {0x0260, 0x0193}, {0x0288, 0x01AE},
	{0x0199, 0x0198}, {0x01CC, 0x01CA}, {0x0263, 0x0194}, {0x0292, 0x01B7},
	{0x01A8, 0x01A7}, {0x01DD, 0x018E}, {0x0268, 0x0197},
	{0}
	};

#ifdef MY_ABC_HERE
	 
	const int uc_icu_table[][2] = {	 
	{0x00B5, 0x039C}, {0x0131, 0x0049}, {0x017F, 0x0053}, {0x0195, 0x01F6},
	{0x019E, 0x0220}, {0x01BF, 0x01F7}, {0x01C5, 0x01C4}, {0x01C8, 0x01C7},
	{0x01CB, 0x01CA}, {0x01F2, 0x01F1}, {0x01F9, 0x01F8}, {0x0219, 0x0218},
	{0x021B, 0x021A}, {0x021D, 0x021C}, {0x021F, 0x021E}, {0x0223, 0x0222},
	{0x0225, 0x0224}, {0x0227, 0x0226}, {0x0229, 0x0228}, {0x022B, 0x022A},
	{0x022D, 0x022C}, {0x022F, 0x022E}, {0x0231, 0x0230}, {0x0233, 0x0232},
	{0x0280, 0x01A6}, {0x0345, 0x0399}, {0x03D0, 0x0392}, {0x03D1, 0x0398},
	{0x03D5, 0x03A6}, {0x03D6, 0x03A0}, {0x03D9, 0x03D8}, {0x03DB, 0x03DA},
	{0x03DD, 0x03DC}, {0x03DF, 0x03DE}, {0x03E1, 0x03E0}, {0x03F0, 0x039A},
	{0x03F1, 0x03A1}, {0x03F2, 0x03A3}, {0x03F5, 0x0395}, {0x0450, 0x0400},
	{0x045D, 0x040D}, {0x048B, 0x048A}, {0x048D, 0x048C}, {0x048F, 0x048E},
	{0x04C6, 0x04C5}, {0x04CA, 0x04C9}, {0x04CE, 0x04CD}, {0x04ED, 0x04EC},
	{0x0501, 0x0500}, {0x0503, 0x0502}, {0x0505, 0x0504}, {0x0507, 0x0506},
	{0x0509, 0x0508}, {0x050B, 0x050A}, {0x050D, 0x050C}, {0x050F, 0x050E},
	{0x1E9B, 0x1E60}, {0x1FBE, 0x0399},
	{0}
	};
#endif  
	int i, r;
	u_int16_t *uc;

	uc = gUC;

	memset(uc, 0, UTF16_UPCASE_TABLE_SIZE * sizeof(u_int16_t));

	for (i = 0; i < UTF16_UPCASE_TABLE_SIZE; i++)
		uc[i] = i;
	for (r = 0; uc_run_table[r][0]; r++)
		for (i = uc_run_table[r][0]; i < uc_run_table[r][1]; i++)
			uc[i] = uc[i] + uc_run_table[r][2];
	for (r = 0; uc_dup_table[r][0]; r++)
		for (i = uc_dup_table[r][0]; i < uc_dup_table[r][1]; i += 2)
			uc[i + 1] = uc[i + 1] - 1;
	for (r = 0; uc_word_table[r][0]; r++)
		uc[uc_word_table[r][0]] = uc_word_table[r][1];
#ifdef MY_ABC_HERE
	for (r = 0; uc_icu_table[r][0]; r++)
		uc[uc_icu_table[r][0]] = uc_icu_table[r][1];
#endif  
	return uc;
}

static u_int16_t *UpcaseTable = NULL;

u_int16_t *DefUpcaseTable()
{
    if(UpcaseTable==NULL)
        UpcaseTable = SYNOUnicodeGenerateDefaultUpcaseTable();

    return UpcaseTable;
}

int SYNOUnicodeUTF8toUpper(u_int8_t *to,const u_int8_t *from, int maxlen, int clenfrom, u_int16_t *upcasetable)
{
        u_int16_t *UpcaseTbl;
        u_int16_t wc;
        u_int8_t *op;
        int size;

        UpcaseTbl = (upcasetable==NULL) ? DefUpcaseTable() : upcasetable;

        op = to;
        while (clenfrom && maxlen) {
                size = SYNOUnicodeUTF8ChrToUTF16Chr(&wc, from, clenfrom);
                if (size == -1) {
                        from++;
                        clenfrom--;
                        continue;
                } else {
                        from += size;
                        clenfrom -= size;
                }
                size = SYNOUnicodeUTF16ChrToUTF8Chr(op, UpcaseTbl[wc], maxlen);
                if (size == -1) {
                        continue;
                } else {
                        op += size;
                        maxlen -= size;
                }
        }
        *op = 0;
        return (op - to);
}
EXPORT_SYMBOL(SYNOUnicodeUTF8toUpper);

int SYNOUnicodeUTF8Strcmp(const u_int8_t *utf8str1,const u_int8_t *utf8str2,int clenUtf8Str1, int clenUtf8Str2, u_int16_t *upcasetable)
{
        u_int16_t *UpcaseTbl;
        u_int16_t wc1, wc2;
        int size1, size2;
        int result = -1;

        UpcaseTbl = (upcasetable==NULL) ? DefUpcaseTable() : upcasetable;

        while (clenUtf8Str1 && clenUtf8Str2) {
                size1 = SYNOUnicodeUTF8ChrToUTF16Chr(&wc1, utf8str1, clenUtf8Str1);
                size2 = SYNOUnicodeUTF8ChrToUTF16Chr(&wc2, utf8str2, clenUtf8Str2);

                if (size1 != -1 && size2 != -1) {
                        if (UpcaseTbl[wc1] != UpcaseTbl[wc2])
                                goto END;
                } else if (size1 == -1 && size2 == -1) {
                        if (*utf8str1 != *utf8str2)
                                goto END;
                        size1 = size2 = 1;
                } else {
                        goto END;
                }
                utf8str1 += size1;
                clenUtf8Str1 -= size1;
                utf8str2 += size2;
                clenUtf8Str2 -= size2;
        }
	if (clenUtf8Str1 == 0 && clenUtf8Str2 == 0)
		result = 0;
END:
        return result;
}
EXPORT_SYMBOL(SYNOUnicodeUTF8Strcmp);

asmlinkage long sys_SYNOUnicodeLoadTbl(u_int16_t *rgUCTable)
{
     
    return 0;
}

#endif  

static int __link_path_walk(const char *name, struct nameidata *nd);

static int do_getname(const char __user *filename, char *page)
{
	int retval;
	unsigned long len = PATH_MAX;

	if (!segment_eq(get_fs(), KERNEL_DS)) {
		if ((unsigned long) filename >= TASK_SIZE)
			return -EFAULT;
		if (TASK_SIZE - (unsigned long) filename < PATH_MAX)
			len = TASK_SIZE - (unsigned long) filename;
	}

	retval = strncpy_from_user(page, filename, len);
	if (retval > 0) {
		if (retval < len)
			return 0;
		return -ENAMETOOLONG;
	} else if (!retval)
		retval = -ENOENT;
	return retval;
}

char * getname(const char __user * filename)
{
	char *tmp, *result;

	result = ERR_PTR(-ENOMEM);
	tmp = __getname();
	if (tmp)  {
		int retval = do_getname(filename, tmp);

		result = tmp;
		if (retval < 0) {
			__putname(tmp);
			result = ERR_PTR(retval);
		}
	}
	audit_getname(result);
	return result;
}

#ifdef CONFIG_AUDITSYSCALL
void putname(const char *name)
{
	if (unlikely(!audit_dummy_context()))
		audit_putname(name);
	else
		__putname(name);
}
EXPORT_SYMBOL(putname);
#endif

static int acl_permission_check(struct inode *inode, int mask,
		int (*check_acl)(struct inode *inode, int mask))
{
	umode_t			mode = inode->i_mode;

	mask &= MAY_READ | MAY_WRITE | MAY_EXEC;

	if (current_fsuid() == inode->i_uid)
		mode >>= 6;
	else {
		if (IS_POSIXACL(inode) && (mode & S_IRWXG) && check_acl) {
			int error = check_acl(inode, mask);
			if (error != -EAGAIN)
				return error;
		}

		if (in_group_p(inode->i_gid))
			mode >>= 3;
	}

	if ((mask & ~mode) == 0)
		return 0;
	return -EACCES;
}

int generic_permission(struct inode *inode, int mask,
		int (*check_acl)(struct inode *inode, int mask))
{
	int ret;

	ret = acl_permission_check(inode, mask, check_acl);
	if (ret != -EACCES)
		return ret;

	if (!(mask & MAY_EXEC) || execute_ok(inode))
		if (capable(CAP_DAC_OVERRIDE))
			return 0;

	mask &= MAY_READ | MAY_WRITE | MAY_EXEC;
	if (mask == MAY_READ || (S_ISDIR(inode->i_mode) && !(mask & MAY_WRITE)))
		if (capable(CAP_DAC_READ_SEARCH))
			return 0;

	return -EACCES;
}

int inode_permission(struct inode *inode, int mask)
{
	int retval;

	if (mask & MAY_WRITE) {
		umode_t mode = inode->i_mode;

		if (IS_RDONLY(inode) &&
		    (S_ISREG(mode) || S_ISDIR(mode) || S_ISLNK(mode)))
			return -EROFS;

		if (IS_IMMUTABLE(inode))
			return -EACCES;
	}

	if (inode->i_op->permission)
		retval = inode->i_op->permission(inode, mask);
	else
		retval = generic_permission(inode, mask, inode->i_op->check_acl);

	if (retval)
		return retval;

	retval = devcgroup_inode_permission(inode, mask);
	if (retval)
		return retval;

	return security_inode_permission(inode,
			mask & (MAY_READ|MAY_WRITE|MAY_EXEC|MAY_APPEND));
}

int file_permission(struct file *file, int mask)
{
#ifdef CONFIG_FS_SYNO_ACL
	struct inode * inode = file->f_path.dentry->d_inode;
	if (IS_SYNOACL(inode)) {
		return synoacl_op_perm(file->f_path.dentry, mask);
	}
	return inode_permission(inode, mask);
#else 
	return inode_permission(file->f_path.dentry->d_inode, mask);
#endif
}

int get_write_access(struct inode * inode)
{
	spin_lock(&inode->i_lock);
	if (atomic_read(&inode->i_writecount) < 0) {
		spin_unlock(&inode->i_lock);
		return -ETXTBSY;
	}
	atomic_inc(&inode->i_writecount);
	spin_unlock(&inode->i_lock);

	return 0;
}

int deny_write_access(struct file * file)
{
	struct inode *inode = file->f_path.dentry->d_inode;

	spin_lock(&inode->i_lock);
	if (atomic_read(&inode->i_writecount) > 0) {
		spin_unlock(&inode->i_lock);
		return -ETXTBSY;
	}
	atomic_dec(&inode->i_writecount);
	spin_unlock(&inode->i_lock);

	return 0;
}
#ifdef CONFIG_AUFS_FS
EXPORT_SYMBOL(deny_write_access);
#endif  

void path_get(struct path *path)
{
	mntget(path->mnt);
	dget(path->dentry);
}
EXPORT_SYMBOL(path_get);

void path_put(struct path *path)
{
	dput(path->dentry);
	mntput(path->mnt);
}
EXPORT_SYMBOL(path_put);

void release_open_intent(struct nameidata *nd)
{
	if (nd->intent.open.file->f_path.dentry == NULL)
		put_filp(nd->intent.open.file);
	else
		fput(nd->intent.open.file);
}

static inline struct dentry *
do_revalidate(struct dentry *dentry, struct nameidata *nd)
{
	int status = dentry->d_op->d_revalidate(dentry, nd);
	if (unlikely(status <= 0)) {
		 
		if (!status) {
			if (!d_invalidate(dentry)) {
				dput(dentry);
				dentry = NULL;
			}
		} else {
			dput(dentry);
			dentry = ERR_PTR(status);
		}
	}
	return dentry;
}

static struct dentry * cached_lookup(struct dentry * parent, struct qstr * name, struct nameidata *nd)
{
	struct dentry * dentry = __d_lookup(parent, name);

	if (!dentry)
		dentry = d_lookup(parent, name);

	if (dentry && dentry->d_op && dentry->d_op->d_revalidate)
		dentry = do_revalidate(dentry, nd);

	return dentry;
}

static int exec_permission_lite(struct inode *inode)
{
	int ret;

	if (inode->i_op->permission) {
		ret = inode->i_op->permission(inode, MAY_EXEC);
		if (!ret)
			goto ok;
		return ret;
	}
	ret = acl_permission_check(inode, MAY_EXEC, inode->i_op->check_acl);
	if (!ret)
		goto ok;

	if (capable(CAP_DAC_OVERRIDE) || capable(CAP_DAC_READ_SEARCH))
		goto ok;

	return ret;
ok:
	return security_inode_permission(inode, MAY_EXEC);
}

static struct dentry * real_lookup(struct dentry * parent, struct qstr * name, struct nameidata *nd)
{
	struct dentry * result;
	struct inode *dir = parent->d_inode;

	mutex_lock(&dir->i_mutex);
	 
	result = d_lookup(parent, name);
	if (!result) {
		struct dentry *dentry;

		result = ERR_PTR(-ENOENT);
		if (IS_DEADDIR(dir))
			goto out_unlock;

		dentry = d_alloc(parent, name);
		result = ERR_PTR(-ENOMEM);
		if (dentry) {
			result = dir->i_op->lookup(dir, dentry, nd);
			if (result)
				dput(dentry);
			else
				result = dentry;
		}
out_unlock:
		mutex_unlock(&dir->i_mutex);
		return result;
	}

	mutex_unlock(&dir->i_mutex);
	if (result->d_op && result->d_op->d_revalidate) {
		result = do_revalidate(result, nd);
		if (!result)
			result = ERR_PTR(-ENOENT);
	}
	return result;
}

static __always_inline int link_path_walk(const char *name, struct nameidata *nd)
{
	struct path save = nd->path;
	int result;

	path_get(&save);

	result = __link_path_walk(name, nd);
	if (result == -ESTALE) {
		 
		nd->path = save;
		path_get(&nd->path);
		nd->flags |= LOOKUP_REVAL;
		result = __link_path_walk(name, nd);
	}

	path_put(&save);

	return result;
}

static __always_inline void set_root(struct nameidata *nd)
{
	if (!nd->root.mnt) {
		struct fs_struct *fs = current->fs;
		read_lock(&fs->lock);
		nd->root = fs->root;
		path_get(&nd->root);
		read_unlock(&fs->lock);
	}
}

static __always_inline int __vfs_follow_link(struct nameidata *nd, const char *link)
{
	int res = 0;
	char *name;
	if (IS_ERR(link))
		goto fail;

	if (*link == '/') {
		set_root(nd);
		path_put(&nd->path);
		nd->path = nd->root;
		path_get(&nd->root);
	}

	res = link_path_walk(link, nd);
	if (nd->depth || res || nd->last_type!=LAST_NORM)
		return res;
	 
	name = __getname();
	if (unlikely(!name)) {
		path_put(&nd->path);
		return -ENOMEM;
	}
	strcpy(name, nd->last.name);
	nd->last.name = name;
	return 0;
fail:
	path_put(&nd->path);
	return PTR_ERR(link);
}

static void path_put_conditional(struct path *path, struct nameidata *nd)
{
	dput(path->dentry);
	if (path->mnt != nd->path.mnt)
		mntput(path->mnt);
}

static inline void path_to_nameidata(struct path *path, struct nameidata *nd)
{
	dput(nd->path.dentry);
	if (nd->path.mnt != path->mnt)
		mntput(nd->path.mnt);
	nd->path.mnt = path->mnt;
	nd->path.dentry = path->dentry;
}

static __always_inline int __do_follow_link(struct path *path, struct nameidata *nd)
{
	int error;
	void *cookie;
	struct dentry *dentry = path->dentry;

	touch_atime(path->mnt, dentry);
	nd_set_link(nd, NULL);

	if (path->mnt != nd->path.mnt) {
		path_to_nameidata(path, nd);
		dget(dentry);
	}
	mntget(path->mnt);
	cookie = dentry->d_inode->i_op->follow_link(dentry, nd);
	error = PTR_ERR(cookie);
	if (!IS_ERR(cookie)) {
		char *s = nd_get_link(nd);
		error = 0;
		if (s)
			error = __vfs_follow_link(nd, s);
		if (dentry->d_inode->i_op->put_link)
			dentry->d_inode->i_op->put_link(dentry, nd, cookie);
	}
	path_put(path);

	return error;
}

static inline int do_follow_link(struct path *path, struct nameidata *nd)
{
	int err = -ELOOP;
	if (current->link_count >= MAX_NESTED_LINKS)
		goto loop;
	if (current->total_link_count >= 40)
		goto loop;
	BUG_ON(nd->depth >= MAX_NESTED_LINKS);
	cond_resched();
	err = security_inode_follow_link(path->dentry, nd);
	if (err)
		goto loop;
	current->link_count++;
	current->total_link_count++;
	nd->depth++;
	err = __do_follow_link(path, nd);
	current->link_count--;
	nd->depth--;
	return err;
loop:
	path_put_conditional(path, nd);
	path_put(&nd->path);
	return err;
}

int follow_up(struct path *path)
{
	struct vfsmount *parent;
	struct dentry *mountpoint;
	spin_lock(&vfsmount_lock);
	parent = path->mnt->mnt_parent;
	if (parent == path->mnt) {
		spin_unlock(&vfsmount_lock);
		return 0;
	}
	mntget(parent);
	mountpoint = dget(path->mnt->mnt_mountpoint);
	spin_unlock(&vfsmount_lock);
	dput(path->dentry);
	path->dentry = mountpoint;
	mntput(path->mnt);
	path->mnt = parent;
	return 1;
}

static int __follow_mount(struct path *path)
{
	int res = 0;
	while (d_mountpoint(path->dentry)) {
		struct vfsmount *mounted = lookup_mnt(path);
		if (!mounted)
			break;
		dput(path->dentry);
		if (res)
			mntput(path->mnt);
		path->mnt = mounted;
		path->dentry = dget(mounted->mnt_root);
		res = 1;
#ifdef MY_ABC_HERE
		path->mounted = 1;
#endif
	}
	return res;
}

static void follow_mount(struct path *path)
{
	while (d_mountpoint(path->dentry)) {
		struct vfsmount *mounted = lookup_mnt(path);
		if (!mounted)
			break;
		dput(path->dentry);
		mntput(path->mnt);
		path->mnt = mounted;
		path->dentry = dget(mounted->mnt_root);
	}
}

int follow_down(struct path *path)
{
	struct vfsmount *mounted;

	mounted = lookup_mnt(path);
	if (mounted) {
		dput(path->dentry);
		mntput(path->mnt);
		path->mnt = mounted;
		path->dentry = dget(mounted->mnt_root);
		return 1;
	}
	return 0;
}

static __always_inline void follow_dotdot(struct nameidata *nd)
{
	set_root(nd);

	while(1) {
		struct vfsmount *parent;
		struct dentry *old = nd->path.dentry;

		if (nd->path.dentry == nd->root.dentry &&
		    nd->path.mnt == nd->root.mnt) {
			break;
		}
		spin_lock(&dcache_lock);
		if (nd->path.dentry != nd->path.mnt->mnt_root) {
			nd->path.dentry = dget(nd->path.dentry->d_parent);
			spin_unlock(&dcache_lock);
			dput(old);
			break;
		}
		spin_unlock(&dcache_lock);
		spin_lock(&vfsmount_lock);
		parent = nd->path.mnt->mnt_parent;
		if (parent == nd->path.mnt) {
			spin_unlock(&vfsmount_lock);
			break;
		}
		mntget(parent);
		nd->path.dentry = dget(nd->path.mnt->mnt_mountpoint);
		spin_unlock(&vfsmount_lock);
		dput(old);
		mntput(nd->path.mnt);
		nd->path.mnt = parent;
	}
	follow_mount(&nd->path);
}

static int do_lookup(struct nameidata *nd, struct qstr *name,
		     struct path *path)
{
	struct vfsmount *mnt = nd->path.mnt;
	struct dentry *dentry = __d_lookup(nd->path.dentry, name);

	if (!dentry)
		goto need_lookup;
	if (dentry->d_op && dentry->d_op->d_revalidate)
		goto need_revalidate;
#ifdef MY_ABC_HERE
	if (!dentry->d_inode && (DCACHE_CASELESS_COMPARE & nd->path.dentry->d_flags)) {
		struct dentry * parent = nd->path.dentry;
		struct inode *dir = parent->d_inode;
		struct dentry * result, *new_dentry;

		if (IS_DEADDIR(dir)) {
			dentry = ERR_PTR(-ENOENT);
			goto fail;
		}

		d_invalidate(dentry);
		dput(dentry);
		mutex_lock(&dir->i_mutex);
		new_dentry = d_alloc(parent, name);
		if (!new_dentry) {
			mutex_unlock(&dir->i_mutex);
			dentry = ERR_PTR(-ENOMEM);
			goto fail;
		}
		result = dir->i_op->lookup(dir, new_dentry, nd);
		mutex_unlock(&dir->i_mutex);
		if (result) {
			dput(new_dentry);
			dentry = result;
			goto fail;
		}
		dentry = new_dentry;
	}
#endif
done:
	path->mnt = mnt;
	path->dentry = dentry;
	__follow_mount(path);
	return 0;

need_lookup:
	dentry = real_lookup(nd->path.dentry, name, nd);
	if (IS_ERR(dentry))
		goto fail;
	goto done;

need_revalidate:
	dentry = do_revalidate(dentry, nd);
	if (!dentry)
		goto need_lookup;
	if (IS_ERR(dentry))
		goto fail;
	goto done;

fail:
	return PTR_ERR(dentry);
}

static inline int follow_on_final(struct inode *inode, unsigned lookup_flags)
{
	return inode && unlikely(inode->i_op->follow_link) &&
		((lookup_flags & LOOKUP_FOLLOW) || S_ISDIR(inode->i_mode));
}

static int __link_path_walk(const char *name, struct nameidata *nd)
{
	struct path next;
	struct inode *inode;
	int err;
	unsigned int lookup_flags = nd->flags;

#ifdef MY_ABC_HERE
	 
	unsigned int slashes_count = 0;
	int fLastCN = 0;        
	int total_len = SYNO_SMB_PSTRING_LEN;
	int caselessFlag = LOOKUP_CASELESS_COMPARE & nd->flags;
	int cur_location_updated = 0;
	unsigned char *cur_location = NULL;
	struct qstr this;

	if (caselessFlag) {
		cur_location = nd->real_filename;
		nd->real_filename_len = 0;
	}

	while (*name=='/') {
		name++;
		slashes_count++;
	}

	if (cur_location && (caselessFlag)) {
		 
		while (slashes_count != 0) {
			nd->real_filename_len++;
			total_len--;
			*cur_location = '/';
			cur_location++;
			slashes_count--;
		}
		*cur_location = '\0';
		if (!*name) {
			cur_location_updated = 1;
		}
	}
#else
	while (*name=='/')
		name++;
#endif
	if (!*name)
		goto return_reval;

#ifdef SYNO_FORCE_UNMOUNT
	if (NULL == nd->path.dentry) {
#ifdef SYNO_DEBUG_FORCE_UNMOUNT
		printk("%s: force unmount hit\n", __FUNCTION__);
#endif
		err = -ENOENT;
		goto return_err;
	}
#endif
	inode = nd->path.dentry->d_inode;
	if (nd->depth)
		lookup_flags = LOOKUP_FOLLOW | (nd->flags & LOOKUP_CONTINUE);

	for(;;) {
		unsigned long hash;
#ifndef MY_ABC_HERE
		struct qstr this;
#endif
		unsigned int c;

#ifdef MY_ABC_HERE
		slashes_count = 0;
		cur_location_updated = 0;
		next.mounted = 0;
		spin_lock(&nd->path.dentry->d_lock);
		if (caselessFlag) {
			nd->path.dentry->d_flags |= DCACHE_CASELESS_COMPARE;
		} else {
			nd->path.dentry->d_flags &= ~DCACHE_CASELESS_COMPARE;
		}
		spin_unlock(&nd->path.dentry->d_lock);
#endif
		nd->flags |= LOOKUP_CONTINUE;
#ifdef CONFIG_FS_SYNO_ACL
		if (IS_SYNOACL(inode)) {
			err = synoacl_op_exec_perm(nd->path.dentry, inode);
		} else
#endif
		err = exec_permission_lite(inode);
 		if (err)
			break;

		this.name = name;
		c = *(const unsigned char *)name;

		hash = init_name_hash();
		do {
			name++;
			hash = partial_name_hash(c, hash);
			c = *(const unsigned char *)name;
		} while (c && (c != '/'));
		this.len = name - (const char *) this.name;
		this.hash = end_name_hash(hash);

#ifdef MY_ABC_HERE
		 
		if (!c) {
			fLastCN = 1;
			goto last_component;
		}

		slashes_count++;
		while (*++name == '/') {
			slashes_count++;
		}

		if (!*name) {
			fLastCN = 1;
			goto last_with_slashes;
		}
#else
		 
		if (!c)
			goto last_component;
		while (*++name == '/');
		if (!*name)
			goto last_with_slashes;
#endif

		if (this.name[0] == '.') switch (this.len) {
			default:
				break;
			case 2:	
				if (this.name[1] != '.')
					break;
				follow_dotdot(nd);
				inode = nd->path.dentry->d_inode;
				 
			case 1:
#ifdef MY_ABC_HERE
				if (cur_location && caselessFlag) {
					cur_location_updated = 1;
					total_len -= (this.len + slashes_count);
					if (total_len < 0) {
						err = -ENAMETOOLONG;
						break;
					}
					memcpy(cur_location, this.name, this.len);
					cur_location += this.len;
					nd->real_filename_len += this.len;
					nd->real_filename_len += slashes_count;
					while (slashes_count != 0) {
						*cur_location = '/';
						cur_location++;
						slashes_count--;
					}
					*cur_location = '\0';
				}
#endif
				continue;
		}
		 
		if (nd->path.dentry->d_op && nd->path.dentry->d_op->d_hash) {
			err = nd->path.dentry->d_op->d_hash(nd->path.dentry,
							    &this);
			if (err < 0)
				break;
		}
		 
		err = do_lookup(nd, &this, &next);
		if (err)
			break;

#ifdef MY_ABC_HERE
		 
		if (cur_location && caselessFlag) {
			cur_location_updated = 1;
			 
			if (next.mounted) {
				next.mounted = 0;
				total_len -= (next.mnt->mnt_mountpoint->d_name.len + slashes_count);
				if (total_len < 0) {
					err = -ENAMETOOLONG;
					break;
				}
				memcpy(cur_location, next.mnt->mnt_mountpoint->d_name.name, next.mnt->mnt_mountpoint->d_name.len);
				cur_location += next.mnt->mnt_mountpoint->d_name.len;
				nd->real_filename_len += next.mnt->mnt_mountpoint->d_name.len;
			}else{
				total_len -= (next.dentry->d_name.len + slashes_count);
				if (total_len < 0) {
					err = -ENAMETOOLONG;
					break;
				}
				memcpy(cur_location, next.dentry->d_name.name, next.dentry->d_name.len);
				cur_location += next.dentry->d_name.len;
				nd->real_filename_len += next.dentry->d_name.len;
			}
			nd->real_filename_len += slashes_count;
			while (slashes_count != 0) {
				*cur_location = '/';
				cur_location++;
				slashes_count--;
			}
			*cur_location = '\0';
		}
#endif
		err = -ENOENT;
		inode = next.dentry->d_inode;
		if (!inode)
			goto out_dput;

		if (inode->i_op->follow_link) {
#ifdef MY_ABC_HERE
			 
			if (caselessFlag) {
				nd->flags &=~ LOOKUP_CASELESS_COMPARE;
			}
#endif
			err = do_follow_link(&next, nd);
#ifdef MY_ABC_HERE
			if (caselessFlag) {
				nd->flags |= LOOKUP_CASELESS_COMPARE;
			}
#endif
			if (err)
				goto return_err;
			err = -ENOENT;
			inode = nd->path.dentry->d_inode;
			if (!inode)
				break;
		} else
			path_to_nameidata(&next, nd);
		err = -ENOTDIR; 
		if (!inode->i_op->lookup)
			break;
		continue;
		 
last_with_slashes:
		lookup_flags |= LOOKUP_FOLLOW | LOOKUP_DIRECTORY;
last_component:
		 
		nd->flags &= lookup_flags | ~LOOKUP_CONTINUE;
		if (lookup_flags & LOOKUP_PARENT)
			goto lookup_parent;
		if (this.name[0] == '.') switch (this.len) {
			default:
				break;
			case 2:	
				if (this.name[1] != '.')
					break;
				follow_dotdot(nd);
				inode = nd->path.dentry->d_inode;
				 
			case 1:
				goto return_reval;
		}
		if (nd->path.dentry->d_op && nd->path.dentry->d_op->d_hash) {
			err = nd->path.dentry->d_op->d_hash(nd->path.dentry,
							    &this);
			if (err < 0)
				break;
		}
		err = do_lookup(nd, &this, &next);
		if (err)
			break;
#ifdef MY_ABC_HERE
		 
		if (cur_location && caselessFlag) {
			cur_location_updated = 1;
			 
			if (next.mounted) {
				next.mounted = 0;
				total_len -= (next.mnt->mnt_mountpoint->d_name.len + slashes_count);
				if (total_len < 0) {
					err = -ENAMETOOLONG;
					break;
				}
				memcpy(cur_location, next.mnt->mnt_mountpoint->d_name.name, next.mnt->mnt_mountpoint->d_name.len);
				cur_location += next.mnt->mnt_mountpoint->d_name.len;
				nd->real_filename_len += next.mnt->mnt_mountpoint->d_name.len;
			}else{
				total_len -= (next.dentry->d_name.len + slashes_count);
				if (total_len < 0) {
					err = -ENAMETOOLONG;
					break;
				}
				memcpy(cur_location, next.dentry->d_name.name, next.dentry->d_name.len);
				cur_location += next.dentry->d_name.len;
				nd->real_filename_len += next.dentry->d_name.len;
			}
			nd->real_filename_len += slashes_count;
			while (slashes_count != 0) {
				*cur_location = '/';
				cur_location++;
				slashes_count--;
			}
			*cur_location = '\0';
		}
#endif
		inode = next.dentry->d_inode;
		if (follow_on_final(inode, lookup_flags)) {
#ifdef MY_ABC_HERE
			 
			if (caselessFlag) {
				nd->flags &=~ LOOKUP_CASELESS_COMPARE;
			}
#endif
			err = do_follow_link(&next, nd);
#ifdef MY_ABC_HERE
			if (caselessFlag) {
				nd->flags |= LOOKUP_CASELESS_COMPARE;
			}
#endif
			if (err)
				goto return_err;
			inode = nd->path.dentry->d_inode;
		} else
			path_to_nameidata(&next, nd);
		err = -ENOENT;
		if (!inode)
			break;
		if (lookup_flags & LOOKUP_DIRECTORY) {
			err = -ENOTDIR; 
			if (!inode->i_op->lookup)
				break;
		}
		goto return_base;
lookup_parent:
		nd->last = this;
		nd->last_type = LAST_NORM;
		if (this.name[0] != '.')
			goto return_base;
		if (this.len == 1)
			nd->last_type = LAST_DOT;
		else if (this.len == 2 && this.name[1] == '.')
			nd->last_type = LAST_DOTDOT;
		else
			goto return_base;
return_reval:
		 
		if (nd->path.dentry && nd->path.dentry->d_sb &&
		    (nd->path.dentry->d_sb->s_type->fs_flags & FS_REVAL_DOT)) {
			err = -ESTALE;
			 
			if (!nd->path.dentry->d_op->d_revalidate(
					nd->path.dentry, nd))
				break;
		}
return_base:
#ifdef MY_ABC_HERE
		if (cur_location && caselessFlag && !cur_location_updated) {
			cur_location_updated = 1;
			total_len -= (this.len + slashes_count);
			if (total_len < 0) {
				err = -ENAMETOOLONG;
				break;
			}
			memcpy(cur_location, this.name, this.len);
			cur_location += this.len;
			nd->real_filename_len += this.len;
			nd->real_filename_len += slashes_count;
			while (slashes_count != 0) {
				*cur_location = '/';
				cur_location++;
				slashes_count--;
			}
			*cur_location = '\0';
		}
#endif
		return 0;
out_dput:
		path_put_conditional(&next, nd);
		break;
	}
	path_put(&nd->path);
return_err:
#ifdef MY_ABC_HERE
	if(fLastCN)
		nd->flags |= LOOKUP_TO_LASTCOMPONENT;

	if (!err && cur_location && caselessFlag && !cur_location_updated) {
		cur_location_updated = 1;
		total_len -= (this.len + slashes_count);
		if (total_len < 0) {
			err = -ENAMETOOLONG;
			return err;
		}
		memcpy(cur_location, this.name, this.len);
		cur_location += this.len;
		nd->real_filename_len += this.len;
		nd->real_filename_len += slashes_count;
		while (slashes_count != 0) {
			*cur_location = '/';
			cur_location++;
			slashes_count--;
		}
		*cur_location = '\0';
	}
#endif
	return err;
}

static int path_walk(const char *name, struct nameidata *nd)
{
	current->total_link_count = 0;
	return link_path_walk(name, nd);
}

static int path_init(int dfd, const char *name, unsigned int flags, struct nameidata *nd)
{
	int retval = 0;
	int fput_needed;
	struct file *file;

	nd->last_type = LAST_ROOT;  
	nd->flags = flags;
	nd->depth = 0;
	nd->root.mnt = NULL;

	if (*name=='/') {
		set_root(nd);
		nd->path = nd->root;
		path_get(&nd->root);
	} else if (dfd == AT_FDCWD) {
		struct fs_struct *fs = current->fs;
		read_lock(&fs->lock);
		nd->path = fs->pwd;
		path_get(&fs->pwd);
		read_unlock(&fs->lock);
	} else {
		struct dentry *dentry;

		file = fget_light(dfd, &fput_needed);
		retval = -EBADF;
		if (!file)
			goto out_fail;

		dentry = file->f_path.dentry;

		retval = -ENOTDIR;
		if (!S_ISDIR(dentry->d_inode->i_mode))
			goto fput_fail;

		retval = file_permission(file, MAY_EXEC);
		if (retval)
			goto fput_fail;

		nd->path = file->f_path;
		path_get(&file->f_path);

		fput_light(file, fput_needed);
	}
	return 0;

fput_fail:
	fput_light(file, fput_needed);
out_fail:
	return retval;
}

static int do_path_lookup(int dfd, const char *name,
				unsigned int flags, struct nameidata *nd)
{
	int retval = path_init(dfd, name, flags, nd);
	if (!retval)
		retval = path_walk(name, nd);
	if (unlikely(!retval && !audit_dummy_context() && nd->path.dentry &&
				nd->path.dentry->d_inode))
		audit_inode(name, nd->path.dentry);
	if (nd->root.mnt) {
		path_put(&nd->root);
		nd->root.mnt = NULL;
	}
	return retval;
}

int path_lookup(const char *name, unsigned int flags,
			struct nameidata *nd)
{
	return do_path_lookup(AT_FDCWD, name, flags, nd);
}

int kern_path(const char *name, unsigned int flags, struct path *path)
{
	struct nameidata nd;
	int res = do_path_lookup(AT_FDCWD, name, flags, &nd);
	if (!res)
		*path = nd.path;
	return res;
}

int vfs_path_lookup(struct dentry *dentry, struct vfsmount *mnt,
		    const char *name, unsigned int flags,
		    struct nameidata *nd)
{
	int retval;

	nd->last_type = LAST_ROOT;
	nd->flags = flags;
	nd->depth = 0;

	nd->path.dentry = dentry;
	nd->path.mnt = mnt;
	path_get(&nd->path);
	nd->root = nd->path;
	path_get(&nd->root);

	retval = path_walk(name, nd);
	if (unlikely(!retval && !audit_dummy_context() && nd->path.dentry &&
				nd->path.dentry->d_inode))
		audit_inode(name, nd->path.dentry);

	path_put(&nd->root);
	nd->root.mnt = NULL;

	return retval;
}

static int path_lookup_open(int dfd, const char *name,
		unsigned int lookup_flags, struct nameidata *nd, int open_flags)
{
	struct file *filp = get_empty_filp();
	int err;

	if (filp == NULL)
		return -ENFILE;
	nd->intent.open.file = filp;
	nd->intent.open.flags = open_flags;
	nd->intent.open.create_mode = 0;
	err = do_path_lookup(dfd, name, lookup_flags|LOOKUP_OPEN, nd);
	if (IS_ERR(nd->intent.open.file)) {
		if (err == 0) {
			err = PTR_ERR(nd->intent.open.file);
			path_put(&nd->path);
		}
	} else if (err != 0)
		release_open_intent(nd);
	return err;
}

static struct dentry *__lookup_hash(struct qstr *name,
		struct dentry *base, struct nameidata *nd)
{
	struct dentry *dentry;
	struct inode *inode;
	int err;

	inode = base->d_inode;

	if (base->d_op && base->d_op->d_hash) {
		err = base->d_op->d_hash(base, name);
		dentry = ERR_PTR(err);
		if (err < 0)
			goto out;
	}

	dentry = cached_lookup(base, name, nd);
	if (!dentry) {
		struct dentry *new;

		dentry = ERR_PTR(-ENOENT);
		if (IS_DEADDIR(inode))
			goto out;

		new = d_alloc(base, name);
		dentry = ERR_PTR(-ENOMEM);
		if (!new)
			goto out;
		dentry = inode->i_op->lookup(inode, new, nd);
		if (!dentry)
			dentry = new;
		else
			dput(new);
	}
out:
	return dentry;
}

#ifdef MY_ABC_HERE
extern struct dentry *lookup_hash(struct nameidata *nd)
#else
#ifdef CONFIG_AUFS_FS
extern struct dentry *lookup_hash(struct nameidata *nd)
#else  
static struct dentry *lookup_hash(struct nameidata *nd)
#endif  
#endif
{
	int err;

#ifdef CONFIG_FS_SYNO_ACL
	struct inode *inode = nd->path.dentry->d_inode;

	if (IS_SYNOACL(inode)) {
		err = synoacl_op_exec_perm(nd->path.dentry, inode);
	} else {
		err = inode_permission(inode, MAY_EXEC);
	}
#else
	err = inode_permission(nd->path.dentry->d_inode, MAY_EXEC);
#endif  
	if (err)
		return ERR_PTR(err);
	return __lookup_hash(&nd->last, nd->path.dentry, nd);
}

#ifdef MY_ABC_HERE
static struct dentry * __lookup_hash1(struct qstr *name,
		struct dentry * base, struct nameidata *nd)
{
	struct dentry * dentry;
	struct inode *inode;
	int err;

	inode = base->d_inode;

	if (base->d_op && base->d_op->d_hash) {
		err = base->d_op->d_hash(base, name);
		dentry = ERR_PTR(err);
		if (err < 0)
			goto out;
	}

	dentry = cached_lookup(base, name, nd);
	if (inode->i_op->lookup_case && dentry)  {
		d_invalidate(dentry);
		dput(dentry);
		dentry = NULL;
	}
	if (!dentry) {
		struct dentry *new = d_alloc(base, name);
		dentry = ERR_PTR(-ENOMEM);
		if (!new)
			goto out;
		if (inode->i_op->lookup_case)
			dentry = inode->i_op->lookup_case(inode, new, nd);
		else
			dentry = inode->i_op->lookup(inode, new, nd);
		if (!dentry)
			dentry = new;
		else
			dput(new);
	}
out:
	return dentry;
}

struct dentry * lookup_hash1(struct nameidata *nd)
{
	int err;

#ifdef CONFIG_FS_SYNO_ACL
	struct inode *inode = nd->path.dentry->d_inode;

	if (IS_SYNOACL(inode)) {
		err = synoacl_op_exec_perm(nd->path.dentry, inode);
	} else {
		err = inode_permission(inode, MAY_EXEC);
	}
#else
	err = inode_permission(nd->path.dentry->d_inode, MAY_EXEC);
#endif  
	if (err)
		return ERR_PTR(err);
	return __lookup_hash1(&nd->last, nd->path.dentry, nd);
}
#endif

#ifdef CONFIG_AUFS_FS
int __lookup_one_len(const char *name, struct qstr *this,
		struct dentry *base, int len)
#else  
static int __lookup_one_len(const char *name, struct qstr *this,
		struct dentry *base, int len)
#endif  
{
	unsigned long hash;
	unsigned int c;

	this->name = name;
	this->len = len;
	if (!len)
		return -EACCES;

	hash = init_name_hash();
	while (len--) {
		c = *(const unsigned char *)name++;
		if (c == '/' || c == '\0')
			return -EACCES;
		hash = partial_name_hash(c, hash);
	}
	this->hash = end_name_hash(hash);
	return 0;
}
#ifdef CONFIG_AUFS_FS
EXPORT_SYMBOL(__lookup_one_len);
#endif  

struct dentry *lookup_one_len(const char *name, struct dentry *base, int len)
{
	int err;
	struct qstr this;

	WARN_ON_ONCE(!mutex_is_locked(&base->d_inode->i_mutex));

	err = __lookup_one_len(name, &this, base, len);
	if (err)
		return ERR_PTR(err);

#ifdef CONFIG_FS_SYNO_ACL
	if (IS_SYNOACL(base->d_inode)) {
		err = synoacl_op_exec_perm(base, base->d_inode);
	} else 
#endif  
	err = inode_permission(base->d_inode, MAY_EXEC);
	if (err)
		return ERR_PTR(err);
	return __lookup_hash(&this, base, NULL);
}

struct dentry *lookup_one_noperm(const char *name, struct dentry *base)
{
	int err;
	struct qstr this;

	err = __lookup_one_len(name, &this, base, strlen(name));
	if (err)
		return ERR_PTR(err);
	return __lookup_hash(&this, base, NULL);
}

#ifdef MY_ABC_HERE
int syno_user_path_at(int dfd, const char __user *name, unsigned flags,
		 struct path *path, char **real_filename, int *real_filename_len)
{
	struct nameidata nd;
	char *tmp = getname(name);
	int err = PTR_ERR(tmp);
	if (!IS_ERR(tmp)) {

		BUG_ON(flags & LOOKUP_PARENT);

		nd.real_filename = *real_filename;
		err = do_path_lookup(dfd, tmp, flags, &nd);
		putname(tmp);
		if (!err) {
			*path = nd.path;
			*real_filename_len = nd.real_filename_len;
		}
	}
	return err;
}
#endif

int user_path_at(int dfd, const char __user *name, unsigned flags,
		 struct path *path)
{
	struct nameidata nd;
	char *tmp = getname(name);
	int err = PTR_ERR(tmp);
	if (!IS_ERR(tmp)) {

		BUG_ON(flags & LOOKUP_PARENT);

		err = do_path_lookup(dfd, tmp, flags, &nd);
		putname(tmp);
		if (!err)
			*path = nd.path;
	}
	return err;
}

static int user_path_parent(int dfd, const char __user *path,
			struct nameidata *nd, char **name)
{
	char *s = getname(path);
	int error;

	if (IS_ERR(s))
		return PTR_ERR(s);

	error = do_path_lookup(dfd, s, LOOKUP_PARENT, nd);
	if (error)
		putname(s);
	else
		*name = s;

	return error;
}

static inline int check_sticky(struct inode *dir, struct inode *inode)
{
	uid_t fsuid = current_fsuid();

	if (!(dir->i_mode & S_ISVTX))
		return 0;
	if (inode->i_uid == fsuid)
		return 0;
	if (dir->i_uid == fsuid)
		return 0;
	return !capable(CAP_FOWNER);
}

static int may_delete(struct inode *dir,struct dentry *victim,int isdir)
{
	int error;

	if (!victim->d_inode)
		return -ENOENT;

	BUG_ON(victim->d_parent->d_inode != dir);
	audit_inode_child(victim->d_name.name, victim, dir);

#ifdef CONFIG_FS_SYNO_ACL
	if (IS_FS_SYNOACL(dir)) {
		error = synoacl_mod_may_delete(victim, dir);
	} else
#endif
	error = inode_permission(dir, MAY_WRITE | MAY_EXEC);
	if (error)
		return error;
	if (IS_APPEND(dir))
		return -EPERM;
#ifdef CONFIG_FS_SYNO_ACL
	if (!IS_SYNOACL(dir) && check_sticky(dir, victim->d_inode)) {
		return -EPERM;
	}
	if (IS_APPEND(victim->d_inode) || IS_IMMUTABLE(victim->d_inode) || IS_SWAPFILE(victim->d_inode)){
		return -EPERM;
	}
#else
	if (check_sticky(dir, victim->d_inode)||IS_APPEND(victim->d_inode)||
	    IS_IMMUTABLE(victim->d_inode) || IS_SWAPFILE(victim->d_inode))
		return -EPERM;
#endif  
	if (isdir) {
		if (!S_ISDIR(victim->d_inode->i_mode))
			return -ENOTDIR;
		if (IS_ROOT(victim))
			return -EBUSY;
	} else if (S_ISDIR(victim->d_inode->i_mode))
		return -EISDIR;
	if (IS_DEADDIR(dir))
		return -ENOENT;
	if (victim->d_flags & DCACHE_NFSFS_RENAMED)
		return -EBUSY;
	return 0;
}

#ifdef CONFIG_FS_SYNO_ACL
static inline int may_create(struct inode *dir, struct dentry *child, int mode)
#else 
static inline int may_create(struct inode *dir, struct dentry *child)
#endif
{
	if (child->d_inode)
		return -EEXIST;
	if (IS_DEADDIR(dir))
		return -ENOENT;

#ifdef CONFIG_FS_SYNO_ACL
	if (IS_SYNOACL(dir)) {
		return synoacl_op_perm(child->d_parent, (S_ISDIR(mode)?MAY_APPEND:MAY_WRITE) | MAY_EXEC);
	} 
#endif  
	return inode_permission(dir, MAY_WRITE | MAY_EXEC);
}

static inline int lookup_flags(unsigned int f)
{
	unsigned long retval = LOOKUP_FOLLOW;

	if (f & O_NOFOLLOW)
		retval &= ~LOOKUP_FOLLOW;
	
	if (f & O_DIRECTORY)
		retval |= LOOKUP_DIRECTORY;

	return retval;
}

struct dentry *lock_rename(struct dentry *p1, struct dentry *p2)
{
	struct dentry *p;

	if (p1 == p2) {
		mutex_lock_nested(&p1->d_inode->i_mutex, I_MUTEX_PARENT);
		return NULL;
	}

	mutex_lock(&p1->d_inode->i_sb->s_vfs_rename_mutex);

	p = d_ancestor(p2, p1);
	if (p) {
		mutex_lock_nested(&p2->d_inode->i_mutex, I_MUTEX_PARENT);
		mutex_lock_nested(&p1->d_inode->i_mutex, I_MUTEX_CHILD);
		return p;
	}

	p = d_ancestor(p1, p2);
	if (p) {
		mutex_lock_nested(&p1->d_inode->i_mutex, I_MUTEX_PARENT);
		mutex_lock_nested(&p2->d_inode->i_mutex, I_MUTEX_CHILD);
		return p;
	}

	mutex_lock_nested(&p1->d_inode->i_mutex, I_MUTEX_PARENT);
	mutex_lock_nested(&p2->d_inode->i_mutex, I_MUTEX_CHILD);
	return NULL;
}

void unlock_rename(struct dentry *p1, struct dentry *p2)
{
	mutex_unlock(&p1->d_inode->i_mutex);
	if (p1 != p2) {
		mutex_unlock(&p2->d_inode->i_mutex);
		mutex_unlock(&p1->d_inode->i_sb->s_vfs_rename_mutex);
	}
}

int vfs_create(struct inode *dir, struct dentry *dentry, int mode,
		struct nameidata *nd)
{
#ifdef CONFIG_FS_SYNO_ACL
	int error = may_create(dir, dentry, S_IFREG);
#else
	int error = may_create(dir, dentry);
#endif

	if (error)
		return error;

	if (!dir->i_op->create)
		return -EACCES;	 
	mode &= S_IALLUGO;
	mode |= S_IFREG;
	error = security_inode_create(dir, dentry, mode);
	if (error)
		return error;
	vfs_dq_init(dir);
	error = dir->i_op->create(dir, dentry, mode, nd);
	if (!error)
		fsnotify_create(dir, dentry);
#ifdef CONFIG_FS_SYNO_ACL
	if (!error && IS_SYNOACL(dir)) {
		 
		synoacl_op_init(dentry);
	}
#endif
	return error;
}

int may_open(struct path *path, int acc_mode, int flag)
{
	struct dentry *dentry = path->dentry;
	struct inode *inode = dentry->d_inode;
	int error;

	if (!inode)
		return -ENOENT;

	switch (inode->i_mode & S_IFMT) {
	case S_IFLNK:
		return -ELOOP;
	case S_IFDIR:
		if (acc_mode & MAY_WRITE)
			return -EISDIR;
		break;
	case S_IFBLK:
	case S_IFCHR:
		if (path->mnt->mnt_flags & MNT_NODEV)
			return -EACCES;
		 
	case S_IFIFO:
	case S_IFSOCK:
		flag &= ~O_TRUNC;
		break;
	}

#ifdef CONFIG_FS_SYNO_ACL
	if (IS_SYNOACL(inode)) {
		error = synoacl_op_perm(dentry, acc_mode);
	} else
#endif  
	error = inode_permission(inode, acc_mode);
	if (error)
		return error;

	error = ima_path_check(path, acc_mode ?
			       acc_mode & (MAY_READ | MAY_WRITE | MAY_EXEC) :
			       ACC_MODE(flag) & (MAY_READ | MAY_WRITE),
			       IMA_COUNT_UPDATE);

	if (error)
		return error;
	 
	if (IS_APPEND(inode)) {
		error = -EPERM;
		if  ((flag & FMODE_WRITE) && !(flag & O_APPEND))
			goto err_out;
		if (flag & O_TRUNC)
			goto err_out;
	}

	if (flag & O_NOATIME)
		if (!is_owner_or_cap(inode)) {
			error = -EPERM;
			goto err_out;
		}

	error = break_lease(inode, flag);
	if (error)
		goto err_out;

	if (flag & O_TRUNC) {
		error = get_write_access(inode);
		if (error)
			goto err_out;

		error = locks_verify_locked(inode);
		if (!error)
			error = security_path_truncate(path, 0,
					       ATTR_MTIME|ATTR_CTIME|ATTR_OPEN);
		if (!error) {
			vfs_dq_init(inode);

			error = do_truncate(dentry, 0,
					    ATTR_MTIME|ATTR_CTIME|ATTR_OPEN,
					    NULL);
		}
		put_write_access(inode);
		if (error)
			goto err_out;
	} else
		if (flag & FMODE_WRITE)
			vfs_dq_init(inode);

	return 0;
err_out:
	ima_counts_put(path, acc_mode ?
		       acc_mode & (MAY_READ | MAY_WRITE | MAY_EXEC) :
		       ACC_MODE(flag) & (MAY_READ | MAY_WRITE));
	return error;
}

static int __open_namei_create(struct nameidata *nd, struct path *path,
				int flag, int mode)
{
	int error;
	struct dentry *dir = nd->path.dentry;

	if (!IS_POSIXACL(dir->d_inode))
		mode &= ~current_umask();
	error = security_path_mknod(&nd->path, path->dentry, mode, 0);
	if (error)
		goto out_unlock;
	error = vfs_create(dir->d_inode, path->dentry, mode, nd);
out_unlock:
	mutex_unlock(&dir->d_inode->i_mutex);
	dput(nd->path.dentry);
	nd->path.dentry = path->dentry;
	if (error)
		return error;
	 
	return may_open(&nd->path, 0, flag & ~O_TRUNC);
}

static inline int open_to_namei_flags(int flag)
{
	if ((flag+1) & O_ACCMODE)
		flag++;
	return flag;
}

static int open_will_write_to_fs(int flag, struct inode *inode)
{
	 
	if (special_file(inode->i_mode))
		return 0;
	return (flag & O_TRUNC);
}

struct file *do_filp_open(int dfd, const char *pathname,
		int open_flag, int mode, int acc_mode)
{
	struct file *filp;
	struct nameidata nd;
	int error;
	struct path path;
	struct dentry *dir;
	int count = 0;
	int will_write;
	int flag = open_to_namei_flags(open_flag);

	if (!acc_mode)
		acc_mode = MAY_OPEN | ACC_MODE(flag);

	if (flag & O_TRUNC)
		acc_mode |= MAY_WRITE;

	if (flag & O_APPEND)
		acc_mode |= MAY_APPEND;

	if (!(flag & O_CREAT)) {
		error = path_lookup_open(dfd, pathname, lookup_flags(flag),
					 &nd, flag);
		if (error)
			return ERR_PTR(error);
		goto ok;
	}

	error = path_init(dfd, pathname, LOOKUP_PARENT, &nd);
	if (error)
		return ERR_PTR(error);
	error = path_walk(pathname, &nd);
	if (error) {
		if (nd.root.mnt)
			path_put(&nd.root);
		return ERR_PTR(error);
	}
	if (unlikely(!audit_dummy_context()))
		audit_inode(pathname, nd.path.dentry);

	error = -EISDIR;
	if (nd.last_type != LAST_NORM || nd.last.name[nd.last.len])
		goto exit_parent;

	error = -ENFILE;
	filp = get_empty_filp();
	if (filp == NULL)
		goto exit_parent;
	nd.intent.open.file = filp;
	nd.intent.open.flags = flag;
	nd.intent.open.create_mode = mode;
	dir = nd.path.dentry;
	nd.flags &= ~LOOKUP_PARENT;
	nd.flags |= LOOKUP_CREATE | LOOKUP_OPEN;
	if (flag & O_EXCL)
		nd.flags |= LOOKUP_EXCL;
	mutex_lock(&dir->d_inode->i_mutex);
	path.dentry = lookup_hash(&nd);
	path.mnt = nd.path.mnt;

do_last:
	error = PTR_ERR(path.dentry);
	if (IS_ERR(path.dentry)) {
		mutex_unlock(&dir->d_inode->i_mutex);
		goto exit;
	}

	if (IS_ERR(nd.intent.open.file)) {
		error = PTR_ERR(nd.intent.open.file);
		goto exit_mutex_unlock;
	}

	if (!path.dentry->d_inode) {
		 
		error = mnt_want_write(nd.path.mnt);
		if (error)
			goto exit_mutex_unlock;
		error = __open_namei_create(&nd, &path, flag, mode);
		if (error) {
			mnt_drop_write(nd.path.mnt);
			goto exit;
		}
		filp = nameidata_to_filp(&nd, open_flag);
		if (IS_ERR(filp))
			ima_counts_put(&nd.path,
				       acc_mode & (MAY_READ | MAY_WRITE |
						   MAY_EXEC));
		mnt_drop_write(nd.path.mnt);
		if (nd.root.mnt)
			path_put(&nd.root);
		return filp;
	}

	mutex_unlock(&dir->d_inode->i_mutex);
	audit_inode(pathname, path.dentry);

	error = -EEXIST;
	if (flag & O_EXCL)
		goto exit_dput;

	if (__follow_mount(&path)) {
		error = -ELOOP;
		if (flag & O_NOFOLLOW)
			goto exit_dput;
	}

	error = -ENOENT;
	if (!path.dentry->d_inode)
		goto exit_dput;
	if (path.dentry->d_inode->i_op->follow_link)
		goto do_link;

	path_to_nameidata(&path, &nd);
	error = -EISDIR;
	if (path.dentry->d_inode && S_ISDIR(path.dentry->d_inode->i_mode))
		goto exit;
ok:
	 
	will_write = open_will_write_to_fs(flag, nd.path.dentry->d_inode);
	if (will_write) {
		error = mnt_want_write(nd.path.mnt);
		if (error)
			goto exit;
	}
	error = may_open(&nd.path, acc_mode, flag);
	if (error) {
		if (will_write)
			mnt_drop_write(nd.path.mnt);
		goto exit;
	}
	filp = nameidata_to_filp(&nd, open_flag);
	if (IS_ERR(filp))
		ima_counts_put(&nd.path,
			       acc_mode & (MAY_READ | MAY_WRITE | MAY_EXEC));
	 
	if (will_write)
		mnt_drop_write(nd.path.mnt);
	if (nd.root.mnt)
		path_put(&nd.root);
	return filp;

exit_mutex_unlock:
	mutex_unlock(&dir->d_inode->i_mutex);
exit_dput:
	path_put_conditional(&path, &nd);
exit:
	if (!IS_ERR(nd.intent.open.file))
		release_open_intent(&nd);
exit_parent:
	if (nd.root.mnt)
		path_put(&nd.root);
	path_put(&nd.path);
	return ERR_PTR(error);

do_link:
	error = -ELOOP;
	if (flag & O_NOFOLLOW)
		goto exit_dput;
	 
	nd.flags |= LOOKUP_PARENT;
	error = security_inode_follow_link(path.dentry, &nd);
	if (error)
		goto exit_dput;
	error = __do_follow_link(&path, &nd);
	if (error) {
		 
		release_open_intent(&nd);
		if (nd.root.mnt)
			path_put(&nd.root);
		return ERR_PTR(error);
	}
	nd.flags &= ~LOOKUP_PARENT;
	if (nd.last_type == LAST_BIND)
		goto ok;
	error = -EISDIR;
	if (nd.last_type != LAST_NORM)
		goto exit;
	if (nd.last.name[nd.last.len]) {
		__putname(nd.last.name);
		goto exit;
	}
	error = -ELOOP;
	if (count++==32) {
		__putname(nd.last.name);
		goto exit;
	}
	dir = nd.path.dentry;
	mutex_lock(&dir->d_inode->i_mutex);
	path.dentry = lookup_hash(&nd);
	path.mnt = nd.path.mnt;
	__putname(nd.last.name);
	goto do_last;
}

struct file *filp_open(const char *filename, int flags, int mode)
{
	return do_filp_open(AT_FDCWD, filename, flags, mode, 0);
}
EXPORT_SYMBOL(filp_open);

struct dentry *lookup_create(struct nameidata *nd, int is_dir)
{
	struct dentry *dentry = ERR_PTR(-EEXIST);

	mutex_lock_nested(&nd->path.dentry->d_inode->i_mutex, I_MUTEX_PARENT);
	 
	if (nd->last_type != LAST_NORM)
		goto fail;
	nd->flags &= ~LOOKUP_PARENT;
	nd->flags |= LOOKUP_CREATE | LOOKUP_EXCL;
	nd->intent.open.flags = O_EXCL;

	dentry = lookup_hash(nd);
	if (IS_ERR(dentry))
		goto fail;

	if (dentry->d_inode)
		goto eexist;
	 
	if (unlikely(!is_dir && nd->last.name[nd->last.len])) {
		dput(dentry);
		dentry = ERR_PTR(-ENOENT);
	}
	return dentry;
eexist:
	dput(dentry);
	dentry = ERR_PTR(-EEXIST);
fail:
	return dentry;
}
EXPORT_SYMBOL_GPL(lookup_create);

int vfs_mknod(struct inode *dir, struct dentry *dentry, int mode, dev_t dev)
{
#ifdef CONFIG_FS_SYNO_ACL
	int error = may_create(dir, dentry, mode);
#else
	int error = may_create(dir, dentry);
#endif

	if (error)
		return error;

	if ((S_ISCHR(mode) || S_ISBLK(mode)) && !capable(CAP_MKNOD))
		return -EPERM;

	if (!dir->i_op->mknod)
		return -EPERM;

	error = devcgroup_inode_mknod(mode, dev);
	if (error)
		return error;

	error = security_inode_mknod(dir, dentry, mode, dev);
	if (error)
		return error;

	vfs_dq_init(dir);
	error = dir->i_op->mknod(dir, dentry, mode, dev);
	if (!error)
		fsnotify_create(dir, dentry);
	return error;
}

static int may_mknod(mode_t mode)
{
	switch (mode & S_IFMT) {
	case S_IFREG:
	case S_IFCHR:
	case S_IFBLK:
	case S_IFIFO:
	case S_IFSOCK:
	case 0:  
		return 0;
	case S_IFDIR:
		return -EPERM;
	default:
		return -EINVAL;
	}
}

SYSCALL_DEFINE4(mknodat, int, dfd, const char __user *, filename, int, mode,
		unsigned, dev)
{
	int error;
	char *tmp;
	struct dentry *dentry;
	struct nameidata nd;

	if (S_ISDIR(mode))
		return -EPERM;

	error = user_path_parent(dfd, filename, &nd, &tmp);
	if (error)
		return error;

	dentry = lookup_create(&nd, 0);
	if (IS_ERR(dentry)) {
		error = PTR_ERR(dentry);
		goto out_unlock;
	}
	if (!IS_POSIXACL(nd.path.dentry->d_inode))
		mode &= ~current_umask();
	error = may_mknod(mode);
	if (error)
		goto out_dput;
	error = mnt_want_write(nd.path.mnt);
	if (error)
		goto out_dput;
	error = security_path_mknod(&nd.path, dentry, mode, dev);
	if (error)
		goto out_drop_write;
	switch (mode & S_IFMT) {
		case 0: case S_IFREG:
			error = vfs_create(nd.path.dentry->d_inode,dentry,mode,&nd);
			break;
		case S_IFCHR: case S_IFBLK:
			error = vfs_mknod(nd.path.dentry->d_inode,dentry,mode,
					new_decode_dev(dev));
			break;
		case S_IFIFO: case S_IFSOCK:
			error = vfs_mknod(nd.path.dentry->d_inode,dentry,mode,0);
			break;
	}
out_drop_write:
	mnt_drop_write(nd.path.mnt);
out_dput:
	dput(dentry);
out_unlock:
	mutex_unlock(&nd.path.dentry->d_inode->i_mutex);
	path_put(&nd.path);
	putname(tmp);

	return error;
}

SYSCALL_DEFINE3(mknod, const char __user *, filename, int, mode, unsigned, dev)
{
	return sys_mknodat(AT_FDCWD, filename, mode, dev);
}

int vfs_mkdir(struct inode *dir, struct dentry *dentry, int mode)
{
#ifdef CONFIG_FS_SYNO_ACL
	int error = may_create(dir, dentry, S_IFDIR);
#else
	int error = may_create(dir, dentry);
#endif
	if (error)
		return error;

	if (!dir->i_op->mkdir)
		return -EPERM;

	mode &= (S_IRWXUGO|S_ISVTX);
	error = security_inode_mkdir(dir, dentry, mode);
	if (error)
		return error;

	vfs_dq_init(dir);
	error = dir->i_op->mkdir(dir, dentry, mode);
	if (!error)
		fsnotify_mkdir(dir, dentry);
#ifdef CONFIG_FS_SYNO_ACL
	if (!error && IS_SYNOACL(dir)) {
		 
		synoacl_op_init(dentry);
	}
#endif
	return error;
}

SYSCALL_DEFINE3(mkdirat, int, dfd, const char __user *, pathname, int, mode)
{
	int error = 0;
	char * tmp;
	struct dentry *dentry;
	struct nameidata nd;

	error = user_path_parent(dfd, pathname, &nd, &tmp);
	if (error)
		goto out_err;

	dentry = lookup_create(&nd, 1);
	error = PTR_ERR(dentry);
	if (IS_ERR(dentry))
		goto out_unlock;

	if (!IS_POSIXACL(nd.path.dentry->d_inode))
		mode &= ~current_umask();
	error = mnt_want_write(nd.path.mnt);
	if (error)
		goto out_dput;
	error = security_path_mkdir(&nd.path, dentry, mode);
	if (error)
		goto out_drop_write;
	error = vfs_mkdir(nd.path.dentry->d_inode, dentry, mode);
out_drop_write:
	mnt_drop_write(nd.path.mnt);
out_dput:
	dput(dentry);
out_unlock:
	mutex_unlock(&nd.path.dentry->d_inode->i_mutex);
	path_put(&nd.path);
	putname(tmp);
out_err:
	return error;
}

SYSCALL_DEFINE2(mkdir, const char __user *, pathname, int, mode)
{
	return sys_mkdirat(AT_FDCWD, pathname, mode);
}

#ifdef SYNO_FORCE_UNMOUNT
 
int syno_get_empty_dir(struct vfsmount **empty_mnt, struct dentry **empty_dentry)
{
	int error = 0;
	struct dentry *dentry;
	struct nameidata nd;

	error = do_path_lookup(AT_FDCWD, "/tmp/forceumount", LOOKUP_PARENT,&nd);
	if (error) {
		goto out_err;
	}

	dentry = lookup_create(&nd, 1);
	error = PTR_ERR(dentry);
	if (IS_ERR(dentry)) {
		goto out_unlock;
	}

	error = mnt_want_write(nd.path.mnt);
	if (error)
		goto out_dput;
	error = security_path_mkdir(&nd.path, dentry, 0);
	if (error)
		goto out_drop_write;
	error = vfs_mkdir(nd.path.dentry->d_inode, dentry, 0);
	dentry->d_inode->i_flags |= S_IMMUTABLE;
	mark_inode_dirty(nd.path.dentry->d_inode);
	*empty_mnt = nd.path.mnt;
	*empty_dentry = dentry;

out_drop_write:
	mnt_drop_write(nd.path.mnt);
out_dput:
	dput(dentry);
out_unlock:
	mutex_unlock(&nd.path.dentry->d_inode->i_mutex);
	path_put(&nd.path);
out_err:

	return error;
}
EXPORT_SYMBOL(syno_get_empty_dir);
#endif

void dentry_unhash(struct dentry *dentry)
{
	dget(dentry);
	shrink_dcache_parent(dentry);
	spin_lock(&dcache_lock);
	spin_lock(&dentry->d_lock);
	if (atomic_read(&dentry->d_count) == 2)
		__d_drop(dentry);
	spin_unlock(&dentry->d_lock);
	spin_unlock(&dcache_lock);
}

int vfs_rmdir(struct inode *dir, struct dentry *dentry)
{
	int error = may_delete(dir, dentry, 1);

	if (error)
		return error;

	if (!dir->i_op->rmdir)
		return -EPERM;

	vfs_dq_init(dir);

	mutex_lock(&dentry->d_inode->i_mutex);
	dentry_unhash(dentry);
	if (d_mountpoint(dentry))
		error = -EBUSY;
	else {
		error = security_inode_rmdir(dir, dentry);
		if (!error) {
			error = dir->i_op->rmdir(dir, dentry);
			if (!error)
				dentry->d_inode->i_flags |= S_DEAD;
		}
	}
	mutex_unlock(&dentry->d_inode->i_mutex);
	if (!error) {
		d_delete(dentry);
	}
	dput(dentry);

	return error;
}

static long do_rmdir(int dfd, const char __user *pathname)
{
	int error = 0;
	char * name;
	struct dentry *dentry;
	struct nameidata nd;

	error = user_path_parent(dfd, pathname, &nd, &name);
	if (error)
		return error;

	switch(nd.last_type) {
	case LAST_DOTDOT:
		error = -ENOTEMPTY;
		goto exit1;
	case LAST_DOT:
		error = -EINVAL;
		goto exit1;
	case LAST_ROOT:
		error = -EBUSY;
		goto exit1;
	}

	nd.flags &= ~LOOKUP_PARENT;

	mutex_lock_nested(&nd.path.dentry->d_inode->i_mutex, I_MUTEX_PARENT);
	dentry = lookup_hash(&nd);
	error = PTR_ERR(dentry);
	if (IS_ERR(dentry))
		goto exit2;
	error = mnt_want_write(nd.path.mnt);
	if (error)
		goto exit3;
	error = security_path_rmdir(&nd.path, dentry);
	if (error)
		goto exit4;
	error = vfs_rmdir(nd.path.dentry->d_inode, dentry);
exit4:
	mnt_drop_write(nd.path.mnt);
exit3:
	dput(dentry);
exit2:
	mutex_unlock(&nd.path.dentry->d_inode->i_mutex);
exit1:
	path_put(&nd.path);
	putname(name);
	return error;
}

SYSCALL_DEFINE1(rmdir, const char __user *, pathname)
{
	return do_rmdir(AT_FDCWD, pathname);
}

int vfs_unlink(struct inode *dir, struct dentry *dentry)
{
	int error = may_delete(dir, dentry, 0);

	if (error)
		return error;

	if (!dir->i_op->unlink)
		return -EPERM;

	vfs_dq_init(dir);

	mutex_lock(&dentry->d_inode->i_mutex);
	if (d_mountpoint(dentry))
		error = -EBUSY;
	else {
		error = security_inode_unlink(dir, dentry);
		if (!error)
			error = dir->i_op->unlink(dir, dentry);
	}
	mutex_unlock(&dentry->d_inode->i_mutex);

	if (!error && !(dentry->d_flags & DCACHE_NFSFS_RENAMED)) {
		fsnotify_link_count(dentry->d_inode);
		d_delete(dentry);
	}

	return error;
}

static long do_unlinkat(int dfd, const char __user *pathname)
{
	int error;
	char *name;
	struct dentry *dentry;
	struct nameidata nd;
	struct inode *inode = NULL;

	error = user_path_parent(dfd, pathname, &nd, &name);
	if (error)
		return error;

	error = -EISDIR;
	if (nd.last_type != LAST_NORM)
		goto exit1;

	nd.flags &= ~LOOKUP_PARENT;

	mutex_lock_nested(&nd.path.dentry->d_inode->i_mutex, I_MUTEX_PARENT);
	dentry = lookup_hash(&nd);
	error = PTR_ERR(dentry);
	if (!IS_ERR(dentry)) {
		 
		if (nd.last.name[nd.last.len])
			goto slashes;
		inode = dentry->d_inode;
		if (inode)
			atomic_inc(&inode->i_count);
		error = mnt_want_write(nd.path.mnt);
		if (error)
			goto exit2;
		error = security_path_unlink(&nd.path, dentry);
		if (error)
			goto exit3;
		error = vfs_unlink(nd.path.dentry->d_inode, dentry);
exit3:
		mnt_drop_write(nd.path.mnt);
	exit2:
		dput(dentry);
	}
	mutex_unlock(&nd.path.dentry->d_inode->i_mutex);
	if (inode)
		iput(inode);	 
exit1:
	path_put(&nd.path);
	putname(name);
	return error;

slashes:
	error = !dentry->d_inode ? -ENOENT :
		S_ISDIR(dentry->d_inode->i_mode) ? -EISDIR : -ENOTDIR;
	goto exit2;
}

SYSCALL_DEFINE3(unlinkat, int, dfd, const char __user *, pathname, int, flag)
{
	if ((flag & ~AT_REMOVEDIR) != 0)
		return -EINVAL;

	if (flag & AT_REMOVEDIR)
		return do_rmdir(dfd, pathname);

	return do_unlinkat(dfd, pathname);
}

SYSCALL_DEFINE1(unlink, const char __user *, pathname)
{
	return do_unlinkat(AT_FDCWD, pathname);
}

int vfs_symlink(struct inode *dir, struct dentry *dentry, const char *oldname)
{
#ifdef CONFIG_FS_SYNO_ACL
	int error = may_create(dir, dentry, S_IFLNK);
#else
	int error = may_create(dir, dentry);
#endif

	if (error)
		return error;

	if (!dir->i_op->symlink)
		return -EPERM;

	error = security_inode_symlink(dir, dentry, oldname);
	if (error)
		return error;

	vfs_dq_init(dir);
	error = dir->i_op->symlink(dir, dentry, oldname);
	if (!error)
		fsnotify_create(dir, dentry);
	return error;
}

SYSCALL_DEFINE3(symlinkat, const char __user *, oldname,
		int, newdfd, const char __user *, newname)
{
	int error;
	char *from;
	char *to;
	struct dentry *dentry;
	struct nameidata nd;

	from = getname(oldname);
	if (IS_ERR(from))
		return PTR_ERR(from);

	error = user_path_parent(newdfd, newname, &nd, &to);
	if (error)
		goto out_putname;

	dentry = lookup_create(&nd, 0);
	error = PTR_ERR(dentry);
	if (IS_ERR(dentry))
		goto out_unlock;

	error = mnt_want_write(nd.path.mnt);
	if (error)
		goto out_dput;
	error = security_path_symlink(&nd.path, dentry, from);
	if (error)
		goto out_drop_write;
	error = vfs_symlink(nd.path.dentry->d_inode, dentry, from);
out_drop_write:
	mnt_drop_write(nd.path.mnt);
out_dput:
	dput(dentry);
out_unlock:
	mutex_unlock(&nd.path.dentry->d_inode->i_mutex);
	path_put(&nd.path);
	putname(to);
out_putname:
	putname(from);
	return error;
}

SYSCALL_DEFINE2(symlink, const char __user *, oldname, const char __user *, newname)
{
	return sys_symlinkat(oldname, AT_FDCWD, newname);
}

int vfs_link(struct dentry *old_dentry, struct inode *dir, struct dentry *new_dentry)
{
	struct inode *inode = old_dentry->d_inode;
	int error;

	if (!inode)
		return -ENOENT;

#ifdef CONFIG_FS_SYNO_ACL
	error = may_create(dir, new_dentry, inode->i_mode);
#else
	error = may_create(dir, new_dentry);
#endif
	if (error)
		return error;

	if (dir->i_sb != inode->i_sb)
		return -EXDEV;

	if (IS_APPEND(inode) || IS_IMMUTABLE(inode))
		return -EPERM;
	if (!dir->i_op->link)
		return -EPERM;
	if (S_ISDIR(inode->i_mode))
		return -EPERM;

	error = security_inode_link(old_dentry, dir, new_dentry);
	if (error)
		return error;

	mutex_lock(&inode->i_mutex);
	vfs_dq_init(dir);
	error = dir->i_op->link(old_dentry, dir, new_dentry);
	mutex_unlock(&inode->i_mutex);
	if (!error)
		fsnotify_link(dir, inode, new_dentry);
	return error;
}

SYSCALL_DEFINE5(linkat, int, olddfd, const char __user *, oldname,
		int, newdfd, const char __user *, newname, int, flags)
{
	struct dentry *new_dentry;
	struct nameidata nd;
	struct path old_path;
	int error;
	char *to;

	if ((flags & ~AT_SYMLINK_FOLLOW) != 0)
		return -EINVAL;

	error = user_path_at(olddfd, oldname,
			     flags & AT_SYMLINK_FOLLOW ? LOOKUP_FOLLOW : 0,
			     &old_path);
	if (error)
		return error;

	error = user_path_parent(newdfd, newname, &nd, &to);
	if (error)
		goto out;
	error = -EXDEV;
	if (old_path.mnt != nd.path.mnt)
		goto out_release;
	new_dentry = lookup_create(&nd, 0);
	error = PTR_ERR(new_dentry);
	if (IS_ERR(new_dentry))
		goto out_unlock;
	error = mnt_want_write(nd.path.mnt);
	if (error)
		goto out_dput;
	error = security_path_link(old_path.dentry, &nd.path, new_dentry);
	if (error)
		goto out_drop_write;
	error = vfs_link(old_path.dentry, nd.path.dentry->d_inode, new_dentry);
out_drop_write:
	mnt_drop_write(nd.path.mnt);
out_dput:
	dput(new_dentry);
out_unlock:
	mutex_unlock(&nd.path.dentry->d_inode->i_mutex);
out_release:
	path_put(&nd.path);
	putname(to);
out:
	path_put(&old_path);

	return error;
}

SYSCALL_DEFINE2(link, const char __user *, oldname, const char __user *, newname)
{
	return sys_linkat(AT_FDCWD, oldname, AT_FDCWD, newname, 0);
}

static int vfs_rename_dir(struct inode *old_dir, struct dentry *old_dentry,
			  struct inode *new_dir, struct dentry *new_dentry)
{
	int error = 0;
	struct inode *target;

	if (new_dir != old_dir) {
#ifdef CONFIG_FS_SYNO_ACL
		if (!IS_SYNOACL(old_dentry->d_inode)) {
			error = inode_permission(old_dentry->d_inode, MAY_WRITE);
		}
#else
		error = inode_permission(old_dentry->d_inode, MAY_WRITE);
#endif
		if (error)
			return error;
	}

	error = security_inode_rename(old_dir, old_dentry, new_dir, new_dentry);
	if (error)
		return error;

	target = new_dentry->d_inode;
	if (target) {
		mutex_lock(&target->i_mutex);
		dentry_unhash(new_dentry);
	}
	if (d_mountpoint(old_dentry)||d_mountpoint(new_dentry))
		error = -EBUSY;
	else 
		error = old_dir->i_op->rename(old_dir, old_dentry, new_dir, new_dentry);
	if (target) {
		if (!error)
			target->i_flags |= S_DEAD;
		mutex_unlock(&target->i_mutex);
		if (d_unhashed(new_dentry))
			d_rehash(new_dentry);
		dput(new_dentry);
	}
	if (!error)
		if (!(old_dir->i_sb->s_type->fs_flags & FS_RENAME_DOES_D_MOVE))
			d_move(old_dentry,new_dentry);
	return error;
}

static int vfs_rename_other(struct inode *old_dir, struct dentry *old_dentry,
			    struct inode *new_dir, struct dentry *new_dentry)
{
	struct inode *target;
	int error;

	error = security_inode_rename(old_dir, old_dentry, new_dir, new_dentry);
	if (error)
		return error;

	dget(new_dentry);
	target = new_dentry->d_inode;
	if (target)
		mutex_lock(&target->i_mutex);
	if (d_mountpoint(old_dentry)||d_mountpoint(new_dentry))
		error = -EBUSY;
	else
		error = old_dir->i_op->rename(old_dir, old_dentry, new_dir, new_dentry);
	if (!error) {
		if (!(old_dir->i_sb->s_type->fs_flags & FS_RENAME_DOES_D_MOVE))
			d_move(old_dentry, new_dentry);
	}
	if (target)
		mutex_unlock(&target->i_mutex);
	dput(new_dentry);
	return error;
}

int vfs_rename(struct inode *old_dir, struct dentry *old_dentry,
	       struct inode *new_dir, struct dentry *new_dentry)
{
	int error;
	int is_dir = S_ISDIR(old_dentry->d_inode->i_mode);
	const char *old_name;

	if (old_dentry->d_inode == new_dentry->d_inode)
 		return 0;
 
	error = may_delete(old_dir, old_dentry, is_dir);
	if (error)
		return error;

	if (!new_dentry->d_inode){
#ifdef CONFIG_FS_SYNO_ACL
		error = may_create(new_dir, new_dentry, old_dentry->d_inode->i_mode);
#else
		error = may_create(new_dir, new_dentry);
#endif
	} else
		error = may_delete(new_dir, new_dentry, is_dir);
	if (error)
		return error;

	if (!old_dir->i_op->rename)
		return -EPERM;

	vfs_dq_init(old_dir);
	vfs_dq_init(new_dir);

	old_name = fsnotify_oldname_init(old_dentry->d_name.name);

	if (is_dir)
		error = vfs_rename_dir(old_dir,old_dentry,new_dir,new_dentry);
	else
		error = vfs_rename_other(old_dir,old_dentry,new_dir,new_dentry);
	if (!error) {
		const char *new_name = old_dentry->d_name.name;
		fsnotify_move(old_dir, new_dir, old_name, new_name, is_dir,
			      new_dentry->d_inode, old_dentry);
	}
	fsnotify_oldname_free(old_name);

	return error;
}

SYSCALL_DEFINE4(renameat, int, olddfd, const char __user *, oldname,
		int, newdfd, const char __user *, newname)
{
	struct dentry *old_dir, *new_dir;
	struct dentry *old_dentry, *new_dentry;
	struct dentry *trap;
	struct nameidata oldnd, newnd;
	char *from;
	char *to;
	int error;

	error = user_path_parent(olddfd, oldname, &oldnd, &from);
	if (error)
		goto exit;

	error = user_path_parent(newdfd, newname, &newnd, &to);
	if (error)
		goto exit1;

	error = -EXDEV;
	if (oldnd.path.mnt != newnd.path.mnt)
		goto exit2;

	old_dir = oldnd.path.dentry;
	error = -EBUSY;
	if (oldnd.last_type != LAST_NORM)
		goto exit2;

	new_dir = newnd.path.dentry;
	if (newnd.last_type != LAST_NORM)
		goto exit2;

	oldnd.flags &= ~LOOKUP_PARENT;
	newnd.flags &= ~LOOKUP_PARENT;
	newnd.flags |= LOOKUP_RENAME_TARGET;

	trap = lock_rename(new_dir, old_dir);

	old_dentry = lookup_hash(&oldnd);
	error = PTR_ERR(old_dentry);
	if (IS_ERR(old_dentry))
		goto exit3;
	 
	error = -ENOENT;
	if (!old_dentry->d_inode)
		goto exit4;
	 
	if (!S_ISDIR(old_dentry->d_inode->i_mode)) {
		error = -ENOTDIR;
		if (oldnd.last.name[oldnd.last.len])
			goto exit4;
		if (newnd.last.name[newnd.last.len])
			goto exit4;
	}
	 
	error = -EINVAL;
	if (old_dentry == trap)
		goto exit4;
#ifdef MY_ABC_HERE
	if (new_dir->d_inode == old_dentry->d_inode) {
		 
		error = -ENOENT;
		goto exit4;
	}

	new_dentry = lookup_hash(&newnd);
	error = PTR_ERR(new_dentry);
	if (IS_ERR(new_dentry))
		goto exit4;

	if (new_dentry->d_inode) {
		if ((new_dir->d_inode->i_ino == old_dir->d_inode->i_ino) &&
			(new_dentry->d_inode->i_ino == old_dentry->d_inode->i_ino)) {
			 
			d_invalidate(new_dentry);
			dput(new_dentry);
			new_dentry = lookup_hash1(&newnd);   
		}
	} else {
		if (memcmp(new_dentry->d_name.name, newnd.last.name, new_dentry->d_name.len)) {
			 
			d_invalidate(new_dentry);
			dput(new_dentry);
			new_dentry = lookup_hash(&newnd);
		}
	}
#else
	new_dentry = lookup_hash(&newnd);
#endif
	error = PTR_ERR(new_dentry);
	if (IS_ERR(new_dentry))
		goto exit4;
	 
	error = -ENOTEMPTY;
	if (new_dentry == trap)
		goto exit5;

	error = mnt_want_write(oldnd.path.mnt);
	if (error)
		goto exit5;
	error = security_path_rename(&oldnd.path, old_dentry,
				     &newnd.path, new_dentry);
	if (error)
		goto exit6;
	error = vfs_rename(old_dir->d_inode, old_dentry,
				   new_dir->d_inode, new_dentry);
exit6:
	mnt_drop_write(oldnd.path.mnt);
exit5:
	dput(new_dentry);
exit4:
	dput(old_dentry);
exit3:
	unlock_rename(new_dir, old_dir);
exit2:
	path_put(&newnd.path);
	putname(to);
exit1:
	path_put(&oldnd.path);
	putname(from);
exit:
	return error;
}

SYSCALL_DEFINE2(rename, const char __user *, oldname, const char __user *, newname)
{
	return sys_renameat(AT_FDCWD, oldname, AT_FDCWD, newname);
}

int vfs_readlink(struct dentry *dentry, char __user *buffer, int buflen, const char *link)
{
	int len;

	len = PTR_ERR(link);
	if (IS_ERR(link))
		goto out;

	len = strlen(link);
	if (len > (unsigned) buflen)
		len = buflen;
	if (copy_to_user(buffer, link, len))
		len = -EFAULT;
out:
	return len;
}

int generic_readlink(struct dentry *dentry, char __user *buffer, int buflen)
{
	struct nameidata nd;
	void *cookie;
	int res;

	nd.depth = 0;
	cookie = dentry->d_inode->i_op->follow_link(dentry, &nd);
	if (IS_ERR(cookie))
		return PTR_ERR(cookie);

	res = vfs_readlink(dentry, buffer, buflen, nd_get_link(&nd));
	if (dentry->d_inode->i_op->put_link)
		dentry->d_inode->i_op->put_link(dentry, &nd, cookie);
	return res;
}

int vfs_follow_link(struct nameidata *nd, const char *link)
{
	return __vfs_follow_link(nd, link);
}

static char *page_getlink(struct dentry * dentry, struct page **ppage)
{
	char *kaddr;
	struct page *page;
	struct address_space *mapping = dentry->d_inode->i_mapping;
	page = read_mapping_page(mapping, 0, NULL);
	if (IS_ERR(page))
		return (char*)page;
	*ppage = page;
	kaddr = kmap(page);
	nd_terminate_link(kaddr, dentry->d_inode->i_size, PAGE_SIZE - 1);
	return kaddr;
}

int page_readlink(struct dentry *dentry, char __user *buffer, int buflen)
{
	struct page *page = NULL;
	char *s = page_getlink(dentry, &page);
	int res = vfs_readlink(dentry,buffer,buflen,s);
	if (page) {
		kunmap(page);
		page_cache_release(page);
	}
	return res;
}

void *page_follow_link_light(struct dentry *dentry, struct nameidata *nd)
{
	struct page *page = NULL;
	nd_set_link(nd, page_getlink(dentry, &page));
	return page;
}

void page_put_link(struct dentry *dentry, struct nameidata *nd, void *cookie)
{
	struct page *page = cookie;

	if (page) {
		kunmap(page);
		page_cache_release(page);
	}
}

int __page_symlink(struct inode *inode, const char *symname, int len, int nofs)
{
	struct address_space *mapping = inode->i_mapping;
	struct page *page;
	void *fsdata;
	int err;
	char *kaddr;
	unsigned int flags = AOP_FLAG_UNINTERRUPTIBLE;
	if (nofs)
		flags |= AOP_FLAG_NOFS;

retry:
	err = pagecache_write_begin(NULL, mapping, 0, len-1,
				flags, &page, &fsdata);
	if (err)
		goto fail;

	kaddr = kmap_atomic(page, KM_USER0);
	memcpy(kaddr, symname, len-1);
	kunmap_atomic(kaddr, KM_USER0);

	err = pagecache_write_end(NULL, mapping, 0, len-1, len-1,
							page, fsdata);
	if (err < 0)
		goto fail;
	if (err < len-1)
		goto retry;

	mark_inode_dirty(inode);
	return 0;
fail:
	return err;
}

int page_symlink(struct inode *inode, const char *symname, int len)
{
	return __page_symlink(inode, symname, len,
			!(mapping_gfp_mask(inode->i_mapping) & __GFP_FS));
}

const struct inode_operations page_symlink_inode_operations = {
	.readlink	= generic_readlink,
	.follow_link	= page_follow_link_light,
	.put_link	= page_put_link,
};

EXPORT_SYMBOL(user_path_at);
EXPORT_SYMBOL(follow_down);
EXPORT_SYMBOL(follow_up);
EXPORT_SYMBOL(get_write_access);  
EXPORT_SYMBOL(getname);
EXPORT_SYMBOL(lock_rename);
EXPORT_SYMBOL(lookup_one_len);
#ifdef MY_ABC_HERE
EXPORT_SYMBOL(lookup_hash);
#else  
#ifdef CONFIG_AUFS_FS
EXPORT_SYMBOL(lookup_hash);
#endif  
#endif
EXPORT_SYMBOL(page_follow_link_light);
EXPORT_SYMBOL(page_put_link);
EXPORT_SYMBOL(page_readlink);
EXPORT_SYMBOL(__page_symlink);
EXPORT_SYMBOL(page_symlink);
EXPORT_SYMBOL(page_symlink_inode_operations);
EXPORT_SYMBOL(path_lookup);
EXPORT_SYMBOL(kern_path);
EXPORT_SYMBOL(vfs_path_lookup);
EXPORT_SYMBOL(inode_permission);
EXPORT_SYMBOL(file_permission);
EXPORT_SYMBOL(unlock_rename);
EXPORT_SYMBOL(vfs_create);
EXPORT_SYMBOL(vfs_follow_link);
EXPORT_SYMBOL(vfs_link);
EXPORT_SYMBOL(vfs_mkdir);
EXPORT_SYMBOL(vfs_mknod);
EXPORT_SYMBOL(generic_permission);
EXPORT_SYMBOL(vfs_readlink);
EXPORT_SYMBOL(vfs_rename);
EXPORT_SYMBOL(vfs_rmdir);
EXPORT_SYMBOL(vfs_symlink);
EXPORT_SYMBOL(vfs_unlink);
EXPORT_SYMBOL(dentry_unhash);
EXPORT_SYMBOL(generic_readlink);
