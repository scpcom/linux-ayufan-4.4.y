/**
 * Copyright (C) Arm Limited 2010-2016. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/mount.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#   include <linux/sched/mm.h>
#endif


#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
#   define get_user_pages_remote(tsk,mm,start,nr_pages,write,force,pages,vmas)  get_user_pages_remote(mm,start,nr_pages,((write) ? FOLL_WRITE : 0) | ((force) ? FOLL_FORCE : 0),pages,vmas,NULL)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0)
/* Kernel version 4.10.0 adds locked argument
   (See https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/commit/include/linux/mm.h?id=5b56d49fc31dbb0487e14ead790fc81ca9fb2c99) */
#   define get_user_pages_remote(tsk,mm,start,nr_pages,write,force,pages,vmas)  get_user_pages_remote(tsk,mm,start,nr_pages,((write) ? FOLL_WRITE : 0) | ((force) ? FOLL_FORCE : 0),pages,vmas,NULL)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)
/* Kernel version 4.9.0 removes write and force arguments from get_user_pages_remote and replaces with gup_flags instead
   (See https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/commit/include/linux/mm.h?id=9beae1ea89305a9667ceaab6d0bf46a045ad71e7) */
#   define get_user_pages_remote(tsk,mm,start,nr_pages,write,force,pages,vmas)  get_user_pages_remote(tsk,mm,start,nr_pages,((write) ? FOLL_WRITE : 0) | ((force) ? FOLL_FORCE : 0),pages,vmas)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 168) && LINUX_VERSION_CODE < KERNEL_VERSION(4, 5, 0)
#   define get_user_pages_remote(tsk,mm,start,nr_pages,write,force,pages,vmas)  get_user_pages(tsk,mm,start,nr_pages,((write) ? FOLL_WRITE : 0) | ((force) ? FOLL_FORCE : 0),pages,vmas)
#elif LINUX_VERSION_CODE < KERNEL_VERSION(4, 6, 0)
/* Kernel version 4.6.0 removes get_user_pages macro. We should use get_user_pages_remote anyway.
   (See https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/commit/include/linux/mm.h?id=c12d2da56d0e07d230968ee2305aaa86b93a6832) */
#   define get_user_pages_remote(tsk,mm,start,nr_pages,write,force,pages,vmas)  get_user_pages(tsk,mm,start,nr_pages,write,force,pages,vmas)
#endif

struct mount {
    struct mount *mnt_parent;
    struct dentry *mnt_mountpoint;
    struct vfsmount mnt;
};

static inline struct mount *real_mount(struct vfsmount *mnt)
{
    return container_of(mnt, struct mount, mnt);
}

#define GET_MNT_ROOT(mount) ((mount)->mnt.mnt_root)

/* must be power of 2 */
#define COOKIEMAP_ENTRIES       1024
/* must be a power of 2 - 512/4 = 128 entries */
#define TRANSLATE_BUFFER_SIZE   512
#define TRANSLATE_TEXT_SIZE     256
#define MAX_COLLISIONS          2

static uint32_t *gator_crc32_table;
static unsigned int translate_buffer_mask;

struct cookie_args {
    struct task_struct *task;
    const char *text;
};

static DEFINE_PER_CPU(char *, translate_text);
static DEFINE_PER_CPU(char *, scratch);
static DEFINE_PER_CPU(uint32_t, cookie_next_key);
static DEFINE_PER_CPU(uint64_t *, cookie_keys);
static DEFINE_PER_CPU(uint32_t *, cookie_values);
static DEFINE_PER_CPU(int, translate_buffer_read);
static DEFINE_PER_CPU(int, translate_buffer_write);
static DEFINE_PER_CPU(struct cookie_args *, translate_buffer);

static uint32_t get_cookie(int cpu, struct task_struct *task, const char *text, bool from_wq);
static void wq_cookie_handler(struct work_struct *unused);
static DECLARE_WORK(cookie_work, wq_cookie_handler);
static struct timer_list app_process_wake_up_timer;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,15,0))
static void app_process_wake_up_handler(struct timer_list *t);
#else
static void app_process_wake_up_handler(unsigned long unused_data);
#endif

