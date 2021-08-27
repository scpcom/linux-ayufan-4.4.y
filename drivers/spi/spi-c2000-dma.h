/*
 *  spi-c2000-dma.h 
 *
 *  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *
 */
#ifndef __SPI_C2000_DMA_H__
#define __SPI_C2000_DMA_H___

#include <linux/dmaengine.h>

//========= dma config ==========

#define DMA_CTL_DEST_MSIZE                      0x3 //0x1               /* Number of data items to be transferred = 4 */
#define DMA_CTL_DEST_MSIZE_WR                   0x1 //0x1               /* Number of data items to be transferred = 4 */
#define DMA_CTL_DEST_MSIZE_MASK                 0x7
#define DMA_CTL_DEST_MSIZE_SHIFT                11

#define DMA_CTL_SRC_MSIZE                       0x3 //0x1              /* Number of data items to be transferred = 4 */
#define DMA_CTL_SRC_MSIZE_MASK                  0x7
#define DMA_CTL_SRC_MSIZE_SHIFT                 14

#define DMA_CFG_FIFO_EMPTY                              0x1                     /* Channel FIFO empty */
#define DMA_CFG_FIFO_EMPTY_MASK                 0x1
#define DMA_CFG_FIFO_EMPTY_SHIFT                9

#define DMA_CFG_PROTCTL                                 0x1
#define DMA_CFG_PROTCTL_MASK                    0x7
#define DMA_CFG_PROTCTL_SHIFT                   2

#define DMA_CFG_CH_SUSP                                 0x0                     /* Not suspended */
#define DMA_CFG_CH_SUSP_MASK                    0x1
#define DMA_CFG_CH_SUSP_SHIFT                   8

#define DMA_CTL_DMS                                     0x0                     /* ?????? AHB master 1 */
#define DMA_CTL_DMS_WR                                  0x1                     /* ?????? AHB master 1 */
#define DMA_CTL_DMS_MASK                                0x3
#define DMA_CTL_DMS_SHIFT                               23

#define DMA_CTL_SMS                                     0x1                     /* ?????? AHB master 2 */
#define DMA_CTL_SMS_WR                                  0x0                     /* ?????? AHB master 2 */
#define DMA_CTL_SMS_MASK                                0x3
#define DMA_CTL_SMS_SHIFT                               25

#define DMA_CFG_DEST_PER                                0x0
#define DMA_CFG_DEST_PER_WR                             0x5
#define DMA_CFG_DEST_PER_MASK                   0xF
#define DMA_CFG_DEST_PER_SHIFT                  11

#define DMA_CFG_SRC_PER                                 0x4
#define DMA_CFG_SRC_PER_WR                              0x0
#define DMA_CFG_SRC_PER_MASK                    0xF
#define DMA_CFG_SRC_PER_SHIFT                   7

#define DMA_CFG_HS_SEL_SRC                              0x0             /* Hardware handshaking */
#define DMA_CFG_HS_SEL_SRC_WR                           0x1             /* s/w handshaking */
#define DMA_CFG_HS_SEL_SRC_MASK                 0x1
#define DMA_CFG_HS_SEL_SRC_SHIFT                11

#define DMA_CFG_HS_SEL_DST                              0x1             /* S/w handshaking */
#define DMA_CFG_HS_SEL_DST_WR                           0x0             /* h/w handshaking */
#define DMA_CFG_HS_SEL_DST_MASK                 0x1
#define DMA_CFG_HS_SEL_DST_SHIFT                10

//========= dma config end ======

#define DMA_PREP_CIRCULAR_LIST		(1 << 10)

/*DMA mode configurations*/
enum spi_dma_mode {
	LNW_DMA_PER_TO_MEM = 0, /*periphral to memory configuration*/
	LNW_DMA_MEM_TO_PER,	/*memory to periphral configuration*/
	LNW_DMA_MEM_TO_MEM,	/*mem to mem confg (testing only)*/
};

/*DMA handshaking*/
enum spi_dma_hs_mode {
	LNW_DMA_HW_HS = 0,	/*HW Handshaking only*/
	LNW_DMA_SW_HS = 1,	/*SW Handshaking not recommended*/
};

/*Burst size configuration*/
enum spi_dma_msize {
	LNW_DMA_MSIZE_1 = 0x0,
	LNW_DMA_MSIZE_4 = 0x1,
	LNW_DMA_MSIZE_8 = 0x2,
	LNW_DMA_MSIZE_16 = 0x3,
	LNW_DMA_MSIZE_32 = 0x4,
	LNW_DMA_MSIZE_64 = 0x5,
};

/**
 * struct spi_dma_slave - DMA slave structure
 *
 * @dirn: DMA trf direction
 * @src_width: tx register width
 * @dst_width: rx register width
 * @hs_mode: HW/SW handshaking mode
 * @cfg_mode: DMA data transfer mode (per-per/mem-per/mem-mem)
 * @src_msize: Source DMA burst size
 * @dst_msize: Dst DMA burst size
 * @per_addr: Periphral address
 * @device_instance: DMA peripheral device instance, we can have multiple
 *		peripheral device connected to single DMAC
 */
struct spi_dma_slave {
        struct device           *dma_dev;
        u32                     cfg_hi;
        u32                     cfg_lo;
        u8                      src_master;
        u8                      dst_master;
};

#endif /*__SPI_C2000_DMA_H__*/
