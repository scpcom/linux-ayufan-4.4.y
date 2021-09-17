#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include "mvOs.h"
#include "mvCommon.h"
#include "mvTypes.h"
#include "mv802_3.h"
#include "mvDebug.h"

#include "mvNeta.h"
#include "bm/mvBm.h"
#include "pnc/mvTcam.h"

static void mvEthRegPrint(MV_U32 reg_addr, char *reg_name)
{
	mvOsPrintf("  %-32s: 0x%x = 0x%08x\n", reg_name, reg_addr, MV_REG_READ(reg_addr));
}

static void mvEthRegPrint0(MV_U32 reg_addr, char *reg_name)
{
	mvOsPrintf("  %-32s: %u\n", reg_name, MV_REG_READ(reg_addr));
}

static void mvEthRegPrint2(MV_U32 reg_addr, char *reg_name, MV_U32 index)
{
	char buf[64];

	mvOsSPrintf(buf, "%s[%d]", reg_name, index);
	mvOsPrintf("  %-32s: 0x%x = 0x%08x\n", buf, reg_addr, MV_REG_READ(reg_addr));
}

static void mvEthMibPrint(int port, int mib, MV_U32 offset, char *mib_name)
{
	MV_U32 regVaLo, regValHi = 0;

	regVaLo = mvNetaMibCounterRead(port, mib, offset, &regValHi);

	if (!regValHi)
		mvOsPrintf("  %-32s: %u\n", mib_name, regVaLo);
	else
		mvOsPrintf("  t%-32s: 0x%08x%08x\n", mib_name, regValHi, regVaLo);
}

void mvEthTxpWrrRegs(int port, int txp)
{
	int queue;

	if (mvNetaTxpCheck(port, txp))
		return;

	mvOsPrintf("\n[TXP WRR/EJP registers: port=%d, txp=%d]\n", port, txp);
	mvEthRegPrint(ETH_TX_QUEUE_COMMAND_REG(port, txp), "ETH_TX_QUEUE_COMMAND_REG");

	mvEthRegPrint(NETA_TX_CMD_1_REG(port, txp),          "NETA_TX_CMD_1_REG");
	mvEthRegPrint(NETA_TX_FIXED_PRIO_CFG_REG(port, txp), "NETA_TX_FIXED_PRIO_CFG_REG");
	mvEthRegPrint(NETA_TX_REFILL_PERIOD_REG(port, txp),  "NETA_TX_REFILL_PERIOD_REG");
	mvEthRegPrint(NETA_TXP_MTU_REG(port, txp),           "NETA_TXP_MTU_REG");
	mvEthRegPrint(NETA_TXP_REFILL_REG(port, txp),        "NETA_TXP_REFILL_REG");
	mvEthRegPrint(NETA_TXP_TOKEN_SIZE_REG(port, txp),    "NETA_TXP_TOKEN_SIZE_REG");
	mvEthRegPrint(NETA_TXP_TOKEN_CNTR_REG(port, txp),    "NETA_TXP_TOKEN_CNTR_REG");
	mvEthRegPrint(NETA_TXP_EJP_HI_LO_REG(port, txp),     "NETA_TXP_EJP_HI_LO_REG");
	mvEthRegPrint(NETA_TXP_EJP_HI_ASYNC_REG(port, txp),  "NETA_TXP_EJP_HI_ASYNC_REG");
	mvEthRegPrint(NETA_TXP_EJP_LO_ASYNC_REG(port, txp),  "NETA_TXP_EJP_LO_ASYNC_REG");
	mvEthRegPrint(NETA_TXP_EJP_SPEED_REG(port, txp),     "NETA_TXP_EJP_SPEED_REG");

	for (queue = 0; queue < MV_ETH_MAX_TXQ; queue++) {
		mvOsPrintf("\n[TXQ WRR/EJP registers: port=%d, txp=%d, txq=%d]\n", port, txp, queue);
		mvEthRegPrint(NETA_TXQ_REFILL_REG(port, txp, queue), "NETA_TXQ_REFILL_REG");
		mvEthRegPrint(NETA_TXQ_TOKEN_SIZE_REG(port, txp, queue), "NETA_TXQ_TOKEN_SIZE_REG");
		mvEthRegPrint(NETA_TXQ_TOKEN_CNTR_REG(port, txp, queue), "NETA_TXQ_TOKEN_CNTR_REG");
		mvEthRegPrint(NETA_TXQ_WRR_ARBITER_REG(port, txp, queue), "NETA_TXQ_WRR_ARBITER_REG");
		if ((queue == 2) || (queue == 3))
			mvEthRegPrint(NETA_TXQ_EJP_IPG_REG(port, txp, queue), "NETA_TXQ_EJP_IPG_REG");
	}
}

