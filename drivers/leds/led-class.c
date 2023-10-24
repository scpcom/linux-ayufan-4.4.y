/*
 * LED Class Core
 *
 * Copyright (C) 2005 John Lenz <lenz@cs.wisc.edu>
 * Copyright (C) 2005-2007 Richard Purdie <rpurdie@openedhand.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/sysdev.h>
#include <linux/timer.h>
#include <linux/err.h>
#include <linux/ctype.h>
#include <linux/leds.h>
#include <mach/gpio.h>
#include "leds.h"

#define MAX_USERS          32
#define N_COLORS           4
#define N_EVENTS           3
#define USR_LEN            81
#define EVENT_LEN          81
#define INDEX_LEN          8


/* LED users */
#define EV_NAS_SYSTEM      0    /* Overall system: NAS ready, booting, shutdown... */
#define EV_DISK_SMART      1    /* Disk SMART including temp., error lba, ...*/
#define EV_DISK_IO         2    /* Disk read/write error */
#define EV_RAID_CFG        3    /* RAID setup failure: assembling, formatting, rebuild ...*/
#define EV_FW_UPDATE       4    /* NAS firmware update */
#define EV_NETWORK         5    /* Network connectivity error */
#define EV_VM              6    /* Volume manager */

char Led_user_arr[MAX_USERS][USR_LEN] = { "EV_NAS_SYSTEM", \
                                          "EV_DISK_SMART", \
                                          "EV_DISK_IO"   , \
                                          "EV_RAID_CFG"  , \
                                          "EV_FW_UPDATE" , \
                                          "EV_NETWORK"   , \
                                          "EV_VM",         \
                                        };
/* LED event types */
#define   LED_STAT_OK        0    /* Happy user, normal operation */
#define   LED_STAT_ERR       1    /* User error, needs led indication */
#define   LED_STAT_IN_PROG   2    /* User doing something important, needs led indication */

char *Led_ev_arr[] = { "LED_STAT_OK", "LED_STAT_ERR", "LED_STAT_IN_PROG" };

char Color_map[MAX_USERS][N_EVENTS]  = { {'g','r','w'}, /* EV_NAS_SYSTEM */ \
                                         {'g','y','w'}, /* EV_DISK_SMART */ \
                                         {'g','r','w'}, /* EV_DISK_IO    */ \
                                         {'g','r','w'}, /* EV_RAID_CFG   */ \
                                         {'g','r','w'}, /* EV_FW_UPDATE  */ \
                                         {'g','y','w'}, /* EV_NETWORK    */ \
                                         {'g','r','w'}, /* EV_VM */ \
                                       };

char Blink_map[MAX_USERS][N_EVENTS]  = { {'n','n','n'}, /* EV_NAS_SYSTEM */ \
                                         {'n','y','n'}, /* EV_DISK_SMART */ \
                                         {'n','n','n'}, /* EV_DISK_IO    */ \
                                         {'n','n','n'}, /* EV_RAID_CFG   */ \
                                         {'n','n','n'}, /* EV_FW_UPDATE  */ \
                                         {'n','y','n'}, /* EV_NETWORK    */ \
                                         {'n','n','n'}, /* EV_VM */ \
                                       };

u32  Led_error_bits = 0;
int  N_USERS = 7;    /* default number of users */

static struct class *leds_class;

static void led_update_brightness(struct led_classdev *led_cdev)
{
	if (led_cdev->brightness_get)
		led_cdev->brightness = led_cdev->brightness_get(led_cdev);
}

static ssize_t led_brightness_show(struct device *dev, 
                                   struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);

	/* no lock needed for this */
	led_update_brightness(led_cdev);

	return sprintf(buf, "%u\n", led_cdev->brightness);
}

static ssize_t led_brightness_store(struct device *dev,
                                    struct device_attribute *attr, const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	char *after;
	unsigned long state = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;
	if (count == size) {
		ret = count;
		if (state == LED_OFF)
			led_trigger_remove(led_cdev);
		led_set_brightness(led_cdev, state);
	}
	printk(KERN_DEBUG "We are here 10\n");
	return ret;
}

