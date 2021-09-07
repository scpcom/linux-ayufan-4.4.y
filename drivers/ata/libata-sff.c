#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/libata.h>
#include <linux/highmem.h>

#include "libata.h"

const struct ata_port_operations ata_sff_port_ops = {
	.inherits		= &ata_base_port_ops,

	.qc_prep		= ata_sff_qc_prep,
	.qc_issue		= ata_sff_qc_issue,
	.qc_fill_rtf		= ata_sff_qc_fill_rtf,

	.freeze			= ata_sff_freeze,
	.thaw			= ata_sff_thaw,
	.prereset		= ata_sff_prereset,
	.softreset		= ata_sff_softreset,
	.hardreset		= sata_sff_hardreset,
	.postreset		= ata_sff_postreset,
	.drain_fifo		= ata_sff_drain_fifo,
	.error_handler		= ata_sff_error_handler,
	.post_internal_cmd	= ata_sff_post_internal_cmd,

	.sff_dev_select		= ata_sff_dev_select,
	.sff_check_status	= ata_sff_check_status,
	.sff_tf_load		= ata_sff_tf_load,
	.sff_tf_read		= ata_sff_tf_read,
	.sff_exec_command	= ata_sff_exec_command,
	.sff_data_xfer		= ata_sff_data_xfer,
	.sff_irq_on		= ata_sff_irq_on,
	.sff_irq_clear		= ata_sff_irq_clear,

	.lost_interrupt		= ata_sff_lost_interrupt,

	.port_start		= ata_sff_port_start,
};
EXPORT_SYMBOL_GPL(ata_sff_port_ops);

const struct ata_port_operations ata_bmdma_port_ops = {
	.inherits		= &ata_sff_port_ops,

	.mode_filter		= ata_bmdma_mode_filter,

	.bmdma_setup		= ata_bmdma_setup,
	.bmdma_start		= ata_bmdma_start,
	.bmdma_stop		= ata_bmdma_stop,
	.bmdma_status		= ata_bmdma_status,
};
EXPORT_SYMBOL_GPL(ata_bmdma_port_ops);

const struct ata_port_operations ata_bmdma32_port_ops = {
	.inherits		= &ata_bmdma_port_ops,

	.sff_data_xfer		= ata_sff_data_xfer32,
	.port_start		= ata_sff_port_start32,
};
EXPORT_SYMBOL_GPL(ata_bmdma32_port_ops);

static void ata_fill_sg(struct ata_queued_cmd *qc)
{
	struct ata_port *ap = qc->ap;
	struct scatterlist *sg;
	unsigned int si, pi;

	pi = 0;
	for_each_sg(qc->sg, sg, qc->n_elem, si) {
		u32 addr, offset;
		u32 sg_len, len;

		addr = (u32) sg_dma_address(sg);
		sg_len = sg_dma_len(sg);

		while (sg_len) {
			offset = addr & 0xffff;
			len = sg_len;
			if ((offset + sg_len) > 0x10000)
				len = 0x10000 - offset;

			ap->prd[pi].addr = cpu_to_le32(addr);
			ap->prd[pi].flags_len = cpu_to_le32(len & 0xffff);
			VPRINTK("PRD[%u] = (0x%X, 0x%X)\n", pi, addr, len);

			pi++;
			sg_len -= len;
			addr += len;
		}
	}

	ap->prd[pi - 1].flags_len |= cpu_to_le32(ATA_PRD_EOT);
}

static void ata_fill_sg_dumb(struct ata_queued_cmd *qc)
{
	struct ata_port *ap = qc->ap;
	struct scatterlist *sg;
	unsigned int si, pi;

	pi = 0;
	for_each_sg(qc->sg, sg, qc->n_elem, si) {
		u32 addr, offset;
		u32 sg_len, len, blen;

		addr = (u32) sg_dma_address(sg);
		sg_len = sg_dma_len(sg);

		while (sg_len) {
			offset = addr & 0xffff;
			len = sg_len;
			if ((offset + sg_len) > 0x10000)
				len = 0x10000 - offset;

			blen = len & 0xffff;
			ap->prd[pi].addr = cpu_to_le32(addr);
			if (blen == 0) {
				 
				ap->prd[pi].flags_len = cpu_to_le32(0x8000);
				blen = 0x8000;
				ap->prd[++pi].addr = cpu_to_le32(addr + 0x8000);
			}
			ap->prd[pi].flags_len = cpu_to_le32(blen);
			VPRINTK("PRD[%u] = (0x%X, 0x%X)\n", pi, addr, len);

			pi++;
			sg_len -= len;
			addr += len;
		}
	}

	ap->prd[pi - 1].flags_len |= cpu_to_le32(ATA_PRD_EOT);
}

void ata_sff_qc_prep(struct ata_queued_cmd *qc)
{
	if (!(qc->flags & ATA_QCFLAG_DMAMAP))
		return;

	ata_fill_sg(qc);
}
EXPORT_SYMBOL_GPL(ata_sff_qc_prep);

void ata_sff_dumb_qc_prep(struct ata_queued_cmd *qc)
{
	if (!(qc->flags & ATA_QCFLAG_DMAMAP))
		return;

	ata_fill_sg_dumb(qc);
}
EXPORT_SYMBOL_GPL(ata_sff_dumb_qc_prep);

u8 ata_sff_check_status(struct ata_port *ap)
{
	return ioread8(ap->ioaddr.status_addr);
}
EXPORT_SYMBOL_GPL(ata_sff_check_status);

static u8 ata_sff_altstatus(struct ata_port *ap)
{
	if (ap->ops->sff_check_altstatus)
		return ap->ops->sff_check_altstatus(ap);

	return ioread8(ap->ioaddr.altstatus_addr);
}

static u8 ata_sff_irq_status(struct ata_port *ap)
{
	u8 status;

	if (ap->ops->sff_check_altstatus || ap->ioaddr.altstatus_addr) {
		status = ata_sff_altstatus(ap);
		 
		if (status & ATA_BUSY)
			return status;
	}
	 
	status = ap->ops->sff_check_status(ap);
	return status;
}

static void ata_sff_sync(struct ata_port *ap)
{
	if (ap->ops->sff_check_altstatus)
		ap->ops->sff_check_altstatus(ap);
	else if (ap->ioaddr.altstatus_addr)
		ioread8(ap->ioaddr.altstatus_addr);
}

void ata_sff_pause(struct ata_port *ap)
{
	ata_sff_sync(ap);
	ndelay(400);
}
EXPORT_SYMBOL_GPL(ata_sff_pause);

void ata_sff_dma_pause(struct ata_port *ap)
{
	if (ap->ops->sff_check_altstatus || ap->ioaddr.altstatus_addr) {
		 
		ata_sff_altstatus(ap);
		return;
	}
	 
	BUG();
}
EXPORT_SYMBOL_GPL(ata_sff_dma_pause);

