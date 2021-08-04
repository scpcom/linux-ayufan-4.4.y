#ifndef __ASMARM_ARCH_SCU_H
#define __ASMARM_ARCH_SCU_H

#define SCU_PM_NORMAL	0
#define SCU_PM_DORMANT	2
#define SCU_PM_POWEROFF	3

#ifndef __ASSEMBLER__
struct scu_context {
	u32 ctrl;
	u32 cpu_status;
#ifdef CONFIG_ARM_ERRATA_764369
	u32 diag_ctrl;
#endif
	u32 filter_start;
	u32 filter_end;
	u32 sac;
	u32 snsac;
};

unsigned int scu_get_core_count(void __iomem *);
void scu_enable(void __iomem *);
int __scu_power_mode(void __iomem *, int, unsigned int);
int scu_power_mode(void __iomem *, unsigned int);
void scu_save(void __iomem *, struct scu_context *);
void scu_restore(void __iomem *, struct scu_context *);
#endif

#endif
