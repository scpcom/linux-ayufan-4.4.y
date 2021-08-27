 
#ifndef __AL_PCIE_HAL_AXI_REG_H__
#define __AL_PCIE_HAL_AXI_REG_H__

#ifdef __cplusplus
extern "C" {
#endif
 
struct al_pcie_axi_ctrl {
	 
	uint32_t global;
	uint32_t rsrvd_0;
	 
	uint32_t master_bctl;
	 
	uint32_t master_rctl;
	 
	uint32_t master_ctl;
	 
	uint32_t master_arctl;
	 
	uint32_t master_awctl;
	 
	uint32_t slave_rctl;
	 
	uint32_t slv_wctl;
	 
	uint32_t slv_ctl;
	 
	uint32_t dbi_ctl;
	 
	uint32_t vmid_mask;
	uint32_t rsrvd[4];
};
struct al_pcie_axi_ob_ctrl {
	 
	uint32_t cfg_target_bus;
	 
	uint32_t cfg_control;
	 
	uint32_t io_start_l;
	 
	uint32_t io_start_h;
	 
	uint32_t io_limit_l;
	 
	uint32_t io_limit_h;
	 
	uint32_t msg_start_l;
	 
	uint32_t msg_start_h;
	 
	uint32_t msg_limit_l;
	 
	uint32_t msg_limit_h;
	uint32_t rsrvd[6];
};
struct al_pcie_axi_msg {
	 
	uint32_t addr_high;
	 
	uint32_t addr_low;
	 
	uint32_t type;
};
struct al_pcie_axi_pcie_status {
	 
	uint32_t debug;
};
struct al_pcie_axi_rd_parity {
	 
	uint32_t log_high;
	 
	uint32_t log_low;
};
struct al_pcie_axi_rd_cmpl {
	 
	uint32_t cmpl_log_high;
	 
	uint32_t cmpl_log_low;
};
struct al_pcie_axi_rd_to {
	 
	uint32_t to_log_high;
	 
	uint32_t to_log_low;
};
struct al_pcie_axi_wr_cmpl {
	 
	uint32_t wr_cmpl_log_high;
	 
	uint32_t wr_cmpl_log_low;
};
struct al_pcie_axi_wr_to {
	 
	uint32_t wr_to_log_high;
	 
	uint32_t wr_to_log_low;
};
struct al_pcie_axi_pcie_global {
	 
	uint32_t conf;
};
struct al_pcie_axi_status {
	 
	uint32_t lane0;
	 
	uint32_t lane1;
	 
	uint32_t lane2;
	 
	uint32_t lane3;
};
struct al_pcie_axi_conf {
	 
	uint32_t zero_lane0;
	 
	uint32_t zero_lane1;
	 
	uint32_t zero_lane2;
	 
	uint32_t zero_lane3;
	 
	uint32_t one_lane0;
	 
	uint32_t one_lane1;
	 
	uint32_t one_lane2;
	 
	uint32_t one_lane3;
};
struct al_pcie_axi_parity {
	 
	uint32_t en_axi;
	 
	uint32_t status_axi;
};
struct al_pcie_axi_pos_logged {
	 
	uint32_t error_low;
	 
	uint32_t error_high;
};
struct al_pcie_axi_ordering {
	 
	uint32_t pos_cntl;
};
struct al_pcie_axi_link_down {
	 
	uint32_t reset_extend;
};
struct al_pcie_axi_pre_configuration {
	 
	uint32_t pcie_core_setup;
};
struct al_pcie_axi_init_fc {
	 
	uint32_t cfg;
};
struct al_pcie_axi_int_grp_a_axi {
	 
	uint32_t cause;
	uint32_t rsrvd_0;
	 
	uint32_t cause_set;
	uint32_t rsrvd_1;
	 
	uint32_t mask;
	uint32_t rsrvd_2;
	 
	uint32_t mask_clear;
	uint32_t rsrvd_3;
	 
	uint32_t status;
	uint32_t rsrvd_4;
	 
	uint32_t control;
	uint32_t rsrvd_5;
	 
	uint32_t abort_mask;
	uint32_t rsrvd_6;
	 
	uint32_t log_mask;
	uint32_t rsrvd;
};

struct al_pcie_axi_regs {
	struct al_pcie_axi_ctrl ctrl;      
	struct al_pcie_axi_ob_ctrl ob_ctrl;  
	uint32_t rsrvd_0[4];
	struct al_pcie_axi_msg msg;                      
	struct al_pcie_axi_pcie_status pcie_status;      
	struct al_pcie_axi_rd_parity rd_parity;          
	struct al_pcie_axi_rd_cmpl rd_cmpl;              
	struct al_pcie_axi_rd_to rd_to;                  
	struct al_pcie_axi_wr_cmpl wr_cmpl;              
	struct al_pcie_axi_wr_to wr_to;                  
	struct al_pcie_axi_pcie_global pcie_global;      
	struct al_pcie_axi_status status;                
	struct al_pcie_axi_conf conf;                    
	struct al_pcie_axi_parity parity;                
	struct al_pcie_axi_pos_logged pos_logged;        
	struct al_pcie_axi_ordering ordering;            
	struct al_pcie_axi_link_down link_down;          
	struct al_pcie_axi_pre_configuration pre_configuration;  
	struct al_pcie_axi_init_fc init_fc;              
	uint32_t rsrvd_1[57];
	struct al_pcie_axi_int_grp_a_axi int_grp_a;  
};

#define PCIE_AXI_CTRL_GLOBAL_CPL_AFTER_P_ORDER_DIS (1 << 0)
 
#define PCIE_AXI_CTRL_GLOBAL_CPU_CPL_ONLY_EN (1 << 1)
 
#define PCIE_AXI_CTRL_GLOBAL_BLOCK_PCIE_SLAVE_EN (1 << 2)
 
#define PCIE_AXI_CTRL_GLOBAL_WAIT_SLV_FLUSH_EN (1 << 3)
 
#define PCIE_AXI_CTRL_GLOBAL_MEM_BAR_MAP_TO_ERR (1 << 4)
 
#define PCIE_AXI_CTRL_GLOBAL_WAIT_DBI_FLUSH_EN (1 << 5)
 
#define PCIE_AXI_CTRL_GLOBAL_PARITY_CALC_EN_MSTR (1 << 16)
 
#define PCIE_AXI_CTRL_GLOBAL_PARITY_ERR_EN_RD (1 << 17)
 
#define PCIE_AXI_CTRL_GLOBAL_PARITY_CALC_EN_SLV (1 << 18)
 
#define PCIE_AXI_CTRL_GLOBAL_PARITY_ERR_EN_WR (1 << 19)
 
#define PCIE_AXI_CTRL_GLOBAL_ERROR_TRACK_DIS (1 << 20)

#define PCIE_AXI_CTRL_MASTER_ARCTL_OVR_ARCACHE (1 << 0)
 
#define PCIE_AXI_CTRL_MASTER_ARCTL_ARACHE_VA_MASK 0x0000001E
#define PCIE_AXI_CTRL_MASTER_ARCTL_ARACHE_VA_SHIFT 1
 
#define PCIE_AXI_CTRL_MASTER_ARCTL_ARPROT_OVR (1 << 5)
 
#define PCIE_AXI_CTRL_MASTER_ARCTL_ARPROT_VALUE_MASK 0x000001C0
#define PCIE_AXI_CTRL_MASTER_ARCTL_ARPROT_VALUE_SHIFT 6
 
#define PCIE_AXI_CTRL_MASTER_ARCTL_VMID_VAL_MASK 0x01FFFE00
#define PCIE_AXI_CTRL_MASTER_ARCTL_VMID_VAL_SHIFT 9
 
#define PCIE_AXI_CTRL_MASTER_ARCTL_IPA_VAL (1 << 25)
 
#define PCIE_AXI_CTRL_MASTER_ARCTL_OVR_SNOOP (1 << 26)
 
#define PCIE_AXI_CTRL_MASTER_ARCTL_SNOOP (1 << 27)
 
#define PCIE_AXI_CTRL_MASTER_ARCTL_ARQOS_MASK 0xF0000000
#define PCIE_AXI_CTRL_MASTER_ARCTL_ARQOS_SHIFT 28

#define PCIE_AXI_CTRL_MASTER_AWCTL_OVR_ARCACHE (1 << 0)
 
#define PCIE_AXI_CTRL_MASTER_AWCTL_AWACHE_VA_MASK 0x0000001E
#define PCIE_AXI_CTRL_MASTER_AWCTL_AWACHE_VA_SHIFT 1
 
#define PCIE_AXI_CTRL_MASTER_AWCTL_AWPROT_OVR (1 << 5)
 
#define PCIE_AXI_CTRL_MASTER_AWCTL_AWPROT_VALUE_MASK 0x000001C0
#define PCIE_AXI_CTRL_MASTER_AWCTL_AWPROT_VALUE_SHIFT 6
 
#define PCIE_AXI_CTRL_MASTER_AWCTL_VMID_VAL_MASK 0x01FFFE00
#define PCIE_AXI_CTRL_MASTER_AWCTL_VMID_VAL_SHIFT 9
 
#define PCIE_AXI_CTRL_MASTER_AWCTL_IPA_VAL (1 << 25)
 
#define PCIE_AXI_CTRL_MASTER_AWCTL_OVR_SNOOP (1 << 26)
 
#define PCIE_AXI_CTRL_MASTER_AWCTL_SNOOP (1 << 27)
 
#define PCIE_AXI_CTRL_MASTER_AWCTL_AWQOS_MASK 0xF0000000
#define PCIE_AXI_CTRL_MASTER_AWCTL_AWQOS_SHIFT 28

#ifdef CONFIG_SYNO_ALPINE_V2_5_3
 
#define PCIE_AXI_CTRL_SLV_CTRL_IO_BAR_EN	(1 << 6)
#endif

#define PCIE_AXI_MISC_OB_CTRL_CFG_TARGET_BUS_MASK_MASK 0x000000FF
#define PCIE_AXI_MISC_OB_CTRL_CFG_TARGET_BUS_MASK_SHIFT 0
 
#define PCIE_AXI_MISC_OB_CTRL_CFG_TARGET_BUS_BUSNUM_MASK 0x0000FF00
#define PCIE_AXI_MISC_OB_CTRL_CFG_TARGET_BUS_BUSNUM_SHIFT 8

#define PCIE_AXI_MISC_OB_CTRL_CFG_CONTROL_PBUS_MASK 0x000000FF
#define PCIE_AXI_MISC_OB_CTRL_CFG_CONTROL_PBUS_SHIFT 0
 
#define PCIE_AXI_MISC_OB_CTRL_CFG_CONTROL_SUBBUS_MASK 0x0000FF00
#define PCIE_AXI_MISC_OB_CTRL_CFG_CONTROL_SUBBUS_SHIFT 8
 
#define PCIE_AXI_MISC_OB_CTRL_CFG_CONTROL_SEC_BUS_MASK 0x00FF0000
#define PCIE_AXI_MISC_OB_CTRL_CFG_CONTROL_SEC_BUS_SHIFT 16
 
#define PCIE_AXI_MISC_OB_CTRL_CFG_CONTROL_IATU_EN (1 << 31)

#define PCIE_AXI_MISC_OB_CTRL_IO_START_H_ADDR_MASK 0x000003FF
#define PCIE_AXI_MISC_OB_CTRL_IO_START_H_ADDR_SHIFT 0

#define PCIE_AXI_MISC_OB_CTRL_IO_LIMIT_H_ADDR_MASK 0x000003FF
#define PCIE_AXI_MISC_OB_CTRL_IO_LIMIT_H_ADDR_SHIFT 0

#define PCIE_AXI_MISC_OB_CTRL_MSG_START_H_ADDR_MASK 0x000003FF
#define PCIE_AXI_MISC_OB_CTRL_MSG_START_H_ADDR_SHIFT 0

#define PCIE_AXI_MISC_OB_CTRL_MSG_LIMIT_H_ADDR_MASK 0x000003FF
#define PCIE_AXI_MISC_OB_CTRL_MSG_LIMIT_H_ADDR_SHIFT 0

#define PCIE_AXI_MISC_MSG_TYPE_TYPE_MASK 0x00FFFFFF
#define PCIE_AXI_MISC_MSG_TYPE_TYPE_SHIFT 0
 
#define PCIE_AXI_MISC_MSG_TYPE_RSRVD_MASK 0xFF000000
#define PCIE_AXI_MISC_MSG_TYPE_RSRVD_SHIFT 24

#define PCIE_AXI_MISC_PCIE_STATUS_DEBUG_AXI_BRIDGE_RESET (1 << 0)
 
#define PCIE_AXI_MISC_PCIE_STATUS_DEBUG_CORE_RESET (1 << 1)
 
#define PCIE_AXI_MISC_PCIE_STATUS_DEBUG_SB_FLUSH_OB_STATUS (1 << 2)
 
#define PCIE_AXI_MISC_PCIE_STATUS_DEBUG_SB_MAP_TO_ERR (1 << 3)
 
#define PCIE_AXI_MISC_PCIE_STATUS_DEBUG_CORE_CLK_GATE_OFF (1 << 4)
 
#define PCIE_AXI_MISC_PCIE_STATUS_DEBUG_RSRVD_MASK 0xFFFFFFE0
#define PCIE_AXI_MISC_PCIE_STATUS_DEBUG_RSRVD_SHIFT 5

#define PCIE_AXI_MISC_PCIE_GLOBAL_CONF_DEV_TYPE_MASK 0x0000000F
#define PCIE_AXI_MISC_PCIE_GLOBAL_CONF_DEV_TYPE_SHIFT 0
 
#define PCIE_AXI_MISC_PCIE_GLOBAL_CONF_NOF_ACT_LANES_MASK 0x000000F0
#define PCIE_AXI_MISC_PCIE_GLOBAL_CONF_NOF_ACT_LANES_SHIFT 4
 
#define PCIE_AXI_MISC_PCIE_GLOBAL_CONF_MEM_SHUTDOWN 0x100
#define PCIE_AXI_MISC_PCIE_GLOBAL_CONF_RESERVED_MASK 0xFFFFFE00
#define PCIE_AXI_MISC_PCIE_GLOBAL_CONF_RESERVED_SHIFT 9

#define PCIE_AXI_MISC_ZERO_LANEX_PHY_MAC_LOCAL_FS_MASK		0x0000003f
#define PCIE_AXI_MISC_ZERO_LANEX_PHY_MAC_LOCAL_FS_SHIFT	0
 
#define PCIE_AXI_MISC_ZERO_LANEX_PHY_MAC_LOCAL_LF_MASK		0x00000fc0
#define PCIE_AXI_MISC_ZERO_LANEX_PHY_MAC_LOCAL_LF_SHIFT	6

#define PCIE_AXI_POS_ORDER_AXI_POS_BYPASS (1 << 0)
 
#define PCIE_AXI_POS_ORDER_AXI_POS_CLEAR (1 << 1)
 
#define PCIE_AXI_POS_ORDER_AXI_POS_RSO_ENABLE (1 << 2)
 
#define PCIE_AXI_POS_ORDER_AXI_DW_RD_FLUSH_WR (1 << 3)
 
#define PCIE_AXI_POS_ORDER_RD_CMPL_AFTER_WR_SUPPORT_RD_INTERLV (1 << 4)
 
#define PCIE_AXI_POS_ORDER_BYPASS_CMPL_AFTER_WR_FIX (1 << 5)
 
#define PCIE_AXI_POS_ORDER_EP_CMPL_AFTER_WR_DIS (1 << 6)
 
#define PCIE_AXI_POS_ORDER_EP_CMPL_AFTER_WR_SUPPORT_INTERLV_DIS (1 << 7)
 
#define PCIE_AXI_CORE_SETUP_ATS_CAP_DIS	AL_BIT(13)

#define PCIE_AXI_CORE_SETUP_DELAY_MAC_PHY_RATE_MASK 0x000000FF
#define PCIE_AXI_CORE_SETUP_DELAY_MAC_PHY_RATE_SHIFT 0
 
#define PCIE_AXI_CORE_SETUP_NOF_READS_ONSLAVE_INTRF_PCIE_CORE_MASK 0x0000FF00
#define PCIE_AXI_CORE_SETUP_NOF_READS_ONSLAVE_INTRF_PCIE_CORE_SHIFT 8
 
#define PCIE_AXI_CORE_SETUP_SRIOV_ENABLE AL_BIT(16)

#define PCIE_AXI_INIT_FC_CFG_NOF_P_HDR_MASK 0x0000007F
#define PCIE_AXI_INIT_FC_CFG_NOF_P_HDR_SHIFT 0
 
#define PCIE_AXI_INIT_FC_CFG_NOF_NP_HDR_MASK 0x00003F80
#define PCIE_AXI_INIT_FC_CFG_NOF_NP_HDR_SHIFT 7
 
#define PCIE_AXI_INIT_FC_CFG_NOF_CPL_HDR_MASK 0x001FC000
#define PCIE_AXI_INIT_FC_CFG_NOF_CPL_HDR_SHIFT 14

#define PCIE_AXI_INIT_FC_CFG_RSRVD_MASK 0xFFE00000
#define PCIE_AXI_INIT_FC_CFG_RSRVD_SHIFT 21

#define PCIE_AXI_INT_GRP_A_CAUSE_GM_COMPOSER_LOOKUP_ERR (1 << 0)
 
#define PCIE_AXI_INT_GRP_A_CAUSE_PARITY_ERR_DATA_PATH_RD (1 << 2)
 
#define PCIE_AXI_INT_GRP_A_CAUSE_PARITY_ERR_OUT_ADDR_RD (1 << 3)
 
#define PCIE_AXI_INT_GRP_A_CAUSE_PARITY_ERR_OUT_ADDR_WR (1 << 4)
 
#define PCIE_AXI_INT_GRP_A_CAUSE_PARITY_ERR_OUT_DATA_WR (1 << 5)
 
#define PCIE_AXI_INT_GRP_A_CAUSE_RESERVED_6 (1 << 6)
 
#define PCIE_AXI_INT_GRP_A_CAUSE_SW_ECAM_ERR_RD (1 << 7)
 
#define PCIE_AXI_INT_GRP_A_CAUSE_SW_ECAM_ERR_WR (1 << 8)
 
#define PCIE_AXI_INT_GRP_A_CAUSE_PCIE_CORE_INT (1 << 9)
 
#define PCIE_AXI_INT_GRP_A_CAUSE_MSTR_AXI_GETOUT_MSG (1 << 10)
 
#define PCIE_AXI_INT_GRP_A_CAUSE_RD_CMPL_ERR (1 << 11)
 
#define PCIE_AXI_INT_GRP_A_CAUSE_WR_CMPL_ERR (1 << 12)
 
#define PCIE_AXI_INT_GRP_A_CAUSE_RD_CMPL_TO (1 << 13)
 
#define PCIE_AXI_INT_GRP_A_CAUSE_WR_CMPL_TO (1 << 14)
 
#define PCIE_AXI_INT_GRP_A_CAUSE_PARITY_ERROR_AXI (1 << 15)
 
#define PCIE_AXI_INT_GRP_A_CAUSE_POS_AXI_BRESP (1 << 16)
 
#define PCIE_AXI_INT_GRP_A_CAUSE_WRITE_CNT_FULL_ERR (1 << 17)
 
#define PCIE_AXI_INT_GRP_A_CAUSE_BRESP_BEFORE_WR_CNT_INC_ERR (1 << 18)

#define PCIE_AXI_INT_GRP_A_CTRL_CLEAR_ON_READ (1 << 0)
 
#define PCIE_AXI_INT_GRP_A_CTRL_AUTO_MASK (1 << 1)
 
#define PCIE_AXI_INT_GRP_A_CTRL_AUTO_CLEAR (1 << 2)
 
#define PCIE_AXI_INT_GRP_A_CTRL_SET_ON_POS (1 << 3)
 
#define PCIE_AXI_INT_GRP_A_CTRL_MOD_RST (1 << 4)
 
#define PCIE_AXI_INT_GRP_A_CTRL_MASK_MSI_X (1 << 5)
 
#define PCIE_AXI_INT_GRP_A_CTRL_AWID_MASK 0x00000F00
#define PCIE_AXI_INT_GRP_A_CTRL_AWID_SHIFT 8
 
#define PCIE_AXI_INT_GRP_A_CTRL_MOD_INTV_MASK 0x00FF0000
#define PCIE_AXI_INT_GRP_A_CTRL_MOD_INTV_SHIFT 16
 
#define PCIE_AXI_INT_GRP_A_CTRL_MOD_RES_MASK 0x0F000000
#define PCIE_AXI_INT_GRP_A_CTRL_MOD_RES_SHIFT 24

#ifdef __cplusplus
}
#endif

#endif  
