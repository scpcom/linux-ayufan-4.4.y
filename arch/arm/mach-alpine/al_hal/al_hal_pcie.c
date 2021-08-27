 
#include "al_hal_pcie.h"
#include "al_hal_pbs_regs.h"
#include "al_hal_pcie_regs.h"
#include "al_hal_unit_adapter_regs.h"

#define AL_PCIE_REV_ID_0			0
#define AL_PCIE_REV_ID_1			1

#define AL_PCIE_AXI_REGS_OFFSET			0x0
#define AL_PCIE_APP_REGS_OFFSET			0x1000
#define AL_PCIE_CORE_CONF_BASE_OFFSET		0x2000

#define AL_PCIE_LTSSM_STATE_L0			0x11
#define AL_PCIE_LTSSM_STATE_L0S			0x12
#define AL_PCIE_DEVCTL_PAYLOAD_128B		0x00
#define AL_PCIE_DEVCTL_PAYLOAD_256B		0x20

#define AL_PCIE_SECBUS_DEFAULT			0x1
#define AL_PCIE_SUBBUS_DEFAULT			0x1
#define AL_PCIE_LINKUP_WAIT_INTERVAL		50	 
#define AL_PCIE_LINKUP_WAIT_INTERVALS_PER_SEC	20

#define AL_PCIE_LINKUP_RETRIES			8

#define AL_PCIE_MAX_32_MEMORY_BAR_SIZE		(0x100000000ULL)
#define AL_PCIE_MIN_MEMORY_BAR_SIZE		(1 << 12)
#define AL_PCIE_MIN_IO_BAR_SIZE			(1 << 8)

#define AL_PCIE_PARSE_LANES(v)		(((1 << v) - 1) << \
		PCIE_AXI_MISC_PCIE_GLOBAL_CONF_NOF_ACT_LANES_SHIFT)

static void
al_pcie_port_enable_wr_to_rd_only(struct al_pcie_port *pcie_port)
{
	if (pcie_port->write_to_read_only_enabled == AL_TRUE)
		return;
	al_dbg("PCIe %d: Enable write to Read Only fields\n", pcie_port->port_id);
	al_reg_write32(&pcie_port->regs->core_space.port_regs.rd_only_wr_en, 1);

	pcie_port->write_to_read_only_enabled = AL_TRUE;
}

void al_reg_write32_dbi_cs2(uint32_t * offset, uint32_t val)
{
	al_reg_write32(offset + (0x1000 >> 2),val);
}

int al_pcie_port_max_lanes_set(struct al_pcie_port *pcie_port, uint8_t lanes)
{
	struct al_pcie_regs *regs = pcie_port->regs;

	uint32_t active_lanes_val = AL_PCIE_PARSE_LANES(lanes);

	al_reg_write32_masked(&regs->axi.pcie_global.conf,
			      PCIE_AXI_MISC_PCIE_GLOBAL_CONF_NOF_ACT_LANES_MASK,
			      active_lanes_val);

	pcie_port->max_lanes = lanes;
	return 0;
}

void al_pcie_port_memory_shutdown_set(
	struct al_pcie_port	*pcie_port,
	al_bool			enable)
{
	struct al_pcie_regs *regs = pcie_port->regs;

	al_reg_write32_masked(&regs->axi.pcie_global.conf,
	      PCIE_AXI_MISC_PCIE_GLOBAL_CONF_MEM_SHUTDOWN,
	      enable == AL_TRUE ?
	      PCIE_AXI_MISC_PCIE_GLOBAL_CONF_MEM_SHUTDOWN : 0);
}

static unsigned int al_pcie_speed_gen_code(enum al_pcie_link_speed speed)
{
	if (speed == AL_PCIE_LINK_SPEED_GEN1)
		return 1;
	if (speed == AL_PCIE_LINK_SPEED_GEN2)
		return 2;
	if (speed == AL_PCIE_LINK_SPEED_GEN3)
		return 3;
	 
	return 0;
}

static int
al_pcie_port_link_config(struct al_pcie_port *pcie_port,
	       		 struct al_pcie_link_params *link_params)
{
	struct al_pcie_regs *regs = pcie_port->regs;

	al_dbg("PCIe %d: link config: max speed gen %d, max lanes %d, reversal %s\n",
	       pcie_port->port_id, link_params->max_speed,
	       pcie_port->max_lanes, link_params->enable_reversal? "enable" : "disable");

	al_pcie_port_enable_wr_to_rd_only(pcie_port);

	if (link_params->max_speed != AL_PCIE_LINK_SPEED_DEFAULT) {
		uint16_t max_speed_val = (uint16_t)al_pcie_speed_gen_code(link_params->max_speed);
		al_reg_write32_masked((uint32_t __iomem *)(&regs->core_space.pcie_link_cap_base),
				       0xF, max_speed_val);
		al_reg_write32_masked((uint32_t __iomem *)(&regs->core_space.pcie_cap_base + (AL_PCI_EXP_LNKCTL2 >> 2)),
				       0xF, max_speed_val);
	}

	if (link_params->enable_reversal) {
		al_err("PCIe %d: enabling reversal mode not implemented\n",
			pcie_port->port_id);
		return -ENOSYS;
	}
	return 0;
}

static void al_pcie_port_ram_parity_int_config(
	struct al_pcie_port *pcie_port,
	al_bool enable)
{
	struct al_pcie_regs *regs = pcie_port->regs;

	al_reg_write32(&regs->app.parity.en_core,
		(enable == AL_TRUE) ? 0xffffffff : 0x0);
}

static void al_pcie_port_axi_parity_int_config(
	struct al_pcie_port *pcie_port,
	al_bool enable)
{
	struct al_pcie_regs *regs = pcie_port->regs;

	al_reg_write32(&regs->axi.parity.en_axi,
		(enable == AL_TRUE) ? 0xffffffff : 0x0);
}

static int
al_pcie_port_lat_rply_timers_config(struct al_pcie_port * pcie_port,
				struct al_pcie_latency_replay_timers  *lat_rply_timers)
{
	struct al_pcie_regs *regs = pcie_port->regs;
	uint32_t	reg = 0;

	AL_REG_FIELD_SET(reg, 0xFFFF, 0, lat_rply_timers->round_trip_lat_limit);
	AL_REG_FIELD_SET(reg, 0xFFFF0000, 16, lat_rply_timers->replay_timer_limit);

	al_reg_write32(&regs->core_space.port_regs.ack_lat_rply_timer, reg);
	return 0;
}

