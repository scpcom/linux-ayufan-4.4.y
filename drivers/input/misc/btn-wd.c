//
// Driver to support reset button interactions
//

#include <linux/kernel.h>
#include <mach/gpio.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/major.h>
#include <linux/poll.h>
#include <linux/freezer.h>
#include <linux/delay.h> 
//#include <linux/suspend.h>
//#include <linux/rculist.h>
//#include <linux/sched.h>
//#include <linux/ioport.h>
//#include <linux/completion.h>
//#include <linux/leds.h>
//#include <linux/of.h>
//#include <linux/of_platform.h>
//#include <linux/spinlock.h>
//#include <linux/types.h>
//#include <linux/wait.h>
//#include <linux/signal.h>
//#include <linux/ioport.h>
//#include <linux/netlink.h>
//#include <linux/init.h>
//#include <linux/device.h>


#define DEV_NAME           "sequoia"

#define WPS_BTN            GPIO_PIN_1    // Active-low button
#define RESET_BTN          GPIO_PIN_2    // Active-low button
#define ALL_BTN            (GPIO_PIN_1 | GPIO_PIN_2 )

#define RESET_BTN_INDX	   0

#define BTNDEV_MINOR_BASE  31
#define BTNDEV_MINORS      31

#define PACKET_QUEUE_LEN   16
#define POL_RATE_MSECS     200

struct btndev_hw_data {
	int abs_event;
	unsigned long buttons;
};

struct btndev {
	int exist;
	int open;
	int minor;
	struct input_handle handle;
	wait_queue_head_t wait;
	struct list_head client_list;
	spinlock_t client_lock;        // protects client_list
	struct mutex mutex;
	struct device dev;
	struct list_head mixdev_node;
	int mixdev_open;
	struct btndev_hw_data packet;
	unsigned int pkt_count;
};

struct btndev_motion {
	unsigned long buttons;
};

struct btn_client {
	struct fasync_struct *fasync;
	struct btndev *btndev;
	struct list_head node;

	struct btndev_motion packets[PACKET_QUEUE_LEN];
	unsigned int head, tail;
	spinlock_t packet_lock;

	signed char ps2[6];
	unsigned char ready, buffer, bufsiz;
	unsigned char imexseq, impsseq;
	unsigned long last_buttons;
};

static void __iomem *button_port = NULL;
static struct input_dev *input_dev;
struct btndev       *btndev_ptr;
struct task_struct  *btn_threadptr;
struct btndev       *btndev_table[BTNDEV_MINORS];
static DEFINE_MUTEX(btndev_table_mutex);
struct input_event btn_input_event;
int    btn_ev_flag = 0;


static void btn_dev_event(struct input_handle *handle, unsigned int type, unsigned int code, int value) {
	switch (type) {
		case EV_KEY:
			printk(KERN_ERR "Key Press\n");
			break;
		case EV_REL:
			printk(KERN_ERR "Key Release\n");
			break;
		default:
		break;
	}
}

static ssize_t btn_read(struct file *file, char __user *buffer, size_t count, loff_t *ppos) {
	struct btn_client *client = file->private_data;
	struct btndev *btndev = client->btndev;
	#define MAX_EV_LEN sizeof(struct input_event)

	signed char data[MAX_EV_LEN];
	int retval = 0; 

	if (file->f_flags & O_NONBLOCK) {
		/* 
		* This read is from VFT, as VFT does a NON_BLOCKING call for btn
		* status, hence, copy data to user buffer anyway.
		*/
		spin_lock_irq(&client->packet_lock);
		if (count > MAX_EV_LEN)
			count = MAX_EV_LEN;
		memcpy(data, (char*)&btn_input_event, count);
		/* client->buffer -= count; */
		spin_unlock_irq(&client->packet_lock);
		if (copy_to_user(buffer, data, count))
			return -EFAULT;

		return -EAGAIN;
	}

	retval = wait_event_interruptible(btndev->wait, !btndev->exist || 
	                                                client->ready  || 
	                                                btn_ev_flag);
	if (btn_ev_flag)
		btn_ev_flag = 0;
	if (retval)
		return retval;
	if (!btndev->exist)
		return -ENODEV;
	spin_lock_irq(&client->packet_lock);           // Disable interrupts 
	if (count > MAX_EV_LEN)                        // Don't write more than Maximum length of an event
		count = MAX_EV_LEN;                
	memcpy(data, (char*)&btn_input_event, count);  // Setup struct Data to be sent to /dev
	client->buffer -= count;                       // remove NULL
	spin_unlock_irq(&client->packet_lock);         // Re-enable interrupts
	if (copy_to_user(buffer, data, count))         // Write struct to /dev
		return -EFAULT;

	return count;
}
//static ssize_t btndev_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos) { return 0; }
//static unsigned int btndev_poll(struct file *file, poll_table *wait) { return 0; }
//static int btndev_release(struct inode *inode, struct file *file) { return 0; }
static int btndev_fasync(int fd, struct file *file, int on) {
	struct btn_client *client = file->private_data;
	return fasync_helper(fd, file, on, &client->fasync);
}

