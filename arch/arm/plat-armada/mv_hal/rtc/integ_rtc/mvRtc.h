#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __INCmvRtch
#define __INCmvRtch

#ifdef __cplusplus
extern "C" {
#endif

#include "mvCommon.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"
#include "mvSysRtcConfig.h"

typedef struct time {
	MV_U8  seconds;
	MV_U8  minutes;
	MV_U8  hours;
	MV_U8  day;
	MV_U8  date;
	MV_U8  month;
	MV_U8  century;
	MV_U8  year;
} MV_RTC_TIME;

MV_VOID mvRtcInit(MV_VOID);

MV_VOID mvRtcTimeSet(MV_RTC_TIME *time);

MV_VOID mvRtcTimeGet(MV_RTC_TIME *time);

MV_VOID mvRtcAlarmSet(MV_RTC_TIME *time);

#ifdef CONFIG_SYNO_ARMADA
MV_VOID SYNOmvRtcExtAlarmSet(MV_U32 time);
MV_VOID SYNOmvRtcExtAlarmClean(void);
#endif

#ifdef __cplusplus
}
#endif

#endif   
