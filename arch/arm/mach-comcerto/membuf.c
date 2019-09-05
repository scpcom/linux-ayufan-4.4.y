/*
 *  linux/arch/arm/mach-comcerto/membuf.c
 *
 *  Copyright (C) 2010 Mindspeed Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/scatterlist.h>
#include <asm/uaccess.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33)
#define sg_page(sg)	((sg)->page)

#define sg_virt(sg) (page_address(sg_page(sg)) + (sg)->offset)
#endif

#define MAX_BUFFERS	48

#define MEMBUF_MINOR		0
#define MEMBUF_MINOR_COUNT	1
#define MEMBUF_DEV_COUNT	1

#define MEMBUF_DEFAULT_PG_ORDER	6

#define MEMBUF_DRV_NAME		"membuf"


#define MEMBUF_GET_SCATTER _IOR('m', 1, struct usr_scatter_list)

struct usr_scatter_list
{
	u8 entries;
	u8 pg_order[MAX_BUFFERS];
	u32 addr[MAX_BUFFERS];
};

struct membuf
{
	struct scatterlist sg[MAX_BUFFERS];
	int sg_nr;

	int size;	/* Total memory in use for the buffer data */
	int real_size;	/* Total memory allocated for the scatter list */

	int pg_order;

	char mapped;
};

static struct membuf_dev
{
	struct cdev dev;
	int devno;
} membuf_dev;


static int membuf_map(struct membuf *buf)
{
	int rc;

	if (buf->mapped)
	{
		printk(KERN_ERR "%s: buffer already mapped\n", __func__);
		rc = -EINVAL;
		goto out;
	}

	rc = dma_map_sg(NULL, buf->sg, buf->sg_nr, DMA_TO_DEVICE);
	if (rc != buf->sg_nr)
		printk(KERN_ERR "%s: dma_map_sg() failed\n", __func__);
	else
		buf->mapped = 1;

out:
	return rc;
}

static void membuf_unmap(struct membuf *buf)
{
	if (!buf->mapped)
		return;

	dma_unmap_sg(NULL, buf->sg, buf->sg_nr, DMA_TO_DEVICE);

	buf->mapped = 0;
}

static void membuf_free(struct membuf *buf)
{
	struct scatterlist *sg;
	int i;

	for (i = 0, sg = buf->sg; i < buf->sg_nr; i++, sg++) {
		__free_pages(sg_page(sg), get_order(sg->length));
	}

	buf->sg_nr = 0;
	buf->size = 0;
	buf->real_size = 0;
	buf->pg_order = MEMBUF_DEFAULT_PG_ORDER;
}

static void membuf_sync(struct membuf *buf)
{
	dma_sync_sg_for_device(NULL, buf->sg, buf->sg_nr, DMA_TO_DEVICE);
}

static int membuf_expand(struct membuf *buf, int size)
{
	struct scatterlist *sg;
	void *page;
	int len;

	membuf_unmap(buf);

	while (size > 0)
	{
		if (buf->sg_nr >= MAX_BUFFERS) {
			printk(KERN_ERR "%s: too many scatter entries(%d) failed\n", __func__, buf->sg_nr);
			return -ENOMEM;
		}

	retry:
		page = (void *)__get_free_pages(GFP_KERNEL | __GFP_NOWARN, buf->pg_order);
		if (!page) {
			if (buf->pg_order-- > 1)
				goto retry;

			printk(KERN_ERR "%s: __get_free_pages(%d) failed\n", __func__, buf->pg_order);
			return -ENOMEM;
		}

		sg = &buf->sg[buf->sg_nr];

		len = PAGE_SIZE * (1 << buf->pg_order);

		sg_set_buf(sg, page, len);

		buf->sg_nr++;

		buf->real_size += len;

		if (size > len)
			buf->size += len;
		else
			buf->size += size;

		size -= len;
	}

	return 0;
}


static int get_sg_index(struct membuf *buf, int *offset)
{
	struct scatterlist *sg;
	int i;

	for (i = 0, sg = buf->sg; i < buf->sg_nr; i++, sg++) {
		if (*offset < sg->length)
			break;

		*offset -= sg->length;
	}

//	printk(KERN_INFO "%d %d\n", i, *offset);

	return i;
}


static ssize_t membuf_write(struct file *file, const char __user *data, size_t count, loff_t *off)
{
	struct membuf *buf = (struct membuf *)file->private_data;
	struct scatterlist *sg;
	int sg_offset, sg_i;
	int count_now;
	int offset = 0;
	int rc;

//	printk(KERN_INFO "%d\n", *off);

	if ((*off + count) > buf->real_size) {

		rc = membuf_expand(buf, (*off + count) - buf->real_size);
		if (rc < 0) {
			printk(KERN_ERR "%s: membuf_expand() failed\n", __func__);
			goto out;
		}
	}

	sg_offset = *off;
	sg_i = get_sg_index(buf, &sg_offset);
	sg = &buf->sg[sg_i];

	while (count) {

		count_now = min(sg->length - sg_offset, count);

		if (copy_from_user(sg_virt(sg) + sg_offset, data + offset, count_now)) {
			printk(KERN_ERR "%s: copy_from_user() failed\n", __func__);
			rc = -EFAULT;
			goto out;
		}

		count -= count_now;
		offset += count_now;
		sg++;
		sg_offset = 0;
	}

	*off += offset;
	rc = offset;

out:
	return rc;
}

