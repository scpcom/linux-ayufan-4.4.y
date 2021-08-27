#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 


#ifndef _PXA_SDHCI_H_
#define _PXA_SDHCI_H_

#if defined(CONFIG_SYNO_ARMADA) || || defined(CONFIG_SYNO_ARMADA_V2)
#include <linux/mbus.h>
#endif

#define PXA_FLAG_ENABLE_CLOCK_GATING (1<<0)
 
#define PXA_FLAG_CARD_PERMANENT	(1<<1)
 
#define PXA_FLAG_SD_8_BIT_CAPABLE_SLOT (1<<2)

struct sdhci_pxa_platdata {
	unsigned int	flags;
	unsigned int	clk_delay_cycles;
	unsigned int	clk_delay_sel;
	bool		clk_delay_enable;
	unsigned int	ext_cd_gpio;
	bool		ext_cd_gpio_invert;
	unsigned int	max_speed;
	unsigned int	host_caps;
	unsigned int	quirks;
	unsigned int	pm_caps;
#if defined(CONFIG_SYNO_ARMADA) || defined(CONFIG_SYNO_ARMADA_V2)
	struct		mbus_dram_target_info *dram;
#endif
};

struct sdhci_pxa {
	u8	clk_enable;
	u8	power_mode;
};
#endif  
