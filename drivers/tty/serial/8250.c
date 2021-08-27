#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#if defined(CONFIG_SERIAL_8250_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/tty.h>
#include <linux/ratelimit.h>
#include <linux/tty_flip.h>
#include <linux/serial_reg.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/serial_8250.h>
#include <linux/nmi.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#ifdef CONFIG_GEN3_UART
#include <linux/pci.h>
#endif

#include <asm/io.h>
#include <asm/irq.h>

#include "8250.h"

#ifdef CONFIG_SPARC
#include "suncore.h"
#endif

#if defined(CONFIG_SYNO_COMCERTO) && defined(CONFIG_ARCH_M86XXX)
#include <linux/clk.h>
#include <mach/reset.h>
static struct clk *uart_clk;      
#endif 

static unsigned int share_irqs = SERIAL8250_SHARE_IRQS;

static unsigned int nr_uarts = CONFIG_SERIAL_8250_RUNTIME_UARTS;

static struct uart_driver serial8250_reg;

static int serial_index(struct uart_port *port)
{
	return (serial8250_reg.minor - 64) + port->line;
}

static unsigned int skip_txen_test;  

#if 0
#define DEBUG_AUTOCONF(fmt...)	printk(fmt)
#else
#define DEBUG_AUTOCONF(fmt...)	do { } while (0)
#endif

#if 0
#define DEBUG_INTR(fmt...)	printk(fmt)
#else
#define DEBUG_INTR(fmt...)	do { } while (0)
#endif

#define PASS_LIMIT	512

#define BOTH_EMPTY 	(UART_LSR_TEMT | UART_LSR_THRE)

#define is_real_interrupt(irq)	((irq) != 0)

#ifdef CONFIG_SERIAL_8250_DETECT_IRQ
#define CONFIG_SERIAL_DETECT_IRQ 1
#endif
#ifdef CONFIG_SERIAL_8250_MANY_PORTS
#define CONFIG_SERIAL_MANY_PORTS 1
#endif

#define CONFIG_HUB6 1

#include <asm/serial.h>
 
#ifndef SERIAL_PORT_DFNS
#define SERIAL_PORT_DFNS
#endif

static const struct old_serial_port old_serial_port[] = {
	SERIAL_PORT_DFNS  
};

#define UART_NR	CONFIG_SERIAL_8250_NR_UARTS

#ifdef CONFIG_SERIAL_8250_RSA

#define PORT_RSA_MAX 4
static unsigned long probe_rsa[PORT_RSA_MAX];
static unsigned int probe_rsa_count;
#endif  

struct uart_8250_port {
	struct uart_port	port;
	struct timer_list	timer;		 
	struct list_head	list;		 
	unsigned short		capabilities;	 
	unsigned short		bugs;		 
	unsigned int		tx_loadsz;	 
	unsigned char		acr;
	unsigned char		ier;
	unsigned char		lcr;
	unsigned char		mcr;
	unsigned char		mcr_mask;	 
	unsigned char		mcr_force;	 
	unsigned char		cur_iotype;	 

#define LSR_SAVE_FLAGS UART_LSR_BRK_ERROR_BITS
	unsigned char		lsr_saved_flags;
#define MSR_SAVE_FLAGS UART_MSR_ANY_DELTA
	unsigned char		msr_saved_flags;
};

struct irq_info {
	struct			hlist_node node;
	int			irq;
	spinlock_t		lock;	 
	struct list_head	*head;
};

#define NR_IRQ_HASH		32	 
static struct hlist_head irq_lists[NR_IRQ_HASH];
static DEFINE_MUTEX(hash_mutex);	 

static const struct serial8250_config uart_config[] = {
	[PORT_UNKNOWN] = {
		.name		= "unknown",
		.fifo_size	= 1,
		.tx_loadsz	= 1,
	},
	[PORT_8250] = {
		.name		= "8250",
		.fifo_size	= 1,
		.tx_loadsz	= 1,
	},
	[PORT_16450] = {
		.name		= "16450",
		.fifo_size	= 1,
		.tx_loadsz	= 1,
	},
	[PORT_16550] = {
		.name		= "16550",
		.fifo_size	= 1,
		.tx_loadsz	= 1,
	},
	[PORT_16550A] = {
		.name		= "16550A",
		.fifo_size	= 16,
		.tx_loadsz	= 16,
		.fcr		= UART_FCR_ENABLE_FIFO | UART_FCR_R_TRIG_10,
		.flags		= UART_CAP_FIFO,
	},
	[PORT_CIRRUS] = {
		.name		= "Cirrus",
		.fifo_size	= 1,
		.tx_loadsz	= 1,
	},
	[PORT_16650] = {
		.name		= "ST16650",
		.fifo_size	= 1,
		.tx_loadsz	= 1,
		.flags		= UART_CAP_FIFO | UART_CAP_EFR | UART_CAP_SLEEP,
	},
	[PORT_16650V2] = {
		.name		= "ST16650V2",
		.fifo_size	= 32,
		.tx_loadsz	= 16,
		.fcr		= UART_FCR_ENABLE_FIFO | UART_FCR_R_TRIG_01 |
				  UART_FCR_T_TRIG_00,
		.flags		= UART_CAP_FIFO | UART_CAP_EFR | UART_CAP_SLEEP,
	},
	[PORT_16750] = {
		.name		= "TI16750",
		.fifo_size	= 64,
		.tx_loadsz	= 64,
		.fcr		= UART_FCR_ENABLE_FIFO | UART_FCR_R_TRIG_10 |
				  UART_FCR7_64BYTE,
		.flags		= UART_CAP_FIFO | UART_CAP_SLEEP | UART_CAP_AFE,
	},
	[PORT_STARTECH] = {
		.name		= "Startech",
		.fifo_size	= 1,
		.tx_loadsz	= 1,
	},
	[PORT_16C950] = {
		.name		= "16C950/954",
		.fifo_size	= 128,
		.tx_loadsz	= 128,
		.fcr		= UART_FCR_ENABLE_FIFO | UART_FCR_R_TRIG_10,
		 
		.flags		= UART_CAP_FIFO | UART_CAP_SLEEP,
	},
	[PORT_16654] = {
		.name		= "ST16654",
		.fifo_size	= 64,
		.tx_loadsz	= 32,
		.fcr		= UART_FCR_ENABLE_FIFO | UART_FCR_R_TRIG_01 |
				  UART_FCR_T_TRIG_10,
		.flags		= UART_CAP_FIFO | UART_CAP_EFR | UART_CAP_SLEEP,
	},
	[PORT_16850] = {
		.name		= "XR16850",
		.fifo_size	= 128,
		.tx_loadsz	= 128,
		.fcr		= UART_FCR_ENABLE_FIFO | UART_FCR_R_TRIG_10,
		.flags		= UART_CAP_FIFO | UART_CAP_EFR | UART_CAP_SLEEP,
	},
	[PORT_RSA] = {
		.name		= "RSA",
		.fifo_size	= 2048,
		.tx_loadsz	= 2048,
		.fcr		= UART_FCR_ENABLE_FIFO | UART_FCR_R_TRIG_11,
		.flags		= UART_CAP_FIFO,
	},
	[PORT_NS16550A] = {
		.name		= "NS16550A",
		.fifo_size	= 16,
		.tx_loadsz	= 16,
		.fcr		= UART_FCR_ENABLE_FIFO | UART_FCR_R_TRIG_10,
		.flags		= UART_CAP_FIFO | UART_NATSEMI,
	},
	[PORT_XSCALE] = {
#ifdef CONFIG_GEN3_UART
 
		.name		= "GEN3_serial",
		.fifo_size	= 64,
		.tx_loadsz	= 64,
		.fcr		= UART_FCR_ENABLE_FIFO | UART_FCR_DMA_SELECT | UART_FCR_R_TRIG_10,
#else
		.name		= "XScale",
		.fifo_size	= 32,
		.tx_loadsz	= 32,
		.fcr		= UART_FCR_ENABLE_FIFO | UART_FCR_R_TRIG_10,
#endif
		.flags		= UART_CAP_FIFO | UART_CAP_UUE | UART_CAP_RTOIE,
	},
	[PORT_RM9000] = {
		.name		= "RM9000",
		.fifo_size	= 16,
		.tx_loadsz	= 16,
		.fcr		= UART_FCR_ENABLE_FIFO | UART_FCR_R_TRIG_10,
		.flags		= UART_CAP_FIFO,
	},
	[PORT_OCTEON] = {
		.name		= "OCTEON",
		.fifo_size	= 64,
		.tx_loadsz	= 64,
		.fcr		= UART_FCR_ENABLE_FIFO | UART_FCR_R_TRIG_10,
		.flags		= UART_CAP_FIFO,
	},
	[PORT_AR7] = {
		.name		= "AR7",
		.fifo_size	= 16,
		.tx_loadsz	= 16,
		.fcr		= UART_FCR_ENABLE_FIFO | UART_FCR_R_TRIG_00,
		.flags		= UART_CAP_FIFO | UART_CAP_AFE,
	},
	[PORT_U6_16550A] = {
		.name		= "U6_16550A",
		.fifo_size	= 64,
		.tx_loadsz	= 64,
		.fcr		= UART_FCR_ENABLE_FIFO | UART_FCR_R_TRIG_10,
		.flags		= UART_CAP_FIFO | UART_CAP_AFE,
	},
	[PORT_TEGRA] = {
		.name		= "Tegra",
		.fifo_size	= 32,
		.tx_loadsz	= 8,
		.fcr		= UART_FCR_ENABLE_FIFO | UART_FCR_R_TRIG_01 |
				  UART_FCR_T_TRIG_01,
		.flags		= UART_CAP_FIFO | UART_CAP_RTOIE,
	},
	[PORT_XR17D15X] = {
		.name		= "XR17D15X",
		.fifo_size	= 64,
		.tx_loadsz	= 64,
		.fcr		= UART_FCR_ENABLE_FIFO | UART_FCR_R_TRIG_10,
		.flags		= UART_CAP_FIFO | UART_CAP_AFE | UART_CAP_EFR,
	},
	[PORT_BRCM_TRUMANAGE] = {
		.name		= "TruManage",
		.fifo_size	= 1,
		.tx_loadsz	= 1024,
		.flags		= UART_CAP_HFIFO,
	},
};

#if defined(CONFIG_MIPS_ALCHEMY)

static const u8 au_io_in_map[] = {
	[UART_RX]  = 0,
	[UART_IER] = 2,
	[UART_IIR] = 3,
	[UART_LCR] = 5,
	[UART_MCR] = 6,
	[UART_LSR] = 7,
	[UART_MSR] = 8,
};

static const u8 au_io_out_map[] = {
	[UART_TX]  = 1,
	[UART_IER] = 2,
	[UART_FCR] = 4,
	[UART_LCR] = 5,
	[UART_MCR] = 6,
};

static inline int map_8250_in_reg(struct uart_port *p, int offset)
{
	if (p->iotype != UPIO_AU)
		return offset;
	return au_io_in_map[offset];
}

static inline int map_8250_out_reg(struct uart_port *p, int offset)
{
	if (p->iotype != UPIO_AU)
		return offset;
	return au_io_out_map[offset];
}

#elif defined(CONFIG_SERIAL_8250_RM9K)

static const u8
	regmap_in[8] = {
		[UART_RX]	= 0x00,
		[UART_IER]	= 0x0c,
		[UART_IIR]	= 0x14,
		[UART_LCR]	= 0x1c,
		[UART_MCR]	= 0x20,
		[UART_LSR]	= 0x24,
		[UART_MSR]	= 0x28,
		[UART_SCR]	= 0x2c
	},
	regmap_out[8] = {
		[UART_TX] 	= 0x04,
		[UART_IER]	= 0x0c,
		[UART_FCR]	= 0x18,
		[UART_LCR]	= 0x1c,
		[UART_MCR]	= 0x20,
		[UART_LSR]	= 0x24,
		[UART_MSR]	= 0x28,
		[UART_SCR]	= 0x2c
	};

static inline int map_8250_in_reg(struct uart_port *p, int offset)
{
	if (p->iotype != UPIO_RM9000)
		return offset;
	return regmap_in[offset];
}

static inline int map_8250_out_reg(struct uart_port *p, int offset)
{
	if (p->iotype != UPIO_RM9000)
		return offset;
	return regmap_out[offset];
}

#else

#define map_8250_in_reg(up, offset) (offset)
#define map_8250_out_reg(up, offset) (offset)

#endif

