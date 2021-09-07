#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
// Copyright (c) 2000-2009 Synology Inc. All rights reserved.

#include <linux/synobios.h>
#include "../mapping.h"

int GetModel(void);
int SetDiskLedStatus(int disknum, SYNO_DISK_LED status);
int GetFanStatus(int fanno, FAN_STATUS *pStatus);
int GetFanNum(int *pFanNum);
int SetFanStatus(FAN_STATUS status, FAN_SPEED speed);
int GetSysTemperature(int *Temperature);
int SetAlarmLed(unsigned char type);
int GetBackPlaneStatus(BACKPLANE_STATUS *pStatus);
int GetFanSpeedBits(int start, int end, MEMORY_BYTE *pMemory);
int GetMemByte( MEMORY_BYTE *pMemory );
int InitModuleType(struct synobios_ops *ops);

extern int SYNO_CTRL_FAN_STATUS_GET(int index, int *pValue);
extern int SYNO_CTRL_FAN_PERSISTER(int index, int status, int isWrite);
extern int SYNO_CTRL_INTERNAL_HDD_LED_SET(int index, int status);
extern int SYNO_CTRL_EXT_CHIP_HDD_LED_SET(int index, int status);
#if defined(MY_ABC_HERE)
extern int SYNO_CTRL_HDD_POWERON(int index, int *pValue, int isWrite);
#elif defined(CONFIG_MACH_SYNOLOGY_6281)
extern int SYNO_CTRL_HDD_POWERON(int index, int value);
#endif
extern int SYNO_MV6281_GPIO_PIN(int pin, int *value, int isWrite);
extern int SYNO_MV6281_GPIO_CLASS(int class, int *value, int isWrite);
extern int SYNO_CTRL_ALARM_LED_SET(int status);
extern int SYNO_CTRL_BACKPLANE_STATUS_GET(int *pStatus);
extern int SYNO_CTRL_BUZZER_CLEARED_GET(int *pValue);
