#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/cache.h>
#include <linux/mutex.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/slab.h>
#include <linux/mod_devicetable.h>
#include <linux/spi/spi.h>
#include <linux/of_gpio.h>
#include <linux/pm_runtime.h>
#include <linux/export.h>
#include <linux/sched/rt.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/ioport.h>
#include <linux/acpi.h>

static void spidev_release(struct device *dev)
{
	struct spi_device	*spi = to_spi_device(dev);

	if (spi->master->cleanup)
		spi->master->cleanup(spi);

	spi_master_put(spi->master);
	kfree(spi);
}

static ssize_t
modalias_show(struct device *dev, struct device_attribute *a, char *buf)
{
	const struct spi_device	*spi = to_spi_device(dev);

	return sprintf(buf, "%s%s\n", SPI_MODULE_PREFIX, spi->modalias);
}

static struct device_attribute spi_dev_attrs[] = {
	__ATTR_RO(modalias),
	__ATTR_NULL,
};

static const struct spi_device_id *spi_match_id(const struct spi_device_id *id,
						const struct spi_device *sdev)
{
	while (id->name[0]) {
		if (!strcmp(sdev->modalias, id->name))
			return id;
		id++;
	}
	return NULL;
}

const struct spi_device_id *spi_get_device_id(const struct spi_device *sdev)
{
	const struct spi_driver *sdrv = to_spi_driver(sdev->dev.driver);

	return spi_match_id(sdrv->id_table, sdev);
}
EXPORT_SYMBOL_GPL(spi_get_device_id);

static int spi_match_device(struct device *dev, struct device_driver *drv)
{
	const struct spi_device	*spi = to_spi_device(dev);
	const struct spi_driver	*sdrv = to_spi_driver(drv);

	if (of_driver_match_device(dev, drv))
		return 1;

	if (acpi_driver_match_device(dev, drv))
		return 1;

	if (sdrv->id_table)
		return !!spi_match_id(sdrv->id_table, spi);

	return strcmp(spi->modalias, drv->name) == 0;
}

static int spi_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	const struct spi_device		*spi = to_spi_device(dev);

	add_uevent_var(env, "MODALIAS=%s%s", SPI_MODULE_PREFIX, spi->modalias);
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int spi_legacy_suspend(struct device *dev, pm_message_t message)
{
	int			value = 0;
	struct spi_driver	*drv = to_spi_driver(dev->driver);

	if (drv) {
		if (drv->suspend)
			value = drv->suspend(to_spi_device(dev), message);
		else
			dev_dbg(dev, "... can't suspend\n");
	}
	return value;
}

static int spi_legacy_resume(struct device *dev)
{
	int			value = 0;
	struct spi_driver	*drv = to_spi_driver(dev->driver);

	if (drv) {
		if (drv->resume)
			value = drv->resume(to_spi_device(dev));
		else
			dev_dbg(dev, "... can't resume\n");
	}
	return value;
}

static int spi_pm_suspend(struct device *dev)
{
	const struct dev_pm_ops *pm = dev->driver ? dev->driver->pm : NULL;

	if (pm)
		return pm_generic_suspend(dev);
	else
		return spi_legacy_suspend(dev, PMSG_SUSPEND);
}

static int spi_pm_resume(struct device *dev)
{
	const struct dev_pm_ops *pm = dev->driver ? dev->driver->pm : NULL;

	if (pm)
		return pm_generic_resume(dev);
	else
		return spi_legacy_resume(dev);
}

static int spi_pm_freeze(struct device *dev)
{
	const struct dev_pm_ops *pm = dev->driver ? dev->driver->pm : NULL;

	if (pm)
		return pm_generic_freeze(dev);
	else
		return spi_legacy_suspend(dev, PMSG_FREEZE);
}

static int spi_pm_thaw(struct device *dev)
{
	const struct dev_pm_ops *pm = dev->driver ? dev->driver->pm : NULL;

	if (pm)
		return pm_generic_thaw(dev);
	else
		return spi_legacy_resume(dev);
}

static int spi_pm_poweroff(struct device *dev)
{
	const struct dev_pm_ops *pm = dev->driver ? dev->driver->pm : NULL;

	if (pm)
		return pm_generic_poweroff(dev);
	else
		return spi_legacy_suspend(dev, PMSG_HIBERNATE);
}

static int spi_pm_restore(struct device *dev)
{
	const struct dev_pm_ops *pm = dev->driver ? dev->driver->pm : NULL;

	if (pm)
		return pm_generic_restore(dev);
	else
		return spi_legacy_resume(dev);
}
#else
#define spi_pm_suspend	NULL
#define spi_pm_resume	NULL
#define spi_pm_freeze	NULL
#define spi_pm_thaw	NULL
#define spi_pm_poweroff	NULL
#define spi_pm_restore	NULL
#endif