static loff_t membuf_llseek(struct file *file, loff_t offset, int origin)
{
	struct membuf *buf = (struct membuf *)file->private_data;
	loff_t new_offset;
	int rc;

	switch (origin) {
	case 0: /* SEEK_SET */
		new_offset = offset;
		break;

	case 1: /* SEEK_CUR */
		new_offset = file->f_pos + offset;
		break;

	case 2: /* SEEK_END */
		new_offset = buf->size + offset;
		break;

	default:
		rc = -EINVAL;
		goto err;
	}

	if (new_offset > buf->real_size) {
		rc = membuf_expand(buf, new_offset - buf->real_size);
		if (rc < 0) {
			printk(KERN_ERR "%s: membuf_expand() failed\n", __func__);
			goto err;
		}
	}

	file->f_pos = new_offset;

	return new_offset;

err:
	return rc;
}


static long membuf_ioctl(struct file *file,
                unsigned int cmd, unsigned long arg)
{
	struct membuf *buf = (struct membuf *)file->private_data;
	struct usr_scatter_list usr_sg;
	struct scatterlist *sg;
	int i;
	int rc = 0;

	switch (cmd) {
	case MEMBUF_GET_SCATTER:

		if (!buf->mapped)
		{
			rc = membuf_map(buf);
			if (rc < 0)
				break;
		}

		membuf_sync(buf);

		usr_sg.entries = 0;
		for (i = 0, sg = buf->sg; i < buf->sg_nr; i++, sg++) {
			usr_sg.addr[usr_sg.entries] = sg_dma_address(sg);
			usr_sg.pg_order[usr_sg.entries] = get_order(sg_dma_len(sg));

/*
			printk(KERN_INFO "%d %x %x %x %x %d\n", i, sg_virt(sg), sg_phys(sg),
								 sg_dma_address(sg), usr_sg.pg_order[usr_sg.entries],
								 sg->length);
*/
			usr_sg.entries++;
		}

		if (copy_to_user((void __user *)arg, &usr_sg, sizeof(struct usr_scatter_list))) {
			printk(KERN_ERR "%s: copy_to_user() failed\n", __func__);
			rc = -EFAULT;
		}

		break;

	default:
		printk(KERN_ERR "%s: unknown ioctl(%d)\n", __func__, cmd);
		rc = -EINVAL;
		break;
	}

	return rc;
}



static int membuf_open(struct inode *in, struct file *file)
{
	struct membuf *buf;

	buf = kzalloc(sizeof(struct membuf), GFP_KERNEL);
	if (!buf) {
		printk(KERN_ERR "%s: kmalloc() failed\n", __func__);
		return -ENOMEM;
	}

	buf->pg_order = MEMBUF_DEFAULT_PG_ORDER;

	file->private_data = buf;

	return 0;
}


static int membuf_release(struct inode *in, struct file *file)
{
	struct membuf *buf = (struct membuf *)file->private_data;

	membuf_unmap(buf);
	membuf_free(buf);

	kfree(buf);

	return 0;
}


static const struct file_operations membuf_fops = {
	.owner = THIS_MODULE,
	.write = membuf_write,
	.llseek = membuf_llseek,
	.unlocked_ioctl = membuf_ioctl,
	.open = membuf_open,
	.release = membuf_release,
};

static int __init membuf_init(void)
{
	int rc = 0;

	rc = alloc_chrdev_region(&membuf_dev.devno, MEMBUF_MINOR, MEMBUF_MINOR_COUNT, MEMBUF_DRV_NAME);
	if (rc < 0) {
		printk(KERN_ERR "%s: alloc_chrdev_region() failed\n", __func__);
		goto err0;
	}

	cdev_init(&membuf_dev.dev, &membuf_fops);
	membuf_dev.dev.owner = THIS_MODULE;

	rc = cdev_add (&membuf_dev.dev, membuf_dev.devno, MEMBUF_DEV_COUNT);
	if (rc < 0) {
		printk(KERN_ERR "%s: cdev_add() failed\n", __func__);
		goto err1;
	}

	printk(KERN_INFO "%s: created membuf device(%d, %d)\n", __func__, MAJOR(membuf_dev.devno), MINOR(membuf_dev.devno));

	return 0;

err1:
	unregister_chrdev_region(membuf_dev.devno, MEMBUF_DEV_COUNT);

err0:
	return rc;
}

static void __exit membuf_exit(void)
{
	cdev_del(&membuf_dev.dev);

	unregister_chrdev_region(membuf_dev.devno, MEMBUF_DEV_COUNT);
}


module_init(membuf_init);
module_exit(membuf_exit);
