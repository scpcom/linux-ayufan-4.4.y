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
*******************************************************************************/

#include "mvCommon.h"  /* Should be included before mvSysHwConfig */
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/pci.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/tcp.h>
#include <linux/version.h>
#include <net/ip.h>
#include <net/xfrm.h>

#include "mvOs.h"
#include "dbg-trace.h"
#include "mvSysHwConfig.h"
#include "eth/mvEth.h"
#include "eth-phy/mvEthPhy.h"
#include "boardEnv/mvBoardEnvLib.h"
#include "msApi.h"

#include "mv_netdev.h"

#ifdef CONFIG_MV_INCLUDE_SWITCH
#include "mv_switch/mv_switch.h"
#endif

/* Switch port directed towards CPU */
int SWITCH_PORT_CPU[BOARD_MAX_CASCADED_SWITCHES];

/* Inter-connect port between the two switches */
int SWITCH_PORT_IC[BOARD_MAX_CASCADED_SWITCHES];

/* Bitmask of Switch ports that should not be modified by the Gateway driver */
unsigned int SWITCH_UNUSED_PORTS[BOARD_MAX_CASCADED_SWITCHES];

/* Map logical port number (array index) to physical Switch port (array value) */
MV_BOARD_SWITCH_PORT SWITCH_PORT[BOARD_ETH_SWITCH_PORT_NUM];

/* number of Switches connected to this GbE port */
int num_connected_switches;

int use_dsa_tag = 0;

/* Bitmask of Switch ports that are currently "enabled" - used in an interface */
unsigned int switch_enabled_ports[BOARD_MAX_CASCADED_SWITCHES];

/* helpers for VLAN tag handling */
#define MV_GTW_PORT_VLAN_ID(grp,port)  ((grp)+(port)+1)
#define MV_GTW_GROUP_VLAN_ID(grp)      (((grp)+1)<<8)
#define MV_GTW_VLANID_TO_PORT(vlanid)  (((vlanid) & 0xf)-1)

#ifdef CONFIG_MV_GTW_LINK_STATUS
static int          switch_irq[BOARD_MAX_CASCADED_SWITCHES] = {-1, -1};
struct timer_list   switch_link_timer;
#endif

#ifdef CONFIG_MV_GTW_IGMP
extern int mv_gtw_igmp_snoop_init(void);
extern int mv_gtw_igmp_snoop_exit(void);
extern int mv_gtw_igmp_snoop_process(struct sk_buff* skb, unsigned char port, unsigned char vlan_dbnum);
#endif

/* Example: "mv_net_config=5,(eth0,00:99:88:88:99:77,0)(eth1,00:55:44:55:66:77,1:2:3:4),mtu=1500" */
static char *cmdline = NULL;

struct mv_gtw_config    gtw_config;

/* Map a physical Switch port to a logical port number */
static int mv_gtw_phys_switch_port_to_logical_port(int switch_index, int switch_port)
{
    int p = 0;

    for (p = 0; p < BOARD_ETH_SWITCH_PORT_NUM; p++)
    {
        if ((switch_port == SWITCH_PORT[p].qdPortNum) &&
            (switch_index == SWITCH_PORT[p].qdSwitchNum) )
            return p;
    }
    ETH_DBG(ETH_DBG_GTW, ("mv_gtw_phys_switch_port_to_logical_port failed, switch = %d, port = %d\n", switch_index, switch_port));
    return -1;
}

/* Required to get the configuration string from the Kernel Command Line */
int mv_gtw_cmdline_config(char *s);
__setup("mv_net_config=", mv_gtw_cmdline_config);

int mv_gtw_cmdline_config(char *s)
{
    cmdline = s;
    return 1;
}

static int mv_gtw_check_open_bracket(char **p_net_config)
{
    if (**p_net_config == '(') {
        (*p_net_config)++;
        return 0;
    }
    printk("Syntax error: could not find opening bracket\n");
    return -EINVAL;
}

static int mv_gtw_check_closing_bracket(char **p_net_config)
{
    if (**p_net_config == ')') {
        (*p_net_config)++;
        return 0;
    }
    printk("Syntax error: could not find closing bracket\n");
    return -EINVAL;
}

static int mv_gtw_check_comma(char **p_net_config)
{
    if (**p_net_config == ',') {
        (*p_net_config)++;
        return 0;
    }
    printk("Syntax error: could not find comma\n");
    return -EINVAL;
}

static int mv_gtw_is_digit(char ch)
{
    if(((ch >= '0') && (ch <= '9')) ||
       ((ch >= 'a') && (ch <= 'f')) ||
       ((ch >= 'A') && (ch <= 'F'))) {
        return 0;
    }

    return -1;
}

static int mv_gtw_get_cmdline_mac_addr(char **p_net_config, int idx)
{
    /* the MAC address should look like: 00:99:88:88:99:77 */
    /* that is, 6 two-digit numbers, separated by :        */
    /* 6 times two-digits, plus 5 colons, total: 17 characters */
    const int   exact_len = 17;
    int         i = 0;
    int         syntax_err = 0;
    char        *p_mac_addr = *p_net_config;

    /* check first 15 characters in groups of 3 characters at a time */
    for (i = 0; i < exact_len-2; i+=3)
    {
	    if ( (mv_gtw_is_digit(**p_net_config) == 0) &&
	         (mv_gtw_is_digit(*(*p_net_config+1)) == 0) &&
	         ((*(*p_net_config+2)) == ':') )
	    {
	        (*p_net_config) += 3;
	    }
	    else {
	        syntax_err = 1;
	        break;
	    }
    }

    /* two characters remaining, must be two digits */
    if ( (mv_gtw_is_digit(**p_net_config) == 0) &&
         (mv_gtw_is_digit(*(*p_net_config+1)) == 0) )
    {
	    (*p_net_config) += 2;
    }
    else
	    syntax_err = 1;

    if (syntax_err == 0) {
        mvMacStrToHex(p_mac_addr, gtw_config.vlan_cfg[idx].macaddr);
        return 0;
    }
    printk("Syntax error while parsing MAC address from command line\n");
    return -EINVAL;
}

static void mv_gtw_update_curr_port_mask(int number, unsigned int curr_port_mask[BOARD_MAX_CASCADED_SWITCHES], int add_del_flag)
{
	if (add_del_flag)
	    curr_port_mask[SWITCH_PORT[number].qdSwitchNum] |= (1 << SWITCH_PORT[number].qdPortNum);
	else
	    curr_port_mask[SWITCH_PORT[number].qdSwitchNum] &= ~(1 << SWITCH_PORT[number].qdPortNum);
}

static int mv_gtw_get_integer(const char *str, int *num)
{
    if (sscanf(str, "%d", num) != 1)
    {
        return -EINVAL;
    }

    if ( (*num >= 0) && (*num <=9) )
        return 1;
    else if ( (*num >= 10) && (*num <= 99) )
        return 2;
    else {
        return -EINVAL;
    }
}

static int mv_gtw_get_port_mask(char **p_net_config, int idx)
{
    /* the port mask should look like this: */
    /* example 1: 0 */
    /* example 2: 1:2:3:4 */
    /* that is, one or more one-digit numbers, separated with : */
    /* we have up to MV_GTW_MAX_NUM_OF_IFS interfaces */

    unsigned int curr_port_mask[BOARD_MAX_CASCADED_SWITCHES] = {0, 0};
    unsigned int i = 0;
    int syntax_err = 0;
    int number = 0, size = 0;

    for (i = 0; i < MV_GTW_MAX_NUM_OF_IFS; i++)
    {
	    if ((size = mv_gtw_get_integer(*p_net_config, &number)) > 0)
        {
		    mv_gtw_update_curr_port_mask(number, curr_port_mask, 1);
		    (*p_net_config) += size;  /* size of integer in characters */
        }
        else
        {
            syntax_err = 1;
            break;
        }

        if (**p_net_config == ':')
        {
		    (*p_net_config)++;
	    }
	    else if (**p_net_config == ')')
        {
		    break;
	    }
        else
        {
            syntax_err = 1;
            break;
        }
    }

    if (syntax_err == 0) {
        for (i = 0; i < BOARD_MAX_CASCADED_SWITCHES; i++)
            gtw_config.vlan_cfg[idx].ports_mask[i] = curr_port_mask[i];
        return 0;
    }
    printk("Syntax error while parsing port mask from command line\n");
    return -EINVAL;
}

static int mv_gtw_get_mtu(char **p_net_config)
{
    /* the mtu value is constructed as follows: */
    /* mtu=value                                */
    unsigned int mtu;
    int syntax_err = 0;

    if(strncmp(*p_net_config,"mtu=",4) == 0)
    {
        *p_net_config += 4;
        mtu = 0;
        while((**p_net_config >= '0') && (**p_net_config <= '9'))
        {
            mtu = (mtu * 10) + (**p_net_config - '0');
            *p_net_config += 1;
        }
        if(**p_net_config != '\0')
            syntax_err = 1;
    }
    else
    {
        syntax_err = 1;
    }

    if(syntax_err == 0)
    {
        gtw_config.mtu = mtu;
        printk("     o MTU set to %d\n", mtu);
        return 0;
    }

    printk("Syntax error while parsing mtu value from command line\n");
    return -EINVAL;
}

static int mv_gtw_get_if_num(char **p_net_config)
{
    int if_num = -1;

    /* scanning one integer parameter, the number of interfaces. Afterwards there's a comma. */
    if (sscanf(*p_net_config, "%d,", &if_num) != 1) {
        printk("Syntax error while parsing number of interfaces from command line\n");
        return -EINVAL;
    }

    if ( (if_num >= 0) && (if_num <=9) )
        (*p_net_config) += 1;
    else if ( (if_num >= 10) && (if_num <= 99) )
        (*p_net_config) += 2;
    else {
        printk("Syntax error while parsing number of interfaces from command line\n");
        return -EINVAL;
    }

    gtw_config.vlans_num = if_num;

    return 0;
}

static int mv_gtw_parse_net_config(char* cmdline)
{
    char    *p_net_config = cmdline;
    int     i = 0;
    int     status = 0;

    if (p_net_config == NULL)
	    return -EINVAL;

    /* Parse number of gateway interfaces */
    status = mv_gtw_get_if_num(&p_net_config);
    if (status != 0)
        return status;

    if (gtw_config.vlans_num == 0)
        return 1;

    if (gtw_config.vlans_num > MV_GTW_MAX_NUM_OF_IFS)
    {
        printk("Too large number of interfaces (%d) in command line: cut to %d\n",
                gtw_config.vlans_num, MV_GTW_MAX_NUM_OF_IFS);
        gtw_config.vlans_num = MV_GTW_MAX_NUM_OF_IFS;
    }

    status = mv_gtw_check_comma(&p_net_config);
    if (status != 0)
        return status;

    for (i = 0; (i < MV_GTW_MAX_NUM_OF_IFS) && (*p_net_config != '\0'); i++)
    {
        status = mv_gtw_check_open_bracket(&p_net_config);
        if (status != 0)
	        break;
        status = mv_gtw_get_cmdline_mac_addr(&p_net_config, i);
        if (status != 0)
	        break;
        status = mv_gtw_check_comma(&p_net_config);
        if (status != 0)
	        break;
        status = mv_gtw_get_port_mask(&p_net_config, i);
        if (status != 0)
	        break;
        status = mv_gtw_check_closing_bracket(&p_net_config);
        if (status != 0)
	        break;

        /* If we have a comma after the closing bracket, then interface */
        /* definition is done.                                          */
        if(*p_net_config == ',')
            break;
    }

    if(*p_net_config != '\0')
    {
        status = mv_gtw_check_comma(&p_net_config);
        if (status == 0)
        {
            status = mv_gtw_get_mtu(&p_net_config);
        }
    }
    else
    {
        gtw_config.mtu = 1500;
        printk("     o Using default MTU %d\n", gtw_config.mtu);
    }

    /* at this point, we have parsed up to MV_GTW_MAX_NUM_OF_IFS, and the mtu value */
    /* if the net_config string is not finished yet, then its format is invalid */
    if (*p_net_config != '\0')
    {
        printk("Gateway config string is too long: %s\n", p_net_config);
        status = -EINVAL;
    }
    return status;
}

