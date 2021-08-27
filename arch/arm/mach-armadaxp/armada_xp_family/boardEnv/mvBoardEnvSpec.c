#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include "mvCommon.h"
#include "mvBoardEnvLib.h"
#include "mvBoardEnvSpec.h"
#include "twsi/mvTwsi.h"
#include "pex/mvPexRegs.h"

#define ARRSZ(x)	(sizeof(x)/sizeof(x[0]))

#define DB_88F78XX0_BOARD_NAND_READ_PARAMS	0x000C0282
#define DB_88F78XX0_BOARD_NAND_WRITE_PARAMS	0x00010305
 
#define DB_88F78XX0_BOARD_NAND_CONTROL		0x01c00543

#define DB_88F78XX0_BOARD_NOR_READ_PARAMS	0x403E07CF
#define DB_88F78XX0_BOARD_NOR_WRITE_PARAMS	0x000F0F0F

MV_U8 mvDbDisableModuleDetection = 0;

MV_U8	db88f6781InfoBoardDebugLedIf[] = {26, 27, 48};

MV_BOARD_TWSI_INFO	db88f78XX0InfoBoardTwsiDev[] = {
	 
	{BOARD_DEV_TWSI_SATR, 0x4C, ADDR7_BIT},
	{BOARD_DEV_TWSI_SATR, 0x4D, ADDR7_BIT},
	{BOARD_DEV_TWSI_SATR, 0x4E, ADDR7_BIT},
	{BOARD_DEV_TWSI_SATR, 0x4F, ADDR7_BIT}
};

MV_BOARD_MAC_INFO db88f78XX0InfoBoardMacInfo[] = {
	 
	{BOARD_MAC_SPEED_AUTO, 0x0,0x0, 0x0,},
	{BOARD_MAC_SPEED_AUTO, 0x1,0x0, 0x1,},
	{BOARD_MAC_SPEED_AUTO, 0x19,0x800 , 0x18},   
	{BOARD_MAC_SPEED_AUTO, 0x1B,0x1800, 0x18}   
};

MV_BOARD_MODULE_TYPE_INFO db88f78XX0InfoBoardModTypeInfo[] = {
	{
		.boardMppMod		= MV_BOARD_AUTO,
		.boardOtherMod		= MV_BOARD_NONE
	}
};

MV_BOARD_GPP_INFO db88f78XX0InfoBoardGppInfo[] = {
	 
	{BOARD_GPP_USB_VBUS,    24},  
	{BOARD_GPP_RESET,       47},
};

MV_DEV_CS_INFO db88f78XX0InfoBoardDeCsInfo[] = {
	 
#if defined(MV_INCLUDE_SPI)
	{SPI_CS0, N_A, BOARD_DEV_SPI_FLASH, 8, 8},  
#endif
#if defined(MV_INCLUDE_NOR)
	{DEV_BOOCS, N_A, BOARD_DEV_NOR_FLASH, 16, 16}  
#endif
};

MV_BOARD_MPP_INFO db88f78XX0InfoBoardMppConfigValue[] = {
	{ {
	DB_88F78XX0_MPP0_7,
	DB_88F78XX0_MPP8_15,
	DB_88F78XX0_MPP16_23,
	DB_88F78XX0_MPP24_31,
	DB_88F78XX0_MPP32_39,
	DB_88F78XX0_MPP40_47,
	DB_88F78XX0_MPP48_55,
	DB_88F78XX0_MPP56_63,
	DB_88F78XX0_MPP64_67,
	} },
	{ {  
		DB_88F78XX0_MPP0_7,
		DB_88F78XX0_MPP8_15,
		DB_88F78XX0_MPP16_23,
		(DB_88F78XX0_MPP24_31 & 0x00000000) | 0x33333333,
		(DB_88F78XX0_MPP32_39 & 0xFFFF0000) | 0x00003333,
		(DB_88F78XX0_MPP40_47 & 0xFFFFF0FF) | 0x00000300,
		DB_88F78XX0_MPP48_55,
		DB_88F78XX0_MPP56_63,
		DB_88F78XX0_MPP64_67,
	} },
	{ {  
		(DB_88F78XX0_MPP0_7   & 0x00000000) | 0x44444444,
		(DB_88F78XX0_MPP8_15  & 0x00000000) | 0x44444444,
		(DB_88F78XX0_MPP16_23 & 0x00000000) | 0x44444444,
		(DB_88F78XX0_MPP24_31 & 0xFFFF0000) | 0x00004444,
		DB_88F78XX0_MPP32_39,
		(DB_88F78XX0_MPP40_47 & 0xFFFFFF00) | 0x00000044,
		DB_88F78XX0_MPP48_55,
		DB_88F78XX0_MPP56_63,
		DB_88F78XX0_MPP64_67,
	} },
	{ {  
		(DB_88F78XX0_MPP0_7 & 0x00000000) | 0x11111111,
		(DB_88F78XX0_MPP8_15 & 0x00000000) | 0x11111111,
		(DB_88F78XX0_MPP16_23 & 0x000000FF) | 0x11111100,
		DB_88F78XX0_MPP24_31,
		DB_88F78XX0_MPP32_39,
		DB_88F78XX0_MPP40_47,
		DB_88F78XX0_MPP48_55,
		DB_88F78XX0_MPP56_63,
		DB_88F78XX0_MPP64_67,
	} },
	{ {  
		DB_88F78XX0_MPP0_7,
		DB_88F78XX0_MPP8_15,
		DB_88F78XX0_MPP16_23,
		DB_88F78XX0_MPP24_31,
		DB_88F78XX0_MPP32_39,
		DB_88F78XX0_MPP40_47,
		DB_88F78XX0_MPP48_55,
		DB_88F78XX0_MPP56_63,
		DB_88F78XX0_MPP64_67,
	} },
};

MV_BOARD_TDM_INFO	db88f78XX0Tdm880[]	= { {1}, {2} };
MV_BOARD_TDM_INFO	db88f78XX0Tdm792[]	= { {1}, {2}, {3}, {4}, {6}, {7} };
MV_BOARD_TDM_INFO	db88f78XX0Tdm3215[]	= { {1} };

MV_BOARD_INFO db88f78XX0Info = {
	.boardName			= "DB-78460-BP",
	.numBoardMppTypeValue		= ARRSZ(db88f78XX0InfoBoardModTypeInfo),
	.pBoardModTypeValue		= db88f78XX0InfoBoardModTypeInfo,
	.numBoardMppConfigValue		= ARRSZ(db88f78XX0InfoBoardMppConfigValue),
	.pBoardMppConfigValue		= db88f78XX0InfoBoardMppConfigValue,
	.intsGppMaskLow			= 0,
	.intsGppMaskMid			= 0,
	.intsGppMaskHigh		= 0,
	.numBoardDeviceIf		= ARRSZ(db88f78XX0InfoBoardDeCsInfo),
	.pDevCsInfo			= db88f78XX0InfoBoardDeCsInfo,
	.numBoardTwsiDev		= ARRSZ(db88f78XX0InfoBoardTwsiDev),
	.pBoardTwsiDev			= db88f78XX0InfoBoardTwsiDev,
	.numBoardMacInfo		= ARRSZ(db88f78XX0InfoBoardMacInfo),
	.pBoardMacInfo			= db88f78XX0InfoBoardMacInfo,
	.numBoardGppInfo		= ARRSZ(db88f78XX0InfoBoardGppInfo),
	.pBoardGppInfo			= db88f78XX0InfoBoardGppInfo,
	.activeLedsNumber		= ARRSZ(db88f6781InfoBoardDebugLedIf),
	.pLedGppPin			= db88f6781InfoBoardDebugLedIf,
	.ledsPolarity			= 0,

	.gppOutEnValLow			= DB_88F78XX0_GPP_OUT_ENA_LOW,
	.gppOutEnValMid			= DB_88F78XX0_GPP_OUT_ENA_MID,
	.gppOutEnValHigh		= DB_88F78XX0_GPP_OUT_ENA_HIGH,
	.gppOutValLow			= DB_88F78XX0_GPP_OUT_VAL_LOW,
	.gppOutValMid			= DB_88F78XX0_GPP_OUT_VAL_MID,
	.gppOutValHigh			= DB_88F78XX0_GPP_OUT_VAL_HIGH,
	.gppPolarityValLow		= DB_88F78XX0_GPP_POL_LOW,
	.gppPolarityValMid		= DB_88F78XX0_GPP_POL_MID,
	.gppPolarityValHigh		= DB_88F78XX0_GPP_POL_HIGH,

	.pSwitchInfo = NULL,
	.switchInfoNum = 0,

	.numBoardTdmInfo		= {2, 6, 1},
	.pBoardTdmInt2CsInfo		= {db88f78XX0Tdm880,
					   db88f78XX0Tdm792,
					   db88f78XX0Tdm3215},
	.boardTdmInfoIndex		= -1,

	.nandFlashReadParams		= DB_88F78XX0_BOARD_NAND_READ_PARAMS,
	.nandFlashWriteParams		= DB_88F78XX0_BOARD_NAND_WRITE_PARAMS,
	.nandFlashControl		= DB_88F78XX0_BOARD_NAND_CONTROL,
	 
	.norFlashReadParams		= DB_88F78XX0_BOARD_NOR_READ_PARAMS,
	.norFlashWriteParams		= DB_88F78XX0_BOARD_NOR_WRITE_PARAMS
};

#define DB_88F78XX0_REV2_BOARD_NAND_READ_PARAMS	0x000C0282
#define DB_88F78XX0_REV2_BOARD_NAND_WRITE_PARAMS	0x00010305
 
#define DB_88F78XX0_REV2_BOARD_NAND_CONTROL		0x01c00543

#define DB_88F78XX0_REV2_BOARD_NOR_READ_PARAMS	0x403E07CF
#define DB_88F78XX0_REV2_BOARD_NOR_WRITE_PARAMS	0x000F0F0F

