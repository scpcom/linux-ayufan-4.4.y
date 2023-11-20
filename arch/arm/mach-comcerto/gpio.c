/*
 *  linux/arch/arm/mach-comcerto/gpio.c
 */

#include <linux/module.h>
#include <linux/init.h>
#include <asm/irq.h>
#include <asm/delay.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/syscalls.h>
#include <linux/kmod.h>
#include <linux/fs.h>
#include <asm/fcntl.h>
#include <asm/hardirq.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <asm/io.h>
#include <linux/spinlock.h>
#include <mach/hardware.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/signal.h>
#include <asm/segment.h>
#include <linux/buffer_head.h>
#include <linux/slab.h>

#include <mach/comcerto-2000/gpio.h>
#include <mach/comcerto-2000/timer.h>
#include <mach/irqs.h>

MODULE_LICENSE("GPL v2");
#define MODULE_NAME "gpio"

/** 0 ~ 31 bit **/
#define GPIO_LOW_DATA_OUT				COMCERTO_GPIO_OUTPUT_REG
#define GPIO_LOW_DATA_OUT_ENABLE		COMCERTO_GPIO_OE_REG
#define GPIO_LOW_DATA_IN				COMCERTO_GPIO_INPUT_REG

/** 32 ~ 63 bit **/
#define GPIO_HIGH_DATA_OUT				COMCERTO_GPIO_63_32_PIN_OUTPUT
#define GPIO_HIGH_DATA_OUT_ENABLE		COMCERTO_GPIO_63_32_PIN_OUTPUT_EN
#define GPIO_HIGH_DATA_IN				COMCERTO_GPIO_63_32_PIN_INPUT

#define GPIO_DATA_OUT(gpio_bit)     	((gpio_bit > 31) ? GPIO_HIGH_DATA_OUT : GPIO_LOW_DATA_OUT)
#define GPIO_DATA_OUT_ENABLE(gpio_bit)  ((gpio_bit > 31) ? GPIO_HIGH_DATA_OUT_ENABLE : GPIO_LOW_DATA_OUT_ENABLE)
#define GPIO_DATA_IN(gpio_bit)      	((gpio_bit > 31) ? GPIO_HIGH_DATA_IN : GPIO_LOW_DATA_IN)	

#define GPIO_BIT_SET_OFFSET(gpio_bit) ((gpio_bit & 0x1f))
#define INPUT_PIN_TRIGGERED(gpio_bit) \
		( gpio_readl(GPIO_DATA_IN(gpio_bit)) & 0x1 << GPIO_BIT_SET_OFFSET(gpio_bit))

/** define gpio offset **/
#define HDD1_DETECT_REG_OFFSET			0
#define HDD2_DETECT_REG_OFFSET          1
#define HDD3_DETECT_REG_OFFSET          2
#define HDD4_DETECT_REG_OFFSET          3
#define POWER_BUTTON_REG_OFFSET			4
#define RESET_BUTTON_REG_OFFSET			5
#define COPY_BUTTON_REG_OFFSET			6
#define HTP_GPIO_REG_OFFSET				7
#define HDD1_CTRL_REG_OFFSET			8
#define HDD2_CTRL_REG_OFFSET            9
#define HDD3_CTRL_REG_OFFSET            10
#define HDD4_CTRL_REG_OFFSET            11
#define PWREN_USB_REG_OFFSET            14
#define POWER_OFF_REG_OFFSET			15
#define MCU_WDT_REG_OFFSET				39
#define HDD1_LED_GREEN_REG_OFFSET		48
#define HDD1_LED_RED_REG_OFFSET			49
#define HDD2_LED_GREEN_REG_OFFSET		50
#define HDD2_LED_RED_REG_OFFSET			51
#define HDD3_LED_GREEN_REG_OFFSET		52
#define HDD3_LED_RED_REG_OFFSET			53
#define HDD4_LED_GREEN_REG_OFFSET		54
#define HDD4_LED_RED_REG_OFFSET			55
#define SYS_LED_GREEN_REG_OFFSET		56
#define SYS_LED_RED_REG_OFFSET			57
#define COPY_LED_GREEN_REG_OFFSET		58
#define COPY_LED_RED_REG_OFFSET			59

/* define the magic number for ioctl used */
#define BTNCPY_IOC_MAGIC 'g'
#define BTNCPY_IOC_SET_NUM		_IO(BTNCPY_IOC_MAGIC, 1)
#define LED_SET_CTL_IOC_NUM     _IO(BTNCPY_IOC_MAGIC, 2)
#define BUZ_SET_CTL_IOC_NUM 	_IO(BTNCPY_IOC_MAGIC, 4)
#define BUTTON_TEST_IN_IOC_NUM  _IO(BTNCPY_IOC_MAGIC, 9)
#define BUTTON_TEST_OUT_IOC_NUM _IO(BTNCPY_IOC_MAGIC, 10)

/* data structure for passing HDD state */
typedef struct _hdd_ioctl {
	unsigned int port;  /* HDD_PORT_NUM  */
	unsigned int state; /* ON, OFF */
} hdd_ioctl;

#define HDD_SET_CTL_IOC_NUM     _IOW(BTNCPY_IOC_MAGIC, 21, hdd_ioctl)

#if 0
/** define interrupt irq **/
#define POWER_BUTTON_IRQ				IRQ_G4
#define RESET_BUTTON_IRQ				IRQ_G5
#define COPY_BUTTON_IRQ					IRQ_G6
#endif

/** define the timer period **/
#define JIFFIES_1_SEC       (HZ)        /* 1 second */
#define JIFFIES_BLINK_VERYSLOW  (JIFFIES_1_SEC * 2)	// 2s
#define JIFFIES_BLINK_VERYSLOW_ON  (JIFFIES_1_SEC)	// 0.5s - according to the request from ZyXEL, HDD LED flash frequence: 0.5s on and 1.5s off
#define JIFFIES_BLINK_VERYSLOW_OFF  (JIFFIES_1_SEC * 4)	// 1.5s - according to the request from ZyXEL, HDD LED flash frequence: 0.5s on and 1.5s off
#define JIFFIES_BLINK_SLOW  (JIFFIES_1_SEC / 2)
#define JIFFIES_BLINK_FAST  (JIFFIES_1_SEC / 10)

#define TIMER_RUNNING   0x1
#define TIMER_SLEEPING  0x0

#define QUICK_PRESS_TIME    1
#define PRESS_TIME      5
#define BEEP_DURATION   1000

/** define the LED settings **/
#define LED_COLOR_BITS  0   // 0,1
#define LED_STATE_BITS  2   // 2,3,4,5,6,7
#define LED_NUM_BITS    8   // 8,9,10 ...

#define GET_LED_INDEX(map_addr) ((map_addr >> LED_NUM_BITS) & 0xf)
#define GET_LED_COLOR(map_addr) (map_addr & 0x3)
#define GET_LED_STATE(map_addr) ((map_addr >> LED_STATE_BITS) & 0x7)

#define RED 	    (1<<0)
#define GREEN       (2<<0)
#define ORANGE      (RED | GREEN)
#define NO_COLOR	0

/** define the buzzer settings **/
#define BZ_TIMER_PERIOD (HZ/2)
#define RING_FOREVER 1 
#define RING_BRIEF 0

#define TIME_BITS       5
#define FREQ_BITS       4
#define STATE_BITS      2

#define TIME_MASK       (0x1F << (FREQ_BITS + STATE_BITS))
#define FREQ_MASK       (0xF << STATE_BITS)
#define STATE_MASK      0x3

#define GET_TIME(addr)  ((addr & TIME_MASK) >> (FREQ_BITS + STATE_BITS))
#define GET_FREQ(addr)  ((addr & FREQ_MASK) >> STATE_BITS)
#define GET_STATE(addr) (addr & STATE_MASK)

#define BUTTON_NUM      3

enum BUTTON_NUMBER {
	RESET_BTN_NUM,
	COPY_BTN_NUM,
	POWER_BTN_NUM,
};

typedef enum {
	BUZ_OFF = 0,            /* turn off buzzer */
	BUZ_ON,
	BUZ_KILL,               /* kill buzzer daemon, equaling to BUZ_OFF*/
	BUZ_FOREVER             /* keep buzzing */
} buz_cmd_t;

enum LED_STATE {
	LED_OFF = 0,
	LED_ON,
	LED_BLINK_SLOW,
	LED_BLINK_FAST,
	LED_BLINK_VERYSLOW,
};

enum LED_ID {
	LED_HDD1 = 0,
	LED_HDD2,
	LED_HDD3,
	LED_HDD4,
	LED_SYS,
	LED_COPY,
	LED_TOTAL,
};

enum LED_COLOR {
	LED_RED = 0,
	LED_GREEN,
	LED_COLOR_TOTAL,    /* must be last one */
};

static struct timer_list        bz_timer;
static short bz_time;
static short bz_timer_status = TIMER_SLEEPING;
static short bz_type = RING_BRIEF;

dev_t gpio_dev = 0;
static int gpio_nr_devs = 1;
struct cdev *gpio_cdev;
static int is_mcu_burning = 0;

static struct proc_dir_entry *htp_proc;
static struct proc_dir_entry *hdd1_detect_proc;
static struct proc_dir_entry *hdd2_detect_proc;
static struct proc_dir_entry *hdd3_detect_proc;
static struct proc_dir_entry *hdd4_detect_proc;
static struct proc_dir_entry *pwren_usb_proc;
static struct proc_dir_entry *mcu_wdt_proc;

static struct timer_list    btnpow_timer;
static struct timer_list    btnreset_timer;
static struct timer_list    btncpy_timer;

struct LED {
	unsigned int gpio;
	unsigned int color;
	unsigned short state;       /* LED_OFF, LED_ON, LED_BLINK_SLOW, ... */
	
	unsigned short presence;    /* flag. 0: no such LED color */
};

struct LED_SET {
	unsigned int id;            /* LED ID, LED type */
	char name[32];
	struct LED led[LED_COLOR_TOTAL];
	
	struct timer_list timer;
	unsigned short timer_state;
	unsigned short blink_state; /* Binary state, it must be 0 (off) or 1 (on) to present the blinking state */
	
	spinlock_t lock;
	
	unsigned short presence;    /* flag. 0: no such LED */
};

static int pow_polling_times = 0;
static int reset_polling_times = 0;
static int cpy_polling_times = 0;

static atomic_t button_test_enable = ATOMIC_INIT(0);
static atomic_t button_test_num = ATOMIC_INIT(BUTTON_NUM);
static atomic_t pow_button_pressed = ATOMIC_INIT(0);
static atomic_t reset_button_pressed = ATOMIC_INIT(0);
static atomic_t cpy_button_pressed = ATOMIC_INIT(0);
static atomic_t halt_one_time = ATOMIC_INIT(0);

struct workqueue_struct *btn_workqueue;

// trigger signal to button daemon in user space
static int  btncpy_pid = 0;
void btncpy_signal_func10(struct work_struct *in);
void btncpy_signal_func12(struct work_struct *in);
void btncpy_signal_func14(struct work_struct *in);
void nsa_shutdown_func(struct work_struct *in);
void Reset_UserInfo_func(struct work_struct *in);
void Open_Backdoor_func(struct work_struct *in);
void Reset_To_Defu_func(struct work_struct *in);
static DECLARE_WORK(btncpy_signal10, btncpy_signal_func10);
static DECLARE_WORK(btncpy_signal12, btncpy_signal_func12);
static DECLARE_WORK(btncpy_signal14, btncpy_signal_func14);
static DECLARE_WORK(halt_nsa, nsa_shutdown_func);
static DECLARE_WORK(Reset_User_Info, Reset_UserInfo_func);
static DECLARE_WORK(Open_Backdoor, Open_Backdoor_func);
static DECLARE_WORK(Reset_To_Default, Reset_To_Defu_func);

static void led_timer_handler(unsigned long);

struct LED_SET led_set[LED_TOTAL] = {
	[LED_HDD1] = {
		.presence = 1,
		.id = LED_HDD1,
		.name = "HDD1 LED",
		.timer = {
			.data = 0,
			.function = led_timer_handler,
		},
		.led[LED_RED] = {
			.presence = 1,
			.gpio = HDD1_LED_RED_REG_OFFSET,
			.color = RED,
		},
		.led[LED_GREEN] = {
			.presence = 1,
			.gpio = HDD1_LED_GREEN_REG_OFFSET,
			.color = GREEN,
		},
	},
	[LED_HDD2] = {
		.presence = 1,
		.id = LED_HDD2,
		.name = "HDD2 LED",
		.timer = {
			.data = 0,
			.function = led_timer_handler,
		},
		.led[LED_RED] = {
			.presence = 1,
			.gpio = HDD2_LED_RED_REG_OFFSET,
			.color = RED,
		},
		.led[LED_GREEN] = {
			.presence = 1,
			.gpio = HDD2_LED_GREEN_REG_OFFSET,
			.color = GREEN,
		},
	},
	[LED_HDD3] = {
		.presence = 1,
		.id = LED_HDD3,
		.name = "HDD3 LED",
		.timer = {
			.data = 0,
			.function = led_timer_handler,
		},
		.led[LED_RED] = {
			.presence = 1,
			.gpio = HDD3_LED_RED_REG_OFFSET,
			.color = RED,
		},
		.led[LED_GREEN] = {
			.presence = 1,
			.gpio = HDD3_LED_GREEN_REG_OFFSET,
			.color = GREEN,
		},
	},
	[LED_HDD4] = {
		.presence = 1,
		.id = LED_HDD4,
		.name = "HDD4 LED",
		.timer = {
			.data = 0,
			.function = led_timer_handler,
		},
		.led[LED_RED] = {
			.presence = 1,
			.gpio = HDD4_LED_RED_REG_OFFSET,
			.color = RED,
		},
		.led[LED_GREEN] = {
			.presence = 1,
			.gpio = HDD4_LED_GREEN_REG_OFFSET,
			.color = GREEN,
		},
	},
	[LED_SYS] = {
		.presence = 1,
		.id = LED_SYS,
		.name = "SYS LED",
		.timer = {
			.data = 0,
			.function = led_timer_handler,
		},
		.led[LED_RED] = {
			.presence = 1,
			.gpio = SYS_LED_RED_REG_OFFSET,
			.color = RED,
		},
		.led[LED_GREEN] = {
			.presence = 1,
			.gpio = SYS_LED_GREEN_REG_OFFSET,
			.color = GREEN,
		},
	},
	[LED_COPY] = {
		.presence = 1,
		.id = LED_COPY,
		.name = "COPY LED",
		.timer = {
			.data = 0,
			.function = led_timer_handler,
		},
		.led[LED_RED] = {
			.presence = 1,
			.gpio = COPY_LED_RED_REG_OFFSET,
			.color = RED,
		},
		.led[LED_GREEN] = {
			.presence = 1,
			.gpio = COPY_LED_GREEN_REG_OFFSET,
			.color = GREEN,
		},
	},
};