int
al_pcie_port_snoop_config(struct al_pcie_port *pcie_port, al_bool enable_axi_snoop)
{
	struct al_pcie_regs *regs = pcie_port->regs;

	al_info("PCIE_%d: snoop mode %s\n",
			pcie_port->port_id, enable_axi_snoop ? "enable" : "disable");

	if (enable_axi_snoop) {
		al_reg_write32_masked(&regs->axi.ctrl.master_arctl,
			PCIE_AXI_CTRL_MASTER_ARCTL_OVR_SNOOP | PCIE_AXI_CTRL_MASTER_ARCTL_SNOOP,
			PCIE_AXI_CTRL_MASTER_ARCTL_OVR_SNOOP | PCIE_AXI_CTRL_MASTER_ARCTL_SNOOP);

		al_reg_write32_masked(&regs->axi.ctrl.master_awctl,
			PCIE_AXI_CTRL_MASTER_AWCTL_OVR_SNOOP | PCIE_AXI_CTRL_MASTER_AWCTL_SNOOP,
			PCIE_AXI_CTRL_MASTER_AWCTL_OVR_SNOOP | PCIE_AXI_CTRL_MASTER_AWCTL_SNOOP);
	} else {
		al_reg_write32_masked(&regs->axi.ctrl.master_arctl,
			PCIE_AXI_CTRL_MASTER_ARCTL_OVR_SNOOP | PCIE_AXI_CTRL_MASTER_ARCTL_SNOOP,
			PCIE_AXI_CTRL_MASTER_ARCTL_OVR_SNOOP);

		al_reg_write32_masked(&regs->axi.ctrl.master_awctl,
			PCIE_AXI_CTRL_MASTER_AWCTL_OVR_SNOOP | PCIE_AXI_CTRL_MASTER_AWCTL_SNOOP,
			PCIE_AXI_CTRL_MASTER_AWCTL_OVR_SNOOP);
	}
	return 0;
}

static int
al_pcie_port_gen2_params_config(struct al_pcie_port *pcie_port,
				struct al_pcie_gen2_params *gen2_params)
{
	struct al_pcie_regs *regs = pcie_port->regs;
	uint32_t gen2_ctrl;

	al_dbg("PCIe %d: Gen2 params config: Tx Swing %s, interrupt on link Eq %s, set Deemphasis %s\n",
	       pcie_port->port_id,
	       gen2_params->tx_swing_low ? "Low" : "Full",
	       gen2_params->tx_compliance_receive_enable? "enable" : "disable",
	       gen2_params->set_deemphasis? "enable" : "disable");

	gen2_ctrl = al_reg_read32(&regs->core_space.port_regs.gen2_ctrl);

	if (gen2_params->tx_swing_low)
		AL_REG_BIT_SET(gen2_ctrl, PCIE_PORT_GEN2_CTRL_TX_SWING_LOW_SHIFT);
	else
		AL_REG_BIT_CLEAR(gen2_ctrl, PCIE_PORT_GEN2_CTRL_TX_SWING_LOW_SHIFT);

	if (gen2_params->tx_compliance_receive_enable)
		AL_REG_BIT_SET(gen2_ctrl, PCIE_PORT_GEN2_CTRL_TX_COMPLIANCE_RCV_SHIFT);
	else
		AL_REG_BIT_CLEAR(gen2_ctrl, PCIE_PORT_GEN2_CTRL_TX_COMPLIANCE_RCV_SHIFT);

	if (gen2_params->set_deemphasis)
		AL_REG_BIT_SET(gen2_ctrl, PCIE_PORT_GEN2_CTRL_DEEMPHASIS_SET_SHIFT);
	else
		AL_REG_BIT_CLEAR(gen2_ctrl, PCIE_PORT_GEN2_CTRL_DEEMPHASIS_SET_SHIFT);

	al_reg_write32(&regs->core_space.port_regs.gen2_ctrl, gen2_ctrl);

	return 0;
}

static uint16_t
gen3_lane_eq_param_to_val(struct al_pcie_gen3_lane_eq_params *eq_params)
{
	uint16_t eq_control = 0;

	eq_control = eq_params->downstream_port_transmitter_preset & 0xF;
	eq_control |= (eq_params->downstream_port_receiver_preset_hint & 0x7) << 4;
	eq_control |= (eq_params->upstream_port_transmitter_preset & 0xF) << 8;
	eq_control |= (eq_params->upstream_port_receiver_preset_hint & 0x7) << 12;

	return eq_control;
}

static int
al_pcie_port_gen3_params_config(struct al_pcie_port *pcie_port,
				struct al_pcie_gen3_params *gen3_params)
{
	struct al_pcie_regs *regs = pcie_port->regs;
	uint32_t reg = 0;
	uint16_t __iomem *lanes_eq_base = (uint16_t __iomem *)(&regs->core_space.pcie_sec_ext_cap_base + (0xC >> 2));
	int i;

	al_dbg("PCIe %d: Gen3 params config: Equalization %s, interrupt on link Eq %s\n",
	       pcie_port->port_id,
	       gen3_params->perform_eq ? "enable" : "disable",
	       gen3_params->interrupt_enable_on_link_eq_request? "enable" : "disable");

	if (gen3_params->perform_eq)
		AL_REG_BIT_SET(reg, 0);
	if (gen3_params->interrupt_enable_on_link_eq_request)
		AL_REG_BIT_SET(reg, 1);

	al_reg_write32(&regs->core_space.pcie_sec_ext_cap_base + (4 >> 2),
		       reg);

	for (i = 0; i < gen3_params->eq_params_elements; i += 2) {
		uint32_t eq_control =
			(uint32_t)gen3_lane_eq_param_to_val(gen3_params->eq_params + i) |
			(uint32_t)gen3_lane_eq_param_to_val(gen3_params->eq_params + i + 1) << 16;

		al_dbg("PCIe %d: Set EQ (0x%08x) for lane %d, %d\n", pcie_port->port_id, eq_control, i, i + 1);
		al_reg_write32((uint32_t *)(lanes_eq_base + i), eq_control);
	}
	reg = al_reg_read32(&regs->core_space.port_regs.gen3_ctrl);
	if (gen3_params->eq_disable)
		AL_REG_BIT_SET(reg, PCIE_PORT_GEN3_CTRL_EQ_DISABLE_SHIFT);
	else
		AL_REG_BIT_CLEAR(reg, PCIE_PORT_GEN3_CTRL_EQ_DISABLE_SHIFT);

	if (gen3_params->eq_phase2_3_disable)
		AL_REG_BIT_SET(reg, PCIE_PORT_GEN3_CTRL_EQ_PHASE_2_3_DISABLE_SHIFT);
	else
		AL_REG_BIT_CLEAR(reg, PCIE_PORT_GEN3_CTRL_EQ_PHASE_2_3_DISABLE_SHIFT);

	al_reg_write32(&regs->core_space.port_regs.gen3_ctrl, reg);

	reg = 0;
	AL_REG_FIELD_SET(reg, PCIE_PORT_GEN3_EQ_LF_MASK,
			 PCIE_PORT_GEN3_EQ_LF_SHIFT,
			 gen3_params->local_lf);
	AL_REG_FIELD_SET(reg, PCIE_PORT_GEN3_EQ_FS_MASK,
			 PCIE_PORT_GEN3_EQ_FS_SHIFT,
			 gen3_params->local_fs);

