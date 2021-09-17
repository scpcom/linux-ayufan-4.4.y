#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <linux/hisi_ipcm_data_common.h>
#include <linux/hisi_ipcm_proto.h>
#include <linux/hios_type.h>

/*spinlock for trigger interrupt*/
DEFINE_SPINLOCK(spin_lock_ipc);

/*shared memory allocation information*/
struct hisi_ipcm_shared_mem_alloc_info hisi_ipc_shared_mem_info;

/*external variables*/
extern  hisi_ipc_dev_t hisi_ipc_dev;

/*******************************************************************
 **function: write message data to shared memory
 **param:	w_offset: write pointer offset;
		base:     shared memory base address;
		buf:      message data buffer;
		head:     message head;
		len:      message length.
 **return:   the next write pointer offset
 *******************************************************************/
static inline unsigned long _ipcm_memcpy_shbuf(unsigned long w_offset,
				unsigned long base,
				const void *buf,
				struct hisi_ipcm_transfer_head *head,
				unsigned int len)
{
	unsigned int next_wp;
	unsigned int len_altered;

	/*caculate next wp_addr*/
	len_altered = LEN_DIGHT(len);
	next_wp = w_offset + len_altered + _IPCM_HEAD_SIZE;

	/*Write the message*/
	memcpy((void *)(base + HISI_IPCM_SHARED_USEED_SIZE
			+ w_offset - _IPCM_HEAD_SIZE),
		(void *)head, _IPCM_HEAD_SIZE);
	memcpy((void *)(base + HISI_IPCM_SHARED_USEED_SIZE + w_offset),
		(void *)buf, len);
	mcc_trace(IPCM_INFO_LEVEL, "next_wp%u\n", next_wp);
	return next_wp;
}

/***********************************************************
**function:	Calculate the location to store message, according
		to w_offset and r_offset, and write the message.
**param:	p: shared memory info;
		buf:  message buffer;
		len:  message length;
		head: message head.
**
***********************************************************/
static int _ipcm_write_to_shmbuf(struct hisi_ipcm_memory_info *p,
			const void *buf,
			unsigned int len,
			struct hisi_ipcm_transfer_head *head)
{
	int ret;
	int len_altered;
	unsigned int r_offset;
	unsigned int w_offset;
	unsigned long next_wp;
	unsigned long base = p->base_addr;
	struct hisi_ipcm_transfer_head mark;

	r_offset = *(p->rp_addr);
	w_offset = *(p->wp_addr);
	len_altered = LEN_DIGHT(len);

	mcc_trace(IPCM_INFO_LEVEL, "w_offset %u\t r_offset %u\n",
			w_offset, r_offset);
	mcc_trace(IPCM_INFO_LEVEL, "len_altered %d\n", len_altered);

	if (w_offset >= r_offset) {
		/*There is enough space at the bottom of shared memory*/
		if ((base + HISI_IPCM_SHARED_USEED_SIZE + w_offset)
			+ (len_altered + _IPCM_HEAD_SIZE) < p->end_addr) {

			/*Write the message to shared memory*/
			next_wp = _ipcm_memcpy_shbuf(w_offset,
					base, buf, head, len);

			/*Save new wp_offset*/
			*(p->wp_addr) = next_wp;

			ret = len;
		} else {
			/*There is not enough space at the bottom,*
			 *need to check the top*/
			if (r_offset > len_altered + _IPCM_HEAD_SIZE) {
				/*There is enough space at the top*/
				mcc_trace(IPCM_INFO_LEVEL,
					"wp >= rp but jump len %d\n",
					len_altered);

				/*Write the jump flag at the bottom*/
				mark = *head;
				mark.magic = HISI_IPC_HEAD_MAGIC_JUMP;
				memcpy((void *)(base
					+ HISI_IPCM_SHARED_USEED_SIZE
					+ w_offset - _IPCM_HEAD_SIZE),
					&mark, _IPCM_HEAD_SIZE);

				next_wp = _ipcm_memcpy_shbuf(0, base,
						buf, head, len);
				*(p->wp_addr) = next_wp;

				ret = len;
			} else {
				/*There is not enough space at the top,*
				 *return -1*/
				mcc_trace(IPCM_ERR_LEVEL,
					"Do not have enough space at the top!\n");
				ret = -1;
			}
		}
	} else {
		/*read pointer larger than write pointer*/
		if ((r_offset - w_offset) > (len_altered + _IPCM_HEAD_SIZE)) {
			mcc_trace(IPCM_INFO_LEVEL,
				"rp > wp and have enough memory!len_altered %d\n",
				len_altered);
			next_wp = _ipcm_memcpy_shbuf(w_offset,
					base, buf, head, len);
			*(p->wp_addr) = next_wp;
			ret = len;
		} else {
			mcc_trace(IPCM_ERR_LEVEL,
				"Do not have enough space in the share memory!\n");
			ret = -1;
		}
	}