static unsigned int hub6_serial_in(struct uart_port *p, int offset)
{
	offset = map_8250_in_reg(p, offset) << p->regshift;
	outb(p->hub6 - 1 + offset, p->iobase);
	return inb(p->iobase + 1);
}

static void hub6_serial_out(struct uart_port *p, int offset, int value)
{
	offset = map_8250_out_reg(p, offset) << p->regshift;
	outb(p->hub6 - 1 + offset, p->iobase);
	outb(value, p->iobase + 1);
}

static unsigned int mem_serial_in(struct uart_port *p, int offset)
{
	offset = map_8250_in_reg(p, offset) << p->regshift;
	return readb(p->membase + offset);
}

#if defined(CONFIG_SYNO_COMCERTO) && defined(CONFIG_ARCH_M86XXX)
void serial8250_clear_and_reinit_fifos(struct uart_8250_port *p);

void dw8250_force_idle(struct uart_port *p)
{
	struct uart_8250_port *up = container_of(p, struct uart_8250_port, port);

	serial8250_clear_and_reinit_fifos(up);
	(void)p->serial_in(p, UART_RX);
}
#endif

static void mem_serial_out(struct uart_port *p, int offset, int value)
{
#if defined(CONFIG_SYNO_COMCERTO) && defined(CONFIG_ARCH_M86XXX)
	int offset1 = map_8250_out_reg(p, offset) << p->regshift;
	writeb(value, p->membase + offset1);

	if (offset == UART_LCR) {
		int tries = 1000;
		while (tries--) {
			if (value == p->serial_in(p, UART_LCR))
				return;
			dw8250_force_idle(p);
			writeb(value, p->membase + offset1);
		}
		dev_err(p->dev, "Couldn't set LCR to %d\n", value);
	}
#else
	offset = map_8250_out_reg(p, offset) << p->regshift;
	writeb(value, p->membase + offset);
#endif
}

static void mem32_serial_out(struct uart_port *p, int offset, int value)
{
	offset = map_8250_out_reg(p, offset) << p->regshift;
	writel(value, p->membase + offset);
}

static unsigned int mem32_serial_in(struct uart_port *p, int offset)
{
	offset = map_8250_in_reg(p, offset) << p->regshift;
	return readl(p->membase + offset);
}

static unsigned int au_serial_in(struct uart_port *p, int offset)
{
	offset = map_8250_in_reg(p, offset) << p->regshift;
	return __raw_readl(p->membase + offset);
}

static void au_serial_out(struct uart_port *p, int offset, int value)
{
	offset = map_8250_out_reg(p, offset) << p->regshift;
	__raw_writel(value, p->membase + offset);
}

#if defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2) || defined(CONFIG_SYNO_C2K_SERIAL_FIX)
 
static inline void dwapb_save_out_value(struct uart_port *p, int offset,
                                       int value)
{
       struct uart_8250_port *up =
               container_of(p, struct uart_8250_port, port);

       if (offset == UART_LCR)
               up->lcr = value;
}

static inline void dwapb_check_clear_ier(struct uart_port *p, int offset)
{
       if (offset == UART_TX || offset == UART_IER)
               p->serial_in(p, UART_IER);
}

static void dwapb_serial_out(struct uart_port *p, int offset, int value)
{
       int save_offset = offset;
       offset = map_8250_out_reg(p, offset) << p->regshift;
#if defined(CONFIG_PLAT_ARMADA)  || defined(CONFIG_SYNO_ARMADA_ARCH_V2) || defined(CONFIG_SYNO_C2K_SERIAL_FIX)
        
       if ((((readb(p->membase + 0xC) & 0x80) &&
            ((save_offset == UART_DLL) || (save_offset == UART_DLM) ||
             (offset == 0x1C))) ||
            (save_offset == UART_LCR)) && !(readb(p->membase + 0x14) & 0x1)) {
               unsigned int status;
               do {
                       status = *((volatile u32 *)p->private_data);
               } while (status & 0x1);
       }
#endif

       dwapb_save_out_value(p, save_offset, value);
       writeb(value, p->membase + offset);
       dwapb_check_clear_ier(p, save_offset);
}
#endif

static unsigned int io_serial_in(struct uart_port *p, int offset)
{
	offset = map_8250_in_reg(p, offset) << p->regshift;
	return inb(p->iobase + offset);
}

static void io_serial_out(struct uart_port *p, int offset, int value)
{
	offset = map_8250_out_reg(p, offset) << p->regshift;
	outb(value, p->iobase + offset);
}

static int serial8250_default_handle_irq(struct uart_port *port);

static void set_io_from_upio(struct uart_port *p)
{
	struct uart_8250_port *up =
		container_of(p, struct uart_8250_port, port);
	switch (p->iotype) {
	case UPIO_HUB6:
		p->serial_in = hub6_serial_in;
		p->serial_out = hub6_serial_out;
		break;

	case UPIO_MEM:
		p->serial_in = mem_serial_in;
		p->serial_out = mem_serial_out;
		break;

	case UPIO_RM9000:
#if defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2) || defined(CONFIG_SYNO_C2K_SERIAL_FIX)
		p->serial_in = mem_serial_in;
		p->serial_out = dwapb_serial_out;
		break;
#endif
	case UPIO_MEM32:
		p->serial_in = mem32_serial_in;
		p->serial_out = mem32_serial_out;
		break;

	case UPIO_AU:
		p->serial_in = au_serial_in;
		p->serial_out = au_serial_out;
		break;

	default:
		p->serial_in = io_serial_in;
		p->serial_out = io_serial_out;
		break;
	}
	 
	up->cur_iotype = p->iotype;
	p->handle_irq = serial8250_default_handle_irq;
}

static void
serial_out_sync(struct uart_8250_port *up, int offset, int value)
{
	struct uart_port *p = &up->port;
	switch (p->iotype) {
	case UPIO_MEM:
	case UPIO_MEM32:
	case UPIO_AU:
		p->serial_out(p, offset, value);
		p->serial_in(p, UART_LCR);	 
		break;
	default:
		p->serial_out(p, offset, value);
	}
}

#define serial_in(up, offset)		\
	(up->port.serial_in(&(up)->port, (offset)))
#define serial_out(up, offset, value)	\
	(up->port.serial_out(&(up)->port, (offset), (value)))
 
#define serial_inp(up, offset)		serial_in(up, offset)
#define serial_outp(up, offset, value)	serial_out(up, offset, value)

static inline int _serial_dl_read(struct uart_8250_port *up)
{
	return serial_inp(up, UART_DLL) | serial_inp(up, UART_DLM) << 8;
}

static inline void _serial_dl_write(struct uart_8250_port *up, int value)
{
	serial_outp(up, UART_DLL, value & 0xff);
	serial_outp(up, UART_DLM, value >> 8 & 0xff);
}

#if defined(CONFIG_MIPS_ALCHEMY)
 
static int serial_dl_read(struct uart_8250_port *up)
{
	if (up->port.iotype == UPIO_AU)
		return __raw_readl(up->port.membase + 0x28);
	else
		return _serial_dl_read(up);
}

static void serial_dl_write(struct uart_8250_port *up, int value)
{
	if (up->port.iotype == UPIO_AU)
		__raw_writel(value, up->port.membase + 0x28);
	else
		_serial_dl_write(up, value);
}
#elif defined(CONFIG_SERIAL_8250_RM9K)
static int serial_dl_read(struct uart_8250_port *up)
{
	return	(up->port.iotype == UPIO_RM9000) ?
		(((__raw_readl(up->port.membase + 0x10) << 8) |
		(__raw_readl(up->port.membase + 0x08) & 0xff)) & 0xffff) :
		_serial_dl_read(up);
}

static void serial_dl_write(struct uart_8250_port *up, int value)
{
	if (up->port.iotype == UPIO_RM9000) {
		__raw_writel(value, up->port.membase + 0x08);
		__raw_writel(value >> 8, up->port.membase + 0x10);
	} else {
		_serial_dl_write(up, value);
	}
}
#else
#define serial_dl_read(up) _serial_dl_read(up)
#define serial_dl_write(up, value) _serial_dl_write(up, value)
#endif

static void serial_icr_write(struct uart_8250_port *up, int offset, int value)
{
	serial_out(up, UART_SCR, offset);
	serial_out(up, UART_ICR, value);
}

static unsigned int serial_icr_read(struct uart_8250_port *up, int offset)
{
	unsigned int value;

	serial_icr_write(up, UART_ACR, up->acr | UART_ACR_ICRRD);
	serial_out(up, UART_SCR, offset);
	value = serial_in(up, UART_ICR);
	serial_icr_write(up, UART_ACR, up->acr);

	return value;
}

static void serial8250_clear_fifos(struct uart_8250_port *p)
{
	if (p->capabilities & UART_CAP_FIFO) {
		serial_outp(p, UART_FCR, UART_FCR_ENABLE_FIFO);
		serial_outp(p, UART_FCR, UART_FCR_ENABLE_FIFO |
			       UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT);
		serial_outp(p, UART_FCR, 0);
	}
}

#if defined(CONFIG_SYNO_COMCERTO) && defined(CONFIG_ARCH_M86XXX)
void serial8250_clear_and_reinit_fifos(struct uart_8250_port *p)
{
         unsigned char fcr;

         serial8250_clear_fifos(p);
         fcr = uart_config[p->port.type].fcr;
         serial_outp(p, UART_FCR, fcr);
}
#endif

static void serial8250_set_sleep(struct uart_8250_port *p, int sleep)
{
	if (p->capabilities & UART_CAP_SLEEP) {
		if (p->capabilities & UART_CAP_EFR) {
			serial_outp(p, UART_LCR, UART_LCR_CONF_MODE_B);
			serial_outp(p, UART_EFR, UART_EFR_ECB);
			serial_outp(p, UART_LCR, 0);
		}
		serial_outp(p, UART_IER, sleep ? UART_IERX_SLEEP : 0);
		if (p->capabilities & UART_CAP_EFR) {
			serial_outp(p, UART_LCR, UART_LCR_CONF_MODE_B);
			serial_outp(p, UART_EFR, 0);
			serial_outp(p, UART_LCR, 0);
		}
	}
}

#ifdef CONFIG_SERIAL_8250_RSA
 
static int __enable_rsa(struct uart_8250_port *up)
{
	unsigned char mode;
	int result;

	mode = serial_inp(up, UART_RSA_MSR);
	result = mode & UART_RSA_MSR_FIFO;

	if (!result) {
		serial_outp(up, UART_RSA_MSR, mode | UART_RSA_MSR_FIFO);
		mode = serial_inp(up, UART_RSA_MSR);
		result = mode & UART_RSA_MSR_FIFO;
	}

	if (result)
		up->port.uartclk = SERIAL_RSA_BAUD_BASE * 16;

	return result;
}

static void enable_rsa(struct uart_8250_port *up)
{
	if (up->port.type == PORT_RSA) {
		if (up->port.uartclk != SERIAL_RSA_BAUD_BASE * 16) {
			spin_lock_irq(&up->port.lock);
			__enable_rsa(up);
			spin_unlock_irq(&up->port.lock);
		}
		if (up->port.uartclk == SERIAL_RSA_BAUD_BASE * 16)
			serial_outp(up, UART_RSA_FRR, 0);
	}
}

static void disable_rsa(struct uart_8250_port *up)
{
	unsigned char mode;
	int result;

	if (up->port.type == PORT_RSA &&
	    up->port.uartclk == SERIAL_RSA_BAUD_BASE * 16) {
		spin_lock_irq(&up->port.lock);

		mode = serial_inp(up, UART_RSA_MSR);
		result = !(mode & UART_RSA_MSR_FIFO);

		if (!result) {
			serial_outp(up, UART_RSA_MSR, mode & ~UART_RSA_MSR_FIFO);
			mode = serial_inp(up, UART_RSA_MSR);
			result = !(mode & UART_RSA_MSR_FIFO);
		}

		if (result)
			up->port.uartclk = SERIAL_RSA_BAUD_BASE_LO * 16;
		spin_unlock_irq(&up->port.lock);
	}
}
#endif  