static INLINE struct net_device*  mv_gtw_port_to_netdev_map(int switch_num, unsigned int switch_port)
{
    int i;
    struct mv_vlan_cfg  *vlan_cfg;

    for (i = 0; i < mv_net_devs_num; i++) {
        if (mv_net_devs[i] == NULL)
	        break;

        vlan_cfg = MV_NETDEV_VLAN(mv_net_devs[i]);
	    if (vlan_cfg != NULL) {
	        if(vlan_cfg->ports_mask[switch_num] & (1 << switch_port)) {
                return mv_net_devs[i];
            }
        }
    }
    return NULL;
}

static int mv_gtw_set_port_based_vlan(int qd_num, unsigned int ports_mask)
{
    unsigned int p, pl;
    unsigned char cnt;
    GT_LPORT port_list[MAX_SWITCH_PORTS];
    GT_QD_DEV *qd = &qd_dev[qd_num];

    ETH_DBG(ETH_DBG_GTW, ("mv_gtw_set_port_based_vlan, qd_num=%d, qd_p=%p, ports_mask=0x%X\n", qd_num, qd, ports_mask));

    for (p = 0; p < qd->numOfPorts; p++) {
        if (MV_BIT_CHECK(ports_mask, p) && (p != SWITCH_PORT_CPU[qd_num]) && !MV_BIT_CHECK(SWITCH_UNUSED_PORTS[qd_num], p)) {
	        ETH_DBG( ETH_DBG_LOAD|ETH_DBG_MCAST|ETH_DBG_VLAN, ("port based vlan, port %d: ",p));
	        for (pl = 0, cnt = 0; pl < qd->numOfPorts; pl++) {
		        if (MV_BIT_CHECK(ports_mask, pl) && (pl != p) && !MV_BIT_CHECK(SWITCH_UNUSED_PORTS[qd_num], pl)) {
		            ETH_DBG( ETH_DBG_LOAD|ETH_DBG_MCAST|ETH_DBG_VLAN, ("%d ",pl));
		            port_list[cnt] = pl;
                    cnt++;
                }
	        }
            /* we don't want to change the port based VLAN of switch 0 interconnect port because we already */
            /* connected it to all other ports in mv_switch_init() and we want to keep it that way.         */
            if ( (qd_num != SWITCH_0) || (p != SWITCH_PORT_IC[qd_num]) ) {
                if (gvlnSetPortVlanPorts(qd, p, port_list, cnt) != GT_OK) {
                    printk("gvlnSetPortVlanPorts failed\n");
                    return -1;
                }
                ETH_DBG( ETH_DBG_LOAD|ETH_DBG_MCAST|ETH_DBG_VLAN, ("\n"));
            }
        }
    }
    return 0;
}

static int mv_gtw_set_vlan_in_vtu(int qd_num, unsigned short vlan_id, short db_num, unsigned int ports_mask, int add)
{
    GT_VTU_ENTRY vtu_entry;
    unsigned int p;
    GT_QD_DEV *qd = &qd_dev[qd_num];

    vtu_entry.vid = vlan_id;
    vtu_entry.DBNum = db_num;
    vtu_entry.vidPriOverride = GT_FALSE;
    vtu_entry.vidPriority = 0;
    vtu_entry.vidExInfo.useVIDFPri = GT_FALSE;
    vtu_entry.vidExInfo.vidFPri = 0;
    vtu_entry.vidExInfo.useVIDQPri = GT_FALSE;
    vtu_entry.vidExInfo.vidQPri = 0;
    vtu_entry.vidExInfo.vidNRateLimit = GT_FALSE;
    ETH_DBG( ETH_DBG_LOAD|ETH_DBG_MCAST|ETH_DBG_VLAN, ("vtu entry: vid=0x%x, port ", vtu_entry.vid));

    ETH_DBG(ETH_DBG_GTW, ("mv_gtw_set_vlan_in_vtu: qd=%p, db=%d, vlan_id=%d, ports_mask = 0x%X\n",
                            qd, vtu_entry.DBNum, vlan_id, ports_mask));

    for (p = 0; p < qd->numOfPorts; p++) {
        if (MV_BIT_CHECK(ports_mask, p)) {
	        ETH_DBG( ETH_DBG_LOAD|ETH_DBG_MCAST|ETH_DBG_VLAN, ("%d ", p));
		    vtu_entry.vtuData.memberTagP[p] = MEMBER_EGRESS_UNMODIFIED;
	    }
	    else {
	        vtu_entry.vtuData.memberTagP[p] = NOT_A_MEMBER;
	    }
	    vtu_entry.vtuData.portStateP[p] = 0;
    }

    if (add)
    {
	    if(gvtuAddEntry(qd, &vtu_entry) != GT_OK) {
	        printk("gvtuAddEntry failed\n");
	        return -1;
	    }
    }
    else
    {
	    if(gvtuDelEntry(qd, &vtu_entry) != GT_OK) {
		    printk("gvtuDelEntry failed\n");
		    return -1;
	    }
    }

    ETH_DBG( ETH_DBG_LOAD|ETH_DBG_MCAST|ETH_DBG_VLAN, ("\n"));
    return 0;
}

int mv_gtw_set_mac_addr_to_switch(unsigned char *mac_addr, unsigned char db, unsigned int ports_mask[BOARD_MAX_CASCADED_SWITCHES], unsigned char op)
{
    GT_ATU_ENTRY mac_entry;
    struct mv_vlan_cfg *nc;
    int i = 0;

    /* validate db with VLAN id */
    nc = &gtw_config.vlan_cfg[db];
    if (MV_GTW_VLAN_TO_GROUP(nc->vlan_grp_id) != db) {
        printk("mv_gtw_set_mac_addr_to_switch (invalid db)\n");
        return -1;
    }

    memset(&mac_entry,0,sizeof(GT_ATU_ENTRY));

    mac_entry.trunkMember = GT_FALSE;
    mac_entry.prio = 0;
    mac_entry.exPrio.useMacFPri = GT_FALSE;
    mac_entry.exPrio.macFPri = 0;
    mac_entry.exPrio.macQPri = 0;
    mac_entry.DBNum = db;
    memcpy(mac_entry.macAddr.arEther, mac_addr, 6);

    if (is_multicast_ether_addr(mac_addr))
        mac_entry.entryState.mcEntryState = GT_MC_STATIC;
    else
        mac_entry.entryState.ucEntryState = (use_dsa_tag ? GT_UC_TO_CPU_STATIC : GT_UC_NO_PRI_STATIC);

    for (i = 0; i < num_connected_switches; i++) {
        mac_entry.portVec = ports_mask[i];
        /* must add IC port in case of multicast so second switch gets the frame too */
        if ((SWITCH_PORT_IC[i] >= 0) && is_multicast_ether_addr(mac_addr))
            mac_entry.portVec |= (1 << SWITCH_PORT_IC[i]);

        ETH_DBG(ETH_DBG_GTW, ("mv_gtw_set_mac_addr_to_switch: i = %d, db = %d, ports_mask = 0x%lX\n", i, db, mac_entry.portVec));
        ETH_DBG(ETH_DBG_GTW, ("%02x:%02x:%02x:%02x:%02x:%02x\n",
            mac_entry.macAddr.arEther[0],mac_entry.macAddr.arEther[1],mac_entry.macAddr.arEther[2],
            mac_entry.macAddr.arEther[3],mac_entry.macAddr.arEther[4],mac_entry.macAddr.arEther[5]));

        if ((op == 0) || (mac_entry.portVec == 0)) {
            if(gfdbDelAtuEntry(&qd_dev[i], &mac_entry) != GT_OK) {
                printk("gfdbDelAtuEntry failed\n");
                return -1;
            }
            ETH_DBG(ETH_DBG_ALL, ("deleted\n"));
        }
        else {
            if(gfdbAddMacEntry(&qd_dev[i], &mac_entry) != GT_OK) {
                printk("gfdbAddMacEntry failed\n");
                return -1;
            }
            ETH_DBG(ETH_DBG_ALL, ("added\n"));
        }
    }

    return 0;
}

#ifdef CONFIG_MV_GTW_IGMP
int mv_gtw_enable_igmp(void)
{
    unsigned char p;
    int i;

    ETH_DBG( ETH_DBG_IGMP, ("enabling L2 IGMP snooping\n"));

    for (i = 0; i < num_connected_switches; i++) {
        /* enable IGMP snoop on all ports (except cpu port) */
        for (p = 0; p < qd_dev[i].numOfPorts; p++) {
	        if ( (p != SWITCH_PORT_CPU[i]) && !MV_BIT_CHECK(SWITCH_UNUSED_PORTS[i], p) ) {
	            if(gprtSetIGMPSnoop(&qd_dev[i], p, GT_TRUE) != GT_OK) {
		            printk("gprtSetIGMPSnoop failed\n");
		            return -1;
	            }
	        }
        }
    }
    return -1;
}
#endif /* CONFIG_MV_GTW_IGMP */

/* Calculate a bitmask of unused ports in a given Switch */
/* Note: this function assumes SWITCH_PORT_CPU, SWITCH_PORT_IC and SWITCH_PORT are already initialized */
static unsigned int mv_gtw_find_unused_ports_mask(unsigned int switch_num)
{
    int i;
    unsigned int mask = 0xFFFFFFFF;

    if (switch_num >= num_connected_switches)
        return mask;

    mask &= ~(1 << SWITCH_PORT_CPU[switch_num]);
    if (SWITCH_PORT_IC[switch_num] >= 0)
        mask &= ~(1 << SWITCH_PORT_IC[switch_num]);

    for (i = 0; i < BOARD_ETH_SWITCH_PORT_NUM; i++) {
        if ((SWITCH_PORT[i].qdPortNum != -1) && (SWITCH_PORT[i].qdSwitchNum == switch_num))
            mask &= ~(1 << SWITCH_PORT[i].qdPortNum);
    }

    return mask;
}

