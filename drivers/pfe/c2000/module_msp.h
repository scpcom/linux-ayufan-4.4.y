/*
 *  Copyright (c) 2012 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */


#ifndef _MODULE_MSP_H_
#define _MODULE_MSP_H_

#include "mtd.h"

void M_msp_set_tid(PMetadata mtd, int family);
void M_MSP_process_packet(PMetadata mtd);
void M_MSP_rx_process_packet(PMetadata mtd);

#endif /* _MODULE_MSP_H_ */
