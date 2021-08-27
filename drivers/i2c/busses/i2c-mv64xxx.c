#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/mv643xx_i2c.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/delay.h>

#define	MV64XXX_I2C_REG_SLAVE_ADDR			0x00
#define	MV64XXX_I2C_REG_DATA				0x04
#define	MV64XXX_I2C_REG_CONTROL				0x08
#define	MV64XXX_I2C_REG_STATUS				0x0c
#define	MV64XXX_I2C_REG_BAUD				0x0c
#define	MV64XXX_I2C_REG_EXT_SLAVE_ADDR			0x10
#define	MV64XXX_I2C_REG_SOFT_RESET			0x1c

#define	MV64XXX_I2C_REG_CONTROL_ACK			0x00000004
#define	MV64XXX_I2C_REG_CONTROL_IFLG			0x00000008
#define	MV64XXX_I2C_REG_CONTROL_STOP			0x00000010
#define	MV64XXX_I2C_REG_CONTROL_START			0x00000020
#define	MV64XXX_I2C_REG_CONTROL_TWSIEN			0x00000040
#define	MV64XXX_I2C_REG_CONTROL_INTEN			0x00000080

#define	MV64XXX_I2C_STATUS_BUS_ERR			0x00
#define	MV64XXX_I2C_STATUS_MAST_START			0x08
#define	MV64XXX_I2C_STATUS_MAST_REPEAT_START		0x10
#define	MV64XXX_I2C_STATUS_MAST_WR_ADDR_ACK		0x18
#define	MV64XXX_I2C_STATUS_MAST_WR_ADDR_NO_ACK		0x20
#define	MV64XXX_I2C_STATUS_MAST_WR_ACK			0x28
#define	MV64XXX_I2C_STATUS_MAST_WR_NO_ACK		0x30
#define	MV64XXX_I2C_STATUS_MAST_LOST_ARB		0x38
#define	MV64XXX_I2C_STATUS_MAST_RD_ADDR_ACK		0x40
#define	MV64XXX_I2C_STATUS_MAST_RD_ADDR_NO_ACK		0x48
#define	MV64XXX_I2C_STATUS_MAST_RD_DATA_ACK		0x50
#define	MV64XXX_I2C_STATUS_MAST_RD_DATA_NO_ACK		0x58
#define	MV64XXX_I2C_STATUS_MAST_WR_ADDR_2_ACK		0xd0
#define	MV64XXX_I2C_STATUS_MAST_WR_ADDR_2_NO_ACK	0xd8
#define	MV64XXX_I2C_STATUS_MAST_RD_ADDR_2_ACK		0xe0
#define	MV64XXX_I2C_STATUS_MAST_RD_ADDR_2_NO_ACK	0xe8
#define	MV64XXX_I2C_STATUS_NO_STATUS			0xf8

#ifdef CONFIG_SYNO_ARMADA_V2
 
#define	MV64XXX_I2C_REG_TX_DATA_LO			0xC0
#define	MV64XXX_I2C_REG_TX_DATA_HI			0xC4
#define	MV64XXX_I2C_REG_RX_DATA_LO			0xC8
#define	MV64XXX_I2C_REG_RX_DATA_HI			0xCC
#define	MV64XXX_I2C_REG_BRIDGE_CONTROL			0xD0
#define	MV64XXX_I2C_REG_BRIDGE_STATUS			0xD4
#define	MV64XXX_I2C_REG_BRIDGE_INTR_CAUSE		0xD8
#define	MV64XXX_I2C_REG_BRIDGE_INTR_MASK		0xDC
#define	MV64XXX_I2C_REG_BRIDGE_TIMING			0xE0

#define	MV64XXX_I2C_BRIDGE_CONTROL_WR			0x00000001
#define	MV64XXX_I2C_BRIDGE_CONTROL_RD			0x00000002
#define	MV64XXX_I2C_BRIDGE_CONTROL_ADDR_SHIFT		2
#define	MV64XXX_I2C_BRIDGE_CONTROL_ADDR_EXT		0x00001000
#define	MV64XXX_I2C_BRIDGE_CONTROL_TX_SIZE_SHIFT	13
#define	MV64XXX_I2C_BRIDGE_CONTROL_RX_SIZE_SHIFT	16
#define	MV64XXX_I2C_BRIDGE_CONTROL_ENABLE		0x00080000