static const struct dev_pm_ops spi_pm = {
	.suspend = spi_pm_suspend,
	.resume = spi_pm_resume,
	.freeze = spi_pm_freeze,
	.thaw = spi_pm_thaw,
	.poweroff = spi_pm_poweroff,
	.restore = spi_pm_restore,
	SET_RUNTIME_PM_OPS(
		pm_generic_runtime_suspend,
		pm_generic_runtime_resume,
		pm_generic_runtime_idle
	)
};

struct bus_type spi_bus_type = {
	.name		= "spi",
	.dev_attrs	= spi_dev_attrs,
	.match		= spi_match_device,
	.uevent		= spi_uevent,
	.pm		= &spi_pm,
};
EXPORT_SYMBOL_GPL(spi_bus_type);

static int spi_drv_probe(struct device *dev)
{
	const struct spi_driver		*sdrv = to_spi_driver(dev->driver);

	return sdrv->probe(to_spi_device(dev));
}

static int spi_drv_remove(struct device *dev)
{
	const struct spi_driver		*sdrv = to_spi_driver(dev->driver);

	return sdrv->remove(to_spi_device(dev));
}

static void spi_drv_shutdown(struct device *dev)
{
	const struct spi_driver		*sdrv = to_spi_driver(dev->driver);

	sdrv->shutdown(to_spi_device(dev));
}

int spi_register_driver(struct spi_driver *sdrv)
{
	sdrv->driver.bus = &spi_bus_type;
	if (sdrv->probe)
		sdrv->driver.probe = spi_drv_probe;
	if (sdrv->remove)
		sdrv->driver.remove = spi_drv_remove;
	if (sdrv->shutdown)
		sdrv->driver.shutdown = spi_drv_shutdown;
	return driver_register(&sdrv->driver);
}
EXPORT_SYMBOL_GPL(spi_register_driver);

struct boardinfo {
	struct list_head	list;
	struct spi_board_info	board_info;
};

static LIST_HEAD(board_list);
static LIST_HEAD(spi_master_list);

static DEFINE_MUTEX(board_lock);

struct spi_device *spi_alloc_device(struct spi_master *master)
{
	struct spi_device	*spi;
	struct device		*dev = master->dev.parent;

	if (!spi_master_get(master))
		return NULL;

	spi = kzalloc(sizeof *spi, GFP_KERNEL);
	if (!spi) {
		dev_err(dev, "cannot alloc spi_device\n");
		spi_master_put(master);
		return NULL;
	}

	spi->master = master;
	spi->dev.parent = &master->dev;
	spi->dev.bus = &spi_bus_type;
	spi->dev.release = spidev_release;
	spi->cs_gpio = -ENOENT;
	device_initialize(&spi->dev);
	return spi;
}
EXPORT_SYMBOL_GPL(spi_alloc_device);

int spi_add_device(struct spi_device *spi)
{
	static DEFINE_MUTEX(spi_add_lock);
	struct spi_master *master = spi->master;
	struct device *dev = master->dev.parent;
	struct device *d;
	int status;

	if (spi->chip_select >= master->num_chipselect) {
		dev_err(dev, "cs%d >= max %d\n",
			spi->chip_select,
			master->num_chipselect);
		return -EINVAL;
	}

	dev_set_name(&spi->dev, "%s.%u", dev_name(&spi->master->dev),
			spi->chip_select);

	mutex_lock(&spi_add_lock);

	d = bus_find_device_by_name(&spi_bus_type, NULL, dev_name(&spi->dev));
	if (d != NULL) {
		dev_err(dev, "chipselect %d already in use\n",
				spi->chip_select);
		put_device(d);
		status = -EBUSY;
		goto done;
	}

	if (master->cs_gpios)
		spi->cs_gpio = master->cs_gpios[spi->chip_select];

	status = spi_setup(spi);
	if (status < 0) {
		dev_err(dev, "can't setup %s, status %d\n",
				dev_name(&spi->dev), status);
		goto done;
	}

	status = device_add(&spi->dev);
	if (status < 0)
		dev_err(dev, "can't add %s, status %d\n",
				dev_name(&spi->dev), status);
	else
		dev_dbg(dev, "registered child %s\n", dev_name(&spi->dev));

done:
	mutex_unlock(&spi_add_lock);
	return status;
}
EXPORT_SYMBOL_GPL(spi_add_device);

struct spi_device *spi_new_device(struct spi_master *master,
				  struct spi_board_info *chip)
{
	struct spi_device	*proxy;
	int			status;

	proxy = spi_alloc_device(master);
	if (!proxy)
		return NULL;

