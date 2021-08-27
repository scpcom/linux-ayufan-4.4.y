#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 



#ifndef LINUX_MV_NETA_H
#define LINUX_MV_NETA_H

#if defined(CONFIG_SYNO_ARMADA_ARCH)
struct netaSmpGroupStruct {
	MV_U32 portMask;
	MV_U32 cpuMask;
};
#endif

#if defined(CONFIG_SYNO_ARMADA_ARCH_V2)
#define MV_NETA_PORT_NAME	"mv_neta_port"
struct mv_neta_pdata {
	 
	unsigned int  tclk;
	unsigned int  pclk;
	int           max_port;
	int           max_cpu;
	unsigned int  ctrl_model;
	unsigned int  ctrl_rev;

	unsigned int  cpu_mask;
	int           mtu;

	
	int      phy_addr;
#if defined(CONFIG_SYNO_ARMADA_ARCH_V2)
	 
	int      tx_csum_limit;
#endif

	u8       mac_addr[6];

	int      speed;
	int      duplex;

	
	int      lb_enable;
	int      is_sgmii;
	int      is_rgmii;
#if defined(CONFIG_SYNO_ARMADA_ARCH_V2)
	 
	int      irq;
#endif

	int      rx_queue_count;
	int      tx_queue_count;

	int      rx_queue_size;
	int      tx_queue_size;
};
#endif

#endif   
