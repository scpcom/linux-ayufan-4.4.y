 
#include "mvCommon.h"
#include "mvBoardEnvLib.h"
#include "mvBoardEnvSpec.h"
#include "twsi/mvTwsi.h"
#include "pex/mvPexRegs.h"

#define ARRSZ(x)	(sizeof(x)/sizeof(x[0]))

#define DB_88F6710_BOARD_NOR_READ_PARAMS	0x403E07CF
#define DB_88F6710_BOARD_NOR_WRITE_PARAMS	0x000F0F0F

MV_U8	db88f6710InfoBoardDebugLedIf[] = {59, 60, 61};

MV_BOARD_TWSI_INFO	db88f6710InfoBoardTwsiDev[] = {
	 
	{BOARD_DEV_TWSI_SATR, 0x4C, ADDR7_BIT},
	{BOARD_DEV_TWSI_SATR, 0x4D, ADDR7_BIT},
	{BOARD_DEV_TWSI_SATR, 0x4E, ADDR7_BIT},
};

MV_BOARD_MAC_INFO db88f6710InfoBoardMacInfo[] = {
	 
	{BOARD_MAC_SPEED_AUTO, 0x0,0,0},
	{BOARD_MAC_SPEED_AUTO, 0x1,0,0},
};

MV_BOARD_SWITCH_INFO db88f6710InfoBoardSwitchValue[] = {
	{
		.switchIrq = (31 + 128),	 
		.switchPort = {0, 1, 2, 3, 4},
		.cpuPort = 6,
		.connectedPort = {-1, 6},
		.smiScanMode = 2,
		.quadPhyAddr = 0,
		.forceLinkMask = 0x0
	}
};

MV_BOARD_MODULE_TYPE_INFO db88f6710InfoBoardModTypeInfo[] = {
	{
		.boardMppGrp1Mod	= MV_BOARD_AUTO,
		.boardMppGrp2Mod	= MV_BOARD_AUTO
	}
};

MV_BOARD_GPP_INFO db88f6710InfoBoardGppInfo[] = {
	 
	{BOARD_GPP_USB_VBUS, 48}  
};

MV_DEV_CS_INFO db88f6710InfoBoardDeCsInfo[] = {
	 
#if defined(MV_INCLUDE_SPI)
	{SPI_CS0, N_A, BOARD_DEV_SPI_FLASH, 8, 8},  
#endif
#if defined(MV_INCLUDE_NOR)
	{DEV_BOOCS, N_A, BOARD_DEV_NOR_FLASH, 16, 16}  
#endif
};

MV_BOARD_MPP_INFO db88f6710InfoBoardMppConfigValue[] = {
	{ {
		DB_88F6710_MPP0_7,
		DB_88F6710_MPP8_15,
		DB_88F6710_MPP16_23,
		DB_88F6710_MPP24_31,
		DB_88F6710_MPP32_39,
		DB_88F6710_MPP40_47,
		DB_88F6710_MPP48_55,
		DB_88F6710_MPP56_63,
		DB_88F6710_MPP64_67,
	} }
};

MV_BOARD_TDM_INFO	db88f6710Tdm880[]	= { {0} };  

MV_BOARD_TDM_SPI_INFO db88f6710TdmSpiInfo[] = { {1} };  

MV_BOARD_INFO db88f6710Info = {
	.boardName				= "DB-88F6710-BP",
	.enableModuleScan 			= MV_TRUE,
	.numBoardMppTypeValue		= ARRSZ(db88f6710InfoBoardModTypeInfo),
	.pBoardModTypeValue			= db88f6710InfoBoardModTypeInfo,
	.numBoardMppConfigValue		= ARRSZ(db88f6710InfoBoardMppConfigValue),
	.pBoardMppConfigValue		= db88f6710InfoBoardMppConfigValue,
	.intsGppMaskLow				= BIT31,	 
	.intsGppMaskMid				= 0,
	.intsGppMaskHigh			= 0,
	.numBoardDeviceIf			= ARRSZ(db88f6710InfoBoardDeCsInfo),
	.pDevCsInfo					= db88f6710InfoBoardDeCsInfo,
	.numBoardTwsiDev			= ARRSZ(db88f6710InfoBoardTwsiDev),
	.pBoardTwsiDev				= db88f6710InfoBoardTwsiDev,
	.numBoardMacInfo			= ARRSZ(db88f6710InfoBoardMacInfo),
	.pBoardMacInfo				= db88f6710InfoBoardMacInfo,
	.numBoardGppInfo			= ARRSZ(db88f6710InfoBoardGppInfo),
	.pBoardGppInfo				= db88f6710InfoBoardGppInfo,
	.activeLedsNumber			= ARRSZ(db88f6710InfoBoardDebugLedIf),
	.pLedGppPin					= db88f6710InfoBoardDebugLedIf,
	.ledsPolarity				= 0,

	.pmuPwrUpPolarity			= 0,
	.pmuPwrUpDelay				= 16000,

	.gppOutEnValLow			= DB_88F6710_GPP_OUT_ENA_LOW,
	.gppOutEnValMid			= DB_88F6710_GPP_OUT_ENA_MID,
	.gppOutEnValHigh		= DB_88F6710_GPP_OUT_ENA_HIGH,
	.gppOutValLow			= DB_88F6710_GPP_OUT_VAL_LOW,
	.gppOutValMid			= DB_88F6710_GPP_OUT_VAL_MID,
	.gppOutValHigh			= DB_88F6710_GPP_OUT_VAL_HIGH,
	.gppPolarityValLow		= DB_88F6710_GPP_POL_LOW,
	.gppPolarityValMid		= DB_88F6710_GPP_POL_MID,
	.gppPolarityValHigh		= DB_88F6710_GPP_POL_HIGH,

	.pSwitchInfo = db88f6710InfoBoardSwitchValue,
	.switchInfoNum = ARRSZ(db88f6710InfoBoardSwitchValue),

	.numBoardTdmInfo		= {1},
	.pBoardTdmInt2CsInfo		= {db88f6710Tdm880},
	.boardTdmInfoIndex		= 0,
	.pBoardTdmSpiInfo 		= db88f6710TdmSpiInfo,

	.norFlashReadParams		= DB_88F6710_BOARD_NOR_READ_PARAMS,
	.norFlashWriteParams	= DB_88F6710_BOARD_NOR_WRITE_PARAMS
};

