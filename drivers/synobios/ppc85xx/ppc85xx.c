#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
// Copyright (c) 2000-2006 Synology Inc. All rights reserved.
#include <linux/syno.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/errno.h>  /* error codes */
#include <linux/delay.h>
#include <linux/synobios.h>
#include <linux/fs.h>
#include <asm/setup.h>
#include "../i2c/i2c-ppc.h"
#include "../mapping.h"
#include "../rtc/rtc.h"

/* CPLD definition ( 5Bay )
 * Local Bus CS1 : 
 *      Freescale 8241/8245 CPLD mapping to CPU address  0xff00-0000
 *      Freescale 8343/8347 CPLD mapping to CPU address  0xff00-0000
 *      Freescale 8548/8548 CPLD mapping to CPU address  0xf800-0000
 *      Marvell 88F5x81/5x82 CPLD mapping to CPU address  0xf800-0000
 *
 *              offset 0 --> Disk Led Control (R/W)
 *              offset 1 --> Disk 5 and Model register (R/W)
 *              offset 2 --> Memory Type register (RO)
 *              offset 3 --> Fan status (R/W)
 *              offset 4 --> Power status (For Redundant) (RO)
 *
 * Disk Led Control register :
 *              bit 7,6: Disk1 LED
 *              bit 5,4: Disk2 LED
 *              bit 3,2: Disk3 LED
 *              bit 1,0: Disk4 LED
 *                       11: LED Off (default)
 *                       10: Green LED, allow activity blinking from SATA controller
 *                       01: Solid Yellow
 *                       00: Yellow blinking 2 Hz
 *
 * Disk 5 and Model register :
 *              bit 7, 6: Disk5 LED
 *                       11: LED Off (default)
 *                       10: Green LED, allow activity blinking from SATA controller
 *                       01: Solid Yellow
 *                       00: Yellow blinking 2 Hz
 *              bit 5: Alarm LED
 *                       1: LED Off (Backplane is OK)
 * 						 0: Amber LED Flash, low speed
 *              bit 4: Backplane Lock
 *                       1:no Backplane (Alarm LED flash Green, high speed)
 *						 0:Backplane is OK
 *              bit 3-0:
 *                                               0111: DS410
 *                                               0101: DS210p
 *                       			 1110: RS409RPp
 *                       			 1101: RS409p
 *                       			 1100: DS109p
 *                       			 1000: DS110p
 *                       			 1011: DS509p
 *                       			 1010: DS209p
 *                       			 0100: DS209pII
 *                       			 1001: DS409p
 * 						 0110: Redundant RS408
 * 						 0011: DS508
 * 						 0010: RS408
 * 						 0001: DS408
 * 						 0000: Reserved
 *
 * Memory Type register :
 *              bit 7-5: Memory_size & Conf 
 * 					111: DIMM Module
 *              bit 4-2: Reserved
 *              bit 2,1: Revision
 *                       00: CPLD V1.0
 *						 01: CPLD V1.1
 * 						 10: CPLD V2.0
 * 						 11:Sample version
 *
 * Fan Status register :
 *              bit 7,6 --> Reserved
 *              bit 5-4 --> FAN speed
 *						 00: Ultra-Low speed
 *					 	 01: Low speed
 *						 10: middle speed
 *						 11: Full-speed
 *              bit 3 --> EXT_FAN4 status
 *              bit 2 --> EXT_FAN1 status
 *              bit 1 --> EXT_FAN2 status
 *              bit 0 --> EXT_FAN3 status
 *						 1: FAN is running
 *						 0: FAN is not running
 *
 * Fan Status register : (MODEL_DS509p only)
 *              bit 6-4 --> FAN speed
 *						 000: Fan Off
 *						 001: reserved
 *						 010: Ultra-Low speed
 *						 011: Low speed (power default)
 *						 100: middle speed
 *						 101: Full-speed
 *						 110: reserved
 *						 111: reserved
 *
 * Power Status register :
 *              bit 7-3 --> Reserved
 *              bit 2 --> Clear Buzzer
 *						 0: Clear Buzzer Button Pressed
 *						 1: Normal or Read
 *              bit 1 --> Redundant_1 status
 *              bit 0 --> Redundant_0 status
 *						 0: Power Fail or be removed
 *						 1: Power Good
 */
