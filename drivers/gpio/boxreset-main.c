#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/timer.h>
#include <linux/reboot.h>
#include <linux/moduleparam.h>

#include "boxreset-uart.h"

MODULE_DESCRIPTION("BOX gpio reset");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bogdan Harjoc <bharjoc@bitdefender.com>");

int disable;
module_param(disable, uint, 0644);
MODULE_PARM_DESC(disable, "Disable reset on gpio button press (default 0 aka do the reset)");

static void boxr_timer_fn(unsigned long arg);

DEFINE_TIMER(boxr_timer, boxr_timer_fn, 0, 0);

#define TIMER_FREQ 25
#define NUM_VALUES 4

static unsigned char boxr_gpio_values[NUM_VALUES] = { };

static long (*boxr_orig_panic_blink)(int state);

static void boxr_timer_fn(unsigned long arg)
{
    if (disable)
        return;

    memmove(boxr_gpio_values, boxr_gpio_values + 1, NUM_VALUES - 1);
    
    boxr_gpio_values[NUM_VALUES-1] = gpio_get_value(49) == 0;

    if (memchr(boxr_gpio_values, 0, NUM_VALUES) == 0)
        emergency_restart();

    mod_timer(&boxr_timer, jiffies + HZ/TIMER_FREQ);
}

#if 0
static unsigned char led_cmd_spinning[] = {
    0x02, 0x00, 0x10, 0x02, 0x04, 0xff, 0xff, 0xff, 0x07, 0x08, 0x03
};
#endif

static void send_ledboard_cmd(void)
{
    // not working yet, so we don't do blinking lights at panic for now

#if 0
    int i;
    
    //c2k_init_uart_led();

    //c2k_setbrg(57600);

    for (i=0; i<sizeof(led_cmd_spinning); i++)
        c2k_putc(led_cmd_spinning[i]);
#endif
}

static long boxr_panic_blink(int state)
{
    static int called = 0;
    
    if (called) return 0;
    called = 1;
    
    send_ledboard_cmd();

    return 0;
}

static int __init boxr_init(void)
{
	int ret;

	ret = gpio_request(49, "box-reset");
    if (ret) {
        printk("gpio_request: %d\n", ret);
        return ret;
    }

	gpio_direction_input(49);
	gpio_set_debounce(49, 50);
	gpio_export(49, 0);
    
    mod_timer(&boxr_timer, jiffies + HZ/TIMER_FREQ);
    
    boxr_orig_panic_blink = panic_blink;
    panic_blink = boxr_panic_blink;

	return 0;
}

static void __exit boxr_exit(void)
{
    if (panic_blink == boxr_panic_blink)
        panic_blink = boxr_orig_panic_blink;

    del_timer(&boxr_timer);

	gpio_unexport(49);
	gpio_free(49);
}

module_init(boxr_init);
module_exit(boxr_exit);
