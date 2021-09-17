#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/slab.h>
#include <linux/smp.h>
#include <asm-generic/irq_regs.h>
#include <linux/hisi_ipcm_data_common.h>
#include <linux/hisi_ipcm_proto.h>
#include <linux/hios_type.h>
#include <linux/delay.h>

/* cpu id, 0:A17, 1:A7 */
unsigned int cpuid = CONFIG_HIIPCM_CPU_ID;

/* ipc device information */
hisi_ipc_dev_t hisi_ipc_dev;

/*ipc message transfer handle table */
static hisi_ipc_transfer_handle_t *
	hisi_ipc_handle_table[HISI_MAX_IPCM_PORT] = {0};

/*external variable and function*/
extern struct gic_sgi_handle ipc_irq_handle;
extern struct hisi_ipcm_shared_mem_alloc_info hisi_ipc_shared_mem_info;
extern int hisi_ipcm_shared_mem_init(void);

/* The flag for A17 and A7 handshake*/
#define IPCM_HANDSHAKE_FLAG	0x776F7368

#ifdef CONFIG_HIIPCM_IRQ_STATISTIC_ENABLE
/* the kobj pointer for hi-ipcm */
static struct kobject *hi_ipcm_kobj = NULL;

/*static int ipcm;*/

static ssize_t ipcm_irqs_show(struct kobject *kobj, struct attribute *attr,
				char *buf)
{
	hisi_ipc_dev_t *ipc_dev = &hisi_ipc_dev;

	return sprintf(buf, "sended irqs: %d, received irqs: %d\n",
			ipc_dev->sended_irqs, ipc_dev->received_irqs);
}

static struct kobj_attribute ipcm_attribute =
	__ATTR(ipcm, 0666, ipcm_irqs_show, NULL);

static struct attribute *attrs[] = {
	&ipcm_attribute.attr,
	NULL
};

static struct attribute_group attr_group = {
	.attrs = attrs,
};
#endif

/****************************************************************
**function    Send the initial message to the remote
**param       void
**return      0:success    -1:failed
****************************************************************/
static int  ipcm_send_initial_data(void)
{
	hios_int ret;
	hisi_ipc_dev_t  *ipc_dev;
	hisi_ipc_transfer_handle_t *handle;
	char buf[1] = {0};

	ipc_dev = &hisi_ipc_dev;

	/*open the ipc according to cpu id*/
	if (ipc_dev->local_cpu_id & 0x1)
		handle = ipcm_vdd_open(ipc_dev->local_cpu_id - 1,
				0, HISI_IPCM_PRIORITY_MSG);
	else
		handle = ipcm_vdd_open(ipc_dev->local_cpu_id + 1,
				0, HISI_IPCM_PRIORITY_MSG);

	if (!handle) {
		mcc_trace(IPCM_ERR_LEVEL, "Open handle failed!\n");
		return -1;
	}

	/*Send the init data*/
	ret = ipcm_vdd_sendto(handle, buf, 0, HISI_IPCM_INITIAL_DATA);
	if (ret < 0) {
		mcc_trace(IPCM_ERR_LEVEL,
			"INITIAL ERROR: ipcm_vdd_sendto error %d\n",
			ret);
		ipcm_vdd_close(handle);
		return -1;
	}

	ipcm_vdd_close(handle);
	return 0;
}

