#ifndef __ASM_ARM_SYSTEM_INFO_H
#define __ASM_ARM_SYSTEM_INFO_H

#ifndef __ASSEMBLY__

/* information about the system we're running on */
extern const char *machine_name;
extern unsigned int system_rev;
extern const char *system_serial;
extern unsigned int system_serial_low;
extern unsigned int system_serial_high;

#endif /* !__ASSEMBLY__ */

#endif /* __ASM_ARM_SYSTEM_INFO_H */