	WARN_ON(strlen(chip->modalias) >= sizeof(proxy->modalias));

	proxy->chip_select = chip->chip_select;
	proxy->max_speed_hz = chip->max_speed_hz;
	proxy->mode = chip->mode;
	proxy->irq = chip->irq;
	strlcpy(proxy->modalias, chip->modalias, sizeof(proxy->modalias));
	proxy->dev.platform_data = (void *) chip->platform_data;
	proxy->controller_data = chip->controller_data;
	proxy->controller_state = NULL;

	status = spi_add_device(proxy);
	if (status < 0) {
		spi_dev_put(proxy);
		return NULL;
	}

	return proxy;
}
EXPORT_SYMBOL_GPL(spi_new_device);

static void spi_match_master_to_boardinfo(struct spi_master *master,
				struct spi_board_info *bi)
{
	struct spi_device *dev;

	if (master->bus_num != bi->bus_num)
		return;

	dev = spi_new_device(master, bi);
	if (!dev)
		dev_err(master->dev.parent, "can't create new device for %s\n",
			bi->modalias);
}

int spi_register_board_info(struct spi_board_info const *info, unsigned n)
{
	struct boardinfo *bi;
	int i;

	bi = kzalloc(n * sizeof(*bi), GFP_KERNEL);
	if (!bi)
		return -ENOMEM;

	for (i = 0; i < n; i++, bi++, info++) {
		struct spi_master *master;

		memcpy(&bi->board_info, info, sizeof(*info));
		mutex_lock(&board_lock);
		list_add_tail(&bi->list, &board_list);
		list_for_each_entry(master, &spi_master_list, list)
			spi_match_master_to_boardinfo(master, &bi->board_info);
		mutex_unlock(&board_lock);
	}

	return 0;
}

static void spi_pump_messages(struct kthread_work *work)
{
	struct spi_master *master =
		container_of(work, struct spi_master, pump_messages);
	unsigned long flags;
	bool was_busy = false;
	int ret;

	spin_lock_irqsave(&master->queue_lock, flags);
	if (list_empty(&master->queue) || !master->running) {
		if (!master->busy) {
			spin_unlock_irqrestore(&master->queue_lock, flags);
			return;
		}
		master->busy = false;
		spin_unlock_irqrestore(&master->queue_lock, flags);
		if (master->unprepare_transfer_hardware &&
		    master->unprepare_transfer_hardware(master))
			dev_err(&master->dev,
				"failed to unprepare transfer hardware\n");
		return;
	}

	if (master->cur_msg) {
		spin_unlock_irqrestore(&master->queue_lock, flags);
		return;
	}
	 
	master->cur_msg =
	    list_entry(master->queue.next, struct spi_message, queue);

	list_del_init(&master->cur_msg->queue);
	if (master->busy)
		was_busy = true;
	else
		master->busy = true;
	spin_unlock_irqrestore(&master->queue_lock, flags);

	if (!was_busy && master->prepare_transfer_hardware) {
		ret = master->prepare_transfer_hardware(master);
		if (ret) {
			dev_err(&master->dev,
				"failed to prepare transfer hardware\n");
			return;
		}
	}

	ret = master->transfer_one_message(master, master->cur_msg);
	if (ret) {
		dev_err(&master->dev,
			"failed to transfer one message from queue\n");
		return;
	}
}

static int spi_init_queue(struct spi_master *master)
{
	struct sched_param param = { .sched_priority = MAX_RT_PRIO - 1 };

	INIT_LIST_HEAD(&master->queue);
	spin_lock_init(&master->queue_lock);

	master->running = false;
	master->busy = false;

	init_kthread_worker(&master->kworker);
	master->kworker_task = kthread_run(kthread_worker_fn,
					   &master->kworker,
					   dev_name(&master->dev));
	if (IS_ERR(master->kworker_task)) {
		dev_err(&master->dev, "failed to create message pump task\n");
		return -ENOMEM;
	}
	init_kthread_work(&master->pump_messages, spi_pump_messages);

	if (master->rt) {
		dev_info(&master->dev,
			"will run message pump with realtime priority\n");
		sched_setscheduler(master->kworker_task, SCHED_FIFO, &param);
	}

	return 0;
}

struct spi_message *spi_get_next_queued_message(struct spi_master *master)
{
	struct spi_message *next;
	unsigned long flags;

	spin_lock_irqsave(&master->queue_lock, flags);
	if (list_empty(&master->queue))
		next = NULL;
	else
		next = list_entry(master->queue.next,
				  struct spi_message, queue);
	spin_unlock_irqrestore(&master->queue_lock, flags);

	return next;
}
EXPORT_SYMBOL_GPL(spi_get_next_queued_message);