MV_U8	db88f6710pcacInfoBoardDebugLedIf[] = {58, 59, 61};

MV_BOARD_MAC_INFO db88f6710pcacInfoBoardMacInfo[] = {
	 
	{BOARD_MAC_SPEED_AUTO, 0x0, 0, 0},
	{BOARD_MAC_SPEED_AUTO, 0x1, 0, 0},
};

MV_BOARD_MODULE_TYPE_INFO db88f6710pcacInfoBoardModTypeInfo[] = {
	{
		.boardMppGrp1Mod	= MV_BOARD_AUTO,
		.boardMppGrp2Mod	= MV_BOARD_AUTO
	}
};

MV_BOARD_GPP_INFO db88f6710pcacInfoBoardGppInfo[] = {
	 
	{BOARD_GPP_USB_VBUS, 24}  
};

MV_DEV_CS_INFO db88f6710pcacInfoBoardDeCsInfo[] = {
	 
#if defined(MV_INCLUDE_SPI)
	{SPI_CS0, N_A, BOARD_DEV_SPI_FLASH, 8, 8},  
#endif
};

MV_BOARD_MPP_INFO db88f6710pcacInfoBoardMppConfigValue[] = {
	{ {
		DB_88F6710_PCAC_MPP0_7,
		DB_88F6710_PCAC_MPP8_15,
		DB_88F6710_PCAC_MPP16_23,
		DB_88F6710_PCAC_MPP24_31,
		DB_88F6710_PCAC_MPP32_39,
		DB_88F6710_PCAC_MPP40_47,
		DB_88F6710_PCAC_MPP48_55,
		DB_88F6710_PCAC_MPP56_63,
		DB_88F6710_PCAC_MPP64_67,
	} }
};

MV_BOARD_INFO db88f6710pcacInfo = {
	.boardName					= "DB-88F6710-PCAC",
	.enableModuleScan 			= MV_FALSE,
	.numBoardMppTypeValue		= ARRSZ(db88f6710pcacInfoBoardModTypeInfo),
	.pBoardModTypeValue			= db88f6710pcacInfoBoardModTypeInfo,
	.numBoardMppConfigValue		= ARRSZ(db88f6710pcacInfoBoardMppConfigValue),
	.pBoardMppConfigValue		= db88f6710pcacInfoBoardMppConfigValue,
	.intsGppMaskLow				= BIT31,	 
	.intsGppMaskMid				= 0,
	.intsGppMaskHigh			= 0,
	.numBoardDeviceIf			= ARRSZ(db88f6710pcacInfoBoardDeCsInfo),
	.pDevCsInfo					= db88f6710pcacInfoBoardDeCsInfo,
	.numBoardTwsiDev			= 0,
	.pBoardTwsiDev				= NULL,
	.numBoardMacInfo			= ARRSZ(db88f6710pcacInfoBoardMacInfo),
	.pBoardMacInfo				= db88f6710pcacInfoBoardMacInfo,
	.numBoardGppInfo			= ARRSZ(db88f6710pcacInfoBoardGppInfo),
	.pBoardGppInfo				= db88f6710pcacInfoBoardGppInfo,
	.activeLedsNumber			= ARRSZ(db88f6710pcacInfoBoardDebugLedIf),
	.pLedGppPin					= db88f6710pcacInfoBoardDebugLedIf,
	.ledsPolarity				= 0,

	.pmuPwrUpPolarity			= 0,
	.pmuPwrUpDelay				= 80000,

	.gppOutEnValLow			= DB_88F6710_PCAC_GPP_OUT_ENA_LOW,
	.gppOutEnValMid			= DB_88F6710_PCAC_GPP_OUT_ENA_MID,
	.gppOutEnValHigh		= DB_88F6710_PCAC_GPP_OUT_ENA_HIGH,
	.gppOutValLow			= DB_88F6710_PCAC_GPP_OUT_VAL_LOW,
	.gppOutValMid			= DB_88F6710_PCAC_GPP_OUT_VAL_MID,
	.gppOutValHigh			= DB_88F6710_PCAC_GPP_OUT_VAL_HIGH,
	.gppPolarityValLow		= DB_88F6710_PCAC_GPP_POL_LOW,
	.gppPolarityValMid		= DB_88F6710_PCAC_GPP_POL_MID,
	.gppPolarityValHigh		= DB_88F6710_PCAC_GPP_POL_HIGH,

	.pSwitchInfo = NULL,
	.switchInfoNum = 0,

	.numBoardTdmInfo	= { 0 },
	.pBoardTdmInt2CsInfo	= { NULL },
	.boardTdmInfoIndex	= -1,

	.norFlashReadParams	= 0,
	.norFlashWriteParams	= 0,
};

#define RD_88F6710_BOARD_NOR_READ_PARAMS	0x403E07CF
#define RD_88F6710_BOARD_NOR_WRITE_PARAMS	0x000F0F0F

MV_U8	rd88F6710InfoBoardDebugLedIf[] = {32};

MV_BOARD_TWSI_INFO	rd88F6710InfoBoardTwsiDev[] = {
	 
	{BOARD_DEV_TWSI_SATR, 0x50, ADDR7_BIT},
};

MV_BOARD_MAC_INFO rd88F6710InfoBoardMacInfo[] = {
	 
	{BOARD_MAC_SPEED_AUTO, 0x0,  0, 0},
	{BOARD_MAC_SPEED_1000M, 0x10, 0, 0},
};

MV_BOARD_SWITCH_INFO rd88F6710InfoBoardSwitchValue[] = {
	{
		.switchIrq = (31 + 128),	 
		.switchPort = {0, 1, 2, 3, -1},
		.cpuPort = 5,
		.connectedPort = {-1, 5},
		.smiScanMode = 2,
		.quadPhyAddr = 0,
		.forceLinkMask = 0x0
	}
};

