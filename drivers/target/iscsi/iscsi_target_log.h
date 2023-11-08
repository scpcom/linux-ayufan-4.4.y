/**
 * $Header: /home/cvsroot/NasX86/Kernel/linux-3.2.26/drivers/target/iscsi/iscsi_target_log.h,v 1.1.2.1 2014/03/07 07:24:12 jonathanho Exp $
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
 * @brief	iscsi target log function declaration.
 * @author	Nike Chen
 * @date	2009/11/23
 *
 * $Id: iscsi_target_log.h,v 1.1.2.1 2014/03/07 07:24:12 jonathanho Exp $
 */

#ifndef _ISCSI_TARGET_LOG
#define _ISCSI_TARGET_LOG

#include "iscsi_logd.h"

extern int iscsi_log_send_msg(int conn_type, int log_type, char *init_iqn, char *ip, char *target_iqn);
extern int iscsi_log_init(void);
extern void iscsi_log_cleanup(void);

#endif

