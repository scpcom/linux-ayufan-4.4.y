#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __GIANFAR_H
#define __GIANFAR_H

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/mii.h>
#include <linux/phy.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/crc32.h>
#include <linux/workqueue.h>
#include <linux/ethtool.h>

#ifdef CONFIG_GIANFAR_L2SRAM
#include <asm/fsl_85xx_cache_sram.h>
#define ALIGNMENT 0x20
#endif

#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
#define GFAR_DEV_WEIGHT 16
#else
#define GFAR_DEV_WEIGHT 64
#endif

#define GMAC_FCB_LEN 8

#define DEFAULT_PADDING 2

#define RXBUF_ALIGNMENT 64

#define INCREMENTAL_BUFFER_SIZE 512

#define MAC_ADDR_LEN 6

#define PHY_INIT_TIMEOUT 100000
#define GFAR_PHY_CHANGE_TIME 2

#define DEVICE_NAME "%s: Gianfar Ethernet Controller Version 1.4-skbr1.1.5, "
#define DRV_NAME "gfar-enet"
extern const char gfar_driver_name[];
extern const char gfar_driver_version[];

#define MAX_TX_QS	0x8
#define MAX_RX_QS	0x8

#define MAXGROUPS 0x2

#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
#define DEFAULT_TX_RING_SIZE	1024
#define DEFAULT_RX_RING_SIZE	64
#else
#define DEFAULT_TX_RING_SIZE	256
#define DEFAULT_RX_RING_SIZE	256
#endif

#define DEFAULT_WK_RING_SIZE	16

#define GFAR_TX_MAX_RING_SIZE   65536
#define GFAR_MIN_RING_SIZE	4

#define GFAR_MAX_FIFO_THRESHOLD 511
#define GFAR_MAX_FIFO_STARVE	511
#define GFAR_MAX_FIFO_STARVE_OFF 511

#define DEFAULT_RX_BUFFER_SIZE  1536
#define DEFAULT_WK_BUFFER_SIZE	2048
#define TX_RING_MOD_MASK(size) (size-1)
#define RX_RING_MOD_MASK(size) (size-1)
#define JUMBO_BUFFER_SIZE 9728
#define JUMBO_FRAME_SIZE 9600

#define DEFAULT_FIFO_TX_THR 0x100
#define DEFAULT_FIFO_TX_STARVE 0x40
#define DEFAULT_FIFO_TX_STARVE_OFF 0x80
#define DEFAULT_BD_STASH 1
#define DEFAULT_STASH_LENGTH	96
#define DEFAULT_STASH_INDEX	0

#define PTP_GET_RX_TIMESTAMP_SYNC	SIOCDEVPRIVATE
#define PTP_GET_RX_TIMESTAMP_DEL_REQ	(SIOCDEVPRIVATE + 1)
#define PTP_GET_RX_TIMESTAMP_FOLLOWUP	(SIOCDEVPRIVATE + 2)
#define PTP_GET_RX_TIMESTAMP_DEL_RESP	(SIOCDEVPRIVATE + 3)
#define PTP_GET_TX_TIMESTAMP		(SIOCDEVPRIVATE + 4)
#define PTP_SET_CNT			(SIOCDEVPRIVATE + 5)
#define PTP_GET_CNT			(SIOCDEVPRIVATE + 6)
#define PTP_SET_FIPER_ALARM		(SIOCDEVPRIVATE + 7)
#define PTP_ADJ_ADDEND			(SIOCDEVPRIVATE + 9)
#define PTP_GET_ADDEND			(SIOCDEVPRIVATE + 10)
#define PTP_GET_RX_TIMESTAMP_PDELAY_REQ	(SIOCDEVPRIVATE + 11)
#define PTP_GET_RX_TIMESTAMP_PDELAY_RESP	(SIOCDEVPRIVATE + 12)
#define PTP_CLEANUP_TIMESTAMP_BUFFERS	(SIOCDEVPRIVATE + 13)
#define DEFAULT_PTP_RX_BUF_SZ		2000
#define GFAR_PTP_CTRL_SYNC		0x0
#define GFAR_PTP_CTRL_DEL_REQ		0x1
#define GFAR_PTP_CTRL_FOLLOWUP		0x2
#define GFAR_PTP_CTRL_DEL_RESP		0x3
#define GFAR_PTP_CTRL_ALL_OTHER		0x5
#define GFAR_PTP_MSG_TYPE_PDREQ		0x02
#define GFAR_PTP_MSG_TYPE_PDRESP	0x03
#define GFAR_PTP_DOMAIN_DLFT		0xe0000181
#define GFAR_PTP_PKT_TYPE_OFFS		0x1f
#define GFAR_PTP_PROTOCOL_OFFS		0x20
#define GFAR_PTP_MULTI_ADDR_OFFS	0x26
#define GFAR_PTP_PORT_OFFS		0x2A
#define GFAR_PTP_MSG_TYPE_OFFS		0x32
#define GFAR_PTP_SEQ_ID_OFFS		0x50
#define GFAR_PTP_CTRL_OFFS		0x52
#define GFAR_PACKET_TYPE_UDP		0x11

#define GFAR_EM_NUM	15

#define GFAR_GBIT_TIME  512
#define GFAR_100_TIME   2560
#define GFAR_10_TIME    25600

#define DEFAULT_TX_COALESCE 1
#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
#define DEFAULT_TXCOUNT	22
#define DEFAULT_TXTIME	64
#else
#define DEFAULT_TXCOUNT	16
#define DEFAULT_TXTIME	21
#endif

#define DEFAULT_RXTIME	21

#define DEFAULT_RX_COALESCE 0
#define DEFAULT_RXCOUNT	0

#define GFAR_SUPPORTED (SUPPORTED_10baseT_Half \
		| SUPPORTED_10baseT_Full \
		| SUPPORTED_100baseT_Half \
		| SUPPORTED_100baseT_Full \
		| SUPPORTED_Autoneg \
		| SUPPORTED_MII)

#define MII_TBICON		0x11

#define TBICON_CLK_SELECT	0x0020

#define MACCFG1_SOFT_RESET	0x80000000
#define MACCFG1_RESET_RX_MC	0x00080000
#define MACCFG1_RESET_TX_MC	0x00040000
#define MACCFG1_RESET_RX_FUN	0x00020000
#define	MACCFG1_RESET_TX_FUN	0x00010000
#define MACCFG1_LOOPBACK	0x00000100
#define MACCFG1_RX_FLOW		0x00000020
#define MACCFG1_TX_FLOW		0x00000010
#define MACCFG1_SYNCD_RX_EN	0x00000008
#define MACCFG1_RX_EN		0x00000004
#define MACCFG1_SYNCD_TX_EN	0x00000002
#define MACCFG1_TX_EN		0x00000001