/****************************************************************
**funcion	receive message data
**param	cpu_intrf:  gic cpu interfatce number which is
			correspond to source cpu id.
**return	void
******************************************************************/
static void  _ipcm_receive_data(unsigned long cpu_intrf)
{
	int ret;
	unsigned int r_offset;
	unsigned int w_offset;
	unsigned int mem_flag;
	unsigned int mem_base;
	static int check_flag = 0;
	void *data;
	hisi_ipc_dev_t *ipc_dev;
	struct hisi_ipcm_transfer_head *head;
	struct hisi_ipc_transfer_handle *handle;
	struct hisi_ipcm_shared_mem_alloc_info *pshminfo;

	ipc_dev = &hisi_ipc_dev;

#ifdef CONFIG_HIIPCM_IRQ_STATISTIC_ENABLE
	/* increments the number of received irq*/
	ipc_dev->received_irqs++;
#endif

	/*Get mem flag and write pointer offset from data register*/
	mem_flag = readl((void *)(ipc_dev->data_reg_base
			+ GET_DATA_REG_OFFSET(cpu_intrf, 0)));

	mem_flag &= ~(MEM_FLG_BUSY_MASK);
	mcc_trace(IPCM_INFO_LEVEL, "recv flag 0x%x\n", mem_flag);

	w_offset  = readl((void *)(ipc_dev->data_reg_base
			+ GET_DATA_REG_OFFSET(cpu_intrf, 1)));
	mcc_trace(IPCM_INFO_LEVEL, "recv w_offset 0x%x\n", w_offset);

	/*release the memory flage register*/
	writel(0, (void *)(ipc_dev->data_reg_base
			+ GET_DATA_REG_OFFSET(cpu_intrf, 0)));

	/*Get the shared memory base address*/
	pshminfo = &hisi_ipc_shared_mem_info;
	if (mem_flag == IS_IRQ_SHARED_MEM) {
		/*If there is not a message, return directly*/
		if (pshminfo->irq_mem[cpu_intrf]
				.last_recv_w_offset == w_offset)
			return;
		else
			pshminfo->irq_mem[cpu_intrf]
				.last_recv_w_offset = w_offset;
		mem_base = pshminfo->irq_mem[cpu_intrf].base_addr;
	} else {
		/*If there is not a message, return directly*/
		if (pshminfo->thread_mem[cpu_intrf]
				.last_recv_w_offset == w_offset)
			return;
		else
			pshminfo->thread_mem[cpu_intrf]
				.last_recv_w_offset = w_offset;
		mem_base = pshminfo->thread_mem[cpu_intrf].base_addr;
	}

	r_offset = *(unsigned int *)mem_base;

	/* received initial data */
	head = (struct hisi_ipcm_transfer_head *)(mem_base
			+ HISI_IPCM_SHARED_USEED_SIZE
			+ r_offset - _IPCM_HEAD_SIZE);
	if (HISI_IPC_HEAD_MAGIC_INITIAL == head->magic) {
		/* If the flag is already 1, means the remote is *
		 * running, but the remote's flag is 0; send the *
		 * initial message to the remote again. */
		if ((1 == ipc_dev->remote_map_flag) && (0 == check_flag)) {
			check_flag = 1;
			(void)ipcm_send_initial_data();
		}

		/* Set the remote map flag, which means the *
		 * remote module is running */
		ipc_dev->remote_map_flag = 1;
		mcc_trace(IPCM_INFO_LEVEL, "remote_map %d\n",
				ipc_dev->remote_map_flag);

		/* Update the r_offset */
		r_offset += LEN_DIGHT(head->length) + _IPCM_HEAD_SIZE;
		writel(r_offset, (void *)mem_base);

		return;
	}

	/* receive normal data */
	do {
		/* If msg head is jump type, read msg from top */
		if (HISI_IPC_HEAD_MAGIC_JUMP == head->magic) {
			r_offset = 0;
			head = (struct hisi_ipcm_transfer_head *)
				(mem_base + HISI_IPCM_SHARED_USEED_SIZE
				 - _IPCM_HEAD_SIZE);
		}

		/* Get message data address */
		data = (void *)head + _IPCM_HEAD_SIZE;

		/* Call user process function */
		handle = hisi_ipc_handle_table[head->port];
		mcc_trace(IPCM_INFO_LEVEL,
			"hanlde[%d] 0x%8lx notify recv\n",
			head->port, (unsigned long)handle);

		if (handle && handle->ipc_opt.recvfrom_notify) {
			ret = handle->ipc_opt.recvfrom_notify(handle,
					data, head->length);
			if (ret == -1)
				mcc_trace(IPCM_ERR_LEVEL,
					"ipcm_vdd_notifier returned error!\n");
		} else
			mcc_trace(IPCM_ERR_LEVEL,
				"recv_notifier handle is NULL or have not set it !\n");

		/* Read the next message */
		r_offset += LEN_DIGHT(head->length) + _IPCM_HEAD_SIZE;
		head = (struct hisi_ipcm_transfer_head *)
			(mem_base + HISI_IPCM_SHARED_USEED_SIZE
			 + r_offset - _IPCM_HEAD_SIZE);
		mcc_trace(IPCM_INFO_LEVEL,
			"w_offset %u\t new r_offset %u\n",
			w_offset, r_offset);
	} while (r_offset != w_offset);

	/* Save the read pointer to shared meory */
	writel(r_offset, (void *)mem_base);
}

