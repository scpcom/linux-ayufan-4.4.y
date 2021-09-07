#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __INCmvBoardEnvSpech
#define __INCmvBoardEnvSpech

#include "mvSysHwConfig.h"

#define BD_ID_DATA_START_OFFS		0x0
#define BD_DETECT_SEQ_OFFS		0x0
#define BD_SYS_NUM_OFFS			0x4
#define BD_NAME_OFFS			0x8

#define MV_BOARD_CTRL_I2C_ADDR			0x0      
#define MV_BOARD_CTRL_I2C_ADDR_TYPE 		ADDR7_BIT
#define MV_BOARD_DIMM0_I2C_ADDR			0x56
#define MV_BOARD_DIMM0_I2C_ADDR_TYPE 		ADDR7_BIT
#define MV_BOARD_DIMM1_I2C_ADDR			0x54
#define MV_BOARD_DIMM1_I2C_ADDR_TYPE 		ADDR7_BIT
#define MV_BOARD_EEPROM_I2C_ADDR	    	0x51
#define MV_BOARD_EEPROM_I2C_ADDR_TYPE 		ADDR7_BIT
#define MV_BOARD_MAIN_EEPROM_I2C_ADDR	   	0x50
#define MV_BOARD_MAIN_EEPROM_I2C_ADDR_TYPE 	ADDR7_BIT
#define MV_BOARD_MUX_I2C_ADDR_ENTRY		0x2
#define MV_BOARD_DIMM_I2C_CHANNEL		0x0

#define BOOT_FLASH_INDEX			0
#define MAIN_FLASH_INDEX			1

#define BOARD_ETH_START_PORT_NUM	0

#define MV_BOARD_TCLK_100MHZ	100000000
#define MV_BOARD_TCLK_125MHZ	125000000
#define MV_BOARD_TCLK_133MHZ	133333333
#define MV_BOARD_TCLK_150MHZ	150000000
#define MV_BOARD_TCLK_166MHZ	166666667
#define MV_BOARD_TCLK_200MHZ	200000000

#define MV_BOARD_SYSCLK_100MHZ	100000000
#define MV_BOARD_SYSCLK_125MHZ	125000000
#define MV_BOARD_SYSCLK_133MHZ	133333333
#define MV_BOARD_SYSCLK_150MHZ	150000000
#define MV_BOARD_SYSCLK_166MHZ	166666667
#define MV_BOARD_SYSCLK_200MHZ	200000000
#define MV_BOARD_SYSCLK_233MHZ	233333333
#define MV_BOARD_SYSCLK_250MHZ	250000000
#define MV_BOARD_SYSCLK_267MHZ	266666667
#define MV_BOARD_SYSCLK_300MHZ	300000000
#define MV_BOARD_SYSCLK_333MHZ	333333334
#define MV_BOARD_SYSCLK_400MHZ	400000000

#define MV_BOARD_REFCLK_25MHZ	 25000000

#define BOARD_ID_BASE           		0x0

#define DB_88F6281A_BP_ID			(BOARD_ID_BASE)
#define DB_88F6281_BP_MLL_ID        1680
#define RD_88F6281A_ID				(BOARD_ID_BASE+0x1)
#define RD_88F6281_MLL_ID			1682
#define DB_88F6192A_BP_ID			(BOARD_ID_BASE+0x2)
#define RD_88F6192A_ID				(BOARD_ID_BASE+0x3)
#define RD_88F6192_MLL_ID			1681
#define DB_88F6180A_BP_ID			(BOARD_ID_BASE+0x4)
#define DB_88F6190A_BP_ID			(BOARD_ID_BASE+0x5)
#define RD_88F6190A_ID				(BOARD_ID_BASE+0x6)
#define RD_88F6281A_PCAC_ID			(BOARD_ID_BASE+0x7)
#define DB_CUSTOMER_ID			    (BOARD_ID_BASE+0x8)
#define SHEEVA_PLUG_ID			    (BOARD_ID_BASE+0x9)
#define DB_88F6280A_BP_ID		    (BOARD_ID_BASE+0xA)
#define DB_88F6282A_BP_ID		    (BOARD_ID_BASE+0xB)
#define RD_88F6282A_ID		   		(BOARD_ID_BASE+0xC)
#define DB_88F6701A_BP_ID			(BOARD_ID_BASE+0xD)
#define DB_88F6702A_BP_ID			(BOARD_ID_BASE+0xE)
#ifdef MY_ABC_HERE
#define SYNO_DS409_ID				(BOARD_ID_BASE+0x13)
#define SYNO_DS409slim_ID			(BOARD_ID_BASE+0x14)
#define SYNO_DS109_ID				(BOARD_ID_BASE+0x15)
#define SYNO_DS011_ID				(BOARD_ID_BASE+0x16)
#define SYNO_DS211_ID				(BOARD_ID_BASE+0x17)
#define SYNO_DS411slim_ID			(BOARD_ID_BASE+0x18)
#define SYNO_RS_6282_ID				(BOARD_ID_BASE+0x19)
#define SYNO_DS411_ID				(BOARD_ID_BASE+0x1A)
#define SYNO_DS212_ID				(BOARD_ID_BASE+0x1B)
#define SYNO_6702_1BAY_ID			(BOARD_ID_BASE+0x1C)
#define SYNO_RS213_ID				(BOARD_ID_BASE+0x1D)
#define MV_MAX_BOARD_ID				(BOARD_ID_BASE+0x1E)
#else
#define MV_MAX_BOARD_ID 			(DB_88F6702A_BP_ID + 1)
#endif

