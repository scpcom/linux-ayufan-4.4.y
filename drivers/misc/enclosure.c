#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/device.h>
#include <linux/enclosure.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#ifdef MY_DEF_HERE
#include <scsi/scsi_device.h>
#endif  

static LIST_HEAD(container_list);
static DEFINE_MUTEX(container_list_lock);
static struct class enclosure_class;

#ifdef MY_DEF_HERE
 
static void enclosure_remove_links(struct enclosure_component *cdev);
#endif  

struct enclosure_device *enclosure_find(struct device *dev,
					struct enclosure_device *start)
{
	struct enclosure_device *edev;

	mutex_lock(&container_list_lock);
	edev = list_prepare_entry(start, &container_list, node);
	if (start)
		put_device(&start->edev);

	list_for_each_entry_continue(edev, &container_list, node) {
		struct device *parent = edev->edev.parent;
		 
		while (parent) {
			if (parent == dev) {
				get_device(&edev->edev);
				mutex_unlock(&container_list_lock);
				return edev;
			}
			parent = parent->parent;
		}
	}
	mutex_unlock(&container_list_lock);

	return NULL;
}
EXPORT_SYMBOL_GPL(enclosure_find);

int enclosure_for_each_device(int (*fn)(struct enclosure_device *, void *),
			      void *data)
{
	int error = 0;
	struct enclosure_device *edev;

	mutex_lock(&container_list_lock);
	list_for_each_entry(edev, &container_list, node) {
		error = fn(edev, data);
		if (error)
			break;
	}
	mutex_unlock(&container_list_lock);

	return error;
}
EXPORT_SYMBOL_GPL(enclosure_for_each_device);

struct enclosure_device *
enclosure_register(struct device *dev, const char *name, int components,
		   struct enclosure_component_callbacks *cb)
{
	struct enclosure_device *edev =
		kzalloc(sizeof(struct enclosure_device) +
			sizeof(struct enclosure_component)*components,
			GFP_KERNEL);
	int err, i;

	BUG_ON(!cb);

	if (!edev)
		return ERR_PTR(-ENOMEM);

	edev->components = components;

	edev->edev.class = &enclosure_class;
	edev->edev.parent = get_device(dev);
	edev->cb = cb;
	dev_set_name(&edev->edev, "%s", name);
	err = device_register(&edev->edev);
	if (err)
		goto err;

	for (i = 0; i < components; i++)
		edev->component[i].number = -1;

	mutex_lock(&container_list_lock);
	list_add_tail(&edev->node, &container_list);
	mutex_unlock(&container_list_lock);

	return edev;

 err:
	put_device(edev->edev.parent);
	kfree(edev);
	return ERR_PTR(err);
}
EXPORT_SYMBOL_GPL(enclosure_register);

static struct enclosure_component_callbacks enclosure_null_callbacks;

void enclosure_unregister(struct enclosure_device *edev)
{
	int i;
#ifdef MY_DEF_HERE
	struct enclosure_component *cdev = NULL;
#endif  
#ifdef MY_DEF_HERE
	struct scsi_device *scsi_dev;
	struct scsi_device *scsi_enc;
#endif  

	mutex_lock(&container_list_lock);
	list_del(&edev->node);
	mutex_unlock(&container_list_lock);

#ifdef MY_DEF_HERE
	for (i = 0; i < edev->components; i++) {
		if (edev->component[i].number != -1) {
			 
			cdev = &edev->component[i];
			if (cdev->dev != NULL) {
#ifdef MY_DEF_HERE
				if (ENCLOSURE_COMPONENT_ARRAY_DEVICE == cdev->type) {
					scsi_dev = to_scsi_device(cdev->dev);
					scsi_enc = to_scsi_device(edev->edev.parent);
#ifdef MY_ABC_HERE
					printk(KERN_INFO "SCSI device (%s) with disk name (%s) removed from SLOT%02d of enclosure(%s), %.8s-%."CONFIG_SYNO_DISK_MODEL_LEN"s",
							dev_name(cdev->dev), scsi_dev->syno_disk_name, cdev->number + 1, dev_name(&(edev->edev)),
							scsi_enc->vendor, scsi_enc->model);
#else  
					printk(KERN_INFO "SCSI device (%s) with disk name (%s) removed from SLOT%02d of enclosure(%s), %.8s-%.16s",
							dev_name(cdev->dev), scsi_dev->syno_disk_name, cdev->number + 1, dev_name(&(edev->edev)),
							scsi_enc->vendor, scsi_enc->model);
#endif  
				}
#endif  
				enclosure_remove_links(cdev);
				put_device(cdev->dev);
				cdev->dev = NULL;
			}
			 
			device_unregister(&edev->component[i].cdev);
		}
	}
#else  
	for (i = 0; i < edev->components; i++)
		if (edev->component[i].number != -1)
			device_unregister(&edev->component[i].cdev);
#endif  

	edev->cb = &enclosure_null_callbacks;
	device_unregister(&edev->edev);
}
EXPORT_SYMBOL_GPL(enclosure_unregister);

