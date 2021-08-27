#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/kfifo.h>
#include <linux/fdtable.h>
#include <linux/statfs.h>
#include <linux/magic.h>
#include <linux/namei.h>
#include <linux/hardirq.h>
#include <linux/security.h>
#include <linux/uaccess.h>
#include <asm/page.h>

#ifdef MY_ABC_HERE

#define MAX_BUF_SIZE 64
#define MSG_SIZE 256
#define MSG_QUEUE_SIZE 32

extern int syno_hibernation_log_level;
static struct kfifo gMsgQueue;
static spinlock_t gMsgQueueLock;
static struct timer_list gDumpTimer;

extern struct mm_struct *syno_get_task_mm(struct task_struct *task);
extern int syno_access_process_vm(struct task_struct *tsk, unsigned long addr, void *buf, int len, int write);
static int SynoCommGet(struct task_struct *task, char *ptr, int length);
static int SynoUserMMNameGet(struct task_struct *task, char *ptr, int length);
static void SynoProcessNameGet(struct task_struct *task, unsigned char kp, char *buf, int buf_size);
static void SynoFileInfoGet(struct task_struct *pTask, int const fd, char *szFileBuf, char *szDevBuf, int *piIno);
static void SynoHibernationLogForm(const int iIno ,const char __user *szFileName, const char *szDevName, struct kfifo *pMsgQueue);
static void SynoHibernationLogDump(unsigned long data);
static int SynoMsgCheck(const char *szMsg);
static int SynoFsTypeCheck(struct kstatfs *Statfs);

void syno_do_hibernation_scsi_log(const char *szDevName)
{
	char szFileName[MAX_BUF_SIZE] = "SCSI CMD\0";

	if (NULL == szDevName) {
		return;
	}

	SynoHibernationLogForm(-1, szFileName, szDevName, &gMsgQueue);
	 
	if (!timer_pending(&gDumpTimer)) {
		mod_timer(&gDumpTimer, jiffies + 2*HZ);
	}

	return;
			}
EXPORT_SYMBOL(syno_do_hibernation_scsi_log);

void syno_do_hibernation_bio_log(const char *szDevName)
{
	char szFileName[MAX_BUF_SIZE] = "Block I/O\0";

    if (NULL == szDevName) {
		return;
	}

	SynoHibernationLogForm(-1, szFileName, szDevName, &gMsgQueue);

	return;
}
EXPORT_SYMBOL(syno_do_hibernation_bio_log);

void syno_do_hibernation_inode_log(struct inode *inode)
{
	int iIno = inode->i_ino & INT_MAX;
	struct dentry *pDentry = d_find_alias(inode);
	const char *szFileName = NULL;
	const char *szDevName = NULL;
	struct kstatfs Statfs;
	int iErr = -1;

    if (NULL == pDentry) {
		return;
    }

	spin_lock(&pDentry->d_lock);

	szFileName = (const char *) pDentry->d_name.name;
	szDevName = inode->i_sb->s_id;
	 
	if (NULL != pDentry->d_sb->s_op->statfs && 0 == security_sb_statfs(pDentry)) {
		iErr = pDentry->d_sb->s_op->statfs(pDentry, &Statfs);
	}

	spin_unlock(&pDentry->d_lock);
	dput(pDentry);
	 
	if (0 != iErr || 0 == SynoFsTypeCheck(&Statfs)) {
		goto END;
	}

	SynoHibernationLogForm(iIno, szFileName, szDevName, &gMsgQueue);

END:
	return;
}
EXPORT_SYMBOL(syno_do_hibernation_inode_log);

void syno_do_hibernation_filename_log(const char __user *szUserFileName)
{
	struct kstatfs Statfs;
	struct path FilePath;
	int iIno = 0;
	char szFileName[MSG_SIZE];

	if (NULL == szUserFileName) {
		goto END;
	}

	if (copy_from_user(szFileName, szUserFileName, MSG_SIZE)) {
		goto END;
	}
	if (MSG_SIZE > strlen(szFileName)) {
		strncpy(szFileName, szFileName, strlen(szFileName));
	} else {
		szFileName[MSG_SIZE-1] = '\0';
	}

	if (0 != user_lpath(szFileName, &FilePath)) {
		goto END;
	}
	 
	if (0 != vfs_statfs(&FilePath, &Statfs)) {
		goto END;
	}

	if (0 == SynoFsTypeCheck(&Statfs)) {
		goto END;
	}
	 
	iIno = FilePath.dentry->d_inode->i_ino & INT_MAX;
	SynoHibernationLogForm(iIno, szFileName, NULL, &gMsgQueue);

END:
	return;
}
EXPORT_SYMBOL(syno_do_hibernation_filename_log);

void syno_do_hibernation_fd_log(const int fd)
{
	char szDevName[BDEVNAME_SIZE] = {'\0'};
	char szFileName[MAX_BUF_SIZE] = {'\0'};
	int iIno = 0;
	struct kstatfs Statfs;

	if (0 != fd_statfs(fd, &Statfs)) {
		goto END;
	}

	if (0 == SynoFsTypeCheck(&Statfs)) {
		goto END;
	}

	SynoFileInfoGet(current, fd, szFileName, szDevName, &iIno);
	SynoHibernationLogForm(iIno, szFileName, szDevName, &gMsgQueue);

END:
	return;
}
EXPORT_SYMBOL(syno_do_hibernation_fd_log);