MV_U8 mvDbDisableModuleDetection_rev2 = 0;

MV_U8	db88f6781InfoBoardDebugLedIf_rev2[] = {26, 27, 48};

MV_BOARD_TWSI_INFO	db88f78XX0rev2InfoBoardTwsiDev[] = {
	 
	{BOARD_DEV_TWSI_SATR, 0x4C, ADDR7_BIT},
	{BOARD_DEV_TWSI_SATR, 0x4D, ADDR7_BIT},
	{BOARD_DEV_TWSI_SATR, 0x4E, ADDR7_BIT},
	{BOARD_DEV_TWSI_SATR, 0x4F, ADDR7_BIT}
};

MV_BOARD_MAC_INFO db88f78XX0rev2InfoBoardMacInfo[] = {
	 
	{BOARD_MAC_SPEED_AUTO, 0x0,0x0 	  , 0x0 },
	{BOARD_MAC_SPEED_AUTO, 0x1,0x0	  , 0x1 },
	{BOARD_MAC_SPEED_AUTO, 0x19,0x800 , 0x18},   
	{BOARD_MAC_SPEED_AUTO, 0x1B,0x1800, 0x18}   
};

MV_BOARD_MODULE_TYPE_INFO db88f78XX0rev2InfoBoardModTypeInfo[] = {
	{
		.boardMppMod		= MV_BOARD_AUTO,
		.boardOtherMod		= MV_BOARD_NONE
	}
};

MV_BOARD_GPP_INFO db88f78XX0rev2InfoBoardGppInfo[] = {
	 
	{BOARD_GPP_USB_VBUS,    24}  
	 
};

MV_DEV_CS_INFO db88f78XX0rev2InfoBoardDeCsInfo[] = {
	 
#if defined(MV_INCLUDE_SPI)
	{SPI_CS0, N_A, BOARD_DEV_SPI_FLASH, 8, 8},  
#endif
#if defined(MV_INCLUDE_NOR)
	{DEV_BOOCS, N_A, BOARD_DEV_NOR_FLASH, 16, 16}  
#endif
};

MV_BOARD_MPP_INFO db88f78XX0rev2InfoBoardMppConfigValue[] = {
	{ {
	DB_88F78XX0_REV2_MPP0_7,
	DB_88F78XX0_REV2_MPP8_15,
	DB_88F78XX0_REV2_MPP16_23,
	DB_88F78XX0_REV2_MPP24_31,
	DB_88F78XX0_REV2_MPP32_39,
	DB_88F78XX0_REV2_MPP40_47,
	DB_88F78XX0_REV2_MPP48_55,
	DB_88F78XX0_REV2_MPP56_63,
	DB_88F78XX0_REV2_MPP64_67,
	} },
	{ {  
		DB_88F78XX0_REV2_MPP0_7,
		DB_88F78XX0_REV2_MPP8_15,
		DB_88F78XX0_REV2_MPP16_23,
		(DB_88F78XX0_REV2_MPP24_31 & 0x00000000) | 0x33333333,
		(DB_88F78XX0_REV2_MPP32_39 & 0xFFFF0000) | 0x00003333,
		(DB_88F78XX0_REV2_MPP40_47 & 0xFFFFF0FF) | 0x00000300,
		DB_88F78XX0_REV2_MPP48_55,
		DB_88F78XX0_REV2_MPP56_63,
		DB_88F78XX0_REV2_MPP64_67,
	} },
	{ {  
		(DB_88F78XX0_REV2_MPP0_7   & 0x00000000) | 0x44444444,
		(DB_88F78XX0_REV2_MPP8_15  & 0x00000000) | 0x44444444,
		(DB_88F78XX0_REV2_MPP16_23 & 0x00000000) | 0x44444444,
		(DB_88F78XX0_REV2_MPP24_31 & 0xFFFF0000) | 0x00004444,
		DB_88F78XX0_REV2_MPP32_39,
		(DB_88F78XX0_REV2_MPP40_47 & 0xFFFFFF00) | 0x00000044,
		DB_88F78XX0_REV2_MPP48_55,
		DB_88F78XX0_REV2_MPP56_63,
		DB_88F78XX0_REV2_MPP64_67,
	} },
	{ {  
		(DB_88F78XX0_REV2_MPP0_7 & 0x00000000) | 0x11111111,
		(DB_88F78XX0_REV2_MPP8_15 & 0x00000000) | 0x11111111,
		(DB_88F78XX0_REV2_MPP16_23 & 0x000000FF) | 0x11111100,
		DB_88F78XX0_REV2_MPP24_31,
		DB_88F78XX0_REV2_MPP32_39,
		DB_88F78XX0_REV2_MPP40_47,
		DB_88F78XX0_REV2_MPP48_55,
		DB_88F78XX0_REV2_MPP56_63,
		DB_88F78XX0_REV2_MPP64_67,
	} },
	{ {  
		DB_88F78XX0_REV2_MPP0_7,
		DB_88F78XX0_REV2_MPP8_15,
		DB_88F78XX0_REV2_MPP16_23,
		DB_88F78XX0_REV2_MPP24_31,
		DB_88F78XX0_REV2_MPP32_39,
		DB_88F78XX0_REV2_MPP40_47,
		DB_88F78XX0_REV2_MPP48_55,
		DB_88F78XX0_REV2_MPP56_63,
		DB_88F78XX0_REV2_MPP64_67,
	} },
};

MV_BOARD_TDM_INFO	db88f78XX0rev2Tdm880[]	= { {1}, {2} };
MV_BOARD_TDM_INFO	db88f78XX0rev2Tdm792[]	= { {1}, {2}, {3}, {4}, {6}, {7} };
MV_BOARD_TDM_INFO	db88f78XX0rev2Tdm3215[]	= { {1} };

MV_BOARD_INFO db88f78XX0rev2Info = {
	.boardName			= "DB-78460-BP rev 2.0",
	.numBoardMppTypeValue		= ARRSZ(db88f78XX0rev2InfoBoardModTypeInfo),
	.pBoardModTypeValue		= db88f78XX0rev2InfoBoardModTypeInfo,
	.numBoardMppConfigValue		= ARRSZ(db88f78XX0rev2InfoBoardMppConfigValue),
	.pBoardMppConfigValue		= db88f78XX0rev2InfoBoardMppConfigValue,
	.intsGppMaskLow			= 0,
	.intsGppMaskMid			= 0,
	.intsGppMaskHigh		= 0,
	.numBoardDeviceIf		= ARRSZ(db88f78XX0rev2InfoBoardDeCsInfo),
	.pDevCsInfo			= db88f78XX0rev2InfoBoardDeCsInfo,
	.numBoardTwsiDev		= ARRSZ(db88f78XX0rev2InfoBoardTwsiDev),
	.pBoardTwsiDev			= db88f78XX0rev2InfoBoardTwsiDev,
	.numBoardMacInfo		= ARRSZ(db88f78XX0rev2InfoBoardMacInfo),
	.pBoardMacInfo			= db88f78XX0rev2InfoBoardMacInfo,
	.numBoardGppInfo		= ARRSZ(db88f78XX0rev2InfoBoardGppInfo),
	.pBoardGppInfo			= db88f78XX0rev2InfoBoardGppInfo,
	.activeLedsNumber		= ARRSZ(db88f6781InfoBoardDebugLedIf),
	.pLedGppPin			= db88f6781InfoBoardDebugLedIf,
	.ledsPolarity			= 0,

	.gppOutEnValLow			= DB_88F78XX0_REV2_GPP_OUT_ENA_LOW,
	.gppOutEnValMid			= DB_88F78XX0_REV2_GPP_OUT_ENA_MID,
	.gppOutEnValHigh		= DB_88F78XX0_REV2_GPP_OUT_ENA_HIGH,
	.gppOutValLow			= DB_88F78XX0_REV2_GPP_OUT_VAL_LOW,
	.gppOutValMid			= DB_88F78XX0_REV2_GPP_OUT_VAL_MID,
	.gppOutValHigh			= DB_88F78XX0_REV2_GPP_OUT_VAL_HIGH,
	.gppPolarityValLow		= DB_88F78XX0_REV2_GPP_POL_LOW,
	.gppPolarityValMid		= DB_88F78XX0_REV2_GPP_POL_MID,
	.gppPolarityValHigh		= DB_88F78XX0_REV2_GPP_POL_HIGH,

	.pSwitchInfo = NULL,
	.switchInfoNum = 0,

	.numBoardTdmInfo		= {2, 6, 1},
	.pBoardTdmInt2CsInfo		= {db88f78XX0rev2Tdm880,
					   db88f78XX0rev2Tdm792,
					   db88f78XX0rev2Tdm3215},
	.boardTdmInfoIndex		= -1,

	.nandFlashReadParams		= DB_88F78XX0_REV2_BOARD_NAND_READ_PARAMS,
	.nandFlashWriteParams		= DB_88F78XX0_REV2_BOARD_NAND_WRITE_PARAMS,
	.nandFlashControl		= DB_88F78XX0_REV2_BOARD_NAND_CONTROL,
	 
	.norFlashReadParams		= DB_88F78XX0_REV2_BOARD_NOR_READ_PARAMS,
	.norFlashWriteParams		= DB_88F78XX0_REV2_BOARD_NOR_WRITE_PARAMS
};

#define RD_78460_NAS_BOARD_NAND_READ_PARAMS	0x000C0282
#define RD_78460_NAS_BOARD_NAND_WRITE_PARAMS	0x00010305
 
#define RD_78460_NAS_BOARD_NAND_CONTROL		0x01c00543

#define RD_78460_NAS_BOARD_NOR_READ_PARAMS	0x403E07CF
#define RD_78460_NAS_BOARD_NOR_WRITE_PARAMS	0x000F0F0F