static ssize_t led_max_brightness_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", led_cdev->max_brightness);
}

static void led_update_color(struct led_classdev *led_cdev)
{
    if (led_cdev->color_get)
        led_cdev->color = led_cdev->color_get(led_cdev);
}

static ssize_t led_color_show(struct device *dev, struct device_attribute *attr, char *buf) 
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	char * readbuf[] = {"off", "red", "green", "blue", "yellow", "white"} ;
	/* no lock needed for this */
	led_update_color(led_cdev);

	return sprintf(buf, "%s\n", readbuf[led_cdev->color]);
}

static ssize_t led_color_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	unsigned long state = 9;
	char user[USR_LEN], event[EVENT_LEN], index_str[INDEX_LEN], color;
	int i = 0, j = 0, found = 0, tmp = 0, edit_policy = 0;
	int reg_user = -1, reg_event = -1, reg_color = -1;
	const char * cptr = NULL;
	long int index = -1;
	char blink;
	int reg_blink = 'n';

	cptr = &buf[0];

	/* check for 'register' event */
	// NB: Format of register event is 
	// register:event,status,color
	if( cptr[8] == ':' ) {
		if( !memcmp("register", cptr, 8) ) {
			edit_policy = 1;
			cptr = &buf[9];
		}
	}

	/* parse user name */ 
	for( i = 0; i < (USR_LEN -1) && cptr[i]; i++ ) {
		if( cptr[i] == ',' ) {
			break;
		}
		user[i] = cptr[i];
	}

	/* null terminate user buf */
	user[i] = '\0';
	i++; /* skips the ',' delimiter */


	for( j = 0; (j < EVENT_LEN -1) && cptr[i] ; j++,i++ ) {
		if( (cptr[i] == ',') || (cptr[i] == '\0') || (cptr[i] == '\n') ) {
			if( cptr[i] == ',' ) {
				cptr = &cptr[i+1];
			}
			break;
		}
		event[j] = cptr[i];
	}
	/* null terminate event buf */
	event[j] = '\0';

	/* if editing policy, parse the color */ 
	if( edit_policy ) {
		if( cptr != NULL ) {
			reg_color = cptr[0];  /* r,g,b,y,w */
			if( reg_color != 'r' && reg_color != 'g' && 
				reg_color != 'b' && reg_color != 'y' && reg_color != 'w' ) {
				reg_color = -1;  /* invalid color */               
			}

			/** TBD: Get the value of reg_blink from cptr */
		}
	} else {
		/* scan index for some users */
		if( !strcmp(user, Led_user_arr[EV_DISK_SMART]) || 
		    !strcmp(user, Led_user_arr[EV_DISK_IO]) ) {
			if( cptr != NULL ) {
				for( i = 0; (i < INDEX_LEN -1) && cptr[i] ; i++ ) {
					if( (cptr[i] == ',') || (cptr[i] == '\0') || (cptr[i] == '\n') ) {
						break;
					}
					index_str[i] = cptr[i];
				}
			}
		}

		/* null terminate index_str */
		index_str[i] = '\0';
		if( i ) {
			tmp = strict_strtol(index_str, 10, &index);
			if( !tmp && (index >= 0) ) {
				/*
				* TODO: insert code to fulfill req's. Currently not required.
				*/
				/*printk(KERN_INFO "\nindex %ld\n", index);*/
			}
		}
	} /* if( !edit_policy ) */

	/* Validate user and event */
	found = 0; 
	for( i = 0; i < N_USERS; i++ ) {
		if( !strcmp( Led_user_arr[i], user ) ) {
			found = 1;
			break;
		}
	}

	if( found || edit_policy) {
		reg_user = i;
		/* new user registration */
		if( ! found ) {
			if( N_USERS == MAX_USERS ) {
			/* only support up to 32 users */
				return (ssize_t)size;
			}
			reg_user = N_USERS++;

			strcpy(Led_user_arr[reg_user], user);
		}
		found = 0;
		for( j = 0; j < N_EVENTS; j++ ) {
			if( ! strcmp(Led_ev_arr[j], event) ) {
				if( j == LED_STAT_ERR ) {
					Led_error_bits |= (1 << i); /* register error for this user */
				}
				else if( j == LED_STAT_OK ) {
					Led_error_bits &= ~(1 << i); /* clear error for this user */
				}
				found = 1;
				reg_event = j;
				break;
			}
		}
	}

	/* if this is a register event, do just that */
	if( edit_policy ) {
		/* valid event above and color */
		if( (reg_event != -1) && (reg_color != -1) ) {
			Color_map[reg_user][reg_event] = reg_color;

			/** TBD: Add support for registering blink with register: interface*/
			reg_blink = 'n';
			Blink_map[reg_user][reg_event] = reg_blink;
		}
		/*printk( KERN_INFO "reg_user = %d, reg_event= %d, reg_color = %c\n", reg_user, reg_event, reg_color, reg_blink);*/
		return (ssize_t)size;
	}

	/* Be nice ! support older led mechanism */ 
	color = buf[0];
	blink = 'x';

	/* If valid user and event, retrieve color & blink map */
	if( found ) {
		/* if a canceling event and other error(s) existing, don't do anything */
		if( (j == LED_STAT_OK) && (Led_error_bits != 0) ) {
			/* Do nothing */
		} else {
			color = Color_map[i][j];
			blink = Blink_map[i][j];
		}
		/*printk(KERN_INFO "\nUser= %s, event= %s, color %c, %08x\n", user, event, color, blink, Led_error_bits);*/
	}

	switch (color) {
		case 'o':         /* off */
			state = STATE_LED_OFF;
			break;
		case 'r':         /* red */
			state = STATE_LED_RED;
			break;
		case 'g':         /* green */
			state = STATE_LED_GREEN;
			break;
		case 'b':         /* blue */
			state = STATE_LED_BLUE;
			break;
		case 'y':         /* yellow */
			state = STATE_LED_YELLOW;
			break;
		case 'w':         /* white */
			state = STATE_LED_ALL;
			break;
		default:          /* error */
			state = -1;
			break;
	}

	/* do nothing if no color change is required */
	if( state == -1 ) {
		return (ssize_t)size;
	}

	/* printk(KERN_DEBUG "Calling led_set_color with value %c, blink is %c\n", color, blink); */
	led_set_color(led_cdev, state);

	/** blink the led */
	{
		int val = -1;

		/* printk(KERN_DEBUG "Calling led_set_blink with value %c\n", blink); */

		switch (blink) {
			case 'y':	/** yes */
				val = 1;
				break;
			case 'n':	/** no */
				val = 0;
				break;
			case 'f':	/** forced */
				val = 2;
				break;
			default:
				break;
		}

		if( val >= 0 ) {
			led_set_blink( led_cdev, val );
		}
	}


	return (ssize_t)size;
}

