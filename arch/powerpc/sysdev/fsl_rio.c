 
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/rio.h>
#include <linux/rio_drv.h>
#include <linux/of_platform.h>
#include <linux/delay.h>

#include <asm/io.h>

#define IRQ_RIO_BELL(m)		(((struct rio_priv *)(m->priv))->bellirq)
#define IRQ_RIO_TX(m)		(((struct rio_priv *)(m->priv))->txirq)
#define IRQ_RIO_RX(m)		(((struct rio_priv *)(m->priv))->rxirq)

#define RIO_ATMU_REGS_OFFSET	0x10c00
#define RIO_P_MSG_REGS_OFFSET	0x11000
#define RIO_S_MSG_REGS_OFFSET	0x13000
#define RIO_ESCSR		0x158
#define RIO_CCSR		0x15c
#define RIO_ISR_AACR		0x10120
#define RIO_ISR_AACR_AA		0x1	 
#define RIO_MAINT_WIN_SIZE	0x400000
#define RIO_DBELL_WIN_SIZE	0x1000

#define RIO_MSG_OMR_MUI		0x00000002
#define RIO_MSG_OSR_TE		0x00000080
#define RIO_MSG_OSR_QOI		0x00000020
#define RIO_MSG_OSR_QFI		0x00000010
#define RIO_MSG_OSR_MUB		0x00000004
#define RIO_MSG_OSR_EOMI	0x00000002
#define RIO_MSG_OSR_QEI		0x00000001

#define RIO_MSG_IMR_MI		0x00000002
#define RIO_MSG_ISR_TE		0x00000080
#define RIO_MSG_ISR_QFI		0x00000010
#define RIO_MSG_ISR_DIQI	0x00000001

#define RIO_MSG_DESC_SIZE	32
#define RIO_MSG_BUFFER_SIZE	4096
#define RIO_MIN_TX_RING_SIZE	2
#define RIO_MAX_TX_RING_SIZE	2048
#define RIO_MIN_RX_RING_SIZE	2
#define RIO_MAX_RX_RING_SIZE	2048

#define DOORBELL_DMR_DI		0x00000002
#define DOORBELL_DSR_TE		0x00000080
#define DOORBELL_DSR_QFI	0x00000010
#define DOORBELL_DSR_DIQI	0x00000001
#define DOORBELL_TID_OFFSET	0x02
#define DOORBELL_SID_OFFSET	0x04
#define DOORBELL_INFO_OFFSET	0x06

#define DOORBELL_MESSAGE_SIZE	0x08
#define DBELL_SID(x)		(*(u16 *)(x + DOORBELL_SID_OFFSET))
#define DBELL_TID(x)		(*(u16 *)(x + DOORBELL_TID_OFFSET))
#define DBELL_INF(x)		(*(u16 *)(x + DOORBELL_INFO_OFFSET))

struct rio_atmu_regs {
	u32 rowtar;
	u32 rowtear;
	u32 rowbar;
	u32 pad2;
	u32 rowar;
	u32 pad3[3];
};

struct rio_msg_regs {
	u32 omr;
	u32 osr;
	u32 pad1;
	u32 odqdpar;
	u32 pad2;
	u32 osar;
	u32 odpr;
	u32 odatr;
	u32 odcr;
	u32 pad3;
	u32 odqepar;
	u32 pad4[13];
	u32 imr;
	u32 isr;
	u32 pad5;
	u32 ifqdpar;
	u32 pad6;
	u32 ifqepar;
	u32 pad7[226];
	u32 odmr;
	u32 odsr;
	u32 res0[4];
	u32 oddpr;
	u32 oddatr;
	u32 res1[3];
	u32 odretcr;
	u32 res2[12];
	u32 dmr;
	u32 dsr;
	u32 pad8;
	u32 dqdpar;
	u32 pad9;
	u32 dqepar;
	u32 pad10[26];
	u32 pwmr;
	u32 pwsr;
	u32 pad11;
	u32 pwqbar;
};

struct rio_tx_desc {
	u32 res1;
	u32 saddr;
	u32 dport;
	u32 dattr;
	u32 res2;
	u32 res3;
	u32 dwcnt;
	u32 res4;
};

struct rio_dbell_ring {
	void *virt;
	dma_addr_t phys;
};

struct rio_msg_tx_ring {
	void *virt;
	dma_addr_t phys;
	void *virt_buffer[RIO_MAX_TX_RING_SIZE];
	dma_addr_t phys_buffer[RIO_MAX_TX_RING_SIZE];
	int tx_slot;
	int size;
	void *dev_id;
};

