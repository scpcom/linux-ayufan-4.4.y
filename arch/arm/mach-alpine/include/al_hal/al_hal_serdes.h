 
#ifndef __AL_HAL_SERDES_H__
#define __AL_HAL_SERDES_H__

#include "al_hal_common.h"

#ifdef __cplusplus
extern "C" {
#endif
 
struct al_serdes_obj;

enum al_serdes_group {
	AL_SRDS_GRP_A = 0,
	AL_SRDS_GRP_B,
	AL_SRDS_GRP_C,
	AL_SRDS_GRP_D,

	AL_SRDS_NUM_GROUPS
};

struct al_serdes_group_info {
	 
	struct al_serdes_obj		*pobj;

	struct al_serdes_regs __iomem	*regs_base;
};

struct al_serdes_obj {
	struct al_serdes_group_info	grp_info[AL_SRDS_NUM_GROUPS];
};

enum al_serdes_reg_page {
	AL_SRDS_REG_PAGE_0_LANE_0 = 0,
	AL_SRDS_REG_PAGE_1_LANE_1,
	AL_SRDS_REG_PAGE_2_LANE_2,
	AL_SRDS_REG_PAGE_3_LANE_3,
	AL_SRDS_REG_PAGE_4_COMMON,
	AL_SRDS_REG_PAGE_0123_LANES_0123 = 7,
};

enum al_serdes_reg_type {
	AL_SRDS_REG_TYPE_PMA = 0,
	AL_SRDS_REG_TYPE_PCS,
};

enum al_serdes_lane {
	AL_SRDS_LANE_0 = AL_SRDS_REG_PAGE_0_LANE_0,
	AL_SRDS_LANE_1 = AL_SRDS_REG_PAGE_1_LANE_1,
	AL_SRDS_LANE_2 = AL_SRDS_REG_PAGE_2_LANE_2,
	AL_SRDS_LANE_3 = AL_SRDS_REG_PAGE_3_LANE_3,

#ifdef CONFIG_SYNO_ALPINE_A0
	AL_SRDS_NUM_LANES,
	AL_SRDS_LANES_0123 = AL_SRDS_REG_PAGE_0123_LANES_0123,
#else
	AL_SRDS_NUM_LANES
#endif
};

enum al_serdes_lb_mode {
	 
	AL_SRDS_LB_MODE_OFF,

	AL_SRDS_LB_MODE_PMA_IO_UN_TIMED_RX_TO_TX,

	AL_SRDS_LB_MODE_PMA_INTERNALLY_BUFFERED_SERIAL_TX_TO_RX,

	AL_SRDS_LB_MODE_PMA_SERIAL_TX_IO_TO_RX_IO,

	AL_SRDS_LB_MODE_PMA_PARALLEL_RX_TO_TX,

	AL_SRDS_LB_MODE_PCS_PIPE,

	AL_SRDS_LB_MODE_PCS_NEAR_END,

	AL_SRDS_LB_MODE_PCS_FAR_END,
};

enum al_serdes_bist_pattern {
	AL_SRDS_BIST_PATTERN_USER,
	AL_SRDS_BIST_PATTERN_PRBS7,
	AL_SRDS_BIST_PATTERN_PRBS23,
	AL_SRDS_BIST_PATTERN_PRBS31,
	AL_SRDS_BIST_PATTERN_CLK1010,
};

enum al_serdes_rate {
	AL_SRDS_RATE_1_8,
	AL_SRDS_RATE_1_4,
	AL_SRDS_RATE_1_2,
	AL_SRDS_RATE_FULL,
};

enum al_serdes_pm {
	AL_SRDS_PM_PD,
	AL_SRDS_PM_P2,
	AL_SRDS_PM_P1,
	AL_SRDS_PM_P0S,
	AL_SRDS_PM_P0,
};

int al_serdes_handle_init(
	void __iomem		*serdes_regs_base,
	struct al_serdes_obj	*obj);

int al_serdes_reg_read(
	struct al_serdes_obj	*obj,
	enum al_serdes_group	grp,
	enum al_serdes_reg_page	page,
	enum al_serdes_reg_type	type,
	uint16_t		offset,
	uint8_t			*data);

int al_serdes_reg_write(
	struct al_serdes_obj	*obj,
	enum al_serdes_group	grp,
	enum al_serdes_reg_page	page,
	enum al_serdes_reg_type	type,
	uint16_t		offset,
	uint8_t			data);

void al_serdes_bist_overrides_enable(
	struct al_serdes_obj	*obj,
	enum al_serdes_group	grp,
	enum al_serdes_rate	rate);

void al_serdes_group_pm_set(
	struct al_serdes_obj	*obj,
	enum al_serdes_group	grp,
	enum al_serdes_pm	pm);

void al_serdes_lane_pm_set(
	struct al_serdes_obj	*obj,
	enum al_serdes_group	grp,
	enum al_serdes_lane	lane,
	enum al_serdes_pm	rx_pm,
	enum al_serdes_pm	tx_pm);

void al_serdes_pma_hard_reset_group(
	struct al_serdes_obj	*obj,
	enum al_serdes_group	grp,
	al_bool			enable);

void al_serdes_pma_hard_reset_lane(
	struct al_serdes_obj	*obj,
	enum al_serdes_group	grp,
	enum al_serdes_lane	lane,
	al_bool			enable);

void al_serdes_loopback_control(
	struct al_serdes_obj	*obj,
	enum al_serdes_group	grp,
	enum al_serdes_lane	lane,
	enum al_serdes_lb_mode	mode);

void al_serdes_bist_pattern_select(
	struct al_serdes_obj		*obj,
	enum al_serdes_group		grp,
	enum al_serdes_bist_pattern	pattern,
	uint8_t				*user_data);

void al_serdes_bist_tx_enable(
	struct al_serdes_obj	*obj,
	enum al_serdes_group	grp,
	enum al_serdes_lane	lane,
	al_bool			enable);

void al_serdes_bist_tx_err_inject(
	struct al_serdes_obj	*obj,
	enum al_serdes_group	grp);

void al_serdes_bist_rx_enable(
	struct al_serdes_obj	*obj,
	enum al_serdes_group	grp,
	enum al_serdes_lane	lane,
	al_bool			enable);

void al_serdes_bist_rx_status(
	struct al_serdes_obj	*obj,
	enum al_serdes_group	grp,
	enum al_serdes_lane	lane,
	al_bool			*is_locked,
	al_bool			*err_cnt_overflow,
	uint16_t		*err_cnt);

int al_serdes_digital_test_bus(
	struct al_serdes_obj	*obj,
	enum al_serdes_group	grp,
	enum al_serdes_lane	lane,
	uint8_t			sel,
	uint8_t			*sampled_data);

void al_serdes_tx_deemph_preset(
		struct al_serdes_obj	*obj,
		enum al_serdes_group	grp,
		enum al_serdes_lane	lane);

enum al_serdes_tx_deemph_param {
	AL_SERDES_TX_DEEMP_C_ZERO,	 
	AL_SERDES_TX_DEEMP_C_PLUS,	 
	AL_SERDES_TX_DEEMP_C_MINUS,	 
};

al_bool al_serdes_tx_deemph_inc(
		struct al_serdes_obj	*obj,
		enum al_serdes_group	grp,
		enum al_serdes_lane	lane,
		enum al_serdes_tx_deemph_param param);

al_bool al_serdes_tx_deemph_dec(
		struct al_serdes_obj	*obj,
		enum al_serdes_group	grp,
		enum al_serdes_lane	lane,
		enum al_serdes_tx_deemph_param param);

int al_serdes_eye_measure_run(
		struct al_serdes_obj	*obj,
		enum al_serdes_group	grp,
		enum al_serdes_lane	lane,
		uint32_t		timeout,
		unsigned int		*value);

int al_serdes_eye_diag_sample(
		struct al_serdes_obj	*obj,
		enum al_serdes_group	grp,
		enum al_serdes_lane	lane,
		unsigned int		x,
		int			y,
		unsigned int		timeout,
		unsigned int		*value);

al_bool al_serdes_signal_is_detected(
		struct al_serdes_obj	*obj,
		enum al_serdes_group	grp,
		enum al_serdes_lane	lane);

struct al_serdes_adv_tx_params {
	 
	al_bool				override;
	 
	uint8_t				amp;
	 
	uint8_t				total_driver_units;
	 
	uint8_t				c_plus_1;
	 
	uint8_t				c_plus_2;
	 
	uint8_t				c_minus_1;
	 
	uint8_t				slew_rate;
};

struct al_serdes_adv_rx_params {
	 
	al_bool				override;
	 
	uint8_t				dcgain;
	 
	uint8_t				dfe_3db_freq;
	 
	uint8_t				dfe_gain;
	 
	uint8_t				dfe_first_tap_ctrl;
	 
	uint8_t				dfe_secound_tap_ctrl;
	 
	uint8_t				dfe_third_tap_ctrl;
	 
	uint8_t				dfe_fourth_tap_ctrl;
	 
	uint8_t				low_freq_agc_gain;
	 
	uint8_t				precal_code_sel;
	 
	uint8_t				high_freq_agc_boost;
};

void al_serdes_tx_advanced_params_set(struct al_serdes_obj	      	*obj,
				      enum al_serdes_group		grp,
				      enum al_serdes_lane		lane,
				      struct al_serdes_adv_tx_params  *params);

#ifdef CONFIG_SYNO_ALPINE_A0
 
void al_serdes_tx_advanced_params_get(struct al_serdes_obj	     	*obj,
				      enum al_serdes_group	      	grp,
				      enum al_serdes_lane		lane,
				      struct al_serdes_adv_tx_params *params);
#endif

void al_serdes_rx_advanced_params_set(struct al_serdes_obj	      *obj,
				      enum al_serdes_group	      grp,
				      enum al_serdes_lane	      lane,
				      struct al_serdes_adv_rx_params  *params);

#ifdef CONFIG_SYNO_ALPINE_A0
 
void al_serdes_rx_advanced_params_get(struct al_serdes_obj           *obj,
				      enum al_serdes_group	      grp,
				      enum al_serdes_lane	      lane,
				      struct al_serdes_adv_rx_params* params);
#endif

void al_serdes_mode_set_sgmii(
	struct al_serdes_obj	*obj,
	enum al_serdes_group	grp);

void al_serdes_mode_set_kr(
	struct al_serdes_obj	*obj,
	enum al_serdes_group	grp);

#ifdef CONFIG_SYNO_ALPINE_A0
 
int al_serdes_rx_equalization(
		struct al_serdes_obj	*obj,
		enum al_serdes_group	grp,
		enum al_serdes_lane	lane);

int al_serdes_calc_eye_size(
		struct al_serdes_obj *obj,
		enum al_serdes_group grp,
		enum al_serdes_lane  lane,
		int*                 width,
		int*                 height);
#endif

#ifdef __cplusplus
}
#endif

#endif		 
