#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __STMMAC_PLATFORM_DATA
#define __STMMAC_PLATFORM_DATA

#include <linux/platform_device.h>

#define STMMAC_RX_COE_NONE	0
#define STMMAC_RX_COE_TYPE1	1
#define STMMAC_RX_COE_TYPE2	2

#define	STMMAC_CSR_60_100M	0x0	 
#define	STMMAC_CSR_100_150M	0x1	 
#define	STMMAC_CSR_20_35M	0x2	 
#define	STMMAC_CSR_35_60M	0x3	 
#define	STMMAC_CSR_150_250M	0x4	 
#define	STMMAC_CSR_250_300M	0x5	 

#define STMMAC_CSR_I_4		0x8	 
#define STMMAC_CSR_I_6		0x9	 
#define STMMAC_CSR_I_8		0xA	 
#define STMMAC_CSR_I_10		0xB	 
#define STMMAC_CSR_I_12		0xC	 
#define STMMAC_CSR_I_14		0xD	 
#define STMMAC_CSR_I_16		0xE	 
#define STMMAC_CSR_I_18		0xF	 

#define DMA_AXI_BLEN_4		(1 << 1)
#define DMA_AXI_BLEN_8		(1 << 2)
#define DMA_AXI_BLEN_16		(1 << 3)
#define DMA_AXI_BLEN_32		(1 << 4)
#define DMA_AXI_BLEN_64		(1 << 5)
#define DMA_AXI_BLEN_128	(1 << 6)
#define DMA_AXI_BLEN_256	(1 << 7)
#define DMA_AXI_BLEN_ALL (DMA_AXI_BLEN_4 | DMA_AXI_BLEN_8 | DMA_AXI_BLEN_16 \
			| DMA_AXI_BLEN_32 | DMA_AXI_BLEN_64 \
			| DMA_AXI_BLEN_128 | DMA_AXI_BLEN_256)

struct stmmac_mdio_bus_data {
	int (*phy_reset)(void *priv);
	unsigned int phy_mask;
	int *irqs;
	int probed_phy_irq;
#if defined (MY_DEF_HERE)
#ifdef CONFIG_OF
	int reset_gpio, active_low;
	u32 delays[3];
#endif
#endif  
};

struct stmmac_dma_cfg {
	int pbl;
	int fixed_burst;
	int mixed_burst;
	int burst_len;
};

struct plat_stmmacenet_data {
	char *phy_bus_name;
	int bus_id;
	int phy_addr;
	int interface;
	struct stmmac_mdio_bus_data *mdio_bus_data;
	struct stmmac_dma_cfg *dma_cfg;
	int clk_csr;
	int has_gmac;
	int enh_desc;
	int tx_coe;
	int rx_coe;
	int bugged_jumbo;
	int pmt;
	int force_sf_dma_mode;
#if defined (MY_DEF_HERE)
	int force_thresh_dma_mode;
	int riwt_off;
	int max_speed;
	int maxmtu;
#else  
	int riwt_off;
#endif  
	void (*fix_mac_speed)(void *priv, unsigned int speed);
	void (*bus_setup)(void __iomem *ioaddr);
#if defined (MY_DEF_HERE)
	void *(*setup)(struct platform_device *pdev);
	void (*free)(struct platform_device *pdev, void *priv);
	int (*init)(struct platform_device *pdev, void *priv);
	void (*exit)(struct platform_device *pdev, void *priv);
#else  
	int (*init)(struct platform_device *pdev);
	void (*exit)(struct platform_device *pdev);
#endif  
	void *custom_cfg;
	void *custom_data;
	void *bsp_priv;
#if defined (MY_DEF_HERE)
	int eee_force_disable;
#endif  
};
#if defined (MY_DEF_HERE)

struct stmmac_of_data {
	int has_gmac;
	int enh_desc;
	int tx_coe;
	int rx_coe;
	int bugged_jumbo;
	int pmt;
	int riwt_off;
	void (*fix_mac_speed)(void *priv, unsigned int speed);
	void (*bus_setup)(void __iomem *ioaddr);
	void *(*setup)(struct platform_device *pdev);
	void (*free)(struct platform_device *pdev, void *priv);
	int (*init)(struct platform_device *pdev, void *priv);
	void (*exit)(struct platform_device *pdev, void *priv);
};
#endif  
#endif