#define	SYNO_CPLD_BASE		0xF8000000
#define	SYNO_CPLD_END		0xF8000010

#define CPLD_OFFSET_DISK_LED    0x0
#define CPLD_SHIFT_DISK1_LED    0x6
#define CPLD_SHIFT_DISK2_LED    0x4
#define CPLD_SHIFT_DISK3_LED    0x2
#define CPLD_SHIFT_DISK4_LED    0x0
#define DS_LED_OFF				0x3
#define DS_LED_GREEN_SOLID		0x2
#define DS_LED_ORANGE_SOLID		0x1
#define DS_LED_ORANGE_BLINK		0x0
#define CPLD_MASK_DISK1_LED     0xC0
#define CPLD_MASK_DISK2_LED     0x30
#define CPLD_MASK_DISK3_LED     0x0C
#define CPLD_MASK_DISK4_LED     0x03

#define CPLD_OFFSET_MODEL       	0x1
#define CPLD_SHIFT_DISK5_LED    	0x6
#define CPLD_SHIFT_ALARM_LED    	0x5
#define CPLD_SHIFT_BACKPLANE_LOCK   0x4
#define CPLD_SHIFT_MODEL			0x0
#define CPLD_VALUE_ALARM_OFF		0x1
#define CPLD_VALUE_ALARM_ON			0x0
#define CPLD_VALUE_BACKPLANE_FAIL	0x1
#define CPLD_VALUE_BACKPLANE_OK		0x0
#define CPLD_MASK_DISK5_LED     	0xC0
#define CPLD_MASK_ALARM_LED			0x20
#define CPLD_MASK_BACKPLANE_LOCK	0x10
#define CPLD_MASK_MODEL				0x0F

#define CPLD_OFFSET_DRAM       	0x2
#define CPLD_SHIFT_DRAM			0x5
#define CPLD_SHIFT_CPLD_REV     0x0
#define CPLD_VALUE_DRAM_DIMM    0x8
#define CPLD_VALUE_REV_20		0x2
#define CPLD_MASK_DRAM			0xE0
#define CPLD_MASK_CPLD_REV      0x03

#define CPLD_OFFSET_FAN_STATUS  0x3
#define CPLD_SHIFT_FAN_SPEED    0x4
#define CPLD_SHIFT_FAN_EXT1     0x2
#define CPLD_SHIFT_FAN_EXT2     0x1
#define CPLD_SHIFT_FAN_EXT3     0x0
#define CPLD_SHIFT_FAN_EXT4     0x3
#define CPLD_MASK_FAN_SPEED     0x30
#define CPLD_MASK_FAN_EXT1	0x04
#define CPLD_MASK_FAN_EXT2	0x02
#define CPLD_MASK_FAN_EXT3	0x01
#define CPLD_MASK_FAN_EXT4	0x08
#define CPLD_SHIFT_FAN_SPEED_CONTROL	0x4
#define CPLD_FAN_SPEED_MASK	0x30
#define CPLD_FAN_SPEED_ULTRA_LOW	0x00
#define CPLD_FAN_SPEED_LOW	0x01
#define CPLD_FAN_SPEED_MIDDLE	0x02
#define CPLD_FAN_SPEED_FULL	0x03

#define DS509P_CPLD_FAN_SPEED_MASK	0x70

#define CPLD_OFFSET_POWER_STATUS_OFFSET 0x4
#define CPLD_POWER_1_GOOD 0x1
#define CPLD_POWER_2_GOOD 0x2
#define CPLD_CLEAR_BUZZER 0x4

#define SYNO_MODEL_DS410	0x07
#define SYNO_MODEL_DS210p	0x05
#define SYNO_MODEL_RS409rpp	0x0e
#define SYNO_MODEL_RS409p	0x0d
#define SYNO_MODEL_DS109p	0x0c
#define SYNO_MODEL_DS509p	0x0b
#define SYNO_MODEL_DS209p	0x0a
#define SYNO_MODEL_DS209pII	0x04
#define SYNO_MODEL_DS409p	0x09
#define SYNO_MODEL_DS110p	0x08
#define SYNO_MODEL_RS408rp	0x06
#define SYNO_MODEL_DS508	0x03
#define SYNO_MODEL_RS408	0x02    
#define SYNO_MODEL_DS408	0x01

