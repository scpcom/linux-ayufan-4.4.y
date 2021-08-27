 
#include <linux/stddef.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/reboot.h>
#include <linux/pci.h>
#include <linux/kdev_t.h>
#include <linux/major.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/seq_file.h>
#include <linux/initrd.h>
#include <linux/interrupt.h>
#include <linux/fsl_devices.h>
#include <linux/of_platform.h>

#include <asm/system.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <linux/atomic.h>
#include <asm/time.h>
#include <asm/io.h>
#include <asm/machdep.h>
#include <asm/ipic.h>
#include <asm/pci-bridge.h>
#include <asm/irq.h>
#include <mm/mmu_decl.h>
#include <asm/prom.h>
#include <asm/udbg.h>
#include <asm/mpic.h>
#include <asm/i8259.h>

#ifdef CONFIG_SYNO_MPC854X
#include <asm/fsl_errata.h>
#endif

#include <sysdev/fsl_soc.h>
#include <sysdev/fsl_pci.h>

#define CADMUS_BASE (0xf8004000)
#define CADMUS_SIZE (256)
#define CM_VER	(0)
#define CM_CSR	(1)
#define CM_RST	(2)

static int cds_pci_slot = 2;
static volatile u8 *cadmus;

#ifdef CONFIG_PCI

#define ARCADIA_HOST_BRIDGE_IDSEL	17
#define ARCADIA_2ND_BRIDGE_IDSEL	3

static int mpc85xx_exclude_device(struct pci_controller *hose,
				  u_char bus, u_char devfn)
{
	 
	if ((bus == 1) && (PCI_SLOT(devfn) == ARCADIA_2ND_BRIDGE_IDSEL))
		return PCIBIOS_DEVICE_NOT_FOUND;
	if ((bus == 0) && (PCI_SLOT(devfn) == ARCADIA_2ND_BRIDGE_IDSEL))
		return PCIBIOS_DEVICE_NOT_FOUND;
	else
		return PCIBIOS_SUCCESSFUL;
}

static void mpc85xx_cds_restart(char *cmd)
{
	struct pci_dev *dev;
	u_char tmp;

	if ((dev = pci_get_device(PCI_VENDOR_ID_VIA, PCI_DEVICE_ID_VIA_82C686,
					NULL))) {

		pci_read_config_byte(dev, 0x47, &tmp);
		pci_write_config_byte(dev, 0x47, tmp | 1);

		pci_read_config_byte(dev, 0x47, &tmp);

		pci_dev_put(dev);
	}

	fsl_rstcr_restart(NULL);
}

static void __init mpc85xx_cds_pci_irq_fixup(struct pci_dev *dev)
{
	u_char c;

#ifdef CONFIG_SYNO_MPC854X
	 
	if (MPC8548_ERRATA(2, 1)) {
		unsigned short temp;
		if (dev->vendor == PCI_VENDOR_ID_MOTOROLA ||
			dev->vendor == PCI_VENDOR_ID_FREESCALE) {
			if (dev->device == 0x0012 || dev->device == 0x0013) {
				pci_read_config_word(dev, 0x44, &temp);
				temp |= 1 << 10;
				pci_write_config_word(dev, 0x44, temp);
			}
		}
	}
#endif

	if (dev->vendor == PCI_VENDOR_ID_VIA) {
		switch (dev->device) {
		case PCI_DEVICE_ID_VIA_82C586_1:
			 
			pci_read_config_byte(dev, 0x40, &c);
			c |= 0x03;  
			pci_write_config_byte(dev, 0x40, c);

			dev->irq = 14;
			pci_write_config_byte(dev, PCI_INTERRUPT_LINE, dev->irq);
			break;
		 
		case PCI_DEVICE_ID_VIA_82C586_2:
		 
			if (PCI_FUNC(dev->devfn) == 3)
				dev->irq = 11;
			else
				dev->irq = 10;
			pci_write_config_byte(dev, PCI_INTERRUPT_LINE, dev->irq);
		default:
			break;
		}
	}
}

