#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif

#include "mvCommon.h"
#include "mvBoardEnvLib.h"
#include "mvBoardEnvSpec.h"
#include "twsi/mvTwsi.h"

#define DB_88F6281A_BOARD_PCI_IF_NUM            0x0
#define DB_88F6281A_BOARD_TWSI_DEF_NUM		    0x7
#define DB_88F6281A_BOARD_MAC_INFO_NUM		    0x2
#define DB_88F6281A_BOARD_GPP_INFO_NUM		    0x1
#define DB_88F6281A_BOARD_MPP_CONFIG_NUM		0x1
#define DB_88F6281A_BOARD_MPP_GROUP_TYPE_NUM	0x1
#if defined(MV_NAND) && defined(MV_NAND_BOOT)
    #define DB_88F6281A_BOARD_DEVICE_CONFIG_NUM	    0x1
#elif defined(MV_NAND) && defined(MV_SPI_BOOT)
    #define DB_88F6281A_BOARD_DEVICE_CONFIG_NUM	    0x2
#else
    #define DB_88F6281A_BOARD_DEVICE_CONFIG_NUM	    0x1
#endif
#define DB_88F6281A_BOARD_DEBUG_LED_NUM		    0x0
#define DB_88F6281A_BOARD_NAND_READ_PARAMS		    0x000C0282
#define DB_88F6281A_BOARD_NAND_WRITE_PARAMS		    0x00010305
#define DB_88F6281A_BOARD_NAND_CONTROL		        0x01c00541

MV_BOARD_TWSI_INFO	db88f6281AInfoBoardTwsiDev[] =
	
	{
	{BOARD_DEV_TWSI_EXP, 0x20, ADDR7_BIT},
	{BOARD_DEV_TWSI_EXP, 0x21, ADDR7_BIT},
	{BOARD_DEV_TWSI_EXP, 0x27, ADDR7_BIT},
	{BOARD_DEV_TWSI_SATR, 0x4C, ADDR7_BIT},
	{BOARD_DEV_TWSI_SATR, 0x4D, ADDR7_BIT},
	{BOARD_DEV_TWSI_SATR, 0x4E, ADDR7_BIT},
	{BOARD_TWSI_AUDIO_DEC, 0x4A, ADDR7_BIT}
	};

MV_BOARD_MAC_INFO db88f6281AInfoBoardMacInfo[] = 
	
	{
	{BOARD_MAC_SPEED_AUTO, 0x8},
	{BOARD_MAC_SPEED_AUTO, 0x9}
	}; 

MV_BOARD_MPP_TYPE_INFO db88f6281AInfoBoardMppTypeInfo[] = 
	
	{{MV_BOARD_AUTO, MV_BOARD_AUTO}
	}; 

MV_BOARD_GPP_INFO db88f6281AInfoBoardGppInfo[] = 
	
	{
	{BOARD_GPP_TSU_DIRCTION, 33}
	
	};

MV_DEV_CS_INFO db88f6281AInfoBoardDeCsInfo[] = 
					   
#if defined(MV_NAND) && defined(MV_NAND_BOOT)
		 {{0, N_A, BOARD_DEV_NAND_FLASH, 8}};	            
#elif defined(MV_NAND) && defined(MV_SPI_BOOT)
		 {
         {0, N_A, BOARD_DEV_NAND_FLASH, 8},	   
         {1, N_A, BOARD_DEV_SPI_FLASH, 8},	   
         };
#else
	 {{1, N_A, BOARD_DEV_SPI_FLASH, 8}};	            
#endif

MV_BOARD_MPP_INFO	db88f6281AInfoBoardMppConfigValue[] = 
	{{{
	DB_88F6281A_MPP0_7,		
	DB_88F6281A_MPP8_15,		
	DB_88F6281A_MPP16_23,		
	DB_88F6281A_MPP24_31,		
	DB_88F6281A_MPP32_39,		
	DB_88F6281A_MPP40_47,		
	DB_88F6281A_MPP48_55		
	}}};

MV_BOARD_INFO db88f6281AInfo = {
	"DB-88F6281A-BP",				
	DB_88F6281A_BOARD_MPP_GROUP_TYPE_NUM,		
	db88f6281AInfoBoardMppTypeInfo,
	DB_88F6281A_BOARD_MPP_CONFIG_NUM,		
	db88f6281AInfoBoardMppConfigValue,
	0,						
	0,						
	DB_88F6281A_BOARD_DEVICE_CONFIG_NUM,		
	db88f6281AInfoBoardDeCsInfo,
	DB_88F6281A_BOARD_TWSI_DEF_NUM,			
	db88f6281AInfoBoardTwsiDev,					
	DB_88F6281A_BOARD_MAC_INFO_NUM,			
	db88f6281AInfoBoardMacInfo,
	DB_88F6281A_BOARD_GPP_INFO_NUM,			
	db88f6281AInfoBoardGppInfo,
	DB_88F6281A_BOARD_DEBUG_LED_NUM,			              
	NULL,
	0,								
	DB_88F6281A_OE_LOW,				
	DB_88F6281A_OE_HIGH,				
	DB_88F6281A_OE_VAL_LOW,				
	DB_88F6281A_OE_VAL_HIGH,				
	0,						
	BIT6, 						
	NULL,						
    DB_88F6281A_BOARD_NAND_READ_PARAMS,
    DB_88F6281A_BOARD_NAND_WRITE_PARAMS,
    DB_88F6281A_BOARD_NAND_CONTROL
};

#define RD_88F6281A_BOARD_PCI_IF_NUM		0x0
#define RD_88F6281A_BOARD_TWSI_DEF_NUM		0x2
#define RD_88F6281A_BOARD_MAC_INFO_NUM		0x2
#define RD_88F6281A_BOARD_GPP_INFO_NUM		0x5
#define RD_88F6281A_BOARD_MPP_GROUP_TYPE_NUM	0x1
#define RD_88F6281A_BOARD_MPP_CONFIG_NUM		0x1
#if defined(MV_NAND) && defined(MV_NAND_BOOT)
    #define RD_88F6281A_BOARD_DEVICE_CONFIG_NUM	    0x1
#elif defined(MV_NAND) && defined(MV_SPI_BOOT)
    #define RD_88F6281A_BOARD_DEVICE_CONFIG_NUM	    0x2
#else
    #define RD_88F6281A_BOARD_DEVICE_CONFIG_NUM	    0x1
#endif
#define RD_88F6281A_BOARD_DEBUG_LED_NUM		0x0
#define RD_88F6281A_BOARD_NAND_READ_PARAMS		    0x000C0282
#define RD_88F6281A_BOARD_NAND_WRITE_PARAMS		    0x00010305
#define RD_88F6281A_BOARD_NAND_CONTROL		        0x01c00541

MV_BOARD_MAC_INFO rd88f6281AInfoBoardMacInfo[] = 
	
	{{BOARD_MAC_SPEED_1000M, 0xa},
    {BOARD_MAC_SPEED_AUTO, 0xb}
	}; 

MV_BOARD_SWITCH_INFO rd88f6281AInfoBoardSwitchInfo[] = 
	
	{{38, {0, 1, 2, 3, -1}, 5, 2, 0},
	 {-1, {-1}, -1, -1, -1}};

MV_BOARD_TWSI_INFO	rd88f6281AInfoBoardTwsiDev[] =
	
	{
	{BOARD_DEV_TWSI_EXP, 0xFF, ADDR7_BIT}, 
	{BOARD_DEV_TWSI_EXP, 0x27, ADDR7_BIT}
	};

MV_BOARD_MPP_TYPE_INFO rd88f6281AInfoBoardMppTypeInfo[] = 
	{{MV_BOARD_RGMII, MV_BOARD_TDM}
	}; 

MV_DEV_CS_INFO rd88f6281AInfoBoardDeCsInfo[] = 
					   
#if defined(MV_NAND) && defined(MV_NAND_BOOT)
		 {{0, N_A, BOARD_DEV_NAND_FLASH, 8}};	   
#elif defined(MV_NAND) && defined(MV_SPI_BOOT)
		 {
         {0, N_A, BOARD_DEV_NAND_FLASH, 8},	   
         {1, N_A, BOARD_DEV_SPI_FLASH, 8},	   
         };
#else
		 {{1, N_A, BOARD_DEV_SPI_FLASH, 8}};	            
#endif

MV_BOARD_GPP_INFO rd88f6281AInfoBoardGppInfo[] = 
	
	{{BOARD_GPP_SDIO_DETECT, 28},
    {BOARD_GPP_USB_OC, 29},
    {BOARD_GPP_WPS_BUTTON, 35},
    {BOARD_GPP_MV_SWITCH, 38},
    {BOARD_GPP_USB_VBUS, 49}
	};

MV_BOARD_MPP_INFO	rd88f6281AInfoBoardMppConfigValue[] = 
	{{{
	RD_88F6281A_MPP0_7,		
	RD_88F6281A_MPP8_15,		
	RD_88F6281A_MPP16_23,		
	RD_88F6281A_MPP24_31,		
	RD_88F6281A_MPP32_39,		
	RD_88F6281A_MPP40_47,		
	RD_88F6281A_MPP48_55		
	}}};

MV_BOARD_INFO rd88f6281AInfo = {
	"RD-88F6281A",				
	RD_88F6281A_BOARD_MPP_GROUP_TYPE_NUM,		
	rd88f6281AInfoBoardMppTypeInfo,
	RD_88F6281A_BOARD_MPP_CONFIG_NUM,		
	rd88f6281AInfoBoardMppConfigValue,
	0,						
	(1 << 3),					
	RD_88F6281A_BOARD_DEVICE_CONFIG_NUM,		
	rd88f6281AInfoBoardDeCsInfo,
	RD_88F6281A_BOARD_TWSI_DEF_NUM,			
	rd88f6281AInfoBoardTwsiDev,					
	RD_88F6281A_BOARD_MAC_INFO_NUM,			
	rd88f6281AInfoBoardMacInfo,
	RD_88F6281A_BOARD_GPP_INFO_NUM,			
	rd88f6281AInfoBoardGppInfo,
	RD_88F6281A_BOARD_DEBUG_LED_NUM,			              
	NULL,
	0,												
	RD_88F6281A_OE_LOW,				
	RD_88F6281A_OE_HIGH,				
	RD_88F6281A_OE_VAL_LOW,				
	RD_88F6281A_OE_VAL_HIGH,				
	0,						
	BIT6, 						
	rd88f6281AInfoBoardSwitchInfo,			
    RD_88F6281A_BOARD_NAND_READ_PARAMS,
    RD_88F6281A_BOARD_NAND_WRITE_PARAMS,
    RD_88F6281A_BOARD_NAND_CONTROL
};