// MCU burning related functions
mm_segment_t oldfs;

typedef struct _mcu_ioctl 
{
	unsigned int r_mode;	/* ROM Select */
	unsigned int m_mode;	/* Mode Select */
	unsigned int s_mode;	/* Serial Select */
} mcu_ioctl;

typedef struct _mcu_burn 
{
	char path[1024];		/* ROM Select */
} mcu_burn;

static void mcu_reset_func(struct work_struct *in);
static void power_resume_always_on_func(struct work_struct *in);
static void mcu_assign_lock_func(struct work_struct *in);
static DECLARE_WORK(mcu_reset_control, mcu_reset_func);
static DECLARE_WORK(mcu_power_resume, power_resume_always_on_func);
static DECLARE_WORK(mcu_assign_lock, mcu_assign_lock_func);
struct workqueue_struct *mcu_workqueue;

#define MCU_SDATA_REG_OFFSET			35
#define MCU_SCLK_REG_OFFSET				36
#define MCU_RESB_REG_OFFSET				37
#define MCU_BI_REG_OFFSET				38

#define BUTTON_TEST_IN_IOC_NUM  _IO(BTNCPY_IOC_MAGIC, 9)
#define BUTTON_TEST_OUT_IOC_NUM _IO(BTNCPY_IOC_MAGIC, 10)

/* MCU burning related information */
#define NAS_IOC_MCU_TEST			_IOW(BTNCPY_IOC_MAGIC, 15, mcu_ioctl)
#define NAS_IOC_MCU_BURNING         _IOW(BTNCPY_IOC_MAGIC, 16, mcu_burn)
#define NAS_IOC_MCU_ERASE_ALL       _IO(BTNCPY_IOC_MAGIC, 17)

/* set "gpio_bit" to "output" pin and output "out_val" to GPIO pin "gpio_bit" */
void set_gpio_output(int gpio_bit, unsigned char out_val)
{
	unsigned long reg, val;

	/* set gpio_bit to "output" pin */
	reg = GPIO_DATA_OUT_ENABLE(gpio_bit);
	if (gpio_bit > 31)	// active low: 0 -> output, 1 -> input
		val = gpio_readl(reg) & ~(0x1 << GPIO_BIT_SET_OFFSET(gpio_bit));
	else	// active high: 1 -> output, 0 -> input
		val = gpio_readl(reg) | (0x1 << GPIO_BIT_SET_OFFSET(gpio_bit));
	gpio_writel(val, reg);

	// output "out_val" to gpio_bit
	reg = GPIO_DATA_OUT(gpio_bit);

	if(out_val & 0x1)
		gpio_writel(gpio_readl(reg) | (0x1 << GPIO_BIT_SET_OFFSET(gpio_bit)), reg);	// output "1"
	else
		gpio_writel(gpio_readl(reg) & ~(0x1 << GPIO_BIT_SET_OFFSET(gpio_bit)), reg);	// output "0"
}

/* set "gpio_bit" to "input" pin */
void set_gpio_input(int gpio_bit)
{
	unsigned long reg, val;

	reg = GPIO_DATA_OUT_ENABLE(gpio_bit);
	if (gpio_bit > 31)	// active low: 0 -> output, 1 -> input
		val = gpio_readl(reg) | (0x1 << GPIO_BIT_SET_OFFSET(gpio_bit));
	else	// active high: 1 -> output, 0 -> input
		val = gpio_readl(reg) & ~(0x1 << GPIO_BIT_SET_OFFSET(gpio_bit));
	gpio_writel(val, reg);
}

void power_resume_always_on_func(struct work_struct *in)
{
	/* setting power resume always on */
	int ret;
	char *argv[] = {"/sbin/i2cset", "-y", "0", "0xa", "0xa", "0x0107", "w", NULL};
	ret = call_usermodehelper(argv[0], argv, NULL, 0);
}

void mcu_assign_lock_func(struct work_struct *in)
{
	int ret;
	char *argv[] = {"/firmware/sbin/info_setenv", "mcu_lock", "1", NULL};
	ret = call_usermodehelper(argv[0], argv, NULL, 0);	
}

void mcu_reset_func(struct work_struct *in)
{
//	unsigned long reg;

	printk(KERN_ALERT "MCU Resetting...\n");
	ssleep(1);

	/* mcu reset pin */
	set_gpio_output(MCU_RESB_REG_OFFSET, 0x0);
	//reg = GPIO_DATA_OUT(MCU_RESB_REG_OFFSET);
	//gpio_writel(gpio_readl(reg) & ~(0x1 << GPIO_BIT_SET_OFFSET(MCU_RESB_REG_OFFSET)), reg);

	set_gpio_output(MCU_BI_REG_OFFSET, 0x1);
	//reg = GPIO_DATA_OUT(MCU_BI_REG_OFFSET);
	//gpio_writel(gpio_readl(reg) | (0x1 << GPIO_BIT_SET_OFFSET(MCU_BI_REG_OFFSET)), reg);
	mdelay(10);

	set_gpio_output(MCU_RESB_REG_OFFSET, 0x1);
	//reg = GPIO_DATA_OUT(MCU_RESB_REG_OFFSET);
	//gpio_writel(gpio_readl(reg) | (0x1 << GPIO_BIT_SET_OFFSET(MCU_RESB_REG_OFFSET)), reg);
	mdelay(10);

	set_gpio_output(MCU_RESB_REG_OFFSET, 0x0);
	//reg = GPIO_DATA_OUT(MCU_RESB_REG_OFFSET);
	//gpio_writel(gpio_readl(reg) & ~(0x1 << GPIO_BIT_SET_OFFSET(MCU_RESB_REG_OFFSET)), reg);
	mdelay(10);

	set_gpio_output(MCU_BI_REG_OFFSET, 0x0);
	//reg = GPIO_DATA_OUT(MCU_BI_REG_OFFSET);
	//gpio_writel(gpio_readl(reg) & ~(0x1 << GPIO_BIT_SET_OFFSET(MCU_BI_REG_OFFSET)), reg);
}


struct file *openFile(char *path,int flag,int mode)
{
	struct file *fp;
	
	fp=filp_open(path, flag, 0);
	if (fp) return fp;
		else return NULL;
}


int readFile(struct file *fp,char *buf,int gpio_readlen)
{
	if (fp->f_op && fp->f_op->read) {
		return fp->f_op->read(fp,buf,gpio_readlen, &fp->f_pos);
	} else {
		return -1;
	}
}

void closeFile(struct file *fp)
{
	filp_close(fp,NULL);
}

void initKernelEnv(void)
{
	oldfs = get_fs();
	set_fs(KERNEL_DS);
}

int do_mcu_program_entry(unsigned int r_mode, unsigned int m_mode, unsigned int s_mode)
{
	unsigned int data[26] = {0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0};
	unsigned int mode_sel[8][4] = {{0,0,0,0},{0,0,0,1},{0,1,1,0},{0,1,1,1},{1,0,0,0},{1,0,0,1},{1,1,0,0},{1,1,1,0}};
	int i, mcu_sclk_gpio, mcu_resb_gpio, mcu_sdata_gpio, mcu_bi_gpio;
	int check_ack[8] = {1,1,r_mode,mode_sel[m_mode][0],mode_sel[m_mode][1],mode_sel[m_mode][2],mode_sel[m_mode][3],s_mode};
	int read_ack[8];
	int check_fail_flag;

	mcu_sclk_gpio = MCU_SCLK_REG_OFFSET;
	mcu_resb_gpio = MCU_RESB_REG_OFFSET;
	mcu_sdata_gpio = MCU_SDATA_REG_OFFSET;
	mcu_bi_gpio = MCU_BI_REG_OFFSET;
																																									        
	set_gpio_output(mcu_resb_gpio, 0x0);
	set_gpio_output(mcu_sclk_gpio, 0x0);
	set_gpio_output(mcu_bi_gpio, 0x1);
    mdelay(10);

    set_gpio_output(mcu_sclk_gpio, 0x1);
    mdelay(10);

    set_gpio_output(mcu_resb_gpio, 0x1);
    mdelay(10);

    set_gpio_output(mcu_resb_gpio, 0x0);

    for (i=0; i<26; i++) {
		set_gpio_output(mcu_sclk_gpio, 0x1);
		set_gpio_output(mcu_sdata_gpio, data[i]);
		udelay(3);
	    set_gpio_output(mcu_sclk_gpio, 0x0);
		udelay(3);
		set_gpio_output(mcu_sclk_gpio, 0x1);
		udelay(3);
	}

	set_gpio_output(mcu_sclk_gpio, 0x1);
	set_gpio_output(mcu_sdata_gpio, r_mode);
	udelay(3);
	set_gpio_output(mcu_sclk_gpio, 0x0);
	udelay(3);
	set_gpio_output(mcu_sclk_gpio, 0x1);
	udelay(3);
	
	for (i=0; i<4; i++) {
		set_gpio_output(mcu_sclk_gpio, 0x1);
		set_gpio_output(mcu_sdata_gpio, mode_sel[m_mode][i]);
		udelay(3);
		set_gpio_output(mcu_sclk_gpio, 0x0);
		udelay(3);
		set_gpio_output(mcu_sclk_gpio, 0x1);
		udelay(3);
	}

	set_gpio_output(mcu_sclk_gpio, 0x1);
	set_gpio_output(mcu_sdata_gpio, s_mode);
	udelay(3);
	set_gpio_output(mcu_sclk_gpio, 0x0);
	udelay(3);
	set_gpio_output(mcu_sclk_gpio, 0x1);
	udelay(3);
																																																															
	set_gpio_input(mcu_sdata_gpio);
	udelay(100);
	for (i=0; i<8; i++) {
		set_gpio_output(mcu_sclk_gpio, 0x1);
		udelay(5);
		set_gpio_output(mcu_sclk_gpio, 0x0);
		read_ack[i] = (gpio_readl(GPIO_DATA_IN(mcu_sdata_gpio)) & 0x1 << GPIO_BIT_SET_OFFSET(mcu_sdata_gpio)) ? 0x1 : 0x0;
		udelay(5);
		set_gpio_output(mcu_sclk_gpio, 0x1);
		udelay(5);
	}
	
	set_gpio_output(mcu_resb_gpio, 0x1);
	
	// check read ack
	check_fail_flag = 0;
	for (i=0; i<8; i++) {
		if (r_mode == 1) {
			// Mode3~Mode0 don't care
			if (i > 2 && i < 7) continue;
		}
		if (read_ack[i] != check_ack[i]) {
			check_fail_flag = 1;
			break;
		}
	}

	if (check_fail_flag) {
		printk(KERN_ERR "[do_mcu_program_entry] Cannot Enter Programming Mode...\n");
		printk("[do_mcu_program_entry] The reading ACK is (%d%d%d%d%d%d%d%d)...\n", read_ack[0],read_ack[1],read_ack[2],read_ack[3],read_ack[4],read_ack[5],read_ack[6],read_ack[7]);
		printk("[do_mcu_program_entry] The checking ACK is (%d%d%d%d%d%d%d%d)...\n", check_ack[0],check_ack[1],check_ack[2],check_ack[3],check_ack[4],check_ack[5],check_ack[6],check_ack[7]);
		return -1;
	} else {
		//printk(KERN_ALERT "[do_mcu_program_entry] Entering Programming Mode Successfully...\n");
		mdelay(10);
	}

	// for 1024 dummy clock
	for (i=0; i<1024; i++) {
		set_gpio_output(mcu_sclk_gpio, 0x1);
		udelay(3);
		set_gpio_output(mcu_sclk_gpio, 0x0);
		udelay(3);
		set_gpio_output(mcu_sclk_gpio, 0x1);
		udelay(3);
	}

	mdelay(20);
	return 0;
}

