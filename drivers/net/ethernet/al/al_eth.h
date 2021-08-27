#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef AL_ETH_H
#define AL_ETH_H

#include <linux/interrupt.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/inetdevice.h>

#ifdef MY_DEF_HERE
#include <linux/syno.h>
#endif

#include "al_hal_eth.h"
#include "al_hal_udma_iofic.h"
#include "al_hal_udma_debug.h"
#ifdef CONFIG_SYNO_ALPINE_A0
#include "al_init_eth_lm.h"
#endif
#include "al_hal_serdes.h"

enum board_t {
	ALPINE = 0,
	ALPINE_NIC = 1,
};

#define AL_ETH_MAX_HW_QUEUES	4
#define AL_ETH_NUM_QUEUES	4
#define AL_ETH_MAX_MSIX_VEC	(1 + 2 * AL_ETH_MAX_HW_QUEUES)

#define AL_ETH_DEFAULT_TX_SW_DESCS	(512)
#define AL_ETH_DEFAULT_TX_HW_DESCS	(512)
#define AL_ETH_DEFAULT_RX_DESCS		(512)

#if ((AL_ETH_DEFAULT_TX_SW_DESCS / 4) < (MAX_SKB_FRAGS + 2))
#define AL_ETH_TX_WAKEUP_THRESH		(AL_ETH_DEFAULT_TX_SW_DESCS / 4)
#else
#define AL_ETH_TX_WAKEUP_THRESH		(MAX_SKB_FRAGS + 2)
#endif
#define AL_ETH_DEFAULT_SMALL_PACKET_LEN		(128 - NET_IP_ALIGN)
#define AL_ETH_HEADER_COPY_SIZE			(128 - NET_IP_ALIGN)

#define AL_ETH_DEFAULT_MAX_RX_BUFF_ALLOC_SIZE 1536
 
#define AL_ETH_DEFAULT_MIN_RX_BUFF_ALLOC_SIZE 600
#ifdef CONFIG_SYNO_ALPINE_A0
#define AL_ETH_DEFAULT_FORCE_1000_BASEX AL_FALSE
#endif

#define AL_ETH_DEFAULT_LINK_POLL_INTERVAL 100
#define AL_ETH_FIRST_LINK_POLL_INTERVAL 1

#define AL_ETH_NAME_MAX_LEN	20
#define AL_ETH_IRQNAME_SIZE	40

#define AL_ETH_DEFAULT_MDIO_FREQ_KHZ	2500

struct al_eth_irq {
	irq_handler_t	handler;
	void		*data;
	unsigned int	vector;
	u8		requested;
	cpumask_t	affinity_hint_mask;
	char		name[AL_ETH_IRQNAME_SIZE];
};

struct al_eth_napi {
	struct napi_struct	napi		____cacheline_aligned;
	struct al_eth_adapter	*adapter;
	unsigned int qid;
};

struct al_eth_tx_buffer {
	struct sk_buff *skb;
	struct al_eth_pkt hal_pkt;
	struct al_eth_meta_data hal_meta;
	unsigned int	tx_descs;
};

struct al_eth_rx_buffer {
	struct sk_buff *skb;
	struct page *page;
	unsigned int page_offset;
#if defined(CONFIG_AL_ETH_ALLOC_FRAG)
	u8 *data;
	unsigned int data_size;
	unsigned int frag_size;  
#endif
	DEFINE_DMA_UNMAP_ADDR(dma);
	struct al_buf	al_buf;
};

#define AL_ETH_RX_OFFSET	NET_SKB_PAD + NET_IP_ALIGN

struct al_eth_ring {
	struct device *dev;
	struct napi_struct	*napi;
	struct al_eth_pkt hal_pkt;  
	struct al_udma_q *dma_q;  
	u16 next_to_use;
	u16 next_to_clean;
	u32 __iomem *unmask_reg_offset;  
	u32	unmask_val;  
	 
	struct al_eth_tx_buffer *tx_buffer_info;  
	struct al_eth_rx_buffer *rx_buffer_info;  
	int	sw_count;  
	int	hw_count;  
	size_t	descs_size;  
	size_t	cdescs_size;  

	struct net_device *netdev;
	struct al_udma_q_params	q_params;
};

#define AL_ETH_TX_RING_IDX_NEXT(tx_ring, idx) (((idx) + 1) & (AL_ETH_DEFAULT_TX_SW_DESCS - 1))

#define AL_ETH_RX_RING_IDX_NEXT(rx_ring, idx) (((idx) + 1) & (AL_ETH_DEFAULT_RX_DESCS - 1))
#define AL_ETH_RX_RING_IDX_ADD(rx_ring, idx, n) (((idx) + (n)) & (AL_ETH_DEFAULT_RX_DESCS - 1))

#define AL_ETH_FLOW_CTRL_RX_FIFO_TH_HIGH	0x160
#define AL_ETH_FLOW_CTRL_RX_FIFO_TH_LOW		0x90
#define AL_ETH_FLOW_CTRL_QUANTA			0xffff
#define AL_ETH_FLOW_CTRL_QUANTA_TH		0x8000

#define AL_ETH_FLOW_CTRL_AUTONEG  BIT(0)
#define AL_ETH_FLOW_CTRL_RX_PAUSE BIT(1)
#define AL_ETH_FLOW_CTRL_TX_PAUSE BIT(2)

#ifdef CONFIG_SYNO_ALPINE_A0
 
#else
 
#endif
struct al_eth_link_config {
	int old_link;
	 
	int	active_duplex;
	int	active_speed;

	uint8_t flow_ctrl_active;
	 
	uint8_t flow_ctrl_supported;
#ifdef CONFIG_SYNO_ALPINE_A0
	 
	bool	force_1000_base_x;
	bool	autoneg;
#endif
};