void spi_finalize_current_message(struct spi_master *master)
{
	struct spi_message *mesg;
	unsigned long flags;

	spin_lock_irqsave(&master->queue_lock, flags);
	mesg = master->cur_msg;
	master->cur_msg = NULL;

	queue_kthread_work(&master->kworker, &master->pump_messages);
	spin_unlock_irqrestore(&master->queue_lock, flags);

	mesg->state = NULL;
	if (mesg->complete)
		mesg->complete(mesg->context);
}
EXPORT_SYMBOL_GPL(spi_finalize_current_message);

static int spi_start_queue(struct spi_master *master)
{
	unsigned long flags;

	spin_lock_irqsave(&master->queue_lock, flags);

	if (master->running || master->busy) {
		spin_unlock_irqrestore(&master->queue_lock, flags);
		return -EBUSY;
	}

	master->running = true;
	master->cur_msg = NULL;
	spin_unlock_irqrestore(&master->queue_lock, flags);

	queue_kthread_work(&master->kworker, &master->pump_messages);

	return 0;
}

static int spi_stop_queue(struct spi_master *master)
{
	unsigned long flags;
	unsigned limit = 500;
	int ret = 0;

	spin_lock_irqsave(&master->queue_lock, flags);

	while ((!list_empty(&master->queue) || master->busy) && limit--) {
		spin_unlock_irqrestore(&master->queue_lock, flags);
		msleep(10);
		spin_lock_irqsave(&master->queue_lock, flags);
	}

	if (!list_empty(&master->queue) || master->busy)
		ret = -EBUSY;
	else
		master->running = false;

	spin_unlock_irqrestore(&master->queue_lock, flags);

	if (ret) {
		dev_warn(&master->dev,
			 "could not stop message queue\n");
		return ret;
	}
	return ret;
}

static int spi_destroy_queue(struct spi_master *master)
{
	int ret;

	ret = spi_stop_queue(master);

	if (ret) {
		dev_err(&master->dev, "problem destroying queue\n");
		return ret;
	}

	flush_kthread_worker(&master->kworker);
	kthread_stop(master->kworker_task);

	return 0;
}

static int spi_queued_transfer(struct spi_device *spi, struct spi_message *msg)
{
	struct spi_master *master = spi->master;
	unsigned long flags;

	spin_lock_irqsave(&master->queue_lock, flags);

	if (!master->running) {
		spin_unlock_irqrestore(&master->queue_lock, flags);
		return -ESHUTDOWN;
	}
	msg->actual_length = 0;
	msg->status = -EINPROGRESS;

	list_add_tail(&msg->queue, &master->queue);
	if (master->running && !master->busy)
		queue_kthread_work(&master->kworker, &master->pump_messages);

	spin_unlock_irqrestore(&master->queue_lock, flags);
	return 0;
}

static int spi_master_initialize_queue(struct spi_master *master)
{
	int ret;

	master->queued = true;
	master->transfer = spi_queued_transfer;

	ret = spi_init_queue(master);
	if (ret) {
		dev_err(&master->dev, "problem initializing queue\n");
		goto err_init_queue;
	}
	ret = spi_start_queue(master);
	if (ret) {
		dev_err(&master->dev, "problem starting queue\n");
		goto err_start_queue;
	}

	return 0;

err_start_queue:
err_init_queue:
	spi_destroy_queue(master);
	return ret;
}

#if defined(CONFIG_OF)
 
