#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifdef CONFIG_OF
#include <linux/proc_fs.h>
#endif

#define MV_SEM_STAT
typedef struct {
    int                 flags;
    int                 count;
    mv_waitqueue_t      waitqueue;
    struct task_struct  *owner;
    char name[MV_SEM_NAME_LEN+1];
#ifdef  MV_SEM_STAT
    int                 tcount;
    int                 gcount;
    int                 wcount;
#endif
} mvSemaphoreSTC;

static mvSemaphoreSTC   *mvSemaphores = NULL;
static int              mv_num_semaphores = MV_SEMAPHORES_DEF;

module_param(mv_num_semaphores, int, S_IRUGO);

#if defined(MY_ABC_HERE) && defined(CONFIG_OF)
 
#else  
 
static int mvKernelExtSem_read_proc_mem(
        char * page,
        char **start,
        off_t offset,
        int count,
        int *eof,
        void *data)
{
    int len;
    int k;
    int begin = 0;

    len = 0;

    len += sprintf(page+len,"id type count owner");
#ifdef MV_SEM_STAT
    len += sprintf(page+len," tcount gcount wcount");
#endif
    len += sprintf(page+len," name\n");
    for (k = 1; k < mv_num_semaphores; k++)
    {
        mvSemaphoreSTC *sem;
        struct mv_task *p;

        if (!mvSemaphores[k].flags)
            continue;
        sem = mvSemaphores + k;

        len += sprintf(page+len,"%d %c %d %d",
                k,
                (sem->flags & MV_SEMAPTHORE_F_MTX) ? 'M' :
                    (sem->flags & MV_SEMAPTHORE_F_COUNT) ? 'C' :
                    (sem->flags & MV_SEMAPTHORE_F_BINARY) ? 'B' : '?',
                sem->count,
                sem->owner?(sem->owner->pid):0);
#ifdef MV_SEM_STAT
        len += sprintf(page+len," %d %d %d", sem->tcount, sem->gcount, sem->wcount);
#endif
        if (sem->name[0])
            len += sprintf(page+len," %s", sem->name);
        page[len++] = '\n';

        for (p = sem->waitqueue.first; p; p = p->wait_next)
            len += sprintf(page+len, "  q=%d\n", p->task->pid);

        if (len+begin < offset)
        {
            begin += len;
            len = 0;
        }
        if (len+begin >= offset+count)
            break;
    }
    if (len+begin < offset)
        *eof = 1;
    offset -= begin;
    *start = page + offset;
    len -= offset;
    if (len > count)
        len = count;
    if (len < 0)
        len = 0;

    return len;
}
#endif  

#ifdef CONFIG_OF
static int proc_status_show(struct seq_file *m, void *v) {
    int k;

    seq_printf(m, "id type count owner");
#ifdef MV_SEM_STAT
    seq_printf(m, " tcount gcount wcount");
#endif
    seq_printf(m, " name\n");
    for (k = 1; k < mv_num_semaphores; k++)
    {
        mvSemaphoreSTC *sem;
        struct mv_task *p;

        if (!mvSemaphores[k].flags)
            continue;
        sem = mvSemaphores + k;

        seq_printf(m, "%d %c %d %d",
                k,
                (sem->flags & MV_SEMAPTHORE_F_MTX) ? 'M' :
                    (sem->flags & MV_SEMAPTHORE_F_COUNT) ? 'C' :
                    (sem->flags & MV_SEMAPTHORE_F_BINARY) ? 'B' : '?',
                sem->count,
                sem->owner?(sem->owner->pid):0);
#ifdef MV_SEM_STAT
        seq_printf(m," %d %d %d", sem->tcount, sem->gcount, sem->wcount);
#endif
        if (sem->name[0])
            seq_printf(m, " %s", sem->name);
	seq_putc(m, '\n');

        for (p = sem->waitqueue.first; p; p = p->wait_next)
            seq_printf(m,  "  q=%d\n", p->task->pid);

    }

    return 0;

}

static int proc_status_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_status_show, PDE_DATA(inode));
}

static const struct file_operations mv_kext_sem_read_proc_operations = {
	.open		= proc_status_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};
#endif

static int mvKernelExt_SemInit(void)
{
    if (mv_num_semaphores < MV_SEMAPHORES_MIN)
        mv_num_semaphores = MV_SEMAPHORES_MIN;

    mvSemaphores = (mvSemaphoreSTC*) kmalloc(
            mv_num_semaphores * sizeof(mvSemaphoreSTC), GFP_KERNEL);

    if (mvSemaphores == NULL)
    {
        if (mvSemaphores)
            kfree(mvSemaphores);
        mvSemaphores = NULL;
        return 0;
    }

    memset(mvSemaphores, 0, mv_num_semaphores * sizeof(mvSemaphoreSTC));

#ifdef CONFIG_OF
	if (!proc_create("mvKernelExtSem", S_IRUGO, NULL, &mv_kext_sem_read_proc_operations))
		return -ENOMEM;
#else
     
    create_proc_read_entry("mvKernelExtSem", 0, NULL, mvKernelExtSem_read_proc_mem, NULL);
#endif

    return 1;
}