#define DB_88F6192A_BOARD_PCI_IF_NUM            0x0
#define DB_88F6192A_BOARD_TWSI_DEF_NUM		    0x7
#define DB_88F6192A_BOARD_MAC_INFO_NUM		    0x2
#define DB_88F6192A_BOARD_GPP_INFO_NUM		    0x3
#define DB_88F6192A_BOARD_MPP_GROUP_TYPE_NUM	0x1
#define DB_88F6192A_BOARD_MPP_CONFIG_NUM		0x1
#if defined(MV_NAND) && defined(MV_NAND_BOOT)
    #define DB_88F6192A_BOARD_DEVICE_CONFIG_NUM	    0x1
#elif defined(MV_NAND) && defined(MV_SPI_BOOT)
    #define DB_88F6192A_BOARD_DEVICE_CONFIG_NUM	    0x2
#else
    #define DB_88F6192A_BOARD_DEVICE_CONFIG_NUM	    0x1
#endif
#define DB_88F6192A_BOARD_DEBUG_LED_NUM		    0x0
#define DB_88F6192A_BOARD_NAND_READ_PARAMS		    0x000C0282
#define DB_88F6192A_BOARD_NAND_WRITE_PARAMS		    0x00010305
#define DB_88F6192A_BOARD_NAND_CONTROL		        0x01c00541

MV_BOARD_TWSI_INFO	db88f6192AInfoBoardTwsiDev[] =
	
	{
	{BOARD_DEV_TWSI_EXP, 0x20, ADDR7_BIT},
	{BOARD_DEV_TWSI_EXP, 0x21, ADDR7_BIT},
	{BOARD_DEV_TWSI_EXP, 0x27, ADDR7_BIT},
	{BOARD_DEV_TWSI_SATR, 0x4C, ADDR7_BIT},
	{BOARD_DEV_TWSI_SATR, 0x4D, ADDR7_BIT},
	{BOARD_DEV_TWSI_SATR, 0x4E, ADDR7_BIT},
	{BOARD_TWSI_AUDIO_DEC, 0x4A, ADDR7_BIT}
	};

MV_BOARD_MAC_INFO db88f6192AInfoBoardMacInfo[] = 
	
	{
	{BOARD_MAC_SPEED_AUTO, 0x8},
	{BOARD_MAC_SPEED_AUTO, 0x9}
	}; 

MV_BOARD_MPP_TYPE_INFO db88f6192AInfoBoardMppTypeInfo[] = 
	
	{{MV_BOARD_AUTO, MV_BOARD_OTHER}
	}; 

MV_DEV_CS_INFO db88f6192AInfoBoardDeCsInfo[] = 
					   
#if defined(MV_NAND) && defined(MV_NAND_BOOT)
		 {{0, N_A, BOARD_DEV_NAND_FLASH, 8}};	   
#elif defined(MV_NAND) && defined(MV_SPI_BOOT)
		 {
         {0, N_A, BOARD_DEV_NAND_FLASH, 8},	   
         {1, N_A, BOARD_DEV_SPI_FLASH, 8},	   
         };
#else
		 {{1, N_A, BOARD_DEV_SPI_FLASH, 8}};	            
#endif

MV_BOARD_GPP_INFO db88f6192AInfoBoardGppInfo[] = 
	
	{
    {BOARD_GPP_SDIO_WP, 20},
	{BOARD_GPP_USB_VBUS, 22},
	{BOARD_GPP_SDIO_DETECT, 23},
	};

MV_BOARD_MPP_INFO	db88f6192AInfoBoardMppConfigValue[] = 
	{{{
	DB_88F6192A_MPP0_7,		
	DB_88F6192A_MPP8_15,		
	DB_88F6192A_MPP16_23,		
	DB_88F6192A_MPP24_31,		
	DB_88F6192A_MPP32_35
	}}};

MV_BOARD_INFO db88f6192AInfo = {
	"DB-88F6192A-BP",				
	DB_88F6192A_BOARD_MPP_GROUP_TYPE_NUM,		
	db88f6192AInfoBoardMppTypeInfo,
	DB_88F6192A_BOARD_MPP_CONFIG_NUM,		
	db88f6192AInfoBoardMppConfigValue,
	0,						
	(1 << 3),					
	DB_88F6192A_BOARD_DEVICE_CONFIG_NUM,		
	db88f6192AInfoBoardDeCsInfo,
	DB_88F6192A_BOARD_TWSI_DEF_NUM,			
	db88f6192AInfoBoardTwsiDev,					
	DB_88F6192A_BOARD_MAC_INFO_NUM,			
	db88f6192AInfoBoardMacInfo,
	DB_88F6192A_BOARD_GPP_INFO_NUM,			
	db88f6192AInfoBoardGppInfo,
	DB_88F6192A_BOARD_DEBUG_LED_NUM,			              
	NULL,
	0,												
	DB_88F6192A_OE_LOW,				
	DB_88F6192A_OE_HIGH,				
	DB_88F6192A_OE_VAL_LOW,				
	DB_88F6192A_OE_VAL_HIGH,				
	0,						
	0, 						
	NULL,						
    DB_88F6192A_BOARD_NAND_READ_PARAMS,
    DB_88F6192A_BOARD_NAND_WRITE_PARAMS,
    DB_88F6192A_BOARD_NAND_CONTROL
};

MV_BOARD_INFO db88f6701AInfo = {
	"DB-88F6701A-BP",				
	DB_88F6192A_BOARD_MPP_GROUP_TYPE_NUM,		
	db88f6192AInfoBoardMppTypeInfo,
	DB_88F6192A_BOARD_MPP_CONFIG_NUM,		
	db88f6192AInfoBoardMppConfigValue,
	0,						
	(1 << 3),					
	DB_88F6192A_BOARD_DEVICE_CONFIG_NUM,		
	db88f6192AInfoBoardDeCsInfo,
	DB_88F6192A_BOARD_TWSI_DEF_NUM,			
	db88f6192AInfoBoardTwsiDev,					
	DB_88F6192A_BOARD_MAC_INFO_NUM,			
	db88f6192AInfoBoardMacInfo,
	DB_88F6192A_BOARD_GPP_INFO_NUM,			
	db88f6192AInfoBoardGppInfo,
	DB_88F6192A_BOARD_DEBUG_LED_NUM,			              
	NULL,
	0,												
	DB_88F6192A_OE_LOW,				
	DB_88F6192A_OE_HIGH,				
	DB_88F6192A_OE_VAL_LOW,				
	DB_88F6192A_OE_VAL_HIGH,				
	0,						
	0, 						
	NULL,						
    DB_88F6192A_BOARD_NAND_READ_PARAMS,
    DB_88F6192A_BOARD_NAND_WRITE_PARAMS,
    DB_88F6192A_BOARD_NAND_CONTROL
};

MV_BOARD_INFO db88f6702AInfo = {
	"DB-88F6702A-BP",				
	DB_88F6192A_BOARD_MPP_GROUP_TYPE_NUM,		
	db88f6192AInfoBoardMppTypeInfo,
	DB_88F6192A_BOARD_MPP_CONFIG_NUM,		
	db88f6192AInfoBoardMppConfigValue,
	0,						
	(1 << 3),					
	DB_88F6192A_BOARD_DEVICE_CONFIG_NUM,		
	db88f6192AInfoBoardDeCsInfo,
	DB_88F6192A_BOARD_TWSI_DEF_NUM,			
	db88f6192AInfoBoardTwsiDev,					
	DB_88F6192A_BOARD_MAC_INFO_NUM,			
	db88f6192AInfoBoardMacInfo,
	DB_88F6192A_BOARD_GPP_INFO_NUM,			
	db88f6192AInfoBoardGppInfo,
	DB_88F6192A_BOARD_DEBUG_LED_NUM,			              
	NULL,
	0,												
	DB_88F6192A_OE_LOW,				
	DB_88F6192A_OE_HIGH,				
	DB_88F6192A_OE_VAL_LOW,				
	DB_88F6192A_OE_VAL_HIGH,				
	0,						
	0, 						
	NULL,						
    DB_88F6192A_BOARD_NAND_READ_PARAMS,
    DB_88F6192A_BOARD_NAND_WRITE_PARAMS,
    DB_88F6192A_BOARD_NAND_CONTROL
};

#define DB_88F6190A_BOARD_MAC_INFO_NUM		0x1

MV_BOARD_INFO db88f6190AInfo = {
	"DB-88F6190A-BP",				
	DB_88F6192A_BOARD_MPP_GROUP_TYPE_NUM,		
	db88f6192AInfoBoardMppTypeInfo,
	DB_88F6192A_BOARD_MPP_CONFIG_NUM,		
	db88f6192AInfoBoardMppConfigValue,
	0,						
	(1 << 3),					
	DB_88F6192A_BOARD_DEVICE_CONFIG_NUM,		
	db88f6192AInfoBoardDeCsInfo,
	DB_88F6192A_BOARD_TWSI_DEF_NUM,			
	db88f6192AInfoBoardTwsiDev,					
	DB_88F6190A_BOARD_MAC_INFO_NUM,			
	db88f6192AInfoBoardMacInfo,
	DB_88F6192A_BOARD_GPP_INFO_NUM,			
	db88f6192AInfoBoardGppInfo,
	DB_88F6192A_BOARD_DEBUG_LED_NUM,			              
	NULL,
	0,												
	DB_88F6192A_OE_LOW,				
	DB_88F6192A_OE_HIGH,				
	DB_88F6192A_OE_VAL_LOW,				
	DB_88F6192A_OE_VAL_HIGH,				
	0,						
	0, 						
	NULL,						
    DB_88F6192A_BOARD_NAND_READ_PARAMS,
    DB_88F6192A_BOARD_NAND_WRITE_PARAMS,
    DB_88F6192A_BOARD_NAND_CONTROL
};

#define RD_88F6192A_BOARD_PCI_IF_NUM		0x0
#define RD_88F6192A_BOARD_TWSI_DEF_NUM		0x0
#define RD_88F6192A_BOARD_MAC_INFO_NUM		0x1
#define RD_88F6192A_BOARD_GPP_INFO_NUM		0xE
#define RD_88F6192A_BOARD_MPP_GROUP_TYPE_NUM	0x1
#define RD_88F6192A_BOARD_MPP_CONFIG_NUM		0x1
#define RD_88F6192A_BOARD_DEVICE_CONFIG_NUM	0x1
#define RD_88F6192A_BOARD_DEBUG_LED_NUM		0x3
#define RD_88F6192A_BOARD_NAND_READ_PARAMS		    0x000C0282
#define RD_88F6192A_BOARD_NAND_WRITE_PARAMS		    0x00010305
#define RD_88F6192A_BOARD_NAND_CONTROL		        0x01c00541