void mvEthPortRegs(int port)
{
	int txp;
	MV_NETA_PORT_CTRL *pPortCtrl = mvNetaPortHndlGet(port);

	if (mvNetaPortCheck(port))
		return;

	mvEthRegPrint(ETH_PORT_STATUS_REG(port), "ETH_PORT_STATUS_REG");
	mvEthRegPrint(ETH_PORT_SERIAL_CTRL_REG(port), "ETH_PORT_SERIAL_CTRL_REG");
	mvEthRegPrint(ETH_PORT_CONFIG_REG(port), "ETH_PORT_CONFIG_REG");
	mvEthRegPrint(ETH_PORT_CONFIG_EXTEND_REG(port), "ETH_PORT_CONFIG_EXTEND_REG");
	mvEthRegPrint(ETH_SDMA_CONFIG_REG(port), "ETH_SDMA_CONFIG_REG");
	mvEthRegPrint(ETH_RX_MINIMAL_FRAME_SIZE_REG(port), "ETH_RX_MINIMAL_FRAME_SIZE_REG");
	mvEthRegPrint(ETH_INTR_CAUSE_REG(port), "ETH_INTR_CAUSE_REG");
	mvEthRegPrint(ETH_INTR_CAUSE_EXT_REG(port), "ETH_INTR_CAUSE_EXT_REG");
	mvEthRegPrint(ETH_INTR_MASK_REG(port), "ETH_INTR_MASK_REG");
	mvEthRegPrint(ETH_INTR_MASK_EXT_REG(port), "ETH_INTR_MASK_EXT_REG");

	mvEthRegPrint(ETH_RX_QUEUE_COMMAND_REG(port), "ETH_RX_QUEUE_COMMAND_REG");
	for (txp = 0; txp < pPortCtrl->txpNum; txp++)
		mvEthRegPrint(ETH_TX_QUEUE_COMMAND_REG(port, txp), "ETH_TX_QUEUE_COMMAND_REG");
}

void mvEthRegs(int port)
{
	int win;
	MV_U32	regValue;

	if (mvNetaPortCheck(port))
		return;

	mvEthRegPrint(ETH_PHY_ADDR_REG(port), "ETH_PHY_ADDR_REG");
	mvEthRegPrint(ETH_UNIT_INTR_CAUSE_REG(port), "ETH_UNIT_INTR_CAUSE_REG");
	mvEthRegPrint(ETH_UNIT_INTR_MASK_REG(port), "ETH_UNIT_INTR_MASK_REG");
	mvEthRegPrint(ETH_UNIT_ERROR_ADDR_REG(port), "ETH_UNIT_ERROR_ADDR_REG");
	mvEthRegPrint(ETH_UNIT_INT_ADDR_ERROR_REG(port), "ETH_UNIT_INT_ADDR_ERROR_REG");
	mvEthRegPrint(ETH_BASE_ADDR_ENABLE_REG(port), "ETH_BASE_ADDR_ENABLE_REG");
	mvEthRegPrint(ETH_ACCESS_PROTECT_REG(port), "ETH_ACCESS_PROTECT_REG");

	regValue = MV_REG_READ(ETH_BASE_ADDR_ENABLE_REG(port));
	for (win = 0; win < ETH_MAX_DECODE_WIN; win++) {
		if (regValue & (1 << win))
			continue;  
		mvOsPrintf("win[%d]\n", win);
		mvEthRegPrint(ETH_WIN_BASE_REG(port, win), "ETH_WIN_BASE_REG");
		mvEthRegPrint(ETH_WIN_SIZE_REG(port, win), "ETH_WIN_SIZE_REG");
	}
}

#ifdef MV_ETH_GMAC_NEW
void    mvNetaGmacRegs(int port)
{
	if (mvNetaPortCheck(port))
		return;

	if (MV_PON_PORT(port)) {
		mvOsPrintf("Not supported for PON port #%d \n", port);
		return;
	}
	mvEthRegPrint(ETH_PORT_STATUS_REG(port),        "ETH_PORT_STATUS_REG");
	mvEthRegPrint(ETH_PORT_SERIAL_CTRL_REG(port),   "ETH_PORT_SERIAL_CTRL_REG");
	mvEthRegPrint(NETA_GMAC_CTRL_0_REG(port),       "NETA_GMAC_CTRL_0_REG");
	mvEthRegPrint(NETA_GMAC_CTRL_1_REG(port),       "NETA_GMAC_CTRL_1_REG");
	mvEthRegPrint(NETA_GMAC_CTRL_2_REG(port),       "NETA_GMAC_CTRL_2_REG");
	mvEthRegPrint(NETA_GMAC_AN_CTRL_REG(port),      "NETA_GMAC_AN_CTRL_REG");
	mvEthRegPrint(NETA_GMAC_STATUS_REG(port),       "NETA_GMAC_STATUS_REG");
	mvEthRegPrint(NETA_GMAC_SERIAL_REG(port),       "NETA_GMAC_SERIAL_REG");
	mvEthRegPrint(NETA_GMAC_FIFO_PARAM_0_REG(port), "NETA_GMAC_FIFO_PARAM_0_REG");
	mvEthRegPrint(NETA_GMAC_FIFO_PARAM_1_REG(port), "NETA_GMAC_FIFO_PARAM_1_REG");
	mvEthRegPrint(NETA_GMAC_CAUSE_REG(port),        "NETA_GMAC_CAUSE_REG");
	mvEthRegPrint(NETA_GMAC_MASK_REG(port),         "NETA_GMAC_MASK_REG");
	mvEthRegPrint(NETA_GMAC_MIB_CTRL_REG(port),     "NETA_GMAC_MIB_CTRL_REG");
}
#endif  

void mvNetaRxqRegs(int port, int rxq)
{
	if (mvNetaPortCheck(port))
		return;

	if (mvNetaMaxCheck(rxq, MV_ETH_MAX_RXQ, "rxq"))
		return;

	mvOsPrintf("\n[NetA Rx: port=%d, rxq=%d]\n", port, rxq);
	mvEthRegPrint(ETH_RX_QUEUE_COMMAND_REG(port), "ETH_RX_QUEUE_COMMAND_REG");
	mvEthRegPrint(NETA_RXQ_CONFIG_REG(port, rxq), "NETA_RXQ_CONFIG_REG");
	mvEthRegPrint(NETA_RXQ_INTR_TIME_COAL_REG(port, rxq), "NETA_RXQ_INTR_TIME_COAL_REG");
	mvEthRegPrint(NETA_RXQ_BASE_ADDR_REG(port, rxq), "NETA_RXQ_BASE_ADDR_REG");
	mvEthRegPrint(NETA_RXQ_SIZE_REG(port, rxq), "NETA_RXQ_SIZE_REG");
	mvEthRegPrint(NETA_RXQ_THRESHOLD_REG(port, rxq), "NETA_RXQ_THRESHOLD_REG");
	mvEthRegPrint(NETA_RXQ_STATUS_REG(port, rxq), "NETA_RXQ_STATUS_REG");
	mvEthRegPrint(NETA_RXQ_INDEX_REG(port, rxq), "NETA_RXQ_INDEX_REG");
}