/***********************************************
 **function	ipc interrupt process function
 **param	cpu_intrf: gic cpu interface,
		irq: interrupt number,
		regs:the register's value of thread context
 **return     void
***********************************************/
void hisi_ipcm_handle_irq(unsigned int cpu_intrf,
		unsigned int irq,
		struct pt_regs *regs)
{
	unsigned int core_id;
	struct pt_regs *old_regs;

	old_regs = set_irq_regs(regs);

	/* If irq is illegal, exit the proccess function */
	if (IPC_GIC_IRQ != irq)
		goto out;

	/* Get core id, just core 0 proccesses the interrupt */
	core_id = smp_processor_id();
	if (0 != core_id) {
		pr_err("Core id(%d) is illegal!\n", core_id);
		goto out;
	}

	/* Receive message */
	_ipcm_receive_data(cpu_intrf);
out:
	set_irq_regs(old_regs);
}

/**************************************************
 **function    register the interrupt proccess func
 **param       phandle: interrupt information pointer
 **return      void
 *************************************************/
void ipc_irq_register(struct gic_sgi_handle *phandle)
{
	if (NULL == phandle)
		pr_err("ERR: phandle is NULL!\n");

	ipc_irq_handle.irq = phandle->irq;
	ipc_irq_handle.handle = phandle->handle;
}

/*
 *	function: open ipcm handle
 *	input:    target_id, port, priority
 *	output:   NULL
 *	return:   hanlde: success, NULL: failed
 *	notes:
 *	This function will open the handle by targetid and port,
 *	a handle can only opened once
 *
 */
void *ipcm_vdd_open(int target_id, int port, int priority)
{
	hisi_ipc_dev_t *ipc_dev;
	hisi_ipc_transfer_handle_t *handle = NULL;
	mcc_trace(IPCM_INFO_LEVEL, "target_id = %d\n", target_id);

	ipc_dev = &hisi_ipc_dev;

	/* If target id is illegal, return null */
	if (target_id < 0 || target_id >= HISI_IPCM_CPU_MAX
		|| target_id == ipc_dev->local_cpu_id) {
		mcc_trace(IPCM_ERR_LEVEL,
			"VDD ERROR: target_id %d out of range[%d ~ %d]",
			target_id, 0, (HISI_IPCM_CPU_MAX - 1));
		mcc_trace(IPCM_ERR_LEVEL, " or send to yourself.\n");
		return NULL;
	}

	/* If port is out of scope, return null */
	if (port < 0 || port >= HISI_MAX_IPCM_PORT) {
		mcc_trace(IPCM_ERR_LEVEL,
			"VDD ERROR: port %d out of range [0 - %d].\n",
			port, (HISI_MAX_IPCM_PORT - 1));
		return NULL;
	}

	/* If handle has been initialized, return */
	handle = hisi_ipc_handle_table[port];
	if (handle) {
		mcc_trace(IPCM_ERR_LEVEL,
			"handle already exist target_id %d port %d\n",
			target_id, port);
		return handle;
	}

	/* Alloc handle space */
	if (in_interrupt())
		handle = (hisi_ipc_transfer_handle_t *)
			kmalloc(sizeof(hisi_ipc_transfer_handle_t), GFP_ATOMIC);
	else
		handle = (hisi_ipc_transfer_handle_t *)
			kmalloc(sizeof(hisi_ipc_transfer_handle_t), GFP_KERNEL);

	if (!handle) {
		pr_err("VDD ERROR: kmalloc handle error!\n");
		return NULL;
	}

	/* Init handle */
	handle->target_id = target_id;
	handle->port      = port;
	handle->priority  = priority;
	handle->ipc_opt.data = 0;
	handle->ipc_opt.recvfrom_notify = NULL;
	mcc_trace(IPCM_INFO_LEVEL, "port %d handle 0x%8lx\n",
			port, (unsigned long)handle);

	hisi_ipc_handle_table[port] = handle;

	return handle;
}
EXPORT_SYMBOL(ipcm_vdd_open);