	al_reg_write32(&regs->core_space.port_regs.gen3_eq_fs_lf, reg);

	reg = 0;
	AL_REG_FIELD_SET(reg, PCIE_AXI_MISC_ZERO_LANEX_PHY_MAC_LOCAL_LF_MASK,
			 PCIE_AXI_MISC_ZERO_LANEX_PHY_MAC_LOCAL_LF_SHIFT,
			 gen3_params->local_lf);
	AL_REG_FIELD_SET(reg, PCIE_AXI_MISC_ZERO_LANEX_PHY_MAC_LOCAL_FS_MASK,
			 PCIE_AXI_MISC_ZERO_LANEX_PHY_MAC_LOCAL_FS_SHIFT,
			 gen3_params->local_fs);
	al_reg_write32(&regs->axi.conf.zero_lane0, reg);
	al_reg_write32(&regs->axi.conf.zero_lane1, reg);
	al_reg_write32(&regs->axi.conf.zero_lane2, reg);
	al_reg_write32(&regs->axi.conf.zero_lane3, reg);

	reg = 0x00001831;
	al_reg_write32(&regs->core_space.port_regs.gen3_eq_ctrl, reg);

	return 0;
}

static int
al_pcie_port_tl_credits_config(struct al_pcie_port *pcie_port,
				struct al_pcie_tl_credits_params  *tl_credits __attribute__((__unused__)))
{
	al_err("PCIe %d: transport layer credits config not implemented\n",
		pcie_port->port_id);

	return -ENOSYS;

}

static int
al_pcie_port_ep_params_config(struct al_pcie_port *pcie_port,
			      struct al_pcie_ep_params *ep_params)
{
	struct al_pcie_regs *regs = pcie_port->regs;
	int bar_idx;

	al_pcie_port_enable_wr_to_rd_only(pcie_port);

	if (ep_params->cap_d1_d3hot_dis)
		al_reg_write32_masked(
			&regs->core_space.pcie_pm_cap_base,
			AL_FIELD_MASK(26, 25) | AL_FIELD_MASK(31, 28), 0);

	if (ep_params->cap_flr_dis)
		al_reg_write32_masked(
			&regs->core_space.pcie_dev_cap_base,
			AL_BIT(28), 0);

	if (!ep_params->bar_params_valid)
		return 0;

	for (bar_idx = 0; bar_idx < 6;){  
		struct al_pcie_ep_bar_params *params = ep_params->bar_params + bar_idx;
		uint32_t mask = 0;
		uint32_t ctrl = 0;
		uint32_t __iomem *bar_addr = &regs->core_space.config_header[(AL_PCI_BASE_ADDRESS_0 >> 2) + bar_idx];

		if (params->enable) {
			uint64_t size = params->size;

			if (params->memory_64_bit) {
				struct al_pcie_ep_bar_params *next_params = params + 1;
				 
				if (bar_idx & 1)
					return -EINVAL;

				if (next_params->enable)
					return -EINVAL;

				if (params->memory_space)
					return -EINVAL;
			} else {
				if (size > AL_PCIE_MAX_32_MEMORY_BAR_SIZE)
					return -EINVAL;
				 
				if (params->memory_is_prefetchable)
					return -EINVAL;
			}

			if (params->memory_space) {
				if (size < AL_PCIE_MIN_MEMORY_BAR_SIZE) {
					al_err("PCIe %d: memory BAR %d: size (0x%llx) less that minimal allowed value\n",
						pcie_port->port_id, bar_idx, size);
					return -EINVAL;
				}
			} else {
				 
				if (params->memory_is_prefetchable)
					return -EINVAL;

				if (size < AL_PCIE_MIN_IO_BAR_SIZE) {
					al_err("PCIe %d: IO BAR %d: size (0x%llx) less that minimal allowed value\n",
						pcie_port->port_id, bar_idx, size);
					return -EINVAL;
				}
			}

			if (size & (size - 1)) {
				al_err("PCIe %d: BAR %d:size (0x%llx) must be "
					"power of 2\n",
					pcie_port->port_id, bar_idx, size);
				return -EINVAL;
			}

			mask = 1;  
			mask |= (params->size - 1) & 0xFFFFFFFF;

			al_reg_write32_dbi_cs2(bar_addr , mask);

			if (params->memory_space == AL_FALSE)
				ctrl = AL_PCI_BASE_ADDRESS_SPACE_IO;
			if (params->memory_64_bit)
				ctrl |= AL_PCI_BASE_ADDRESS_MEM_TYPE_64;
			if (params->memory_is_prefetchable)
				ctrl |= AL_PCI_BASE_ADDRESS_MEM_PREFETCH;
			al_reg_write32(bar_addr, ctrl);

			if (params->memory_64_bit) {
				mask = ((params->size - 1) >> 32) & 0xFFFFFFFF;
				al_reg_write32_dbi_cs2(bar_addr + 1 , mask);
			}

		} else {
			al_reg_write32_dbi_cs2(bar_addr , mask);
		}
		if (params->enable && params->memory_64_bit)
			bar_idx += 2;
		else
			bar_idx += 1;
	}
	if (ep_params->exp_bar_params.enable) {
		al_err("PCIe %d: Expansion BAR enable not supported\n", pcie_port->port_id );
		return -ENOSYS;
	}

	al_reg_write32(&regs->app.soc_int.mask_inta_leg_0, (1 << 21));

	if (pcie_port->rev_id == AL_PCIE_REV_ID_0) {
		uint32_t backup;

		backup = al_reg_read32(&regs->app.int_grp_a_m0.mask_a);
		al_reg_write32(&regs->app.soc_int.mask_msi_leg_0, (1 << 22));
		al_reg_write32(&regs->app.int_grp_a_m0.mask_a, backup);
	} else
		al_reg_write32(&regs->app.soc_int.mask_msi_leg_0, (1 << 22));

	return 0;
}

static void
al_pcie_port_features_config(struct al_pcie_port *pcie_port,
			      struct al_pcie_features *features)
{
	struct al_pcie_regs *regs = pcie_port->regs;

	al_assert(pcie_port->rev_id > AL_PCIE_REV_ID_0);

	al_reg_write32_masked(
		&regs->app.ctrl_gen.features,
		PCIE_W_CTRL_GEN_FEATURES_SATA_EP_MSI_FIX,
		features->sata_ep_msi_fix ?
		PCIE_W_CTRL_GEN_FEATURES_SATA_EP_MSI_FIX : 0);
}

static void
al_pcie_port_ib_hcrd_config(struct al_pcie_port *pcie_port)
{
	struct al_pcie_regs *regs = pcie_port->regs;

	al_reg_write32_masked(
		&regs->core_space.port_regs.vc0_posted_rcv_q_ctrl,
		RADM_PQ_HCRD_VC0_MASK,
		(pcie_port->nof_p_hdr - 1) << RADM_PQ_HCRD_VC0_SHIFT);

	al_reg_write32_masked(
		&regs->core_space.port_regs.vc0_non_posted_rcv_q_ctrl,
		RADM_NPQ_HCRD_VC0_MASK,
		(pcie_port->nof_np_hdr - 1) << RADM_NPQ_HCRD_VC0_SHIFT);
}

