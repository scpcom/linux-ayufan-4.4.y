/* UART */
#define LCR_CHAR_LEN_5          0x00      /* default */
#define LCR_CHAR_LEN_6          0x01
#define LCR_CHAR_LEN_7          0x02
#define LCR_CHAR_LEN_8          0x03
#define LCR_ONE_STOP            0x00      /* One stop bit! - default */
#define LCR_TWO_STOP            0x04      /* Two stop bit! */
#define LCR_PEN                 0x08      /* Parity Enable */
#define LCR_PARITY_NONE         0x00
#define LCR_EPS                 0x10      /* Even Parity Select */
#define LCR_PS                  0x20      /* Enable Parity  Stuff */
#define LCR_SBRK                0x40  /* Start Break */
#define LCR_PSB                 0x80      /* Parity Stuff Bit */
#define LCR_DLAB                0x80  /* UART 16550 Divisor Latch Assess */

#define LSR_FIFOE               (1 << 7)        /* FIFO Error Status */
#define LSR_TEMT                (1 << 6)        /* Transmitter Empty */
#define LSR_TDRQ                (1 << 5)        /* Transmit Data Request */
#define LSR_BI                  (1 << 4)        /* Break Interrupt */
#define LSR_FE                  (1 << 3)        /* Framing Error */
#define LSR_PE                  (1 << 2)        /* Parity Error */
#define LSR_OE                  (1 << 1)        /* Overrun Error */
#define LSR_DR                  (1 << 0)        /* Data Ready */

#define IER_DMAE                (1 << 7)        /* DMA Requests Enable */
#define IER_UUE                 (1 << 6)        /* UART Unit Enable */
#define IER_NRZE                (1 << 5)        /* NRZ coding Enable */
#define IER_RTIOE               (1 << 4)        /* Receiver Time Out Interrupt Enable */
#define IER_MIE                 (1 << 3)        /* Modem Interrupt Enable */
#define IER_RLSE                (1 << 2)        /* Receiver Line Status Interrupt Enable */
#define IER_TIE                 (1 << 1)        /* Transmit Data request Interrupt Enable */
#define IER_RAVIE               (1 << 0)        /* Receiver Data Available Interrupt Enable */

#define IIR_FIFOES1             (1 << 7)        /* FIFO Mode Enable Status */
#define IIR_FIFOES0             (1 << 6)        /* FIFO Mode Enable Status */
#define IIR_TOD                 (1 << 3)        /* Time Out Detected */
#define IIR_IID2                (1 << 2)        /* Interrupt Source Encoded */
#define IIR_IID1                (1 << 1)        /* Interrupt Source Encoded */
#define IIR_IP                  (1 << 0)        /* Interrupt Pending (active low) */

/* UART 16550 FIFO Control Register */
#define FCR_FIFOEN              0x01
#define FCR_RCVRRES             0x02
#define FCR_XMITRES             0x04

/* Interrupt Enable Register */
/* UART 16550 */
#define IER_RXTH                0x01    /* Enable Received Data Available Interrupt */
#define IER_TXTH                0x02    /* Enable Transmitter Empty Interrupt */

#define UART_THR		0x00
#define UART_RBR		0x00
#define UART_DLL		0x00
#define UART_IER		0x04
#define UART_DLH		0x04
#define UART_IIR		0x08
#define UART_FCR		0x08
#define UART_LCR		0x0C
#define UART_MCR		0x10
#define UART_LSR		0x14
#define UART_MSR		0x18
#define UART_SCR		0x1C