static void __devinit skip_fake_bridge(struct pci_dev *dev)
{
	 
	dev->hdr_type = 0x7f;
}
DECLARE_PCI_FIXUP_EARLY(0x1957, 0x3fff, skip_fake_bridge);
DECLARE_PCI_FIXUP_EARLY(0x3fff, 0x1957, skip_fake_bridge);
DECLARE_PCI_FIXUP_EARLY(0xff3f, 0x5719, skip_fake_bridge);

#ifdef CONFIG_PPC_I8259
static void mpc85xx_8259_cascade_handler(unsigned int irq,
					 struct irq_desc *desc)
{
	unsigned int cascade_irq = i8259_irq();

	if (cascade_irq != NO_IRQ)
		 
		generic_handle_irq(cascade_irq);

	handle_fasteoi_irq(irq, desc);
}

static irqreturn_t mpc85xx_8259_cascade_action(int irq, void *dev_id)
{
	return IRQ_HANDLED;
}

static struct irqaction mpc85xxcds_8259_irqaction = {
	.handler = mpc85xx_8259_cascade_action,
	.flags = IRQF_SHARED,
	.name = "8259 cascade",
};
#endif  
#endif  

static void __init mpc85xx_cds_pic_init(void)
{
	struct mpic *mpic;
	struct resource r;
	struct device_node *np = NULL;

	np = of_find_node_by_type(np, "open-pic");

	if (np == NULL) {
		printk(KERN_ERR "Could not find open-pic node\n");
		return;
	}

	if (of_address_to_resource(np, 0, &r)) {
		printk(KERN_ERR "Failed to map mpic register space\n");
		of_node_put(np);
		return;
	}

	mpic = mpic_alloc(np, r.start,
			MPIC_PRIMARY | MPIC_WANTS_RESET | MPIC_BIG_ENDIAN,
			0, 256, " OpenPIC  ");
	BUG_ON(mpic == NULL);

	of_node_put(np);

	mpic_init(mpic);
}

#if defined(CONFIG_PPC_I8259) && defined(CONFIG_PCI)
static int mpc85xx_cds_8259_attach(void)
{
	int ret;
	struct device_node *np = NULL;
	struct device_node *cascade_node = NULL;
	int cascade_irq;

	for_each_node_by_type(np, "interrupt-controller")
		if (of_device_is_compatible(np, "chrp,iic")) {
			cascade_node = np;
			break;
		}

	if (cascade_node == NULL) {
		printk(KERN_DEBUG "Could not find i8259 PIC\n");
		return -ENODEV;
	}

	cascade_irq = irq_of_parse_and_map(cascade_node, 0);
	if (cascade_irq == NO_IRQ) {
		printk(KERN_ERR "Failed to map cascade interrupt\n");
		return -ENXIO;
	}

	i8259_init(cascade_node, 0);
	of_node_put(cascade_node);

	if ((ret = setup_irq(cascade_irq, &mpc85xxcds_8259_irqaction))) {
		printk(KERN_ERR "Failed to setup cascade interrupt\n");
		return ret;
	}

	irq_set_handler(cascade_irq, mpc85xx_8259_cascade_handler);

	return 0;
}
machine_device_initcall(mpc85xx_cds, mpc85xx_cds_8259_attach);

#endif  