void mcu_erase_all(void)
{
	unsigned int r_mode = 0;
	unsigned int m_mode = 6;
	unsigned int s_mode = 1;

	int i, mcu_sclk_gpio, mcu_resb_gpio, mcu_sdata_gpio, mcu_bi_gpio, entry_flag;

	mcu_sclk_gpio = MCU_SCLK_REG_OFFSET;
	mcu_resb_gpio = MCU_RESB_REG_OFFSET;
	mcu_sdata_gpio = MCU_SDATA_REG_OFFSET;
	mcu_bi_gpio = MCU_BI_REG_OFFSET;
	
	entry_flag = do_mcu_program_entry(r_mode, m_mode, s_mode);
	if (entry_flag == 0) {
		// send 20 bits address
		for (i=0; i<20; i++) {
			set_gpio_output(mcu_sclk_gpio, 0x1);
			set_gpio_output(mcu_sdata_gpio, 0x0);
			udelay(3);
			set_gpio_output(mcu_sclk_gpio, 0x0);
			udelay(3);
			set_gpio_output(mcu_sclk_gpio, 0x1);
			udelay(3);
		}
		
		udelay(3);

		//send 64 bits data
		for (i=0; i<64; i++) {
			set_gpio_output(mcu_sclk_gpio, 0x1);
			set_gpio_output(mcu_sdata_gpio, 0x0);
			udelay(3);
			set_gpio_output(mcu_sclk_gpio, 0x0);
			if (i >= 62) {
				udelay(2000);
			} else {
				udelay(3);
			}
			set_gpio_output(mcu_sclk_gpio, 0x1);
			udelay(3);
		}											
	}
	
	mdelay(10);
	set_gpio_output(mcu_bi_gpio, 0x0);
	set_gpio_output(mcu_resb_gpio, 0x0);
	udelay(2000);
	set_gpio_input(mcu_resb_gpio);
		
	printk(KERN_ALERT "[mcu_erase_all] MCU erases all Successfully...\n");
}

int mcu_program_check(int bit_64_num, long mcu_header, int checksum)
{
	unsigned int r_mode = 0;
	unsigned int m_mode = 1;
	unsigned int s_mode = 1;

	int i, j, mcu_sclk_gpio, mcu_resb_gpio, mcu_sdata_gpio, mcu_bi_gpio, entry_flag;
	int return_num = 0;
	long check_value = 0;
	check_value += mcu_header;

	mcu_sclk_gpio = MCU_SCLK_REG_OFFSET;
	mcu_resb_gpio = MCU_RESB_REG_OFFSET;
	mcu_sdata_gpio = MCU_SDATA_REG_OFFSET;
	mcu_bi_gpio = MCU_BI_REG_OFFSET;
	
	entry_flag = do_mcu_program_entry(r_mode, m_mode, s_mode);
	return_num = entry_flag;
																				
	if (entry_flag == 0) {
		// send 20 bits address
		for (i=0; i<20; i++) {
			set_gpio_output(mcu_sclk_gpio, 0x1);
			set_gpio_output(mcu_sdata_gpio, 0x0);
			udelay(3);
			set_gpio_output(mcu_sclk_gpio, 0x0);
			udelay(3);
			set_gpio_output(mcu_sclk_gpio, 0x1);
			udelay(3);
		}	

		udelay(3);

		//receive 64 bits data
		set_gpio_input(mcu_sdata_gpio);
		for (i=0; i<bit_64_num; i++) {
			for (j=0; j<64; j++) {
				set_gpio_output(mcu_sclk_gpio, 0x1);
				udelay(5);
				set_gpio_output(mcu_sclk_gpio, 0x0);
				check_value += (1 << (j % 8)) * ((gpio_readl(GPIO_DATA_IN(mcu_sdata_gpio)) & 0x1 << GPIO_BIT_SET_OFFSET(mcu_sdata_gpio)) ? 0x1 : 0x0);
				udelay(5);
				set_gpio_output(mcu_sclk_gpio, 0x1);
				udelay(5);
			}
		}
	
		if ((check_value + checksum) % 256 == 0) {
			printk(KERN_ALERT "[mcu_program_check] Verify Program ROM CheckSum is OK...\n");
		} else {
			printk(KERN_ERR "[mcu_program_check] Writing Program ROM unsuccessfully(check_value:%ld, checksum:%d, mcu_header:%ld)...\n", check_value, checksum, mcu_header);
			return_num = -1;
		}	
	}	
																					
	mdelay(10);
	set_gpio_output(mcu_bi_gpio, 0x0);
	set_gpio_output(mcu_resb_gpio, 0x0);
	udelay(2000);
	set_gpio_input(mcu_resb_gpio);

	return return_num;
}

void mcu_write_program(int **Array, int bit_64_num)
{
	unsigned int r_mode = 0;
	unsigned int m_mode = 3;
	unsigned int s_mode = 1;

	int i, j, mcu_sclk_gpio, mcu_resb_gpio, mcu_sdata_gpio, mcu_bi_gpio, entry_flag;

	mcu_sclk_gpio = MCU_SCLK_REG_OFFSET;
	mcu_resb_gpio = MCU_RESB_REG_OFFSET;
	mcu_sdata_gpio = MCU_SDATA_REG_OFFSET;
	mcu_bi_gpio = MCU_BI_REG_OFFSET;
	
	entry_flag = do_mcu_program_entry(r_mode, m_mode, s_mode);

	if (entry_flag == 0) {
		// send 20 bits address
		for (i=0; i<20; i++) {
			set_gpio_output(mcu_sclk_gpio, 0x1);
			set_gpio_output(mcu_sdata_gpio, 0x0);
			udelay(3);
			set_gpio_output(mcu_sclk_gpio, 0x0);
			udelay(3);
			set_gpio_output(mcu_sclk_gpio, 0x1);
			udelay(3);
		}

		udelay(3);

		//send 64 bits data
		for (i=0; i<bit_64_num; i++) {
			for (j=0; j<64; j++) {
				set_gpio_output(mcu_sclk_gpio, 0x1);
				set_gpio_output(mcu_sdata_gpio, Array[i][j]);
				udelay(3);
				set_gpio_output(mcu_sclk_gpio, 0x0);
				if (j > 62) {
					udelay(2000);
				} else {
					udelay(3);
				}
				set_gpio_output(mcu_sclk_gpio, 0x1);
				udelay(3);
			}
		}	
	}
	
	mdelay(10);
	set_gpio_output(mcu_bi_gpio, 0x0);
	set_gpio_output(mcu_resb_gpio, 0x0);
	udelay(2000);
	set_gpio_input(mcu_resb_gpio);
	
	printk(KERN_ALERT "[mcu_write_program] Writing Program ROM finish...\n");
}

int mcu_data_check(int bit_8_num, long mcu_header, int checksum)
{
	unsigned int r_mode = 1;
	unsigned int m_mode = 0;
	unsigned int s_mode = 1;

	int i = 0, j = 0, mcu_sclk_gpio, mcu_resb_gpio, mcu_sdata_gpio, mcu_bi_gpio, entry_flag;
	int return_num = 0;
	long check_value = 0;

	int mcu_addr[bit_8_num][9];
	int sum;

	check_value += mcu_header;

	for (i=0; i<bit_8_num; i++) {
		sum = i;
		do {
			mcu_addr[i][j] = sum % 2;
			sum = sum/2;
			j++;
		} while (j % 9 != 0);
		j=0;
	}

	mcu_sclk_gpio = MCU_SCLK_REG_OFFSET;
	mcu_resb_gpio = MCU_RESB_REG_OFFSET;
	mcu_sdata_gpio = MCU_SDATA_REG_OFFSET;
	mcu_bi_gpio = MCU_BI_REG_OFFSET;
	
	entry_flag = do_mcu_program_entry(r_mode, m_mode, s_mode);
	return_num = entry_flag;

	if (entry_flag == 0) {
		for (i=0; i<bit_8_num; i++) {
			// send start bit
			set_gpio_output(mcu_sclk_gpio, 0x1);
			set_gpio_output(mcu_sdata_gpio, 0x1);
			udelay(3);
			set_gpio_output(mcu_sclk_gpio, 0x0);
			udelay(3);
			set_gpio_output(mcu_sclk_gpio, 0x1);
			udelay(3);

			// send Op code
			set_gpio_output(mcu_sclk_gpio, 0x1);
			set_gpio_output(mcu_sdata_gpio, 0x1);
			udelay(3);
			set_gpio_output(mcu_sclk_gpio, 0x0);
			udelay(3);
			set_gpio_output(mcu_sclk_gpio, 0x1);
			udelay(3);

			set_gpio_output(mcu_sclk_gpio, 0x1);
			set_gpio_output(mcu_sdata_gpio, 0x0);
			udelay(3);
			set_gpio_output(mcu_sclk_gpio, 0x0);
			udelay(3);
			set_gpio_output(mcu_sclk_gpio, 0x1);
			udelay(3);

			// send 9 bits address
			for (j=8; j>=0; j--) {
				set_gpio_output(mcu_sclk_gpio, 0x1);
				set_gpio_output(mcu_sdata_gpio, mcu_addr[i][j]);
				udelay(3);
				set_gpio_output(mcu_sclk_gpio, 0x0);
				udelay(3);
				set_gpio_output(mcu_sclk_gpio, 0x1);
				udelay(3);
			}

			// receive 8 bits data
			set_gpio_input(mcu_sdata_gpio);
			for (j=7; j>=0; j--) {
				set_gpio_output(mcu_sclk_gpio, 0x1);
				udelay(5);
				set_gpio_output(mcu_sclk_gpio, 0x0);
				check_value += (1 << (j % 8)) * ((gpio_readl(GPIO_DATA_IN(mcu_sdata_gpio)) & 0x1 << GPIO_BIT_SET_OFFSET(mcu_sdata_gpio)) ? 0x1 : 0x0);;
				udelay(5);
				set_gpio_output(mcu_sclk_gpio, 0x1);
				udelay(5);
			}

			// x
			set_gpio_output(mcu_sclk_gpio, 0x1);
			udelay(3);
			set_gpio_output(mcu_sclk_gpio, 0x0);
			udelay(3);
			set_gpio_output(mcu_sclk_gpio, 0x1);
			udelay(3);

			// x
			set_gpio_output(mcu_sclk_gpio, 0x1);
			udelay(3);
			set_gpio_output(mcu_sclk_gpio, 0x0);
			udelay(3);
			set_gpio_output(mcu_sclk_gpio, 0x1);
			udelay(3);
		}

		if ((check_value + checksum) % 256 == 0) {
			printk(KERN_ALERT "[mcu_data_check] Verify Data ROM CheckSum is OK...\n");
		} else {
			printk(KERN_ERR "[mcu_data_check] Writing Data ROM unsuccessfully(check_value:%ld, checksum:%d, mcu_header:%ld)...\n", check_value, checksum, mcu_header);
			return_num = -1;
		}
	}
										
	mdelay(10);
	set_gpio_output(mcu_bi_gpio, 0x0);
	set_gpio_output(mcu_resb_gpio, 0x0);
	udelay(2000);
	set_gpio_input(mcu_resb_gpio);
	
	return return_num;
}

void mcu_write_data(int **Array, int bit_8_num)
{
	unsigned int r_mode = 1;
	unsigned int m_mode = 0;
	unsigned int s_mode = 1;

	int i = 0, j = 0, mcu_sclk_gpio, mcu_resb_gpio, mcu_sdata_gpio, mcu_bi_gpio, entry_flag;
	int mcu_addr[bit_8_num][9];
	int sum;
	for (i=0; i<bit_8_num; i++) {
		sum = i;
		do {
			mcu_addr[i][j] = sum % 2;
			sum = sum/2;
			j++;
		} while (j % 9 != 0);
		j=0;
	}

	mcu_sclk_gpio = MCU_SCLK_REG_OFFSET;
	mcu_resb_gpio = MCU_RESB_REG_OFFSET;
	mcu_sdata_gpio = MCU_SDATA_REG_OFFSET;
	mcu_bi_gpio = MCU_BI_REG_OFFSET;
										
	entry_flag = do_mcu_program_entry(r_mode, m_mode, s_mode);

	if (entry_flag == 0) {
		for (i=0; i<bit_8_num; i++) {
			int num;
			// send start bit
			set_gpio_output(mcu_sclk_gpio, 0x1);
			set_gpio_output(mcu_sdata_gpio, 0x1);
			udelay(3);
			set_gpio_output(mcu_sclk_gpio, 0x0);
			udelay(3);
			set_gpio_output(mcu_sclk_gpio, 0x1);
			udelay(3);

			// send Op code
			set_gpio_output(mcu_sclk_gpio, 0x1);
			set_gpio_output(mcu_sdata_gpio, 0x0);
		    udelay(3);
			set_gpio_output(mcu_sclk_gpio, 0x0);
			udelay(3);
			set_gpio_output(mcu_sclk_gpio, 0x1);
			udelay(3);

			set_gpio_output(mcu_sclk_gpio, 0x1);
			set_gpio_output(mcu_sdata_gpio, 0x1);
			udelay(3);
			set_gpio_output(mcu_sclk_gpio, 0x0);
			udelay(3);
			set_gpio_output(mcu_sclk_gpio, 0x1);
			udelay(3);

			// send 9 bits address
			for (j=8; j>=0; j--) {
				set_gpio_output(mcu_sclk_gpio, 0x1);
				set_gpio_output(mcu_sdata_gpio, mcu_addr[i][j]);
				udelay(3);
				set_gpio_output(mcu_sclk_gpio, 0x0);
				udelay(3);
				set_gpio_output(mcu_sclk_gpio, 0x1);
				udelay(3);
			}

			// send 8 bits data
			for (j=7; j>=0; j--) {
				set_gpio_output(mcu_sclk_gpio, 0x1);
				set_gpio_output(mcu_sdata_gpio, Array[i][j]);
				udelay(3);
				set_gpio_output(mcu_sclk_gpio, 0x0);
				udelay(3);
				set_gpio_output(mcu_sclk_gpio, 0x1);
				udelay(3);
			}

			// x
			set_gpio_output(mcu_sclk_gpio, 0x1);
			udelay(3);
			set_gpio_output(mcu_sclk_gpio, 0x0);
			udelay(1000);
			set_gpio_output(mcu_sclk_gpio, 0x1);
			udelay(3);

			//waiting for R=1
			set_gpio_output(mcu_sclk_gpio, 0x1);
			udelay(3);
			set_gpio_output(mcu_sclk_gpio, 0x0);
			set_gpio_input(mcu_sdata_gpio);
			num = 0;
			do {
				num++;
				udelay(1000);
			} while (!(gpio_readl(GPIO_DATA_IN(mcu_sdata_gpio)) & 0x1 << GPIO_BIT_SET_OFFSET(mcu_sdata_gpio)) && num < 1000);
			
			set_gpio_output(mcu_sclk_gpio, 0x1);
			udelay(3);

			if (num >= 1000) {
				printk(KERN_ERR "[mcu_write_data] MCU Burning Fail!!! Program/Erase is busy!!!\n");
				break;
			}
		}
	}
	
	mdelay(10);
	set_gpio_output(mcu_bi_gpio, 0x0);
	set_gpio_output(mcu_resb_gpio, 0x0);
	udelay(2000);
	set_gpio_input(mcu_resb_gpio);
	
	printk(KERN_ALERT "[mcu_write_data] Writing Data ROM finish...\n");
}

