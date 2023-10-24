#include <linux/kernel.h>

#include <linux/platform_device.h>    // needed for 'resource'
#include <linux/module.h>
#include <linux/leds.h>
#include <asm/io.h>                   // needed for 'ioremap'
#include "leds-wd.h"

struct task_struct * threadptr;

static struct led_classdev system_led_dev = {
	.name            = "system_led",
	//.default_trigger = "heartbeat",
	.color_set       = system_led_set,
	.color_get       = system_led_get,
	.blink_set_3g    = system_led_blink,
//	.brightness_set  = 
};

static struct led_classdev wifi_led_dev = {
	.name            = "wifi_led",
	//.default_trigger = "heartbeat",
	.color_set       = wifi_led_set,
	.color_get       = wifi_led_get,
	.blink_set_3g    = wifi_led_blink,
};

static int __devexit a3g_led_remove(struct platform_device *pdev) {
	led_classdev_unregister(&system_led_dev);
	if (led_port) {
		iounmap(led_port);
		led_port = NULL;
	}

	return 0;
}


static struct platform_driver sequoia_led_driver = {
	.probe      = 0, 
	.remove     = __devexit_p(a3g_led_remove),
	.driver     = {
		.name = "sequoia-leds",
		.owner = THIS_MODULE,
	},
};


static int __init sequoia_led_init(void) {
	int retval;	

	led_port = COMCERTO_GPIO_OUTPUT_REG;

	retval = led_classdev_register(NULL, &system_led_dev);
	if (retval) {
		led_classdev_unregister(&system_led_dev);
		iounmap(led_port);
		led_port = NULL;
		return -1;
	}

	/* WIFI LED */
	retval = led_classdev_register(NULL, &wifi_led_dev);
	if (retval) {
		led_classdev_unregister(&wifi_led_dev);
		iounmap(led_port);
		led_port = NULL;
		return -1;
	}


	threadptr = kthread_run(sequoia_blink_thread, NULL, "a3gblink_t");
	return platform_driver_register(&sequoia_led_driver);
	//}
}


module_init(sequoia_led_init);

MODULE_AUTHOR("Arya Ahmadi-Ardakani <arya.ahmadi-ardakani@wdc.com>");
MODULE_LICENSE("GPL");