#ifdef MY_ABC_HERE
extern char gszSerialNum[];
#endif

static int Uninitialize(void);

static int GetBrand(void)
{
#ifdef MY_ABC_HERE
	int Brand = -1;

	//YMXX[Z]SSSSS
	if ( gszSerialNum[4] == 'M' ) {
		Brand = BRAND_LOGITEC;
	} else if ( gszSerialNum[4] == 'U' ) {
		Brand = BRAND_SYNOLOGY_USA;
	} else {
		Brand = BRAND_SYNOLOGY;
	} 

	return Brand;
#else
	return BRAND_SYNOLOGY;
#endif
}

static int GetModel(void)
{
	char *ptr;
	char model;

	ptr = (char *)(SYNO_CPLD_BASE + CPLD_OFFSET_MODEL);
	model = ( *ptr & CPLD_MASK_MODEL ) >> CPLD_SHIFT_MODEL;

	switch (model) {
	case SYNO_MODEL_RS408rp:
		return MODEL_RS408rp;
	case SYNO_MODEL_DS508:
		return MODEL_DS508;
	case SYNO_MODEL_DS509p:
		return MODEL_DS509p;
	case SYNO_MODEL_RS408:
		return MODEL_RS408;
	case SYNO_MODEL_DS408:
		return MODEL_DS408;
	case SYNO_MODEL_DS409p:
		return MODEL_DS409p;
	case SYNO_MODEL_DS410:
		return MODEL_DS410;
	case SYNO_MODEL_RS409p:
		return MODEL_RS409p;
	case SYNO_MODEL_RS409rpp:
		return MODEL_RS409rpp;
	case SYNO_MODEL_DS209p:
		return MODEL_DS209p;
	case SYNO_MODEL_DS209pII:
		return MODEL_DS209pII;
	case SYNO_MODEL_DS210p:
		return MODEL_DS210p;
	case SYNO_MODEL_DS110p:
		return MODEL_DS110p;
	case SYNO_MODEL_DS109p:
		return MODEL_DS109p;
	default:
		return -EINVAL;
	}

	return -EINVAL;
}

static int 
InitModuleType(struct synobios_ops *ops)
{
	PRODUCT_MODEL model = ops->get_model();
	module_t type_509p = MODULE_T_DS509p;
	module_t type_rs409rpp = MODULE_T_RS409rpp;
	module_t type_rs409p = MODULE_T_RS409p;
	module_t type_409p = MODULE_T_DS409p;
	module_t type_410 = MODULE_T_DS410;
	module_t type_210p = MODULE_T_DS210p;
	module_t type_209pII = MODULE_T_DS209pII;
	module_t type_209p = MODULE_T_DS209p;
	module_t type_110p = MODULE_T_DS110p;
	module_t type_109p = MODULE_T_DS109p;
	module_t type_508 = MODULE_T_DS508;
	module_t type_rs408rp = MODULE_T_RS408rp;
	module_t type_rs408 = MODULE_T_RS408;
	module_t type_408 = MODULE_T_DS408;
	module_t *pType = NULL;
	
	switch (model) {
	case MODEL_RS408rp:
		pType = &type_rs408rp;
		break;
	case MODEL_DS508:
		pType = &type_508;
		break;
	case MODEL_DS509p:
		pType = &type_509p;
		break;
	case MODEL_RS408:
		pType = &type_rs408;
		break;
	case MODEL_DS408:
		pType = &type_408;
		break;
	case MODEL_DS409p:
		pType = &type_409p;
		break;
	case MODEL_DS410:
		pType = &type_410;
		break;
	case MODEL_RS409p:
		pType = &type_rs409p;
		break;
	case MODEL_RS409rpp:
		pType = &type_rs409rpp;
		break;
	case MODEL_DS209p:
		pType = &type_209p;
		break;
	case MODEL_DS209pII:
		pType = &type_209pII;
		break;
	case MODEL_DS210p:
		pType = &type_210p;
		break;
	case MODEL_DS110p:
		pType = &type_110p;
		break;
	case MODEL_DS109p:
		pType = &type_109p;
		break;
	default:
		break;
	}

	module_type_set(pType);
	return 0;
}