int mcu_option_check(int bit_64_num, int mcu_header, int checksum, int mcu_tail)
{
	unsigned int r_mode = 0;
	unsigned int m_mode = 0;
	unsigned int s_mode = 1;

	int i, j, mcu_sclk_gpio, mcu_resb_gpio, mcu_sdata_gpio, mcu_bi_gpio, entry_flag;
	int return_num = 0;
	long check_value = 0;

	int Array_CHK[bit_64_num];

	check_value += mcu_header;
	check_value += mcu_tail;

	mcu_sclk_gpio = MCU_SCLK_REG_OFFSET;
	mcu_resb_gpio = MCU_RESB_REG_OFFSET;
	mcu_sdata_gpio = MCU_SDATA_REG_OFFSET;
	mcu_bi_gpio = MCU_BI_REG_OFFSET;
																	
	entry_flag = do_mcu_program_entry(r_mode, m_mode, s_mode);
	return_num = entry_flag;	

	if (entry_flag == 0) {
		// send 20 bits address
		for (i=0; i<20; i++) {
			set_gpio_output(mcu_sclk_gpio, 0x1);
			set_gpio_output(mcu_sdata_gpio, 0x0);
			udelay(3);
			set_gpio_output(mcu_sclk_gpio, 0x0);
			udelay(3);
			set_gpio_output(mcu_sclk_gpio, 0x1);
			udelay(3);
		}

		udelay(3);

		//receive 64 bits data
		set_gpio_input(mcu_sdata_gpio);
		/* skip the last 32 bytes when dumpping mcu */
		for (i=0; i< bit_64_num - 4; i++) {
			for (j=0; j<64; j++) {
				set_gpio_output(mcu_sclk_gpio, 0x1);
				udelay(5);
				set_gpio_output(mcu_sclk_gpio, 0x0);
				check_value += (1 << (j % 8)) * ((gpio_readl(GPIO_DATA_IN(mcu_sdata_gpio)) & 0x1 << GPIO_BIT_SET_OFFSET(mcu_sdata_gpio)) ? 0x1 : 0x0);
				udelay(5);
				set_gpio_output(mcu_sclk_gpio, 0x1);
				udelay(5);
			}
			Array_CHK[i] = check_value;
		}	
		
		if ((check_value + checksum) % 256 == 0) {
			printk(KERN_ALERT "[mcu_option_check] Verify Option ROM CheckSum is OK...\n");
		} else {
			printk(KERN_ERR "[mcu_option_check] Writing Option ROM unsuccessfully(check_value:%ld, checksum:%d, mcu_header:%d, mcu_tail:%d)...\n", check_value, checksum, mcu_header, mcu_tail);
			
			for (i=0; i<bit_64_num; i++) {
				printk("%d\n",Array_CHK[i]);
			}
			
			return_num = -1;
		}
	}
	
	mdelay(10);
	set_gpio_output(mcu_bi_gpio, 0x0);
	set_gpio_output(mcu_resb_gpio, 0x0);
	udelay(2000);
	set_gpio_input(mcu_resb_gpio);

	return return_num;	
}

void mcu_write_option(int **Option, int bit_64_num)
{
	unsigned int r_mode = 0;
	unsigned int m_mode = 2;
	unsigned int s_mode = 1;

	int i, j, mcu_sclk_gpio, mcu_resb_gpio, mcu_sdata_gpio, mcu_bi_gpio, entry_flag;

	mcu_sclk_gpio = MCU_SCLK_REG_OFFSET;
	mcu_resb_gpio = MCU_RESB_REG_OFFSET;
	mcu_sdata_gpio = MCU_SDATA_REG_OFFSET;
	mcu_bi_gpio = MCU_BI_REG_OFFSET;
	
	entry_flag = do_mcu_program_entry(r_mode, m_mode, s_mode);

	if (entry_flag == 0) {
		// send 20 bits address
		for (i=0; i<20; i++) {
			set_gpio_output(mcu_sclk_gpio, 0x1);
			set_gpio_output(mcu_sdata_gpio, 0x0);
			udelay(3);
			set_gpio_output(mcu_sclk_gpio, 0x0);
			udelay(3);
			set_gpio_output(mcu_sclk_gpio, 0x1);
			udelay(3);
		}

		udelay(3);

		//send 64 bits data
		for (i=0; i<bit_64_num; i++) {
			for (j=0; j<64; j++) {
				set_gpio_output(mcu_sclk_gpio, 0x1);
				set_gpio_output(mcu_sdata_gpio, Option[i][j]);
				udelay(3);
				set_gpio_output(mcu_sclk_gpio, 0x0);
				if (j > 62) {
					udelay(2000);
				} else {
					udelay(3);
				}
				set_gpio_output(mcu_sclk_gpio, 0x1);
				udelay(3);
			}
		}
	}
	
	mdelay(10);
	set_gpio_output(mcu_bi_gpio, 0x0);
	set_gpio_output(mcu_resb_gpio, 0x0);
	udelay(2000);
	set_gpio_input(mcu_resb_gpio);
	
	printk(KERN_ALERT "[mcu_write_option] Writing Option ROM finish...\n");
}

void turn_on_led(unsigned int id, unsigned int color)
{
	int i;
	unsigned long reg;
	
	/* System does not have LED_SET[id] */
	if (led_set[id].presence == 0) return;
	
	for (i = 0; i < LED_COLOR_TOTAL; i++) {
		if ((color & led_set[id].led[i].color) && (led_set[id].led[i].presence != 0)) {
			led_set[id].led[i].state = LED_ON;
			reg = GPIO_DATA_OUT(led_set[id].led[i].gpio);
			gpio_writel(gpio_readl(reg) | (0x1 << GPIO_BIT_SET_OFFSET(led_set[id].led[i].gpio)), reg);
		}
	}
}

void turn_off_led(unsigned int id)
{
	int i;
	unsigned long reg;
	
	/* System does not have LED_SET[id] */
	if (led_set[id].presence == 0) return;
	
	for (i = 0; i < LED_COLOR_TOTAL; i++) {
		if (led_set[id].led[i].presence != 0) {
			led_set[id].led[i].state = LED_OFF;
			reg = GPIO_DATA_OUT(led_set[id].led[i].gpio);
			gpio_writel(gpio_readl(reg) & ~(0x1 << GPIO_BIT_SET_OFFSET(led_set[id].led[i].gpio)), reg);
		}
	}
}

void turn_off_led_all(unsigned int id)
{
	int i;
	unsigned long reg;
	
	/* System does not have LED_SET[id] */
	if (led_set[id].presence == 0) return;
		    
	for (i = 0; i < LED_COLOR_TOTAL; i++) {
		if (led_set[id].led[i].presence == 0) continue;
		
		led_set[id].led[i].state = LED_OFF;
		reg = GPIO_DATA_OUT(led_set[id].led[i].gpio);
		gpio_writel(gpio_readl(reg) & ~(0x1 << GPIO_BIT_SET_OFFSET(led_set[id].led[i].gpio)), reg);
	}
}

void led_all_red_on(void)
{
	int i = 0;

	for (i = 0; i < LED_TOTAL; i++)
	{
		turn_off_led(i);
		turn_on_led(i, RED);
	}
}

void mcu_burning(struct _mcu_burn *mcu_data)
{
	char *path = mcu_data->path;
	//char *path = "/home/norman/mcu_file/HT66F30_Addr.mtp";
				
	char buf[2]; 
	struct file *fp; 
	int ret;  
	initKernelEnv(); 
	
	fp=openFile(path,O_RDONLY, 0);

	if(IS_ERR(fp)) {
		printk(KERN_ERR "[ERR] Cannot open the file path:%s\n", path);
		led_all_red_on();
		return;
	}
	
	if (fp!=NULL) 
	{ 
		int c, num=0, record_num=0, i;
		int record_flag = 0;
		int allocate_flag = 0;
		int record_type = 0;
		int length = 0;
		long checksum = 0;
		int sum = 0, x = 0, bit_64_num = 0, opt_64_num = 0, opt_checksum = 0, opt_header = 0, opt_tail = 0;
		int **Array = NULL, **Option = NULL;
		int mcu_header = 0;
		int mcu_check = 0;
		int verify_format_ok = 0;

		memset(buf,0,sizeof(buf));

		while ((ret = readFile(fp,buf,1))>0) {
			c = buf[0];
			if (c != 101 && !record_flag) continue;
			record_flag = 1;
			num++;
			checksum += c;
			if (length == 1) {
				verify_format_ok = 1;
				if (checksum % 256 == 0) {
					printk("[mcu_burning] checksum: %X\n", c);
					if (record_type == 0) {
						/* power resume always on */
						queue_work(mcu_workqueue,&mcu_power_resume);
						
						ssleep(1);
						
						//ERASE MCU Code, Option and Data
						mcu_erase_all();
						
						// write mcu program
						mcu_write_program(Array, bit_64_num);
						mcu_check = mcu_program_check(bit_64_num, mcu_header, c);
					} else if (record_type == 1) {
						memcpy(Option, Array, sizeof(Option));
						opt_64_num = bit_64_num;
						opt_header = mcu_header;
						opt_checksum = c;
					} else if (record_type == 2) {
						/* mcu data rom needn't to burn. so mark it. */
						// write mcu data
						//mcu_write_data(Array, record_num);
						//mcu_check = mcu_data_check( record_num, mcu_header, c);

						if (mcu_check == 0) {
							//write mcu option
							mcu_write_option(Option, opt_64_num);
							mcu_check = mcu_option_check(opt_64_num, opt_header, opt_checksum, opt_tail);
						}

						kfree(Option);
						opt_64_num = 0;
						opt_checksum = 0;
						opt_header = 0;
						opt_tail = 0;
					}
					
					length = 0;
					record_flag = 0;
					checksum = 0;
					num = 0;
					record_num=0;
					record_type++;
					kfree(Array);
					allocate_flag = 0;
					bit_64_num = 0;
					mcu_header = 0;
					if (mcu_check == -1 || record_type == 2) break;
				} else {
					mcu_check = -1;
					printk(KERN_ERR "[ERR] checksum error(%ld)!!!\n", checksum);
					break;
				}
			}

			if (num == 2) {
				length += c;
			} else if (num == 3) {
				length += 256 * c;
			} else if (num > 3 && length > 1) {
				if (num == 7) mcu_header = checksum;
				if (num > 7) {
					if (!allocate_flag) {
						int *pData;
						int m, n;
						allocate_flag = 1;
						if (record_type < 2) {
							m = length / 8 + 1;
							n = 64;
						} else {
							m = length;
							n = 8;
						}
						Array = (int **)kmalloc(m*sizeof(int *)+m*n*sizeof(int), GFP_KERNEL);
						if (record_type == 1) 
							Option = (int **)kmalloc(m*sizeof(int *)+m*n*sizeof(int), GFP_KERNEL);
						for (i = 0, pData = (int *)(Array+m); i < m; i++, pData += n) {
							Array[i]=pData;
							if (record_type == 1)
								Option[i]=pData;
						}
					}
					record_num++;
					if (record_type < 2) {
						/* collect option tail value for option checking */
						if (record_type == 1) {
							if (bit_64_num >= 5)
								opt_tail += c;
						}

						if (record_num % 2 != 0) {
							sum += c;
						} else {
							sum += 256 * c;
							do {
								Array[bit_64_num][x] = sum % 2;
								sum = sum/2;
								x++;
							} while (x % 16 != 0);
							sum = 0;
							/* collect 64 bits data */
							if (x == 64){
								//for (i=0 ;i<x ; i++) {
								//		printk("%d", Array[bit_64_num][i]);
								//}
								//printk("\n");
								bit_64_num++;
								x = 0;
							}
						}
					} else {
						sum = c;
						do {
							Array[record_num-1][x] = sum % 2;
							sum = sum / 2;
							x++;
						} while (x % 8 != 0);
						/* collect 8 bits data */
						if (x == 8){
							//int i;
							//for (i=0 ;i<x ; i++) {
							//		printk("%d", Array[record_num-1][i]);
							//}
							//printk("\n");
							x = 0;
						}
					}
				}
				length--;
			}
		}
																																																	
		// No mcu data and writing mcu program rom successfully
		if (record_type == 2 && mcu_check == 0) {
			// write mcu option
			mcu_write_option( Option, opt_64_num);
			mcu_check = mcu_option_check( opt_64_num, opt_header, opt_checksum, opt_tail);
			kfree(Option);
			opt_64_num = 0;
		}

		if (!verify_format_ok) {
			printk(KERN_ERR "[ERR] The file format error(%s)!!!\n", path);
		}

		printk(KERN_ALERT "MCU burning finish...\n");
		
		/* mcu burning successfully */
		if (verify_format_ok && mcu_check == 0) {
			/* assign a lock file to user space for mcu upgrade key */
			queue_work(mcu_workqueue,&mcu_assign_lock);

			ssleep(1);

			/* Run a MCU Reset */
			queue_work(mcu_workqueue,&mcu_reset_control);
		} else {
			/* Turn on all red led for error state*/
			led_all_red_on();
		}
	}
	
	closeFile(fp);											 
	set_fs(oldfs);
}

