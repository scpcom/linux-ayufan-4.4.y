#ifndef __HI_IRQS_H__
#define __HI_IRQS_H__

#define HISI_GIC_IRQ_START      (32)
#define IRQ_LOCALTIMER          (29)
#define INTNR_COMMTX0           (HISI_GIC_IRQ_START + 52)
#define INTNR_COMMRX0           (HISI_GIC_IRQ_START + 53)
#define INTNR_WATCHDOG          (HISI_GIC_IRQ_START + 4)
#define INTNR_TIMER_0           (HISI_GIC_IRQ_START + 1)
#define INTNR_TIMER_1		(HISI_GIC_IRQ_START + 1)
#define INTNR_TIMER_2		(HISI_GIC_IRQ_START + 2)
#define INTNR_TIMER_3		(HISI_GIC_IRQ_START + 2)
#define INTNR_TIMER_4		(HISI_GIC_IRQ_START + 3)
#define INTNR_TIMER_5		(HISI_GIC_IRQ_START + 3)
#define INTNR_TIMER_6		(HISI_GIC_IRQ_START + 4)
#define INTNR_TIMER_7		(HISI_GIC_IRQ_START + 4)
#define INTNR_GPIO_0            (HISI_GIC_IRQ_START + 57)
#define INTNR_GPIO_1            (HISI_GIC_IRQ_START + 57)
#define INTNR_GPIO_2            (HISI_GIC_IRQ_START + 57)
#define INTNR_GPIO_3            (HISI_GIC_IRQ_START + 57)
#define INTNR_GPIO_4            (HISI_GIC_IRQ_START + 57)
#define INTNR_GPIO_5            (HISI_GIC_IRQ_START + 58)
#define INTNR_GPIO_6            (HISI_GIC_IRQ_START + 58)
#define INTNR_GPIO_7            (HISI_GIC_IRQ_START + 58)
#define INTNR_GPIO_8            (HISI_GIC_IRQ_START + 58)
#define INTNR_GPIO_9            (HISI_GIC_IRQ_START + 58)
#define INTNR_GPIO_10           (HISI_GIC_IRQ_START + 59)
#define INTNR_GPIO_11           (HISI_GIC_IRQ_START + 59)
#define INTNR_GPIO_12           (HISI_GIC_IRQ_START + 59)
#define INTNR_GPIO_13           (HISI_GIC_IRQ_START + 59)
#define INTNR_I2C0              (HISI_GIC_IRQ_START + 12)
#define INTNR_UART0             (HISI_GIC_IRQ_START + 6)
#define INTNR_UART1             (HISI_GIC_IRQ_START + 7)
#define INTNR_UART2             (HISI_GIC_IRQ_START + 8)

#define NR_IRQS                 (HISI_GIC_IRQ_START + 96)

#define MAX_GIC_NR              1

#endif