/*
 *	function: Get local cpu id
 *	input:    NULL
 *	output:   NULL
 *	return:   0: host, 1: slave
 *	notes:
 *
 */
int ipcm_vdd_localid(void)
{
	return hisi_ipc_dev.local_cpu_id;
}
EXPORT_SYMBOL(ipcm_vdd_localid);

/*
 *    function: Get the remote cpu id
 *    input:    handle: ipc handle
 *    output:   target_id: the pointer of target id
 *    return:   0:success
 *    notes:
 *
 */
int ipcm_vdd_remoteid(int *target_id, void *handle)
{
	hisi_ipc_transfer_handle_t *p = handle;
	mcc_trace(IPCM_INFO_LEVEL, "target_id %d\n", p->target_id);
	*target_id = p->target_id;
	return 0;
}
EXPORT_SYMBOL(ipcm_vdd_remoteid);

/*
 *    function: Check the romete is running or not
 *    input:    NULL
 *    output:   NULL
 *    return:   0:the remote is running, -1:the remote isn't running
 *    notes:
 *
 */
int ipcm_vdd_check_remote()
{
	if (hisi_ipc_dev.remote_map_flag) {
		return 0;
	} else {
		(void)ipcm_send_initial_data();
		return -1;
	}
}
EXPORT_SYMBOL(ipcm_vdd_check_remote);

/*
 *	function: close ipc handle
 *	input:    handle:ipc handle
 *	output:   NULL
 *	return:   void
 *	notes:
 *
 */
void ipcm_vdd_close(void *handle)
{
	hisi_ipc_transfer_handle_t *p = handle;
	if (handle) {
		mcc_trace(IPCM_INFO_LEVEL, "handle 0x%8lx\n",
				(unsigned long)handle);
		hisi_ipc_handle_table[p->port] = 0;
		kfree(handle);
	}
}
EXPORT_SYMBOL(ipcm_vdd_close);

/*
 *	function: send message
 *	input:    handle: ipc handle
 *                buf: message buffer address
 *                len: message length
 *	flag:     HISI_IPC_NORMAL_DATA normal data,
 *		  HISI_IPC_INITIAL_DATA initial data from host
 *      output:   NULL
 *	return:   send data length if success , -1 if failed
 *	notes:
 *
 */