static uint32_t cookiemap_code(uint64_t value64)
{
    uint32_t value = (uint32_t)((value64 >> 32) + value64);
    uint32_t cookiecode = (value >> 24) & 0xff;

    cookiecode = cookiecode * 31 + ((value >> 16) & 0xff);
    cookiecode = cookiecode * 31 + ((value >> 8) & 0xff);
    cookiecode = cookiecode * 31 + ((value >> 0) & 0xff);
    cookiecode &= (COOKIEMAP_ENTRIES - 1);
    return cookiecode * MAX_COLLISIONS;
}

static uint32_t gator_chksum_crc32(const char *data)
{
    register unsigned long crc;
    const unsigned char *block = data;
    int i, length = strlen(data);

    crc = 0xFFFFFFFF;
    for (i = 0; i < length; i++)
        crc = ((crc >> 8) & 0x00FFFFFF) ^ gator_crc32_table[(crc ^ *block++) & 0xFF];

    return (crc ^ 0xFFFFFFFF);
}

/*
 * Exists
 *  Pre:  [0][1][v][3]..[n-1]
 *  Post: [v][0][1][3]..[n-1]
 */
static uint32_t cookiemap_exists(uint64_t key)
{
    unsigned long x, flags, retval = 0;
    int cpu = get_physical_cpu();
    uint32_t cookiecode = cookiemap_code(key);
    uint64_t *keys = &(per_cpu(cookie_keys, cpu)[cookiecode]);
    uint32_t *values = &(per_cpu(cookie_values, cpu)[cookiecode]);

    /* Can be called from interrupt handler or from work queue */
    local_irq_save(flags);
    for (x = 0; x < MAX_COLLISIONS; x++) {
        if (keys[x] == key) {
            uint32_t value = values[x];

            for (; x > 0; x--) {
                keys[x] = keys[x - 1];
                values[x] = values[x - 1];
            }
            keys[0] = key;
            values[0] = value;
            retval = value;
            break;
        }
    }
    local_irq_restore(flags);

    return retval;
}

/*
 * Add
 *  Pre:  [0][1][2][3]..[n-1]
 *  Post: [v][0][1][2]..[n-2]
 */
static void cookiemap_add(uint64_t key, uint32_t value)
{
    int cpu = get_physical_cpu();
    int cookiecode = cookiemap_code(key);
    uint64_t *keys = &(per_cpu(cookie_keys, cpu)[cookiecode]);
    uint32_t *values = &(per_cpu(cookie_values, cpu)[cookiecode]);
    int x;

    for (x = MAX_COLLISIONS - 1; x > 0; x--) {
        keys[x] = keys[x - 1];
        values[x] = values[x - 1];
    }
    keys[0] = key;
    values[0] = value;
}

#ifndef CONFIG_PREEMPT_RT_FULL
static void translate_buffer_write_args(int cpu, struct task_struct *task, const char *text)
{
    unsigned long flags;
    int write;
    int next_write;
    struct cookie_args *args;

    local_irq_save(flags);

    write = per_cpu(translate_buffer_write, cpu);
    next_write = (write + 1) & translate_buffer_mask;

    /* At least one entry must always remain available as when read == write, the queue is empty not full */
    if (next_write != per_cpu(translate_buffer_read, cpu)) {
        args = &per_cpu(translate_buffer, cpu)[write];
        args->task = task;
        args->text = text;
        get_task_struct(task);
        per_cpu(translate_buffer_write, cpu) = next_write;
    }

    local_irq_restore(flags);
}
#endif

static void translate_buffer_read_args(int cpu, struct cookie_args *args)
{
    unsigned long flags;
    int read;

    local_irq_save(flags);

    read = per_cpu(translate_buffer_read, cpu);
    *args = per_cpu(translate_buffer, cpu)[read];
    per_cpu(translate_buffer_read, cpu) = (read + 1) & translate_buffer_mask;

    local_irq_restore(flags);
}

