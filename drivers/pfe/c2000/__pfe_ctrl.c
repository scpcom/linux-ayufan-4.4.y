/*
 *  Copyright (c) 2011 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */

#include "module_ipv4.h"
#include "module_ipv6.h"
#include "module_pppoe.h"
#include "module_vlan.h"
#include "module_hidrv.h"
#include "module_tx.h"
#include "module_Rx.h"
#include "module_stat.h"
#include "module_rtp_relay.h"
#include "module_socket.h"
#include "module_qm.h"
#include "module_mc4.h"
#include "module_mc6.h"
#include "icc.h"
#include "module_ipsec.h"
#include "module_bridge.h"
#include "module_tunnel.h"
#include "module_wifi.h"

void __pfe_ctrl_cmd_handler(U16 fcode, U16 length, U16 *payload, U16 *rlen, U16 *rbuf)
{
	CmdProc cmdproc;
	U32 eventid;

	eventid = FCODE_TO_EVENT(fcode);
	cmdproc = gCmdProcTable[eventid];

	if (cmdproc)
	{
		memcpy(rbuf, payload, length);
		*rlen = (*cmdproc)(fcode, length, rbuf);
		if (*rlen == 0)
		{
			rbuf[0] = NO_ERR;
			*rlen = 2;
		}
	}
	else
	{
		rbuf[0] = ERR_UNKNOWN_COMMAND;
		*rlen = 2;
	}
}


int __pfe_ctrl_init(void)
{
	int rc;

	rc = tx_init();
	if (rc < 0)
		goto err_tx;

	rc = rx_init();
	if (rc < 0)
		goto err_rx;
	
	rc = pppoe_init();
	if (rc < 0)
		goto err_pppoe;

	rc = vlan_init();
	if (rc < 0)
		goto err_vlan;
	
	rc = ipv4_init();
	if (rc < 0)
		goto err_ipv4;

	rc = ipv6_init();
	if (rc < 0)
		goto err_ipv6;

	rc = mc4_init();
	if (rc < 0)
		goto err_mc4;

	rc = mc6_init();
	if (rc < 0)
		goto err_mc6;

	rc = statistics_init();
	if (rc < 0)
		goto err_statistics;

	rc = rtp_relay_init();
	if (rc < 0)
		goto err_rtp_relay;

	rc = socket_init();
	if (rc < 0)
		goto err_socket;

	rc = qm_init();
	if (rc < 0)
		goto err_qm;

	rc = icc_control_init();
	if (rc < 0)
		goto err_icc;

	rc = bridge_init();
	if (rc < 0)
		goto err_bridge;

	rc = ipsec_init();
	if (rc < 0)
		goto err_ipsec;

	rc = tunnel_init();
	if (rc < 0)
		goto err_tunnel;

#ifdef WIFI_ENABLE
	rc = wifi_init();
	if (rc < 0)
		goto err_wifi;
#endif
	return 0;

err_wifi:
	tunnel_exit();

err_tunnel:
	ipsec_exit();

err_ipsec:
	bridge_exit();

err_bridge:
	icc_control_exit();

err_icc:
	qm_exit();

err_qm:
	socket_exit();

err_socket:
	rtp_relay_exit();

err_rtp_relay:
	statistics_exit();
	
err_statistics:
	mc6_exit();

err_mc6:
	mc4_exit();

err_mc4:
	ipv6_exit();

err_ipv6:
	ipv4_exit();

err_ipv4:
	vlan_exit();

err_vlan:
	pppoe_exit();

err_pppoe:
	rx_exit();

err_rx:
	tx_exit();

err_tx:
	return rc;
}


void __pfe_ctrl_exit(void)
{
#ifdef WIFI_ENABLE
	wifi_exit();
#endif
	tunnel_exit();

	ipsec_exit();

	bridge_exit();

	icc_control_exit();

	qm_exit();

	socket_exit();

	rtp_relay_exit();

	statistics_exit();

	mc6_exit();

	mc4_exit();

	ipv6_exit();

	ipv4_exit();

	vlan_exit();

	pppoe_exit();

	rx_exit();

	tx_exit();
}