struct rio_msg_rx_ring {
	void *virt;
	dma_addr_t phys;
	void *virt_buffer[RIO_MAX_RX_RING_SIZE];
	int rx_slot;
	int size;
	void *dev_id;
};

struct rio_priv {
	struct device *dev;
	void __iomem *regs_win;
	struct rio_atmu_regs __iomem *atmu_regs;
	struct rio_atmu_regs __iomem *maint_atmu_regs;
	struct rio_atmu_regs __iomem *dbell_atmu_regs;
	void __iomem *dbell_win;
	void __iomem *maint_win;
	struct rio_msg_regs __iomem *msg_regs;
	struct rio_dbell_ring dbell_ring;
	struct rio_msg_tx_ring msg_tx_ring;
	struct rio_msg_rx_ring msg_rx_ring;
	int bellirq;
	int txirq;
	int rxirq;
};

static int fsl_rio_doorbell_send(struct rio_mport *mport,
				int index, u16 destid, u16 data)
{
	struct rio_priv *priv = mport->priv;
	pr_debug("fsl_doorbell_send: index %d destid %4.4x data %4.4x\n",
		 index, destid, data);
	switch (mport->phy_type) {
	case RIO_PHY_PARALLEL:
		out_be32(&priv->dbell_atmu_regs->rowtar, destid << 22);
		out_be16(priv->dbell_win, data);
		break;
	case RIO_PHY_SERIAL:
#ifdef CONFIG_SYNO_QORIQ
		 
		while (in_be32(&priv->msg_regs->odsr) & 0x4) {
			cpu_relax();
		}
		out_be32(&priv->msg_regs->odsr, 0x1602);
#else
		 
		out_be32(&priv->msg_regs->odmr, 0x00000000);
#endif
		out_be32(&priv->msg_regs->odretcr, 0x00000004);
		out_be32(&priv->msg_regs->oddpr, destid << 16);
#ifdef CONFIG_SYNO_QORIQ
		out_be32(&priv->msg_regs->oddatr, data | 0x20000000);
		out_be32(&priv->msg_regs->odmr, 0x00000000);
#else
		out_be32(&priv->msg_regs->oddatr, data);
#endif
		out_be32(&priv->msg_regs->odmr, 0x00000001);
		break;
	}

	return 0;
}

static int fsl_local_config_read(struct rio_mport *mport,
				int index, u32 offset, int len, u32 *data)
{
	struct rio_priv *priv = mport->priv;
	pr_debug("fsl_local_config_read: index %d offset %8.8x\n", index,
		 offset);
	*data = in_be32(priv->regs_win + offset);

	return 0;
}

static int fsl_local_config_write(struct rio_mport *mport,
				int index, u32 offset, int len, u32 data)
{
	struct rio_priv *priv = mport->priv;
	pr_debug
	    ("fsl_local_config_write: index %d offset %8.8x data %8.8x\n",
	     index, offset, data);
	out_be32(priv->regs_win + offset, data);

	return 0;
}

static int
fsl_rio_config_read(struct rio_mport *mport, int index, u16 destid,
			u8 hopcount, u32 offset, int len, u32 *val)
{
	struct rio_priv *priv = mport->priv;
	u8 *data;

	pr_debug
	    ("fsl_rio_config_read: index %d destid %d hopcount %d offset %8.8x len %d\n",
	     index, destid, hopcount, offset, len);
	out_be32(&priv->maint_atmu_regs->rowtar,
		 (destid << 22) | (hopcount << 12) | ((offset & ~0x3) >> 9));

	data = (u8 *) priv->maint_win + offset;
	switch (len) {
	case 1:
		*val = in_8((u8 *) data);
		break;
	case 2:
		*val = in_be16((u16 *) data);
		break;
	default:
		*val = in_be32((u32 *) data);
		break;
	}

	return 0;
}

static int
fsl_rio_config_write(struct rio_mport *mport, int index, u16 destid,
			u8 hopcount, u32 offset, int len, u32 val)
{
	struct rio_priv *priv = mport->priv;
	u8 *data;
	pr_debug
	    ("fsl_rio_config_write: index %d destid %d hopcount %d offset %8.8x len %d val %8.8x\n",
	     index, destid, hopcount, offset, len, val);
	out_be32(&priv->maint_atmu_regs->rowtar,
		 (destid << 22) | (hopcount << 12) | ((offset & ~0x3) >> 9));

	data = (u8 *) priv->maint_win + offset;
	switch (len) {
	case 1:
		out_8((u8 *) data, val);
		break;
	case 2:
		out_be16((u16 *) data, val);
		break;
	default:
		out_be32((u32 *) data, val);
		break;
	}

	return 0;
}

