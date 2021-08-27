#ifndef UDMA_API_H
#define UDMA_API_H

#include <linux/list.h>

/*There're two UDMA ports belong to APPCPU */
#define UDMA_PORT_NUM_TOTAL 2 

typedef enum {
	/** UDMA_BUFFER_INITIAL - The buffer is in initial status. 
	 * For a Tx buffer, means the data contained has not been send out;
	 * For a Rx buffer, means it's empty and there's no packet data containing
	*/
	UDMA_BUFFER_INITIAL = 0,		
	UDMA_BUFFER_TX_DONE = 1,		/* The buffer is transmitted by UDMA, Tx is done 		*/	
	UDMA_BUFFER_RX_DONE = 2,		/* The buffer already contains received data, Rx is done */
	UDMA_BUFFER_RETURNED = 3 		/* The buffer is returned by UDMA */

} udma_buffer_stat_t;

typedef enum {
	UDMA_PACKT_CONTINUE 	= 0,	/* Continuation, this buffer is part of a packet, but not the start and end */
	UDMA_PACKT_START 		= 1,	/* This buffer is starting a packet */
	UDMA_PACKT_END 			= 2,	/* This buffer is ending a packet */
	UDMA_PACKT_START_END 	= 3		/* This buffer is starting and ending of a packet, a single buffer is used for a packet */
} udma_packt_pos_t;

typedef struct {
	void * buf;						/* Address pointed to the buffer */
	unsigned int len;				/* The buffer size */
	unsigned int data_size;			/* The data size in the buffer, UDMA driver will update it to the actual data size when
									receives a packet */	
	udma_buffer_stat_t	state;		/* The buffer state */
	udma_packt_pos_t pos;			/* The buffer position inside a packet */
	void * private;					/* The private data along with this buffer, UDMA driver won't touch it */
	struct list_head list;
} udma_buffer_desc_t;

typedef enum {
        UDMA_OK            = 0x0, /**< 0x0 */
		UDMA_BUSY				, /* UDMA is busy to response */
		UDMA_ERR				, /* UDMA error */
		UDMA_FULL 				, /* The input queue is full to receive new buffers */
		UDMA_EMPTY				, /* The input queue is full to receive new buffers */
		UDMA_INVALID_PARAM		, /* Invalid param */
		UDMA_UNINITIALIZED		, /* UDMA uninitialized */
		UDMA_NO_PERM			, /* UDMA is or going to be stopped that no permision for UDMA access */
		UDMA_AGAIN				 /* Try again */		
} udma_result_t;

/**  udma_send_packet - A buffer is used to send
 * @port - udma port number, could be 0 or 1
 * @buffer_desc - parameter to describe a coming buffer
 * 
 * return 0 on success, UDMA driver took care of the buffer
 * return others for failure
*/
udma_result_t udma_send_packet( unsigned char  port, udma_buffer_desc_t *buffer_desc);

/**  udma_give_free_buffer - Give a free buffer to UDMA driver, the buffer will be used for packet receive
 * @port - udma port number, could be 0 or 1
 * @buffer_desc - parameter to describe a coming buffer
 * 
 * Note the upper layer should calls this functioin periodly to give enough free buffers to UDMA driver 
 *
 * return 0 on success, UDMA driver took care of the buffer
 * return others for failure
*/
/* It's requried that the given Rx buffer size should be no smaller than 2KB due to silicon limitation */
#define UDMA_MIN_RX_BUFFER_SIZE (2*1024) 

udma_result_t udma_give_free_buffer(unsigned char  port, udma_buffer_desc_t *buffer_desc);

/**  udma_flush - Flush the pending requests in a UDMA port and recycling all of the buffers
 * 
 * UDMA driver will flush all of the pending Tx/Rx packets, updating the buffer status and return them to upper layer.
 * The buffers may be already handled or still in a initial status
 * The upper layer is expected to recycle the return buffers
 * 
 * @port - udma port number, could be 0 or 1
 * 
 * This function is expected to be called by upper layer when exit
 *
 * return 0 on success, flush function succeed
 * return others for failure
*/
void udma_flush(unsigned char  port);

/**  udma_register_handler - register the Tx/Rx callback
 *
 * @port - udma port number, could be 0 or 1
 * @tx_handle - Tx callback. Once a buffer is send out, UDMA driver fills in UDMA_BUFFRE_TX_DONE to buffer descriptor information, and calls tx_handle
 * @rx_handle - Rx callback. Once a buffer is received, UDMA driver fills in UDMA_BUFFRE_RX_DONE and udma_packt_pos_t to buffer descriptor information, 
 			   and calls rx_handle
 *
 * Note that the context in which the tx_callback/rx_callback is called is that of a softIRQ
 *
 * 
 * At UDMA driver exit, 
 * 	it will mark the buffer descriptor as "UDMA_BUFFER_NULL", and send all of the unfinished Tx buffers to upper layer for clean by Tx_handle callback
 * 	it will mark the buffer descriptor as "UDMA_BUFFER_NULL", and send all of the received Rx free buffers to upper layer for clean by Rx_handle callback
 *
 * return 0 on success
 * return others for failure
*/
typedef void (*udma_handle_t )(unsigned char  port, udma_buffer_desc_t *buffer_desc);
udma_result_t udma_register_handler(unsigned char port, udma_handle_t tx_handle, udma_handle_t rx_handle);

#endif