int __init mv_gtw_net_setup(int port)
{
    struct mv_vlan_cfg  *nc;
    int    i = 0, grp = 0, p = 0;

    num_connected_switches = mvBoardNumSwitchesOnPortGet(port);
    if (num_connected_switches <= 0)
    {
        printk("mv_gtw_net_setup: no Switches connected on port %d\n", port);
        return -1;
    }

    if (num_connected_switches > 1)
        use_dsa_tag = 1;

    for (i = 0; i < BOARD_MAX_CASCADED_SWITCHES; i++)
    {
        SWITCH_PORT_CPU[i]  = mvBoardSwitchCpuPortGet(port, i);
        SWITCH_PORT_IC[i]   = mvBoardSwitchInterconnectPortGet(port, i);
        switch_enabled_ports[i] = 0;
#ifdef CONFIG_MV_GTW_LINK_STATUS
        switch_irq[i] = mvBoardLinkStatusIrqGet(port, i);
        if (switch_irq[i] >= 0)
            switch_irq[i] += IRQ_KIRKWOOD_GPIO_START;
#endif
    }

    for (p = 0; p < BOARD_ETH_SWITCH_PORT_NUM; p++)
    {
        if (mvBoardSwitchPortGet(port, p, &(SWITCH_PORT[p].qdSwitchNum),
                &(SWITCH_PORT[p].qdPortNum), SWITCH_PORT[p].portStr) != MV_OK)
        {
            printk("mvBoardSwitchPortGet failed\n");
            return -1;
        }
        ETH_DBG( ETH_DBG_LOAD, ("Logical port %d: Switch port = %d, Switch index = %d, String = %s\n",
                p, SWITCH_PORT[p].qdPortNum, SWITCH_PORT[p].qdSwitchNum, SWITCH_PORT[p].portStr));
    }

    /* Note: do this only after initializing the CPU port, IC port and other connected ports */
    for (i = 0; i < BOARD_MAX_CASCADED_SWITCHES; i++)
    {
        SWITCH_UNUSED_PORTS[i] = mv_gtw_find_unused_ports_mask(i);
        ETH_DBG( ETH_DBG_LOAD, ("UNUSED MASK [%d] = 0x%X\n", i, SWITCH_UNUSED_PORTS[i]));
    }

    /* build the net config table */
    memset(&gtw_config, 0, sizeof(struct mv_gtw_config));

    if (cmdline != NULL)
    {
	    printk("     o Using command line network interface configuration\n");
    }
    else
    {
	    printk("     o Using default network configuration, overriding boot MAC address\n");
	    cmdline = CONFIG_MV_GTW_CONFIG;
    }

	if (mv_gtw_parse_net_config(cmdline) < 0)
    {
	    printk("Error parsing mv_net_config\n");
	    return -EINVAL;
    }

    for (grp = 0, nc = &gtw_config.vlan_cfg[grp]; grp < gtw_config.vlans_num; grp++, nc++)
    {
	    /* VLAN ID */
	    nc->vlan_grp_id = MV_GTW_GROUP_VLAN_ID(grp);
        for (i = 0; i < BOARD_MAX_CASCADED_SWITCHES; i++)
	        nc->ports_link[i] = 0;

        if (use_dsa_tag)
            nc->header = nc->vlan_grp_id;
        else /* Marvell Header mode, only one Switch */
	        nc->header = cpu_to_be16((MV_GTW_VLAN_TO_GROUP(nc->vlan_grp_id) << 12) | nc->ports_mask[SWITCH_0]);

	    /* print info */
	    printk("     o mac_addr %02x:%02x:%02x:%02x:%02x:%02x, VID 0x%03x, port list: ",
                nc->macaddr[0], nc->macaddr[1], nc->macaddr[2],
		        nc->macaddr[3], nc->macaddr[4], nc->macaddr[5], nc->vlan_grp_id);

        for (p = 0; p < BOARD_ETH_SWITCH_PORT_NUM; p++)
        {
            for (i = 0; i < BOARD_MAX_CASCADED_SWITCHES; i++)
            {
                if ( (nc->ports_mask[i] & (1 << SWITCH_PORT[p].qdPortNum)) &&
                     (SWITCH_PORT[p].qdSwitchNum == i) )
                    printk("%s ", SWITCH_PORT[p].portStr);
            }
        }
        printk("\n");

        for (i = 0; i < BOARD_MAX_CASCADED_SWITCHES; i++)
            switch_enabled_ports[i] |= nc->ports_mask[i];
    }

    return 0;
}

int mv_switch_set_port_mode(int port, int switch_num)
{
    GT_QD_DEV *qd = &qd_dev[switch_num];
    int cpu_port = SWITCH_PORT_CPU[switch_num];

    /* Configure Switch port connected to the GbE port (the Switch #0 CPU port):
     * Force link up, Speed 1000 Full Duplex */
    if (gpcsSetForceSpeed(qd, cpu_port, PORT_FORCE_SPEED_1000_MBPS) != GT_OK)
    {
        printk("Force speed 1000mbps - Failed\n");
        return -1;
    }

    if (gpcsSetDpxValue (qd, cpu_port, GT_TRUE) != GT_OK ||
        gpcsSetForcedDpx(qd, cpu_port, GT_TRUE) != GT_OK)
    {
        printk("Force duplex FULL - Failed\n");
        return -1;
    }

    if (gpcsSetFCValue (qd, cpu_port, GT_TRUE) != GT_OK ||
        gpcsSetForcedFC(qd, cpu_port, GT_TRUE) != GT_OK)
    {
        printk("Force Flow Control - Failed\n");
        return -1;
    }

    if (gpcsSetLinkValue (qd, cpu_port, GT_TRUE) != GT_OK ||
        gpcsSetForcedLink(qd, cpu_port, GT_TRUE) != GT_OK)
    {
        printk("Force Link UP - Failed\n");
        return -1;
    }

    return 0;
}