static void __init mpc85xx_cds_setup_arch(void)
{
#ifdef CONFIG_PCI
	struct device_node *np;
#endif

	if (ppc_md.progress)
		ppc_md.progress("mpc85xx_cds_setup_arch()", 0);

	cadmus = ioremap(CADMUS_BASE, CADMUS_SIZE);
	cds_pci_slot = ((cadmus[CM_CSR] >> 6) & 0x3) + 1;

	if (ppc_md.progress) {
		char buf[40];
		snprintf(buf, 40, "CDS Version = 0x%x in slot %d\n",
				cadmus[CM_VER], cds_pci_slot);
		ppc_md.progress(buf, 0);
	}

#ifdef CONFIG_SYNO_MPC854X
	if (MPC8548_ERRATA(2, 0)) {             
		uint __iomem *tmp_eebpcr;
		tmp_eebpcr = ioremap(get_immrbase() + 0x1010, 0x4);

		setbits32(tmp_eebpcr, 1 << 16);
		iounmap(tmp_eebpcr);

		tmp_eebpcr = ioremap(get_immrbase() + 0x1010, 0x4);
		printk("Apply CPU 2 errata, bpcr: 0x%08x\n", in_be32(tmp_eebpcr));
		iounmap(tmp_eebpcr);
	}
#endif

#ifdef CONFIG_PCI
	for_each_node_by_type(np, "pci") {
		if (of_device_is_compatible(np, "fsl,mpc8540-pci") ||
#ifdef CONFIG_SYNO_MPC854X
			of_device_is_compatible(np, "fsl,mpc8548-pci") ||
#endif
		    of_device_is_compatible(np, "fsl,mpc8548-pcie")) {
			struct resource rsrc;
			of_address_to_resource(np, 0, &rsrc);
			if ((rsrc.start & 0xfffff) == 0x8000)
				fsl_add_bridge(np, 1);
			else
				fsl_add_bridge(np, 0);
		}
	}

	ppc_md.pci_irq_fixup = mpc85xx_cds_pci_irq_fixup;
	ppc_md.pci_exclude_device = mpc85xx_exclude_device;

#ifdef CONFIG_SYNO_MPC854X
	 
	if (MPC8548_ERRATA(2, 0)) {
		struct ccsr_pci __iomem *tmp;

		tmp = ioremap(get_immrbase() + 0x8000, 0x1000);
		 
		setbits32(&tmp->pex_err_dr, 3 << 3);
		 
		clrbits32(&tmp->pex_err_en, 3 << 3); 
		iounmap(tmp);

		tmp = ioremap(get_immrbase() + 0x9000, 0x1000);
		 
		setbits32(&tmp->pex_err_dr, 3 << 3);
		 
		clrbits32(&tmp->pex_err_en, 3 << 3); 
		iounmap(tmp);
	}
#endif
#endif
}

static void mpc85xx_cds_show_cpuinfo(struct seq_file *m)
{
	uint pvid, svid, phid1;

	pvid = mfspr(SPRN_PVR);
	svid = mfspr(SPRN_SVR);

	seq_printf(m, "Vendor\t\t: Freescale Semiconductor\n");
	seq_printf(m, "Machine\t\t: MPC85xx CDS (0x%x)\n", cadmus[CM_VER]);
	seq_printf(m, "PVR\t\t: 0x%x\n", pvid);
	seq_printf(m, "SVR\t\t: 0x%x\n", svid);

	phid1 = mfspr(SPRN_HID1);
	seq_printf(m, "PLL setting\t: 0x%x\n", ((phid1 >> 24) & 0x3f));
}

static int __init mpc85xx_cds_probe(void)
{
        unsigned long root = of_get_flat_dt_root();

        return of_flat_dt_is_compatible(root, "MPC85xxCDS");
}

static struct of_device_id __initdata of_bus_ids[] = {
	{ .type = "soc", },
	{ .compatible = "soc", },
	{ .compatible = "simple-bus", },
	{ .compatible = "gianfar", },
	{},
};

static int __init declare_of_platform_devices(void)
{
	return of_platform_bus_probe(NULL, of_bus_ids, NULL);
}
machine_device_initcall(mpc85xx_cds, declare_of_platform_devices);

define_machine(mpc85xx_cds) {
	.name		= "MPC85xx CDS",
	.probe		= mpc85xx_cds_probe,
	.setup_arch	= mpc85xx_cds_setup_arch,
	.init_IRQ	= mpc85xx_cds_pic_init,
	.show_cpuinfo	= mpc85xx_cds_show_cpuinfo,
	.get_irq	= mpic_get_irq,
#ifdef CONFIG_PCI
	.restart	= mpc85xx_cds_restart,
	.pcibios_fixup_bus	= fsl_pcibios_fixup_bus,
#else
	.restart	= fsl_rstcr_restart,
#endif
	.calibrate_decr = generic_calibrate_decr,
	.progress	= udbg_progress,
};
