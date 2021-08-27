 
#include "mvCommon.h"
#include "mvBoardEnvLib.h"
#include "mvBoardEnvSpec.h"
#include "twsi/mvTwsi.h"
#include "pex/mvPexRegs.h"

#define ARRSZ(x)                (sizeof(x) / sizeof(x[0]))

#ifdef CONFIG_SYNO_ARMADA_ARCH_V2

MV_BOARD_MAC_INFO syno_ds215j_BoardMacInfo[] = {
	 
	{ BOARD_MAC_SPEED_AUTO, 0x1, 0x1						},
	{ BOARD_MAC_SPEED_AUTO, 0x1, 0x1						},
	{ N_A,			N_A									}
};
MV_BOARD_MPP_TYPE_INFO syno_ds215j_BoardModTypeInfo[] = {
	{
		.boardMppSlic = MV_BOARD_SLIC_DISABLED,
		.ethSataComplexOpt = (MV_ETHCOMP_GE_MAC1_2_PON_ETH_SERDES_SFP | MV_ETHCOMP_GE_MAC0_2_RGMII0),
		.ethPortsMode = 0x0
	}
};

MV_DEV_CS_INFO syno_ds215j_BoardDeCsInfo[] = {
	 
#if defined(MV_INCLUDE_SPI)
	{ SPI_CS0, N_A, BOARD_DEV_SPI_FLASH, 8, 8 },  
#endif
#if defined(MV_INCLUDE_NOR)
	{DEV_BOOCS, N_A, BOARD_DEV_NOR_FLASH, 16, 16},  
#endif
#if defined(MV_INCLUDE_LEGACY_NAND)
	{DEV_BOOCS, N_A, BOARD_DEV_NAND_FLASH, 16, 16}   
#endif
};

MV_BOARD_MPP_INFO syno_ds215j_BoardMppConfigValue[] = {
	{ {
		  SYNO_DS215j_MPP0_7,
		  SYNO_DS215j_MPP8_15,
		  SYNO_DS215j_MPP16_23,
		  SYNO_DS215j_MPP24_31,
		  SYNO_DS215j_MPP32_39,
		  SYNO_DS215j_MPP40_47,
		  SYNO_DS215j_MPP48_55,
		  SYNO_DS215j_MPP56_63,
		  SYNO_DS215j_MPP64_67,
	 } }
};

MV_BOARD_INFO syno_ds215j_info = {
	.boardName			= "SYNO-DS215j",
	.numBoardMppTypeValue		= ARRSZ(syno_ds215j_BoardModTypeInfo),
	.pBoardModTypeValue		= syno_ds215j_BoardModTypeInfo,
	.pBoardMppConfigValue		= syno_ds215j_BoardMppConfigValue,
	.intsGppMaskLow			= 0,
	.intsGppMaskMid			= 0,
	.intsGppMaskHigh		= 0,
	.numBoardDeviceIf		= ARRSZ(syno_ds215j_BoardDeCsInfo),
	.pDevCsInfo			= syno_ds215j_BoardDeCsInfo,

	.numBoardTwsiDev		= 0,
	.pBoardTwsiDev			= NULL,
	.numBoardMacInfo		= ARRSZ(syno_ds215j_BoardMacInfo),
	.pBoardMacInfo			= syno_ds215j_BoardMacInfo,
	.numBoardGppInfo		= 0,
	.pBoardGppInfo			= 0,
	.activeLedsNumber		= 0,
	.pLedGppPin			= NULL,
	.ledsPolarity			= 0,

	.pmuPwrUpPolarity		= 0,
	.pmuPwrUpDelay			= 80000,

	.gppOutEnValLow			= SYNO_DS215j_GPP_OUT_ENA_LOW,
	.gppOutEnValMid			= SYNO_DS215j_GPP_OUT_ENA_MID,
	.gppOutEnValHigh		= SYNO_DS215j_GPP_OUT_ENA_HIGH,
	.gppOutValLow			= SYNO_DS215j_GPP_OUT_VAL_LOW,
	.gppOutValMid			= SYNO_DS215j_GPP_OUT_VAL_MID,
	.gppOutValHigh			= SYNO_DS215j_GPP_OUT_VAL_HIGH,
	.gppPolarityValLow		= SYNO_DS215j_GPP_POL_LOW,
	.gppPolarityValMid		= SYNO_DS215j_GPP_POL_MID,
	.gppPolarityValHigh		= SYNO_DS215j_GPP_POL_HIGH,

	.switchforceLinkMask		= 0x0,

	.numBoardTdmInfo		= {},
	.pBoardTdmInt2CsInfo		= {},
	.boardTdmInfoIndex		= -1,

	.pBoardSpecInit			= NULL,

	.nandFlashReadParams	= 0,
	.nandFlashWriteParams	= 0,
	.nandFlashControl		= 0,
	 
	.norFlashReadParams 	= 0,
	.norFlashWriteParams	= 0,
	 
	.configAutoDetect		= MV_FALSE
};