enum al_pcie_function_mode al_pcie_function_mode_get(
	struct al_pcie_port *pcie_port)
{
	struct al_pcie_regs *regs = pcie_port->regs;
	uint32_t reg, device_type;

	al_assert(pcie_port);

	reg = al_reg_read32(&regs->axi.pcie_global.conf);

	device_type = AL_REG_FIELD_GET(reg,
			PCIE_AXI_MISC_PCIE_GLOBAL_CONF_DEV_TYPE_MASK,
			PCIE_AXI_MISC_PCIE_GLOBAL_CONF_DEV_TYPE_SHIFT);

	switch (device_type) {
	case PCIE_AXI_MISC_PCIE_GLOBAL_CONF_DEV_TYPE_EP:
		return AL_PCIE_FUNCTION_MODE_EP;
	case PCIE_AXI_MISC_PCIE_GLOBAL_CONF_DEV_TYPE_RC:
		return AL_PCIE_FUNCTION_MODE_RC;
	default:
		al_err("PCIe %d: unknown device type (%d) in global conf "
			"register.\n",
			pcie_port->port_id, device_type);
	}
	return AL_PCIE_FUNCTION_MODE_UNKNOWN;
}

static void
al_pcie_port_ep_iov_setup(
	struct al_pcie_port		*pcie_port,
	struct al_pcie_ep_iov_params	*ep_iov_params)
{
	struct al_pcie_regs *regs = pcie_port->regs;
	enum al_pcie_function_mode func_mode =
		al_pcie_function_mode_get(pcie_port);

	al_assert(func_mode == AL_PCIE_FUNCTION_MODE_EP);

	al_reg_write32_masked(
		&regs->axi.pre_configuration.pcie_core_setup,
		PCIE_AXI_CORE_SETUP_SRIOV_ENABLE,
		((ep_iov_params->sriov_vfunc_en == AL_TRUE) ?
		 PCIE_AXI_CORE_SETUP_SRIOV_ENABLE : 0));

	al_reg_write32_masked(&regs->app.cfg_elbi.emulation,
		PCIE_W_CFG_EMUL_CTRL_FIX_CLIENT1_FMT_EN,
		((ep_iov_params->support_32b_address_in_iov == AL_TRUE) ?
		 PCIE_W_CFG_EMUL_CTRL_FIX_CLIENT1_FMT_EN : 0));
}

static al_bool al_pcie_check_link(struct al_pcie_port *pcie_port,
				uint8_t *ltssm_ret)
{
	struct al_pcie_regs *regs = (struct al_pcie_regs *)pcie_port->regs;
	uint32_t info_0;
	uint8_t	ltssm_state;

	info_0 = al_reg_read32(&regs->app.debug.info_0);

	ltssm_state = AL_REG_FIELD_GET(info_0,
			PCIE_W_DEBUG_INFO_0_LTSSM_STATE_MASK,
			PCIE_W_DEBUG_INFO_0_LTSSM_STATE_SHIFT);

	al_dbg("PCIe %d: Port Debug 0: 0x%08x. LTSSM state :0x%x\n",
		pcie_port->port_id, info_0, ltssm_state);

	if (ltssm_ret)
		*ltssm_ret = ltssm_state;

	if ((ltssm_state == AL_PCIE_LTSSM_STATE_L0) ||
			(ltssm_state == AL_PCIE_LTSSM_STATE_L0S))
		return AL_TRUE;
	return AL_FALSE;
}

int al_pcie_port_enable(
	struct al_pcie_port	*pcie_port,
	void __iomem		*pbs_reg_base)
{
	struct al_pbs_regs *pbs_regs = (struct al_pbs_regs *)pbs_reg_base;
	struct al_pcie_regs *regs = pcie_port->regs;
	unsigned int port_id = pcie_port->port_id;

	al_reg_write32_masked(
		&regs->axi.ordering.pos_cntl,
		PCIE_AXI_CORE_SETUP_ATS_CAP_DIS,
		PCIE_AXI_CORE_SETUP_ATS_CAP_DIS);

	al_reg_write32_masked(
		&pbs_regs->unit.pcie_conf_1,
		1 << (port_id + PBS_UNIT_PCIE_CONF_1_PCIE_EXIST_SHIFT),
		1 << (port_id + PBS_UNIT_PCIE_CONF_1_PCIE_EXIST_SHIFT));

	return 0;
}

void al_pcie_port_disable(
	struct al_pcie_port	*pcie_port,
	void __iomem		*pbs_reg_base)
{
	struct al_pbs_regs *pbs_regs = (struct al_pbs_regs *)pbs_reg_base;
	unsigned int port_id = pcie_port->port_id;

	al_reg_write32_masked(
		&pbs_regs->unit.pcie_conf_1,
		1 << (port_id + PBS_UNIT_PCIE_CONF_1_PCIE_EXIST_SHIFT),
		0);
}

int al_pcie_handle_init(struct al_pcie_port 	*pcie_port,
			 void __iomem		*pcie_reg_base,
			 unsigned int		port_id)
{
	pcie_port->regs = pcie_reg_base;
	pcie_port->port_id = port_id;
	pcie_port->write_to_read_only_enabled = AL_FALSE;
	pcie_port->max_lanes = 0;
	pcie_port->ib_hcrd_config_required = AL_FALSE;

	al_dbg("pcie port handle initialized. port id: %d. regs base %p\n",
	       port_id, pcie_reg_base);
	return 0;
}

int
al_pcie_port_func_mode_config(struct al_pcie_port *pcie_port,
			      enum al_pcie_function_mode mode)
{
	struct al_pcie_regs *regs = pcie_port->regs;
	uint32_t reg, device_type, new_device_type;

	reg = al_reg_read32(&regs->axi.pcie_global.conf);

	device_type = AL_REG_FIELD_GET(reg,
			PCIE_AXI_MISC_PCIE_GLOBAL_CONF_DEV_TYPE_MASK,
			PCIE_AXI_MISC_PCIE_GLOBAL_CONF_DEV_TYPE_SHIFT);
	if (mode == AL_PCIE_FUNCTION_MODE_EP)
		new_device_type = PCIE_AXI_MISC_PCIE_GLOBAL_CONF_DEV_TYPE_EP;
	else
		new_device_type = PCIE_AXI_MISC_PCIE_GLOBAL_CONF_DEV_TYPE_RC;

