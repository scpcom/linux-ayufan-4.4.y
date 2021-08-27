/* l2fw/mv_eth_l2fw.h */

#ifndef L2FW_MV_ETH_L2FW_H
#define L2FW_MV_ETH_L2FW_H

#include "mvOs.h"
#include "mv_neta/net_dev/mv_netdev.h"

#define	L2FW_HASH_SIZE   (1 << 17)
extern int espEnabled;

struct eth_port_l2fw {
	int cmd;
	int txPort;
};

typedef struct l2fw_rule {
	MV_U32 srcIP;
	MV_U32 dstIP;
	MV_U8 port;
	struct l2fw_rule *next;
} L2FW_RULE;



void l2fw(int cmd, int rx_port, int tx_port);
void l2fw_xor(int threshold);
MV_STATUS l2fw_add(MV_U32 srcIP, MV_U32 dstIP, int port);
MV_STATUS l2fw_add_ip(const char *buf);
void l2fw_esp_show(void);
void l2fw_esp_set(int enableEsp);
void l2fw_flush(void);
void l2fw_dump(void);
void l2fw_show_numHashEntries(void);
void l2fw_stats(void);
void l2fw_mode_show(void);
void l2fw_mode(int mode);


#endif