int mv_gw_switch_init(int port)
{
    unsigned int        i, p;
    unsigned char       cnt;
    GT_LPORT            port_list[MAX_SWITCH_PORTS];
    struct mv_vlan_cfg  *nc;
    GT_JUMBO_MODE       jumbo_mode;
    int                 num;
    GT_QD_DEV           *qd = NULL;
    unsigned int        ports_mask;
#ifdef CONFIG_MV_GTW_LINK_STATUS
    GT_DEV_EVENT        gt_event;
#endif /* CONFIG_MV_GTW_LINK_STATUS */

    if (mv_switch_set_port_mode(port, SWITCH_0))
        return -1;

    for (num = 0; num < num_connected_switches; num++) {
        qd = &qd_dev[num];

        if (qd == NULL) {
            printk("%s: Switch %d not initialized, call mv_switch_load() first\n", __func__, num);
            return -1;
        }

        /* disable all ports */
        for (p = 0; p < qd->numOfPorts; p++) {
            if (!MV_BIT_CHECK(SWITCH_UNUSED_PORTS[num], p))
                gstpSetPortState(qd, p, GT_PORT_DISABLE);
        }

        /* set all ports not to modify the tag on egress */
        for (p = 0; p < qd->numOfPorts; p++) {
            if (!MV_BIT_CHECK(SWITCH_UNUSED_PORTS[num], p)) {
                if (gprtSetEgressMode(qd, p, GT_UNMODIFY_EGRESS) != GT_OK) {
                    printk("gprtSetEgressMode GT_UNMODIFY_EGRESS failed\n");
                    return -1;
                }
            }
        }

        if (use_dsa_tag) {  /* DSA tag mode */
            /* set all ports to work in Normal mode, except the CPU port and the */
            /* ports connecting the two switches, which are configured as DSA ports. */
            for(p = 0; p < qd->numOfPorts; p++) {
                if (!MV_BIT_CHECK(SWITCH_UNUSED_PORTS[num], p)) {
                    if ((p != SWITCH_PORT_CPU[num]) && (p != SWITCH_PORT_IC[num])) {
                        if (gprtSetFrameMode(qd, p, GT_FRAME_MODE_NORMAL) != GT_OK) {
                            printk("gprtSetFrameMode GT_FRAME_MODE_NORMAL failed\n");
                            return -1;
                        }
                    }
                    else {
                        if (gprtSetFrameMode(qd, p, GT_FRAME_MODE_DSA) != GT_OK) {
                            printk("gprtSetFrameMode GT_FRAME_MODE_DSA failed\n");
                            return -1;
                        }
                    }
                }
            }

            /* do not use Marvell Header mode */
            for(p = 0; p < qd->numOfPorts; p++) {
                if (!MV_BIT_CHECK(SWITCH_UNUSED_PORTS[num], p)) {
                    if(gprtSetHeaderMode(qd, p, GT_FALSE) != GT_OK) {
                        printk("gprtSetHeaderMode GT_FALSE failed\n");
                        return -1;
                    }
                }
            }
        }
        else /* Marvell Header mode (note: this implies that only one Switch is connected) */
        {
            /* set all ports to work in Normal mode */
            for(p = 0; p < qd->numOfPorts; p++) {
                if (!MV_BIT_CHECK(SWITCH_UNUSED_PORTS[num], p)) {
                    if (gprtSetFrameMode(qd, p, GT_FRAME_MODE_NORMAL) != GT_OK) {
                        printk("gprtSetFrameMode GT_FRAME_MODE_DSA failed\n");
                        return -1;
                    }
                }
            }

            /* use Marvell Header mode */
            if (gprtSetHeaderMode(qd, SWITCH_PORT_CPU[num], GT_TRUE) != GT_OK) {
                printk("gprtSetHeaderMode GT_TRUE failed\n");
                return -1;
            }
        }

        /* initialize Switch according to Switch ID */
        switch (qd->deviceId)
        {
            case GT_88E6161:
            case GT_88E6165:
            case GT_88E6171:
                /* flush all counters for all ports */
                if(gstatsFlushAll(qd) != GT_OK) {
                    printk("gstatsFlushAll failed\n");
                }

                if(gpvtInitialize(qd) != GT_OK) {
                    printk("gpvtInitialize failed\n");
                    return -1;
                }

                /* set device number */
                if(gsysSetDeviceNumber(qd, num) != GT_OK) {
                    printk("gsysSetDeviceNumber failed\n");
                    return -1;
                }

                /* Set 802.1Q Fallback mode for all ports */
                for(p = 0; p < qd->numOfPorts; p++) {
                    if(!MV_BIT_CHECK(SWITCH_UNUSED_PORTS[num], p)) {
                        if(gvlnSetPortVlanDot1qMode(qd, p, GT_FALLBACK) != GT_OK) {
                            printk("gvlnSetPortVlanDot1qMode failed\n");
                            return -1;
                        }
                    }
                }

                /* Setup jumbo frames mode */
                if (MV_RX_BUF_SIZE(gtw_config.mtu) <= 1522)
                    jumbo_mode = GT_JUMBO_MODE_1522;
                else if(MV_RX_BUF_SIZE(gtw_config.mtu) <= 2048)
                    jumbo_mode = GT_JUMBO_MODE_2048;
                else
                    jumbo_mode = GT_JUMBO_MODE_10240;

                for (p = 0; p < qd->numOfPorts; p++) {
                    if (gsysSetJumboMode(qd, p, jumbo_mode) != GT_OK) {
                        printk("gsysSetJumboMode %d failed\n",jumbo_mode);
                        return -1;
                    }
                }

                break;

            default:
                printk("Unsupported Switch. Switch ID is 0x%X.\n", qd->deviceId);
                return -1;
        }

        for (p = 0; p < qd->numOfPorts; p++) {
            /* default port priority to queue zero */
            if (gcosSetPortDefaultTc(qd, p, 0) != GT_OK)
                printk("gcosSetPortDefaultTc failed (port %d)\n", p);

            /* enable IP TOS Prio */
            if (gqosIpPrioMapEn(qd, p, GT_TRUE) != GT_OK)
                printk("gqosIpPrioMapEn failed (port %d)\n",p);

            /* set IP QoS */
            if (gqosSetPrioMapRule(qd, p, GT_FALSE) != GT_OK)
                printk("gqosSetPrioMapRule failed (port %d)\n", p);

            /* disable Vlan QoS Prio */
            if (gqosUserPrioMapEn(qd, p, GT_FALSE) != GT_OK)
                printk("gqosUserPrioMapEn failed (port %d)\n", p);

            if ((p != SWITCH_PORT_CPU[num]) &&
                (p != SWITCH_PORT_IC[num]) &&
                !MV_BIT_CHECK(SWITCH_UNUSED_PORTS[num], p) )
            {
                if( ( gpcsSetFCValue(qd, p, GT_FALSE) != GT_OK) ||
                    ( gpcsSetForcedFC(qd, p, GT_FALSE) != GT_OK) )
                    printk("Force Flow Control - Failed\n");

#if 0
                /* TODO - decide if we want to enable auto-negotiation of Flow Control for external ports */
                if (gprtSetPause(qd, p, GT_PHY_PAUSE) != GT_OK)
                    printk("gprtSetPause failed (Switch %d port %d)\n", num, p);
#endif
            }
        }

        /* The switch CPU port is not part of the VLAN, but rather connected by tunneling to each */
        /* of the VLAN's ports. Our MAC addr will be added during start operation to the VLAN DB  */
        /* at switch level to forward packets with this DA to CPU port.                           */
        ETH_DBG( ETH_DBG_LOAD, ("Enabling Tunneling on ports: "));
        for (p = 0; p < qd->numOfPorts; p++) {
            if( (p != SWITCH_PORT_CPU[num]) && !MV_BIT_CHECK(SWITCH_UNUSED_PORTS[num], p) ) {
                if(gprtSetVlanTunnel(qd, p, GT_TRUE) != GT_OK) {
                    printk("gprtSetVlanTunnel failed (port %d)\n", p);
                    return -1;
                }
                else {
                    ETH_DBG( ETH_DBG_LOAD, ("%d ", p));
                }
            }
        }
        ETH_DBG( ETH_DBG_LOAD, ("\n"));

        /* configure ports (excluding CPU port) for each network interface (VLAN): */
        for (i = 0, nc=&gtw_config.vlan_cfg[i]; i < gtw_config.vlans_num; i++, nc++) {
            ETH_DBG( ETH_DBG_LOAD, ("vlan%d configuration (nc->ports_mask = 0x%08x) \n", i, nc->ports_mask));
            /* set port's default private vlan id and database number (DB per group): */
            for (p = 0; p < qd->numOfPorts; p++) {
                if( MV_BIT_CHECK(nc->ports_mask[num], p)    &&
                    (p != SWITCH_PORT_CPU[num])             &&
                    !MV_BIT_CHECK(SWITCH_UNUSED_PORTS[num], p) ) {
                    if (gvlnSetPortVid(qd, p, nc->vlan_grp_id) != GT_OK ) {
                        printk("gvlnSetPortVid failed");
                        return -1;
                    }
                    if (gvlnSetPortVlanDBNum(qd, p, MV_GTW_VLAN_TO_GROUP(nc->vlan_grp_id)) != GT_OK) {
                        printk("gvlnSetPortVlanDBNum failed\n");
                        return -1;
                    }
                }
            }

            ports_mask = nc->ports_mask[num];
            if (SWITCH_PORT_IC[num] >= 0)
                ports_mask |= (1 << SWITCH_PORT_IC[num]);
            if (num == SWITCH_0)
                ports_mask &= ~(1 << SWITCH_PORT_CPU[num]);
            /* set port's port-based vlan (CPU port is not part of VLAN) */
            if (mv_gtw_set_port_based_vlan(num, ports_mask) != 0)
                printk("mv_gtw_set_port_based_vlan failed\n");

            /* set vtu with group vlan id (used in tx) */
            ports_mask = (nc->ports_mask[num] | (1 << SWITCH_PORT_CPU[num]));
            if (SWITCH_PORT_IC[num] >= 0)
                ports_mask |= (1 << SWITCH_PORT_IC[num]);
            if (mv_gtw_set_vlan_in_vtu(num, nc->vlan_grp_id, MV_GTW_VLAN_TO_GROUP(nc->vlan_grp_id), ports_mask, 1) != 0) {
                printk("mv_gtw_set_vlan_in_vtu failed\n");
            }

            if (!use_dsa_tag && num == SWITCH_0) /* Marvell Header mode */
            {
                /* set vtu with each port private vlan id (used in rx) */
                for (p = 0; p < qd->numOfPorts; p++) {
                    if (MV_BIT_CHECK(nc->ports_mask[num], p) && (p != SWITCH_PORT_CPU[num])) {
                        if (mv_gtw_set_vlan_in_vtu(num, MV_GTW_PORT_VLAN_ID(
                            nc->vlan_grp_id, p), MV_GTW_VLAN_TO_GROUP(nc->vlan_grp_id),
                           (nc->ports_mask[num] & ~(1 << SWITCH_PORT_CPU[num])), 1) != 0) {
                                printk("mv_gtw_set_vlan_in_vtu failed\n");
                        }
                    }
                }
            }
        }

        /* set cpu-port with port-based vlan to all other ports */
        ETH_DBG( ETH_DBG_LOAD, ("cpu port-based vlan:"));
        for (p = 0,cnt = 0; p < qd->numOfPorts; p++) {
            if (p != SWITCH_PORT_CPU[num] && !MV_BIT_CHECK(SWITCH_UNUSED_PORTS[num], p) ) {
                ETH_DBG( ETH_DBG_LOAD, ("%d ",p));
                port_list[cnt] = p;
                cnt++;
            }
        }
        ETH_DBG( ETH_DBG_LOAD, ("\n"));
        if ( gvlnSetPortVlanPorts(qd, SWITCH_PORT_CPU[num], port_list, cnt) != GT_OK) {
            printk("gvlnSetPortVlanPorts failed\n");
            return -1;
        }

        if ( (use_dsa_tag) && (num == SWITCH_0) )
        {
            /* set switch #0 inter-connect port with port-based vlan to             */
            /* other ports in switch #0 which can be part of a cross-switch vlan.   */
            for (p = 0, cnt = 0; p < qd->numOfPorts; p++) {
                if ((p != SWITCH_PORT_IC[num])  &&
                    (p != SWITCH_PORT_CPU[num]) &&
                    !MV_BIT_CHECK(SWITCH_UNUSED_PORTS[num], p) ) {
                    port_list[cnt] = p;
                    cnt++;
                }
            }
            if( gvlnSetPortVlanPorts(qd, SWITCH_PORT_IC[num], port_list, cnt) != GT_OK) {
                printk("gvlnSetPortVlanPorts failed\n");
                return -1;
            }
        }

        if(gfdbFlush(qd,GT_FLUSH_ALL) != GT_OK) {
            printk("gfdbFlush failed\n");
        }

#ifdef CONFIG_MV_GTW_LINK_STATUS
        /* Enable Phy Link Status Changed interrupt at Phy level for the all enabled ports */
        for (p = 0; p < qd->numOfPorts; p++) {
            if ((p != SWITCH_PORT_IC[num])  &&
                (p != SWITCH_PORT_CPU[num]) &&
                !MV_BIT_CHECK(SWITCH_UNUSED_PORTS[num], p) ) {
                if(gprtPhyIntEnable(qd, p, (GT_LINK_STATUS_CHANGED)) != GT_OK) {
                    printk("gprtPhyIntEnable failed port %d\n", p);
                }
            }
        }

        if ((qd->deviceId != GT_88E6161) &&
            (qd->deviceId != GT_88E6165) &&
            (qd->deviceId != GT_88E6171)) {
            if (switch_irq[num] != -1) {
                if(eventSetActive(qd, GT_PHY_INTERRUPT) != GT_OK) {
                    printk("eventSetActive failed\n");
                }
            }
        }
        else {
            gt_event.event = GT_DEV_INT_PHY;
            gt_event.portList = 0;
            gt_event.phyList = 0x7FF & ~(SWITCH_UNUSED_PORTS[num] | (1 << SWITCH_PORT_CPU[num]));   /* 0x7FF is a bit mask for ports 0 - 10 */
            if (SWITCH_PORT_IC[num] >= 0)
                gt_event.phyList &= ~(1 << SWITCH_PORT_IC[num]);

            if (switch_irq[num] != -1) {
                if(eventSetDevInt(qd, &gt_event) != GT_OK) {
                    printk("eventSetDevInt failed\n");
                }
                if(eventSetActive(qd, GT_DEVICE_INT) != GT_OK) {
                    printk("eventSetActive failed\n");
                }
            }
        }
#endif /* CONFIG_MV_GTW_LINK_STATUS */

        /* Configure Ethernet related LEDs, currently according to Switch ID */
        switch (qd->deviceId) {
            case GT_88E6161:
            case GT_88E6165:
            case GT_88E6171:
                break; /* do nothing */

            default:
                printk("LED Configuration - Unsupported Switch. Switch ID is 0x%X.\n", qd->deviceId);
                break;
        }

        /* done! enable all Switch ports according to the net config table */
        ETH_DBG( ETH_DBG_LOAD, ("enabling: ports "));
        for (p = 0; p < qd->numOfPorts; p++) {
            if (!MV_BIT_CHECK(SWITCH_UNUSED_PORTS[num], p)) {
                if(gstpSetPortState(qd, p, GT_PORT_FORWARDING) != GT_OK) {
                    printk("gstpSetPortState failed\n");
                }
            }
        }
        ETH_DBG( ETH_DBG_LOAD, ("\n"));
    }

    return 0;
}

int mv_gtw_switch_tos_get(int port, unsigned char tos)
{
    unsigned char   queue;
    int             rc;

    /* should be the same for both switches, so we just return what's configured in switch #0 */
    rc = gcosGetDscp2Tc(&qd_dev[SWITCH_0], (tos >> 2), &queue);
    if (rc)
        return -1;

    return (int)queue;
}

int mv_gtw_switch_tos_set(int port, unsigned char tos, int queue)
{
    int num;
    GT_STATUS status = 0;

    for (num = 0; num < num_connected_switches; num++)
    {
        status |= gcosSetDscp2Tc(&qd_dev[num], (tos >> 2), (unsigned char)queue);
    }
    return status;
}

static struct net_device* mv_gtw_main_net_dev_get(void)
{
    int                 i;
    mv_eth_priv         *priv;
    struct net_device   *dev;

    for (i = 0; i < mv_net_devs_num; i++) {
        dev = mv_net_devs[i];
        priv = MV_ETH_PRIV(dev);

        if (netif_running(dev) && priv->isGtw)
            return dev;
    }
    return NULL;
}

