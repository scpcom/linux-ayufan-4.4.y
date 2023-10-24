/*
 *  Copyright (c) 2010 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */

#ifndef _CONTROL_BRIDGE_H_
#define _CONTROL_BRIDGE_H_

int bridge_interface_register(unsigned char *name, unsigned short phy_port_id);
int bridge_interface_deregister(unsigned short phy_port_id);

#endif