#define	MV64XXX_I2C_BRIDGE_STATUS_ERROR			0x00000001
#endif

enum {
	MV64XXX_I2C_STATE_INVALID,
	MV64XXX_I2C_STATE_IDLE,
	MV64XXX_I2C_STATE_WAITING_FOR_START_COND,
	MV64XXX_I2C_STATE_WAITING_FOR_ADDR_1_ACK,
	MV64XXX_I2C_STATE_WAITING_FOR_ADDR_2_ACK,
	MV64XXX_I2C_STATE_WAITING_FOR_SLAVE_ACK,
	MV64XXX_I2C_STATE_WAITING_FOR_SLAVE_DATA,
};

enum {
	MV64XXX_I2C_ACTION_INVALID,
	MV64XXX_I2C_ACTION_CONTINUE,
	MV64XXX_I2C_ACTION_SEND_START,
	MV64XXX_I2C_ACTION_SEND_ADDR_1,
	MV64XXX_I2C_ACTION_SEND_ADDR_2,
	MV64XXX_I2C_ACTION_SEND_DATA,
	MV64XXX_I2C_ACTION_RCV_DATA,
	MV64XXX_I2C_ACTION_RCV_DATA_STOP,
	MV64XXX_I2C_ACTION_SEND_STOP,
};

struct mv64xxx_i2c_data {
	int			irq;
	u32			state;
	u32			action;
	u32			aborting;
	u32			cntl_bits;
	void __iomem		*reg_base;
	u32			reg_base_p;
	u32			reg_size;
	u32			addr1;
	u32			addr2;
	u32			bytes_left;
	u32			byte_posn;
	u32			block;
	int			rc;
	u32			freq_m;
	u32			freq_n;
	wait_queue_head_t	waitq;
	spinlock_t		lock;
	struct i2c_msg		*msg;
	struct i2c_adapter	adapter;
};

static void
mv64xxx_i2c_hw_init(struct mv64xxx_i2c_data *drv_data)
{
	writel(0, drv_data->reg_base + MV64XXX_I2C_REG_SOFT_RESET);
	writel((((drv_data->freq_m & 0xf) << 3) | (drv_data->freq_n & 0x7)),
		drv_data->reg_base + MV64XXX_I2C_REG_BAUD);
	writel(0, drv_data->reg_base + MV64XXX_I2C_REG_SLAVE_ADDR);
	writel(0, drv_data->reg_base + MV64XXX_I2C_REG_EXT_SLAVE_ADDR);
	writel(MV64XXX_I2C_REG_CONTROL_TWSIEN | MV64XXX_I2C_REG_CONTROL_STOP,
		drv_data->reg_base + MV64XXX_I2C_REG_CONTROL);

#ifdef CONFIG_SYNO_ARMADA_V2
#ifdef CONFIG_I2C_MV64XXX_BRIDGE
	writel(0, drv_data->reg_base + MV64XXX_I2C_REG_BRIDGE_CONTROL);
	writel(0, drv_data->reg_base + MV64XXX_I2C_REG_BRIDGE_TIMING);
	writel(0, drv_data->reg_base + MV64XXX_I2C_REG_BRIDGE_INTR_CAUSE);
	writel(0, drv_data->reg_base + MV64XXX_I2C_REG_BRIDGE_INTR_MASK);
#endif  
#endif

	drv_data->state = MV64XXX_I2C_STATE_IDLE;
}

