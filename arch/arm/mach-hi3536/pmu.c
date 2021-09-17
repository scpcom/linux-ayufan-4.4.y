#include <linux/platform_device.h>
#include <mach/irqs.h>

static struct resource pmu_resource_hi3536[] = {
	[0] = {
		.start = INTNR_A17_PMU_INT0,
		.end   = INTNR_A17_PMU_INT0,
		.flags = IORESOURCE_IRQ,
	},

	[1] = {
		.start = INTNR_A17_PMU_INT1,
		.end   = INTNR_A17_PMU_INT1,
		.flags = IORESOURCE_IRQ,
	}
};

static struct platform_device A17_pmu_device_hi3536 = {
	.name = "arm-pmu",
	.id   = -1,
	.resource = pmu_resource_hi3536,
	.num_resources = ARRAY_SIZE(pmu_resource_hi3536),
};

static int __init pmu_init(void)
{
	platform_device_register(&A17_pmu_device_hi3536);

	return 0;
};
arch_initcall(pmu_init);