int
rio_hw_add_outb_message(struct rio_mport *mport, struct rio_dev *rdev, int mbox,
			void *buffer, size_t len)
{
	struct rio_priv *priv = mport->priv;
	u32 omr;
	struct rio_tx_desc *desc = (struct rio_tx_desc *)priv->msg_tx_ring.virt
					+ priv->msg_tx_ring.tx_slot;
	int ret = 0;

	pr_debug
	    ("RIO: rio_hw_add_outb_message(): destid %4.4x mbox %d buffer %8.8x len %8.8x\n",
	     rdev->destid, mbox, (int)buffer, len);

	if ((len < 8) || (len > RIO_MAX_MSG_SIZE)) {
		ret = -EINVAL;
		goto out;
	}

	memcpy(priv->msg_tx_ring.virt_buffer[priv->msg_tx_ring.tx_slot], buffer,
			len);
	if (len < (RIO_MAX_MSG_SIZE - 4))
		memset(priv->msg_tx_ring.virt_buffer[priv->msg_tx_ring.tx_slot]
				+ len, 0, RIO_MAX_MSG_SIZE - len);

	switch (mport->phy_type) {
	case RIO_PHY_PARALLEL:
		 
		desc->dport = mbox & 0x3;

		desc->dattr = 0x28000000 | (rdev->destid << 2);
		break;
	case RIO_PHY_SERIAL:
		 
		desc->dport = (rdev->destid << 16) | (mbox & 0x3);

		desc->dattr = 0x28000000;
		break;
	}

	desc->dwcnt = is_power_of_2(len) ? len : 1 << get_bitmask_order(len);

	desc->saddr = 0x00000004
		| priv->msg_tx_ring.phys_buffer[priv->msg_tx_ring.tx_slot];

	omr = in_be32(&priv->msg_regs->omr);
	out_be32(&priv->msg_regs->omr, omr | RIO_MSG_OMR_MUI);

	if (++priv->msg_tx_ring.tx_slot == priv->msg_tx_ring.size)
		priv->msg_tx_ring.tx_slot = 0;

      out:
	return ret;
}

EXPORT_SYMBOL_GPL(rio_hw_add_outb_message);

static irqreturn_t
fsl_rio_tx_handler(int irq, void *dev_instance)
{
	int osr;
	struct rio_mport *port = (struct rio_mport *)dev_instance;
	struct rio_priv *priv = port->priv;

	osr = in_be32(&priv->msg_regs->osr);

	if (osr & RIO_MSG_OSR_TE) {
		pr_info("RIO: outbound message transmission error\n");
		out_be32(&priv->msg_regs->osr, RIO_MSG_OSR_TE);
		goto out;
	}

	if (osr & RIO_MSG_OSR_QOI) {
		pr_info("RIO: outbound message queue overflow\n");
		out_be32(&priv->msg_regs->osr, RIO_MSG_OSR_QOI);
		goto out;
	}

	if (osr & RIO_MSG_OSR_EOMI) {
		u32 dqp = in_be32(&priv->msg_regs->odqdpar);
		int slot = (dqp - priv->msg_tx_ring.phys) >> 5;
		port->outb_msg[0].mcback(port, priv->msg_tx_ring.dev_id, -1,
				slot);

		out_be32(&priv->msg_regs->osr, RIO_MSG_OSR_EOMI);
	}

      out:
	return IRQ_HANDLED;
}