int ata_sff_busy_sleep(struct ata_port *ap,
		       unsigned long tmout_pat, unsigned long tmout)
{
	unsigned long timer_start, timeout;
	u8 status;

	status = ata_sff_busy_wait(ap, ATA_BUSY, 300);
	timer_start = jiffies;
	timeout = ata_deadline(timer_start, tmout_pat);
	while (status != 0xff && (status & ATA_BUSY) &&
	       time_before(jiffies, timeout)) {
		msleep(50);
		status = ata_sff_busy_wait(ap, ATA_BUSY, 3);
	}

	if (status != 0xff && (status & ATA_BUSY))
		ata_port_printk(ap, KERN_WARNING,
				"port is slow to respond, please be patient "
				"(Status 0x%x)\n", status);

	timeout = ata_deadline(timer_start, tmout);
	while (status != 0xff && (status & ATA_BUSY) &&
	       time_before(jiffies, timeout)) {
		msleep(50);
		status = ap->ops->sff_check_status(ap);
	}

	if (status == 0xff)
		return -ENODEV;

	if (status & ATA_BUSY) {
		ata_port_printk(ap, KERN_ERR, "port failed to respond "
				"(%lu secs, Status 0x%x)\n",
				DIV_ROUND_UP(tmout, 1000), status);
		return -EBUSY;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(ata_sff_busy_sleep);

static int ata_sff_check_ready(struct ata_link *link)
{
	u8 status = link->ap->ops->sff_check_status(link->ap);

	return ata_check_ready(status);
}

int ata_sff_wait_ready(struct ata_link *link, unsigned long deadline)
{
	return ata_wait_ready(link, deadline, ata_sff_check_ready);
}
EXPORT_SYMBOL_GPL(ata_sff_wait_ready);

void ata_sff_dev_select(struct ata_port *ap, unsigned int device)
{
	u8 tmp;

	if (device == 0)
		tmp = ATA_DEVICE_OBS;
	else
		tmp = ATA_DEVICE_OBS | ATA_DEV1;

	iowrite8(tmp, ap->ioaddr.device_addr);
	ata_sff_pause(ap);	 
}
EXPORT_SYMBOL_GPL(ata_sff_dev_select);

void ata_dev_select(struct ata_port *ap, unsigned int device,
			   unsigned int wait, unsigned int can_sleep)
{
	if (ata_msg_probe(ap))
		ata_port_printk(ap, KERN_INFO, "ata_dev_select: ENTER, "
				"device %u, wait %u\n", device, wait);

	if (wait)
		ata_wait_idle(ap);

	ap->ops->sff_dev_select(ap, device);

	if (wait) {
		if (can_sleep && ap->link.device[device].class == ATA_DEV_ATAPI)
			msleep(150);
		ata_wait_idle(ap);
	}
}

u8 ata_sff_irq_on(struct ata_port *ap)
{
	struct ata_ioports *ioaddr = &ap->ioaddr;
	u8 tmp;

	ap->ctl &= ~ATA_NIEN;
	ap->last_ctl = ap->ctl;

	if (ioaddr->ctl_addr)
		iowrite8(ap->ctl, ioaddr->ctl_addr);
	tmp = ata_wait_idle(ap);

	ap->ops->sff_irq_clear(ap);

	return tmp;
}
EXPORT_SYMBOL_GPL(ata_sff_irq_on);

void ata_sff_irq_clear(struct ata_port *ap)
{
	void __iomem *mmio = ap->ioaddr.bmdma_addr;

	if (!mmio)
		return;

	iowrite8(ioread8(mmio + ATA_DMA_STATUS), mmio + ATA_DMA_STATUS);
}
EXPORT_SYMBOL_GPL(ata_sff_irq_clear);

void ata_sff_tf_load(struct ata_port *ap, const struct ata_taskfile *tf)
{
	struct ata_ioports *ioaddr = &ap->ioaddr;
	unsigned int is_addr = tf->flags & ATA_TFLAG_ISADDR;

	if (tf->ctl != ap->last_ctl) {
		if (ioaddr->ctl_addr)
			iowrite8(tf->ctl, ioaddr->ctl_addr);
		ap->last_ctl = tf->ctl;
		ata_wait_idle(ap);
	}

	if (is_addr && (tf->flags & ATA_TFLAG_LBA48)) {
		WARN_ON_ONCE(!ioaddr->ctl_addr);
		iowrite8(tf->hob_feature, ioaddr->feature_addr);
		iowrite8(tf->hob_nsect, ioaddr->nsect_addr);
		iowrite8(tf->hob_lbal, ioaddr->lbal_addr);
		iowrite8(tf->hob_lbam, ioaddr->lbam_addr);
		iowrite8(tf->hob_lbah, ioaddr->lbah_addr);
		VPRINTK("hob: feat 0x%X nsect 0x%X, lba 0x%X 0x%X 0x%X\n",
			tf->hob_feature,
			tf->hob_nsect,
			tf->hob_lbal,
			tf->hob_lbam,
			tf->hob_lbah);
	}

	if (is_addr) {
		iowrite8(tf->feature, ioaddr->feature_addr);
		iowrite8(tf->nsect, ioaddr->nsect_addr);
		iowrite8(tf->lbal, ioaddr->lbal_addr);
		iowrite8(tf->lbam, ioaddr->lbam_addr);
		iowrite8(tf->lbah, ioaddr->lbah_addr);
		VPRINTK("feat 0x%X nsect 0x%X lba 0x%X 0x%X 0x%X\n",
			tf->feature,
			tf->nsect,
			tf->lbal,
			tf->lbam,
			tf->lbah);
	}

	if (tf->flags & ATA_TFLAG_DEVICE) {
		iowrite8(tf->device, ioaddr->device_addr);
		VPRINTK("device 0x%X\n", tf->device);
	}

	ata_wait_idle(ap);
}
EXPORT_SYMBOL_GPL(ata_sff_tf_load);

void ata_sff_tf_read(struct ata_port *ap, struct ata_taskfile *tf)
{
	struct ata_ioports *ioaddr = &ap->ioaddr;

	tf->command = ata_sff_check_status(ap);
	tf->feature = ioread8(ioaddr->error_addr);
	tf->nsect = ioread8(ioaddr->nsect_addr);
	tf->lbal = ioread8(ioaddr->lbal_addr);
	tf->lbam = ioread8(ioaddr->lbam_addr);
	tf->lbah = ioread8(ioaddr->lbah_addr);
	tf->device = ioread8(ioaddr->device_addr);

	if (tf->flags & ATA_TFLAG_LBA48) {
		if (likely(ioaddr->ctl_addr)) {
			iowrite8(tf->ctl | ATA_HOB, ioaddr->ctl_addr);
			tf->hob_feature = ioread8(ioaddr->error_addr);
			tf->hob_nsect = ioread8(ioaddr->nsect_addr);
			tf->hob_lbal = ioread8(ioaddr->lbal_addr);
			tf->hob_lbam = ioread8(ioaddr->lbam_addr);
			tf->hob_lbah = ioread8(ioaddr->lbah_addr);
			iowrite8(tf->ctl, ioaddr->ctl_addr);
			ap->last_ctl = tf->ctl;
		} else
			WARN_ON_ONCE(1);
	}
}
EXPORT_SYMBOL_GPL(ata_sff_tf_read);

void ata_sff_exec_command(struct ata_port *ap, const struct ata_taskfile *tf)
{
	DPRINTK("ata%u: cmd 0x%X\n", ap->print_id, tf->command);

	iowrite8(tf->command, ap->ioaddr.command_addr);
	ata_sff_pause(ap);
}
EXPORT_SYMBOL_GPL(ata_sff_exec_command);

static inline void ata_tf_to_host(struct ata_port *ap,
				  const struct ata_taskfile *tf)
{
	ap->ops->sff_tf_load(ap, tf);
	ap->ops->sff_exec_command(ap, tf);
}

unsigned int ata_sff_data_xfer(struct ata_device *dev, unsigned char *buf,
			       unsigned int buflen, int rw)
{
	struct ata_port *ap = dev->link->ap;
	void __iomem *data_addr = ap->ioaddr.data_addr;
	unsigned int words = buflen >> 1;

	if (rw == READ)
		ioread16_rep(data_addr, buf, words);
	else
		iowrite16_rep(data_addr, buf, words);

	if (unlikely(buflen & 0x01)) {
		unsigned char pad[2];

		buf += buflen - 1;

		if (rw == READ) {
			ioread16_rep(data_addr, pad, 1);
			*buf = pad[0];
		} else {
			pad[0] = *buf;
			iowrite16_rep(data_addr, pad, 1);
		}
		words++;
	}

	return words << 1;
}
EXPORT_SYMBOL_GPL(ata_sff_data_xfer);

unsigned int ata_sff_data_xfer32(struct ata_device *dev, unsigned char *buf,
			       unsigned int buflen, int rw)
{
	struct ata_port *ap = dev->link->ap;
	void __iomem *data_addr = ap->ioaddr.data_addr;
	unsigned int words = buflen >> 2;
	int slop = buflen & 3;
	
	if (!(ap->pflags & ATA_PFLAG_PIO32))
		return ata_sff_data_xfer(dev, buf, buflen, rw);

	if (rw == READ)
		ioread32_rep(data_addr, buf, words);
	else
		iowrite32_rep(data_addr, buf, words);

	if (unlikely(slop)) {
		unsigned char pad[4];

		buf += buflen - slop;

		if (rw == READ) {
			if (slop < 3)
				ioread16_rep(data_addr, pad, 1);
			else
				ioread32_rep(data_addr, pad, 1);
			memcpy(buf, pad, slop);
		} else {
			memcpy(pad, buf, slop);
			if (slop < 3)
				iowrite16_rep(data_addr, pad, 1);
			else
				iowrite32_rep(data_addr, pad, 1);
		}
	}
	return (buflen + 1) & ~1;
}
EXPORT_SYMBOL_GPL(ata_sff_data_xfer32);

unsigned int ata_sff_data_xfer_noirq(struct ata_device *dev, unsigned char *buf,
				     unsigned int buflen, int rw)
{
	unsigned long flags;
	unsigned int consumed;

	local_irq_save(flags);
	consumed = ata_sff_data_xfer(dev, buf, buflen, rw);
	local_irq_restore(flags);

	return consumed;
}
EXPORT_SYMBOL_GPL(ata_sff_data_xfer_noirq);

static void ata_pio_sector(struct ata_queued_cmd *qc)
{
	int do_write = (qc->tf.flags & ATA_TFLAG_WRITE);
	struct ata_port *ap = qc->ap;
	struct page *page;
	unsigned int offset;
	unsigned char *buf;

	if (qc->curbytes == qc->nbytes - qc->sect_size)
		ap->hsm_task_state = HSM_ST_LAST;

	page = sg_page(qc->cursg);
	offset = qc->cursg->offset + qc->cursg_ofs;

	page = nth_page(page, (offset >> PAGE_SHIFT));
	offset %= PAGE_SIZE;

	DPRINTK("data %s\n", qc->tf.flags & ATA_TFLAG_WRITE ? "write" : "read");

	if (PageHighMem(page)) {
		unsigned long flags;

		local_irq_save(flags);
		buf = kmap_atomic(page, KM_IRQ0);

		ap->ops->sff_data_xfer(qc->dev, buf + offset, qc->sect_size,
				       do_write);

		kunmap_atomic(buf, KM_IRQ0);
		local_irq_restore(flags);
	} else {
		buf = page_address(page);
		ap->ops->sff_data_xfer(qc->dev, buf + offset, qc->sect_size,
				       do_write);
	}

	if (!do_write)
		flush_dcache_page(page);

	qc->curbytes += qc->sect_size;
	qc->cursg_ofs += qc->sect_size;

	if (qc->cursg_ofs == qc->cursg->length) {
		qc->cursg = sg_next(qc->cursg);
		qc->cursg_ofs = 0;
	}
}

static void ata_pio_sectors(struct ata_queued_cmd *qc)
{
	if (is_multi_taskfile(&qc->tf)) {
		 
		unsigned int nsect;

		WARN_ON_ONCE(qc->dev->multi_count == 0);

		nsect = min((qc->nbytes - qc->curbytes) / qc->sect_size,
			    qc->dev->multi_count);
		while (nsect--)
			ata_pio_sector(qc);
	} else
		ata_pio_sector(qc);

	ata_sff_sync(qc->ap);  
}

static void atapi_send_cdb(struct ata_port *ap, struct ata_queued_cmd *qc)
{
	 
	DPRINTK("send cdb\n");
	WARN_ON_ONCE(qc->dev->cdb_len < 12);

	ap->ops->sff_data_xfer(qc->dev, qc->cdb, qc->dev->cdb_len, 1);
	ata_sff_sync(ap);
	 
	switch (qc->tf.protocol) {
	case ATAPI_PROT_PIO:
		ap->hsm_task_state = HSM_ST;
		break;
	case ATAPI_PROT_NODATA:
		ap->hsm_task_state = HSM_ST_LAST;
		break;
	case ATAPI_PROT_DMA:
		ap->hsm_task_state = HSM_ST_LAST;
		 
		ap->ops->bmdma_start(qc);
		break;
	}
}

static int __atapi_pio_bytes(struct ata_queued_cmd *qc, unsigned int bytes)
{
	int rw = (qc->tf.flags & ATA_TFLAG_WRITE) ? WRITE : READ;
	struct ata_port *ap = qc->ap;
	struct ata_device *dev = qc->dev;
	struct ata_eh_info *ehi = &dev->link->eh_info;
	struct scatterlist *sg;
	struct page *page;
	unsigned char *buf;
	unsigned int offset, count, consumed;

next_sg:
	sg = qc->cursg;
	if (unlikely(!sg)) {
		ata_ehi_push_desc(ehi, "unexpected or too much trailing data "
				  "buf=%u cur=%u bytes=%u",
				  qc->nbytes, qc->curbytes, bytes);
		return -1;
	}

	page = sg_page(sg);
	offset = sg->offset + qc->cursg_ofs;

	page = nth_page(page, (offset >> PAGE_SHIFT));
	offset %= PAGE_SIZE;

	count = min(sg->length - qc->cursg_ofs, bytes);

	count = min(count, (unsigned int)PAGE_SIZE - offset);

	DPRINTK("data %s\n", qc->tf.flags & ATA_TFLAG_WRITE ? "write" : "read");

	if (PageHighMem(page)) {
		unsigned long flags;

		local_irq_save(flags);
		buf = kmap_atomic(page, KM_IRQ0);

		consumed = ap->ops->sff_data_xfer(dev,  buf + offset,
								count, rw);

		kunmap_atomic(buf, KM_IRQ0);
		local_irq_restore(flags);
	} else {
		buf = page_address(page);
		consumed = ap->ops->sff_data_xfer(dev,  buf + offset,
								count, rw);
	}

	bytes -= min(bytes, consumed);
	qc->curbytes += count;
	qc->cursg_ofs += count;

	if (qc->cursg_ofs == sg->length) {
		qc->cursg = sg_next(qc->cursg);
		qc->cursg_ofs = 0;
	}

	if (bytes)
		goto next_sg;
	return 0;
}

static void atapi_pio_bytes(struct ata_queued_cmd *qc)
{
	struct ata_port *ap = qc->ap;
	struct ata_device *dev = qc->dev;
	struct ata_eh_info *ehi = &dev->link->eh_info;
	unsigned int ireason, bc_lo, bc_hi, bytes;
	int i_write, do_write = (qc->tf.flags & ATA_TFLAG_WRITE) ? 1 : 0;

	ap->ops->sff_tf_read(ap, &qc->result_tf);
	ireason = qc->result_tf.nsect;
	bc_lo = qc->result_tf.lbam;
	bc_hi = qc->result_tf.lbah;
	bytes = (bc_hi << 8) | bc_lo;

	if (unlikely(ireason & (1 << 0)))
		goto atapi_check;

	i_write = ((ireason & (1 << 1)) == 0) ? 1 : 0;
	if (unlikely(do_write != i_write))
		goto atapi_check;

	if (unlikely(!bytes))
		goto atapi_check;

	VPRINTK("ata%u: xfering %d bytes\n", ap->print_id, bytes);

	if (unlikely(__atapi_pio_bytes(qc, bytes)))
		goto err_out;
	ata_sff_sync(ap);  

	return;

 atapi_check:
	ata_ehi_push_desc(ehi, "ATAPI check failed (ireason=0x%x bytes=%u)",
			  ireason, bytes);
 err_out:
	qc->err_mask |= AC_ERR_HSM;
	ap->hsm_task_state = HSM_ST_ERR;
}

static inline int ata_hsm_ok_in_wq(struct ata_port *ap,
						struct ata_queued_cmd *qc)
{
	if (qc->tf.flags & ATA_TFLAG_POLLING)
		return 1;

	if (ap->hsm_task_state == HSM_ST_FIRST) {
		if (qc->tf.protocol == ATA_PROT_PIO &&
		   (qc->tf.flags & ATA_TFLAG_WRITE))
		    return 1;

		if (ata_is_atapi(qc->tf.protocol) &&
		   !(qc->dev->flags & ATA_DFLAG_CDB_INTR))
			return 1;
	}

	return 0;
}

static void ata_hsm_qc_complete(struct ata_queued_cmd *qc, int in_wq)
{
	struct ata_port *ap = qc->ap;
	unsigned long flags;

	if (ap->ops->error_handler) {
		if (in_wq) {
			spin_lock_irqsave(ap->lock, flags);

			qc = ata_qc_from_tag(ap, qc->tag);
			if (qc) {
#if defined(MY_ABC_HERE)
				if (IS_SYNO_SPINUP_CMD(qc)) {
					ap->ops->sff_irq_on(ap);
					 
					if (qc->err_mask || 
						qc->flags & ATA_QCFLAG_RESULT_TF || 
						qc->flags & ATA_QCFLAG_FAILED) {
						qc->result_tf.flags = qc->tf.flags;
						ap->ops->qc_fill_rtf(qc);
					}
					__ata_qc_complete(qc);
				} else if (likely(!(qc->err_mask & AC_ERR_HSM))) {
#else
				if (likely(!(qc->err_mask & AC_ERR_HSM))) {
#endif
					ap->ops->sff_irq_on(ap);
					ata_qc_complete(qc);
#ifdef MY_ABC_HERE
				} else {
					if (NULL == qc->scsicmd && !ata_tag_internal(qc->tag)) {
						DBGMESG("disk %d:its our insert cmd,don't freeze. cmd 0x%x tag %d feature 0x%x\n",
								qc->ap->print_id, qc->tf.command, qc->tag, qc->tf.feature);
						__ata_qc_complete(qc);
					} else {
						ata_port_freeze(ap);
					}
				}
#else
				} else
					ata_port_freeze(ap);
#endif
			}

			spin_unlock_irqrestore(ap->lock, flags);
		} else {
#if defined(MY_ABC_HERE)
			if (IS_SYNO_SPINUP_CMD(qc)) {
				 
				if (qc->err_mask || 
					qc->flags & ATA_QCFLAG_RESULT_TF || 
					qc->flags & ATA_QCFLAG_FAILED) {
					qc->result_tf.flags = qc->tf.flags;
					ap->ops->qc_fill_rtf(qc);
				}
				__ata_qc_complete(qc);
			} else if (likely(!(qc->err_mask & AC_ERR_HSM))) 
#else
			if (likely(!(qc->err_mask & AC_ERR_HSM)))
#endif
				ata_qc_complete(qc);
#ifdef MY_ABC_HERE
			else {
				if (NULL == qc->scsicmd && !ata_tag_internal(qc->tag)) {
					DBGMESG("disk %d:its our insert cmd,don't freeze. cmd 0x%x tag %d feature 0x%x\n",
							qc->ap->print_id, qc->tf.command, qc->tag, qc->tf.feature);
					__ata_qc_complete(qc);
				} else {
					ata_port_freeze(ap);
				}
			}
#else
			else
				ata_port_freeze(ap);
#endif
		}
	} else {
		if (in_wq) {
			spin_lock_irqsave(ap->lock, flags);
			ap->ops->sff_irq_on(ap);
			ata_qc_complete(qc);
			spin_unlock_irqrestore(ap->lock, flags);
		} else
			ata_qc_complete(qc);
	}
}

int ata_sff_hsm_move(struct ata_port *ap, struct ata_queued_cmd *qc,
		     u8 status, int in_wq)
{
	struct ata_eh_info *ehi = &ap->link.eh_info;
#if defined(MY_ABC_HERE) || defined(MY_ABC_HERE)
	struct ata_taskfile *tf = &qc->tf;
#endif
	unsigned long flags = 0;
	int poll_next;

#if defined(MY_ABC_HERE) || defined(MY_ABC_HERE)
	 
	if (!(IS_SYNO_PMP_CMD(tf) || IS_SYNO_SPINUP_CMD(qc))) {
		WARN_ON_ONCE((qc->flags & ATA_QCFLAG_ACTIVE) == 0);

		WARN_ON_ONCE(in_wq != ata_hsm_ok_in_wq(ap, qc));
	}
#else

	WARN_ON_ONCE((qc->flags & ATA_QCFLAG_ACTIVE) == 0);
	WARN_ON_ONCE(in_wq != ata_hsm_ok_in_wq(ap, qc));
#endif

fsm_start:
	DPRINTK("ata%u: protocol %d task_state %d (dev_stat 0x%X)\n",
		ap->print_id, qc->tf.protocol, ap->hsm_task_state, status);

	switch (ap->hsm_task_state) {
	case HSM_ST_FIRST:
		 
		poll_next = (qc->tf.flags & ATA_TFLAG_POLLING);

		if (unlikely((status & ATA_DRQ) == 0)) {
			 
			if (likely(status & (ATA_ERR | ATA_DF)))
				 
				qc->err_mask |= AC_ERR_DEV;
			else {
				 
				ata_ehi_push_desc(ehi,
					"ST_FIRST: !(DRQ|ERR|DF)");
				qc->err_mask |= AC_ERR_HSM;
			}

			ap->hsm_task_state = HSM_ST_ERR;
			goto fsm_start;
		}

		if (unlikely(status & (ATA_ERR | ATA_DF))) {
			 
			if (!(qc->dev->horkage & ATA_HORKAGE_STUCK_ERR)) {
				ata_ehi_push_desc(ehi, "ST_FIRST: "
					"DRQ=1 with device error, "
					"dev_stat 0x%X", status);
				qc->err_mask |= AC_ERR_HSM;
				ap->hsm_task_state = HSM_ST_ERR;
				goto fsm_start;
			}
		}

		if (in_wq)
			spin_lock_irqsave(ap->lock, flags);

		if (qc->tf.protocol == ATA_PROT_PIO) {
			 
			ap->hsm_task_state = HSM_ST;
			ata_pio_sectors(qc);
		} else
			 
			atapi_send_cdb(ap, qc);

		if (in_wq)
			spin_unlock_irqrestore(ap->lock, flags);

		break;

	case HSM_ST:
		 
		if (qc->tf.protocol == ATAPI_PROT_PIO) {
			 
			if ((status & ATA_DRQ) == 0) {
				 
				ap->hsm_task_state = HSM_ST_LAST;
				goto fsm_start;
			}

			if (unlikely(status & (ATA_ERR | ATA_DF))) {
				ata_ehi_push_desc(ehi, "ST-ATAPI: "
					"DRQ=1 with device error, "
					"dev_stat 0x%X", status);
				qc->err_mask |= AC_ERR_HSM;
				ap->hsm_task_state = HSM_ST_ERR;
				goto fsm_start;
			}

			atapi_pio_bytes(qc);

			if (unlikely(ap->hsm_task_state == HSM_ST_ERR))
				 
				goto fsm_start;

		} else {
			 
			if (unlikely((status & ATA_DRQ) == 0)) {
				 
				if (likely(status & (ATA_ERR | ATA_DF))) {
					 
					qc->err_mask |= AC_ERR_DEV;

					if (qc->dev->horkage &
					    ATA_HORKAGE_DIAGNOSTIC)
						qc->err_mask |=
							AC_ERR_NODEV_HINT;
				} else {
					 
					ata_ehi_push_desc(ehi, "ST-ATA: "
						"DRQ=0 without device error, "
						"dev_stat 0x%X", status);
					qc->err_mask |= AC_ERR_HSM |
							AC_ERR_NODEV_HINT;
				}

				ap->hsm_task_state = HSM_ST_ERR;
				goto fsm_start;
			}

			if (unlikely(status & (ATA_ERR | ATA_DF))) {
				 
				qc->err_mask |= AC_ERR_DEV;

				if (!(qc->tf.flags & ATA_TFLAG_WRITE)) {
					ata_pio_sectors(qc);
					status = ata_wait_idle(ap);
				}

				if (status & (ATA_BUSY | ATA_DRQ)) {
					ata_ehi_push_desc(ehi, "ST-ATA: "
						"BUSY|DRQ persists on ERR|DF, "
						"dev_stat 0x%X", status);
					qc->err_mask |= AC_ERR_HSM;
				}

				if (status == 0x7f)
					qc->err_mask |= AC_ERR_NODEV_HINT;

				ap->hsm_task_state = HSM_ST_ERR;
				goto fsm_start;
			}

			ata_pio_sectors(qc);

			if (ap->hsm_task_state == HSM_ST_LAST &&
			    (!(qc->tf.flags & ATA_TFLAG_WRITE))) {
				 
				status = ata_wait_idle(ap);
				goto fsm_start;
			}
		}

		poll_next = 1;
		break;

	case HSM_ST_LAST:
		if (unlikely(!ata_ok(status))) {
			qc->err_mask |= __ac_err_mask(status);
			ap->hsm_task_state = HSM_ST_ERR;
			goto fsm_start;
		}

		DPRINTK("ata%u: dev %u command complete, drv_stat 0x%x\n",
			ap->print_id, qc->dev->devno, status);

		WARN_ON_ONCE(qc->err_mask & (AC_ERR_DEV | AC_ERR_HSM));

		ap->hsm_task_state = HSM_ST_IDLE;

		ata_hsm_qc_complete(qc, in_wq);

		poll_next = 0;
		break;

	case HSM_ST_ERR:
		ap->hsm_task_state = HSM_ST_IDLE;

		ata_hsm_qc_complete(qc, in_wq);

		poll_next = 0;
		break;
	default:
		poll_next = 0;
		BUG();
	}

	return poll_next;
}
EXPORT_SYMBOL_GPL(ata_sff_hsm_move);

void ata_pio_task(struct work_struct *work)
{
	struct ata_port *ap =
		container_of(work, struct ata_port, port_task.work);
	struct ata_queued_cmd *qc = ap->port_task_data;
	u8 status;
	int poll_next;

fsm_start:
	WARN_ON_ONCE(ap->hsm_task_state == HSM_ST_IDLE);

	status = ata_sff_busy_wait(ap, ATA_BUSY, 5);
	if (status & ATA_BUSY) {
		msleep(2);
		status = ata_sff_busy_wait(ap, ATA_BUSY, 10);
		if (status & ATA_BUSY) {
			ata_pio_queue_task(ap, qc, ATA_SHORT_PAUSE);
			return;
		}
	}

	poll_next = ata_sff_hsm_move(ap, qc, status, 1);

	if (poll_next)
		goto fsm_start;
}

unsigned int ata_sff_qc_issue(struct ata_queued_cmd *qc)
{
	struct ata_port *ap = qc->ap;

	if (ap->flags & ATA_FLAG_PIO_POLLING) {
		switch (qc->tf.protocol) {
		case ATA_PROT_PIO:
		case ATA_PROT_NODATA:
		case ATAPI_PROT_PIO:
		case ATAPI_PROT_NODATA:
			qc->tf.flags |= ATA_TFLAG_POLLING;
			break;
		case ATAPI_PROT_DMA:
			if (qc->dev->flags & ATA_DFLAG_CDB_INTR)
				 
				BUG();
			break;
		default:
			break;
		}
	}

	ata_dev_select(ap, qc->dev->devno, 1, 0);

	switch (qc->tf.protocol) {
	case ATA_PROT_NODATA:
		if (qc->tf.flags & ATA_TFLAG_POLLING)
			ata_qc_set_polling(qc);

		ata_tf_to_host(ap, &qc->tf);
		ap->hsm_task_state = HSM_ST_LAST;

		if (qc->tf.flags & ATA_TFLAG_POLLING)
			ata_pio_queue_task(ap, qc, 0);

		break;

	case ATA_PROT_DMA:
		WARN_ON_ONCE(qc->tf.flags & ATA_TFLAG_POLLING);

		ap->ops->sff_tf_load(ap, &qc->tf);   
		ap->ops->bmdma_setup(qc);	     
		ap->ops->bmdma_start(qc);	     
		ap->hsm_task_state = HSM_ST_LAST;
		break;

	case ATA_PROT_PIO:
		if (qc->tf.flags & ATA_TFLAG_POLLING)
			ata_qc_set_polling(qc);

		ata_tf_to_host(ap, &qc->tf);

		if (qc->tf.flags & ATA_TFLAG_WRITE) {
			 
			ap->hsm_task_state = HSM_ST_FIRST;
			ata_pio_queue_task(ap, qc, 0);

		} else {
			 
			ap->hsm_task_state = HSM_ST;

			if (qc->tf.flags & ATA_TFLAG_POLLING)
				ata_pio_queue_task(ap, qc, 0);

		}

		break;

	case ATAPI_PROT_PIO:
	case ATAPI_PROT_NODATA:
		if (qc->tf.flags & ATA_TFLAG_POLLING)
			ata_qc_set_polling(qc);

		ata_tf_to_host(ap, &qc->tf);

		ap->hsm_task_state = HSM_ST_FIRST;

		if ((!(qc->dev->flags & ATA_DFLAG_CDB_INTR)) ||
		    (qc->tf.flags & ATA_TFLAG_POLLING))
			ata_pio_queue_task(ap, qc, 0);
		break;

	case ATAPI_PROT_DMA:
		WARN_ON_ONCE(qc->tf.flags & ATA_TFLAG_POLLING);

		ap->ops->sff_tf_load(ap, &qc->tf);   
		ap->ops->bmdma_setup(qc);	     
		ap->hsm_task_state = HSM_ST_FIRST;

		if (!(qc->dev->flags & ATA_DFLAG_CDB_INTR))
			ata_pio_queue_task(ap, qc, 0);
		break;

	default:
		WARN_ON_ONCE(1);
		return AC_ERR_SYSTEM;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(ata_sff_qc_issue);

bool ata_sff_qc_fill_rtf(struct ata_queued_cmd *qc)
{
	qc->ap->ops->sff_tf_read(qc->ap, &qc->result_tf);
	return true;
}
EXPORT_SYMBOL_GPL(ata_sff_qc_fill_rtf);

unsigned int ata_sff_host_intr(struct ata_port *ap,
				      struct ata_queued_cmd *qc)
{
	struct ata_eh_info *ehi = &ap->link.eh_info;
	u8 status, host_stat = 0;

	VPRINTK("ata%u: protocol %d task_state %d\n",
		ap->print_id, qc->tf.protocol, ap->hsm_task_state);

	switch (ap->hsm_task_state) {
	case HSM_ST_FIRST:
		 
		if (!(qc->dev->flags & ATA_DFLAG_CDB_INTR))
			goto idle_irq;
		break;
	case HSM_ST_LAST:
		if (qc->tf.protocol == ATA_PROT_DMA ||
		    qc->tf.protocol == ATAPI_PROT_DMA) {
			 
			host_stat = ap->ops->bmdma_status(ap);
			VPRINTK("ata%u: host_stat 0x%X\n",
				ap->print_id, host_stat);

			if (!(host_stat & ATA_DMA_INTR))
				goto idle_irq;

			ap->ops->bmdma_stop(qc);

			if (unlikely(host_stat & ATA_DMA_ERR)) {
				 
				qc->err_mask |= AC_ERR_HOST_BUS;
				ap->hsm_task_state = HSM_ST_ERR;
			}
		}
		break;
	case HSM_ST:
		break;
	default:
		goto idle_irq;
	}

	status = ata_sff_irq_status(ap);
	if (status & ATA_BUSY)
		goto idle_irq;

	ap->ops->sff_irq_clear(ap);

	ata_sff_hsm_move(ap, qc, status, 0);

	if (unlikely(qc->err_mask) && (qc->tf.protocol == ATA_PROT_DMA ||
				       qc->tf.protocol == ATAPI_PROT_DMA))
		ata_ehi_push_desc(ehi, "BMDMA stat 0x%x", host_stat);

	return 1;	 

idle_irq:
	ap->stats.idle_irq++;

#ifdef ATA_IRQ_TRAP
	if ((ap->stats.idle_irq % 1000) == 0) {
		ap->ops->sff_check_status(ap);
		ap->ops->sff_irq_clear(ap);
		ata_port_printk(ap, KERN_WARNING, "irq trap\n");
		return 1;
	}
#endif
	return 0;	 
}
EXPORT_SYMBOL_GPL(ata_sff_host_intr);

irqreturn_t ata_sff_interrupt(int irq, void *dev_instance)
{
	struct ata_host *host = dev_instance;
	unsigned int i;
	unsigned int handled = 0;
	unsigned long flags;

	spin_lock_irqsave(&host->lock, flags);

	for (i = 0; i < host->n_ports; i++) {
		struct ata_port *ap;

		ap = host->ports[i];
		if (ap &&
		    !(ap->flags & ATA_FLAG_DISABLED)) {
			struct ata_queued_cmd *qc;

			qc = ata_qc_from_tag(ap, ap->link.active_tag);
			if (qc && (!(qc->tf.flags & ATA_TFLAG_POLLING)) &&
			    (qc->flags & ATA_QCFLAG_ACTIVE))
				handled |= ata_sff_host_intr(ap, qc);
		}
	}

	spin_unlock_irqrestore(&host->lock, flags);

	return IRQ_RETVAL(handled);
}
EXPORT_SYMBOL_GPL(ata_sff_interrupt);

void ata_sff_lost_interrupt(struct ata_port *ap)
{
	u8 status;
	struct ata_queued_cmd *qc;

	qc = ata_qc_from_tag(ap, ap->link.active_tag);
	 
	if (qc == NULL ||  !(qc->flags & ATA_QCFLAG_ACTIVE))
		return;
	 
	if (qc->tf.flags & ATA_TFLAG_POLLING)
		return;
	 
	status = ata_sff_altstatus(ap);
	if (status & ATA_BUSY)
		return;

	ata_port_printk(ap, KERN_WARNING, "lost interrupt (Status 0x%x)\n",
								status);
	 
	ata_sff_host_intr(ap, qc);
}
EXPORT_SYMBOL_GPL(ata_sff_lost_interrupt);

void ata_sff_freeze(struct ata_port *ap)
{
	struct ata_ioports *ioaddr = &ap->ioaddr;

	ap->ctl |= ATA_NIEN;
	ap->last_ctl = ap->ctl;

	if (ioaddr->ctl_addr)
		iowrite8(ap->ctl, ioaddr->ctl_addr);

	ap->ops->sff_check_status(ap);

	ap->ops->sff_irq_clear(ap);
}
EXPORT_SYMBOL_GPL(ata_sff_freeze);

void ata_sff_thaw(struct ata_port *ap)
{
	 
	ap->ops->sff_check_status(ap);
	ap->ops->sff_irq_clear(ap);
	ap->ops->sff_irq_on(ap);
}
EXPORT_SYMBOL_GPL(ata_sff_thaw);

int ata_sff_prereset(struct ata_link *link, unsigned long deadline)
{
	struct ata_eh_context *ehc = &link->eh_context;
	int rc;

	rc = ata_std_prereset(link, deadline);
	if (rc)
		return rc;

	if (ehc->i.action & ATA_EH_HARDRESET)
		return 0;

	if (!ata_link_offline(link)) {
		rc = ata_sff_wait_ready(link, deadline);
		if (rc && rc != -ENODEV) {
			ata_link_printk(link, KERN_WARNING, "device not ready "
					"(errno=%d), forcing hardreset\n", rc);
			ehc->i.action |= ATA_EH_HARDRESET;
		}
	}

	return 0;
}
EXPORT_SYMBOL_GPL(ata_sff_prereset);

static unsigned int ata_devchk(struct ata_port *ap, unsigned int device)
{
	struct ata_ioports *ioaddr = &ap->ioaddr;
	u8 nsect, lbal;

	ap->ops->sff_dev_select(ap, device);

	iowrite8(0x55, ioaddr->nsect_addr);
	iowrite8(0xaa, ioaddr->lbal_addr);

	iowrite8(0xaa, ioaddr->nsect_addr);
	iowrite8(0x55, ioaddr->lbal_addr);

	iowrite8(0x55, ioaddr->nsect_addr);
	iowrite8(0xaa, ioaddr->lbal_addr);

	nsect = ioread8(ioaddr->nsect_addr);
	lbal = ioread8(ioaddr->lbal_addr);

	if ((nsect == 0x55) && (lbal == 0xaa))
		return 1;	 

	return 0;		 
}

unsigned int ata_sff_dev_classify(struct ata_device *dev, int present,
				  u8 *r_err)
{
	struct ata_port *ap = dev->link->ap;
	struct ata_taskfile tf;
	unsigned int class;
	u8 err;

	ap->ops->sff_dev_select(ap, dev->devno);

	memset(&tf, 0, sizeof(tf));

	ap->ops->sff_tf_read(ap, &tf);
	err = tf.feature;
	if (r_err)
		*r_err = err;

	if (err == 0)
		 
		dev->horkage |= ATA_HORKAGE_DIAGNOSTIC;
	else if (err == 1)
		  ;
	else if ((dev->devno == 0) && (err == 0x81))
		  ;
	else
		return ATA_DEV_NONE;

	class = ata_dev_classify(&tf);

	if (class == ATA_DEV_UNKNOWN) {
		 
		if (present && (dev->horkage & ATA_HORKAGE_DIAGNOSTIC))
			class = ATA_DEV_ATA;
		else
			class = ATA_DEV_NONE;
	} else if ((class == ATA_DEV_ATA) &&
		   (ap->ops->sff_check_status(ap) == 0))
		class = ATA_DEV_NONE;

	return class;
}
EXPORT_SYMBOL_GPL(ata_sff_dev_classify);

int ata_sff_wait_after_reset(struct ata_link *link, unsigned int devmask,
			     unsigned long deadline)
{
	struct ata_port *ap = link->ap;
	struct ata_ioports *ioaddr = &ap->ioaddr;
	unsigned int dev0 = devmask & (1 << 0);
	unsigned int dev1 = devmask & (1 << 1);
	int rc, ret = 0;

	msleep(ATA_WAIT_AFTER_RESET);

	rc = ata_sff_wait_ready(link, deadline);
	 
	if (rc)
		return rc;

	if (dev1) {
		int i;

		ap->ops->sff_dev_select(ap, 1);

		for (i = 0; i < 2; i++) {
			u8 nsect, lbal;

			nsect = ioread8(ioaddr->nsect_addr);
			lbal = ioread8(ioaddr->lbal_addr);
			if ((nsect == 1) && (lbal == 1))
				break;
			msleep(50);	 
		}

		rc = ata_sff_wait_ready(link, deadline);
		if (rc) {
			if (rc != -ENODEV)
				return rc;
			ret = rc;
		}
	}

	ap->ops->sff_dev_select(ap, 0);
	if (dev1)
		ap->ops->sff_dev_select(ap, 1);
	if (dev0)
		ap->ops->sff_dev_select(ap, 0);

	return ret;
}
EXPORT_SYMBOL_GPL(ata_sff_wait_after_reset);

static int ata_bus_softreset(struct ata_port *ap, unsigned int devmask,
			     unsigned long deadline)
{
	struct ata_ioports *ioaddr = &ap->ioaddr;

	DPRINTK("ata%u: bus reset via SRST\n", ap->print_id);

	iowrite8(ap->ctl, ioaddr->ctl_addr);
	udelay(20);	 
	iowrite8(ap->ctl | ATA_SRST, ioaddr->ctl_addr);
	udelay(20);	 
	iowrite8(ap->ctl, ioaddr->ctl_addr);
	ap->last_ctl = ap->ctl;

	return ata_sff_wait_after_reset(&ap->link, devmask, deadline);
}

int ata_sff_softreset(struct ata_link *link, unsigned int *classes,
		      unsigned long deadline)
{
	struct ata_port *ap = link->ap;
	unsigned int slave_possible = ap->flags & ATA_FLAG_SLAVE_POSS;
	unsigned int devmask = 0;
	int rc;
	u8 err;

	DPRINTK("ENTER\n");

	if (ata_devchk(ap, 0))
		devmask |= (1 << 0);
	if (slave_possible && ata_devchk(ap, 1))
		devmask |= (1 << 1);

	ap->ops->sff_dev_select(ap, 0);

	DPRINTK("about to softreset, devmask=%x\n", devmask);
	rc = ata_bus_softreset(ap, devmask, deadline);
	 
	if (rc && (rc != -ENODEV || sata_scr_valid(link))) {
		ata_link_printk(link, KERN_ERR, "SRST failed (errno=%d)\n", rc);
		return rc;
	}

	classes[0] = ata_sff_dev_classify(&link->device[0],
					  devmask & (1 << 0), &err);
	if (slave_possible && err != 0x81)
		classes[1] = ata_sff_dev_classify(&link->device[1],
						  devmask & (1 << 1), &err);

	DPRINTK("EXIT, classes[0]=%u [1]=%u\n", classes[0], classes[1]);
	return 0;
}
EXPORT_SYMBOL_GPL(ata_sff_softreset);

int sata_sff_hardreset(struct ata_link *link, unsigned int *class,
		       unsigned long deadline)
{
	struct ata_eh_context *ehc = &link->eh_context;
	const unsigned long *timing = sata_ehc_deb_timing(ehc);
	bool online;
	int rc;

	rc = sata_link_hardreset(link, timing, deadline, &online,
				 ata_sff_check_ready);
	if (online)
		*class = ata_sff_dev_classify(link->device, 1, NULL);

	DPRINTK("EXIT, class=%u\n", *class);
	return rc;
}
EXPORT_SYMBOL_GPL(sata_sff_hardreset);

void ata_sff_postreset(struct ata_link *link, unsigned int *classes)
{
	struct ata_port *ap = link->ap;

	ata_std_postreset(link, classes);

	if (classes[0] != ATA_DEV_NONE)
		ap->ops->sff_dev_select(ap, 1);
	if (classes[1] != ATA_DEV_NONE)
		ap->ops->sff_dev_select(ap, 0);

	if (classes[0] == ATA_DEV_NONE && classes[1] == ATA_DEV_NONE) {
		DPRINTK("EXIT, no device\n");
		return;
	}

	if (ap->ioaddr.ctl_addr) {
		iowrite8(ap->ctl, ap->ioaddr.ctl_addr);
		ap->last_ctl = ap->ctl;
	}
}
EXPORT_SYMBOL_GPL(ata_sff_postreset);

void ata_sff_drain_fifo(struct ata_queued_cmd *qc)
{
	int count;
	struct ata_port *ap;

	if (qc == NULL || qc->dma_dir == DMA_TO_DEVICE)
		return;

	ap = qc->ap;
	 
	for (count = 0; (ap->ops->sff_check_status(ap) & ATA_DRQ)
						&& count < 32768; count++)
		ioread16(ap->ioaddr.data_addr);

	if (count)
		ata_port_printk(ap, KERN_DEBUG,
			"drained %d bytes to clear DRQ.\n", count);

}
EXPORT_SYMBOL_GPL(ata_sff_drain_fifo);

void ata_sff_error_handler(struct ata_port *ap)
{
	ata_reset_fn_t softreset = ap->ops->softreset;
	ata_reset_fn_t hardreset = ap->ops->hardreset;
	struct ata_queued_cmd *qc;
	unsigned long flags;
	int thaw = 0;

	qc = __ata_qc_from_tag(ap, ap->link.active_tag);
	if (qc && !(qc->flags & ATA_QCFLAG_FAILED))
		qc = NULL;

	spin_lock_irqsave(ap->lock, flags);

	ap->hsm_task_state = HSM_ST_IDLE;

	if (ap->ioaddr.bmdma_addr &&
	    qc && (qc->tf.protocol == ATA_PROT_DMA ||
		   qc->tf.protocol == ATAPI_PROT_DMA)) {
		u8 host_stat;

		host_stat = ap->ops->bmdma_status(ap);

		if (qc->err_mask == AC_ERR_TIMEOUT
						&& (host_stat & ATA_DMA_ERR)) {
			qc->err_mask = AC_ERR_HOST_BUS;
			thaw = 1;
		}

		ap->ops->bmdma_stop(qc);
	}

	ata_sff_sync(ap);		 
	ap->ops->sff_check_status(ap);
	ap->ops->sff_irq_clear(ap);
	 
	if (ap->ops->drain_fifo)
		ap->ops->drain_fifo(qc);

	spin_unlock_irqrestore(ap->lock, flags);

	if (thaw)
		ata_eh_thaw_port(ap);

	if (softreset == ata_sff_softreset && !ap->ioaddr.ctl_addr)
		softreset = NULL;
	if (ata_is_builtin_hardreset(hardreset) && !sata_scr_valid(&ap->link))
		hardreset = NULL;

	ata_do_eh(ap, ap->ops->prereset, softreset, hardreset,
		  ap->ops->postreset);
}
EXPORT_SYMBOL_GPL(ata_sff_error_handler);

void ata_sff_post_internal_cmd(struct ata_queued_cmd *qc)
{
	struct ata_port *ap = qc->ap;
	unsigned long flags;

	spin_lock_irqsave(ap->lock, flags);

	ap->hsm_task_state = HSM_ST_IDLE;

	if (ap->ioaddr.bmdma_addr)
		ata_bmdma_stop(qc);

	spin_unlock_irqrestore(ap->lock, flags);
}
EXPORT_SYMBOL_GPL(ata_sff_post_internal_cmd);

int ata_sff_port_start(struct ata_port *ap)
{
	if (ap->ioaddr.bmdma_addr)
		return ata_port_start(ap);
	return 0;
}
EXPORT_SYMBOL_GPL(ata_sff_port_start);

int ata_sff_port_start32(struct ata_port *ap)
{
	ap->pflags |= ATA_PFLAG_PIO32 | ATA_PFLAG_PIO32CHANGE;
	if (ap->ioaddr.bmdma_addr)
		return ata_port_start(ap);
	return 0;
}
EXPORT_SYMBOL_GPL(ata_sff_port_start32);

void ata_sff_std_ports(struct ata_ioports *ioaddr)
{
	ioaddr->data_addr = ioaddr->cmd_addr + ATA_REG_DATA;
	ioaddr->error_addr = ioaddr->cmd_addr + ATA_REG_ERR;
	ioaddr->feature_addr = ioaddr->cmd_addr + ATA_REG_FEATURE;
	ioaddr->nsect_addr = ioaddr->cmd_addr + ATA_REG_NSECT;
	ioaddr->lbal_addr = ioaddr->cmd_addr + ATA_REG_LBAL;
	ioaddr->lbam_addr = ioaddr->cmd_addr + ATA_REG_LBAM;
	ioaddr->lbah_addr = ioaddr->cmd_addr + ATA_REG_LBAH;
	ioaddr->device_addr = ioaddr->cmd_addr + ATA_REG_DEVICE;
	ioaddr->status_addr = ioaddr->cmd_addr + ATA_REG_STATUS;
	ioaddr->command_addr = ioaddr->cmd_addr + ATA_REG_CMD;
}
EXPORT_SYMBOL_GPL(ata_sff_std_ports);

unsigned long ata_bmdma_mode_filter(struct ata_device *adev,
				    unsigned long xfer_mask)
{
	 
	if (adev->link->ap->ioaddr.bmdma_addr == NULL)
		xfer_mask &= ~(ATA_MASK_MWDMA | ATA_MASK_UDMA);
	return xfer_mask;
}
EXPORT_SYMBOL_GPL(ata_bmdma_mode_filter);

void ata_bmdma_setup(struct ata_queued_cmd *qc)
{
	struct ata_port *ap = qc->ap;
	unsigned int rw = (qc->tf.flags & ATA_TFLAG_WRITE);
	u8 dmactl;

	mb();	 
	iowrite32(ap->prd_dma, ap->ioaddr.bmdma_addr + ATA_DMA_TABLE_OFS);

	dmactl = ioread8(ap->ioaddr.bmdma_addr + ATA_DMA_CMD);
	dmactl &= ~(ATA_DMA_WR | ATA_DMA_START);
	if (!rw)
		dmactl |= ATA_DMA_WR;
	iowrite8(dmactl, ap->ioaddr.bmdma_addr + ATA_DMA_CMD);

	ap->ops->sff_exec_command(ap, &qc->tf);
}
EXPORT_SYMBOL_GPL(ata_bmdma_setup);

void ata_bmdma_start(struct ata_queued_cmd *qc)
{
	struct ata_port *ap = qc->ap;
	u8 dmactl;

	dmactl = ioread8(ap->ioaddr.bmdma_addr + ATA_DMA_CMD);
	iowrite8(dmactl | ATA_DMA_START, ap->ioaddr.bmdma_addr + ATA_DMA_CMD);

}
EXPORT_SYMBOL_GPL(ata_bmdma_start);

void ata_bmdma_stop(struct ata_queued_cmd *qc)
{
	struct ata_port *ap = qc->ap;
	void __iomem *mmio = ap->ioaddr.bmdma_addr;

	iowrite8(ioread8(mmio + ATA_DMA_CMD) & ~ATA_DMA_START,
		 mmio + ATA_DMA_CMD);

	ata_sff_dma_pause(ap);
}
EXPORT_SYMBOL_GPL(ata_bmdma_stop);

u8 ata_bmdma_status(struct ata_port *ap)
{
	return ioread8(ap->ioaddr.bmdma_addr + ATA_DMA_STATUS);
}
EXPORT_SYMBOL_GPL(ata_bmdma_status);

void ata_bus_reset(struct ata_port *ap)
{
	struct ata_device *device = ap->link.device;
	struct ata_ioports *ioaddr = &ap->ioaddr;
	unsigned int slave_possible = ap->flags & ATA_FLAG_SLAVE_POSS;
	u8 err;
	unsigned int dev0, dev1 = 0, devmask = 0;
	int rc;

	DPRINTK("ENTER, host %u, port %u\n", ap->print_id, ap->port_no);

	if (ap->flags & ATA_FLAG_SATA_RESET)
		dev0 = 1;
	else {
		dev0 = ata_devchk(ap, 0);
		if (slave_possible)
			dev1 = ata_devchk(ap, 1);
	}

	if (dev0)
		devmask |= (1 << 0);
	if (dev1)
		devmask |= (1 << 1);

	ap->ops->sff_dev_select(ap, 0);

	if (ap->flags & ATA_FLAG_SRST) {
		rc = ata_bus_softreset(ap, devmask,
				       ata_deadline(jiffies, 40000));
		if (rc && rc != -ENODEV)
			goto err_out;
	}

	device[0].class = ata_sff_dev_classify(&device[0], dev0, &err);
	if ((slave_possible) && (err != 0x81))
		device[1].class = ata_sff_dev_classify(&device[1], dev1, &err);

	if (device[1].class != ATA_DEV_NONE)
		ap->ops->sff_dev_select(ap, 1);
	if (device[0].class != ATA_DEV_NONE)
		ap->ops->sff_dev_select(ap, 0);

	if ((device[0].class == ATA_DEV_NONE) &&
	    (device[1].class == ATA_DEV_NONE))
		goto err_out;

	if (ap->flags & (ATA_FLAG_SATA_RESET | ATA_FLAG_SRST)) {
		 
		iowrite8(ap->ctl, ioaddr->ctl_addr);
		ap->last_ctl = ap->ctl;
	}

	DPRINTK("EXIT\n");
	return;

err_out:
	ata_port_printk(ap, KERN_ERR, "disabling port\n");
	ata_port_disable(ap);

	DPRINTK("EXIT\n");
}
EXPORT_SYMBOL_GPL(ata_bus_reset);

#ifdef CONFIG_PCI

int ata_pci_bmdma_clear_simplex(struct pci_dev *pdev)
{
	unsigned long bmdma = pci_resource_start(pdev, 4);
	u8 simplex;

	if (bmdma == 0)
		return -ENOENT;

	simplex = inb(bmdma + 0x02);
	outb(simplex & 0x60, bmdma + 0x02);
	simplex = inb(bmdma + 0x02);
	if (simplex & 0x80)
		return -EOPNOTSUPP;
	return 0;
}
EXPORT_SYMBOL_GPL(ata_pci_bmdma_clear_simplex);

int ata_pci_bmdma_init(struct ata_host *host)
{
	struct device *gdev = host->dev;
	struct pci_dev *pdev = to_pci_dev(gdev);
	int i, rc;

	if (pci_resource_start(pdev, 4) == 0)
		return 0;

	rc = pci_set_dma_mask(pdev, ATA_DMA_MASK);
	if (rc)
		return rc;
	rc = pci_set_consistent_dma_mask(pdev, ATA_DMA_MASK);
	if (rc)
		return rc;

	rc = pcim_iomap_regions(pdev, 1 << 4, dev_driver_string(gdev));
	if (rc) {
		dev_printk(KERN_ERR, gdev, "failed to request/iomap BAR4\n");
		return -ENOMEM;
	}
	host->iomap = pcim_iomap_table(pdev);

	for (i = 0; i < 2; i++) {
		struct ata_port *ap = host->ports[i];
		void __iomem *bmdma = host->iomap[4] + 8 * i;

		if (ata_port_is_dummy(ap))
			continue;

		ap->ioaddr.bmdma_addr = bmdma;
		if ((!(ap->flags & ATA_FLAG_IGN_SIMPLEX)) &&
		    (ioread8(bmdma + 2) & 0x80))
			host->flags |= ATA_HOST_SIMPLEX;

		ata_port_desc(ap, "bmdma 0x%llx",
		    (unsigned long long)pci_resource_start(pdev, 4) + 8 * i);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(ata_pci_bmdma_init);

static int ata_resources_present(struct pci_dev *pdev, int port)
{
	int i;

	port = port * 2;
	for (i = 0; i < 2; i++) {
		if (pci_resource_start(pdev, port + i) == 0 ||
		    pci_resource_len(pdev, port + i) == 0)
			return 0;
	}
	return 1;
}

int ata_pci_sff_init_host(struct ata_host *host)
{
	struct device *gdev = host->dev;
	struct pci_dev *pdev = to_pci_dev(gdev);
	unsigned int mask = 0;
	int i, rc;

	for (i = 0; i < 2; i++) {
		struct ata_port *ap = host->ports[i];
		int base = i * 2;
		void __iomem * const *iomap;

		if (ata_port_is_dummy(ap))
			continue;

		if (!ata_resources_present(pdev, i)) {
			ap->ops = &ata_dummy_port_ops;
			continue;
		}

		rc = pcim_iomap_regions(pdev, 0x3 << base,
					dev_driver_string(gdev));
		if (rc) {
			dev_printk(KERN_WARNING, gdev,
				   "failed to request/iomap BARs for port %d "
				   "(errno=%d)\n", i, rc);
			if (rc == -EBUSY)
				pcim_pin_device(pdev);
			ap->ops = &ata_dummy_port_ops;
			continue;
		}
		host->iomap = iomap = pcim_iomap_table(pdev);

		ap->ioaddr.cmd_addr = iomap[base];
		ap->ioaddr.altstatus_addr =
		ap->ioaddr.ctl_addr = (void __iomem *)
			((unsigned long)iomap[base + 1] | ATA_PCI_CTL_OFS);
		ata_sff_std_ports(&ap->ioaddr);

		ata_port_desc(ap, "cmd 0x%llx ctl 0x%llx",
			(unsigned long long)pci_resource_start(pdev, base),
			(unsigned long long)pci_resource_start(pdev, base + 1));

		mask |= 1 << i;
	}

	if (!mask) {
		dev_printk(KERN_ERR, gdev, "no available native port\n");
		return -ENODEV;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(ata_pci_sff_init_host);

int ata_pci_sff_prepare_host(struct pci_dev *pdev,
			     const struct ata_port_info * const *ppi,
			     struct ata_host **r_host)
{
	struct ata_host *host;
	int rc;

	if (!devres_open_group(&pdev->dev, NULL, GFP_KERNEL))
		return -ENOMEM;

	host = ata_host_alloc_pinfo(&pdev->dev, ppi, 2);
	if (!host) {
		dev_printk(KERN_ERR, &pdev->dev,
			   "failed to allocate ATA host\n");
		rc = -ENOMEM;
		goto err_out;
	}

	rc = ata_pci_sff_init_host(host);
	if (rc)
		goto err_out;

	rc = ata_pci_bmdma_init(host);
	if (rc)
		goto err_bmdma;

	devres_remove_group(&pdev->dev, NULL);
	*r_host = host;
	return 0;

err_bmdma:
	 
	pcim_iounmap_regions(pdev, 0xf);
err_out:
	devres_release_group(&pdev->dev, NULL);
	return rc;
}
EXPORT_SYMBOL_GPL(ata_pci_sff_prepare_host);

int ata_pci_sff_activate_host(struct ata_host *host,
			      irq_handler_t irq_handler,
			      struct scsi_host_template *sht)
{
	struct device *dev = host->dev;
	struct pci_dev *pdev = to_pci_dev(dev);
	const char *drv_name = dev_driver_string(host->dev);
	int legacy_mode = 0, rc;

	rc = ata_host_start(host);
	if (rc)
		return rc;

	if ((pdev->class >> 8) == PCI_CLASS_STORAGE_IDE) {
		u8 tmp8, mask;

		pci_read_config_byte(pdev, PCI_CLASS_PROG, &tmp8);
		mask = (1 << 2) | (1 << 0);
		if ((tmp8 & mask) != mask)
			legacy_mode = 1;
#if defined(CONFIG_NO_ATA_LEGACY)
		 
		if (legacy_mode) {
			printk(KERN_ERR "ata: Compatibility mode ATA is not supported on this platform, skipping.\n");
			return -EOPNOTSUPP;
		}
#endif
	}

	if (!devres_open_group(dev, NULL, GFP_KERNEL))
		return -ENOMEM;

	if (!legacy_mode && pdev->irq) {
		rc = devm_request_irq(dev, pdev->irq, irq_handler,
				      IRQF_SHARED, drv_name, host);
		if (rc)
			goto out;

		ata_port_desc(host->ports[0], "irq %d", pdev->irq);
		ata_port_desc(host->ports[1], "irq %d", pdev->irq);
	} else if (legacy_mode) {
		if (!ata_port_is_dummy(host->ports[0])) {
			rc = devm_request_irq(dev, ATA_PRIMARY_IRQ(pdev),
					      irq_handler, IRQF_SHARED,
					      drv_name, host);
			if (rc)
				goto out;

			ata_port_desc(host->ports[0], "irq %d",
				      ATA_PRIMARY_IRQ(pdev));
		}

		if (!ata_port_is_dummy(host->ports[1])) {
			rc = devm_request_irq(dev, ATA_SECONDARY_IRQ(pdev),
					      irq_handler, IRQF_SHARED,
					      drv_name, host);
			if (rc)
				goto out;

			ata_port_desc(host->ports[1], "irq %d",
				      ATA_SECONDARY_IRQ(pdev));
		}
	}

	rc = ata_host_register(host, sht);
out:
	if (rc == 0)
		devres_remove_group(dev, NULL);
	else
		devres_release_group(dev, NULL);

	return rc;
}
EXPORT_SYMBOL_GPL(ata_pci_sff_activate_host);

int ata_pci_sff_init_one(struct pci_dev *pdev,
			 const struct ata_port_info * const *ppi,
			 struct scsi_host_template *sht, void *host_priv)
{
	struct device *dev = &pdev->dev;
	const struct ata_port_info *pi = NULL;
	struct ata_host *host = NULL;
	int i, rc;

	DPRINTK("ENTER\n");

	for (i = 0; i < 2 && ppi[i]; i++) {
		if (ppi[i]->port_ops != &ata_dummy_port_ops) {
			pi = ppi[i];
			break;
		}
	}

	if (!pi) {
		dev_printk(KERN_ERR, &pdev->dev,
			   "no valid port_info specified\n");
		return -EINVAL;
	}

	if (!devres_open_group(dev, NULL, GFP_KERNEL))
		return -ENOMEM;

	rc = pcim_enable_device(pdev);
	if (rc)
		goto out;

	rc = ata_pci_sff_prepare_host(pdev, ppi, &host);
	if (rc)
		goto out;
	host->private_data = host_priv;

	pci_set_master(pdev);
	rc = ata_pci_sff_activate_host(host, ata_sff_interrupt, sht);
out:
	if (rc == 0)
		devres_remove_group(&pdev->dev, NULL);
	else
		devres_release_group(&pdev->dev, NULL);

	return rc;
}
EXPORT_SYMBOL_GPL(ata_pci_sff_init_one);

#endif  