#define MACCFG2_INIT_SETTINGS	0x00007205
#define MACCFG2_FULL_DUPLEX	0x00000001
#define MACCFG2_IF              0x00000300
#define MACCFG2_MII             0x00000100
#define MACCFG2_GMII            0x00000200
#define MACCFG2_HUGEFRAME	0x00000020
#define MACCFG2_LENGTHCHECK	0x00000010
#define MACCFG2_MPEN		0x00000008

#define ECNTRL_INIT_SETTINGS	0x00001000
#define ECNTRL_TBI_MODE         0x00000020
#define ECNTRL_REDUCED_MODE	0x00000010
#define ECNTRL_R100		0x00000008
#define ECNTRL_REDUCED_MII_MODE	0x00000004
#define ECNTRL_SGMII_MODE	0x00000002

#define MRBLR_INIT_SETTINGS	DEFAULT_RX_BUFFER_SIZE

#define MINFLR_INIT_SETTINGS	0x00000040

#define TQUEUE_EN0		0x00008000
#define TQUEUE_EN1		0x00004000
#define TQUEUE_EN2		0x00002000
#define TQUEUE_EN3		0x00001000
#define TQUEUE_EN4		0x00000800
#define TQUEUE_EN5		0x00000400
#define TQUEUE_EN6		0x00000200
#define TQUEUE_EN7		0x00000100
#define TQUEUE_EN_ALL		0x0000FF00

#define TR03WT_WT0_MASK		0xFF000000
#define TR03WT_WT1_MASK		0x00FF0000
#define TR03WT_WT2_MASK		0x0000FF00
#define TR03WT_WT3_MASK		0x000000FF

#define TR47WT_WT4_MASK		0xFF000000
#define TR47WT_WT5_MASK		0x00FF0000
#define TR47WT_WT6_MASK		0x0000FF00
#define TR47WT_WT7_MASK		0x000000FF

#define WRRS_TR03WT		0xFFFFFFFF
#define WRRS_TR47WT		0xFFFFFFFF

#define RQUEUE_EX0		0x00800000
#define RQUEUE_EX1		0x00400000
#define RQUEUE_EX2		0x00200000
#define RQUEUE_EX3		0x00100000
#define RQUEUE_EX4		0x00080000
#define RQUEUE_EX5		0x00040000
#define RQUEUE_EX6		0x00020000
#define RQUEUE_EX7		0x00010000
#define RQUEUE_EX_ALL		0x00FF0000

#define RQUEUE_EN0		0x00000080
#define RQUEUE_EN1		0x00000040
#define RQUEUE_EN2		0x00000020
#define RQUEUE_EN3		0x00000010
#define RQUEUE_EN4		0x00000008
#define RQUEUE_EN5		0x00000004
#define RQUEUE_EN6		0x00000002
#define RQUEUE_EN7		0x00000001
#define RQUEUE_EN_ALL		0x000000FF

#define GIANFAR_WOL_PHY		(1 << 0)
#define GIANFAR_WOL_UCAST	(1 << 1)
#define GIANFAR_WOL_MCAST	(1 << 2)
#define GIANFAR_WOL_BCAST	(1 << 3)
#define GIANFAR_WOL_ARP		(1 << 4)
#define GIANFAR_WOL_MAGIC	(1 << 5)

#define DMACTRL_INIT_SETTINGS   0x000000c3
#define DMACTRL_GRS             0x00000010
#define DMACTRL_GTS             0x00000008

#define TSTAT_TXF_MASK_ALL	0x0000FF00
#define TSTAT_TXF0_MASK		0x00008000

#define TSTAT_CLEAR_THALT_ALL	0xFF000000
#define TSTAT_CLEAR_THALT	0x80000000
#define TSTAT_CLEAR_THALT0	0x80000000
#define TSTAT_CLEAR_THALT1	0x40000000
#define TSTAT_CLEAR_THALT2	0x20000000
#define TSTAT_CLEAR_THALT3	0x10000000
#define TSTAT_CLEAR_THALT4	0x08000000
#define TSTAT_CLEAR_THALT5	0x04000000
#define TSTAT_CLEAR_THALT6	0x02000000
#define TSTAT_CLEAR_THALT7	0x01000000

#define IC_ICEN			0x80000000
#define IC_ICFT_MASK		0x1fe00000
#define IC_ICFT_SHIFT		21
#define mk_ic_icft(x)		\
	(((unsigned int)x << IC_ICFT_SHIFT)&IC_ICFT_MASK)
#define IC_ICTT_MASK		0x0000ffff
#define mk_ic_ictt(x)		(x&IC_ICTT_MASK)

#define mk_ic_value(count, time) (IC_ICEN | \
				mk_ic_icft(count) | \
				mk_ic_ictt(time))
#define get_icft_value(ic)	(((unsigned long)ic & IC_ICFT_MASK) >> \
				 IC_ICFT_SHIFT)
#define get_ictt_value(ic)	((unsigned long)ic & IC_ICTT_MASK)

#define DEFAULT_TXIC mk_ic_value(DEFAULT_TXCOUNT, DEFAULT_TXTIME)
#define DEFAULT_RXIC mk_ic_value(DEFAULT_RXCOUNT, DEFAULT_RXTIME)

#define skip_bd(bdp, stride, base, ring_size) ({ \
	typeof(bdp) new_bd = (bdp) + (stride); \
	(new_bd >= (base) + (ring_size)) ? (new_bd - (ring_size)) : new_bd; })

#define next_bd(bdp, base, ring_size) skip_bd(bdp, 1, base, ring_size)

#define RCTRL_TS_ENABLE		0x01000000
#define RCTRL_PADB_SIZE		(0x8 << 16)
#define RCTRL_PAL_MASK		0x001f0000
#define RCTRL_VLEX		0x00002000
#define RCTRL_FILREN		0x00001000
#define RCTRL_FSQEN		0x00000800
#define RCTRL_GHTX		0x00000400
#define RCTRL_IPCSEN		0x00000200
#define RCTRL_TUCSEN		0x00000100
#define RCTRL_PRSDEP_MASK	0x000000c0
#define RCTRL_PRSDEP_INIT	0x000000c0
#define RCTRL_PRSDEP_L2 	0x00000040
#define RCTRL_PRSDEP_L2L3	0x00000080
#define RCTRL_PRSDEP_L2L3L4	0x000000c0
#define RCTRL_PROM		0x00000008
#define RCTRL_EMEN		0x00000002
#define RCTRL_REQ_PARSER	(RCTRL_VLEX | RCTRL_IPCSEN | \
				 RCTRL_TUCSEN)