MV_BOARD_MAC_INFO syno_ds115_BoardMacInfo[] = {
	 
	{ BOARD_MAC_SPEED_AUTO, 0x1, 0x1						},
	{ BOARD_MAC_SPEED_AUTO, 0x1, 0x1						},
	{ N_A,			N_A									}
};
MV_BOARD_MPP_TYPE_INFO syno_ds115_BoardModTypeInfo[] = {
	{
		.boardMppSlic = MV_BOARD_SLIC_DISABLED,
		.ethSataComplexOpt = (MV_ETHCOMP_GE_MAC1_2_PON_ETH_SERDES_SFP | MV_ETHCOMP_GE_MAC0_2_RGMII0),
		.ethPortsMode = 0x0
	}
};

MV_DEV_CS_INFO syno_ds115_BoardDeCsInfo[] = {
	 
#if defined(MV_INCLUDE_SPI)
	{ SPI_CS0, N_A, BOARD_DEV_SPI_FLASH, 8, 8 },  
#endif
#if defined(MV_INCLUDE_NOR)
	{DEV_BOOCS, N_A, BOARD_DEV_NOR_FLASH, 16, 16},  
#endif
#if defined(MV_INCLUDE_LEGACY_NAND)
	{DEV_BOOCS, N_A, BOARD_DEV_NAND_FLASH, 16, 16}   
#endif
};

MV_BOARD_MPP_INFO syno_ds115_BoardMppConfigValue[] = {
	{ {
		  SYNO_DS115_MPP0_7,
		  SYNO_DS115_MPP8_15,
		  SYNO_DS115_MPP16_23,
		  SYNO_DS115_MPP24_31,
		  SYNO_DS115_MPP32_39,
		  SYNO_DS115_MPP40_47,
		  SYNO_DS115_MPP48_55,
		  SYNO_DS115_MPP56_63,
		  SYNO_DS115_MPP64_67,
	 } }
};

MV_BOARD_INFO syno_ds115_info = {
	.boardName			= "SYNO-DS115",
	.numBoardMppTypeValue		= ARRSZ(syno_ds115_BoardModTypeInfo),
	.pBoardModTypeValue		= syno_ds115_BoardModTypeInfo,
	.pBoardMppConfigValue		= syno_ds115_BoardMppConfigValue,
	.intsGppMaskLow			= 0,
	.intsGppMaskMid			= 0,
	.intsGppMaskHigh		= 0,
	.numBoardDeviceIf		= ARRSZ(syno_ds115_BoardDeCsInfo),
	.pDevCsInfo			= syno_ds115_BoardDeCsInfo,

	.numBoardTwsiDev		= 0,
	.pBoardTwsiDev			= NULL,
	.numBoardMacInfo		= ARRSZ(syno_ds115_BoardMacInfo),
	.pBoardMacInfo			= syno_ds115_BoardMacInfo,
	.numBoardGppInfo		= 0,
	.pBoardGppInfo			= 0,
	.activeLedsNumber		= 0,
	.pLedGppPin			= NULL,
	.ledsPolarity			= 0,

	.pmuPwrUpPolarity		= 0,
	.pmuPwrUpDelay			= 80000,

	.gppOutEnValLow			= SYNO_DS115_GPP_OUT_ENA_LOW,
	.gppOutEnValMid			= SYNO_DS115_GPP_OUT_ENA_MID,
	.gppOutEnValHigh		= SYNO_DS115_GPP_OUT_ENA_HIGH,
	.gppOutValLow			= SYNO_DS115_GPP_OUT_VAL_LOW,
	.gppOutValMid			= SYNO_DS115_GPP_OUT_VAL_MID,
	.gppOutValHigh			= SYNO_DS115_GPP_OUT_VAL_HIGH,
	.gppPolarityValLow		= SYNO_DS115_GPP_POL_LOW,
	.gppPolarityValMid		= SYNO_DS115_GPP_POL_MID,
	.gppPolarityValHigh		= SYNO_DS115_GPP_POL_HIGH,

	.switchforceLinkMask		= 0x0,

	.numBoardTdmInfo		= {},
	.pBoardTdmInt2CsInfo		= {},
	.boardTdmInfoIndex		= -1,

	.pBoardSpecInit			= NULL,

	.nandFlashReadParams            = 0,
	.nandFlashWriteParams           = 0,
	.nandFlashControl		= 0,
	 
	.norFlashReadParams             = 0,
	.norFlashWriteParams            = 0,
	 
	.configAutoDetect		= MV_FALSE
};