	if (new_device_type == device_type) {
		al_dbg("PCIe %d: function mode already set to %s",
		       pcie_port->port_id, (mode == AL_PCIE_FUNCTION_MODE_EP) ?
		       "EndPoint" : "Root Complex");
		return 0;
	}
	al_info("PCIe %d: set function mode to %s",
		pcie_port->port_id, (mode == AL_PCIE_FUNCTION_MODE_EP) ?
		"EndPoint" : "Root Complex");
	AL_REG_FIELD_SET(reg, PCIE_AXI_MISC_PCIE_GLOBAL_CONF_DEV_TYPE_MASK,
			 PCIE_AXI_MISC_PCIE_GLOBAL_CONF_DEV_TYPE_SHIFT,
			 new_device_type);

	al_reg_write32(&regs->axi.pcie_global.conf, reg);

	return 0;
}

void al_pcie_port_ib_hcrd_os_ob_reads_config(
	struct al_pcie_port *pcie_port,
	struct al_pcie_ib_hcrd_os_ob_reads_config *ib_hcrd_os_ob_reads_config)
{
	struct al_pcie_regs *regs = pcie_port->regs;

	al_assert(ib_hcrd_os_ob_reads_config->nof_np_hdr > 0);

	al_assert(ib_hcrd_os_ob_reads_config->nof_p_hdr > 0);

	al_assert(ib_hcrd_os_ob_reads_config->nof_cpl_hdr > 0);

	al_assert(
		(ib_hcrd_os_ob_reads_config->nof_cpl_hdr +
		ib_hcrd_os_ob_reads_config->nof_np_hdr +
		ib_hcrd_os_ob_reads_config->nof_p_hdr) == AL_PCIE_IB_HCRD_SUM);

	al_assert(
		(ib_hcrd_os_ob_reads_config->nof_outstanding_ob_reads *
		(unsigned int)AL_PCIE_NOF_CPL_HDR_NOF_OS_OB_READS_MIN_RATIO) <=
		ib_hcrd_os_ob_reads_config->nof_cpl_hdr);

	al_assert(
		ib_hcrd_os_ob_reads_config->nof_p_hdr <=
		AL_PCIE_NOF_P_NP_HDR_MAX);

	al_assert(
		ib_hcrd_os_ob_reads_config->nof_np_hdr <=
		AL_PCIE_NOF_P_NP_HDR_MAX);

	al_reg_write32_masked(
		&regs->axi.init_fc.cfg,
		PCIE_AXI_INIT_FC_CFG_NOF_P_HDR_MASK |
		PCIE_AXI_INIT_FC_CFG_NOF_NP_HDR_MASK |
		PCIE_AXI_INIT_FC_CFG_NOF_CPL_HDR_MASK,
		(ib_hcrd_os_ob_reads_config->nof_p_hdr <<
		 PCIE_AXI_INIT_FC_CFG_NOF_P_HDR_SHIFT) |
		(ib_hcrd_os_ob_reads_config->nof_np_hdr <<
		 PCIE_AXI_INIT_FC_CFG_NOF_NP_HDR_SHIFT) |
		(ib_hcrd_os_ob_reads_config->nof_cpl_hdr <<
		 PCIE_AXI_INIT_FC_CFG_NOF_CPL_HDR_SHIFT));

	al_reg_write32_masked(
		&regs->axi.pre_configuration.pcie_core_setup,
		PCIE_AXI_CORE_SETUP_NOF_READS_ONSLAVE_INTRF_PCIE_CORE_MASK,
		ib_hcrd_os_ob_reads_config->nof_outstanding_ob_reads <<
		PCIE_AXI_CORE_SETUP_NOF_READS_ONSLAVE_INTRF_PCIE_CORE_SHIFT);

	pcie_port->nof_np_hdr =	ib_hcrd_os_ob_reads_config->nof_np_hdr;
	pcie_port->nof_p_hdr =	ib_hcrd_os_ob_reads_config->nof_p_hdr;
	pcie_port->ib_hcrd_config_required = AL_TRUE;
}

enum al_pcie_function_mode
al_pcie_function_type_get(struct al_pcie_port *pcie_port)
{
	return al_pcie_function_mode_get(pcie_port);
}

int al_pcie_port_config(struct al_pcie_port *pcie_port,
			struct al_pcie_config_params *params)
{
	struct al_pcie_regs *regs = pcie_port->regs;
	int status = 0;

	al_assert(pcie_port);
	al_assert(params);

	al_dbg("PCIe %d: port config\n", pcie_port->port_id);

	pcie_port->rev_id = al_reg_read32(
		(uint32_t __iomem *)(&regs->core_space.config_header[0] +
		(PCI_CLASS_REVISION >> 2))) & 0xff;

	if (pcie_port->rev_id == AL_PCIE_REV_ID_0) {
		pcie_port->app_int_grp_a_base =
				(uint32_t __iomem *)&regs->app.int_grp_a_m0;
		pcie_port->app_int_grp_b_base =
				(uint32_t __iomem *)&regs->app.int_grp_b_m0;
	} else {
		pcie_port->app_int_grp_a_base =
				(uint32_t __iomem *)&regs->app.int_grp_a;
		pcie_port->app_int_grp_b_base =
				(uint32_t __iomem *)&regs->app.int_grp_b;
	}

	pcie_port->axi_int_grp_a_base =
		(uint32_t __iomem *)&regs->axi.int_grp_a;

	if (pcie_port->max_lanes == 0) {
		uint32_t global_conf = al_reg_read32(&regs->axi.pcie_global.conf);
		pcie_port->max_lanes = AL_REG_FIELD_GET(global_conf,
				PCIE_AXI_MISC_PCIE_GLOBAL_CONF_NOF_ACT_LANES_MASK,
				PCIE_AXI_MISC_PCIE_GLOBAL_CONF_DEV_TYPE_SHIFT);
	}

	if (params->link_params)
		status = al_pcie_port_link_config(pcie_port, params->link_params);
	if (status)
		goto done;

	status = al_pcie_port_snoop_config(pcie_port, params->enable_axi_snoop);
	if (status)
		goto done;

	al_pcie_port_ram_parity_int_config(pcie_port, params->enable_ram_parity_int);

	al_pcie_port_axi_parity_int_config(pcie_port, params->enable_axi_parity_int);

	if (params->lat_rply_timers)
		status = al_pcie_port_lat_rply_timers_config(pcie_port, params->lat_rply_timers);
	if (status)
		goto done;

	if (params->gen2_params)
		status = al_pcie_port_gen2_params_config(pcie_port, params->gen2_params);
	if (status)
		goto done;

	if (params->gen3_params)
		status = al_pcie_port_gen3_params_config(pcie_port, params->gen3_params);
	if (status)
		goto done;

	if (params->tl_credits)
		status = al_pcie_port_tl_credits_config(pcie_port, params->tl_credits);
	if (status)
		goto done;

	if (params->ep_params)
		status = al_pcie_port_ep_params_config(pcie_port, params->ep_params);
	if (status)
		goto done;

	if (params->features)
		al_pcie_port_features_config(pcie_port, params->features);

	if (pcie_port->ib_hcrd_config_required == AL_TRUE)
		al_pcie_port_ib_hcrd_config(pcie_port);

