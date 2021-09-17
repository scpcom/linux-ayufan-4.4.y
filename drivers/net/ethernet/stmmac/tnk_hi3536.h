#define SYSCTL_BASE		0x12040000
#define TOE_CLK_SRST		0x000000c8
#define TOE_MAC_INTERFACE	0x000000ec

#define MUXCTL_BASE		IO_ADDRESS(0x120f0000)
#define MUXCTL_REG16_OFFSET	(0x040)
#define MUXCTL_REG34_OFFSET	(0x088)
#define MUXCTL_PHY_MASK		0x3

#define TOE_CLK_EXT_PHY0_RST_BIT		13
#define TOE_CLK_EXT_PHY1_RST_BIT		14

#define REG_GPIO_DIR		0x400

#define TOE_CLK_DEF_250M	(1 << 10)

#define TOE_MAX_RETRY_NUM	0x0000000f
#define TOE_DEFAULT_CLK_250M	250000000
#define TOE_DEFAULT_CLK_150M	150000000

#define HW_REG(a) (*(unsigned long *)(a))
#define REG_CRG0_OFFSET    IO_ADDRESS(SYSCTL_BASE + 0x0)

unsigned long tnk_clk_init(void);
unsigned long mdio_clk_init(void);
int get_clk_csr(unsigned long tnkclk);
void reset_mac_interface_dual(int id, void *addr, u32 val);
void stmmac_external_phy_reset(int port_id, void *syscfg_addr);