static ssize_t led_blink_show(struct device *dev, struct device_attribute *attr,
                              char *buf) {
        char *blinkStr = "on";
        struct led_classdev  *led_cdev = dev_get_drvdata(dev);
        
        if( led_cdev->blink == 0 ) {
		blinkStr = "on";
        } else if (led_cdev->blink == 1 ) {
		blinkStr = "blink";
        } else if (led_cdev->blink == 2 ) {
		blinkStr = "pulse";
	}

    return sprintf(buf, "%s\n", blinkStr);
}

static ssize_t led_blink_store(struct device *dev, struct device_attribute *attr,
	                       const char *buf, size_t size) {
	int val = 0;
	struct led_classdev * led_cdev = dev_get_drvdata(dev);

	if (buf[0] == 'b') {
		val = 1;
	} else if (buf[0] == 'o') {
		val = 0;
	} else if (buf[0] == 'f') {
		val = 2;
	} else if (buf[0] == 'p') {
		val = 3;
	} else {
		val = 0;
	}

	led_set_blink(led_cdev, val);

	return (ssize_t)size;
}

static struct device_attribute led_class_attrs[] = {
	__ATTR(color, 0644, led_color_show, led_color_store),
	__ATTR(blink, 0644, led_blink_show, /*led_set_blink*/led_blink_store),
	__ATTR(brightness, 0644, led_brightness_show, led_brightness_store),
	__ATTR(max_brightness, 0444, led_max_brightness_show, NULL),
#ifdef CONFIG_LEDS_TRIGGERS
	__ATTR(trigger, 0644, led_trigger_show, led_trigger_store),
#endif
	__ATTR_NULL,
};


