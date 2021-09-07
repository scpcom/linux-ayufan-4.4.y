 
#ifndef __LINUX_USB_EHCI_DEF_H
#define __LINUX_USB_EHCI_DEF_H

struct ehci_caps {
	 
	u32		hc_capbase;
#define HC_LENGTH(p)		(((p)>>00)&0x00ff)	 
#define HC_VERSION(p)		(((p)>>16)&0xffff)	 
	u32		hcs_params;      
#define HCS_DEBUG_PORT(p)	(((p)>>20)&0xf)	 
#define HCS_INDICATOR(p)	((p)&(1 << 16))	 
#define HCS_N_CC(p)		(((p)>>12)&0xf)	 
#define HCS_N_PCC(p)		(((p)>>8)&0xf)	 
#define HCS_PORTROUTED(p)	((p)&(1 << 7))	 
#define HCS_PPC(p)		((p)&(1 << 4))	 
#define HCS_N_PORTS(p)		(((p)>>0)&0xf)	 

	u32		hcc_params;       
#define HCC_EXT_CAPS(p)		(((p)>>8)&0xff)	 
#define HCC_ISOC_CACHE(p)       ((p)&(1 << 7))   
#define HCC_ISOC_THRES(p)       (((p)>>4)&0x7)   
#define HCC_CANPARK(p)		((p)&(1 << 2))   
#define HCC_PGM_FRAMELISTLEN(p) ((p)&(1 << 1))   
#define HCC_64BIT_ADDR(p)       ((p)&(1))        
	u8		portroute [8];	  
} __attribute__ ((packed));

struct ehci_regs {

	u32		command;
 
#define CMD_PARK	(1<<11)		 
#define CMD_PARK_CNT(c)	(((c)>>8)&3)	 
#define CMD_LRESET	(1<<7)		 
#define CMD_IAAD	(1<<6)		 
#define CMD_ASE		(1<<5)		 
#define CMD_PSE		(1<<4)		 
 
#define CMD_RESET	(1<<1)		 
#define CMD_RUN		(1<<0)		 

	u32		status;
#define STS_ASS		(1<<15)		 
#define STS_PSS		(1<<14)		 
#define STS_RECL	(1<<13)		 
#define STS_HALT	(1<<12)		 
 
#define STS_IAA		(1<<5)		 
#define STS_FATAL	(1<<4)		 
#define STS_FLR		(1<<3)		 
#define STS_PCD		(1<<2)		 
#define STS_ERR		(1<<1)		 
#define STS_INT		(1<<0)		 

	u32		intr_enable;

	u32		frame_index;	 
	 
	u32		segment;	 
	 
	u32		frame_list;	 
	 
	u32		async_next;	 

#ifdef CONFIG_SYNO_PLX_PORTING
	u32 ttctrl;
	u32 burstsize;
	u32 txfilltuning;
	u32 txttfilltuning;
	u32 reserved_1;
	u32 ulpi_viewport;
	u32 reserved_2;
	u32 endpknack;
	u32 endptnalek;
#else
	u32		reserved [9];
#endif

	u32		configured_flag;
#define FLAG_CF		(1<<0)		 

#ifdef CONFIG_SYNO_PLX_PORTING
	u32		port_status [8];	 
#else
	u32		port_status [0];	 
#endif
 
#define PORT_WKOC_E	(1<<22)		 
#define PORT_WKDISC_E	(1<<21)		 
#define PORT_WKCONN_E	(1<<20)		 
 
#define PORT_TEST_PKT	(0x4<<16)	 
#define PORT_LED_OFF	(0<<14)
#define PORT_LED_AMBER	(1<<14)
#define PORT_LED_GREEN	(2<<14)
#define PORT_LED_MASK	(3<<14)
#define PORT_OWNER	(1<<13)		 
#define PORT_POWER	(1<<12)		 
#define PORT_USB11(x) (((x)&(3<<10)) == (1<<10))	 
 
#define PORT_RESET	(1<<8)		 
#define PORT_SUSPEND	(1<<7)		 
#define PORT_RESUME	(1<<6)		 
#define PORT_OCC	(1<<5)		 
#define PORT_OC		(1<<4)		 
#define PORT_PEC	(1<<3)		 
#define PORT_PE		(1<<2)		 
#define PORT_CSC	(1<<1)		 
#define PORT_CONNECT	(1<<0)		 
#define PORT_RWC_BITS   (PORT_CSC | PORT_PEC | PORT_OCC)
#ifdef CONFIG_SYNO_PLX_PORTING
 	u32 otgsc;
 	u32 usbmode;
 	u32 endptsetupstack;
 	u32 endptprime;
 	u32 endptflush;
 	u32 endptstat;
 	u32 endptcomplete;
 	u32 endptctrl[8];
#endif
} __attribute__ ((packed));

#define USBMODE		0x68		 
#define USBMODE_SDIS	(1<<3)		 
#define USBMODE_BE	(1<<2)		 
#define USBMODE_CM_HC	(3<<0)		 
#define USBMODE_CM_IDLE	(0<<0)		 

#define HOSTPC0		0x84		 
#define HOSTPC_PHCD	(1<<22)		 
#define HOSTPC_PSPD	(3<<25)		 
#define USBMODE_EX	0xc8		 
#define USBMODE_EX_VBPS	(1<<5)		 
#define USBMODE_EX_HC	(3<<0)		 
#define TXFILLTUNING	0x24		 
#define TXFIFO_DEFAULT	(8<<16)		 

struct ehci_dbg_port {
	u32	control;
#define DBGP_OWNER	(1<<30)
#define DBGP_ENABLED	(1<<28)
#define DBGP_DONE	(1<<16)
#define DBGP_INUSE	(1<<10)
#define DBGP_ERRCODE(x)	(((x)>>7)&0x07)
#	define DBGP_ERR_BAD	1
#	define DBGP_ERR_SIGNAL	2
#define DBGP_ERROR	(1<<6)
#define DBGP_GO		(1<<5)
#define DBGP_OUT	(1<<4)
#define DBGP_LEN(x)	(((x)>>0)&0x0f)
	u32	pids;
#define DBGP_PID_GET(x)		(((x)>>16)&0xff)
#define DBGP_PID_SET(data, tok)	(((data)<<8)|(tok))
	u32	data03;
	u32	data47;
	u32	address;
#define DBGP_EPADDR(dev, ep)	(((dev)<<8)|(ep))
} __attribute__ ((packed));

#ifdef CONFIG_EARLY_PRINTK_DBGP
#include <linux/init.h>
extern int __init early_dbgp_init(char *s);
extern struct console early_dbgp_console;
#endif  

#ifdef CONFIG_EARLY_PRINTK_DBGP
 
extern int dbgp_external_startup(void);
extern int dbgp_reset_prep(void);
#else
static inline int dbgp_reset_prep(void)
{
	return 1;
}
static inline int dbgp_external_startup(void)
{
	return -1;
}
#endif

#endif  