MV_U8	rd88f6192AInfoBoardDebugLedIf[] =
	{17, 28, 29};

MV_BOARD_MAC_INFO rd88f6192AInfoBoardMacInfo[] = 
	
	{{BOARD_MAC_SPEED_AUTO, 0x8}
	}; 

MV_BOARD_MPP_TYPE_INFO rd88f6192AInfoBoardMppTypeInfo[] = 
	
	{{MV_BOARD_OTHER, MV_BOARD_OTHER}
	}; 

MV_DEV_CS_INFO rd88f6192AInfoBoardDeCsInfo[] = 
					   
		 {{1, N_A, BOARD_DEV_SPI_FLASH, 8}};	   

MV_BOARD_GPP_INFO rd88f6192AInfoBoardGppInfo[] = 
	
	{
	{BOARD_GPP_USB_VBUS_EN, 10},
	{BOARD_GPP_USB_HOST_DEVICE, 11},
	{BOARD_GPP_RESET, 14},
	{BOARD_GPP_POWER_ON_LED, 15},
	{BOARD_GPP_HDD_POWER, 16},
	{BOARD_GPP_WPS_BUTTON, 24},
	{BOARD_GPP_TS_BUTTON_C, 25},
	{BOARD_GPP_USB_VBUS, 26},
	{BOARD_GPP_USB_OC, 27},
	{BOARD_GPP_TS_BUTTON_U, 30},
	{BOARD_GPP_TS_BUTTON_R, 31},
	{BOARD_GPP_TS_BUTTON_L, 32},
	{BOARD_GPP_TS_BUTTON_D, 34},
	{BOARD_GPP_FAN_POWER, 35}
	};

MV_BOARD_MPP_INFO	rd88f6192AInfoBoardMppConfigValue[] = 
	{{{
	RD_88F6192A_MPP0_7,		
	RD_88F6192A_MPP8_15,		
	RD_88F6192A_MPP16_23,		
	RD_88F6192A_MPP24_31,		
	RD_88F6192A_MPP32_35
	}}};

MV_BOARD_INFO rd88f6192AInfo = {
	"RD-88F6192A-NAS",				
	RD_88F6192A_BOARD_MPP_GROUP_TYPE_NUM,		
	rd88f6192AInfoBoardMppTypeInfo,
	RD_88F6192A_BOARD_MPP_CONFIG_NUM,		
	rd88f6192AInfoBoardMppConfigValue,
	0,						
	(1 << 3),					
	RD_88F6192A_BOARD_DEVICE_CONFIG_NUM,		
	rd88f6192AInfoBoardDeCsInfo,
	RD_88F6192A_BOARD_TWSI_DEF_NUM,			
	NULL,					
	RD_88F6192A_BOARD_MAC_INFO_NUM,			
	rd88f6192AInfoBoardMacInfo,
	RD_88F6192A_BOARD_GPP_INFO_NUM,			
	rd88f6192AInfoBoardGppInfo,
	RD_88F6192A_BOARD_DEBUG_LED_NUM,			              
	rd88f6192AInfoBoardDebugLedIf,
	0,												
	RD_88F6192A_OE_LOW,				
	RD_88F6192A_OE_HIGH,				
	RD_88F6192A_OE_VAL_LOW,				
	RD_88F6192A_OE_VAL_HIGH,				
	0,						
	0, 						
	NULL,						
    RD_88F6192A_BOARD_NAND_READ_PARAMS,
    RD_88F6192A_BOARD_NAND_WRITE_PARAMS,
    RD_88F6192A_BOARD_NAND_CONTROL
};

MV_BOARD_INFO rd88f6190AInfo = {
	"RD-88F6190A-NAS",				
	RD_88F6192A_BOARD_MPP_GROUP_TYPE_NUM,		
	rd88f6192AInfoBoardMppTypeInfo,
	RD_88F6192A_BOARD_MPP_CONFIG_NUM,		
	rd88f6192AInfoBoardMppConfigValue,
	0,						
	(1 << 3),					
	RD_88F6192A_BOARD_DEVICE_CONFIG_NUM,		
	rd88f6192AInfoBoardDeCsInfo,
	RD_88F6192A_BOARD_TWSI_DEF_NUM,			
	NULL,					
	RD_88F6192A_BOARD_MAC_INFO_NUM,			
	rd88f6192AInfoBoardMacInfo,
	RD_88F6192A_BOARD_GPP_INFO_NUM,			
	rd88f6192AInfoBoardGppInfo,
	RD_88F6192A_BOARD_DEBUG_LED_NUM,			              
	rd88f6192AInfoBoardDebugLedIf,
	0,												
	RD_88F6192A_OE_LOW,				
	RD_88F6192A_OE_HIGH,				
	RD_88F6192A_OE_VAL_LOW,				
	RD_88F6192A_OE_VAL_HIGH,				
	0,						
	0, 						
	NULL,						
    RD_88F6192A_BOARD_NAND_READ_PARAMS,
    RD_88F6192A_BOARD_NAND_WRITE_PARAMS,
    RD_88F6192A_BOARD_NAND_CONTROL
};

#define DB_88F6180A_BOARD_PCI_IF_NUM		0x0
#define DB_88F6180A_BOARD_TWSI_DEF_NUM		0x5
#define DB_88F6180A_BOARD_MAC_INFO_NUM		0x1
#define DB_88F6180A_BOARD_GPP_INFO_NUM		0x0
#define DB_88F6180A_BOARD_MPP_GROUP_TYPE_NUM	0x2
#define DB_88F6180A_BOARD_MPP_CONFIG_NUM		0x1
#define DB_88F6180A_BOARD_DEVICE_CONFIG_NUM	    0x1
#define DB_88F6180A_BOARD_DEBUG_LED_NUM		0x0
#define DB_88F6180A_BOARD_NAND_READ_PARAMS		    0x000C0282
#define DB_88F6180A_BOARD_NAND_WRITE_PARAMS		    0x00010305
#define DB_88F6180A_BOARD_NAND_CONTROL		        0x01c00541

MV_BOARD_TWSI_INFO	db88f6180AInfoBoardTwsiDev[] =
	
	{
    {BOARD_DEV_TWSI_EXP, 0x20, ADDR7_BIT},
    {BOARD_DEV_TWSI_EXP, 0x21, ADDR7_BIT},
    {BOARD_DEV_TWSI_EXP, 0x27, ADDR7_BIT},
	{BOARD_DEV_TWSI_SATR, 0x4C, ADDR7_BIT},
	{BOARD_TWSI_AUDIO_DEC, 0x4A, ADDR7_BIT}
	};

MV_BOARD_MAC_INFO db88f6180AInfoBoardMacInfo[] = 
	
	{{BOARD_MAC_SPEED_AUTO, 0x8}
	}; 

MV_BOARD_GPP_INFO db88f6180AInfoBoardGppInfo[] = 
	
	{
	
	};

MV_BOARD_MPP_TYPE_INFO db88f6180AInfoBoardMppTypeInfo[] = 
	
	{{MV_BOARD_OTHER, MV_BOARD_AUTO}
	}; 

MV_DEV_CS_INFO db88f6180AInfoBoardDeCsInfo[] = 
					   
#if defined(MV_NAND_BOOT)
		 {{0, N_A, BOARD_DEV_NAND_FLASH, 8}};	            
#else
		 {{1, N_A, BOARD_DEV_SPI_FLASH, 8}};	            
#endif

MV_BOARD_MPP_INFO	db88f6180AInfoBoardMppConfigValue[] = 
	{{{
	DB_88F6180A_MPP0_7,		
	DB_88F6180A_MPP8_15,
    DB_88F6180A_MPP16_23,
    DB_88F6180A_MPP24_31,		
    DB_88F6180A_MPP32_39,
    DB_88F6180A_MPP40_44
	}}};

MV_BOARD_INFO db88f6180AInfo = {
	"DB-88F6180A-BP",				
	DB_88F6180A_BOARD_MPP_GROUP_TYPE_NUM,		
	db88f6180AInfoBoardMppTypeInfo,
	DB_88F6180A_BOARD_MPP_CONFIG_NUM,		
	db88f6180AInfoBoardMppConfigValue,
	0,						
	0,					
	DB_88F6180A_BOARD_DEVICE_CONFIG_NUM,		
	db88f6180AInfoBoardDeCsInfo,
	DB_88F6180A_BOARD_TWSI_DEF_NUM,			
	db88f6180AInfoBoardTwsiDev,					
	DB_88F6180A_BOARD_MAC_INFO_NUM,			
	db88f6180AInfoBoardMacInfo,
	DB_88F6180A_BOARD_GPP_INFO_NUM,			
	NULL,
	DB_88F6180A_BOARD_DEBUG_LED_NUM,			              
	NULL,
	0,												
	DB_88F6180A_OE_LOW,				
	DB_88F6180A_OE_HIGH,				
	DB_88F6180A_OE_VAL_LOW,				
	DB_88F6180A_OE_VAL_HIGH,				
	0,						
	0, 						
	NULL,						
    DB_88F6180A_BOARD_NAND_READ_PARAMS,
    DB_88F6180A_BOARD_NAND_WRITE_PARAMS,
    DB_88F6180A_BOARD_NAND_CONTROL
};

#define RD_88F6281A_PCAC_BOARD_PCI_IF_NUM		0x0
#define RD_88F6281A_PCAC_BOARD_TWSI_DEF_NUM		0x1
#define RD_88F6281A_PCAC_BOARD_MAC_INFO_NUM		0x1
#define RD_88F6281A_PCAC_BOARD_GPP_INFO_NUM		0x0
#define RD_88F6281A_PCAC_BOARD_MPP_GROUP_TYPE_NUM	0x1
#define RD_88F6281A_PCAC_BOARD_MPP_CONFIG_NUM		0x1
#if defined(MV_NAND) && defined(MV_NAND_BOOT)
    #define RD_88F6281A_PCAC_BOARD_DEVICE_CONFIG_NUM	    0x1
#elif defined(MV_NAND) && defined(MV_SPI_BOOT)
    #define RD_88F6281A_PCAC_BOARD_DEVICE_CONFIG_NUM	    0x2
#else
    #define RD_88F6281A_PCAC_BOARD_DEVICE_CONFIG_NUM	    0x1
#endif
#define RD_88F6281A_PCAC_BOARD_DEBUG_LED_NUM		0x4
#define RD_88F6281A_PCAC_BOARD_NAND_READ_PARAMS		    0x000C0282
#define RD_88F6281A_PCAC_BOARD_NAND_WRITE_PARAMS		    0x00010305
#define RD_88F6281A_PCAC_BOARD_NAND_CONTROL		        0x01c00541