MV_BOARD_MODULE_TYPE_INFO rd88F6710InfoBoardModTypeInfo[] = {
	{
		.boardMppGrp1Mod	= MV_BOARD_SDIO | MV_BOARD_RGMII1,
		.boardMppGrp2Mod	= MV_BOARD_TDM
	}
};

MV_BOARD_GPP_INFO rd88F6710InfoBoardGppInfo[] = {
	 
	{BOARD_GPP_USB_VBUS, 24}  
};

MV_DEV_CS_INFO rd88F6710InfoBoardDeCsInfo[] = {
	 
#if defined(MV_INCLUDE_SPI)
	{SPI_CS0, N_A, BOARD_DEV_SPI_FLASH, 8, 8},  
#endif
#if defined(MV_INCLUDE_NOR)
	{DEV_BOOCS, N_A, BOARD_DEV_NOR_FLASH, 16, 16}  
#endif
};

MV_BOARD_MPP_INFO rd88F6710InfoBoardMppConfigValue[] = {
	{ {
		RD_88F6710_MPP0_7,
		RD_88F6710_MPP8_15,
		RD_88F6710_MPP16_23,
		RD_88F6710_MPP24_31,
		RD_88F6710_MPP32_39,
		RD_88F6710_MPP40_47,
		RD_88F6710_MPP48_55,
		RD_88F6710_MPP56_63,
		RD_88F6710_MPP64_67,
	} }
};

MV_BOARD_TDM_INFO	rd88F6710Tdm880[]	= { {1}, {2} };
MV_BOARD_TDM_INFO	rd88F6710Tdm792[]	= { {1}, {2}, {3}, {4}, {6}, {7} };
MV_BOARD_TDM_INFO	rd88F6710Tdm3215[]	= { {1} };

MV_BOARD_INFO rd88F6710Info = {
	.boardName				= "RD-88F6710",
	.enableModuleScan 			= MV_FALSE,
	.numBoardMppTypeValue			= ARRSZ(rd88F6710InfoBoardModTypeInfo),
	.pBoardModTypeValue			= rd88F6710InfoBoardModTypeInfo,
	.numBoardMppConfigValue			= ARRSZ(rd88F6710InfoBoardMppConfigValue),
	.pBoardMppConfigValue			= rd88F6710InfoBoardMppConfigValue,
	.intsGppMaskLow				= BIT31,	 
	.intsGppMaskMid				= 0,
	.intsGppMaskHigh			= 0,
	.numBoardDeviceIf			= ARRSZ(rd88F6710InfoBoardDeCsInfo),
	.pDevCsInfo				= rd88F6710InfoBoardDeCsInfo,
	.numBoardTwsiDev			= ARRSZ(rd88F6710InfoBoardTwsiDev),
	.pBoardTwsiDev				= rd88F6710InfoBoardTwsiDev,
	.numBoardMacInfo			= ARRSZ(rd88F6710InfoBoardMacInfo),
	.pBoardMacInfo				= rd88F6710InfoBoardMacInfo,
	.numBoardGppInfo			= ARRSZ(rd88F6710InfoBoardGppInfo),
	.pBoardGppInfo				= rd88F6710InfoBoardGppInfo,
	.activeLedsNumber			= ARRSZ(rd88F6710InfoBoardDebugLedIf),
	.pLedGppPin				= rd88F6710InfoBoardDebugLedIf,
	.ledsPolarity				= 0,

	.pmuPwrUpPolarity			= 0,
	.pmuPwrUpDelay				= 80000,

	.gppOutEnValLow			= RD_88F6710_GPP_OUT_ENA_LOW,
	.gppOutEnValMid			= RD_88F6710_GPP_OUT_ENA_MID,
	.gppOutEnValHigh		= RD_88F6710_GPP_OUT_ENA_HIGH,
	.gppOutValLow			= RD_88F6710_GPP_OUT_VAL_LOW,
	.gppOutValMid			= RD_88F6710_GPP_OUT_VAL_MID,
	.gppOutValHigh			= RD_88F6710_GPP_OUT_VAL_HIGH,
	.gppPolarityValLow		= RD_88F6710_GPP_POL_LOW,
	.gppPolarityValMid		= RD_88F6710_GPP_POL_MID,
	.gppPolarityValHigh		= RD_88F6710_GPP_POL_HIGH,

	.pSwitchInfo = rd88F6710InfoBoardSwitchValue,
	.switchInfoNum = ARRSZ(rd88F6710InfoBoardSwitchValue),

	.numBoardTdmInfo		= {2, 6, 1},
	.pBoardTdmInt2CsInfo		= {rd88F6710Tdm880, rd88F6710Tdm792, rd88F6710Tdm3215},
	.boardTdmInfoIndex		= -1,

	.norFlashReadParams		= RD_88F6710_BOARD_NOR_READ_PARAMS,
	.norFlashWriteParams	= RD_88F6710_BOARD_NOR_WRITE_PARAMS
};

#if defined(CONFIG_SYNO_ARMADA_ARCH)
 
MV_BOARD_MAC_INFO synods213jInfoBoardMacInfo[] = {
	 
	{BOARD_MAC_SPEED_AUTO, 0x1, 0, 0},
};

MV_BOARD_MODULE_TYPE_INFO synods213jInfoBoardModTypeInfo[] = {
	{
		.boardMppGrp1Mod	= MV_BOARD_AUTO,
		.boardMppGrp2Mod	= MV_BOARD_AUTO
	}
};

MV_DEV_CS_INFO synods213jInfoBoardDeCsInfo[] = {
	 
#if defined(MV_INCLUDE_SPI)
	{SPI_CS0, N_A, BOARD_DEV_SPI_FLASH, 8, 8},  
#endif
#if defined(MV_INCLUDE_NOR)
	{DEV_BOOCS, N_A, BOARD_DEV_NOR_FLASH, 16, 16}  
#endif
};

