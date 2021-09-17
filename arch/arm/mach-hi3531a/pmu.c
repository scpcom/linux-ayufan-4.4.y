#include <linux/platform_device.h>
#include <asm/pmu.h>
#include <mach/irqs.h>

static struct resource pmu_resource_hi3531a[] = {
	[0] = {
		.start = INTNR_A9_PMU_INT0,
		.end   = INTNR_A9_PMU_INT0,
		.flags = IORESOURCE_IRQ,
	},

	[1] = {
		.start = INTNR_A9_PMU_INT1,
		.end   = INTNR_A9_PMU_INT1,
		.flags = IORESOURCE_IRQ,
	}
};

static struct platform_device A9_pmu_device_hi3531a = {
	.name = "arm-pmu",
	.id   = ARM_PMU_DEVICE_CPU,
	.resource = pmu_resource_hi3531a,
	.num_resources = ARRAY_SIZE(pmu_resource_hi3531a),
};

static int __init pmu_init(void)
{
	platform_device_register(&A9_pmu_device_hi3531a);

	return 0;
};

arch_initcall(pmu_init);
