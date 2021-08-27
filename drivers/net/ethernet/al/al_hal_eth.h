 
#ifndef __AL_HAL_ETH_H__
#define __AL_HAL_ETH_H__

#include "al_hal_common.h"
#include "al_hal_udma.h"
#include "al_hal_eth_macsec.h"

#ifdef __cplusplus
extern "C" {
#endif
 
#define AL_ETH_PKT_MAX_BUFS		20
#define AL_ETH_UDMA_TX_QUEUES		4
#define AL_ETH_UDMA_RX_QUEUES		4

#define AL_ETH_DEV_ID_STANDARD		0x0001
#define AL_ETH_DEV_ID_ADVANCED		0x0002
#define AL_ETH_REV_ID_0		0
#define AL_ETH_REV_ID_1		1

#define AL_ETH_UDMA_BAR			0
#define AL_ETH_EC_BAR			4
#define AL_ETH_MAC_BAR			2

#define AL_ETH_MAX_FRAME_LEN		10000
#define AL_ETH_MIN_FRAME_LEN		30

#define AL_ETH_TSO_MSS_MAX_IDX		8
#define AL_ETH_TSO_MSS_MIN_VAL		1
 
#define AL_ETH_TSO_MSS_MAX_VAL		(AL_ETH_MAX_FRAME_LEN - 200)

enum AL_ETH_PROTO_ID {
	AL_ETH_PROTO_ID_UNKNOWN = 0,
	AL_ETH_PROTO_ID_IPv4	= 8,
	AL_ETH_PROTO_ID_IPv6	= 11,
	AL_ETH_PROTO_ID_TCP	= 12,
	AL_ETH_PROTO_ID_UDP	= 13,
	AL_ETH_PROTO_ID_ANY	= 32,  
};
#define AL_ETH_PROTOCOLS_NUM		(AL_ETH_PROTO_ID_ANY)

enum AL_ETH_TX_TUNNEL_MODE {
	AL_ETH_NO_TUNNELING	= 0,
	AL_ETH_TUNNEL_NO_UDP	= 1,  
	AL_ETH_TUNNEL_WITH_UDP	= 3,	 
};

#define AL_ETH_RX_THASH_TABLE_SIZE	(1 << 8)
#define AL_ETH_RX_FSM_TABLE_SIZE	(1 << 7)
#define AL_ETH_RX_CTRL_TABLE_SIZE	(1 << 11)
#define AL_ETH_RX_HASH_KEY_NUM		10
#define AL_ETH_FWD_MAC_NUM			32
#define AL_ETH_FWD_MAC_HASH_NUM			256
#define AL_ETH_FWD_PBITS_TABLE_NUM	(1 << 3)
#define AL_ETH_FWD_PRIO_TABLE_NUM	(1 << 3)
#define AL_ETH_FWD_VID_TABLE_NUM	(1 << 12)
#define AL_ETH_FWD_DSCP_TABLE_NUM	(1 << 8)
#define AL_ETH_FWD_TC_TABLE_NUM	(1 << 8)

enum al_eth_mac_mode {
	AL_ETH_MAC_MODE_RGMII = 0,
	AL_ETH_MAC_MODE_XAUI  = 1,	 
	AL_ETH_MAC_MODE_RXAUI = 2,	 
	AL_ETH_MAC_MODE_SGMII = 3,
	AL_ETH_MAC_MODE_10GbE_Serial    = 4,	 
	AL_ETH_MAC_MODE_10G_SGMII	= 5  
};

struct al_eth_capabilities {
	al_bool	speed_10_HD;
	al_bool	speed_10_FD;
	al_bool speed_100_HD;
	al_bool speed_100_FD;
	al_bool speed_1000_HD;
	al_bool speed_1000_FD;
	al_bool speed_10000_HD;
	al_bool speed_10000_FD;
	al_bool pfc;  
	al_bool eee;  
};

enum al_eth_mdio_if {
	AL_ETH_MDIO_IF_1G_MAC = 0,
	AL_ETH_MDIO_IF_10G_MAC = 1
};

enum al_eth_mdio_type {
	AL_ETH_MDIO_TYPE_CLAUSE_22 = 0,
	AL_ETH_MDIO_TYPE_CLAUSE_45 = 1
};

enum al_eth_flow_control_type {
	AL_ETH_FLOW_CONTROL_TYPE_LINK_PAUSE,
	AL_ETH_FLOW_CONTROL_TYPE_PFC
};

enum al_eth_tx_switch_dec_type {
	AL_ETH_TX_SWITCH_TYPE_MAC = 0,
	AL_ETH_TX_SWITCH_TYPE_VLAN_TABLE = 1,
	AL_ETH_TX_SWITCH_TYPE_VLAN_TABLE_AND_MAC = 2,
	AL_ETH_TX_SWITCH_TYPE_BITMAP = 3
};

enum al_eth_tx_switch_vid_sel_type {
	AL_ETH_TX_SWITCH_VID_SEL_TYPE_VLAN1 = 0,
	AL_ETH_TX_SWITCH_VID_SEL_TYPE_VLAN2 = 1,
	AL_ETH_TX_SWITCH_VID_SEL_TYPE_NEW_VLAN1 = 2,
	AL_ETH_TX_SWITCH_VID_SEL_TYPE_NEW_VLAN2 = 3,
	AL_ETH_TX_SWITCH_VID_SEL_TYPE_DEFAULT_VLAN1 = 4,
	AL_ETH_TX_SWITCH_VID_SEL_TYPE_FINAL_VLAN1 = 5
};

enum al_eth_rx_desc_lro_context_val_res {
	AL_ETH_LRO_CONTEXT_VALUE = 0,  
	AL_ETH_L4_OFFSET = 1,  
};

enum al_eth_rx_desc_l4_chk_res_sel {
	AL_ETH_L4_INNER_CHK = 0,  
	AL_ETH_l4_INNER_OUTER_CHK = 1,  
};

enum al_eth_rx_desc_l3_chk_res_sel {
	AL_ETH_L3_CHK_TYPE_0 = 0,  
	AL_ETH_L3_CHK_TYPE_1 = 1,  
	AL_ETH_L3_CHK_TYPE_2 = 2,  
	AL_ETH_L3_CHK_TYPE_3 = 3,  
};

struct al_eth_flow_control_params{
	enum al_eth_flow_control_type type;  
	al_bool		obay_enable;  
	al_bool		gen_enable;  
	uint16_t	rx_fifo_th_high;
	uint16_t	rx_fifo_th_low;
	uint16_t	quanta;
	uint16_t	quanta_th;
	uint8_t		prio_q_map[4][8];  
};

#define AL_ETH_TX_FLAGS_TSO		AL_BIT(7)   
#define AL_ETH_TX_FLAGS_IPV4_L3_CSUM	AL_BIT(13)  
#define AL_ETH_TX_FLAGS_L4_CSUM		AL_BIT(14)  
#define AL_ETH_TX_FLAGS_L4_PARTIAL_CSUM	AL_BIT(17)  
#define AL_ETH_TX_FLAGS_L2_MACSEC_PKT	AL_BIT(16)  
#define AL_ETH_TX_FLAGS_L2_DIS_FCS	AL_BIT(15)  
#define AL_ETH_TX_FLAGS_TS		AL_BIT(21)  

#define AL_ETH_TX_FLAGS_INT		AL_M2S_DESC_INT_EN
#define AL_ETH_TX_FLAGS_NO_SNOOP	AL_M2S_DESC_NO_SNOOP_H

struct al_eth_meta_data{
	uint8_t store :1;  
	uint8_t words_valid :4;  

	uint8_t vlan1_cfi_sel:2;
	uint8_t vlan2_vid_sel:2;
	uint8_t vlan2_cfi_sel:2;
	uint8_t vlan2_pbits_sel:2;
	uint8_t vlan2_ether_sel:2;

	uint16_t vlan1_new_vid:12;
	uint8_t vlan1_new_cfi :1;
	uint8_t vlan1_new_pbits :3;
	uint16_t vlan2_new_vid:12;
	uint8_t vlan2_new_cfi :1;
	uint8_t vlan2_new_pbits :3;

	uint8_t l3_header_len;  
	uint8_t l3_header_offset;
	uint8_t l4_header_len;  

	uint8_t mss_idx_sel:3;  

	uint8_t	ts_index:4;  
	uint16_t mss_val :14;  
	uint8_t outer_l3_offset;  
	uint8_t outer_l3_len;  
};

#define AL_ETH_RX_FLAGS_VMID_MASK	AL_FIELD_MASK(15, 0)
#define AL_ETH_RX_FLAGS_NO_SNOOP	AL_M2S_DESC_NO_SNOOP_H
#define AL_ETH_RX_FLAGS_INT		AL_M2S_DESC_INT_EN
#define AL_ETH_RX_FLAGS_DUAL_BUF	AL_BIT(31)

#define AL_ETH_RX_ERROR			AL_BIT(16)  
#define AL_ETH_RX_FLAGS_L4_CSUM_ERR	AL_BIT(14)
#define AL_ETH_RX_FLAGS_L3_CSUM_ERR	AL_BIT(13)

struct al_eth_pkt{
	uint32_t flags;  
	enum AL_ETH_PROTO_ID l3_proto_idx;
	enum AL_ETH_PROTO_ID l4_proto_idx;
	uint8_t source_vlan_count:2;
	uint8_t vlan_mod_add_count:2;
	uint8_t vlan_mod_del_count:2;
	uint8_t vlan_mod_v1_ether_sel:2;
	uint8_t vlan_mod_v1_vid_sel:2;
	uint8_t vlan_mod_v1_pbits_sel:2;

	enum AL_ETH_TX_TUNNEL_MODE tunnel_mode;
	enum AL_ETH_PROTO_ID outer_l3_proto_idx;  

	uint16_t vmid;

	struct al_buf	bufs[AL_ETH_PKT_MAX_BUFS];
	uint8_t num_of_bufs;
	uint32_t rx_header_len;  
	struct al_eth_meta_data *meta;  
#ifdef AL_ETH_RX_DESC_RAW_GET
	uint32_t rx_desc_raw[4];
#endif
	uint16_t rxhash;

	uint8_t macsec_secure_channel:6;   
	uint8_t macsec_association_number:2;  
	uint16_t macsec_secured_pyld_len:14;	 
	uint16_t macsec_rx_flags;		 
	al_bool macsec_encrypt;
	al_bool macsec_sign;
};

struct al_ec_regs;

struct al_hal_eth_adapter{
	uint16_t dev_id;  
	uint8_t rev_id;  
	uint8_t udma_id;  
	struct unit_regs __iomem * unit_regs;
	void __iomem *udma_regs_base;
	struct al_ec_regs __iomem *ec_regs_base;
	void __iomem *ec_ints_base;
	struct al_eth_mac_regs __iomem *mac_regs_base;
	struct interrupt_controller_ctrl __iomem *mac_ints_base;

	char *name;  

	struct al_udma tx_udma;
	  
	struct al_udma rx_udma;
	  
	uint8_t		enable_rx_parser;  

	enum al_eth_flow_control_type fc_type;  

	enum al_eth_mac_mode mac_mode;
	enum al_eth_mdio_if	mdio_if;  
	enum al_eth_mdio_type mdio_type;  
	al_bool	shared_mdio_if;  
};

struct al_eth_adapter_params{
	uint16_t dev_id;  
	uint8_t rev_id;  
	uint8_t udma_id;  
	uint8_t	enable_rx_parser;  
	void __iomem *udma_regs_base;  
	void __iomem *ec_regs_base;  
	void __iomem *mac_regs_base;  
	char *name;  
};

int al_eth_adapter_init(struct al_hal_eth_adapter *adapter, struct al_eth_adapter_params *params);

int al_eth_adapter_stop(struct al_hal_eth_adapter *adapter);

int al_eth_adapter_reset(struct al_hal_eth_adapter *adapter);

int al_eth_ec_mac_ints_config(struct al_hal_eth_adapter *adapter);

int al_eth_queue_config(struct al_hal_eth_adapter *adapter, enum al_udma_type type, uint32_t qid,
			struct al_udma_q_params *q_params);

int al_eth_queue_enable(struct al_hal_eth_adapter *adapter, enum al_udma_type type, uint32_t qid);

int al_eth_queue_disable(struct al_hal_eth_adapter *adapter, enum al_udma_type type, uint32_t qid);

int al_eth_mac_config(struct al_hal_eth_adapter *adapter, enum al_eth_mac_mode mode);

int al_eth_mac_stop(struct al_hal_eth_adapter *adapter);

int al_eth_mac_start(struct al_hal_eth_adapter *adapter);

int al_eth_capabilities_get(struct al_hal_eth_adapter *adapter, struct al_eth_capabilities *caps);

#ifdef CONFIG_SYNO_ALPINE_A0
 
int al_eth_mac_link_config(struct al_hal_eth_adapter *adapter,
			   al_bool force_1000_base_x,
			   al_bool an_enable,
			   uint32_t speed,
			   al_bool full_duplex);
#else
 
int al_eth_mac_link_config(struct al_hal_eth_adapter *adapter, uint32_t speed, al_bool full_duplex);
#endif

int al_eth_mac_loopback_config(struct al_hal_eth_adapter *adapter, int enable);

int al_eth_rx_pkt_limit_config(struct al_hal_eth_adapter *adapter, uint32_t min_rx_len, uint32_t max_rx_len);

enum al_eth_ref_clk_freq {
	AL_ETH_REF_FREQ_375_MHZ		= 0,
	AL_ETH_REF_FREQ_187_5_MHZ	= 1,
	AL_ETH_REF_FREQ_250_MHZ		= 2,
	AL_ETH_REF_FREQ_500_MHZ		= 3,
};

int al_eth_mdio_config(struct al_hal_eth_adapter *adapter,
		       enum al_eth_mdio_type mdio_type,
		       al_bool shared_mdio_if,
		       enum al_eth_ref_clk_freq ref_clk_freq,
		       unsigned int mdio_clk_freq_khz);

int al_eth_mdio_read(struct al_hal_eth_adapter *adapter, uint32_t phy_addr,
		     uint32_t device, uint32_t reg, uint16_t *val);

int al_eth_mdio_write(struct al_hal_eth_adapter *adapter, uint32_t phy_addr,
		      uint32_t device, uint32_t reg, uint16_t val);

static INLINE uint32_t al_eth_tx_available_get(struct al_hal_eth_adapter *adapter,
					       uint32_t qid)
{
	struct al_udma_q *udma_q;

	al_udma_q_handle_get(&adapter->tx_udma, qid, &udma_q);

	return al_udma_available_get(udma_q);
}

int al_eth_tx_pkt_prepare(struct al_udma_q *tx_dma_q, struct al_eth_pkt *pkt);

void al_eth_tx_dma_action(struct al_udma_q *tx_dma_q, uint32_t tx_descs);

int al_eth_comp_tx_get(struct al_udma_q *tx_dma_q);

int al_eth_tso_mss_config(struct al_hal_eth_adapter *adapter, uint8_t idx, uint32_t mss_val);

void al_eth_rx_desc_config(
			struct al_hal_eth_adapter *adapter,
			enum al_eth_rx_desc_lro_context_val_res lro_sel,
			enum al_eth_rx_desc_l4_chk_res_sel l4_sel,
			enum al_eth_rx_desc_l3_chk_res_sel l3_sel);

int al_eth_rx_buffer_add(struct al_udma_q *rx_dma_q,
			      struct al_buf *buf, uint32_t flags,
			      struct al_buf *header_buf);

void al_eth_rx_buffer_action(struct al_udma_q *rx_dma_q,
				uint32_t descs_num);

 uint32_t al_eth_pkt_rx(struct al_udma_q *rx_dma_q, struct al_eth_pkt *pkt);

int al_eth_thash_table_set(struct al_hal_eth_adapter *adapter, uint32_t idx, uint32_t entry);
int al_eth_fsm_table_set(struct al_hal_eth_adapter *adapter, uint32_t idx, uint32_t entry);

enum AL_ETH_FWD_CTRL_IDX_VLAN_TABLE_OUT {
	AL_ETH_FWD_CTRL_IDX_VLAN_TABLE_OUT_0 = 0,
	AL_ETH_FWD_CTRL_IDX_VLAN_TABLE_OUT_1 = 1,
	AL_ETH_FWD_CTRL_IDX_VLAN_TABLE_OUT_ANY = 2,
};

enum AL_ETH_FWD_CTRL_IDX_TUNNEL {
	AL_ETH_FWD_CTRL_IDX_TUNNEL_NOT_EXIST = 0,
	AL_ETH_FWD_CTRL_IDX_TUNNEL_EXIST = 1,
	AL_ETH_FWD_CTRL_IDX_TUNNEL_ANY = 2,
};

enum AL_ETH_FWD_CTRL_IDX_VLAN {
	AL_ETH_FWD_CTRL_IDX_VLAN_NOT_EXIST = 0,
	AL_ETH_FWD_CTRL_IDX_VLAN_EXIST = 1,
	AL_ETH_FWD_CTRL_IDX_VLAN_ANY = 2,
};

enum AL_ETH_FWD_CTRL_IDX_MAC_TABLE {
	AL_ETH_FWD_CTRL_IDX_MAC_TABLE_NO_MATCH = 0,
	AL_ETH_FWD_CTRL_IDX_MAC_TABLE_MATCH = 1,
	AL_ETH_FWD_CTRL_IDX_MAC_TABLE_ANY = 2,
};

enum AL_ETH_FWD_CTRL_IDX_MAC_DA_TYPE {
	AL_ETH_FWD_CTRL_IDX_MAC_DA_TYPE_UC = 0,  
	AL_ETH_FWD_CTRL_IDX_MAC_DA_TYPE_MC = 1,  
	AL_ETH_FWD_CTRL_IDX_MAC_DA_TYPE_BC = 2,  
	AL_ETH_FWD_CTRL_IDX_MAC_DA_TYPE_ANY = 4,  
};

struct al_eth_fwd_ctrl_table_index {
	enum AL_ETH_FWD_CTRL_IDX_VLAN_TABLE_OUT	vlan_table_out;
	enum AL_ETH_FWD_CTRL_IDX_TUNNEL tunnel_exist;
	enum AL_ETH_FWD_CTRL_IDX_VLAN vlan_exist;
	enum AL_ETH_FWD_CTRL_IDX_MAC_TABLE mac_table_match;
	enum AL_ETH_PROTO_ID		protocol_id;
	enum AL_ETH_FWD_CTRL_IDX_MAC_DA_TYPE mac_type;
};

enum AL_ETH_CTRL_TABLE_PRIO_SEL {
	AL_ETH_CTRL_TABLE_PRIO_SEL_PBITS_TABLE	= 0,
	AL_ETH_CTRL_TABLE_PRIO_SEL_DSCP_TABLE	= 1,
	AL_ETH_CTRL_TABLE_PRIO_SEL_TC_TABLE	= 2,
	AL_ETH_CTRL_TABLE_PRIO_SEL_REG1		= 3,
	AL_ETH_CTRL_TABLE_PRIO_SEL_REG2		= 4,
	AL_ETH_CTRL_TABLE_PRIO_SEL_REG3		= 5,
	AL_ETH_CTRL_TABLE_PRIO_SEL_REG4		= 6,
	AL_ETH_CTRL_TABLE_PRIO_SEL_REG5		= 7,
	AL_ETH_CTRL_TABLE_PRIO_SEL_REG6		= 7,
	AL_ETH_CTRL_TABLE_PRIO_SEL_REG7		= 9,
	AL_ETH_CTRL_TABLE_PRIO_SEL_REG8		= 10,
	AL_ETH_CTRL_TABLE_PRIO_SEL_VAL_3	= 11,
	AL_ETH_CTRL_TABLE_PRIO_SEL_VAL_0	= 12,
};
 
enum AL_ETH_CTRL_TABLE_QUEUE_SEL_1 {
	AL_ETH_CTRL_TABLE_QUEUE_SEL_1_PRIO_TABLE	= 0,
	AL_ETH_CTRL_TABLE_QUEUE_SEL_1_THASH_TABLE	= 1,
	AL_ETH_CTRL_TABLE_QUEUE_SEL_1_MAC_TABLE		= 2,
	AL_ETH_CTRL_TABLE_QUEUE_SEL_1_MHASH_TABLE	= 3,
	AL_ETH_CTRL_TABLE_QUEUE_SEL_1_REG1		= 4,
	AL_ETH_CTRL_TABLE_QUEUE_SEL_1_REG2		= 5,
	AL_ETH_CTRL_TABLE_QUEUE_SEL_1_REG3		= 6,
	AL_ETH_CTRL_TABLE_QUEUE_SEL_1_REG4		= 7,
	AL_ETH_CTRL_TABLE_QUEUE_SEL_1_VAL_3		= 12,
	AL_ETH_CTRL_TABLE_QUEUE_SEL_1_VAL_0		= 13,
};

enum AL_ETH_CTRL_TABLE_QUEUE_SEL_2 {
	AL_ETH_CTRL_TABLE_QUEUE_SEL_2_PRIO_TABLE	= 0,  
	AL_ETH_CTRL_TABLE_QUEUE_SEL_2_PRIO		= 1,  
	AL_ETH_CTRL_TABLE_QUEUE_SEL_2_PRIO_QUEUE	= 2,  
	AL_ETH_CTRL_TABLE_QUEUE_SEL_2_NO_PRIO		= 3,  
};

enum AL_ETH_CTRL_TABLE_UDMA_SEL {
	AL_ETH_CTRL_TABLE_UDMA_SEL_THASH_TABLE		= 0,
	AL_ETH_CTRL_TABLE_UDMA_SEL_THASH_AND_VLAN	= 1,
	AL_ETH_CTRL_TABLE_UDMA_SEL_VLAN_TABLE		= 2,
	AL_ETH_CTRL_TABLE_UDMA_SEL_VLAN_AND_MAC		= 3,
	AL_ETH_CTRL_TABLE_UDMA_SEL_MAC_TABLE		= 4,
	AL_ETH_CTRL_TABLE_UDMA_SEL_MAC_AND_MHASH	= 5,
	AL_ETH_CTRL_TABLE_UDMA_SEL_MHASH_TABLE		= 6,
	AL_ETH_CTRL_TABLE_UDMA_SEL_REG1			= 7,
	AL_ETH_CTRL_TABLE_UDMA_SEL_REG2			= 8,
	AL_ETH_CTRL_TABLE_UDMA_SEL_REG3			= 9,
	AL_ETH_CTRL_TABLE_UDMA_SEL_REG4			= 10,
	AL_ETH_CTRL_TABLE_UDMA_SEL_REG5			= 11,
	AL_ETH_CTRL_TABLE_UDMA_SEL_REG6			= 12,
	AL_ETH_CTRL_TABLE_UDMA_SEL_REG7			= 13,
	AL_ETH_CTRL_TABLE_UDMA_SEL_REG8			= 14,
	AL_ETH_CTRL_TABLE_UDMA_SEL_VAL_0		= 15,
};

struct al_eth_fwd_ctrl_table_entry {
	enum AL_ETH_CTRL_TABLE_PRIO_SEL		prio_sel;
	enum AL_ETH_CTRL_TABLE_QUEUE_SEL_1	queue_sel_1;  
	enum AL_ETH_CTRL_TABLE_QUEUE_SEL_2	queue_sel_2;  
	enum AL_ETH_CTRL_TABLE_UDMA_SEL		udma_sel;
	al_bool 	filter;  
};
 
int al_eth_ctrl_table_def_set(struct al_hal_eth_adapter *adapter,
			      al_bool use_table,
			      struct al_eth_fwd_ctrl_table_entry *entry);

int al_eth_ctrl_table_set(struct al_hal_eth_adapter *adapter,
			  struct al_eth_fwd_ctrl_table_index *index,
			  struct al_eth_fwd_ctrl_table_entry *entry);

int al_eth_ctrl_table_raw_set(struct al_hal_eth_adapter *adapter, uint32_t idx, uint32_t entry);
int al_eth_ctrl_table_def_raw_set(struct al_hal_eth_adapter *adapter, uint32_t val);

int al_eth_hash_key_set(struct al_hal_eth_adapter *adapter, uint32_t idx, uint32_t val);

struct al_eth_fwd_mac_table_entry {
	uint8_t		addr[6];  
	uint8_t		mask[6];
	al_bool		tx_valid;
	uint8_t		tx_target;
	al_bool		rx_valid;
	uint8_t		udma_mask;  
	uint8_t		qid;  
	al_bool 	filter;  
};

int al_eth_fwd_mac_table_set(struct al_hal_eth_adapter *adapter, uint32_t idx,
				struct al_eth_fwd_mac_table_entry *entry);

int al_eth_fwd_mac_addr_raw_set(struct al_hal_eth_adapter *adapter, uint32_t idx,
				uint32_t addr_lo, uint32_t addr_hi, uint32_t mask_lo, uint32_t mask_hi);
int al_eth_fwd_mac_ctrl_raw_set(struct al_hal_eth_adapter *adapter, uint32_t idx, uint32_t ctrl);

int al_eth_mac_addr_store(void * __iomem ec_base, uint32_t idx, uint8_t *addr);
int al_eth_mac_addr_read(void * __iomem ec_base, uint32_t idx, uint8_t *addr);

int al_eth_fwd_pbits_table_set(struct al_hal_eth_adapter *adapter, uint32_t idx, uint8_t prio);

int al_eth_fwd_priority_table_set(struct al_hal_eth_adapter *adapter, uint8_t prio, uint8_t qid);

int al_eth_fwd_dscp_table_set(struct al_hal_eth_adapter *adapter, uint32_t idx, uint8_t prio);

int al_eth_fwd_tc_table_set(struct al_hal_eth_adapter *adapter, uint32_t idx, uint8_t prio);

int al_eth_fwd_mhash_table_set(struct al_hal_eth_adapter *adapter, uint32_t idx, uint8_t udma_mask, uint8_t qid);

struct al_eth_fwd_vid_table_entry {
	uint8_t	control:1;  
	uint8_t filter:1;  
	uint8_t udma_mask:4;  
};

int al_eth_fwd_vid_config_set(struct al_hal_eth_adapter *adapter, al_bool use_table,
			      struct al_eth_fwd_vid_table_entry *default_entry,
			      uint32_t default_vlan);
 
int al_eth_fwd_vid_table_set(struct al_hal_eth_adapter *adapter, uint32_t idx,
			     struct al_eth_fwd_vid_table_entry *entry);

int al_eth_fwd_default_udma_config(struct al_hal_eth_adapter *adapter, uint32_t idx,
				   uint8_t udma_mask);

int al_eth_fwd_default_queue_config(struct al_hal_eth_adapter *adapter, uint32_t idx,
				   uint8_t qid);

int al_eth_fwd_default_priority_config(struct al_hal_eth_adapter *adapter, uint32_t idx,
				   uint8_t prio);

#define AL_ETH_RFW_FILTER_UNDET_MAC          (1 << 0)
 
#define AL_ETH_RFW_FILTER_DET_MAC            (1 << 1)
 
#define AL_ETH_RFW_FILTER_TAGGED             (1 << 2)
 
#define AL_ETH_RFW_FILTER_UNTAGGED           (1 << 3)
 
#define AL_ETH_RFW_FILTER_BC                 (1 << 4)
 
#define AL_ETH_RFW_FILTER_MC                 (1 << 5)
 
#define AL_ETH_RFW_FILTER_VLAN_VID           (1 << 7)
 
#define AL_ETH_RFW_FILTER_CTRL_TABLE         (1 << 8)
 
#define AL_ETH_RFW_FILTER_PROT_INDEX         (1 << 9)
 
#define AL_ETH_RFW_FILTER_WOL		     (1 << 10)

struct al_eth_filter_params {
	al_bool		enable;
	uint32_t	filters;  
	al_bool		filter_proto[AL_ETH_PROTOCOLS_NUM];  
};

struct al_eth_filter_override_params {
	uint32_t	filters;  
	uint8_t		udma;  
	uint8_t		qid;  
};

int al_eth_filter_config(struct al_hal_eth_adapter *adapter, struct al_eth_filter_params *params);

int al_eth_filter_override_config(struct al_hal_eth_adapter *adapter,
				  struct al_eth_filter_override_params *params);

int al_eth_switching_config_set(struct al_hal_eth_adapter *adapter, uint8_t udma_id, uint8_t forward_all_to_mac, uint8_t enable_int_switching,
					enum al_eth_tx_switch_vid_sel_type vid_sel_type,
					enum al_eth_tx_switch_dec_type uc_dec,
					enum al_eth_tx_switch_dec_type mc_dec,
					enum al_eth_tx_switch_dec_type bc_dec);
int al_eth_switching_default_bitmap_set(struct al_hal_eth_adapter *adapter, uint8_t udma_id, uint8_t udma_uc_bitmask,
						uint8_t udma_mc_bitmask,uint8_t udma_bc_bitmask);
int al_eth_flow_control_config(struct al_hal_eth_adapter *adapter, struct al_eth_flow_control_params *params);

struct al_eth_eee_params{
	uint8_t enable;
	uint32_t tx_eee_timer;  
	uint32_t min_interval;  
};

int al_eth_eee_config(struct al_hal_eth_adapter *adapter, struct al_eth_eee_params *params);

int al_eth_eee_get(struct al_hal_eth_adapter *adapter, struct al_eth_eee_params *params);

int al_eth_vlan_mod_config(struct al_hal_eth_adapter *adapter, uint8_t udma_id, uint16_t udma_etype, uint16_t vlan1_data, uint16_t vlan2_data);

int al_eth_ts_init(struct al_hal_eth_adapter *adapter);

#define AL_ETH_PTH_TX_SAMPLES_NUM	16

int al_eth_tx_ts_val_get(struct al_hal_eth_adapter *adapter, uint8_t ts_index,
			 uint32_t *timestamp);

struct al_eth_pth_time {
	uint32_t	seconds;  
	uint64_t	femto;  
};

int al_eth_pth_systime_read(struct al_hal_eth_adapter *adapter,
			    struct al_eth_pth_time *systime);

int al_eth_pth_clk_period_write(struct al_hal_eth_adapter *adapter,
				uint64_t clk_period);

enum al_eth_pth_update_method {
	AL_ETH_PTH_UPDATE_METHOD_SET = 0,  
	AL_ETH_PTH_UPDATE_METHOD_INC = 1,  
	AL_ETH_PTH_UPDATE_METHOD_DEC = 2,  
	AL_ETH_PTH_UPDATE_METHOD_ADD_TO_LAST = 3,  
};

enum al_eth_pth_int_trig {
	AL_ETH_PTH_INT_TRIG_OUT_PULSE_0 = 0,  
	AL_ETH_PTH_INT_TRIG_REG_WRITE = 1,  
};

struct al_eth_pth_int_update_params {
	al_bool		enable;  
	enum al_eth_pth_update_method	method;  
	enum al_eth_pth_int_trig	trigger;  
};

int al_eth_pth_int_update_config(struct al_hal_eth_adapter *adapter,
				 struct al_eth_pth_int_update_params *params);

int al_eth_pth_int_update_time_set(struct al_hal_eth_adapter *adapter,
				   struct al_eth_pth_time *time);

struct al_eth_pth_ext_update_params {
	uint8_t		triggers;  
	enum al_eth_pth_update_method	method;  
};

int al_eth_pth_ext_update_config(struct al_hal_eth_adapter *adapter,
				 struct al_eth_pth_ext_update_params *params);

int al_eth_pth_ext_update_time_set(struct al_hal_eth_adapter *adapter,
				   struct al_eth_pth_time *time);
 
int al_eth_pth_read_compensation_set(struct al_hal_eth_adapter *adapter,
				     uint64_t subseconds);
 
int al_eth_pth_int_write_compensation_set(struct al_hal_eth_adapter *adapter,
					  uint64_t subseconds);

int al_eth_pth_ext_write_compensation_set(struct al_hal_eth_adapter *adapter,
					  uint64_t subseconds);

int al_eth_pth_sync_compensation_set(struct al_hal_eth_adapter *adapter,
				     uint64_t subseconds);

#define AL_ETH_PTH_PULSE_OUT_NUM	8
struct al_eth_pth_pulse_out_params {
	uint8_t		index;  
	al_bool		enable;
	al_bool		periodic;  
	uint8_t		period_sec;  
	uint32_t	period_us;  
	struct al_eth_pth_time	start_time;  
	uint64_t	pulse_width;  
};

int al_eth_pth_pulse_out_config(struct al_hal_eth_adapter *adapter,
				struct al_eth_pth_pulse_out_params *params);

struct al_eth_link_status {
	al_bool		link_up;
};

int al_eth_link_status_get(struct al_hal_eth_adapter *adapter, struct al_eth_link_status *status);

#ifdef CONFIG_SYNO_ALPINE_V2_5_3
 
int al_eth_led_set(struct al_hal_eth_adapter *adapter, al_bool link_is_up);
#else
 
  int al_eth_led_config(struct al_hal_eth_adapter *adapter, al_bool link_is_up);
#endif

struct al_eth_mac_stats{
	uint64_t aOctetsReceivedOK;
	uint64_t aOctetsTransmittedOK;
	uint32_t etherStatsPkts;
	uint32_t ifInUcastPkts;
	uint32_t ifInMulticastPkts;
	uint32_t ifInBroadcastPkts;
	uint32_t ifInErrors;

	uint32_t ifOutUcastPkts;
	uint32_t ifOutMulticastPkts;
	uint32_t ifOutBroadcastPkts;
	uint32_t ifOutErrors;

	uint32_t aFramesReceivedOK;
	uint32_t aFramesTransmittedOK;
	uint32_t etherStatsUndersizePkts;
	uint32_t etherStatsFragments;
	uint32_t etherStatsJabbers;
	uint32_t etherStatsOversizePkts;
	uint32_t aFrameCheckSequenceErrors;
	uint32_t aAlignmentErrors;
	uint32_t etherStatsDropEvents;

	uint32_t eee_in;
	uint32_t eee_out;
};

int al_eth_mac_stats_get(struct al_hal_eth_adapter *adapter, struct al_eth_mac_stats *stats);

int al_eth_flr_rmn(int (* pci_read_config_u32)(void *handle, int where, uint32_t *val),
		   int (* pci_write_config_u32)(void *handle, int where, uint32_t val),
		   void *handle,
		   void __iomem	*mac_base);

enum al_eth_board_media_type {
	AL_ETH_BOARD_MEDIA_TYPE_AUTO_DETECT		= 0,
	AL_ETH_BOARD_MEDIA_TYPE_RGMII			= 1,
	AL_ETH_BOARD_MEDIA_TYPE_10GBASE_SR		= 2,
	AL_ETH_BOARD_MEDIA_TYPE_SGMII			= 3,
	AL_ETH_BOARD_MEDIA_TYPE_1000BASE_X		= 4,
	AL_ETH_BOARD_MEDIA_TYPE_AUTO_DETECT_AUTO_SPEED	= 5,
};

enum al_eth_board_mdio_freq {
	AL_ETH_BOARD_MDIO_FREQ_2_5_MHZ	= 0,
	AL_ETH_BOARD_MDIO_FREQ_1_MHZ	= 1,
};

enum al_eth_board_ext_phy_if {
	AL_ETH_BOARD_PHY_IF_MDIO	= 0,
	AL_ETH_BOARD_PHY_IF_XMDIO	= 1,
	AL_ETH_BOARD_PHY_IF_I2C		= 2,

};

enum al_eth_board_auto_neg_mode {
	AL_ETH_BOARD_AUTONEG_OUT_OF_BAND	= 0,
	AL_ETH_BOARD_AUTONEG_IN_BAND		= 1,

};
#ifdef CONFIG_SYNO_ALPINE_A0
 
enum al_eth_board_1g_speed {
	AL_ETH_BOARD_1G_SPEED_1000M		= 0,
	AL_ETH_BOARD_1G_SPEED_100M		= 1,
	AL_ETH_BOARD_1G_SPEED_10M		= 2,
};
#endif

#ifdef CONFIG_SYNO_ALPINE_V2_5_3
enum al_eth_retimer_channel {
	AL_ETH_RETIMER_CHANNEL_A		= 0,
	AL_ETH_RETIMER_CHANNEL_B		= 1,
};
#endif

struct al_eth_board_params {
	enum al_eth_board_media_type	media_type;
	al_bool		phy_exist;  
	uint8_t		phy_mdio_addr;  
	al_bool		sfp_plus_module_exist;  
	al_bool		autoneg_enable;  
	al_bool		kr_lt_enable;  
	al_bool		kr_fec_enable;  
	enum al_eth_board_mdio_freq	mdio_freq;  
	uint8_t		i2c_adapter_id;  
	enum al_eth_board_ext_phy_if	phy_if;  
	enum al_eth_board_auto_neg_mode	an_mode;  
	uint8_t		serdes_grp;  
	uint8_t		serdes_lane;  
	enum al_eth_ref_clk_freq	ref_clk_freq;  
#ifdef CONFIG_SYNO_ALPINE_A0
	al_bool		dont_override_serdes;  
	al_bool		force_1000_base_x;  
	al_bool		an_disable;  
	enum al_eth_board_1g_speed	speed;  
	al_bool		half_duplex;  
	al_bool		fc_disable;  
#endif
#ifdef CONFIG_SYNO_ALPINE_V2_5_3
	al_bool		retimer_exist;  
	uint8_t		retimer_bus_id;  
	uint8_t		retimer_i2c_addr;  
	enum al_eth_retimer_channel retimer_channel;  
	al_bool		dac;  
	uint8_t		dac_len;  
#endif
};

int al_eth_board_params_set(void * __iomem mac_base, struct al_eth_board_params *params);

int al_eth_board_params_get(void * __iomem mac_base, struct al_eth_board_params *params);

#define AL_ETH_WOL_INT_UNICAST		AL_BIT(0)
 
#define AL_ETH_WOL_INT_MULTICAST	AL_BIT(1)
 
#define AL_ETH_WOL_INT_BROADCAST	AL_BIT(2)
 
#define AL_ETH_WOL_INT_IPV4		AL_BIT(3)
 
#define AL_ETH_WOL_INT_IPV6		AL_BIT(4)
 
#define AL_ETH_WOL_INT_ETHERTYPE_DA	AL_BIT(5)
 
#define AL_ETH_WOL_INT_ETHERTYPE_BC	AL_BIT(6)
 
#define AL_ETH_WOL_INT_PARSER		AL_BIT(7)
 
#define AL_ETH_WOL_INT_MAGIC		AL_BIT(8)
 
#define AL_ETH_WOL_INT_MAGIC_PSWD	AL_BIT(9)

#define AL_ETH_WOL_FWRD_UNICAST		AL_BIT(0)
 
#define AL_ETH_WOL_FWRD_MULTICAST	AL_BIT(1)
 
#define AL_ETH_WOL_FWRD_BROADCAST	AL_BIT(2)
 
#define AL_ETH_WOL_FWRD_IPV4		AL_BIT(3)
 
#define AL_ETH_WOL_FWRD_IPV6		AL_BIT(4)
 
#define AL_ETH_WOL_FWRD_ETHERTYPE_DA	AL_BIT(5)
 
#define AL_ETH_WOL_FWRD_ETHERTYPE_BC	AL_BIT(6)
 
#define AL_ETH_WOL_FWRD_PARSER		AL_BIT(7)

struct al_eth_wol_params {
	uint8_t *dest_addr;  
	uint8_t *pswd;  
	uint8_t *ipv4;  
	uint8_t *ipv6;  
	uint16_t ethr_type1;  
	uint16_t ethr_type2;  
	uint16_t forward_mask;  
	uint16_t int_mask;  
};

int al_eth_wol_enable(
		struct al_hal_eth_adapter *adapter,
		struct al_eth_wol_params *wol);

int al_eth_wol_disable(
		struct al_hal_eth_adapter *adapter);

#ifdef __cplusplus
}
#endif
 
#endif		 
 