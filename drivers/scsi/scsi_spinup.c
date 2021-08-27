 
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/list.h>

#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_spinup.h>

#include "scsi_priv.h"
#include "scsi_logging.h"

#ifdef CONFIG_SYNO_ARMADA_V2
extern int ss_stats[128];
#endif

static char *cmdline = NULL;

int spinup_cmdline_config(char *s);
__setup("spinup_config=", spinup_cmdline_config);

int spinup_cmdline_config(char *s)
{
    cmdline = s;
    return 1;
}

static int spinup_timeout = 0;
static int spinup_max = 0;
static int spinup_enabled = 0;

static atomic_t spinup_now;
struct spinup_node {
	struct list_head list;
	struct scsi_device *sdev;
};
static struct list_head spinup_list;

int scsi_parse_spinup_max(char *p_config)
{
	 
	unsigned int _spinup_max;
	int syntax_err = 0;

	_spinup_max = 0;
	if (p_config == NULL)
	    return 0;
	while((*p_config >= '0') && (*p_config <= '9'))	{
		_spinup_max = (_spinup_max * 10) + (*p_config - '0');
		p_config++;
	}
	if ((*p_config != ',') || (_spinup_max == 0)) {
		printk("Syntax error while parsing spinup_max value from command line\n");
		syntax_err = 1;
	}
	if ((_spinup_max < 0)||(_spinup_max > 8)) {
		printk("Syntax error while parsing spinup_max value from command line. (value must between 1 to 8\n");
		syntax_err = 1;
	}
	if(syntax_err == 0) {
		printk("  o Maximum Disk Spinup set to: %d.\n", _spinup_max);
		return _spinup_max;
	}
	return 0;
}

int scsi_parse_spinup_timeout(char *p_config)
{
	 
	unsigned int _spinup_timout;
	int syntax_err = 0;

	_spinup_timout = 0;
	if (p_config == NULL)
	    return 0;
	while((*p_config != ',') && (*p_config != '\0'))
		p_config++;
		
	p_config++;
	while((*p_config >= '0') && (*p_config <= '9'))	{
		_spinup_timout = (_spinup_timout * 10) + (*p_config - '0');
		p_config++;
	}
	if ((*p_config != '\0') || (_spinup_timout == 0)) {
		printk("Syntax error while parsing spinup_timout value from command line\n");
		syntax_err = 1;
	}
	if ((_spinup_timout < 0) || (_spinup_timout > 6)) {
		printk("Syntax error while parsing spinup_timout value from command line (spinup_timeout must be between 1 to 6) \n");
		syntax_err = 1;
	}
	if(syntax_err == 0) {
		printk("  o Disk Spinup timeout set to: %d.\n", _spinup_timout);
		return _spinup_timout;
	}
	return 0;
}

int timeout_to_jiffies (int timeout)
{
	unsigned int secs=0;

	switch(timeout) {
		case 0:		 
			break;
		case 252:	 
			secs = 21 * 60;
			break;
		case 253:	 
			break;
		case 254:	 
			break;
		case 255:	 
			secs = 21 * 60 + 15;
			break;
		default:
			if (timeout <= 240) {
				secs = timeout * 5;
			} else if (timeout <= 251) {
				secs = ((timeout - 240) * 30) * 60;
			} else
				printk("illegal value");
			break;
	}
	return msecs_to_jiffies ((secs-1) * 1000);
}

void standby_add_timer(struct scsi_device *sdev, int timeout,
		    void (*complete)(struct scsi_device *))
{
	 
	if (sdev->standby_timeout.function)
		del_timer(&sdev->standby_timeout);

	sdev->standby_timeout.data = (unsigned long)sdev;
	sdev->standby_timeout_secs = timeout;

	sdev->standby_timeout.expires = timeout + jiffies ;
	sdev->standby_timeout.function = (void (*)(unsigned long)) complete;

	SCSI_LOG_ERROR_RECOVERY(5, printk("%s: scmd: %p, time:"
					  " %d, (%p)\n", __FUNCTION__,
					  sdev, timeout, complete));

	add_timer(&sdev->standby_timeout);
}

int standby_delete_timer(struct scsi_device *sdev)
{
	int rtn;

	rtn = del_timer(&sdev->standby_timeout);

	SCSI_LOG_ERROR_RECOVERY(5, printk("%s: scmd: %p,"
					 " rtn: %d\n", __FUNCTION__,
					 sdev, rtn));

	sdev->standby_timeout.data = (unsigned long)NULL;
	sdev->standby_timeout.function = NULL;

	return rtn;
}

void standby_times_out(struct scsi_device *sdev)
{
	unsigned long flags = 0;

#ifdef CONFIG_SYNO_ARMADA_V2
	 
	sdev->sdev_power_state = SDEV_PW_STANDBY_TIMEOUT_PASSED;
	ss_stats[sdev->ss_id] = sdev->sdev_power_state;
#else
 
	spin_unlock_irqrestore(sdev->host->host_lock, flags);
	sdev->sdev_power_state = SDEV_PW_STANDBY_TIMEOUT_PASSED;
	spin_lock_irqsave(sdev->host->host_lock, flags);
#endif

	standby_delete_timer(sdev);
}