void mvNetaTxqRegs(int port, int txp, int txq)
{
	if (mvNetaTxpCheck(port, txp))
		return;

	if (mvNetaMaxCheck(txq, MV_ETH_MAX_TXQ, "txq"))
		return;

	mvOsPrintf("\n[NetA Tx: port=%d, txp=%d, txq=%d]\n", port, txp, txq);
	mvEthRegPrint(NETA_TXQ_BASE_ADDR_REG(port, txp, txq), "NETA_TXQ_BASE_ADDR_REG");
	mvEthRegPrint(NETA_TXQ_SIZE_REG(port, txp, txq), "NETA_TXQ_SIZE_REG");
	mvEthRegPrint(NETA_TXQ_STATUS_REG(port, txp, txq), "NETA_TXQ_STATUS_REG");
	mvEthRegPrint(NETA_TXQ_INDEX_REG(port, txp, txq), "NETA_TXQ_INDEX_REG");
	mvEthRegPrint(NETA_TXQ_SENT_DESC_REG(port, txp, txq), "NETA_TXQ_SENT_DESC_REG");
}

void mvNetaTxpRegs(int port, int txp)
{
	int queue;

	if (mvNetaTxpCheck(port, txp))
		return;

	mvOsPrintf("\n[NetA Tx: port=%d, txp=%d]\n", port, txp);
	mvEthRegPrint(ETH_TX_QUEUE_COMMAND_REG(port, txp), "ETH_TX_QUEUE_COMMAND_REG");
	for (queue = 0; queue < CONFIG_MV_ETH_TXQ; queue++)
		mvNetaTxqRegs(port, txp, queue);
}

#ifdef CONFIG_MV_ETH_PNC
void mvNetaPncRegs(void)
{
	mvEthRegPrint(MV_PNC_LOOP_CTRL_REG, "PNC_LOOP_CTRL_REG");
	mvEthRegPrint(MV_PNC_TCAM_CTRL_REG, "PNC_TCAM_CTRL_REG");
	mvEthRegPrint(MV_PNC_INIT_OFFS_REG, "PNC_INIT_OFFS_REG");
	mvEthRegPrint(MV_PNC_INIT_LOOKUP_REG, "PNC_INIT_LOOKUP_REG");
	mvEthRegPrint(MV_PNC_CAUSE_REG, "PNC_CAUSE_REG");
	mvEthRegPrint(MV_PNC_MASK_REG, "PNC_MASK_REG");
	mvEthRegPrint(MV_PNC_HIT_SEQ0_REG, "PNC_HIT_SEQ0_REG");
	mvEthRegPrint(MV_PNC_HIT_SEQ1_REG, "PNC_HIT_SEQ1_REG");
	mvEthRegPrint(MV_PNC_HIT_SEQ2_REG, "PNC_HIT_SEQ2_REG");
	mvEthRegPrint(MV_PNC_XBAR_RET_REG, "PNC_XBAR_RET_REG");

#ifdef MV_ETH_PNC_AGING
	{
		int     i;

		mvEthRegPrint(MV_PNC_AGING_CTRL_REG,  "PNC_AGING_CTRL_REG");
		mvEthRegPrint(MV_PNC_AGING_HI_THRESH_REG,  "PNC_AGING_HI_THRESH_REG");
		mvOsPrintf("\n");
		for (i = 0; i < MV_PNC_AGING_MAX_GROUP; i++)
			mvEthRegPrint2(MV_PNC_AGING_LO_THRESH_REG(i), "PNC_AGING_LO_THRESH_REG", i);
	}
#endif  

#ifdef MV_ETH_PNC_LB
	mvEthRegPrint(MV_PNC_LB_CRC_INIT_REG, "PNC_LB_CRC_INIT_REG");
#endif  
}
#endif  

