#ifndef C2K_DMA_H_
#define C2K_DMA_H_

#ifndef __ASSEMBLY__

//#include <linux/mm_types.h>
//#include <linux/time.h>

//	Mem to IO Control
#define M2IO_START	(1 << 0)
#define M2IO_FLENEN	(1 << 1)
#define M2IO_FCOM	(1 << 2)
#define M2IO_DONOSTOP	(1 << 3)
#define M2IO_DONOSTA	(1 << 4)

//	IO to Mem: DMA Control
#define IO2M_IRQFRDYN	(1 << 0)
#define IO2M_IRQFLST	(1 << 1)
#define IO2M_IRQFDON	(1 << 2)
#define IO2M_IRQFLSH	(1 << 3)
#define IO2M_IRQFLEN	(1 << 4)
#define IO2M_IRQFTHLD	(1 << 5)

// IRQ Status Register
#define IRQ_IRQFRDYN	(1 << 0)
#define IRQ_IRQFLST	(1 << 1)
#define IRQ_IRQFDON	(1 << 2)
#define IRQ_IRQFLSH	(1 << 3)
#define IRQ_IRQFLEN	(1 << 4)
#define IRQ_IRQFTHLD	(1 << 5)
#define IRQ_IRQFCTRL	(1 << 6)

// Inbound Frame and Buffer Descriptor Programming

// BControl
#define BLAST	(1 << 16)
#define BFIX	(1 << 17)

// Block Size
#define XOR_BLOCK_SIZE_256	0
#define XOR_BLOCK_SIZE_512	1
#define XOR_BLOCK_SIZE_1024	2
#define XOR_BLOCK_SIZE_2048	3
#define XOR_BLOCK_SIZE_4096	4

#define COMCERTO_XOR_MAX_SRC    6

#define MDMA_INBOUND_BUF_DESC		256
#define MDMA_OUTBOUND_BUF_DESC	256

#define XOR_INBOUND_BUF_DESC	6
#define XOR_OUTBOUND_BUF_DESC	2

/* FLEN => Maximum no. of fdescs mdma can process at a time is 4k-1 */
/* Need to verify if these many can be created in aram_pool. Don't know whether someone else use iram_pool */
//#define XOR_FDESC_COUNT	256

#define MDMA_MAX_BUF_SIZE		0xffff
#define MDMA_SPLIT_BUF_SIZE		0x8000	/* half a page with 64kB pages */

/* AxCACHE[3:0], bit[3]= WA, bit[2]= RA, bit[1]= C, bit[0]= B */
#define BUFFERABLE		(1 << 0)
#define CACHEABLE		(1 << 1)
#define READ_ALLOC		(1 << 2)
#define WRITE_ALLOC		(1 << 3)

/* AxUSER[0] = coherent(1)/non_coherent(0) */

/* AxUSER[4:1] */
#define STRONGLY_ORDERED	0x0
#define DEVICE			0x1
#define NORMAL_NONCACHEABLE	0x3
#define WRITETHROUGH		0x6
#define WRITEBACK		0x7
#define WRITEBACK_WA		0xf

/* AxPROT[2:0], bit[2]= instruction(1)/data(0), bit[1]= non-secure(1)/secure(0) , bit[0] = privilege(1)/normal(0) */
#define PRIVILEGE		(1 << 0)
#define NON_SECURE		(1 << 1)
#define INSTRUCTION		(1 << 2)


#define AWUSER_COHERENT(x)		((((x) << 1) | 1) << 19)
#define AWPROT(x)			((x) << 16)
#define AWCACHE(x)			((x) << 12)

#define ARUSER_COHERENT(x)		((((x) << 1) | 1) << 7)
#define ARPROT(x)			((x) << 4)
#define ARCACHE(x)			((x) << 0)

enum mdma_transaction_type {
	MDMA_MEMCPY,
	MDMA_XOR,
	MDMA_XOR_VAL,
};

struct comcerto_mdma_buffer_desc {
	u32 bpointer;
	u32 bcontrol;
}__attribute__ ((aligned(8)));

struct comcerto_memcpy_inbound_fdesc {
	u32  next_desc;
	u32  fcontrol;
	u32  fstatus0;
	u32  fstatus1;
	struct comcerto_mdma_buffer_desc bdesc[MDMA_INBOUND_BUF_DESC];
}__attribute__ ((aligned(16)));

struct comcerto_memcpy_outbound_fdesc {
	u32  next_desc;
	u32  fcontrol;
	u32  fstatus0;
	u32  fstatus1;
	struct comcerto_mdma_buffer_desc bdesc[MDMA_OUTBOUND_BUF_DESC];
}__attribute__ ((aligned(16)));

struct comcerto_xor_inbound_fdesc {
	u32  next_desc;
	u32  fcontrol;
	u32  fstatus0;
	u32  fstatus1;
	struct comcerto_mdma_buffer_desc bdesc[XOR_INBOUND_BUF_DESC];
}__attribute__ ((aligned(16)));