#define RCTRL_CHECKSUMMING	(RCTRL_IPCSEN | RCTRL_TUCSEN | \
				RCTRL_PRSDEP_INIT)
#define RCTRL_EXTHASH		(RCTRL_GHTX)
#define RCTRL_VLAN		(RCTRL_PRSDEP_INIT)
#define RCTRL_PADDING(x)	((x << 16) & RCTRL_PAL_MASK)

#define RSTAT_CLEAR_RHALT       0x00800000
#define RSTAT_RXF_ALL_MASK	0x000000FF
#define RSTAT_RXF0_MASK		0x00000080

#define TCTRL_IPCSEN		0x00004000
#define TCTRL_TUCSEN		0x00002000
#define TCTRL_VLINS		0x00001000
#define TCTRL_THDF		0x00000800
#define TCTRL_RFCPAUSE		0x00000010
#define TCTRL_TFCPAUSE		0x00000008
#define TCTRL_TXSCHED_MASK	0x00000006
#define TCTRL_TXSCHED_INIT	0x00000000
#define TCTRL_TXSCHED_PRIO	0x00000002
#define TCTRL_TXSCHED_WRRS	0x00000004
#define TCTRL_INIT_CSUM		(TCTRL_TUCSEN | TCTRL_IPCSEN)

#define IEVENT_INIT_CLEAR	0xffffffff
#define IEVENT_BABR		0x80000000
#define IEVENT_RXC		0x40000000
#define IEVENT_BSY		0x20000000
#define IEVENT_EBERR		0x10000000
#define IEVENT_MSRO		0x04000000
#define IEVENT_GTSC		0x02000000
#define IEVENT_BABT		0x01000000
#define IEVENT_TXC		0x00800000
#define IEVENT_TXE		0x00400000
#define IEVENT_TXB		0x00200000
#define IEVENT_TXF		0x00100000
#define IEVENT_LC		0x00040000
#define IEVENT_CRL		0x00020000
#define IEVENT_XFUN		0x00010000
#define IEVENT_RXB0		0x00008000
#define IEVENT_MAG		0x00000800
#define IEVENT_GRSC		0x00000100
#define IEVENT_RXF0		0x00000080
#define IEVENT_FGPI		0x00000010
#define IEVENT_FIR		0x00000008
#define IEVENT_FIQ		0x00000004
#define IEVENT_DPE		0x00000002
#define IEVENT_PERR		0x00000001
#define IEVENT_RX_MASK          (IEVENT_RXB0 | IEVENT_RXF0 | \
					IEVENT_FGPI | IEVENT_BSY)
#define IEVENT_TX_MASK          (IEVENT_TXB | IEVENT_TXF)
#define IEVENT_RTX_MASK         (IEVENT_RX_MASK | IEVENT_TX_MASK)
#define IEVENT_ERR_MASK         \
(IEVENT_RXC | IEVENT_BSY | IEVENT_EBERR | IEVENT_MSRO | \
 IEVENT_BABT | IEVENT_TXC | IEVENT_TXE | IEVENT_LC \
 | IEVENT_CRL | IEVENT_XFUN | IEVENT_FIR | IEVENT_FIQ \
 | IEVENT_DPE | IEVENT_PERR | IEVENT_MAG | IEVENT_BABR)

#define IMASK_INIT_CLEAR	0x00000000
#define IMASK_BABR              0x80000000
#define IMASK_RXC               0x40000000
#define IMASK_BSY               0x20000000
#define IMASK_EBERR             0x10000000
#define IMASK_MSRO		0x04000000
#define IMASK_GRSC              0x02000000
#define IMASK_BABT		0x01000000
#define IMASK_TXC               0x00800000
#define IMASK_TXEEN		0x00400000
#define IMASK_TXBEN		0x00200000
#define IMASK_TXFEN             0x00100000
#define IMASK_LC		0x00040000
#define IMASK_CRL		0x00020000
#define IMASK_XFUN		0x00010000
#define IMASK_RXB0              0x00008000
#define IMASK_MAG		0x00000800
#define IMASK_GTSC              0x00000100
#define IMASK_RXFEN0		0x00000080
#define IMASK_FGPI		0x00000010
#define IMASK_FIR		0x00000008
#define IMASK_FIQ		0x00000004
#define IMASK_DPE		0x00000002
#define IMASK_PERR		0x00000001
#define IMASK_RX_DISABLED (~(IMASK_RXFEN0 | IMASK_BSY))
#define IMASK_DEFAULT  (IMASK_TXEEN | IMASK_TXFEN | IMASK_TXBEN | \
		IMASK_RXFEN0 | IMASK_BSY | IMASK_EBERR | IMASK_BABR | \
		IMASK_XFUN | IMASK_RXC | IMASK_BABT | IMASK_FGPI | \
		IMASK_FIR | IMASK_FIQ | IMASK_DPE | IMASK_PERR | IMASK_RXB0)
#define IMASK_RTX_DISABLED ((~(IMASK_RXFEN0 | IMASK_RXB0 | IMASK_BSY | \
				IMASK_TXFEN)) & IMASK_DEFAULT)

#ifdef CONFIG_GIANFAR_TXNAPI
#define IMASK_DEFAULT_TX	(IMASK_TXFEN | IMASK_TXBEN)
#define IMASK_DEFAULT_RX	(IMASK_RXFEN0 | IMASK_RXB0 | IMASK_BSY)
#define IMASK_RX_DISABLED	((~IMASK_DEFAULT_RX) \
				& IMASK_DEFAULT)
#define IMASK_TX_DISABLED	((~IMASK_DEFAULT_TX) \
				& IMASK_DEFAULT)
#endif
 
#define FIFO_TX_THR_MASK	0x01ff
#define FIFO_TX_STARVE_MASK	0x01ff
#define FIFO_TX_STARVE_OFF_MASK	0x01ff

#define ATTR_BDSTASH		0x00000800

#define ATTR_BUFSTASH		0x00004000