#ifdef CONFIG_MV_ETH_PMT
void mvNetaPmtRegs(int port, int txp)
{
	int i;

	if (mvNetaTxpCheck(port, txp))
		return;

	mvOsPrintf("\n[NetA PMT registers: port=%d, txp=%d]\n", port, txp);

#ifdef MV_ETH_PMT_NEW
	mvEthRegPrint(NETA_TX_PMT_ACCESS_REG(port), "NETA_TX_PMT_ACCESS_REG");
	mvEthRegPrint(NETA_TX_PMT_FIFO_THRESH_REG(port), "NETA_TX_PMT_FIFO_THRESH_REG");
	mvEthRegPrint(NETA_TX_PMT_MTU_REG(port), "NETA_TX_PMT_MTU_REG");

	mvOsPrintf("\n");
	for (i = 0; i < NETA_TX_PMT_MAX_ETHER_TYPES; i++)
		mvEthRegPrint2(NETA_TX_PMT_ETHER_TYPE_REG(port, i), "NETA_TX_PMT_ETHER_TYPE_REG", i);

	mvOsPrintf("\n");
	mvEthRegPrint(NETA_TX_PMT_DEF_VLAN_CFG_REG(port), "NETA_TX_PMT_DEF_VLAN_CFG_REG");
	mvEthRegPrint(NETA_TX_PMT_DEF_DSA_1_CFG_REG(port), "NETA_TX_PMT_DEF_DSA_1_CFG_REG");
	mvEthRegPrint(NETA_TX_PMT_DEF_DSA_2_CFG_REG(port), "NETA_TX_PMT_DEF_DSA_2_CFG_REG");
	mvEthRegPrint(NETA_TX_PMT_DEF_DSA_SRC_DEV_REG(port), "NETA_TX_PMT_DEF_DSA_SRC_DEV_REG");

	mvEthRegPrint(NETA_TX_PMT_TTL_ZERO_CNTR_REG(port), "NETA_TX_PMT_TTL_ZERO_CNTR_REG");
	mvEthRegPrint(NETA_TX_PMT_TTL_ZERO_CNTR_REG(port), "NETA_TX_PMT_TTL_ZERO_CNTR_REG");

	mvOsPrintf("\n");
	mvEthRegPrint(NETA_TX_PMT_PPPOE_TYPE_REG(port), "NETA_TX_PMT_PPPOE_TYPE_REG");
	mvEthRegPrint(NETA_TX_PMT_PPPOE_DATA_REG(port), "NETA_TX_PMT_PPPOE_DATA_REG");
	mvEthRegPrint(NETA_TX_PMT_PPPOE_LEN_REG(port), "NETA_TX_PMT_PPPOE_LEN_REG");
	mvEthRegPrint(NETA_TX_PMT_PPPOE_PROTO_REG(port), "NETA_TX_PMT_PPPOE_PROTO_REG");
	mvOsPrintf("\n");

	mvEthRegPrint(NETA_TX_PMT_CONFIG_REG(port), "NETA_TX_PMT_CONFIG_REG");
	mvEthRegPrint(NETA_TX_PMT_STATUS_1_REG(port), "NETA_TX_PMT_STATUS_1_REG");
	mvEthRegPrint(NETA_TX_PMT_STATUS_2_REG(port), "NETA_TX_PMT_STATUS_2_REG");
#else
	for (i = 0; i < NETA_TX_MAX_MH_REGS; i++)
		mvEthRegPrint2(NETA_TX_MH_REG(port, txp, i), "NETA_TX_MH_REG", i);

	mvEthRegPrint(NETA_TX_DSA_SRC_DEV_REG(port, txp), "NETA_TX_DSA_SRC_DEV_REG");

	for (i = 0; i < NETA_TX_MAX_ETH_TYPE_REGS; i++)
		mvEthRegPrint2(NETA_TX_ETH_TYPE_REG(port, txp, i), "NETA_TX_ETH_TYPE_REG", i);
#endif  
}
#endif  

void mvNetaPortRegs(int port)
{
	int i;
	MV_NETA_PORT_CTRL *pPortCtrl = mvNetaPortHndlGet(port);

	if (mvNetaPortCheck(port))
		return;

	for (i = 0; i < NETA_MAX_CPU_REGS; i++)
		mvEthRegPrint2(NETA_CPU_MAP_REG(port, i), "NETA_CPU_MAP_REG", i);

	for (i = 0; i < MV_BM_POOLS; i++)
		mvEthRegPrint2(NETA_POOL_BUF_SIZE_REG(port, i), "NETA_POOL_BUF_SIZE_REG", i);

	mvEthRegPrint(NETA_VERSION_REG(port), "NETA_VERSION_REG");
	mvEthRegPrint(NETA_PORT_RX_RESET_REG(port), "NETA_PORT_RX_RESET_REG");
	for (i = 0; i < pPortCtrl->txpNum; i++)
		mvEthRegPrint(NETA_PORT_TX_RESET_REG(port, i), "NETA_PORT_TX_RESET_REG");
	mvEthRegPrint(NETA_BM_ADDR_REG(port), "NETA_BM_ADDR_REG");
	mvEthRegPrint(NETA_ACC_MODE_REG(port), "NETA_ACC_MODE_REG");
	mvEthRegPrint(NETA_INTR_NEW_CAUSE_REG(port), "NETA_INTR_NEW_CAUSE_REG");
	mvEthRegPrint(NETA_INTR_NEW_MASK_REG(port), "NETA_INTR_NEW_MASK_REG");
	mvEthRegPrint(NETA_INTR_MISC_CAUSE_REG(port), "NETA_INTR_MISC_CAUSE_REG");
	mvEthRegPrint(NETA_INTR_MISC_MASK_REG(port), "NETA_INTR_MISC_MASK_REG");
	mvEthRegPrint(NETA_INTR_ENABLE_REG(port), "NETA_INTR_ENABLE_REG");
}

