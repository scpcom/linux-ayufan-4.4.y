#include <linux/delay.h>       // msleep
#include <linux/suspend.h>     // The following is needed for wake_up()
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/signal.h>
#include <linux/freezer.h>
#include <mach/gpio.h>
#include "leds.h"

//#define SYSTEM_LED                1
#define SYSTEM_LED_RED            GPIO_PIN_7
#define SYSTEM_LED_GREEN          GPIO_PIN_5
#define SYSTEM_LED_BLUE           GPIO_PIN_6
#define SYSTEM_LED_YELLOW         (SYSTEM_LED_GREEN | SYSTEM_LED_RED)
#define SYSTEM_LED_ALL            (SYSTEM_LED_GREEN | SYSTEM_LED_RED | SYSTEM_LED_BLUE)

//#define SYSTEM_LED_RED_PWM_REG    COMCERTO_LOW_DUTY_PWM3
//#define SYSTEM_LED_GREEN_PWM_REG  COMCERTO_LOW_DUTY_PWM1
//#define SYSTEM_LED_BLUE_PWM_REG   COMCERTO_LOW_DUTY_PWM2

//#define WIFI_LED                  2
#define WIFI_LED_YELLOW           GPIO_PIN_12
#define WIFI_LED_BLUE             GPIO_PIN_13
#define WIFI_LED_WHITE            (GPIO_PIN_12 | GPIO_PIN_13)
#define WIFI_LED_ALL              (GPIO_PIN_12 | GPIO_PIN_13)
#define LED_OFF                   0x00

//#define BOTH_LED                  3
//#define NO_LED                    0

#define  _BLINK_NO                0
#define  _BLINK_YES               1
#define  _BLINK_FORCE             2
#define  _PULSE                   3

#define  HDD_BLINK_RATE 250

static DEFINE_SPINLOCK(led_lock);
wait_queue_head_t  ts_wait;

typedef struct led_state_s {
	int cur_color;
	int cur_action;
} led_state_t;

void __iomem *led_port = NULL;
int  system_blink_flag = _BLINK_NO;
int  wifi_blink_flag   = _BLINK_NO;

int PWM_index = 32;                 // PWM Value

led_state_t  system_led_state = {
	.cur_color  = STATE_LED_ALL,
	.cur_action = _BLINK_NO
};

led_state_t  wifi_led_state   = {
	.cur_color  = STATE_LED_OFF,
	.cur_action = _BLINK_NO
};

void write_led_gpio(u32 mask ,u32 value) {
	u32 regval;

	regval = __raw_readl(led_port);
	regval &= ~mask;
	regval |= (value & mask); 
	__raw_writel(regval, led_port);
}


static void system_led_set(struct led_classdev *led_cdev,  int value) {
	unsigned long flags;

	spin_lock_irqsave(&led_lock, flags);
	switch (value) {
		case STATE_LED_RED:
			write_led_gpio(SYSTEM_LED_ALL, SYSTEM_LED_RED);  
			break;
		case STATE_LED_GREEN:
			write_led_gpio(SYSTEM_LED_ALL, SYSTEM_LED_GREEN);  
			break;
		case STATE_LED_BLUE:
			write_led_gpio(SYSTEM_LED_ALL, SYSTEM_LED_BLUE);  
			break;
		case STATE_LED_OFF:
			write_led_gpio(SYSTEM_LED_ALL, LED_OFF);  
			break;
		case STATE_LED_ALL:
			write_led_gpio(SYSTEM_LED_ALL, SYSTEM_LED_ALL);  
			break;
		case STATE_LED_YELLOW:
			write_led_gpio(SYSTEM_LED_ALL, SYSTEM_LED_YELLOW);  
			break;
		default:
		break; /* should never be here */
	}
	system_led_state.cur_color = value; 
	spin_unlock_irqrestore(&led_lock, flags);
}

static void wifi_led_set(struct led_classdev *led_cdev,  int value) {
	unsigned long flags;

	spin_lock_irqsave(&led_lock, flags);
	switch (value) {
		case STATE_LED_RED:
			//write_led_gpio(WIFI_LED_ALL, SYSTEM_LED_RED);  
			break;
		case STATE_LED_GREEN:
			//write_led_gpio(WIFI_LED_ALL, SYSTEM_LED_GREEN);  
			break;
		case STATE_LED_BLUE:
			write_led_gpio(WIFI_LED_ALL, WIFI_LED_BLUE);  
			break;
		case STATE_LED_OFF:
			write_led_gpio(WIFI_LED_ALL, LED_OFF);  
			break;
		case STATE_LED_WHITE:
			write_led_gpio(WIFI_LED_ALL, WIFI_LED_WHITE);  
			break;
		case STATE_LED_YELLOW:
			write_led_gpio(WIFI_LED_ALL, WIFI_LED_YELLOW);  
			break;
		default:
		break; /* should never be here */
	}
	wifi_led_state.cur_color = value; 
	spin_unlock_irqrestore(&led_lock, flags);
}


