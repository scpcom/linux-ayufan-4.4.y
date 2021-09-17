#include <asm/uaccess.h>
#include <linux/io.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define HI_GPIO_BASE_ADDR    0x12150000
#define HI_GPIO_GROUP_OFFSET 0x00010000
#define HI_GPIO_GROUP_ADDR(GROUP)   (HI_GPIO_BASE_ADDR + (GROUP) * HI_GPIO_GROUP_OFFSET)

#define HI_GPIO_DATA_ADDR(PIN)      (0x4 << (PIN))
#define HI_GPIO_GET_GROUP(PIN)      ((PIN) / 8)
#define HI_GPIO_GET_GROUP_PIN(PIN)  ((PIN) & 0x7)

#define HI_GPIO_PIN_IO_ADDR(PIN)    ((volatile void __iomem *)IO_ADDRESS(HI_GPIO_GROUP_ADDR(HI_GPIO_GET_GROUP(PIN)) + HI_GPIO_DATA_ADDR(HI_GPIO_GET_GROUP_PIN(PIN))))

void syno_gpio_write(int gpio, int value)
{
	writel(value << HI_GPIO_GET_GROUP_PIN(gpio), HI_GPIO_PIN_IO_ADDR(gpio));
}
EXPORT_SYMBOL(syno_gpio_write);

int syno_gpio_read(int gpio)
{
	return readl(HI_GPIO_PIN_IO_ADDR(gpio)) >> HI_GPIO_GET_GROUP_PIN(gpio);
}
EXPORT_SYMBOL(syno_gpio_read);

#define USB3_GPIO_PIN 45
#define MANUFACTURING_GPIO_PIN 54

static int usb3_gpio_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", syno_gpio_read(USB3_GPIO_PIN));
	return 0;
}

static int usb3_gpio_open(struct inode *inode, struct file *file)
{
	return single_open(file, usb3_gpio_proc_show, NULL);
}

static ssize_t usb3_write(struct file *file, const char __user *buf,
		size_t count, loff_t *offset)
{
	int ret;
	long val;
	char buffer[3];

	memset(buffer, 0, sizeof(buffer));
	if (count == 0 || count > sizeof(buffer) - 1) {
		pr_err("GPIO input value must be 0 or 1!\n");
		return -EINVAL;
	}
	if (copy_from_user(buffer, buf, count)) {
		return -EFAULT;
	}

	ret = kstrtol(buffer, 10, &val);
	if (ret < 0 || val < 0 || val > 1) {
		pr_err("GPIO input value must be 0 or 1!\n");
	} else {
		syno_gpio_write(USB3_GPIO_PIN, val);
	}

	return count;
}

static const struct file_operations proc_usb3_gpio_operations = {
	.open		= usb3_gpio_open,
	.read		= seq_read,
	.write		= usb3_write,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

static int manufacturing_gpio_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", syno_gpio_read(MANUFACTURING_GPIO_PIN));
	return 0;
}

static int manufacturing_gpio_open(struct inode *inode, struct file *file)
{
	return single_open(file, manufacturing_gpio_proc_show, NULL);
}

static const struct file_operations proc_manufacturing_gpio_operations = {
	.open		= manufacturing_gpio_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

static int __init proc_gpio_init(void)
{
	proc_create("manufacturing_gpio", 0, NULL, &proc_manufacturing_gpio_operations);
	proc_create("usb3_gpio", 0, NULL, &proc_usb3_gpio_operations);
	return 0;
}
module_init(proc_gpio_init);
