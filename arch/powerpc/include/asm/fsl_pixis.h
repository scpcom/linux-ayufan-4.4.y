#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifdef MY_DEF_HERE
 
#ifndef __PPC_FSL_PIXIS_H
#define __PPC_FSL_PIXIS_H

#include <linux/types.h>

#define PX_CMD_START	0x1
#define PX_CMD_STOP		0x0
#define PX_CMD_SLEEP	0x2

#define VOLT_FMT		0x00
#define CURR_FMT		0x01
#define TEMP_FMT		0x02

#define PXOC_MSG		(0x01 << 0)
#define PXMA_ERR		(0x01 << 1)
#define PXMA_ACK		(0x01 << 0)

#define DATA_ADDR		0x80	 
#define DIVIDE_FACTOR   1000     
#define REC_NUM			9		 

#define OM_END			0x00
#define OM_SETDLY		0x01
#define OM_RST0			0x02
#define OM_RST1			0x03
#define OM_CHKDLY		0x04
#define OM_PWR			0x05
#define OM_WAKE			0x07
#define OM_GETMEM		0x08
#define OM_SETMEM		0x09
#define OM_SCLR			0x10
#define OM_START		0x11
#define OM_STOP			0x12
#define OM_GET			0x13
#define OM_ENABLE		0x14
#define OM_TIMER		0x15
#define OM_SETV			0x30
#define OM_INFO			0x31

struct pixis_reg {
	u8	id;			 
	u8	arch;		 
	u8	scver;		 
	u8	ctl;		 
	u8	rst;		 
	u8	stat;		 
	u8	aux;		 
	u8	spd;		 
	u8	cfg0;		 
	u8	cfg1;		 
	u8	addr;		 
	u8	res1[2];	 
	u8	data;		 
	u8	led;		 
	u8	tagd;		 
	u8	vctl;		 
	u8	vstat;		 
	u8	vcfgen0;	 
	u8	rrsn;		 
	u8	ocmd;		 
	u8	omsg;		 
	u8	gmdbg;		 
	u8	gmdd;		 
	u8	mack;		 
	u8	sclk0;		 
	u8	sclk1;		 
	u8	sclk2;		 
	u8	dclk0;		 
	u8	dclk1;		 
	u8	dclk2;		 
	u8	watch;		 
	u8	sw1;		 
	u8	en1;		 
	u8	sw2;
	u8	en2;
	u8	sw3;
	u8	en4;
	u8	sw5;
	u8	en5;
	u8	sw6;
	u8	en6;
	u8	sw7;
	u8	en7;
	u8	sw8;
	u8	en8;
} __attribute__ ((packed));

struct crecord {
	u16	curr;		 
	u16	max;		 
	u16	qty1;		 
	u8	qty2;
	u32	acc;		 
} __attribute__((packed));

struct fsl_pixis {
	struct pixis_reg __iomem *base;
	struct crecord rec[REC_NUM];
	u32	pm_cmd;
	char mode[10];
};

#ifdef CONFIG_FSL_PIXIS
int pixis_start_pm_sleep(void);
int pixis_stop_pm_sleep(void);
int pmbus_2volt(int);
int pmbus_2cur(int);
#else
static inline int pixis_start_pm_sleep(void) { return 0; }
static inline int pixis_stop_pm_sleep(void) { return 0; }
static inline int pmbus_2volt(int a) { return 0; }
static inline int pmbus_2cur(int a) { return 0; }
#endif

#endif
#endif  