#define ATTR_SNOOPING		0x000000c0
#define ATTR_INIT_SETTINGS      ATTR_SNOOPING

#define ATTRELI_INIT_SETTINGS   0x0
#define ATTRELI_EL_MASK		0x3fff0000
#define ATTRELI_EL(x) (x << 16)
#define ATTRELI_EI_MASK		0x00003fff
#define ATTRELI_EI(x) (x)

#define BD_LFLAG(flags) ((flags) << 16)
#define BD_LENGTH_MASK		0x0000ffff

#define CLASS_CODE_UNRECOG		0x00
#define CLASS_CODE_DUMMY1		0x01
#define CLASS_CODE_ETHERTYPE1		0x02
#define CLASS_CODE_ETHERTYPE2		0x03
#define CLASS_CODE_USER_PROG1		0x04
#define CLASS_CODE_USER_PROG2		0x05
#define CLASS_CODE_USER_PROG3		0x06
#define CLASS_CODE_USER_PROG4		0x07
#define CLASS_CODE_TCP_IPV4		0x08
#define CLASS_CODE_UDP_IPV4		0x09
#define CLASS_CODE_AH_ESP_IPV4		0x0a
#define CLASS_CODE_SCTP_IPV4		0x0b
#define CLASS_CODE_TCP_IPV6		0x0c
#define CLASS_CODE_UDP_IPV6		0x0d
#define CLASS_CODE_AH_ESP_IPV6		0x0e
#define CLASS_CODE_SCTP_IPV6		0x0f

#define FPR_FILER_MASK	0xFFFFFFFF
#define MAX_FILER_IDX	0xFF
 
#define DEFAULT_RIR0	0x05397700

#define TWO_QUEUE_RIR0	0x04104100

#define RQFCR_GPI		0x80000000
#define RQFCR_HASHTBL_Q		0x00000000
#define RQFCR_HASHTBL_0		0x00020000
#define RQFCR_HASHTBL_1		0x00040000
#define RQFCR_HASHTBL_2		0x00060000
#define RQFCR_HASHTBL_3		0x00080000
#define RQFCR_HASH		0x00010000
#define RQFCR_CLE		0x00000200
#define RQFCR_RJE		0x00000100
#define RQFCR_AND		0x00000080
#define RQFCR_CMP_EXACT		0x00000000
#define RQFCR_CMP_MATCH		0x00000020
#define RQFCR_CMP_NOEXACT	0x00000040
#define RQFCR_CMP_NOMATCH	0x00000060

#define	RQFCR_PID_MASK		0x00000000
#define	RQFCR_PID_PARSE		0x00000001
#define	RQFCR_PID_ARB		0x00000002
#define	RQFCR_PID_DAH		0x00000003
#define	RQFCR_PID_DAL		0x00000004
#define	RQFCR_PID_SAH		0x00000005
#define	RQFCR_PID_SAL		0x00000006
#define	RQFCR_PID_ETY		0x00000007
#define	RQFCR_PID_VID		0x00000008
#define	RQFCR_PID_PRI		0x00000009
#define	RQFCR_PID_TOS		0x0000000A
#define	RQFCR_PID_L4P		0x0000000B
#define	RQFCR_PID_DIA		0x0000000C
#define	RQFCR_PID_SIA		0x0000000D
#define	RQFCR_PID_DPT		0x0000000E
#define	RQFCR_PID_SPT		0x0000000F

#define RQFPR_HDR_GE_512	0x00200000
#define RQFPR_LERR		0x00100000
#define RQFPR_RAR		0x00080000
#define RQFPR_RARQ		0x00040000
#define RQFPR_AR		0x00020000
#define RQFPR_ARQ		0x00010000
#define RQFPR_EBC		0x00008000
#define RQFPR_VLN		0x00004000
#define RQFPR_CFI		0x00002000
#define RQFPR_JUM		0x00001000
#define RQFPR_IPF		0x00000800
#define RQFPR_FIF		0x00000400
#define RQFPR_IPV4		0x00000200
#define RQFPR_IPV6		0x00000100
#define RQFPR_ICC		0x00000080
#define RQFPR_ICV		0x00000040
#define RQFPR_TCP		0x00000020
#define RQFPR_UDP		0x00000010
#define RQFPR_TUC		0x00000008
#define RQFPR_TUV		0x00000004
#define RQFPR_PER		0x00000002
#define RQFPR_EER		0x00000001

#define TXBD_READY		0x8000
#define TXBD_PADCRC		0x4000
#define TXBD_WRAP		0x2000
#define TXBD_INTERRUPT		0x1000
#define TXBD_LAST		0x0800
#define TXBD_CRC		0x0400
#define TXBD_DEF		0x0200
#define TXBD_HUGEFRAME		0x0080
#define TXBD_LATECOLLISION	0x0080
#define TXBD_RETRYLIMIT		0x0040
#define	TXBD_RETRYCOUNTMASK	0x003c
#define TXBD_UNDERRUN		0x0002
#define TXBD_TOE		0x0002

#define TXFCB_VLN		0x80
#define TXFCB_IP		0x40
#define TXFCB_IP6		0x20
#define TXFCB_TUP		0x10
#define TXFCB_UDP		0x08
#define TXFCB_CIP		0x04
#define TXFCB_CTU		0x02
#define TXFCB_NPH		0x01
#define TXFCB_DEFAULT		(TXFCB_IP|TXFCB_CIP)

#define RXBD_EMPTY		0x8000
#define RXBD_RO1		0x4000
#define RXBD_WRAP		0x2000
#define RXBD_INTERRUPT		0x1000
#define RXBD_LAST		0x0800
#define RXBD_FIRST		0x0400
#define RXBD_MISS		0x0100
#define RXBD_BROADCAST		0x0080
#define RXBD_MULTICAST		0x0040
#define RXBD_LARGE		0x0020
#define RXBD_NONOCTET		0x0010
#define RXBD_SHORT		0x0008
#define RXBD_CRCERR		0x0004
#define RXBD_OVERRUN		0x0002
#define RXBD_TRUNCATED		0x0001
#define RXBD_STATS		0x01ff
#define RXBD_CLEAN              0x3000
#define RXBD_ERR		(RXBD_LARGE | RXBD_SHORT | RXBD_NONOCTET 	\
				| RXBD_CRCERR | RXBD_OVERRUN			\
				| RXBD_TRUNCATED)

