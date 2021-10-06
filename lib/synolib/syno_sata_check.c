#include <linux/init.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/pci.h>
#include <linux/syno.h>

MODULE_LICENSE("Proprietary");

const unsigned jmb_port_addr[3][5] = {{0x74, 0x76, 0x78, 0x7A, 0x7C},
                                      {0x73, 0x75, 0x77, 0x79, 0x7B},
                                      {0x04, 0x11, 0x1e, 0x2b, 0x38}};

static void syno_jmb_read_phy_reg(void __iomem *bar5, u32 addr, u32 *data, unsigned long sata)
{
	// Index port has 24 bits, PHY registers access uses bit[12:0] and bit[18], bit[18] is used to select:
	//   0: PCIe PHY registers.
	//   1: SATA PHY registers.

	// Offset C0 [IDXP] is index port register
	writel((addr & 0x01FFFUL) + (sata << 18UL), bar5 + 0xC0);

	// Offset C8 [DPHY] is data port for PCIe/SATA PHY registers access.
	*data = readl(bar5 + 0xC8);
}

static int syno_jmb_sata_check(unsigned pID, int portNum)
{
	struct pci_dev *pdev = NULL;
	void __iomem *bar5 = NULL;
	u32 value = 0;
	int i = 0;
	int j = 0;
	int iRet = -1;

	printk("======= JMicron %x =======\n", pID);
	while ((pdev = pci_get_device(0x197b, pID, pdev)) != NULL) {
		bar5 = ioremap(pci_resource_start(pdev, 5), pci_resource_len(pdev, 5));
		if (!bar5) {
			printk("Can't map jmb%x registers pci%d:%d\n", pID, pdev->bus->number, pdev->bus->primary);
			goto END;
		}

		//check pcie register
		syno_jmb_read_phy_reg(bar5, 0x1035, &value, 0);// 0x00000001 1: enable, 0: disable
		printk("pci %d:%d Preset %s\n", pdev->bus->number, pdev->bus->primary, (value & 0x00000001)?"Enable":"Disable");
		syno_jmb_read_phy_reg(bar5, 0x1034, &value, 0);// 0x00000044 preset value, lane0/1 preset = 0x4
		printk("pci %d:%d Preset Value: 0x%x\n", pdev->bus->number, pdev->bus->primary, value);
		syno_jmb_read_phy_reg(bar5, 0x28, &value, 0);  // 0x222A90F3 // PCIe impedance
		printk("pci %d:%d PCIe impedance reg: 0x%x\n", pdev->bus->number, pdev->bus->primary, value);
		syno_jmb_read_phy_reg(bar5, 0x428, &value, 0); // 0x222A90F3 // PCIe impedance
		printk("pci %d:%d PCIe impedance reg: 0x%x\n", pdev->bus->number, pdev->bus->primary, value);

		//check sata signal
		for (i = 0 ; i < portNum; i++) {
			for (j = 0 ; j < 3; j++) {
				syno_jmb_read_phy_reg(bar5, jmb_port_addr[j][i], &value, 1);
				printk("pci %d:%d port %d gen %d: %x\n",pdev->bus->number, pdev->bus->primary, i, j + 1, value);
			}
		}
		if (bar5) {
			iounmap(bar5);
			bar5 = NULL;
		}
	}
	printk("============================\n");
	iRet = 0;

END:
	return iRet;
}

const unsigned mv_port_gen[3] = {0x8D, 0x8F, 0x91};
const unsigned mv_port_addr[4] = {0x178, 0x1f8, 0x278, 0x2f8};
const unsigned mv_port_data[4] = {0x17c, 0x1fc, 0x27c, 0x2fc};

static int syno_mv_sata_check(unsigned pID, int portNum)
{
	struct pci_dev *pdev = NULL;
	void __iomem *bar5 = NULL;
	u32 value = 0;
	int i = 0;
	int j = 0;
	int iRet = -1;

	printk("======= Marvell %x =======\n", pID);
	while ((pdev = pci_get_device(0x1b4b, pID, pdev)) != NULL) {
		bar5 = ioremap(pci_resource_start(pdev, 5), pci_resource_len(pdev, 5));
		if (!bar5) {
			printk("Can't map mv%x registers pci%d:%d\n", pID, pdev->bus->number, pdev->bus->primary);
			goto END;
		}
		for (i = 0 ; i < portNum; i++) {
			for (j = 0; j < 3; j++) {
				writel(mv_port_gen[j], bar5 + mv_port_addr[i]);
				value = readl(bar5 + mv_port_data[i]);
				printk("pci %d:%d port %d gen %d: %x\n",pdev->bus->number, pdev->bus->primary, i, j + 1 ,value);
			}
		}
		if (bar5) {
			iounmap(bar5);
			bar5 = NULL;
		}
	}
	printk("============================\n");
	iRet = 0;

END:
	return iRet;
}

const unsigned asmedia_gen[3] = {0x4, 0x5, 0x6};
const unsigned asmedia_addr[2] = {0xCA0, 0xDA0};