static int GetCPLDVersion(void)
{
	char *ptr;
	int Version;

	ptr = (char *)(SYNO_CPLD_BASE + CPLD_OFFSET_DRAM);
	Version = ( *ptr & CPLD_MASK_CPLD_REV ) >> CPLD_SHIFT_CPLD_REV;

	return Version;
}

static int SetDiskLedStatus(int disknum, SYNO_DISK_LED status)
{
	char *ptr;
	int led;
	int max_disk = 4;

	int model = GetModel();
	if ( model == MODEL_DS508 ||
	     model == MODEL_DS509p ) {
		max_disk = 5;
	} else if ( model == MODEL_DS209p ||
		model == MODEL_DS209pII ||
		model == MODEL_DS210p ) {
		max_disk = 2;
	} else if ( model == MODEL_DS109p ||
		 model == MODEL_DS110p ) {
		max_disk = 1; /* 109+ don't support this */
	}

	if (disknum < 1 || disknum > max_disk || 
			MODEL_DS109p == model || 
			MODEL_DS110p == model ) {
		return -EINVAL;
	}

	switch (status) {
	case DISK_LED_OFF:
		led = DS_LED_OFF;
		break;
	case DISK_LED_GREEN_SOLID:
		led = DS_LED_GREEN_SOLID;
		break;
	case DISK_LED_ORANGE_SOLID:
		led = DS_LED_ORANGE_SOLID;
		break;
	case DISK_LED_ORANGE_BLINK:
		led = DS_LED_ORANGE_BLINK;
		break;
	default:
		printk("%s (%d), error control to LED, status = %d\n", __FILE__, __LINE__, status);
		return -EINVAL;
	}

	if ( disknum < 5 ) {
		ptr = (char *)(SYNO_CPLD_BASE + CPLD_OFFSET_DISK_LED);
	} else {
		ptr = (char *)(SYNO_CPLD_BASE + CPLD_OFFSET_MODEL);
	}	
	switch (disknum) {
		case 1:
			led <<= CPLD_SHIFT_DISK1_LED;
			*ptr &= ~(CPLD_MASK_DISK1_LED);
			break;
		case 2:
			led <<= CPLD_SHIFT_DISK2_LED;
			*ptr &= ~(CPLD_MASK_DISK2_LED);
			break;
		case 3:
			led <<= CPLD_SHIFT_DISK3_LED;
			*ptr &= ~(CPLD_MASK_DISK3_LED);
			break;
		case 4:
			led <<= CPLD_SHIFT_DISK4_LED;
			*ptr &= ~(CPLD_MASK_DISK4_LED);
			break;
		case 5:
			led <<= CPLD_SHIFT_DISK5_LED;
			*ptr &= ~(CPLD_MASK_DISK5_LED);
			break;
		default:
			return -EINVAL;
	}
	*ptr |= led;

	return 0;
}

static int GetNFanStatus(int fanno, FAN_STATUS *pStatus)
{
	char *ptr;
	char FanStatus;
	int shift = 0, mask = 0;

	switch (fanno) {
		case 1:
			shift = CPLD_SHIFT_FAN_EXT1;
			mask = CPLD_MASK_FAN_EXT1;
			break;
		case 2:
			shift = CPLD_SHIFT_FAN_EXT2;
			mask = CPLD_MASK_FAN_EXT2;
			break;
		case 3:
			shift = CPLD_SHIFT_FAN_EXT3;
			mask = CPLD_MASK_FAN_EXT3;
			break;
		case 4:
			shift = CPLD_SHIFT_FAN_EXT4;
			mask = CPLD_MASK_FAN_EXT4;
			break;
		default:
			return -EINVAL;
	}

	ptr = (char *)(SYNO_CPLD_BASE + CPLD_OFFSET_FAN_STATUS);
	FanStatus = (*ptr & mask) >> shift;	

	if ((int)FanStatus == 0) {
		*pStatus = FAN_STATUS_STOP;
	} else {
		*pStatus = FAN_STATUS_RUNNING;
	}

	return 0;
}