	mcc_trace(IPCM_INFO_LEVEL, "return ret %d\n", ret);
	return ret;
}

/**************************************************
 **function:	Trigger the interrupt, send the message
		to the remote.
 **param:    mem_addr: shared memory information
 **return:   void
**************************************************/
static void  _ipcm_send_data(unsigned long mem_addr)
{
	unsigned int core_id;
	unsigned int cpu_intrf;
	unsigned int intr_triger;
	unsigned int mem_flag;
	unsigned long flags;
	unsigned long ipcm_send_timeout;
	hisi_ipc_dev_t  *ipc_dev;
	struct hisi_ipcm_memory_info *p;

	p = (struct hisi_ipcm_memory_info *)mem_addr;
	ipc_dev = &hisi_ipc_dev;

	/*Get the corresponding gic cpu interface*/
	core_id = smp_processor_id();
	if (ipc_dev->local_cpu_id)
		cpu_intrf = A7_CPU_INTRF;
	else
		cpu_intrf = core_id;

	/*Write the mem_flag and wp_offset to data registers*/
	ipcm_send_timeout = jiffies
		+ CONFIG_HIIPCM_WAIT_TIMEOUT * HZ;
	while (1) {
		spin_lock_irqsave(&spin_lock_ipc, flags);

		mem_flag = readl((void *)(ipc_dev->data_reg_base
				+ GET_DATA_REG_OFFSET(cpu_intrf, 0)));
		if (!(mem_flag & MEM_FLG_BUSY_MASK)) {
			writel((*p->flag_addr) | MEM_FLG_BUSY_MASK,
				(void *)(ipc_dev->data_reg_base
					+ GET_DATA_REG_OFFSET(cpu_intrf, 0)));
			break;
		} else {
			spin_unlock_irqrestore(&spin_lock_ipc, flags);
			udelay(10);
		}

		if (!time_before(jiffies, ipcm_send_timeout)) {
			mcc_trace(IPCM_ERR_LEVEL,
					"Wait mem flag to be free is timeout!\n");
			return;
		}
	}

	writel((*p->wp_addr),
		(void *)(ipc_dev->data_reg_base
			+ GET_DATA_REG_OFFSET(cpu_intrf, 1)));

	/*Write the gic soft intrrupt register, trigger the interrupt*/
	if (ipc_dev->local_cpu_id & 0x1)
		intr_triger = GIC_CPU_INTR0_MAP | ipc_dev->irq;
	else
		intr_triger = GIC_CPU_INTR4_MAP | ipc_dev->irq;

	writel(intr_triger,
		(void *)(ipc_dev->gic_dist_base + GIC_DIST_SOFTINT));

#ifdef CONFIG_HIIPCM_IRQ_STATISTIC_ENABLE
	/* increments the number of sended irq */
	ipc_dev->sended_irqs++;
#endif

	spin_unlock_irqrestore(&spin_lock_ipc, flags);

	mcc_trace(IPCM_INFO_LEVEL,
		"already send irq, r_offset: %lu\t w_offset :%lu\n",
		*p->rp_addr, *p->wp_addr);

}

/*********************************************************
 **function:	Send message in the interrupt context.
 **param:	buf:  message buffer;
		len:  message length;
		head: message head.
 **return: 0: success; -1: failed.
*********************************************************/
int hisi_ipcm_send_irq_data(const void *buf,
			unsigned int len,
			struct hisi_ipcm_transfer_head *head)
{
	unsigned long flag;
	unsigned int core_id;
	unsigned int ret;
	hisi_ipc_dev_t  *ipc_dev;
	struct hisi_ipcm_memory_info *p_irq_mem;
	struct hisi_ipcm_shared_mem_alloc_info *pinfo;

	/*Get the corresponding shared memory information*/
	ipc_dev = &hisi_ipc_dev;
	pinfo = &hisi_ipc_shared_mem_info;
	core_id = smp_processor_id();
	p_irq_mem = &(pinfo->irq_mem[(ipc_dev->local_cpu_id << 2) + core_id]);

	/*Write message to the shared memory*/
	local_irq_save(flag);
	ret = _ipcm_write_to_shmbuf(p_irq_mem, buf, len, head);
	local_irq_restore(flag);
	if (-1 == ret) {
		mcc_trace(IPCM_ERR_LEVEL,
			"Write message to shared memory failed!\n");
		return -1;
	}

	/*Trigger interrupt*/
	_ipcm_send_data((unsigned long)p_irq_mem);

	return 0;
}