static void SynoHibernationLogForm(const int iIno ,const char __user *szFileName, const char *szDevName, struct kfifo *pMsgQueue)
{
	char p_cups[MAX_BUF_SIZE] = {'\0'};
	char p_kups[MAX_BUF_SIZE] = {'\0'};
	char szParent[MAX_BUF_SIZE] = {'\0'};
	char szCurrent[MSG_SIZE] = {'\0'};
	char szMsg[MSG_SIZE] = {'\0'};
	struct task_struct *Parent = current->parent;
	int iErr = -1;

	if (NULL == pMsgQueue) {
		goto END;
	}
	 
	if (!kfifo_initialized(pMsgQueue)) {
		spin_lock_init(&gMsgQueueLock);
		iErr = kfifo_alloc(pMsgQueue, MSG_QUEUE_SIZE*MSG_SIZE, GFP_KERNEL);
		setup_timer(&gDumpTimer, SynoHibernationLogDump, 0);

		if (0 != iErr) {
			printk(KERN_DEBUG"[Hibernation Debug]: Queue init Failed.");

			goto END;
		}
	}

    if (NULL == szFileName) {
		szFileName = "Missing\0";
    }else if (strstr(szFileName, "pipe:[") == szFileName ||
			  strstr(szFileName, "socket:[") == szFileName ||
			  strstr(szFileName, "/etc/ld.so.cache") != NULL ||  
			  strstr(szFileName, "eventfd") != NULL) {  
		iErr = 0;
		goto END;
	}

    SynoProcessNameGet(Parent, 1, szParent, MAX_BUF_SIZE);
	 
    SynoProcessNameGet(current, 1, p_kups, MAX_BUF_SIZE);
     
    if (3 > syno_hibernation_log_level || 0 > iIno) {
		snprintf(szCurrent, sizeof(szCurrent), "comm:(%s)", p_kups);
    }else {
		SynoProcessNameGet(current, 0, p_cups, MAX_BUF_SIZE);
		snprintf(szCurrent, sizeof(szCurrent), "comm:(%s), u:(%s)", p_kups, p_cups);
    }

	if (3 > syno_hibernation_log_level) {
		if (strstr(szFileName, "/usr/syno/lib") == szFileName ||  
			strstr(szFileName, "/usr/lib") == szFileName ||
			strstr(szFileName, "/lib") == szFileName ||
			strstr(szCurrent, "syslog-ng") != NULL ||
			strstr(szCurrent, "swapper") != NULL ||  
			strstr(szCurrent, "kworker") != NULL ||
			strstr(szCurrent, "ksoftirqd") != NULL ||
			strstr(szCurrent, "jbd2") != NULL ||
			strstr(szCurrent, "_raid") != NULL) {
				iErr = 0;
				goto END;
		}
	}

	memset(szMsg, 0 ,sizeof(szMsg));
	snprintf(szMsg, sizeof(szMsg), "[%s]", szFileName);

	if (0 < iIno) {
		snprintf(szMsg, sizeof(szMsg), "%s (%hu)", szMsg, iIno);
	}

	if (NULL != szDevName && strcmp(szDevName, "")) {
		snprintf(szMsg, sizeof(szMsg), "%s on [%s]", szMsg, szDevName);
	}

	snprintf(szMsg, sizeof(szMsg), "%s - pid %d [%s], ppid %d [%s] \n",
			szMsg, current->pid, szCurrent, Parent->pid, szParent);

	if (0 == SynoMsgCheck(szMsg)) {
		iErr = 0;
		goto END;
	}

	if (0 == kfifo_in_spinlocked(&gMsgQueue, szMsg, MSG_SIZE, &gMsgQueueLock)) {
        goto END;
	}

	iErr = 0;
END:
	return;
}

static void SynoHibernationLogDump(unsigned long data)
{
	char szMsg[MSG_SIZE];
	int iErr = -1;

	while (!kfifo_is_empty(&gMsgQueue)) {
		memset(szMsg, 0 ,sizeof(szMsg));
		iErr = kfifo_out_spinlocked(&gMsgQueue, szMsg, MSG_SIZE, &gMsgQueueLock);
		if (0 == iErr) {
			printk(KERN_DEBUG"Fail to get message. Queue reseted. \n");
			kfifo_reset(&gMsgQueue);
			break;
		}
		printk("%s", szMsg);
	}
}

static int SynoMsgCheck(const char *szMsg)
{
	static char szUsedMsg[MSG_SIZE] ={'\0'};
	static unsigned long ulLastCheck = 0;

	int ret = -1;

	ret = strcmp(szUsedMsg, szMsg);

	if (0 != ret) {
		if (time_after(jiffies, ulLastCheck + 2*HZ)) {
			kfifo_reset(&gMsgQueue);
		}
		ulLastCheck = jiffies;
		strlcpy(szUsedMsg, szMsg, MSG_SIZE);
	}

	return ret;
}