static int size_fifo(struct uart_8250_port *up)
{
	unsigned char old_fcr, old_mcr, old_lcr;
	unsigned short old_dl;
	int count;

	old_lcr = serial_inp(up, UART_LCR);
	serial_outp(up, UART_LCR, 0);
	old_fcr = serial_inp(up, UART_FCR);
	old_mcr = serial_inp(up, UART_MCR);
	serial_outp(up, UART_FCR, UART_FCR_ENABLE_FIFO |
		    UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT);
	serial_outp(up, UART_MCR, UART_MCR_LOOP);
	serial_outp(up, UART_LCR, UART_LCR_CONF_MODE_A);
	old_dl = serial_dl_read(up);
	serial_dl_write(up, 0x0001);
	serial_outp(up, UART_LCR, 0x03);
	for (count = 0; count < 256; count++)
		serial_outp(up, UART_TX, count);
	mdelay(20); 
	for (count = 0; (serial_inp(up, UART_LSR) & UART_LSR_DR) &&
	     (count < 256); count++)
		serial_inp(up, UART_RX);
	serial_outp(up, UART_FCR, old_fcr);
	serial_outp(up, UART_MCR, old_mcr);
	serial_outp(up, UART_LCR, UART_LCR_CONF_MODE_A);
	serial_dl_write(up, old_dl);
	serial_outp(up, UART_LCR, old_lcr);

	return count;
}

static unsigned int autoconfig_read_divisor_id(struct uart_8250_port *p)
{
	unsigned char old_dll, old_dlm, old_lcr;
	unsigned int id;

	old_lcr = serial_inp(p, UART_LCR);
	serial_outp(p, UART_LCR, UART_LCR_CONF_MODE_A);

	old_dll = serial_inp(p, UART_DLL);
	old_dlm = serial_inp(p, UART_DLM);

	serial_outp(p, UART_DLL, 0);
	serial_outp(p, UART_DLM, 0);

	id = serial_inp(p, UART_DLL) | serial_inp(p, UART_DLM) << 8;

	serial_outp(p, UART_DLL, old_dll);
	serial_outp(p, UART_DLM, old_dlm);
	serial_outp(p, UART_LCR, old_lcr);

	return id;
}

static void autoconfig_has_efr(struct uart_8250_port *up)
{
	unsigned int id1, id2, id3, rev;

	up->capabilities |= UART_CAP_EFR | UART_CAP_SLEEP;

	up->acr = 0;
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_B);
	serial_out(up, UART_EFR, UART_EFR_ECB);
	serial_out(up, UART_LCR, 0x00);
	id1 = serial_icr_read(up, UART_ID1);
	id2 = serial_icr_read(up, UART_ID2);
	id3 = serial_icr_read(up, UART_ID3);
	rev = serial_icr_read(up, UART_REV);

	DEBUG_AUTOCONF("950id=%02x:%02x:%02x:%02x ", id1, id2, id3, rev);

	if (id1 == 0x16 && id2 == 0xC9 &&
	    (id3 == 0x50 || id3 == 0x52 || id3 == 0x54)) {
		up->port.type = PORT_16C950;

		if (id3 == 0x52 && rev == 0x01)
			up->bugs |= UART_BUG_QUOT;
		return;
	}

	id1 = autoconfig_read_divisor_id(up);
	DEBUG_AUTOCONF("850id=%04x ", id1);

	id2 = id1 >> 8;
	if (id2 == 0x10 || id2 == 0x12 || id2 == 0x14) {
		up->port.type = PORT_16850;
		return;
	}

	if (size_fifo(up) == 64)
		up->port.type = PORT_16654;
	else
		up->port.type = PORT_16650V2;
}

static void autoconfig_8250(struct uart_8250_port *up)
{
	unsigned char scratch, status1, status2;

	up->port.type = PORT_8250;

	scratch = serial_in(up, UART_SCR);
	serial_outp(up, UART_SCR, 0xa5);
	status1 = serial_in(up, UART_SCR);
	serial_outp(up, UART_SCR, 0x5a);
	status2 = serial_in(up, UART_SCR);
	serial_outp(up, UART_SCR, scratch);

	if (status1 == 0xa5 && status2 == 0x5a)
		up->port.type = PORT_16450;
}

static int broken_efr(struct uart_8250_port *up)
{
	 
	if (autoconfig_read_divisor_id(up) == 0x0201 && size_fifo(up) == 16)
		return 1;

	return 0;
}

static inline int ns16550a_goto_highspeed(struct uart_8250_port *up)
{
	unsigned char status;

	status = serial_in(up, 0x04);  
#define PRESL(x) ((x) & 0x30)
	if (PRESL(status) == 0x10) {
		 
		return 0;
	} else {
		status &= ~0xB0;  
		status |= 0x10;   
		serial_outp(up, 0x04, status);
	}
	return 1;
}

static void autoconfig_16550a(struct uart_8250_port *up)
{
	unsigned char status1, status2;
	unsigned int iersave;

	up->port.type = PORT_16550A;
	up->capabilities |= UART_CAP_FIFO;

	serial_outp(up, UART_LCR, UART_LCR_CONF_MODE_A);
	if (serial_in(up, UART_EFR) == 0) {
		serial_outp(up, UART_EFR, 0xA8);
		if (serial_in(up, UART_EFR) != 0) {
			DEBUG_AUTOCONF("EFRv1 ");
			up->port.type = PORT_16650;
			up->capabilities |= UART_CAP_EFR | UART_CAP_SLEEP;
		} else {
			DEBUG_AUTOCONF("Motorola 8xxx DUART ");
		}
		serial_outp(up, UART_EFR, 0);
		return;
	}

	serial_outp(up, UART_LCR, UART_LCR_CONF_MODE_B);
	if (serial_in(up, UART_EFR) == 0 && !broken_efr(up)) {
		DEBUG_AUTOCONF("EFRv2 ");
		autoconfig_has_efr(up);
		return;
	}

	serial_outp(up, UART_LCR, 0);
	status1 = serial_in(up, UART_MCR);
	serial_outp(up, UART_LCR, 0xE0);
	status2 = serial_in(up, 0x02);  

	if (!((status2 ^ status1) & UART_MCR_LOOP)) {
		serial_outp(up, UART_LCR, 0);
		serial_outp(up, UART_MCR, status1 ^ UART_MCR_LOOP);
		serial_outp(up, UART_LCR, 0xE0);
		status2 = serial_in(up, 0x02);  
		serial_outp(up, UART_LCR, 0);
		serial_outp(up, UART_MCR, status1);

		if ((status2 ^ status1) & UART_MCR_LOOP) {
			unsigned short quot;

			serial_outp(up, UART_LCR, 0xE0);

			quot = serial_dl_read(up);
			quot <<= 3;

			if (ns16550a_goto_highspeed(up))
				serial_dl_write(up, quot);

			serial_outp(up, UART_LCR, 0);

			up->port.uartclk = 921600*16;
			up->port.type = PORT_NS16550A;
			up->capabilities |= UART_NATSEMI;
			return;
		}
	}

	serial_outp(up, UART_LCR, 0);
	serial_outp(up, UART_FCR, UART_FCR_ENABLE_FIFO | UART_FCR7_64BYTE);
	status1 = serial_in(up, UART_IIR) >> 5;
	serial_outp(up, UART_FCR, UART_FCR_ENABLE_FIFO);
	serial_outp(up, UART_LCR, UART_LCR_CONF_MODE_A);
	serial_outp(up, UART_FCR, UART_FCR_ENABLE_FIFO | UART_FCR7_64BYTE);
	status2 = serial_in(up, UART_IIR) >> 5;
	serial_outp(up, UART_FCR, UART_FCR_ENABLE_FIFO);
	serial_outp(up, UART_LCR, 0);

	DEBUG_AUTOCONF("iir1=%d iir2=%d ", status1, status2);

	if (status1 == 6 && status2 == 7) {
		up->port.type = PORT_16750;
		up->capabilities |= UART_CAP_AFE | UART_CAP_SLEEP;
		return;
	}

	iersave = serial_in(up, UART_IER);
	serial_outp(up, UART_IER, iersave & ~UART_IER_UUE);
	if (!(serial_in(up, UART_IER) & UART_IER_UUE)) {
		 
		serial_outp(up, UART_IER, iersave | UART_IER_UUE);
		if (serial_in(up, UART_IER) & UART_IER_UUE) {
			 
			DEBUG_AUTOCONF("Xscale ");
			up->port.type = PORT_XSCALE;
#if defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2) || defined(CONFIG_SYNO_C2K_SERIAL_FIX)
			up->capabilities |= UART_CAP_UUE;
#else
			up->capabilities |= UART_CAP_UUE | UART_CAP_RTOIE;
#endif
			return;
		}
	} else {
		 
		DEBUG_AUTOCONF("Couldn't force IER_UUE to 0 ");
	}
	serial_outp(up, UART_IER, iersave);

	if (up->port.flags & UPF_EXAR_EFR) {
		up->port.type = PORT_XR17D15X;
		up->capabilities |= UART_CAP_AFE | UART_CAP_EFR;
	}

	if (up->port.type == PORT_16550A && size_fifo(up) == 64) {
		up->port.type = PORT_U6_16550A;
		up->capabilities |= UART_CAP_AFE;
	}
}

static void autoconfig(struct uart_8250_port *up, unsigned int probeflags)
{
	unsigned char status1, scratch, scratch2, scratch3;
	unsigned char save_lcr, save_mcr;
	unsigned long flags;
#ifdef CONFIG_GEN3_UART
	unsigned int id;
#endif

	if (!up->port.iobase && !up->port.mapbase && !up->port.membase)
		return;

	DEBUG_AUTOCONF("ttyS%d: autoconf (0x%04lx, 0x%p): ",
		       serial_index(&up->port), up->port.iobase, up->port.membase);

	spin_lock_irqsave(&up->port.lock, flags);

	up->capabilities = 0;
#ifdef CONFIG_GEN3_UART
	 
	intelce_get_soc_info(&id, NULL);
	if((CE4200_SOC_DEVICE_ID == id) && (0 == serial_index(&up->port)))
		up->bugs = UART_BUG_NOMSR;
	else
#endif
	up->bugs = 0;

	if (!(up->port.flags & UPF_BUGGY_UART)) {
		 
		scratch = serial_inp(up, UART_IER);
		serial_outp(up, UART_IER, 0);
#ifdef __i386__
		outb(0xff, 0x080);
#endif
		 
		scratch2 = serial_inp(up, UART_IER) & 0x0f;
		serial_outp(up, UART_IER, 0x0F);
#ifdef __i386__
		outb(0, 0x080);
#endif
		scratch3 = serial_inp(up, UART_IER) & 0x0f;
		serial_outp(up, UART_IER, scratch);
		if (scratch2 != 0 || scratch3 != 0x0F) {
			 
			DEBUG_AUTOCONF("IER test failed (%02x, %02x) ",
				       scratch2, scratch3);
			goto out;
		}
	}

	save_mcr = serial_in(up, UART_MCR);
	save_lcr = serial_in(up, UART_LCR);

	if (!(up->port.flags & UPF_SKIP_TEST)) {
		serial_outp(up, UART_MCR, UART_MCR_LOOP | 0x0A);
		status1 = serial_inp(up, UART_MSR) & 0xF0;
		serial_outp(up, UART_MCR, save_mcr);
		if (status1 != 0x90) {
			DEBUG_AUTOCONF("LOOP test failed (%02x) ",
				       status1);
			goto out;
		}
	}

	serial_outp(up, UART_LCR, UART_LCR_CONF_MODE_B);
	serial_outp(up, UART_EFR, 0);
	serial_outp(up, UART_LCR, 0);

	serial_outp(up, UART_FCR, UART_FCR_ENABLE_FIFO);
	scratch = serial_in(up, UART_IIR) >> 6;

	DEBUG_AUTOCONF("iir=%d ", scratch);

	switch (scratch) {
	case 0:
		autoconfig_8250(up);
		break;
	case 1:
		up->port.type = PORT_UNKNOWN;
		break;
	case 2:
		up->port.type = PORT_16550;
		break;
	case 3:
		autoconfig_16550a(up);
		break;
	}

#ifdef CONFIG_SERIAL_8250_RSA
	 
	if (up->port.type == PORT_16550A && probeflags & PROBE_RSA) {
		int i;

		for (i = 0 ; i < probe_rsa_count; ++i) {
			if (probe_rsa[i] == up->port.iobase &&
			    __enable_rsa(up)) {
				up->port.type = PORT_RSA;
				break;
			}
		}
	}
#endif

	serial_outp(up, UART_LCR, save_lcr);

	if (up->capabilities != uart_config[up->port.type].flags) {
		printk(KERN_WARNING
		       "ttyS%d: detected caps %08x should be %08x\n",
		       serial_index(&up->port), up->capabilities,
		       uart_config[up->port.type].flags);
	}

	up->port.fifosize = uart_config[up->port.type].fifo_size;
	up->capabilities = uart_config[up->port.type].flags;
	up->tx_loadsz = uart_config[up->port.type].tx_loadsz;

	if (up->port.type == PORT_UNKNOWN)
		goto out;

#ifdef CONFIG_SERIAL_8250_RSA
	if (up->port.type == PORT_RSA)
		serial_outp(up, UART_RSA_FRR, 0);
#endif
	serial_outp(up, UART_MCR, save_mcr);
	serial8250_clear_fifos(up);
	serial_in(up, UART_RX);
	if (up->capabilities & UART_CAP_UUE)
		serial_outp(up, UART_IER, UART_IER_UUE);
	else
		serial_outp(up, UART_IER, 0);

 out:
	spin_unlock_irqrestore(&up->port.lock, flags);
	DEBUG_AUTOCONF("type=%s\n", uart_config[up->port.type].name);
}

