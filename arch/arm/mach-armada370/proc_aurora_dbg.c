#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/sysdev.h>
#include <linux/proc_fs.h>
#include <linux/version.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/setup.h>
#include <asm/mach-types.h>

static struct proc_dir_entry *aurora_dbg;

unsigned int aurora_core_index(void)
{
        unsigned int value;

        __asm__ __volatile__("mrc p15, 0, %0, c0, c0, 5   @ read CPU ID reg\n"
                : "=r" (value) :: "memory");
        return (value & 0xF);
}

/********************************************************************/
int aurora_dbg_read (char *buffer, char **buffer_location, off_t offset,
                            int buffer_length, int *zero, void *ptr) {

	char *p = buffer;
	unsigned int val, len;

#ifdef CONFIG_SMP
	p += sprintf(p,"CPU %d:\n", aurora_core_index());
#endif
	__asm volatile ("mrc  p15, 1, %0, c15, c1, 1" : "=r" (val));
	p += sprintf(p, "c1, 1 %x \n", val);
	__asm volatile ("mrc  p15, 1, %0, c15, c1, 2" : "=r" (val));
	p += sprintf(p, "c1, 2 %x \n", val);
	__asm volatile ("mrc  p15, 1, %0, c15, c2, 0" : "=r" (val));
	p += sprintf(p, "c2, 0 %x \n", val);
	__asm volatile ("mrc  p15, 1, %0, c15, c2, 1" : "=r" (val));
	p += sprintf(p, "c2, 1 %x \n", val);
	__asm volatile ("mrc  p15, 1, %0, c15, c1, 0" : "=r" (val));
	p += sprintf(p, "c1, 0 %x \n", val);

#ifdef CONFIG_PERF_EVENTS
	__asm volatile ("mrc  p15, 0, %0, c9, c12, 0" : "=r" (val));
	p += sprintf(p, "pmon ctrl %x \n", val);
	__asm volatile ("mrc  p15, 0, %0, c9, c12, 1" : "=r" (val));
	p += sprintf(p, "pmon cntrs en %x \n", val);
	__asm volatile ("mrc  p15, 0, %0, c9, c12, 3" : "=r" (val));
	p += sprintf(p, "pmon cntrs oflow %x \n", val);
	__asm volatile ("mrc  p15, 0, %0, c9, c12, 5" : "=r" (val));
	p += sprintf(p, "pmon cntr sel %x \n", val);
	__asm volatile ("mrc  p15, 0, %0, c9, c13, 0" : "=r" (val));
	p += sprintf(p, "pmon cycle cnt %x \n", val);
	__asm volatile ("mrc  p15, 0, %0, c9, c13, 1" : "=r" (val));
	p += sprintf(p, "pmon evt sel %x \n", val);
	__asm volatile ("mrc  p15, 0, %0, c9, c13, 2" : "=r" (val));
	p += sprintf(p, "pmon cntr val %x \n", val);
	__asm volatile ("mrc  p15, 0, %0, c9, c14, 1" : "=r" (val));
	p += sprintf(p, "pmon int en %x \n", val);
#endif

	len = (p - buffer);
	return len;
}

/********************************************************************/
int __init start_aurora_dbg(void)
{
        aurora_dbg = create_proc_entry ("aurora_dbg" , 0666 , NULL);
	aurora_dbg->read_proc = aurora_dbg_read;
	aurora_dbg->write_proc = NULL;
	aurora_dbg->nlink = 1;
	return 0;
}
void __exit stop_aurora_dbg(void)
{
        remove_proc_entry("aurora_dbg",  NULL);
        return;
}
module_init(start_aurora_dbg);
module_exit(stop_aurora_dbg);