int rio_open_outb_mbox(struct rio_mport *mport, void *dev_id, int mbox, int entries)
{
	int i, j, rc = 0;
	struct rio_priv *priv = mport->priv;

	if ((entries < RIO_MIN_TX_RING_SIZE) ||
	    (entries > RIO_MAX_TX_RING_SIZE) || (!is_power_of_2(entries))) {
		rc = -EINVAL;
		goto out;
	}

	priv->msg_tx_ring.dev_id = dev_id;
	priv->msg_tx_ring.size = entries;

	for (i = 0; i < priv->msg_tx_ring.size; i++) {
		priv->msg_tx_ring.virt_buffer[i] =
			dma_alloc_coherent(priv->dev, RIO_MSG_BUFFER_SIZE,
				&priv->msg_tx_ring.phys_buffer[i], GFP_KERNEL);
		if (!priv->msg_tx_ring.virt_buffer[i]) {
			rc = -ENOMEM;
			for (j = 0; j < priv->msg_tx_ring.size; j++)
				if (priv->msg_tx_ring.virt_buffer[j])
					dma_free_coherent(priv->dev,
							RIO_MSG_BUFFER_SIZE,
							priv->msg_tx_ring.
							virt_buffer[j],
							priv->msg_tx_ring.
							phys_buffer[j]);
			goto out;
		}
	}

	priv->msg_tx_ring.virt = dma_alloc_coherent(priv->dev,
				priv->msg_tx_ring.size * RIO_MSG_DESC_SIZE,
				&priv->msg_tx_ring.phys, GFP_KERNEL);
	if (!priv->msg_tx_ring.virt) {
		rc = -ENOMEM;
		goto out_dma;
	}
	memset(priv->msg_tx_ring.virt, 0,
			priv->msg_tx_ring.size * RIO_MSG_DESC_SIZE);
	priv->msg_tx_ring.tx_slot = 0;

	out_be32(&priv->msg_regs->odqdpar, priv->msg_tx_ring.phys);
	out_be32(&priv->msg_regs->odqepar, priv->msg_tx_ring.phys);

	out_be32(&priv->msg_regs->osar, 0x00000004);

	out_be32(&priv->msg_regs->osr, 0x000000b3);

	rc = request_irq(IRQ_RIO_TX(mport), fsl_rio_tx_handler, 0,
			 "msg_tx", (void *)mport);
	if (rc < 0)
		goto out_irq;

	out_be32(&priv->msg_regs->omr, 0x00100220);

	out_be32(&priv->msg_regs->omr,
		 in_be32(&priv->msg_regs->omr) |
		 ((get_bitmask_order(entries) - 2) << 12));

	out_be32(&priv->msg_regs->omr, in_be32(&priv->msg_regs->omr) | 0x1);

      out:
	return rc;

      out_irq:
	dma_free_coherent(priv->dev,
			  priv->msg_tx_ring.size * RIO_MSG_DESC_SIZE,
			  priv->msg_tx_ring.virt, priv->msg_tx_ring.phys);

      out_dma:
	for (i = 0; i < priv->msg_tx_ring.size; i++)
		dma_free_coherent(priv->dev, RIO_MSG_BUFFER_SIZE,
				  priv->msg_tx_ring.virt_buffer[i],
				  priv->msg_tx_ring.phys_buffer[i]);

	return rc;
}

void rio_close_outb_mbox(struct rio_mport *mport, int mbox)
{
	struct rio_priv *priv = mport->priv;
	 
	out_be32(&priv->msg_regs->omr, 0);

	dma_free_coherent(priv->dev,
			  priv->msg_tx_ring.size * RIO_MSG_DESC_SIZE,
			  priv->msg_tx_ring.virt, priv->msg_tx_ring.phys);

	free_irq(IRQ_RIO_TX(mport), (void *)mport);
}

static irqreturn_t
fsl_rio_rx_handler(int irq, void *dev_instance)
{
	int isr;
	struct rio_mport *port = (struct rio_mport *)dev_instance;
	struct rio_priv *priv = port->priv;

	isr = in_be32(&priv->msg_regs->isr);

	if (isr & RIO_MSG_ISR_TE) {
		pr_info("RIO: inbound message reception error\n");
		out_be32((void *)&priv->msg_regs->isr, RIO_MSG_ISR_TE);
		goto out;
	}

	if (isr & RIO_MSG_ISR_DIQI) {
		 
		port->inb_msg[0].mcback(port, priv->msg_rx_ring.dev_id, -1, -1);

		out_be32(&priv->msg_regs->isr, RIO_MSG_ISR_DIQI);
	}

      out:
	return IRQ_HANDLED;
}