void mcu_program(struct _mcu_ioctl *mcu_data)
{
	unsigned int r_mode = mcu_data->r_mode;
	unsigned int m_mode = mcu_data->m_mode;
	unsigned int s_mode = mcu_data->s_mode;
						
	unsigned int data[26] = {0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0};
	unsigned int mode_sel[8][4] = {{0,0,0,0},{0,0,0,1},{0,1,1,0},{0,1,1,1},{1,0,0,0},{1,0,0,1},{1,1,0,0},{1,1,1,0}};
	int i, mcu_sclk_gpio, mcu_resb_gpio, mcu_sdata_gpio, mcu_bi_gpio;
	int check_ack[8] = {1,1,r_mode,mode_sel[m_mode][0],mode_sel[m_mode][1],mode_sel[m_mode][2],mode_sel[m_mode][3],s_mode};
	int read_ack[8];
	int check_fail_flag;
											
	mcu_sclk_gpio = MCU_SCLK_REG_OFFSET;
	mcu_resb_gpio = MCU_RESB_REG_OFFSET;
	mcu_sdata_gpio = MCU_SDATA_REG_OFFSET;
	mcu_bi_gpio = MCU_BI_REG_OFFSET;
													
	set_gpio_output(mcu_resb_gpio, 0x0);
	set_gpio_output(mcu_sclk_gpio, 0x0);
	set_gpio_output(mcu_bi_gpio, 0x1);
																	
	mdelay(10);																	
	set_gpio_output(mcu_sclk_gpio, 0x1);
	mdelay(10);
	set_gpio_output(mcu_resb_gpio, 0x1);
	mdelay(10);
	set_gpio_output(mcu_resb_gpio, 0x0);
																							
	for (i=0; i<26; i++) {
		set_gpio_output(mcu_sclk_gpio, 0x1);
		set_gpio_output(mcu_sdata_gpio, data[i]);
		udelay(3);
		set_gpio_output(mcu_sclk_gpio, 0x0);
		udelay(3);
		set_gpio_output(mcu_sclk_gpio, 0x1);
		udelay(3);			
	}
																								
	set_gpio_output(mcu_sclk_gpio, 0x1);
	set_gpio_output(mcu_sdata_gpio, r_mode);
	udelay(3);
	set_gpio_output(mcu_sclk_gpio, 0x0);
	udelay(3);	
	set_gpio_output(mcu_sclk_gpio, 0x1);
	udelay(3);

	for (i=0; i<4; i++) {
		set_gpio_output(mcu_sclk_gpio, 0x1);
		set_gpio_output(mcu_sdata_gpio, mode_sel[m_mode][i]);
		udelay(3);
		set_gpio_output(mcu_sclk_gpio, 0x0);
		udelay(3);
		set_gpio_output(mcu_sclk_gpio, 0x1);
		udelay(3);
	}
																																
	set_gpio_output(mcu_sclk_gpio, 0x1);
	set_gpio_output(mcu_sdata_gpio, s_mode);
	udelay(3);
	set_gpio_output(mcu_sclk_gpio, 0x0);
	udelay(3);
	set_gpio_output(mcu_sclk_gpio, 0x1);
	udelay(3);

	set_gpio_input(mcu_sdata_gpio);
	udelay(100);
																																									
	for (i=0; i<8; i++) {
		set_gpio_output(mcu_sclk_gpio, 0x1);
		udelay(5);
		set_gpio_output(mcu_sclk_gpio, 0x0);
		read_ack[i] = (gpio_readl(GPIO_DATA_IN(mcu_sdata_gpio)) & 0x1 << GPIO_BIT_SET_OFFSET(mcu_sdata_gpio)) ? 0x1 : 0x0;
		udelay(5);
		set_gpio_output(mcu_sclk_gpio, 0x1);
		udelay(5);
	}

	set_gpio_output(mcu_resb_gpio, 0x1);

	// check read ack
	check_fail_flag = 0;
	for (i=0; i<8; i++) {
		if (read_ack[i] != check_ack[i]) {
			check_fail_flag = 1;
			break;
		}
	}
																																														
	if (check_fail_flag) {
		printk(KERN_ERR "Cannot Enter Programming Mode...");
		printk("The reading ACK is (%d%d%d%d%d%d%d%d)...\n", read_ack[0],read_ack[1],read_ack[2],read_ack[3],read_ack[4],read_ack[5],read_ack[6],read_ack[7]);
		printk("The checking ACK is (%d%d%d%d%d%d%d%d)...\n", check_ack[0],check_ack[1],check_ack[2],check_ack[3],check_ack[4],check_ack[5],check_ack[6],check_ack[7]);
		return;
	} else {
		printk(KERN_ALERT "Entering Programming Mode Successfully...\n");
	}
																																															
	// for 1024 dummy clock
	for (i=0; i<1024; i++) {
		set_gpio_output(mcu_sclk_gpio, 0x1);
		udelay(3);
		set_gpio_output(mcu_sclk_gpio, 0x0);
		udelay(3);
		set_gpio_output(mcu_sclk_gpio, 0x1);
		udelay(3);
	}
	
	mdelay(10);
	set_gpio_output(mcu_bi_gpio, 0x0);
	set_gpio_output(mcu_resb_gpio, 0x0);
	udelay(2000);
	set_gpio_input(mcu_resb_gpio);

	printk(KERN_ALERT "MCU Testing finish...\n");
}

///////////////////////////////////////////////////////////////////////////////////


/* Initialize HTP pin */
static void init_htp_pin_gpio(void)
{
	/* Output Enable Low as Input */
	unsigned long reg;
	reg = GPIO_DATA_OUT_ENABLE(HTP_GPIO_REG_OFFSET);
	gpio_writel(gpio_readl(reg) & ~(0x1 << GPIO_BIT_SET_OFFSET(HTP_GPIO_REG_OFFSET)), reg);

	/* Select Pin for GPIO Mode (15:14 '00' - GPIO[7]) */
	gpio_writel((gpio_readl(COMCERTO_GPIO_PIN_SELECT_REG) & ~(GPIO_PIN_14)) & ~(GPIO_PIN_15), COMCERTO_GPIO_PIN_SELECT_REG);	
}

/* Initialize HDD detect pin */
static void init_hdd_detect_pin_gpio(void)
{
	/* Output Enable Low as Input */
	unsigned long reg;
	reg = GPIO_DATA_OUT_ENABLE(HDD1_DETECT_REG_OFFSET);
	gpio_writel(gpio_readl(reg) & ~(0x1 << GPIO_BIT_SET_OFFSET(HDD1_DETECT_REG_OFFSET)), reg);

	reg = GPIO_DATA_OUT_ENABLE(HDD2_DETECT_REG_OFFSET);
	gpio_writel(gpio_readl(reg) & ~(0x1 << GPIO_BIT_SET_OFFSET(HDD2_DETECT_REG_OFFSET)), reg);
	
	reg = GPIO_DATA_OUT_ENABLE(HDD3_DETECT_REG_OFFSET);
	gpio_writel(gpio_readl(reg) & ~(0x1 << GPIO_BIT_SET_OFFSET(HDD3_DETECT_REG_OFFSET)), reg);

	reg = GPIO_DATA_OUT_ENABLE(HDD4_DETECT_REG_OFFSET);
	gpio_writel(gpio_readl(reg) & ~(0x1 << GPIO_BIT_SET_OFFSET(HDD4_DETECT_REG_OFFSET)), reg);

	/* GPIO[0~3] are always selected for GPIO Mode */
}

/* Initialize HDD control pin */
static void init_hdd_ctrl_pin_gpio(void)
{
	/* Output Enable High as Output */
	unsigned long reg;
	reg = GPIO_DATA_OUT_ENABLE(HDD1_CTRL_REG_OFFSET);
	gpio_writel(gpio_readl(reg) | (0x1 << GPIO_BIT_SET_OFFSET(HDD1_CTRL_REG_OFFSET)), reg);
	
	reg = GPIO_DATA_OUT_ENABLE(HDD2_CTRL_REG_OFFSET);
	gpio_writel(gpio_readl(reg) | (0x1 << GPIO_BIT_SET_OFFSET(HDD2_CTRL_REG_OFFSET)), reg);
	
	reg = GPIO_DATA_OUT_ENABLE(HDD3_CTRL_REG_OFFSET);
	gpio_writel(gpio_readl(reg) | (0x1 << GPIO_BIT_SET_OFFSET(HDD3_CTRL_REG_OFFSET)), reg);
	
	reg = GPIO_DATA_OUT_ENABLE(HDD4_CTRL_REG_OFFSET);
	gpio_writel(gpio_readl(reg) | (0x1 << GPIO_BIT_SET_OFFSET(HDD4_CTRL_REG_OFFSET)), reg);

	/* Select Pin for GPIO Mode (17:16 '00' - GPIO[8]) */
	gpio_writel((gpio_readl(COMCERTO_GPIO_PIN_SELECT_REG) & ~(GPIO_PIN_16)) & ~(GPIO_PIN_17), COMCERTO_GPIO_PIN_SELECT_REG);

	/* Select Pin for GPIO Mode (19:18 '00' - GPIO[9]) */
	gpio_writel((gpio_readl(COMCERTO_GPIO_PIN_SELECT_REG) & ~(GPIO_PIN_18)) & ~(GPIO_PIN_19), COMCERTO_GPIO_PIN_SELECT_REG);

	/* Select Pin for GPIO Mode (21:20 '00' - GPIO[10]) */
	gpio_writel((gpio_readl(COMCERTO_GPIO_PIN_SELECT_REG) & ~(GPIO_PIN_20)) & ~(GPIO_PIN_21), COMCERTO_GPIO_PIN_SELECT_REG);

	/* Select Pin for GPIO Mode (23:22 '00' - GPIO[11]) */
	gpio_writel((gpio_readl(COMCERTO_GPIO_PIN_SELECT_REG) & ~(GPIO_PIN_22)) & ~(GPIO_PIN_23), COMCERTO_GPIO_PIN_SELECT_REG);
}

// Initialize USB control pin
static void init_pwren_usb_pin_gpio(void)
{
	/* Output Enable High as Output */
	unsigned long reg;
	reg = GPIO_DATA_OUT_ENABLE(PWREN_USB_REG_OFFSET);
	gpio_writel(gpio_readl(reg) | (0x1 << GPIO_BIT_SET_OFFSET(PWREN_USB_REG_OFFSET)), reg);

	/* Select Pin for GPIO Mode (29:28 '00' - GPIO[14]) */
	gpio_writel((gpio_readl(COMCERTO_GPIO_PIN_SELECT_REG) & ~(GPIO_PIN_28)) & ~(GPIO_PIN_29), COMCERTO_GPIO_PIN_SELECT_REG);
}

static void init_button_pin_gpio(void)
{
	/* Output Enable Low as Input */
	unsigned long reg;
	reg = GPIO_DATA_OUT_ENABLE(POWER_BUTTON_REG_OFFSET);
	gpio_writel(gpio_readl(reg) & ~(0x1 << GPIO_BIT_SET_OFFSET(POWER_BUTTON_REG_OFFSET)), reg);
	
	reg = GPIO_DATA_OUT_ENABLE(RESET_BUTTON_REG_OFFSET);
	gpio_writel(gpio_readl(reg) & ~(0x1 << GPIO_BIT_SET_OFFSET(RESET_BUTTON_REG_OFFSET)), reg);

	reg = GPIO_DATA_OUT_ENABLE(COPY_BUTTON_REG_OFFSET);
	gpio_writel(gpio_readl(reg) & ~(0x1 << GPIO_BIT_SET_OFFSET(COPY_BUTTON_REG_OFFSET)), reg);

#if 0
	/* Choose the "Interrupt configuration" register */
	gpio_writel((gpio_readl(COMCERTO_GPIO_INT_CFG_REG) & ~(GPIO_PIN_8)) | GPIO_PIN_9, COMCERTO_GPIO_INT_CFG_REG);   //GPIO[4] rising edge interrupt
	gpio_writel((gpio_readl(COMCERTO_GPIO_INT_CFG_REG) | GPIO_PIN_10) & ~(GPIO_PIN_11), COMCERTO_GPIO_INT_CFG_REG); //GPIO[5] falling edge interrupt
	gpio_writel((gpio_readl(COMCERTO_GPIO_INT_CFG_REG) | GPIO_PIN_12) & ~(GPIO_PIN_13), COMCERTO_GPIO_INT_CFG_REG); //GPIO[6] falling edge interrupt
#endif	

	/* Select Pin for GPIO Mode (9:8 '00' - GPIO[4]) */
	gpio_writel((gpio_readl(COMCERTO_GPIO_PIN_SELECT_REG) & ~(GPIO_PIN_8)) & ~(GPIO_PIN_9), COMCERTO_GPIO_PIN_SELECT_REG);
	/* Select Pin for GPIO Mode (11:10 '00' - GPIO[5]) */
	gpio_writel((gpio_readl(COMCERTO_GPIO_PIN_SELECT_REG) & ~(GPIO_PIN_10)) & ~(GPIO_PIN_11), COMCERTO_GPIO_PIN_SELECT_REG);
	/* Select Pin for GPIO Mode (13:12 '00' - GPIO[6]) */
	gpio_writel((gpio_readl(COMCERTO_GPIO_PIN_SELECT_REG) & ~(GPIO_PIN_12)) & ~(GPIO_PIN_13), COMCERTO_GPIO_PIN_SELECT_REG);
}