MV_BOARD_MPP_INFO synods213jInfoBoardMppConfigValue[] = {
	{ {
		SYNO_DS213j_MPP0_7,
		SYNO_DS213j_MPP8_15,
		SYNO_DS213j_MPP16_23,
		SYNO_DS213j_MPP24_31,
		SYNO_DS213j_MPP32_39,
		SYNO_DS213j_MPP40_47,
		SYNO_DS213j_MPP48_55,
		SYNO_DS213j_MPP56_63,
		SYNO_DS213j_MPP64_67,
	} }
};

MV_BOARD_TDM_INFO	synods213jTdm880[]	= { {0} };

MV_BOARD_TDM_SPI_INFO synods213jTdmSpiInfo[] = { {1} };

MV_BOARD_INFO synods213jInfo = {
	.boardName				= "SYNO-DS213j-BP",
	.enableModuleScan 			= MV_FALSE,
	.numBoardMppTypeValue		= ARRSZ(synods213jInfoBoardModTypeInfo),
	.pBoardModTypeValue			= synods213jInfoBoardModTypeInfo,
	.numBoardMppConfigValue		= ARRSZ(synods213jInfoBoardMppConfigValue),
	.pBoardMppConfigValue		= synods213jInfoBoardMppConfigValue,
	.intsGppMaskLow				= 0,
	.intsGppMaskMid				= 0,
	.intsGppMaskHigh			= 0,
	.numBoardDeviceIf			= ARRSZ(synods213jInfoBoardDeCsInfo),
	.pDevCsInfo					= synods213jInfoBoardDeCsInfo,
	.numBoardTwsiDev			= 0,
	.pBoardTwsiDev				= NULL,
	.numBoardMacInfo			= ARRSZ(synods213jInfoBoardMacInfo),
	.pBoardMacInfo				= synods213jInfoBoardMacInfo,
	.numBoardGppInfo			= 0,
	.pBoardGppInfo				= NULL,
	.activeLedsNumber			= 0,
	.pLedGppPin					= NULL,
	.ledsPolarity				= 0,

	.pmuPwrUpPolarity			= 0,
	.pmuPwrUpDelay				= 16000,

	.gppOutEnValLow			= SYNO_DS213j_GPP_OUT_ENA_LOW,
	.gppOutEnValMid			= SYNO_DS213j_GPP_OUT_ENA_MID,
	.gppOutEnValHigh		= SYNO_DS213j_GPP_OUT_ENA_HIGH,
	.gppOutValLow			= SYNO_DS213j_GPP_OUT_VAL_LOW,
	.gppOutValMid			= SYNO_DS213j_GPP_OUT_VAL_MID,
	.gppOutValHigh			= SYNO_DS213j_GPP_OUT_VAL_HIGH,
	.gppPolarityValLow		= SYNO_DS213j_GPP_POL_LOW,
	.gppPolarityValMid		= SYNO_DS213j_GPP_POL_MID,
	.gppPolarityValHigh		= SYNO_DS213j_GPP_POL_HIGH,

	.pSwitchInfo = NULL,
	.switchInfoNum = 0,

	.numBoardTdmInfo		= {1},
	.pBoardTdmInt2CsInfo		= {synods213jTdm880},
	.boardTdmInfoIndex		= 0,
	.pBoardTdmSpiInfo 		= synods213jTdmSpiInfo,

	.norFlashReadParams		= 0,
	.norFlashWriteParams	= 0
};

MV_BOARD_MAC_INFO synods214seInfoBoardMacInfo[] = {
	 
	{BOARD_MAC_SPEED_AUTO, 0x1, 0, 0},
};

MV_BOARD_MODULE_TYPE_INFO synods214seInfoBoardModTypeInfo[] = {
	{
		.boardMppGrp1Mod	= MV_BOARD_RGMII0,
		.boardMppGrp2Mod	= MV_BOARD_AUTO
	}
};

MV_DEV_CS_INFO synods214seInfoBoardDeCsInfo[] = {
	 
#if defined(MV_INCLUDE_SPI)
	{SPI_CS0, N_A, BOARD_DEV_SPI_FLASH, 8, 8},  
#endif
#if defined(MV_INCLUDE_NOR)
	{DEV_BOOCS, N_A, BOARD_DEV_NOR_FLASH, 16, 16}  
#endif
};

MV_BOARD_MPP_INFO synods214seInfoBoardMppConfigValue[] = {
	{ {
		SYNO_DS214se_MPP0_7,
		SYNO_DS214se_MPP8_15,
		SYNO_DS214se_MPP16_23,
		SYNO_DS214se_MPP24_31,
		SYNO_DS214se_MPP32_39,
		SYNO_DS214se_MPP40_47,
		SYNO_DS214se_MPP48_55,
		SYNO_DS214se_MPP56_63,
		SYNO_DS214se_MPP64_67,
	} }
};

MV_BOARD_TDM_INFO	synods214seTdm880[]	= { {0} };

MV_BOARD_TDM_SPI_INFO synods214seTdmSpiInfo[] = { {1} };

