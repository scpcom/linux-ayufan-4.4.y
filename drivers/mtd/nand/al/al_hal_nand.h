 
#ifndef __AL_HAL_NAND_H__
#define __AL_HAL_NAND_H__

#include "al_hal_common.h"
#ifdef CONFIG_SYNO_ALPINE_V2_5_3
#include "al_hal_ssm.h"
#include "al_hal_ssm_raid.h"
#else
#include "al_hal_raid.h"
#endif
#include "al_hal_nand_defs.h"

#ifdef __cplusplus
extern "C" {
#endif
 
int al_nand_init(
	struct al_nand_ctrl_obj	*obj,
	void __iomem		*nand_base,
#ifdef CONFIG_SYNO_ALPINE_V2_5_3
	struct al_ssm_dma	*raid_dma,
#else
	struct al_raid_dma	*raid_dma,
#endif
	uint32_t		raid_dma_qid);

void al_nand_terminate(
	struct al_nand_ctrl_obj	*obj);

void al_nand_reset(
	struct al_nand_ctrl_obj	*obj,
	int				reset_mask);

void al_nand_dev_select(
	struct al_nand_ctrl_obj	*obj,
	int			device_index);

int al_nand_dev_config_basic(
	struct al_nand_ctrl_obj *obj);

int al_nand_dev_config(
	struct al_nand_ctrl_obj		*obj,
	struct al_nand_dev_properties	*dev_properties,
	struct al_nand_ecc_config	*ecc_config);

int al_nand_properties_decode(
	void __iomem				*pbs_regs_base,
	struct al_nand_dev_properties		*dev_properties,
	struct al_nand_ecc_config		*ecc_config,
	struct al_nand_extra_dev_properties	*dev_ext_props);

void al_nand_cw_config(
	struct al_nand_ctrl_obj	*obj,
	uint32_t		cw_size,
	uint32_t		cw_count);

void al_nand_cw_config_buffs_prepare(
	struct al_nand_ctrl_obj	*obj,
	uint32_t		cw_size,
	uint32_t		cw_count,
	uint32_t		*buff_arr[2]);

int al_nand_cw_config_dma(
	struct al_nand_ctrl_obj	*obj,
	struct al_buf		tx_buff_arr[2],
	int			trigger_interrupt,
	int			*num_transactions);

void al_nand_ecc_set_enabled(
	struct al_nand_ctrl_obj	*obj,
	int			enabled);

void al_nand_wp_set_enable(
	struct al_nand_ctrl_obj	*obj,
	int			enable);

void al_nand_tx_set_enable(
	struct al_nand_ctrl_obj	*obj,
	int			enable);

void al_nand_misc_ctrl_buffs_prepare(
	struct al_nand_ctrl_obj	*obj,
	int			wp_enable,
	int			tx_enable,
	uint32_t		*tx_buff_arr[1]);

int al_nand_misc_ctrl_dma(
	struct al_nand_ctrl_obj	*obj,
	struct al_buf		tx_buff_arr[1],
	int			trigger_interrupt,
	int			*num_transactions);

int al_nand_uncorr_err_get(
	struct al_nand_ctrl_obj	*obj);

void al_nand_uncorr_err_clear(
	struct al_nand_ctrl_obj	*obj);

int al_nand_corr_err_get(
	struct al_nand_ctrl_obj	*obj);

void al_nand_corr_err_clear(
	struct al_nand_ctrl_obj	*obj);

int al_nand_dev_is_ready(
	struct al_nand_ctrl_obj	*obj);

int al_nand_cmd_seq_size_page_read(
	struct al_nand_ctrl_obj	*obj,
	int			num_bytes,
	int			ecc_enabled,
	int			*cmd_seq_buff_num_entries);

int al_nand_cmd_seq_gen_page_read(
	struct al_nand_ctrl_obj	*obj,
	int			column,
	int			row,
	int			num_bytes,
	int			ecc_enabled,
	uint32_t		*cmd_seq_buff,
	int			*cmd_seq_buff_num_entries,
	uint32_t		*cw_size,
	uint32_t		*cw_count);

void al_nand_cmd_seq_size_page_write(
	struct al_nand_ctrl_obj	*obj,
	int			num_bytes,
	int			ecc_enabled,
	int			*cmd_seq_buff_num_entries);

int al_nand_cmd_seq_gen_page_write(
	struct al_nand_ctrl_obj	*obj,
	int			column,
	int			row,
	int			num_bytes,
	int			ecc_enabled,
	uint32_t		*cmd_seq_buff,
	int			*cmd_seq_buff_num_entries,
	uint32_t		*cw_size,
	uint32_t		*cw_count);

#define AL_NAND_CMD_SEQ_ENTRY(type, arg)	\
	(((type) << 8) | (arg))

void al_nand_cmd_single_execute(
	struct al_nand_ctrl_obj	*obj,
	uint32_t		cmd);

void al_nand_cmd_seq_execute(
	struct al_nand_ctrl_obj	*obj,
	uint32_t		*cmd_seq_buff,
	int			cmd_seq_buff_num_entries);

int al_nand_cmd_buff_is_empty(
	struct al_nand_ctrl_obj	*obj);

int al_nand_cmd_seq_execute_dma(
	struct al_nand_ctrl_obj	*obj,
	struct al_buf		*cmd_seq_buff,
	int			trigger_interrupt);

void al_nand_cmd_seq_scion_buff_prepare(
	struct al_nand_ctrl_obj	*obj,
	uint32_t		*buff);

int al_nand_cmd_seq_scion_dma(
	struct al_nand_ctrl_obj	*obj,
	struct al_buf		*tx_buff,
	int			trigger_interrupt,
	int			*num_transactions);

void __iomem *al_nand_data_buff_base_get(
			struct al_nand_ctrl_obj	*obj);

int al_nand_data_buff_read(
	struct al_nand_ctrl_obj	*obj,
	int			num_bytes,
	int			num_bytes_skip_head,
	int			num_bytes_skip_tail,
	uint8_t			*buff);

int al_nand_data_buff_read_dma(
	struct al_nand_ctrl_obj	*obj,
	struct al_buf		*buff,
	int			trigger_interrupt);

int al_nand_data_buff_write(
	struct al_nand_ctrl_obj	*obj,
	int			num_bytes,
	const uint8_t		*buff);

int al_nand_data_buff_write_dma(
	struct al_nand_ctrl_obj	*obj,
	struct al_buf		*buff,
	int			trigger_interrupt);

int al_nand_transaction_completion(
	struct al_nand_ctrl_obj	*obj,
	uint32_t		*comp_status);

uint32_t al_nand_int_status_get(
		struct al_nand_ctrl_obj	*obj);

void al_nand_int_enable(
		struct al_nand_ctrl_obj	*obj,
		uint32_t int_mask);

void al_nand_int_disable(
		struct al_nand_ctrl_obj	*obj,
		uint32_t int_mask);

#ifdef __cplusplus
}
#endif
 
#endif		 