MV_BOARD_INFO *customerBoardInfoTbl[] = {
	&syno_ds215j_info,
	&syno_ds115_info,
};
#else
#define A375_CUSTOMER_BOARD_0_NOR_READ_PARAMS	0x403E07CF
#define A375_CUSTOMER_BOARD_0_NOR_WRITE_PARAMS	0x000F0F0F

#define A375_CUSTOMER_BOARD_0_NAND_READ_PARAMS	0x003E07CF
#define A375_CUSTOMER_BOARD_0_NAND_WRITE_PARAMS	0x000F0F0F

MV_BOARD_TWSI_INFO armada_375_customer_board_0_BoardTwsiDev[] = {
	 
	{ BOARD_DEV_TWSI_SATR,		0,	0x4C,	   ADDR7_BIT	},
	{ BOARD_DEV_TWSI_SATR,		1,	0x4D,	   ADDR7_BIT	},
	{ BOARD_DEV_TWSI_EEPROM,	0,	0x52,	   ADDR7_BIT	},
};

MV_BOARD_MAC_INFO armada_375_customer_board_0_BoardMacInfo[] = {
	 
	{ BOARD_MAC_SPEED_AUTO, 0x0, 0x0},
	{ BOARD_MAC_SPEED_AUTO, 0x3, 0x3},
	{ N_A,			N_A, N_A}
};
MV_BOARD_MPP_TYPE_INFO armada_375_customer_board_0_BoardModTypeInfo[] = {
	{
		.boardMppSlic = MV_BOARD_SLIC_DISABLED,
		.ethSataComplexOpt = (MV_ETHCOMP_GE_MAC1_2_GE_PHY_P3 | MV_ETHCOMP_GE_MAC0_2_RGMII0),
		.ethPortsMode = 0x0
	}
};

MV_DEV_CS_INFO armada_375_customer_board_0_BoardDeCsInfo[] = {
	 
#if defined(MV_INCLUDE_SPI)
	{ SPI_CS0, N_A, BOARD_DEV_SPI_FLASH, 8, 8 },  
#endif
#if defined(MV_INCLUDE_NOR)
	{DEV_BOOCS, N_A, BOARD_DEV_NOR_FLASH, 16, 16},  
#endif
#if defined(MV_INCLUDE_LEGACY_NAND)
	{DEV_BOOCS, N_A, BOARD_DEV_NAND_FLASH, 16, 16}   
#endif
};

MV_BOARD_MPP_INFO armada_375_customer_board_0_BoardMppConfigValue[] = {
	{ {
		  A375_CUSTOMER_BOARD_0_MPP0_7,
		  A375_CUSTOMER_BOARD_0_MPP8_15,
		  A375_CUSTOMER_BOARD_0_MPP16_23,
		  A375_CUSTOMER_BOARD_0_MPP24_31,
		  A375_CUSTOMER_BOARD_0_MPP32_39,
		  A375_CUSTOMER_BOARD_0_MPP40_47,
		  A375_CUSTOMER_BOARD_0_MPP48_55,
		  A375_CUSTOMER_BOARD_0_MPP56_63,
		  A375_CUSTOMER_BOARD_0_MPP64_67,
	 } }
};