MV_BOARD_INFO synods214seInfo = {
	.boardName				= "SYNO-DS214se-BP",
	.enableModuleScan 			= MV_FALSE,
	.numBoardMppTypeValue		= ARRSZ(synods214seInfoBoardModTypeInfo),
	.pBoardModTypeValue			= synods214seInfoBoardModTypeInfo,
	.numBoardMppConfigValue		= ARRSZ(synods214seInfoBoardMppConfigValue),
	.pBoardMppConfigValue		= synods214seInfoBoardMppConfigValue,
	.intsGppMaskLow				= 0,
	.intsGppMaskMid				= 0,
	.intsGppMaskHigh			= 0,
	.numBoardDeviceIf			= ARRSZ(synods214seInfoBoardDeCsInfo),
	.pDevCsInfo					= synods214seInfoBoardDeCsInfo,
	.numBoardTwsiDev			= 0,
	.pBoardTwsiDev				= NULL,
	.numBoardMacInfo			= ARRSZ(synods214seInfoBoardMacInfo),
	.pBoardMacInfo				= synods214seInfoBoardMacInfo,
	.numBoardGppInfo			= 0,
	.pBoardGppInfo				= NULL,
	.activeLedsNumber			= 0,
	.pLedGppPin					= NULL,
	.ledsPolarity				= 0,

	.pmuPwrUpPolarity			= 0,
	.pmuPwrUpDelay				= 16000,

	.gppOutEnValLow			= SYNO_DS214se_GPP_OUT_ENA_LOW,
	.gppOutEnValMid			= SYNO_DS214se_GPP_OUT_ENA_MID,
	.gppOutEnValHigh		= SYNO_DS214se_GPP_OUT_ENA_HIGH,
	.gppOutValLow			= SYNO_DS214se_GPP_OUT_VAL_LOW,
	.gppOutValMid			= SYNO_DS214se_GPP_OUT_VAL_MID,
	.gppOutValHigh			= SYNO_DS214se_GPP_OUT_VAL_HIGH,
	.gppPolarityValLow		= SYNO_DS214se_GPP_POL_LOW,
	.gppPolarityValMid		= SYNO_DS214se_GPP_POL_MID,
	.gppPolarityValHigh		= SYNO_DS214se_GPP_POL_HIGH,

	.pSwitchInfo = NULL,
	.switchInfoNum = 0,

	.numBoardTdmInfo		= {1},
	.pBoardTdmInt2CsInfo		= {synods214seTdm880},
	.boardTdmInfoIndex		= 0,
	.pBoardTdmSpiInfo 		= synods214seTdmSpiInfo,

	.norFlashReadParams		= 0,
	.norFlashWriteParams	= 0
};

MV_BOARD_MAC_INFO synous3InfoBoardMacInfo[] = {
	 
	{BOARD_MAC_SPEED_AUTO, 0x1, 0 ,0},
	{BOARD_MAC_SPEED_AUTO, 0x0, 0 ,0},
};

MV_BOARD_MODULE_TYPE_INFO synous3InfoBoardModTypeInfo[] = {
	{
		.boardMppGrp1Mod	= MV_BOARD_RGMII1 | MV_BOARD_RGMII0,
		.boardMppGrp2Mod	= MV_BOARD_AUTO
	}
};

MV_DEV_CS_INFO synous3InfoBoardDeCsInfo[] = {
	 
#if defined(MV_INCLUDE_SPI)
	{SPI_CS0, N_A, BOARD_DEV_SPI_FLASH, 8, 8},  
#endif
#if defined(MV_INCLUDE_NOR)
	{DEV_BOOCS, N_A, BOARD_DEV_NOR_FLASH, 16, 16}  
#endif
};

MV_BOARD_MPP_INFO synous3InfoBoardMppConfigValue[] = {
	{ {
		SYNO_US3_MPP0_7,
		SYNO_US3_MPP8_15,
		SYNO_US3_MPP16_23,
		SYNO_US3_MPP24_31,
		SYNO_US3_MPP32_39,
		SYNO_US3_MPP40_47,
		SYNO_US3_MPP48_55,
		SYNO_US3_MPP56_63,
		SYNO_US3_MPP64_67,
	} }
};

MV_BOARD_TDM_INFO	synous3Tdm880[]	= { {0} };

MV_BOARD_TDM_SPI_INFO synous3TdmSpiInfo[] = { {1} };

MV_BOARD_INFO synous3Info = {
	.boardName				= "SYNO-US3-BP",
	.enableModuleScan 			= MV_FALSE,
	.numBoardMppTypeValue		= ARRSZ(synous3InfoBoardModTypeInfo),
	.pBoardModTypeValue			= synous3InfoBoardModTypeInfo,
	.numBoardMppConfigValue		= ARRSZ(synous3InfoBoardMppConfigValue),
	.pBoardMppConfigValue		= synous3InfoBoardMppConfigValue,
	.intsGppMaskLow				= 0,
	.intsGppMaskMid				= 0,
	.intsGppMaskHigh			= 0,
	.numBoardDeviceIf			= ARRSZ(synous3InfoBoardDeCsInfo),
	.pDevCsInfo					= synous3InfoBoardDeCsInfo,
	.numBoardTwsiDev			= 0,
	.pBoardTwsiDev				= NULL,
	.numBoardMacInfo			= ARRSZ(synous3InfoBoardMacInfo),
	.pBoardMacInfo				= synous3InfoBoardMacInfo,
	.numBoardGppInfo			= 0,
	.pBoardGppInfo				= NULL,
	.activeLedsNumber			= 0,
	.pLedGppPin					= NULL,
	.ledsPolarity				= 0,

	.pmuPwrUpPolarity			= 0,
	.pmuPwrUpDelay				= 16000,

	.gppOutEnValLow			= SYNO_US3_GPP_OUT_ENA_LOW,
	.gppOutEnValMid			= SYNO_US3_GPP_OUT_ENA_MID,
	.gppOutEnValHigh		= SYNO_US3_GPP_OUT_ENA_HIGH,
	.gppOutValLow			= SYNO_US3_GPP_OUT_VAL_LOW,
	.gppOutValMid			= SYNO_US3_GPP_OUT_VAL_MID,
	.gppOutValHigh			= SYNO_US3_GPP_OUT_VAL_HIGH,
	.gppPolarityValLow		= SYNO_US3_GPP_POL_LOW,
	.gppPolarityValMid		= SYNO_US3_GPP_POL_MID,
	.gppPolarityValHigh		= SYNO_US3_GPP_POL_HIGH,

	.pSwitchInfo = NULL,
	.switchInfoNum = 0,

	.numBoardTdmInfo		= {1},
	.pBoardTdmInt2CsInfo		= {synous3Tdm880},
	.boardTdmInfoIndex		= 0,
	.pBoardTdmSpiInfo 		= synous3TdmSpiInfo,

	.norFlashReadParams		= 0,
	.norFlashWriteParams	= 0
};

