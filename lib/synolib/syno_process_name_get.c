#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
/* Copyright (c) 2000-2014 Synology Inc. All rights reserved. */
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/kfifo.h>
#include <linux/fdtable.h>
#include <linux/statfs.h>
#include <linux/magic.h>
#include <linux/namei.h>
#include <asm/page.h>

#ifdef SYNO_DEBUG_FLAG

#define MAX_BUF_SIZE 64
#define MSG_SIZE 256
#define MSG_QUEUE_SIZE 8

extern int syno_hibernation_log_level;
unsigned long gScsiLogTimeout;
unsigned int gcMsgCount;
static struct kfifo gMsgQueue;
static spinlock_t gMsgQueueLock;

extern struct mm_struct *syno_get_task_mm(struct task_struct *task);
extern int syno_access_process_vm(struct task_struct *tsk, unsigned long addr, void *buf, int len, int write);
static int SynoCommGet(struct task_struct *task, char *ptr, int length);
static int SynoUserMMNameGet(struct task_struct *task, char *ptr, int length);
static void SynoProcessNameGet(struct task_struct *task, unsigned char kp, char *buf, int buf_size);
static void SynoFileNameGet(struct task_struct *pTask, int fd, char *szBuf, size_t cbBuf);
static void SynoHibernationLogForm(const char __user *szFileName);
static void SynoHibernationLogDump(void);
static int SynoMsgChek(const char* szMsg);
static int SynoFsTypeChek(struct kstatfs *statfs);

/**
 * Prints out the hibernation debug message.
 * This function is called whenever a scsi command is issued and the log level fits.
 * It will dump all filesystem level hibernation debug log queued and generate a message with
 * information provided from scsi layer.
 *
 * @param DeviceName	[IN]the device name that scsi command is issued on
 */
void syno_do_hibernation_log_print(const char *DeviceName)
{
	struct task_struct *Parent;

	/* dump all FS layer messages in the queue before printing out scsi layer message */
	SynoHibernationLogDump();

    /* these logs are not helpful, printing them is meaningless in most cases. Only shown in higher log level */
	if (syno_hibernation_log_level < 3){
		if(strstr(current->comm, "swapper") != NULL ||
			strstr(current->comm, "kworker") != NULL ||
			strstr(current->comm, "_raid") != NULL){
		goto END;
	}
	}
    /* print out the filtered scsi layer log message*/
    Parent = current->parent;
	printk("[%s]: scsi cmd issued - pid:%d, comm:%s, ppid:%d(%s)\n",
			DeviceName,current->pid, current->comm, Parent->pid, Parent->comm);
END:
	/* set up timeout and counter, following messages will be printed out instead of storing in the queue. */
	gScsiLogTimeout = jiffies + 2*HZ;
	gcMsgCount = MSG_QUEUE_SIZE;
	return;
}
EXPORT_SYMBOL(syno_do_hibernation_log_print);

/**
 * Trigger FS layer hibernation message generation
 * Check the file system type and generate file system layer
 * hibernation debug message with provided file name.
 *
 * @param szFileName	[IN] The file name that is operated in FS system call
 */
void syno_do_hibernation_log(const char __user *szFileName)
{
	struct kstatfs statfs;
	struct path filepath;

	/* Get filepath  */
	if(0 != user_lpath(szFileName, &filepath)){
		goto END;
	}

	/* Get stat of file system */
	if(0 != vfs_statfs(&filepath, &statfs)){
		goto END;
	}

	/* checking for filesystem type */
	if(0 == SynoFsTypeChek(&statfs)){
		goto END;
	}

	SynoHibernationLogForm(szFileName);

END:
	return;
}
EXPORT_SYMBOL(syno_do_hibernation_log);

/**
 * Trigger FS layer hibernation message generation.
 * Check the file system type and generate file system layer
 * hibernation debug message with provided file descriptor.
 *
 * @param fd	[IN] The file descriptor that is operated in FS system call
 */
void syno_do_hibernation_fd_log(const int fd)
{
	char szFileName[MAX_BUF_SIZE];
	struct kstatfs statfs;

	/* Get stat of file system */
	if(0 != fd_statfs(fd, &statfs)){
		goto END;
	}

	/* checking for filesystem type */
	if(0 == SynoFsTypeChek(&statfs)){
		goto END;
	}

	SynoFileNameGet(current, fd, szFileName, sizeof(szFileName));
	SynoHibernationLogForm(szFileName);

END:
	return;
}
EXPORT_SYMBOL(syno_do_hibernation_fd_log);

