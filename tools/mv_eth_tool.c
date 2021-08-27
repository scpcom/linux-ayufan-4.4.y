/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

********************************************************************************
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
	    this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer in the
	documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
	used to endorse or promote products derived from this software without
	specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "mv_eth_proc.h"

extern char **environ; /* all environments */

static unsigned int port = 0, tag = 0, q = 0, weight = 0, status = 0, mac[6] = {0,};
static unsigned int policy =0, command = 0, packet = 0;
static unsigned int value = 0;
static unsigned int inport, outport, dip, sip, da[6] = {0, }, sa[6] = {0, };
static unsigned int db_type = 0;
static char* name = NULL;

void show_usage(int badarg)
{
        fprintf(stderr,
	"Usage: 										\n"
	" mv_eth_tool -h		       Display this help				\n"
	"                                                                                       \n"
	" --- Global commands ---								\n"
        " mv_eth_tool -txdone <quota>          Set threshold to start tx_done operations       	\n"
	" mv_eth_tool -reuse   <0|1>           SKB reuse support:   0 - Disable, 1 - Enable    	\n"
        " mv_eth_tool -recycle <0|1>           SKB recycle support: 0 - Disable, 1 - Enable     \n"
        "                                                                                       \n"
    " --- NetDev commands ---                                                               \n"
    " mv_eth_tool -netdev_sts <name>       Print network device status                      \n"
    " mv_eth_tool -port_add <name> <port>  Map switch port to one of network devices        \n"
    " mv_eth_tool -port_del <name> <port>  Unmap switch port from network device            \n"
    " --- vtu commands ---                                                                  \n"
    " mv_eth_tool -vtu_add <port> <tag>    Add vlan tag to port                             \n"
    " mv_eth_tool -vtu_del <port> <tag>    Remove vlan tag from port                        \n"
	" --- Port commands ---                                                                 \n"
        " mv_eth_tool -rxcoal <port> <usec>    Set RX interrupt coalescing value               	\n"
        " mv_eth_tool -txcoal <port> <usec>    Set TX interrupt coalescing value               	\n"
	" mv_eth_tool -txen   <port> <deep>    Set deep of lookup for TX enable race.          	\n"
        "                                      0 - means disable.                              	\n"
	" mv_eth_tool -ejp    <port> <0|1>     Set EJP mode: 0 - Disable, 1 - Enable           	\n"
	" mv_eth_tool -tx_noq <idx> <0|1>      Set queuing mode: 0 - Disable, 1 - Enable        \n"
	" mv_eth_tool -lro    <port> <0|1>     Set LRO mode: 0 - Disable, 1 - Enable           	\n"
	" mv_eth_tool -lro_desc <port> <value> Set LRO max concurrent sessions			\n"
	"                                                                                       \n"
	" --- Port/Queue commands ---                                                           \n"
	" mv_eth_tool -tos <port> <rxq> <tos>  Map TOS field to RX queue number                 \n"
	"                                                                                       \n"
	" mv_eth_tool -srq <port> <rxq> <bpdu|arp|tcp|udp>                                     	\n"
        "                                      Set RX queue number for different packet types. 	\n"
	"                                      rxq==8 means no special treatment.		\n"
	"		                       rxq==8 for Arp packets means drop.      		\n"
	"                                                                                       \n"
	" mv_eth_tool -sq <port> <rxq> <%%2x:%%2x:%%2x:%%2x:%%2x:%%2x> 				\n"
	"                                      Set RX queue number for a Multicast MAC         	\n"
	"                                      rxq==8 indicates delete entry.		        \n"
        " mv_eth_tool -srp <port> <WRR|FIXED>  Set the Rx Policy to WRR or Fixed 		\n"
	"											\n"
	" mv_eth_tool -St <option> <port>			       		                \n"
	"   Display different status information of the port through the kernel printk.		\n"
	"   OPTIONS:										\n"
	"   p                                   Display General port information.		\n"
        "   mac                                 Display MAC addresses information               \n"
	"   q   <0..7>                          Display specific q information.			\n"
	"   rxp                                 Display Rx Policy information.			\n"
	"   txp                                 Display Tx Policy information.			\n"
	"   cntrs                               Display the HW MIB counters			\n"
	"   regs                                Display a dump of the giga registers		\n"
	"   stats                               Display port statistics information. 		\n"
	"   tos                                 Display port TOS to RXQ mapping                 \n"
	"   switch                              Display switch statistics                 	\n"
        "\n"
        );
        exit(badarg);
}