static void init_buzzer_pin(void)
{
	/* Select Pin for PWM (27:26 '01' - PWM[5]) */
	gpio_writel((gpio_readl(COMCERTO_GPIO_PIN_SELECT_REG) | (GPIO_PIN_26)) & ~(GPIO_PIN_27), COMCERTO_GPIO_PIN_SELECT_REG);

	/* Enable the Clock Divider and set the value to 1 */
	gpio_writel((gpio_readl(COMCERTO_PWM_CLOCK_DIVIDER_CONTROL) | (GPIO_PIN_0)) | (GPIO_PIN_31), COMCERTO_PWM_CLOCK_DIVIDER_CONTROL);

	/* Enable PWM #5 timer and set the max value (0x10000) of PWM #5 */
	gpio_writel((gpio_readl(COMCERTO_PWM5_ENABLE_MAX) | (GPIO_PIN_16)) | (GPIO_PIN_31), COMCERTO_PWM5_ENABLE_MAX);
}

static void init_led_pin_gpio(void)
{
	int i, j;
	unsigned long reg;
	for (i = 0; i < LED_TOTAL; i++) {
		for (j = 0; j < LED_COLOR_TOTAL; j++) {
			/* Output Enable Low as Output for GPIO[63:32] */
			reg = GPIO_DATA_OUT_ENABLE(led_set[i].led[j].gpio);
			gpio_writel(gpio_readl(reg) & ~(0x1 << GPIO_BIT_SET_OFFSET(led_set[i].led[j].gpio)), reg);

			/* Select Pin for GPIO Mode */
			gpio_writel(gpio_readl(COMCERTO_GPIO_63_32_PIN_SELECT) | (0x1 << GPIO_BIT_SET_OFFSET(led_set[i].led[j].gpio)), COMCERTO_GPIO_63_32_PIN_SELECT);
		}
	}
}

static void Beep(void)
{
	int i;

	for(i = 0 ; i < BEEP_DURATION ; i++)
	{
		//gpio_writel(gpio_readl(COMCERTO_PWM5_LOW_DUTY_CYCLE) ^ (GPIO_PIN_12), COMCERTO_PWM5_LOW_DUTY_CYCLE);
		gpio_writel(gpio_readl(COMCERTO_PWM5_LOW_DUTY_CYCLE) ^ ((GPIO_PIN_12)|(GPIO_PIN_13)|(GPIO_PIN_15)), COMCERTO_PWM5_LOW_DUTY_CYCLE);
		udelay(500);
	}

	//gpio_writel(gpio_readl(COMCERTO_PWM5_LOW_DUTY_CYCLE) & ~(GPIO_PIN_12), COMCERTO_PWM5_LOW_DUTY_CYCLE);
	gpio_writel(gpio_readl(COMCERTO_PWM5_LOW_DUTY_CYCLE) & ~((GPIO_PIN_12)|(GPIO_PIN_13)|(GPIO_PIN_15)), COMCERTO_PWM5_LOW_DUTY_CYCLE);
}

static void Beep_Beep(int duty_high, int duty_low)
{
	// Duty cycle unit : ms
	int i;

	for(i = 0 ; i < BEEP_DURATION ; i++){
		gpio_writel(gpio_readl(COMCERTO_PWM5_LOW_DUTY_CYCLE) ^ ((GPIO_PIN_12)|(GPIO_PIN_13)|(GPIO_PIN_15)), COMCERTO_PWM5_LOW_DUTY_CYCLE);
		udelay(duty_high);
	}

	for(i = 0 ; i < BEEP_DURATION ; i++){
		gpio_writel(gpio_readl(COMCERTO_PWM5_LOW_DUTY_CYCLE) & ~((GPIO_PIN_12)|(GPIO_PIN_13)|(GPIO_PIN_15)), COMCERTO_PWM5_LOW_DUTY_CYCLE);
		udelay(duty_low);
	}
}


void btncpy_signal_func10(struct work_struct *in)
{
	sys_kill(btncpy_pid, 10);
}

void btncpy_signal_func12(struct work_struct *in)
{
	sys_kill(btncpy_pid, 12);
}

void btncpy_signal_func14(struct work_struct *in)
{
	sys_kill(btncpy_pid, 14);
}

void nsa_shutdown_func(struct work_struct *in)
{
	if(atomic_read(&halt_one_time) == 0)
	{
		int ret;
		char *argv[] = {"/sbin/halt", NULL};
		atomic_set(&halt_one_time, 1);

		ret = call_usermodehelper("/sbin/halt", argv, NULL, 0);
	}
}

void Reset_UserInfo_func(struct work_struct *in)
{
	char *argv[] = {"/usr/local/btn/reset_userinfo.sh", NULL};
	
	call_usermodehelper("/usr/local/btn/reset_userinfo.sh", argv, NULL, 0);
}

void Open_Backdoor_func(struct work_struct *in)
{
	call_usermodehelper("/usr/local/btn/open_back_door.sh", NULL, NULL, 0);
}

void Reset_To_Defu_func(struct work_struct *in)
{
	ssleep(1);

	Beep();
	ssleep(1);

	Beep();
	ssleep(1);

	Beep();
	ssleep(1);
	call_usermodehelper("/usr/local/btn/reset_and_reboot.sh", NULL, NULL, 0);
}

void zyxel_power_off(void)
{
	unsigned long reg;

	printk(KERN_ERR"GPIO[15] is pull high for power off\n");
	
	/* Output Enable High as Output */
	reg = GPIO_DATA_OUT_ENABLE(POWER_OFF_REG_OFFSET);
	gpio_writel(gpio_readl(reg) | (0x1 << GPIO_BIT_SET_OFFSET(POWER_OFF_REG_OFFSET)), reg);

	/* GPIO[15] pull high */
	reg = GPIO_DATA_OUT(POWER_OFF_REG_OFFSET);
	gpio_writel(gpio_readl(reg) | (0x1 << GPIO_BIT_SET_OFFSET(POWER_OFF_REG_OFFSET)), reg);

	/* Select Pin for GPIO Mode (31:30 '00' - GPIO[15]) */
	gpio_writel((gpio_readl(COMCERTO_GPIO_PIN_SELECT_REG) & ~(GPIO_PIN_30)) & ~(GPIO_PIN_31), COMCERTO_GPIO_PIN_SELECT_REG);
}

static void btnpow_timer_func(unsigned long in_data)
{

	if (INPUT_PIN_TRIGGERED(POWER_BUTTON_REG_OFFSET)) {
		/* handle the button pressed behavior */
		atomic_set(&pow_button_pressed, 1);
		++pow_polling_times;
		if(pow_polling_times  == (QUICK_PRESS_TIME << 3) || pow_polling_times  == (PRESS_TIME << 3)) Beep();	
	} else {
		if (atomic_read(&pow_button_pressed)) {
			if(atomic_read(&button_test_enable) &&
				(atomic_read(&button_test_num) == POWER_BTN_NUM))
			{
				/* handle the testButton for HTP */
				atomic_set(&button_test_enable, 0);
				queue_work(btn_workqueue, &btncpy_signal10);
			}
			else
			{
				/* handle the NAS behavior */
				if(pow_polling_times >= (QUICK_PRESS_TIME << 3)  && pow_polling_times  < (PRESS_TIME << 3))
				{
					queue_work(btn_workqueue,&halt_nsa);
				} else if(pow_polling_times >= (PRESS_TIME << 3)) {
					printk(KERN_ERR"Power Off\n");
					zyxel_power_off();
				}	
			}
			pow_polling_times = 0;
			atomic_set(&pow_button_pressed, 0);
			//enable_irq(POWER_BUTTON_IRQ);
		}
	}
	mod_timer(&btnpow_timer, jiffies + (JIFFIES_1_SEC >> 3));
}

static void btnreset_timer_func(unsigned long in_data)
{
	if (INPUT_PIN_TRIGGERED(RESET_BUTTON_REG_OFFSET)) {
		if(atomic_read(&reset_button_pressed)) {
			if(atomic_read(&button_test_enable) &&
				(atomic_read(&button_test_num) == RESET_BTN_NUM))
			{
				/* handle the testButton for HTP */
				atomic_set(&button_test_enable, 0);
				queue_work(btn_workqueue, &btncpy_signal10);
			}
			else
			{
				/* handle the NAS behavior */
				if(reset_polling_times >= (2 << 3) && reset_polling_times  <= (3 << 3))
				{
					printk(KERN_INFO"Reset admin password & ip setting ........\n");  // May move to Reset_UserInfo_func
					queue_work(btn_workqueue, &Reset_User_Info);
				}
				else if(reset_polling_times >= (6 << 3) && reset_polling_times <= (7 << 3))
				{
					printk(KERN_INFO"Open backdoor ... \n");
					queue_work(btn_workqueue, &Open_Backdoor);
				}
				else if(reset_polling_times >= (10 << 3))
				{
					printk(KERN_INFO"remove configuration (etc/zyxel/config) and reboot\n");

					queue_work(btn_workqueue, &Reset_To_Default);
				}
				else ;
			}
			reset_polling_times = 0;
			atomic_set(&reset_button_pressed, 0);
			//enable_irq(RESET_BUTTON_IRQ);
		}
	} else {
		/* handle the button pressed behavior */
		atomic_set(&reset_button_pressed, 1);
		++reset_polling_times;
		if(reset_polling_times == (10 << 3)) Beep();
		else if(reset_polling_times == (6 << 3)) Beep();
		else if(reset_polling_times == (2 << 3)) Beep();
		else;
	}

	mod_timer(&btnreset_timer, jiffies + (JIFFIES_1_SEC >> 3));
}

static void btncpy_timer_func(unsigned long in_data)
{
	if (INPUT_PIN_TRIGGERED(COPY_BUTTON_REG_OFFSET)) {
		if(atomic_read(&cpy_button_pressed)) {
			if(atomic_read(&button_test_enable) &&
				(atomic_read(&button_test_num) == COPY_BTN_NUM))
			{
				/* handle the testButton for HTP */
				atomic_set(&button_test_enable, 0);
				queue_work(btn_workqueue, &btncpy_signal10);
			}
			else
			{
				/* handle the NAS behavior */
				if(btncpy_pid)
				{
					if(cpy_polling_times >= (6 << 3) && cpy_polling_times < (30 << 3)) {
						//printk(KERN_ERR"btncpy cancel button\n");
						queue_work(btn_workqueue, &btncpy_signal14);
					} 
					else if(cpy_polling_times >= (3 << 3) && cpy_polling_times < (6 << 3)) {
						//printk(KERN_ERR"btncpy sync button\n");
						queue_work(btn_workqueue, &btncpy_signal12);
					}
					else
					{
						if(atomic_read(&button_test_enable) == 0) {
							//printk(KERN_ERR"btncpy copy button\n");
							queue_work(btn_workqueue, &btncpy_signal10);
						}
					}
				}
				
				if(cpy_polling_times >= (30 << 3))
				{
					show_state();
					show_mem(0);
				}
			}
			cpy_polling_times = 0;
			atomic_set(&cpy_button_pressed, 0);
			//enable_irq(COPY_BUTTON_IRQ);
		}
	} else {
		/* handle the button pressed behavior */
		atomic_set(&cpy_button_pressed, 1);
		++cpy_polling_times;
		if(cpy_polling_times == (3 << 3))
		{
			//Sync Beep
			Beep();
		} else if(cpy_polling_times == (6 << 3)) { 
			//Cancel Beep Beep
			Beep_Beep(500,500);
			Beep_Beep(500,500);
		} else if(cpy_polling_times == (30 << 3)) {
			//Reset Beep
			Beep();
		} else;
	}

	mod_timer(&btncpy_timer, jiffies + (JIFFIES_1_SEC >> 3));
}

static void buzzer_timer_func(unsigned long in_data)
{
	if(bz_time != 0)    /* continue the timer */
	{
		int i;	
		for(i = 0 ; i < BEEP_DURATION ; i++)
		{
//			gpio_writel(gpio_readl(COMCERTO_PWM5_LOW_DUTY_CYCLE) ^ (GPIO_PIN_12), COMCERTO_PWM5_LOW_DUTY_CYCLE);
			gpio_writel(gpio_readl(COMCERTO_PWM5_LOW_DUTY_CYCLE) ^ ((GPIO_PIN_12)|(GPIO_PIN_13)|(GPIO_PIN_15)), COMCERTO_PWM5_LOW_DUTY_CYCLE);
			udelay(500);
		}
		
//		gpio_writel(gpio_readl(COMCERTO_PWM5_LOW_DUTY_CYCLE) & ~(GPIO_PIN_12), COMCERTO_PWM5_LOW_DUTY_CYCLE);
		gpio_writel(gpio_readl(COMCERTO_PWM5_LOW_DUTY_CYCLE) & ~((GPIO_PIN_12)|(GPIO_PIN_13)|(GPIO_PIN_15)), COMCERTO_PWM5_LOW_DUTY_CYCLE);
		mod_timer(&bz_timer, jiffies + BZ_TIMER_PERIOD);
	}
	--bz_time;
}