static int GetFanStatus(int fanno, FAN_STATUS *pStatus)
{
	int model = GetModel();

	if (model == MODEL_RS408rp ||
	    model == MODEL_RS409rpp) {
		if ( fanno > 0 && fanno <= 4 ) {
			return GetNFanStatus(fanno, pStatus);
		}
	} else if (model == MODEL_RS408 ||
		   model == MODEL_RS409p) {
		if ( fanno > 0 && fanno <= 3 ) {
			return GetNFanStatus(fanno, pStatus);
		}
	} else if (model == MODEL_DS508 ||
		   model == MODEL_DS509p ||
		   model == MODEL_DS409p ||
		   model == MODEL_DS410) {
		if ( fanno > 0 && fanno <= 2 ) {
			return GetNFanStatus(fanno, pStatus);
		}
	} else {
		if ( fanno == 1 ) {
			return GetNFanStatus(fanno, pStatus);
		}
	}

	return -EINVAL;
}

int
FanStatusMapping409pv20(FAN_STATUS status, FAN_SPEED speed, char *pSpeed_value)
{
	int ret = -1;

	if (status == FAN_STATUS_STOP) {
		*pSpeed_value = CPLD_FAN_SPEED_0;
	} else {
		switch (speed) {
		case FAN_SPEED_STOP:
			*pSpeed_value = CPLD_FAN_SPEED_0;
			break;
		case FAN_SPEED_ULTRA_LOW:
			*pSpeed_value = CPLD_FAN_SPEED_5;
			break;
		case FAN_SPEED_VERY_LOW:
		case FAN_SPEED_TEST_1:
			*pSpeed_value = CPLD_FAN_SPEED_4;
			break;
		case FAN_SPEED_LOW:	
		case FAN_SPEED_TEST_2:
			*pSpeed_value = CPLD_FAN_SPEED_1;
			break;
		case FAN_SPEED_MIDDLE:	
		case FAN_SPEED_TEST_4:
			*pSpeed_value = CPLD_FAN_SPEED_2;
			break;
		case FAN_SPEED_HIGH:
			*pSpeed_value = CPLD_FAN_SPEED_6;
			break;
		case FAN_SPEED_VERY_HIGH:
			*pSpeed_value = CPLD_FAN_SPEED_7;
			break;
		case FAN_SPEED_ULTRA_HIGH:
		case FAN_SPEED_FULL:
			*pSpeed_value = CPLD_FAN_SPEED_3;
			break;
		default:
			goto END;
		}
	}

	ret = 0;
END:
	return ret;
}

int
FanStatusMapping410(FAN_STATUS status, FAN_SPEED speed, char *pSpeed_value)
{
	int ret = -1;

	if (status == FAN_STATUS_STOP) {
		*pSpeed_value = CPLD_FAN_SPEED_0;
	} else {
		switch (speed) {
		case FAN_SPEED_STOP:
			*pSpeed_value = CPLD_FAN_SPEED_0;
			break;
		case FAN_SPEED_ULTRA_LOW:
		case FAN_SPEED_TEST_1:
			*pSpeed_value = CPLD_FAN_SPEED_4;
			break;
		case FAN_SPEED_VERY_LOW:
		case FAN_SPEED_TEST_2:
			*pSpeed_value = CPLD_FAN_SPEED_5;
			break;
		case FAN_SPEED_LOW:
			*pSpeed_value = CPLD_FAN_SPEED_2;
			break;
		case FAN_SPEED_MIDDLE:
		case FAN_SPEED_TEST_4:
			*pSpeed_value = CPLD_FAN_SPEED_1;
			break;
		case FAN_SPEED_HIGH:
			*pSpeed_value = CPLD_FAN_SPEED_7;
			break;
		case FAN_SPEED_VERY_HIGH:
			*pSpeed_value = CPLD_FAN_SPEED_6;
			break;
		case FAN_SPEED_ULTRA_HIGH:
		case FAN_SPEED_FULL:
			*pSpeed_value = CPLD_FAN_SPEED_3;
			break;
		default:
			goto END;
		}
	}

	ret = 0;
END:
	return ret;
}