void mvNetaPortStatus(int port)
{
	MV_ETH_PORT_STATUS	link;
	MV_NETA_PORT_CTRL 	*pPortCtrl;

	if (mvNetaPortCheck(port))
		return;

	pPortCtrl = mvNetaPortHndlGet(port);
	mvOsPrintf("\n[Link: port=%d, ctrl=%p]\n", port, pPortCtrl);
	if (!pPortCtrl)
		return;

	if (MV_PON_PORT(port)) {
		mvOsPrintf("GPON port %d link is up\n", port);
	} else {

		mvNetaLinkStatus(port, &link);

		if (link.linkup) {
			mvOsPrintf("link up");
			mvOsPrintf(", %s duplex", (link.duplex == MV_ETH_DUPLEX_FULL) ? "full" : "half");
			mvOsPrintf(", speed ");

			if (link.speed == MV_ETH_SPEED_1000)
				mvOsPrintf("1 Gbps\n");
			else if (link.speed == MV_ETH_SPEED_100)
				mvOsPrintf("100 Mbps\n");
			else
				mvOsPrintf("10 Mbps\n");

			mvOsPrintf("rxFC - %s, txFC - %s\n",
				(link.rxFc == MV_ETH_FC_DISABLE) ? "disabled" : "enabled",
				(link.txFc == MV_ETH_FC_DISABLE) ? "disabled" : "enabled");
		} else
			mvOsPrintf("link down\n");
	}
#ifndef CONFIG_MV_ETH_PNC
	MV_U32	regValue = MV_REG_READ(ETH_PORT_CONFIG_REG(port));

	mvOsPrintf("default queue: rx=%d, arp=%d, bpdu=%d, tcp=%d, udp=%d\n",
	   (regValue & ETH_DEF_RX_QUEUE_ALL_MASK) >> ETH_DEF_RX_QUEUE_OFFSET,
	   (regValue & ETH_DEF_RX_ARP_QUEUE_ALL_MASK) >> ETH_DEF_RX_ARP_QUEUE_OFFSET,
	   (regValue & ETH_DEF_RX_BPDU_QUEUE_ALL_MASK) >> ETH_DEF_RX_BPDU_QUEUE_OFFSET,
	   (regValue & ETH_DEF_RX_TCP_QUEUE_ALL_MASK) >> ETH_DEF_RX_TCP_QUEUE_OFFSET,
	   (regValue & ETH_DEF_RX_UDP_QUEUE_ALL_MASK) >> ETH_DEF_RX_UDP_QUEUE_OFFSET);
#else  
	if (!MV_NETA_PNC_CAP()) {
		MV_U32	regValue = MV_REG_READ(ETH_PORT_CONFIG_REG(port));

		mvOsPrintf("default queue: rx=%d, arp=%d, bpdu=%d, tcp=%d, udp=%d\n",
		   (regValue & ETH_DEF_RX_QUEUE_ALL_MASK) >> ETH_DEF_RX_QUEUE_OFFSET,
		   (regValue & ETH_DEF_RX_ARP_QUEUE_ALL_MASK) >> ETH_DEF_RX_ARP_QUEUE_OFFSET,
		   (regValue & ETH_DEF_RX_BPDU_QUEUE_ALL_MASK) >> ETH_DEF_RX_BPDU_QUEUE_OFFSET,
		   (regValue & ETH_DEF_RX_TCP_QUEUE_ALL_MASK) >> ETH_DEF_RX_TCP_QUEUE_OFFSET,
		   (regValue & ETH_DEF_RX_UDP_QUEUE_ALL_MASK) >> ETH_DEF_RX_UDP_QUEUE_OFFSET);
	}
#endif  
}

void mvNetaRxqShow(int port, int rxq, int mode)
{
	MV_NETA_PORT_CTRL *pPortCtrl;
	MV_NETA_QUEUE_CTRL *pQueueCtrl;

	if (mvNetaPortCheck(port))
		return;

	if (mvNetaMaxCheck(rxq, MV_ETH_MAX_RXQ, "rxq"))
		return;

	pPortCtrl = mvNetaPortHndlGet(port);
	if (!pPortCtrl)
		return;

	pQueueCtrl = &pPortCtrl->pRxQueue[rxq].queueCtrl;
	mvOsPrintf("\n[NetA RxQ: port=%d, rxq=%d]\n", port, rxq);

	if (!pQueueCtrl->pFirst) {
		mvOsPrintf("rx queue %d wasn't created\n", rxq);
		return;
	}

	mvOsPrintf("intr_coal: %d [pkts] or %d [usec]\n",
		mvNetaRxqPktsCoalGet(port, rxq), mvNetaRxqTimeCoalGet(port, rxq));

	mvOsPrintf("pFirst=%p (0x%x), numOfDescr=%d\n",
		   pQueueCtrl->pFirst,
		   (MV_U32) netaDescVirtToPhys(pQueueCtrl, (MV_U8 *) pQueueCtrl->pFirst), pQueueCtrl->lastDesc + 1);

	mvOsPrintf("nextToProc=%d (%p), rxqOccupied=%d, rxqNonOccupied=%d\n",
		   pQueueCtrl->nextToProc,
		   MV_NETA_QUEUE_DESC_PTR(pQueueCtrl, pQueueCtrl->nextToProc),
		   mvNetaRxqBusyDescNumGet(port, rxq), mvNetaRxqFreeDescNumGet(port, rxq));

	if (mode > 0) {
		int i;
		NETA_RX_DESC *pRxDesc;

		for (i = 0; i <= pQueueCtrl->lastDesc; i++) {
			pRxDesc = (NETA_RX_DESC *) MV_NETA_QUEUE_DESC_PTR(pQueueCtrl, i);

			mvOsPrintf("%3d. desc=%p, status=%08x, data=%4d, bufAddr=%08x, bufCookie=%08x\n",
				   i, pRxDesc, pRxDesc->status,
				   pRxDesc->dataSize, (MV_U32) pRxDesc->bufPhysAddr, (MV_U32) pRxDesc->bufCookie);

#if defined(MY_ABC_HERE)
			mvOsCacheLineInv(pPortCtrl->osHandle, pRxDesc);
#else  
			mvOsCacheLineInv(NULL, pRxDesc);
#endif  
		}
	}
}