MV_U8	rd88f6281APcacInfoBoardDebugLedIf[] =
	{38, 39, 40, 41};

MV_BOARD_MAC_INFO rd88f6281APcacInfoBoardMacInfo[] = 
	
	{{BOARD_MAC_SPEED_AUTO, 0x8}
	}; 

MV_BOARD_TWSI_INFO	rd88f6281APcacInfoBoardTwsiDev[] =
	
	{
	{BOARD_TWSI_OTHER, 0xa7, ADDR7_BIT}
	};

MV_BOARD_MPP_TYPE_INFO rd88f6281APcacInfoBoardMppTypeInfo[] = 
	{{MV_BOARD_OTHER, MV_BOARD_OTHER}
	}; 

MV_DEV_CS_INFO rd88f6281APcacInfoBoardDeCsInfo[] = 
					   
#if defined(MV_NAND) && defined(MV_NAND_BOOT)
		 {{0, N_A, BOARD_DEV_NAND_FLASH, 8}};	   
#elif defined(MV_NAND) && defined(MV_SPI_BOOT)
		 {
         {0, N_A, BOARD_DEV_NAND_FLASH, 8},	   
         {1, N_A, BOARD_DEV_SPI_FLASH, 8},	   
         };
#else
	 {{1, N_A, BOARD_DEV_SPI_FLASH, 8}};	            
#endif

MV_BOARD_MPP_INFO	rd88f6281APcacInfoBoardMppConfigValue[] = 
	{{{
	RD_88F6281A_PCAC_MPP0_7,		
	RD_88F6281A_PCAC_MPP8_15,		
	RD_88F6281A_PCAC_MPP16_23,		
	RD_88F6281A_PCAC_MPP24_31,		
	RD_88F6281A_PCAC_MPP32_39,		
	RD_88F6281A_PCAC_MPP40_47,		
	RD_88F6281A_PCAC_MPP48_55		
	}}};

MV_BOARD_INFO rd88f6281APcacInfo = {
	"RD-88F6281A-PCAC",				
	RD_88F6281A_PCAC_BOARD_MPP_GROUP_TYPE_NUM,	
	rd88f6281APcacInfoBoardMppTypeInfo,
	RD_88F6281A_PCAC_BOARD_MPP_CONFIG_NUM,		
	rd88f6281APcacInfoBoardMppConfigValue,
	0,						
	(1 << 3),					
	RD_88F6281A_PCAC_BOARD_DEVICE_CONFIG_NUM,	
	rd88f6281APcacInfoBoardDeCsInfo,
	RD_88F6281A_PCAC_BOARD_TWSI_DEF_NUM,		
	rd88f6281APcacInfoBoardTwsiDev,					
	RD_88F6281A_PCAC_BOARD_MAC_INFO_NUM,		
	rd88f6281APcacInfoBoardMacInfo,
	RD_88F6281A_PCAC_BOARD_GPP_INFO_NUM,		
	0,
	RD_88F6281A_PCAC_BOARD_DEBUG_LED_NUM,		              
	NULL,
	0,												
	RD_88F6281A_PCAC_OE_LOW,			
	RD_88F6281A_PCAC_OE_HIGH,			
	RD_88F6281A_PCAC_OE_VAL_LOW,			
	RD_88F6281A_PCAC_OE_VAL_HIGH,			
	0,						
	0, 	 					
	NULL,						
    RD_88F6281A_PCAC_BOARD_NAND_READ_PARAMS,
    RD_88F6281A_PCAC_BOARD_NAND_WRITE_PARAMS,
    RD_88F6281A_PCAC_BOARD_NAND_CONTROL
};

#define DB_88F6280A_BOARD_PCI_IF_NUM            0x0
#define DB_88F6280A_BOARD_TWSI_DEF_NUM		    0x7
#define DB_88F6280A_BOARD_MAC_INFO_NUM		    0x1
#define DB_88F6280A_BOARD_GPP_INFO_NUM		    0x0
#define DB_88F6280A_BOARD_MPP_CONFIG_NUM		0x1
#define DB_88F6280A_BOARD_MPP_GROUP_TYPE_NUM	0x1
#if defined(MV_NAND) && defined(MV_NAND_BOOT)
    #define DB_88F6280A_BOARD_DEVICE_CONFIG_NUM	    0x1
#elif defined(MV_NAND) && defined(MV_SPI_BOOT)
    #define DB_88F6280A_BOARD_DEVICE_CONFIG_NUM	    0x2
#else
    #define DB_88F6280A_BOARD_DEVICE_CONFIG_NUM	    0x1
#endif
#define DB_88F6280A_BOARD_DEBUG_LED_NUM		    0x0
#define DB_88F6280A_BOARD_NAND_READ_PARAMS		    0x000C0282
#define DB_88F6280A_BOARD_NAND_WRITE_PARAMS		    0x00010305
#define DB_88F6280A_BOARD_NAND_CONTROL		        0x01c00541

MV_BOARD_TWSI_INFO	db88f6280AInfoBoardTwsiDev[] =
	
	{
	{BOARD_DEV_TWSI_EXP, 0x20, ADDR7_BIT},
	{BOARD_DEV_TWSI_EXP, 0x21, ADDR7_BIT},
	{BOARD_DEV_TWSI_EXP, 0x27, ADDR7_BIT},
	{BOARD_DEV_TWSI_SATR, 0x4C, ADDR7_BIT},
	{BOARD_DEV_TWSI_SATR, 0x4D, ADDR7_BIT},
	{BOARD_DEV_TWSI_SATR, 0x4E, ADDR7_BIT},
	{BOARD_TWSI_AUDIO_DEC, 0x4A, ADDR7_BIT}
	};

MV_BOARD_MAC_INFO db88f6280AInfoBoardMacInfo[] = 
	
	{
	{BOARD_MAC_SPEED_AUTO, 0x8}
	}; 

MV_BOARD_MPP_TYPE_INFO db88f6280AInfoBoardMppTypeInfo[] = 
	
	{{MV_BOARD_AUTO, MV_BOARD_OTHER}
	}; 

MV_DEV_CS_INFO db88f6280AInfoBoardDeCsInfo[] = 
					   
#if defined(MV_NAND) && defined(MV_NAND_BOOT)
		 {{0, N_A, BOARD_DEV_NAND_FLASH, 8}};	            
#elif defined(MV_NAND) && defined(MV_SPI_BOOT)
		 {
         {0, N_A, BOARD_DEV_NAND_FLASH, 8},	   
         {1, N_A, BOARD_DEV_SPI_FLASH, 8},	   
         };
#else
	 {{0, N_A, BOARD_DEV_SPI_FLASH, 8}};	            
#endif

MV_BOARD_MPP_INFO	db88f6280AInfoBoardMppConfigValue[] = 
	{{{
	DB_88F6280A_MPP0_7,		
	DB_88F6280A_MPP8_15,		
	DB_88F6280A_MPP16_23,		
	DB_88F6280A_MPP24_31,		
	DB_88F6280A_MPP32_39,		
	DB_88F6280A_MPP40_47,		
	DB_88F6280A_MPP48_55		
	}}};

MV_BOARD_INFO db88f6280AInfo = {
	"DB-88F6280A-BP",				
	DB_88F6280A_BOARD_MPP_GROUP_TYPE_NUM,		
	db88f6280AInfoBoardMppTypeInfo,
	DB_88F6280A_BOARD_MPP_CONFIG_NUM,		
	db88f6280AInfoBoardMppConfigValue,
	0,						
	0,						
	DB_88F6280A_BOARD_DEVICE_CONFIG_NUM,		
	db88f6280AInfoBoardDeCsInfo,
	DB_88F6280A_BOARD_TWSI_DEF_NUM,			
	db88f6280AInfoBoardTwsiDev,					
	DB_88F6280A_BOARD_MAC_INFO_NUM,			
	db88f6280AInfoBoardMacInfo,
	DB_88F6280A_BOARD_GPP_INFO_NUM,			
	NULL,
	DB_88F6280A_BOARD_DEBUG_LED_NUM,			              
	NULL,
	0,								
	DB_88F6280A_OE_LOW,				
	DB_88F6280A_OE_HIGH,				
	DB_88F6280A_OE_VAL_LOW,				
	DB_88F6280A_OE_VAL_HIGH,				
	0,						
	BIT6, 						
	NULL,						
    DB_88F6280A_BOARD_NAND_READ_PARAMS,
    DB_88F6280A_BOARD_NAND_WRITE_PARAMS,
    DB_88F6280A_BOARD_NAND_CONTROL
};

#define RD_88F6282A_BOARD_PCI_IF_NUM            	0x0
#define RD_88F6282A_BOARD_TWSI_DEF_NUM		0x0
#define RD_88F6282A_BOARD_MAC_INFO_NUM		0x2
#define RD_88F6282A_BOARD_GPP_INFO_NUM		0x5
#define RD_88F6282A_BOARD_MPP_CONFIG_NUM		0x1
#define RD_88F6282A_BOARD_MPP_GROUP_TYPE_NUM	0x1
#define RD_88F6282A_BOARD_DEVICE_CONFIG_NUM	0x1
#define RD_88F6282A_BOARD_NAND_READ_PARAMS	0x000C0282
#define RD_88F6282A_BOARD_NAND_WRITE_PARAMS	0x00010305
#define RD_88F6282A_BOARD_NAND_CONTROL		0x01c00541

MV_BOARD_TWSI_INFO	rd88f6282aInfoBoardTwsiDev[] =
	
	{
	};

MV_BOARD_MAC_INFO rd88f6282aInfoBoardMacInfo[] = 
	
	{
	{BOARD_MAC_SPEED_AUTO, 0x0},
	{BOARD_MAC_SPEED_1000M, 0x10}
	}; 

MV_BOARD_MPP_TYPE_INFO rd88f6282aInfoBoardMppTypeInfo[] = 
	
	{{MV_BOARD_RGMII, MV_BOARD_TDM}
	}; 

MV_BOARD_GPP_INFO rd88f6282aInfoBoardGppInfo[] = 
	
	{{BOARD_GPP_WPS_BUTTON, 29},
	{BOARD_GPP_HDD_POWER, 35},
    	{BOARD_GPP_FAN_POWER, 34},
    	{BOARD_GPP_USB_VBUS, 37},
    	{BOARD_GPP_USB_VBUS_EN, 37}
	};

MV_DEV_CS_INFO rd88f6282aInfoBoardDeCsInfo[] = 
					   
		 {{0, N_A, BOARD_DEV_NAND_FLASH, 8}};	            

MV_BOARD_MPP_INFO	rd88f6282aInfoBoardMppConfigValue[] = 
	{{{
	RD_88F6282A_MPP0_7,		
	RD_88F6282A_MPP8_15,		
	RD_88F6282A_MPP16_23,		
	RD_88F6282A_MPP24_31,		
	RD_88F6282A_MPP32_39,		
	RD_88F6282A_MPP40_47,		
	RD_88F6282A_MPP48_55		
	}}};