static void mvKernelExt_DeleteAll(void)
{
    int k;

    for (k = 1; k < mv_num_semaphores; k++)
    {
        if (mvSemaphores[k].flags)
        {
            mv_waitqueue_wake_all(&(mvSemaphores[k].waitqueue));
            mv_waitqueue_cleanup(&(mvSemaphores[k].waitqueue));
        }
        mvSemaphores[k].flags = 0;
    }
}

static void mvKernelExt_SemCleanup(void)
{
    MV_GLOBAL_LOCK();

    if (mvSemaphores)
    {
        mvKernelExt_DeleteAll();
        kfree(mvSemaphores);
    }

    mvSemaphores = NULL;

    MV_GLOBAL_UNLOCK();

    remove_proc_entry("mvKernelExtSem", NULL);
}

int mvKernelExt_SemCreate(int flags, const char *name)
{
    int k;
    mvSemaphoreSTC *sem = NULL;

    if ((flags & MV_SEMAPTHORE_F_TYPE_MASK) == 0)
        return -MVKERNELEXT_EINVAL;

    MV_GLOBAL_LOCK();

    if (flags & MV_SEMAPTHORE_F_OPENEXIST)
    {
         
        for (k = 1; k < mv_num_semaphores; k++)
        {
            sem = mvSemaphores + k;
            if (!sem->flags)
                continue;

            if (!strncmp(sem->name, name, MV_SEM_NAME_LEN))
                break;
        }

        if (k < mv_num_semaphores)
        {
             
            if ((sem->flags & MV_SEMAPTHORE_F_TYPE_MASK) !=
                    (flags & MV_SEMAPTHORE_F_TYPE_MASK))
            {
                 
                MV_GLOBAL_UNLOCK();
                return -MVKERNELEXT_ECONFLICT;
            }

            MV_GLOBAL_UNLOCK();
            return k;
        }
    }

    for (k = 1; k < mv_num_semaphores; k++)
    {
        if (mvSemaphores[k].flags == 0)
            break;
    }
    if (k >= mv_num_semaphores)
    {
        MV_GLOBAL_UNLOCK();
        return -MVKERNELEXT_ENOMEM;
    }
    sem = mvSemaphores + k;

    memset(sem, 0, sizeof(*sem));
    mv_waitqueue_init(&(sem->waitqueue));

    sem->flags = flags & MV_SEMAPTHORE_F_TYPE_MASK;

    if (sem->flags == MV_SEMAPTHORE_F_MTX)
        sem->count = 1;
    if (sem->flags == MV_SEMAPTHORE_F_BINARY)
        sem->count = (flags & MV_SEMAPTHORE_F_COUNT_MASK) ? 1 : 0;
    if (sem->flags == MV_SEMAPTHORE_F_COUNT)
        sem->count = flags & MV_SEMAPTHORE_F_COUNT_MASK;

    strncpy(sem->name, name, MV_SEM_NAME_LEN);
    sem->name[MV_SEM_NAME_LEN] = 0;

    MV_GLOBAL_UNLOCK();
    return k;
}

#define SEMAPHORE_BY_ID(semid) \
    MV_GLOBAL_LOCK(); \
    if (unlikely(semid == 0 || semid >= mv_num_semaphores)) \
    { \
ret_einval: \
        MV_GLOBAL_UNLOCK(); \
        return -MVKERNELEXT_EINVAL; \
    } \
    sem = mvSemaphores + semid; \
    if (unlikely(!sem->flags)) \
        goto ret_einval;

#define TASK_WILL_WAIT(tsk) \
    struct mv_task *p; \
    if (unlikely((p = gettask_cr(tsk)) == NULL)) \
    { \
        MV_GLOBAL_UNLOCK(); \
        return -MVKERNELEXT_ENOMEM; \
    }
#define CHECK_SEM() \
    if (unlikely(!sem->flags)) \
    { \
        MV_GLOBAL_UNLOCK(); \
        return -MVKERNELEXT_EDELETED; \
    }

int mvKernelExt_SemDelete(int semid)
{
    mvSemaphoreSTC *sem;

    SEMAPHORE_BY_ID(semid);
    mv_waitqueue_wake_all(&(sem->waitqueue));
    mv_waitqueue_cleanup(&(sem->waitqueue));
    sem->flags = 0;

    MV_GLOBAL_UNLOCK();
    return 0;
}

