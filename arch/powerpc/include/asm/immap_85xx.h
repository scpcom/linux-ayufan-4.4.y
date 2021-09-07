#ifdef CONFIG_SYNO_QORIQ
 
#ifndef __ASM_POWERPC_IMMAP_85XX_H__
#define __ASM_POWERPC_IMMAP_85XX_H__
#ifdef __KERNEL__

struct ccsr_guts {
	__be32	porpllsr;	 
	__be32	porbmsr;	 
	u8	res1[0xc - 0x8];
	__be32	pordevsr;	 
	__be32	pordbgmsr;	 
	__be32  pordevsr2;	 
	u8	res2[0x20 - 0x18];
	__be32	gpporcr;	 
	u8	res3[0x60 - 0x24];
	__be32	pmuxcr;		 
#define CCSR_GUTS_PMUXCR_SDT_S		0xa
#define CCSR_GUTS_PMUXCR_SDT_M		0x3
#define CCSR_GUTS_PMUXCR_SDT_SSI	0x0
#define CCSR_GUTS_PMUXCR_SDT_DMA	0x1
#define CCSR_GUTS_PMUXCR_SDT_TDM	0x2
#define CCSR_GUTS_PMUXCR_SDT_GPIO	0x3
	__be32  pmuxcr2;	 
	__be32  dmuxcr;		 
#define CCSR_GUTS_DMUXCR_D1C0_S	0x1e
#define CCSR_GUTS_DMUXCR_D1C1_S	0x1c
#define CCSR_GUTS_DMUXCR_D1C2_S	0x1a
#define CCSR_GUTS_DMUXCR_D1C3_S	0x18
#define CCSR_GUTS_DMUXCR_D2C0_S	0x16
#define CCSR_GUTS_DMUXCR_D2C1_S	0x14
#define CCSR_GUTS_DMUXCR_D2C2_S	0x12
#define CCSR_GUTS_DMUXCR_D2C3_S	0x10
#define CCSR_GUTS_DMUXCR_DC_M	0x3
#define CCSR_GUTS_DMUXCR_DC_NC0	0x0
#define CCSR_GUTS_DMUXCR_DC_SSI	0x1
#define CCSR_GUTS_DMUXCR_DC_PAD	0x1
#define CCSR_GUTS_DMUXCR_DC_NC1	0x3
	u8	res4[0x70 - 0x6c];
	__be32	devdisr;	 
	u8	res5[0x7c - 0x74];
	__be32  pmjcr;		 
	__be32	powmgtcsr;	 
	__be32  pmrccr;		 
	__be32  pmpdccr;	 
	__be32  pmcdr;		 
	__be32	mcpsumr;	 
	__be32	rstrscr;	 
	__be32  ectrstcr;	 
	__be32  autorstsr;	 
	__be32	pvr;		 
	__be32	svr;		 
	u8	res6[0xB0 - 0xA8];
	__be32	rstcr;		 
	u8	res7[0xC0 - 0xB4];
	__be32	iovselsr;	 
	u8	res8[0x220 - 0xC4];
	__be32	dscr;
#define CCSR_GUTS_DSCR_ENB_PWR_DWN	0x80000000
#define CCSR_GUTS_DSCR_TRI_MCS_B	0x20000000
#define CCSR_GUTS_DSCR_TRI_MCK		0x10000000
#define CCSR_GUTS_DSCR_TRI_MCKE		0x08000000
#define CCSR_GUTS_DSCR_TRI_MODT		0x04000000
	__be32	iodelay1;	 
	__be32  iodelay2;	 
	u8	res9[0x800 - 0x22c];
	__be32  clkdvdr;	 
	u8	res10[0xb28 - 0x804];
	__be32	ddrclkdr;	 
	u8	res11[0xc00 - 0xB2c];
	__be32	esr;		 
	u8	res12[0xe00 - 0xc04];
	__be32	clkocr;		 
	u8	res13[0xe20 - 0xe04];
	__be32	ecmcr;		 
	__be32	cpfor;		 
	u8	res14[0xf2c - 0xE28];
	__be32	itcr;		 
	u8	res15[0x1000 - 0xf30];
} __attribute__ ((packed));

#endif  
#endif  
#endif  