static void of_register_spi_devices(struct spi_master *master)
{
	struct spi_device *spi;
	struct device_node *nc;
	const __be32 *prop;
	char modalias[SPI_NAME_SIZE + 4];
	int rc;
	int len;

	if (!master->dev.of_node)
		return;

	for_each_available_child_of_node(master->dev.of_node, nc) {
		 
		spi = spi_alloc_device(master);
		if (!spi) {
			dev_err(&master->dev, "spi_device alloc error for %s\n",
				nc->full_name);
			spi_dev_put(spi);
			continue;
		}

		if (of_modalias_node(nc, spi->modalias,
				     sizeof(spi->modalias)) < 0) {
			dev_err(&master->dev, "cannot find modalias for %s\n",
				nc->full_name);
			spi_dev_put(spi);
			continue;
		}

		prop = of_get_property(nc, "reg", &len);
		if (!prop || len < sizeof(*prop)) {
			dev_err(&master->dev, "%s has no 'reg' property\n",
				nc->full_name);
			spi_dev_put(spi);
			continue;
		}
		spi->chip_select = be32_to_cpup(prop);

		if (of_find_property(nc, "spi-cpha", NULL))
			spi->mode |= SPI_CPHA;
		if (of_find_property(nc, "spi-cpol", NULL))
			spi->mode |= SPI_CPOL;
		if (of_find_property(nc, "spi-cs-high", NULL))
			spi->mode |= SPI_CS_HIGH;
		if (of_find_property(nc, "spi-3wire", NULL))
			spi->mode |= SPI_3WIRE;
#if defined(MY_ABC_HERE)
		if (of_find_property(nc, "spi-1byte-cs", NULL))
			spi->mode |= SPI_1BYTE_CS;
#endif  

		prop = of_get_property(nc, "spi-max-frequency", &len);
		if (!prop || len < sizeof(*prop)) {
			dev_err(&master->dev, "%s has no 'spi-max-frequency' property\n",
				nc->full_name);
			spi_dev_put(spi);
			continue;
		}
		spi->max_speed_hz = be32_to_cpup(prop);

		spi->irq = irq_of_parse_and_map(nc, 0);

		of_node_get(nc);
		spi->dev.of_node = nc;

		snprintf(modalias, sizeof(modalias), "%s%s", SPI_MODULE_PREFIX,
			 spi->modalias);
		request_module(modalias);
		rc = spi_add_device(spi);
		if (rc) {
			dev_err(&master->dev, "spi_device register error %s\n",
				nc->full_name);
			spi_dev_put(spi);
		}

	}
}
#else
static void of_register_spi_devices(struct spi_master *master) { }
#endif

#ifdef CONFIG_ACPI
static int acpi_spi_add_resource(struct acpi_resource *ares, void *data)
{
	struct spi_device *spi = data;

	if (ares->type == ACPI_RESOURCE_TYPE_SERIAL_BUS) {
		struct acpi_resource_spi_serialbus *sb;

		sb = &ares->data.spi_serial_bus;
		if (sb->type == ACPI_RESOURCE_SERIAL_TYPE_SPI) {
			spi->chip_select = sb->device_selection;
			spi->max_speed_hz = sb->connection_speed;

			if (sb->clock_phase == ACPI_SPI_SECOND_PHASE)
				spi->mode |= SPI_CPHA;
			if (sb->clock_polarity == ACPI_SPI_START_HIGH)
				spi->mode |= SPI_CPOL;
			if (sb->device_polarity == ACPI_SPI_ACTIVE_HIGH)
				spi->mode |= SPI_CS_HIGH;
		}
	} else if (spi->irq < 0) {
		struct resource r;

		if (acpi_dev_resource_interrupt(ares, 0, &r))
			spi->irq = r.start;
	}

	return 1;
}

static acpi_status acpi_spi_add_device(acpi_handle handle, u32 level,
				       void *data, void **return_value)
{
	struct spi_master *master = data;
	struct list_head resource_list;
	struct acpi_device *adev;
	struct spi_device *spi;
	int ret;

	if (acpi_bus_get_device(handle, &adev))
		return AE_OK;
	if (acpi_bus_get_status(adev) || !adev->status.present)
		return AE_OK;

	spi = spi_alloc_device(master);
	if (!spi) {
		dev_err(&master->dev, "failed to allocate SPI device for %s\n",
			dev_name(&adev->dev));
		return AE_NO_MEMORY;
	}

	ACPI_HANDLE_SET(&spi->dev, handle);
	spi->irq = -1;

	INIT_LIST_HEAD(&resource_list);
	ret = acpi_dev_get_resources(adev, &resource_list,
				     acpi_spi_add_resource, spi);
	acpi_dev_free_resource_list(&resource_list);

	if (ret < 0 || !spi->max_speed_hz) {
		spi_dev_put(spi);
		return AE_OK;
	}

	strlcpy(spi->modalias, dev_name(&adev->dev), sizeof(spi->modalias));
	if (spi_add_device(spi)) {
		dev_err(&master->dev, "failed to add SPI device %s from ACPI\n",
			dev_name(&adev->dev));
		spi_dev_put(spi);
	}

	return AE_OK;
}

static void acpi_register_spi_devices(struct spi_master *master)
{
	acpi_status status;
	acpi_handle handle;

	handle = ACPI_HANDLE(master->dev.parent);
	if (!handle)
		return;

	status = acpi_walk_namespace(ACPI_TYPE_DEVICE, handle, 1,
				     acpi_spi_add_device, NULL,
				     master, NULL);
	if (ACPI_FAILURE(status))
		dev_warn(&master->dev, "failed to enumerate SPI slaves\n");
}
#else
static inline void acpi_register_spi_devices(struct spi_master *master) {}
#endif  

static void spi_master_release(struct device *dev)
{
	struct spi_master *master;

	master = container_of(dev, struct spi_master, dev);
	kfree(master);
}