MV_BOARD_MAC_INFO rd78460nasInfoBoardMacInfo[] = {
	 
	{BOARD_MAC_SPEED_AUTO, 0x10,0x0, 0x10},
	{BOARD_MAC_SPEED_AUTO, 0x11,0x0, 0x10},
	{BOARD_MAC_SPEED_AUTO, 0x12,0x0, 0x10},
	{BOARD_MAC_SPEED_AUTO, 0x13,0x0, 0x10}
};

MV_BOARD_MODULE_TYPE_INFO rd78460nasInfoBoardModTypeInfo[] = {
	{
		.boardMppMod		= MV_BOARD_AUTO,
		.boardOtherMod		= MV_BOARD_NONE
	}
};

MV_BOARD_GPP_INFO rd78460nasInfoBoardGppInfo[] = {
	 
	{BOARD_GPP_RESET, 21}
};

MV_DEV_CS_INFO rd78460nasInfoBoardDeCsInfo[] = {
	 
#if defined(MV_INCLUDE_SPI)
	{SPI_CS0, N_A, BOARD_DEV_SPI_FLASH, 8, 8}  
#endif
};

MV_BOARD_MPP_INFO rd78460nasInfoBoardMppConfigValue[] = {
	{ {
	RD_78460_NAS_MPP0_7,
	RD_78460_NAS_MPP8_15,
	RD_78460_NAS_MPP16_23,
	RD_78460_NAS_MPP24_31,
	RD_78460_NAS_MPP32_39,
	RD_78460_NAS_MPP40_47,
	RD_78460_NAS_MPP48_55,
	RD_78460_NAS_MPP56_63,
	RD_78460_NAS_MPP64_67,
	} }
};

MV_BOARD_INFO rd78460nasInfo = {
	.boardName			= "RD-AXP-NAS rev 1.0",
	.numBoardMppTypeValue		= ARRSZ(rd78460nasInfoBoardModTypeInfo),
	.pBoardModTypeValue		= rd78460nasInfoBoardModTypeInfo,
	.numBoardMppConfigValue		= ARRSZ(rd78460nasInfoBoardMppConfigValue),
	.pBoardMppConfigValue		= rd78460nasInfoBoardMppConfigValue,
	.intsGppMaskLow			= 0,
	.intsGppMaskMid			= 0,
	.intsGppMaskHigh		= 0,
	.numBoardDeviceIf		= ARRSZ(rd78460nasInfoBoardDeCsInfo),
	.pDevCsInfo			= rd78460nasInfoBoardDeCsInfo,
	.numBoardTwsiDev		= 0,
	.pBoardTwsiDev			= NULL,
	.numBoardMacInfo		= ARRSZ(rd78460nasInfoBoardMacInfo),
	.pBoardMacInfo			= rd78460nasInfoBoardMacInfo,
	.numBoardGppInfo		= ARRSZ(rd78460nasInfoBoardGppInfo),
	.pBoardGppInfo			= rd78460nasInfoBoardGppInfo,
	.activeLedsNumber		= 0,
	.pLedGppPin			= NULL,
	.ledsPolarity			= 0,

	.gppOutEnValLow			= RD_78460_NAS_GPP_OUT_ENA_LOW,
	.gppOutEnValMid			= RD_78460_NAS_GPP_OUT_ENA_MID,
	.gppOutEnValHigh		= RD_78460_NAS_GPP_OUT_ENA_HIGH,
	.gppOutValLow			= RD_78460_NAS_GPP_OUT_VAL_LOW,
	.gppOutValMid			= RD_78460_NAS_GPP_OUT_VAL_MID,
	.gppOutValHigh			= RD_78460_NAS_GPP_OUT_VAL_HIGH,
	.gppPolarityValLow		= RD_78460_NAS_GPP_POL_LOW,
	.gppPolarityValMid		= RD_78460_NAS_GPP_POL_MID,
	.gppPolarityValHigh		= RD_78460_NAS_GPP_POL_HIGH,

	.pSwitchInfo = NULL,
	.switchInfoNum = 0,

	.numBoardTdmInfo			= {},
	.pBoardTdmInt2CsInfo			= {},
	.boardTdmInfoIndex			= -1,

	.nandFlashReadParams		= RD_78460_NAS_BOARD_NAND_READ_PARAMS,
	.nandFlashWriteParams		= RD_78460_NAS_BOARD_NAND_WRITE_PARAMS,
	.nandFlashControl		= RD_78460_NAS_BOARD_NAND_CONTROL,
	 
	.norFlashReadParams		= RD_78460_NAS_BOARD_NOR_READ_PARAMS,
	.norFlashWriteParams		= RD_78460_NAS_BOARD_NOR_WRITE_PARAMS
};

#define RD_78460_BOARD_NAND_READ_PARAMS		0x000C0282
#define RD_78460_BOARD_NAND_WRITE_PARAMS	0x00010305
 
#define RD_78460_BOARD_NAND_CONTROL			0x01c00543

MV_BOARD_MAC_INFO rd78460InfoBoardMacInfo[] = {
	 
	{BOARD_MAC_SPEED_1000M, 0x1, 0x0, 0x1},
	{BOARD_MAC_SPEED_1000M, 0x2, 0x0, 0x2},
	{BOARD_MAC_SPEED_AUTO,  0x0, 0x0, 0x0},
	{BOARD_MAC_SPEED_1000M, 0x1B,0x0,0x18}
};

MV_BOARD_MODULE_TYPE_INFO rd78460InfoBoardModTypeInfo[] = {
	{
		.boardMppMod		= MV_BOARD_AUTO,
		.boardOtherMod		= MV_BOARD_NONE
	}
};

MV_DEV_CS_INFO rd78460InfoBoardDeCsInfo[] = {
	 
#if defined(MV_INCLUDE_SPI)
	{SPI_CS0, N_A, BOARD_DEV_SPI_FLASH, 8}  
#endif
};

MV_BOARD_MPP_INFO rd78460InfoBoardMppConfigValue[] = {
	{ {
		RD_78460_MPP0_7,
		RD_78460_MPP8_15,
		RD_78460_MPP16_23,
		RD_78460_MPP24_31,
		RD_78460_MPP32_39,
		RD_78460_MPP40_47,
		RD_78460_MPP48_55,
		RD_78460_MPP56_63,
		RD_78460_MPP64_67,
	} }
};

MV_BOARD_INFO rd78460Info = {
 .boardName				= "RD-78460-SERVER",
 .numBoardMppTypeValue			= ARRSZ(rd78460InfoBoardModTypeInfo),
 .pBoardModTypeValue			= rd78460InfoBoardModTypeInfo,
 .numBoardMppConfigValue		= ARRSZ(rd78460InfoBoardMppConfigValue),
 .pBoardMppConfigValue			= rd78460InfoBoardMppConfigValue,
 .intsGppMaskLow			= 0,
 .intsGppMaskMid			= 0,
 .intsGppMaskHigh			= 0,
 .numBoardDeviceIf			= ARRSZ(rd78460InfoBoardDeCsInfo),
 .pDevCsInfo				= rd78460InfoBoardDeCsInfo,
 .numBoardTwsiDev			= 0,
 .pBoardTwsiDev				= NULL,
 .numBoardMacInfo			= ARRSZ(rd78460InfoBoardMacInfo),
 .pBoardMacInfo				= rd78460InfoBoardMacInfo,
 .numBoardGppInfo			= 0,
 .pBoardGppInfo				= NULL,
 .activeLedsNumber			= 0,
 .pLedGppPin				= NULL,
 .ledsPolarity				= 0,

 .gppOutEnValLow			= RD_78460_GPP_OUT_ENA_LOW,
 .gppOutEnValMid			= RD_78460_GPP_OUT_ENA_MID,
 .gppOutEnValHigh			= RD_78460_GPP_OUT_ENA_HIGH,
 .gppOutValLow				= RD_78460_GPP_OUT_VAL_LOW,
 .gppOutValMid				= RD_78460_GPP_OUT_VAL_MID,
 .gppOutValHigh				= RD_78460_GPP_OUT_VAL_HIGH,
 .gppPolarityValLow			= RD_78460_GPP_POL_LOW,
 .gppPolarityValMid			= RD_78460_GPP_POL_MID,
 .gppPolarityValHigh			= RD_78460_GPP_POL_HIGH,

	.pSwitchInfo = NULL,
	.switchInfoNum = 0,

 .numBoardTdmInfo			= {},
 .pBoardTdmInt2CsInfo			= {},
 .boardTdmInfoIndex			= -1,

 .nandFlashReadParams			= RD_78460_BOARD_NAND_READ_PARAMS,
 .nandFlashWriteParams			= RD_78460_BOARD_NAND_WRITE_PARAMS,
 .nandFlashControl			= RD_78460_BOARD_NAND_CONTROL
};

#define RD_78460_SERVER_REV2_BOARD_NAND_READ_PARAMS		0x000C0282
#define RD_78460_SERVER_REV2_BOARD_NAND_WRITE_PARAMS		0x00010305
 
#define RD_78460_SERVER_REV2_BOARD_NAND_CONTROL			0x01c00543

MV_BOARD_MAC_INFO rd78460ServerRev2InfoBoardMacInfo[] = {
	 
	{BOARD_MAC_SPEED_1000M, 0x1,0x0, 0x1},
	{BOARD_MAC_SPEED_1000M, 0x2,0x0, 0x1},
	{BOARD_MAC_SPEED_AUTO,  0x0,0x0, 0x1},
	{BOARD_MAC_SPEED_1000M, 0x1B,0x0,0x18}
};

MV_BOARD_MODULE_TYPE_INFO rd78460ServerRev2InfoBoardModTypeInfo[] = {
	{
		.boardMppMod		= MV_BOARD_AUTO,
		.boardOtherMod		= MV_BOARD_NONE
	}
};

MV_DEV_CS_INFO rd78460ServerRev2InfoBoardDeCsInfo[] = {
	 
#if defined(MV_INCLUDE_SPI)
	{SPI_CS0, N_A, BOARD_DEV_SPI_FLASH, 8}  
#endif
};