static int
SetFanSpeedValue3bits(
	FAN_STATUS status, 
	FAN_SPEED speed,
	int (*FanSpeedMapping)(FAN_STATUS , FAN_SPEED , char *)
	)
{
	char *fan_register, speed_value;
	int res = -EINVAL;

	fan_register = (char *)(SYNO_CPLD_BASE + CPLD_OFFSET_FAN_STATUS);

	if (NULL == FanSpeedMapping) {
		goto END;
	}

	if (FanSpeedMapping(status, speed, &speed_value)) {
		goto END;
	}

	// DS2.0 #8126
	*fan_register &= ~DS509P_CPLD_FAN_SPEED_MASK;
	*fan_register |= (FAN_ACTIVATION_SPEED << CPLD_SHIFT_FAN_SPEED);
	mdelay(FAN_ACTIVATION_DURATION);

	speed_value <<= CPLD_SHIFT_FAN_SPEED;
	/* set fan speed control bit to 00 */
	*fan_register &= ~DS509P_CPLD_FAN_SPEED_MASK;
	/* set current fan speed */
	*fan_register |= speed_value;

	res = 0;
END:
	return res;
}

static int SetFanStatus(FAN_STATUS status, FAN_SPEED speed)
{
	char *fan_register, speed_value;
	int res = -EINVAL;
	int model = GetModel();

	switch (model) {
	case MODEL_DS508:
	case MODEL_DS408:
	case MODEL_DS209p:
		break;
	case MODEL_DS409p:
#ifdef MY_ABC_HERE
		if (syno_is_hw_version(HW_DS409pv20)) {
			res = SetFanSpeedValue3bits(status, speed, FanStatusMapping409pv20);
			goto END;
		}
#endif
		break;
	case MODEL_DS410:
		res = SetFanSpeedValue3bits(status, speed, FanStatusMapping410);
		goto END;
	case MODEL_DS109p:
	case MODEL_DS110p:
	case MODEL_DS209pII:
	case MODEL_DS210p:
		res = SetFanSpeedValue3bits(status, speed, FanStatusMappingType1);
		goto END;
	case MODEL_DS509p:
		res = SetFanSpeedValue3bits(status, speed, FanStatusMappingType2);
		goto END;
	default:
		goto END;
	}
	
	fan_register = (char *)(SYNO_CPLD_BASE + CPLD_OFFSET_FAN_STATUS);

	if(status == FAN_STATUS_STOP) {
		/* currently not support fan off */
		goto END;
	}

	switch (speed) {
		case FAN_SPEED_ULTRA_LOW:
		{
			speed_value = CPLD_FAN_SPEED_ULTRA_LOW;
			break;
		}
		case FAN_SPEED_LOW:
		{
			speed_value = CPLD_FAN_SPEED_LOW;
			break;
		}
		case FAN_SPEED_MIDDLE:
		{
			speed_value = CPLD_FAN_SPEED_MIDDLE;
			break;
		}
		case FAN_SPEED_HIGH:
		case FAN_SPEED_FULL:
		{
			speed_value = CPLD_FAN_SPEED_FULL;
			break;
		}
		default:
			goto END;
			break;
	}
	speed_value <<= CPLD_SHIFT_FAN_SPEED_CONTROL;

	/* set fan speed control bit to 00 */
	*fan_register &= ~CPLD_FAN_SPEED_MASK;
	/* set current fan speed */
	*fan_register |= speed_value;
	
	res = 0;
END:
	return res;
}

#define I2C_RTC_ADDR            0x32
#define I2C_TEMPERATURE_ADDR    0x48

static int GetSysTemperature(int *Temperature)
{
	int count = 0;
	u16 data = 0;

	count = mpc_i2c_read(I2C_TEMPERATURE_ADDR, (u8 *)&data, 2, 0, 0);
	if (count != 2) {
		return -1;
	}

	/* The temperature data only 9 bits */
	data = data >> 7;

	if (data >> 8) { /* bit 9 is minus sign */
		*Temperature = -1 * (0x100 - ((u8 *)&data)[1]); 
	} else {
		*Temperature = data;
	}

	return 0;
}

static int GetCpldReg(CPLDREG *pCpld)
{
	unsigned char *ptr;
	ptr = (unsigned char *)SYNO_CPLD_BASE;
	pCpld->diskledctrl = *ptr++;
	pCpld->diskpowerstate = *ptr++;
	pCpld->modelnumber = *ptr++;
	pCpld->fanstatus = *ptr;
	return 0;

}