static void
mv64xxx_i2c_fsm(struct mv64xxx_i2c_data *drv_data, u32 status)
{
	 
	if (drv_data->state == MV64XXX_I2C_STATE_IDLE) {
		drv_data->action = MV64XXX_I2C_ACTION_SEND_STOP;
		return;
	}

	switch (status) {
	 
	case MV64XXX_I2C_STATUS_MAST_START:  
	case MV64XXX_I2C_STATUS_MAST_REPEAT_START:  
		drv_data->action = MV64XXX_I2C_ACTION_SEND_ADDR_1;
		drv_data->state = MV64XXX_I2C_STATE_WAITING_FOR_ADDR_1_ACK;
		break;

	case MV64XXX_I2C_STATUS_MAST_WR_ADDR_ACK:  
		if (drv_data->msg->flags & I2C_M_TEN) {
			drv_data->action = MV64XXX_I2C_ACTION_SEND_ADDR_2;
			drv_data->state =
				MV64XXX_I2C_STATE_WAITING_FOR_ADDR_2_ACK;
			break;
		}
		 
	case MV64XXX_I2C_STATUS_MAST_WR_ADDR_2_ACK:  
	case MV64XXX_I2C_STATUS_MAST_WR_ACK:  
		if ((drv_data->bytes_left == 0)
				|| (drv_data->aborting
					&& (drv_data->byte_posn != 0))) {
				drv_data->action = MV64XXX_I2C_ACTION_SEND_STOP;
				drv_data->state = MV64XXX_I2C_STATE_IDLE;
			} else {
			drv_data->action = MV64XXX_I2C_ACTION_SEND_DATA;
			drv_data->state =
				MV64XXX_I2C_STATE_WAITING_FOR_SLAVE_ACK;
			drv_data->bytes_left--;
		}
		break;

	case MV64XXX_I2C_STATUS_MAST_RD_ADDR_ACK:  
		if (drv_data->msg->flags & I2C_M_TEN) {
			drv_data->action = MV64XXX_I2C_ACTION_SEND_ADDR_2;
			drv_data->state =
				MV64XXX_I2C_STATE_WAITING_FOR_ADDR_2_ACK;
			break;
		}
		 
	case MV64XXX_I2C_STATUS_MAST_RD_ADDR_2_ACK:  
		if (drv_data->bytes_left == 0) {
			drv_data->action = MV64XXX_I2C_ACTION_SEND_STOP;
			drv_data->state = MV64XXX_I2C_STATE_IDLE;
			break;
		}
		 
	case MV64XXX_I2C_STATUS_MAST_RD_DATA_ACK:  
		if (status != MV64XXX_I2C_STATUS_MAST_RD_DATA_ACK)
			drv_data->action = MV64XXX_I2C_ACTION_CONTINUE;
		else {
			drv_data->action = MV64XXX_I2C_ACTION_RCV_DATA;
			drv_data->bytes_left--;
		}
		drv_data->state = MV64XXX_I2C_STATE_WAITING_FOR_SLAVE_DATA;

		if ((drv_data->bytes_left == 1) || drv_data->aborting)
			drv_data->cntl_bits &= ~MV64XXX_I2C_REG_CONTROL_ACK;
		break;

	case MV64XXX_I2C_STATUS_MAST_RD_DATA_NO_ACK:  
		drv_data->action = MV64XXX_I2C_ACTION_RCV_DATA_STOP;
		drv_data->state = MV64XXX_I2C_STATE_IDLE;
		break;

	case MV64XXX_I2C_STATUS_MAST_WR_ADDR_NO_ACK:  
	case MV64XXX_I2C_STATUS_MAST_WR_NO_ACK:  
	case MV64XXX_I2C_STATUS_MAST_RD_ADDR_NO_ACK:  
		 
		drv_data->action = MV64XXX_I2C_ACTION_SEND_STOP;
		drv_data->state = MV64XXX_I2C_STATE_IDLE;
		drv_data->rc = -ENODEV;
		break;

	default:
		dev_err(&drv_data->adapter.dev,
			"mv64xxx_i2c_fsm: Ctlr Error -- state: 0x%x, "
			"status: 0x%x, addr: 0x%x, flags: 0x%x\n",
			 drv_data->state, status, drv_data->msg->addr,
			 drv_data->msg->flags);
		drv_data->action = MV64XXX_I2C_ACTION_SEND_STOP;
		mv64xxx_i2c_hw_init(drv_data);
		drv_data->rc = -EIO;
	}
}

