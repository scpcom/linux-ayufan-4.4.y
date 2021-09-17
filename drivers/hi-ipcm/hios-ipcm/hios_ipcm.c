#include <linux/module.h>
#include <linux/slab.h>
#include <linux/hardirq.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/hi_ipcm_usrdev.h>
#include <linux/hisi_ipcm_proto.h>

void hios_ipcm_handle_attr_init(struct hi_ipcm_handle_attr *attr)
{
	int i;

	/*ipcm host and slave dev*/
	attr->target_id = HI_IPCM_TARGET_ID_IPCM_START + 2;
	attr->port = HI_IPCM_PORT_NR + 1;
	attr->priority = 1;
	for (i = 0; i < (HISI_MAX_MAP_DEV - 1); i++)
		attr->remote_id[i] = -1;
}
EXPORT_SYMBOL(hios_ipcm_handle_attr_init);

int hios_ipcm_close(hios_ipcm_handle_t *handle)
{
	if (handle && (handle->ipcm_handle)) {
		ipcm_vdd_close((void *)handle->ipcm_handle);
		kfree(handle);
		return 0;
	}

	return -1;
}
EXPORT_SYMBOL(hios_ipcm_close);

hios_ipcm_handle_t *hios_ipcm_open(struct hi_ipcm_handle_attr *attr)
{
	hios_ipcm_handle_t *handle = NULL;
	unsigned long data;

	if (attr == NULL) {
		hi_ipcm_trace(2, "open attr is null %d\n", 0);
		return NULL;
	}

	if (in_interrupt())
		handle = kmalloc(sizeof(hios_ipcm_handle_t), GFP_ATOMIC);
	else
		handle = kmalloc(sizeof(hios_ipcm_handle_t), GFP_KERNEL);

	if (handle == NULL) {
		hi_ipcm_trace(2, "kmalloc error %d\n", 0);
		return NULL;
	}

	data = (unsigned long)ipcm_vdd_open(attr->target_id,
			attr->port, attr->priority);
	if (data)
		handle->ipcm_handle = data;
	else {
		kfree(handle);
		hi_ipcm_trace(2, "ipcm_vdd_open error%d\n", 0);
		return NULL;
	}

	return handle;
}
EXPORT_SYMBOL(hios_ipcm_open);

int hios_ipcm_sendto(hios_ipcm_handle_t *handle,
			const void *buf,
			unsigned int len)
{
	if (handle && (handle->ipcm_handle))
		return ipcm_vdd_sendto((void *)handle->ipcm_handle,
				buf , len, HISI_IPCM_NORMAL_DATA);
	return -1;
}
EXPORT_SYMBOL(hios_ipcm_sendto);

int hios_ipcm_getopt(hios_ipcm_handle_t *handle, hios_ipcm_handle_opt_t *opt)
{
	ipcm_vdd_opt_t ipcm_vdd_opt;

	if (handle && (handle->ipcm_handle)) {
		ipcm_vdd_getopt((void *)handle->ipcm_handle, &ipcm_vdd_opt);
		opt->recvfrom_notify = ipcm_vdd_opt.recvfrom_notify;
		opt->data = ipcm_vdd_opt.data;
	}

	return 0;
}
EXPORT_SYMBOL(hios_ipcm_getopt);

int hios_ipcm_setopt(hios_ipcm_handle_t *handle,
			const hios_ipcm_handle_opt_t *opt)
{
	ipcm_vdd_opt_t ipcm_vdd_opt;

	if (opt) {
		ipcm_vdd_opt.recvfrom_notify = opt->recvfrom_notify;
		ipcm_vdd_opt.data = opt->data;
	}

	if (handle && (handle->ipcm_handle))
		return ipcm_vdd_setopt((void *)handle->ipcm_handle,
				&ipcm_vdd_opt);
	return -1;
}
EXPORT_SYMBOL(hios_ipcm_setopt);

int hios_ipcm_getlocalid(hios_ipcm_handle_t *handle)
{
	if (handle && (handle->ipcm_handle))
		return ipcm_vdd_localid();
	return 0;
}
EXPORT_SYMBOL(hios_ipcm_getlocalid);

int hios_ipcm_getremoteids(int ids[],
				hios_ipcm_handle_t *handle)
{
	if (handle && (handle->ipcm_handle))
		return ipcm_vdd_remoteid(&ids[0],
				(void *)handle->ipcm_handle);
	return -1;
}
EXPORT_SYMBOL(hios_ipcm_getremoteids);

int hios_ipcm_check_remote(int remote_id,
				hios_ipcm_handle_t *handle)
{
	if (handle && (handle->ipcm_handle))
		return ipcm_vdd_check_remote();

	return -1;
}
EXPORT_SYMBOL(hios_ipcm_check_remote);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("chanjinn");