static enum led_brightness system_led_get(struct led_classdev * led_cdev) {
	return system_led_state.cur_color;
}

static enum led_brightness wifi_led_get(struct led_classdev * led_cdev) {
	return wifi_led_state.cur_color;
}

static int system_led_blink(struct led_classdev *led_cdev,  int value) {

	 __raw_writel(__raw_readl(COMCERTO_GPIO_PIN_SELECT_REG) & ~((3 << 10) | (3 << 12) | (3 << 14)), COMCERTO_GPIO_PIN_SELECT_REG);

	/*
	 * If forced blink, don't set blink_flag. Use the old value of 
	 * blink_flag so that force works.
	 */
	if (system_blink_flag == 2) {
		return 0;
	}


	/*
	 * Used in thread function and not set if blink_flag was set to 2.
	 * The above if statement avoids blink_flag to be set.
	 */
	system_blink_flag = value;

	// user wants to pulse led
	if (value == 3) {
		__raw_writel( (__raw_readl(COMCERTO_GPIO_PIN_SELECT_REG) & ~(3 << 10)) | GPIO5_PWM1, COMCERTO_GPIO_PIN_SELECT_REG);    // GPIO5 to PWM1 SELECT
		__raw_writel( (__raw_readl(COMCERTO_GPIO_PIN_SELECT_REG) & ~(3 << 12)) | GPIO6_PWM2, COMCERTO_GPIO_PIN_SELECT_REG);    // GPIO6 to PWM1 SELECT
		__raw_writel( (__raw_readl(COMCERTO_GPIO_PIN_SELECT_REG) & ~(3 << 14)) | GPIO7_PWM3, COMCERTO_GPIO_PIN_SELECT_REG);    // GPIO7 to PWM1 SELECT

		__raw_writel(__raw_readl(COMCERTO_MAX_EN_PWM1) | (1 << 31), COMCERTO_MAX_EN_PWM1);                                     // Enable PWM1
		__raw_writel(__raw_readl(COMCERTO_MAX_EN_PWM2) | (1 << 31), COMCERTO_MAX_EN_PWM2);                                     // Enable PWM1
		__raw_writel(__raw_readl(COMCERTO_MAX_EN_PWM3) | (1 << 31), COMCERTO_MAX_EN_PWM3);                                     // Enable PWM1

	}

	// wake-up thread for blink, force-blink, or pulse
	if (value == 1 || value == 2 || value == 3)
		wake_up(&ts_wait);


	return 0;
}

static int wifi_led_blink(struct led_classdev *led_cdev,  int value) {

	printk("We are in wifi_led_blink() with value %d.\n", value);
	/*
	 * If forced blink, don't set blink_flag. Use the old value of 
	 * blink_flag so that force works.
	 */

	if (wifi_blink_flag == 2) {
		return 0;
	}

	wifi_blink_flag = value;
	// wake-up thread for blink, force-blink, or pulse
	if (value == 1 || value == 2 || value == 3)
		wake_up(&ts_wait);


	return 0;	
}


