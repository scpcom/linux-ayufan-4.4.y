#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/export.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/sem.h>
#include <linux/msg.h>
#include <linux/shm.h>
#include <linux/stat.h>
#include <linux/syscalls.h>
#include <linux/mman.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/ipc.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#if defined(MY_DEF_HERE)
#include <linux/printk.h>
#endif  

asmlinkage long sys_arm_fadvise64_64(int fd, int advice,
				     loff_t offset, loff_t len)
{
	return sys_fadvise64_64(fd, offset, len, advice);
}

#if defined(MY_DEF_HERE) && (PAGE_SHIFT > 12)
	 
asmlinkage unsigned long sys_arm_mmap_4koff(unsigned long addr,
		unsigned long len, unsigned long prot, unsigned long flags,
		unsigned long fd, unsigned long offset)
{
	unsigned long pgoff;
	if (offset & ((PAGE_SIZE-1)>>12)) {
		printk(KERN_WARNING
				"mmap received unaligned request offset: %x.",
				(unsigned int)offset);
		return -EINVAL;
	}
	pgoff = offset >> (PAGE_SHIFT - 12);

	return sys_mmap_pgoff(addr, len, prot, flags, fd, pgoff);
}
#endif  