#define ENCLOSURE_NAME_SIZE	64

static void enclosure_link_name(struct enclosure_component *cdev, char *name)
{
	strcpy(name, "enclosure_device:");
	strcat(name, dev_name(&cdev->cdev));
}

static void enclosure_remove_links(struct enclosure_component *cdev)
{
	char name[ENCLOSURE_NAME_SIZE];

	if (!cdev->dev->kobj.sd)
		return;

	enclosure_link_name(cdev, name);
	sysfs_remove_link(&cdev->dev->kobj, name);
	sysfs_remove_link(&cdev->cdev.kobj, "device");
}

static int enclosure_add_links(struct enclosure_component *cdev)
{
	int error;
	char name[ENCLOSURE_NAME_SIZE];

	error = sysfs_create_link(&cdev->cdev.kobj, &cdev->dev->kobj, "device");
	if (error)
		return error;

	enclosure_link_name(cdev, name);
	error = sysfs_create_link(&cdev->dev->kobj, &cdev->cdev.kobj, name);
	if (error)
		sysfs_remove_link(&cdev->cdev.kobj, "device");

	return error;
}

static void enclosure_release(struct device *cdev)
{
	struct enclosure_device *edev = to_enclosure_device(cdev);

	put_device(cdev->parent);
	kfree(edev);
}

static void enclosure_component_release(struct device *dev)
{
#ifdef MY_DEF_HERE
	 
#else  
	struct enclosure_component *cdev = to_enclosure_component(dev);

	if (cdev->dev) {
		enclosure_remove_links(cdev);
		put_device(cdev->dev);
	}
#endif  

	put_device(dev->parent);
}

static const struct attribute_group *enclosure_groups[];

struct enclosure_component *
enclosure_component_register(struct enclosure_device *edev,
			     unsigned int number,
			     enum enclosure_component_type type,
			     const char *name)
{
	struct enclosure_component *ecomp;
	struct device *cdev;
	int err;

	if (number >= edev->components)
		return ERR_PTR(-EINVAL);

	ecomp = &edev->component[number];

	if (ecomp->number != -1)
		return ERR_PTR(-EINVAL);

	ecomp->type = type;
	ecomp->number = number;
	cdev = &ecomp->cdev;
	cdev->parent = get_device(&edev->edev);
	if (name && name[0])
		dev_set_name(cdev, "%s", name);
	else
		dev_set_name(cdev, "%u", number);

	cdev->release = enclosure_component_release;
	cdev->groups = enclosure_groups;

	err = device_register(cdev);
	if (err) {
		ecomp->number = -1;
		put_device(cdev);
		return ERR_PTR(err);
	}

#ifdef MY_DEF_HERE
	 
	if (ENCLOSURE_COMPONENT_ARRAY_DEVICE == ecomp->type && edev->cb->set_locate) {
		edev->cb->set_locate(edev, ecomp, 0);
	}
#endif  

	return ecomp;
}
EXPORT_SYMBOL_GPL(enclosure_component_register);

int enclosure_add_device(struct enclosure_device *edev, int component,
			 struct device *dev)
{
	struct enclosure_component *cdev;
#ifdef MY_DEF_HERE
	struct scsi_device *scsi_dev;
	struct scsi_device *scsi_enc;
#endif  

	if (!edev || component >= edev->components)
		return -EINVAL;

	cdev = &edev->component[component];

	if (cdev->dev == dev)
		return -EEXIST;

	if (cdev->dev)
		enclosure_remove_links(cdev);

	put_device(cdev->dev);
	cdev->dev = get_device(dev);

#ifdef MY_DEF_HERE
	 
	if (ENCLOSURE_COMPONENT_ARRAY_DEVICE == cdev->type && edev->cb->set_locate) {
		edev->cb->set_locate(edev, cdev, 1);
	}
#endif  
#ifdef MY_DEF_HERE
	if (ENCLOSURE_COMPONENT_ARRAY_DEVICE == cdev->type) {
		scsi_dev = to_scsi_device(dev);
		scsi_enc = to_scsi_device(edev->edev.parent);
#ifdef MY_ABC_HERE
		printk(KERN_INFO "SCSI device (%s) with disk name (%s) plugged in SLOT%02d of enclosure(%s), %.8s-%."CONFIG_SYNO_DISK_MODEL_LEN"s",
				dev_name(dev), scsi_dev->syno_disk_name, cdev->number + 1, dev_name(&(edev->edev)),
				scsi_enc->vendor, scsi_enc->model);
#else  
		printk(KERN_INFO "SCSI device (%s) with disk name (%s) plugged in SLOT%02d of enclosure(%s), %.8s-%.16s",
				dev_name(dev), scsi_dev->syno_disk_name, cdev->number + 1, dev_name(&(edev->edev)),
				scsi_enc->vendor, scsi_enc->model);
#endif  
	}
#endif  