void spinup_add_timer(struct scsi_device *sdev, int timeout,
		    void (*complete)(struct scsi_device *))
{
	 
	if (sdev->spinup_timeout.function)
		del_timer(&sdev->spinup_timeout);
		
	sdev->spinup_timeout.data = (unsigned long)sdev;
	
	sdev->spinup_timeout.expires = jiffies + msecs_to_jiffies (timeout * 1000);
	sdev->spinup_timeout.function = (void (*)(unsigned long)) complete;

	SCSI_LOG_ERROR_RECOVERY(5, printk("%s: scmd: %p, time:"
					  " %d, (%p)\n", __FUNCTION__,
					  sdev, timeout, complete));
	add_timer(&sdev->spinup_timeout);
	
}

int spinup_delete_timer(struct scsi_device *sdev)
{
	int rtn;

	rtn = del_timer(&sdev->spinup_timeout);

	SCSI_LOG_ERROR_RECOVERY(5, printk("%s: scmd: %p,"
					 " rtn: %d\n", __FUNCTION__,
					 sdev, rtn));

	sdev->spinup_timeout.data = (unsigned long)NULL;
	sdev->spinup_timeout.function = NULL;

	return rtn;
}
void spinup_times_out(struct scsi_device *sdev)
{
 
	scsi_spinup_up();
	spinup_delete_timer(sdev);
	scsi_spinup_device_dequeue_next();
	
}

int scsi_spinup_enabled(void)
{
	return spinup_enabled;
}

int scsi_spinup_get_timeout(void)
{
	return spinup_timeout;
}

int __init scsi_spinup_init(void)
{
	printk("SCSI Staggered Spinup:\n");
	spinup_max = scsi_parse_spinup_max(cmdline);
	spinup_timeout = scsi_parse_spinup_timeout(cmdline);
	if ((spinup_max == 0) || (spinup_timeout == 0)){
		spinup_enabled = 0;
		printk("SCSI Staggered Spinup Feature Status: Disabled\n");
	}else{
		spinup_enabled = 1;
		atomic_set(&spinup_now,spinup_max);
		printk("SCSI Staggered Spinup Feature Status: Enabled\n");
	}
	INIT_LIST_HEAD(&spinup_list);
	return 0;
}

void scsi_spinup_device_queue(struct scsi_device *sdev)
{
	struct list_head *ptr;
	struct spinup_node *entry;
	struct spinup_node *new = (struct spinup_node *) kmalloc(sizeof(struct spinup_node), GFP_KERNEL);

	new->sdev = sdev;
	list_add_tail(&new->list, &spinup_list);

}

int scsi_spinup_device(struct scsi_cmnd *cmd)
{
	switch (cmd->device->sdev_power_state) {
		case SDEV_PW_STANDBY:
		case SDEV_PW_STANDBY_TIMEOUT_PASSED:
			cmd->device->sdev_power_state = SDEV_PW_WAIT_FOR_SPIN_UP;
#ifdef CONFIG_SYNO_ARMADA_V2
			ss_stats[cmd->device->ss_id] = cmd->device->sdev_power_state;
#endif
			 
			if (!scsi_spinup_down())
			{
 
				scsi_spinup_device_queue(cmd->device);
				return 1;
			}
			cmd->device->sdev_power_state = SDEV_PW_SPINNING_UP;
#ifdef CONFIG_SYNO_ARMADA_V2
			ss_stats[cmd->device->ss_id] = cmd->device->sdev_power_state;
#endif
			 
			spinup_add_timer(cmd->device, scsi_spinup_get_timeout(), spinup_times_out);
		break;
		case SDEV_PW_STANDBY_TIMEOUT_WAIT:
			standby_add_timer(cmd->device, cmd->device->standby_timeout_secs, standby_times_out);
		break;
		default:
		break;
	}
	return 0;
}
int scsi_spinup_down(void)
{
	if (atomic_read(&spinup_now) > 0) {
		atomic_dec(&spinup_now);
 
		return 1;
	}
	return 0;
}
int scsi_spinup_up(void)
{
	atomic_inc(&spinup_now);
 
	return 0;	
}
int scsi_spinup_device_dequeue_next(void)
{
	struct list_head *ptr;
	struct spinup_node *entry;

	if(!list_empty(&spinup_list))
	{
		ptr = spinup_list.next;
		entry = list_entry(ptr, struct spinup_node, list);
 
		if (scsi_spinup_down())
		{
			entry->sdev->sdev_power_state = SDEV_PW_SPINNING_UP;
#ifdef CONFIG_SYNO_ARMADA_V2
			ss_stats[entry->sdev->ss_id] = entry->sdev->sdev_power_state;
#endif
			spinup_add_timer(entry->sdev, scsi_spinup_get_timeout(), spinup_times_out);
			scsi_internal_device_unblock(entry->sdev);
			list_del(ptr);
		}
	}
 
	return 0;
}