/********************************************************************
 **function:	Send the message in thread context.
 **param:	handle: ipc handle;
		buf:    message buffer;
		len:    message length;
		head:   message head.
 **return:  0: success; -1: failed.
********************************************************************/
int hisi_ipcm_send_thread_data(struct hisi_ipc_transfer_handle *handle,
				const void *buf,
				unsigned int len,
				struct hisi_ipcm_transfer_head *head)
{
	unsigned int ret;
	unsigned int core_id;
	hisi_ipc_dev_t  *ipc_dev;
	struct hisi_ipcm_memory_info *p = NULL;
	struct hisi_ipcm_shared_mem_alloc_info *pinfo;

	/*Get the corresponding shared memory information*/
	ipc_dev = &hisi_ipc_dev;
	pinfo = &hisi_ipc_shared_mem_info;
	core_id = smp_processor_id();
	p = &pinfo->thread_mem[(ipc_dev->local_cpu_id << 2) + core_id];

	/*Write the message to shared memory*/
	down(&p->shared_handle_sem);
	ret = _ipcm_write_to_shmbuf(p, buf, len, head);
	up(&p->shared_handle_sem);

	/*There is not space in the shread memory, need to send the message*
	*already in the shared memory and return -1*/
	if (-1 == ret) {
		if (timer_pending(&(ipc_dev->timer))) {
			mcc_trace(IPCM_INFO_LEVEL,
				"remain buffer null del timer!\n");
			del_timer(&(ipc_dev->timer));
			_ipcm_send_data((unsigned long)p);
		} else {
			mcc_trace(IPCM_INFO_LEVEL,
				"before normal send set timer!\n");
			ipc_dev->timer.expires = jiffies
				+ msecs_to_jiffies(IPCM_TIMEOUT_IN_THREAD_AREA);
			ipc_dev->timer.data = (unsigned long)p;
			ipc_dev->timer.function = _ipcm_send_data;
			add_timer(&ipc_dev->timer);
		}

		return -1;
	}

	/*If the message is priority or for initial, need to *
	*be sent imediately*/
	if ((handle->priority & HISI_IPCM_PRIORITY_MSG)
		|| (head->magic == HISI_IPC_HEAD_MAGIC_INITIAL)) {
		if (timer_pending(&ipc_dev->timer)) {
			mcc_trace(IPCM_INFO_LEVEL,
				"priority or initial, del timer!\n");
			del_timer(&ipc_dev->timer);
		}

		_ipcm_send_data((unsigned long)p);
	} else {
		/*Normal message need to set timer*/
		if (!timer_pending(&ipc_dev->timer)) {
			mcc_trace(IPCM_INFO_LEVEL, "normal send set timer!\n");
			ipc_dev->timer.expires = jiffies
				+ msecs_to_jiffies(IPCM_TIMEOUT_IN_THREAD_AREA);
			ipc_dev->timer.data = (unsigned long)p;
			ipc_dev->timer.function = _ipcm_send_data;
			add_timer(&ipc_dev->timer);
		}
	}

	return 0;
}

