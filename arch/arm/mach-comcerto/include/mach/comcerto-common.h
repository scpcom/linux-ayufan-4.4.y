#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __ASM_ARCH_HARDWARE_H
#error "Do not include this directly, instead #include <asm/hardware.h>"
#endif

#ifndef __ASM_COMCERTO_COMMON_H__
#define __ASM_COMCERTO_COMMON_H__

#include <asm/types.h>

#define APB_VADDR(x)	((x) - COMCERTO_AXI_APB_BASE + COMCERTO_APB_VADDR)		 
#define AXI_VADDR(x)	((x) - COMCERTO_AXI_SPI_BASE + COMCERTO_AXI_SSI_VADDR)	 

#define  BIT_0_MSK   0x00000001
#define  BIT_1_MSK   0x00000002
#define  BIT_2_MSK   0x00000004
#define  BIT_3_MSK   0x00000008
#define  BIT_4_MSK   0x00000010
#define  BIT_5_MSK   0x00000020
#define  BIT_6_MSK   0x00000040
#define  BIT_7_MSK   0x00000080
#define  BIT_8_MSK   0x00000100
#define  BIT_9_MSK   0x00000200
#define  BIT_10_MSK  0x00000400
#define  BIT_11_MSK  0x00000800
#define  BIT_12_MSK  0x00001000
#define  BIT_13_MSK  0x00002000
#define  BIT_14_MSK  0x00004000
#define  BIT_15_MSK  0x00008000
#define  BIT_16_MSK  0x00010000
#define  BIT_17_MSK  0x00020000
#define  BIT_18_MSK  0x00040000
#define  BIT_19_MSK  0x00080000
#define  BIT_20_MSK  0x00100000
#define  BIT_21_MSK  0x00200000
#define  BIT_22_MSK  0x00400000
#define  BIT_23_MSK  0x00800000
#define  BIT_24_MSK  0x01000000
#define  BIT_25_MSK  0x02000000
#define  BIT_26_MSK  0x04000000
#define  BIT_27_MSK  0x08000000
#define  BIT_28_MSK  0x10000000
#define  BIT_29_MSK  0x20000000
#define  BIT_30_MSK  0x40000000
#define  BIT_31_MSK  0x80000000

#define comcerto_gpio_enable_output(gpiomask)	__raw_writel(__raw_readl(COMCERTO_GPIO_OE_REG) | (gpiomask), COMCERTO_GPIO_OE_REG)

#define comcerto_gpio_set_0(gpiomask)	__raw_writel(__raw_readl(COMCERTO_GPIO_OUTPUT_REG) & ~(gpiomask), COMCERTO_GPIO_OUTPUT_REG)

#define comcerto_gpio_set_1(gpiomask)	__raw_writel(__raw_readl(COMCERTO_GPIO_OUTPUT_REG) | (gpiomask), COMCERTO_GPIO_OUTPUT_REG)

#define comcerto_gpio_read(gpiomask)	(__raw_readl(COMCERTO_GPIO_INPUT_REG) & (gpiomask))

#ifndef __ASSEMBLY__
#define CONFIG_COMCERTO_GEMAC		1

#define CONFIG_COMCERTO_USE_MII		1
#define CONFIG_COMCERTO_USE_RMII		2
#define CONFIG_COMCERTO_USE_GMII		4
#define CONFIG_COMCERTO_USE_RGMII	8
#define CONFIG_COMCERTO_USE_SGMII	0x10

#define GEMAC_SW_CONF			(1 << 8) | (1 << 11)	 
#define GEMAC_PHY_CONF		0			 
#define GEMAC_SW_FULL_DUPLEX	(1 << 9)
#define GEMAC_SW_SPEED_10M	(0 << 12)
#define GEMAC_SW_SPEED_100M	(1 << 12)
#define GEMAC_SW_SPEED_1G		(2 << 12)

#define GEMAC_NO_PHY			(1 << 0)		 
#define GEMAC_PHY_RGMII_ADD_DELAY	(1 << 1)


#define GEM0_ITF_NAME "eth0"
#if defined(CONFIG_SYNO_C2K_NET)
#define GEM1_ITF_NAME "eth1"
#else
#define GEM1_ITF_NAME "eth2"
#endif
#define GEM2_ITF_NAME "eth3"

#define GEM0_MAC { 0x00, 0xED, 0xCD, 0xEF, 0xAA, 0xCC }
#define GEM1_MAC { 0x00, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E }
#define GEM2_MAC { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 }

struct comcerto_eth_platform_data {
	 
	u32 device_flags;
	char name[16];

	u32 mii_config;
	u32 gemac_mode;
	u32 phy_flags;
	u32 gem_id;
	u32 bus_id;
	u32 phy_id;
	u8 *mac_addr;
};

struct comcerto_mdio_platform_data {
	int enabled;
	int irq[32];
	u32 phy_mask;
	int mdc_div;
};

struct comcerto_pfe_platform_data
{
	struct comcerto_eth_platform_data comcerto_eth_pdata[3];
	struct comcerto_mdio_platform_data comcerto_mdio_pdata[3];
};

struct comcerto_tdm_data {
	u8 fsoutput;  
	u8 fspolarity;  
	u16 fshwidth;  
	u16 fslwidth;  
	u32 clockhz;  
	u8 clockout;  
	u8 tdmmux;
#if 0
	u32 tdmck;
	u32 tdmfs;
	u32 tdmdx;
	u32 tdmdr;
#endif
};

#define L210_AUX_CTRL_REG	0x01330241

#endif
#endif