MV_BOARD_SWITCH_INFO rd88f6282aInfoBoardSwitchInfo[] = 
	
	 {{-1, {-1}, -1, -1, -1},
	{38, {0, 1, 2, 3, -1}, 5, 2, 1}}; 

MV_BOARD_INFO rd88f6282aInfo = {
	"RD-88F6282A",					
	RD_88F6282A_BOARD_MPP_GROUP_TYPE_NUM,		
	rd88f6282aInfoBoardMppTypeInfo,
	RD_88F6282A_BOARD_MPP_CONFIG_NUM,		
	rd88f6282aInfoBoardMppConfigValue,
	0,						
	BIT6,						
	RD_88F6282A_BOARD_DEVICE_CONFIG_NUM,		
	rd88f6282aInfoBoardDeCsInfo,
	RD_88F6282A_BOARD_TWSI_DEF_NUM,			
	rd88f6282aInfoBoardTwsiDev,					
	RD_88F6282A_BOARD_MAC_INFO_NUM,			
	rd88f6282aInfoBoardMacInfo,
	RD_88F6282A_BOARD_GPP_INFO_NUM,			
	rd88f6282aInfoBoardGppInfo,
	0,						              
	NULL,
	0,								
	RD_88F6282A_OE_LOW,				
	RD_88F6282A_OE_HIGH,				
	RD_88F6282A_OE_VAL_LOW,				
	RD_88F6282A_OE_VAL_HIGH,				
	BIT29,						
	BIT6, 						
	rd88f6282aInfoBoardSwitchInfo,			
    	RD_88F6282A_BOARD_NAND_READ_PARAMS,
    	RD_88F6282A_BOARD_NAND_WRITE_PARAMS,
    	RD_88F6282A_BOARD_NAND_CONTROL
};

#define DB_88F6282A_BOARD_PCI_IF_NUM            0x0
#define DB_88F6282A_BOARD_TWSI_DEF_NUM		    0x7
#define DB_88F6282A_BOARD_MAC_INFO_NUM		    0x2
#define DB_88F6282A_BOARD_GPP_INFO_NUM		    0x1
#define DB_88F6282A_BOARD_MPP_CONFIG_NUM		0x1
#define DB_88F6282A_BOARD_MPP_GROUP_TYPE_NUM	0x1
#if defined(MV_NAND) && defined(MV_NAND_BOOT)
    #define DB_88F6282A_BOARD_DEVICE_CONFIG_NUM	    0x1
#elif defined(MV_NAND) && defined(MV_SPI_BOOT)
    #define DB_88F6282A_BOARD_DEVICE_CONFIG_NUM	    0x2
#else
    #define DB_88F6282A_BOARD_DEVICE_CONFIG_NUM	    0x1
#endif
#define DB_88F6282A_BOARD_DEBUG_LED_NUM		    0x0
#define DB_88F6282A_BOARD_NAND_READ_PARAMS		    0x000C0282
#define DB_88F6282A_BOARD_NAND_WRITE_PARAMS		    0x00010305
#define DB_88F6282A_BOARD_NAND_CONTROL		        0x01c00541

MV_BOARD_TWSI_INFO	db88f6282AInfoBoardTwsiDev[] =
	
	{
	{BOARD_DEV_TWSI_EXP, 0x20, ADDR7_BIT},
	{BOARD_DEV_TWSI_EXP, 0x21, ADDR7_BIT},
	{BOARD_DEV_TWSI_EXP, 0x27, ADDR7_BIT},
	{BOARD_DEV_TWSI_SATR, 0x4C, ADDR7_BIT},
	{BOARD_DEV_TWSI_SATR, 0x4D, ADDR7_BIT},
	{BOARD_DEV_TWSI_SATR, 0x4E, ADDR7_BIT},
	{BOARD_TWSI_AUDIO_DEC, 0x4A, ADDR7_BIT}
	};

MV_BOARD_MAC_INFO db88f6282AInfoBoardMacInfo[] = 
	
	{
	{BOARD_MAC_SPEED_AUTO, 0x8},
	{BOARD_MAC_SPEED_AUTO, 0x9}
	}; 

MV_BOARD_MPP_TYPE_INFO db88f6282AInfoBoardMppTypeInfo[] = 
	
	{{MV_BOARD_AUTO, MV_BOARD_AUTO}
	}; 

MV_BOARD_GPP_INFO db88f6282AInfoBoardGppInfo[] = 
	
	{
	{BOARD_GPP_TSU_DIRCTION, 33}
	
	};

MV_DEV_CS_INFO db88f6282AInfoBoardDeCsInfo[] = 
					   
#if defined(MV_NAND) && defined(MV_NAND_BOOT)
		 {{0, N_A, BOARD_DEV_NAND_FLASH, 8}};	            
#elif defined(MV_NAND) && defined(MV_SPI_BOOT)
		 {
         {0, N_A, BOARD_DEV_NAND_FLASH, 8},	   
         {1, N_A, BOARD_DEV_SPI_FLASH, 8},	   
         };
#else
	 {{1, N_A, BOARD_DEV_SPI_FLASH, 8}};	            
#endif

MV_BOARD_MPP_INFO	db88f6282AInfoBoardMppConfigValue[] = 
	{{{
	DB_88F6282A_MPP0_7,		
	DB_88F6282A_MPP8_15,		
	DB_88F6282A_MPP16_23,		
	DB_88F6282A_MPP24_31,		
	DB_88F6282A_MPP32_39,		
	DB_88F6282A_MPP40_47,		
	DB_88F6282A_MPP48_55		
	}}};

MV_BOARD_INFO db88f6282AInfo = {
	"DB-88F6282A-BP",				
	DB_88F6282A_BOARD_MPP_GROUP_TYPE_NUM,		
	db88f6282AInfoBoardMppTypeInfo,
	DB_88F6282A_BOARD_MPP_CONFIG_NUM,		
	db88f6282AInfoBoardMppConfigValue,
	0,						
	0,						
	DB_88F6282A_BOARD_DEVICE_CONFIG_NUM,		
	db88f6282AInfoBoardDeCsInfo,
	DB_88F6282A_BOARD_TWSI_DEF_NUM,			
	db88f6282AInfoBoardTwsiDev,					
	DB_88F6282A_BOARD_MAC_INFO_NUM,			
	db88f6282AInfoBoardMacInfo,
	DB_88F6282A_BOARD_GPP_INFO_NUM,			
	db88f6282AInfoBoardGppInfo,
	DB_88F6282A_BOARD_DEBUG_LED_NUM,			              
	NULL,
	0,								
	DB_88F6282A_OE_LOW,				
	DB_88F6282A_OE_HIGH,				
	DB_88F6282A_OE_VAL_LOW,				
	DB_88F6282A_OE_VAL_HIGH,				
	0,						
	BIT6, 						
	NULL,						
    DB_88F6282A_BOARD_NAND_READ_PARAMS,
    DB_88F6282A_BOARD_NAND_WRITE_PARAMS,
    DB_88F6282A_BOARD_NAND_CONTROL
};



#define SHEEVA_PLUG_BOARD_PCI_IF_NUM		        0x0
#define SHEEVA_PLUG_BOARD_TWSI_DEF_NUM		        0x0
#define SHEEVA_PLUG_BOARD_MAC_INFO_NUM		        0x1
#define SHEEVA_PLUG_BOARD_GPP_INFO_NUM		        0x0
#define SHEEVA_PLUG_BOARD_MPP_GROUP_TYPE_NUN        0x1
#define SHEEVA_PLUG_BOARD_MPP_CONFIG_NUM		    0x1
#define SHEEVA_PLUG_BOARD_DEVICE_CONFIG_NUM	        0x1
#define SHEEVA_PLUG_BOARD_DEBUG_LED_NUM		        0x1
#define SHEEVA_PLUG_BOARD_NAND_READ_PARAMS		    0x000E02C2
#define SHEEVA_PLUG_BOARD_NAND_WRITE_PARAMS		    0x00010305
#define SHEEVA_PLUG_BOARD_NAND_CONTROL		        0x01c00541

MV_U8	sheevaPlugInfoBoardDebugLedIf[] =
	{49};

MV_BOARD_MAC_INFO sheevaPlugInfoBoardMacInfo[] = 
    
	{{BOARD_MAC_SPEED_AUTO, 0x0}}; 

MV_BOARD_TWSI_INFO	sheevaPlugInfoBoardTwsiDev[] =
	
	{{BOARD_TWSI_OTHER, 0x0, ADDR7_BIT}};

MV_BOARD_MPP_TYPE_INFO sheevaPlugInfoBoardMppTypeInfo[] = 
	{{MV_BOARD_OTHER, MV_BOARD_OTHER}
	}; 

MV_DEV_CS_INFO sheevaPlugInfoBoardDeCsInfo[] = 
					   
		 {{0, N_A, BOARD_DEV_NAND_FLASH, 8}};	   

MV_BOARD_MPP_INFO	sheevaPlugInfoBoardMppConfigValue[] = 
	{{{
	RD_SHEEVA_PLUG_MPP0_7,		
	RD_SHEEVA_PLUG_MPP8_15,		
	RD_SHEEVA_PLUG_MPP16_23,		
	RD_SHEEVA_PLUG_MPP24_31,		
	RD_SHEEVA_PLUG_MPP32_39,		
	RD_SHEEVA_PLUG_MPP40_47,		
	RD_SHEEVA_PLUG_MPP48_55		
	}}};

MV_BOARD_INFO sheevaPlugInfo = {
	"SHEEVA PLUG",				                
	SHEEVA_PLUG_BOARD_MPP_GROUP_TYPE_NUN,		
	sheevaPlugInfoBoardMppTypeInfo,
	SHEEVA_PLUG_BOARD_MPP_CONFIG_NUM,		    
	sheevaPlugInfoBoardMppConfigValue,
	0,						                    
	0,					                        
	SHEEVA_PLUG_BOARD_DEVICE_CONFIG_NUM,		
	sheevaPlugInfoBoardDeCsInfo,
	SHEEVA_PLUG_BOARD_TWSI_DEF_NUM,			    
	sheevaPlugInfoBoardTwsiDev,					
	SHEEVA_PLUG_BOARD_MAC_INFO_NUM,			    
	sheevaPlugInfoBoardMacInfo,
	SHEEVA_PLUG_BOARD_GPP_INFO_NUM,			    
	0,
	SHEEVA_PLUG_BOARD_DEBUG_LED_NUM,			              
	sheevaPlugInfoBoardDebugLedIf,
	0,												
	RD_SHEEVA_PLUG_OE_LOW,				            
	RD_SHEEVA_PLUG_OE_HIGH,				        
	RD_SHEEVA_PLUG_OE_VAL_LOW,				        
	RD_SHEEVA_PLUG_OE_VAL_HIGH,				    
	0,						                    
	0, 						                    
	NULL,						
    SHEEVA_PLUG_BOARD_NAND_READ_PARAMS,
    SHEEVA_PLUG_BOARD_NAND_WRITE_PARAMS,
    SHEEVA_PLUG_BOARD_NAND_CONTROL
};



