#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifndef __CRASHLOG_H
#define __CRASHLOG_H

#if defined(MY_ABC_HERE)
#ifdef CONFIG_CRASHLOG
void __init crashlog_init_mem(struct bootmem_data *bdata);
#else
static inline void crashlog_init_mem(struct bootmem_data *bdata)
{
}
#endif
#endif

#endif