#define RXFCB_VLN		0x8000
#define RXFCB_IP		0x4000
#define RXFCB_IP6		0x2000
#define RXFCB_TUP		0x1000
#define RXFCB_CIP		0x0800
#define RXFCB_CTU		0x0400
#define RXFCB_EIP		0x0200
#define RXFCB_ETU		0x0100
#define RXFCB_CSUM_MASK		0x0f00
#define RXFCB_PERR_MASK		0x000c
#define RXFCB_PERR_BADL3	0x0008

#define TMR_CTRL_ENABLE		0x00000004
#define TMR_CTRL_RTC_CLK	0x00000003
#define TMR_CTRL_EXT_CLK	0x00000000
#define TMR_CTRL_SYS_CLK	0x00000001
#define TMR_CTRL_TCLK_PRD	0x000A0000
#define TMR_OSC_FREQ		250000000
#define TMR_ADD_VAL		CONFIG_GFAR_PTP_TMR_ADD
#define TMR_CTRL_TCLK_MASK	0x03ff0000
#define TMR_PTPD_MAX_FREQ	0x80000
#define TMR_CTRL_FIPER_START	0x10000000
#define TMR_FIPER1		1000000000
 
#define TMR_ALARM1_L	0xD964B800
#define TMR_ALARM1_H	0x00000045
#define TMR_PRSC	0x2
#define TMR_SEC		1000000000

#define GFAR_INT_NAME_MAX	IFNAMSIZ + 4

#define GFAR_TX_BASE_H	0xf
#define GFAR_RX_BASE_H	0xf

#define TCP_CHL_NUM 5
#define TCP_CHL_OFFSET 2
#define RESERVE_CHL_NUM 3

struct txbd8
{
	union {
		struct {
			u16	status;	 
			u16	length;	 
		};
		u32 lstatus;
	};
	u32	bufPtr;	 
};

struct txfcb {
	u8	flags;
	u8	ptp;	 
	u8	l4os;	 
	u8	l3os; 	 
	u16	phcs;	 
	u16	vlctl;	 
};

struct rxbd8
{
	union {
		struct {
			u16	status;	 
			u16	length;	 
		};
		u32 lstatus;
	};
	u32	bufPtr;	 
};

struct rxfcb {
	u16	flags;
	u8	rq;	 
	u8	pro;	 
	u16	reserved;
	u16	vlctl;	 
};

struct rmon_mib
{
	u32	tr64;	 
	u32	tr127;	 
	u32	tr255;	 
	u32	tr511;	 
	u32	tr1k;	 
	u32	trmax;	 
	u32	trmgv;	 
	u32	rbyt;	 
	u32	rpkt;	 
	u32	rfcs;	 
	u32	rmca;	 
	u32	rbca;	 
	u32	rxcf;	 
	u32	rxpf;	 
	u32	rxuo;	 
	u32	raln;	 
	u32	rflr;	 
	u32	rcde;	 
	u32	rcse;	 
	u32	rund;	 
	u32	rovr;	 
	u32	rfrg;	 
	u32	rjbr;	 
	u32	rdrp;	 
	u32	tbyt;	 
	u32	tpkt;	 
	u32	tmca;	 
	u32	tbca;	 
	u32	txpf;	 
	u32	tdfr;	 
	u32	tedf;	 
	u32	tscl;	 
	u32	tmcl;	 
	u32	tlcl;	 
	u32	txcl;	 
	u32	tncl;	 
	u8	res1[4];
	u32	tdrp;	 
	u32	tjbr;	 
	u32	tfcs;	 
	u32	txcf;	 
	u32	tovr;	 
	u32	tund;	 
	u32	tfrg;	 
	u32	car1;	 
	u32	car2;	 
	u32	cam1;	 
	u32	cam2;	 
};

struct gfar_extra_stats {
	u64 kernel_dropped;
#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
	u64 rx_skbr;
	u64 rx_skbr_free;
#endif
#ifdef CONFIG_NET_GIANFAR_FP
	u64 rx_fast;
#endif
	u64 rx_large;
	u64 rx_short;
	u64 rx_nonoctet;
	u64 rx_crcerr;
	u64 rx_overrun;
	u64 rx_bsy;
	u64 rx_babr;
	u64 rx_trunc;
	u64 eberr;
	u64 tx_babt;
	u64 tx_underrun;
	u64 rx_skbmissing;
	u64 tx_timeout;
};

#define GFAR_RMON_LEN ((sizeof(struct rmon_mib) - 16)/sizeof(u32))
#define GFAR_EXTRA_STATS_LEN (sizeof(struct gfar_extra_stats)/sizeof(u64))

#define GFAR_STATS_LEN (GFAR_RMON_LEN + GFAR_EXTRA_STATS_LEN)

#define GFAR_INFOSTR_LEN 32

struct gfar_stats {
	u64 extra[GFAR_EXTRA_STATS_LEN];
	u64 rmon[GFAR_RMON_LEN];
};

struct gfar_regs_1588 {
	u32	tmr_ctrl;	 
	u32	tmr_tevent;	 
	u32	tmr_temask;	 
	u32	tmr_pevent;	 
	u32	tmr_pemask;	 
	u32	tmr_stat;	 
	u32	tmr_cnt_h;	 
	u32	tmr_cnt_l;	 
	u32	tmr_add;	 
					 
	u32	tmr_acc;	 
	u32	tmr_prsc;	 
	u8	res24a[4];	 
	u32	tmr_off_h;	 
	u32	tmr_off_l;	 
	u8	res24b[8];	 
	u32	tmr_alarm1_h;	 
	u32	tmr_alarm1_l;	 
	u32	tmr_alarm2_h;	 
	u32	tmr_alarm2_l;	 
	u8	res24c[48];	 
	u32	tmr_fiper1;	 
	u32	tmr_fiper2;	 
	u32	tmr_fiper3;	 
	u8	res24d[20];	 
	u32	tmr_etts1_h;	 
					 
	u32	tmr_etts1_l;	 
					 
	u32	tmr_etts2_h;	 
					 
	u32	tmr_etts2_l;	 
};