#define DB_CUSTOMER_BOARD_PCI_IF_NUM		        0x0
#define DB_CUSTOMER_BOARD_TWSI_DEF_NUM		        0x0
#define DB_CUSTOMER_BOARD_MAC_INFO_NUM		        0x0
#define DB_CUSTOMER_BOARD_GPP_INFO_NUM		        0x0
#define DB_CUSTOMER_BOARD_MPP_GROUP_TYPE_NUN        0x0
#define DB_CUSTOMER_BOARD_MPP_CONFIG_NUM		    0x0
#if defined(MV_NAND) && defined(MV_NAND_BOOT)
    #define DB_CUSTOMER_BOARD_DEVICE_CONFIG_NUM	    0x0
#elif defined(MV_NAND) && defined(MV_SPI_BOOT)
    #define DB_CUSTOMER_BOARD_DEVICE_CONFIG_NUM	    0x0
#else
    #define DB_CUSTOMER_BOARD_DEVICE_CONFIG_NUM	    0x0
#endif
#define DB_CUSTOMER_BOARD_DEBUG_LED_NUM		0x0
#define DB_CUSTOMER_BOARD_NAND_READ_PARAMS		    0x000E02C2
#define DB_CUSTOMER_BOARD_NAND_WRITE_PARAMS		    0x00010305
#define DB_CUSTOMER_BOARD_NAND_CONTROL		        0x01c00541

MV_U8	dbCustomerInfoBoardDebugLedIf[] =
	{0};

MV_BOARD_MAC_INFO dbCustomerInfoBoardMacInfo[] = 
    
	{{BOARD_MAC_SPEED_AUTO, 0x0}}; 

MV_BOARD_TWSI_INFO	dbCustomerInfoBoardTwsiDev[] =
	
	{{BOARD_TWSI_OTHER, 0x0, ADDR7_BIT}};

MV_BOARD_MPP_TYPE_INFO dbCustomerInfoBoardMppTypeInfo[] = 
	{{MV_BOARD_OTHER, MV_BOARD_OTHER}
	}; 

MV_DEV_CS_INFO dbCustomerInfoBoardDeCsInfo[] = 
					   
#if defined(MV_NAND) && defined(MV_NAND_BOOT)
		 {{0, N_A, BOARD_DEV_NAND_FLASH, 8}};	   
#elif defined(MV_NAND) && defined(MV_SPI_BOOT)
		 {
         {0, N_A, BOARD_DEV_NAND_FLASH, 8},	   
         {2, N_A, BOARD_DEV_SPI_FLASH, 8},	   
         };
#else
		 {{2, N_A, BOARD_DEV_SPI_FLASH, 8}};	            
#endif

MV_BOARD_MPP_INFO	dbCustomerInfoBoardMppConfigValue[] = 
	{{{
	DB_CUSTOMER_MPP0_7,		
	DB_CUSTOMER_MPP8_15,		
	DB_CUSTOMER_MPP16_23,		
	DB_CUSTOMER_MPP24_31,		
	DB_CUSTOMER_MPP32_39,		
	DB_CUSTOMER_MPP40_47,		
	DB_CUSTOMER_MPP48_55		
	}}};

MV_BOARD_INFO dbCustomerInfo = {
	"DB-CUSTOMER",				                
	DB_CUSTOMER_BOARD_MPP_GROUP_TYPE_NUN,		
	dbCustomerInfoBoardMppTypeInfo,
	DB_CUSTOMER_BOARD_MPP_CONFIG_NUM,		    
	dbCustomerInfoBoardMppConfigValue,
	0,						                    
	0,					                        
	DB_CUSTOMER_BOARD_DEVICE_CONFIG_NUM,		
	dbCustomerInfoBoardDeCsInfo,
	DB_CUSTOMER_BOARD_TWSI_DEF_NUM,			    
	dbCustomerInfoBoardTwsiDev,					
	DB_CUSTOMER_BOARD_MAC_INFO_NUM,			    
	dbCustomerInfoBoardMacInfo,
	DB_CUSTOMER_BOARD_GPP_INFO_NUM,			    
	0,
	DB_CUSTOMER_BOARD_DEBUG_LED_NUM,			              
	NULL,
	0,												
	DB_CUSTOMER_OE_LOW,				            
	DB_CUSTOMER_OE_HIGH,				        
	DB_CUSTOMER_OE_VAL_LOW,				        
	DB_CUSTOMER_OE_VAL_HIGH,				    
	0,						                    
	0, 						                    
	NULL,						
    DB_CUSTOMER_BOARD_NAND_READ_PARAMS,
    DB_CUSTOMER_BOARD_NAND_WRITE_PARAMS,
    DB_CUSTOMER_BOARD_NAND_CONTROL
};

#ifdef MY_ABC_HERE

#define SYNO_DS109_BOARD_MPP_GROUP_TYPE_NUM  0x1
MV_BOARD_MPP_TYPE_INFO SYNO_DS109InfoBoardMppTypeInfo[] =
{
        { MV_BOARD_AUTO, MV_BOARD_AUDIO }
};

#define SYNO_DS109_BOARD_MPP_CONFIG_NUM             0x1
#define SYNO_DS109_MPP0_7                    0x01002222
#define SYNO_DS109_MPP8_15                   0x03303311
#define SYNO_DS109_MPP16_23                  0x00550000
#define SYNO_DS109_MPP24_31                  0x00000000
#define SYNO_DS109_MPP32_39                  0x44440000
#define SYNO_DS109_MPP40_47                  0x00044444
#define SYNO_DS109_MPP48_55                  0x00000000

MV_BOARD_MPP_INFO SYNO_DS109InfoBoardMppConfigValue[] =
{
       {
               {
                       SYNO_DS109_MPP0_7,
                       SYNO_DS109_MPP8_15,
                       SYNO_DS109_MPP16_23,
                       SYNO_DS109_MPP24_31,
                       SYNO_DS109_MPP32_39,
                       SYNO_DS109_MPP40_47,
                       SYNO_DS109_MPP48_55
               }
       }
};

#define SYNO_DS109_BOARD_DEVICE_CONFIG_NUM   0x1

MV_DEV_CS_INFO SYNO_DS109InfoBoardDeCsInfo[] =
#if defined(MV_NAND) || defined(MV_NAND_BOOT)

{
       {0, N_A, BOARD_DEV_NAND_FLASH, 8}
};
#else

{
       {2, N_A, BOARD_DEV_SPI_FLASH, 8}
};
#endif

#define SYNO_DS109_BOARD_TWSI_DEF_NUM                        0x1
MV_BOARD_TWSI_INFO      SYNO_DS109InfoBoardTwsiDev[] =
{
        {BOARD_TWSI_AUDIO_DEC, 0x4A, ADDR7_BIT},
};

#define SYNO_DS109_BOARD_MAC_INFO_NUM                        0x1
MV_BOARD_MAC_INFO SYNO_DS109InfoBoardMacInfo[] =
{
       {BOARD_MAC_SPEED_AUTO, 0x8}
};


MV_BOARD_INFO SYNO_DS109_INFO = {
    "Synology Disk Station",                      

    SYNO_DS109_BOARD_MPP_GROUP_TYPE_NUM,          
    SYNO_DS109InfoBoardMppTypeInfo,

    SYNO_DS109_BOARD_MPP_CONFIG_NUM,              
    SYNO_DS109InfoBoardMppConfigValue,

    0,                                            
    0,                                            

    SYNO_DS109_BOARD_DEVICE_CONFIG_NUM,           
    SYNO_DS109InfoBoardDeCsInfo,

    SYNO_DS109_BOARD_TWSI_DEF_NUM,                
    SYNO_DS109InfoBoardTwsiDev,

    SYNO_DS109_BOARD_MAC_INFO_NUM,                
    SYNO_DS109InfoBoardMacInfo,

    0,                
    NULL,

    0,                                            

    NULL,
    N_A,                                          

    0,                            
    0,                           
    0,                        
    0,                       
    0,
    0,
    NULL,                                          
	0,
	0,
	0
};

#define SYNO_DS409slim_BOARD_MPP_GROUP_TYPE_NUM  0x1
MV_BOARD_MPP_TYPE_INFO SYNO_DS409slimInfoBoardMppTypeInfo[] =
{
        { MV_BOARD_AUTO, MV_BOARD_AUDIO }
};      

#define SYNO_DS409slim_BOARD_MPP_CONFIG_NUM             0x1
#define SYNO_DS409slim_MPP0_7                    0x01002222
#define SYNO_DS409slim_MPP8_15                   0x03303311
#define SYNO_DS409slim_MPP16_23                  0x00000000
#define SYNO_DS409slim_MPP24_31                  0x00000000
#define SYNO_DS409slim_MPP32_39                  0x44440000
#define SYNO_DS409slim_MPP40_47                  0x00044444
#define SYNO_DS409slim_MPP48_55                  0x00000000

MV_BOARD_MPP_INFO SYNO_DS409slimInfoBoardMppConfigValue[] =
{
       {
               {
                       SYNO_DS409slim_MPP0_7,
                       SYNO_DS409slim_MPP8_15,
                       SYNO_DS409slim_MPP16_23,
                       SYNO_DS409slim_MPP24_31,
                       SYNO_DS409slim_MPP32_39,
                       SYNO_DS409slim_MPP40_47,
                       SYNO_DS409slim_MPP48_55
               }
       }
};

#define SYNO_DS409slim_BOARD_DEVICE_CONFIG_NUM   0x1

MV_DEV_CS_INFO SYNO_DS409slimInfoBoardDeCsInfo[] =
#if defined(MV_NAND) || defined(MV_NAND_BOOT)

{
       {0, N_A, BOARD_DEV_NAND_FLASH, 8}
};
#else

{
       {2, N_A, BOARD_DEV_SPI_FLASH, 8}
};
#endif

#define SYNO_DS409slim_BOARD_TWSI_DEF_NUM                      0x1
MV_BOARD_TWSI_INFO      SYNO_DS409slimInfoBoardTwsiDev[] =
{
        {BOARD_TWSI_AUDIO_DEC, 0x4A, ADDR7_BIT},
};