static int SetMemByte( MEMORY_BYTE *pMemory )
{
	if ( NULL == pMemory ||
		(SYNO_CPLD_END - SYNO_CPLD_BASE) <= pMemory->offset ){
		return -1;
	}

	*((unsigned char *)(SYNO_CPLD_BASE + pMemory->offset)) = pMemory->value;
	return 0;
}

static int GetMemByte( MEMORY_BYTE *pMemory )
{
	if ( NULL == pMemory ||
		(SYNO_CPLD_END - SYNO_CPLD_BASE) <= pMemory->offset ){
		return -1;
	}

	pMemory->value = *((unsigned char *)(SYNO_CPLD_BASE + pMemory->offset));
	return 0;
}

static int SetAlarmLed(unsigned char type)
{
	int model = GetModel();

	if( MODEL_DS508 != model &&
		MODEL_DS509p != model ) {
		return -1;
	}

	if(type) {
		*((unsigned char *)SYNO_CPLD_BASE + CPLD_OFFSET_MODEL) &= 0xDF;
	}else{
		*((unsigned char *)SYNO_CPLD_BASE + CPLD_OFFSET_MODEL) |= 0x20;		
	}
	return 0;
}

static int GetPowerStatus(POWER_INFO *power_info)
{
	unsigned char value;
	int model = GetModel();

	if(MODEL_RS408rp != model &&
		MODEL_RS409rpp != model) {
		return -1;
	}

	value = *((unsigned char *)SYNO_CPLD_BASE+CPLD_OFFSET_POWER_STATUS_OFFSET);

	if(!(value & CPLD_POWER_1_GOOD)) {
		power_info->power_1 = POWER_STATUS_BAD;
	}else{
		power_info->power_1 = POWER_STATUS_GOOD;
	}

	if(!(value & CPLD_POWER_2_GOOD)) {
		power_info->power_2 = POWER_STATUS_BAD;
	}else{
		power_info->power_2 = POWER_STATUS_GOOD;
	}

	return 0;
}

static int GetBuzzerCleared(unsigned char *buzzer_cleared)
{
	unsigned char value;
	int model = GetModel();

	if(MODEL_RS408rp != model &&
		MODEL_RS409rpp != model &&
		MODEL_RS409p != model ) {
		return -1;
	}

	value = *((unsigned char *)SYNO_CPLD_BASE+CPLD_OFFSET_POWER_STATUS_OFFSET);

	if(!(value & CPLD_CLEAR_BUZZER)) {
		*buzzer_cleared = 1;
	}else{
		*buzzer_cleared = 0;
	}

	return 0;
}

static struct synobios_ops synobios_ops = {
	.owner               = THIS_MODULE,
	.get_brand           = GetBrand,
	.get_model           = GetModel,
	.get_cpld_version    = GetCPLDVersion,
	.get_rtc_time        = rtc_ricoh_get_time,
	.set_rtc_time        = rtc_ricoh_set_time,
	.get_fan_status      = GetFanStatus,
	.set_fan_status      = SetFanStatus,
	.get_sys_temperature = GetSysTemperature,
	.set_disk_led        = SetDiskLedStatus,
	.get_cpld_reg        = GetCpldReg,
	.get_auto_poweron    = rtc_ricoh_get_auto_poweron,
	.set_auto_poweron    = rtc_ricoh_set_auto_poweron,
	.init_auto_poweron   = rtc_ricoh_auto_poweron_init,
	.uninit_auto_poweron = rtc_ricoh_auto_poweron_uninit,
	.set_mem_byte        = SetMemByte,
	.get_mem_byte        = GetMemByte,
	.set_alarm_led       = SetAlarmLed,
	.get_buzzer_cleared  = GetBuzzerCleared,
	.get_power_status    = GetPowerStatus,
	.module_type_init	 = InitModuleType,
	.uninitialize		 = Uninitialize,
};

int synobios_model_init(struct file_operations *fops, struct synobios_ops **ops)
{
	*ops = &synobios_ops;

	mpc_i2c_init();

	if( synobios_ops.init_auto_poweron ) {
		synobios_ops.init_auto_poweron();
	}

	return 0;
}

static int Uninitialize(void)
{
	if( synobios_ops.uninit_auto_poweron ) {
		synobios_ops.uninit_auto_poweron();
	}

	return 0;
}

int synobios_model_cleanup(struct file_operations *fops, struct synobios_ops **ops)
{
	return 0;
}