static void wq_cookie_handler(struct work_struct *unused)
{
    struct cookie_args args;
    int cpu = get_physical_cpu(), cookie;

    mutex_lock(&start_mutex);

    if (gator_started != 0) {
        while (per_cpu(translate_buffer_read, cpu) != per_cpu(translate_buffer_write, cpu)) {
            translate_buffer_read_args(cpu, &args);
            cookie = get_cookie(cpu, args.task, args.text, true);
            marshal_link(cookie, args.task->tgid, args.task->pid);
            put_task_struct(args.task);
        }
    }

    mutex_unlock(&start_mutex);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,15,0))
static void app_process_wake_up_handler(struct timer_list *t)
#else
static void app_process_wake_up_handler(unsigned long unused_data)
#endif
{
    /* had to delay scheduling work as attempting to schedule work during the context switch is illegal in kernel versions 3.5 and greater */
    schedule_work(&cookie_work);
}

/* Retrieve full name from proc/pid/cmdline for java processes on Android */
static int translate_app_process(const char **text, int cpu, struct task_struct *task, bool from_wq)
{
    void *maddr;
    unsigned int len;
    unsigned long addr;
    struct mm_struct *mm;
    struct page *page = NULL;
    struct vm_area_struct *page_vma;
    int bytes, offset, retval = 0;
    char *buf = per_cpu(translate_text, cpu);

#ifndef CONFIG_PREEMPT_RT_FULL
    /* Push work into a work queue if in atomic context as the kernel
     * functions below might sleep. Rely on the in_interrupt variable
     * rather than in_irq() or in_interrupt() kernel functions, as the
     * value of these functions seems inconsistent during a context
     * switch between android/linux versions
     */
    if (!from_wq) {
        /* Check if already in buffer */
        int pos = per_cpu(translate_buffer_read, cpu);

        while (pos != per_cpu(translate_buffer_write, cpu)) {
            if (per_cpu(translate_buffer, cpu)[pos].task == task)
                goto out;
            pos = (pos + 1) & translate_buffer_mask;
        }

        translate_buffer_write_args(cpu, task, *text);

        /* Not safe to call in RT-Preempt full in schedule switch context */
        mod_timer(&app_process_wake_up_timer, jiffies + 1);
        goto out;
    }
#endif

    mm = get_task_mm(task);
    if (!mm)
        goto out;
    if (!mm->arg_end)
        goto outmm;
    addr = mm->arg_start;
    len = mm->arg_end - mm->arg_start;

    if (len > TRANSLATE_TEXT_SIZE)
        len = TRANSLATE_TEXT_SIZE;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
    down_read(&mm->mmap_lock);
#else
    down_read(&mm->mmap_sem);
#endif
    while (len) {
        if (get_user_pages_remote(task, mm, addr, 1, 0, 1, &page, &page_vma) <= 0)
            goto outsem;

        maddr = kmap(page);
        offset = addr & (PAGE_SIZE - 1);
        bytes = len;
        if (bytes > PAGE_SIZE - offset)
            bytes = PAGE_SIZE - offset;

        copy_from_user_page(page_vma, page, addr, buf, maddr + offset, bytes);

        /* release page allocated by get_user_pages_remote() */
        kunmap(page);
        page_cache_release(page);

        len -= bytes;
        buf += bytes;
        addr += bytes;

        *text = per_cpu(translate_text, cpu);
        retval = 1;
    }

    /* On app_process startup, /proc/pid/cmdline is initially "zygote" then "<pre-initialized>" but changes after an initial startup period */
    if (strcmp(*text, "zygote") == 0 || strcmp(*text, "<pre-initialized>") == 0)
        retval = 0;

outsem:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
    up_read(&mm->mmap_lock);
#else
    up_read(&mm->mmap_sem);
#endif
outmm:
    mmput(mm);
out:
    return retval;
}

static const char APP_PROCESS[] = "app_process";