void mvNetaTxqShow(int port, int txp, int txq, int mode)
{
	MV_NETA_PORT_CTRL *pPortCtrl;
	MV_NETA_TXQ_CTRL *pTxqCtrl;
	MV_NETA_QUEUE_CTRL *pQueueCtrl;

	if (mvNetaTxpCheck(port, txp))
		return;

	pPortCtrl = mvNetaPortHndlGet(port);
	if (!pPortCtrl)
		return;

	if (mvNetaMaxCheck(txq, MV_ETH_MAX_TXQ, "txq"))
		return;

	mvOsPrintf("\n[NetA TxQ: port=%d, txp=%d, txq=%d]\n", port, txp, txq);

	pTxqCtrl = mvNetaTxqHndlGet(port, txp, txq);
	pQueueCtrl = &pTxqCtrl->queueCtrl;

	if (!pQueueCtrl->pFirst) {
		mvOsPrintf("tx queue %d wasn't created\n", txq);
		return;
	}

	mvOsPrintf("pFirst=%p (0x%x), numOfDescr=%d\n",
		   pQueueCtrl->pFirst,
		   (MV_U32) netaDescVirtToPhys(pQueueCtrl, (MV_U8 *) pQueueCtrl->pFirst), pQueueCtrl->lastDesc + 1);

	mvOsPrintf("nextToProc=%d (%p), txqSent=%d, txqPending=%d\n",
		   pQueueCtrl->nextToProc,
		   MV_NETA_QUEUE_DESC_PTR(pQueueCtrl, pQueueCtrl->nextToProc),
		   mvNetaTxqSentDescNumGet(port, txp, txq), mvNetaTxqPendDescNumGet(port, txp, txq));

	if (mode > 0) {
		int i;
		NETA_TX_DESC *pTxDesc;

		for (i = 0; i <= pQueueCtrl->lastDesc; i++) {
			pTxDesc = (NETA_TX_DESC *) MV_NETA_QUEUE_DESC_PTR(pQueueCtrl, i);

			mvOsPrintf("%3d. pTxDesc=%p, cmd=%08x, data=%4d, bufAddr=%08x, gponinfo=%x\n",
				   i, pTxDesc, pTxDesc->command, pTxDesc->dataSize,
				   (MV_U32) pTxDesc->bufPhysAddr, pTxDesc->hw_cmd);

#if defined(MY_ABC_HERE)
			mvOsCacheLineInv(pPortCtrl->osHandle, pTxDesc);
#else  
			mvOsCacheLineInv(NULL, pTxDesc);
#endif  
		}
	}
}

void mvEthPortCounters(int port, int mib)
{
#ifndef MV_PON_MIB_SUPPORT
	if (MV_PON_PORT(port)) {
		mvOsPrintf("%s: not supported for PON port\n", __func__);
		return;
	}
#endif  

	if (mvNetaTxpCheck(port, mib))
		return;

	if (!mvNetaPortHndlGet(port))
		return;

	mvOsPrintf("\nMIBs: port=%d, mib=%d\n", port, mib);

#ifdef CONFIG_MV_PON
	if (MV_PON_PORT(port)) {
		int	i;

		for (i = 0; i < NETA_PON_MIB_MAX_GEM_PID; i++)
			mvEthRegPrint2(NETA_PON_MIB_RX_CTRL_REG(i), "NETA_PON_MIB_RX_CTRL_REG", i);

		mvEthRegPrint(NETA_PON_MIB_RX_DEF_REG, "NETA_PON_MIB_RX_DEF_REG");
	}
#endif  

	mvOsPrintf("\n[Rx]\n");
	mvEthMibPrint(port, mib, ETH_MIB_GOOD_FRAMES_RECEIVED, "GOOD_FRAMES_RECEIVED");
	mvEthMibPrint(port, mib, ETH_MIB_BAD_FRAMES_RECEIVED, "BAD_FRAMES_RECEIVED");
	mvEthMibPrint(port, mib, ETH_MIB_BROADCAST_FRAMES_RECEIVED, "BROADCAST_FRAMES_RECEIVED");
	mvEthMibPrint(port, mib, ETH_MIB_MULTICAST_FRAMES_RECEIVED, "MULTICAST_FRAMES_RECEIVED");
	mvEthMibPrint(port, mib, ETH_MIB_GOOD_OCTETS_RECEIVED_LOW, "GOOD_OCTETS_RECEIVED");
	mvOsPrintf("\n[Rx Errors]\n");
	mvEthMibPrint(port, mib, ETH_MIB_BAD_OCTETS_RECEIVED, "BAD_OCTETS_RECEIVED");
	mvEthMibPrint(port, mib, ETH_MIB_UNDERSIZE_RECEIVED, "UNDERSIZE_RECEIVED");
	mvEthMibPrint(port, mib, ETH_MIB_FRAGMENTS_RECEIVED, "FRAGMENTS_RECEIVED");
	mvEthMibPrint(port, mib, ETH_MIB_OVERSIZE_RECEIVED, "OVERSIZE_RECEIVED");
	mvEthMibPrint(port, mib, ETH_MIB_JABBER_RECEIVED, "JABBER_RECEIVED");
	mvEthMibPrint(port, mib, ETH_MIB_MAC_RECEIVE_ERROR, "MAC_RECEIVE_ERROR");
	mvEthMibPrint(port, mib, ETH_MIB_BAD_CRC_EVENT, "BAD_CRC_EVENT");
	mvEthRegPrint0(ETH_RX_DISCARD_PKTS_CNTR_REG(port), "RX_DISCARD_PKTS_CNTR_REG");
	mvEthRegPrint0(ETH_RX_OVERRUN_PKTS_CNTR_REG(port), "RX_OVERRUN_PKTS_CNTR_REG");
	mvOsPrintf("\n[Tx]\n");
	mvEthMibPrint(port, mib, ETH_MIB_GOOD_FRAMES_SENT, "GOOD_FRAMES_SENT");
	mvEthMibPrint(port, mib, ETH_MIB_BROADCAST_FRAMES_SENT, "BROADCAST_FRAMES_SENT");
	mvEthMibPrint(port, mib, ETH_MIB_MULTICAST_FRAMES_SENT, "MULTICAST_FRAMES_SENT");
	mvEthMibPrint(port, mib, ETH_MIB_GOOD_OCTETS_SENT_LOW, "GOOD_OCTETS_SENT");
	mvOsPrintf("\n[Tx Errors]\n");
	mvEthMibPrint(port, mib, ETH_MIB_INTERNAL_MAC_TRANSMIT_ERR, "INTERNAL_MAC_TRANSMIT_ERR");
	mvEthMibPrint(port, mib, ETH_MIB_EXCESSIVE_COLLISION, "EXCESSIVE_COLLISION");
	mvEthMibPrint(port, mib, ETH_MIB_COLLISION, "COLLISION");
	mvEthMibPrint(port, mib, ETH_MIB_LATE_COLLISION, "LATE_COLLISION");
#ifdef MV_ETH_PMT_NEW
	mvEthRegPrint0(NETA_TX_BAD_FCS_CNTR_REG(port, mib), "NETA_TX_BAD_FCS_CNTR_REG");
	mvEthRegPrint0(NETA_TX_DROP_CNTR_REG(port, mib), "NETA_TX_DROP_CNTR_REG");
#endif
	mvOsPrintf("\n[FC control]\n");
	mvEthMibPrint(port, mib, ETH_MIB_UNREC_MAC_CONTROL_RECEIVED, "UNREC_MAC_CONTROL_RECEIVED");
	mvEthMibPrint(port, mib, ETH_MIB_GOOD_FC_RECEIVED, "GOOD_FC_RECEIVED");
	mvEthMibPrint(port, mib, ETH_MIB_BAD_FC_RECEIVED, "BAD_FC_RECEIVED");
	mvEthMibPrint(port, mib, ETH_MIB_FC_SENT, "FC_SENT");
	mvOsPrintf("\n");
}