int mv_gtw_set_mac_addr( struct net_device *dev, void *p )
{
    struct mv_vlan_cfg *vlan_cfg = MV_NETDEV_VLAN(dev);
    struct sockaddr *addr = p;
    unsigned int port_mask[BOARD_MAX_CASCADED_SWITCHES];

    if (!is_valid_ether_addr(addr->sa_data))
	    return -EADDRNOTAVAIL;

    port_mask[SWITCH_0] = (1 << SWITCH_PORT_CPU[SWITCH_0]);
    port_mask[SWITCH_1] = (1 << SWITCH_PORT_CPU[SWITCH_1]);

    /* remove old mac addr from VLAN DB */
    mv_gtw_set_mac_addr_to_switch(dev->dev_addr, MV_GTW_VLAN_TO_GROUP(vlan_cfg->vlan_grp_id), port_mask, 0);

    memcpy(dev->dev_addr, addr->sa_data, 6);

    /* add new mac addr to VLAN DB */
    mv_gtw_set_mac_addr_to_switch(dev->dev_addr, MV_GTW_VLAN_TO_GROUP(vlan_cfg->vlan_grp_id), port_mask, 1);

    printk("mv_gateway: %s change mac address to %02x:%02x:%02x:%02x:%02x:%02x\n",
        dev->name, *(dev->dev_addr), *(dev->dev_addr+1), *(dev->dev_addr+2),
       *(dev->dev_addr+3), *(dev->dev_addr+4), *(dev->dev_addr+5));

    return 0;
}

int     mv_gtw_all_multicasts_del(int db_num)
{
    GT_STATUS status = GT_OK;
    GT_ATU_ENTRY  atu_entry;
    int i;
    GT_U8 mc_mac[] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    GT_U8 bc_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    for (i = 0; i < num_connected_switches; i++)
    {
        memcpy(atu_entry.macAddr.arEther, &mc_mac, 6);
        atu_entry.DBNum = db_num;

        while ((status = gfdbGetAtuEntryNext(&qd_dev[i], &atu_entry)) == GT_OK) {

            /* we don't want to delete the broadcast entry which is the last one */
            if (memcmp(atu_entry.macAddr.arEther, &bc_mac, 6) == 0)
                break;

            ETH_DBG(ETH_DBG_GTW, ("Deleting ATU Entry: qd = %d, db = %d, MAC = %02X:%02X:%02X:%02X:%02X:%02X\n",
		        i, atu_entry.DBNum, atu_entry.macAddr.arEther[0], atu_entry.macAddr.arEther[1],
		        atu_entry.macAddr.arEther[2], atu_entry.macAddr.arEther[3],
		        atu_entry.macAddr.arEther[4], atu_entry.macAddr.arEther[5]));

            if (gfdbDelAtuEntry(&qd_dev[i], &atu_entry) != GT_OK) {
	            printk("gfdbDelAtuEntry failed\n");
	            return -1;
            }
	        memcpy(atu_entry.macAddr.arEther, &mc_mac, 6);
	        atu_entry.DBNum = db_num;
        }
    }

    return 0;
}

int     mv_gtw_promisc_set(int switch_num, struct mv_vlan_cfg *vlan_cfg, int promisc_on)
{
    unsigned int ports_mask, p;
    unsigned char cnt;
    GT_LPORT port_list[MAX_SWITCH_PORTS];

    if (promisc_on)
    {
        ports_mask = vlan_cfg->ports_mask[switch_num];
        if (SWITCH_PORT_IC[switch_num] >= 0)
            ports_mask |= (1 << SWITCH_PORT_IC[switch_num]);

        ports_mask |= (1 << SWITCH_PORT_CPU[switch_num]); /* promiscuous, so connect CPU port */

        mv_gtw_set_port_based_vlan(switch_num, ports_mask);

        /* TODO: issue - when setting any interface to be promiscuous, all Switch #1 ports become "promiscuous" because */
        /* they are connected by port based VLAN to IC[1] port, amd here we connect IC[0] port to CPU[0] port */
        if (use_dsa_tag && (switch_num == SWITCH_0))
        {
            /* set switch #0 inter-connect port with port-based vlan to	*/
            /* other ports including CPU port.                          */
            for (p = 0, cnt = 0; p < qd_dev[switch_num].numOfPorts; p++) {
                if ( (p != SWITCH_PORT_IC[switch_num]) && !MV_BIT_CHECK(SWITCH_UNUSED_PORTS[switch_num], p)) {
                    port_list[cnt] = p;
                    cnt++;
                }
            }
            if (gvlnSetPortVlanPorts(&qd_dev[switch_num], SWITCH_PORT_IC[switch_num], port_list, cnt) != GT_OK) {
                printk("gvlnSetPortVlanPorts failed\n");
            }
        }
        if ( (!use_dsa_tag) && (switch_num == SWITCH_0) ) /* Marvell Header mode */
        {
            for (p = 0; p < qd_dev[switch_num].numOfPorts; p++)
            {
                if (MV_BIT_CHECK(vlan_cfg->ports_mask[switch_num], p) && (p != SWITCH_PORT_CPU[switch_num]))
                {
                    if (mv_gtw_set_vlan_in_vtu(switch_num, MV_GTW_PORT_VLAN_ID(vlan_cfg->vlan_grp_id, p), MV_GTW_VLAN_TO_GROUP(vlan_cfg->vlan_grp_id),
                                      (vlan_cfg->ports_mask[switch_num] | (1 << SWITCH_PORT_CPU[switch_num])), 1) != 0)
                    {
		                printk("mv_gtw_set_vlan_in_vtu failed\n");
		            }
	            }
	        }
        }
    }
    else
    {
        ports_mask = vlan_cfg->ports_mask[switch_num];
        if (SWITCH_PORT_IC[switch_num] >= 0)
            ports_mask |= (1 << SWITCH_PORT_IC[switch_num]);
        if (switch_num == SWITCH_0)
            ports_mask &= ~(1 << SWITCH_PORT_CPU[switch_num]);

        mv_gtw_set_port_based_vlan(switch_num, ports_mask);

        if (use_dsa_tag && (switch_num == SWITCH_0))
        {
            /* set switch #0 inter-connect port with port-based vlan to		*/
            /* other ports in switch #0 which can be part of a cross-switch vlan.	*/
            for (p = 0, cnt = 0; p < qd_dev[switch_num].numOfPorts; p++) {
                if ((p != SWITCH_PORT_IC[switch_num]) &&
                    (p != SWITCH_PORT_CPU[switch_num]) &&
                    !MV_BIT_CHECK(SWITCH_UNUSED_PORTS[switch_num], p) ) {
                    port_list[cnt] = p;
                    cnt++;
                }
            }
            if (gvlnSetPortVlanPorts(&qd_dev[switch_num], SWITCH_PORT_IC[switch_num], port_list, cnt) != GT_OK) {
                printk("gvlnSetPortVlanPorts failed\n");
            }
        }
        if ( (!use_dsa_tag) && (switch_num == SWITCH_0) ) /* Marvell Header mode */
        {
            for (p = 0; p < qd_dev[switch_num].numOfPorts; p++)
            {
                if (MV_BIT_CHECK(vlan_cfg->ports_mask[switch_num], p) && (p != SWITCH_PORT_CPU[switch_num]))
                {
		            if (mv_gtw_set_vlan_in_vtu(switch_num, MV_GTW_PORT_VLAN_ID(vlan_cfg->vlan_grp_id, p), MV_GTW_VLAN_TO_GROUP(vlan_cfg->vlan_grp_id),
                                      (vlan_cfg->ports_mask[switch_num] & ~(1 << SWITCH_PORT_CPU[switch_num])), 1) != 0)
                    {
	                printk("mv_gtw_set_vlan_in_vtu failed\n");
	            }
	            }
            }
        }
    }

    return 0;
}

void    mv_gtw_set_multicast_list(struct net_device *dev)
{
    struct mv_vlan_cfg *vlan_cfg = MV_NETDEV_VLAN(dev);
    int i, num;
    unsigned int mac_ports_mask[BOARD_MAX_CASCADED_SWITCHES];

    mac_ports_mask[SWITCH_0] = ((1 << SWITCH_PORT_CPU[SWITCH_0]) | vlan_cfg->ports_mask[SWITCH_0]);
    mac_ports_mask[SWITCH_1] = ((1 << SWITCH_PORT_CPU[SWITCH_1]) | vlan_cfg->ports_mask[SWITCH_1]);

    for (num = 0; num < num_connected_switches; num++) {
        if (dev->flags & IFF_PROMISC) {
            /* promiscuous mode - connect the CPU port to the VLAN (port based + 802.1q) */
            if (mv_gtw_promisc_set(num, vlan_cfg, 1))
                printk("mv_gtw_promisc_set to 1 failed\n");
        } else  {
            /* not in promiscuous or allmulti mode - disconnect the CPU port to the VLAN (port based + 802.1q) */
            if (mv_gtw_promisc_set(num, vlan_cfg, 0))
                printk("mv_gtw_promisc_set to 0 failed\n");
        }
    }

    if (dev->flags & IFF_ALLMULTI)
        printk("mv_gtw_set_multicast_list: setting all-multicast mode is not supported\n");

    /* this part deals with both (all) Switches anyway, so no need to loop on Switches here */
    if (!(dev->flags & IFF_PROMISC) && !(dev->flags & IFF_ALLMULTI)) {
        /* Add or delete specific multicast addresses:					*/
        /* Linux provides a list of the current multicast addresses for the device.		*/
        /* First, we delete all the multicast addresses in the ATU.				*/
        /* Then we add the specific multicast addresses Linux provides.			*/
        if (mv_gtw_all_multicasts_del(MV_GTW_VLAN_TO_GROUP(vlan_cfg->vlan_grp_id)))
            printk("mv_gtw_all_multicasts_del failed\n");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 34)
        if (!netdev_mc_empty(dev)) {
            struct netdev_hw_addr *ha;
            netdev_for_each_mc_addr(ha, dev) {
                mv_gtw_set_mac_addr_to_switch(ha->addr,
                                MV_GTW_VLAN_TO_GROUP(vlan_cfg->vlan_grp_id),
                                mac_ports_mask, 1);
            }
        }
#else
        if (dev->mc_count) {
            struct dev_mc_list *curr_addr = dev->mc_list;
            for (i = 0; i < dev->mc_count; i++, curr_addr = curr_addr->next) {
                if (!curr_addr)
                    break;
                mv_gtw_set_mac_addr_to_switch(curr_addr->dmi_addr,
                                MV_GTW_VLAN_TO_GROUP(vlan_cfg->vlan_grp_id),
                                mac_ports_mask, 1);
            }
        }
#endif /* KERNEL_VERSION >= 2.6.34 */
    }
}