static int sequoia_blink_thread(void * data) {
	unsigned int readval, system_color, wifi_color;
	int i = 0;

	struct task_struct * tsk = current;
	struct sched_param param = { .sched_priority = 1};

	init_waitqueue_head(&ts_wait);

	sched_setscheduler(tsk, SCHED_FIFO, &param);
	set_freezable();

	while ( !kthread_should_stop() ) {
		system_led_state.cur_action = _BLINK_NO;
		wifi_led_state.cur_action = _BLINK_NO;
		/* always set current color before blinking */
		system_led_set(NULL, system_led_state.cur_color);
		wifi_led_set(NULL, wifi_led_state.cur_color);
		wait_event_freezable_timeout(ts_wait, system_blink_flag || wifi_blink_flag || kthread_should_stop(), MAX_SCHEDULE_TIMEOUT); 
		if (led_port) {
			readval = __raw_readl(led_port);                        // Read Register
			system_color = readval & SYSTEM_LED_ALL;                // Capture Current system_color
			wifi_color = readval & WIFI_LED_ALL;                    // Capture Current wifi_color
			if (system_blink_flag == _BLINK_YES) {
				system_led_state.cur_action = _BLINK_YES;       //
				write_led_gpio(SYSTEM_LED_ALL, LED_OFF);        // OFF
			}
			if (wifi_blink_flag == _BLINK_YES) {
				wifi_led_state.cur_action = _BLINK_YES;         //
				write_led_gpio(WIFI_LED_ALL, LED_OFF);          // OFF
			}

			if (wifi_blink_flag == _BLINK_YES || system_blink_flag == _BLINK_YES)
				msleep(HDD_BLINK_RATE);

			if (system_blink_flag == _BLINK_YES) {
				write_led_gpio(SYSTEM_LED_ALL, system_color);   // ON
			}
			if (wifi_blink_flag == _BLINK_YES) {
				write_led_gpio(WIFI_LED_ALL, wifi_color);       // ON
			}


			if (wifi_blink_flag == _BLINK_YES || system_blink_flag == _BLINK_YES)
				msleep(HDD_BLINK_RATE); 	

			if (system_blink_flag == _PULSE) {
				system_led_state.cur_action = _PULSE;
				switch (system_led_state.cur_color) {
					case STATE_LED_RED:
						__raw_writel(__raw_readl(COMCERTO_MAX_EN_PWM1) | 0, COMCERTO_MAX_EN_PWM1);            // Set bits 0:19 Max
						__raw_writel(__raw_readl(COMCERTO_MAX_EN_PWM2) | 0, COMCERTO_MAX_EN_PWM2);            // Set bits 0:19 Max
						__raw_writel(__raw_readl(COMCERTO_MAX_EN_PWM3) | PWM_index, COMCERTO_MAX_EN_PWM3);    // Set bits 0:19 Max
						break;
					case STATE_LED_GREEN:
						__raw_writel(__raw_readl(COMCERTO_MAX_EN_PWM1) | PWM_index, COMCERTO_MAX_EN_PWM1);    // Set bits 0:19 Max
						__raw_writel(__raw_readl(COMCERTO_MAX_EN_PWM2) | 0, COMCERTO_MAX_EN_PWM2);            // Set bits 0:19 Max
						__raw_writel(__raw_readl(COMCERTO_MAX_EN_PWM3) | 0, COMCERTO_MAX_EN_PWM3);            // Set bits 0:19 Max
						break;
					case STATE_LED_BLUE:
						__raw_writel(__raw_readl(COMCERTO_MAX_EN_PWM1) | 0, COMCERTO_MAX_EN_PWM1);            // Set bits 0:19 Max
						__raw_writel(__raw_readl(COMCERTO_MAX_EN_PWM2) | PWM_index, COMCERTO_MAX_EN_PWM2);    // Set bits 0:19 Max
						__raw_writel(__raw_readl(COMCERTO_MAX_EN_PWM3) | 0, COMCERTO_MAX_EN_PWM3);            // Set bits 0:19 Max
						break;
//					case STATE_LED_OFF:
//						__raw_writel(__raw_readl(COMCERTO_MAX_EN_PWM1) | 0, COMCERTO_MAX_EN_PWM1);            // Set bits 0:19 Max
//						__raw_writel(__raw_readl(COMCERTO_MAX_EN_PWM2) | 0, COMCERTO_MAX_EN_PWM2);            // Set bits 0:19 Max
//						__raw_writel(__raw_readl(COMCERTO_MAX_EN_PWM3) | 0, COMCERTO_MAX_EN_PWM3);            // Set bits 0:19 Max
//						break;
					case STATE_LED_ALL:
						__raw_writel(__raw_readl(COMCERTO_MAX_EN_PWM1) | 0, COMCERTO_MAX_EN_PWM1);            // Set bits 0:19 Max
						__raw_writel(__raw_readl(COMCERTO_MAX_EN_PWM2) | 0, COMCERTO_MAX_EN_PWM2);            // Set bits 0:19 Max
						__raw_writel(__raw_readl(COMCERTO_MAX_EN_PWM3) | PWM_index, COMCERTO_MAX_EN_PWM3);    // Set bits 0:19 Max
						break;
					case STATE_LED_YELLOW:
						__raw_writel(__raw_readl(COMCERTO_MAX_EN_PWM1) | PWM_index, COMCERTO_MAX_EN_PWM1);    // Set bits 0:19 Max
						__raw_writel(__raw_readl(COMCERTO_MAX_EN_PWM2) | 0, COMCERTO_MAX_EN_PWM2);            // Set bits 0:19 Max
						__raw_writel(__raw_readl(COMCERTO_MAX_EN_PWM3) | PWM_index, COMCERTO_MAX_EN_PWM3);    // Set bits 0:19 Max
						break;
					default:
					break; /* should never be here */
				}
				for (i=1; i<PWM_index; i++) {
					__raw_writel(i, COMCERTO_LOW_DUTY_PWM1);            // Set bits 0:19 Low Duty
					__raw_writel(i, COMCERTO_LOW_DUTY_PWM2);            // Set bits 0:19 Low Duty
					__raw_writel(i, COMCERTO_LOW_DUTY_PWM3);            // Set bits 0:19 Low Duty			
					msleep(50);
				}
				for (i=PWM_index-1; i>=0; i--) {
					__raw_writel(i, COMCERTO_LOW_DUTY_PWM1);            // Set bits 0:19 Low Duty
					__raw_writel(i, COMCERTO_LOW_DUTY_PWM2);            // Set bits 0:19 Low Duty
					__raw_writel(i, COMCERTO_LOW_DUTY_PWM3);            // Set bits 0:19 Low Duty
					msleep(50);
				}
			}
		}
	}


	return 0;
}


