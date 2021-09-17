#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/semaphore.h>
#include <linux/miscdevice.h>
#include <linux/hi_ipcm_usrdev.h>

extern void *vmalloc(unsigned long size);
extern void vfree(void *addr);
struct kcom_ipcm *ipcm_kcom_handle;

spinlock_t msg_list_lock;

/* mem_list using for handle mem clean */
struct hios_mem_list {
	struct list_head head;
	void *data;
	unsigned int data_len;
};

/* handle_list using for relase func clean handle */
static struct semaphore handle_sem;

static int hios_ipcm_notifier_recv_ipcm(void *vdd_handle,
					void *buf,
					unsigned int length)
{
	hios_ipcm_handle_t ipcm_handle;
	hios_ipcm_handle_t *_handle;
	hios_ipcm_handle_opt_t opt;

	struct hios_mem_list *mem;
	void *data;
	unsigned long flags;

	ipcm_handle.ipcm_handle = (unsigned long)vdd_handle;
	hios_ipcm_getopt(&ipcm_handle, &opt);

	_handle = (hios_ipcm_handle_t *)opt.data;

	data = kmalloc(length + sizeof(struct hios_mem_list),
			GFP_ATOMIC);
	if (!data) {
		hi_ipcm_trace(3,
			"nortifier error %d\n",
			0);
		return -1;
	}

	hi_ipcm_trace(1,
		"nortifier_recv addr 0x%8lx len %d\n",
		(unsigned long)buf,
		length);

	mem = (struct hios_mem_list *)data;
	mem->data = data + sizeof(struct hios_mem_list);

	memcpy((void *)mem->data, (void *)buf, length);

	mem->data_len = length;
	spin_lock_irqsave(&msg_list_lock, flags);
	list_add_tail(&mem->head, &_handle->mem_list);
	spin_unlock_irqrestore(&msg_list_lock, flags);
	wake_up_interruptible(&_handle->wait);
	return 0;
}

static void usrdev_setopt_recv_ipcm(hios_ipcm_handle_t *handle)
{
	hios_ipcm_handle_opt_t opt;
	opt.recvfrom_notify = &hios_ipcm_notifier_recv_ipcm;
	opt.data = (unsigned long) handle;
	hios_ipcm_setopt(handle, &opt);
}

static void usrdev_setopt_null(hios_ipcm_handle_t *handle)
{
	hios_ipcm_handle_opt_t opt;
	opt.recvfrom_notify = NULL;
	opt.data = 0;
	hios_ipcm_setopt(handle, &opt);
}

static int hi_ipcm_userdev_open(struct inode *inode, struct file *file)
{
	file->private_data = 0;
	sema_init(&handle_sem, 1);
	spin_lock_init(&msg_list_lock);
	return 0;
}

static void _del_mem_list(hios_ipcm_handle_t *handle)
{
	struct list_head *entry , *tmp;
	struct hios_mem_list *mem;
	/* if mem list empty means no data comming*/
	if (!list_empty(&handle->mem_list)) {
		list_for_each_safe(entry, tmp, &handle->mem_list) {
			mem = list_entry(entry, struct hios_mem_list, head);
			list_del(&mem->head);
			kfree(mem);
			hi_ipcm_trace(3,
				"handle 0x%8lx not empty\n",
				(unsigned long)handle);
		}
	}
}

static int hi_ipcm_userdev_release(struct inode *inode,
				struct file *file)
{
	unsigned long flags;

	hios_ipcm_handle_t *handle =
		(hios_ipcm_handle_t *) file->private_data;
	if (!handle) {
		pr_info("handle is not open !\n");
		return -1;
	}

	hi_ipcm_trace(1, "close 0x%8lx\n", (unsigned long)handle);

	usrdev_setopt_null(handle);

	down(&handle_sem);