struct gfar {
	u32	tsec_id;	 
	u32	tsec_id2;	 
	u8	res1[8];
	u32	ievent;		 
	u32	imask;		 
	u32	edis;		 
	u32	emapg;		 
	u32	ecntrl;		 
	u32	minflr;		 
	u32	ptv;		 
	u32	dmactrl;	 
	u32	tbipa;		 
	u8	res2[28];
	u32	fifo_rx_pause;	 
	u32	fifo_rx_pause_shutoff;	 
	u32	fifo_rx_alarm;	 
	u32	fifo_rx_alarm_shutoff;	 
	u8	res3[44];
	u32	fifo_tx_thr;	 
	u8	res4[8];
	u32	fifo_tx_starve;	 
	u32	fifo_tx_starve_shutoff;	 
	u8	res5[96];
	u32	tctrl;		 
	u32	tstat;		 
	u32	dfvlan;		 
	u32	tbdlen;		 
	u32	txic;		 
	u32	tqueue;		 
	u8	res7[40];
	u32	tr03wt;		 
	u32	tr47wt;		 
	u8	res8[52];
	u32	tbdbph;		 
	u8	res9a[4];
	u32	tbptr0;		 
	u8	res9b[4];
	u32	tbptr1;		 
	u8	res9c[4];
	u32	tbptr2;		 
	u8	res9d[4];
	u32	tbptr3;		 
	u8	res9e[4];
	u32	tbptr4;		 
	u8	res9f[4];
	u32	tbptr5;		 
	u8	res9g[4];
	u32	tbptr6;		 
	u8	res9h[4];
	u32	tbptr7;		 
	u8	res9[64];
	u32	tbaseh;		 
	u32	tbase0;		 
	u8	res10a[4];
	u32	tbase1;		 
	u8	res10b[4];
	u32	tbase2;		 
	u8	res10c[4];
	u32	tbase3;		 
	u8	res10d[4];
	u32	tbase4;		 
	u8	res10e[4];
	u32	tbase5;		 
	u8	res10f[4];
	u32	tbase6;		 
	u8	res10g[4];
	u32	tbase7;		 
	u8	res10h[64];
	u32	tmr_txts1_id;	 
	u32	tmr_txts2_id;	 
	u8	res10i[56];
	u32	tmr_txts1_h;	 
	u32	tmr_txts1_l;	 
	u32	tmr_txts2_h;	 
	u32	tmr_txts2_l;	 
	u8	res10j[48];
	u32	rctrl;		 
	u32	rstat;		 
	u8	res12[8];
	u32	rxic;		 
	u32	rqueue;		 
	u32	rir0;		 
	u32	rir1;		 
	u32	rir2;		 
	u32	rir3;		 
	u8	res13[8];
	u32	rbifx;		 
	u32	rqfar;		 
	u32	rqfcr;		 
	u32	rqfpr;		 
	u32	mrblr;		 
	u8	res14[56];
	u32	rbdbph;		 
	u8	res15a[4];
	u32	rbptr0;		 
	u8	res15b[4];
	u32	rbptr1;		 
	u8	res15c[4];
	u32	rbptr2;		 
	u8	res15d[4];
	u32	rbptr3;		 
	u8	res15e[4];
	u32	rbptr4;		 
	u8	res15f[4];
	u32	rbptr5;		 
	u8	res15g[4];
	u32	rbptr6;		 
	u8	res15h[4];
	u32	rbptr7;		 
	u8	res16[64];
	u32	rbaseh;		 
	u32	rbase0;		 
	u8	res17a[4];
	u32	rbase1;		 
	u8	res17b[4];
	u32	rbase2;		 
	u8	res17c[4];
	u32	rbase3;		 
	u8	res17d[4];
	u32	rbase4;		 
	u8	res17e[4];
	u32	rbase5;		 
	u8	res17f[4];
	u32	rbase6;		 
	u8	res17g[4];
	u32	rbase7;		 
	u8	res17h[128];
	u32	tmr_rxts_h;	 
	u32	tmr_rxts_l;	 
	u8	res17i[56];
	u32	maccfg1;	 
	u32	maccfg2;	 
	u32	ipgifg;		 
	u32	hafdup;		 
	u32	maxfrm;		 
	u8	res18[12];
	u8	gfar_mii_regs[24];	 
	u32	ifctrl;		 
	u32	ifstat;		 
	u32	macstnaddr1;	 
	u32	macstnaddr2;	 
	u32	mac01addr1;	 
	u32	mac01addr2;	 
	u32	mac02addr1;	 
	u32	mac02addr2;	 
	u32	mac03addr1;	 
	u32	mac03addr2;	 
	u32	mac04addr1;	 
	u32	mac04addr2;	 
	u32	mac05addr1;	 
	u32	mac05addr2;	 
	u32	mac06addr1;	 
	u32	mac06addr2;	 
	u32	mac07addr1;	 
	u32	mac07addr2;	 
	u32	mac08addr1;	 
	u32	mac08addr2;	 
	u32	mac09addr1;	 
	u32	mac09addr2;	 
	u32	mac10addr1;	 
	u32	mac10addr2;	 
	u32	mac11addr1;	 
	u32	mac11addr2;	 
	u32	mac12addr1;	 
	u32	mac12addr2;	 
	u32	mac13addr1;	 
	u32	mac13addr2;	 
	u32	mac14addr1;	 
	u32	mac14addr2;	 
	u32	mac15addr1;	 
	u32	mac15addr2;	 
	u8	res20[192];
	struct rmon_mib	rmon;	 
	u32	rrej;		 
	u8	res21[188];
	u32	igaddr0;	 
	u32	igaddr1;	 
	u32	igaddr2;	 
	u32	igaddr3;	 
	u32	igaddr4;	 
	u32	igaddr5;	 
	u32	igaddr6;	 
	u32	igaddr7;	 
	u8	res22[96];
	u32	gaddr0;		 
	u32	gaddr1;		 
	u32	gaddr2;		 
	u32	gaddr3;		 
	u32	gaddr4;		 
	u32	gaddr5;		 
	u32	gaddr6;		 
	u32	gaddr7;		 
	u8	res23a[352];
	u32	fifocfg;	 
	u8	res23b[252];
	u8	res23c[248];
	u32	attr;		 
	u32	attreli;	 
	u8	res24[512];
	struct gfar_regs_1588 regs_1588;
	u32	isrg0;		 
	u32	isrg1;		 
	u32	isrg2;		 
	u32	isrg3;		 
	u8	res25[16];
	u32	rxic0;		 
	u32	rxic1;		 
	u32	rxic2;		 
	u32	rxic3;		 
	u32	rxic4;		 
	u32	rxic5;		 
	u32	rxic6;		 
	u32	rxic7;		 
	u8	res26[32];
	u32	txic0;		 
	u32	txic1;		 
	u32	txic2;		 
	u32	txic3;		 
	u32	txic4;		 
	u32	txic5;		 
	u32	txic6;		 
	u32	txic7;		 
	u8	res27[208];
};