static int btndev_open_device(struct btndev *btndev) {
	int retval;

	retval = mutex_lock_interruptible(&btndev->mutex);
	if (retval)
		return retval;

	if (!btndev->exist)
		retval = -ENODEV;
	else if (!btndev->open++) {
		retval = input_open_device(&btndev->handle);
		if (retval)
			btndev->open--;
	}
	mutex_unlock(&btndev->mutex);


	return retval;
}

static void btndev_attach_client(struct btndev *btndev, struct btn_client *client) {
	spin_lock(&btndev->client_lock);
	list_add_tail_rcu(&client->node, &btndev->client_list);
	spin_unlock(&btndev->client_lock);
	synchronize_rcu();
}

static void btndev_detach_client(struct btndev *btndev, struct btn_client *client) {
	spin_lock(&btndev->client_lock);
	list_del_rcu(&client->node);
	spin_unlock(&btndev->client_lock);
	synchronize_rcu();
}

static int btndev_open(struct inode *inode, struct file *file) {
	struct btn_client *client;
	struct btndev *btndev;
	int error;
	int i;

	i = iminor(inode) - BTNDEV_MINOR_BASE;
	if (i >= BTNDEV_MINORS) {
		printk(KERN_ERR "[Error] btndev_open() failure!\n");
		return -ENODEV;
	}

	error = mutex_lock_interruptible(&btndev_table_mutex);
	if (error) {
		return error;
	}    

	btndev = btndev_table[i];
	if (btndev)
		get_device(&btndev->dev);
	else
		return -ENODEV;
	mutex_unlock(&btndev_table_mutex);

	client = kzalloc(sizeof(struct btn_client), GFP_KERNEL);
	if (!client) {
		error = -ENOMEM;
		goto err_put_btndev;
	}

	spin_lock_init(&client->packet_lock);
	client->btndev = btndev;
	btndev_attach_client(btndev, client);

	error = btndev_open_device(btndev);
	if (error)
		goto err_free_client;

	file->private_data = client;
	return 0;

err_free_client:
	btndev_detach_client(btndev, client);
	kfree(client);
err_put_btndev:
	put_device(&btndev->dev);
	return error;
}


static const struct file_operations btn_fops = {
        .owner =        THIS_MODULE,
        .read =         btn_read,
        .write =        0/*btndev_write*/,
        .poll =         0/*btndev_poll*/,
        .open =         btndev_open,
        .release =      0/*btndev_release*/,
        .fasync =       btndev_fasync,
};

/*static int btn_connect(struct input_handler *handler, struct input_dev *dev, const struct input_device_id *id) {
	return 0;
}

static void btn_disconnect(struct input_handle *handle) { }*/

static const struct input_device_id btndev_ids[] = {
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT  |
		         INPUT_DEVICE_ID_MATCH_KEYBIT |
		         INPUT_DEVICE_ID_MATCH_RELBIT,
		         .evbit = { BIT_MASK(EV_KEY) | BIT_MASK(EV_REL) },
	},      
	{},
};


static struct input_handler btn_handler = {
        .event      = btn_dev_event,
        .connect    = 0/*btn_connect*/,
        .disconnect = 0/*btn_disconnect*/,
        .fops       = &btn_fops,
        .minor      = BTNDEV_MINOR_BASE,
        .name       = "btndev",
        .id_table   = btndev_ids,
};

static int which_button_pressed(void) {
	u32 reg_val = 0;
	u32 cmp_val = 0;

	reg_val = __raw_readl(button_port);  // Read current register value
	cmp_val = (~reg_val & ALL_BTN);      // Buttons are active low

	switch (cmp_val) {
		case WPS_BTN:                // WPS button is pressed
			return 1;
			break;
		case RESET_BTN:              // Reset button is pressed
			return 2;
			break;
		case ALL_BTN:                // Both are pressed
			return 3;
			break;
		default:                     // None is pressed
			return 0;
			break;
	}
	
}

static int btn_thread(void *data) {
	struct task_struct *tsk = current;
	struct sched_param param = {.sched_priority = 1}; 
	int btn_last_state, btn_cur_state;
	btn_cur_state = btn_last_state  = EV_REL;

	sched_setscheduler(tsk, SCHED_FIFO, &param);
	set_freezable();

	if (button_port) {
		btn_last_state = which_button_pressed();
	}
	while( !kthread_should_stop() ) {
		msleep(POL_RATE_MSECS);
		if (button_port) {
			btn_cur_state = which_button_pressed();
		}
		if (btn_last_state != btn_cur_state) {
			//  printk(KERN_INFO "state changed from %d to %d\n", btn_last_state, btn_cur_state);
			btn_last_state = btn_cur_state;
			do_gettimeofday(&btn_input_event.time);
			btn_input_event.type  = btn_cur_state;  // type of button is apparent from the returned status
			btn_input_event.code  = btn_cur_state;  // code is irrelevant
			btn_input_event.value = btn_cur_state;  // value is irrelevant
			btn_ev_flag = 1;                        // wake up event read [line 135]
			if (btndev_ptr) {
				wake_up_interruptible(&(btndev_ptr->wait));
			}
		}
		
	}
	return 0;
}