static void
mv64xxx_i2c_do_action(struct mv64xxx_i2c_data *drv_data)
{
	switch(drv_data->action) {
	case MV64XXX_I2C_ACTION_CONTINUE:
		writel(drv_data->cntl_bits,
			drv_data->reg_base + MV64XXX_I2C_REG_CONTROL);
		break;

	case MV64XXX_I2C_ACTION_SEND_START:
		writel(drv_data->cntl_bits | MV64XXX_I2C_REG_CONTROL_START,
			drv_data->reg_base + MV64XXX_I2C_REG_CONTROL);
		break;

	case MV64XXX_I2C_ACTION_SEND_ADDR_1:
		writel(drv_data->addr1,
			drv_data->reg_base + MV64XXX_I2C_REG_DATA);
		writel(drv_data->cntl_bits,
			drv_data->reg_base + MV64XXX_I2C_REG_CONTROL);
		break;

	case MV64XXX_I2C_ACTION_SEND_ADDR_2:
		writel(drv_data->addr2,
			drv_data->reg_base + MV64XXX_I2C_REG_DATA);
		writel(drv_data->cntl_bits,
			drv_data->reg_base + MV64XXX_I2C_REG_CONTROL);
		break;

	case MV64XXX_I2C_ACTION_SEND_DATA:
		writel(drv_data->msg->buf[drv_data->byte_posn++],
			drv_data->reg_base + MV64XXX_I2C_REG_DATA);
		writel(drv_data->cntl_bits,
			drv_data->reg_base + MV64XXX_I2C_REG_CONTROL);
		break;

	case MV64XXX_I2C_ACTION_RCV_DATA:
		drv_data->msg->buf[drv_data->byte_posn++] =
			readl(drv_data->reg_base + MV64XXX_I2C_REG_DATA);
		writel(drv_data->cntl_bits,
			drv_data->reg_base + MV64XXX_I2C_REG_CONTROL);
		break;

	case MV64XXX_I2C_ACTION_RCV_DATA_STOP:
		drv_data->msg->buf[drv_data->byte_posn++] =
			readl(drv_data->reg_base + MV64XXX_I2C_REG_DATA);
		drv_data->cntl_bits &= ~MV64XXX_I2C_REG_CONTROL_INTEN;
		writel(drv_data->cntl_bits | MV64XXX_I2C_REG_CONTROL_STOP,
			drv_data->reg_base + MV64XXX_I2C_REG_CONTROL);
		drv_data->block = 0;
#if defined(CONFIG_ARMADA_XP_REV_A0) || defined(CONFIG_ARMADA_XP_REV_B0)
		 
		udelay(5);
#endif  
		wake_up_interruptible(&drv_data->waitq);
		break;

	case MV64XXX_I2C_ACTION_INVALID:
	default:
		dev_err(&drv_data->adapter.dev,
			"mv64xxx_i2c_do_action: Invalid action: %d\n",
			drv_data->action);
		drv_data->rc = -EIO;
		 
	case MV64XXX_I2C_ACTION_SEND_STOP:
		drv_data->cntl_bits &= ~MV64XXX_I2C_REG_CONTROL_INTEN;
		writel(drv_data->cntl_bits | MV64XXX_I2C_REG_CONTROL_STOP,
			drv_data->reg_base + MV64XXX_I2C_REG_CONTROL);
		drv_data->block = 0;

		wake_up_interruptible(&drv_data->waitq);
		break;
	}
}

static irqreturn_t
mv64xxx_i2c_intr(int irq, void *dev_id)
{
	struct mv64xxx_i2c_data	*drv_data = dev_id;
	unsigned long	flags;
	u32		status;
	irqreturn_t	rc = IRQ_NONE;

	spin_lock_irqsave(&drv_data->lock, flags);
#ifdef CONFIG_SYNO_ARMADA_V2
#ifdef CONFIG_I2C_MV64XXX_BRIDGE
	if (readl(drv_data->reg_base + MV64XXX_I2C_REG_BRIDGE_INTR_CAUSE)) {
		writel(0, drv_data->reg_base +
					MV64XXX_I2C_REG_BRIDGE_CONTROL);
		writel(0, drv_data->reg_base +
					MV64XXX_I2C_REG_BRIDGE_INTR_CAUSE);
		drv_data->block = 0;
		wake_up_interruptible(&drv_data->waitq);
		rc = IRQ_HANDLED;
	}
#endif  
#endif
	while (readl(drv_data->reg_base + MV64XXX_I2C_REG_CONTROL) &
						MV64XXX_I2C_REG_CONTROL_IFLG) {
		status = readl(drv_data->reg_base + MV64XXX_I2C_REG_STATUS);
		mv64xxx_i2c_fsm(drv_data, status);
		mv64xxx_i2c_do_action(drv_data);
		rc = IRQ_HANDLED;
	}
	spin_unlock_irqrestore(&drv_data->lock, flags);

	return rc;
}

static void
mv64xxx_i2c_prepare_for_io(struct mv64xxx_i2c_data *drv_data,
	struct i2c_msg *msg)
{
	u32	dir = 0;

