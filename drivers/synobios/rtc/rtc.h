#include <linux/synobios.h>

int rtc_ricoh_get_time(struct _SynoRtcTimePkt *pRtcTimePkt);
int rtc_ricoh_set_time(struct _SynoRtcTimePkt *pRtcTimePkt);
int rtc_ricoh_get_auto_poweron(SYNO_AUTO_POWERON *pAutoPowerOn);
int rtc_ricoh_set_auto_poweron(SYNO_AUTO_POWERON *pAutoPowerOn);
int rtc_ricoh_auto_poweron_init(void);
int rtc_ricoh_auto_poweron_uninit(void);

int rtc_seiko_get_time(struct _SynoRtcTimePkt *pRtcTimePkt);
int rtc_seiko_set_time(struct _SynoRtcTimePkt *pRtcTimePkt);
int rtc_seiko_get_auto_poweron(SYNO_AUTO_POWERON *pAutoPowerOn);
int rtc_seiko_set_auto_poweron(SYNO_AUTO_POWERON *pAutoPowerOn);
int rtc_seiko_auto_poweron_init(void);
int rtc_seiko_auto_poweron_uninit(void);

int rtc_bandon_get_time(struct _SynoRtcTimePkt* pRtcTimePkt);
int rtc_bandon_set_time(struct _SynoRtcTimePkt* pRtcTimePkt);
int rtc_bandon_get_auto_poweron(SYNO_AUTO_POWERON* pAutoPowerOn);
int rtc_bandon_set_auto_poweron(SYNO_AUTO_POWERON* pAutoPowerOn);
int rtc_bandon_auto_poweron_uninit(void);
int rtc_bandon_auto_poweron_uninit(void);
