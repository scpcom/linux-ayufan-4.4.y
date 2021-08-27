 
#ifndef _AL_HAL_PCIE_H_
#define _AL_HAL_PCIE_H_

#include "al_hal_common.h"

#define AL_PCIE_IB_HCRD_SUM				97

#define AL_PCIE_NOF_CPL_HDR_NOF_OS_OB_READS_MIN_RATIO	5

#define AL_PCIE_NOF_P_NP_HDR_MAX			24

struct al_pcie_port {
	struct al_pcie_regs __iomem *regs;

	uint8_t		rev_id;

	unsigned int	port_id;
	al_bool		write_to_read_only_enabled;
	uint8_t		max_lanes;

	al_bool		ib_hcrd_config_required;

	unsigned int	nof_np_hdr;

	unsigned int	nof_p_hdr;

	uint32_t __iomem *app_int_grp_a_base;
	uint32_t __iomem *app_int_grp_b_base;
	uint32_t __iomem *axi_int_grp_a_base;
};

enum al_pcie_function_mode {
	AL_PCIE_FUNCTION_MODE_EP,
	AL_PCIE_FUNCTION_MODE_RC,
	AL_PCIE_FUNCTION_MODE_UNKNOWN
};

enum al_pcie_link_speed {
	AL_PCIE_LINK_SPEED_DEFAULT,
	AL_PCIE_LINK_SPEED_GEN1 = 1,
	AL_PCIE_LINK_SPEED_GEN2 = 2,
	AL_PCIE_LINK_SPEED_GEN3 = 3
};

struct al_pcie_max_capability {
	al_bool		end_point_mode_supported;
	al_bool		root_complex_mode_supported;
	enum al_pcie_link_speed	max_speed;
	uint8_t		max_lanes;
	al_bool		reversal_supported;
	uint8_t		atu_regions_num;
	uint32_t	atu_min_size;
};

struct al_pcie_link_params {
	enum al_pcie_link_speed	max_speed;
	al_bool			enable_reversal;
};

struct al_pcie_gen2_params {
	al_bool	tx_swing_low;  
	al_bool	tx_compliance_receive_enable;
	al_bool	set_deemphasis;
};

struct al_pcie_gen3_lane_eq_params {
	uint8_t		downstream_port_transmitter_preset;
	uint8_t		downstream_port_receiver_preset_hint;
	uint8_t		upstream_port_transmitter_preset;
	uint8_t		upstream_port_receiver_preset_hint;
};

struct al_pcie_gen3_params {
	al_bool	perform_eq;
	al_bool	interrupt_enable_on_link_eq_request;
	struct al_pcie_gen3_lane_eq_params *eq_params;  
	int	eq_params_elements;  

	al_bool	eq_disable;  
	al_bool eq_phase2_3_disable;  
				      
	uint8_t local_lf;  
			   
	uint8_t	local_fs;  
};

struct al_pcie_tl_credits_params {
};

struct al_pcie_ep_bar_params {
	al_bool		enable;
	al_bool		memory_space;  
	al_bool		memory_64_bit;  
	al_bool		memory_is_prefetchable;
	uint64_t	size;  
};

struct al_pcie_ep_params {
	al_bool				cap_d1_d3hot_dis;
	al_bool				cap_flr_dis;
	al_bool				bar_params_valid;
	struct al_pcie_ep_bar_params	bar_params[6];
	struct al_pcie_ep_bar_params	exp_bar_params; 
};

struct al_pcie_features {
	 
	al_bool sata_ep_msi_fix;
};

struct al_pcie_ib_hcrd_os_ob_reads_config {
	 
	uint8_t nof_outstanding_ob_reads;

	unsigned int nof_cpl_hdr;

	unsigned int nof_np_hdr;

	unsigned int nof_p_hdr;
};

struct al_pcie_ep_iov_params {
	 
	al_bool sriov_vfunc_en;

	al_bool support_32b_address_in_iov;
};

struct al_pcie_latency_replay_timers {
	uint16_t	round_trip_lat_limit;
	uint16_t	replay_timer_limit;
};

struct al_pcie_config_params {
	enum al_pcie_function_mode	function_mode;  
	struct al_pcie_link_params	*link_params;
	al_bool				enable_axi_snoop;
	al_bool				enable_ram_parity_int;
	al_bool				enable_axi_parity_int;
	struct al_pcie_latency_replay_timers *lat_rply_timers;
	struct al_pcie_gen2_params *gen2_params;
	struct al_pcie_gen3_params	*gen3_params;
	struct al_pcie_tl_credits_params	*tl_credits;
	struct al_pcie_ep_params	*ep_params;
	struct al_pcie_features		*features;
	struct al_pcie_ep_iov_params	*ep_iov_params;
	al_bool				fast_link_mode;  
	al_bool                         enable_axi_slave_err_resp;  
};

struct al_pcie_link_status {
	al_bool			link_up;
	enum al_pcie_link_speed	speed;
	uint8_t			lanes;
	uint8_t			ltssm_state;
};

int al_pcie_port_enable(
	struct al_pcie_port	*pcie_port,
	void __iomem		*pbs_reg_base);

void al_pcie_port_disable(
	struct al_pcie_port	*pcie_port,
	void __iomem		*pbs_reg_base);