#define SYNO_DS409slim_BOARD_MAC_INFO_NUM                      0x1
MV_BOARD_MAC_INFO SYNO_DS409slimInfoBoardMacInfo[] =
{
       {BOARD_MAC_SPEED_AUTO, 0x8}
};


MV_BOARD_INFO SYNO_DS409slim_INFO = {
    "Synology Disk Station",                          

    SYNO_DS409slim_BOARD_MPP_GROUP_TYPE_NUM,          
    SYNO_DS409slimInfoBoardMppTypeInfo,

    SYNO_DS409slim_BOARD_MPP_CONFIG_NUM,              
    SYNO_DS409slimInfoBoardMppConfigValue,

    0,                                                
    0,                                                

    SYNO_DS409slim_BOARD_DEVICE_CONFIG_NUM,           
    SYNO_DS409slimInfoBoardDeCsInfo,

    SYNO_DS409slim_BOARD_TWSI_DEF_NUM,                
    SYNO_DS409slimInfoBoardTwsiDev,

    SYNO_DS409slim_BOARD_MAC_INFO_NUM,                
    SYNO_DS409slimInfoBoardMacInfo,

    0,                
    NULL,

    0,                                                

    NULL,
    N_A,                                                  
    
    0,                            
    0,                           
    0,                        
    0,                       
    0,
	0,
    NULL,                                              
	0,
	0,
	0
};


#define SYNO_DS409_BOARD_MPP_GROUP_TYPE_NUM  0x1
MV_BOARD_MPP_TYPE_INFO SYNO_DS409InfoBoardMppTypeInfo[] =
{
        { MV_BOARD_AUTO, MV_BOARD_AUTO }
};      

#define SYNO_DS409_BOARD_MPP_CONFIG_NUM             0x1
#define SYNO_DS409_MPP0_7                    0x01002222
#define SYNO_DS409_MPP8_15                   0x03303311
#define SYNO_DS409_MPP16_23                  0x33330000
#define SYNO_DS409_MPP24_31                  0x33003333
#define SYNO_DS409_MPP32_39                  0x00005533
#define SYNO_DS409_MPP40_47                  0x00000000
#define SYNO_DS409_MPP48_55                  0x00000000

MV_BOARD_MPP_INFO SYNO_DS409InfoBoardMppConfigValue[] =
{
       {
               {
                       SYNO_DS409_MPP0_7,
                       SYNO_DS409_MPP8_15,
                       SYNO_DS409_MPP16_23,
                       SYNO_DS409_MPP24_31,
                       SYNO_DS409_MPP32_39,
                       SYNO_DS409_MPP40_47,
                       SYNO_DS409_MPP48_55
               }
       }
};

#define SYNO_DS409_BOARD_DEVICE_CONFIG_NUM   0x1
MV_DEV_CS_INFO SYNO_DS409InfoBoardDeCsInfo[] =
#if defined(MV_NAND) || defined(MV_NAND_BOOT)

{
       {0, N_A, BOARD_DEV_NAND_FLASH, 8}
};
#else

{
       {2, N_A, BOARD_DEV_SPI_FLASH, 8}
};
#endif

#define SYNO_DS409_BOARD_MAC_INFO_NUM                      0x2
MV_BOARD_MAC_INFO SYNO_DS409InfoBoardMacInfo[] =
{
	{BOARD_MAC_SPEED_AUTO, 0x8},
	{BOARD_MAC_SPEED_AUTO, 0x9}
};


MV_BOARD_INFO SYNO_DS409_INFO = {
    "Synology Disk Station",                          

    SYNO_DS409_BOARD_MPP_GROUP_TYPE_NUM,          
    SYNO_DS409InfoBoardMppTypeInfo,

    SYNO_DS409_BOARD_MPP_CONFIG_NUM,              
    SYNO_DS409InfoBoardMppConfigValue,

    0,                                                
    0,                                                

    SYNO_DS409_BOARD_DEVICE_CONFIG_NUM,           
    SYNO_DS409InfoBoardDeCsInfo,

    0,                							
    NULL,

    SYNO_DS409_BOARD_MAC_INFO_NUM,                
    SYNO_DS409InfoBoardMacInfo,

    0,                
    NULL,

    0,                                                

    NULL,
    N_A,                                                  
    
    0,                            
    0,                           
    0,                        
    0,                           
    0,
    0,
    NULL,                                              
	0,
	0,
	0
};

#define DS211_MPP0_7		0x01002222
#define DS211_MPP8_15		0x03303311
#define DS211_MPP16_23		0x00550000
#define DS211_MPP24_31		0x00000000
#define DS211_MPP32_39		0x00000000
#define DS211_MPP40_47		0x00000000
#define DS211_MPP48_55		0x00000000

MV_BOARD_MPP_INFO	DS211InfoBoardMppConfigValue[] = 
{
	{
		{
			DS211_MPP0_7,
			DS211_MPP8_15,
			DS211_MPP16_23,
			DS211_MPP24_31,
			DS211_MPP32_39,
			DS211_MPP40_47,
			DS211_MPP48_55
		}
	}
};

#define DS211_BOARD_MAC_INFO_NUM 1
MV_BOARD_MAC_INFO DS211InfoBoardMacInfo[] = 
	
{
	{BOARD_MAC_SPEED_AUTO, 0x8},
    {BOARD_MAC_SPEED_AUTO, 0x9}
};


MV_BOARD_INFO SYNO_DS211_INFO = {
	"Synology Disk Station",				
	DB_88F6282A_BOARD_MPP_GROUP_TYPE_NUM,		
	db88f6282AInfoBoardMppTypeInfo,
	DB_88F6282A_BOARD_MPP_CONFIG_NUM,		
	DS211InfoBoardMppConfigValue,
	0,						
	0,						
	DB_88F6282A_BOARD_DEVICE_CONFIG_NUM,		
	db88f6282AInfoBoardDeCsInfo,
	0,			
	NULL,					
	DS211_BOARD_MAC_INFO_NUM,			
	DS211InfoBoardMacInfo,
	0,			
	NULL,
	0,			              
	NULL,
	0,								
	0,				
	0,				
	0,				
	0,				
	0,						
	0, 						
	NULL,						
    0,
    0,
    0
};

#define DS_6282_4BAY_MPP0_7			0x01002222
#define DS_6282_4BAY_MPP8_15		0x03303311
#define DS_6282_4BAY_MPP16_23		0x00000000
#define DS_6282_4BAY_MPP24_31		0x00000000
#define DS_6282_4BAY_MPP32_39		0x00000000
#define DS_6282_4BAY_MPP40_47		0x00000000
#define DS_6282_4BAY_MPP48_55		0x00000000

MV_BOARD_MPP_INFO	DS_6282_4BayInfoBoardMppConfigValue[] =
{
	{
		{
			DS_6282_4BAY_MPP0_7,
			DS_6282_4BAY_MPP8_15,
			DS_6282_4BAY_MPP16_23,
			DS_6282_4BAY_MPP24_31,
			DS_6282_4BAY_MPP32_39,
			DS_6282_4BAY_MPP40_47,
			DS_6282_4BAY_MPP48_55
		}
	}
};


MV_BOARD_INFO SYNO_DS_6282_4BAY_INFO = {
	"Synology Disk Station",				
	DB_88F6282A_BOARD_MPP_GROUP_TYPE_NUM,		
	db88f6282AInfoBoardMppTypeInfo,
	DB_88F6282A_BOARD_MPP_CONFIG_NUM,		
	DS_6282_4BayInfoBoardMppConfigValue,
	0,						
	0,						
	DB_88F6282A_BOARD_DEVICE_CONFIG_NUM,		
	db88f6282AInfoBoardDeCsInfo,
	0,			
	NULL,
	DS211_BOARD_MAC_INFO_NUM,			
	DS211InfoBoardMacInfo,
	0,			
	NULL,
	0,			
	NULL,
	0,						
	0,				
	0,				
	0,				
	0,				
	0,						
	0, 						
	NULL,						
    0,
    0,
    0
};


#define RS_6282_MPP0_7         0x01002222
#define RS_6282_MPP8_15        0x03303311
#define RS_6282_MPP16_23       0x33330000
#define RS_6282_MPP24_31       0x33003333
#define RS_6282_MPP32_39       0x00000033
#define RS_6282_MPP40_47       0x00000000
#define RS_6282_MPP48_55       0x00000000
MV_BOARD_MPP_INFO	RS6282InfoBoardMppConfigValue[] =
{
	{
		{
			RS_6282_MPP0_7,
			RS_6282_MPP8_15,
			RS_6282_MPP16_23,
			RS_6282_MPP24_31,
			RS_6282_MPP32_39,
			RS_6282_MPP40_47,
			RS_6282_MPP48_55
		}
	}
};

#define RS_6282_BOARD_MAC_INFO_NUM 2

MV_BOARD_INFO SYNO_RS_6282_INFO = {
	"Synology Disk Station",				
	DB_88F6282A_BOARD_MPP_GROUP_TYPE_NUM,		
	db88f6282AInfoBoardMppTypeInfo,
	DB_88F6282A_BOARD_MPP_CONFIG_NUM,		
	RS6282InfoBoardMppConfigValue,
	0,						
	0,						
	DB_88F6282A_BOARD_DEVICE_CONFIG_NUM,		
	db88f6282AInfoBoardDeCsInfo,
	0,			
	NULL,
	RS_6282_BOARD_MAC_INFO_NUM,			
	DS211InfoBoardMacInfo,
	0,			
	NULL,
	0,			
	NULL,
	0,						
	0,				
	0,				
	0,				
	0,				
	0,						
	0, 						
	NULL,						
    0,
    0,
    0
};

MV_BOARD_MAC_INFO RS213InfoBoardMacInfo[] = 
	
{
	{BOARD_MAC_SPEED_AUTO, 0x0},
    {BOARD_MAC_SPEED_AUTO, 0x1}
};
MV_BOARD_INFO SYNO_RS213_INFO = {
	"Synology Disk Station",				
	DB_88F6282A_BOARD_MPP_GROUP_TYPE_NUM,		
	db88f6282AInfoBoardMppTypeInfo,
	DB_88F6282A_BOARD_MPP_CONFIG_NUM,		
	RS6282InfoBoardMppConfigValue,
	0,						
	0,						
	DB_88F6282A_BOARD_DEVICE_CONFIG_NUM,		
	db88f6282AInfoBoardDeCsInfo,
	0,			
	NULL,
	RS_6282_BOARD_MAC_INFO_NUM,			
	RS213InfoBoardMacInfo,
	0,			
	NULL,
	0,			
	NULL,
	0,						
	0,				
	0,				
	0,				
	0,				
	0,						
	0, 						
	NULL,						
    0,
    0,
    0
};