MV_BOARD_MAC_INFO synors214InfoBoardMacInfo[] = {
	 
	{BOARD_MAC_SPEED_AUTO, 0x1, 0, 0},
	{BOARD_MAC_SPEED_AUTO, 0x0, 0, 0},
};

MV_BOARD_MODULE_TYPE_INFO synors214InfoBoardModTypeInfo[] = {
	{
		.boardMppGrp1Mod	= MV_BOARD_RGMII1 | MV_BOARD_RGMII0,
		.boardMppGrp2Mod	= MV_BOARD_AUTO
	}
};

MV_DEV_CS_INFO synors214InfoBoardDeCsInfo[] = {
	 
#if defined(MV_INCLUDE_SPI)
	{SPI_CS0, N_A, BOARD_DEV_SPI_FLASH, 8, 8},  
#endif
#if defined(MV_INCLUDE_NOR)
	{DEV_BOOCS, N_A, BOARD_DEV_NOR_FLASH, 16, 16}  
#endif
};

MV_BOARD_MPP_INFO synors214InfoBoardMppConfigValue[] = {
	{ {
		SYNO_RS214_MPP0_7,
		SYNO_RS214_MPP8_15,
		SYNO_RS214_MPP16_23,
		SYNO_RS214_MPP24_31,
		SYNO_RS214_MPP32_39,
		SYNO_RS214_MPP40_47,
		SYNO_RS214_MPP48_55,
		SYNO_RS214_MPP56_63,
		SYNO_RS214_MPP64_67,
	} }
};

MV_BOARD_TDM_INFO	synors214Tdm880[]	= { {0} };

MV_BOARD_TDM_SPI_INFO synors214TdmSpiInfo[] = { {1} };

MV_BOARD_INFO synors214Info = {
	.boardName				= "SYNO-RS214-BP",
	.enableModuleScan 			= MV_FALSE,
	.numBoardMppTypeValue		= ARRSZ(synors214InfoBoardModTypeInfo),
	.pBoardModTypeValue			= synors214InfoBoardModTypeInfo,
	.numBoardMppConfigValue		= ARRSZ(synors214InfoBoardMppConfigValue),
	.pBoardMppConfigValue		= synors214InfoBoardMppConfigValue,
	.intsGppMaskLow				= 0,
	.intsGppMaskMid				= 0,
	.intsGppMaskHigh			= 0,
	.numBoardDeviceIf			= ARRSZ(synors214InfoBoardDeCsInfo),
	.pDevCsInfo					= synors214InfoBoardDeCsInfo,
	.numBoardTwsiDev			= 0,
	.pBoardTwsiDev				= NULL,
	.numBoardMacInfo			= ARRSZ(synors214InfoBoardMacInfo),
	.pBoardMacInfo				= synors214InfoBoardMacInfo,
	.numBoardGppInfo			= 0,
	.pBoardGppInfo				= NULL,
	.activeLedsNumber			= 0,
	.pLedGppPin					= NULL,
	.ledsPolarity				= 0,

	.pmuPwrUpPolarity			= 0,
	.pmuPwrUpDelay				= 16000,

	.gppOutEnValLow			= SYNO_RS214_GPP_OUT_ENA_LOW,
	.gppOutEnValMid			= SYNO_RS214_GPP_OUT_ENA_MID,
	.gppOutEnValHigh		= SYNO_RS214_GPP_OUT_ENA_HIGH,
	.gppOutValLow			= SYNO_RS214_GPP_OUT_VAL_LOW,
	.gppOutValMid			= SYNO_RS214_GPP_OUT_VAL_MID,
	.gppOutValHigh			= SYNO_RS214_GPP_OUT_VAL_HIGH,
	.gppPolarityValLow		= SYNO_RS214_GPP_POL_LOW,
	.gppPolarityValMid		= SYNO_RS214_GPP_POL_MID,
	.gppPolarityValHigh		= SYNO_RS214_GPP_POL_HIGH,

	.pSwitchInfo = NULL,
	.switchInfoNum = 0,

	.numBoardTdmInfo		= {1},
	.pBoardTdmInt2CsInfo		= {synors214Tdm880},
	.boardTdmInfoIndex		= 0,
	.pBoardTdmSpiInfo 		= synors214TdmSpiInfo,

	.norFlashReadParams		= 0,
	.norFlashWriteParams	= 0
};

MV_BOARD_MAC_INFO synods414slimInfoBoardMacInfo[] = {
	 
	{BOARD_MAC_SPEED_AUTO, 0x1, 0, 0},
	{BOARD_MAC_SPEED_AUTO, 0x0, 0, 0},
};

MV_BOARD_MODULE_TYPE_INFO synods414slimInfoBoardModTypeInfo[] = {
	{
		.boardMppGrp1Mod	= MV_BOARD_RGMII0|MV_BOARD_RGMII1,
		.boardMppGrp2Mod	= MV_BOARD_AUTO
	}
};

MV_DEV_CS_INFO synods414slimInfoBoardDeCsInfo[] = {
	 
#if defined(MV_INCLUDE_SPI)
	{SPI_CS0, N_A, BOARD_DEV_SPI_FLASH, 8, 8},  
#endif
#if defined(MV_INCLUDE_NOR)
	{DEV_BOOCS, N_A, BOARD_DEV_NOR_FLASH, 16, 16}  
#endif
};

MV_BOARD_MPP_INFO synods414slimInfoBoardMppConfigValue[] = {
	{ {
		SYNO_DS414slim_MPP0_7,
		SYNO_DS414slim_MPP8_15,
		SYNO_DS414slim_MPP16_23,
		SYNO_DS414slim_MPP24_31,
		SYNO_DS414slim_MPP32_39,
		SYNO_DS414slim_MPP40_47,
		SYNO_DS414slim_MPP48_55,
		SYNO_DS414slim_MPP56_63,
		SYNO_DS414slim_MPP64_67,
	} }
};

MV_BOARD_TDM_INFO	synods414slimTdm880[]	= { {0} };