static void autoconfig_irq(struct uart_8250_port *up)
{
	unsigned char save_mcr, save_ier;
	unsigned char save_ICP = 0;
	unsigned int ICP = 0;
	unsigned long irqs;
	int irq;

	if (up->port.flags & UPF_FOURPORT) {
		ICP = (up->port.iobase & 0xfe0) | 0x1f;
		save_ICP = inb_p(ICP);
		outb_p(0x80, ICP);
		(void) inb_p(ICP);
	}

	probe_irq_off(probe_irq_on());
	save_mcr = serial_inp(up, UART_MCR);
	save_ier = serial_inp(up, UART_IER);
	serial_outp(up, UART_MCR, UART_MCR_OUT1 | UART_MCR_OUT2);

	irqs = probe_irq_on();
	serial_outp(up, UART_MCR, 0);
	udelay(10);
	if (up->port.flags & UPF_FOURPORT) {
		serial_outp(up, UART_MCR,
			    UART_MCR_DTR | UART_MCR_RTS);
	} else {
		serial_outp(up, UART_MCR,
			    UART_MCR_DTR | UART_MCR_RTS | UART_MCR_OUT2);
	}
	serial_outp(up, UART_IER, 0x0f);	 
	(void)serial_inp(up, UART_LSR);
	(void)serial_inp(up, UART_RX);
	(void)serial_inp(up, UART_IIR);
	(void)serial_inp(up, UART_MSR);
	serial_outp(up, UART_TX, 0xFF);
	udelay(20);
	irq = probe_irq_off(irqs);

	serial_outp(up, UART_MCR, save_mcr);
	serial_outp(up, UART_IER, save_ier);

	if (up->port.flags & UPF_FOURPORT)
		outb_p(save_ICP, ICP);

	up->port.irq = (irq > 0) ? irq : 0;
}

static inline void __stop_tx(struct uart_8250_port *p)
{
	if (p->ier & UART_IER_THRI) {
		p->ier &= ~UART_IER_THRI;
		serial_out(p, UART_IER, p->ier);
	}
}

static void serial8250_stop_tx(struct uart_port *port)
{
	struct uart_8250_port *up =
		container_of(port, struct uart_8250_port, port);

	__stop_tx(up);

	if (up->port.type == PORT_16C950) {
		up->acr |= UART_ACR_TXDIS;
		serial_icr_write(up, UART_ACR, up->acr);
	}
}

static void transmit_chars(struct uart_8250_port *up);

static void serial8250_start_tx(struct uart_port *port)
{
	struct uart_8250_port *up =
		container_of(port, struct uart_8250_port, port);

	if (!(up->ier & UART_IER_THRI)) {
		up->ier |= UART_IER_THRI;
		serial_out(up, UART_IER, up->ier);

		if (up->bugs & UART_BUG_TXEN) {
			unsigned char lsr;
			lsr = serial_in(up, UART_LSR);
			up->lsr_saved_flags |= lsr & LSR_SAVE_FLAGS;
			if ((up->port.type == PORT_RM9000) ?
				(lsr & UART_LSR_THRE) :
				(lsr & UART_LSR_TEMT))
				transmit_chars(up);
		}
	}

	if (up->port.type == PORT_16C950 && up->acr & UART_ACR_TXDIS) {
		up->acr &= ~UART_ACR_TXDIS;
		serial_icr_write(up, UART_ACR, up->acr);
	}
}

static void serial8250_stop_rx(struct uart_port *port)
{
	struct uart_8250_port *up =
		container_of(port, struct uart_8250_port, port);

	up->ier &= ~UART_IER_RLSI;
	up->port.read_status_mask &= ~UART_LSR_DR;
	serial_out(up, UART_IER, up->ier);
}

static void serial8250_enable_ms(struct uart_port *port)
{
	struct uart_8250_port *up =
		container_of(port, struct uart_8250_port, port);

	if (up->bugs & UART_BUG_NOMSR)
		return;

	up->ier |= UART_IER_MSI;
	serial_out(up, UART_IER, up->ier);
}

static void clear_rx_fifo(struct uart_8250_port *up)
{
	unsigned int status, tmout = 10000;
	do {
		status = serial_in(up, UART_LSR);
		if (status & (UART_LSR_FIFOE | UART_LSR_BRK_ERROR_BITS))
			status = serial_in(up, UART_RX);
		else
			break;
		if (--tmout == 0)
			break;
		udelay(1);
	} while (1);
}

static void
receive_chars(struct uart_8250_port *up, unsigned int *status)
{
	struct tty_struct *tty = up->port.state->port.tty;
	unsigned char ch, lsr = *status;
	int max_count = 256;
	char flag;

	do {
		if (likely(lsr & UART_LSR_DR))
			ch = serial_inp(up, UART_RX);
		else
			 
			ch = 0;

		flag = TTY_NORMAL;
		up->port.icount.rx++;

		lsr |= up->lsr_saved_flags;
		up->lsr_saved_flags = 0;

		if (unlikely(lsr & UART_LSR_BRK_ERROR_BITS)) {
			 
			if (lsr & UART_LSR_BI) {
				lsr &= ~(UART_LSR_FE | UART_LSR_PE);
				up->port.icount.brk++;
				 
				if (up->port.type == PORT_TEGRA)
					clear_rx_fifo(up);

				if (uart_handle_break(&up->port))
					goto ignore_char;
			} else if (lsr & UART_LSR_PE)
				up->port.icount.parity++;
			else if (lsr & UART_LSR_FE)
				up->port.icount.frame++;
			if (lsr & UART_LSR_OE)
				up->port.icount.overrun++;

			lsr &= up->port.read_status_mask;

			if (lsr & UART_LSR_BI) {
				DEBUG_INTR("handling break....");
				flag = TTY_BREAK;
			} else if (lsr & UART_LSR_PE)
				flag = TTY_PARITY;
			else if (lsr & UART_LSR_FE)
				flag = TTY_FRAME;
		}
		if (uart_handle_sysrq_char(&up->port, ch))
			goto ignore_char;

		uart_insert_char(&up->port, lsr, UART_LSR_OE, ch, flag);

ignore_char:
		lsr = serial_inp(up, UART_LSR);
	} while ((lsr & (UART_LSR_DR | UART_LSR_BI)) && (max_count-- > 0));
	spin_unlock(&up->port.lock);
	tty_flip_buffer_push(tty);
	spin_lock(&up->port.lock);
	*status = lsr;
}

static void transmit_chars(struct uart_8250_port *up)
{
	struct circ_buf *xmit = &up->port.state->xmit;
	int count;

	if (up->port.x_char) {
		serial_outp(up, UART_TX, up->port.x_char);
		up->port.icount.tx++;
		up->port.x_char = 0;
		return;
	}
	if (uart_tx_stopped(&up->port)) {
		serial8250_stop_tx(&up->port);
		return;
	}
	if (uart_circ_empty(xmit)) {
		__stop_tx(up);
		return;
	}

	count = up->tx_loadsz;
	do {
		serial_out(up, UART_TX, xmit->buf[xmit->tail]);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		up->port.icount.tx++;
		if (uart_circ_empty(xmit))
			break;
		if (up->capabilities & UART_CAP_HFIFO) {
			if ((serial_in(up, UART_LSR) & BOTH_EMPTY) !=
			    BOTH_EMPTY)
				break;
		}
	} while (--count > 0);

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(&up->port);

	DEBUG_INTR("THRE...");

	if (uart_circ_empty(xmit))
		__stop_tx(up);
}

static unsigned int check_modem_status(struct uart_8250_port *up)
{
	unsigned int status = serial_in(up, UART_MSR);

	status |= up->msr_saved_flags;
	up->msr_saved_flags = 0;
	if (status & UART_MSR_ANY_DELTA && up->ier & UART_IER_MSI &&
	    up->port.state != NULL) {
		if (status & UART_MSR_TERI)
			up->port.icount.rng++;
		if (status & UART_MSR_DDSR)
			up->port.icount.dsr++;
		if (status & UART_MSR_DDCD)
			uart_handle_dcd_change(&up->port, status & UART_MSR_DCD);
		if (status & UART_MSR_DCTS)
			uart_handle_cts_change(&up->port, status & UART_MSR_CTS);

		wake_up_interruptible(&up->port.state->port.delta_msr_wait);
	}

	return status;
}

static void serial8250_handle_port(struct uart_8250_port *up)
{
	unsigned int status;
	unsigned long flags;

	spin_lock_irqsave(&up->port.lock, flags);

	status = serial_inp(up, UART_LSR);

	DEBUG_INTR("status = %x...", status);

	if (status & (UART_LSR_DR | UART_LSR_BI))
		receive_chars(up, &status);
	check_modem_status(up);
	if (status & UART_LSR_THRE)
		transmit_chars(up);

	spin_unlock_irqrestore(&up->port.lock, flags);
}