int rio_open_inb_mbox(struct rio_mport *mport, void *dev_id, int mbox, int entries)
{
	int i, rc = 0;
	struct rio_priv *priv = mport->priv;

	if ((entries < RIO_MIN_RX_RING_SIZE) ||
	    (entries > RIO_MAX_RX_RING_SIZE) || (!is_power_of_2(entries))) {
		rc = -EINVAL;
		goto out;
	}

	priv->msg_rx_ring.dev_id = dev_id;
	priv->msg_rx_ring.size = entries;
	priv->msg_rx_ring.rx_slot = 0;
	for (i = 0; i < priv->msg_rx_ring.size; i++)
		priv->msg_rx_ring.virt_buffer[i] = NULL;

	priv->msg_rx_ring.virt = dma_alloc_coherent(priv->dev,
				priv->msg_rx_ring.size * RIO_MAX_MSG_SIZE,
				&priv->msg_rx_ring.phys, GFP_KERNEL);
	if (!priv->msg_rx_ring.virt) {
		rc = -ENOMEM;
		goto out;
	}

	out_be32(&priv->msg_regs->ifqdpar, (u32) priv->msg_rx_ring.phys);
	out_be32(&priv->msg_regs->ifqepar, (u32) priv->msg_rx_ring.phys);

	out_be32(&priv->msg_regs->isr, 0x00000091);

	rc = request_irq(IRQ_RIO_RX(mport), fsl_rio_rx_handler, 0,
			 "msg_rx", (void *)mport);
	if (rc < 0) {
		dma_free_coherent(priv->dev, RIO_MSG_BUFFER_SIZE,
				  priv->msg_tx_ring.virt_buffer[i],
				  priv->msg_tx_ring.phys_buffer[i]);
		goto out;
	}

	out_be32(&priv->msg_regs->imr, 0x001b0060);

	setbits32(&priv->msg_regs->imr, (get_bitmask_order(entries) - 2) << 12);

	setbits32(&priv->msg_regs->imr, 0x1);

      out:
	return rc;
}

void rio_close_inb_mbox(struct rio_mport *mport, int mbox)
{
	struct rio_priv *priv = mport->priv;
	 
	out_be32(&priv->msg_regs->imr, 0);

	dma_free_coherent(priv->dev, priv->msg_rx_ring.size * RIO_MAX_MSG_SIZE,
			  priv->msg_rx_ring.virt, priv->msg_rx_ring.phys);

	free_irq(IRQ_RIO_RX(mport), (void *)mport);
}

int rio_hw_add_inb_buffer(struct rio_mport *mport, int mbox, void *buf)
{
	int rc = 0;
	struct rio_priv *priv = mport->priv;

	pr_debug("RIO: rio_hw_add_inb_buffer(), msg_rx_ring.rx_slot %d\n",
		 priv->msg_rx_ring.rx_slot);

	if (priv->msg_rx_ring.virt_buffer[priv->msg_rx_ring.rx_slot]) {
		printk(KERN_ERR
		       "RIO: error adding inbound buffer %d, buffer exists\n",
		       priv->msg_rx_ring.rx_slot);
		rc = -EINVAL;
		goto out;
	}

	priv->msg_rx_ring.virt_buffer[priv->msg_rx_ring.rx_slot] = buf;
	if (++priv->msg_rx_ring.rx_slot == priv->msg_rx_ring.size)
		priv->msg_rx_ring.rx_slot = 0;

      out:
	return rc;
}

EXPORT_SYMBOL_GPL(rio_hw_add_inb_buffer);

void *rio_hw_get_inb_message(struct rio_mport *mport, int mbox)
{
	struct rio_priv *priv = mport->priv;
	u32 phys_buf, virt_buf;
	void *buf = NULL;
	int buf_idx;

	phys_buf = in_be32(&priv->msg_regs->ifqdpar);

	if (phys_buf == in_be32(&priv->msg_regs->ifqepar))
		goto out2;

	virt_buf = (u32) priv->msg_rx_ring.virt + (phys_buf
						- priv->msg_rx_ring.phys);
	buf_idx = (phys_buf - priv->msg_rx_ring.phys) / RIO_MAX_MSG_SIZE;
	buf = priv->msg_rx_ring.virt_buffer[buf_idx];

	if (!buf) {
		printk(KERN_ERR
		       "RIO: inbound message copy failed, no buffers\n");
		goto out1;
	}

	memcpy(buf, (void *)virt_buf, RIO_MAX_MSG_SIZE);

	priv->msg_rx_ring.virt_buffer[buf_idx] = NULL;

      out1:
	setbits32(&priv->msg_regs->imr, RIO_MSG_IMR_MI);

      out2:
	return buf;
}

EXPORT_SYMBOL_GPL(rio_hw_get_inb_message);