static int SynoFsTypeCheck(struct kstatfs *Statfs)
{
	int iFsType;
	int ret = -1;

	if (NULL == Statfs) {
		ret = 0;
		goto END;
	}

	iFsType = Statfs->f_type;

	if (SYSFS_MAGIC == iFsType ||
	    RAMFS_MAGIC == iFsType ||
	    TMPFS_MAGIC == iFsType ||
	    NFS_SUPER_MAGIC == iFsType ||
	    PROC_SUPER_MAGIC == iFsType ||
	    SECURITYFS_MAGIC == iFsType ||
	    DEVPTS_SUPER_MAGIC == iFsType) {
		ret = 0;
		goto END;
	}

	ret = iFsType;
END:
	return ret;
}

static void SynoFileInfoGet(struct task_struct *pTask, int const fd, char *szFileBuf, char *szDevBuf, int *piIno)
{
	int iErr=-1;
	char *pageTmp;
	struct files_struct *pFileStr;
	struct file *pFile;
	struct path FilePath;
	struct dentry *pDentry;
	struct inode *pInode;

	if ((NULL == pTask) || (NULL == szFileBuf) || (NULL == szDevBuf)) {
		goto END;
	}

	memset(szFileBuf, 0, MAX_BUF_SIZE);
	memset(szDevBuf, 0, BDEVNAME_SIZE);

	pFileStr = pTask->files;
	spin_lock(&pFileStr->file_lock);

	pFile = fcheck_files(pFileStr, fd);

	if (!pFile) {
		spin_unlock(&pFileStr->file_lock);
		snprintf(szFileBuf, sizeof(szFileBuf), "UNKOWN");
		snprintf(szDevBuf, sizeof(szDevBuf), "UNKOWN");
		iErr = 0;
		goto END;
	}
	FilePath = pFile->f_path;
	path_get(&pFile->f_path);
	spin_unlock(&pFileStr->file_lock);
	pageTmp = (char *)__get_free_page(GFP_TEMPORARY);

	if (!pageTmp) {
		path_put(&FilePath);
		printk(KERN_DEBUG"[Hibernation debug error]: Get page failed.\n");
		goto END;
	}

	strlcpy(szFileBuf, d_path(&FilePath, pageTmp, PAGE_SIZE), MAX_BUF_SIZE);
	pDentry = FilePath.dentry;
	szDevBuf = (char *) pDentry->d_name.name;
	pInode = pDentry->d_inode;
	*piIno = pInode->i_ino & INT_MAX;
	path_put(&FilePath);
	free_page((unsigned long)pageTmp);

    iErr = 0;

END:
	if (0 != iErr) {
		szFileBuf[0] = '\0';
		szDevBuf[0] = '\0';
	}
	return;
}

static void SynoProcessNameGet(struct task_struct *task, unsigned char kp, char *buf, int buf_size)
{
	if(0 >= buf_size) {
		goto END;
	}

	memset(buf, 0, buf_size);
	if(kp) {
		if(SynoCommGet(task, buf, buf_size) < 0){
			buf[0] = '\0';
			goto END;
		}
	}else{
		if(SynoUserMMNameGet(task, buf, buf_size) < 0){
			buf[0] = '\0';
			goto END;
		}
	}
END:
	return;
}

static int SynoCommGet(struct task_struct *task, char *ptr, int length)
{
	if(!task){
		return -1;
	}

	strlcpy(ptr, task->comm, length);
	return 0;
}

static int SynoUserMMNameGet(struct task_struct *task, char *ptr, int length)
{
	struct mm_struct *mm;
	int len = 0;
	int res = -1;
	int res_len = -1;
	int iBufferIdx = 0;
	char buffer[256];
	int buf_size = sizeof(buffer);

	if(!task) {
		return -1;
	}

	mm = syno_get_task_mm(task);
	if(!mm) {
		printk("%s %d get_task_mm_syno == NULL \n", __FUNCTION__, __LINE__);
		goto END;
	}

	if(!mm->arg_end) {
		printk("!mm->arg_end \n");
		goto END;
	}

	len = mm->arg_end - mm->arg_start;

	if(len <= 0) {
		printk("len <= 0 \n");
		goto END;
	}

	if(len > PAGE_SIZE) {
		len = PAGE_SIZE;
	}

	if(len > buf_size) {
		len = buf_size;
	}

	res_len = syno_access_process_vm(task, mm->arg_start, buffer, len, 0);
	if(res_len <= 0) {
		printk(KERN_DEBUG"access_process_vm_syno  fail\n");
		goto END;
	}

	for (iBufferIdx = 0;iBufferIdx < res_len;iBufferIdx++)
	{
		if (buffer[iBufferIdx] == '\0' ) {
			buffer[iBufferIdx] = ' ';
		}
	}

	if(res_len >= buf_size) {
		res_len = buf_size-1;
	}
	buffer[res_len] = '\0';
	strlcpy(ptr, buffer, length);

	res = 0;
END:
	if(mm) {
		mmput(mm);
	}
	return res;
}
#endif  