MV_BOARD_MPP_INFO rd78460ServerRev2InfoBoardMppConfigValue[] = {
	{ {
		RD_78460_SERVER_REV2_MPP0_7,
		RD_78460_SERVER_REV2_MPP8_15,
		RD_78460_SERVER_REV2_MPP16_23,
		RD_78460_SERVER_REV2_MPP24_31,
		RD_78460_SERVER_REV2_MPP32_39,
		RD_78460_SERVER_REV2_MPP40_47,
		RD_78460_SERVER_REV2_MPP48_55,
		RD_78460_SERVER_REV2_MPP56_63,
		RD_78460_SERVER_REV2_MPP64_67,
	} }
};

MV_BOARD_INFO rd78460ServerRev2Info = {
 .boardName				= "RD-78460-SERVER-REV2",
 .numBoardMppTypeValue			= ARRSZ(rd78460ServerRev2InfoBoardModTypeInfo),
 .pBoardModTypeValue			= rd78460ServerRev2InfoBoardModTypeInfo,
 .numBoardMppConfigValue		= ARRSZ(rd78460ServerRev2InfoBoardMppConfigValue),
 .pBoardMppConfigValue			= rd78460ServerRev2InfoBoardMppConfigValue,
 .intsGppMaskLow			= 0,
 .intsGppMaskMid			= 0,
 .intsGppMaskHigh			= 0,
 .numBoardDeviceIf			= ARRSZ(rd78460ServerRev2InfoBoardDeCsInfo),
 .pDevCsInfo				= rd78460ServerRev2InfoBoardDeCsInfo,
 .numBoardTwsiDev			= 0,
 .pBoardTwsiDev				= NULL,
 .numBoardMacInfo			= ARRSZ(rd78460ServerRev2InfoBoardMacInfo),
 .pBoardMacInfo				= rd78460ServerRev2InfoBoardMacInfo,
 .numBoardGppInfo			= 0,
 .pBoardGppInfo				= NULL,
 .activeLedsNumber			= 0,
 .pLedGppPin				= NULL,
 .ledsPolarity				= 0,

 .gppOutEnValLow			= RD_78460_SERVER_REV2_GPP_OUT_ENA_LOW,
 .gppOutEnValMid			= RD_78460_SERVER_REV2_GPP_OUT_ENA_MID,
 .gppOutEnValHigh			= RD_78460_SERVER_REV2_GPP_OUT_ENA_HIGH,
 .gppOutValLow				= RD_78460_SERVER_REV2_GPP_OUT_VAL_LOW,
 .gppOutValMid				= RD_78460_SERVER_REV2_GPP_OUT_VAL_MID,
 .gppOutValHigh				= RD_78460_SERVER_REV2_GPP_OUT_VAL_HIGH,
 .gppPolarityValLow			= RD_78460_SERVER_REV2_GPP_POL_LOW,
 .gppPolarityValMid			= RD_78460_SERVER_REV2_GPP_POL_MID,
 .gppPolarityValHigh			= RD_78460_SERVER_REV2_GPP_POL_HIGH,

	.pSwitchInfo = NULL,
	.switchInfoNum = 0,

 .numBoardTdmInfo			= {},
 .pBoardTdmInt2CsInfo			= {},
 .boardTdmInfoIndex			= -1,

 .nandFlashReadParams			= RD_78460_SERVER_REV2_BOARD_NAND_READ_PARAMS,
 .nandFlashWriteParams			= RD_78460_SERVER_REV2_BOARD_NAND_WRITE_PARAMS,
 .nandFlashControl			= RD_78460_SERVER_REV2_BOARD_NAND_CONTROL
};
 
#define DB_78X60_PCAC_BOARD_NAND_READ_PARAMS		0x000C0282
#define DB_78X60_PCAC_BOARD_NAND_WRITE_PARAMS		0x00010305
 
#define DB_78X60_PCAC_BOARD_NAND_CONTROL			0x01c00543

MV_U8	db78X60pcacInfoBoardDebugLedIf[] = {53, 54, 55, 56};

MV_BOARD_TWSI_INFO	db78X60pcacInfoBoardTwsiDev[] = {
	 
	{BOARD_DEV_TWSI_SATR, 0x4C, ADDR7_BIT},
	{BOARD_DEV_TWSI_SATR, 0x4D, ADDR7_BIT},
	{BOARD_DEV_TWSI_SATR, 0x4E, ADDR7_BIT},
	{BOARD_DEV_TWSI_SATR, 0x4F, ADDR7_BIT}
};

MV_BOARD_MAC_INFO db78X60pcacInfoBoardMacInfo[] = {
	 
	{BOARD_MAC_SPEED_AUTO, 0x1,0x0,0x0},
	{BOARD_MAC_SPEED_AUTO, 0x3,0x0,0x0},
	{BOARD_MAC_SPEED_AUTO, 0x2,0x0,0x0},
	{BOARD_MAC_SPEED_AUTO, 0x0,0x0,0x0}		 
};

MV_BOARD_MODULE_TYPE_INFO db78X60pcacInfoBoardModTypeInfo[] = {
	{
		.boardMppMod	= MV_BOARD_AUTO,
		.boardOtherMod	= MV_BOARD_NONE
	}
};

MV_BOARD_GPP_INFO db78X60pcacInfoBoardGppInfo[] = {
	 
	{BOARD_GPP_USB_VBUS,    23}  
};

MV_DEV_CS_INFO db78X60pcacInfoBoardDeCsInfo[] = {
	 
#if defined(MV_INCLUDE_SPI)
	{SPI_CS0, N_A, BOARD_DEV_SPI_FLASH, 8}  
#endif
};

MV_BOARD_MPP_INFO db78X60pcacInfoBoardMppConfigValue[] = {
	{ {
		DB_78X60_PCAC_MPP0_7,
		DB_78X60_PCAC_MPP8_15,
		DB_78X60_PCAC_MPP16_23,
		DB_78X60_PCAC_MPP24_31,
		DB_78X60_PCAC_MPP32_39,
		DB_78X60_PCAC_MPP40_47,
		DB_78X60_PCAC_MPP48_55,
		DB_78X60_PCAC_MPP56_63,
		DB_78X60_PCAC_MPP64_67,
	} }
};

MV_BOARD_TDM_INFO	db78X60pcacTdm880[]		= { {1}, {2} };
MV_BOARD_TDM_INFO	db78X60pcacTdm792[]		= { {1}, {2}, {3}, {4}, {6}, {7} };
MV_BOARD_TDM_INFO	db78X60pcacTdm3215[]	= { {1} };

MV_BOARD_INFO db78X60pcacInfo = {
	.boardName			= "DB-78460-PCAC",
	.numBoardMppTypeValue		= ARRSZ(db78X60pcacInfoBoardModTypeInfo),
	.pBoardModTypeValue		= db78X60pcacInfoBoardModTypeInfo,
	.numBoardMppConfigValue		= ARRSZ(db78X60pcacInfoBoardMppConfigValue),
	.pBoardMppConfigValue		= db78X60pcacInfoBoardMppConfigValue,
	.intsGppMaskLow			= 0,
	.intsGppMaskMid			= 0,
	.intsGppMaskHigh		= 0,
	.numBoardDeviceIf		= ARRSZ(db78X60pcacInfoBoardDeCsInfo),
	.pDevCsInfo			= db78X60pcacInfoBoardDeCsInfo,
	.numBoardTwsiDev		= ARRSZ(db78X60pcacInfoBoardTwsiDev),
	.pBoardTwsiDev			= db78X60pcacInfoBoardTwsiDev,
	.numBoardMacInfo		= ARRSZ(db78X60pcacInfoBoardMacInfo),
	.pBoardMacInfo			= db78X60pcacInfoBoardMacInfo,
	.numBoardGppInfo		= ARRSZ(db78X60pcacInfoBoardGppInfo),
	.pBoardGppInfo			= db78X60pcacInfoBoardGppInfo,
	.activeLedsNumber		= ARRSZ(db78X60pcacInfoBoardDebugLedIf),
	.pLedGppPin			= db78X60pcacInfoBoardDebugLedIf,
	.ledsPolarity			= 0,

	.gppOutEnValLow			= DB_78X60_PCAC_GPP_OUT_ENA_LOW,
	.gppOutEnValMid			= DB_78X60_PCAC_GPP_OUT_ENA_MID,
	.gppOutEnValHigh		= DB_78X60_PCAC_GPP_OUT_ENA_HIGH,
	.gppOutValLow			= DB_78X60_PCAC_GPP_OUT_VAL_LOW,
	.gppOutValMid			= DB_78X60_PCAC_GPP_OUT_VAL_MID,
	.gppOutValHigh			= DB_78X60_PCAC_GPP_OUT_VAL_HIGH,
	.gppPolarityValLow		= DB_78X60_PCAC_GPP_POL_LOW,
	.gppPolarityValMid		= DB_78X60_PCAC_GPP_POL_MID,
	.gppPolarityValHigh		= DB_78X60_PCAC_GPP_POL_HIGH,

	.pSwitchInfo = NULL,
	.switchInfoNum = 0,

	.numBoardTdmInfo		= {2, 6, 1},
	.pBoardTdmInt2CsInfo		= {db78X60pcacTdm880,
					   db78X60pcacTdm792,
					   db78X60pcacTdm3215},
	.boardTdmInfoIndex		= -1,

	.nandFlashReadParams		= DB_78X60_PCAC_BOARD_NAND_READ_PARAMS,
	.nandFlashWriteParams		= DB_78X60_PCAC_BOARD_NAND_WRITE_PARAMS,
	.nandFlashControl		= DB_78X60_PCAC_BOARD_NAND_CONTROL
};

#define DB_78X60_PCAC_REV2_BOARD_NAND_READ_PARAMS		0x000C0282
#define DB_78X60_PCAC_REV2_BOARD_NAND_WRITE_PARAMS		0x00010305
 
#define DB_78X60_PCAC_REV2_BOARD_NAND_CONTROL			0x01c00543