static uint32_t get_cookie(int cpu, struct task_struct *task, const char *text, bool from_wq)
{
    unsigned long flags, cookie;
    uint64_t key;

    key = gator_chksum_crc32(text);
    key = (key << 32) | (uint32_t)task->tgid;

    cookie = cookiemap_exists(key);
    if (cookie)
        return cookie;

    /* On 64-bit android app_process can be app_process32 or app_process64 */
    if (strstr(text, APP_PROCESS) != NULL) {
        if (!translate_app_process(&text, cpu, task, from_wq))
            return UNRESOLVED_COOKIE;
    }

    /* Can be called from interrupt handler or from work queue or from scheduler trace */
    local_irq_save(flags);

    cookie = UNRESOLVED_COOKIE;
    if (marshal_cookie_header(text)) {
        cookie = per_cpu(cookie_next_key, cpu) += nr_cpu_ids;
        cookiemap_add(key, cookie);
        marshal_cookie(cookie, text);
    }

    local_irq_restore(flags);

    return cookie;
}

/* Can't call d_path in interrupt context so create something similar */
static const char *gator_d_path(const struct path *path, char *buf, int buflen)
{
    struct dentry *dentry = path->dentry;
    struct mount *mount = real_mount(path->mnt);
    int pos = buflen - 1;
    int len;

    buf[pos] = '\0';

    for (;;) {
        if (dentry == NULL) {
            pr_err("gator: dentry is null!\n");
            break;
        }
        if (dentry->d_name.name[0] == '\0') {
            pr_err("gator: path is empty string\n");
            break;
        }
        if (dentry->d_name.name[0] == '/' && dentry->d_name.name[1] == '\0') {
            /* Normal operation */
            /* pr_err("gator: path is /\n"); */
            break;
        }

        len = strlen(dentry->d_name.name);
        if (pos < len) {
            pr_err("gator: path is too long\n");
            break;
        }
        pos -= len;
        memcpy(buf + pos, dentry->d_name.name, len);

        if (pos == 0) {
            pr_err("gator: no room for slash\n");
            /* Fall back to name only */
            return path->dentry->d_name.name;
        }
        --pos;
        buf[pos] = '/';

        if (dentry->d_parent == GET_MNT_ROOT(mount)) {
            /* pr_err("gator: filesystem is complete, moving to next '%s'\n", buf + pos); */
            dentry = mount->mnt_mountpoint;
            mount = mount->mnt_parent;
            continue;
        }
        if (dentry == dentry->d_parent) {
            /* Normal operation, at least for ashmem */
            /* pr_err("gator: parent is self\n"); */
            break;
        }
        dentry = dentry->d_parent;
    }

    if (pos < 0) {
        pr_err("gator: pos is somenow negative\n");
        /* Fall back to name only */
        return path->dentry->d_name.name;
    }

    return buf + pos;
}

#define d_path gator_d_path

static int get_exec_cookie(int cpu, struct task_struct *task)
{
    struct mm_struct *mm = task->mm;
    const char *text;

    /* kernel threads have no address space */
    if (!mm)
        return NO_COOKIE;

    if (task && task->mm && task->mm->exe_file) {
        text = d_path(&task->mm->exe_file->f_path, per_cpu(scratch, get_physical_cpu()), PAGE_SIZE);
        return get_cookie(cpu, task, text, false);
    }

    return UNRESOLVED_COOKIE;
}