int mv_gtw_change_mtu(struct net_device *dev, int mtu)
{
    GT_JUMBO_MODE jumbo_mode;
    int i, num;
    mv_eth_priv *priv = MV_ETH_PRIV(dev);

    if (netif_running(dev)) {
        printk("mv_gateway does not support changing MTU for active interface.\n");
        return -EPERM;
    }

    /* check mtu - can't change mtu if there is a gateway interface that */
    /* is currently up and has a different mtu */
    for (i = 0; i < mv_net_devs_num; i++) {
        if ((MV_ETH_PRIV(mv_net_devs[i]) == priv) &&
            (mv_net_devs[i]->mtu != mtu) &&
            (netif_running(mv_net_devs[i])) )
        {
            printk(KERN_ERR "All gateway devices must have same MTU\n");
            return -EPERM;
        }
    }

    if (dev->mtu != mtu) {
        int old_mtu = dev->mtu;

        /* stop tx/rx activity, mask all interrupts, relese skb in rings,*/
        mv_eth_stop_internals(priv);

        if (mv_eth_change_mtu_internals(dev, mtu) == -1)
            return -EPERM;

        /* fill rx buffers, start rx/tx activity, set coalescing */
        if(mv_eth_start_internals(priv, dev->mtu) != 0 ) {
            printk( KERN_ERR "%s: start internals failed\n", dev->name );
            return -EPERM;
        }

        printk( KERN_NOTICE "%s: change mtu %d (buffer-size %d) to %d (buffer-size %d)\n",
            dev->name, old_mtu, MV_RX_BUF_SIZE(old_mtu),
            dev->mtu, MV_RX_BUF_SIZE(dev->mtu) );
    }

    if (gtw_config.mtu != mtu) {
        /* Setup jumbo frames mode */
        if (MV_RX_BUF_SIZE(mtu) <= 1522)
            jumbo_mode = GT_JUMBO_MODE_1522;
        else if (MV_RX_BUF_SIZE(mtu) <= 2048)
            jumbo_mode = GT_JUMBO_MODE_2048;
        else
            jumbo_mode = GT_JUMBO_MODE_10240;

        for (num = 0; num < num_connected_switches; num++) {
            for (i = 0; i < qd_dev[num].numOfPorts; i++) {
                if (!MV_BIT_CHECK(SWITCH_UNUSED_PORTS[num], i)) {
                    if (gsysSetJumboMode(&qd_dev[num], i, jumbo_mode) != GT_OK) {
                        printk("gsysSetJumboMode %d failed for port %d\n", jumbo_mode, i);
                    }
                }
            }
        }
        gtw_config.mtu = mtu;
    }
	return 0;
}

#ifdef CONFIG_MV_GTW_LINK_STATUS
static void mv_gtw_update_link_status(int qd_num, struct net_device* dev, unsigned int p)
{
    struct mv_vlan_cfg  *vlan_cfg = MV_NETDEV_VLAN(dev);
    GT_BOOL             link;
    unsigned int        prev_ports_link = 0, new_ports_link = 0;
    int i;

    if (gprtGetLinkState(&qd_dev[qd_num], p, &link) != GT_OK) {
        printk("gprtGetLinkState failed (port %d)\n", p);
        return;
    }

    for (i = 0; i < num_connected_switches; i++)
        prev_ports_link |= vlan_cfg->ports_link[i];

    if (link)
        vlan_cfg->ports_link[qd_num] |= (1 << p);
    else
        vlan_cfg->ports_link[qd_num] &= ~(1 << p);

    for (i = 0; i < num_connected_switches; i++)
        new_ports_link |= vlan_cfg->ports_link[i];

    if ( (new_ports_link == 0) && (prev_ports_link != 0) )
    {
            netif_carrier_off(dev);
            netif_stop_queue(dev);
            printk("%s: interface down\n", dev->name );
    }
    else if ( (new_ports_link != 0) && (prev_ports_link == 0) )
    {
        netif_carrier_on(dev);
        netif_wake_queue(dev);
        printk("%s: interface up\n", dev->name );
    }
}

static void mv_gtw_link_status_print(int qd_num, unsigned int port)
{
    char                *duplex_str = NULL, *speed_str = NULL;
    GT_BOOL             link, duplex;
    GT_PORT_SPEED_MODE  speed_mode;

    if (gprtGetLinkState(&qd_dev[qd_num], port, &link) != GT_OK) {
        printk("gprtGetLinkState failed (switch %d, port %d)\n", qd_num, port);
        return;
    }

    if (link) {
        if (gprtGetDuplex(&qd_dev[qd_num], port, &duplex) != GT_OK) {
            printk("gprtGetDuplex failed (switch %d, port %d)\n", qd_num, port);
            duplex_str = "ERR";
        }
        else
            duplex_str = (duplex) ? "Full" : "Half";

        if (gprtGetSpeedMode(&qd_dev[qd_num], port, &speed_mode) != GT_OK) {
	        printk("gprtGetSpeedMode failed (switch %d, port %d)\n", qd_num, port);
            speed_str = "ERR";
        }
        else {
            if (speed_mode == PORT_SPEED_1000_MBPS)
                speed_str = "1000Mbps";
            else if (speed_mode == PORT_SPEED_100_MBPS)
                speed_str = "100Mbps";
            else
                speed_str = "10Mbps";
        }
        printk("%s: Link-up, Duplex-%s, Speed-%s.\n",
                SWITCH_PORT[mv_gtw_phys_switch_port_to_logical_port(qd_num, port)].portStr, duplex_str, speed_str);
    }
    else
    {
        printk("%s: Link-down\n",
                SWITCH_PORT[mv_gtw_phys_switch_port_to_logical_port(qd_num, port)].portStr);
    }
}

static irqreturn_t mv_gtw_link_interrupt_handler(int irq , void *dev_id)
{
    unsigned short switch_cause = 0, phy_cause, phys_port = 0, p, switch_ind;
    OUT GT_DEV_INT_STATUS devIntStatus;

    /* go over all connected switches */
    for (switch_ind = 0; switch_ind < num_connected_switches; switch_ind++)
    {
        if (switch_irq[switch_ind] != -1 ) {
	        if ((qd_dev[switch_ind].deviceId == GT_88E6161) ||
                (qd_dev[switch_ind].deviceId == GT_88E6165) ||
                (qd_dev[switch_ind].deviceId == GT_88E6171)) {
                /* required to clear the interrupt, and updates phys_port */
	            if (geventGetDevIntStatus(&qd_dev[switch_ind], &devIntStatus) != GT_OK) {
                    printk("geventGetDevIntStatus failed, ignoring interrupt\n");
                    return IRQ_HANDLED;
                }
                if (devIntStatus.devIntCause != GT_DEV_INT_PHY) {
                    ETH_DBG(ETH_DBG_GTW, ("devIntCause != GT_DEV_INT_PHY, ignoring interrupt\n"));
                    return IRQ_HANDLED;
                }

                phys_port = devIntStatus.phyInt & 0xFF;
	            if (phys_port)
		            switch_cause = GT_PHY_INTERRUPT;
	        }
	        else {
	            if(eventGetIntStatus(&qd_dev[switch_ind], &switch_cause) != GT_OK)
	                switch_cause = 0;
	        }
        }
        else {
	        switch_cause = GT_PHY_INTERRUPT;
        }

        if (switch_cause & GT_PHY_INTERRUPT) {
	        /* If we're using a 6161/6165/6171 Switch and using the Switch interrupt, we already have phys_port updated above */
	        /* If we're using any other Switch, or if we're using polling, we need to update phys_port now */
	        if ((qd_dev[switch_ind].deviceId == GT_88E6161) ||
                (qd_dev[switch_ind].deviceId == GT_88E6165) ||
                (qd_dev[switch_ind].deviceId == GT_88E6171)) {
		        if (switch_irq[switch_ind] == -1) {
			        geventGetDevIntStatus(&qd_dev[switch_ind], &devIntStatus);
			        phys_port = devIntStatus.phyInt & 0xFF;
			        /* TODO: check if next line is needed */
			        phys_port |= 0x18; /* we cannot get indication for these ports in this method, so check them */
		        }
	        }
	        else {
		    gprtGetPhyIntPortSummary(&qd_dev[switch_ind],&phys_port);
	        }

	        /* go over all switch ports */
            for (p = 0; p < qd_dev[switch_ind].numOfPorts; p++) {
                if (MV_BIT_CHECK(phys_port, p)          &&
                    (p != SWITCH_PORT_CPU[switch_ind])  &&
                    (p != SWITCH_PORT_IC[switch_ind])   &&
                    (!MV_BIT_CHECK(SWITCH_UNUSED_PORTS[switch_ind], p))) {
		            if (gprtGetPhyIntStatus(&qd_dev[switch_ind],p,&phy_cause) == GT_OK) {
		                if (phy_cause & GT_LINK_STATUS_CHANGED)
                        {
                            struct net_device* dev = mv_gtw_port_to_netdev_map(switch_ind, p);

                            if( (dev != NULL) && (netif_running(dev)) )
	                            mv_gtw_update_link_status(switch_ind, dev, p);

                            mv_gtw_link_status_print(switch_ind, p);
                        }
		            }
		        }
	        }
        }
    }

    if ( (switch_irq[SWITCH_0] == -1) ||
         ((switch_irq[SWITCH_1] == -1) && (num_connected_switches > 1))) {
        switch_link_timer.expires = jiffies + (HZ); /* 1 second */
        add_timer(&switch_link_timer);
    }

    return IRQ_HANDLED;
}

static void mv_gtw_link_timer_function(unsigned long data)
{
    /* The parameters are not used anyway */
    mv_gtw_link_interrupt_handler(switch_irq[SWITCH_0], NULL);
}
#endif /* CONFIG_MV_GTW_LINK_STATUS */

