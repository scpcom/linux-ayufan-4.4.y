/*
 * PMU IRQ registration for the ARMADA XP PMU families.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/platform_device.h>
#include <asm/pmu.h>
#include <mach/irqs.h>

static struct resource pmu_resource = {
	.start	= IRQ_AURORA_MP,
	.end	= IRQ_AURORA_MP,
	.flags	= IORESOURCE_IRQ,
};

static struct platform_device pmu_device = {
	.name		= "arm-pmu",
	.id		= ARM_PMU_DEVICE_CPU,
	.resource	= &pmu_resource,
	.num_resources	= 1,
};

static int __init armadaxp_pmu_init(void)
{
	platform_device_register(&pmu_device);
	return 0;
}
arch_initcall(armadaxp_pmu_init);