int serial8250_handle_irq(struct uart_port *port, unsigned int iir)
{
	struct uart_8250_port *up =
		container_of(port, struct uart_8250_port, port);

	if (!(iir & UART_IIR_NO_INT)) {
		serial8250_handle_port(up);
		return 1;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(serial8250_handle_irq);

static int serial8250_default_handle_irq(struct uart_port *port)
{
	struct uart_8250_port *up =
		container_of(port, struct uart_8250_port, port);
	unsigned int iir = serial_in(up, UART_IIR);

	return serial8250_handle_irq(port, iir);
}

#ifdef CONFIG_GEN3_UART
 
static irqreturn_t serial8250_interrupt(int irq, void *dev_id)
{
	struct irq_info *i = dev_id;
	struct list_head *l, *end = NULL;
	int pass_counter = 0, handled = 0;
	unsigned int my_flags, ier, lsr;

	DEBUG_INTR("serial8250_interrupt(%d)...", irq);

	spin_lock(&i->lock);

	l = i->head;
	do {
		struct uart_8250_port *up;
		unsigned int iir;
		my_flags = 0x00;

		up = list_entry(l, struct uart_8250_port, list);

		iir = serial_in(up, UART_IIR);
		if (!(iir & UART_IIR_NO_INT)) {
			serial8250_handle_port(up);
			my_flags |= 0x01;

			handled = 1;

			end = NULL;
		} else if ((iir & UART_IIR_BUSY) == UART_IIR_BUSY) {
			 
			unsigned int status;
			status = *(volatile u32 *)up->port.private_data;
			serial_out(up, UART_LCR, up->lcr);

			handled = 1;

			end = NULL;
		}
		ier = serial_in(up, UART_IER);
		 
		if(ier & UART_IER_THRI) {
			lsr = serial_in(up, UART_LSR);
			 
			if(lsr & (UART_LSR_THRE | UART_LSR_TEMT)) {
				 
				spin_lock(&up->port.lock);
				transmit_chars(up);	 
				spin_unlock(&up->port.lock);

				my_flags |= 0x02;   

				handled = 1;

				end = NULL;
			}
		}
		 
		if(0x00 == my_flags) {
			if (end == NULL)
				end = l;
		}

		l = l->next;

		if (l == i->head && pass_counter++ > PASS_LIMIT) {
			 
#ifdef CONFIG_GEN3_UART
			printk_ratelimited(KERN_ERR "serial8250: too much work for "
#else
			printk(KERN_ERR "serial8250: too much work for "
#endif
				"irq%d\n", irq);
			break;
		}
	} while (l != end);

	spin_unlock(&i->lock);

	DEBUG_INTR("end.\n");

	return IRQ_RETVAL(handled);
}

#else
 
static irqreturn_t serial8250_interrupt(int irq, void *dev_id)
{
	struct irq_info *i = dev_id;
	struct list_head *l, *end = NULL;
	int pass_counter = 0, handled = 0;

	DEBUG_INTR("serial8250_interrupt(%d)...", irq);

	spin_lock(&i->lock);

	l = i->head;
	do {
		struct uart_8250_port *up;
		struct uart_port *port;
#if defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2) || defined(CONFIG_SYNO_C2K_SERIAL_FIX)
		unsigned int iir;
#endif

		up = list_entry(l, struct uart_8250_port, list);
		port = &up->port;

#if defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2) || defined(CONFIG_SYNO_C2K_SERIAL_FIX)
		iir = serial_in(up, UART_IIR);
		if (!(iir & UART_IIR_NO_INT)) {
			serial8250_handle_port(up);
			handled = 1;
			end = NULL;
                }
		else if ((up->port.iotype == UPIO_DWAPB ||
                            up->port.iotype == UPIO_DWAPB32) &&
                          (iir & UART_IIR_BUSY) == UART_IIR_BUSY) {
                         
                        unsigned int status;
                        status = *(volatile u32 *)up->port.private_data;
#ifdef CONFIG_SYNO_ARMADA_ARCH_V2
			 
#else			
                        serial_out(up, UART_LCR, up->lcr);
#endif
                        handled = 1;
                        end = NULL;

                }
		else if (end == NULL)
                        end = l;
#else
 		if (port->handle_irq(port)) {
 			handled = 1;
 			end = NULL;
		} else if (end == NULL)
			end = l;
#endif

		l = l->next;

		if (l == i->head && pass_counter++ > PASS_LIMIT) {
			 
			printk_ratelimited(KERN_ERR
				"serial8250: too much work for irq%d\n", irq);
			break;
		}
	} while (l != end);

	spin_unlock(&i->lock);

	DEBUG_INTR("end.\n");

	return IRQ_RETVAL(handled);
}

#endif  
 
static void serial_do_unlink(struct irq_info *i, struct uart_8250_port *up)
{
	spin_lock_irq(&i->lock);

	if (!list_empty(i->head)) {
		if (i->head == &up->list)
			i->head = i->head->next;
		list_del(&up->list);
	} else {
		BUG_ON(i->head != &up->list);
		i->head = NULL;
	}
	spin_unlock_irq(&i->lock);
	 
	if (i->head == NULL) {
		hlist_del(&i->node);
		kfree(i);
	}
}

static int serial_link_irq_chain(struct uart_8250_port *up)
{
	struct hlist_head *h;
	struct hlist_node *n;
	struct irq_info *i;
	int ret, irq_flags = up->port.flags & UPF_SHARE_IRQ ? IRQF_SHARED : 0;

	mutex_lock(&hash_mutex);

	h = &irq_lists[up->port.irq % NR_IRQ_HASH];

	hlist_for_each(n, h) {
		i = hlist_entry(n, struct irq_info, node);
		if (i->irq == up->port.irq)
			break;
	}

	if (n == NULL) {
		i = kzalloc(sizeof(struct irq_info), GFP_KERNEL);
		if (i == NULL) {
			mutex_unlock(&hash_mutex);
			return -ENOMEM;
		}
		spin_lock_init(&i->lock);
		i->irq = up->port.irq;
		hlist_add_head(&i->node, h);
	}
	mutex_unlock(&hash_mutex);

	spin_lock_irq(&i->lock);

	if (i->head) {
		list_add(&up->list, i->head);
		spin_unlock_irq(&i->lock);

		ret = 0;
	} else {
		INIT_LIST_HEAD(&up->list);
		i->head = &up->list;
		spin_unlock_irq(&i->lock);
		irq_flags |= up->port.irqflags;
		ret = request_irq(up->port.irq, serial8250_interrupt,
				  irq_flags, "serial", i);
		if (ret < 0)
			serial_do_unlink(i, up);
	}

	return ret;
}

static void serial_unlink_irq_chain(struct uart_8250_port *up)
{
	struct irq_info *i;
	struct hlist_node *n;
	struct hlist_head *h;

	mutex_lock(&hash_mutex);

	h = &irq_lists[up->port.irq % NR_IRQ_HASH];

	hlist_for_each(n, h) {
		i = hlist_entry(n, struct irq_info, node);
		if (i->irq == up->port.irq)
			break;
	}

	BUG_ON(n == NULL);
	BUG_ON(i->head == NULL);

	if (list_empty(i->head))
		free_irq(up->port.irq, i);

	serial_do_unlink(i, up);
	mutex_unlock(&hash_mutex);
}

static void serial8250_timeout(unsigned long data)
{
	struct uart_8250_port *up = (struct uart_8250_port *)data;
	unsigned int iir;

	iir = serial_in(up, UART_IIR);
	if (!(iir & UART_IIR_NO_INT))
		serial8250_handle_port(up);
	mod_timer(&up->timer, jiffies + uart_poll_timeout(&up->port));
}

static void serial8250_backup_timeout(unsigned long data)
{
	struct uart_8250_port *up = (struct uart_8250_port *)data;
	unsigned int iir, ier = 0, lsr;
	unsigned long flags;

	spin_lock_irqsave(&up->port.lock, flags);

	if (is_real_interrupt(up->port.irq)) {
		ier = serial_in(up, UART_IER);
		serial_out(up, UART_IER, 0);
	}

	iir = serial_in(up, UART_IIR);

	lsr = serial_in(up, UART_LSR);
	up->lsr_saved_flags |= lsr & LSR_SAVE_FLAGS;
	if ((iir & UART_IIR_NO_INT) && (up->ier & UART_IER_THRI) &&
	    (!uart_circ_empty(&up->port.state->xmit) || up->port.x_char) &&
	    (lsr & UART_LSR_THRE)) {
		iir &= ~(UART_IIR_ID | UART_IIR_NO_INT);
		iir |= UART_IIR_THRI;
	}

	if (!(iir & UART_IIR_NO_INT))
		transmit_chars(up);

	if (is_real_interrupt(up->port.irq))
		serial_out(up, UART_IER, ier);

	spin_unlock_irqrestore(&up->port.lock, flags);

	mod_timer(&up->timer,
		jiffies + uart_poll_timeout(&up->port) + HZ / 5);
}

static unsigned int serial8250_tx_empty(struct uart_port *port)
{
	struct uart_8250_port *up =
		container_of(port, struct uart_8250_port, port);
	unsigned long flags;
	unsigned int lsr;

	spin_lock_irqsave(&up->port.lock, flags);
	lsr = serial_in(up, UART_LSR);
	up->lsr_saved_flags |= lsr & LSR_SAVE_FLAGS;
	spin_unlock_irqrestore(&up->port.lock, flags);

	return (lsr & BOTH_EMPTY) == BOTH_EMPTY ? TIOCSER_TEMT : 0;
}

static unsigned int serial8250_get_mctrl(struct uart_port *port)
{
	struct uart_8250_port *up =
		container_of(port, struct uart_8250_port, port);
	unsigned int status;
	unsigned int ret;

	status = check_modem_status(up);

	ret = 0;
	if (status & UART_MSR_DCD)
		ret |= TIOCM_CAR;
	if (status & UART_MSR_RI)
		ret |= TIOCM_RNG;
	if (status & UART_MSR_DSR)
		ret |= TIOCM_DSR;
	if (status & UART_MSR_CTS)
		ret |= TIOCM_CTS;
	return ret;
}

static void serial8250_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	struct uart_8250_port *up =
		container_of(port, struct uart_8250_port, port);
	unsigned char mcr = 0;

	if (mctrl & TIOCM_RTS)
		mcr |= UART_MCR_RTS;
	if (mctrl & TIOCM_DTR)
		mcr |= UART_MCR_DTR;
	if (mctrl & TIOCM_OUT1)
		mcr |= UART_MCR_OUT1;
	if (mctrl & TIOCM_OUT2)
		mcr |= UART_MCR_OUT2;
	if (mctrl & TIOCM_LOOP)
		mcr |= UART_MCR_LOOP;

	mcr = (mcr & up->mcr_mask) | up->mcr_force | up->mcr;

	serial_out(up, UART_MCR, mcr);
}

static void serial8250_break_ctl(struct uart_port *port, int break_state)
{
	struct uart_8250_port *up =
		container_of(port, struct uart_8250_port, port);
	unsigned long flags;

	spin_lock_irqsave(&up->port.lock, flags);
	if (break_state == -1)
		up->lcr |= UART_LCR_SBC;
	else
		up->lcr &= ~UART_LCR_SBC;
	serial_out(up, UART_LCR, up->lcr);
	spin_unlock_irqrestore(&up->port.lock, flags);
}

static void wait_for_xmitr(struct uart_8250_port *up, int bits)
{
	unsigned int status, tmout = 10000;

	for (;;) {
		status = serial_in(up, UART_LSR);

		up->lsr_saved_flags |= status & LSR_SAVE_FLAGS;

		if ((status & bits) == bits)
			break;
		if (--tmout == 0)
			break;
		udelay(1);
	}

	if (up->port.flags & UPF_CONS_FLOW) {
		unsigned int tmout;
		for (tmout = 1000000; tmout; tmout--) {
			unsigned int msr = serial_in(up, UART_MSR);
			up->msr_saved_flags |= msr & MSR_SAVE_FLAGS;
			if (msr & UART_MSR_CTS)
				break;
			udelay(1);
			touch_nmi_watchdog();
		}
	}
}

#ifdef CONFIG_CONSOLE_POLL
 
static int serial8250_get_poll_char(struct uart_port *port)
{
	struct uart_8250_port *up =
		container_of(port, struct uart_8250_port, port);
	unsigned char lsr = serial_inp(up, UART_LSR);

	if (!(lsr & UART_LSR_DR))
		return NO_POLL_CHAR;

	return serial_inp(up, UART_RX);
}

static void serial8250_put_poll_char(struct uart_port *port,
			 unsigned char c)
{
	unsigned int ier;
	struct uart_8250_port *up =
		container_of(port, struct uart_8250_port, port);

	ier = serial_in(up, UART_IER);
	if (up->capabilities & UART_CAP_UUE)
		serial_out(up, UART_IER, UART_IER_UUE);
	else
		serial_out(up, UART_IER, 0);

	wait_for_xmitr(up, BOTH_EMPTY);
	 
	serial_out(up, UART_TX, c);
	if (c == 10) {
		wait_for_xmitr(up, BOTH_EMPTY);
		serial_out(up, UART_TX, 13);
	}

	wait_for_xmitr(up, BOTH_EMPTY);
	serial_out(up, UART_IER, ier);
}

#endif  

static int serial8250_startup(struct uart_port *port)
{
	struct uart_8250_port *up =
		container_of(port, struct uart_8250_port, port);
	unsigned long flags;
	unsigned char lsr, iir;
	int retval;

	up->port.fifosize = uart_config[up->port.type].fifo_size;
	up->tx_loadsz = uart_config[up->port.type].tx_loadsz;
	up->capabilities = uart_config[up->port.type].flags;
	up->mcr = 0;

	if (up->port.iotype != up->cur_iotype)
		set_io_from_upio(port);

	if (up->port.type == PORT_16C950) {
		 
		up->acr = 0;
		serial_outp(up, UART_LCR, UART_LCR_CONF_MODE_B);
		serial_outp(up, UART_EFR, UART_EFR_ECB);
		serial_outp(up, UART_IER, 0);
		serial_outp(up, UART_LCR, 0);
		serial_icr_write(up, UART_CSR, 0);  
		serial_outp(up, UART_LCR, 0xBF);
		serial_outp(up, UART_EFR, UART_EFR_ECB);
		serial_outp(up, UART_LCR, 0);
	}

#ifdef CONFIG_SERIAL_8250_RSA
	 
	enable_rsa(up);
#endif

	serial8250_clear_fifos(up);

	(void) serial_inp(up, UART_LSR);
	(void) serial_inp(up, UART_RX);
	(void) serial_inp(up, UART_IIR);
	(void) serial_inp(up, UART_MSR);

	if (!(up->port.flags & UPF_BUGGY_UART) &&
	    (serial_inp(up, UART_LSR) == 0xff)) {
		printk_ratelimited(KERN_INFO "ttyS%d: LSR safety check engaged!\n",
				   serial_index(&up->port));
		return -ENODEV;
	}

	if (up->port.type == PORT_16850) {
		unsigned char fctr;

		serial_outp(up, UART_LCR, UART_LCR_CONF_MODE_B);

		fctr = serial_inp(up, UART_FCTR) & ~(UART_FCTR_RX|UART_FCTR_TX);
		serial_outp(up, UART_FCTR, fctr | UART_FCTR_TRGD | UART_FCTR_RX);
		serial_outp(up, UART_TRG, UART_TRG_96);
		serial_outp(up, UART_FCTR, fctr | UART_FCTR_TRGD | UART_FCTR_TX);
		serial_outp(up, UART_TRG, UART_TRG_96);

		serial_outp(up, UART_LCR, 0);
	}

	if (is_real_interrupt(up->port.irq)) {
		unsigned char iir1;
		 
		spin_lock_irqsave(&up->port.lock, flags);
		if (up->port.irqflags & IRQF_SHARED)
			disable_irq_nosync(up->port.irq);

		wait_for_xmitr(up, UART_LSR_THRE);
		serial_out_sync(up, UART_IER, UART_IER_THRI);
		udelay(1);  
		iir1 = serial_in(up, UART_IIR);
		serial_out(up, UART_IER, 0);
		serial_out_sync(up, UART_IER, UART_IER_THRI);
		udelay(1);  
		iir = serial_in(up, UART_IIR);
		serial_out(up, UART_IER, 0);

		if (up->port.irqflags & IRQF_SHARED)
			enable_irq(up->port.irq);
		spin_unlock_irqrestore(&up->port.lock, flags);

		if (!(iir1 & UART_IIR_NO_INT) && (iir & UART_IIR_NO_INT)) {
			up->bugs |= UART_BUG_THRE;
			pr_debug("ttyS%d - using backup timer\n",
				 serial_index(port));
		}
	}

	if (up->bugs & UART_BUG_THRE) {
		up->timer.function = serial8250_backup_timeout;
		up->timer.data = (unsigned long)up;
		mod_timer(&up->timer, jiffies +
			uart_poll_timeout(port) + HZ / 5);
	}

	if (!is_real_interrupt(up->port.irq)) {
		up->timer.data = (unsigned long)up;
		mod_timer(&up->timer, jiffies + uart_poll_timeout(port));
	} else {
		retval = serial_link_irq_chain(up);
		if (retval)
			return retval;
	}

	serial_outp(up, UART_LCR, UART_LCR_WLEN8);

	spin_lock_irqsave(&up->port.lock, flags);
	if (up->port.flags & UPF_FOURPORT) {
		if (!is_real_interrupt(up->port.irq))
			up->port.mctrl |= TIOCM_OUT1;
	} else
		 
		if (is_real_interrupt(up->port.irq))
			up->port.mctrl |= TIOCM_OUT2;

	serial8250_set_mctrl(&up->port, up->port.mctrl);

	if (skip_txen_test || up->port.flags & UPF_NO_TXEN_TEST)
		goto dont_test_tx_en;

	serial_outp(up, UART_IER, UART_IER_THRI);
	lsr = serial_in(up, UART_LSR);
	iir = serial_in(up, UART_IIR);
	serial_outp(up, UART_IER, 0);

	if (lsr & UART_LSR_TEMT && iir & UART_IIR_NO_INT) {
		if (!(up->bugs & UART_BUG_TXEN)) {
			up->bugs |= UART_BUG_TXEN;
			pr_debug("ttyS%d - enabling bad tx status workarounds\n",
				 serial_index(port));
		}
	} else {
		up->bugs &= ~UART_BUG_TXEN;
	}

dont_test_tx_en:
	spin_unlock_irqrestore(&up->port.lock, flags);

	serial_inp(up, UART_LSR);
	serial_inp(up, UART_RX);
	serial_inp(up, UART_IIR);
	serial_inp(up, UART_MSR);
	up->lsr_saved_flags = 0;
	up->msr_saved_flags = 0;

	up->ier = UART_IER_RLSI | UART_IER_RDI;
	serial_outp(up, UART_IER, up->ier);

	if (up->port.flags & UPF_FOURPORT) {
		unsigned int icp;
		 
		icp = (up->port.iobase & 0xfe0) | 0x01f;
		outb_p(0x80, icp);
		(void) inb_p(icp);
	}

	return 0;
}

static void serial8250_shutdown(struct uart_port *port)
{
	struct uart_8250_port *up =
		container_of(port, struct uart_8250_port, port);
	unsigned long flags;

	up->ier = 0;
	serial_outp(up, UART_IER, 0);

	spin_lock_irqsave(&up->port.lock, flags);
	if (up->port.flags & UPF_FOURPORT) {
		 
		inb((up->port.iobase & 0xfe0) | 0x1f);
		up->port.mctrl |= TIOCM_OUT1;
	} else
		up->port.mctrl &= ~TIOCM_OUT2;

	serial8250_set_mctrl(&up->port, up->port.mctrl);
	spin_unlock_irqrestore(&up->port.lock, flags);

	serial_out(up, UART_LCR, serial_inp(up, UART_LCR) & ~UART_LCR_SBC);
	serial8250_clear_fifos(up);

#ifdef CONFIG_SERIAL_8250_RSA
	 
	disable_rsa(up);
#endif

	(void) serial_in(up, UART_RX);

	del_timer_sync(&up->timer);
	up->timer.function = serial8250_timeout;
	if (is_real_interrupt(up->port.irq))
		serial_unlink_irq_chain(up);
}

static unsigned int serial8250_get_divisor(struct uart_port *port, unsigned int baud)
{
	unsigned int quot;

	if ((port->flags & UPF_MAGIC_MULTIPLIER) &&
	    baud == (port->uartclk/4))
		quot = 0x8001;
	else if ((port->flags & UPF_MAGIC_MULTIPLIER) &&
		 baud == (port->uartclk/8))
		quot = 0x8002;
	else
		quot = uart_get_divisor(port, baud);

	return quot;
}

void
serial8250_do_set_termios(struct uart_port *port, struct ktermios *termios,
		          struct ktermios *old)
{
	struct uart_8250_port *up =
		container_of(port, struct uart_8250_port, port);
	unsigned char cval, fcr = 0;
	unsigned long flags;
	unsigned int baud, quot;

	switch (termios->c_cflag & CSIZE) {
	case CS5:
		cval = UART_LCR_WLEN5;
		break;
	case CS6:
		cval = UART_LCR_WLEN6;
		break;
	case CS7:
		cval = UART_LCR_WLEN7;
		break;
	default:
	case CS8:
		cval = UART_LCR_WLEN8;
		break;
	}

	if (termios->c_cflag & CSTOPB)
		cval |= UART_LCR_STOP;
	if (termios->c_cflag & PARENB)
		cval |= UART_LCR_PARITY;
	if (!(termios->c_cflag & PARODD))
		cval |= UART_LCR_EPAR;
#ifdef CMSPAR
	if (termios->c_cflag & CMSPAR)
		cval |= UART_LCR_SPAR;
#endif

	baud = uart_get_baud_rate(port, termios, old,
				  port->uartclk / 16 / 0xffff,
				  port->uartclk / 16);
	quot = serial8250_get_divisor(port, baud);

	if (up->bugs & UART_BUG_QUOT && (quot & 0xff) == 0)
		quot++;

	if (up->capabilities & UART_CAP_FIFO && up->port.fifosize > 1) {
		fcr = uart_config[up->port.type].fcr;
		if (baud < 2400) {
			fcr &= ~UART_FCR_TRIGGER_MASK;
			fcr |= UART_FCR_TRIGGER_1;
		}
	}

	
#if defined(CONFIG_SYNO_COMCERTO) && defined(CONFIG_ARCH_M86XXX)
	up->mcr &= ~UART_MCR_AFE;
        if (termios->c_cflag & CRTSCTS)
               up->mcr |= UART_MCR_AFE;
#else
	if (up->capabilities & UART_CAP_AFE && up->port.fifosize >= 32) {
		up->mcr &= ~UART_MCR_AFE;
		if (termios->c_cflag & CRTSCTS)
			up->mcr |= UART_MCR_AFE;
	}
#endif

	spin_lock_irqsave(&up->port.lock, flags);

	uart_update_timeout(port, termios->c_cflag, baud);

	up->port.read_status_mask = UART_LSR_OE | UART_LSR_THRE | UART_LSR_DR;
	if (termios->c_iflag & INPCK)
		up->port.read_status_mask |= UART_LSR_FE | UART_LSR_PE;
	if (termios->c_iflag & (BRKINT | PARMRK))
		up->port.read_status_mask |= UART_LSR_BI;

	up->port.ignore_status_mask = 0;
	if (termios->c_iflag & IGNPAR)
		up->port.ignore_status_mask |= UART_LSR_PE | UART_LSR_FE;
	if (termios->c_iflag & IGNBRK) {
		up->port.ignore_status_mask |= UART_LSR_BI;
		 
		if (termios->c_iflag & IGNPAR)
			up->port.ignore_status_mask |= UART_LSR_OE;
	}

	if ((termios->c_cflag & CREAD) == 0)
		up->port.ignore_status_mask |= UART_LSR_DR;

	up->ier &= ~UART_IER_MSI;
	if (!(up->bugs & UART_BUG_NOMSR) &&
			UART_ENABLE_MS(&up->port, termios->c_cflag))
		up->ier |= UART_IER_MSI;
	if (up->capabilities & UART_CAP_UUE)
		up->ier |= UART_IER_UUE;
	if (up->capabilities & UART_CAP_RTOIE)
		up->ier |= UART_IER_RTOIE;

	serial_out(up, UART_IER, up->ier);

	if (up->capabilities & UART_CAP_EFR) {
		unsigned char efr = 0;
		 
		if (termios->c_cflag & CRTSCTS)
			efr |= UART_EFR_CTS;

		serial_outp(up, UART_LCR, UART_LCR_CONF_MODE_B);
		if (up->port.flags & UPF_EXAR_EFR)
			serial_outp(up, UART_XR_EFR, efr);
		else
			serial_outp(up, UART_EFR, efr);
	}

#ifdef CONFIG_ARCH_OMAP
	 
	if (cpu_is_omap1510() && is_omap_port(up)) {
		if (baud == 115200) {
			quot = 1;
			serial_out(up, UART_OMAP_OSC_12M_SEL, 1);
		} else
			serial_out(up, UART_OMAP_OSC_12M_SEL, 0);
	}
#endif

	if (up->capabilities & UART_NATSEMI) {
		 
		serial_outp(up, UART_LCR, 0xe0);
	} else {
		serial_outp(up, UART_LCR, cval | UART_LCR_DLAB); 
	}

	serial_dl_write(up, quot);

	if (up->port.type == PORT_16750)
		serial_outp(up, UART_FCR, fcr);

	serial_outp(up, UART_LCR, cval);		 
	up->lcr = cval;					 
	if (up->port.type != PORT_16750) {
		if (fcr & UART_FCR_ENABLE_FIFO) {
			 
			serial_outp(up, UART_FCR, UART_FCR_ENABLE_FIFO);
		}
		serial_outp(up, UART_FCR, fcr);		 
	}
	serial8250_set_mctrl(&up->port, up->port.mctrl);
	spin_unlock_irqrestore(&up->port.lock, flags);
	 
	if (tty_termios_baud_rate(termios))
		tty_termios_encode_baud_rate(termios, baud, baud);
}
EXPORT_SYMBOL(serial8250_do_set_termios);

static void
serial8250_set_termios(struct uart_port *port, struct ktermios *termios,
		       struct ktermios *old)
{
	if (port->set_termios)
		port->set_termios(port, termios, old);
	else
		serial8250_do_set_termios(port, termios, old);
}

static void
serial8250_set_ldisc(struct uart_port *port, int new)
{
	if (new == N_PPS) {
		port->flags |= UPF_HARDPPS_CD;
		serial8250_enable_ms(port);
	} else
		port->flags &= ~UPF_HARDPPS_CD;
}

void serial8250_do_pm(struct uart_port *port, unsigned int state,
		      unsigned int oldstate)
{
	struct uart_8250_port *p =
		container_of(port, struct uart_8250_port, port);

	serial8250_set_sleep(p, state != 0);
}
EXPORT_SYMBOL(serial8250_do_pm);

static void
serial8250_pm(struct uart_port *port, unsigned int state,
	      unsigned int oldstate)
{
	if (port->pm)
		port->pm(port, state, oldstate);
	else
		serial8250_do_pm(port, state, oldstate);
}

static unsigned int serial8250_port_size(struct uart_8250_port *pt)
{
	if (pt->port.iotype == UPIO_AU)
		return 0x1000;
#ifdef CONFIG_ARCH_OMAP
	if (is_omap_port(pt))
		return 0x16 << pt->port.regshift;
#endif
	return 8 << pt->port.regshift;
}

static int serial8250_request_std_resource(struct uart_8250_port *up)
{
	unsigned int size = serial8250_port_size(up);
	int ret = 0;

	switch (up->port.iotype) {
	case UPIO_AU:
	case UPIO_TSI:
	case UPIO_MEM32:
	case UPIO_MEM:
		if (!up->port.mapbase)
			break;

		if (!request_mem_region(up->port.mapbase, size, "serial")) {
			ret = -EBUSY;
			break;
		}

		if (up->port.flags & UPF_IOREMAP) {
			up->port.membase = ioremap_nocache(up->port.mapbase,
									size);
			if (!up->port.membase) {
				release_mem_region(up->port.mapbase, size);
				ret = -ENOMEM;
			}
		}
		break;

	case UPIO_HUB6:
	case UPIO_PORT:
		if (!request_region(up->port.iobase, size, "serial"))
			ret = -EBUSY;
		break;
	}
	return ret;
}

static void serial8250_release_std_resource(struct uart_8250_port *up)
{
	unsigned int size = serial8250_port_size(up);

	switch (up->port.iotype) {
	case UPIO_AU:
	case UPIO_TSI:
	case UPIO_MEM32:
	case UPIO_MEM:
		if (!up->port.mapbase)
			break;

		if (up->port.flags & UPF_IOREMAP) {
			iounmap(up->port.membase);
			up->port.membase = NULL;
		}

		release_mem_region(up->port.mapbase, size);
		break;

	case UPIO_HUB6:
	case UPIO_PORT:
		release_region(up->port.iobase, size);
		break;
	}
}

static int serial8250_request_rsa_resource(struct uart_8250_port *up)
{
	unsigned long start = UART_RSA_BASE << up->port.regshift;
	unsigned int size = 8 << up->port.regshift;
	int ret = -EINVAL;

	switch (up->port.iotype) {
	case UPIO_HUB6:
	case UPIO_PORT:
		start += up->port.iobase;
		if (request_region(start, size, "serial-rsa"))
			ret = 0;
		else
			ret = -EBUSY;
		break;
	}

	return ret;
}

static void serial8250_release_rsa_resource(struct uart_8250_port *up)
{
	unsigned long offset = UART_RSA_BASE << up->port.regshift;
	unsigned int size = 8 << up->port.regshift;

	switch (up->port.iotype) {
	case UPIO_HUB6:
	case UPIO_PORT:
		release_region(up->port.iobase + offset, size);
		break;
	}
}

static void serial8250_release_port(struct uart_port *port)
{
	struct uart_8250_port *up =
		container_of(port, struct uart_8250_port, port);

	serial8250_release_std_resource(up);
	if (up->port.type == PORT_RSA)
		serial8250_release_rsa_resource(up);
}

static int serial8250_request_port(struct uart_port *port)
{
	struct uart_8250_port *up =
		container_of(port, struct uart_8250_port, port);
	int ret = 0;

	ret = serial8250_request_std_resource(up);
	if (ret == 0 && up->port.type == PORT_RSA) {
		ret = serial8250_request_rsa_resource(up);
		if (ret < 0)
			serial8250_release_std_resource(up);
	}

	return ret;
}

static void serial8250_config_port(struct uart_port *port, int flags)
{
	struct uart_8250_port *up =
		container_of(port, struct uart_8250_port, port);
	int probeflags = PROBE_ANY;
	int ret;

	ret = serial8250_request_std_resource(up);
	if (ret < 0)
		return;

	ret = serial8250_request_rsa_resource(up);
	if (ret < 0)
		probeflags &= ~PROBE_RSA;

	if (up->port.iotype != up->cur_iotype)
		set_io_from_upio(port);

	if (flags & UART_CONFIG_TYPE)
		autoconfig(up, probeflags);

	if (up->port.type == PORT_16550A && up->port.iotype == UPIO_AU)
		up->bugs |= UART_BUG_NOMSR;

	if (up->port.type != PORT_UNKNOWN && flags & UART_CONFIG_IRQ)
		autoconfig_irq(up);

	if (up->port.type != PORT_RSA && probeflags & PROBE_RSA)
		serial8250_release_rsa_resource(up);
	if (up->port.type == PORT_UNKNOWN)
		serial8250_release_std_resource(up);
}

static int
serial8250_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	if (ser->irq >= nr_irqs || ser->irq < 0 ||
	    ser->baud_base < 9600 || ser->type < PORT_UNKNOWN ||
	    ser->type >= ARRAY_SIZE(uart_config) || ser->type == PORT_CIRRUS ||
	    ser->type == PORT_STARTECH || uart_config[ser->type].name == NULL)
		return -EINVAL;
	return 0;
}

static const char *
serial8250_type(struct uart_port *port)
{
	int type = port->type;

	if (type >= ARRAY_SIZE(uart_config) || uart_config[type].name == NULL)
		type = 0;
	return uart_config[type].name;
}

static struct uart_ops serial8250_pops = {
	.tx_empty	= serial8250_tx_empty,
	.set_mctrl	= serial8250_set_mctrl,
	.get_mctrl	= serial8250_get_mctrl,
	.stop_tx	= serial8250_stop_tx,
	.start_tx	= serial8250_start_tx,
	.stop_rx	= serial8250_stop_rx,
	.enable_ms	= serial8250_enable_ms,
	.break_ctl	= serial8250_break_ctl,
	.startup	= serial8250_startup,
	.shutdown	= serial8250_shutdown,
	.set_termios	= serial8250_set_termios,
	.set_ldisc	= serial8250_set_ldisc,
	.pm		= serial8250_pm,
	.type		= serial8250_type,
	.release_port	= serial8250_release_port,
	.request_port	= serial8250_request_port,
	.config_port	= serial8250_config_port,
	.verify_port	= serial8250_verify_port,
#ifdef CONFIG_CONSOLE_POLL
	.poll_get_char = serial8250_get_poll_char,
	.poll_put_char = serial8250_put_poll_char,
#endif
};

static struct uart_8250_port serial8250_ports[UART_NR];

static void (*serial8250_isa_config)(int port, struct uart_port *up,
	unsigned short *capabilities);

void serial8250_set_isa_configurator(
	void (*v)(int port, struct uart_port *up, unsigned short *capabilities))
{
	serial8250_isa_config = v;
}
EXPORT_SYMBOL(serial8250_set_isa_configurator);

static void __init serial8250_isa_init_ports(void)
{
	struct uart_8250_port *up;
	static int first = 1;
	int i, irqflag = 0;

	if (!first)
		return;
	first = 0;

	for (i = 0; i < nr_uarts; i++) {
		struct uart_8250_port *up = &serial8250_ports[i];

		up->port.line = i;
		spin_lock_init(&up->port.lock);

		init_timer(&up->timer);
		up->timer.function = serial8250_timeout;

		up->mcr_mask = ~ALPHA_KLUDGE_MCR;
		up->mcr_force = ALPHA_KLUDGE_MCR;

		up->port.ops = &serial8250_pops;
	}

	if (share_irqs)
		irqflag = IRQF_SHARED;

	for (i = 0, up = serial8250_ports;
	     i < ARRAY_SIZE(old_serial_port) && i < nr_uarts;
	     i++, up++) {
		up->port.iobase   = old_serial_port[i].port;
		up->port.irq      = irq_canonicalize(old_serial_port[i].irq);
		up->port.irqflags = old_serial_port[i].irqflags;
		up->port.uartclk  = old_serial_port[i].baud_base * 16;
		up->port.flags    = old_serial_port[i].flags;
		up->port.hub6     = old_serial_port[i].hub6;
		up->port.membase  = old_serial_port[i].iomem_base;
		up->port.iotype   = old_serial_port[i].io_type;
		up->port.regshift = old_serial_port[i].iomem_reg_shift;
		set_io_from_upio(&up->port);
		up->port.irqflags |= irqflag;
		if (serial8250_isa_config != NULL)
			serial8250_isa_config(i, &up->port, &up->capabilities);

	}
}

static void
serial8250_init_fixed_type_port(struct uart_8250_port *up, unsigned int type)
{
	up->port.type = type;
	up->port.fifosize = uart_config[type].fifo_size;
	up->capabilities = uart_config[type].flags;
	up->tx_loadsz = uart_config[type].tx_loadsz;
}

static void __init
serial8250_register_ports(struct uart_driver *drv, struct device *dev)
{
	int i;

	for (i = 0; i < nr_uarts; i++) {
		struct uart_8250_port *up = &serial8250_ports[i];
		up->cur_iotype = 0xFF;
	}

	serial8250_isa_init_ports();

	for (i = 0; i < nr_uarts; i++) {
		struct uart_8250_port *up = &serial8250_ports[i];

		up->port.dev = dev;

		if (up->port.flags & UPF_FIXED_TYPE)
			serial8250_init_fixed_type_port(up, up->port.type);

		uart_add_one_port(drv, &up->port);
	}
}

#ifdef CONFIG_SERIAL_8250_CONSOLE

static void serial8250_console_putchar(struct uart_port *port, int ch)
{
	struct uart_8250_port *up =
		container_of(port, struct uart_8250_port, port);

	wait_for_xmitr(up, UART_LSR_THRE);
	serial_out(up, UART_TX, ch);
}

static void
serial8250_console_write(struct console *co, const char *s, unsigned int count)
{
	struct uart_8250_port *up = &serial8250_ports[co->index];
	unsigned long flags;
	unsigned int ier;
	int locked = 1;

	touch_nmi_watchdog();

	local_irq_save(flags);
	if (up->port.sysrq) {
		 
		locked = 0;
	} else if (oops_in_progress) {
		locked = spin_trylock(&up->port.lock);
	} else
		spin_lock(&up->port.lock);

	ier = serial_in(up, UART_IER);

#ifdef CONFIG_GEN3_UART
	 
		serial_out(up, UART_IER, UART_IER_UUE);
#else
	if (up->capabilities & UART_CAP_UUE)
		serial_out(up, UART_IER, UART_IER_UUE);
	else
		serial_out(up, UART_IER, 0);
#endif

	uart_console_write(&up->port, s, count, serial8250_console_putchar);

	wait_for_xmitr(up, BOTH_EMPTY);
	serial_out(up, UART_IER, ier);

	if (up->msr_saved_flags)
		check_modem_status(up);

	if (locked)
		spin_unlock(&up->port.lock);
	local_irq_restore(flags);
}

static int __init serial8250_console_setup(struct console *co, char *options)
{
	struct uart_port *port;
#ifdef MY_DEF_HERE
	int baud = 115200;
#else
	int baud = 9600;
#endif
	int bits = 8;
	int parity = 'n';
	int flow = 'n';

	if (co->index >= nr_uarts)
		co->index = 0;
	port = &serial8250_ports[co->index].port;
	if (!port->iobase && !port->membase)
		return -ENODEV;

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);

	return uart_set_options(port, co, baud, parity, bits, flow);
}