MV_U8	db78X60pcacrev2InfoBoardDebugLedIf[] = {53, 54, 55, 56};

MV_BOARD_MAC_INFO db78X60pcacrev2InfoBoardMacInfo[] = {
	 
	{BOARD_MAC_SPEED_AUTO, 0x1,0x0,0x0},
	{BOARD_MAC_SPEED_AUTO, 0x3,0x0,0x0},
	{BOARD_MAC_SPEED_AUTO, 0x2,0x0,0x0},
	{BOARD_MAC_SPEED_AUTO, 0x0,0x0,0x0}		 
};

MV_BOARD_MODULE_TYPE_INFO db78X60pcacrev2InfoBoardModTypeInfo[] = {
	{
		.boardMppMod	= MV_BOARD_AUTO,
		.boardOtherMod	= MV_BOARD_NONE
	}
};

MV_BOARD_GPP_INFO db78X60pcacrev2InfoBoardGppInfo[] = {
	 
	{BOARD_GPP_USB_VBUS,    23}  
};

MV_DEV_CS_INFO db78X60pcacrev2InfoBoardDeCsInfo[] = {
	 
#if defined(MV_INCLUDE_SPI)
	{SPI_CS0, N_A, BOARD_DEV_SPI_FLASH, 8}  
#endif
};

MV_BOARD_MPP_INFO db78X60pcacrev2InfoBoardMppConfigValue[] = {
	{ {
		DB_78X60_PCAC_REV2_MPP0_7,
		DB_78X60_PCAC_REV2_MPP8_15,
		DB_78X60_PCAC_REV2_MPP16_23,
		DB_78X60_PCAC_REV2_MPP24_31,
		DB_78X60_PCAC_REV2_MPP32_39,
		DB_78X60_PCAC_REV2_MPP40_47,
		DB_78X60_PCAC_REV2_MPP48_55,
		DB_78X60_PCAC_REV2_MPP56_63,
		DB_78X60_PCAC_REV2_MPP64_67,
	} }
};

MV_BOARD_INFO db78X60pcacrev2Info = {
	.boardName			= "DB-78460-PCAC-REV2",
	.numBoardMppTypeValue		= ARRSZ(db78X60pcacrev2InfoBoardModTypeInfo),
	.pBoardModTypeValue		= db78X60pcacrev2InfoBoardModTypeInfo,
	.numBoardMppConfigValue		= ARRSZ(db78X60pcacrev2InfoBoardMppConfigValue),
	.pBoardMppConfigValue		= db78X60pcacrev2InfoBoardMppConfigValue,
	.intsGppMaskLow			= 0,
	.intsGppMaskMid			= 0,
	.intsGppMaskHigh		= 0,
	.numBoardDeviceIf		= ARRSZ(db78X60pcacrev2InfoBoardDeCsInfo),
	.pDevCsInfo			= db78X60pcacrev2InfoBoardDeCsInfo,
	.numBoardTwsiDev		= 0,
	.pBoardTwsiDev			= NULL,
	.numBoardMacInfo		= ARRSZ(db78X60pcacrev2InfoBoardMacInfo),
	.pBoardMacInfo			= db78X60pcacrev2InfoBoardMacInfo,
	.numBoardGppInfo		= ARRSZ(db78X60pcacrev2InfoBoardGppInfo),
	.pBoardGppInfo			= db78X60pcacrev2InfoBoardGppInfo,
	.activeLedsNumber		= ARRSZ(db78X60pcacrev2InfoBoardDebugLedIf),
	.pLedGppPin			= db78X60pcacrev2InfoBoardDebugLedIf,
	.ledsPolarity			= 0,

	.gppOutEnValLow			= DB_78X60_PCAC_REV2_GPP_OUT_ENA_LOW,
	.gppOutEnValMid			= DB_78X60_PCAC_REV2_GPP_OUT_ENA_MID,
	.gppOutEnValHigh		= DB_78X60_PCAC_REV2_GPP_OUT_ENA_HIGH,
	.gppOutValLow			= DB_78X60_PCAC_REV2_GPP_OUT_VAL_LOW,
	.gppOutValMid			= DB_78X60_PCAC_REV2_GPP_OUT_VAL_MID,
	.gppOutValHigh			= DB_78X60_PCAC_REV2_GPP_OUT_VAL_HIGH,
	.gppPolarityValLow		= DB_78X60_PCAC_REV2_GPP_POL_LOW,
	.gppPolarityValMid		= DB_78X60_PCAC_REV2_GPP_POL_MID,
	.gppPolarityValHigh		= DB_78X60_PCAC_REV2_GPP_POL_HIGH,

	.pSwitchInfo = NULL,
	.switchInfoNum = 0,

	.numBoardTdmInfo			= {},
	.pBoardTdmInt2CsInfo			= {},
	.boardTdmInfoIndex			= -1,

	.nandFlashReadParams		= DB_78X60_PCAC_REV2_BOARD_NAND_READ_PARAMS,
	.nandFlashWriteParams		= DB_78X60_PCAC_REV2_BOARD_NAND_WRITE_PARAMS,
	.nandFlashControl		= DB_78X60_PCAC_REV2_BOARD_NAND_CONTROL
};

#define FPGA_88F78XX0_BOARD_NAND_READ_PARAMS	0x000C0282
#define FPGA_88F78XX0_BOARD_NAND_WRITE_PARAMS	0x00010305
 
#define FPGA_88F78XX0_BOARD_NAND_CONTROL	0x01c00543

MV_BOARD_TWSI_INFO	fpga88f78XX0InfoBoardTwsiDev[] = {
	 
	{BOARD_DEV_TWSI_SATR, 0x4C, ADDR7_BIT},
	{BOARD_DEV_TWSI_SATR, 0x4D, ADDR7_BIT},
	{BOARD_DEV_TWSI_SATR, 0x4E, ADDR7_BIT},
	{BOARD_DEV_TWSI_SATR, 0x4F, ADDR7_BIT}
};

MV_BOARD_MAC_INFO fpga88f78XX0InfoBoardMacInfo[] = {
	 
	{BOARD_MAC_SPEED_AUTO, 0x1,0x0,0x0},
	{BOARD_MAC_SPEED_AUTO, 0x2,0x0,0x0},
	{BOARD_MAC_SPEED_AUTO, 0x3,0x0,0x0},
	{BOARD_MAC_SPEED_AUTO, 0x4,0x0,0x0}
};

MV_BOARD_MODULE_TYPE_INFO fpga88f78XX0InfoBoardModTypeInfo[] = {
	{
		.boardMppMod		= MV_BOARD_AUTO,
		.boardOtherMod		= MV_BOARD_NONE
	}
};

MV_BOARD_GPP_INFO fpga88f78XX0InfoBoardGppInfo[] = {
	 
	{BOARD_GPP_USB_VBUS,    24}  
};

MV_DEV_CS_INFO fpga88f78XX0InfoBoardDeCsInfo[] = {
		 
#if defined(MV_INCLUDE_SPI)
	{SPI_CS0, N_A, BOARD_DEV_SPI_FLASH, 8}  
#endif
};

MV_BOARD_MPP_INFO fpga88f78XX0InfoBoardMppConfigValue[] = {
	{ {
	FPGA_88F78XX0_MPP0_7,
	FPGA_88F78XX0_MPP8_15,
	FPGA_88F78XX0_MPP16_23,
	FPGA_88F78XX0_MPP24_31,
	FPGA_88F78XX0_MPP32_39,
	FPGA_88F78XX0_MPP40_47,
	FPGA_88F78XX0_MPP48_55,
	FPGA_88F78XX0_MPP56_63,
	FPGA_88F78XX0_MPP64_67,
	} }
};

MV_BOARD_TDM_INFO	fpga88f78XX0Tdm880[]	= { {1}, {2} };
MV_BOARD_TDM_INFO	fpga88f78XX0Tdm792[]	= { {1}, {2}, {3}, {4}, {6}, {7} };
MV_BOARD_TDM_INFO	fpga88f78XX0Tdm3215[]	= { {1} };

MV_BOARD_INFO fpga88f78XX0Info = {
	.boardName			= "FPGA-88F78XX0",
	.numBoardMppTypeValue		= ARRSZ(fpga88f78XX0InfoBoardModTypeInfo),
	.pBoardModTypeValue		= fpga88f78XX0InfoBoardModTypeInfo,
	.numBoardMppConfigValue		= ARRSZ(fpga88f78XX0InfoBoardMppConfigValue),
	.pBoardMppConfigValue		= fpga88f78XX0InfoBoardMppConfigValue,
	.intsGppMaskLow			= 0,
	.intsGppMaskMid			= 0,
	.intsGppMaskHigh		= 0,
	.numBoardDeviceIf		= ARRSZ(fpga88f78XX0InfoBoardDeCsInfo),
	.pDevCsInfo			= fpga88f78XX0InfoBoardDeCsInfo,
	.numBoardTwsiDev		= ARRSZ(fpga88f78XX0InfoBoardTwsiDev),
	.pBoardTwsiDev			= fpga88f78XX0InfoBoardTwsiDev,
	.numBoardMacInfo		= ARRSZ(fpga88f78XX0InfoBoardMacInfo),
	.pBoardMacInfo			= fpga88f78XX0InfoBoardMacInfo,
	.numBoardGppInfo		= ARRSZ(fpga88f78XX0InfoBoardGppInfo),
	.pBoardGppInfo			= fpga88f78XX0InfoBoardGppInfo,
	.activeLedsNumber		= 0,
	.pLedGppPin			= NULL,
	.ledsPolarity			= 0,

	.gppOutEnValLow			= FPGA_88F78XX0_GPP_OUT_ENA_LOW,
	.gppOutEnValMid			= FPGA_88F78XX0_GPP_OUT_ENA_MID,
	.gppOutEnValHigh		= FPGA_88F78XX0_GPP_OUT_ENA_HIGH,
	.gppOutValLow			= FPGA_88F78XX0_GPP_OUT_VAL_LOW,
	.gppOutValMid			= FPGA_88F78XX0_GPP_OUT_VAL_MID,
	.gppOutValHigh			= FPGA_88F78XX0_GPP_OUT_VAL_HIGH,
	.gppPolarityValLow		= FPGA_88F78XX0_GPP_POL_LOW,
	.gppPolarityValMid		= FPGA_88F78XX0_GPP_POL_MID,
	.gppPolarityValHigh		= FPGA_88F78XX0_GPP_POL_HIGH,

	.pSwitchInfo = NULL,
	.switchInfoNum = 0,

	.numBoardTdmInfo		= { 2, 6, 1 },
	.pBoardTdmInt2CsInfo		= { fpga88f78XX0Tdm880,
					    fpga88f78XX0Tdm792,
					    fpga88f78XX0Tdm3215
					  },
	.boardTdmInfoIndex		= -1,

	.nandFlashReadParams		= FPGA_88F78XX0_BOARD_NAND_READ_PARAMS,
	.nandFlashWriteParams		= FPGA_88F78XX0_BOARD_NAND_WRITE_PARAMS,
	.nandFlashControl		= FPGA_88F78XX0_BOARD_NAND_CONTROL
};