void mvEthPortRmonCounters(int port, int mib)
{
	void	*pHndl;

	if (mvNetaTxpCheck(port, mib))
		return;

	pHndl = mvNetaPortHndlGet(port);
	if (!pHndl)
		return;

	mvOsPrintf("\n[RMON]: port=%d, mib=%d\n", port, mib);

	mvEthMibPrint(port, mib, ETH_MIB_FRAMES_64_OCTETS, "0...64");
	mvEthMibPrint(port, mib, ETH_MIB_FRAMES_65_TO_127_OCTETS, "65...127");
	mvEthMibPrint(port, mib, ETH_MIB_FRAMES_128_TO_255_OCTETS, "128...255");
	mvEthMibPrint(port, mib, ETH_MIB_FRAMES_256_TO_511_OCTETS, "256...511");
	mvEthMibPrint(port, mib, ETH_MIB_FRAMES_512_TO_1023_OCTETS, "512...1023");
	mvEthMibPrint(port, mib, ETH_MIB_FRAMES_1024_TO_MAX_OCTETS, "1024...Max");
}

void mvEthPortUcastShow(int port)
{
	MV_U32 unicastReg, macL, macH;
	int i, j;

	macL = MV_REG_READ(ETH_MAC_ADDR_LOW_REG(port));
	macH = MV_REG_READ(ETH_MAC_ADDR_HIGH_REG(port));

	mvOsPrintf("\nUnicast MAC Table: port=%d %02x:%02x:%02x:%02x:%02x:%02x\n",
		   port, ((macH >> 24) & 0xff), ((macH >> 16) & 0xff),
		   ((macH >> 8) & 0xff), (macH & 0xff), ((macL >> 8) & 0xff), (macL & 0xff));

	for (i = 0; i < 4; i++) {
		unicastReg = MV_REG_READ((ETH_DA_FILTER_UCAST_BASE(port) + i * 4));
		for (j = 0; j < 4; j++) {
			MV_U8 macEntry = (unicastReg >> (8 * j)) & 0xFF;
			mvOsPrintf("%X: %8s, Q = %d\n", i * 4 + j,
				   (macEntry & BIT0) ? "accept" : "reject", (macEntry >> 1) & 0x7);
		}
	}
}

void mvEthPortMcastShow(int port)
{
	int tblIdx, regIdx;
	MV_U32 regVal;
	MV_NETA_PORT_CTRL *pPortCtrl;

	if (mvNetaPortCheck(port))
		return;

	pPortCtrl = mvNetaPortHndlGet(port);
	if (!pPortCtrl)
		return;

	mvOsPrintf("\nSpecial (IP) Multicast Table port=%d: 01:00:5E:00:00:XX\n", port);

	for (tblIdx = 0; tblIdx < (256 / 4); tblIdx++) {
		regVal = MV_REG_READ((ETH_DA_FILTER_SPEC_MCAST_BASE(port) + tblIdx * 4));
		for (regIdx = 0; regIdx < 4; regIdx++) {
			if ((regVal & (0x01 << (regIdx * 8))) != 0) {
				mvOsPrintf("0x%02X: accepted, rxQ = %d\n",
					   tblIdx * 4 + regIdx, ((regVal >> (regIdx * 8 + 1)) & 0x07));
			}
		}
	}

	mvOsPrintf("\nOther Multicast Table: port=%d\n", port);
	for (tblIdx = 0; tblIdx < (256 / 4); tblIdx++) {
		regVal = MV_REG_READ((ETH_DA_FILTER_OTH_MCAST_BASE(port) + tblIdx * 4));
		for (regIdx = 0; regIdx < 4; regIdx++) {
			if ((regVal & (0x01 << (regIdx * 8))) != 0) {
				mvOsPrintf("crc8=0x%02X: accepted, rxq = %d, ref=%d\n",
					   tblIdx * 4 + regIdx, ((regVal >> (regIdx * 8 + 1)) & 0x07),
					   pPortCtrl->mcastCount[tblIdx * 4 + regIdx]);
			}
		}
	}
}

