/*
;=======================================================================
;        Filename : dma.c
;        Purpose  : DW DMA APIs for Fast SPI and Fast UART.
;======================================================================
*/

#include <mach/serial.h>
#include <mach/hardware.h>
#include <mach/dma.h>
#include <asm/io.h>
#include <linux/module.h>




/*
 ********************************************
 * dma_ssi_xfer_cmplete_chk ()
 *
 * Check for the complete of DMA data xfer on given channal no.
 *
 ********************************************
 */
retcode dma_xfer_cmplete_chk(unsigned int ch_no)
{
        unsigned int channel_status = 0xF;

        while(channel_status != 0x0)
        {
                channel_status = readl(DW_DMA_DMAC_CH_EN_REG);
		channel_status = channel_status & ((1 << ch_no) & 0xFF);
	}

	return RETCODE_OK;
}

EXPORT_SYMBOL(dma_xfer_cmplete_chk);

/*
 ****************************************
 *   dma_configure ()
 *
 *   Single-block Transfer -- without write-back of control
 *   and status information enabled at the end of the single-block transfer
 ****************************************
 */
void dma_configure(unsigned int source_add, unsigned int target_add, unsigned int data_len, unsigned int ch_no)
{
	unsigned int dma_channel = (1 << ch_no) & 0xFF ;
	unsigned int ch_reg_multiplier = 0;
	unsigned int ch_ctrl_l = 0;
	unsigned int ch_cfg_l = 0;
	unsigned int ch_cfg_h = 0;

	/* DMA enable */
	writel(DMA_GLOBAL_ENABLE, DW_DMA_DMAC_DMA_CFG_REG);

	/* verify channel is not busy, done with last transaction */
	dma_xfer_cmplete_chk(ch_no);

	/* Configure DMA handshaking */

	/* Writing a 1 to the Last Destination Transaction Request Register initiates a transaction. */
	writel(((dma_channel << DMA_REG_WE_SHIFT) | (0x0 & 0xFF)), DW_DMA_DMAC_LST_DST_REG);

/* The type of transaction, single or burst, depends on the state of the corresponding channel bit
 * in the Single Source/Destination Transaction Request register.
 */
	writel(((dma_channel << DMA_REG_WE_SHIFT) | (0x0 & 0xFF)), DW_DMA_DMAC_SGL_REQ_DST_REG);

	/* Clear Interrupts on the channal */

	/* Clear for IntTfr Interrupt */
	writel(dma_channel, DW_DMA_DMAC_CLEAR_TFR);
	/* Clear for IntBlock Interrupt */
	writel(dma_channel, DW_DMA_DMAC_CLEAR_BLK);
	/* Clear for IntSrcTran Interrupts */
	writel(dma_channel, DW_DMA_DMAC_CLEAR_SRC_TRAN);
	/* Clear for IntDstTran Interrupt */
	writel(dma_channel, DW_DMA_DMAC_CLEAR_DST_TRAN);
	/* Clear for IntErr Interrupt */
	writel(dma_channel, DW_DMA_DMAC_CLEAR_ERR);

	/* Set up Interrupt Mask registers */

	/* Mask for IntTfr Interrupt */
	writel(((dma_channel << DMA_REG_WE_SHIFT) | (0x0 & 0xFF)), DW_DMA_DMAC_MASK_TFR);
	/* Mask for IntBlock Interrupt */
	writel(((dma_channel << DMA_REG_WE_SHIFT) | dma_channel), DW_DMA_DMAC_MASK_BLOCK);
	/* Mask for IntSrcTran Interrupt */
	writel(((dma_channel << DMA_REG_WE_SHIFT) | dma_channel), DW_DMA_DMAC_MASK_SRC_TRAN);
	/* Mask for IntDstTran Interrupt */
	writel(((dma_channel << DMA_REG_WE_SHIFT) | dma_channel), DW_DMA_DMAC_MASK_DST_TRAN);
	/* Mask for IntErr Interrupt */
	writel(((dma_channel << DMA_REG_WE_SHIFT) | (0x0 & 0xFF)), DW_DMA_DMAC_MASK_ERR);

	/* configure channel specific registers */
	if( ch_no != 0 )
	{
		ch_reg_multiplier = (DMA_CHANNEL_REG_COUNT << 3) ;
		ch_reg_multiplier = ch_no * ch_reg_multiplier;
	}

	/* configure : Source Address Register for Channel */

/* The starting source address is programmed before the DMA channel is enabled. While the DMA transfer is in
 * progress, this register is updated to reflect the source address of the current AHB transfer.
 */
	/* SAR Address must be alligned to DMA_CTL_SRC_TR_WIDTH boundry */
	writel(source_add, (DMA_CHANNEL_REG_SAR_BASE + ch_reg_multiplier));

	/* configure : Destination Address Register for Channel */

/* The starting destination address is programmed before the DMA channel is enabled. While the DMA transfer is in
 * progress, this register is updated to reflect the destination address of the current AHB transfer.
 */
	/* DAR Address must be alligned to DMA_CTL_DST_TR_WIDTH boundry */
	writel(target_add, (DMA_CHANNEL_REG_DAR_BASE + ch_reg_multiplier));


	/* configure : Control Register for Channel [32-63] */
/* The number programmed into BLOCK_TS indicates the total number of single transactions
 * to perform for every block transfer; a single transaction is mapped to a single AMBA beat.
 */
	writel(data_len, (DMA_CHANNEL_REG_CTL_BASE + ch_reg_multiplier + ch_no));

	/* configure : Control Register for Channel [0-31] */
	ch_ctrl_l = (((DMA_CTL_INT_EN & DMA_CTL_INT_EN_MASK) << DMA_CTL_INT_EN_SHIFT) |
				((DMA_CTL_DST_TR_WIDTH & DMA_CTL_DST_TR_WIDTH_MASK) << DMA_CTL_DST_TR_WIDTH_SHIFT) |
				((DMA_CTL_SRC_TR_WIDTH & DMA_CTL_SRC_TR_WIDTH_MASK) << DMA_CTL_SRC_TR_WIDTH_SHIFT) |
				((DMA_CTL_DINC & DMA_CTL_DINC_MASK) << DMA_CTL_DINC_SHIFT) |
				((DMA_CTL_SINC & DMA_CTL_SINC_MASK) << DMA_CTL_SINC_SHIFT) |
				((DMA_CTL_DEST_MSIZE & DMA_CTL_DEST_MSIZE_MASK) << DMA_CTL_DEST_MSIZE_SHIFT) |
				((DMA_CTL_SRC_MSIZE & DMA_CTL_SRC_MSIZE_MASK) << DMA_CTL_SRC_MSIZE_SHIFT) |
				((DMA_CTL_SRC_GATHER_EN & DMA_CTL_SRC_GATHER_EN_MASK) << DMA_CTL_SRC_GATHER_EN_SHIFT) |
				((DMA_CTL_DST_SCATTER_EN & DMA_CTL_DST_SCATTER_EN_MASK) << DMA_CTL_DST_SCATTER_EN_SHIFT) |
				((DMA_CTL_TT_FC & DMA_CTL_TT_FC_MASK) << DMA_CTL_TT_FC_SHIFT) |
				((DMA_CTL_DMS & DMA_CTL_DMS_MASK) << DMA_CTL_DMS_SHIFT) |
				((DMA_CTL_SMS & DMA_CTL_SMS_MASK) << DMA_CTL_SMS_SHIFT) |
				((DMA_CTL_LLP_DST_EN & DMA_CTL_LLP_DST_EN_MASK) << DMA_CTL_LLP_DST_EN_SHIFT) |
				((DMA_CTL_LLP_SRC_EN & DMA_CTL_LLP_SRC_EN_MASK) << DMA_CTL_LLP_SRC_EN_SHIFT));

	writel(ch_ctrl_l, (DMA_CHANNEL_REG_CTL_BASE + ch_reg_multiplier));

	/* configure : Linked List Pointer Register for Channel */
	writel(0x0, (DMA_CHANNEL_REG_LLP_BASE + ch_reg_multiplier));

	/* configure : Configuration Register for Channel [32-63] */
	ch_cfg_h = (((DMA_CFG_FCMODE & DMA_CFG_FCMODE_MASK) << DMA_CFG_FCMODE_SHIFT) |
			   ((DMA_CFG_FIFO_MODE & DMA_CFG_FIFO_MODE_MASK) << DMA_CFG_FIFO_MODE_SHIFT) |
			   ((DMA_CFG_PROTCTL & DMA_CFG_PROTCTL_MASK) << DMA_CFG_PROTCTL_SHIFT) |
			   ((DMA_CFG_DS_UPD_EN & DMA_CFG_DS_UPD_EN_MASK) << DMA_CFG_DS_UPD_EN_SHIFT) |
			   ((DMA_CFG_SS_UPD_EN & DMA_CFG_SS_UPD_EN_MASK) << DMA_CFG_SS_UPD_EN_SHIFT) |
			   ((ch_no /*DMA_CFG_SRC_PER*/ & DMA_CFG_SRC_PER_MASK) << DMA_CFG_SRC_PER_SHIFT) |
			   ((DMA_CFG_DEST_PER & DMA_CFG_DEST_PER_MASK) << DMA_CFG_DEST_PER_SHIFT));

	writel(ch_cfg_h, (DMA_CHANNEL_REG_CFG_BASE + ch_reg_multiplier + ch_no));

	/* configure : Configuration Register for Channel [0-31] */
	ch_cfg_l = (((ch_no & DMA_CFG_CH_PRIOR_MASK) << DMA_CFG_CH_PRIOR_SHIFT) |
			   ((DMA_CFG_CH_SUSP & DMA_CFG_CH_SUSP_MASK) << DMA_CFG_CH_SUSP_SHIFT) |
			   ((DMA_CFG_FIFO_EMPTY & DMA_CFG_FIFO_EMPTY_MASK) << DMA_CFG_FIFO_EMPTY_SHIFT) |
			   ((DMA_CFG_HS_SEL_DST & DMA_CFG_HS_SEL_DST_MASK) << DMA_CFG_HS_SEL_DST_SHIFT) |
			   ((DMA_CFG_HS_SEL_SRC & DMA_CFG_HS_SEL_SRC_MASK) << DMA_CFG_HS_SEL_SRC_SHIFT) |
			   ((DMA_CFG_LOCK_CH_L & DMA_CFG_LOCK_CH_L_MASK) << DMA_CFG_LOCK_CH_L_SHIFT) |
			   ((DMA_CFG_LOCK_B_L & DMA_CFG_LOCK_B_L_MASK) << DMA_CFG_LOCK_B_L_SHIFT) |
			   ((DMA_CFG_LOCK_CH & DMA_CFG_LOCK_CH_MASK) << DMA_CFG_LOCK_CH_SHIFT) |
			   ((DMA_CFG_LOCK_B & DMA_CFG_LOCK_B_MASK) << DMA_CFG_LOCK_B_SHIFT) |
			   ((DMA_CFG_DST_HS_POL & DMA_CFG_DST_HS_POL_MASK) << DMA_CFG_DST_HS_POL_SHIFT) |
			   ((DMA_CFG_SRC_HS_POL & DMA_CFG_SRC_HS_POL_MASK) << DMA_CFG_SRC_HS_POL_SHIFT) |
			   ((DMA_CFG_MAX_ABRST & DMA_CFG_MAX_ABRST_MASK) << DMA_CFG_MAX_ABRST_SHIFT) |
			   ((DMA_CFG_RELOAD_SRC & DMA_CFG_RELOAD_SRC_MASK) << DMA_CFG_RELOAD_SRC_SHIFT) |
			   ((DMA_CFG_RELOAD_DST & DMA_CFG_RELOAD_DST_MASK) << DMA_CFG_RELOAD_DST_SHIFT));

	writel(ch_cfg_l, (DMA_CHANNEL_REG_CFG_BASE + ch_reg_multiplier));

	/* Enable the DMA channel */
	writel(((dma_channel << DMA_REG_WE_SHIFT) | dma_channel), DW_DMA_DMAC_CH_EN_REG);

	return ;
}

EXPORT_SYMBOL(dma_configure);

int fast_uart_write(unsigned int len, const char *str)
{
	unsigned int dma_len;

	while (len)
	{
		if (len > DMA_XFER_DLEN)
			dma_len = DMA_XFER_DLEN;
		else
			dma_len = len;

		while ((readl(DW_DMA_UART1_BASEADDR + UART_LSR) & LSR_TEMT) == 0) ;

		dma_configure((unsigned int) str, DW_DMA_UART1_BASEADDR + UART_THR, dma_len, DMA_CHANNEL_1);

		/* Wait for completion of DMA xfer (till channel is disabled by HW) */
		if (RETCODE_OK != dma_xfer_cmplete_chk(DMA_CHANNEL_1))
			return RETCODE_ERROR;

		len -= dma_len;

		str += dma_len;
	}

	return RETCODE_OK;
}

EXPORT_SYMBOL(fast_uart_write);

