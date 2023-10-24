/*
 *  Copyright (c) 2011 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */

#include "module_Rx.h"
#include "module_hidrv.h"
#include "module_lro.h"

static U16 M_rx_cmdproc(U16 cmd_code, U16 cmd_len, U16 *pcmd)
{
	U32  portid;
	U16 acklen;
	U16 ackstatus;
	U8 enable;
	int id;
	struct pfe_ctrl *ctrl = &pfe->ctrl;
	
	acklen = 2;
	ackstatus = CMD_OK;

	switch (cmd_code)
	{
	case CMD_RX_ENABLE:
		portid = (U8)*pcmd;
		if (portid >= GEM_PORTS) {
			ackstatus = CMD_ERR;
			break;
		}

//		M_expt_rx_enable(portid);
//		M_rx_enable(portid);
		break;

	case CMD_RX_DISABLE:
		portid = (U8)*pcmd;
		if (portid >= GEM_PORTS) {
			ackstatus = CMD_ERR;
			break;
		}

//		M_rx_disable(portid);
//		M_expt_rx_disable(portid);
		break;
	case CMD_RX_LRO:
		enable = (U8)*pcmd;
		if(enable > 1){
			ackstatus = CMD_ERR;
			break;
		}
		if (pe_sync_stop(ctrl, CLASS_MASK) < 0){
			ackstatus = CMD_ERR;
			break;
		}
		/* update the DMEM in class-pe */
        for (id = CLASS0_ID; id <= CLASS_MAX_ID; id++){
			pe_dmem_writeb(id, enable,virt_to_class_dmem(&lro_enable));
        }
        pe_start(ctrl, CLASS_MASK);
		break;
	default:
		ackstatus = CMD_ERR;
		break;
	}

	*pcmd = ackstatus;
	return acklen;
}


int rx_init(void)
{
	set_cmd_handler(EVENT_PKT_RX, M_rx_cmdproc);

	ff_enable = 1;

	return 0;
}

void rx_exit(void)
{

}