#define DB_78X60_AMC_BOARD_NAND_READ_PARAMS		0x000C0282
#define DB_78X60_AMC_BOARD_NAND_WRITE_PARAMS		0x00010305
 
#define DB_78X60_AMC_BOARD_NAND_CONTROL			0x01c00543

MV_U8	db78X60amcInfoBoardDebugLedIf[] = {53, 54, 55, 56};  

MV_BOARD_TWSI_INFO	db78X60amcInfoBoardTwsiDev[] = {
	 
};

MV_BOARD_MAC_INFO db78X60amcInfoBoardMacInfo[] = {
	 
	{BOARD_MAC_SPEED_AUTO, 0x1,0x0,0x0},
	{BOARD_MAC_SPEED_AUTO, 0xF,0x0,0x0},
	{BOARD_MAC_SPEED_AUTO, 0xE,0x0,0x0},
	{BOARD_MAC_SPEED_AUTO, 0x0,0x0,0x0}
};

MV_BOARD_MODULE_TYPE_INFO db78X60amcInfoBoardModTypeInfo[] = {
	 
};

MV_BOARD_GPP_INFO db78X60amcInfoBoardGppInfo[] = {
	 
	{BOARD_GPP_USB_VBUS,    46}  
};

MV_DEV_CS_INFO db78X60amcInfoBoardDeCsInfo[] = {
	 
#if defined(MV_INCLUDE_SPI)
	{SPI_CS0, N_A, BOARD_DEV_SPI_FLASH, 8, 8},  
#endif
};

MV_BOARD_MPP_INFO db78X60amcInfoBoardMppConfigValue[] = {
	{ {
		DB_78X60_AMC_MPP0_7,
		DB_78X60_AMC_MPP8_15,
		DB_78X60_AMC_MPP16_23,
		DB_78X60_AMC_MPP24_31,
		DB_78X60_AMC_MPP32_39,
		DB_78X60_AMC_MPP40_47,
		DB_78X60_AMC_MPP48_55,
		DB_78X60_AMC_MPP56_63,
		DB_78X60_AMC_MPP64_67,
	} }
};

MV_BOARD_TDM_INFO	db78X60amcTdm880[]	= {};
MV_BOARD_TDM_INFO	db78X60amcTdm792[]	= {};
MV_BOARD_TDM_INFO	db78X60amcTdm3215[]	= {};

MV_BOARD_INFO db78X60amcInfo = {
	.boardName			= "DB-78460-AMC",
	.numBoardMppTypeValue		= ARRSZ(db78X60amcInfoBoardModTypeInfo),
	.pBoardModTypeValue		= db78X60amcInfoBoardModTypeInfo,
	.numBoardMppConfigValue		= ARRSZ(db78X60amcInfoBoardMppConfigValue),
	.pBoardMppConfigValue		= db78X60amcInfoBoardMppConfigValue,
	.intsGppMaskLow			= 0,
	.intsGppMaskMid			= 0,
	.intsGppMaskHigh		= 0,
	.numBoardDeviceIf		= ARRSZ(db78X60amcInfoBoardDeCsInfo),
	.pDevCsInfo			= db78X60amcInfoBoardDeCsInfo,
	.numBoardTwsiDev		= ARRSZ(db78X60amcInfoBoardTwsiDev),
	.pBoardTwsiDev			= db78X60amcInfoBoardTwsiDev,
	.numBoardMacInfo		= ARRSZ(db78X60amcInfoBoardMacInfo),
	.pBoardMacInfo			= db78X60amcInfoBoardMacInfo,
	.numBoardGppInfo		= ARRSZ(db78X60amcInfoBoardGppInfo),
	.pBoardGppInfo			= db78X60amcInfoBoardGppInfo,
	.activeLedsNumber		= ARRSZ(db78X60amcInfoBoardDebugLedIf),
	.pLedGppPin			= db78X60amcInfoBoardDebugLedIf,
	.ledsPolarity			= 0,

	.gppOutEnValLow			= DB_78X60_AMC_GPP_OUT_ENA_LOW,
	.gppOutEnValMid			= DB_78X60_AMC_GPP_OUT_ENA_MID,
	.gppOutEnValHigh		= DB_78X60_AMC_GPP_OUT_ENA_HIGH,
	.gppOutValLow			= DB_78X60_AMC_GPP_OUT_VAL_LOW,
	.gppOutValMid			= DB_78X60_AMC_GPP_OUT_VAL_MID,
	.gppOutValHigh			= DB_78X60_AMC_GPP_OUT_VAL_HIGH,
	.gppPolarityValLow		= DB_78X60_AMC_GPP_POL_LOW,
	.gppPolarityValMid		= DB_78X60_AMC_GPP_POL_MID,
	.gppPolarityValHigh		= DB_78X60_AMC_GPP_POL_HIGH,

	.pSwitchInfo = NULL,
	.switchInfoNum = 0,

	.numBoardTdmInfo		= {},
	.pBoardTdmInt2CsInfo		= {},
	.boardTdmInfoIndex		= -1,

	.nandFlashReadParams		= DB_78X60_AMC_BOARD_NAND_READ_PARAMS,
	.nandFlashWriteParams		= DB_78X60_AMC_BOARD_NAND_WRITE_PARAMS,
	.nandFlashControl		= DB_78X60_AMC_BOARD_NAND_CONTROL
};

#define RD_78460_GP_BOARD_NAND_READ_PARAMS		0x000C0282
#define RD_78460_GP_BOARD_NAND_WRITE_PARAMS		0x00010305

#define RD_78460_GP_BOARD_NAND_CONTROL			0x01c00543

#define RD_78460_GP_BOARD_NOR_READ_PARAMS		0x403E07CF
#define RD_78460_GP_BOARD_NOR_WRITE_PARAMS		0x000F0F0F

MV_BOARD_TWSI_INFO   rd78460gpInfoBoardTwsiDev[] = {
	 
	{BOARD_DEV_TWSI_SATR, 0x4C, ADDR7_BIT},
	{BOARD_DEV_TWSI_SATR, 0x4D, ADDR7_BIT},
	{BOARD_DEV_TWSI_SATR, 0x4E, ADDR7_BIT}
};

MV_BOARD_MAC_INFO rd78460gpInfoBoardMacInfo[] = {
	 
	{BOARD_MAC_SPEED_AUTO, 0x10,0x0000, 0x10},
	{BOARD_MAC_SPEED_AUTO, 0x11,0x0800, 0x10},
	{BOARD_MAC_SPEED_AUTO, 0x12,0x1000, 0x10},
	{BOARD_MAC_SPEED_AUTO, 0x13,0x1800, 0x10}
};

MV_BOARD_MODULE_TYPE_INFO rd78460gpInfoBoardModTypeInfo[] = {
	{
		.boardMppMod		= MV_BOARD_AUTO,
		.boardOtherMod		= MV_BOARD_NONE
	}
};

MV_BOARD_GPP_INFO rd78460gpInfoBoardGppInfo[] = {
	 
	{BOARD_GPP_RESET, 21}
};

MV_DEV_CS_INFO rd78460gpInfoBoardDeCsInfo[] = {
	 
#if defined(MV_INCLUDE_SPI)
	{SPI_CS0, N_A, BOARD_DEV_SPI_FLASH, 8, 8},  
#endif
#if defined(MV_INCLUDE_NOR)
	{DEV_BOOCS, N_A, BOARD_DEV_NOR_FLASH, 16, 16}  
#endif
};

MV_BOARD_MPP_INFO rd78460gpInfoBoardMppConfigValue[] = {
	{ {
	RD_78460_GP_MPP0_7,
	RD_78460_GP_MPP8_15,
	RD_78460_GP_MPP16_23,
	RD_78460_GP_MPP24_31,
	RD_78460_GP_MPP32_39,
	RD_78460_GP_MPP40_47,
	RD_78460_GP_MPP48_55,
	RD_78460_GP_MPP56_63,
	RD_78460_GP_MPP64_67,
	} }
};

