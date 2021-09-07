#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#include <linux/syno.h>

#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/errno.h>  /* error codes */
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/genhd.h>
#include <linux/major.h>
#include <asm/uaccess.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/smp_lock.h>
#include <linux/sched.h>

#include <linux/synobios.h>
#include <linux/ioport.h>
#include "mapping.h"

#ifdef MY_ABC_HERE
#include <linux/raid/libmd-report.h>
#endif

#if 0
#define	DBGMESG(x...)	printk(x)
#else
#define	DBGMESG(x...)
#endif

static int check_fan = 1;
module_param(check_fan, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(check_fan, "seconds to delay before using a new device");

static struct synobios_ops *synobios_ops;
int synobios_model_init(struct file_operations *fops, struct synobios_ops **ops);
int synobios_model_cleanup(struct file_operations *fops, struct synobios_ops **ops);

struct sd_softc {
	int	countEvents;
	int	idxPtr;
	SYNOBIOSEVENT	rgEvents[SYNOBIOS_NEVENTS];
	wait_queue_head_t wq_poll;
};
static struct sd_softc scSynoBios;
static SYNO_SYS_STATUS *pgSysStatus = NULL;

#ifdef MY_ABC_HERE
extern int (*funcSYNOGetHwCapability)(CAPABILITY *);
#endif
#ifdef MY_ABC_HERE
extern int (*funcSYNOSendEboxRefreshEvent)(int portIndex);
#endif

static int synobios_record_event_new(struct sd_softc *sc, SYNOBIOSEVENT *pEvent)
{
	if (scSynoBios.countEvents == SYNOBIOS_NEVENTS) {
		return 1;
	}

	scSynoBios.countEvents++;
	scSynoBios.rgEvents[sc->idxPtr] = *pEvent;
	scSynoBios.idxPtr++;
	scSynoBios.idxPtr %= SYNOBIOS_NEVENTS;

	wake_up_interruptible(&(scSynoBios.wq_poll));

	return 0;
}

static int synobios_record_event(struct sd_softc *sc, u_int event_type)
{
	SYNOBIOSEVENT   event;
	
	event.event = event_type;
	event.data1 = event.data2 = event.data3 = event.data4 = 0;
	return synobios_record_event_new(sc, &event);
}

static int synobios_record_shutdown_event(unsigned int type, SYNO_SHUTDOWN_LOG shutdown_event)
{
	int ret;
	SYNOBIOSEVENT   event;

	event.event = SYNO_EVENT_SHUTDOWN_LOG;
	event.data1 = shutdown_event;
	event.data2 = event.data3 = event.data4 = 0;
	ret = synobios_record_event_new(&scSynoBios, &event);

	return ret;
}

#ifdef MY_ABC_HERE
static int synobios_record_raid_event(unsigned int type, unsigned int raidno, unsigned int diskno, unsigned int sector)
{
	int ret;
	SYNOBIOSEVENT   event;

	event.event = SYNO_EVENT_RAID;
	event.data1 = type;
	event.data2 = raidno;
	event.data3 = diskno + 1;   // scemd record disk1,2,3,4, 
								// raid driver use disk0,1,2,3
	event.data4 = sector;	

	ret = synobios_record_event_new(&scSynoBios, &event);

	return ret;
}
#endif

#ifdef MY_ABC_HERE
static int synobios_record_ebox_refresh_event(int portIndex)
{
	int ret = 0;
	SYNOBIOSEVENT event;

	event.event = SYNO_EVENT_EBOX_REFRESH;
	event.data1 = portIndex;
	event.data2 = 0;
	event.data3 = 0;
	event.data4 = 0;

	ret = synobios_record_event_new(&scSynoBios, &event);

	return ret;
}
#endif

void synobios_rtc_init(void)
{
	int ret;
	struct _SynoRtcTimePkt RtcTimePkt;
	struct timespec tv;

	/* 1. read time from rtc. */
	if (synobios_ops->get_rtc_time) {
		ret = synobios_ops->get_rtc_time(&RtcTimePkt);
	}else{
		ret = -1;
	}

	if (ret < 0) {
		printk("%s(%d) read RTC error\n", __FILE__, __LINE__);
	}
	//printk("%s(%d) XX YYYY/MM/DD hh:mm:ss %04x/%02x/%02x %02x:%02x:%02x\n", __FILE__, __LINE__, RtcTimePkt.year, RtcTimePkt.month, RtcTimePkt.day, RtcTimePkt.hour, RtcTimePkt.min, RtcTimePkt.sec);
#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
	tv.tv_sec = mktime(RtcTimePkt.year + 1900, RtcTimePkt.month + 1, RtcTimePkt.day, RtcTimePkt.hour, RtcTimePkt.min, RtcTimePkt.sec);
	tv.tv_nsec = 0;
#else
	/* Make RTC time start from year 2000. */
	if ( (signed char)RtcTimePkt.year < 0) {
		RtcTimePkt.year = 0;
	}
	RtcTimePkt.year = RtcTimePkt.year + 0x30;
	RtcTimePkt.year = BCD_TO_BIN(RtcTimePkt.year);

	RtcTimePkt.month = BCD_TO_BIN(RtcTimePkt.month);
	RtcTimePkt.day = BCD_TO_BIN(RtcTimePkt.day);
	RtcTimePkt.min = BCD_TO_BIN(RtcTimePkt.min);
	RtcTimePkt.sec = BCD_TO_BIN(RtcTimePkt.sec);
	if (RtcTimePkt.hour == 0x12) {
		RtcTimePkt.hour = 0;
	} else if (RtcTimePkt.hour == 0x32) {
		RtcTimePkt.hour = 12;
	} else if (RtcTimePkt.hour > 0x20) {
		RtcTimePkt.hour = BCD_TO_BIN(RtcTimePkt.hour) - 20 + 12;
	} else {
		RtcTimePkt.hour = BCD_TO_BIN(RtcTimePkt.hour);
	}

	/* 2. set xscale timer as rtc value. */
	/*printk("%s(%d) YYYY/MM/DD hh:mm:ss %04d/%02d/%02d %02d:%02d:%02d\n", __FILE__, __LINE__, RtcTimePkt.year+1970, RtcTimePkt.month, RtcTimePkt.day, RtcTimePkt.hour, RtcTimePkt.min, RtcTimePkt.sec);*/
	tv.tv_sec = mktime(RtcTimePkt.year+1970, RtcTimePkt.month, RtcTimePkt.day, RtcTimePkt.hour, RtcTimePkt.min, RtcTimePkt.sec);
	tv.tv_nsec = 0;
#endif
	do_settimeofday(&tv);
}

int update_comp_stat(SYNO_SYS_STATUS *pSysStatus, sys_comp_stat_t com_stat)
{
	int res = 0;	
	int comp_num = sizeof(SYNO_SYS_STATUS)/sizeof(sys_comp_stat_t);
	SYNO_SYS_STAT_SIGNATURE signature = SIGNATURE_GET(com_stat);
	int idx;
	sys_comp_stat_t *pCom_stat;

	pCom_stat = (sys_comp_stat_t *)pSysStatus;
	for (idx = 0; idx < comp_num; idx++, pCom_stat++) {
		SYNO_SYS_STAT_SIGNATURE comp_signature = SIGNATURE_GET((*pCom_stat));
		if (signature == comp_signature) {			
			*pCom_stat = com_stat;
			break;
		}
	}	

	if (idx == comp_num) {
		res = -1;
	}

	return res;
}

static unsigned int synobios_poll(struct file *pfile, struct poll_table_struct *ppolltable)
{
	int revents = 0;

	if(synobios_ops->get_buzzer_cleared) {
		unsigned char buzzer_cleared = 0;
		if ( 0 == synobios_ops->get_buzzer_cleared(&buzzer_cleared) ) {
			if ( buzzer_cleared ) {
				synobios_record_event(&scSynoBios, SYNO_EVENT_BUTTON_BUZZER_CLEAR);
				printk("synobios: buzzer stop button pressed\n");
			}
		}
	}

	if (scSynoBios.countEvents) {
		revents |= (POLLIN | POLLRDNORM);
	} else {
		poll_wait(pfile, &(scSynoBios.wq_poll), ppolltable);
	}
	return (revents);
}							

static int synobios_ioctl (struct inode *inode, struct file *filp,
				 unsigned int cmd, unsigned long arg) 
{
	struct _SynoRtcTimePkt *pRtcTimePkt;
	int ret = 0;
	int i;
	DEFINE_RWLOCK(sys_status_lock);

	if (_IOC_TYPE(cmd) != SYNOBIOS_IOC_MAGIC) {
		ret = -ENOTTY;
		goto END;
	}

	switch (cmd) {
	case SYNOIO_NEXTEVENT:
		if (scSynoBios.countEvents < 0) {
			ret = -EINVAL;
		} else if (scSynoBios.countEvents == 0) {
			ret = -EAGAIN;
		} else {
			/*XXX??? need mutex*/
			i = scSynoBios.idxPtr + SYNOBIOS_NEVENTS - scSynoBios.countEvents;
			i %= SYNOBIOS_NEVENTS;
			copy_to_user((void __user *)arg, &scSynoBios.rgEvents[i], sizeof(SYNOBIOSEVENT));
			scSynoBios.countEvents--;			
		}
		break;
	case SYNOIO_RTC_TIME_READ:
		pRtcTimePkt = (struct _SynoRtcTimePkt *)arg;
		if (synobios_ops->get_rtc_time) {		
			ret = synobios_ops->get_rtc_time(pRtcTimePkt);
		}else{
			ret = -1;
		}
		if (ret < 0) {
			printk("%s: Failed to get rtc time.\n", __FUNCTION__);
		}
#if !defined(MY_DEF_HERE) && !defined(MY_DEF_HERE)
		if ( (signed char)pRtcTimePkt->year < 0) {
			pRtcTimePkt->year = 0;
		}
#endif
		DBGMESG("(0h, %x) (1h, %x) (2h, %x) (3h, %x) (4h, %x) (5h, %x) (6h, %x)\n", (unsigned int)pRtcTimePkt->sec, (unsigned int)pRtcTimePkt->min, (unsigned int)pRtcTimePkt->hour, (unsigned int)pRtcTimePkt->weekday, (unsigned int)pRtcTimePkt->day, (unsigned int)pRtcTimePkt->month, (unsigned int)pRtcTimePkt->year); 
		break;
	case SYNOIO_RTC_TIME_WRITE:
		DBGMESG("synobios_ioctl: SYNOIO_RTC_TIME_WRITE\n");
		pRtcTimePkt = (struct _SynoRtcTimePkt *)arg;
#if !defined(MY_DEF_HERE) && !defined(MY_DEF_HERE)
		if ( (signed char)pRtcTimePkt->year < 0) {
			pRtcTimePkt->year = 0;
		}
#endif
		if (synobios_ops->set_rtc_time) {
			ret = synobios_ops->set_rtc_time(pRtcTimePkt);
		}else{
			ret=-1;
		}
		if (ret < 0) {
			printk("%s: Failed to set rtc time\n", __FUNCTION__);
		}
		DBGMESG("(0h, %x) (1h, %x) (2h, %x) (3h, %x) (4h, %x) (5h, %x) (6h, %x)\n", (unsigned int)pRtcTimePkt->sec, (unsigned int)pRtcTimePkt->min, (unsigned int)pRtcTimePkt->hour, (unsigned int)pRtcTimePkt->weekday, (unsigned int)pRtcTimePkt->day, (unsigned int)pRtcTimePkt->month, (unsigned int)pRtcTimePkt->year);
		break;
	
    case SYNOIO_MANUTIL_MODE:
        if (*(int *)arg != 0) {
            /*for manutil test, send manutil mode swich event*/
            printk(KERN_INFO "synobios_ioctl: MANUTIL BUTTON MODE\n");
            ret = synobios_record_event(&scSynoBios, SYNO_EVENT_BUTTON_MANUTIL);
        } else {
            printk(KERN_INFO"synobios_ioctl: NORMAL BUTTON MODE\n");
            ret = synobios_record_event(&scSynoBios, SYNO_EVENT_BUTTON_NORMAL);
        }
		break;
	case SYNOIO_RECORD_EVENT:
		/*for event test, generate event from user space*/
		printk(KERN_INFO "synobios_ioctl: SYNOIO_RECORD_EVENT, event id %x\n", *((u_int *) arg));
		ret = synobios_record_event(&scSynoBios, *(u_int *)arg);
		break;
	
	case	SYNOIO_BUTTON_RESET:
		ret = synobios_record_event(&scSynoBios, SYNO_EVENT_BUTTON_RESET);
		printk("synobios: reset button pressed, ret = %d\n", ret);
		break;
	case	SYNOIO_BUTTON_POWER:
		ret = synobios_record_event(&scSynoBios, SYNO_EVENT_BUTTON_SHUTDOWN);
		printk("synobios: power button pressed, ret = %d\n", ret);
		break;
	case	SYNOIO_BUTTON_USB:
		ret = synobios_record_event(&scSynoBios, SYNO_EVENT_USBCOPY_START);
		printk("synobios: usb button pressed, ret = %d\n", ret);
		break;
	case	SYNOIO_SET_DISK_LED:
		{
			DISKLEDSTATUS*   pDiskLedStatus = (DISKLEDSTATUS *)arg;
			if (synobios_ops->set_disk_led) {
				ret = synobios_ops->set_disk_led(pDiskLedStatus->diskno, pDiskLedStatus->status);
			}else{
				ret=-1;
			}			
			break;
		}
	case	SYNOIO_GET_FAN_STATUS:
		{
			FANSTATUS*  pFanStatus = (FANSTATUS *)arg;
			if (check_fan == 0) {
				pFanStatus->status = FAN_STATUS_RUNNING;
				return 0;
			}
			if (synobios_ops->get_fan_status) {
				ret = synobios_ops->get_fan_status(pFanStatus->fanno, &pFanStatus->status);
			}else{
				ret=-1;
			}
			
			break;
		}
	case	SYNOIO_SET_FAN_STATUS:
		{
			FANSTATUS*  pFanStatus = (FANSTATUS *)arg;

			if (synobios_ops->set_fan_status) {			
				ret = synobios_ops->set_fan_status(pFanStatus->status, pFanStatus->speed);
			}else{
				ret = -1;
			}
			
			break;
		}
	case	SYNOIO_GET_FAN_NUM:
		{
			int*  pFanNum = (int *)arg;
			ret = GetFanNum(pFanNum);
			break;
		}
	case	SYNOIO_GET_DS_BRAND:
		{
			int *pBrand = (int *)arg;

			if (synobios_ops->get_brand) {
				*pBrand = synobios_ops->get_brand();
			}			

			if (*pBrand != BRAND_SYNOLOGY && *pBrand != BRAND_LOGITEC && *pBrand != BRAND_SYNOLOGY_USA) {
				ret = -EINVAL;
			}
			break;
		}
	case	SYNOIO_GET_DS_MODEL:
		{
			int* pModel = (int *)arg;
			if (synobios_ops->get_model) {
				*pModel = synobios_ops->get_model();
			}
			
			break;
		}
	case	SYNOIO_GET_CPLD_VERSION:
		{
			int *pVersion = (int *)arg;
			if (synobios_ops->get_cpld_version) {
				*pVersion = synobios_ops->get_cpld_version();
			}
			
			break;
		}
	case	SYNOIO_GET_TEMPERATURE:
		{
			int *Temperature = (int *)arg;

			if (synobios_ops->get_sys_temperature) {
				ret = synobios_ops->get_sys_temperature(Temperature);
			}else{
				ret=-1;
			}
			
			break;
		}
	case    SYNOIO_GET_CPLD_REG:
		{
			CPLDREG *pCpld = (CPLDREG *)arg;
			if (synobios_ops->get_cpld_reg) {
				ret = synobios_ops->get_cpld_reg(pCpld);
			}else{
				ret=-1;
			}			
			break;
		}
	case    SYNOIO_SET_MEM_BYTE:
        	{
			MEMORY_BYTE   *pMemory = (MEMORY_BYTE *)arg;
			if (synobios_ops->set_mem_byte) {
				ret = synobios_ops->set_mem_byte(pMemory);
			}else{
				ret=-1;
			}			
			break;
		}
	case    SYNOIO_GET_MEM_BYTE:
		{
			MEMORY_BYTE   *pMemory = (MEMORY_BYTE *)arg;
			if (synobios_ops->get_mem_byte) {
				ret = synobios_ops->get_mem_byte(pMemory);
			}else{
				ret=-1;
			}			
			break;
		}
	case    SYNOIO_GPIO_PIN_WRITE:
                {
                        GPIO_PIN *pPin = (GPIO_PIN *)arg;
						if (synobios_ops->set_gpio_pin) {
							ret = synobios_ops->set_gpio_pin(pPin);
						}else{
							ret=-1;
						}                        
                        break;
                }
        case    SYNOIO_GPIO_PIN_READ:
                {
                        GPIO_PIN *pPin = (GPIO_PIN *)arg;         
						if (synobios_ops->get_gpio_pin) {
							ret = synobios_ops->get_gpio_pin(pPin);
						}else{
							ret=-1;
						}                        
                        break;
                }
	case SYNOIO_GET_AUTO_POWERON:
		{
			SYNO_AUTO_POWERON *pAutoPowerOn = (SYNO_AUTO_POWERON *)arg;
			if (synobios_ops->get_auto_poweron) {
				ret = synobios_ops->get_auto_poweron(pAutoPowerOn);
			}else{
				ret=-1;
			}
		break;
		}
	case SYNOIO_SET_AUTO_POWERON:
		{
			SYNO_AUTO_POWERON *pAutoPowerOn = (SYNO_AUTO_POWERON *)arg;
			if (synobios_ops->set_auto_poweron) {
				ret = synobios_ops->set_auto_poweron(pAutoPowerOn);
			}else{
				ret=-1;
			}			
			break;
		}
	case SYNOIO_GET_HW_CAPABILITY:
		{
			CAPABILITY *pCapability = (CAPABILITY *)arg;
			ret = GetHwCapability(pCapability);
			break;
		}
	case SYNOIO_SET_ALARM_LED:
		{
			if(synobios_ops->set_alarm_led) {
				ret = synobios_ops->set_alarm_led((unsigned char)arg);
			}else{
				ret = -1;
			}
			break;
		}
	case SYNOIO_GET_BUZZER_CLEARED:
		{
			//for manutil
			unsigned char *pucBuzzer_cleared = (unsigned char *)arg;
			if(synobios_ops->get_buzzer_cleared) {
				ret = synobios_ops->get_buzzer_cleared(pucBuzzer_cleared);
				if ( *pucBuzzer_cleared ) {
					printk("synobios: buzzer stop button pressed, ret = %d\n", ret);
				}
			}else{
				ret = -1;
			}
			break;
		}
	case SYNOIO_GET_POWER_STATUS:
		{
			if(synobios_ops->get_power_status) {
				ret = synobios_ops->get_power_status((POWER_INFO *)arg);
			}else{
				ret = -1;
			}
			break;
		}
	case SYNOIO_SHUTDOWN_LOG:
		{			
			int event = (SYNO_SHUTDOWN_LOG)arg;
			ret = synobios_record_shutdown_event(SYNO_EVENT_SHUTDOWN_LOG, event);
			break;
		}
	case SYNOIO_UNINITIALIZE:
		{
			if(synobios_ops->uninitialize) {
				ret = synobios_ops->uninitialize();
			}else{
				ret = -1;
			}
			break;
		}
	case SYNOIO_GET_SYS_STATUS:
		{
			SYNO_SYS_STATUS *pUSysStat = (SYNO_SYS_STATUS *)arg;
			if (NULL != pUSysStat){
				read_lock(&sys_status_lock);
				copy_to_user(pUSysStat, pgSysStatus, sizeof(SYNO_SYS_STATUS));
				read_unlock(&sys_status_lock);
			} else{
				ret = -1;
			}
			break;
		}
	case SYNOIO_SET_SYS_STATUS:
		{
			sys_comp_stat_t uSysStat = (sys_comp_stat_t)arg;
			write_lock(&sys_status_lock);
			ret = update_comp_stat(pgSysStatus, uSysStat);
			write_unlock(&sys_status_lock);
			break;
		}
	case SYNOIO_GET_MODULE_TYPE:
		{
			copy_to_user((void __user *)arg, module_type_get(), sizeof(module_t));
			break;
		}
	case SYNOIO_GET_BACKPLANE_STATUS:
		{			
			if (synobios_ops->get_backplane_status) {
				ret = synobios_ops->get_backplane_status((BACKPLANE_STATUS *)arg);
			}else{
				ret = -1;
			}
			break;
		}
    case SYNOIO_SET_UART2:
		{
#ifdef MY_ABC_HERE
			extern int syno_ttys_write(const int index, const char* szBuf);
			char *cmd = (char *)arg;
			char szBuf[16];
			
			snprintf(szBuf, sizeof(szBuf), "%s", cmd);
			syno_ttys_write(1, szBuf);
#else
			ret = -1;
#endif
			break;
		}
	case SYNOIO_GET_CPU_TEMPERATURE:
		{
			if (synobios_ops->get_cpu_temperature) {
				ret = synobios_ops->get_cpu_temperature((struct _SynoCpuTemp*)arg);
			} else {
				ret = -1;
			}
			break;
		}
	case SYNOIO_SET_CPU_FAN_STATUS:
		{
			FANSTATUS*  pFanStatus = (FANSTATUS *)arg;

			if (synobios_ops->set_cpu_fan_status) {			
				ret = synobios_ops->set_cpu_fan_status(pFanStatus->status, pFanStatus->speed);
			}else{
				ret = -1;
			}
			
			break;
		}
	default:
		ret=-ENOSYS;		
		//printk(KERN_INFO "synobios_ioctl: un-defined ioctl number %x\n", cmd);
		break;
	}
END:
	return ret;
}

int synobios_open(struct inode *inode, struct file *filp)
{
	return 0;
}

int synobios_release(struct inode *inode, struct file *filp)
{
	return 0;
}

struct file_operations synobios_fops = {
	/*llseek:*/
	/*read:*/
	/*write:*/
	/*readdir:*/
	poll:     synobios_poll,
	ioctl:	  synobios_ioctl,
/*	int (*mmap) (struct file *, struct vm_area_struct *);*/
	open:     synobios_open,
/*	int (*flush) (struct file *);*/
	release:  synobios_release,
/*	int (*fsync) (struct file *, struct dentry *, int datasync);*/
/*	int (*fasync) (int, struct file *, int);*/
/*	int (*lock) (struct file *, int, struct file_lock *);*/
/*	ssize_t (*readv) (struct file *, const struct iovec *, unsigned long, loff_t *);*/
/*	ssize_t (*writev) (struct file *, const struct iovec *, unsigned long, loff_t *);*/
/*	ssize_t (*sendpage) (struct file *, struct page *, int, size_t, loff_t *, int);*/
/*	unsigned long (*get_unmapped_area)(struct file *, unsigned long, unsigned long, unsigned long, unsigned long);*/
};

typedef struct _tag_SYNO_MODEL_MAPPING {
	PRODUCT_MODEL	model;
	char *szModelName;
} SYNO_MODEL_MAPPING;

static SYNO_MODEL_MAPPING gSynoModelMapping[] = {
	{MODEL_CS406e,	    "CS-406e"},
	{MODEL_CS406,	    "CS-406"},
	{MODEL_RS406,	    "RS-406"},
	{MODEL_DS107mv,	    "DS-107+"},
	{MODEL_DS207,	    "DS-207"},
	{MODEL_DS207mv,	    "DS-207+"},
	{MODEL_CS407e,      "CS-407e"},
	{MODEL_CS407,	    "CS-407"},
	{MODEL_RS407,	    "RS-407"},
	{MODEL_DS508,	    "DS-508"},
	{MODEL_RS408,	    "RS-408"},
	{MODEL_DS408,	    "DS-408"},
	{MODEL_RS408rp,     "RS-408rp"},
	{MODEL_DS209p,	    "DS-209+"},
	{MODEL_DS409p,	    "DS-409+"},
	{MODEL_DS509p,	    "DS-509+"},
	{MODEL_RS409p,	    "RS-409+"},
	{MODEL_RS409rpp,    "RS-409rp+"},
	{MODEL_DS109,	    "DS-109"},
	{MODEL_DS109p,	    "DS-109+"},
	{MODEL_DS110p,	    "DS-110+"},
	{MODEL_DS209,	    "DS-209"},
	{MODEL_DS409slim,   "DS-409slim"},
	{MODEL_DS409,       "DS-409"},
	{MODEL_RS409,       "RS-409"},
	{MODEL_DS110j,      "DS-110j"},
	{MODEL_DS210j,      "DS-210j"},
	{MODEL_DS210p,      "DS-210+"},
	{MODEL_DS410j,      "DS-410j"},
	{MODEL_DS410,       "DS-410"},
	{MODEL_DS710p,      "DS-710+"},
	{MODEL_DS1010p,     "DS-1010+"},
	{MODEL_DS110,	    "DS-110"},
	{MODEL_DS410p,	    "DS-410+"},
	{MODEL_RS810p,	    "RS-810+"},
	{MODEL_RS810rpp,    "RS-810rp+"},
	{MODEL_INVALID,	    "Unknown"},
};

static void synobios_print_model(void)
{
	int brand, model;
	SYNO_MODEL_MAPPING *pModelMapping;
	
	if (synobios_ops->get_brand) {
		printk("Brand: ");
		brand = synobios_ops->get_brand();
		switch (brand) {
		case BRAND_SYNOLOGY:
			printk("Synology");
			break;
		case BRAND_LOGITEC:
			printk("Logitec");
			break;
		default:
			printk("Unknown brand");
		}
		printk("\n");
	} else {
		printk("Get brand function not defined.\n");
	}
	if (synobios_ops->get_model) {
		printk("Model: ");        
		model = synobios_ops->get_model();
		pModelMapping = gSynoModelMapping;
		while (pModelMapping->model != MODEL_INVALID) {
			if (pModelMapping->model == model) {
				break;
			}
			pModelMapping++;
		}
		printk("%s\n", pModelMapping->szModelName);
	} else {
		printk("Get model function not defined.\n");
	}

	return;
}

int synobios_init(void)
{
	int result;
	
	scSynoBios.countEvents = 0;
	scSynoBios.idxPtr = 0;

	synobios_model_init(&synobios_fops, &synobios_ops);
	synobios_rtc_init();

	if (synobios_ops->module_type_init) {
		synobios_ops->module_type_init(synobios_ops);
	} else {
		module_type_set(NULL);
	}

	/*initialize wait queues*/
	init_waitqueue_head(&(scSynoBios.wq_poll));

	if (NULL == (pgSysStatus = kzalloc(sizeof(SYNO_SYS_STATUS), GFP_KERNEL))) {
		printk("malloc SYNO_SYS_STATUS fail\n");
		return 0;
	}

	pgSysStatus->fan_fail |= SIGNATURE_FAN_FAIL;
	pgSysStatus->volume_degrade |= SIGNATURE_VOLUME_DEGRADE;
	pgSysStatus->volume_crashed |= SIGNATURE_VOLUME_CRASHED;
	pgSysStatus->power_fail |= SIGNATURE_POWER_FAIL;
	pgSysStatus->ebox_fan_fail |= SIGNATURE_EBOX_FAN_FAIL;

#ifdef MY_ABC_HERE
	funcSYNOSendRaidEvent = synobios_record_raid_event;
#endif
#ifdef MY_ABC_HERE
	funcSYNOGetHwCapability = GetHwCapability;
#endif
#ifdef MY_ABC_HERE
	funcSYNOSendEboxRefreshEvent = synobios_record_ebox_refresh_event;
#endif

	printk(KERN_INFO "synobios: load, major number %d\n", SYNOBIOS_MAJOR);
	result = register_chrdev(SYNOBIOS_MAJOR, "synobios", &synobios_fops);
	if (result < 0) {
		printk(KERN_INFO "synobios: can't set major number\n");
		return result;
	}	
	synobios_print_model();
	return 0;
}

void synobios_cleanup(void)
{
#ifdef MY_ABC_HERE
	funcSYNOSendRaidEvent = NULL;
#endif
#ifdef MY_ABC_HERE
	funcSYNOGetHwCapability = NULL;
#endif
#ifdef MY_ABC_HERE
	funcSYNOSendEboxRefreshEvent = NULL;
#endif

	if (pgSysStatus) {
		kfree(pgSysStatus);
	}
	synobios_model_cleanup(&synobios_fops, &synobios_ops);
	printk("synobios: unload\n");
	unregister_chrdev(SYNOBIOS_MAJOR, "synobios");
}

MODULE_AUTHOR("Alex Wang");
MODULE_DESCRIPTION("synobios\n") ;
MODULE_LICENSE("Synology Inc.");

module_init(synobios_init);
module_exit(synobios_cleanup);