	return enclosure_add_links(cdev);
}
EXPORT_SYMBOL_GPL(enclosure_add_device);

int enclosure_remove_device(struct enclosure_device *edev, struct device *dev)
{
	struct enclosure_component *cdev;
#ifdef MY_DEF_HERE
	struct scsi_device *scsi_dev;
	struct scsi_device *scsi_enc;
#endif  
	int i;

	if (!edev || !dev)
		return -EINVAL;

	for (i = 0; i < edev->components; i++) {
		cdev = &edev->component[i];
		if (cdev->dev == dev) {
#ifdef MY_DEF_HERE
			 
			if (ENCLOSURE_COMPONENT_ARRAY_DEVICE == cdev->type && edev->cb->set_locate) {
				edev->cb->set_locate(edev, cdev, 0);
			}
#endif  
#ifdef MY_DEF_HERE
			if (ENCLOSURE_COMPONENT_ARRAY_DEVICE == cdev->type) {
				scsi_dev = to_scsi_device(dev);
				scsi_enc = to_scsi_device(edev->edev.parent);
#ifdef MY_ABC_HERE
				printk(KERN_INFO "SCSI device (%s) with disk name (%s) removed from SLOT%02d of enclosure(%s), %.8s-%."CONFIG_SYNO_DISK_MODEL_LEN"s",
						dev_name(dev), scsi_dev->syno_disk_name, cdev->number + 1, dev_name(&(edev->edev)),
						scsi_enc->vendor, scsi_enc->model);
#else  
				printk(KERN_INFO "SCSI device (%s) with disk name (%s) removed from SLOT%02d of enclosure(%s), %.8s-%.16s",
						dev_name(dev), scsi_dev->syno_disk_name, cdev->number + 1, dev_name(&(edev->edev)),
						scsi_enc->vendor, scsi_enc->model);
#endif  
			}
#endif  
			enclosure_remove_links(cdev);
			device_del(&cdev->cdev);
			put_device(dev);
			cdev->dev = NULL;
			return device_add(&cdev->cdev);
		}
	}
	return -ENODEV;
}
EXPORT_SYMBOL_GPL(enclosure_remove_device);

static ssize_t enclosure_show_components(struct device *cdev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct enclosure_device *edev = to_enclosure_device(cdev);

	return snprintf(buf, 40, "%d\n", edev->components);
}

static struct device_attribute enclosure_attrs[] = {
	__ATTR(components, S_IRUGO, enclosure_show_components, NULL),
	__ATTR_NULL
};

static struct class enclosure_class = {
	.name			= "enclosure",
	.owner			= THIS_MODULE,
	.dev_release		= enclosure_release,
	.dev_attrs		= enclosure_attrs,
};

static const char *const enclosure_status [] = {
	[ENCLOSURE_STATUS_UNSUPPORTED] = "unsupported",
	[ENCLOSURE_STATUS_OK] = "OK",
	[ENCLOSURE_STATUS_CRITICAL] = "critical",
	[ENCLOSURE_STATUS_NON_CRITICAL] = "non-critical",
	[ENCLOSURE_STATUS_UNRECOVERABLE] = "unrecoverable",
	[ENCLOSURE_STATUS_NOT_INSTALLED] = "not installed",
	[ENCLOSURE_STATUS_UNKNOWN] = "unknown",
	[ENCLOSURE_STATUS_UNAVAILABLE] = "unavailable",
	[ENCLOSURE_STATUS_MAX] = NULL,
};

static const char *const enclosure_type [] = {
	[ENCLOSURE_COMPONENT_DEVICE] = "device",
	[ENCLOSURE_COMPONENT_ARRAY_DEVICE] = "array device",
};

static ssize_t get_component_fault(struct device *cdev,
				   struct device_attribute *attr, char *buf)
{
	struct enclosure_device *edev = to_enclosure_device(cdev->parent);
	struct enclosure_component *ecomp = to_enclosure_component(cdev);

	if (edev->cb->get_fault)
		edev->cb->get_fault(edev, ecomp);
	return snprintf(buf, 40, "%d\n", ecomp->fault);
}