#define SYNO_DS011_BOARD_MPP_GROUP_TYPE_NUM  0x1
MV_BOARD_MPP_TYPE_INFO SYNO_DS011InfoBoardMppTypeInfo[] =
{
	{ MV_BOARD_OTHER, MV_BOARD_OTHER }
};

#define SYNO_DS011_BOARD_MPP_CONFIG_NUM             0x1
#define SYNO_DS011_MPP0_7                    0x01002222
#define SYNO_DS011_MPP8_15                   0x03303311
#define SYNO_DS011_MPP16_23                  0x00000000
#define SYNO_DS011_MPP32_39                  0x00000000
#define SYNO_DS011_MPP40_44                  0x00000000

MV_BOARD_MPP_INFO SYNO_DS011InfoBoardMppConfigValue[] =
{
	{
		{
			SYNO_DS011_MPP0_7,
			SYNO_DS011_MPP8_15,
			SYNO_DS011_MPP16_23,
			SYNO_DS011_MPP32_39,
			SYNO_DS011_MPP40_44,
		}
	}
};

#define SYNO_DS011_BOARD_DEVICE_CONFIG_NUM   0x1


MV_DEV_CS_INFO SYNO_DS011InfoBoardDeCsInfo[] =
#if defined(MV_NAND) || defined(MV_NAND_BOOT)
	
{
	{0, N_A, BOARD_DEV_NAND_FLASH, 8}
};
#else

{
	{2, N_A, BOARD_DEV_SPI_FLASH, 8}
};
#endif

#define SYNO_DS011_BOARD_TWSI_DEF_NUM                        0x1
MV_BOARD_TWSI_INFO      SYNO_DS011InfoBoardTwsiDev[] =
{
	{BOARD_TWSI_AUDIO_DEC, 0x4A, ADDR7_BIT},
};

#define SYNO_DS011_BOARD_MAC_INFO_NUM                        0x1
MV_BOARD_MAC_INFO SYNO_DS011InfoBoardMacInfo[] =
{
	{BOARD_MAC_SPEED_AUTO, 0x8}
};

#define SYNO_DS011_BOARD_GPP_INFO_NUM                        0x4
MV_BOARD_GPP_INFO SYNO_DS011InfoBoardGppInfo[] =
{
	{SYNO_DS011_GPP_LED_USBDISK_ORANGE, 40},
	{SYNO_DS011_GPP_LED_USBDISK_GREEN, 36},
	{SYNO_DS011_GPP_LED_STATUS, 37},
	{SYNO_DS011_GPP_BUTTON_RESET, 38},
	{SYNO_DS011_GPP_BUTTON_EJECT, 39},                  
};

#define SYNO_DS011_OE_LOW                    0x00000000

#define SYNO_DS011_OE_HIGH                   (BIT6|BIT7)
#define SYNO_DS011_OE_VAL_LOW                0x00000000
 
#define SYNO_DS011_OE_VAL_HIGH               (BIT4|BIT4)

MV_BOARD_INFO SYNO_DS011_INFO = {
	"Synology Disk Station",                      

	SYNO_DS011_BOARD_MPP_GROUP_TYPE_NUM,          
	SYNO_DS011InfoBoardMppTypeInfo,

	SYNO_DS011_BOARD_MPP_CONFIG_NUM,              
	SYNO_DS011InfoBoardMppConfigValue,

	0,                                            
	0,                                            

	SYNO_DS011_BOARD_DEVICE_CONFIG_NUM,           
	SYNO_DS011InfoBoardDeCsInfo,

	SYNO_DS011_BOARD_TWSI_DEF_NUM,                
	SYNO_DS011InfoBoardTwsiDev,

	SYNO_DS011_BOARD_MAC_INFO_NUM,                
	SYNO_DS011InfoBoardMacInfo,

	SYNO_DS011_BOARD_GPP_INFO_NUM,                
	SYNO_DS011InfoBoardGppInfo,

	0,                                            

	NULL,
	N_A,                                          

	SYNO_DS011_OE_LOW,                            
	SYNO_DS011_OE_HIGH,                           
	SYNO_DS011_OE_VAL_LOW,                        
	SYNO_DS011_OE_VAL_HIGH,                       
	NULL,
	NULL,
	NULL                                          
};

#define DS411_BOARD_MAC_INFO_NUM 1
MV_BOARD_MAC_INFO PhyAddr1BoardMacInfo[] = 
	
{
	{BOARD_MAC_SPEED_AUTO, 0x1}
};

MV_BOARD_INFO SYNO_DS411_INFO = {
	"Synology Disk Station",				
	DB_88F6282A_BOARD_MPP_GROUP_TYPE_NUM,		
	db88f6282AInfoBoardMppTypeInfo,
	DB_88F6282A_BOARD_MPP_CONFIG_NUM,		
	DS_6282_4BayInfoBoardMppConfigValue,
	0,						
	0,						
	DB_88F6282A_BOARD_DEVICE_CONFIG_NUM,		
	db88f6282AInfoBoardDeCsInfo,
	0,			
	NULL,
	DS411_BOARD_MAC_INFO_NUM,			
	PhyAddr1BoardMacInfo,
	0,			
	NULL,
	0,			
	NULL,
	0,						
	0,				
	0,				
	0,				
	0,				
	0,						
	0, 						
	NULL,						
    0,
    0,
    0
};

#define DS212_BOARD_MAC_INFO_NUM 1


MV_BOARD_INFO SYNO_DS212_INFO = {
	"Synology Disk Station",				
	DB_88F6282A_BOARD_MPP_GROUP_TYPE_NUM,		
	db88f6282AInfoBoardMppTypeInfo,
	DB_88F6282A_BOARD_MPP_CONFIG_NUM,		
	DS211InfoBoardMppConfigValue, /
	0,						
	0,						
	DB_88F6282A_BOARD_DEVICE_CONFIG_NUM,		
	db88f6282AInfoBoardDeCsInfo,
	0,			
	NULL,					
	DS212_BOARD_MAC_INFO_NUM,			
	PhyAddr1BoardMacInfo,
	0,			
	NULL,
	0,			              
	NULL,
	0,								
	0,				
	0,				
	0,				
	0,				
	0,						
	0, 						
	NULL,						
    0,
    0,
    0
};

#define SYNO_6702_1BAY_BOARD_MPP_GROUP_TYPE_NUM  0x1
MV_BOARD_MPP_TYPE_INFO SYNO_6702_1BAYInfoBoardMppTypeInfo[] =
{
        { MV_BOARD_AUTO, MV_BOARD_AUDIO }
};

#define SYNO_6702_1BAY_BOARD_MPP_CONFIG_NUM             0x1
#define SYNO_6702_1BAY_MPP7_0                    0x01222222
#define SYNO_6702_1BAY_MPP15_8                   0x30005511
#define SYNO_6702_1BAY_MPP23_16                  0x00000003
#define SYNO_6702_1BAY_MPP31_24                  0x00000000
#define SYNO_6702_1BAY_MPP39_32                  0x00000000
#define SYNO_6702_1BAY_MPP47_40                  0x00000000
#define SYNO_6702_1BAY_MPP55_48                  0x00000000

MV_BOARD_MPP_INFO SYNO_6702_1BAY_InfoBoardMppConfigValue[] =
{
       {
               {
                       SYNO_6702_1BAY_MPP7_0,
                       SYNO_6702_1BAY_MPP15_8,
                       SYNO_6702_1BAY_MPP23_16,
                       SYNO_6702_1BAY_MPP31_24,
                       SYNO_6702_1BAY_MPP39_32,
                       SYNO_6702_1BAY_MPP47_40,
                       SYNO_6702_1BAY_MPP55_48
               }
       }
};



MV_DEV_CS_INFO SYNO_DS112jInfoBoardDeCsInfo[] =
{
       {0, N_A, BOARD_DEV_SPI_FLASH, 8}
};

#define SYNO_6702_1BAY_OE_LOW                    0x0
#define SYNO_6702_1BAY_OE_HIGH                   0x0
#define SYNO_6702_1BAY_OE_VAL_LOW                (BIT13|BIT17|BIT18|BIT19)
#define SYNO_6702_1BAY_OE_VAL_HIGH               0x0

MV_BOARD_INFO SYNO_6702_1BAY_INFO = {
    "Synology Disk Station",                      

    SYNO_6702_1BAY_BOARD_MPP_GROUP_TYPE_NUM,          
    SYNO_6702_1BAYInfoBoardMppTypeInfo,

    SYNO_6702_1BAY_BOARD_MPP_CONFIG_NUM,              
    SYNO_6702_1BAY_InfoBoardMppConfigValue,

    0,                                            
    0,                                            

    SYNO_DS109_BOARD_DEVICE_CONFIG_NUM,           
	SYNO_DS112jInfoBoardDeCsInfo,

    SYNO_DS109_BOARD_TWSI_DEF_NUM,                
    SYNO_DS109InfoBoardTwsiDev,

	DS212_BOARD_MAC_INFO_NUM,                
	PhyAddr1BoardMacInfo,

	0,												
	NULL, 

    0,                                            

    NULL,
    N_A,                                          

    SYNO_6702_1BAY_OE_LOW,                            
    SYNO_6702_1BAY_OE_HIGH,                           
    SYNO_6702_1BAY_OE_VAL_LOW,                        
    SYNO_6702_1BAY_OE_VAL_HIGH,                       
    0,
	0,
    NULL,                                          
	0,
	0,
	0
};

#endif 

MV_BOARD_INFO*	boardInfoTbl[] = 	{
                    &db88f6281AInfo,
                    &rd88f6281AInfo,
                    &db88f6192AInfo,
                    &rd88f6192AInfo,
                    &db88f6180AInfo,
                    &db88f6190AInfo,
                    &rd88f6190AInfo,
                    &rd88f6281APcacInfo,
                    &dbCustomerInfo,
                    &sheevaPlugInfo,
                    &db88f6280AInfo,
                    &db88f6282AInfo,
					&rd88f6282aInfo,
                    &db88f6701AInfo,
                    &db88f6702AInfo
#ifdef MY_ABC_HERE
						,
                    NULL,                           
                    NULL,                           
                    NULL,                           
                    NULL,                           
                    &SYNO_DS409_INFO,               
                    &SYNO_DS409slim_INFO,           
                    &SYNO_DS109_INFO,               
                    &SYNO_DS011_INFO,               
                    &SYNO_DS211_INFO,               
					&SYNO_DS_6282_4BAY_INFO,		
					&SYNO_RS_6282_INFO,				
					&SYNO_DS411_INFO,				
					&SYNO_DS212_INFO,               
					&SYNO_6702_1BAY_INFO,           
					&SYNO_RS213_INFO,				
#endif
					};