MV_BOARD_INFO rd78460gpInfo = {
	.boardName			= "RD-AXP-GP rev 1.0",
	.numBoardMppTypeValue		= ARRSZ(rd78460gpInfoBoardModTypeInfo),
	.pBoardModTypeValue		= rd78460gpInfoBoardModTypeInfo,
	.numBoardMppConfigValue		= ARRSZ(rd78460gpInfoBoardMppConfigValue),
	.pBoardMppConfigValue		= rd78460gpInfoBoardMppConfigValue,
	.intsGppMaskLow			= 0,
	.intsGppMaskMid			= 0,
	.intsGppMaskHigh		= 0,
	.numBoardDeviceIf		= ARRSZ(rd78460gpInfoBoardDeCsInfo),
	.pDevCsInfo				= rd78460gpInfoBoardDeCsInfo,
	.numBoardTwsiDev		= ARRSZ(rd78460gpInfoBoardTwsiDev),
	.pBoardTwsiDev			= rd78460gpInfoBoardTwsiDev,
	.numBoardMacInfo		= ARRSZ(rd78460gpInfoBoardMacInfo),
	.pBoardMacInfo			= rd78460gpInfoBoardMacInfo,
	.numBoardGppInfo		= ARRSZ(rd78460gpInfoBoardGppInfo),
	.pBoardGppInfo			= rd78460gpInfoBoardGppInfo,
	.activeLedsNumber		= 0,
	.pLedGppPin			= NULL,
	.ledsPolarity			= 0,

	.gppOutEnValLow			= RD_78460_GP_GPP_OUT_ENA_LOW,
	.gppOutEnValMid			= RD_78460_GP_GPP_OUT_ENA_MID,
	.gppOutEnValHigh		= RD_78460_GP_GPP_OUT_ENA_HIGH,
	.gppOutValLow			= RD_78460_GP_GPP_OUT_VAL_LOW,
	.gppOutValMid			= RD_78460_GP_GPP_OUT_VAL_MID,
	.gppOutValHigh			= RD_78460_GP_GPP_OUT_VAL_HIGH,
	.gppPolarityValLow		= RD_78460_GP_GPP_POL_LOW,
	.gppPolarityValMid		= RD_78460_GP_GPP_POL_MID,
	.gppPolarityValHigh		= RD_78460_GP_GPP_POL_HIGH,

	.pSwitchInfo = NULL,
	.switchInfoNum = 0,

	.numBoardTdmInfo			= {},
	.pBoardTdmInt2CsInfo			= {},
	.boardTdmInfoIndex			= -1,

	.nandFlashReadParams		= RD_78460_GP_BOARD_NAND_READ_PARAMS,
	.nandFlashWriteParams		= RD_78460_GP_BOARD_NAND_WRITE_PARAMS,
	.nandFlashControl		= RD_78460_GP_BOARD_NAND_CONTROL,
	 
	.norFlashReadParams		= RD_78460_GP_BOARD_NOR_READ_PARAMS,
	.norFlashWriteParams		= RD_78460_GP_BOARD_NOR_WRITE_PARAMS
};

#define RD_78460_CUSTOMER_BOARD_NAND_READ_PARAMS	0x000C0282
#define RD_78460_CUSTOMER_BOARD_NAND_WRITE_PARAMS	0x00010305
 
#define RD_78460_CUSTOMER_BOARD_NAND_CONTROL		0x01c00543

#define RD_78460_CUSTOMER_BOARD_NOR_READ_PARAMS	0x403E07CF
#define RD_78460_CUSTOMER_BOARD_NOR_WRITE_PARAMS	0x000F0F0F

MV_BOARD_MAC_INFO rd78460customerInfoBoardMacInfo[] = {
	 
	{BOARD_MAC_SPEED_AUTO, 0x10,0x0000, 0x10},
	{BOARD_MAC_SPEED_AUTO, 0x11,0x0800, 0x10},
	{BOARD_MAC_SPEED_AUTO, 0x12,0x1000, 0x10},
	{BOARD_MAC_SPEED_AUTO, 0x13,0x1800, 0x10}
};

MV_BOARD_MODULE_TYPE_INFO rd78460customerInfoBoardModTypeInfo[] = {
	{
		.boardMppMod		= MV_BOARD_AUTO,
		.boardOtherMod		= MV_BOARD_NONE
	}
};
 
MV_BOARD_GPP_INFO rd78460customerInfoBoardGppInfo[] = {
	 
	{BOARD_GPP_RESET, 21}
};

MV_DEV_CS_INFO rd78460customerInfoBoardDeCsInfo[] = {
	 
#if defined(MV_INCLUDE_SPI)
	{SPI_CS0, N_A, BOARD_DEV_SPI_FLASH, 8, 8}  
#endif
};

MV_BOARD_MPP_INFO rd78460customerInfoBoardMppConfigValue[] = {
	{ {
	RD_78460_CUSTOMER_MPP0_7,
	RD_78460_CUSTOMER_MPP8_15,
	RD_78460_CUSTOMER_MPP16_23,
	RD_78460_CUSTOMER_MPP24_31,
	RD_78460_CUSTOMER_MPP32_39,
	RD_78460_CUSTOMER_MPP40_47,
	RD_78460_CUSTOMER_MPP48_55,
	RD_78460_CUSTOMER_MPP56_63,
	RD_78460_CUSTOMER_MPP64_67,
	} }
};

MV_BOARD_INFO rd78460customerInfo = {
	.boardName			= "RD-AXP-CUSTOMER",
	.numBoardMppTypeValue		= ARRSZ(rd78460customerInfoBoardModTypeInfo),
	.pBoardModTypeValue		= rd78460customerInfoBoardModTypeInfo,
	.numBoardMppConfigValue		= ARRSZ(rd78460customerInfoBoardMppConfigValue),
	.pBoardMppConfigValue		= rd78460customerInfoBoardMppConfigValue,
	.intsGppMaskLow			= 0,
	.intsGppMaskMid			= 0,
	.intsGppMaskHigh		= 0,
	.numBoardDeviceIf		= ARRSZ(rd78460customerInfoBoardDeCsInfo),
	.pDevCsInfo			= rd78460customerInfoBoardDeCsInfo,
	.numBoardTwsiDev		= 0,
	.pBoardTwsiDev			= NULL,
	.numBoardMacInfo		= ARRSZ(rd78460customerInfoBoardMacInfo),
	.pBoardMacInfo			= rd78460customerInfoBoardMacInfo,
	.numBoardGppInfo		= ARRSZ(rd78460customerInfoBoardGppInfo),
	.pBoardGppInfo			= rd78460customerInfoBoardGppInfo,
	.activeLedsNumber		= 0,
	.pLedGppPin			= NULL,
	.ledsPolarity			= 0,

	.gppOutEnValLow			= RD_78460_CUSTOMER_GPP_OUT_ENA_LOW,
	.gppOutEnValMid			= RD_78460_CUSTOMER_GPP_OUT_ENA_MID,
	.gppOutEnValHigh		= RD_78460_CUSTOMER_GPP_OUT_ENA_HIGH,
	.gppOutValLow			= RD_78460_CUSTOMER_GPP_OUT_VAL_LOW,
	.gppOutValMid			= RD_78460_CUSTOMER_GPP_OUT_VAL_MID,
	.gppOutValHigh			= RD_78460_CUSTOMER_GPP_OUT_VAL_HIGH,
	.gppPolarityValLow		= RD_78460_CUSTOMER_GPP_POL_LOW,
	.gppPolarityValMid		= RD_78460_CUSTOMER_GPP_POL_MID,
	.gppPolarityValHigh		= RD_78460_CUSTOMER_GPP_POL_HIGH,

	.pSwitchInfo = NULL,
	.switchInfoNum = 0,

	.numBoardTdmInfo			= {},
	.pBoardTdmInt2CsInfo			= {},
	.boardTdmInfoIndex			= -1,

	.nandFlashReadParams		= RD_78460_CUSTOMER_BOARD_NAND_READ_PARAMS,
	.nandFlashWriteParams		= RD_78460_CUSTOMER_BOARD_NAND_WRITE_PARAMS,
	.nandFlashControl		= RD_78460_CUSTOMER_BOARD_NAND_CONTROL,
	 
	.norFlashReadParams		= RD_78460_CUSTOMER_BOARD_NOR_READ_PARAMS,
	.norFlashWriteParams		= RD_78460_CUSTOMER_BOARD_NOR_WRITE_PARAMS
};
 

#ifdef CONFIG_SYNO_ARMADA_ARCH





 
MV_BOARD_MAC_INFO Syno78230AxpBoardMacInfo[] = {
	 
	{BOARD_MAC_SPEED_AUTO, 0x1,0x0,0x0},
	{BOARD_MAC_SPEED_AUTO, 0x0,0x0,0x0},
};

MV_BOARD_MODULE_TYPE_INFO Syno78230AxpBoardModTypeInfo[] = {
	{
		.boardMppMod		= MV_BOARD_AUTO,
		.boardOtherMod		= MV_BOARD_NONE
	}
};

MV_BOARD_GPP_INFO Syno78230AxpBoardGppInfo[] = {
	 
	{BOARD_GPP_USB_VBUS,    24}  
	 
};

MV_DEV_CS_INFO Syno78230AxpBoardDeCsInfo[] = {
	 
	{SPI_CS0, N_A, BOARD_DEV_SPI_FLASH, 8, 8},  
};

#define SYNOAXP_4BAY_MPP0_7			0x11111111
#define SYNOAXP_4BAY_MPP8_15		0x22221111
#define SYNOAXP_4BAY_MPP16_23		0x22222222
#define SYNOAXP_4BAY_MPP24_31		0x00000000
#define SYNOAXP_4BAY_MPP32_39		0x11110000
#define SYNOAXP_4BAY_MPP40_47		0x00004000
#define SYNOAXP_4BAY_MPP48_55		0x00000000
#define SYNOAXP_4BAY_MPP56_63		0x00000000
#define SYNOAXP_4BAY_MPP64_67		0x00000000

MV_BOARD_MPP_INFO Syno78230AxpBoardMppConfigValue[] = {
	{ {
		SYNOAXP_4BAY_MPP0_7,
		SYNOAXP_4BAY_MPP8_15,
		SYNOAXP_4BAY_MPP16_23,
		SYNOAXP_4BAY_MPP24_31,
		SYNOAXP_4BAY_MPP32_39,
		SYNOAXP_4BAY_MPP40_47,
		SYNOAXP_4BAY_MPP48_55,
		SYNOAXP_4BAY_MPP56_63,
		SYNOAXP_4BAY_MPP64_67,
	} },
};