static struct class spi_master_class = {
	.name		= "spi_master",
	.owner		= THIS_MODULE,
	.dev_release	= spi_master_release,
};

struct spi_master *spi_alloc_master(struct device *dev, unsigned size)
{
	struct spi_master	*master;

	if (!dev)
		return NULL;

	master = kzalloc(size + sizeof *master, GFP_KERNEL);
	if (!master)
		return NULL;

	device_initialize(&master->dev);
	master->bus_num = -1;
	master->num_chipselect = 1;
	master->dev.class = &spi_master_class;
	master->dev.parent = dev;
	spi_master_set_devdata(master, &master[1]);

	return master;
}
EXPORT_SYMBOL_GPL(spi_alloc_master);

#ifdef CONFIG_OF
static int of_spi_register_master(struct spi_master *master)
{
	int nb, i, *cs;
	struct device_node *np = master->dev.of_node;

	if (!np)
		return 0;

	nb = of_gpio_named_count(np, "cs-gpios");
	master->num_chipselect = max(nb, (int)master->num_chipselect);

	if (nb == 0 || nb == -ENOENT)
		return 0;
	else if (nb < 0)
		return nb;

	cs = devm_kzalloc(&master->dev,
			  sizeof(int) * master->num_chipselect,
			  GFP_KERNEL);
	master->cs_gpios = cs;

	if (!master->cs_gpios)
		return -ENOMEM;

	for (i = 0; i < master->num_chipselect; i++)
		cs[i] = -ENOENT;

	for (i = 0; i < nb; i++)
		cs[i] = of_get_named_gpio(np, "cs-gpios", i);

	return 0;
}
#else
static int of_spi_register_master(struct spi_master *master)
{
	return 0;
}
#endif

int spi_register_master(struct spi_master *master)
{
	static atomic_t		dyn_bus_id = ATOMIC_INIT((1<<15) - 1);
	struct device		*dev = master->dev.parent;
	struct boardinfo	*bi;
	int			status = -ENODEV;
	int			dynamic = 0;

	if (!dev)
		return -ENODEV;

	status = of_spi_register_master(master);
	if (status)
		return status;

	if (master->num_chipselect == 0)
		return -EINVAL;

	if ((master->bus_num < 0) && master->dev.of_node)
		master->bus_num = of_alias_get_id(master->dev.of_node, "spi");

	if (master->bus_num < 0) {
		 
		master->bus_num = atomic_dec_return(&dyn_bus_id);
		dynamic = 1;
	}

	spin_lock_init(&master->bus_lock_spinlock);
	mutex_init(&master->bus_lock_mutex);
	master->bus_lock_flag = 0;

	dev_set_name(&master->dev, "spi%u", master->bus_num);
	status = device_add(&master->dev);
	if (status < 0)
		goto done;
	dev_dbg(dev, "registered master %s%s\n", dev_name(&master->dev),
			dynamic ? " (dynamic)" : "");

	if (master->transfer)
		dev_info(dev, "master is unqueued, this is deprecated\n");
	else {
		status = spi_master_initialize_queue(master);
		if (status) {
			device_unregister(&master->dev);
			goto done;
		}
	}

	mutex_lock(&board_lock);
	list_add_tail(&master->list, &spi_master_list);
	list_for_each_entry(bi, &board_list, list)
		spi_match_master_to_boardinfo(master, &bi->board_info);
	mutex_unlock(&board_lock);

	of_register_spi_devices(master);
	acpi_register_spi_devices(master);
done:
	return status;
}
EXPORT_SYMBOL_GPL(spi_register_master);

static int __unregister(struct device *dev, void *null)
{
	spi_unregister_device(to_spi_device(dev));
	return 0;
}

void spi_unregister_master(struct spi_master *master)
{
	int dummy;

	if (master->queued) {
		if (spi_destroy_queue(master))
			dev_err(&master->dev, "queue remove failed\n");
	}

	mutex_lock(&board_lock);
	list_del(&master->list);
	mutex_unlock(&board_lock);

	dummy = device_for_each_child(&master->dev, NULL, __unregister);
	device_unregister(&master->dev);
}
EXPORT_SYMBOL_GPL(spi_unregister_master);

int spi_master_suspend(struct spi_master *master)
{
	int ret;

	if (!master->queued)
		return 0;

	ret = spi_stop_queue(master);
	if (ret)
		dev_err(&master->dev, "queue stop failed\n");

	return ret;
}
EXPORT_SYMBOL_GPL(spi_master_suspend);

int spi_master_resume(struct spi_master *master)
{
	int ret;

	if (!master->queued)
		return 0;

	ret = spi_start_queue(master);
	if (ret)
		dev_err(&master->dev, "queue restart failed\n");

	return ret;
}
EXPORT_SYMBOL_GPL(spi_master_resume);