	/* if mem list empty means no data comming*/
	spin_lock_irqsave(&msg_list_lock, flags);
	if (!list_empty(&handle->mem_list))
		_del_mem_list(handle);
	spin_unlock_irqrestore(&msg_list_lock, flags);

	hios_ipcm_close(handle);
	file->private_data = 0;
	up(&handle_sem);
	hi_ipcm_trace(2, "release success 0x%d\n", 0);
	return 0;
}

static int hi_ipcm_userdev_read(struct file *file,
				char __user *buf,
				size_t count,
				loff_t *f_pos)
{
	hios_ipcm_handle_t *handle =
		(hios_ipcm_handle_t *) file->private_data;
	struct list_head *entry, *tmp;
	unsigned int len = 0;
	unsigned long readed = 0;
	unsigned long flags;
	struct hios_mem_list *mem;

	if (!handle) {
		pr_info("handle is not open\n");
		return -1;
	}

	hi_ipcm_trace(2, "read  empty %d handle 0x%8lx\n",
			list_empty(&handle->mem_list),
			(unsigned long) handle);

	spin_lock_irqsave(&msg_list_lock, flags);

	/* if mem list empty means no data comming*/
	if (!list_empty(&handle->mem_list)) {
		list_for_each_safe(entry, tmp, &handle->mem_list) {
			mem = list_entry(entry, struct hios_mem_list, head);
			len = mem->data_len;
			if (len > count)
				len = count;

			list_del(&mem->head);
			break;
		}
	}

	spin_unlock_irqrestore(&msg_list_lock, flags);

	if (len) {
		readed = copy_to_user(buf, mem->data, len);
		if (readed != 0)
			pr_err("copy to user error!\n");
		hi_ipcm_trace(1, "read %d\n", len);
		kfree(mem);
	}

	hi_ipcm_trace(1, "read success %d\n", len);
	return len;
}

static int hi_ipcm_userdev_write(struct file *file,
				const char __user *buf,
				size_t count,
				loff_t *f_pos)
{
	int ret = 0;
	unsigned long writed = 0;
	char *kbuf;
	hios_ipcm_handle_t *handle = (hios_ipcm_handle_t *)
					file->private_data;
	kbuf = vmalloc(count);
	if (!handle || !buf) {
		pr_err("handle or buffer is null, please check\n");
		return -1;
	}

	writed = copy_from_user(kbuf, buf, count);
	if (writed < 0)
		pr_err("copy from user error\n");
	hi_ipcm_trace(1, "ipcm_handle%p\n", handle);

	ret = hios_ipcm_sendto(handle, kbuf, count);
	if (ret < 0)
		hi_ipcm_trace(3, "sendto error! %d\n", 0);

	vfree(kbuf);
	hi_ipcm_trace(1, "send success %d\n", ret);
	return ret;
}