	drv_data->msg = msg;
	drv_data->byte_posn = 0;
	drv_data->bytes_left = msg->len;
	drv_data->aborting = 0;
	drv_data->rc = 0;
	drv_data->cntl_bits = MV64XXX_I2C_REG_CONTROL_ACK |
		MV64XXX_I2C_REG_CONTROL_INTEN | MV64XXX_I2C_REG_CONTROL_TWSIEN;

	if (msg->flags & I2C_M_RD)
		dir = 1;

	if (msg->flags & I2C_M_TEN) {
		drv_data->addr1 = 0xf0 | (((u32)msg->addr & 0x300) >> 7) | dir;
		drv_data->addr2 = (u32)msg->addr & 0xff;
	} else {
		drv_data->addr1 = ((u32)msg->addr & 0x7f) << 1 | dir;
		drv_data->addr2 = 0;
	}
}

static void
mv64xxx_i2c_wait_for_completion(struct mv64xxx_i2c_data *drv_data)
{
	long		time_left;
	unsigned long	flags;
	char		abort = 0;

	time_left = wait_event_interruptible_timeout(drv_data->waitq,
		!drv_data->block, drv_data->adapter.timeout);

	spin_lock_irqsave(&drv_data->lock, flags);
	if (!time_left) {  
		drv_data->rc = -ETIMEDOUT;
		abort = 1;
	} else if (time_left < 0) {  
		drv_data->rc = time_left;  
		abort = 1;
	}

	if (abort && drv_data->block) {
		drv_data->aborting = 1;
		spin_unlock_irqrestore(&drv_data->lock, flags);

		time_left = wait_event_timeout(drv_data->waitq,
			!drv_data->block, drv_data->adapter.timeout);

		if ((time_left <= 0) && drv_data->block) {
			drv_data->state = MV64XXX_I2C_STATE_IDLE;
			dev_err(&drv_data->adapter.dev,
				"mv64xxx: I2C bus locked, block: %d, "
				"time_left: %d\n", drv_data->block,
				(int)time_left);
			mv64xxx_i2c_hw_init(drv_data);
		}
	} else
		spin_unlock_irqrestore(&drv_data->lock, flags);
}

static int
mv64xxx_i2c_execute_msg(struct mv64xxx_i2c_data *drv_data, struct i2c_msg *msg)
{
	unsigned long	flags;

	spin_lock_irqsave(&drv_data->lock, flags);
	mv64xxx_i2c_prepare_for_io(drv_data, msg);

	if (unlikely(msg->flags & I2C_M_NOSTART)) {  
		if (drv_data->msg->flags & I2C_M_RD) {
			 
			drv_data->action = MV64XXX_I2C_ACTION_CONTINUE;
			drv_data->state =
				MV64XXX_I2C_STATE_WAITING_FOR_SLAVE_DATA;
		} else {
			drv_data->action = MV64XXX_I2C_ACTION_SEND_DATA;
			drv_data->state =
				MV64XXX_I2C_STATE_WAITING_FOR_SLAVE_ACK;
			drv_data->bytes_left--;
		}
	} else {
			drv_data->action = MV64XXX_I2C_ACTION_SEND_START;
		drv_data->state = MV64XXX_I2C_STATE_WAITING_FOR_START_COND;
	}

	drv_data->block = 1;
	mv64xxx_i2c_do_action(drv_data);
	spin_unlock_irqrestore(&drv_data->lock, flags);

	mv64xxx_i2c_wait_for_completion(drv_data);
	return drv_data->rc;
}