int mv_gtw_start( struct net_device *dev )
{
    mv_eth_priv     *priv = MV_ETH_PRIV(dev);
    struct          mv_vlan_cfg *vlan_cfg = MV_NETDEV_VLAN(dev);
    unsigned char   broadcast[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
    extern struct   net_device** mv_net_devs;
    extern int      mv_net_devs_num;
    unsigned short  p, switch_ind;
    unsigned int    port_mask[BOARD_MAX_CASCADED_SWITCHES];
    int i;

    printk("mv_gateway: starting %s\n",dev->name);

    /* start upper layer accordingly with ports_map */
#ifdef CONFIG_MV_GTW_LINK_STATUS
    for (i = 0; i < num_connected_switches; i++)
        vlan_cfg->ports_link[i] = 0;

    /* go over all connected switches */
    for (switch_ind = 0; switch_ind < num_connected_switches; switch_ind++) {
        /* go over all switch ports */
        for (p = 0; p < qd_dev[switch_ind].numOfPorts; p++) {
            /* if switch port is connected */
            if (MV_BIT_CHECK(vlan_cfg->ports_mask[switch_ind], p)) {
                mv_gtw_update_link_status(switch_ind, dev, p);
            }
        }
    }
#else
    for (i = 0; i < num_connected_switches; i++)
        vlan_cfg->ports_link[i] = vlan_cfg->ports_mask[i];
#endif

    /* Check MTU */
    for (i = 0; i < mv_net_devs_num; i++) {
        if (MV_ETH_PRIV(mv_net_devs[i]) == priv && mv_net_devs[i]->mtu != dev->mtu)
        {
            printk(KERN_ERR "All gateway devices must have same MTU\n");
            return -EPERM;
        }
    }

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
    netif_poll_enable(dev);
#else
    if ( (priv->net_dev == dev) || !netif_running(priv->net_dev) )
    {
        napi_enable(&priv->napi);
    }
#endif

    /* Add our MAC addr to the VLAN DB at switch level to forward packets with this DA  */
    /* to the CPU port. The device is always in promisc mode.                           */
    for (switch_ind = 0; switch_ind < num_connected_switches; switch_ind++) {
        port_mask[switch_ind] = (1 << SWITCH_PORT_CPU[switch_ind]);
    }
    mv_gtw_set_mac_addr_to_switch(dev->dev_addr, MV_GTW_VLAN_TO_GROUP(vlan_cfg->vlan_grp_id), port_mask, 1);

    /* We also need to allow L2 broadcasts comming up for this interface */
    for (switch_ind = 0; switch_ind < num_connected_switches; switch_ind++) {
        port_mask[switch_ind] = (vlan_cfg->ports_mask[switch_ind] | (1 << SWITCH_PORT_CPU[switch_ind]));
    }
    mv_gtw_set_mac_addr_to_switch(broadcast, MV_GTW_VLAN_TO_GROUP(vlan_cfg->vlan_grp_id), port_mask, 1);

    if (!(priv->flags & MV_ETH_F_TIMER))
    {
        priv->timer.expires = jiffies + ((HZ*CONFIG_MV_ETH_TIMER_PERIOD)/1000); /*ms*/
        add_timer( &(priv->timer) );
        priv->flags |= MV_ETH_F_TIMER;
    }

    if ( (priv->net_dev == dev) || !netif_running(priv->net_dev) )
    {
        priv->net_dev = dev;

        /* connect to MAC port interrupt line */
        if ( request_irq( IRQ_KIRKWOOD_GE(priv->port), mv_eth_interrupt_handler,
             (IRQF_DISABLED | IRQF_SAMPLE_RANDOM), "mv_gateway", priv) )
        {
            printk(KERN_ERR "failed to assign irq%d\n", IRQ_KIRKWOOD_GE(priv->port));
        }

        /* unmask interrupts */
        mv_eth_unmask_interrupts(priv);
    }

    return 0;
}

int mv_gtw_stop( struct net_device *dev )
{
    mv_eth_priv		    *priv = MV_ETH_PRIV(dev);
    struct mv_vlan_cfg	*vlan_cfg = MV_NETDEV_VLAN(dev);
    int num = 0;

    printk("mv_gateway: stopping %s\n",dev->name);

    /* stop upper layer */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
    netif_poll_disable(dev);
#endif
    netif_carrier_off(dev);
    netif_stop_queue(dev);

    for (num = 0; num < num_connected_switches; num++) {
        /* stop switch from forwarding packets from this VLAN toward CPU port */
        if (gfdbFlushInDB(&qd_dev[num], GT_FLUSH_ALL, MV_GTW_VLAN_TO_GROUP(vlan_cfg->vlan_grp_id)) != GT_OK) {
            printk("gfdbFlushInDB failed\n");
        }
        /* It is possible that the interface is in promiscuous mode */
        /* If so, the CPU port is connected with port based VLAN to the other ports, and */
        /* we must disconnect it now to stop the Switch from forwarding packets to the CPU */
        /* when the interface is down. */
        /* mv_gtw_set_multicast_list will be called anyway by Linux when we do ifconfig up */
        /* and will re-set the promiscuous feature if needed */
        if (dev->flags & IFF_PROMISC) {
	        if (mv_gtw_promisc_set(num, vlan_cfg, 0))
	            printk("mv_switch_promisc_set to 0 failed\n");
        }
    }

    if (priv->net_dev == dev)
    {
        struct net_device *main_dev = mv_gtw_main_net_dev_get();

        if (main_dev == NULL)
        {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
            napi_disable(&priv->napi);
#endif
            mv_eth_mask_interrupts(priv);
            priv->flags &= ~MV_ETH_F_TIMER;
            del_timer(&priv->timer);

            free_irq(dev->irq, priv);
        }
        else
        {
            priv->net_dev = main_dev;
        }
    }
    return 0;
}

/***********************************************************
 * gtw_init_complete --                                    *
 *   complete all initializations relevant for Gateway.    *
 ***********************************************************/
int  __init mv_gtw_init_complete(mv_eth_priv* priv)
{
    int status = 0, i = 0;
#ifdef CONFIG_MV_GTW_LINK_STATUS
    int timer_initialized = 0;
#endif

    status = mv_gw_switch_init(priv->port);
    if (status != 0)
	    return status;

    status = mv_eth_start_internals(priv, gtw_config.mtu);
    if (status != 0)
	    return status;

    if (use_dsa_tag)
    {
        status = mvEthHeaderModeSet(priv->hal_priv, MV_ETH_DISABLE_HEADER_MODE);
        if (status != 0)
            return status;

        status = mvEthDSAModeSet(priv->hal_priv, MV_ETH_DSA_TAG_NON_EXTENDED);
        if (status != 0)
            return status;
    }
    else /* Marvell Header mode */
    {
        status = mvEthHeaderModeSet(priv->hal_priv, MV_ETH_ENABLE_HEADER_MODE_PRI_2_1);
        if (status != 0)
	        return status;
    }

    /* Mask interrupts */
    mv_eth_mask_interrupts(priv);

#ifdef CONFIG_MV_GTW_IGMP
    /* Initialize the IGMP snooping handler */
    if(mv_gtw_igmp_snoop_init()) {
        printk("failed to init IGMP snooping handler\n");
    }
#endif

#ifdef CONFIG_MV_GTW_LINK_STATUS
    for (i = 0; i < num_connected_switches; i++)
    {
        if (switch_irq[i] != -1)
        {
            if (request_irq(switch_irq[i], mv_gtw_link_interrupt_handler,
                (IRQF_DISABLED | IRQF_SAMPLE_RANDOM), "link status", NULL))
	        {
                printk(KERN_ERR "failed to assign irq%d\n", switch_irq[i]);
	        }
        }
        else if (!timer_initialized)
        {
            memset( &switch_link_timer, 0, sizeof(struct timer_list) );
            init_timer(&switch_link_timer);
            switch_link_timer.function = mv_gtw_link_timer_function;
            switch_link_timer.data = -1;
	    switch_link_timer.expires = jiffies + (HZ); /* 1 second */
            add_timer(&switch_link_timer);
            timer_initialized = 1;
        }
    }
#endif /* CONFIG_MV_GTW_LINK_STATUS */

    return 0;
}

void    mv_gtw_status_print(void)
{
    mv_eth_priv         *priv = gtw_config.priv;
    struct mv_vlan_cfg  *vlan_cfg;
    int                 i;

    if (use_dsa_tag)
    {
        printk("Gateway Giga port #%d, priv=%p, vlans_num=%d, mtu=%d, switch_enabled_ports[0]=0x%x, switch_enabled_ports[1]=0x%x\n",
                priv->port, priv, gtw_config.vlans_num, gtw_config.mtu, switch_enabled_ports[SWITCH_0], switch_enabled_ports[SWITCH_1]);
    }
    else
    {
        printk("Gateway Giga port #%d, priv=%p, vlans_num=%d, mtu=%d, switch_enabled_ports=0x%x\n",
                priv->port, priv, gtw_config.vlans_num, gtw_config.mtu, switch_enabled_ports[SWITCH_0]);
    }

    for (i = 0; i < MV_GTW_MAX_NUM_OF_IFS; i++)
    {
        vlan_cfg = &gtw_config.vlan_cfg[i];
        if (vlan_cfg->vlan_grp_id == 0)
            continue;

        if (use_dsa_tag)
        {
            printk("%s: vlan_grp_id=0x%x, ports_mask[0]=0x%x, ports_mask[1]=0x%x, ports_link[0]=0x%x, ports_link[1]=0x%x, header=0x%04x\n",
                    vlan_cfg->net_dev->name, vlan_cfg->vlan_grp_id, vlan_cfg->ports_mask[SWITCH_0], vlan_cfg->ports_mask[SWITCH_1],
                    vlan_cfg->ports_link[SWITCH_0], vlan_cfg->ports_link[SWITCH_1], vlan_cfg->header);
        }
        else
        {
            printk("%s: vlan_grp_id=0x%x, ports_mask=0x%x, ports_link=0x%x, mv_header=0x%04x\n",
                    vlan_cfg->net_dev->name, vlan_cfg->vlan_grp_id, vlan_cfg->ports_mask[SWITCH_0],
                    vlan_cfg->ports_link[SWITCH_0], vlan_cfg->header);
        }
    }
}

int     mv_gtw_switch_port_add(struct net_device* dev, int port)
{
    struct mv_vlan_cfg  *vlan_cfg = MV_NETDEV_VLAN(dev);
    int                 switch_num, switch_port, p;
    unsigned int        ports_mask;

    if (vlan_cfg == NULL)
    {
        printk("%s is not connected to switch\n", dev->name);
        return 1;
    }
    if (netif_running(dev))
    {
        printk("%s must be down to change switch ports map\n", dev->name);
        return 1;
    }

    switch_num  = SWITCH_PORT[port].qdSwitchNum;
    switch_port = SWITCH_PORT[port].qdPortNum;
    if ( (switch_num < 0) || (switch_port < 0) )
    {
        printk("Switch port %d can't be added\n", port);
        return 1;
    }
    if (MV_BIT_CHECK(switch_enabled_ports[switch_num], switch_port))
    {
        printk("Switch port %d is already enabled\n", port);
        return 0;
    }

    /* Set default VLAN_ID for port */
    if (gvlnSetPortVid(&qd_dev[switch_num], switch_port, vlan_cfg->vlan_grp_id) != GT_OK ) {
        printk("gvlnSetPortVid failed");
        return -1;
    }
    /* Map port to VLAN DB */
    if (gvlnSetPortVlanDBNum(&qd_dev[switch_num], switch_port, MV_GTW_VLAN_TO_GROUP(vlan_cfg->vlan_grp_id)) != GT_OK) {
        printk("gvlnSetPortVlanDBNum failed\n");
        return -1;
    }

    /* Update data base */
    vlan_cfg->ports_mask[switch_num] |= (1 << switch_port);
    switch_enabled_ports[switch_num] |= (1 << switch_port);
    if (use_dsa_tag)
        vlan_cfg->header = vlan_cfg->vlan_grp_id;
    else /* Marvell Header mode, only one Switch */
        vlan_cfg->header = cpu_to_be16((MV_GTW_VLAN_TO_GROUP(vlan_cfg->vlan_grp_id) << 12) | vlan_cfg->ports_mask[SWITCH_0]);

    /* Add port to the VLAN (CPU port is not part of VLAN) */
    ports_mask = vlan_cfg->ports_mask[switch_num];
    if (use_dsa_tag)
        ports_mask |= (1 << SWITCH_PORT_IC[switch_num]);
    if (switch_num == SWITCH_0)
        ports_mask &= ~(1 << SWITCH_PORT_CPU[switch_num]);

    if(mv_gtw_set_port_based_vlan(switch_num, ports_mask) != 0) {
        printk("mv_gtw_set_port_based_vlan failed\n");
    }

    /* Add port to vtu (used in tx) */
    if (mv_gtw_set_vlan_in_vtu(switch_num, vlan_cfg->vlan_grp_id, MV_GTW_VLAN_TO_GROUP(vlan_cfg->vlan_grp_id),
            (ports_mask | (1 << SWITCH_PORT_CPU[switch_num])), 1)) {
        printk("mv_gtw_set_vlan_in_vtu failed\n");
    }

    if ( (!use_dsa_tag) && (switch_num == SWITCH_0) ) /* Marvell Header mode */
    {
        /* set vtu with each port private vlan id (used in rx) */
        for (p = 0; p < qd_dev[switch_num].numOfPorts; p++) {
	        if (MV_BIT_CHECK(vlan_cfg->ports_mask[switch_num], p) && (p != SWITCH_PORT_CPU[switch_num])) {
                if (mv_gtw_set_vlan_in_vtu(switch_num, MV_GTW_PORT_VLAN_ID(vlan_cfg->vlan_grp_id, p), MV_GTW_VLAN_TO_GROUP(vlan_cfg->vlan_grp_id),
                                          (vlan_cfg->ports_mask[switch_num] & ~(1 << SWITCH_PORT_CPU[switch_num])), 1) != 0) {
		            printk("mv_gtw_set_vlan_in_vtu failed\n");
                }
	        }
	    }
    }

    /* Enable port */
    if (gstpSetPortState(&qd_dev[switch_num], switch_port, GT_PORT_FORWARDING) != GT_OK) {
        printk("gstpSetPortState failed\n");
    }

#ifdef CONFIG_MV_GTW_LINK_STATUS
    /* Enable Phy Link Status Changed interrupt at Phy level for the port */
    if (gprtPhyIntEnable(&qd_dev[switch_num], switch_port, (GT_LINK_STATUS_CHANGED)) != GT_OK) {
        printk("gprtPhyIntEnable failed port %d\n", switch_port);
    }
#endif /* CONFIG_MV_GTW_LINK_STATUS */

    printk("%s: Switch port #%d mapped\n", dev->name, port);

    return 0;
}

int     mv_gtw_switch_port_del(struct net_device* dev, int port)
{
    struct mv_vlan_cfg  *vlan_cfg = MV_NETDEV_VLAN(dev);
    int                 switch_num, switch_port, p;
    unsigned int        ports_mask;

    if (vlan_cfg == NULL)
    {
        printk("%s aren't connected to switch\n", dev->name);
        return 1;
    }
    if (netif_running(dev))
    {
        printk("%s must be down to change switch ports map\n", dev->name);
        return 1;
    }

    switch_num  = SWITCH_PORT[port].qdSwitchNum;
    switch_port = SWITCH_PORT[port].qdPortNum;
    if ( (switch_num < 0) || (switch_port < 0) )
    {
        printk("Switch port %d can't be deleted\n", port);
        return 1;
    }
    if(!MV_BIT_CHECK(switch_enabled_ports[switch_num], switch_port))
    {
        printk("Switch port %d is disabled\n", port);
        return 1;
    }
    if(!MV_BIT_CHECK(vlan_cfg->ports_mask[switch_num], switch_port))
    {
        printk("Switch port %d is already not mapped on %s\n", port, dev->name);
        return 0;
    }

#ifdef CONFIG_MV_GTW_LINK_STATUS
    /* Disable link change interrupts on unmapped port */
    if (gprtPhyIntEnable(&qd_dev[switch_num], switch_port, 0) != GT_OK) {
        printk("gprtPhyIntEnable failed on port #%d\n", switch_port);
    }
    vlan_cfg->ports_link[switch_num] &= ~(1 << switch_port);
#endif /* CONFIG_MV_GTW_LINK_STATUS */

    /* Disable unmapped port */
    if (gstpSetPortState(&qd_dev[switch_num], switch_port, GT_PORT_DISABLE) != GT_OK) {
        printk("gstpSetPortState failed on port #%d\n",  port);
    }

    /* Update data base */
    vlan_cfg->ports_mask[switch_num] &= ~(1 << switch_port);
    switch_enabled_ports[switch_num] &= ~(1 << switch_port);

    if (use_dsa_tag)
        vlan_cfg->header = vlan_cfg->vlan_grp_id;
    else /* Marvell Header mode, only one Switch */
        vlan_cfg->header = cpu_to_be16((MV_GTW_VLAN_TO_GROUP(vlan_cfg->vlan_grp_id) << 12) | vlan_cfg->ports_mask[SWITCH_0]);

    /* Remove port from the VLAN (CPU port is not part of VLAN) */
    ports_mask = vlan_cfg->ports_mask[switch_num];
    if (use_dsa_tag)
        ports_mask |= (1 << SWITCH_PORT_IC[switch_num]);
    if (switch_num == SWITCH_0)
        ports_mask &= ~(1 << SWITCH_PORT_CPU[switch_num]);

    if (mv_gtw_set_port_based_vlan(switch_num, ports_mask) != 0) {
	        printk("mv_gtw_set_port_based_vlan failed\n");
    }

    /* Remove port from vtu (used in tx) */
    if (mv_gtw_set_vlan_in_vtu(switch_num, vlan_cfg->vlan_grp_id, MV_GTW_VLAN_TO_GROUP(vlan_cfg->vlan_grp_id),
            (ports_mask | (1 << SWITCH_PORT_CPU[switch_num])), 1)) {
        printk("mv_gtw_set_vlan_in_vtu failed\n");
    }

    if ( (!use_dsa_tag) && (switch_num == SWITCH_0) ) /* Marvell Header mode */
    {
        /* set vtu with each port private vlan id (used in rx) */
        for (p = 0; p < qd_dev[switch_num].numOfPorts; p++) {
	        if (MV_BIT_CHECK(vlan_cfg->ports_mask[switch_num], p) && (p != SWITCH_PORT_CPU[switch_num])) {
                if (mv_gtw_set_vlan_in_vtu(switch_num, MV_GTW_PORT_VLAN_ID(vlan_cfg->vlan_grp_id, p), MV_GTW_VLAN_TO_GROUP(vlan_cfg->vlan_grp_id),
                                          (vlan_cfg->ports_mask[switch_num] & ~(1 << SWITCH_PORT_CPU[switch_num])), 1) != 0) {
		            printk("mv_gtw_set_vlan_in_vtu failed\n");
                }
	        }
	    }
    }

    printk("%s: Switch port #%d unmapped\n", dev->name, port);

    return 0;
}

short mv_gtw_net_device_get(int switch_num, int switch_port)
{
	int i;
	int port_map = (1 << switch_port);

	for (i = 0 ; i < MV_GTW_MAX_NUM_OF_IFS ; i++)
	{
		if (gtw_config.vlan_cfg[i].ports_mask[switch_num] & port_map) /* If member of VLAN */
		{
			return gtw_config.vlan_cfg[i].vlan_grp_id;
		}
	}

	printk("%s: Device not found\n", __FUNCTION__);
	return -1;
}

int mv_gtw_set_vtu(int port, int tag, int add)
{
	int i = 0;
	int switch_num;
	int switch_port;
	unsigned int port_mask = 0;
	short group_id = 0;

    switch_num  = SWITCH_PORT[port].qdSwitchNum;
    switch_port = SWITCH_PORT[port].qdPortNum;
    if ( (switch_num < 0) || (switch_port < 0) )
	{
		printk("%s: Error mapping port.\n", __FUNCTION__);
		return MV_FAIL;
	}

	group_id = mv_gtw_net_device_get(switch_num, switch_port);
	if (group_id < 0)
	{
		printk("%s: Error getting group ID.\n", __FUNCTION__);
		return MV_FAIL;
	}

	for (i = 0 ; i < num_connected_switches ; i++)
	{
		port_mask = (1 << SWITCH_PORT_CPU[i]);
        if (SWITCH_PORT_IC[i] >= 0)
            port_mask |= (1 << SWITCH_PORT_IC[i]);
		if (i == switch_num)
		{
			port_mask |= (1 << switch_port);
		}

		if (mv_gtw_set_vlan_in_vtu(i, (unsigned short)tag, MV_GTW_VLAN_TO_GROUP(group_id),  port_mask, add) < 0)
		{
			printk("%s: Error setting vtu.\n", __FUNCTION__);
			return MV_FAIL;
		}
	}
	printk("VLAN tag %d is now %sset for port %d.\n", tag, add ? "" : "un", port);
	return MV_OK;
}

#define QD_FMT "%10lu %10lu %10lu %10lu %10lu %10lu %10lu\n"
#define QD_CNT(c,f) c[0].f, c[1].f, c[2].f, c[3].f, c[4].f, c[5].f, c[6].f
#define QD_MAX 7
void    mv_gtw_switch_stats(int port)
{
    GT_STATS_COUNTER_SET3 counters[QD_MAX];
    int p, num;

    for (num = 0; num < num_connected_switches; num++)
    {
        if (&qd_dev[num] == NULL) {
            printk("Switch %d is not initialized\n", num);
            return;
	}
        memset(counters, 0, sizeof(GT_STATS_COUNTER_SET3) * QD_MAX);

        for (p = 0; p < QD_MAX; p++)
            gstatsGetPortAllCounters3(&qd_dev[num], p, &counters[p]);

        printk("Switch #%d\n", num);

        printk("PortNum         " QD_FMT,  (GT_U32)0, (GT_U32)1, (GT_U32)2, (GT_U32)3, (GT_U32)4, (GT_U32)5, (GT_U32)6);
        printk("-----------------------------------------------------------------------------------------------\n");
        printk("InGoodOctetsLo  " QD_FMT,  QD_CNT(counters,InGoodOctetsLo));
        printk("InGoodOctetsHi  " QD_FMT,  QD_CNT(counters,InGoodOctetsHi));
        printk("InBadOctets     " QD_FMT,  QD_CNT(counters,InBadOctets));
        printk("OutFCSErr       " QD_FMT,  QD_CNT(counters,OutFCSErr));
        printk("Deferred        " QD_FMT,  QD_CNT(counters,Deferred));
        printk("InBroadcasts    " QD_FMT,  QD_CNT(counters,InBroadcasts));
        printk("InMulticasts    " QD_FMT,  QD_CNT(counters,InMulticasts));
        printk("Octets64        " QD_FMT,  QD_CNT(counters,Octets64));
        printk("Octets127       " QD_FMT,  QD_CNT(counters,Octets127));
        printk("Octets255       " QD_FMT,  QD_CNT(counters,Octets255));
        printk("Octets511       " QD_FMT,  QD_CNT(counters,Octets511));
        printk("Octets1023      " QD_FMT,  QD_CNT(counters,Octets1023));
        printk("OctetsMax       " QD_FMT,  QD_CNT(counters,OctetsMax));
        printk("OutOctetsLo     " QD_FMT,  QD_CNT(counters,OutOctetsLo));
        printk("OutOctetsHi     " QD_FMT,  QD_CNT(counters,OutOctetsHi));
        printk("OutUnicasts     " QD_FMT,  QD_CNT(counters,OutOctetsHi));
        printk("Excessive       " QD_FMT,  QD_CNT(counters,Excessive));
        printk("OutMulticasts   " QD_FMT,  QD_CNT(counters,OutMulticasts));
        printk("OutBroadcasts   " QD_FMT,  QD_CNT(counters,OutBroadcasts));
        printk("Single          " QD_FMT,  QD_CNT(counters,OutBroadcasts));
        printk("OutPause        " QD_FMT,  QD_CNT(counters,OutPause));
        printk("InPause         " QD_FMT,  QD_CNT(counters,InPause));
        printk("Multiple        " QD_FMT,  QD_CNT(counters,InPause));
        printk("Undersize       " QD_FMT,  QD_CNT(counters,Undersize));
        printk("Fragments       " QD_FMT,  QD_CNT(counters,Fragments));
        printk("Oversize        " QD_FMT,  QD_CNT(counters,Oversize));
        printk("Jabber          " QD_FMT,  QD_CNT(counters,Jabber));
        printk("InMACRcvErr     " QD_FMT,  QD_CNT(counters,InMACRcvErr));
        printk("InFCSErr        " QD_FMT,  QD_CNT(counters,InFCSErr));
        printk("Collisions      " QD_FMT,  QD_CNT(counters,Collisions));
        printk("Late            " QD_FMT,  QD_CNT(counters,Late));
        printk("\n");

        gstatsFlushAll(&qd_dev[num]);
    }
}