static long hi_ipcm_userdev_ioctl(struct file *file,
				unsigned int cmd,
				unsigned long arg)
{
	hios_ipcm_handle_t *handle;
	struct hi_ipcm_handle_attr attr;
	int check;
	int local_id;
	down(&handle_sem);

	if (copy_from_user((void *)&attr, (void *)arg,
			sizeof(struct hi_ipcm_handle_attr))) {
		hi_ipcm_trace(3, "can not get the parameter %d\n", 0);
		up(&handle_sem);
		return -1;
	}

	if (_IOC_TYPE(cmd) == 'M') {
		switch (_IOC_NR(cmd)) {
		case _IOC_NR(HI_IPCM_IOC_ATTR_INIT):
			hios_ipcm_handle_attr_init(&attr);
			if (copy_to_user((void *)arg,
					(void *)&attr,
					sizeof(struct hi_ipcm_handle_attr))) {
				hi_ipcm_trace(3,
					"can not put the parameter to user mode %d\n",
					0);
				up(&handle_sem);
				return -1;
			}
			up(&handle_sem);
			break;

		case _IOC_NR(HI_IPCM_IOC_CONNECT):
			handle = hios_ipcm_open(&attr);
			if (handle) {
				INIT_LIST_HEAD(&handle->mem_list);
				init_waitqueue_head(&handle->wait);
				file->private_data = (void *)handle;
				usrdev_setopt_recv_ipcm(handle);
				if (copy_to_user((void *)arg,
					(void *)&handle,
					sizeof(handle)))
					hi_ipcm_trace(3,
						"copy handle to user failed %x\n!",
						(unsigned int)handle);
				hi_ipcm_trace(1,
					"open success 0x%8lx\n",
					(unsigned long)handle);
			} else {
				file->private_data = NULL;
				up(&handle_sem);
				return -1;
			}

			up(&handle_sem);
			break;

		case _IOC_NR(HI_IPCM_IOC_CHECK):
			handle = (hios_ipcm_handle_t *)
				file->private_data;
			check = hios_ipcm_check_remote(
					attr.target_id,
					(hios_ipcm_handle_t *)handle);
			up(&handle_sem);
			return check;

		case _IOC_NR(HI_IPCM_IOC_GET_LOCAL_ID):
			handle = (hios_ipcm_handle_t *)
				file->private_data;
			local_id = hios_ipcm_getlocalid(
					(hios_ipcm_handle_t *)handle);
			up(&handle_sem);
			return local_id;

		case _IOC_NR(HI_IPCM_IOC_GET_REMOTE_ID):
			handle = (hios_ipcm_handle_t *)
				file->private_data;
			hios_ipcm_getremoteids(attr.remote_id,
					(hios_ipcm_handle_t *)handle);
			if (copy_to_user((void *)arg, (void *)&attr,
				sizeof(struct hi_ipcm_handle_attr))) {
				hi_ipcm_trace(3,
					"can not put the para to user mode %d\n",
					0);
				up(&handle_sem);
				return -1;
			}
			up(&handle_sem);
			break;

		default:
			up(&handle_sem);
			hi_ipcm_trace(3,
				"warning not defined cmd %d\n", 0);
			break;
		}
	}

	return 0;
}

static unsigned int hi_ipcm_userdev_poll(struct file *file,
		struct poll_table_struct *table)
{
	unsigned long flags;

	hios_ipcm_handle_t *handle = (hios_ipcm_handle_t *) file->private_data;
	if (!handle) {
		pr_err("handle is not open\n");
		return -1;
	}
	poll_wait(file, &handle->wait, table);

	/* if mem list empty means no data comming*/
	spin_lock_irqsave(&msg_list_lock, flags);
	if (!list_empty(&handle->mem_list)) {
		hi_ipcm_trace(2, "poll not empty handle 0x%8lx\n",
				(unsigned long)handle);
		spin_unlock_irqrestore(&msg_list_lock, flags);
		return POLLIN | POLLRDNORM;
	}

	spin_unlock_irqrestore(&msg_list_lock, flags);
	return 0;
}

static const struct file_operations ipcm_userdev_fops = {
	.owner		= THIS_MODULE,
	.open		= hi_ipcm_userdev_open,
	.release	= hi_ipcm_userdev_release,
	.unlocked_ioctl	= hi_ipcm_userdev_ioctl,
	.write		= hi_ipcm_userdev_write,
	.read		= hi_ipcm_userdev_read,
	.poll		= hi_ipcm_userdev_poll,
};

static struct miscdevice hi_ipcm_userdev = {
	.minor  = MISC_DYNAMIC_MINOR,
	.fops   = &ipcm_userdev_fops,
	.name   = "ipcm_userdev"
};

int __init hi_mmc_userdev_init(void)
{
	misc_register(&hi_ipcm_userdev);
	return 0;
}

void __exit hi_mmc_userdev_exit(void)
{
	misc_deregister(&hi_ipcm_userdev);
}

module_init(hi_mmc_userdev_init);
module_exit(hi_mmc_userdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("chanjinn");