static int syno_asm_sata_check(unsigned pID, int portNum)
{
	struct pci_dev *pdev = NULL;
	void __iomem *bar5 = NULL;
	u8 value = 0;
	unsigned int devfn = 0;
	int i = 0;
	int j = 0;
	int iRet = -1;

	devfn = PCI_DEVFN(0x00, 0x0);
	printk("======= Asmedia 1061 =======\n");
	while ((pdev = pci_get_device(0x1b21, pID, pdev)) != NULL) {
		bar5 = ioremap(pci_resource_start(pdev, 5), pci_resource_len(pdev, 5));
		if (!bar5) {
			printk("Can't map asm1061 registers pci%d:%d\n", pdev->bus->number, pdev->bus->primary);
			goto END;
		}
		for (i = 0 ; i < portNum; i++) {
			for (j = 0; j < 3; j++) {
				pci_bus_read_config_byte(pdev->bus, devfn, asmedia_addr[i] | asmedia_gen[j], &value);
				printk("pci %d:%d port %d gen %d: %x\n",pdev->bus->number, pdev->bus->primary, i, j + 1 ,value);
			}
		}
		if (bar5) {
			iounmap(bar5);
			bar5 = NULL;
		}
	}
	printk("============================\n");
	iRet = 0;

END:
	return iRet;
}

typedef  union u {
	u32 raw;
	struct u_fields{
		u8   byteen   ;
		u8   Register ;
		u8   portid   ;
		u8   opcode   ;
	} field;
} SideBandMsg_t ;

#define UNCORE_REG_MCR                 0xd0 // Message Control
#define UNCORE_REG_MDR                 0xd4 // Message Data
#define UNCORE_REG_MER                 0xd8 // Extended address or offset attributes

int MsgPortRead(u8 Opcode, u8 Port, u32 Register, u32 *Data, u32 addr)
{
	SideBandMsg_t readMsg;
	u32 Data32;
	Data32= Register & 0xFFFFFF00;
	readMsg.field.opcode = (u8) Opcode;
	readMsg.field.portid = (u8) Port;
	readMsg.field.byteen = (u8) 0xf0;

	if( Register > 255 ) { // needs other register to hold 8+ bits
		outl(addr + UNCORE_REG_MER, 0xCF8);
		outl(Data32, 0xCFC);
		readMsg.field.Register = (u8) Register ;
	}else {
		outl(addr + UNCORE_REG_MER, 0xCF8);
		outl(0, 0xCFC);
		readMsg.field.Register = (u8) Register ;
	};

	outl(addr + UNCORE_REG_MCR, 0xCF8);
	outl(readMsg.raw, 0xCFC);

	outl(addr + UNCORE_REG_MDR, 0xCF8);
	*Data = inl(0xCFC);

	return 0;
}

#define PCI_CONF1_ADDRESS(bus, devfn, reg) \
	(0x80000000 | ((reg & 0xF00) << 16) | (bus << 16) \
	| (devfn << 8) | (reg & 0xFC))

static int intel_soc_sata(unsigned pID)
{
	struct pci_dev *pdev = NULL;
	u32 value = 0;
	int i = 0;
	int j = 0;
	u32 offset_t = 0;
    unsigned int port[4]= {0x0, 0x200, 0x400, 0x600};
	unsigned int offset[6]= {0x88, 0x8c, 0x90, 0x100, 0x104, 0x108};
	u32 addr = 0;

	printk("======= Intel SoC SATA =======\n");
	while ((pdev = pci_get_device(0x8086, pID, pdev)) != NULL) {
		addr = PCI_CONF1_ADDRESS(0, pdev->devfn, 0);
		for (i = 0 ; i < 2; i++) {
			for (j = 0; j < 6; j++) {
				offset_t = port[i] + offset[j];
				MsgPortRead(0x0, 0x46, offset_t, &value, addr);
				printk("gen3 port[%x] offset[%x]=0x%x\n", i, j, value);
			}
			printk("\n");
		}

		for (i = 0 ; i < 4; i++) {
			for (j = 0; j < 6 ; j++) {
				offset_t = port[i] + offset[j];
				MsgPortRead(0x0, 0x43, offset_t, &value, addr);
				printk("gen2 port[%x] offset[%x]=0x%x\n", i, j, value);
			}
			printk("\n");
		}
	}
	return 0;
}

static int __init syno_sata_check_init(void)
{
	syno_mv_sata_check(0x9235, 4);
	syno_mv_sata_check(0x9215, 4);
	syno_mv_sata_check(0x9170, 2);
	syno_asm_sata_check(0x0612, 2);
	syno_jmb_sata_check(0x0585, 5);

	intel_soc_sata(0x1f0c);

	printk("Syno_SATA_check: Initialization completed.\n");

	return 0;
}

static void __exit syno_sata_check_exit(void)
{
	return;
}

MODULE_AUTHOR("EricC");
MODULE_DESCRIPTION("Syno_SATA_check\n");
MODULE_LICENSE("Synology Inc.");

module_init(syno_sata_check_init);
module_exit(syno_sata_check_exit);