static int serial8250_console_early_setup(void)
{
	return serial8250_find_port_for_earlycon();
}

static struct console serial8250_console = {
	.name		= "ttyS",
	.write		= serial8250_console_write,
	.device		= uart_console_device,
	.setup		= serial8250_console_setup,
	.early_setup	= serial8250_console_early_setup,
	.flags		= CON_PRINTBUFFER | CON_ANYTIME,
	.index		= -1,
	.data		= &serial8250_reg,
};

static int __init serial8250_console_init(void)
{
	if (nr_uarts > UART_NR)
		nr_uarts = UART_NR;

	serial8250_isa_init_ports();
	register_console(&serial8250_console);
	return 0;
}
console_initcall(serial8250_console_init);

int serial8250_find_port(struct uart_port *p)
{
	int line;
	struct uart_port *port;

	for (line = 0; line < nr_uarts; line++) {
		port = &serial8250_ports[line].port;
		if (uart_match_port(p, port))
			return line;
	}
	return -ENODEV;
}

#define SERIAL8250_CONSOLE	&serial8250_console
#else
#define SERIAL8250_CONSOLE	NULL
#endif

static struct uart_driver serial8250_reg = {
	.owner			= THIS_MODULE,
	.driver_name		= "serial",
	.dev_name		= "ttyS",
	.major			= TTY_MAJOR,
	.minor			= 64,
	.cons			= SERIAL8250_CONSOLE,
};