#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
#define GFAR_DEFAULT_RECYCLE_MAX 64
#define GFAR_DEFAULT_RECYCLE_TRUESIZE (SKB_DATA_ALIGN(DEFAULT_RX_BUFFER_SIZE \
		+ RXBUF_ALIGNMENT + NET_SKB_PAD) + sizeof(struct sk_buff))

struct gfar_skb_handler {
	 
	spinlock_t	lock;
	short int	recycle_max;
	short int	recycle_count;
	short int	recycle_enable;
	struct sk_buff *recycle_queue;
};

extern void gfar_free_recycle_queue(struct gfar_skb_handler *sh,
		int lock_flag);
#endif

struct gfar_ptp_time {
	u32	high;
	u32	low;
};

struct gfar_ptp_data_t {
	int	key;
	struct	gfar_ptp_time	item;
};

struct gfar_ptp_circular_t {
	int	front;
	int	end;
	int	size;
	struct	gfar_ptp_data_t *data_buf;
};

#define FSL_GIANFAR_DEV_HAS_GIGABIT		0x00000001
#define FSL_GIANFAR_DEV_HAS_COALESCE		0x00000002
#define FSL_GIANFAR_DEV_HAS_RMON		0x00000004
#define FSL_GIANFAR_DEV_HAS_MULTI_INTR		0x00000008
#define FSL_GIANFAR_DEV_HAS_CSUM		0x00000010
#define FSL_GIANFAR_DEV_HAS_VLAN		0x00000020
#define FSL_GIANFAR_DEV_HAS_EXTENDED_HASH	0x00000040
#define FSL_GIANFAR_DEV_HAS_PADDING		0x00000080
#define FSL_GIANFAR_DEV_HAS_MAGIC_PACKET	0x00000100
#define FSL_GIANFAR_DEV_HAS_BD_STASHING		0x00000200
#define FSL_GIANFAR_DEV_HAS_BUF_STASHING	0x00000400
#define FSL_GIANFAR_DEV_HAS_ARP_PACKET		0x00000800

#if (MAXGROUPS == 2)
#define DEFAULT_MAPPING 	0xAA
#else
#define DEFAULT_MAPPING 	0xFF
#endif

#define ISRG_SHIFT_TX	0x10
#define ISRG_SHIFT_RX	0x18

enum {
	SQ_SG_MODE = 0,
	MQ_MG_MODE
};

struct gfar_priv_tx_q {
	spinlock_t txlock __attribute__ ((aligned (SMP_CACHE_BYTES)));
	struct sk_buff ** tx_skbuff;
	 
	struct	txbd8 *tx_bd_base;
	struct	txbd8 *cur_tx;
	struct	txbd8 *dirty_tx;
	struct	net_device *dev;
	struct gfar_priv_grp *grp;
	u16	skb_curtx;
	u16	skb_dirtytx;
	u16	qindex;
	unsigned int tx_ring_size;
	unsigned int num_txbdfree;
	 
	unsigned char txcoalescing;
	unsigned long txic;
	unsigned short txcount;
	unsigned short txtime;
#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
	struct gfar_skb_handler *local_sh;  
#endif
};

struct rx_q_stats {
	unsigned long rx_packets;
	unsigned long rx_bytes;
	unsigned long rx_dropped;
};

struct gfar_priv_rx_q {
	spinlock_t rxlock __attribute__ ((aligned (SMP_CACHE_BYTES)));
	struct	sk_buff ** rx_skbuff;
	struct	rxbd8 *rx_bd_base;
	struct	rxbd8 *cur_rx;
	struct	net_device *dev;
	struct gfar_priv_grp *grp;
	struct	rx_q_stats stats;
	u16	skb_currx;
	u16	qindex;
	unsigned int	rx_ring_size;
	 
	unsigned char rxcoalescing;
	unsigned long rxic;
#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
	unsigned int rx_skbuff_truesize;
	struct gfar_skb_handler skb_handler;
	struct gfar_skb_handler *local_sh;  
#endif
};

struct gfar_priv_grp {
	spinlock_t grplock __attribute__ ((aligned (SMP_CACHE_BYTES)));
#ifdef CONFIG_GIANFAR_TXNAPI
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	struct napi_struct napi_tx[NR_CPUS];
#else
	struct napi_struct napi_tx;
#endif
	struct napi_struct napi_rx;
#else
	struct	napi_struct napi;
#endif
	struct gfar_private *priv;
	struct gfar __iomem *regs;
	unsigned int grp_id;
	unsigned long rx_bit_map;
	unsigned long tx_bit_map;
	unsigned long num_tx_queues;
	unsigned long num_rx_queues;
	unsigned int rstat;
	unsigned int tstat;
	unsigned int imask;
	unsigned int ievent;
	unsigned int interruptTransmit;
	unsigned int interruptReceive;
	unsigned int interruptError;

	char int_name_tx[GFAR_INT_NAME_MAX];
	char int_name_rx[GFAR_INT_NAME_MAX];
	char int_name_er[GFAR_INT_NAME_MAX];
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	struct fsl_msg_unit *msg_virtual_tx[NR_CPUS];
	char int_name_vtx[NR_CPUS][GFAR_INT_NAME_MAX];
#endif
};

struct gfar_private {

	unsigned int num_tx_queues;
	unsigned int num_rx_queues;
	unsigned int num_grps;
	unsigned int mode;

	unsigned int total_tx_ring_size;
	unsigned int total_rx_ring_size;

	struct device_node *node;
	struct net_device *ndev;
	struct of_device *ofdev;

	struct gfar_priv_grp gfargrp[MAXGROUPS];
	struct gfar_priv_tx_q *tx_queue[MAX_TX_QS];
	struct gfar_priv_rx_q *rx_queue[MAX_RX_QS];

	unsigned int rx_buffer_size;
	unsigned int rx_stash_size;
	unsigned int rx_stash_index;

	u32 cur_filer_idx;
#ifdef CONFIG_GFAR_HW_TCP_RECEIVE_OFFLOAD
	u16 tcp_filer_idx;
	u16 empty_tcp_channel;
	struct sock *tcp_hw_channel[MAX_RX_QS];
#endif

	struct rxbd8 *wk_bd_base;
	struct rxbd8 *cur_wk;