MV_BOARD_TDM_SPI_INFO synods414slimTdmSpiInfo[] = { {1} };

MV_BOARD_INFO synods414slimInfo = {
	.boardName				= "SYNO-DS414slim",
	.enableModuleScan 			= MV_FALSE,
	.numBoardMppTypeValue		= ARRSZ(synods414slimInfoBoardModTypeInfo),
	.pBoardModTypeValue			= synods414slimInfoBoardModTypeInfo,
	.numBoardMppConfigValue		= ARRSZ(synods414slimInfoBoardMppConfigValue),
	.pBoardMppConfigValue		= synods414slimInfoBoardMppConfigValue,
	.intsGppMaskLow				= 0,
	.intsGppMaskMid				= 0,
	.intsGppMaskHigh			= 0,
	.numBoardDeviceIf			= ARRSZ(synods414slimInfoBoardDeCsInfo),
	.pDevCsInfo					= synods414slimInfoBoardDeCsInfo,
	.numBoardTwsiDev			= 0,
	.pBoardTwsiDev				= NULL,
	.numBoardMacInfo			= ARRSZ(synods414slimInfoBoardMacInfo),
	.pBoardMacInfo				= synods414slimInfoBoardMacInfo,
	.numBoardGppInfo			= 0,
	.pBoardGppInfo				= NULL,
	.activeLedsNumber			= 0,
	.pLedGppPin					= NULL,
	.ledsPolarity				= 0,

	.pmuPwrUpPolarity			= 0,
	.pmuPwrUpDelay				= 16000,

	.gppOutEnValLow			= SYNO_DS414slim_GPP_OUT_ENA_LOW,
	.gppOutEnValMid			= SYNO_DS414slim_GPP_OUT_ENA_MID,
	.gppOutEnValHigh		= SYNO_DS414slim_GPP_OUT_ENA_HIGH,
	.gppOutValLow			= SYNO_DS414slim_GPP_OUT_VAL_LOW,
	.gppOutValMid			= SYNO_DS414slim_GPP_OUT_VAL_MID,
	.gppOutValHigh			= SYNO_DS414slim_GPP_OUT_VAL_HIGH,
	.gppPolarityValLow		= SYNO_DS414slim_GPP_POL_LOW,
	.gppPolarityValMid		= SYNO_DS414slim_GPP_POL_MID,
	.gppPolarityValHigh		= SYNO_DS414slim_GPP_POL_HIGH,

	.pSwitchInfo = NULL,
	.switchInfoNum = 0,

	.numBoardTdmInfo		= {1},
	.pBoardTdmInt2CsInfo		= {synods414slimTdm880},
	.boardTdmInfoIndex		= 0,
	.pBoardTdmSpiInfo 		= synods414slimTdmSpiInfo,

	.norFlashReadParams		= 0,
	.norFlashWriteParams	= 0
};

MV_BOARD_MAC_INFO synods115jInfoBoardMacInfo[] = {
	 
	{BOARD_MAC_SPEED_AUTO, 0x1, 0, 0},
};

MV_BOARD_MODULE_TYPE_INFO synods115jInfoBoardModTypeInfo[] = {
	{
		.boardMppGrp1Mod	= MV_BOARD_RGMII0,
		.boardMppGrp2Mod	= MV_BOARD_AUTO
	}
};

MV_DEV_CS_INFO synods115jInfoBoardDeCsInfo[] = {
	 
#if defined(MV_INCLUDE_SPI)
	{SPI_CS0, N_A, BOARD_DEV_SPI_FLASH, 8, 8},  
#endif
#if defined(MV_INCLUDE_NOR)
	{DEV_BOOCS, N_A, BOARD_DEV_NOR_FLASH, 16, 16}  
#endif
};

MV_BOARD_MPP_INFO synods115jInfoBoardMppConfigValue[] = {
	{ {
		SYNO_DS115j_MPP0_7,
		SYNO_DS115j_MPP8_15,
		SYNO_DS115j_MPP16_23,
		SYNO_DS115j_MPP24_31,
		SYNO_DS115j_MPP32_39,
		SYNO_DS115j_MPP40_47,
		SYNO_DS115j_MPP48_55,
		SYNO_DS115j_MPP56_63,
		SYNO_DS115j_MPP64_67,
	} }
};

MV_BOARD_TDM_INFO	synods115jTdm880[]	= { {0} };

MV_BOARD_TDM_SPI_INFO synods115jTdmSpiInfo[] = { {1} };

MV_BOARD_INFO synods115jInfo = {
	.boardName					= "SYNO-DS115j",
	.enableModuleScan 			= MV_FALSE,
	.numBoardMppTypeValue		= ARRSZ(synods115jInfoBoardModTypeInfo),
	.pBoardModTypeValue			= synods115jInfoBoardModTypeInfo,
	.numBoardMppConfigValue		= ARRSZ(synods115jInfoBoardMppConfigValue),
	.pBoardMppConfigValue		= synods115jInfoBoardMppConfigValue,
	.intsGppMaskLow				= 0,
	.intsGppMaskMid				= 0,
	.intsGppMaskHigh			= 0,
	.numBoardDeviceIf			= ARRSZ(synods115jInfoBoardDeCsInfo),
	.pDevCsInfo					= synods115jInfoBoardDeCsInfo,
	.numBoardTwsiDev			= 0,
	.pBoardTwsiDev				= NULL,
	.numBoardMacInfo			= ARRSZ(synods115jInfoBoardMacInfo),
	.pBoardMacInfo				= synods115jInfoBoardMacInfo,
	.numBoardGppInfo			= 0,
	.pBoardGppInfo				= NULL,
	.activeLedsNumber			= 0,
	.pLedGppPin					= NULL,
	.ledsPolarity				= 0,

	.pmuPwrUpPolarity			= 0,
	.pmuPwrUpDelay				= 16000,

	.gppOutEnValLow			= SYNO_DS115j_GPP_OUT_ENA_LOW,
	.gppOutEnValMid			= SYNO_DS115j_GPP_OUT_ENA_MID,
	.gppOutEnValHigh		= SYNO_DS115j_GPP_OUT_ENA_HIGH,
	.gppOutValLow			= SYNO_DS115j_GPP_OUT_VAL_LOW,
	.gppOutValMid			= SYNO_DS115j_GPP_OUT_VAL_MID,
	.gppOutValHigh			= SYNO_DS115j_GPP_OUT_VAL_HIGH,
	.gppPolarityValLow		= SYNO_DS115j_GPP_POL_LOW,
	.gppPolarityValMid		= SYNO_DS115j_GPP_POL_MID,
	.gppPolarityValHigh		= SYNO_DS115j_GPP_POL_HIGH,

	.pSwitchInfo = NULL,
	.switchInfoNum = 0,

	.numBoardTdmInfo		= {1},
	.pBoardTdmInt2CsInfo		= {synods115jTdm880},
	.boardTdmInfoIndex		= 0,
	.pBoardTdmSpiInfo 		= synods115jTdmSpiInfo,

	.norFlashReadParams		= 0,
	.norFlashWriteParams	= 0
};