static irqreturn_t
fsl_rio_dbell_handler(int irq, void *dev_instance)
{
	int dsr;
	struct rio_mport *port = (struct rio_mport *)dev_instance;
	struct rio_priv *priv = port->priv;

	dsr = in_be32(&priv->msg_regs->dsr);

	if (dsr & DOORBELL_DSR_TE) {
		pr_info("RIO: doorbell reception error\n");
		out_be32(&priv->msg_regs->dsr, DOORBELL_DSR_TE);
		goto out;
	}

	if (dsr & DOORBELL_DSR_QFI) {
		pr_info("RIO: doorbell queue full\n");
		out_be32(&priv->msg_regs->dsr, DOORBELL_DSR_QFI);
		goto out;
	}

	if (dsr & DOORBELL_DSR_DIQI) {
		u32 dmsg =
		    (u32) priv->dbell_ring.virt +
		    (in_be32(&priv->msg_regs->dqdpar) & 0xfff);
		struct rio_dbell *dbell;
		int found = 0;

		pr_debug
		    ("RIO: processing doorbell, sid %2.2x tid %2.2x info %4.4x\n",
		     DBELL_SID(dmsg), DBELL_TID(dmsg), DBELL_INF(dmsg));

		list_for_each_entry(dbell, &port->dbells, node) {
			if ((dbell->res->start <= DBELL_INF(dmsg)) &&
			    (dbell->res->end >= DBELL_INF(dmsg))) {
				found = 1;
				break;
			}
		}
		if (found) {
			dbell->dinb(port, dbell->dev_id, DBELL_SID(dmsg), DBELL_TID(dmsg),
				    DBELL_INF(dmsg));
		} else {
			pr_debug
			    ("RIO: spurious doorbell, sid %2.2x tid %2.2x info %4.4x\n",
			     DBELL_SID(dmsg), DBELL_TID(dmsg), DBELL_INF(dmsg));
		}
		setbits32(&priv->msg_regs->dmr, DOORBELL_DMR_DI);
		out_be32(&priv->msg_regs->dsr, DOORBELL_DSR_DIQI);
	}

      out:
	return IRQ_HANDLED;
}

static int fsl_rio_doorbell_init(struct rio_mport *mport)
{
	struct rio_priv *priv = mport->priv;
	int rc = 0;

	priv->dbell_win = ioremap(mport->iores.start + RIO_MAINT_WIN_SIZE,
			    RIO_DBELL_WIN_SIZE);
	if (!priv->dbell_win) {
		printk(KERN_ERR
		       "RIO: unable to map outbound doorbell window\n");
		rc = -ENOMEM;
		goto out;
	}

	priv->dbell_ring.virt = dma_alloc_coherent(priv->dev, 512 *
		    DOORBELL_MESSAGE_SIZE, &priv->dbell_ring.phys, GFP_KERNEL);
	if (!priv->dbell_ring.virt) {
		printk(KERN_ERR "RIO: unable allocate inbound doorbell ring\n");
		rc = -ENOMEM;
		iounmap(priv->dbell_win);
		goto out;
	}

	out_be32(&priv->msg_regs->dqdpar, (u32) priv->dbell_ring.phys);
	out_be32(&priv->msg_regs->dqepar, (u32) priv->dbell_ring.phys);

	out_be32(&priv->msg_regs->dsr, 0x00000091);

	rc = request_irq(IRQ_RIO_BELL(mport), fsl_rio_dbell_handler, 0,
			 "dbell_rx", (void *)mport);
	if (rc < 0) {
		iounmap(priv->dbell_win);
		dma_free_coherent(priv->dev, 512 * DOORBELL_MESSAGE_SIZE,
				  priv->dbell_ring.virt, priv->dbell_ring.phys);
		printk(KERN_ERR
		       "MPC85xx RIO: unable to request inbound doorbell irq");
		goto out;
	}

	out_be32(&priv->msg_regs->dmr, 0x00108161);

      out:
	return rc;
}

static char *cmdline = NULL;

static int fsl_rio_get_hdid(int index)
{
	 
	if (!cmdline)
		return -1;

	return simple_strtol(cmdline, NULL, 0);
}

static int fsl_rio_get_cmdline(char *s)
{
	if (!s)
		return 0;

	cmdline = s;
	return 1;
}

__setup("riohdid=", fsl_rio_get_cmdline);