struct comcerto_xor_outbound_fdesc {
	u32  next_desc;
	u32  fcontrol;
	u32  fstatus0;
	u32  fstatus1;
	struct comcerto_mdma_buffer_desc bdesc[XOR_OUTBOUND_BUF_DESC];
}__attribute__ ((aligned(16)));

struct comcerto_dma_buf {
	dma_addr_t phys_addr;
	unsigned int len;
	unsigned int split;
};

struct comcerto_dma_sg {
	unsigned int input_idx;
	unsigned int output_idx;
	dma_addr_t high_phys_addr;
	dma_addr_t low_phys_addr;
#if defined(CONFIG_COMCERTO_MDMA_PROF)
	struct timeval start;
	struct timeval end;
#endif
	struct comcerto_dma_buf in_bdesc[MDMA_INBOUND_BUF_DESC];
	struct comcerto_dma_buf out_bdesc[MDMA_OUTBOUND_BUF_DESC];
};

struct mdma_xor_struct {
	int transaction_type;
	int xor_block_size;
	int xor_src_cnt;
	dma_addr_t **xor_srcs;
	dma_addr_t *xor_dest;
};


extern struct comcerto_memcpy_inbound_fdesc *mdma_in_desc;
extern struct comcerto_memcpy_outbound_fdesc *mdma_out_desc;

extern struct comcerto_xor_inbound_fdesc *xor_in_fdesc[];
extern struct comcerto_xor_outbound_fdesc *xor_out_fdesc[];


static inline void comcerto_dma_set_in_bdesc(u32 idx, u32 addr, u32 ctrl)
{
	mdma_in_desc->bdesc[idx].bpointer = addr;
	mdma_in_desc->bdesc[idx].bcontrol = ctrl;
}

static inline void comcerto_dma_set_out_bdesc(u32 idx, u32 addr, u32 ctrl)
{
	mdma_out_desc->bdesc[idx].bpointer = addr;
	mdma_out_desc->bdesc[idx].bcontrol = ctrl;
}

static inline void comcerto_dma_in_bdesc_ctrl_update(u32 idx, u32 ctrl)
{
	mdma_in_desc->bdesc[idx].bcontrol |= ctrl;
}

static inline void comcerto_dma_out_bdesc_ctrl_update(u32 idx, u32 ctrl)
{
	mdma_out_desc->bdesc[idx].bcontrol |= ctrl;
}

/****************** XOR functions ********************/
static inline void mdma_xor_set_in_bdesc(u32 xor_cbuf_wr_cntr, u32 idx, u32 addr, u32 ctrl)
{
	xor_in_fdesc[xor_cbuf_wr_cntr]->bdesc[idx].bpointer = addr;
	xor_in_fdesc[xor_cbuf_wr_cntr]->bdesc[idx].bcontrol = ctrl;
}

static inline void mdma_xor_set_out_bdesc(u32 xor_cbuf_wr_cntr, u32 idx, u32 addr, u32 ctrl)
{
	xor_out_fdesc[xor_cbuf_wr_cntr]->bdesc[idx].bpointer = addr;
	xor_out_fdesc[xor_cbuf_wr_cntr]->bdesc[idx].bcontrol = ctrl;
}

static inline void mdma_xor_in_bdesc_ctrl_update(u32 xor_cbuf_wr_cntr,u32 idx, u32 ctrl)
{
	xor_in_fdesc[xor_cbuf_wr_cntr]->bdesc[idx].bcontrol |= ctrl;
}

static inline void mdma_xor_out_bdesc_ctrl_update(u32 xor_cbuf_wr_cntr,u32 idx, u32 ctrl)
{
	xor_out_fdesc[xor_cbuf_wr_cntr]->bdesc[idx].bcontrol |= ctrl;
}

/****************** XOR functions end ********************/

extern void comcerto_dma_get(void);
extern void comcerto_dma_put(void);
extern void comcerto_dma_set_in_bdesc(u32 idx, u32 addr, u32 ctrl);
extern void comcerto_dma_set_out_bdesc(u32 idx, u32 addr, u32 ctrl);
extern void comcerto_dma_start(void);
extern int comcerto_dma_wait(void);
extern void comcerto_do_mdma_xor(unsigned int src_count, unsigned int bytes, dma_addr_t dest, dma_addr_t *srcs);
extern void comcerto_do_mdma_memcpy(void);

int comcerto_dma_sg_add_input(struct comcerto_dma_sg *sg, void *p, unsigned int len, int use_acp);
int comcerto_dma_sg_add_output(struct comcerto_dma_sg *sg, void *p, unsigned int len, int use_acp);
void comcerto_dma_sg_setup(struct comcerto_dma_sg *sg, unsigned int len);
void comcerto_dma_sg_cleanup(struct comcerto_dma_sg *sg);

static inline void comcerto_dma_sg_init(struct comcerto_dma_sg *sg)
{
	sg->input_idx = 0;
	sg->output_idx = 0;
	sg->high_phys_addr = 0x0;
	sg->low_phys_addr = 0xffffffff;
}

#endif

#endif /* C2K_DMA_H_ */