void set_buzzer(unsigned long bz_data)
{
	unsigned short time, status;
	
	time = GET_TIME(bz_data);
	status = GET_STATE(bz_data);
	
	printk(KERN_ERR"bz time = %x\n", time);
	printk(KERN_ERR"bz status = %x\n", status);
	printk(KERN_ERR"bz_timer_status = %x\n", bz_timer_status);
	
	// Turn off bz first
	if(bz_timer_status == TIMER_RUNNING)
	{
		if(bz_type == RING_FOREVER && status == BUZ_ON)
		{
			//printk(KERN_ERR"Buzzer Forever Already On \n");
			return;
		}
		bz_timer_status = TIMER_SLEEPING;
		
		/* Disable buzzer first */
//		gpio_writel(gpio_readl(COMCERTO_PWM5_LOW_DUTY_CYCLE) & ~(GPIO_PIN_12), COMCERTO_PWM5_LOW_DUTY_CYCLE);
		gpio_writel(gpio_readl(COMCERTO_PWM5_LOW_DUTY_CYCLE) & ~((GPIO_PIN_12)|(GPIO_PIN_13)|(GPIO_PIN_15)), COMCERTO_PWM5_LOW_DUTY_CYCLE);
		del_timer_sync(&bz_timer);
		bz_type = RING_BRIEF;
		//printk(KERN_ERR"Closed Buzzer, bz_type = %d\n", bz_type);
	}
	
	if(status == BUZ_ON || status == BUZ_FOREVER)
	{
		// set bz time
		bz_timer_status = TIMER_RUNNING;
		if(time >= 32 || status == BUZ_FOREVER) time = -1;
		if(time == 0) time = 1;
		bz_time = time ;
		printk(KERN_ERR"start buzzer\n");
		bz_timer.function = buzzer_timer_func;
		mod_timer(&bz_timer, jiffies + BZ_TIMER_PERIOD);
		if(status == BUZ_FOREVER){
			bz_type = RING_FOREVER;
			//printk(KERN_ERR"Buzzer Forever, bz_type = %d\n", bz_type);
		}
	}
}

//turn on to off, turn off to on
void reverse_on_off_led(unsigned int id, unsigned int color)
{
	int i;
	unsigned long reg;
	
	/* System does not have LED_SET[id] */
	if (led_set[id].presence == 0) return;
	
	for (i = 0; i < LED_COLOR_TOTAL; i++) {
		if ((color & led_set[id].led[i].color) && (led_set[id].led[i].presence != 0)) {
			reg = GPIO_DATA_OUT(led_set[id].led[i].gpio);
			gpio_writel(gpio_readl(reg) ^ (0x1 << GPIO_BIT_SET_OFFSET(led_set[id].led[i].gpio)), reg);
		}
	}
}

void led_blink_start(unsigned int id, unsigned int color, unsigned int state)
{
	int i;
	unsigned long expire_period[] = { 0, 0, JIFFIES_BLINK_SLOW, JIFFIES_BLINK_FAST, JIFFIES_BLINK_VERYSLOW};
	unsigned long slow_expire_period[] = {JIFFIES_BLINK_VERYSLOW_OFF, JIFFIES_BLINK_VERYSLOW_ON};
	short led_color;
	
	if (led_set[id].presence == 0) return;
//	if ((state != LED_BLINK_SLOW) && (state != LED_BLINK_FAST) && (state != LED_BLINK_VERYSLOW)) return;
	if (expire_period[state] == 0) return;
	
	spin_lock(&(led_set[id].lock));
	
	if (led_set[id].timer_state == TIMER_RUNNING) {
		/* Maybe there is already a timer running, restart one. */
		led_set[id].timer_state = TIMER_SLEEPING;
		del_timer(&(led_set[id].timer));
	}
	
	for (i = 0; i < LED_COLOR_TOTAL; i++) {
		if ((color & led_set[id].led[i].color) && (led_set[id].led[i].presence != 0)) {
			led_set[id].led[i].state = state;
			led_color = i;
		}
	}
	
	led_set[id].timer_state = TIMER_RUNNING;

	if (state == LED_BLINK_VERYSLOW)	// according to the request from ZyXEL, HDD LED flash frequence: 0.5s on and 1.5s off
		for (i = LED_HDD1; i <= LED_HDD4; i++)
		{
			if (led_set[i].led[led_color].state == LED_BLINK_VERYSLOW)
			{
				led_set[i].blink_state = 0;		// synchronize all blink state of the HDD leds blinked very slow

				if (i != id)					// reset the led timer for other HDDs
					del_timer(&(led_set[i].timer));
					
				mod_timer(&led_set[i].timer, jiffies + slow_expire_period[led_set[i].blink_state]);
			}
		}
	else
		mod_timer(&led_set[id].timer, jiffies + expire_period[state]);

#if 0
	if (state == LED_BLINK_FAST)
		mod_timer(&led_set[id].timer, jiffies + JIFFIES_BLINK_FAST);
	else if (state == LED_BLINK_SLOW)
		mod_timer(&led_set[id].timer, jiffies + JIFFIES_BLINK_SLOW);
#endif
	
	spin_unlock(&(led_set[id].lock));
}

void led_blink_stop(unsigned int id)
{
	int i;
	
	if (led_set[id].presence == 0) return;
	
	spin_lock(&(led_set[id].lock));
	
	for (i = 0; i < LED_COLOR_TOTAL; i++) {
		if (led_set[id].led[i].presence == 0) continue;
		led_set[id].led[i].state = LED_OFF;
	}
		
	if (led_set[id].timer_state == TIMER_RUNNING) {
		del_timer(&(led_set[id].timer));
	}
	led_set[id].timer_state = TIMER_SLEEPING;
		
	spin_unlock(&(led_set[id].lock));
}

/* all LED_SET[] timer handler for blinking */
static void led_timer_handler(unsigned long data)
{
	struct LED_SET *_led_set = (struct LED_SET*) data;
	int state = LED_BLINK_SLOW;
	int i;
	unsigned long reg;
	unsigned long expire_period[] = { 0, 0, JIFFIES_BLINK_SLOW, JIFFIES_BLINK_FAST, JIFFIES_BLINK_VERYSLOW};
	unsigned long slow_expire_period[] = {JIFFIES_BLINK_VERYSLOW_OFF, JIFFIES_BLINK_VERYSLOW_ON};
	
	spin_lock(&(_led_set->lock));
	
	if (_led_set->timer_state == TIMER_RUNNING) {
		/* Maybe there is already a timer running, restart one. */
		_led_set->timer_state = TIMER_SLEEPING;
		del_timer(&(_led_set->timer));
	}
	
	/* Invert the previous blinking state for next state. */
	_led_set->blink_state ^= 1;
	
	for (i = 0; i < LED_COLOR_TOTAL; i++) {
		if (_led_set->led[i].presence == 0) continue;
		
//		if ((_led_set->led[i].state == LED_BLINK_FAST) || (_led_set->led[i].state == LED_BLINK_SLOW)) {
		if (expire_period[_led_set->led[i].state] != 0) {

			state = _led_set->led[i].state;
			
			reg = GPIO_DATA_OUT(_led_set->led[i].gpio);
			if (_led_set->blink_state == 0) {
				gpio_writel(gpio_readl(reg) & ~(0x1 << GPIO_BIT_SET_OFFSET(_led_set->led[i].gpio)), reg);
			} else {
				gpio_writel(gpio_readl(reg) | (0x1 << GPIO_BIT_SET_OFFSET(_led_set->led[i].gpio)), reg);
			}
		}
	}
	
	_led_set->timer_state = TIMER_RUNNING;
	
	if (state == LED_BLINK_VERYSLOW)
		mod_timer(&_led_set->timer, jiffies + slow_expire_period[_led_set->blink_state]);
	else
		mod_timer(&_led_set->timer, jiffies + expire_period[state]);

#if 0
	if (state == LED_BLINK_FAST)
		mod_timer(&_led_set->timer, jiffies + JIFFIES_BLINK_FAST);
	else if (state == LED_BLINK_SLOW)
		mod_timer(&_led_set->timer, jiffies + JIFFIES_BLINK_SLOW);
#endif
	
	spin_unlock(&(_led_set->lock));
}

static int set_led_config(unsigned long led_data)
{
	unsigned long led_index, color, state;
	
	led_index = GET_LED_INDEX(led_data);
	color = GET_LED_COLOR(led_data);
	state = GET_LED_STATE(led_data);

	/* check the value range of LED_SET type */
	if ((led_index < 0) || (led_index >= LED_TOTAL)) return -ENOTTY;
	
	/* check the LED_SET presence */
	if (led_set[led_index].presence == 0) return -ENOTTY;

	switch (state) {
		case LED_OFF:
			led_blink_stop(led_index);
			turn_off_led_all(led_index);
			break;
		case LED_ON:
			led_blink_stop(led_index);
			turn_on_led(led_index, color);
			break;
		case LED_BLINK_SLOW:
		case LED_BLINK_FAST:
		case LED_BLINK_VERYSLOW:
			turn_off_led_all(led_index);
			led_blink_start(led_index, color, state);
	}

	return 0;
}

void hdd_power_set(struct _hdd_ioctl *hdd_data)
{
	unsigned int port = hdd_data->port;
	unsigned int state = hdd_data->state;
	unsigned int gpio;
	unsigned long reg;

	if (port == 1)
		gpio = HDD1_CTRL_REG_OFFSET;
	else if (port == 2)
		gpio = HDD2_CTRL_REG_OFFSET;
	else if (port == 3)
		gpio = HDD3_CTRL_REG_OFFSET;
	else if (port == 4)
		gpio = HDD4_CTRL_REG_OFFSET;
	else
		return;
	
	reg = GPIO_DATA_OUT(gpio);
	/* HDD Power On */
	if (state == 1)
		gpio_writel(gpio_readl(reg) | (0x1 << GPIO_BIT_SET_OFFSET(gpio)), reg);
	else 
		gpio_writel(gpio_readl(reg) & ~(0x1 << GPIO_BIT_SET_OFFSET(gpio)), reg);

}

static void set_init_timer(void)
{
	int i;

	/* Timer function for power button */
	init_timer(&btnpow_timer);
	btnpow_timer.function = btnpow_timer_func;
	btnpow_timer.data = 0;
	mod_timer(&btnpow_timer, jiffies + (JIFFIES_1_SEC >> 3));
	
	/* Timer function for reset button */
	init_timer(&btnreset_timer);
	btnreset_timer.function = btnreset_timer_func;
	btnreset_timer.data = 0;
	mod_timer(&btnreset_timer, jiffies + (JIFFIES_1_SEC >> 3));
	
	/* Timer function for copy button */
	init_timer(&btncpy_timer);
	btncpy_timer.function = btncpy_timer_func;
	btncpy_timer.data = 0;
	mod_timer(&btncpy_timer, jiffies + (JIFFIES_1_SEC >> 3));

	// init bz timer
	init_timer(&bz_timer);
	bz_timer.function = buzzer_timer_func;
	bz_timer_status = TIMER_SLEEPING;

	// init leds timer
	for (i = 0; i < LED_TOTAL; i++) {
		if (led_set[i].presence == 0) continue;
		
		init_timer(&led_set[i].timer);
		led_set[i].timer.data = (unsigned long) &led_set[i];    /* timer handler can get own LED_SET[i] */	
	}
}

#if 0
irqreturn_t gpio_interrupt(int irq, void *dev_id)
{
	if(irq == RESET_BUTTON_IRQ)
	{
		/* Disable reset button GPIO interrupt */
		disable_irq_nosync(irq);
		
		/* Init timer for reset button */
		printk(KERN_ERR"Trigger the reset button irq!!!\n");
		mod_timer(&btnreset_timer, jiffies + BTN_POLLING_PERIOD);
		
		return IRQ_HANDLED;
	}
	
	if(irq == POWER_BUTTON_IRQ)
	{
		/* Disable power button GPIO line interrupt */
		disable_irq_nosync(irq);
		
		/* Init timer for power button */
		printk(KERN_ERR"Trigger the power button irq!!!\n");
		mod_timer(&btnpow_timer, jiffies + BTN_POLLING_PERIOD);
		
		return IRQ_HANDLED;
	}
	
	if(irq == COPY_BUTTON_IRQ)
	{
		/* Disable the copy button GPIO line interrupt */
		disable_irq_nosync(irq);
		
		/* Init timer for copy button */
		printk(KERN_ERR"Trigger the copy button irq!!!\n");
		mod_timer(&btncpy_timer, jiffies + BTN_POLLING_PERIOD);
		
		return IRQ_HANDLED;
	}
	else;
	
	return IRQ_HANDLED;
}
#endif

static int gpio_open(struct inode *inode , struct file* filp)
{
	return 0;
}

static int gpio_release(struct inode *inode , struct file *filp)
{
	return 0;
}

static ssize_t gpio_read(struct file *file, char *buf, size_t count, loff_t *ptr)
{
	printk(KERN_INFO "Read system call is no useful\n");
	return 0;
}

static ssize_t gpio_write(struct file * file, const char *buf, size_t count, loff_t * ppos)
{
	printk(KERN_INFO "Write system call is no useful\n");
	return 0;
}