static void parse_pt(char *src)
{
        if (!strcmp(src, "bpdu"))
		packet = PT_BPDU;
	else if(!strcmp(src, "arp"))
		packet = PT_ARP;
	else if(!strcmp(src, "tcp"))
		packet = PT_TCP;
	else if(!strcmp(src, "udp"))
		packet = PT_UDP;
	else {
		fprintf(stderr, "Illegall packet type, packet type should be bpdu/arp/tcp/udp. \n");
                exit(-1);
        }
        return;
}

static void parse_db_name(char *src)
{
	if (!strcmp(src, "routing") || !strcmp(src, "route"))
		db_type = DB_ROUTING;
	else if (!strcmp(src, "nat"))
		db_type = DB_NAT;
	else if (!strcmp(src, "fdb") || !strcmp(src, "bridge"))
		db_type = DB_FDB;
	else if (!strcmp(src, "ppp"))
		db_type = DB_PPP;
	else if (!strcmp(src, "sec"))
		db_type = DB_SEC;
	else {
		fprintf(stderr, "Illegall DB name, expected routing|nat|bridge|ppp|sec. \n");
		exit(-1);
	}
	return;
}

static void parse_port(char *src)
{
	int count;

        count = sscanf(src, "%x",&port);

        if (count != 1)  {
		fprintf(stderr, "Port parsing error: count=%d\n", count);
                exit(-1);
        }
        return;
}

static void parse_tag(char* src)
{
    int count;

    count = sscanf(src, "%d",&tag);

    if (count != 1)  {
        fprintf(stderr, "Port parsing error: count=%d\n", count);
        exit(-1);
    }
    return;
}

static void parse_q(char *src)
{
	int count;

        count = sscanf(src, "%x",&q);

        if (count != 1) {
		fprintf(stderr, "Queue parsing error: count=%d\n", count);
                exit(-1);
        }
        return;
}

static void parse_policy(char *src)
{

        if (!strcmp(src, "WRR"))
		policy = WRR;
	else if (!strcmp(src, "FIXED"))
		policy = FIXED;
	else {
		fprintf(stderr, "Illegall policy, policy can be WRR or Fixed.\n");
                exit(-1);
        }
        return;
}

static void parse_status(char *src)
{
	if (!strcmp(src, "p")) {
		status = STS_PORT;
	}
        else if (!strcmp(src, "mac")) {
                status = STS_PORT_MAC;
        }
	else if(!strcmp(src, "q")) {
		status = STS_PORT_Q;
	}
	else if(!strcmp(src, "rxp")) {
		status = STS_PORT_RXP;
	}
	else if(!strcmp(src, "txp")) {
		status = STS_PORT_TXP;
	}
        else if(!strcmp(src, "cntrs")) {
                status = STS_PORT_MIB;
        }
        else if(!strcmp(src, "regs")) {
                status = STS_PORT_REGS;
        }
	else if(!strcmp(src, "stats")) {
		status = STS_PORT_STATS;
        }
        else if(!strcmp(src, "tos")) {
                status = STS_PORT_TOS_MAP;
        }
        else if(!strcmp(src, "switch")) {
                status = STS_SWITCH_STATS;
        }
        else {
                fprintf(stderr, "Illegall status %d.\n");
                exit(-1);
        }
        return;
}
static void parse_dec_val(char *src, unsigned int* val_ptr)
{
    int i, count;

    count = sscanf(src, "%d", val_ptr);
    if(count != 1) {
        fprintf(stderr, "Illegall value - should be decimal.\n");
        exit(-1);
    }
    return;
}

static void parse_hex_val(char *src, unsigned int* hex_val_ptr)
{
    int i, count;

    count = sscanf(src, "%x", hex_val_ptr);
    if(count != 1) {
        fprintf(stderr, "Illegall value - should be hex.\n");
        exit(-1);
    }
    return;
}

static int parse_mac(char *src, unsigned int macaddr[])
{
        int count;
        int i;
        int buf[6];

        count = sscanf(src, "%2x:%2x:%2x:%2x:%2x:%2x",
                &buf[0], &buf[1], &buf[2], &buf[3], &buf[4], &buf[5]);

        if (count != 6) {
		fprintf(stderr, "MAC parsing error: Expected %%2x:%%2x:%%2x:%%2x:%%2x:%%2x.\n");
                exit(-1);
        }

        for (i = 0; i < count; i++) {
                macaddr[i] = buf[i];
        }
        return 0;
}