int ipcm_vdd_sendto(void *handle,
		const void *buf,
		unsigned int len,
		unsigned int flag)
{
	int ret;
	unsigned int core_id;
	unsigned int max_len;
	hisi_ipc_dev_t  *ipc_dev;
	struct hisi_ipcm_transfer_head head;
	struct hisi_ipcm_memory_info *ipc_core_mem;
	struct hisi_ipc_transfer_handle *phandle = handle;
	struct hisi_ipcm_shared_mem_alloc_info *ipc_share_mem;

	/* If the input parameters are illegal, return -1 */
	if (!phandle) {
		mcc_trace(IPCM_ERR_LEVEL, "VDD ERROR: IPCM handle is NULL!\n");
		return -1;
	}

	if (!buf) {
		mcc_trace(IPCM_ERR_LEVEL, "VDD ERROR: sendto buffer NULL!\n");
		return -1;
	}

	ipc_dev = &hisi_ipc_dev;
	ipc_share_mem = &hisi_ipc_shared_mem_info;
	core_id = smp_processor_id();

	/* Calculate the max message length */
	if (in_interrupt()) {
		ipc_core_mem = &(ipc_share_mem->irq_mem[
				(ipc_dev->local_cpu_id << 2)
				+ core_id]);
		max_len = LEN_DIGHT(ipc_core_mem->buf_len - _IPCM_HEAD_SIZE);
	} else {
		ipc_core_mem = &(ipc_share_mem->thread_mem[
				(ipc_dev->local_cpu_id << 2)
				+ core_id]);
		max_len = LEN_DIGHT(ipc_core_mem->buf_len
				- _IPCM_HEAD_SIZE);
	}

	/* If message length is to large, return -1 */
	if (len >= max_len) {
		mcc_trace(IPCM_ERR_LEVEL,
			"VDD ERROR: sendto length(%d) is out of range! max:%d\n",
			len, max_len);
		return -1;
	}

	/* Construct the message head */
	head.target_id = phandle->target_id;
	head.port      = phandle->port;
	head.src       = ipc_dev->local_cpu_id;
	head.length    = len;

	if (flag & HISI_IPCM_INITIAL_DATA)
		head.magic = HISI_IPC_HEAD_MAGIC_INITIAL;
	else
		head.magic = 0;
	mcc_trace(IPCM_INFO_LEVEL, "send magic 0x%8x\n", head.magic);

	/* Send the message */
	if (in_interrupt())
		ret = hisi_ipcm_send_irq_data(buf, len, &head);
	else
		ret = hisi_ipcm_send_thread_data(phandle, buf, len, &head);

	return ret;
}
EXPORT_SYMBOL(ipcm_vdd_sendto);

/*
 *      function: get handle option
 *      input:    handle: ipc handle
 *      output:   opt: channel option
 *	return:   0: success, -1: failed
 *	notes:
 *	This function get the channel option by handle,
 *
 */
int ipcm_vdd_getopt(void *handle, ipcm_vdd_opt_t *opt)
{
	hisi_ipc_transfer_handle_t *p = handle;

	if (!p) {
		mcc_trace(IPCM_ERR_LEVEL,
			"VDD ERROR: getopt handle is NULL!\n");
		return -1;
	}

	opt->recvfrom_notify = p->ipc_opt.recvfrom_notify;
	opt->data = p->ipc_opt.data;

	return 0;
}
EXPORT_SYMBOL(ipcm_vdd_getopt);

/*
 *      function: set handle option
 *      input:    handle: ipc handle
 *	return:   0: success, -1: failed
 *	notes:
 *	This function set the channel option by handle and option,
 *
 */
int ipcm_vdd_setopt(void *handle, ipcm_vdd_opt_t *opt)
{
	hisi_ipc_transfer_handle_t *p = handle;

	if (!p) {
		mcc_trace(IPCM_ERR_LEVEL,
			"VDD ERROR: setopt handle is NULL!\n");
		return -1;
	}

	p->ipc_opt.data = opt->data;
	p->ipc_opt.recvfrom_notify = opt->recvfrom_notify;

	return 0;
}
EXPORT_SYMBOL(ipcm_vdd_setopt);

/*
 *      function: ipc module exit
 *      input:    NULL
 *      output:   NULL
 *      return:   NULL
 *	notes:
 *
 */