#if defined(MV_NAND)
    #define DB_88F6281A_MPP0_7                   	0x21111111
#else
    #define DB_88F6281A_MPP0_7                   	0x21112220
#endif
#define DB_88F6281A_MPP8_15                   	0x11113311
#define DB_88F6281A_MPP16_23                   	0x00551111
#define DB_88F6281A_MPP24_31                   	0x00000000
#define DB_88F6281A_MPP32_39                   	0x00000000
#define DB_88F6281A_MPP40_47                   	0x00000000
#define DB_88F6281A_MPP48_55                   	0x00000000
#define DB_88F6281A_OE_LOW                       0x0
#if defined(MV_TDM_5CHANNELS)
	#define DB_88F6281A_OE_HIGH		(BIT6)
#else
#define DB_88F6281A_OE_HIGH                      0x0
#endif
#define DB_88F6281A_OE_VAL_LOW                   0x0
#define DB_88F6281A_OE_VAL_HIGH                  0x0

#if defined(MV_NAND)
    #define DB_88F6282A_MPP0_7                   	0x21111111
#else
    #define DB_88F6282A_MPP0_7                   	0x21112220
#endif
#define DB_88F6282A_MPP8_15                   	0x11113311
#define DB_88F6282A_MPP16_23                   	0x00551111
#define DB_88F6282A_MPP24_31                   	0x00000000
#define DB_88F6282A_MPP32_39                   	0x00000000
#define DB_88F6282A_MPP40_47                   	0x00000000
#define DB_88F6282A_MPP48_55                   	0x00000000
#define DB_88F6282A_OE_LOW                       0x0
#if defined(MV_TDM_5CHANNELS)
	#define DB_88F6282A_OE_HIGH		(BIT6)
#else
#define DB_88F6282A_OE_HIGH                      0x0
#endif
#define DB_88F6282A_OE_VAL_LOW                   0x0
#define DB_88F6282A_OE_VAL_HIGH                  0x0

#define RD_88F6282A_MPP0_7                   	0x21111111
#define RD_88F6282A_MPP8_15                   	0x433B2211
#define RD_88F6282A_MPP16_23                   	0x33331104
#define RD_88F6282A_MPP24_31                   	0x33023333
#define RD_88F6282A_MPP32_39                   	0x40000033
#define RD_88F6282A_MPP40_47                   	0x22224444
#define RD_88F6282A_MPP48_55                   	0x00000002
#define RD_88F6282A_OE_LOW                       ~(BIT17)
#define RD_88F6282A_OE_HIGH                      ~(BIT2 | BIT3 | BIT4)
#define RD_88F6282A_OE_VAL_LOW                   BIT17
#define RD_88F6282A_OE_VAL_HIGH                  (BIT2|BIT3|BIT4)

#if defined(MV_NAND)
    #define RD_88F6281A_MPP0_7                   	0x21111111
#else
    #define RD_88F6281A_MPP0_7                   	0x21112220
#endif
#define RD_88F6281A_MPP8_15                   	0x11113311
#define RD_88F6281A_MPP16_23                   	0x33331111
#define RD_88F6281A_MPP24_31                   	0x33003333
#define RD_88F6281A_MPP32_39                   	0x20440533
#define RD_88F6281A_MPP40_47                   	0x22202222
#define RD_88F6281A_MPP48_55                   	0x00000002
#define RD_88F6281A_OE_LOW                      (BIT28 | BIT29)
#define RD_88F6281A_OE_HIGH                     (BIT3 | BIT6 | BIT17)
#define RD_88F6281A_OE_VAL_LOW                   0x0
#define RD_88F6281A_OE_VAL_HIGH                  0x0

#if defined(MV_NAND)
    #define DB_88F6192A_MPP0_7                   	0x21111111
#else
    #define DB_88F6192A_MPP0_7                   	0x21112220
#endif
#define DB_88F6192A_MPP8_15                   	0x11113311
#define DB_88F6192A_MPP16_23                   	0x00501111
#define DB_88F6192A_MPP24_31                   	0x00000000
#define DB_88F6192A_MPP32_35                   	0x00000000
#define DB_88F6192A_OE_LOW                       (BIT22 | BIT23)
#define DB_88F6192A_OE_HIGH                      0x0
#define DB_88F6192A_OE_VAL_LOW                   0x0
#define DB_88F6192A_OE_VAL_HIGH                  0x0