	unsigned int wk_ring_size;
	unsigned int wk_buffer_size;

	unsigned long wk_buf_vaddr;
	unsigned long wk_buf_paddr;
	unsigned long wk_buf_align_vaddr;
	unsigned long wk_buf_align_paddr;

	struct vlan_group *vlgrp;

	u32 __iomem *hash_regs[16];
	int hash_width;

	unsigned int fifo_threshold;
	unsigned int fifo_starve;
	unsigned int fifo_starve_off;

	spinlock_t bflock;

	phy_interface_t interface;
	struct device_node *phy_node;
	struct device_node *tbi_node;
	u32 device_flags;
	unsigned char rx_csum_enable:1,
		extended_hash:1,
		bd_stash_en:1,
		filer_en:1,
		wol_en:1;  
	unsigned short padding;

	struct phy_device *phydev;
	struct mii_bus *mii_bus;
	int oldspeed;
	int oldduplex;
	int oldlink;

	uint32_t msg_enable;

	struct work_struct reset_task;

	u8 ip_addr[4];
	int wol_opts;
	int should_wake;

	struct gfar_extra_stats extra_stats;
	struct gfar_ptp_circular_t rx_time_sync;
	struct gfar_ptp_circular_t rx_time_del_req;

	struct gfar_ptp_circular_t rx_time_pdel_req;
	struct gfar_ptp_circular_t rx_time_pdel_resp;
	struct gfar_regs_1588 __iomem *ptimer;
	struct resource timer_resource;
	uint32_t ptimer_present;
#ifdef CONFIG_GIANFAR_L2SRAM
	int bd_in_ram;
#endif
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	int sps;  
#endif
#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
	unsigned int skbuff_truesize;
	struct gfar_skb_handler skb_handler;
#endif
#ifdef MY_DEF_HERE
	unsigned long wbuf_vaddr;
	unsigned long wbuf_addr;
	unsigned long wbuf_size;
	unsigned long bds_vaddr;
	unsigned long bds_addr;
#endif

	unsigned int ftp_rqfpr[MAX_FILER_IDX + 1];
	unsigned int ftp_rqfcr[MAX_FILER_IDX + 1];
};

#ifdef CONFIG_GFAR_SW_PKT_STEERING
#define INTR_COALESCE_CNT 22
#define INTR_COALESCE_TIMEOUT 32000  
#define GFAR_CPU_BUFF_SIZE 64

struct shared_buffer {
	struct sk_buff *buffer[GFAR_CPU_BUFF_SIZE];
	int in;
	int out;
	atomic_t buff_cnt;
};

struct gfar_cpu_dev {
	struct net_device dev;
	struct napi_struct napi;
	struct shared_buffer tx_queue;
	int intr_coalesce_cnt;
	struct hrtimer intr_coalesce_timer;
	struct fsl_msg_unit *msg_virtual_rx;
	char int_name[GFAR_INT_NAME_MAX];
	int enabled;
#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
	struct gfar_skb_handler sh;
#endif
};

extern int rcv_pkt_steering;
#endif

extern unsigned int ftp_rqfpr[MAX_FILER_IDX + 1];
extern unsigned int ftp_rqfcr[MAX_FILER_IDX + 1];

static inline u32 gfar_read(volatile unsigned __iomem *addr)
{
	u32 val;
	val = in_be32(addr);
	return val;
}

static inline void gfar_write(volatile unsigned __iomem *addr, u32 val)
{
	out_be32(addr, val);
}

static inline void gfar_read_filer(struct gfar_private *priv,
		unsigned int far, u32 *fcr, u32 *fpr)
{
	struct gfar __iomem *regs = priv->gfargrp[0].regs;

	gfar_write(&regs->rqfar, far);
	*fcr = gfar_read(&regs->rqfcr);
	*fpr = gfar_read(&regs->rqfpr);
}

 static inline void gfar_write_filer(struct gfar_private *priv,
		unsigned int far, unsigned int fcr, unsigned int fpr)
{
	struct gfar __iomem *regs = priv->gfargrp[0].regs;

	gfar_write(&regs->rqfar, far);
	gfar_write(&regs->rqfcr, fcr);
	gfar_write(&regs->rqfpr, fpr);
}

static inline void lock_rx_qs(struct gfar_private *priv)
{
        int i = 0x0;

        for (i = 0; i < priv->num_rx_queues; i++)
                spin_lock(&priv->rx_queue[i]->rxlock);
}

static inline void lock_tx_qs(struct gfar_private *priv)
{
        int i = 0x0;

        for (i = 0; i < priv->num_tx_queues; i++)
                spin_lock(&priv->tx_queue[i]->txlock);
}

static inline void unlock_rx_qs(struct gfar_private *priv)
{
        int i = 0x0;

        for (i = 0; i < priv->num_rx_queues; i++)
                spin_unlock(&priv->rx_queue[i]->rxlock);
}

static inline void unlock_tx_qs(struct gfar_private *priv)
{
        int i = 0x0;

        for (i = 0; i < priv->num_tx_queues; i++)
                spin_unlock(&priv->tx_queue[i]->txlock);
}

extern irqreturn_t gfar_receive(int irq, void *dev_id);
extern int startup_gfar(struct net_device *dev);
extern void stop_gfar(struct net_device *dev);
extern void gfar_halt(struct net_device *dev);
extern void gfar_1588_start(struct net_device *dev);
extern void gfar_1588_stop(struct net_device *dev);
extern int gfar_ptp_init(struct gfar_private *priv);
extern void gfar_ptp_cleanup(struct gfar_private *priv);
extern int gfar_ptp_do_txstamp(struct sk_buff *skb);
extern void pmuxcr_guts_write(void);
extern void gfar_ptp_store_rxstamp(struct net_device *dev, struct sk_buff *skb);
extern int gfar_ioctl_1588(struct net_device *dev, struct ifreq *ifr, int cmd);
extern void gfar_phy_test(struct mii_bus *bus, struct phy_device *phydev,
		int enable, u32 regnum, u32 read);
extern void gfar_configure_tx_coalescing(struct gfar_private *priv,
					unsigned long tx_mask);
extern void gfar_configure_rx_coalescing(struct gfar_private *priv,
					unsigned long rx_mask);
void gfar_init_sysfs(struct net_device *dev);

extern const struct ethtool_ops gfar_ethtool_ops;

#ifdef CONFIG_NET_GIANFAR_FP
extern int netdev_fastroute;
#endif
#endif  