static void led_timer_function(unsigned long data)
{
	struct led_classdev *led_cdev = (void *)data;
	unsigned long brightness;
	unsigned long delay;

	if (!led_cdev->blink_delay_on || !led_cdev->blink_delay_off) {
		led_set_brightness(led_cdev, LED_OFF);
		return;
	}

	brightness = led_get_brightness(led_cdev);
	if (!brightness) {
		/* Time to switch the LED on. */
		brightness = led_cdev->blink_brightness;
		delay = led_cdev->blink_delay_on;
	} else {
		/* Store the current brightness value to be able
		 * to restore it when the delay_off period is over.
		 */
		led_cdev->blink_brightness = brightness;
		brightness = LED_OFF;
		delay = led_cdev->blink_delay_off;
	}

	led_set_brightness(led_cdev, brightness);

	mod_timer(&led_cdev->blink_timer, jiffies + msecs_to_jiffies(delay));
}

static void led_stop_software_blink(struct led_classdev *led_cdev)
{
	/* deactivate previous settings */
	del_timer_sync(&led_cdev->blink_timer);
	led_cdev->blink_delay_on = 0;
	led_cdev->blink_delay_off = 0;
}

static void led_set_software_blink(struct led_classdev *led_cdev,
				   unsigned long delay_on,
				   unsigned long delay_off)
{
	int current_brightness;

	current_brightness = led_get_brightness(led_cdev);
	if (current_brightness)
		led_cdev->blink_brightness = current_brightness;
	if (!led_cdev->blink_brightness)
		led_cdev->blink_brightness = led_cdev->max_brightness;

	if (led_get_trigger_data(led_cdev) &&
	    delay_on == led_cdev->blink_delay_on &&
	    delay_off == led_cdev->blink_delay_off)
		return;

	led_stop_software_blink(led_cdev);

	led_cdev->blink_delay_on = delay_on;
	led_cdev->blink_delay_off = delay_off;

	/* never on - don't blink */
	if (!delay_on)
		return;

	/* never off - just set to brightness */
	if (!delay_off) {
		led_set_brightness(led_cdev, led_cdev->blink_brightness);
		return;
	}

	mod_timer(&led_cdev->blink_timer, jiffies + 1);
}


/**
 * led_classdev_suspend - suspend an led_classdev.
 * @led_cdev: the led_classdev to suspend.
 */
void led_classdev_suspend(struct led_classdev *led_cdev)
{
	led_cdev->flags |= LED_SUSPENDED;
	led_cdev->brightness_set(led_cdev, 0);
}
EXPORT_SYMBOL_GPL(led_classdev_suspend);

/**
 * led_classdev_resume - resume an led_classdev.
 * @led_cdev: the led_classdev to resume.
 */
void led_classdev_resume(struct led_classdev *led_cdev)
{
	led_cdev->brightness_set(led_cdev, led_cdev->brightness);
	led_cdev->flags &= ~LED_SUSPENDED;
}
EXPORT_SYMBOL_GPL(led_classdev_resume);

static int led_suspend(struct device *dev, pm_message_t state)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);

	if (led_cdev->flags & LED_CORE_SUSPENDRESUME)
		led_classdev_suspend(led_cdev);

	return 0;
}