static int __spi_master_match(struct device *dev, const void *data)
{
	struct spi_master *m;
	const u16 *bus_num = data;

	m = container_of(dev, struct spi_master, dev);
	return m->bus_num == *bus_num;
}

struct spi_master *spi_busnum_to_master(u16 bus_num)
{
	struct device		*dev;
	struct spi_master	*master = NULL;

	dev = class_find_device(&spi_master_class, NULL, &bus_num,
				__spi_master_match);
	if (dev)
		master = container_of(dev, struct spi_master, dev);
	 
	return master;
}
EXPORT_SYMBOL_GPL(spi_busnum_to_master);

int spi_setup(struct spi_device *spi)
{
#if defined(CONFIG_SYNO_LSP_HI3536)
#if !CONFIG_SPI_CORE_HISILICON
	unsigned	bad_bits;
#endif
#endif  
	int		status = 0;

#if defined(CONFIG_SYNO_LSP_HI3536)
#if !CONFIG_SPI_CORE_HISILICON
	bad_bits = spi->mode & ~spi->master->mode_bits;
	if (bad_bits) {
		dev_err(&spi->dev, "setup: unsupported mode bits %x\n",
			bad_bits);
		return -EINVAL;
	}
#endif
#endif  

	if (!spi->bits_per_word)
		spi->bits_per_word = 8;

	if (spi->master->setup)
		status = spi->master->setup(spi);

#if defined(MY_ABC_HERE)
	dev_dbg(&spi->dev, "setup mode %d, %s%s%s%s%s"
#else  
	dev_dbg(&spi->dev, "setup mode %d, %s%s%s%s"
#endif  
				"%u bits/w, %u Hz max --> %d\n",
			(int) (spi->mode & (SPI_CPOL | SPI_CPHA)),
			(spi->mode & SPI_CS_HIGH) ? "cs_high, " : "",
			(spi->mode & SPI_LSB_FIRST) ? "lsb, " : "",
			(spi->mode & SPI_3WIRE) ? "3wire, " : "",
			(spi->mode & SPI_LOOP) ? "loopback, " : "",
#if defined(MY_ABC_HERE)
			(spi->mode & SPI_1BYTE_CS) ? "single_cs_byte, " : "",
#endif  
			spi->bits_per_word, spi->max_speed_hz,
			status);

	return status;
}
EXPORT_SYMBOL_GPL(spi_setup);

static int __spi_async(struct spi_device *spi, struct spi_message *message)
{
	struct spi_master *master = spi->master;
	struct spi_transfer *xfer;

	if ((master->flags & SPI_MASTER_HALF_DUPLEX)
			|| (spi->mode & SPI_3WIRE)) {
		unsigned flags = master->flags;

		list_for_each_entry(xfer, &message->transfers, transfer_list) {
			if (xfer->rx_buf && xfer->tx_buf)
				return -EINVAL;
			if ((flags & SPI_MASTER_NO_TX) && xfer->tx_buf)
				return -EINVAL;
			if ((flags & SPI_MASTER_NO_RX) && xfer->rx_buf)
				return -EINVAL;
		}
	}

	list_for_each_entry(xfer, &message->transfers, transfer_list) {
		if (!xfer->bits_per_word)
			xfer->bits_per_word = spi->bits_per_word;
		if (!xfer->speed_hz)
			xfer->speed_hz = spi->max_speed_hz;
		if (master->bits_per_word_mask) {
			 
			if (xfer->bits_per_word > 32)
				return -EINVAL;
			if (!(master->bits_per_word_mask &
					BIT(xfer->bits_per_word - 1)))
				return -EINVAL;
		}

#if defined(MY_ABC_HERE)
#if defined(MY_ABC_HERE)
		if (!xfer->speed_hz)
			xfer->speed_hz = spi->max_speed_hz;
		if (master->min_speed_hz &&
		    xfer->speed_hz < master->min_speed_hz)
			return -EINVAL;
		if (master->max_speed_hz &&
		    xfer->speed_hz > master->max_speed_hz)
			xfer->speed_hz = master->max_speed_hz;
#else  
		if (xfer->speed_hz && master->min_speed_hz &&
		    xfer->speed_hz < master->min_speed_hz)
			return -EINVAL;
		if (xfer->speed_hz && master->max_speed_hz &&
		    xfer->speed_hz > master->max_speed_hz)
			return -EINVAL;
#endif  
#endif  
	}

	message->spi = spi;
	message->status = -EINPROGRESS;
	return master->transfer(spi, message);
}

int spi_async(struct spi_device *spi, struct spi_message *message)
{
	struct spi_master *master = spi->master;
	int ret;
	unsigned long flags;

	spin_lock_irqsave(&master->bus_lock_spinlock, flags);

	if (master->bus_lock_flag)
		ret = -EBUSY;
	else
		ret = __spi_async(spi, message);

	spin_unlock_irqrestore(&master->bus_lock_spinlock, flags);

	return ret;
}
EXPORT_SYMBOL_GPL(spi_async);

int spi_async_locked(struct spi_device *spi, struct spi_message *message)
{
	struct spi_master *master = spi->master;
	int ret;
	unsigned long flags;

	spin_lock_irqsave(&master->bus_lock_spinlock, flags);

	ret = __spi_async(spi, message);

	spin_unlock_irqrestore(&master->bus_lock_spinlock, flags);

	return ret;

}
EXPORT_SYMBOL_GPL(spi_async_locked);

static void spi_complete(void *arg)
{
	complete(arg);
}

static int __spi_sync(struct spi_device *spi, struct spi_message *message,
		      int bus_locked)
{
	DECLARE_COMPLETION_ONSTACK(done);
	int status;
	struct spi_master *master = spi->master;

	message->complete = spi_complete;
	message->context = &done;

	if (!bus_locked)
		mutex_lock(&master->bus_lock_mutex);

	status = spi_async_locked(spi, message);

	if (!bus_locked)
		mutex_unlock(&master->bus_lock_mutex);

	if (status == 0) {
		wait_for_completion(&done);
		status = message->status;
	}
	message->context = NULL;
	return status;
}

int spi_sync(struct spi_device *spi, struct spi_message *message)
{
	return __spi_sync(spi, message, 0);
}
EXPORT_SYMBOL_GPL(spi_sync);

int spi_sync_locked(struct spi_device *spi, struct spi_message *message)
{
	return __spi_sync(spi, message, 1);
}
EXPORT_SYMBOL_GPL(spi_sync_locked);

int spi_bus_lock(struct spi_master *master)
{
	unsigned long flags;

	mutex_lock(&master->bus_lock_mutex);

	spin_lock_irqsave(&master->bus_lock_spinlock, flags);
	master->bus_lock_flag = 1;
	spin_unlock_irqrestore(&master->bus_lock_spinlock, flags);

	return 0;
}
EXPORT_SYMBOL_GPL(spi_bus_lock);

int spi_bus_unlock(struct spi_master *master)
{
	master->bus_lock_flag = 0;

	mutex_unlock(&master->bus_lock_mutex);

	return 0;
}
EXPORT_SYMBOL_GPL(spi_bus_unlock);

#define	SPI_BUFSIZ	max(32,SMP_CACHE_BYTES)

static u8	*buf;

int spi_write_then_read(struct spi_device *spi,
		const void *txbuf, unsigned n_tx,
		void *rxbuf, unsigned n_rx)
{
	static DEFINE_MUTEX(lock);

	int			status;
	struct spi_message	message;
	struct spi_transfer	x[2];
	u8			*local_buf;

	if ((n_tx + n_rx) > SPI_BUFSIZ || !mutex_trylock(&lock)) {
		local_buf = kmalloc(max((unsigned)SPI_BUFSIZ, n_tx + n_rx),
				    GFP_KERNEL | GFP_DMA);
		if (!local_buf)
			return -ENOMEM;
	} else {
		local_buf = buf;
	}

	spi_message_init(&message);
	memset(x, 0, sizeof x);
	if (n_tx) {
		x[0].len = n_tx;
		spi_message_add_tail(&x[0], &message);
	}
	if (n_rx) {
		x[1].len = n_rx;
		spi_message_add_tail(&x[1], &message);
	}

	memcpy(local_buf, txbuf, n_tx);
	x[0].tx_buf = local_buf;
	x[1].rx_buf = local_buf + n_tx;

	status = spi_sync(spi, &message);
	if (status == 0)
		memcpy(rxbuf, x[1].rx_buf, n_rx);

	if (x[0].tx_buf == buf)
		mutex_unlock(&lock);
	else
		kfree(local_buf);

	return status;
}
EXPORT_SYMBOL_GPL(spi_write_then_read);

static int __init spi_init(void)
{
	int	status;

	buf = kmalloc(SPI_BUFSIZ, GFP_KERNEL);
	if (!buf) {
		status = -ENOMEM;
		goto err0;
	}

	status = bus_register(&spi_bus_type);
	if (status < 0)
		goto err1;

	status = class_register(&spi_master_class);
	if (status < 0)
		goto err2;
	return 0;

err2:
	bus_unregister(&spi_bus_type);
err1:
	kfree(buf);
	buf = NULL;
err0:
	return status;
}

postcore_initcall(spi_init);