static long gpio_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned long ret = 0;
	struct _hdd_ioctl hdd_data;
	struct _mcu_burn mcu_data2;


	/* implement a lock scheme by myself */
	/* get the inode ==> file->f_dentry->d_inode */
	switch (cmd) {
		case BTNCPY_IOC_SET_NUM:
			if(!capable(CAP_SYS_ADMIN)) return -EPERM;
			btncpy_pid = arg;
			break;
		case BUTTON_TEST_IN_IOC_NUM:
			btncpy_pid = arg >> 3;
			atomic_set(&button_test_enable, 1);
			atomic_set(&button_test_num, arg & 0x7);
			break;
		case BUTTON_TEST_OUT_IOC_NUM:
			atomic_set(&button_test_enable, 0);
			atomic_set(&button_test_num, BUTTON_NUM);
			break;
		case BUZ_SET_CTL_IOC_NUM:
			set_buzzer(arg);
			break;
		case LED_SET_CTL_IOC_NUM:       // Just set leds, no check.
			ret = set_led_config(arg);
			if(ret < 0)
				return ret;
			break;
		case HDD_SET_CTL_IOC_NUM:
			if (!copy_from_user(&hdd_data, (void __user *) arg, sizeof(struct _hdd_ioctl)))
				hdd_power_set(&hdd_data);
			break;
		case NAS_IOC_MCU_BURNING:
			is_mcu_burning = 1;
			if (!copy_from_user(&mcu_data2, (void __user *) arg, sizeof(struct _mcu_burn)))
				mcu_burning(&mcu_data2);
			break;

		default :
			return -ENOTTY;
	}
	
	return 0;
}

struct file_operations gpio_fops =
{
	owner:              THIS_MODULE,
	read:               gpio_read,
	write:              gpio_write,
	unlocked_ioctl:     gpio_ioctl,
	open:               gpio_open,
	release:            gpio_release,
};

/*static int htp_status_read_fun(char *buf, char **start, off_t offset,
		int count, int *eof, void *data)
{
	int len;
	
	if (INPUT_PIN_TRIGGERED(HTP_GPIO_REG_OFFSET)) {
		len = sprintf(buf, "1\n");
	} else {
		len = sprintf(buf, "0\n");
	}
	
	*eof = 1;
	
	return len;
}*/

static ssize_t htp_status_read_fun(struct file *file, char __user *buff,
		size_t count, loff_t *pos)
{
	int len;
	char tmpbuf[64];

	if (INPUT_PIN_TRIGGERED(HTP_GPIO_REG_OFFSET)) {
		len = sprintf(tmpbuf, "1\n");
	} else {
		len = sprintf(tmpbuf, "0\n");
	}

	if (*pos != 0)
		len = 0;
	if (!buff)
		return len;
	if (copy_to_user(buff, tmpbuf, len))
		len = 0;
	else
		*pos += len;

	return len;
}

static ssize_t htp_status_write_fun(struct file *file, const char __user *buff,
		size_t count, loff_t *pos)
{
	/* do nothing */
	return 0;
}

static const struct file_operations htp_status_fops = {
	.read = htp_status_read_fun,
	.write = htp_status_write_fun,
};

static ssize_t hdd1_status_read_fun(struct file *file, char __user *buff,
		size_t count, loff_t *pos)
{
	int len;
	char tmpbuf[64];

	if (INPUT_PIN_TRIGGERED(HDD1_DETECT_REG_OFFSET)) {
		len = sprintf(tmpbuf, "0\n");
	} else {
		len = sprintf(tmpbuf, "1\n");
	}

	if (*pos != 0)
		len = 0;
	if (!buff)
		return len;
	if (copy_to_user(buff, tmpbuf, len))
		len = 0;
	else
		*pos += len;

	return len;
}

static ssize_t hdd1_status_write_fun(struct file *file, const char __user *buff,
		size_t count, loff_t *pos)
{
	/* do nothing */
	return 0;
}

static const struct file_operations hdd1_status_fops = {
	.read = hdd1_status_read_fun,
	.write = hdd1_status_write_fun,
};

static ssize_t hdd2_status_read_fun(struct file *file, char __user *buff,
		size_t count, loff_t *pos)
{
	int len;
	char tmpbuf[64];

	if (INPUT_PIN_TRIGGERED(HDD2_DETECT_REG_OFFSET)) {
		len = sprintf(tmpbuf, "0\n");
	} else {
		len = sprintf(tmpbuf, "1\n");
	}

	if (*pos != 0)
		len = 0;
	if (!buff)
		return len;
	if (copy_to_user(buff, tmpbuf, len))
		len = 0;
	else
		*pos += len;

	return len;
}

static ssize_t hdd2_status_write_fun(struct file *file, const char __user *buff,
		size_t count, loff_t *pos)
{
	/* do nothing */
	return 0;
}

static const struct file_operations hdd2_status_fops = {
	.read = hdd2_status_read_fun,
	.write = hdd2_status_write_fun,
};

static ssize_t hdd3_status_read_fun(struct file *file, char __user *buff,
		size_t count, loff_t *pos)
{
	int len;
	char tmpbuf[64];

	if (INPUT_PIN_TRIGGERED(HDD3_DETECT_REG_OFFSET)) {
		len = sprintf(tmpbuf, "0\n");
	} else {
		len = sprintf(tmpbuf, "1\n");
	}

	if (*pos != 0)
		len = 0;
	if (!buff)
		return len;
	if (copy_to_user(buff, tmpbuf, len))
		len = 0;
	else
		*pos += len;

	return len;
}

static ssize_t hdd3_status_write_fun(struct file *file, const char __user *buff,
		size_t count, loff_t *pos)
{
	/* do nothing */
	return 0;
}

static const struct file_operations hdd3_status_fops = {
	.read = hdd3_status_read_fun,
	.write = hdd3_status_write_fun,
};

static ssize_t hdd4_status_read_fun(struct file *file, char __user *buff,
		size_t count, loff_t *pos)
{
	int len;
	char tmpbuf[64];

	if (INPUT_PIN_TRIGGERED(HDD4_DETECT_REG_OFFSET)) {
		len = sprintf(tmpbuf, "0\n");
	} else {
		len = sprintf(tmpbuf, "1\n");
	}

	if (*pos != 0)
		len = 0;
	if (!buff)
		return len;
	if (copy_to_user(buff, tmpbuf, len))
		len = 0;
	else
		*pos += len;

	return len;
}

static ssize_t hdd4_status_write_fun(struct file *file, const char __user *buff,
		size_t count, loff_t *pos)
{
	/* do nothing */
	return 0;
}

static const struct file_operations hdd4_status_fops = {
	.read = hdd4_status_read_fun,
	.write = hdd4_status_write_fun,
};

static ssize_t pwren_usb_read_fun(struct file *file, char __user *buff,
		size_t count, loff_t *pos)
{
	/* do nothing */
	return 0;
}

static ssize_t pwren_usb_write_fun(struct file *file, const char __user *buff,
		size_t count, loff_t *pos)
{
	char tmpbuf[64];
	unsigned int gpio = PWREN_USB_REG_OFFSET;
	unsigned long reg = GPIO_DATA_OUT(gpio);

	if (buff && !copy_from_user(tmpbuf, buff, count))
	{
		tmpbuf[count-1] = '\0';
		if ( tmpbuf[0] == '1' )
		{
			// enable usb
			gpio_writel(gpio_readl(reg) | (0x1 << GPIO_BIT_SET_OFFSET(gpio)), reg);
			printk(KERN_NOTICE " \033[033mUSB is enabled!\033[0m\n");
		}
		else
		{
			// disable usb
			gpio_writel(gpio_readl(reg) & ~(0x1 << GPIO_BIT_SET_OFFSET(gpio)), reg);
			printk(KERN_NOTICE "\033[033mUSB is disabled!\033[0m\n");
		}

	}

	return count;
}

static const struct file_operations pwren_usb_fops = {
	.read = pwren_usb_read_fun,
	.write = pwren_usb_write_fun,
};

static ssize_t mcu_wdt_read_fun(struct file *file, char __user *buff,
		size_t count, loff_t *pos)
{
	int len;
	char tmpbuf[64];
	unsigned int gpio = MCU_WDT_REG_OFFSET;
	unsigned long reg = GPIO_DATA_OUT(gpio);

	unsigned int value = gpio_readl(reg) & (0x1 << GPIO_BIT_SET_OFFSET(gpio));

	if (value == 0)
		len = sprintf(tmpbuf, "0\n");
	else
		len = sprintf(tmpbuf, "1\n");
	
	if (*pos != 0)
		len = 0;
	if (!buff)
		return len;
	if (copy_to_user(buff, tmpbuf, len))
		len = 0;
	else
		*pos += len;

	return len;
}

static ssize_t mcu_wdt_write_fun(struct file *file, const char __user *buff,
		size_t count, loff_t *pos)
{
	char tmpbuf[64];
	unsigned int gpio = MCU_WDT_REG_OFFSET;
	unsigned long reg = GPIO_DATA_OUT(gpio);

	if (buff && !copy_from_user(tmpbuf, buff, count)) 
	{
		tmpbuf[count-1] = '\0';
		if ( tmpbuf[0] == '1' ) 
		{
			// reset mcu watchdog timer
			gpio_writel(gpio_readl(reg) | (0x1 << GPIO_BIT_SET_OFFSET(gpio)), reg);
			printk(KERN_NOTICE " \033[033mMCU watchdog is reset!\033[0m\n");
		}
		else 
		{
			// keep mcu watching timer going
			gpio_writel(gpio_readl(reg) & ~(0x1 << GPIO_BIT_SET_OFFSET(gpio)), reg);
			printk(KERN_NOTICE "\033[033mMCU watchdog is not reset(system is booting up)!\033[0m\n");
		}

    }        

	return count; 
}

static const struct file_operations mcu_wdt_fops = {
	.read = mcu_wdt_read_fun,
	.write = mcu_wdt_write_fun,
};

static int __init gpio_init(void)
{
	int result = 0;
	int err = 0;
	
	/* create /dev/gpio for ioctl using */
	err = alloc_chrdev_region(&gpio_dev, 0, gpio_nr_devs, "gpio");
	if(err < 0)
	{
		printk(KERN_ERR"%s: failed to allocate char dev region\n", __FILE__);
		return -1;
	}
	
	
	printk(KERN_ERR"gpio_dev = %x\n", gpio_dev);
	
	gpio_cdev = cdev_alloc();
	gpio_cdev->ops = &gpio_fops;
	gpio_cdev->owner = THIS_MODULE;
	err = cdev_add(gpio_cdev, gpio_dev, 1);
	
	if(err) printk(KERN_INFO "Error adding device\n");
	
	/* create /proc/htp_pin */
	htp_proc = proc_create_data("htp_pin", 0644, NULL, &htp_status_fops, NULL);
	hdd1_detect_proc = proc_create_data("hdd1_detect", 0644, NULL, &hdd1_status_fops, NULL);
	hdd2_detect_proc = proc_create_data("hdd2_detect", 0644, NULL, &hdd2_status_fops, NULL);
	hdd3_detect_proc = proc_create_data("hdd3_detect", 0644, NULL, &hdd3_status_fops, NULL);
	hdd4_detect_proc = proc_create_data("hdd4_detect", 0644, NULL, &hdd4_status_fops, NULL);
	pwren_usb_proc = proc_create_data("enable_usb", 0644, NULL, &pwren_usb_fops, NULL);
	mcu_wdt_proc = proc_create_data("mcu_wdt", 0644, NULL, &mcu_wdt_fops, NULL);
	
	init_htp_pin_gpio();
	init_hdd_detect_pin_gpio();
	init_hdd_ctrl_pin_gpio();
	init_pwren_usb_pin_gpio();
	init_button_pin_gpio();
	init_buzzer_pin();
	init_led_pin_gpio();
	set_init_timer();

	btn_workqueue = create_workqueue("button controller");

	mcu_workqueue = create_workqueue("mcu controller");

#if 0	
	result = request_irq(POWER_BUTTON_IRQ, gpio_interrupt, IRQF_DISABLED,"Button Event", NULL);
	if(result)
	{
		printk(KERN_ERR"Power Button : Can't get assigned irq %d\n",POWER_BUTTON_IRQ);
	}

	result = request_irq(COPY_BUTTON_IRQ, gpio_interrupt, IRQF_DISABLED,"Button Event", NULL);
	if(result)
	{
		printk(KERN_ERR"Power Button : Can't get assigned irq %d\n",COPY_BUTTON_IRQ);
	}

	result = request_irq(RESET_BUTTON_IRQ, gpio_interrupt, IRQF_DISABLED,"Button Event", NULL);
	if(result)
	{
		printk(KERN_ERR"Power Button : Can't get assigned irq %d\n",RESET_BUTTON_IRQ);
	}
#endif	

	return result;
}

static void __exit gpio_exit(void)
{

#if 0	
	free_irq(POWER_BUTTON_IRQ, NULL);
	free_irq(COPY_BUTTON_IRQ, NULL);
	free_irq(RESET_BUTTON_IRQ, NULL);
#endif	
	
	remove_proc_entry("htp_pin", NULL);
	remove_proc_entry("hdd1_detect", NULL);
	remove_proc_entry("hdd2_detect", NULL);
	remove_proc_entry("hdd3_detect", NULL);
	remove_proc_entry("hdd4_detect", NULL);
	remove_proc_entry("enable_usb", NULL);
	remove_proc_entry("mcu_wdt", NULL);
	
	if(timer_pending(&btnpow_timer))
		del_timer(&btnpow_timer);
	if(timer_pending(&btnreset_timer))
		del_timer(&btnreset_timer);
	if(timer_pending(&btncpy_timer))
		del_timer(&btncpy_timer);

	unregister_chrdev_region(gpio_dev, gpio_nr_devs);
	destroy_workqueue(btn_workqueue);
}

module_init(gpio_init);
module_exit(gpio_exit);