#ifdef CONFIG_MV_ETH_HWF
void mvNetaHwfTxpCntrs(int port, int p, int txp)
{
	int txq;
	MV_U32 regVal;

	if (mvNetaPortCheck(port) || mvNetaPortCheck(p))
		return;

	if (mvNetaTxpCheck(p, txp))
		return;

	mvOsPrintf("\n[HWF Counters: port=%d]\n", port);

	for (txq = 0; txq < CONFIG_MV_ETH_TXQ; txq++) {
		mvOsPrintf("\n[HWF: hwf_txp=%d, txq=%d]\n", p + txp, txq);

		regVal = NETA_HWF_TX_PORT_MASK(p + txp) | NETA_HWF_TXQ_MASK(txq);
		MV_REG_WRITE(NETA_HWF_TX_PTR_REG(port), regVal);

		mvEthRegPrint(NETA_HWF_ACCEPTED_CNTR(port), "NETA_HWF_ACCEPTED_CNTR");
		mvEthRegPrint(NETA_HWF_YELLOW_DROP_CNTR(port), "NETA_HWF_YELLOW_DROP_CNTR");
		mvEthRegPrint(NETA_HWF_GREEN_DROP_CNTR(port), "NETA_HWF_GREEN_DROP_CNTR");
		mvEthRegPrint(NETA_HWF_THRESH_DROP_CNTR(port), "NETA_HWF_THRESH_DROP_CNTR");
	}
}

void mvNetaHwfRxpRegs(int port)
{
	int txpNum, txp;

	if (mvNetaPortCheck(port))
		return;

	mvOsPrintf("\n[HWF Config: port=%d]\n", port);
	mvEthRegPrint(NETA_HWF_RX_CTRL_REG(port), "NETA_HWF_RX_CTRL_REG");
	mvEthRegPrint(NETA_HWF_RX_THRESH_REG(port), "NETA_HWF_RX_THRESH_REG");

	txpNum = 2;

#ifdef CONFIG_MV_PON
	txpNum += MV_ETH_MAX_TCONT();
#endif  

	for (txp = 0; txp < txpNum; txp += 2)
		mvEthRegPrint2(NETA_HWF_TXP_CFG_REG(port, txp), "NETA_HWF_TXP_CFG_REG", txp);
}

void mvNetaHwfTxpRegs(int port, int p, int txp)
{
	int txq;
	MV_U32 regVal;

	if (mvNetaPortCheck(port) || mvNetaPortCheck(p))
		return;

	if (mvNetaTxpCheck(p, txp))
		return;

	for (txq = 0; txq < CONFIG_MV_ETH_TXQ; txq++) {
		mvOsPrintf("\n[HWF: hwf_txp=%d, txq=%d]\n", p + txp, txq);
		regVal = NETA_HWF_TX_PORT_MASK(p + txp) | NETA_HWF_TXQ_MASK(txq);
		MV_REG_WRITE(NETA_HWF_TX_PTR_REG(port), regVal);

		mvEthRegPrint(NETA_HWF_DROP_TH_REG(port), "NETA_HWF_DROP_TH_REG");
		mvEthRegPrint(NETA_HWF_TXQ_BASE_REG(port), "NETA_HWF_TXQ_BASE_REG");
		mvEthRegPrint(NETA_HWF_TXQ_SIZE_REG(port), "NETA_HWF_TXQ_SIZE_REG");
		mvEthRegPrint(NETA_HWF_TXQ_ENABLE_REG(port), "NETA_HWF_TXQ_ENABLE_REG");
	}
}
#endif  

void mvNetaCpuDump(int port, int cpu, int rxTx)
{
	MV_U32 regVal = MV_REG_READ(NETA_CPU_MAP_REG(port, cpu));
	int j;
	static const char  *qType[] = {"RXQ", "TXQ"};

	if (rxTx > 1 || rxTx < 0) {
		mvOsPrintf("Error - invalid queue type %d , valid values are 0 for TXQ or 1 for RXQ\n", rxTx);
		return;
	}

	if (rxTx == 1)
		regVal >>= 8;

	for (j = 0; j < CONFIG_MV_ETH_RXQ; j++) {
		if (regVal & 1)
			mvOsPrintf("%s-%d ", qType[rxTx], j);
		else
			mvOsPrintf("       ");

		regVal >>= 1;
	}
	mvOsPrintf("\n");
}

#ifdef CONFIG_MV_PON
void mvNetaPonTxpRegs(int port, int txp)
{
	int txq;

	if (mvNetaTxpCheck(port, txp))
		return;

	mvOsPrintf("\n[NetA PON TXQ Bytes registers: port=%d, txp=%d]\n", port, txp);
	for (txq = 0; txq < MV_ETH_MAX_TXQ; txq++)
		mvEthRegPrint2(NETA_TXQ_NEW_BYTES_REG(port, txp, txq), "NETA_TXQ_NEW_BYTES_REG", txq);
}
#endif  