static unsigned long get_address_cookie(int cpu, struct task_struct *task, unsigned long addr, off_t *offset)
{
    unsigned long cookie = NO_COOKIE;
    struct mm_struct *mm = task->mm;
    struct vm_area_struct *vma;
    const char *text;

    if (!mm)
        return cookie;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
    vma = find_vma(mm, addr);
    if (!vma)
        return UNRESOLVED_COOKIE;
    {
      struct maple_tree *mt = &vma->vm_mm->mm_mt;
      MA_STATE(mas, mt, vma->vm_end, vma->vm_end);
      for ( ; vma; vma = mas_find(&mas, ULONG_MAX)) {
#else
    for (vma = find_vma(mm, addr); vma; vma = vma->vm_next) {
#endif
        if (addr < vma->vm_start || addr >= vma->vm_end)
            continue;

        if (vma->vm_file) {
            text = d_path(&vma->vm_file->f_path, per_cpu(scratch, get_physical_cpu()), PAGE_SIZE);
            cookie = get_cookie(cpu, task, text, false);
            *offset = (vma->vm_pgoff << PAGE_SHIFT) + addr - vma->vm_start;
        } else {
            /* must be an anonymous map */
            *offset = addr;
        }

        break;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
      }
#endif
    }

    if (!vma)
        cookie = UNRESOLVED_COOKIE;

    return cookie;
}

static int cookies_initialize(void)
{
    uint32_t crc, poly;
    int i, j, cpu, size, err = 0;

    translate_buffer_mask = TRANSLATE_BUFFER_SIZE / sizeof(per_cpu(translate_buffer, 0)[0]) - 1;

    for_each_present_cpu(cpu) {
        per_cpu(cookie_next_key, cpu) = nr_cpu_ids + cpu;

        size = COOKIEMAP_ENTRIES * MAX_COLLISIONS * sizeof(uint64_t);
        per_cpu(cookie_keys, cpu) = kmalloc(size, GFP_KERNEL);
        if (!per_cpu(cookie_keys, cpu)) {
            err = -ENOMEM;
            goto cookie_setup_error;
        }
        memset(per_cpu(cookie_keys, cpu), 0, size);

        size = COOKIEMAP_ENTRIES * MAX_COLLISIONS * sizeof(uint32_t);
        per_cpu(cookie_values, cpu) = kmalloc(size, GFP_KERNEL);
        if (!per_cpu(cookie_values, cpu)) {
            err = -ENOMEM;
            goto cookie_setup_error;
        }
        memset(per_cpu(cookie_values, cpu), 0, size);

        per_cpu(translate_buffer, cpu) = kmalloc(TRANSLATE_BUFFER_SIZE, GFP_KERNEL);
        if (!per_cpu(translate_buffer, cpu)) {
            err = -ENOMEM;
            goto cookie_setup_error;
        }

        per_cpu(translate_buffer_write, cpu) = 0;
        per_cpu(translate_buffer_read, cpu) = 0;

        per_cpu(translate_text, cpu) = kmalloc(TRANSLATE_TEXT_SIZE, GFP_KERNEL);
        if (!per_cpu(translate_text, cpu)) {
            err = -ENOMEM;
            goto cookie_setup_error;
        }

        per_cpu(scratch, cpu) = kmalloc(PAGE_SIZE, GFP_KERNEL);
        if (!per_cpu(scratch, cpu)) {
            err = -ENOMEM;
            goto cookie_setup_error;
        }
    }

    /* build CRC32 table */
    poly = 0x04c11db7;
    gator_crc32_table = kmalloc(256 * sizeof(*gator_crc32_table), GFP_KERNEL);
    if (!gator_crc32_table) {
        err = -ENOMEM;
        goto cookie_setup_error;
    }
    for (i = 0; i < 256; i++) {
        crc = i;
        for (j = 8; j > 0; j--) {
            if (crc & 1)
                crc = (crc >> 1) ^ poly;
            else
                crc >>= 1;
        }
        gator_crc32_table[i] = crc;
    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,15,0))
    timer_setup(&app_process_wake_up_timer, app_process_wake_up_handler, TIMER_DEFERRABLE);
#else
    setup_deferrable_timer_on_stack(&app_process_wake_up_timer, app_process_wake_up_handler, 0);
#endif

cookie_setup_error:
    return err;
}

static void cookies_release(void)
{
    int cpu;

    for_each_present_cpu(cpu) {
        kfree(per_cpu(cookie_keys, cpu));
        per_cpu(cookie_keys, cpu) = NULL;

        kfree(per_cpu(cookie_values, cpu));
        per_cpu(cookie_values, cpu) = NULL;

        kfree(per_cpu(translate_buffer, cpu));
        per_cpu(translate_buffer, cpu) = NULL;
        per_cpu(translate_buffer_read, cpu) = 0;
        per_cpu(translate_buffer_write, cpu) = 0;

        kfree(per_cpu(translate_text, cpu));
        per_cpu(translate_text, cpu) = NULL;

        kfree(per_cpu(scratch, cpu));
        per_cpu(scratch, cpu) = NULL;
    }

    del_timer_sync(&app_process_wake_up_timer);
    kfree(gator_crc32_table);
    gator_crc32_table = NULL;
}