MV_BOARD_MAC_INFO synods216seInfoBoardMacInfo[] = {
	 
	{BOARD_MAC_SPEED_AUTO, 0x1, 0, 0},
};

MV_BOARD_MODULE_TYPE_INFO synods216seInfoBoardModTypeInfo[] = {
	{
		.boardMppGrp1Mod	= MV_BOARD_RGMII0,
		.boardMppGrp2Mod	= MV_BOARD_AUTO
	}
};

MV_DEV_CS_INFO synods216seInfoBoardDeCsInfo[] = {
	 
#if defined(MV_INCLUDE_SPI)
	{SPI_CS0, N_A, BOARD_DEV_SPI_FLASH, 8, 8},  
#endif
#if defined(MV_INCLUDE_NOR)
	{DEV_BOOCS, N_A, BOARD_DEV_NOR_FLASH, 16, 16}  
#endif
};

MV_BOARD_MPP_INFO synods216seInfoBoardMppConfigValue[] = {
	{ {
		SYNO_DS216se_MPP0_7,
		SYNO_DS216se_MPP8_15,
		SYNO_DS216se_MPP16_23,
		SYNO_DS216se_MPP24_31,
		SYNO_DS216se_MPP32_39,
		SYNO_DS216se_MPP40_47,
		SYNO_DS216se_MPP48_55,
		SYNO_DS216se_MPP56_63,
		SYNO_DS216se_MPP64_67,
	} }
};

MV_BOARD_TDM_INFO	synods216seTdm880[]	= { {0} };

MV_BOARD_TDM_SPI_INFO synods216seTdmSpiInfo[] = { {1} };

MV_BOARD_INFO synods216seInfo = {
	.boardName				= "SYNO-DS216se-BP",
	.enableModuleScan 			= MV_FALSE,
	.numBoardMppTypeValue		= ARRSZ(synods216seInfoBoardModTypeInfo),
	.pBoardModTypeValue			= synods216seInfoBoardModTypeInfo,
	.numBoardMppConfigValue		= ARRSZ(synods216seInfoBoardMppConfigValue),
	.pBoardMppConfigValue		= synods216seInfoBoardMppConfigValue,
	.intsGppMaskLow				= 0,
	.intsGppMaskMid				= 0,
	.intsGppMaskHigh			= 0,
	.numBoardDeviceIf			= ARRSZ(synods216seInfoBoardDeCsInfo),
	.pDevCsInfo					= synods216seInfoBoardDeCsInfo,
	.numBoardTwsiDev			= 0,
	.pBoardTwsiDev				= NULL,
	.numBoardMacInfo			= ARRSZ(synods216seInfoBoardMacInfo),
	.pBoardMacInfo				= synods216seInfoBoardMacInfo,
	.numBoardGppInfo			= 0,
	.pBoardGppInfo				= NULL,
	.activeLedsNumber			= 0,
	.pLedGppPin					= NULL,
	.ledsPolarity				= 0,

	.pmuPwrUpPolarity			= 0,
	.pmuPwrUpDelay				= 16000,

	.gppOutEnValLow			= SYNO_DS216se_GPP_OUT_ENA_LOW,
	.gppOutEnValMid			= SYNO_DS216se_GPP_OUT_ENA_MID,
	.gppOutEnValHigh		= SYNO_DS216se_GPP_OUT_ENA_HIGH,
	.gppOutValLow			= SYNO_DS216se_GPP_OUT_VAL_LOW,
	.gppOutValMid			= SYNO_DS216se_GPP_OUT_VAL_MID,
	.gppOutValHigh			= SYNO_DS216se_GPP_OUT_VAL_HIGH,
	.gppPolarityValLow		= SYNO_DS216se_GPP_POL_LOW,
	.gppPolarityValMid		= SYNO_DS216se_GPP_POL_MID,
	.gppPolarityValHigh		= SYNO_DS216se_GPP_POL_HIGH,

	.pSwitchInfo = NULL,
	.switchInfoNum = 0,

	.numBoardTdmInfo		= {1},
	.pBoardTdmInt2CsInfo		= {synods216seTdm880},
	.boardTdmInfoIndex		= 0,
	.pBoardTdmSpiInfo 		= synods216seTdmSpiInfo,

	.norFlashReadParams		= 0,
	.norFlashWriteParams	= 0
};
#endif  

MV_BOARD_INFO *boardInfoTbl[] = {
	&db88f6710Info,
	&db88f6710pcacInfo,
	&rd88F6710Info
#if defined(CONFIG_SYNO_ARMADA_ARCH)
	,NULL  
	,NULL
	,NULL
	,NULL
	,NULL
	,NULL
	,NULL
	,NULL
	,NULL
	,NULL
	,NULL
	,NULL
	,NULL  
	,&synods213jInfo
	,&synous3Info
	,&synors214Info
	,&synods214seInfo
	,&synods414slimInfo
	,&synods115jInfo
	,&synods216seInfo
#endif
};
