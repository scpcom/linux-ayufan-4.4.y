 
#ifndef __ALPINE_MACHINE_H__
#define __ALPINE_MACHINE_H__

void __iomem *alpine_serdes_resource_get(u32 group);

enum alpine_serdes_eth_mode {
	ALPINE_SERDES_ETH_MODE_SGMII,
	ALPINE_SERDES_ETH_MODE_KR,
};

int alpine_serdes_eth_mode_set(
	u32				group,
	enum alpine_serdes_eth_mode	mode);

#ifdef CONFIG_SYNO_ALPINE_A0
 
void alpine_serdes_eth_group_lock(u32 group);

void alpine_serdes_eth_group_unlock(u32 group);
#endif

void __init alpine_cpu_pm_init(void);

int alpine_cpu_suspend_wakeup_supported(void);

void alpine_cpu_wakeup(unsigned int cpu, uintptr_t resume_addr);

void alpine_cpu_die(unsigned int cpu);

void alpine_cpu_suspend(void);

#endif  
