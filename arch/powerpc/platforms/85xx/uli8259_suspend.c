#ifdef CONFIG_SYNO_QORIQ
/*
 * Copyright (C) 2008 Freescale Semiconductor, Inc. All rights reserved.
 *
 * Description: Suspend support for ULI M1575 i8259 on MPC8572DS
 *
 * Changelog:
 * May 2008 Jason Jin <jason.jin@freescale.com >
 * - Initialization
 *
 * This file is part of the Linux kernel
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include <linux/stddef.h>
#include <linux/kernel.h>
#include <linux/pci.h>

#include <asm/system.h>
#include <asm/pci-bridge.h>

#ifdef CONFIG_PM
static DEFINE_SPINLOCK(uli_pm_lock);

struct saved_config
{
	u32 save_0x48;
	u8  save_0x86_0x8f[10];
	u8  save_0x74;
	u8  save_0x44;
	u8  save_0x75;
};
struct saved_config uli8259_saved_config;

static int uli8259_suspend(struct pci_dev *pdev, pm_message_t state)
{
	int i;
	unsigned long flags;
	spin_lock_irqsave(&uli_pm_lock, flags);

	pci_read_config_dword(pdev, 0x48, &uli8259_saved_config.save_0x48);

	for (i = 0; i < 10; i++)
		pci_read_config_byte(pdev, 0x86 + i,
				&uli8259_saved_config.save_0x86_0x8f[i]);

	pci_read_config_byte(pdev, 0x74, &uli8259_saved_config.save_0x74);
	pci_read_config_byte(pdev, 0x75, &uli8259_saved_config.save_0x75);
	pci_read_config_byte(pdev, 0x44, &uli8259_saved_config.save_0x44);

	spin_unlock_irqrestore(&uli_pm_lock, flags);
	return 0;
}

static int uli8259_resume(struct pci_dev *pdev)
{
	int i;

	unsigned long flags;
	spin_lock_irqsave(&uli_pm_lock, flags);

	pci_write_config_dword(pdev, 0x48, uli8259_saved_config.save_0x48);

	for (i = 0; i < 10; i++)
		pci_write_config_byte(pdev, 0x86 + i,
				uli8259_saved_config.save_0x86_0x8f[i]);

	pci_write_config_byte(pdev, 0x74, uli8259_saved_config.save_0x74);
	pci_write_config_byte(pdev, 0x75, uli8259_saved_config.save_0x75);
	pci_write_config_byte(pdev, 0x44, uli8259_saved_config.save_0x44);

	outb(0xfa, 0x4d0);
	outb(0x1e, 0x4d1);

	spin_unlock_irqrestore(&uli_pm_lock, flags);
	return 0;
}
#endif
static int __devinit uli8259_suspend_probe(struct pci_dev *pdev,
		const struct pci_device_id *ent)
{
	return 0;
}


static struct pci_device_id uli8259_pci_tbl[] = {
	{ PCI_VENDOR_ID_AL, 0x1575, PCI_ANY_ID, PCI_ANY_ID, 0, 0},
	{ 0, }
};

static struct pci_driver uli8259_suspend_driver = {
	.name     = "uli8259 suspend",
	.id_table = uli8259_pci_tbl,
	.probe    = uli8259_suspend_probe,
#ifdef CONFIG_PM
	.suspend  = uli8259_suspend,
	.resume   = uli8259_resume,
#endif
};

static int uli8259_suspend_init(void)
{
	int ret;

	ret = pci_register_driver(&uli8259_suspend_driver);

	return ret;
}

module_init(uli8259_suspend_init);
#endif /* CONFIG_SYNO_QORIQ */
