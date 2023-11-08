/**
 * $Header: /home/cvsroot/NasX86/Kernel/linux-3.2.26/drivers/target/iscsi/iscsi_logd.h,v 1.1.2.1 2014/03/07 07:24:11 jonathanho Exp $
 *
 * Copyright (c) 2009, 2010 QNAP SYSTEMS, INC.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * @brief	iscsi target connection log daemon function declaration.
 * @author	Nike Chen
 * @date	2009/11/25
 *
 * $Id: iscsi_logd.h,v 1.1.2.1 2014/03/07 07:24:11 jonathanho Exp $
 */

#ifndef _ISCSI_LOG_DAEMON_HDR
#define _ISCSI_LOG_DAEMON_HDR

#define MAX_IP_SIZE		64
#define MAX_LOG_IQN_SIZE	256

#ifndef NETLINK_ISCSI_TARGET
#define	NETLINK_ISCSI_TARGET	20
#endif

typedef struct _iscsi_conn_log
{
    // Extract following constants from naslog.h
    enum {
        LOGIN_FAIL = 9,
    	LOGIN_OK,
    	LOGOUT,
    } conn_type;
	
    enum {
	LOG_INFO = 0,
	LOG_WARN,
	LOG_ERROR,
    } log_type;
	
    char init_iqn[MAX_LOG_IQN_SIZE];
    char init_ip[MAX_IP_SIZE];
    char target_iqn[MAX_LOG_IQN_SIZE];
} iscsi_conn_log;

#endif

