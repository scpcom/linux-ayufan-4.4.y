#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
// Copyright (c) 2003-2015 Synology Inc. All rights reserved.
#ifndef __SYNO_SATA_H_
#define __SYNO_SATA_H_

#include <linux/kernel.h>
#include <linux/synobios.h>
#include <uapi/linux/synosata.h>
#ifdef MY_ABC_HERE
#include <linux/gpio.h>
#endif /* MY_ABC_HERE */

/*
 * We use g_syno_hdd_powerup_seq this variable pass from uboot for determine whether wake up in sequence.
 * because if we need power in sequence at booting,
 * it's mean we also need wake up in sequence for power issue
 *
 * For old product, they don't passing g_syno_hdd_powerup_seq from u-boot, but in new kernel it had defined.
 * so the default value is -1, it will still doing original job. So this define can compatible to old platform.
 *
 * I put the g_syno_hdd_powerup_seq check in the sata driver instaed of this. Because we only need to check in
 * queuecommand. Others is just callbacks. We don't need it really.
 *
 * -1 : no specify. Always do spinup delay
 *  0 : do not spinup delay
 * >0 : The number that we would delay
 */
#ifdef MY_ABC_HERE
#ifdef MY_DEF_HERE
extern int gSynoHddPowerupSeq, gSynoInternalHddNumber;
#else /* MY_DEF_HERE */
extern long g_syno_hdd_powerup_seq;
#endif /* MY_DEF_HERE */
extern long syno_boot_hd_count;
#endif /* MY_ABC_HERE */
#ifdef MY_DEF_HERE
extern int giSynoSpinupGroup[16];
extern int giSynoSpinupGroupNum;
extern int giSynoSpinupGroupDelay;
extern int giSynoSpinupGroupDebug;
extern int giSynoDSleepCurrentSpinupGroupNum;
extern int giSynoDSleepCurrentSpinupGroupDiskNum;
extern int giSynoDSleepCurrentPoweronDisks;
extern void DBG_SpinupGroupListGpio(void);
extern int SynoHaveRPDetectPin(void);
extern int SynoAllRedundantPowerDetected(void);
#endif /* MY_DEF_HERE */

#ifdef MY_ABC_HERE
static inline void SleepForLatency(void)
{
	mdelay(3000);
}
#endif /* MY_ABC_HERE */