int al_pcie_handle_init(struct al_pcie_port *pcie_port,
			 void __iomem *pcie_reg_base,
			 unsigned int port_id);

int al_pcie_port_max_lanes_set(struct al_pcie_port *pcie_port, uint8_t lanes);

void al_pcie_port_memory_shutdown_set(
	struct al_pcie_port	*pcie_port,
	al_bool			enable);

int al_pcie_port_func_mode_config(struct al_pcie_port *pcie_port,
				  enum al_pcie_function_mode mode);

void al_pcie_port_ib_hcrd_os_ob_reads_config(
	struct al_pcie_port *pcie_port,
	struct al_pcie_ib_hcrd_os_ob_reads_config *ib_hcrd_os_ob_reads_config);

enum al_pcie_function_mode
al_pcie_function_type_get(struct al_pcie_port *pcie_port);

int al_pcie_port_config(struct al_pcie_port *pcie_port,
			struct al_pcie_config_params *params);

void al_pcie_app_req_retry_set(
	struct al_pcie_port	*pcie_port,
	al_bool			en);

int al_pcie_port_snoop_config(struct al_pcie_port *pcie_port,
				al_bool enable_axi_snoop);

int al_pcie_link_start(struct al_pcie_port *pcie_port);

int al_pcie_link_stop(struct al_pcie_port *pcie_port);

int al_pcie_link_up_wait(struct al_pcie_port *pcie_port, uint32_t timeout_ms);

int al_pcie_link_status(struct al_pcie_port *pcie_port, struct al_pcie_link_status *status);

int al_pcie_link_hot_reset(struct al_pcie_port *pcie_port);

int al_pcie_link_change_speed(struct al_pcie_port *pcie_port, enum al_pcie_link_speed new_speed);

int al_pcie_link_change_width(struct al_pcie_port *pcie_port, uint8_t width);

int al_pcie_target_bus_set(struct al_pcie_port *pcie_port,
			   uint8_t target_bus,
			   uint8_t mask_target_bus);

int al_pcie_target_bus_get(struct al_pcie_port *pcie_port,
			   uint8_t *target_bus,
			   uint8_t *mask_target_bus);

int al_pcie_secondary_bus_set(struct al_pcie_port *pcie_port, uint8_t secbus);

int al_pcie_subordinary_bus_set(struct al_pcie_port *pcie_port,uint8_t subbus);

int al_pcie_config_space_get(struct al_pcie_port *pcie_port,
			     uint8_t __iomem **addr);

uint32_t al_pcie_cfg_emul_local_cfg_space_read(
	struct al_pcie_port	*pcie_port,
	unsigned int	reg_offset);

void al_pcie_cfg_emul_local_cfg_space_write(
	struct al_pcie_port	*pcie_port,
	unsigned int	reg_offset,
	uint32_t	data,
	al_bool		ro);

enum al_pcie_atu_dir {
	al_pcie_atu_dir_outbound = 0,
	al_pcie_atu_dir_inbound = 1,
};

enum al_pcie_atu_tlp {
	AL_PCIE_TLP_TYPE_MEM = 0,
	AL_PCIE_TLP_TYPE_IO = 2,
	AL_PCIE_TLP_TYPE_CFG0 = 4,
	AL_PCIE_TLP_TYPE_CFG1 = 5,
	AL_PCIE_TLP_TYPE_MSG = 0x10,
	AL_PCIE_TLP_TYPE_RESERVED = 0x1f
};

struct al_pcie_atu_region {
	al_bool			enable;
	enum al_pcie_atu_dir	direction;  
	uint8_t			index;  
	uint64_t		base_addr;
	uint64_t		limit;  
	uint64_t		target_addr;  
	al_bool			invert_matching;
	enum al_pcie_atu_tlp	tlp_type;  
	uint8_t			attr;  
	 
	uint8_t			msg_code;  
	al_bool			cfg_shift_mode;
	 
	uint8_t			bar_number;
	uint8_t			match_mode;  
	al_bool			enable_attr_match_mode;
	al_bool			enable_msg_match_mode;
};

int al_pcie_atu_region_set(struct al_pcie_port *pcie_port, struct al_pcie_atu_region *atu_region);

#ifdef CONFIG_SYNO_ALPINE_V2_5_3
 
void al_pcie_axi_io_config(struct al_pcie_port *pcie_port, al_phys_addr_t start, al_phys_addr_t end);
#endif

enum al_pcie_legacy_int_type{
	AL_PCIE_LEGACY_INTA = 0,
	AL_PCIE_LEGACY_INTB,
	AL_PCIE_LEGACY_INTC,
	AL_PCIE_LEGACY_INTD
};

int al_pcie_legacy_int_gen(struct al_pcie_port *pcie_port, al_bool assert,
			   enum al_pcie_legacy_int_type type  );

int al_pcie_msi_int_gen(struct al_pcie_port *pcie_port, uint8_t vector);

int al_pcie_local_pipe_loopback_enter(struct al_pcie_port *pcie_port);

int al_pcie_local_pipe_loopback_exit(struct al_pcie_port *pcie_port);

int al_pcie_remote_loopback_enter(struct al_pcie_port *pcie_port);

int al_pcie_remote_loopback_exit(struct al_pcie_port *pcie_port);

#endif
 