static int parse_ip(char *src, unsigned int* ip)
{
    int count, i;
    int buf[4];

    count = sscanf(src, "%d.%d.%d.%d",
                &buf[0], &buf[1], &buf[2], &buf[3]);

    if (count != 4) {
        fprintf(stderr, "Illegall IP address (should be %%d.%%d.%%d.%%d)\n");
        exit(-1);
    }
    *ip = (((buf[0] & 0xFF) << 24) | ((buf[1] & 0xFF) << 16) |
           ((buf[2] & 0xFF) << 8) | ((buf[3] & 0xFF) << 0));
    return 0;
}

static void parse_cmdline(int argc, char **argp)
{
	unsigned int i = 1;

	if(argc < 2) {
		show_usage(1);
	}

	if (!strcmp(argp[i], "-h")) {
		show_usage(0);
	}
	else if (!strcmp(argp[i], "-srq")) {
		command = COM_SRQ;
		i++;
		if(argc != 5)
			show_usage(1);
		parse_port(argp[i++]);
		parse_q(argp[i++]);
		parse_pt(argp[i++]);
	}
	else if (!strcmp(argp[i], "-sq")) {
		command = COM_SQ;
		i++;
		if(argc != 6)
			show_usage(1);
		parse_port(argp[i++]);
		parse_q(argp[i++]);
		parse_mac(argp[i++], mac);
	}
	else if (!strcmp(argp[i], "-srp")) {
		command = COM_SRP;
		i++;
		if(argc != 4)
			show_usage(1);
		parse_port(argp[i++]);
		parse_policy(argp[i++]);
	}
	else if (!strcmp(argp[i], "-srqw")) {
		command = COM_SRQW;
		i++;
		if(argc != 5)
			show_usage(1);
		parse_port(argp[i++]);
		parse_q(argp[i++]);
		parse_hex_val(argp[i++], &weight);
	}
	else if (!strcmp(argp[i], "-stp")) {
		command = COM_STP;
		i++;
		if(argc != 6)
			show_usage(1);
		parse_port(argp[i++]);
		parse_q(argp[i++]);
		parse_hex_val(argp[i++], &weight);
		parse_policy(argp[i++]);
	}
	else if (!strcmp(argp[i], "-fprs")) {
		command = COM_IP_RULE_SET;
		i++;
		if(argc != 8)
			show_usage(1);
		parse_dec_val(argp[i++], &inport);
		parse_dec_val(argp[i++], &outport);
		parse_ip(argp[i++], &dip);
		parse_ip(argp[i++], &sip);
		parse_mac(argp[i++], da);
		parse_mac(argp[i++], sa);
	}
	else if (!strcmp(argp[i], "-fprd")) {
		command = COM_IP_RULE_DEL;
		i++;
		if(argc != 4)
			show_usage(1);
		parse_ip(argp[i++], &dip);
		parse_ip(argp[i++], &sip);
	}
	else if (!strcmp(argp[i], "-txdone")) {
		command = COM_TXDONE_Q;
		i++;
		if(argc != 3)
			show_usage(1);
		parse_dec_val(argp[i++], &value);
	}
	else if (!strcmp(argp[i], "-txen")) {
                command = COM_TX_EN;
                i++;
		if(argc != 4)
			show_usage(1);
		parse_port(argp[i++]);
                parse_dec_val(argp[i++], &value);
        }
	else if (!strcmp(argp[i], "-lro")) {
		command = COM_LRO;
		i++;
		if(argc != 4)
			show_usage(1);
		parse_port(argp[i++]);
		parse_dec_val(argp[i++], &value);
	}
	else if (!strcmp(argp[i], "-lro_desc")) {
		command = COM_LRO_DESC;
		i++;
		if(argc != 4)
			show_usage(1);
		parse_port(argp[i++]);
		parse_dec_val(argp[i++], &value);
	}
        else if (!strcmp(argp[i], "-reuse")) {
                command = COM_SKB_REUSE;
                i++;
		if(argc != 3)
			show_usage(1);
                parse_dec_val(argp[i++], &value);
        }
        else if (!strcmp(argp[i], "-recycle")) {
                command = COM_SKB_RECYCLE;
                i++;
                if(argc != 3)
                        show_usage(1);
                parse_dec_val(argp[i++], &value);
        }
	else if (!strcmp(argp[i], "-rxcoal")) {
		command = COM_RX_COAL;
		i++;
		if(argc != 4)
			show_usage(1);
		parse_port(argp[i++]);
		parse_dec_val(argp[i++], &value);
	}
	else if (!strcmp(argp[i], "-txcoal")) {
		command = COM_TX_COAL;
		i++;
		if(argc != 4)
			show_usage(1);
		parse_port(argp[i++]);
		parse_dec_val(argp[i++], &value);
	}
        else if (!strcmp(argp[i], "-ejp")) {
                command = COM_EJP_MODE;
                i++;
                if(argc != 4)
                        show_usage(1);
                parse_port(argp[i++]);
                parse_dec_val(argp[i++], &value);
	}
        else if (!strcmp(argp[i], "-tos")) {
                command = COM_TOS_MAP;
                i++;
                if(argc != 5)
                        show_usage(1);
                parse_port(argp[i++]);
		parse_q(argp[i++]);
                parse_hex_val(argp[i++], &value);
        }
		else if (!strcmp(argp[i], "-tx_noq")) {
				command = COM_TX_NOQUEUE;
				i++;
				if(argc != 4)
						show_usage(1);
				parse_port(argp[i++]);
				parse_dec_val(argp[i++], &value);
	}
    else if (!strcmp(argp[i], "-netdev_sts")) {
        command = COM_NETDEV_STS;
        i++;
        if(argc != 3)
            show_usage(1);
        name = argp[i++];
    }
    else if (!strcmp(argp[i], "-port_add")) {
        command = COM_NETDEV_PORT_ADD;
        i++;
        if(argc != 4)
            show_usage(1);
        name = argp[i++];
        parse_dec_val(argp[i++], &port);
    }
    else if (!strcmp(argp[i], "-port_del")) {
        command = COM_NETDEV_PORT_DEL;
        i++;
        if(argc != 4)
            show_usage(1);
        name = argp[i++];
        parse_dec_val(argp[i++], &port);
    }
    else if (!strcmp(argp[i], "-vtu_add")) {
        command = COM_NETDEV_VTU_ADD;
        i++;
        if(argc != 4)
            show_usage(1);
        parse_port(argp[i++]);
		parse_tag(argp[i++]);
    }
    else if (!strcmp(argp[i], "-vtu_del")) {
        command = COM_NETDEV_VTU_DEL;
        i++;
        if(argc != 4)
            show_usage(1);
        parse_port(argp[i++]);
		parse_tag(argp[i++]);
    }
	else if (!strcmp(argp[i], "-St")) {
		command = COM_STS;
		i++;
		if(argc < 4)
			show_usage(1);
		parse_status(argp[i++]);
		if( status == STS_PORT_Q ) {
			if(argc != 5)
				show_usage(1);
			parse_q(argp[i++]);
		}
		else if(argc != 4)
			show_usage(1);

		parse_port(argp[i++]);
        }
	else {
		show_usage(i++);
	}
}