MV_BOARD_INFO armada_375_customer_board_0_info = {
	.boardName			= "Armada-375-Customer-Board-0",
	.numBoardMppTypeValue		= ARRSZ(armada_375_customer_board_0_BoardModTypeInfo),
	.pBoardModTypeValue		= armada_375_customer_board_0_BoardModTypeInfo,
	.pBoardMppConfigValue		= armada_375_customer_board_0_BoardMppConfigValue,
	.intsGppMaskLow			= 0,
	.intsGppMaskMid			= 0,
	.intsGppMaskHigh		= 0,
	.numBoardDeviceIf		= ARRSZ(armada_375_customer_board_0_BoardDeCsInfo),
	.pDevCsInfo			= armada_375_customer_board_0_BoardDeCsInfo,

	.numBoardTwsiDev		= ARRSZ(armada_375_customer_board_0_BoardTwsiDev),
	.pBoardTwsiDev			= armada_375_customer_board_0_BoardTwsiDev,
	.numBoardMacInfo		= ARRSZ(armada_375_customer_board_0_BoardMacInfo),
	.pBoardMacInfo			= armada_375_customer_board_0_BoardMacInfo,
	.numBoardGppInfo		= 0,
	.pBoardGppInfo			= 0,
	.activeLedsNumber		= 0,
	.pLedGppPin			= NULL,
	.ledsPolarity			= 0,

	.pmuPwrUpPolarity		= 0,
	.pmuPwrUpDelay			= 80000,

	.gppOutEnValLow			= A375_CUSTOMER_BOARD_0_GPP_OUT_ENA_LOW,
	.gppOutEnValMid			= A375_CUSTOMER_BOARD_0_GPP_OUT_ENA_MID,
	.gppOutEnValHigh		= A375_CUSTOMER_BOARD_0_GPP_OUT_ENA_HIGH,
	.gppOutValLow			= A375_CUSTOMER_BOARD_0_GPP_OUT_VAL_LOW,
	.gppOutValMid			= A375_CUSTOMER_BOARD_0_GPP_OUT_VAL_MID,
	.gppOutValHigh			= A375_CUSTOMER_BOARD_0_GPP_OUT_VAL_HIGH,
	.gppPolarityValLow		= A375_CUSTOMER_BOARD_0_GPP_POL_LOW,
	.gppPolarityValMid		= A375_CUSTOMER_BOARD_0_GPP_POL_MID,
	.gppPolarityValHigh		= A375_CUSTOMER_BOARD_0_GPP_POL_HIGH,

	.switchforceLinkMask		= 0x0,

	.numBoardTdmInfo		= {},
	.pBoardTdmInt2CsInfo		= {},
	.boardTdmInfoIndex		= -1,

	.pBoardSpecInit			= NULL,

	.nandFlashReadParams		= A375_CUSTOMER_BOARD_0_NAND_READ_PARAMS,
	.nandFlashWriteParams		= A375_CUSTOMER_BOARD_0_NAND_WRITE_PARAMS,
	.nandFlashControl		= 0,
	 
	.norFlashReadParams 	= 0,
	.norFlashWriteParams	= 0,
	 
	.configAutoDetect		= MV_FALSE
};

MV_BOARD_INFO *customerBoardInfoTbl[] = {
	&armada_375_customer_board_0_info,
	&armada_375_customer_board_0_info
};

#endif
 
#define DB_88F6720_BOARD_NOR_READ_PARAMS	0x403E07CF
#define DB_88F6720_BOARD_NOR_WRITE_PARAMS	0x000F0F0F

#define DB_88F6720_BOARD_NAND_READ_PARAMS	0x003E07CF
#define DB_88F6720_BOARD_NAND_WRITE_PARAMS	0x000F0F0F

MV_BOARD_TWSI_INFO db88f6720InfoBoardTwsiDev[] = {
	 
	{ BOARD_DEV_TWSI_SATR,		0,	0x4C,	   ADDR7_BIT	},
	{ BOARD_DEV_TWSI_SATR,		1,	0x4D,	   ADDR7_BIT	},
	{ BOARD_DEV_TWSI_EEPROM,	0,	0x52,	   ADDR7_BIT	},
	{ BOARD_DEV_TWSI_IO_EXPANDER,	0,	0x24,	   ADDR7_BIT	},
};

MV_BOARD_MAC_INFO db88f6720InfoBoardMacInfo[] = {
	 
	{ BOARD_MAC_SPEED_AUTO, 0x0, 0x0 },
#ifndef CONFIG_MAC1_2_PON_ETH_SERDES_SFP
	{ BOARD_MAC_SPEED_AUTO, 0x3, 0x3 },
#else
	{ BOARD_MAC_SPEED_AUTO, -1,  -1  },
#endif
	{ N_A,			N_A, N_A },
};
MV_BOARD_MPP_TYPE_INFO db88f6720InfoBoardModTypeInfo[] = {
	{
		.boardMppSlic = MV_BOARD_SLIC_DISABLED,
#ifdef CONFIG_MAC1_2_PON_ETH_SERDES_SFP
		.ethSataComplexOpt = (MV_ETHCOMP_GE_MAC1_2_PON_ETH_SERDES_SFP | MV_ETHCOMP_GE_MAC0_2_RGMII0),
#else
		.ethSataComplexOpt = (MV_ETHCOMP_GE_MAC1_2_GE_PHY_P3 | MV_ETHCOMP_GE_MAC0_2_RGMII0),
#endif
		.ethPortsMode = 0x0
	}
};