#define RD_88F6192A_MPP0_7                   	0x01222222
#define RD_88F6192A_MPP8_15                   	0x00000011
#define RD_88F6192A_MPP16_23                   	0x05550000
#define RD_88F6192A_MPP24_31                   	0x0
#define RD_88F6192A_MPP32_35                   	0x0
#define RD_88F6192A_OE_LOW                      (BIT11 | BIT14 | BIT24 | BIT25 | BIT26 | BIT27 | BIT30 | BIT31)
#define RD_88F6192A_OE_HIGH                     (BIT0 | BIT2)
#define RD_88F6192A_OE_VAL_LOW                  0x18400
#define RD_88F6192A_OE_VAL_HIGH                 0x8

#if defined(MV_NAND)
    #define DB_88F6180A_MPP0_7                   	0x21111111
#else
    #define DB_88F6180A_MPP0_7                   	0x01112222
#endif
#define DB_88F6180A_MPP8_15                   	0x11113311
#define DB_88F6180A_MPP16_23                   	0x00001111
#define DB_88F6180A_MPP24_31                   	0x0
#define DB_88F6180A_MPP32_39                   	0x4444c000
#define DB_88F6180A_MPP40_44                   	0x00044444
#define DB_88F6180A_OE_LOW                       0x0
#define DB_88F6180A_OE_HIGH                      0x0
#define DB_88F6180A_OE_VAL_LOW                   0x0
#define DB_88F6180A_OE_VAL_HIGH                  0x0

#define RD_88F6281A_PCAC_MPP0_7                	0x21111111
#define RD_88F6281A_PCAC_MPP8_15               	0x00003311
#define RD_88F6281A_PCAC_MPP16_23              	0x00001100
#define RD_88F6281A_PCAC_MPP24_31              	0x00000000
#define RD_88F6281A_PCAC_MPP32_39              	0x00000000
#define RD_88F6281A_PCAC_MPP40_47              	0x00000000
#define RD_88F6281A_PCAC_MPP48_55              	0x00000000
#define RD_88F6281A_PCAC_OE_LOW                 0x0
#define RD_88F6281A_PCAC_OE_HIGH                0x0
#define RD_88F6281A_PCAC_OE_VAL_LOW             0x0
#define RD_88F6281A_PCAC_OE_VAL_HIGH            0x0

#if defined(MV_NAND)
    #define DB_88F6280A_MPP0_7                 	0x01111111
#else
    #define DB_88F6280A_MPP0_7                 	0x01222222
#endif
#define DB_88F6280A_MPP8_15                   	0x00300011
#define DB_88F6280A_MPP16_23                   	0x00001103
#define DB_88F6280A_MPP24_31                   	0x00000000
#define DB_88F6280A_MPP32_39                   	0x00000000
#define DB_88F6280A_MPP40_47                   	0x00000000
#define DB_88F6280A_MPP48_55                   	0x00000000
#define DB_88F6280A_OE_LOW                      (0xffffffff)
#define DB_88F6280A_OE_HIGH                     0x0
#define DB_88F6280A_OE_VAL_LOW                  0x0
#define DB_88F6280A_OE_VAL_HIGH                 0x0

#define RD_SHEEVA_PLUG_MPP0_7                   0x01111111
#define RD_SHEEVA_PLUG_MPP8_15                  0x11113322
#define RD_SHEEVA_PLUG_MPP16_23                 0x00001111
#define RD_SHEEVA_PLUG_MPP24_31                 0x00100000
#define RD_SHEEVA_PLUG_MPP32_39                 0x00000000
#define RD_SHEEVA_PLUG_MPP40_47                 0x00000000
#define RD_SHEEVA_PLUG_MPP48_55                 0x00000000
#define RD_SHEEVA_PLUG_OE_LOW                   0x0
#define RD_SHEEVA_PLUG_OE_HIGH                  0x0
#define RD_SHEEVA_PLUG_OE_VAL_LOW               (BIT29)
#define RD_SHEEVA_PLUG_OE_VAL_HIGH              ((~(BIT17 | BIT16 | BIT15)) | BIT14) 

#define DB_CUSTOMER_MPP0_7                	    0x21111111
#define DB_CUSTOMER_MPP8_15               	    0x00003311
#define DB_CUSTOMER_MPP16_23              	    0x00001100
#define DB_CUSTOMER_MPP24_31              	    0x00000000
#define DB_CUSTOMER_MPP32_39              	    0x00000000
#define DB_CUSTOMER_MPP40_47              	    0x00000000
#define DB_CUSTOMER_MPP48_55              	    0x00000000
#define DB_CUSTOMER_OE_LOW                      0x0
#define DB_CUSTOMER_OE_HIGH                     (~((BIT6) | (BIT7) | (BIT8) | (BIT9)))
#define DB_CUSTOMER_OE_VAL_LOW                  0x0
#define DB_CUSTOMER_OE_VAL_HIGH                 0x0

#endif  