//============================================================================//
//                         Button Helper functions                            //
//============================================================================//
static int btndev_install_chrdev(struct btndev *btndev) {
        btndev_table[btndev->minor] = btndev;
        return 0;
}

static void btndev_remove_chrdev(struct btndev *btndev) {
        mutex_lock(&btndev_table_mutex);
        btndev_table[btndev->minor] = NULL;
        mutex_unlock(&btndev_table_mutex);
}

static void btndev_mark_dead(struct btndev *btndev) {
        mutex_lock(&btndev->mutex);
        btndev->exist = 0;
        mutex_unlock(&btndev->mutex);
}

static void btndev_cleanup(struct btndev *btndev) {
	struct input_handle *handle = &btndev->handle;

	btndev_mark_dead(btndev);
	btndev_remove_chrdev(btndev);
	if (btndev->open)
		input_close_device(handle);
}

static void btndev_free(struct device * dev) {
	struct btndev * btndevptr = container_of( dev, struct btndev, dev);
	input_put_device( btndevptr->handle.dev);
	kfree(btndevptr);
}

static struct btndev *btndev_create(struct input_dev *dev, struct input_handler *handler, int minor) {
	struct btndev *btndev;
	int error;

	btndev = kzalloc(sizeof(struct btndev), GFP_KERNEL);
	if (!btndev) {
		error = -ENOMEM;
		goto err_out;
	}

	INIT_LIST_HEAD(&btndev->client_list);
	INIT_LIST_HEAD(&btndev->mixdev_node);
	spin_lock_init(&btndev->client_lock);
	mutex_init(&btndev->mutex);
	lockdep_set_subclass(&btndev->mutex, 0);
	init_waitqueue_head(&btndev->wait);

	dev_set_name(&btndev->dev, "button");

	btndev->minor = minor;
	btndev->exist = 1;
	btndev->handle.dev = input_get_device(dev);
	btndev->handle.name = dev_name(&btndev->dev);
	btndev->handle.handler = handler;
	btndev->handle.private = btndev;

	btndev->dev.class = &input_class;
	if (dev)
		btndev->dev.parent = &dev->dev;
	btndev->dev.devt = MKDEV(INPUT_MAJOR, BTNDEV_MINOR_BASE + minor);
	btndev->dev.release = btndev_free;
	device_initialize(&btndev->dev);

	error = input_register_handle(&(btndev->handle));
	if (error) {
		goto err_free_btndev;
	}

	error = btndev_install_chrdev(btndev);
	if (error)
		goto err_unregister_handle;

	error = device_add(&btndev->dev);
	if (error)
		goto err_cleanup_btndev;

	return btndev;    // When everything is OK

err_cleanup_btndev:
	btndev_cleanup(btndev);

err_unregister_handle:
	input_unregister_handle(&btndev->handle);

err_free_btndev:
	put_device(&btndev->dev);

err_out:
	return ERR_PTR(error);
}


static int button_dev_init(struct platform_device *parent_pdev) {
	int error;

	input_dev = input_allocate_device();
	if (!input_dev) {
		printk(KERN_ERR "[Error] unable to allocate input_dev!\n");
		return -ENOMEM;
	}

	input_dev->name = "btn_wd";
	input_dev->phys = "sequoia/input0";
	input_dev->id.bustype = BUS_HOST;
	input_set_capability(input_dev, EV_MSC, MSC_SCAN);
	__set_bit( EV_KEY, input_dev->evbit);
	__set_bit( EV_REL, input_dev->relbit);

	error = input_register_device(input_dev);
	if (error) {
		input_free_device( input_dev );
		printk(KERN_ERR "[Error] input_register_device() failure!\n");
		return error;
	}

	error = input_register_handler( &btn_handler);
	if (error) {
		printk(KERN_ERR "[Error] input_register_handler() failure!\n");
		input_free_device( input_dev );
		/*btndev_destroy(btndev_ptr);*/
		return error;
	}

	btndev_ptr = btndev_create(input_dev , &btn_handler, RESET_BTN_INDX);
	if ( IS_ERR(btndev_ptr) ) {
		input_free_device(input_dev);
		printk(KERN_ERR "[Error] btndev_create() failure!\n");
		return PTR_ERR(btndev_ptr);
	}


	return 0;
}


static int __init WD_btn_init(void) {
	int retval;

	button_port = (void __iomem *) COMCERTO_GPIO_INPUT_REG;

	retval = button_dev_init(NULL);
	if (retval != 0) {
		printk(KERN_ERR "[Error] button_dev_init() failure - Code %d!\n", retval);
		iounmap(button_port);
		button_port = NULL;
		return -1;
	} 

	btn_threadptr = kthread_run(btn_thread, NULL, "btn_t");
	retval = (btn_threadptr == NULL) ? -1 : 0;


	return retval;
 
	/* return platform_driver_register( &a3g_button_driver ); */
}


module_init(WD_btn_init);

MODULE_AUTHOR("Arya Ahmadi-Ardakani");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Support for WD specific platform buttons");
MODULE_ALIAS("platform:" DEV_NAME);