static inline void fsl_rio_info(struct device *dev, u32 ccsr)
{
	const char *str;
	if (ccsr & 1) {
		 
		switch (ccsr >> 30) {
		case 0:
			str = "1";
			break;
		case 1:
			str = "4";
			break;
		default:
			str = "Unknown";
			break;
		}
		dev_info(dev, "Hardware port width: %s\n", str);

		switch ((ccsr >> 27) & 7) {
		case 0:
			str = "Single-lane 0";
			break;
		case 1:
			str = "Single-lane 2";
			break;
		case 2:
			str = "Four-lane";
			break;
		default:
			str = "Unknown";
			break;
		}
		dev_info(dev, "Training connection status: %s\n", str);
	} else {
		 
		if (!(ccsr & 0x80000000))
			dev_info(dev, "Output port operating in 8-bit mode\n");
		if (!(ccsr & 0x08000000))
			dev_info(dev, "Input port operating in 8-bit mode\n");
	}
}

int fsl_rio_setup(struct of_device *dev)
{
	struct rio_ops *ops;
	struct rio_mport *port;
	struct rio_priv *priv;
	int rc = 0;
	const u32 *dt_range, *cell;
	struct resource regs;
	int rlen;
	u32 ccsr;
	u64 law_start, law_size;
	int paw, aw, sw;

	if (!dev->node) {
		dev_err(&dev->dev, "Device OF-Node is NULL");
		return -EFAULT;
	}

	rc = of_address_to_resource(dev->node, 0, &regs);
	if (rc) {
		dev_err(&dev->dev, "Can't get %s property 'reg'\n",
				dev->node->full_name);
		return -EFAULT;
	}
	dev_info(&dev->dev, "Of-device full name %s\n", dev->node->full_name);
	dev_info(&dev->dev, "Regs: %pR\n", &regs);

	dt_range = of_get_property(dev->node, "ranges", &rlen);
	if (!dt_range) {
		dev_err(&dev->dev, "Can't get %s property 'ranges'\n",
				dev->node->full_name);
		return -EFAULT;
	}

	cell = of_get_property(dev->node, "#address-cells", NULL);
	if (cell)
		aw = *cell;
	else
		aw = of_n_addr_cells(dev->node);
	 
	cell = of_get_property(dev->node, "#size-cells", NULL);
	if (cell)
		sw = *cell;
	else
		sw = of_n_size_cells(dev->node);
	 
	paw = of_n_addr_cells(dev->node);

	law_start = of_read_number(dt_range + aw, paw);
	law_size = of_read_number(dt_range + aw + paw, sw);

	dev_info(&dev->dev, "LAW start 0x%016llx, size 0x%016llx.\n",
			law_start, law_size);

	ops = kmalloc(sizeof(struct rio_ops), GFP_KERNEL);
	if (!ops) {
		rc = -ENOMEM;
		goto err_ops;
	}
	ops->lcread = fsl_local_config_read;
	ops->lcwrite = fsl_local_config_write;
	ops->cread = fsl_rio_config_read;
	ops->cwrite = fsl_rio_config_write;
	ops->dsend = fsl_rio_doorbell_send;

	port = kzalloc(sizeof(struct rio_mport), GFP_KERNEL);
	if (!port) {
		rc = -ENOMEM;
		goto err_port;
	}
	port->id = 0;
	port->index = 0;

	priv = kzalloc(sizeof(struct rio_priv), GFP_KERNEL);
	if (!priv) {
		printk(KERN_ERR "Can't alloc memory for 'priv'\n");
		rc = -ENOMEM;
		goto err_priv;
	}

	INIT_LIST_HEAD(&port->dbells);
	port->iores.start = law_start;
	port->iores.end = law_start + law_size - 1;
	port->iores.flags = IORESOURCE_MEM;
	port->iores.name = "rio_io_win";

	priv->bellirq = irq_of_parse_and_map(dev->node, 2);
	priv->txirq = irq_of_parse_and_map(dev->node, 3);
	priv->rxirq = irq_of_parse_and_map(dev->node, 4);
	dev_info(&dev->dev, "bellirq: %d, txirq: %d, rxirq %d\n", priv->bellirq,
				priv->txirq, priv->rxirq);

	rio_init_dbell_res(&port->riores[RIO_DOORBELL_RESOURCE], 0, 0xffff);
	rio_init_mbox_res(&port->riores[RIO_INB_MBOX_RESOURCE], 0, 0);
	rio_init_mbox_res(&port->riores[RIO_OUTB_MBOX_RESOURCE], 0, 0);
	strcpy(port->name, "RIO0 mport");

	priv->dev = &dev->dev;

	port->ops = ops;
	port->host_deviceid = fsl_rio_get_hdid(port->id);

	port->priv = priv;
	rio_register_mport(port);

	priv->regs_win = ioremap(regs.start, regs.end - regs.start + 1);

	ccsr = in_be32(priv->regs_win + RIO_CCSR);
	port->phy_type = (ccsr & 1) ? RIO_PHY_SERIAL : RIO_PHY_PARALLEL;
	dev_info(&dev->dev, "RapidIO PHY type: %s\n",
			(port->phy_type == RIO_PHY_PARALLEL) ? "parallel" :
			((port->phy_type == RIO_PHY_SERIAL) ? "serial" :
			 "unknown"));
	 
	if (in_be32((priv->regs_win + RIO_ESCSR)) & 1) {
		dev_err(&dev->dev, "Port is not ready. "
				   "Try to restart connection...\n");
		switch (port->phy_type) {
		case RIO_PHY_SERIAL:
			 
			out_be32(priv->regs_win + RIO_CCSR, 0);
			 
			setbits32(priv->regs_win + RIO_CCSR, 0x02000000);
			 
			setbits32(priv->regs_win + RIO_CCSR, 0x00600000);
			break;
		case RIO_PHY_PARALLEL:
			 
			out_be32(priv->regs_win + RIO_CCSR, 0x22000000);
			 
			out_be32(priv->regs_win + RIO_CCSR, 0x44000000);
			break;
		}
		msleep(100);
		if (in_be32((priv->regs_win + RIO_ESCSR)) & 1) {
			dev_err(&dev->dev, "Port restart failed.\n");
			rc = -ENOLINK;
			goto err;
		}
		dev_info(&dev->dev, "Port restart success!\n");
	}
	fsl_rio_info(&dev->dev, ccsr);

	port->sys_size = (in_be32((priv->regs_win + RIO_PEF_CAR))
					& RIO_PEF_CTLS) >> 4;
	dev_info(&dev->dev, "RapidIO Common Transport System size: %d\n",
			port->sys_size ? 65536 : 256);

	priv->atmu_regs = (struct rio_atmu_regs *)(priv->regs_win
					+ RIO_ATMU_REGS_OFFSET);
	priv->maint_atmu_regs = priv->atmu_regs + 1;
	priv->dbell_atmu_regs = priv->atmu_regs + 2;
	priv->msg_regs = (struct rio_msg_regs *)(priv->regs_win +
				((port->phy_type == RIO_PHY_SERIAL) ?
				RIO_S_MSG_REGS_OFFSET : RIO_P_MSG_REGS_OFFSET));

	if (port->phy_type == RIO_PHY_SERIAL)
		out_be32((priv->regs_win + RIO_ISR_AACR), RIO_ISR_AACR_AA);

	out_be32(&priv->maint_atmu_regs->rowbar, law_start >> 12);
	out_be32(&priv->maint_atmu_regs->rowar, 0x80077015);	 

	priv->maint_win = ioremap(law_start, RIO_MAINT_WIN_SIZE);

	out_be32(&priv->dbell_atmu_regs->rowbar,
			(law_start + RIO_MAINT_WIN_SIZE) >> 12);
	out_be32(&priv->dbell_atmu_regs->rowar, 0x8004200b);	 
	fsl_rio_doorbell_init(port);

	return 0;
err:
	iounmap(priv->regs_win);
	kfree(priv);
err_priv:
	kfree(port);
err_port:
	kfree(ops);
err_ops:
	return rc;
}

static int __devinit fsl_of_rio_rpn_probe(struct of_device *dev,
				     const struct of_device_id *match)
{
	int rc;
	printk(KERN_INFO "Setting up RapidIO peer-to-peer network %s\n",
			dev->node->full_name);

	rc = fsl_rio_setup(dev);
	if (rc)
		goto out;

	rc = rio_init_mports();
out:
	return rc;
};

static const struct of_device_id fsl_of_rio_rpn_ids[] = {
	{
		.compatible = "fsl,rapidio-delta",
	},
	{},
};

static struct of_platform_driver fsl_of_rio_rpn_driver = {
	.name = "fsl-of-rio",
	.match_table = fsl_of_rio_rpn_ids,
	.probe = fsl_of_rio_rpn_probe,
};

static __init int fsl_of_rio_rpn_init(void)
{
	return of_register_platform_driver(&fsl_of_rio_rpn_driver);
}

subsys_initcall(fsl_of_rio_rpn_init);