#ifdef MY_ABC_HERE
static inline void SleepForHD(int i)
{
#ifdef MY_DEF_HERE
	if ((syno_boot_hd_count != gSynoInternalHddNumber - 1) && /* the last disk shouldn't wait */
		(gSynoHddPowerupSeq && (syno_boot_hd_count < gSynoInternalHddNumber))) {
#else /* MY_DEF_HERE */
	if ((syno_boot_hd_count != g_syno_hdd_powerup_seq - 1) && /* the last disk shouldn't wait */
		(( g_syno_hdd_powerup_seq < 0 ) || /* not specified in boot command line */
		  syno_boot_hd_count < g_syno_hdd_powerup_seq) ) {
#endif /* MY_DEF_HERE */
		printk("Delay 10 seconds to wait for disk %d ready.\n", i);
		mdelay(10000);
	}
	syno_boot_hd_count++;
}

/*
 * delay for HW ready, if this port already wait for latency,
 * we delay 5s, otherwise we dleay 7s. And the first, last
 * disks, we shouldn't delay them.
 *
 * @param iDisk [IN] disk number
 *        iIsDoLatency [IN] is do latency before
 *
 **/
static inline void SleepForHW(int iDisk, int iIsDoLatency)
{
#ifdef MY_DEF_HERE
	static int iDisksInSpinupGroup = 0;
	static int iCurrentSpinupGroup = 0;
	if(0 < giSynoSpinupGroupNum) {
		DBG_SpinupGroupListGpio();
		if (iDisksInSpinupGroup < giSynoSpinupGroup[iCurrentSpinupGroup]) {
			goto skip_wait_or_wait_done;
		}
		if (SynoHaveRPDetectPin() && SynoAllRedundantPowerDetected()) {
			goto skip_wait_or_wait_done;
		}
	}
#endif /* MY_ABC_HERE */
	/* the first shouldn't wait */
#ifdef MY_DEF_HERE
	if (syno_boot_hd_count &&
		(gSynoHddPowerupSeq && (syno_boot_hd_count < gSynoInternalHddNumber))) {
#else /* MY_DEF_HERE */
	if (syno_boot_hd_count &&
		(( g_syno_hdd_powerup_seq < 0 ) || /* not specified in boot command line */
		  syno_boot_hd_count < g_syno_hdd_powerup_seq) ) {
#endif /* MY_DEF_HERE */
#ifdef MY_DEF_HERE
		if (0 < giSynoSpinupGroupDelay) {
			printk("Delay %d seconds to wait for disk %d ready.\n", giSynoSpinupGroupDelay, iDisk);
			mdelay(giSynoSpinupGroupDelay * 1000);
			goto skip_wait_or_wait_done;
		}
#endif /* MY_DEF_HERE */
		if (iIsDoLatency) {
			printk("Delay 5 seconds to wait for disk %d ready.\n", iDisk);
			mdelay(5000);
		} else {
			printk("Delay 7 seconds to wait for disk %d ready.\n", iDisk);
			mdelay(7000);
		}
	}
#ifdef MY_DEF_HERE
skip_wait_or_wait_done:
	if (0 < giSynoSpinupGroupNum) {
		if (iDisksInSpinupGroup >= giSynoSpinupGroup[iCurrentSpinupGroup]) {
			iCurrentSpinupGroup++;
			iDisksInSpinupGroup = 0;
		}
		iDisksInSpinupGroup++;
	}
#endif /* MY_ABC_HERE */
	syno_boot_hd_count++;
}
#endif /* MY_ABC_HERE */

#ifdef MY_ABC_HERE
#include <linux/fs.h>

#define GPIO_3XXX_CMD_POWER_CTL 0x40
#define GPIO_3XXX_CMD_POWER_CLR 0x00

#define GPI_3XXX_HDD_PWR_OFF(x)		(0x10&x)

#define GPIO_3826_CMD_ENABLE_POWERBTN	(0 << 15)

#define GPIO_9705_PKG_INIT(addr,data)	((addr << 10) | (0x3 << 8) | data)
/**
 * Kernel gpio package of our ebox.
 */
typedef struct _tag_SYNO_PM_PKG {
	/*	use for read/write */
	unsigned int	var;

	/* the gpio address */
	int	gpio_addr;

	/* the encode of gpio */
	void (*encode)(struct _tag_SYNO_PM_PKG *pm_pkg, int rw);

	/* the decode of gpio */
	void (*decode)(struct _tag_SYNO_PM_PKG *pm_pkg, int rw);
} SYNO_PM_PKG;

/**
 * You should reference ebox spec for
 * the gpio definition of our 3xxx.
 *
 * Otherwise, you don't know what we do here
 *
 * @param pPM_pkg [OUT] Store the result. Should not be NULL.
 * @param rw      [IN] indicate the request is read or write.
 *                0: read
 *                1: write
 */
static inline void
SIMG3xxx_gpio_decode(SYNO_PM_PKG *pPM_pkg, int rw)
{
#define GPI_3XXX_BIT1(GPIO)	(1&GPIO)
#define GPI_3XXX_BIT2(GPIO)	((1<<1)&GPIO)
#define GPI_3XXX_BIT3(GPIO)	((1<<13)&GPIO)>>11
#define GPI_3XXX_BIT4(GPIO)	((1<<26)&GPIO)>>23
#define GPI_3XXX_BIT5(GPIO)	((1<<28)&GPIO)>>24
#define GPI_3XXX_BIT6(GPIO)	((1<<29)&GPIO)>>24
#define GPI_3XXX_BIT7(GPIO)	((1<<31)&GPIO)>>25

	if (!rw) {
		pPM_pkg->var =
			GPI_3XXX_BIT1(pPM_pkg->var)|
			GPI_3XXX_BIT2(pPM_pkg->var)|
			GPI_3XXX_BIT3(pPM_pkg->var)|
			GPI_3XXX_BIT4(pPM_pkg->var)|
			GPI_3XXX_BIT5(pPM_pkg->var)|
			GPI_3XXX_BIT6(pPM_pkg->var)|
			GPI_3XXX_BIT7(pPM_pkg->var);
	}
}
/* 3xxx GPIO table */
//	GPIO31	GPIO30	GPIO29	GPIO28	GPIO27	GPIO26	GPIO25	GPIO24
//R	GPI 7	--		GPI 6	GPI 5	--		GPI 4	--		--
//W	GPO16	GPO15	--		--		--		--		--		--
//
//	GPIO23	GPIO22	GPIO21	GPIO20	GPIO19	GPIO18	GPIO17	GPIO16
//R	--		--		--		--		--		--		--		--
//W	--		--		GPO14	GPO13	GPO12	GPO11	GPO10	GPO9
//
//	GPIO15	GPIO14	GPIO13	GPIO12	GPIO11	GPIO10	GPIO09	GPIO08
//R	--		--		GPI 3	EMID2	EMID1	EMID0	1		0
//W	GPO8	GPO7	GPO6	GPO5	GPO4	GPO3	--		--
//
//	GPIO07	GPIO06	GPIO05	GPIO04	GPIO03	GPIO02	GPIO01	GPIO00
//R	0		0		0		0		0		0		GPI 2	GPI 1
//W	--		--		--		--		--		--		GPO2	GPO1

/**
 * You should reference ebox spec for
 * the gpio definition of our 3xxx.
 *
 * Otherwise, you don't know what we do here
 *
 * @param pPM_pkg [OUT] Store the result. Should not be NULL.
 * @param rw      [IN] indicate the request is read or write.
 *                0: read
 *                1: write
 */
static inline void
SIMG3xxx_gpio_encode(SYNO_PM_PKG *pPM_pkg, int rw)
{
#define GPIO_3XXX_BIT00(GPO)	(1&GPO)
#define GPIO_3XXX_BIT01(GPO)	((1<<1)&GPO)
#define GPIO_3XXX_BIT10(GPO)	((1<<2)&GPO)<<8
#define GPIO_3XXX_BIT11(GPO)	((1<<3)&GPO)<<8
#define GPIO_3XXX_BIT12(GPO)	((1<<4)&GPO)<<8
#define GPIO_3XXX_BIT13(GPO)	((1<<5)&GPO)<<8
#define GPIO_3XXX_BIT14(GPO)	((1<<6)&GPO)<<8
#define GPIO_3XXX_BIT15(GPO)	((1<<7)&GPO)<<8
#define GPIO_3XXX_BIT16(GPO)	((1<<8)&GPO)<<8
#define GPIO_3XXX_BIT17(GPO)	((1<<9)&GPO)<<8
#define GPIO_3XXX_BIT18(GPO)	((1<<10)&GPO)<<8
#define GPIO_3XXX_BIT19(GPO)	((1<<11)&GPO)<<8
#define GPIO_3XXX_BIT20(GPO)	((1<<12)&GPO)<<8
#define GPIO_3XXX_BIT21(GPO)	((1<<13)&GPO)<<8
#define GPIO_3XXX_BIT30(GPO)	((1<<14)&GPO)<<16
#define GPIO_3XXX_BIT31(GPO)	((1<<15)&GPO)<<16

	if (rw) {
		pPM_pkg->var =
			GPIO_3XXX_BIT00(pPM_pkg->var)|
			GPIO_3XXX_BIT01(pPM_pkg->var)|
			GPIO_3XXX_BIT10(pPM_pkg->var)|
			GPIO_3XXX_BIT11(pPM_pkg->var)|
			GPIO_3XXX_BIT12(pPM_pkg->var)|
			GPIO_3XXX_BIT13(pPM_pkg->var)|
			GPIO_3XXX_BIT14(pPM_pkg->var)|
			GPIO_3XXX_BIT15(pPM_pkg->var)|
			GPIO_3XXX_BIT16(pPM_pkg->var)|
			GPIO_3XXX_BIT17(pPM_pkg->var)|
			GPIO_3XXX_BIT18(pPM_pkg->var)|
			GPIO_3XXX_BIT19(pPM_pkg->var)|
			GPIO_3XXX_BIT20(pPM_pkg->var)|
			GPIO_3XXX_BIT21(pPM_pkg->var)|
			GPIO_3XXX_BIT30(pPM_pkg->var)|
			GPIO_3XXX_BIT31(pPM_pkg->var);
	}
}

/* 9705 GPIO table */
/*
 *   NA      NA      NA      NA      GPIO19  GPIO18  GPIO17  GPIO16
 * R --      --      --      --      --      --      --      --
 * W --      --      --      --      A2	     A1      A0      Mask
 *
 *   GPIO15  GPIO14  GPIO13  GPIO12  GPIO11  GPIO10  GPIO09  GPIO08
 * R --      --      GPI8    GPI7    GPI6    LED_5   LED_4   LED_3
 * W R_CTL   W_CTL   GPO8    GPO7    GPO6    LED_5   LED_4   LED_3
 *
 *   GPIO07  GPIO06  GPIO05  GPIO04  GPIO03  GPIO02  GPIO01  GPIO00
 * R LED_2   LED_1   LED_H   GPI5    GPI4    GPI3    GPI2    GPI1
 * W LED_2   LED_1   LED_H   GPO5    GPO4    GPO3    GPO2    GPO1
 *
 */

/**
 * You should reference ebox spec for
 * the gpio definition of our 9705.
 *
 * Otherwise, you don't know what we do here
 *
 * @param pPM_pkg [OUT] Store the result. Should not be NULL.
 * @param rw      [IN] indicate the request is read or write.
 *                0: read
 *                1: write
 */
static inline void
SIMG9705_gpio_decode(SYNO_PM_PKG *pPM_pkg, int rw)
{
#define GPI_9705_BIT1(GPIO)	(1&GPIO)
#define GPI_9705_BIT2(GPIO)	((1<<1)&GPIO)
#define GPI_9705_BIT3(GPIO)	((1<<2)&GPIO)
#define GPI_9705_BIT4(GPIO)	((1<<3)&GPIO)
#define GPI_9705_BIT5(GPIO)	((1<<4)&GPIO)
#define GPI_9705_BIT6(GPIO)	((1<<11)&GPIO)>>6
#define GPI_9705_BIT7(GPIO)	((1<<12)&GPIO)>>6
#define GPI_9705_BIT8(GPIO)	((1<<13)&GPIO)>>6

	if (!rw) {
		pPM_pkg->var =
			GPI_9705_BIT1(pPM_pkg->var)|
			GPI_9705_BIT2(pPM_pkg->var)|
			GPI_9705_BIT3(pPM_pkg->var)|
			GPI_9705_BIT4(pPM_pkg->var)|
			GPI_9705_BIT5(pPM_pkg->var)|
			GPI_9705_BIT6(pPM_pkg->var)|
			GPI_9705_BIT7(pPM_pkg->var)|
			GPI_9705_BIT8(pPM_pkg->var);
	}
}

/**
 * You should reference ebox spec for
 * the gpio definition of our 9705.
 *
 * Otherwise, you don't know what we do here
 *
 * @param pPM_pkg [OUT] Store the result. Should not be NULL.
 * @param rw      [IN] indicate the request is read or write.
 *                0: read
 *                1: write
 */
static inline void
SIMG9705_gpio_encode(SYNO_PM_PKG *pPM_pkg, int rw)
{
#define GPIO_9705_BIT00(GPO)	(1&GPO)
#define GPIO_9705_BIT01(GPO)	((1<<1)&GPO)
#define GPIO_9705_BIT02(GPO)	((1<<2)&GPO)
#define GPIO_9705_BIT03(GPO)	((1<<3)&GPO)
#define GPIO_9705_BIT04(GPO)	((1<<4)&GPO)
#define GPIO_9705_BIT11(GPO)	((1<<5)&GPO)<<6
#define GPIO_9705_BIT12(GPO)	((1<<6)&GPO)<<6
#define GPIO_9705_BIT13(GPO)	((1<<7)&GPO)<<6
#define GPIO_9705_BIT14(GPO)	((1<<8)&GPO)<<6
#define GPIO_9705_BIT15(GPO)	((1<<9)&GPO)<<6
#define GPIO_9705_BIT17(GPO)	((1<<10)&GPO)<<7
#define GPIO_9705_BIT18(GPO)	((1<<11)&GPO)<<7
#define GPIO_9705_BIT19(GPO)	((1<<12)&GPO)<<7

	if (rw) {
		pPM_pkg->var =
			GPIO_9705_BIT00(pPM_pkg->var)|
			GPIO_9705_BIT01(pPM_pkg->var)|
			GPIO_9705_BIT02(pPM_pkg->var)|
			GPIO_9705_BIT03(pPM_pkg->var)|
			GPIO_9705_BIT04(pPM_pkg->var)|
			GPIO_9705_BIT11(pPM_pkg->var)|
			GPIO_9705_BIT12(pPM_pkg->var)|
			GPIO_9705_BIT13(pPM_pkg->var)|
			GPIO_9705_BIT14(pPM_pkg->var)|
			GPIO_9705_BIT15(pPM_pkg->var)|
			GPIO_9705_BIT17(pPM_pkg->var)|
			GPIO_9705_BIT18(pPM_pkg->var)|
			GPIO_9705_BIT19(pPM_pkg->var);
	}
}

static inline unsigned char
syno_pm_is_3726(unsigned short vendor, unsigned short devid)
{
	return (vendor == 0x1095 && devid == 0x3726);
}

static inline unsigned char
syno_pm_is_3826(unsigned short vendor, unsigned short devid)
{
	return (vendor == 0x1095 && devid == 0x3826);
}

static inline unsigned char
syno_pm_is_9705(unsigned short vendor, unsigned short devid)
{
	return (vendor == 0x1B4B  && devid == 0x9705);
}

static inline unsigned char
syno_pm_is_3xxx(unsigned short vendor, unsigned short devid)
{
	return (syno_pm_is_3726(vendor, devid) || syno_pm_is_3826(vendor, devid));
}

static inline void
syno_pm_systemstate_pkg_init(unsigned short vendor, unsigned short devid, SYNO_PM_PKG *pPKG)
{
	/* do not check parameters, caller should do it */

	memset(pPKG, 0, sizeof(*pPKG));
	if (syno_pm_is_3xxx(vendor, devid)) {
		pPKG->var = 0x200;
	} else if (syno_pm_is_9705(vendor, devid)) {
		pPKG->var = GPIO_9705_PKG_INIT(3,0);
	}

	/* add other port multiplier here */
}

static inline void
syno_pm_unique_pkg_init(unsigned short vendor, unsigned short devid, SYNO_PM_PKG *pPKG)
{
	/* do not check parameters, caller should do it */

	memset(pPKG, 0, sizeof(*pPKG));
	if (syno_pm_is_3xxx(vendor, devid)) {
		pPKG->var = 0x100;
	} else if (syno_pm_is_9705(vendor, devid)) {
		pPKG->var = GPIO_9705_PKG_INIT(0,0);
	}

	/* add other port multiplier here */
}

static inline void
syno_pm_raidledstate_pkg_init(unsigned short vendor, unsigned short devid, SYNO_PM_PKG *pPKG)
{
	/* do not check parameters, caller should do it */

	memset(pPKG, 0, sizeof(*pPKG));
	if (syno_pm_is_3xxx(vendor, devid)) {
		pPKG->var = 0x280;
	} else if (syno_pm_is_9705(vendor, devid)) {
		pPKG->var = GPIO_9705_PKG_INIT(4,0);
	}
	/* add other port multiplier here */
}

static inline void
syno_pm_fanstatus_pkg_init(unsigned short vendor, unsigned short devid, SYNO_PM_PKG *pPKG)
{
	/* do not check parameters, caller should do it */

	memset(pPKG, 0, sizeof(*pPKG));
	if (syno_pm_is_3xxx(vendor, devid)) {
		pPKG->var = 0x80;
	} else if (syno_pm_is_9705(vendor, devid)) {
		pPKG->var = GPIO_9705_PKG_INIT(2,0);
	}

	/* add other port multiplier here */
}

static inline void
syno_pm_poweron_pkg_init(unsigned short vendor, unsigned short devid, SYNO_PM_PKG *pPKG, unsigned char blCLR)
{
	/* do not check parameters, caller should do it */

	memset(pPKG, 0, sizeof(*pPKG));
	if (syno_pm_is_3xxx(vendor, devid)) {
		if (blCLR) {
			pPKG->var = GPIO_3XXX_CMD_POWER_CLR;
		} else {
			pPKG->var = GPIO_3XXX_CMD_POWER_CTL;
		}
	} else if (syno_pm_is_9705(vendor, devid)) {
		if (blCLR) {
			pPKG->var = GPIO_9705_PKG_INIT(4,0b10);
		} else {
			pPKG->var = GPIO_9705_PKG_INIT(4,0b10010);
		}
	}

	/* add other port multiplier here */
}

#ifdef MY_ABC_HERE
/**
 * Init eunit deepsleep indicator
 *
 * @param vendor  [IN] PMP vendor
 * @param devid   [IN] device id
 * @param pPM_pkg [IN] Store the result. Should not be NULL.
 * @param blCLR   [IN] clean or not
 *
 * return 0: not support deepsleep indicator
 *        1: support deepsleep indicator
 */
static inline int
syno_pm_deepsleep_indicator_pkg_init(unsigned short vendor, unsigned short devid, SYNO_PM_PKG *pPKG, unsigned char blCLR)
{
	/* do not check parameters, caller should do it */
	int iRet = 0;

	memset(pPKG, 0, sizeof(*pPKG));
	if (syno_pm_is_9705(vendor, devid)) {
		if (blCLR) {
			pPKG->var = GPIO_9705_PKG_INIT(1,0);
		} else {
			pPKG->var = GPIO_9705_PKG_INIT(1,0x80);
		}
		iRet = 1;
	}
	/* add other port multiplier here */
	return iRet;
}
#endif /* MY_ABC_HERE */

static inline void
syno_pm_enable_powerbtn_pkg_init(unsigned short vendor, unsigned short devid, SYNO_PM_PKG *pPKG)
{
	/* do not check parameters, caller should do it */

	memset(pPKG, 0, sizeof(*pPKG));
	/* DX513 and DX213 use silicon 3826 chip, but its cpld faked 3726 chip */
	if (syno_pm_is_3xxx(vendor, devid)) {
		pPKG->var = GPIO_3826_CMD_ENABLE_POWERBTN;
	} else if (syno_pm_is_9705(vendor, devid)) {
		pPKG->var = GPIO_9705_PKG_INIT(4,0x20);
	}

	/* add other port multiplier here */
}

static inline unsigned int
syno_support_disk_num(unsigned short vendor,
					  unsigned short devid,
					  unsigned int syno_uniq)
{
	unsigned int ret = 0;

	if (syno_pm_is_3xxx(vendor, devid)) {
		if (IS_SYNOLOGY_RX4(syno_uniq) || IS_SYNOLOGY_RX415(syno_uniq)) {
			ret = 4;
		} else if (IS_SYNOLOGY_DX5(syno_uniq) || IS_SYNOLOGY_DX513(syno_uniq)) {
			ret = 5;
		} else if (IS_SYNOLOGY_DXC(syno_uniq) || IS_SYNOLOGY_RXC(syno_uniq)) {
			ret = 3;
		} else if (IS_SYNOLOGY_DX213(syno_uniq)) {
			ret = 2;
		} else {
			printk("%s not RX4 or DX5", __FUNCTION__);
			ret = 5;
		}
		goto END;
	} else if (syno_pm_is_9705(vendor, devid)) {
		if (IS_SYNOLOGY_RX413(syno_uniq) || IS_SYNOLOGY_RX418(syno_uniq)) {
			ret = 4;
		} else if (IS_SYNOLOGY_RX1214(syno_uniq) || IS_SYNOLOGY_RX1217(syno_uniq) || IS_SYNOLOGY_DX1215(syno_uniq)) {
			ret = 3;
		} else if (IS_SYNOLOGY_DX517(syno_uniq)) {
			ret = 5;
		} else {
			printk("%s not synology device", __FUNCTION__);
			ret = 5;
		}
	}

	/* add other chip here */
END:
	return ret;
}

static inline void
syno_pm_hddled_status_pkg_init(unsigned short vendor, unsigned short devid, SYNO_PM_PKG *pPKG)
{
	/* do not check parameters, caller should do it */

	memset(pPKG, 0, sizeof(*pPKG));

	if (syno_pm_is_3xxx(vendor, devid)) {
		pPKG->var = 0x180;
	} else if (syno_pm_is_9705(vendor, devid)) {
		pPKG->var = GPIO_9705_PKG_INIT(1,0);
	}

	/* add other port multiplier here */
}

#ifdef MY_ABC_HERE
extern EUNIT_PWRON_TYPE (*funcSynoEunitPowerctlType)(void);
#endif /* MY_ABC_HERE */
extern char gszSynoHWVersion[16];
static inline unsigned char
is_ebox_support(void)
{
	unsigned char ret = 0;

#ifdef MY_ABC_HERE
	if (funcSynoEunitPowerctlType) {
		if (EUNIT_NOT_SUPPORT == funcSynoEunitPowerctlType()) {
			goto END;
		}
	}
#endif /* MY_ABC_HERE */
	/* FIXME: is there a better way to do this ?
	 *        No synobios is loaded(boot time or some unexpect situation). use a plain list.
	 *        If you want to deny the support of some models at boot time.
	 *        Please put the comparision logic here.
	 */

	ret = 1;
#ifdef MY_ABC_HERE
END:
#endif /* MY_ABC_HERE */
	return ret;
}
#endif /* MY_ABC_HERE */

#ifdef MY_ABC_HERE

/*
 *back porting from linux 2.6.28. add SYNO prefix in order to not mixed with libata
 */
#define SYNO_ATA_ID_MAJOR_VER	 80
#define SYNO_ATA_ID_MINOR_VER	 81
#define SYNO_ATA_ID_COMMAND_SET_1 82
#define SYNO_ATA_ID_COMMAND_SET_2 83
#define SYNO_ATA_ID_CFSSE		 84
#define SYNO_ATA_ID_ROT_SPEED	 217

/**
 * Determind the ata version.
 *
 * Copy from ata.h
 *
 * @param id     [IN] Should not be NULL. ata identify buffer.
 *
 * @return ata version
 */
static inline unsigned int
ata_major_version(const unsigned short *id)
{
	unsigned int mver;

	if (id[SYNO_ATA_ID_MAJOR_VER] == 0xFFFF)
		return 0;

	for (mver = 14; mver >= 1; mver--)
		if (id[SYNO_ATA_ID_MAJOR_VER] & (1 << mver))
			break;
	return mver;
}

/**
 * Determind the ata version.
 *
 * Copy from linux-2.6.28 later in ata.h. Original from mail
 * list. But it has bug. So i customized it.
 *
 * Sometime you can't just only take care in major version.
 * The actually ATA version might need to look minor version.
 * Please refer smartmontools-5.38/atacmds.cpp
 * const char minor_str []  = ...
 *
 * @param id     [IN] Should not be NULL. ata identify buffer.
 *
 * @return ata version
 */
static inline int
syno_ata_id_is_ssd(const unsigned short *id)
{
	int res = 0;
	unsigned int major_id = ata_major_version(id);

	/* ATA8-ACS version 4c or higher (=> 4c or 6 at the moment) */
	if (7 <= major_id){
		if (id[SYNO_ATA_ID_ROT_SPEED] == 0x01) {
			// intel ssd, and the laters ssd
			res = 1;
			goto END;
		}
	}

	if ((id[SYNO_ATA_ID_COMMAND_SET_2]>>14) == 0x01 &&
		!(id[SYNO_ATA_ID_COMMAND_SET_1] & 0x0001)) {
		// not support smart. like innodisk
		res = 1;
		goto END;
	}

	// transcend. Not support smart error log
	if ((id[SYNO_ATA_ID_COMMAND_SET_2]>>14) == 0x01 &&
		(id[SYNO_ATA_ID_COMMAND_SET_1] & 0x0001) &&
		!(id[SYNO_ATA_ID_CFSSE] & 0x1)) {
		res = 1;
		goto END;
	}

END:
	return res;
}
#endif /* MY_ABC_HERE */

#ifdef MY_ABC_HERE
#define SZK_PMP_UEVENT "SYNO_PMP_EVENT"
#define SZV_PMP_CONNECT "CABLE_CONNECT"
#define SZV_PMP_DISCONNECT "CABLE_DISCONNECT"
#endif /* MY_ABC_HERE */

#ifdef MY_DEF_HERE
#define DBG_SpinupGroup(x...)	\
	if (0 < giSynoSpinupGroupDebug) printk(x)

static inline void SynoResetDSleepGroup(void)
{
	if (0 < giSynoSpinupGroupNum) {
		//Reset to group 0
		giSynoDSleepCurrentSpinupGroupNum = 0;
		//Reset disk num of group 0, ex [2,1,1,1], here is 2
		giSynoDSleepCurrentSpinupGroupDiskNum = giSynoSpinupGroup[0];
		giSynoDSleepCurrentPoweronDisks = 0;
	}
}
static inline int SynoDSleepNeedUpdateLastPmOn(void)
{
	int ret = 0;
	if (0 == giSynoSpinupGroupNum) {
		ret = 1;
	} else {
		/* no RP or with RP pin but not plug both them */
		if (!SynoHaveRPDetectPin() ||
			(SynoHaveRPDetectPin() && !SynoAllRedundantPowerDetected())) {
			giSynoDSleepCurrentPoweronDisks++;
		}

		/* with RP pin and plug both them  */
		if (SynoHaveRPDetectPin() && SynoAllRedundantPowerDetected()) {
			/* don't add giSynoDSleepCurrentPoweronDisks means always not full */
			ret = 1;
		}

		if (giSynoDSleepCurrentPoweronDisks >= giSynoDSleepCurrentSpinupGroupDiskNum) {
			DBG_SpinupGroup("Disk Group %d is full, going to delay for power on.\n",giSynoDSleepCurrentSpinupGroupNum);
			DBG_SpinupGroupListGpio();
			giSynoDSleepCurrentPoweronDisks = 0;
			giSynoDSleepCurrentSpinupGroupNum++;
			if (giSynoDSleepCurrentSpinupGroupNum < giSynoSpinupGroupNum) {
				giSynoDSleepCurrentSpinupGroupDiskNum = giSynoSpinupGroup[giSynoDSleepCurrentSpinupGroupNum];
			} else {
				/* if syno_spinup_group not use all disks, left hdd poweron 1 by 1 */
				giSynoDSleepCurrentSpinupGroupDiskNum = 1;
			}
			ret = 1;
		}
	}
	return ret;
}
static inline unsigned long SynoWakeInterval(void)
{
	/* original WAKEINTERVAL = 7UL*HZ */
	static unsigned long uiSynoWakeInterval = 7UL*HZ;
	if (0 < giSynoSpinupGroupDelay) {
		uiSynoWakeInterval = (unsigned long)giSynoSpinupGroupDelay * HZ;
	}
	return uiSynoWakeInterval;
}
#endif /* MY_ABC_HERE */
#endif /* __SYNO_SATA_H_ */