/* The FS layer hibernation logs are formed and queued here */
static void SynoHibernationLogForm(const char __user *szFileName)
{
	char p_cups[MAX_BUF_SIZE], p_ckps[MAX_BUF_SIZE], szParent[MAX_BUF_SIZE], szMsg[MSG_SIZE];
    int error = -1;

	if(true == kfifo_initialized(&gMsgQueue)){
		error = kfifo_alloc(&gMsgQueue, MSG_QUEUE_SIZE*MSG_SIZE, GFP_KERNEL);
		spin_lock_init(&gMsgQueueLock);
		}

    if(NULL==szFileName){
		szFileName = "UNKNOWN";
	}

	/* A simple black list of filenames that should not be considered as the cause of hibernation issue */
    if(strstr(szFileName, "pipe:[") == szFileName ||
			  strstr(szFileName, "socket:[") == szFileName ||
			  strstr(szFileName, "/etc/ld.so.cache") != NULL || //a cache list of dynamic libraries.
	   strstr(szFileName, "eventfd") != NULL){ //an eventfd does not descript a real file on disk.
		error = 0;
		goto END;
	}

    /* get the process name */
		SynoProcessNameGet(current, 0, p_cups, MAX_BUF_SIZE);
	SynoProcessNameGet(current, 1, p_ckps, MAX_BUF_SIZE);
	SynoProcessNameGet(current->parent, 1, szParent, MAX_BUF_SIZE);

	/* form the message here*/
	memset(szMsg, 0 ,sizeof(szMsg));
	snprintf(szMsg, sizeof(szMsg), "[%s] accessed - pid %d [u:(%s), comm:(%s)], parent [%s]\n",
			 szFileName, current->pid, p_cups, p_ckps, szParent);

	/* These logs are only shown in higher log level*/
	if (syno_hibernation_log_level < 3){
		if(strstr(szMsg, "syslog-ng") != NULL || //it is obvious that syslog-ng is accessing disk while writing any other logs.
			strstr(szFileName, "/usr/syno/lib") == szFileName || //loading share libraries are not likely to affect disk hibernation.
			strstr(szFileName, "/lib") == szFileName){
				error = 0;
				goto END;
	}
	}

	/* checking for the latest log message*/
	if(0 == SynoMsgChek(szMsg)){
		error = 0;
		goto END;
	}

	/* a fs layer log message will be printed out immediately if there was an scsi command issued in the past 2 sec,
	  otherwise the message will be queued in the kernel queue */
	if((0 < gcMsgCount) && (time_before(jiffies, gScsiLogTimeout))){
		printk("%s",szMsg);
		gcMsgCount--;
	}else if( 0 > kfifo_in_spinlocked(&gMsgQueue, szMsg, MSG_SIZE, &gMsgQueueLock)){
        goto END;
	}

	error = 0;
END:
    if (0 > error){
		printk(KERN_INFO"[Hibernation debug error]: Fail on hibernation log\n");
    }

	return;
}

/* Output the FS layer hibernation debug messages queued.
 * Print out all log messages in the kernel queue one after another
 * until the queue is empty.
 * Reset the whole kernel queue if there is something wrong about it.
 */
static void SynoHibernationLogDump(void)
{
	char szMsg[MSG_SIZE];
	while (!kfifo_is_empty(&gMsgQueue)) {
		memset(szMsg, 0 ,sizeof(szMsg));
		if(0 > kfifo_out_spinlocked(&gMsgQueue, szMsg, MSG_SIZE, &gMsgQueueLock)){
			printk(KERN_INFO"Fail to get message. Queue reseted. \n");
			kfifo_reset(&gMsgQueue);
			break;
		}
		printk("%s", szMsg);
	}
}

/*FIX ME:A simple and crude mechanism that only checks for the last message.
Need to be further designed to guarantee the quality of log printed.*/
static int SynoMsgChek(const char* szMsg)
{
	static char szUsedMsg[MSG_SIZE] ={'\0'};
	static long uiLastCheck = 0;

	int ret = -1;

	ret = strcmp(szUsedMsg, szMsg);

	if (0 != ret) {
		if(time_before(jiffies, uiLastCheck + 2*HZ))
		{
			kfifo_reset(&gMsgQueue);
		}
		uiLastCheck = jiffies;
		strcpy(szUsedMsg, szMsg);
	}

	return ret;
}

/* checks if a file is on proc, sysfs, ramfs, tmpfs, securityfs ,or nfs */
static int SynoFsTypeChek(struct kstatfs *statfs)
{
	int iFsType;
	int ret = -1;

	if(NULL == statfs){
		ret = 0;
		goto END;
	}

	iFsType = statfs->f_type;

	/* check filesystem type magic */
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

static void SynoFileNameGet(struct task_struct *pTask, int fd, char *szBuf, size_t cbBuf)
{
	int error=-1;
	char *pageTmp;
	struct files_struct *pFileStr;
	struct file *pFile;
	struct path FilePath;

	if((NULL == pTask) || (NULL == szBuf) || (0 >= cbBuf)){
		goto END;
	}

	memset(szBuf, 0 ,cbBuf);

	pFileStr = pTask->files;
	spin_lock(&pFileStr->file_lock);

	pFile = fcheck_files(pFileStr, fd);

	if (!pFile) {
		spin_unlock(&pFileStr->file_lock);
		snprintf(szBuf, sizeof(szBuf), "UNKOWN");
		error = 0;
		goto END;
	}

	FilePath = pFile->f_path;
	path_get(&pFile->f_path);
	spin_unlock(&pFileStr->file_lock);

	pageTmp = (char *)__get_free_page(GFP_TEMPORARY);

	if (!pageTmp) {
		path_put(&FilePath);
		printk(KERN_INFO"[Hibernation debug error]: Get page failed.\n");
		goto END;
	}

	strlcpy(szBuf, d_path(&FilePath, pageTmp, PAGE_SIZE), cbBuf);

	path_put(&FilePath);
	free_page((unsigned long)pageTmp);

    error = 0;

END:
	if(0 != error){
		szBuf[0] = '\0';
	}
	return;
}

/**
 * Process name get
 *
 * @param task     [IN] task structure, use for get process info.
 *                 Should not be NULL
 * @param kp       [IN]
 *                 0: get user mm task name
 *                 1: get task->comm process name
 *                 hould not be NULL.
 * @param buf      [IN] for copy process name, Should not be NULL.
 * @param buf_size [IN] buf size, Should more than 1.
 */
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
		printk(KERN_INFO"access_process_vm_syno  fail\n");
		goto END;
	}

	/*repalce all 0 by space to aviod string formate problem*/
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

#endif //SYNO_DEBUG_FLAG
