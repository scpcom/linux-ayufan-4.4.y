
/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

********************************************************************************
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

********************************************************************************/

/* Marvell Telephony Adaptation Layer */

#include <linux/export.h>
#include "tal.h"
#include "tdm_if.h"

/* GLobals */
static tdm_if_register_ops_t tal_tdm_if_register_ops;
static tal_mmp_ops_t* tal_mmp_ops;
static tdm_if_params_t tal_tdm_if_params;

/* Static APIs */
static void tal_pcm_tx_callback(uint8_t* tx_buff, int size);
static void tal_pcm_rx_callback(uint8_t* rx_buff, int size);

/*---------------------------------------------------------------------------*
 * tal_init
 * Issue telephony subsytem initialization and callbacks registration
 *---------------------------------------------------------------------------*/
tal_stat_t tal_init(tal_params_t* tal_params, tal_mmp_ops_t* mmp_ops)
{
	if((tal_params == NULL) || (mmp_ops == NULL))
	{
		mvOsPrintf("%s: Error, bad parameters\n",__FUNCTION__);
		return TAL_STAT_BAD_PARAM;
	}

	if(mmp_ops->tal_mmp_rx_callback == NULL ||
	   mmp_ops->tal_mmp_tx_callback == NULL)
	{
		mvOsPrintf("%s:Error, missing callbacks(MMP)\n",__FUNCTION__);
		return TAL_STAT_BAD_PARAM;
	}

	/* Convert tal_params to tdm_if_params */
	memcpy(&tal_tdm_if_params, tal_params, sizeof(tal_params_t));

	/* Assign MMP operations */
	tal_mmp_ops = mmp_ops;

	/* Clear tdm_if operations structure */
	memset(&tal_tdm_if_register_ops, 0, sizeof(tdm_if_register_ops_t));

	/* Assign tdm_if operations */
	tal_tdm_if_register_ops.tdm_if_pcm_ops.pcm_tx_callback = tal_pcm_tx_callback;
	tal_tdm_if_register_ops.tdm_if_pcm_ops.pcm_rx_callback = tal_pcm_rx_callback;

	/* Dispatch tdm_if driver */
	if(tdm_if_init(&tal_tdm_if_register_ops, &tal_tdm_if_params) != MV_OK)
	{
		mvOsPrintf("%s: Error, could not initialize tdm_if driver !!!\n",__FUNCTION__);
		return TAL_STAT_INIT_ERROR;
	}

	/* Verify control callbacks were assigned properly */
	if(tal_tdm_if_register_ops.tdm_if_ctl_ops.ctl_pcm_start == NULL ||
	   tal_tdm_if_register_ops.tdm_if_ctl_ops.ctl_pcm_stop == NULL)
	{
		mvOsPrintf("%s:Error, missing callbacks(tdm_if)\n",__FUNCTION__);
		return TAL_STAT_BAD_PARAM;
	}

	return TAL_STAT_OK;
}

/*---------------------------------------------------------------------------*
 * tal_pcm_tx_completion
 * Tx callback
 *---------------------------------------------------------------------------*/

static void tal_pcm_tx_callback(uint8_t* tx_buff, int size)
{
	tal_mmp_ops->tal_mmp_tx_callback(tx_buff, size);
}

/*---------------------------------------------------------------------------*
 * tal_pcm_rx_completion
 * Rx callback
 *---------------------------------------------------------------------------*/

static void tal_pcm_rx_callback(uint8_t* rx_buff, int size)
{
	tal_mmp_ops->tal_mmp_rx_callback(rx_buff, size);
}

/*---------------------------------------------------------------------------*
 * tal_pcm_start
 * Start PCM bus
 *---------------------------------------------------------------------------*/
tal_stat_t tal_pcm_start(void)
{
	tal_tdm_if_register_ops.tdm_if_ctl_ops.ctl_pcm_start();
	return TAL_STAT_OK;
}

/*---------------------------------------------------------------------------*
 * tal_pcm_stop
 * Stop PCM bus
 *---------------------------------------------------------------------------*/
tal_stat_t tal_pcm_stop(void)
{
	tal_tdm_if_register_ops.tdm_if_ctl_ops.ctl_pcm_stop();
	return TAL_STAT_OK;
}

/*---------------------------------------------------------------------------*
 * tal_exit
 * Stop TDM channels and release all resources
 *---------------------------------------------------------------------------*/
tal_stat_t tal_exit(void)
{
	tdm_if_exit();
	return TAL_STAT_OK;
}

/*---------------------------------------------------------------------------*
 * tal_stats_get
 * Get TDM statistics
 *---------------------------------------------------------------------------*/
tal_stat_t tal_stats_get(tal_stats_t* tal_stats)
{
	tdm_if_stats_t stats;

	tdm_if_stats_get(&stats);
	memcpy(tal_stats, &stats, sizeof(tal_stats_t));

	return TAL_STAT_OK;
}

/*---------------------------------------------------------------------------*
 * tal_tdm_clk_config
 * Config TDM clock
 *---------------------------------------------------------------------------*/
void tal_tdm_clk_config(void)
{
	return;
}

#if defined(MV_TDM_USE_DCO)
/*---------------------------------------------------------------------------*
 * tal_tdm_clk_get
 * Get current TDM clock correction
 *---------------------------------------------------------------------------*/
int tal_tdm_clk_get(void)
{
	return mvCtrlTdmClkCtrlGet();
}

/*---------------------------------------------------------------------------*
 * tal_tdm_clk_set
 * Set TDM clock correction
 *---------------------------------------------------------------------------*/
void tal_tdm_clk_set(int correction)
{
	return mvCtrlTdmClkCtrlSet(correction);
}
#endif

/* EXPORTS */
EXPORT_SYMBOL(tal_init);
EXPORT_SYMBOL(tal_pcm_start);
EXPORT_SYMBOL(tal_pcm_stop);
EXPORT_SYMBOL(tal_exit);
EXPORT_SYMBOL(tal_stats_get);
EXPORT_SYMBOL(tal_tdm_clk_config);
#if defined(MV_TDM_USE_DCO)
EXPORT_SYMBOL(tal_tdm_clk_get);
EXPORT_SYMBOL(tal_tdm_clk_set);
#endif
