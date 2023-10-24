#ifndef _EAPE_H_
#define _EAPE_H_

#define EAPE_STATUS		(EAPE_BASE_ADDR + 0x0)
#define EAPE_INT_ENABLE		(EAPE_BASE_ADDR + 0x4)
#define EAPE_INT_SRC		(EAPE_BASE_ADDR + 0x8)
#define EAPE_HOST_INT_ENABLE	(EAPE_BASE_ADDR + 0xc)

/** The following bits represents to enable interrupts from host and to host
* from / to utilpe */

#define IRQ_EN_EFET_TO_UTIL	0x1
#define IRQ_EN_QB_TO_UTIL	0x2
#define IRQ_EN_INQ_TO_UTIL	0x4
#define IRQ_EN_EAPE_TO_UTIL	0x8
#define IRQ_EN_GPT_TMR_TO_UTIL	0x10
#define IRQ_EN_UART_TO_UTIL	0x20
#define IRQ_EN_SYSLP_TO_UTIL	0x40
#define IRQ_EN_UPEGP_TO_UTIL	0x80

/** Out interrupts */

#define IRQ_EN_EFET_OUT		0x100
#define IRQ_EN_QB_OUT		0x200
#define IRQ_EN_INQ_OUT		0x400
#define IRQ_EN_EAPE_OUT		0x800
#define IRQ_EN_GPT_TMR_OUT	0x1000
#define IRQ_EN_UART_OUT		0x2000
#define IRQ_EN_SYSLP_OUT	0x4000
#define IRQ_EN_UPEGP_OUT	0x8000

/** The following bits are enabled in the status register
 * which are mapped to IPSEC status register bits */
#define EAPE_IN_STAT_AVAIL      0x1
#define EAPE_OUT_STAT_AVAIL     0x2
#define EAPE_IN_CMD_AVAIL       0x4
#define EAPE_OUT_CMD_AVAIL      0x8

#endif /* _EAPE_H_ */
