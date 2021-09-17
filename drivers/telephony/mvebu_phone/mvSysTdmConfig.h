#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __INCmvSysTdmConfigh
#define __INCmvSysTdmConfigh

#include "mvOs.h"
#include "voiceband/mvSysTdmSpi.h"
#include "voiceband/common/mvTdmComm.h"

extern int tdm_base, use_pclk_external;
#define MV_TDM_REGS_BASE	(tdm_base)

#define DCO_CLK_DIV_MOD_OFFS			24
#define DCO_CLK_DIV_APPLY			(0x1 << DCO_CLK_DIV_MOD_OFFS)
#define DCO_CLK_DIV_RESET_OFFS			25
#define DCO_CLK_DIV_RESET			(0x1 << DCO_CLK_DIV_RESET_OFFS)

#define DCO_CLK_DIV_RATIO_OFFS			26
#define DCO_CLK_DIV_RATIO_MASK			0xfc000000
#define DCO_CLK_DIV_RATIO_8M			(0x3 << DCO_CLK_DIV_RATIO_OFFS)
#define DCO_CLK_DIV_RATIO_4M			(0x6 << DCO_CLK_DIV_RATIO_OFFS)
#define DCO_CLK_DIV_RATIO_2M			(0xc << DCO_CLK_DIV_RATIO_OFFS)

#define TDM_PLL_CONF_REG0			0x0
#define TDM_PLL_FB_CLK_DIV_OFFSET		10
#define TDM_PLL_FB_CLK_DIV_MASK			0x7fc00

#define TDM_PLL_CONF_REG1			0x4
#define TDM_PLL_FREQ_OFFSET_MASK		0xffff
#define TDM_PLL_FREQ_OFFSET_VALID		BIT16
#define TDM_PLL_SW_RESET			BIT31

#define TDM_PLL_CONF_REG2			0x8
#define TDM_PLL_POSTDIV_MASK			0x7f

#define TRC_INIT(...)
#define TRC_RELEASE(...)
#define TRC_START(...)
#define TRC_STOP(...)
#define TRC_REC(...)
#define TRC_OUTPUT(...)

#if defined(MY_ABC_HERE)
#ifdef CONFIG_MV_TDM_EXT_STATS
	#define MV_TDM_EXT_STATS
#endif
#endif  

#if defined(MY_ABC_HERE)
 
#define TDM_CTRL_REGS_NUM         36
#define TDM_SPI_REGS_OFFSET           0x3100
#define TDM_SPI_REGS_NUM          16
#endif  

struct mv_phone_dev {
	void __iomem *tdm_base;
	void __iomem *pll_base;
	void __iomem *dco_div_reg;
	MV_TDM_PARAMS *tdm_params;
	struct platform_device *parent;
	struct device_node *np;
	struct clk *clk;
	u32 pclk_freq_mhz;
	int irq;

#if defined(MY_ABC_HERE)
	 
	u32 tdm_ctrl_regs[TDM_CTRL_REGS_NUM];
	u32 tdm_spi_regs[TDM_SPI_REGS_NUM];
	u32 tdm_spi_mux_reg;
	u32 tdm_mbus_config_reg;
	u32 tdm_misc_reg;
#endif  
};

typedef enum {
	SLIC_EXTERNAL_ID,
	SLIC_ZARLINK_ID,
	SLIC_SILABS_ID,
	SLIC_LANTIQ_ID
} MV_SLIC_UNIT_TYPE;

typedef enum {
	MV_TDM_UNIT_NONE,
	MV_TDM_UNIT_TDM2C,
	MV_TDM_UNIT_TDMMC
} MV_TDM_UNIT_TYPE;

enum {
	SPI_TYPE_FLASH = 0,
	SPI_TYPE_SLIC_ZARLINK_SILABS,
	SPI_TYPE_SLIC_LANTIQ,
	SPI_TYPE_SLIC_ZSI,
	SPI_TYPE_SLIC_ISI
};

typedef enum _devBoardSlicType {
	MV_BOARD_SLIC_DISABLED,
	MV_BOARD_SLIC_SSI_ID,  
	MV_BOARD_SLIC_ISI_ID,  
	MV_BOARD_SLIC_ZSI_ID,  
	MV_BOARD_SLIC_EXTERNAL_ID  
} MV_BOARD_SLIC_TYPE;

u32 mvBoardSlicUnitTypeGet(void);
u32 mvCtrlTdmUnitIrqGet(void);
MV_TDM_UNIT_TYPE mvCtrlTdmUnitTypeGet(void);
int mvSysTdmInit(MV_TDM_PARAMS *tdmParams);

#endif  