MV_DEV_CS_INFO db88f6720InfoBoardDeCsInfo[] = {
	 
#if defined(MV_INCLUDE_SPI)
	{ SPI_CS0, N_A, BOARD_DEV_SPI_FLASH, 8, 8 },  
#endif
#if defined(MV_INCLUDE_NOR)
	{DEV_BOOCS, N_A, BOARD_DEV_NOR_FLASH, 16, 16},  
#endif
#if defined(MV_INCLUDE_LEGACY_NAND)
	{DEV_BOOCS, N_A, BOARD_DEV_NAND_FLASH, 16, 16}   
#endif
};

MV_BOARD_MPP_INFO db88f6720InfoBoardMppConfigValue[] = {
	{ {
		  DB_88F6720_MPP0_7,
		  DB_88F6720_MPP8_15,
		  DB_88F6720_MPP16_23,
		  DB_88F6720_MPP24_31,
		  DB_88F6720_MPP32_39,
		  DB_88F6720_MPP40_47,
		  DB_88F6720_MPP48_55,
		  DB_88F6720_MPP56_63,
		  DB_88F6720_MPP64_67,
	 } }
};

MV_BOARD_INFO db88f6720_board_info = {
	.boardName			= "DB-88F6720-V2",
	.numBoardMppTypeValue		= ARRSZ(db88f6720InfoBoardModTypeInfo),
	.pBoardModTypeValue		= db88f6720InfoBoardModTypeInfo,
	.pBoardMppConfigValue		= db88f6720InfoBoardMppConfigValue,
	.intsGppMaskLow			= 0,
	.intsGppMaskMid			= 0,
	.intsGppMaskHigh		= 0,
	.numBoardDeviceIf		= ARRSZ(db88f6720InfoBoardDeCsInfo),
	.pDevCsInfo			= db88f6720InfoBoardDeCsInfo,
	.numBoardTwsiDev		= ARRSZ(db88f6720InfoBoardTwsiDev),
	.pBoardTwsiDev			= db88f6720InfoBoardTwsiDev,
	.numBoardMacInfo		= ARRSZ(db88f6720InfoBoardMacInfo),
	.pBoardMacInfo			= db88f6720InfoBoardMacInfo,
	.numBoardGppInfo		= 0,
	.pBoardGppInfo			= 0,
	.activeLedsNumber		= 0,
	.pLedGppPin			= NULL,
	.ledsPolarity			= 0,

	.pmuPwrUpPolarity		= 0,
	.pmuPwrUpDelay			= 80000,

	.gppOutEnValLow			= DB_88F6720_GPP_OUT_ENA_LOW,
	.gppOutEnValMid			= DB_88F6720_GPP_OUT_ENA_MID,
	.gppOutEnValHigh		= DB_88F6720_GPP_OUT_ENA_HIGH,
	.gppOutValLow			= DB_88F6720_GPP_OUT_VAL_LOW,
	.gppOutValMid			= DB_88F6720_GPP_OUT_VAL_MID,
	.gppOutValHigh			= DB_88F6720_GPP_OUT_VAL_HIGH,
	.gppPolarityValLow		= DB_88F6720_GPP_POL_LOW,
	.gppPolarityValMid		= DB_88F6720_GPP_POL_MID,
	.gppPolarityValHigh		= DB_88F6720_GPP_POL_HIGH,

	.switchforceLinkMask		= 0x0,

	.numBoardTdmInfo		= {},
	.pBoardTdmInt2CsInfo		= {},
	.boardTdmInfoIndex		= -1,

	.pBoardSpecInit			= NULL,

	.nandFlashReadParams		= DB_88F6720_BOARD_NAND_READ_PARAMS,
	.nandFlashWriteParams		= DB_88F6720_BOARD_NAND_WRITE_PARAMS,
	.nandFlashControl		= 0,
	 
	.norFlashReadParams		= DB_88F6720_BOARD_NOR_READ_PARAMS,
	.norFlashWriteParams		= DB_88F6720_BOARD_NOR_WRITE_PARAMS,
	 
	.configAutoDetect		= MV_TRUE
};

MV_BOARD_INFO *marvellBoardInfoTbl[] = {
#ifdef CONFIG_SYNO_ARMADA_ARCH_V2
	&syno_ds215j_info,
	&syno_ds115_info,
#else
	&db88f6720_board_info,
	&armada_375_customer_board_0_info,
#endif
};