int __init early_serial_setup(struct uart_port *port)
{
	struct uart_port *p;

	if (port->line >= ARRAY_SIZE(serial8250_ports))
		return -ENODEV;

	serial8250_isa_init_ports();
	p = &serial8250_ports[port->line].port;
	p->iobase       = port->iobase;
	p->membase      = port->membase;
	p->irq          = port->irq;
	p->irqflags     = port->irqflags;
	p->uartclk      = port->uartclk;
	p->fifosize     = port->fifosize;
	p->regshift     = port->regshift;
	p->iotype       = port->iotype;
	p->flags        = port->flags;
	p->mapbase      = port->mapbase;
	p->private_data = port->private_data;
	p->type		= port->type;
	p->line		= port->line;

	set_io_from_upio(p);
	if (port->serial_in)
		p->serial_in = port->serial_in;
	if (port->serial_out)
		p->serial_out = port->serial_out;
	if (port->handle_irq)
		p->handle_irq = port->handle_irq;
	else
		p->handle_irq = serial8250_default_handle_irq;

	return 0;
}

void serial8250_suspend_port(int line)
{
	uart_suspend_port(&serial8250_reg, &serial8250_ports[line].port);
}

void serial8250_resume_port(int line)
{
	struct uart_8250_port *up = &serial8250_ports[line];

	if (up->capabilities & UART_NATSEMI) {
		 
		serial_outp(up, UART_LCR, 0xE0);

		ns16550a_goto_highspeed(up);

		serial_outp(up, UART_LCR, 0);
		up->port.uartclk = 921600*16;
	}
	uart_resume_port(&serial8250_reg, &up->port);
}


