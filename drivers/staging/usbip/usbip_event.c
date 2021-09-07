#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include "usbip_common.h"
#include <linux/kthread.h>

static int event_handler(struct usbip_device *ud)
{
	usbip_dbg_eh("enter\n");

	while (usbip_event_happened(ud)) {
		usbip_dbg_eh("pending event %lx\n", ud->event);

#ifdef MY_ABC_HERE
		 
		if (ud->event & USBIP_EH_CLOSE_SOCKET) {
			ud->eh_ops.close_connection(ud);
			ud->eh_ops.cleanup_urb(ud);

			ud->event &= ~USBIP_EH_CLOSE_SOCKET;

			break;
		}
#endif
		 
		if (ud->event & USBIP_EH_SHUTDOWN) {
			ud->eh_ops.shutdown(ud);

			ud->event &= ~USBIP_EH_SHUTDOWN;

			break;
		}

		if (ud->event & USBIP_EH_BYE)
			return -1;

		if (ud->event & USBIP_EH_RESET) {
			ud->eh_ops.reset(ud);

			ud->event &= ~USBIP_EH_RESET;

			break;
		}

		if (ud->event & USBIP_EH_UNUSABLE) {
			ud->eh_ops.unusable(ud);

			ud->event &= ~USBIP_EH_UNUSABLE;

			break;
		}

		printk(KERN_ERR "%s: unknown event\n", __func__);
		return -1;
	}

	return 0;
}

static void event_handler_loop(struct usbip_task *ut)
{
	struct usbip_device *ud = container_of(ut, struct usbip_device, eh);

	while (1) {
		if (signal_pending(current)) {
			usbip_dbg_eh("signal catched!\n");
			break;
		}

		if (event_handler(ud) < 0)
			break;

		wait_event_interruptible(ud->eh_waitq,
					usbip_event_happened(ud));
		usbip_dbg_eh("wakeup\n");
	}
}

int usbip_start_eh(struct usbip_device *ud)
{
	struct usbip_task *eh = &ud->eh;
	struct task_struct *th;

	init_waitqueue_head(&ud->eh_waitq);
	ud->event = 0;

	usbip_task_init(eh, "usbip_eh", event_handler_loop);

	th = kthread_run(usbip_thread, (void *)eh, "usbip");
	if (IS_ERR(th)) {
		printk(KERN_WARNING
			"Unable to start control thread\n");
		return PTR_ERR(th);
	}

	wait_for_completion(&eh->thread_done);
	return 0;
}
EXPORT_SYMBOL_GPL(usbip_start_eh);

void usbip_stop_eh(struct usbip_device *ud)
{
	struct usbip_task *eh = &ud->eh;
#ifdef MY_ABC_HERE
	if(eh->thread == current) {
		return;
	}
#endif
	wait_for_completion(&eh->thread_done);
	usbip_dbg_eh("usbip_eh has finished\n");
}
EXPORT_SYMBOL_GPL(usbip_stop_eh);

void usbip_event_add(struct usbip_device *ud, unsigned long event)
{
	spin_lock(&ud->lock);

	ud->event |= event;

	wake_up(&ud->eh_waitq);

	spin_unlock(&ud->lock);
}
EXPORT_SYMBOL_GPL(usbip_event_add);

int usbip_event_happened(struct usbip_device *ud)
{
	int happened = 0;

	spin_lock(&ud->lock);

	if (ud->event != 0)
		happened = 1;

	spin_unlock(&ud->lock);

	return happened;
}
EXPORT_SYMBOL_GPL(usbip_event_happened);

#ifdef MY_ABC_HERE
int syno_usbip_event_happened(struct usbip_device *ud)
{
	int happened = 0;

	spin_lock(&ud->lock);

	if (SDEV_EVENT_ERROR_TCP == ud->event) {
		ud->sockfd = -1;
	} else if (0 != ud->event) {
		happened = 1;
	}

	spin_unlock(&ud->lock);

	return happened;
}
EXPORT_SYMBOL_GPL(syno_usbip_event_happened);
#endif