int mvKernelExt_SemSignal(int semid)
{
    mvSemaphoreSTC *sem;

    SEMAPHORE_BY_ID(semid);

    if (sem->flags & MV_SEMAPTHORE_F_MTX)
    {
        if (unlikely(sem->owner != current))
        {
            MV_GLOBAL_UNLOCK();
            return -MVKERNELEXT_EPERM;
        }
        sem->count++;
        if (sem->count > 0)
        {
            sem->owner = NULL;
            mv_waitqueue_wake_first(&(sem->waitqueue));
        }
    } else {
        if (sem->flags & MV_SEMAPTHORE_F_COUNT)
            sem->count++;
        else
            sem->count = 1;  
        mv_waitqueue_wake_first(&(sem->waitqueue));
    }

#ifdef MV_SEM_STAT
    sem->gcount++;
#endif
    MV_GLOBAL_UNLOCK();
    return 0;
}

static void mvKernelExt_SemUnlockMutexes(
        struct task_struct  *owner
)
{
    int k;
    for (k = 1; k < mv_num_semaphores; k++)
    {
        mvSemaphoreSTC *sem = mvSemaphores + k;

        if ((sem->flags & MV_SEMAPTHORE_F_MTX) != 0
                && sem->owner == owner)
        {
            sem->count = 1;
            sem->owner = NULL;
            mv_waitqueue_wake_first(&(sem->waitqueue));
        }
    }
}

static int mvKernelExt_SemTryWait_common(
        mvSemaphoreSTC *sem
)
{
    if (sem->flags & MV_SEMAPTHORE_F_MTX)
    {
        if (sem->count > 0)
        {
            sem->count--;
            sem->owner = current;
        } else {
            if (sem->owner == current)
                sem->count--;
            else
                return -MVKERNELEXT_EBUSY;
        }
    } else {
        if (sem->count <= 0)
            return -MVKERNELEXT_EBUSY;
        sem->count--;
    }
#ifdef MV_SEM_STAT
    sem->tcount++;
#endif
    return 0;
}

int mvKernelExt_SemTryWait(int semid)
{
    mvSemaphoreSTC *sem;
    int ret;

    SEMAPHORE_BY_ID(semid);

    ret = mvKernelExt_SemTryWait_common(sem);

    MV_GLOBAL_UNLOCK();
    return ret;
}

int mvKernelExt_SemWait(int semid)
{
    mvSemaphoreSTC *sem;

    SEMAPHORE_BY_ID(semid);

    if (mvKernelExt_SemTryWait_common(sem) != 0)
    {
        TASK_WILL_WAIT(current);
#ifdef  MV_SEM_STAT
        sem->wcount++;
#endif
        if (sem->flags & MV_SEMAPTHORE_F_MTX)
        {
            do {
                if (unlikely(mv_do_short_wait_on_queue(&(sem->waitqueue), p, &(sem->owner))))
                {
                    MV_GLOBAL_UNLOCK();
                    return -MVKERNELEXT_EINTR;
                }
                CHECK_SEM();
            } while (sem->count <= 0);

            sem->owner = p->task;
        } else {
            do {
                if (unlikely(mv_do_wait_on_queue(&(sem->waitqueue), p)))
                {
                    MV_GLOBAL_UNLOCK();
                    return -MVKERNELEXT_EINTR;
                }
                CHECK_SEM();
            } while (sem->count == 0);
        }
        sem->count--;

#ifdef MV_SEM_STAT
        sem->tcount++;
#endif
    }

    MV_GLOBAL_UNLOCK();
    return 0;
}

int mvKernelExt_SemWaitTimeout(
        int semid,
        unsigned long timeout
)
{
    mvSemaphoreSTC *sem;

    SEMAPHORE_BY_ID(semid);

    if (mvKernelExt_SemTryWait_common(sem) != 0)
    {
        TASK_WILL_WAIT(current);
#ifdef  MV_SEM_STAT
        sem->wcount++;
#endif
#if HZ != 1000
        timeout += 1000 / HZ - 1;
        timeout /= 1000 / HZ;
#endif
        do {
            timeout = mv_do_wait_on_queue_timeout(&(sem->waitqueue), p, timeout);
            CHECK_SEM();
            if (!timeout)
            {
                MV_GLOBAL_UNLOCK();
                return -MVKERNELEXT_ETIMEOUT;
            }
            if (timeout == (unsigned long)(-1))
            {
                MV_GLOBAL_UNLOCK();
                return -MVKERNELEXT_EINTR;
            }
        } while (sem->count <= 0);

        if (sem->flags & MV_SEMAPTHORE_F_MTX)
            sem->owner = p->task;

        sem->count--;
#ifdef MV_SEM_STAT
        sem->tcount++;
#endif
    }

    MV_GLOBAL_UNLOCK();
    return 0;
}

EXPORT_SYMBOL(mvKernelExt_SemCreate);
EXPORT_SYMBOL(mvKernelExt_SemDelete);
EXPORT_SYMBOL(mvKernelExt_SemSignal);
EXPORT_SYMBOL(mvKernelExt_SemTryWait);
EXPORT_SYMBOL(mvKernelExt_SemWait);
EXPORT_SYMBOL(mvKernelExt_SemWaitTimeout);