static int procit(void)
{
	FILE *mvethproc;
	mvethproc = fopen(FILE_PATH FILE_NAME, "w");
	if (!mvethproc) {
		printf ("Eror opening file %s/%s\n",FILE_PATH,FILE_NAME);
		exit(-1);
	}

	switch (command) {
		case COM_TXDONE_Q:
		case COM_SKB_REUSE:
	    case COM_SKB_RECYCLE:
			fprintf (mvethproc, ETH_CMD_STRING, ETH_PRINTF_LIST);
			break;
		case COM_RX_COAL:
		case COM_TX_COAL:
		case COM_TX_EN:
		case COM_EJP_MODE:
		case COM_TX_NOQUEUE:
		case COM_LRO:
		case COM_LRO_DESC:
			fprintf (mvethproc, PORT_CMD_STRING, PORT_PRINTF_LIST);
			break;
		case COM_TOS_MAP:
			fprintf (mvethproc, QUEUE_CMD_STRING, QUEUE_PRINTF_LIST);
			break;
		case COM_IP_RULE_SET:
			fprintf(mvethproc, IP_RULE_STRING, IP_RULE_PRINT_LIST);
			break;
		case COM_IP_RULE_DEL:
			fprintf(mvethproc, IP_RULE_DEL_STRING, IP_RULE_DEL_PRINT_LIST);
			break;
		case COM_NETDEV_PORT_ADD:
		case COM_NETDEV_PORT_DEL:
		case COM_NETDEV_STS:
			fprintf(mvethproc, NETDEV_CMD_STRING, NETDEV_PRINTF_LIST);
			break;
		case COM_NETDEV_VTU_ADD:
		case COM_NETDEV_VTU_DEL:
			fprintf(mvethproc, NETDEV_VTU_CMD_STRING, NETDEV_VTU_PRINTF_LIST);
			break;
		default:
			fprintf (mvethproc, PROC_STRING, PROC_PRINT_LIST);
			break;
	}

	fclose (mvethproc);
	return 0;
}

int main(int argc, char **argp, char **envp)
{
        parse_cmdline(argc, argp);
        return procit();
}
