/* l2fw_sysfs.c */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/capability.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>

#include "mvTypes.h"
#include "mv_eth_l2fw.h"
#include "linux/inet.h"

#ifdef CONFIG_MV_ETH_L2SEC
extern int l2fw_set_cesa_chan(int port, int cesaChan);
#endif

static ssize_t l2fw_help(char *buf)
{
	int off = 0;
	off += sprintf(buf+off, "help\n");
	off += sprintf(buf+off, "echo mode rxp txp > l2fw - set l2f <rxp>->");
	off += sprintf(buf+off, "<txp><mode> 0-dis,1-as_is,2-swap,3-copy\n");
	off += sprintf(buf+off, "echo threshold > l2fw_xor: set threshold\n");
#ifdef CONFIG_MV_ETH_L2SEC
	off += sprintf(buf+off, "echo 1 > esp   - enable ESP\n");
#endif
	off += sprintf(buf+off, "cat dump - display L2fw rules DB\n");
	off += sprintf(buf+off, "echo 1 > flush - flush L2fw rules DB\n");
	return off;
}

static ssize_t l2fw_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
    const char	*name = attr->attr.name;
    int             off = 0;

    if (!capable(CAP_NET_ADMIN))
	return -EPERM;

	if (!strcmp(name, "help")) {
	    off = l2fw_help(buf);
		return off;
	}
	if (!strcmp(name, "dump")) {
		l2fw_dump();
		return off;
	}
	if (!strcmp(name, "numHashEntries")) {
		l2fw_show_numHashEntries();
		return off;
	}
#ifdef CONFIG_MV_ETH_L2SEC
	if (!strcmp(name, "esp")) {
		l2fw_esp_show();
		return off;
	}
#endif
	if (!strcmp(name, "help")) {
	    off = l2fw_help(buf);
		return off;
	}

#ifdef CONFIG_MV_ETH_L2SEC
	if (!strcmp(name, "stats")) {
	    l2fw_stats();
		return off;
	}
#endif

	return off;
}

static ssize_t l2fw_hex_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    addr1, addr2;
	int port;
	unsigned long   flags;
#ifdef CONFIG_MV_ETH_L2SEC
	int             enableEsp;
#endif
	if (!capable(CAP_NET_ADMIN))
		return -EPERM;
	err = addr1 = addr2 = port = 0;

	local_irq_save(flags);
	if (!strcmp(name, "l2fw_add")) {
		sscanf(buf, "%x %x %d", &addr1, &addr2, &port);
		l2fw_add(addr1, addr2, port);
	} else if (!strcmp(name, "l2fw_add_ip")) {
		l2fw_add_ip(buf);
#ifdef CONFIG_MV_ETH_L2SEC
	} else if (!strcmp(name, "esp")) {
		sscanf(buf, "%d", &enableEsp);
		l2fw_esp_set(enableEsp);
#endif
	} else if (!strcmp(name, "flush")) {
		l2fw_flush();
	}

	local_irq_restore(flags);

	return err ? -EINVAL : len;
}

static ssize_t l2fw_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char	*name = attr->attr.name;
	int             err;

	unsigned int    p, txp, txq, v;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	err = p = txp = txq = v = 0;
	sscanf(buf, "%d %d %d %d", &p, &txp, &txq, &v);

	local_irq_save(flags);

	if (!strcmp(name, "l2fw_xor"))
		l2fw_xor(p);

	else if (!strcmp(name, "l2fw"))
		l2fw(p, txp, txq);
#ifdef CONFIG_MV_ETH_L2SEC
	else if (!strcmp(name, "cesa_chan"))
		err = l2fw_set_cesa_chan(p, txp);
#endif
	local_irq_restore(flags);

	if (err)
		mvOsPrintf("%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;

}


static DEVICE_ATTR(l2fw,		S_IWUSR, l2fw_show, l2fw_store);
static DEVICE_ATTR(l2fw_xor,		S_IWUSR, l2fw_show, l2fw_store);
static DEVICE_ATTR(l2fw_add,		S_IWUSR, l2fw_show, l2fw_hex_store);
static DEVICE_ATTR(l2fw_add_ip,		S_IWUSR, l2fw_show, l2fw_hex_store);
static DEVICE_ATTR(help,		S_IRUSR, l2fw_show,  NULL);
static DEVICE_ATTR(dump,			S_IRUSR, l2fw_show,  NULL);
static DEVICE_ATTR(numHashEntries,	S_IRUSR, l2fw_show,  NULL);
#ifdef CONFIG_MV_ETH_L2SEC
static DEVICE_ATTR(stats,			S_IRUSR, l2fw_show, NULL);
static DEVICE_ATTR(esp,				S_IWUSR, l2fw_show,  l2fw_hex_store);
static DEVICE_ATTR(cesa_chan,		S_IWUSR, NULL,  l2fw_store);
#endif
static DEVICE_ATTR(flush,			S_IWUSR, NULL,  	 l2fw_hex_store);


static struct attribute *l2fw_attrs[] = {
	&dev_attr_l2fw.attr,
	&dev_attr_l2fw_xor.attr,
	&dev_attr_l2fw_add.attr,
	&dev_attr_l2fw_add_ip.attr,
	&dev_attr_help.attr,
	&dev_attr_dump.attr,
	&dev_attr_flush.attr,
#ifdef CONFIG_MV_ETH_L2SEC
	&dev_attr_esp.attr,
	&dev_attr_stats.attr,
	&dev_attr_cesa_chan.attr,
#endif
	&dev_attr_numHashEntries.attr,
	NULL
};

static struct attribute_group l2fw_group = {
	.name = "l2fw",
	.attrs = l2fw_attrs,
};

#ifdef CONFIG_MV_ETH_L2FW
int __devinit mv_l2fw_sysfs_init(void)
{
	int err;
	struct device *pd;

	pd = bus_find_device_by_name(&platform_bus_type, NULL, "neta");
	if (!pd) {
		platform_device_register_simple("neta", -1, NULL, 0);
		pd = bus_find_device_by_name(&platform_bus_type, NULL, "neta");
	}

	if (!pd) {
		printk(KERN_ERR "%s: cannot find neta device\n", __func__);
		pd = &platform_bus;
	}

	err = sysfs_create_group(&pd->kobj, &l2fw_group);
	if (err) {
		printk(KERN_ERR "sysfs group failed %d\n", err);
		goto out;
	}
out:
	return err;
}
#endif

module_init(mv_l2fw_sysfs_init);

MODULE_AUTHOR("Rami Rosen");
MODULE_DESCRIPTION("sysfs for marvell l2fw");
MODULE_LICENSE("GPL");