static int led_resume(struct device *dev)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);

	if (led_cdev->flags & LED_CORE_SUSPENDRESUME)
		led_classdev_resume(led_cdev);

	return 0;
}

/**
 * led_classdev_register - register a new object of led_classdev class.
 * @parent: The device to register.
 * @led_cdev: the led_classdev structure for this device.
 */
int led_classdev_register(struct device *parent, struct led_classdev *led_cdev)
{
	led_cdev->dev = device_create(leds_class, parent, 0, led_cdev,"%s", led_cdev->name);
	if (IS_ERR(led_cdev->dev))
		return PTR_ERR(led_cdev->dev);

#ifdef CONFIG_LEDS_TRIGGERS
	init_rwsem(&led_cdev->trigger_lock);
#endif
	/* add to the list of leds */
	down_write(&leds_list_lock);
	list_add_tail(&led_cdev->node, &leds_list);
	up_write(&leds_list_lock);

	if (!led_cdev->max_brightness)
		led_cdev->max_brightness = LED_FULL;

	led_update_brightness(led_cdev);

	init_timer(&led_cdev->blink_timer);
	led_cdev->blink_timer.function = led_timer_function;
	led_cdev->blink_timer.data = (unsigned long)led_cdev;

#ifdef CONFIG_LEDS_TRIGGERS
	led_trigger_set_default(led_cdev);
#endif

	printk(KERN_DEBUG "Registered led device: %s\n",
			led_cdev->name);

	return 0;
}
EXPORT_SYMBOL_GPL(led_classdev_register);

/**
 * led_classdev_unregister - unregisters a object of led_properties class.
 * @led_cdev: the led device to unregister
 *
 * Unregisters a previously registered via led_classdev_register object.
 */
void led_classdev_unregister(struct led_classdev *led_cdev)
{
#ifdef CONFIG_LEDS_TRIGGERS
	down_write(&led_cdev->trigger_lock);
	if (led_cdev->trigger)
		led_trigger_set(led_cdev, NULL);
	up_write(&led_cdev->trigger_lock);
#endif

	/* Stop blinking */
	led_brightness_set(led_cdev, LED_OFF);

	device_unregister(led_cdev->dev);

	down_write(&leds_list_lock);
	list_del(&led_cdev->node);
	up_write(&leds_list_lock);
}
EXPORT_SYMBOL_GPL(led_classdev_unregister);

void led_blink_set(struct led_classdev *led_cdev,
		   unsigned long *delay_on,
		   unsigned long *delay_off)
{
	del_timer_sync(&led_cdev->blink_timer);

	if (led_cdev->blink_set &&
	    !led_cdev->blink_set(led_cdev, delay_on, delay_off))
		return;

	/* blink with 1 Hz as default if nothing specified */
	if (!*delay_on && !*delay_off)
		*delay_on = *delay_off = 500;

	led_set_software_blink(led_cdev, *delay_on, *delay_off);
}
EXPORT_SYMBOL(led_blink_set);

void led_brightness_set(struct led_classdev *led_cdev,
			enum led_brightness brightness)
{
	led_stop_software_blink(led_cdev);
	led_cdev->brightness_set(led_cdev, brightness);
}
EXPORT_SYMBOL(led_brightness_set);

static int __init leds_init(void)
{
	leds_class = class_create(THIS_MODULE, "leds");
	if (IS_ERR(leds_class))
		return PTR_ERR(leds_class);
	leds_class->suspend = led_suspend;
	leds_class->resume = led_resume;
	leds_class->dev_attrs = led_class_attrs;
	return 0;
}

static void __exit leds_exit(void)
{
	class_destroy(leds_class);
}

subsys_initcall(leds_init);
module_exit(leds_exit);

MODULE_AUTHOR("John Lenz, Richard Purdie, Arya Ahmadi-Ardakani");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("LED Class Interface");
