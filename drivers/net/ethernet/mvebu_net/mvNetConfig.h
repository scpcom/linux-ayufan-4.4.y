#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __mv_net_config_h__
#define __mv_net_config_h__

#define MV_ETH_MAX_PORTS                4

#if defined(CONFIG_MV_PP3)
#define INTER_REGS_PHYS_BASE		0xF1000000
#define INTER_REGS_SIZE			0x1000000
#endif

#if defined(CONFIG_MV_ETH_PP2) || defined(CONFIG_MV_ETH_PP2_MODULE)

#define INTER_REGS_PHYS_BASE		0xF1000000
#define INTER_REGS_VIRT_BASE		0xFBC00000

#define PP2_CPU0_VIRT_BASE		0xFEC00000
#define PP2_CPU1_VIRT_BASE		0xFEC10000

#define MV_PP2_PON_EXIST
#define MV_PP2_PON_PORT_ID              7
#define MV_PP2_MAX_RXQ                  16       
#define MV_PP2_MAX_TXQ                  8
#define MV_PP2_RXQ_TOTAL_NUM            32       
#define MV_PP2_MAX_TCONT                16       
#define MV_PP2_TX_CSUM_MAX_SIZE         1790

#define IRQ_GLOBAL_GOP			82  
#define IRQ_GLOBAL_NET_WAKE_UP		112  

#define MV_ETH_TCLK			(166666667)

#ifdef CONFIG_OF
extern int pp2_vbase;
extern int eth_vbase;
extern int pp2_port_vbase[MV_ETH_MAX_PORTS];
#define MV_PP2_REG_BASE                 (pp2_vbase)
#define MV_ETH_BASE_ADDR                (eth_vbase)
#define GOP_REG_BASE(port)		(pp2_port_vbase[port])
#else
#define MV_PP2_REG_BASE                 (0xF0000)
#define MV_ETH_BASE_ADDR                (0xC0000)
#define GOP_REG_BASE(port)		(MV_ETH_BASE_ADDR + 0x4000 + ((port) / 2) * 0x3000 + ((port) % 2) * 0x1000)
#endif

#define LMS_REG_BASE                    (MV_ETH_BASE_ADDR)
#define MIB_COUNTERS_REG_BASE           (MV_ETH_BASE_ADDR + 0x1000)
#define GOP_MNG_REG_BASE                (MV_ETH_BASE_ADDR + 0x3000)
#define MV_PON_REGS_OFFSET              (MV_ETH_BASE_ADDR + 0x8000)

#endif  

#if defined(CONFIG_MV_ETH_NETA) || defined(CONFIG_MV_ETH_NETA_MODULE)

#define MV_PON_PORT(p)			MV_FALSE
#define MV_ETH_MAX_TCONT		1

#define MV_ETH_MAX_RXQ			8
#define MV_ETH_MAX_TXQ			8
#if defined(MY_ABC_HERE)
#define MV_ETH_TX_CSUM_MIN_SIZE		2048
#endif  
#define MV_ETH_TX_CSUM_MAX_SIZE		9800
#define MV_PNC_TCAM_LINES		1024     
#define MV_BM_WIN_ID		12
#define MV_PNC_WIN_ID		11
#define MV_BM_WIN_ATTR		0x4
#define MV_PNC_WIN_ATTR		0x4

#define MV_ETH_GMAC_NEW
 
#define MV_ETH_WRR_NEW
 
#define MV_ETH_LEGACY_PARSER_IPV6
#define MV_ETH_PNC_NEW
#define MV_ETH_PNC_LB

#endif  

#endif  