static int __devinit serial8250_probe(struct platform_device *dev)
{
	struct plat_serial8250_port *p = dev->dev.platform_data;
	struct uart_port port;
	int ret, i, irqflag = 0;

#if defined(CONFIG_SYNO_COMCERTO) && defined(CONFIG_ARCH_M86XXX)
	unsigned long uart_rate;

	
	c2000_block_reset(COMPONENT_AXI_FAST_UART,0);

		
	uart_clk = clk_get(NULL,"DUS");
	
	if (IS_ERR(uart_clk)) {
		pr_err("%s: Unable to get UART clock: %ld\n",__func__,PTR_ERR(uart_clk));
		return PTR_ERR(uart_clk);
        }
	
	ret = clk_enable(uart_clk);
	if (ret){
                pr_err("%s: UART clock failed to enable:\n",__func__);
		return  ret;       
	}
	
	uart_rate = clk_get_rate(uart_clk);
#endif

	memset(&port, 0, sizeof(struct uart_port));

	if (share_irqs)
		irqflag = IRQF_SHARED;

	for (i = 0; p && p->flags != 0; p++, i++) {
		port.iobase		= p->iobase;
		port.membase		= p->membase;
		port.irq		= p->irq;
		port.irqflags		= p->irqflags;
#if defined(CONFIG_SYNO_COMCERTO) && defined(CONFIG_ARCH_M86XXX)
		port.uartclk		= uart_rate;     
#else	
		port.uartclk		= p->uartclk;
#endif
		port.regshift		= p->regshift;
		port.iotype		= p->iotype;
		port.flags		= p->flags;
		port.mapbase		= p->mapbase;
		port.hub6		= p->hub6;
		port.private_data	= p->private_data;
		port.type		= p->type;
		port.serial_in		= p->serial_in;
		port.serial_out		= p->serial_out;
		port.handle_irq		= p->handle_irq;
		port.set_termios	= p->set_termios;
		port.pm			= p->pm;
		port.dev		= &dev->dev;
		port.irqflags		|= irqflag;
		ret = serial8250_register_port(&port);
		if (ret < 0) {
			dev_err(&dev->dev, "unable to register port at index %d "
				"(IO%lx MEM%llx IRQ%d): %d\n", i,
				p->iobase, (unsigned long long)p->mapbase,
				p->irq, ret);
		}
	}
	return 0;
}

static int __devexit serial8250_remove(struct platform_device *dev)
{
	int i;

	for (i = 0; i < nr_uarts; i++) {
		struct uart_8250_port *up = &serial8250_ports[i];

		if (up->port.dev == &dev->dev)
			serial8250_unregister_port(i);
	}
#if defined(CONFIG_SYNO_COMCERTO) && defined(CONFIG_ARCH_M86XXX)
	 
	clk_disable(uart_clk);	
	clk_put(uart_clk);

	
	c2000_block_reset(COMPONENT_AXI_FAST_UART,1);
#endif

	return 0;
}

#if !defined(CONFIG_SYNO_COMCERTO) || defined(CONFIG_PM)
static int serial8250_suspend(struct platform_device *dev, pm_message_t state)
{
	int i;

	for (i = 0; i < UART_NR; i++) {
		struct uart_8250_port *up = &serial8250_ports[i];

		if (up->port.type != PORT_UNKNOWN && up->port.dev == &dev->dev)
			uart_suspend_port(&serial8250_reg, &up->port);
	}

#if defined(CONFIG_SYNO_COMCERTO) && defined(CONFIG_ARCH_M86XXX)
	 
	clk_disable(uart_clk);
#endif

	return 0;
}

static int serial8250_resume(struct platform_device *dev)
{
	int i;

#if defined(CONFIG_SYNO_COMCERTO) && defined(CONFIG_ARCH_M86XXX)
	 
	if (clk_enable(uart_clk)){
                pr_err("%s: Unable to enable FAST-UART clock: \n",__func__);
		  
        }
#endif

	for (i = 0; i < UART_NR; i++) {
		struct uart_8250_port *up = &serial8250_ports[i];

		if (up->port.type != PORT_UNKNOWN && up->port.dev == &dev->dev)
			serial8250_resume_port(i);
	}

	return 0;
}
#endif

static struct platform_driver serial8250_isa_driver = {
	.probe		= serial8250_probe,
	.remove		= __devexit_p(serial8250_remove),
#if !defined(CONFIG_SYNO_COMCERTO) || defined(CONFIG_PM)
	.suspend	= serial8250_suspend,
	.resume		= serial8250_resume,
#endif
	.driver		= {
		.name	= "serial8250",
		.owner	= THIS_MODULE,
	},
};

static struct platform_device *serial8250_isa_devs;

static DEFINE_MUTEX(serial_mutex);

static struct uart_8250_port *serial8250_find_match_or_unused(struct uart_port *port)
{
	int i;

	for (i = 0; i < nr_uarts; i++)
		if (uart_match_port(&serial8250_ports[i].port, port))
			return &serial8250_ports[i];

	for (i = 0; i < nr_uarts; i++)
		if (serial8250_ports[i].port.type == PORT_UNKNOWN &&
		    serial8250_ports[i].port.iobase == 0)
			return &serial8250_ports[i];

	for (i = 0; i < nr_uarts; i++)
		if (serial8250_ports[i].port.type == PORT_UNKNOWN)
			return &serial8250_ports[i];

	return NULL;
}

int serial8250_register_port(struct uart_port *port)
{
	struct uart_8250_port *uart;
	int ret = -ENOSPC;

	if (port->uartclk == 0)
		return -EINVAL;

	mutex_lock(&serial_mutex);

	uart = serial8250_find_match_or_unused(port);
	if (uart) {
		uart_remove_one_port(&serial8250_reg, &uart->port);

		uart->port.iobase       = port->iobase;
		uart->port.membase      = port->membase;
		uart->port.irq          = port->irq;
		uart->port.irqflags     = port->irqflags;
		uart->port.uartclk      = port->uartclk;
		uart->port.fifosize     = port->fifosize;
		uart->port.regshift     = port->regshift;
		uart->port.iotype       = port->iotype;
		uart->port.flags        = port->flags | UPF_BOOT_AUTOCONF;
		uart->port.mapbase      = port->mapbase;
		uart->port.private_data = port->private_data;
		if (port->dev)
			uart->port.dev = port->dev;

		if (port->flags & UPF_FIXED_TYPE)
			serial8250_init_fixed_type_port(uart, port->type);

		set_io_from_upio(&uart->port);
		 
		if (port->serial_in)
			uart->port.serial_in = port->serial_in;
		if (port->serial_out)
			uart->port.serial_out = port->serial_out;
		if (port->handle_irq)
			uart->port.handle_irq = port->handle_irq;
		 
		if (port->set_termios)
			uart->port.set_termios = port->set_termios;
		if (port->pm)
			uart->port.pm = port->pm;

		if (serial8250_isa_config != NULL)
			serial8250_isa_config(0, &uart->port,
					&uart->capabilities);

		ret = uart_add_one_port(&serial8250_reg, &uart->port);
		if (ret == 0)
			ret = uart->port.line;
	}
	mutex_unlock(&serial_mutex);

	return ret;
}
EXPORT_SYMBOL(serial8250_register_port);

void serial8250_unregister_port(int line)
{
	struct uart_8250_port *uart = &serial8250_ports[line];

	mutex_lock(&serial_mutex);
	uart_remove_one_port(&serial8250_reg, &uart->port);
	if (serial8250_isa_devs) {
		uart->port.flags &= ~UPF_BOOT_AUTOCONF;
		uart->port.type = PORT_UNKNOWN;
		uart->port.dev = &serial8250_isa_devs->dev;
		uart->capabilities = uart_config[uart->port.type].flags;
		uart_add_one_port(&serial8250_reg, &uart->port);
	} else {
		uart->port.dev = NULL;
	}
	mutex_unlock(&serial_mutex);
}
EXPORT_SYMBOL(serial8250_unregister_port);

static int __init serial8250_init(void)
{
	int ret;

	if (nr_uarts > UART_NR)
		nr_uarts = UART_NR;

	printk(KERN_INFO "Serial: 8250/16550 driver, "
		"%d ports, IRQ sharing %sabled\n", nr_uarts,
		share_irqs ? "en" : "dis");

#ifdef CONFIG_SPARC
	ret = sunserial_register_minors(&serial8250_reg, UART_NR);
#else
	serial8250_reg.nr = UART_NR;
	ret = uart_register_driver(&serial8250_reg);
#endif
	if (ret)
		goto out;

	serial8250_isa_devs = platform_device_alloc("serial8250",
						    PLAT8250_DEV_LEGACY);
	if (!serial8250_isa_devs) {
		ret = -ENOMEM;
		goto unreg_uart_drv;
	}

	ret = platform_device_add(serial8250_isa_devs);
	if (ret)
		goto put_dev;

	serial8250_register_ports(&serial8250_reg, &serial8250_isa_devs->dev);

	ret = platform_driver_register(&serial8250_isa_driver);
	if (ret == 0)
		goto out;

	platform_device_del(serial8250_isa_devs);
put_dev:
	platform_device_put(serial8250_isa_devs);
unreg_uart_drv:
#ifdef CONFIG_SPARC
	sunserial_unregister_minors(&serial8250_reg, UART_NR);
#else
	uart_unregister_driver(&serial8250_reg);
#endif
out:
	return ret;
}

static void __exit serial8250_exit(void)
{
	struct platform_device *isa_dev = serial8250_isa_devs;

	serial8250_isa_devs = NULL;

	platform_driver_unregister(&serial8250_isa_driver);
	platform_device_unregister(isa_dev);

#ifdef CONFIG_SPARC
	sunserial_unregister_minors(&serial8250_reg, UART_NR);
#else
	uart_unregister_driver(&serial8250_reg);
#endif
}

module_init(serial8250_init);
module_exit(serial8250_exit);

EXPORT_SYMBOL(serial8250_suspend_port);
EXPORT_SYMBOL(serial8250_resume_port);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Generic 8250/16x50 serial driver");

module_param(share_irqs, uint, 0644);
MODULE_PARM_DESC(share_irqs, "Share IRQs with other non-8250/16x50 devices"
	" (unsafe)");

module_param(nr_uarts, uint, 0644);
MODULE_PARM_DESC(nr_uarts, "Maximum number of UARTs supported. (1-" __MODULE_STRING(CONFIG_SERIAL_8250_NR_UARTS) ")");

module_param(skip_txen_test, uint, 0644);
MODULE_PARM_DESC(skip_txen_test, "Skip checking for the TXEN bug at init time");

#ifdef CONFIG_SERIAL_8250_RSA
module_param_array(probe_rsa, ulong, &probe_rsa_count, 0444);
MODULE_PARM_DESC(probe_rsa, "Probe I/O ports for RSA");
#endif
MODULE_ALIAS_CHARDEV_MAJOR(TTY_MAJOR);