MV_BOARD_INFO Syno78230AxpInfo = {
	.boardName			= "Synology AXP 78230",
	.numBoardMppTypeValue		= ARRSZ(Syno78230AxpBoardModTypeInfo),
	.pBoardModTypeValue		= Syno78230AxpBoardModTypeInfo,
	.numBoardMppConfigValue		= ARRSZ(Syno78230AxpBoardMppConfigValue),
	.pBoardMppConfigValue		= Syno78230AxpBoardMppConfigValue,
	.intsGppMaskLow			= 0,
	.intsGppMaskMid			= 0,
	.intsGppMaskHigh		= 0,
	.numBoardDeviceIf		= ARRSZ(Syno78230AxpBoardDeCsInfo),
	.pDevCsInfo			= Syno78230AxpBoardDeCsInfo,
	.numBoardTwsiDev		= 0,
	.pBoardTwsiDev			= NULL,
	.numBoardMacInfo		= ARRSZ(Syno78230AxpBoardMacInfo),
	.pBoardMacInfo			= Syno78230AxpBoardMacInfo,
	.numBoardGppInfo		= 0,
	.pBoardGppInfo			= NULL,
	.activeLedsNumber		= 0,
	.pLedGppPin			= NULL,
	.ledsPolarity			= 0,

	.gppOutEnValLow			= (~(BIT25 | BIT30)),
	.gppOutEnValMid			= (~(BIT10 | BIT12 | BIT13 | BIT14 | BIT15)),
	.gppOutEnValHigh		= (~(0x0)),
	.gppOutValLow			= BIT25 | BIT30,
	.gppOutValMid			= BIT10,
	.gppOutValHigh			= 0x0,
	.gppPolarityValLow		= 0x0,
	.gppPolarityValMid		= 0x0,
	.gppPolarityValHigh		= 0x0,

	.numBoardTdmInfo		= {},
	.pBoardTdmInt2CsInfo		= {},
	.boardTdmInfoIndex		= -1,

	.nandFlashReadParams		= 0,
	.nandFlashWriteParams		= 0,
	.nandFlashControl		= 0,
	 
	.norFlashReadParams		= 0,
	.norFlashWriteParams		= 0
};

MV_BOARD_MAC_INFO Syno78230AxpBoardMacInfo_2bay[] = {
	 
	{BOARD_MAC_SPEED_AUTO, 0x1,0x0,0x0},
};

#define SYNOAXP_2BAY_MPP0_7			0x00000000
#define SYNOAXP_2BAY_MPP8_15		0x00000000
#define SYNOAXP_2BAY_MPP16_23		0x00000000
#define SYNOAXP_2BAY_MPP24_31		0x00000000   
#define SYNOAXP_2BAY_MPP32_39		0x11110000
#define SYNOAXP_2BAY_MPP40_47		0x00004000
#define SYNOAXP_2BAY_MPP48_55		0x00000000
#define SYNOAXP_2BAY_MPP56_63		0x00000000
#define SYNOAXP_2BAY_MPP64_67		0x00000000

MV_BOARD_MPP_INFO Syno78230AxpBoardMppConfigValue_2bay[] = {
	{ {
		SYNOAXP_2BAY_MPP0_7,
		SYNOAXP_2BAY_MPP8_15,
		SYNOAXP_2BAY_MPP16_23,
		SYNOAXP_2BAY_MPP24_31,
		SYNOAXP_2BAY_MPP32_39,
		SYNOAXP_2BAY_MPP40_47,
		SYNOAXP_2BAY_MPP48_55,
		SYNOAXP_2BAY_MPP56_63,
		SYNOAXP_2BAY_MPP64_67,
	} },
};

MV_BOARD_INFO Syno78230AxpInfo_2bay = {
	.boardName			= "Synology AXP 78230 2 bay",
	.numBoardMppTypeValue		= ARRSZ(Syno78230AxpBoardModTypeInfo),
	.pBoardModTypeValue		= Syno78230AxpBoardModTypeInfo,
	.numBoardMppConfigValue		= ARRSZ(Syno78230AxpBoardMppConfigValue_2bay),
	.pBoardMppConfigValue		= Syno78230AxpBoardMppConfigValue_2bay,
	.intsGppMaskLow			= 0,
	.intsGppMaskMid			= 0,
	.intsGppMaskHigh		= 0,
	.numBoardDeviceIf		= ARRSZ(Syno78230AxpBoardDeCsInfo),
	.pDevCsInfo			= Syno78230AxpBoardDeCsInfo,
	.numBoardTwsiDev		= 0,
	.pBoardTwsiDev			= NULL,
	.numBoardMacInfo		= ARRSZ(Syno78230AxpBoardMacInfo_2bay),
	.pBoardMacInfo			= Syno78230AxpBoardMacInfo_2bay,
	.numBoardGppInfo		= 0,
	.pBoardGppInfo			= NULL,
	.activeLedsNumber		= 0,
	.pLedGppPin			= NULL,
	.ledsPolarity			= 0,

	.pmuPwrUpPolarity		= 0,
	.pmuPwrUpDelay			= 80000,

	.gppOutEnValLow			= (~(BIT24 | BIT25)),   
	.gppOutEnValMid			= (~(BIT12 | BIT13 | BIT14)),
	.gppOutEnValHigh		= (~(0x0)),
	.gppOutValLow			= 0x0,
	.gppOutValMid			= BIT12,
	.gppOutValHigh			= 0x0,
	.gppPolarityValLow		= 0x0,
	.gppPolarityValMid		= 0x0,
	.gppPolarityValHigh		= 0x0,

	.numBoardTdmInfo		= {},
	.pBoardTdmInt2CsInfo		= {},
	.boardTdmInfoIndex		= -1,

	.nandFlashReadParams		= 0,
	.nandFlashWriteParams		= 0,
	.nandFlashControl		= 0,
	 
	.norFlashReadParams		= 0,
	.norFlashWriteParams		= 0
};

#define SYNOAXP_4_RACK_MPP0_7		0x11111111
#define SYNOAXP_4_RACK_MPP8_15		0x22221111
#define SYNOAXP_4_RACK_MPP16_23		0x22222222
#define SYNOAXP_4_RACK_MPP24_31		0x00000000
#define SYNOAXP_4_RACK_MPP32_39		0x11110000
#define SYNOAXP_4_RACK_MPP40_47		0x00004000
#define SYNOAXP_4_RACK_MPP48_55		0x00000000
#define SYNOAXP_4_RACK_MPP56_63		0x00000000
#define SYNOAXP_4_RACK_MPP64_67		0x00000000

MV_BOARD_MPP_INFO Syno78230AxpBoardMppConfigValue_4bay_rack[] = {
	{ {
		SYNOAXP_4_RACK_MPP0_7,
		SYNOAXP_4_RACK_MPP8_15,
		SYNOAXP_4_RACK_MPP16_23,
		SYNOAXP_4_RACK_MPP24_31,
		SYNOAXP_4_RACK_MPP32_39,
		SYNOAXP_4_RACK_MPP40_47,
		SYNOAXP_4_RACK_MPP48_55,
		SYNOAXP_4_RACK_MPP56_63,
		SYNOAXP_4_RACK_MPP64_67,
	} },
};

MV_BOARD_INFO Syno78230AxpInfo_4bay_rack = {
	.boardName			= "Synology AXP 78230 4-bay rack",
	.numBoardMppTypeValue		= ARRSZ(Syno78230AxpBoardModTypeInfo),
	.pBoardModTypeValue		= Syno78230AxpBoardModTypeInfo,
	.numBoardMppConfigValue		= ARRSZ(Syno78230AxpBoardMppConfigValue_4bay_rack),
	.pBoardMppConfigValue		= Syno78230AxpBoardMppConfigValue_4bay_rack,
	.intsGppMaskLow			= 0,
	.intsGppMaskMid			= 0,
	.intsGppMaskHigh		= 0,
	.numBoardDeviceIf		= ARRSZ(Syno78230AxpBoardDeCsInfo),
	.pDevCsInfo			= Syno78230AxpBoardDeCsInfo,
	.numBoardTwsiDev		= 0,
	.pBoardTwsiDev			= NULL,
	.numBoardMacInfo		= ARRSZ(Syno78230AxpBoardMacInfo),
	.pBoardMacInfo			= Syno78230AxpBoardMacInfo,
	.numBoardGppInfo		= 0,
	.pBoardGppInfo			= NULL,
	.activeLedsNumber		= 0,
	.pLedGppPin			= NULL,
	.ledsPolarity			= 0,

	.gppOutEnValLow			= (~(0x0)),   
	.gppOutEnValMid			= (~(BIT8 | BIT13 | BIT15)),
	.gppOutEnValHigh		= (~(0x0)),
	.gppOutValLow			= 0x0,
	.gppOutValMid			= BIT15,
	.gppOutValHigh			= 0x0,
	.gppPolarityValLow		= 0x0,
	.gppPolarityValMid		= 0x0,
	.gppPolarityValHigh		= 0x0,

	.numBoardTdmInfo		= {},
	.pBoardTdmInt2CsInfo		= {},
	.boardTdmInfoIndex		= -1,

	.nandFlashReadParams		= 0,
	.nandFlashWriteParams		= 0,
	.nandFlashControl		= 0,
	 
	.norFlashReadParams		= 0,
	.norFlashWriteParams		= 0
};
#endif

MV_BOARD_INFO *boardInfoTbl[] = {
	&db88f78XX0Info,
	&rd78460Info,
	&db78X60pcacInfo,
	&fpga88f78XX0Info,
	&db88f78XX0rev2Info,
	&rd78460nasInfo,
	&db78X60amcInfo,
	&db78X60pcacrev2Info,
	&rd78460ServerRev2Info,
	&rd78460gpInfo,
	&rd78460customerInfo
#ifdef CONFIG_SYNO_ARMADA_ARCH
	,NULL
	,NULL
	,NULL
	,NULL
	,NULL
	,&Syno78230AxpInfo
	,&Syno78230AxpInfo_2bay
	,&Syno78230AxpInfo_4bay_rack
#endif
};