#ifdef CONFIG_SYNO_ARMADA_V2
#ifdef CONFIG_I2C_MV64XXX_BRIDGE
static int
mv64xxx_i2c_offload_msg(struct mv64xxx_i2c_data *drv_data, struct i2c_msg *msg)
{
	unsigned long data_reg_hi = 0;
	unsigned long data_reg_lo = 0;
	unsigned long status_reg;
	unsigned long ctrl_reg;
	unsigned long flags;
	unsigned int i;

	if ((msg->flags & ~(I2C_M_TEN | I2C_M_RD)) != 0)
		return 1;

	if (msg->len < 1 || msg->len > 8)
		return 1;

	ctrl_reg = MV64XXX_I2C_BRIDGE_CONTROL_ENABLE |
		   (msg->addr << MV64XXX_I2C_BRIDGE_CONTROL_ADDR_SHIFT);

	if ((msg->flags & I2C_M_TEN) != 0)
		ctrl_reg |=  MV64XXX_I2C_BRIDGE_CONTROL_ADDR_EXT;

	if ((msg->flags & I2C_M_RD) == 0) {
		for (i = 0; i < 4 && i < msg->len; i++)
			data_reg_lo = data_reg_lo |
					(msg->buf[i] << ((i & 0x3) * 8));

		for (i = 4; i < 8 && i < msg->len; i++)
			data_reg_hi = data_reg_hi |
					(msg->buf[i] << ((i & 0x3) * 8));

		ctrl_reg |= MV64XXX_I2C_BRIDGE_CONTROL_WR |
		    (msg->len - 1) << MV64XXX_I2C_BRIDGE_CONTROL_TX_SIZE_SHIFT;
	} else {
		ctrl_reg |= MV64XXX_I2C_BRIDGE_CONTROL_RD |
		    (msg->len - 1) << MV64XXX_I2C_BRIDGE_CONTROL_RX_SIZE_SHIFT;
	}

	spin_lock_irqsave(&drv_data->lock, flags);
	drv_data->block = 1;
	writel(data_reg_lo, drv_data->reg_base + MV64XXX_I2C_REG_TX_DATA_LO);
	writel(data_reg_hi, drv_data->reg_base + MV64XXX_I2C_REG_TX_DATA_HI);
	writel(ctrl_reg, drv_data->reg_base + MV64XXX_I2C_REG_BRIDGE_CONTROL);
	spin_unlock_irqrestore(&drv_data->lock, flags);

	mv64xxx_i2c_wait_for_completion(drv_data);

	spin_lock_irqsave(&drv_data->lock, flags);
	status_reg = readl(drv_data->reg_base + MV64XXX_I2C_REG_BRIDGE_STATUS);
	data_reg_lo = readl(drv_data->reg_base + MV64XXX_I2C_REG_RX_DATA_LO);
	data_reg_hi = readl(drv_data->reg_base + MV64XXX_I2C_REG_RX_DATA_HI);
	spin_unlock_irqrestore(&drv_data->lock, flags);

	if (status_reg & MV64XXX_I2C_BRIDGE_STATUS_ERROR)
		return -EIO;

	if ((msg->flags & I2C_M_RD) != 0) {
		for (i = 0; i < 4 && i < msg->len; i++) {
			msg->buf[i] = data_reg_lo & 0xFF;
			data_reg_lo >>= 8;
		}

		for (i = 4; i < 8 && i < msg->len; i++) {
			msg->buf[i] = data_reg_hi & 0xFF;
			data_reg_hi >>= 8;
		}
	}

	return 0;
}
#endif  
#endif
 
static u32
mv64xxx_i2c_functionality(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_10BIT_ADDR | I2C_FUNC_SMBUS_EMUL;
}

static int
mv64xxx_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg msgs[], int num)
{
	struct mv64xxx_i2c_data *drv_data = i2c_get_adapdata(adap);
	int	i, rc;

	for (i=0; i<num; i++) {
#ifdef CONFIG_SYNO_ARMADA_V2
#ifdef CONFIG_I2C_MV64XXX_BRIDGE
		rc = mv64xxx_i2c_offload_msg(drv_data, &msgs[i]);
		if (rc == 0)
			continue;
		if (rc < 0)
			return rc;
#endif  
#endif

		if ((rc = mv64xxx_i2c_execute_msg(drv_data, &msgs[i])) < 0)
			return rc;
	}
	return num;
}

static const struct i2c_algorithm mv64xxx_i2c_algo = {
	.master_xfer = mv64xxx_i2c_xfer,
	.functionality = mv64xxx_i2c_functionality,
};

static int __devinit
mv64xxx_i2c_map_regs(struct platform_device *pd,
	struct mv64xxx_i2c_data *drv_data)
{
	int size;
	struct resource	*r = platform_get_resource(pd, IORESOURCE_MEM, 0);

	if (!r)
		return -ENODEV;

	size = resource_size(r);

	if (!request_mem_region(r->start, size, drv_data->adapter.name))
		return -EBUSY;

	drv_data->reg_base = ioremap(r->start, size);
	drv_data->reg_base_p = r->start;
	drv_data->reg_size = size;

	return 0;
}

static void
mv64xxx_i2c_unmap_regs(struct mv64xxx_i2c_data *drv_data)
{
	if (drv_data->reg_base) {
		iounmap(drv_data->reg_base);
		release_mem_region(drv_data->reg_base_p, drv_data->reg_size);
	}

	drv_data->reg_base = NULL;
	drv_data->reg_base_p = 0;
}