	if (params->ep_iov_params)
		al_pcie_port_ep_iov_setup(pcie_port, params->ep_iov_params);

	if (params->fast_link_mode) {
		al_reg_write32_masked(&regs->core_space.port_regs.port_link_ctrl,
			      1 << PCIE_PORT_LINK_CTRL_FAST_LINK_EN_SHIFT,
			      1 << PCIE_PORT_LINK_CTRL_FAST_LINK_EN_SHIFT);
	}

	if (params->enable_axi_slave_err_resp)
		al_reg_write32_masked(&regs->core_space.port_regs.axi_slave_err_resp,
				1 << PCIE_PORT_AXI_SLAVE_ERR_RESP_ALL_MAPPING_SHIFT,
				1 << PCIE_PORT_AXI_SLAVE_ERR_RESP_ALL_MAPPING_SHIFT);

	if (params->function_mode == AL_PCIE_FUNCTION_MODE_RC) {
		al_reg_write16_masked((uint16_t __iomem *)(&regs->core_space.config_header[0] + (0x4 >> 2)),
					0x7,  
					0x7);
		 
		al_reg_write32_masked((uint32_t __iomem *)(&regs->core_space.config_header[0] + (PCI_CLASS_REVISION >> 2)),
					0xFFFFFF00,
					0x06040000);
	}
done:
	al_dbg("PCIe %d: port config %s\n", pcie_port->port_id, status? "failed": "done");

	return status;
}

void al_pcie_app_req_retry_set(
	struct al_pcie_port	*pcie_port,
	al_bool			en)
{
	struct al_pcie_regs *regs = pcie_port->regs;

	al_reg_write32_masked(
		&regs->app.global_ctrl.pm_control,
		PCIE_W_GLOBAL_CTRL_PM_CONTROL_APP_REQ_RETRY_EN,
		(en == AL_TRUE) ?
		PCIE_W_GLOBAL_CTRL_PM_CONTROL_APP_REQ_RETRY_EN : 0);
}

int al_pcie_link_start(struct al_pcie_port *pcie_port)
{
	struct al_pcie_regs *regs = (struct al_pcie_regs *)pcie_port->regs;

	al_dbg("PCIe_%d: start port link.\n", pcie_port->port_id);

	al_reg_write32_masked(
			&regs->app.global_ctrl.port_init,
			PCIE_W_GLOBAL_CTRL_PORT_INIT_APP_LTSSM_EN,
			PCIE_W_GLOBAL_CTRL_PORT_INIT_APP_LTSSM_EN);

	return 0;
}

int al_pcie_link_stop(struct al_pcie_port *pcie_port)
{
	struct al_pcie_regs *regs = (struct al_pcie_regs *)pcie_port->regs;

	al_dbg("PCIe_%d: stop port link.\n", pcie_port->port_id);

	al_reg_write32_masked(
			&regs->app.global_ctrl.port_init,
			PCIE_W_GLOBAL_CTRL_PORT_INIT_APP_LTSSM_EN,
			~PCIE_W_GLOBAL_CTRL_PORT_INIT_APP_LTSSM_EN);

	return 0;
}

int al_pcie_link_up_wait(struct al_pcie_port *pcie_port, uint32_t timeout_ms)
{
	int wait_count = timeout_ms * AL_PCIE_LINKUP_WAIT_INTERVALS_PER_SEC;

	while (wait_count-- > 0)	{
		if (al_pcie_check_link(pcie_port, NULL)) {
			al_info("PCIe_%d: <<<<<<<<< Link up >>>>>>>>>\n", pcie_port->port_id);
			return 0;
		} else
			al_dbg("PCIe_%d: No link up, %d attempts remaining\n",
				pcie_port->port_id, wait_count);

		al_udelay(AL_PCIE_LINKUP_WAIT_INTERVAL);
	}
	al_info("PCIE_%d: link is not established in time\n",
				pcie_port->port_id);

	return -ETIME;
}

int al_pcie_link_status(struct al_pcie_port *pcie_port,
			struct al_pcie_link_status *status)
{
	struct al_pcie_regs *regs = pcie_port->regs;
	uint16_t	pcie_lnksta;

	al_assert(status);

	status->link_up = al_pcie_check_link(pcie_port, &status->ltssm_state);

	if (!status->link_up) {
		status->speed = AL_PCIE_LINK_SPEED_DEFAULT;
		status->lanes = 0;
		return 0;
	}

	pcie_lnksta = al_reg_read16((uint16_t __iomem *)&regs->core_space.pcie_cap_base + (AL_PCI_EXP_LNKSTA >> 1));

	switch(pcie_lnksta & AL_PCI_EXP_LNKSTA_CLS) {
		case AL_PCI_EXP_LNKSTA_CLS_2_5GB:
			status->speed = AL_PCIE_LINK_SPEED_GEN1;
			break;
		case AL_PCI_EXP_LNKSTA_CLS_5_0GB:
			status->speed = AL_PCIE_LINK_SPEED_GEN2;
			break;
		case AL_PCI_EXP_LNKSTA_CLS_8_0GB:
			status->speed = AL_PCIE_LINK_SPEED_GEN3;
			break;
		default:
			status->speed = AL_PCIE_LINK_SPEED_DEFAULT;
			al_err("PCIe %d: unknown link speed indication. PCIE LINK STATUS %x\n",
				pcie_port->port_id, pcie_lnksta);
	}
	status->lanes = (pcie_lnksta & AL_PCI_EXP_LNKSTA_NLW) >> AL_PCI_EXP_LNKSTA_NLW_SHIFT;
	al_info("PCIe %d: Link up. speed gen%d negotiated width %d\n",
		pcie_port->port_id, status->speed, status->lanes);

	return 0;
}

int al_pcie_link_hot_reset(struct al_pcie_port *pcie_port)
{
	al_err("PCIe %d: link hot reset not implemented\n",
		pcie_port->port_id);

	return -ENOSYS;
}

int al_pcie_link_change_speed(struct al_pcie_port *pcie_port,
			      enum al_pcie_link_speed new_speed __attribute__((__unused__)))
{
	al_err("PCIe %d: link change speed not implemented\n",
		pcie_port->port_id);

	return -ENOSYS;
}

int al_pcie_link_change_width(struct al_pcie_port *pcie_port,
			      uint8_t width __attribute__((__unused__)))
{
	al_err("PCIe %d: link change width not implemented\n",
		pcie_port->port_id);

	return -ENOSYS;
}

