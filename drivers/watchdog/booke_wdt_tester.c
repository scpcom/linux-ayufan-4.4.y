#ifdef CONFIG_SYNO_QORIQ
 
#include <linux/module.h>

static void __loop_occupy_cpu(void *data)
{
	printk(KERN_INFO "__loop_occupy_cpu id=%08x\n", smp_processor_id());
	while (1);
}

static int __init wdt_tester_init(void)
{
	on_each_cpu(__loop_occupy_cpu, NULL, 0);
	return 0;
}

module_init(wdt_tester_init);

MODULE_AUTHOR("Jiang Yutang");
MODULE_LICENSE("GPL");
#endif  
