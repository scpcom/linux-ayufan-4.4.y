#ifndef __PPC_FSL_SOC_H
#define __PPC_FSL_SOC_H
#ifdef __KERNEL__

#include <asm/mmu.h>
#ifdef CONFIG_SYNO_QORIQ
#include <asm/of_device.h>
#include <linux/suspend.h>
#endif

struct spi_device;

extern phys_addr_t get_immrbase(void);
#if defined(CONFIG_CPM2) || defined(CONFIG_QUICC_ENGINE) || defined(CONFIG_8xx)
extern u32 get_brgfreq(void);
extern u32 get_baudrate(void);
#else
static inline u32 get_brgfreq(void) { return -1; }
static inline u32 get_baudrate(void) { return -1; }
#endif
extern u32 fsl_get_sys_freq(void);

struct spi_board_info;
struct device_node;

extern void fsl_rstcr_restart(char *cmd);

#ifdef CONFIG_SYNO_QORIQ
#ifdef CONFIG_FSL_PMC
typedef int (*wakeup_event_t)(struct of_device *);
int pmc_enable_wake(struct of_device *ofdev, wakeup_event_t func,
		bool enable);
void pmc_enable_lossless(int enable);
#else
#define pmc_enable_wake(ofdev, func, enable)	(-EINVAL)
#define pmc_enable_lossless(enable) do {} while (0);
#endif
#endif

#if defined(CONFIG_FB_FSL_DIU) || defined(CONFIG_FB_FSL_DIU_MODULE)
struct platform_diu_data_ops {
	unsigned int (*get_pixel_format) (unsigned int bits_per_pixel,
		int monitor_port);
	void (*set_gamma_table) (int monitor_port, char *gamma_table_base);
	void (*set_monitor_port) (int monitor_port);
	void (*set_pixel_clock) (unsigned int pixclock);
	ssize_t (*show_monitor_port) (int monitor_port, char *buf);
	int (*set_sysfs_monitor_port) (int val);
};

extern struct platform_diu_data_ops diu_ops;
#endif

#endif
#endif