int al_pcie_target_bus_set(struct al_pcie_port *pcie_port,
			   uint8_t target_bus,
			   uint8_t mask_target_bus)
{
	struct al_pcie_regs *regs = (struct al_pcie_regs *)pcie_port->regs;
	uint32_t reg;

	reg = al_reg_read32(&regs->axi.ob_ctrl.cfg_target_bus);
	AL_REG_FIELD_SET(reg, PCIE_AXI_MISC_OB_CTRL_CFG_TARGET_BUS_MASK_MASK,
			PCIE_AXI_MISC_OB_CTRL_CFG_TARGET_BUS_MASK_SHIFT,
			mask_target_bus);
	AL_REG_FIELD_SET(reg, PCIE_AXI_MISC_OB_CTRL_CFG_TARGET_BUS_BUSNUM_MASK,
			PCIE_AXI_MISC_OB_CTRL_CFG_TARGET_BUS_BUSNUM_SHIFT,
			target_bus);
	al_reg_write32(&regs->axi.ob_ctrl.cfg_target_bus, reg);
	return 0;
}

int al_pcie_target_bus_get(struct al_pcie_port *pcie_port,
			   uint8_t *target_bus,
			   uint8_t *mask_target_bus)
{
	struct al_pcie_regs *regs = (struct al_pcie_regs *)pcie_port->regs;
	uint32_t reg;

	al_assert(target_bus);
	al_assert(mask_target_bus);

	reg = al_reg_read32(&regs->axi.ob_ctrl.cfg_target_bus);

	*mask_target_bus = AL_REG_FIELD_GET(reg,
				PCIE_AXI_MISC_OB_CTRL_CFG_TARGET_BUS_MASK_MASK,
				PCIE_AXI_MISC_OB_CTRL_CFG_TARGET_BUS_MASK_SHIFT);
	*target_bus = AL_REG_FIELD_GET(reg,
			PCIE_AXI_MISC_OB_CTRL_CFG_TARGET_BUS_BUSNUM_MASK,
			PCIE_AXI_MISC_OB_CTRL_CFG_TARGET_BUS_BUSNUM_SHIFT);
	return 0;
}

int al_pcie_secondary_bus_set(struct al_pcie_port *pcie_port, uint8_t secbus)
{
	struct al_pcie_regs *regs = pcie_port->regs;

	uint32_t secbus_val = (secbus <<
			PCIE_AXI_MISC_OB_CTRL_CFG_CONTROL_SEC_BUS_SHIFT);

	al_reg_write32_masked(
		&regs->axi.ob_ctrl.cfg_control,
		PCIE_AXI_MISC_OB_CTRL_CFG_CONTROL_SEC_BUS_MASK,
		secbus_val);
	return 0;
}

int al_pcie_subordinary_bus_set(struct al_pcie_port *pcie_port, uint8_t subbus)
{
	struct al_pcie_regs *regs = pcie_port->regs;

	uint32_t subbus_val = (subbus <<
			PCIE_AXI_MISC_OB_CTRL_CFG_CONTROL_SUBBUS_SHIFT);

	al_reg_write32_masked(
		&regs->axi.ob_ctrl.cfg_control,
		PCIE_AXI_MISC_OB_CTRL_CFG_CONTROL_SUBBUS_MASK,
		subbus_val);
	return 0;
}

int al_pcie_config_space_get(struct al_pcie_port *pcie_port,
			     uint8_t __iomem **addr)
{
	struct al_pcie_regs *regs = pcie_port->regs;

	*addr = (uint8_t __iomem *)&regs->core_space.config_header[0];
	return 0;
}

uint32_t al_pcie_cfg_emul_local_cfg_space_read(
	struct al_pcie_port	*pcie_port,
	unsigned int	reg_offset)
{
	struct al_pcie_regs *regs = pcie_port->regs;
	uint32_t data;

	data = al_reg_read32(&regs->core_space.config_header[reg_offset]);

	return data;
}

void al_pcie_cfg_emul_local_cfg_space_write(
	struct al_pcie_port	*pcie_port,
	unsigned int		reg_offset,
	uint32_t		data,
	al_bool			ro)
{
	struct al_pcie_regs *regs = pcie_port->regs;
	uint32_t *offset = (ro == AL_FALSE) ?
		(&regs->core_space.config_header[reg_offset]) :
		(&regs->core_space.config_header[reg_offset] + (0x1000 >> 2));

	al_reg_write32(offset, data);
}

#ifdef CONFIG_SYNO_ALPINE_V2_5_3
void al_pcie_axi_io_config(struct al_pcie_port	*pcie_port,
			   al_phys_addr_t	start,
			   al_phys_addr_t	end)
{
	struct al_pcie_regs *regs = pcie_port->regs;

	al_reg_write32(&regs->axi.ob_ctrl.io_start_h,
			(uint32_t)((start >> 32) & 0xFFFFFFFF));

	al_reg_write32(&regs->axi.ob_ctrl.io_start_l,
			(uint32_t)(start & 0xFFFFFFFF));

	al_reg_write32(&regs->axi.ob_ctrl.io_limit_h,
			(uint32_t)((end >> 32) & 0xFFFFFFFF));

	al_reg_write32(&regs->axi.ob_ctrl.io_limit_l,
			(uint32_t)(end & 0xFFFFFFFF));

	al_reg_write32_masked(&regs->axi.ctrl.slv_ctl,
			      PCIE_AXI_CTRL_SLV_CTRL_IO_BAR_EN,
			      PCIE_AXI_CTRL_SLV_CTRL_IO_BAR_EN);
}
#endif

