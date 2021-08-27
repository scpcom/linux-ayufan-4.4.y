 
#ifndef __AL_INIT_ETH_LM_H__
#define __AL_INIT_ETH_LM_H__

#include "al_hal_serdes.h"
#include "al_hal_eth.h"
#include "al_init_eth_kr.h"

enum al_eth_lm_link_mode {
	AL_ETH_LM_MODE_DISCONNECTED,
	AL_ETH_LM_MODE_10G_OPTIC,
	AL_ETH_LM_MODE_10G_DA,
	AL_ETH_LM_MODE_1G,
};

struct al_eth_lm_context {
	struct al_hal_eth_adapter	*adapter;
	struct al_serdes_obj		*serdes_obj;
	enum al_serdes_group		grp;
	enum al_serdes_lane		lane;

	struct al_eth_link_status	last_link_status;
#ifdef CONFIG_SYNO_ALPINE_V2_5_3
	uint32_t			link_training_failures;
#else
	uint32_t			link_establish_failures;
#endif

	al_bool				tx_param_dirty;
	al_bool				serdes_tx_params_valid;
	struct al_serdes_adv_tx_params	tx_params_override;
	al_bool				rx_param_dirty;
	al_bool				serdes_rx_params_valid;
	struct al_serdes_adv_rx_params	rx_params_override;

	struct al_eth_an_adv		local_adv;
	struct al_eth_an_adv		partner_adv;

	enum al_eth_lm_link_mode	mode;
#ifdef CONFIG_SYNO_ALPINE_V2_5_3
	uint8_t				da_len;
#endif
	al_bool				debug;

	al_bool				sfp_detection;
#ifdef CONFIG_SYNO_ALPINE_V2_5_3
	uint8_t				sfp_bus_id;
	uint8_t				sfp_i2c_addr;
#endif

	enum al_eth_lm_link_mode	default_mode;
#ifdef CONFIG_SYNO_ALPINE_V2_5_3
	uint8_t				default_dac_len;
#endif
	al_bool				link_training;
	al_bool				rx_equal;
	al_bool				static_values;
#ifdef CONFIG_SYNO_ALPINE_V2_5_3
	al_bool				retimer_exist;
	uint8_t				retimer_bus_id;
	uint8_t				retimer_i2c_addr;
	enum al_eth_retimer_channel	retimer_channel;
#endif

#ifdef CONFIG_SYNO_ALPINE_V2_5_3
	int (*i2c_read)(void *handle, uint8_t bus_id, uint8_t i2c_addr,
			uint8_t reg_addr, uint8_t *val);
	int (*i2c_write)(void *handle, uint8_t bus_id, uint8_t i2c_addr,
			 uint8_t reg_addr, uint8_t val);
	void				*i2c_context;
#else
	int (*eeprom_read)(void *handle, uint8_t addr, uint8_t *val);
	void				*eeprom_context;
#endif
	uint8_t (*get_random_byte)(void);
};

struct al_eth_lm_init_params {
	 
	struct al_hal_eth_adapter	*adapter;
	 
	struct al_serdes_obj		*serdes_obj;
	 
	enum al_serdes_group		grp;
	 
	enum al_serdes_lane		lane;

	al_bool				sfp_detection;
#ifdef CONFIG_SYNO_ALPINE_V2_5_3
	 
	uint8_t				sfp_bus_id;
	 
	uint8_t				sfp_i2c_addr;
	 
#else
	 
#endif
	enum al_eth_lm_link_mode	default_mode;
#ifdef CONFIG_SYNO_ALPINE_V2_5_3
	uint8_t				default_dac_len;

	uint8_t				retimer_bus_id;
	uint8_t				retimer_i2c_addr;
	 
	enum al_eth_retimer_channel	retimer_channel;
#endif

#ifdef CONFIG_SYNO_ALPINE_V2_5_3
	al_bool				retimer_exist;
#endif
	al_bool				link_training;
	al_bool				rx_equal;
	al_bool				static_values;

	al_bool				kr_fec_enable;

#ifdef CONFIG_SYNO_ALPINE_V2_5_3
	int (*i2c_read)(void *handle, uint8_t bus_id, uint8_t i2c_addr,
			uint8_t reg_addr, uint8_t *val);
	int (*i2c_write)(void *handle, uint8_t bus_id, uint8_t i2c_addr,
			 uint8_t reg_addr, uint8_t val);
	void				*i2c_context;
#else
	int (*eeprom_read)(void *handle, uint8_t addr, uint8_t *val);
	void				*eeprom_context;
#endif
	 
	uint8_t (*get_random_byte)(void);
};

int al_eth_lm_init(struct al_eth_lm_context	*lm_context,
		   struct al_eth_lm_init_params	*params);

int al_eth_lm_link_detection(struct al_eth_lm_context	*lm_context,
			     al_bool			*link_fault,
			     enum al_eth_lm_link_mode	*old_mode,
			     enum al_eth_lm_link_mode	*new_mode);

int al_eth_lm_link_establish(struct al_eth_lm_context	*lm_context,
			     al_bool			*link_up);

int al_eth_lm_static_parameters_override(struct al_eth_lm_context	*lm_context,
					 struct al_serdes_adv_tx_params *tx_params,
					 struct al_serdes_adv_rx_params *rx_params);

int al_eth_lm_static_parameters_override_disable(
					struct al_eth_lm_context *lm_context,
					al_bool			 tx_params,
					al_bool			 rx_params);

int al_eth_lm_static_parameters_get(struct al_eth_lm_context	*lm_context,
				    struct al_serdes_adv_tx_params *tx_params,
				    struct al_serdes_adv_rx_params *rx_params);

const char *al_eth_lm_mode_convert_to_str(enum al_eth_lm_link_mode val);

void al_eth_lm_debug_mode_set(struct al_eth_lm_context	*lm_context,
			      al_bool			enable);

#endif
