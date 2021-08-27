/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
********************************************************************************/

/* Marvell Telephony Adaptation Layer */

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
EXPORT_SYMBOL(tal_init);

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
EXPORT_SYMBOL(tal_pcm_start);

/*---------------------------------------------------------------------------*
 * tal_pcm_stop
 * Stop PCM bus
 *---------------------------------------------------------------------------*/
tal_stat_t tal_pcm_stop(void)
{
	tal_tdm_if_register_ops.tdm_if_ctl_ops.ctl_pcm_stop();
	return TAL_STAT_OK;
}
EXPORT_SYMBOL(tal_pcm_stop);

/*---------------------------------------------------------------------------*
 * tal_exit
 * Stop TDM channels and release all resources
 *---------------------------------------------------------------------------*/
tal_stat_t tal_exit(void)
{
	tdm_if_exit();
	return TAL_STAT_OK;
}
EXPORT_SYMBOL(tal_exit);

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
EXPORT_SYMBOL(tal_stats_get);

#if defined(MV_TDM_USE_DCO)
/*---------------------------------------------------------------------------*
 * tal_tdm_clk_config
 * Config TDM clock
 *---------------------------------------------------------------------------*/
void tal_tdm_clk_config(void)
{
	mvCtrlTdmClkCtrlConfig();
}
EXPORT_SYMBOL(tal_tdm_clk_config);

/*---------------------------------------------------------------------------*
 * tal_tdm_clk_get
 * Get current TDM clock correction
 *---------------------------------------------------------------------------*/
int tal_tdm_clk_get(void)
{
	return mvCtrlTdmClkCtrlGet();
}
EXPORT_SYMBOL(tal_tdm_clk_get);

/*---------------------------------------------------------------------------*
 * tal_tdm_clk_set
 * Set TDM clock correction
 *---------------------------------------------------------------------------*/
void tal_tdm_clk_set(int correction)
{
	return mvCtrlTdmClkCtrlSet(correction);
}
EXPORT_SYMBOL(tal_tdm_clk_set);

#endif
