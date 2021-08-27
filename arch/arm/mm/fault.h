#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
void do_bad_area(unsigned long addr, unsigned int fsr, struct pt_regs *regs);

unsigned long search_exception_table(unsigned long addr);
