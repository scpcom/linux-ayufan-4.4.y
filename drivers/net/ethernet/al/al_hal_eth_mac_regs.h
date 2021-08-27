 
#ifndef __AL_HAL_ETH_MAC_REG_H
#define __AL_HAL_ETH_MAC_REG_H

#ifdef __cplusplus
extern "C" {
#endif
 
struct al_eth_mac_1g {
	 
	uint32_t rev;
	uint32_t scratch;
	uint32_t cmd_cfg;
	uint32_t mac_0;
	 
	uint32_t mac_1;
	uint32_t frm_len;
	uint32_t pause_quant;
	uint32_t rx_section_empty;
	 
	uint32_t rx_section_full;
	uint32_t tx_section_empty;
	uint32_t tx_section_full;
	uint32_t rx_almost_empty;
	 
	uint32_t rx_almost_full;
	uint32_t tx_almost_empty;
	uint32_t tx_almost_full;
	uint32_t mdio_addr0;
	 
	uint32_t mdio_addr1;
	uint32_t Reserved[5];
	 
	uint32_t reg_stat;
	uint32_t tx_ipg_len;
	 
	uint32_t Reserved1[104];
	 
	uint32_t phy_regs_base;
	uint32_t Reserved2[127];
};

struct al_eth_mac_10g {
	 
	uint32_t rev;
	uint32_t scratch;
	uint32_t cmd_cfg;
	uint32_t mac_0;
	 
	uint32_t mac_1;
	uint32_t frm_len;
	uint32_t Reserved;
	uint32_t rx_fifo_sections;
	 
	uint32_t tx_fifo_sections;
	uint32_t rx_fifo_almost_f_e;
	uint32_t tx_fifo_almost_f_e;
	uint32_t hashtable_load;
	 
	uint32_t mdio_cfg_status;
	uint16_t mdio_cmd;
	uint16_t reserved1;
	uint16_t mdio_data;
	uint16_t reserved2;
	uint16_t mdio_regaddr;
	uint16_t reserved3;
	 
	uint32_t status;
	uint32_t tx_ipg_len;
	uint32_t Reserved1[3];
	 
	uint32_t cl01_pause_quanta;
	uint32_t cl23_pause_quanta;
	uint32_t cl45_pause_quanta;
	 
	uint32_t cl67_pause_quanta;
	uint32_t cl01_quanta_thresh;
	uint32_t cl23_quanta_thresh;
	uint32_t cl45_quanta_thresh;
	 
	uint32_t cl67_quanta_thresh;
	uint32_t rx_pause_status;
	uint32_t Reserved2;
	uint32_t ts_timestamp;
	 
	uint32_t Reserved3[160];

	uint32_t control;
	uint32_t status_reg;
	uint32_t phy_id[2];
	 
	uint32_t dev_ability;
	uint32_t partner_ability;
	uint32_t an_expansion;
	uint32_t device_np;
	 
	uint32_t partner_np;
	uint32_t Reserved4[9];

	uint32_t link_timer_lo;
	uint32_t link_timer_hi;
	 
	uint32_t if_mode;

	uint32_t Reserved5[43];
};

struct al_eth_mac_gen {
	 
	uint32_t version;
	uint32_t rsrvd_0[2];
	 
	uint32_t cfg;
	 
	uint32_t mac_1g_cfg;
	 
	uint32_t mac_1g_stat;
	 
	uint32_t rgmii_cfg;
	 
	uint32_t rgmii_stat;
	 
	uint32_t mac_10g_cfg;
	 
	uint32_t mac_10g_stat;
	 
	uint32_t xaui_cfg;
	 
	uint32_t xaui_stat;
	 
	uint32_t rxaui_cfg;
	 
	uint32_t rxaui_stat;
	 
	uint32_t sd_cfg;
	 
	uint32_t mdio_ctrl_1;
	 
	uint32_t mdio_1;
	 
	uint32_t mdio_ctrl_2;
	 
	uint32_t mdio_2;
	 
	uint32_t xgmii_dfifo_32_64;
	 
	uint32_t mac_res_1_out;
	 
	uint32_t xgmii_dfifo_64_32;
	 
	uint32_t mac_res_1_in;
	 
	uint32_t sd_fifo_ctrl;
	 
	uint32_t sd_fifo_stat;
	 
	uint32_t mux_sel;
	 
	uint32_t clk_cfg;
	uint32_t rsrvd_1;
	 
	uint32_t los_sel;
	 
	uint32_t rgmii_sel;
	 
	uint32_t led_cfg;
	uint32_t rsrvd[33];
};
struct al_eth_mac_kr {
	 
	uint32_t pcs_addr;
	 
	uint32_t pcs_data;
	 
	uint32_t an_addr;
	 
	uint32_t an_data;
	 
	uint32_t pma_addr;
	 
	uint32_t pma_data;
	 
	uint32_t mtip_addr;
	 
	uint32_t mtip_data;
	 
	uint32_t pcs_cfg;
	 
	uint32_t pcs_stat;
	uint32_t rsrvd[54];
};
struct al_eth_mac_sgmii {
	 
	uint32_t reg_addr;
	 
	uint32_t reg_data;
	 
	uint32_t clk_div;
	 
	uint32_t link_stat;
	uint32_t rsrvd[60];
};
struct al_eth_mac_stat {
	 
	uint32_t match_fault;
	 
	uint32_t eee_in;
	 
	uint32_t eee_out;
	uint32_t rsrvd[61];
};
struct al_eth_mac_stat_lane {
	 
	uint32_t char_err;
	 
	uint32_t disp_err;
	 
	uint32_t pat;
	uint32_t rsrvd[13];
};

struct al_eth_mac_regs {
	struct al_eth_mac_1g mac_1g;				 
	struct al_eth_mac_10g mac_10g;				 
	uint32_t rsrvd_0[64];					 
	struct al_eth_mac_gen gen;                               
	struct al_eth_mac_kr kr;                                 
	struct al_eth_mac_sgmii sgmii;                           
	struct al_eth_mac_stat stat;                             
	struct al_eth_mac_stat_lane stat_lane[4];                
};

#define AL_ETH_1G_MAC_CTRL_HD_EN		(1 << 10)
 
#define AL_ETH_1G_MAC_CTRL_1G_SPD		(1 << 3)
 
#define AL_ETH_1G_MAC_CTRL_10M_SPD		(1 << 25)

#define ETH_MAC_GEN_VERSION_RELEASE_NUM_MINOR_MASK 0x000000FF
#define ETH_MAC_GEN_VERSION_RELEASE_NUM_MINOR_SHIFT 0
 
#define ETH_MAC_GEN_VERSION_RELEASE_NUM_MAJOR_MASK 0x0000FF00
#define ETH_MAC_GEN_VERSION_RELEASE_NUM_MAJOR_SHIFT 8
 
#define ETH_MAC_GEN_VERSION_DATE_DAY_MASK 0x001F0000
#define ETH_MAC_GEN_VERSION_DATE_DAY_SHIFT 16
 
#define ETH_MAC_GEN_VERSION_DATA_MONTH_MASK 0x01E00000
#define ETH_MAC_GEN_VERSION_DATA_MONTH_SHIFT 21
 
#define ETH_MAC_GEN_VERSION_DATE_YEAR_MASK 0x3E000000
#define ETH_MAC_GEN_VERSION_DATE_YEAR_SHIFT 25
 
#define ETH_MAC_GEN_VERSION_RESERVED_MASK 0xC0000000
#define ETH_MAC_GEN_VERSION_RESERVED_SHIFT 30

#define ETH_MAC_GEN_CFG_MAC_1_10         (1 << 0)
 
#define ETH_MAC_GEN_CFG_XGMII_SGMII_MASK 0x00000006
#define ETH_MAC_GEN_CFG_XGMII_SGMII_SHIFT 1
 
#define ETH_MAC_GEN_CFG_XAUI_RXAUI       (1 << 3)
 
#define ETH_MAC_GEN_CFG_SWAP_TBI_RX      (1 << 4)
 
#define ETH_MAC_GEN_CFG_TBI_MSB_RX       (1 << 5)
 
#define ETH_MAC_GEN_CFG_SGMII_SEL        (1 << 6)
 
#define ETH_MAC_GEN_CFG_RGMII_SGMII_SEL  (1 << 7)
 
#define ETH_MAC_GEN_CFG_SWAP_TBI_TX      (1 << 8)
 
#define ETH_MAC_GEN_CFG_TBI_MSB_TX       (1 << 9)
 
#define ETH_MAC_GEN_CFG_MDIO_1_10        (1 << 10)
 
#define ETH_MAC_GEN_CFG_MDIO_POL         (1 << 11)
 
#define ETH_MAC_GEN_CFG_SWAP_SERDES_RX_MASK 0x000F0000
#define ETH_MAC_GEN_CFG_SWAP_SERDES_RX_SHIFT 16
 
#define ETH_MAC_GEN_CFG_SWAP_SERDES_TX_MASK 0x0F000000
#define ETH_MAC_GEN_CFG_SWAP_SERDES_TX_SHIFT 24

#define ETH_MAC_GEN_MAC_1G_CFG_SET_1000_SEL (1 << 0)
 
#define ETH_MAC_GEN_MAC_1G_CFG_SET_1000_DEF (1 << 1)
 
#define ETH_MAC_GEN_MAC_1G_CFG_SET_10_SEL (1 << 4)
 
#define ETH_MAC_GEN_MAC_1G_CFG_SET_10_DEF (1 << 5)
 
#define ETH_MAC_GEN_MAC_1G_CFG_LOWP_ENA  (1 << 8)
 
#define ETH_MAC_GEN_MAC_1G_CFG_SLEEPN    (1 << 9)
 
#define ETH_MAC_GEN_MAC_1G_CFG_SWAP_FF_TX_CRC (1 << 12)

#define ETH_MAC_GEN_MAC_1G_STAT_EN_10    (1 << 0)
 
#define ETH_MAC_GEN_MAC_1G_STAT_ETH_MODE (1 << 1)
 
#define ETH_MAC_GEN_MAC_1G_STAT_LOWP     (1 << 4)
 
#define ETH_MAC_GEN_MAC_1G_STAT_WAKEUP   (1 << 5)

#define ETH_MAC_GEN_RGMII_CFG_SET_1000_SEL (1 << 0)
 
#define ETH_MAC_GEN_RGMII_CFG_SET_1000_DEF (1 << 1)
 
#define ETH_MAC_GEN_RGMII_CFG_SET_10_SEL (1 << 4)
 
#define ETH_MAC_GEN_RGMII_CFG_SET_10_DEF (1 << 5)
 
#define ETH_MAC_GEN_RGMII_CFG_ENA_AUTO   (1 << 8)
 
#define ETH_MAC_GEN_RGMII_CFG_SET_FD     (1 << 9)

#define ETH_MAC_GEN_RGMII_STAT_SPEED_MASK 0x00000003
#define ETH_MAC_GEN_RGMII_STAT_SPEED_SHIFT 0
 
#define ETH_MAC_GEN_RGMII_STAT_LINK      (1 << 4)
 
#define ETH_MAC_GEN_RGMII_STAT_DUP       (1 << 5)

#define ETH_MAC_GEN_MAC_10G_CFG_TX_LOC_FAULT (1 << 0)
 
#define ETH_MAC_GEN_MAC_10G_CFG_TX_REM_FAULT (1 << 1)
 
#define ETH_MAC_GEN_MAC_10G_CFG_TX_LI_FAULT (1 << 2)
 
#define ETH_MAC_GEN_MAC_10G_CFG_SG_SRESET (1 << 3)
 
#define ETH_MAC_GEN_MAC_10G_CFG_PHY_LOS_SEL_MASK 0x00000030
#define ETH_MAC_GEN_MAC_10G_CFG_PHY_LOS_SEL_SHIFT 4
 
#define ETH_MAC_GEN_MAC_10G_CFG_PHY_LOS_DEF (1 << 6)
 
#define ETH_MAC_GEN_MAC_10G_CFG_PHY_LOS_POL (1 << 7)
 
#define ETH_MAC_GEN_MAC_10G_CFG_LOWP_ENA (1 << 8)
 
#define ETH_MAC_GEN_MAC_10G_CFG_SWAP_FF_TX_CRC (1 << 12)

#define ETH_MAC_GEN_MAC_10G_STAT_LOC_FAULT (1 << 0)
 
#define ETH_MAC_GEN_MAC_10G_STAT_REM_FAULT (1 << 1)
 
#define ETH_MAC_GEN_MAC_10G_STAT_LI_FAULT (1 << 2)
 
#define ETH_MAC_GEN_MAC_10G_STAT_PFC_MODE (1 << 3)

#define ETH_MAC_GEN_MAC_10G_STAT_SG_ENA  (1 << 4)

#define ETH_MAC_GEN_MAC_10G_STAT_SG_ANDONE (1 << 5)

#define ETH_MAC_GEN_MAC_10G_STAT_SG_SYNC (1 << 6)

#define ETH_MAC_GEN_MAC_10G_STAT_SG_SPEED_MASK 0x00000180
#define ETH_MAC_GEN_MAC_10G_STAT_SG_SPEED_SHIFT 7
 
#define ETH_MAC_GEN_MAC_10G_STAT_LOWP    (1 << 9)
 
#define ETH_MAC_GEN_MAC_10G_STAT_TS_AVAIL (1 << 10)
 
#define ETH_MAC_GEN_MAC_10G_STAT_PAUSE_ON_MASK 0xFF000000
#define ETH_MAC_GEN_MAC_10G_STAT_PAUSE_ON_SHIFT 24

#define ETH_MAC_GEN_XAUI_CFG_JUMBO_EN    (1 << 0)

#define ETH_MAC_GEN_XAUI_STAT_ALIGN_DONE (1 << 0)
 
#define ETH_MAC_GEN_XAUI_STAT_SYNC_MASK  0x000000F0
#define ETH_MAC_GEN_XAUI_STAT_SYNC_SHIFT 4
 
#define ETH_MAC_GEN_XAUI_STAT_CG_ALIGN_MASK 0x00000F00
#define ETH_MAC_GEN_XAUI_STAT_CG_ALIGN_SHIFT 8

#define ETH_MAC_GEN_RXAUI_CFG_JUMBO_EN   (1 << 0)
 
#define ETH_MAC_GEN_RXAUI_CFG_SRBL_EN    (1 << 1)
 
#define ETH_MAC_GEN_RXAUI_CFG_DISP_ACROSS_LANE (1 << 2)

#define ETH_MAC_GEN_RXAUI_STAT_ALIGN_DONE (1 << 0)
 
#define ETH_MAC_GEN_RXAUI_STAT_SYNC_MASK 0x000000F0
#define ETH_MAC_GEN_RXAUI_STAT_SYNC_SHIFT 4
 
#define ETH_MAC_GEN_RXAUI_STAT_CG_ALIGN_MASK 0x00000F00
#define ETH_MAC_GEN_RXAUI_STAT_CG_ALIGN_SHIFT 8

#define ETH_MAC_GEN_SD_CFG_SEL_MASK      0x0000000F
#define ETH_MAC_GEN_SD_CFG_SEL_SHIFT     0
 
#define ETH_MAC_GEN_SD_CFG_VAL_MASK      0x000000F0
#define ETH_MAC_GEN_SD_CFG_VAL_SHIFT     4
 
#define ETH_MAC_GEN_SD_CFG_POL_MASK      0x00000F00
#define ETH_MAC_GEN_SD_CFG_POL_SHIFT     8

#define ETH_MAC_GEN_MDIO_CTRL_1_AVAIL    (1 << 0)

#define ETH_MAC_GEN_MDIO_1_INFO_MASK     0x000000FF
#define ETH_MAC_GEN_MDIO_1_INFO_SHIFT    0

#define ETH_MAC_GEN_MDIO_CTRL_2_AVAIL    (1 << 0)

#define ETH_MAC_GEN_MDIO_2_INFO_MASK     0x000000FF
#define ETH_MAC_GEN_MDIO_2_INFO_SHIFT    0

#define ETH_MAC_GEN_XGMII_DFIFO_32_64_ENABLE (1 << 0)
 
#define ETH_MAC_GEN_XGMII_DFIFO_32_64_RW_2_CYCLES (1 << 1)
 
#define ETH_MAC_GEN_XGMII_DFIFO_32_64_SWAP_LSB_MSB (1 << 2)
 
#define ETH_MAC_GEN_XGMII_DFIFO_32_64_SW_RESET (1 << 4)
 
#define ETH_MAC_GEN_XGMII_DFIFO_32_64_READ_TH_MASK 0x0000FF00
#define ETH_MAC_GEN_XGMII_DFIFO_32_64_READ_TH_SHIFT 8
 
#define ETH_MAC_GEN_XGMII_DFIFO_32_64_USED_MASK 0x00FF0000
#define ETH_MAC_GEN_XGMII_DFIFO_32_64_USED_SHIFT 16

#define ETH_MAC_GEN_XGMII_DFIFO_64_32_ENABLE (1 << 0)
 
#define ETH_MAC_GEN_XGMII_DFIFO_64_32_RW_2_CYCLES (1 << 1)
 
#define ETH_MAC_GEN_XGMII_DFIFO_64_32_SWAP_LSB_MSB (1 << 2)
 
#define ETH_MAC_GEN_XGMII_DFIFO_64_32_SW_RESET (1 << 4)
 
#define ETH_MAC_GEN_XGMII_DFIFO_64_32_READ_TH_MASK 0x0000FF00
#define ETH_MAC_GEN_XGMII_DFIFO_64_32_READ_TH_SHIFT 8
 
#define ETH_MAC_GEN_XGMII_DFIFO_64_32_USED_MASK 0x00FF0000
#define ETH_MAC_GEN_XGMII_DFIFO_64_32_USED_SHIFT 16

#define ETH_MAC_GEN_SD_FIFO_CTRL_ENABLE_MASK 0x0000000F
#define ETH_MAC_GEN_SD_FIFO_CTRL_ENABLE_SHIFT 0
 
#define ETH_MAC_GEN_SD_FIFO_CTRL_SW_RESET_MASK 0x000000F0
#define ETH_MAC_GEN_SD_FIFO_CTRL_SW_RESET_SHIFT 4
 
#define ETH_MAC_GEN_SD_FIFO_CTRL_READ_TH_MASK 0x0000FF00
#define ETH_MAC_GEN_SD_FIFO_CTRL_READ_TH_SHIFT 8

#define ETH_MAC_GEN_SD_FIFO_STAT_USED_0_MASK 0x000000FF
#define ETH_MAC_GEN_SD_FIFO_STAT_USED_0_SHIFT 0
 
#define ETH_MAC_GEN_SD_FIFO_STAT_USED_1_MASK 0x0000FF00
#define ETH_MAC_GEN_SD_FIFO_STAT_USED_1_SHIFT 8
 
#define ETH_MAC_GEN_SD_FIFO_STAT_USED_2_MASK 0x00FF0000
#define ETH_MAC_GEN_SD_FIFO_STAT_USED_2_SHIFT 16
 
#define ETH_MAC_GEN_SD_FIFO_STAT_USED_3_MASK 0xFF000000
#define ETH_MAC_GEN_SD_FIFO_STAT_USED_3_SHIFT 24

#define ETH_MAC_GEN_MUX_SEL_SGMII_IN_MASK 0x00000003
#define ETH_MAC_GEN_MUX_SEL_SGMII_IN_SHIFT 0
 
#define ETH_MAC_GEN_MUX_SEL_RXAUI_0_IN_MASK 0x0000000C
#define ETH_MAC_GEN_MUX_SEL_RXAUI_0_IN_SHIFT 2
 
#define ETH_MAC_GEN_MUX_SEL_RXAUI_1_IN_MASK 0x00000030
#define ETH_MAC_GEN_MUX_SEL_RXAUI_1_IN_SHIFT 4
 
#define ETH_MAC_GEN_MUX_SEL_XAUI_0_IN_MASK 0x000000C0
#define ETH_MAC_GEN_MUX_SEL_XAUI_0_IN_SHIFT 6
 
#define ETH_MAC_GEN_MUX_SEL_XAUI_1_IN_MASK 0x00000300
#define ETH_MAC_GEN_MUX_SEL_XAUI_1_IN_SHIFT 8
 
#define ETH_MAC_GEN_MUX_SEL_XAUI_2_IN_MASK 0x00000C00
#define ETH_MAC_GEN_MUX_SEL_XAUI_2_IN_SHIFT 10
 
#define ETH_MAC_GEN_MUX_SEL_XAUI_3_IN_MASK 0x00003000
#define ETH_MAC_GEN_MUX_SEL_XAUI_3_IN_SHIFT 12
 
#define ETH_MAC_GEN_MUX_SEL_KR_IN_MASK   0x0000C000
#define ETH_MAC_GEN_MUX_SEL_KR_IN_SHIFT  14
 
#define ETH_MAC_GEN_MUX_SEL_SERDES_0_TX_MASK 0x00070000
#define ETH_MAC_GEN_MUX_SEL_SERDES_0_TX_SHIFT 16
 
#define ETH_MAC_GEN_MUX_SEL_SERDES_1_TX_MASK 0x00380000
#define ETH_MAC_GEN_MUX_SEL_SERDES_1_TX_SHIFT 19
 
#define ETH_MAC_GEN_MUX_SEL_SERDES_2_TX_MASK 0x01C00000
#define ETH_MAC_GEN_MUX_SEL_SERDES_2_TX_SHIFT 22
 
#define ETH_MAC_GEN_MUX_SEL_SERDES_3_TX_MASK 0x0E000000
#define ETH_MAC_GEN_MUX_SEL_SERDES_3_TX_SHIFT 25

#define ETH_MAC_GEN_CLK_CFG_LANE_0_CLK_SEL_MASK 0x00000003
#define ETH_MAC_GEN_CLK_CFG_LANE_0_CLK_SEL_SHIFT 0
 
#define ETH_MAC_GEN_CLK_CFG_LANE_1_CLK_SEL_MASK 0x00000030
#define ETH_MAC_GEN_CLK_CFG_LANE_1_CLK_SEL_SHIFT 4
 
#define ETH_MAC_GEN_CLK_CFG_LANE_2_CLK_SEL_MASK 0x00000300
#define ETH_MAC_GEN_CLK_CFG_LANE_2_CLK_SEL_SHIFT 8
 
#define ETH_MAC_GEN_CLK_CFG_LANE_3_CLK_SEL_MASK 0x00003000
#define ETH_MAC_GEN_CLK_CFG_LANE_3_CLK_SEL_SHIFT 12
 
#define ETH_MAC_GEN_CLK_CFG_GMII_RX_CLK_SEL (1 << 16)
 
#define ETH_MAC_GEN_CLK_CFG_GMII_TX_CLK_SEL (1 << 18)
 
#define ETH_MAC_GEN_CLK_CFG_TX_CLK_SEL   (1 << 28)
 
#define ETH_MAC_GEN_CLK_CFG_RX_CLK_SEL   (1 << 30)

#define ETH_MAC_GEN_LOS_SEL_LANE_0_SEL_MASK 0x00000003
#define ETH_MAC_GEN_LOS_SEL_LANE_0_SEL_SHIFT 0
 
#define ETH_MAC_GEN_LOS_SEL_LANE_1_SEL_MASK 0x00000030
#define ETH_MAC_GEN_LOS_SEL_LANE_1_SEL_SHIFT 4
 
#define ETH_MAC_GEN_LOS_SEL_LANE_2_SEL_MASK 0x00000300
#define ETH_MAC_GEN_LOS_SEL_LANE_2_SEL_SHIFT 8
 
#define ETH_MAC_GEN_LOS_SEL_LANE_3_SEL_MASK 0x00003000
#define ETH_MAC_GEN_LOS_SEL_LANE_3_SEL_SHIFT 12

#define ETH_MAC_GEN_RGMII_SEL_RX_SWAP_3_0 (1 << 0)
 
#define ETH_MAC_GEN_RGMII_SEL_RX_SWAP_4  (1 << 1)
 
#define ETH_MAC_GEN_RGMII_SEL_RX_SWAP_7_3 (1 << 2)
 
#define ETH_MAC_GEN_RGMII_SEL_RX_SWAP_9  (1 << 3)
 
#define ETH_MAC_GEN_RGMII_SEL_TX_SWAP_3_0 (1 << 4)
 
#define ETH_MAC_GEN_RGMII_SEL_TX_SWAP_4  (1 << 5)
 
#define ETH_MAC_GEN_RGMII_SEL_TX_SWAP_7_3 (1 << 6)
 
#define ETH_MAC_GEN_RGMII_SEL_TX_SWAP_9  (1 << 7)

#define ETH_MAC_GEN_LED_CFG_SEL_MASK     0x0000000F
#define ETH_MAC_GEN_LED_CFG_SEL_SHIFT    0

#define ETH_MAC_GEN_LED_CFG_SEL_DEFAULT_REG	0
#define ETH_MAC_GEN_LED_CFG_SEL_RX_ACTIVITY_DEPRECIATED	1
#define ETH_MAC_GEN_LED_CFG_SEL_TX_ACTIVITY_DEPRECIATED	2
#define ETH_MAC_GEN_LED_CFG_SEL_RX_TX_ACTIVITY_DEPRECIATED 3
#define ETH_MAC_GEN_LED_CFG_SEL_LINK_ACTIVITY 10

#define ETH_MAC_GEN_LED_CFG_DEF          (1 << 4)
 
#define ETH_MAC_GEN_LED_CFG_POL          (1 << 5)
 
#define ETH_MAC_GEN_LED_CFG_ACT_TIMER_MASK 0x00FF0000
#define ETH_MAC_GEN_LED_CFG_ACT_TIMER_SHIFT 16
 
#define ETH_MAC_GEN_LED_CFG_BLINK_TIMER_MASK 0xFF000000
#define ETH_MAC_GEN_LED_CFG_BLINK_TIMER_SHIFT 24

#define ETH_MAC_KR_PCS_ADDR_VAL_MASK     0x0000FFFF
#define ETH_MAC_KR_PCS_ADDR_VAL_SHIFT    0

#define ETH_MAC_KR_PCS_DATA_VAL_MASK     0x0000FFFF
#define ETH_MAC_KR_PCS_DATA_VAL_SHIFT    0

#define ETH_MAC_KR_AN_ADDR_VAL_MASK      0x0000FFFF
#define ETH_MAC_KR_AN_ADDR_VAL_SHIFT     0

#define ETH_MAC_KR_AN_DATA_VAL_MASK      0x0000FFFF
#define ETH_MAC_KR_AN_DATA_VAL_SHIFT     0

#define ETH_MAC_KR_PMA_ADDR_VAL_MASK     0x0000FFFF
#define ETH_MAC_KR_PMA_ADDR_VAL_SHIFT    0

#define ETH_MAC_KR_PMA_DATA_VAL_MASK     0x0000FFFF
#define ETH_MAC_KR_PMA_DATA_VAL_SHIFT    0

#define ETH_MAC_KR_MTIP_ADDR_VAL_MASK    0x0000FFFF
#define ETH_MAC_KR_MTIP_ADDR_VAL_SHIFT   0

#define ETH_MAC_KR_MTIP_DATA_VAL_MASK    0x0000FFFF
#define ETH_MAC_KR_MTIP_DATA_VAL_SHIFT   0

#define ETH_MAC_KR_PCS_CFG_STRAP_AN_ENA  (1 << 0)
 
#define ETH_MAC_KR_PCS_CFG_EEE_SD_SEL    (1 << 4)
 
#define ETH_MAC_KR_PCS_CFG_EEE_DEF_VAL   (1 << 5)
 
#define ETH_MAC_KR_PCS_CFG_EEE_SD_POL    (1 << 6)
 
#define ETH_MAC_KR_PCS_CFG_EEE_TIMER_VAL_MASK 0x0000FF00
#define ETH_MAC_KR_PCS_CFG_EEE_TIMER_VAL_SHIFT 8
 
#define ETH_MAC_KR_PCS_CFG_DME_SEL       (1 << 16)
 
#define ETH_MAC_KR_PCS_CFG_DME_VAL       (1 << 17)
 
#define ETH_MAC_KR_PCS_CFG_DME_POL       (1 << 18)

#define ETH_MAC_KR_PCS_STAT_AN_LINK_CONTROL_MASK 0x0000003F
#define ETH_MAC_KR_PCS_STAT_AN_LINK_CONTROL_SHIFT 0
 
#define ETH_MAC_KR_PCS_STAT_BLOCK_LOCK   (1 << 8)
 
#define ETH_MAC_KR_PCS_STAT_HI_BER       (1 << 9)

#define ETH_MAC_KR_PCS_STAT_RX_WAKE_ERR  (1 << 16)

#define ETH_MAC_KR_PCS_STAT_PMA_TXMODE_ALERT (1 << 17)

#define ETH_MAC_KR_PCS_STAT_PMA_TXMODE_QUIET (1 << 18)

#define ETH_MAC_KR_PCS_STAT_PMA_RXMODE_QUIET (1 << 19)

#define ETH_MAC_KR_PCS_STAT_RX_LPI_ACTIVE (1 << 20)

#define ETH_MAC_KR_PCS_STAT_TX_LPI_ACTIVE (1 << 21)

#define ETH_MAC_SGMII_REG_ADDR_VAL_MASK  0x0000001F
#define ETH_MAC_SGMII_REG_ADDR_VAL_SHIFT 0

#ifdef CONFIG_SYNO_ALPINE_A0
#define ETH_MAC_SGMII_REG_ADDR_CTRL_REG	0x0
#define ETH_MAC_SGMII_REG_ADDR_IF_MODE_REG 0x14
#endif
 
#define ETH_MAC_SGMII_REG_DATA_VAL_MASK  0x0000FFFF
#define ETH_MAC_SGMII_REG_DATA_VAL_SHIFT 0
#ifdef CONFIG_SYNO_ALPINE_A0
#define ETH_MAC_SGMII_REG_DATA_CTRL_AN_ENABLE			(1 << 12)
#define ETH_MAC_SGMII_REG_DATA_IF_MODE_SGMII_EN			(1 << 0)
#define ETH_MAC_SGMII_REG_DATA_IF_MODE_SGMII_AN			(1 << 1)
#define ETH_MAC_SGMII_REG_DATA_IF_MODE_SGMII_SPEED_MASK		0xC
#define ETH_MAC_SGMII_REG_DATA_IF_MODE_SGMII_SPEED_SHIFT	2
#define ETH_MAC_SGMII_REG_DATA_IF_MODE_SGMII_SPEED_10		0x0
#define ETH_MAC_SGMII_REG_DATA_IF_MODE_SGMII_SPEED_100		0x1
#define ETH_MAC_SGMII_REG_DATA_IF_MODE_SGMII_SPEED_1000		0x2
#define ETH_MAC_SGMII_REG_DATA_IF_MODE_SGMII_DUPLEX		(1 << 4)
#endif

#define ETH_MAC_SGMII_CLK_DIV_VAL_1000_MASK 0x000000FF
#define ETH_MAC_SGMII_CLK_DIV_VAL_1000_SHIFT 0
 
#define ETH_MAC_SGMII_CLK_DIV_VAL_100_MASK 0x0000FF00
#define ETH_MAC_SGMII_CLK_DIV_VAL_100_SHIFT 8
 
#define ETH_MAC_SGMII_CLK_DIV_VAL_10_MASK 0x00FF0000
#define ETH_MAC_SGMII_CLK_DIV_VAL_10_SHIFT 16
 
#define ETH_MAC_SGMII_CLK_DIV_BYPASS     (1 << 24)
 
#define ETH_MAC_SGMII_CLK_DIV_SEL_MASK   0x0E000000
#define ETH_MAC_SGMII_CLK_DIV_SEL_SHIFT  25

#define ETH_MAC_SGMII_LINK_STAT_SET_1000 (1 << 0)

#define ETH_MAC_SGMII_LINK_STAT_SET_100  (1 << 1)

#define ETH_MAC_SGMII_LINK_STAT_SET_10   (1 << 2)

#define ETH_MAC_SGMII_LINK_STAT_LED_AN   (1 << 3)

#define ETH_MAC_SGMII_LINK_STAT_HD_ENA   (1 << 4)

#define ETH_MAC_SGMII_LINK_STAT_LED_LINK (1 << 5)

#ifdef __cplusplus
}
#endif

#endif  
