/*
 *  Copyright (c) 2009 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */


#include "types.h"
#include "system.h"
#include "fpp.h"
#include "multicast.h"

#if !defined(COMCERTO_1000)
int multicast_init(void)
{
	return 0;
}
#endif

#ifdef COMCERTO_1000
extern	unsigned char Image$$aram_mdma0$$Length[];
extern	unsigned char Image$$aram_mdma0$$ZI$$Length[];
extern	unsigned char Image$$aram_mdma0$$Base[];

#define MDMA0_DEV_MEMSIZE ((U32)Image$$aram_mdma0$$Length + (U32)Image$$aram_mdma0$$ZI$$Length)
#define MDMA0_DEV_MEMADDR Image$$aram_mdma0$$Base

int multicast_init(void)
{
	static BOOL mc_init_done = FALSE;
	U32 tmp;
	mdma_lane *pcntx;
	MMRXdesc tmp_dsc,*prxdsc;

	if (mc_init_done)
		return 0;

	if (( MDMA0_TX_NUMDESCR * sizeof(MMTXdesc) + MDMA0_RX_NUMDESCR *sizeof(MMRXdesc)) >  MDMA0_DEV_MEMSIZE )
		return 1; // Not enough memory should probably hang a chip here.

	// Fill control structure in software.
	pcntx= &mdma_lane0_cntx;
	memset(pcntx,0,sizeof(mdma_lane0_cntx));
	tmp = (U32)  MDMA0_DEV_MEMADDR;
	pcntx->tx_cur = pcntx->tx_top =  (void*) tmp;
	pcntx->tx_bot = pcntx->tx_top + (MDMA0_TX_NUMDESCR - 1);
	pcntx->free_tx = MDMA0_TX_NUMDESCR;
	memset( pcntx->tx_top, 0,  MDMA0_TX_NUMDESCR * sizeof(MMTXdesc));

	tmp += MDMA0_TX_NUMDESCR * sizeof(MMTXdesc);

	pcntx->rx_cur = pcntx->rx_top =  (void*) tmp;
	pcntx->rx_bot = pcntx->rx_top + (MDMA0_RX_NUMDESCR - 1);
	pcntx->pending_rx = 0;
	prxdsc=pcntx->rx_cur;

	// Initialise rx rescriptors
	memset(&tmp_dsc,0,sizeof(tmp_dsc));
	tmp_dsc.rxctl = ( MMRX_BUF_ALLOC | ((BaseOffset + ETH_HDR_SIZE)<< MMRX_OFFSET_SHIFT));
	do {
		SFL_memcpy(prxdsc, &tmp_dsc, sizeof(*prxdsc));
		prxdsc +=1;
	} while(prxdsc != pcntx->rx_bot);
	tmp_dsc.rxctl |= 	MMRX_WRAP;
	SFL_memcpy(pcntx->rx_bot, &tmp_dsc,sizeof(*prxdsc));

	// hw bindings
	pcntx->txcnt = (V32*)  MDMA_TX0_PATH_CTR;
	pcntx->rxcnt = (V32*)  MDMA_RX0_PATH_CTR;
	// Enable mdma block and lane0
	*(V32*) 	MDMA_GLB_CTRL_CFG = 1;
	*(V32*)	MDMA_TX0_PATH_HEAD = (U32) pcntx->tx_top;
	*(V32*)	MDMA_RX0_PATH_HEAD = (U32) pcntx->rx_top;
	*(V32*)	MDMA_INT_MASK_REG &= ~0x33UL; // turn off interrupts from lane0
	//TODO mat-20081017 - is burst length needed?
	// default is 0x10
	//      *(V32*)	MDMA_AHB_CFG  = 	(*(V32*)MDMA_AHB_CFG & ~0x1f) | TBD 
	// I am purposly not touching lane1
	// It is safe as long work is not queued to it.

	mc_init_done = TRUE;
	return 0;
}

#endif /* COMCERTO_1000 */
