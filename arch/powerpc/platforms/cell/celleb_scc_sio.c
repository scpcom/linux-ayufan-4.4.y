#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/tty.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/console.h>

#include <asm/io.h>
#include <asm/prom.h>

static int txx9_serial_bitmap __initdata;

static struct {
	uint32_t offset;
	uint32_t index;
} txx9_scc_tab[3] __initdata = {
	{ 0x300, 0 },	 
	{ 0x400, 0 },	 
	{ 0x800, 1 }	 
};

static int __init txx9_serial_init(void)
{
	extern int early_serial_txx9_setup(struct uart_port *port);
	struct device_node *node;
	int i;
	struct uart_port req;
#if defined(MY_ABC_HERE)
	struct of_phandle_args irq;
#else  
	struct of_irq irq;
#endif  
	struct resource res;

	for_each_compatible_node(node, "serial", "toshiba,sio-scc") {
		for (i = 0; i < ARRAY_SIZE(txx9_scc_tab); i++) {
			if (!(txx9_serial_bitmap & (1<<i)))
				continue;

#if defined(MY_ABC_HERE)
			if (of_irq_parse_one(node, i, &irq))
#else  
			if (of_irq_map_one(node, i, &irq))
#endif  
				continue;
			if (of_address_to_resource(node,
				txx9_scc_tab[i].index, &res))
				continue;

			memset(&req, 0, sizeof(req));
			req.line = i;
			req.iotype = UPIO_MEM;
			req.mapbase = res.start + txx9_scc_tab[i].offset;
#ifdef CONFIG_SERIAL_TXX9_CONSOLE
			req.membase = ioremap(req.mapbase, 0x24);
#endif
#if defined(MY_ABC_HERE)
			req.irq = irq_create_of_mapping(&irq);
#else  
			req.irq = irq_create_of_mapping(irq.controller,
				irq.specifier, irq.size);
#endif  
			req.flags |= UPF_IOREMAP | UPF_BUGGY_UART
				 ;
			req.uartclk = 83300000;
			early_serial_txx9_setup(&req);
		}
	}

	return 0;
}

static int __init txx9_serial_config(char *ptr)
{
	int	i;

	for (;;) {
		switch (get_option(&ptr, &i)) {
		default:
			return 0;
		case 2:
			txx9_serial_bitmap |= 1 << i;
			break;
		case 1:
			txx9_serial_bitmap |= 1 << i;
			return 0;
		}
	}
}
__setup("txx9_serial=", txx9_serial_config);

console_initcall(txx9_serial_init);