/***********************************************************************
 **function: Init the shared memory.
 **param:    void
 **return:   0:success
***********************************************************************/
int hisi_ipcm_shared_mem_init(void)
{
	unsigned int core_index;
	struct hisi_ipcm_memory_info *p_irq_mem;
	struct hisi_ipcm_memory_info *p_thr_mem;
	struct hisi_ipcm_shared_mem_alloc_info *pinfo;

	pinfo = &hisi_ipc_shared_mem_info;
	pinfo->share_mem_start_addr = (unsigned int)
			ioremap(SHARE_MEM_START_ADDR,
				SHARE_MEM_TOTAL_SIZE);

	/*Init the shared memory of A17*/
	for (core_index = 0;
		core_index < (HISI_IPC_MAX_CORE_NUM - 1);
		core_index++) {

		/*Init irq shared memory*/
		p_irq_mem = &(pinfo->irq_mem[core_index]);
		p_irq_mem->base_addr = pinfo->share_mem_start_addr
				+ core_index * (A17_CORE_IRQ_SHARED_MEM_SIZE
				+ A17_CORE_THR_SHARED_MEM_SIZE);
		p_irq_mem->end_addr = p_irq_mem->base_addr
				+ A17_CORE_IRQ_SHARED_MEM_SIZE;
		p_irq_mem->buf_len  = A17_CORE_IRQ_SHARED_MEM_SIZE
				- HISI_IPCM_SHARED_USEED_SIZE;
		p_irq_mem->buf_remain = p_irq_mem->buf_len;

		p_irq_mem->rp_addr   = (unsigned long *)p_irq_mem->base_addr;
		p_irq_mem->wp_addr   = (unsigned long *)(
				(unsigned int)p_irq_mem->rp_addr
				+ sizeof(unsigned int));
		p_irq_mem->flag_addr = (unsigned long *)(
				(unsigned int)p_irq_mem->wp_addr
				+ sizeof(unsigned int));
		sema_init(&p_irq_mem->shared_handle_sem, 1);
		p_irq_mem->last_recv_w_offset = 0;

		*(p_irq_mem->rp_addr)   = 0;
		*(p_irq_mem->wp_addr)   = 0;
		*(p_irq_mem->flag_addr) = IS_IRQ_SHARED_MEM;

		/*Init thread  shared memory*/
		p_thr_mem = &(pinfo->thread_mem[core_index]);
		p_thr_mem->base_addr  = pinfo->share_mem_start_addr
				+ A17_CORE_IRQ_SHARED_MEM_SIZE
				+ core_index * (A17_CORE_IRQ_SHARED_MEM_SIZE
				+ A17_CORE_THR_SHARED_MEM_SIZE);
		p_thr_mem->end_addr   = p_thr_mem->base_addr
			+ A17_CORE_THR_SHARED_MEM_SIZE;
		p_thr_mem->buf_len    = A17_CORE_THR_SHARED_MEM_SIZE
			- HISI_IPCM_SHARED_USEED_SIZE;
		p_thr_mem->buf_remain = p_thr_mem->buf_len;

		p_thr_mem->rp_addr   = (unsigned long *)p_thr_mem->base_addr;
		p_thr_mem->wp_addr   = (unsigned long *)(
				(unsigned int)p_thr_mem->rp_addr
				+ sizeof(unsigned int));
		p_thr_mem->flag_addr = (unsigned long *)(
				(unsigned int)p_thr_mem->wp_addr
				+ sizeof(unsigned int));
		sema_init(&p_thr_mem->shared_handle_sem, 1);
		p_thr_mem->last_recv_w_offset = 0;

		*(p_thr_mem->rp_addr)   = 0;
		*(p_thr_mem->wp_addr)   = 0;
		*(p_thr_mem->flag_addr) = IS_THR_SHARED_MEM;
	}

	/*Init the shared memory of A7*/
	/*1. Init irq shared memory*/
	p_irq_mem = &(pinfo->irq_mem[HISI_IPC_MAX_CORE_NUM - 1]);
	p_irq_mem->base_addr  = pinfo->share_mem_start_addr
				+ core_index
				* (A17_CORE_IRQ_SHARED_MEM_SIZE
				+ A17_CORE_THR_SHARED_MEM_SIZE);
	p_irq_mem->end_addr   = p_irq_mem->base_addr
			+ A7_CORE_IRQ_SHARED_MEM_SIZE;
	p_irq_mem->buf_len    = A7_CORE_IRQ_SHARED_MEM_SIZE
			- HISI_IPCM_SHARED_USEED_SIZE;
	p_irq_mem->buf_remain = p_irq_mem->buf_len;

	p_irq_mem->rp_addr   = (unsigned long *)p_irq_mem->base_addr;
	p_irq_mem->wp_addr   = (unsigned long *)(
			(unsigned int)p_irq_mem->rp_addr
			+ sizeof(unsigned int));
	p_irq_mem->flag_addr = (unsigned long *)(
			(unsigned int)p_irq_mem->wp_addr
			+ sizeof(unsigned int));
	sema_init(&p_irq_mem->shared_handle_sem, 1);
	p_irq_mem->last_recv_w_offset = 0;

	*(p_irq_mem->rp_addr)   = 0;
	*(p_irq_mem->wp_addr)   = 0;
	*(p_irq_mem->flag_addr) = IS_IRQ_SHARED_MEM;

	/*2. Init thread shared memory*/
	p_thr_mem = &(pinfo->thread_mem[HISI_IPC_MAX_CORE_NUM - 1]);
	p_thr_mem->base_addr  = p_irq_mem->end_addr;
	p_thr_mem->end_addr   = p_thr_mem->base_addr
				+ A7_CORE_THR_SHARED_MEM_SIZE;
	p_thr_mem->buf_len    = A7_CORE_THR_SHARED_MEM_SIZE
				- HISI_IPCM_SHARED_USEED_SIZE;
	p_thr_mem->buf_remain = p_thr_mem->buf_len;

	p_thr_mem->rp_addr   = (unsigned long *)p_thr_mem->base_addr;
	p_thr_mem->wp_addr   = (unsigned long *)(
			(unsigned int)p_thr_mem->rp_addr
			+ sizeof(unsigned int));
	p_thr_mem->flag_addr = (unsigned long *)(
			(unsigned int)p_thr_mem->wp_addr
			+ sizeof(unsigned int));
	sema_init(&p_thr_mem->shared_handle_sem, 1);
	p_thr_mem->last_recv_w_offset = 0;

	*(p_thr_mem->rp_addr)   = 0;
	*(p_thr_mem->wp_addr)   = 0;
	*(p_thr_mem->flag_addr) = IS_THR_SHARED_MEM;
	return 0;
}