int al_pcie_atu_region_set(struct al_pcie_port *pcie_port, struct al_pcie_atu_region *atu_region)
{
	struct al_pcie_regs *regs = pcie_port->regs;
	uint32_t reg = 0;
	uint32_t limit_reg_val;

	AL_REG_FIELD_SET(reg, 0xF, 0, atu_region->index);
	AL_REG_BIT_VAL_SET(reg, 31, atu_region->direction);
	al_reg_write32(&regs->core_space.port_regs.iatu.index, reg);

	al_reg_write32(&regs->core_space.port_regs.iatu.lower_base_addr,
			(uint32_t)(atu_region->base_addr & 0xFFFFFFFF));
	al_reg_write32(&regs->core_space.port_regs.iatu.upper_base_addr,
			(uint32_t)((atu_region->base_addr >> 32)& 0xFFFFFFFF));
	al_reg_write32(&regs->core_space.port_regs.iatu.lower_target_addr,
			(uint32_t)(atu_region->target_addr & 0xFFFFFFFF));
	al_reg_write32(&regs->core_space.port_regs.iatu.upper_target_addr,
			(uint32_t)((atu_region->target_addr >> 32)& 0xFFFFFFFF));

	if (pcie_port->rev_id > AL_PCIE_REV_ID_0) {
		uint32_t *limit_ext_reg =
			(atu_region->direction == al_pcie_atu_dir_outbound) ?
			&regs->app.atu.out_mask_pair[atu_region->index / 2] :
			&regs->app.atu.in_mask_pair[atu_region->index / 2];
		uint32_t limit_ext_reg_mask =
			(atu_region->index % 2) ?
			PCIE_W_ATU_MASK_EVEN_ODD_ATU_MASK_40_32_ODD_MASK :
			PCIE_W_ATU_MASK_EVEN_ODD_ATU_MASK_40_32_EVEN_MASK;
		unsigned int limit_ext_reg_shift =
			(atu_region->index % 2) ?
			PCIE_W_ATU_MASK_EVEN_ODD_ATU_MASK_40_32_ODD_SHIFT :
			PCIE_W_ATU_MASK_EVEN_ODD_ATU_MASK_40_32_EVEN_SHIFT;
		uint64_t limit_sz =
			atu_region->limit - atu_region->base_addr;
		uint64_t limit_sz_msk = limit_sz - 1;
		uint32_t limit_ext_reg_val = (uint32_t)(((limit_sz_msk) >>
				32) & 0xFFFFFFFF);

		if (limit_ext_reg_val) {
			limit_reg_val =	(uint32_t)((limit_sz_msk) & 0xFFFFFFFF);
			al_assert(limit_reg_val == 0xFFFFFFFF);
		} else {
			limit_reg_val = (uint32_t)(atu_region->limit &
				0xFFFFFFFF);
		}

		al_reg_write32_masked(
			limit_ext_reg,
			limit_ext_reg_mask,
			limit_ext_reg_val << limit_ext_reg_shift);
	} else {
		limit_reg_val = (uint32_t)(atu_region->limit & 0xFFFFFFFF);
	}

	al_reg_write32(&regs->core_space.port_regs.iatu.limit_addr,
		limit_reg_val);

	reg = 0;
	AL_REG_FIELD_SET(reg, 0x1F, 0, atu_region->tlp_type);
	AL_REG_FIELD_SET(reg, 0x3 << 9, 9, atu_region->attr);
	al_reg_write32(&regs->core_space.port_regs.iatu.cr1, reg);

	reg = 0;
	AL_REG_FIELD_SET(reg, 0xFF, 0, atu_region->msg_code);
	AL_REG_FIELD_SET(reg, 0x700, 8, atu_region->bar_number);
	AL_REG_BIT_VAL_SET(reg, 16, atu_region->enable_attr_match_mode == AL_TRUE);
	AL_REG_BIT_VAL_SET(reg, 21, atu_region->enable_msg_match_mode == AL_TRUE);
	AL_REG_BIT_VAL_SET(reg, 28, atu_region->cfg_shift_mode == AL_TRUE);
	AL_REG_BIT_VAL_SET(reg, 29, atu_region->invert_matching == AL_TRUE);
	if (atu_region->tlp_type == AL_PCIE_TLP_TYPE_MEM || atu_region->tlp_type == AL_PCIE_TLP_TYPE_IO)
		AL_REG_BIT_VAL_SET(reg, 30, !!atu_region->match_mode);
	AL_REG_BIT_VAL_SET(reg, 31, !!atu_region->enable);

	al_reg_write32(&regs->core_space.port_regs.iatu.cr2, reg);

	return 0;
}

int al_pcie_legacy_int_gen(struct al_pcie_port *pcie_port, al_bool assert,
			   enum al_pcie_legacy_int_type type)
{
	struct al_pcie_regs *regs = pcie_port->regs;
	uint32_t reg;

	al_assert(type == AL_PCIE_LEGACY_INTA);  
	reg = al_reg_read32(&regs->app.global_ctrl.events_gen);
	AL_REG_BIT_VAL_SET(reg, 3, !!assert);
	al_reg_write32(&regs->app.global_ctrl.events_gen, reg);

	return 0;
}

int al_pcie_msi_int_gen(struct al_pcie_port *pcie_port, uint8_t vector)
{
	struct al_pcie_regs *regs = pcie_port->regs;
	uint32_t reg;

	reg = al_reg_read32(&regs->app.global_ctrl.events_gen);
	AL_REG_BIT_CLEAR(reg, 4);
	AL_REG_FIELD_SET(reg,
			PCIE_W_GLOBAL_CTRL_EVENTS_GEN_MSI_VECTOR_MASK,
			PCIE_W_GLOBAL_CTRL_EVENTS_GEN_MSI_VECTOR_SHIFT,
			vector);
	al_reg_write32(&regs->app.global_ctrl.events_gen, reg);
	 
	AL_REG_BIT_SET(reg, 4);
	al_reg_write32(&regs->app.global_ctrl.events_gen, reg);

	return 0;
}

int al_pcie_local_pipe_loopback_enter(struct al_pcie_port *pcie_port)
{
	struct al_pcie_regs *regs = pcie_port->regs;

	al_dbg("PCIe %d: Enter LOCAL PIPE Loopback mode", pcie_port->port_id);

	al_reg_write32_masked(&regs->core_space.port_regs.pipe_loopback_ctrl,
			      1 << PCIE_PORT_PIPE_LOOPBACK_CTRL_PIPE_LB_EN_SHIFT,
			      1 << PCIE_PORT_PIPE_LOOPBACK_CTRL_PIPE_LB_EN_SHIFT);

	al_reg_write32_masked(&regs->core_space.port_regs.port_link_ctrl,
			      1 << PCIE_PORT_LINK_CTRL_LB_EN_SHIFT,
			      1 << PCIE_PORT_LINK_CTRL_LB_EN_SHIFT);

	return 0;
}

int al_pcie_local_pipe_loopback_exit(struct al_pcie_port *pcie_port)
{
	struct al_pcie_regs *regs = pcie_port->regs;

	al_dbg("PCIe %d: Exit LOCAL PIPE Loopback mode", pcie_port->port_id);

	al_reg_write32_masked(&regs->core_space.port_regs.pipe_loopback_ctrl,
			      1 << PCIE_PORT_PIPE_LOOPBACK_CTRL_PIPE_LB_EN_SHIFT,
			      0);

	al_reg_write32_masked(&regs->core_space.port_regs.port_link_ctrl,
			      1 << PCIE_PORT_LINK_CTRL_LB_EN_SHIFT,
			      0);
	return 0;
}

int al_pcie_remote_loopback_enter(struct al_pcie_port *pcie_port)
{
	struct al_pcie_regs *regs = pcie_port->regs;

	al_dbg("PCIe %d: Enter REMOTE Loopback mode", pcie_port->port_id);

	al_reg_write32_masked(&regs->core_space.port_regs.port_link_ctrl,
			      1 << PCIE_PORT_PIPE_LOOPBACK_CTRL_PIPE_LB_EN_SHIFT,
			      1 << PCIE_PORT_PIPE_LOOPBACK_CTRL_PIPE_LB_EN_SHIFT);

	return 0;
}

int al_pcie_remote_loopback_exit(struct al_pcie_port *pcie_port)
{
	struct al_pcie_regs *regs = pcie_port->regs;

	al_dbg("PCIe %d: Exit REMOTE Loopback mode", pcie_port->port_id);

	al_reg_write32_masked(&regs->core_space.port_regs.port_link_ctrl,
			      1 << PCIE_PORT_LINK_CTRL_LB_EN_SHIFT,
			      0);
	return 0;
}