static void __exit ipcm_vdd_cleanup(void)
{
	del_timer(&hisi_ipc_dev.timer);
	iounmap((void *)hisi_ipc_shared_mem_info.share_mem_start_addr);
#ifdef CONFIG_HIIPCM_IRQ_STATISTIC_ENABLE
	kobject_put(hi_ipcm_kobj);
#endif
}

/*
 *      function: ipcm module init
 *      input:    void
 *	output:   NULL
 *	return:   0: success, -1: failed
 *      Notes:
 *      This initial function create handle table, *
 *	create kernel receive thread,
 *      timer initial, register irq and so on
 *
 */
static int __init ipcm_vdd_init(void)
{
	hisi_ipc_dev_t *ipc_dev;
	unsigned int flag_value;
	struct gic_sgi_handle ipc_irq;
	unsigned long ipcm_handshake_timeout;

	mcc_trace(IPCM_DBG_LEVEL, "local_cpu_id = %d\n", cpuid);

	/* Init ipcm device */
	ipc_dev = &hisi_ipc_dev;
	ipc_dev->local_cpu_id     = cpuid;
	ipc_dev->gic_cpu_inf_base = CFG_GIC_CPU_BASE;
	ipc_dev->gic_dist_base    = CFG_GIC_DIST_BASE;
	ipc_dev->data_reg_base    = IPC_DATA_REG_BASE;
	ipc_dev->irq              = IPC_GIC_IRQ;
	ipc_dev->remote_map_flag  = 0;
	ipc_dev->sended_irqs	  = 0;
	ipc_dev->received_irqs	  = 0;

#ifdef CONFIG_HIIPCM_IRQ_STATISTIC_ENABLE
	/* create and add the kobj for hi-ipcm */
	hi_ipcm_kobj = kobject_create_and_add("hi-ipcm", NULL);
	if (NULL == hi_ipcm_kobj) {
		mcc_trace(IPCM_ERR_LEVEL, "create and add kobj failed!\n");
		return -1;
	}

	if (sysfs_create_group(hi_ipcm_kobj, &attr_group)) {
		kobject_put(hi_ipcm_kobj);
		mcc_trace(IPCM_ERR_LEVEL, "create sysfs file failed!\n");
		return -1;
	}
#endif

	/* Init timer */
	init_timer(&(ipc_dev->timer));

	/* Init shared memory */
	hisi_ipcm_shared_mem_init();

	/* Register interrupt process function */
	ipc_irq.irq    = ipc_dev->irq;
	ipc_irq.handle = hisi_ipcm_handle_irq;
	ipc_irq_register(&ipc_irq);

	/* The slave cpu send initial data to the master cpu */
	if (ipc_dev->local_cpu_id  & 0x1) {
		ipcm_handshake_timeout = jiffies
			+ CONFIG_HIIPCM_WAIT_TIMEOUT * HZ;
		while (1) {

			flag_value = readl((void *)ipc_dev->data_reg_base
					+ GET_DATA_REG_OFFSET(A7_CPU_INTRF, 3));

			if (flag_value & IPCM_HANDSHAKE_FLAG) {
				ipcm_send_initial_data();
				break;
			}

			udelay(10);

			if (!time_before(jiffies, ipcm_handshake_timeout)) {
				mcc_trace(IPCM_ERR_LEVEL,
					"Wait A17 handshake flag is timeout!\n");
				return -1;
			}

		}
	} else
		/* Set the handshake flag */
		writel(IPCM_HANDSHAKE_FLAG,
				(void *)ipc_dev->data_reg_base
				+ GET_DATA_REG_OFFSET(A7_CPU_INTRF, 3));

	mcc_trace(IPCM_INFO_LEVEL, "IPCM hardware initialized successfully!\n");
	return 0;
}

module_init(ipcm_vdd_init);
module_exit(ipcm_vdd_cleanup);

module_param(cpuid, uint, 0);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("chendazheng");
MODULE_DESCRIPTION("IPCM communicate vdd layer for Hisilicon IPCM Communicate Solution");