static int __devinit
mv64xxx_i2c_probe(struct platform_device *pd)
{
	struct mv64xxx_i2c_data		*drv_data;
	struct mv64xxx_i2c_pdata	*pdata = pd->dev.platform_data;
	int	rc;

#if defined(CONFIG_SYNO_ARMADA) || defined(CONFIG_SYNO_ARMADA_V2)
	if ( (pd->id > 1) || (pd->id < 0) || !pdata)
#else
	if ((pd->id != 0) || !pdata)
#endif
		return -ENODEV;

	drv_data = kzalloc(sizeof(struct mv64xxx_i2c_data), GFP_KERNEL);
	if (!drv_data)
		return -ENOMEM;

	if (mv64xxx_i2c_map_regs(pd, drv_data)) {
		rc = -ENODEV;
		goto exit_kfree;
	}

	strlcpy(drv_data->adapter.name, MV64XXX_I2C_CTLR_NAME " adapter",
		sizeof(drv_data->adapter.name));

	init_waitqueue_head(&drv_data->waitq);
	spin_lock_init(&drv_data->lock);

	drv_data->freq_m = pdata->freq_m;
	drv_data->freq_n = pdata->freq_n;
	drv_data->irq = platform_get_irq(pd, 0);
	if (drv_data->irq < 0) {
		rc = -ENXIO;
		goto exit_unmap_regs;
	}
	drv_data->adapter.dev.parent = &pd->dev;
	drv_data->adapter.algo = &mv64xxx_i2c_algo;
	drv_data->adapter.owner = THIS_MODULE;
	drv_data->adapter.class = I2C_CLASS_HWMON | I2C_CLASS_SPD;
	drv_data->adapter.timeout = msecs_to_jiffies(pdata->timeout);
	drv_data->adapter.nr = pd->id;
	platform_set_drvdata(pd, drv_data);
	i2c_set_adapdata(&drv_data->adapter, drv_data);

	mv64xxx_i2c_hw_init(drv_data);

	if (request_irq(drv_data->irq, mv64xxx_i2c_intr, 0,
			MV64XXX_I2C_CTLR_NAME, drv_data)) {
		dev_err(&drv_data->adapter.dev,
			"mv64xxx: Can't register intr handler irq: %d\n",
			drv_data->irq);
		rc = -EINVAL;
		goto exit_unmap_regs;
	} else if ((rc = i2c_add_numbered_adapter(&drv_data->adapter)) != 0) {
		dev_err(&drv_data->adapter.dev,
			"mv64xxx: Can't add i2c adapter, rc: %d\n", -rc);
		goto exit_free_irq;
	}

	return 0;

	exit_free_irq:
		free_irq(drv_data->irq, drv_data);
	exit_unmap_regs:
		mv64xxx_i2c_unmap_regs(drv_data);
	exit_kfree:
		kfree(drv_data);
	return rc;
}

static int __devexit
mv64xxx_i2c_remove(struct platform_device *dev)
{
	struct mv64xxx_i2c_data		*drv_data = platform_get_drvdata(dev);
	int	rc;

	rc = i2c_del_adapter(&drv_data->adapter);
	free_irq(drv_data->irq, drv_data);
	mv64xxx_i2c_unmap_regs(drv_data);
	kfree(drv_data);

	return rc;
}

static struct platform_driver mv64xxx_i2c_driver = {
	.probe	= mv64xxx_i2c_probe,
	.remove	= __devexit_p(mv64xxx_i2c_remove),
	.driver	= {
		.owner	= THIS_MODULE,
		.name	= MV64XXX_I2C_CTLR_NAME,
	},
};

static int __init
mv64xxx_i2c_init(void)
{
	return platform_driver_register(&mv64xxx_i2c_driver);
}

static void __exit
mv64xxx_i2c_exit(void)
{
	platform_driver_unregister(&mv64xxx_i2c_driver);
}

module_init(mv64xxx_i2c_init);
module_exit(mv64xxx_i2c_exit);

MODULE_AUTHOR("Mark A. Greer <mgreer@mvista.com>");
MODULE_DESCRIPTION("Marvell mv64xxx host bridge i2c ctlr driver");
MODULE_LICENSE("GPL");