static ssize_t set_component_fault(struct device *cdev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct enclosure_device *edev = to_enclosure_device(cdev->parent);
	struct enclosure_component *ecomp = to_enclosure_component(cdev);
	int val = simple_strtoul(buf, NULL, 0);

	if (edev->cb->set_fault)
		edev->cb->set_fault(edev, ecomp, val);
	return count;
}

static ssize_t get_component_status(struct device *cdev,
				    struct device_attribute *attr,char *buf)
{
	struct enclosure_device *edev = to_enclosure_device(cdev->parent);
	struct enclosure_component *ecomp = to_enclosure_component(cdev);

	if (edev->cb->get_status)
		edev->cb->get_status(edev, ecomp);
	return snprintf(buf, 40, "%s\n", enclosure_status[ecomp->status]);
}

static ssize_t set_component_status(struct device *cdev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct enclosure_device *edev = to_enclosure_device(cdev->parent);
	struct enclosure_component *ecomp = to_enclosure_component(cdev);
	int i;

	for (i = 0; enclosure_status[i]; i++) {
		if (strncmp(buf, enclosure_status[i],
			    strlen(enclosure_status[i])) == 0 &&
		    (buf[strlen(enclosure_status[i])] == '\n' ||
		     buf[strlen(enclosure_status[i])] == '\0'))
			break;
	}

	if (enclosure_status[i] && edev->cb->set_status) {
		edev->cb->set_status(edev, ecomp, i);
		return count;
	} else
		return -EINVAL;
}

static ssize_t get_component_active(struct device *cdev,
				    struct device_attribute *attr, char *buf)
{
	struct enclosure_device *edev = to_enclosure_device(cdev->parent);
	struct enclosure_component *ecomp = to_enclosure_component(cdev);

	if (edev->cb->get_active)
		edev->cb->get_active(edev, ecomp);
	return snprintf(buf, 40, "%d\n", ecomp->active);
}

static ssize_t set_component_active(struct device *cdev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct enclosure_device *edev = to_enclosure_device(cdev->parent);
	struct enclosure_component *ecomp = to_enclosure_component(cdev);
	int val = simple_strtoul(buf, NULL, 0);

	if (edev->cb->set_active)
		edev->cb->set_active(edev, ecomp, val);
	return count;
}

static ssize_t get_component_locate(struct device *cdev,
				    struct device_attribute *attr, char *buf)
{
	struct enclosure_device *edev = to_enclosure_device(cdev->parent);
	struct enclosure_component *ecomp = to_enclosure_component(cdev);

	if (edev->cb->get_locate)
		edev->cb->get_locate(edev, ecomp);
	return snprintf(buf, 40, "%d\n", ecomp->locate);
}

static ssize_t set_component_locate(struct device *cdev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct enclosure_device *edev = to_enclosure_device(cdev->parent);
	struct enclosure_component *ecomp = to_enclosure_component(cdev);
	int val = simple_strtoul(buf, NULL, 0);

	if (edev->cb->set_locate)
		edev->cb->set_locate(edev, ecomp, val);
	return count;
}

static ssize_t get_component_type(struct device *cdev,
				  struct device_attribute *attr, char *buf)
{
	struct enclosure_component *ecomp = to_enclosure_component(cdev);

	return snprintf(buf, 40, "%s\n", enclosure_type[ecomp->type]);
}

static DEVICE_ATTR(fault, S_IRUGO | S_IWUSR, get_component_fault,
		    set_component_fault);
static DEVICE_ATTR(status, S_IRUGO | S_IWUSR, get_component_status,
		   set_component_status);
static DEVICE_ATTR(active, S_IRUGO | S_IWUSR, get_component_active,
		   set_component_active);
static DEVICE_ATTR(locate, S_IRUGO | S_IWUSR, get_component_locate,
		   set_component_locate);
static DEVICE_ATTR(type, S_IRUGO, get_component_type, NULL);

static struct attribute *enclosure_component_attrs[] = {
	&dev_attr_fault.attr,
	&dev_attr_status.attr,
	&dev_attr_active.attr,
	&dev_attr_locate.attr,
	&dev_attr_type.attr,
	NULL
};

static struct attribute_group enclosure_group = {
	.attrs = enclosure_component_attrs,
};

static const struct attribute_group *enclosure_groups[] = {
	&enclosure_group,
	NULL
};

static int __init enclosure_init(void)
{
	int err;

	err = class_register(&enclosure_class);
	if (err)
		return err;

	return 0;
}

static void __exit enclosure_exit(void)
{
	class_unregister(&enclosure_class);
}

module_init(enclosure_init);
module_exit(enclosure_exit);

MODULE_AUTHOR("James Bottomley");
MODULE_DESCRIPTION("Enclosure Services");
MODULE_LICENSE("GPL v2");