enum al_eth_sfp_detect_evt {
	 
	AL_ETH_SFP_DETECT_EVT_NO_CHANGE,
	 
	AL_ETH_SFP_DETECT_EVT_CONNECTED,
	 
	AL_ETH_SFP_DETECT_EVT_DISCONNECTED,
	 
	AL_ETH_SFP_DETECT_EVT_CHANGED,
};

struct al_eth_sfp_detect_stat {
	 
	bool			valid;
	bool			connected;
	uint8_t			sfp_10g;
	uint8_t			sfp_1g;
	uint8_t			sfp_cable_tech;

	bool			lt_en;
	bool			an_en;
	enum al_eth_mac_mode	mac_mode;
};

#ifdef CONFIG_SYNO_ALPINE_V2_5_3
 
struct al_eth_retimer_params {
	al_bool				exist;
	uint8_t				bus_id;
	uint8_t				i2c_addr;
	enum al_eth_retimer_channel	channel;
};
#endif

struct al_eth_adapter {
	 
	struct net_device *netdev;
	struct pci_dev *pdev;
	enum board_t	board_type;
	u16 dev_id;
	u8 rev_id;

	u32 flags;
#define AL_ETH_FLAG_MSIX_CAPABLE		(u32)(1 << 1)
#define AL_ETH_FLAG_MSIX_ENABLED		(u32)(1 << 2)
#define AL_ETH_FLAG_IN_NETPOLL			(u32)(1 << 3)
#define AL_ETH_FLAG_MQ_CAPABLE			(u32)(1 << 4)
#define AL_ETH_FLAG_SRIOV_CAPABLE		(u32)(1 << 5)
#define AL_ETH_FLAG_SRIOV_ENABLED		(u32)(1 << 6)
#define AL_ETH_FLAG_RESET_REQUESTED		(u32)(1 << 7)

	struct al_hal_eth_adapter hal_adapter;

	unsigned int small_copy_len;

	unsigned int max_rx_buff_alloc_size;

	int num_tx_queues;

	int num_rx_queues;

	struct al_eth_ring tx_ring[AL_ETH_NUM_QUEUES] ____cacheline_aligned_in_smp;

	struct al_eth_ring rx_ring[AL_ETH_NUM_QUEUES];

#define AL_ETH_RXQ_NAPI_IDX(adapter, q)	(q)
#define AL_ETH_TXQ_NAPI_IDX(adapter, q)	((adapter)->num_rx_queues + (q))
	struct al_eth_napi al_napi[2 * AL_ETH_NUM_QUEUES];

	enum al_iofic_mode int_mode;

#define AL_ETH_MGMT_IRQ_IDX		0
#define AL_ETH_RXQ_IRQ_IDX(adapter, q)	(1 + (q))
#define AL_ETH_TXQ_IRQ_IDX(adapter, q)	(1 + (adapter)->num_rx_queues + (q))
	struct al_eth_irq irq_tbl[AL_ETH_MAX_MSIX_VEC];
	struct msix_entry *msix_entries;
	int	msix_vecs;
	int	irq_vecs;

	unsigned int tx_usecs, rx_usecs;  

	unsigned int tx_ring_count;
	unsigned int tx_descs_count;
	unsigned int rx_ring_count;
	unsigned int rx_descs_count;

	uint32_t toeplitz_hash_key[AL_ETH_RX_HASH_KEY_NUM];
#define AL_ETH_RX_RSS_TABLE_SIZE	AL_ETH_RX_THASH_TABLE_SIZE
	uint8_t	 rss_ind_tbl[AL_ETH_RX_RSS_TABLE_SIZE];

	uint32_t msg_enable;
	struct al_eth_mac_stats mac_stats;

	enum al_eth_mac_mode	mac_mode;
	bool			mac_mode_set;  
	u8 mac_addr[ETH_ALEN];
	 
	bool			phy_exist;
	struct mii_bus		*mdio_bus;
	struct phy_device	*phydev;
	uint8_t			phy_addr;
	struct al_eth_link_config	link_config;

	int	id_number;
	char	name[AL_ETH_NAME_MAX_LEN];
	void __iomem	*udma_base;
	void __iomem	*ec_base;
	void __iomem	*mac_base;

	struct al_eth_flow_control_params flow_ctrl_params;

	struct al_eth_adapter_params eth_hal_params;

	struct delayed_work	link_status_task;
	uint32_t		link_poll_interval;  
#ifdef SYNO_ALPINE_SUPPORT_WOL
	uint32_t		wol_opts;
#endif

	bool			serdes_init;
	struct al_serdes_obj	serdes_obj;
	uint8_t			serdes_grp;
	uint8_t			serdes_lane;

	bool			an_en;	 
	bool			lt_en;	 
#ifdef MY_DEF_HERE
	bool			rx_eq_en;
#endif

#ifdef CONFIG_SYNO_ALPINE_A0
 
#else
	struct al_eth_link_status last_link_status;

	bool			lt_completed;
	uint32_t		lt_failures;
#endif

	al_bool		sfp_detection_needed;  
	bool		auto_speed;  
	uint8_t		i2c_adapter_id;  
	enum al_eth_ref_clk_freq	ref_clk_freq;  
	unsigned int	mdio_freq;  

	bool up;

#ifdef CONFIG_SYNO_ALPINE_A0
	bool				last_link;
	bool				last_establish_failed;
	struct al_eth_lm_context	lm_context;

	bool				dont_override_serdes;  
	spinlock_t			serdes_config_lock;
#else
	struct al_eth_sfp_detect_stat sfp_detect_status;
#endif
#ifdef CONFIG_SYNO_ALPINE_V2_5_3
	struct al_eth_retimer_params    retimer;
#endif
};

#endif  
