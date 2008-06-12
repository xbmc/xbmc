/* 
   Unix SMB/CIFS implementation.
   messages.c header
   Copyright (C) Andrew Tridgell 2000
   Copyright (C) 2001, 2002 by Martin Pool
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef _MESSAGES_H_
#define _MESSAGES_H_

/* general messages */
#define MSG_DEBUG		1
#define MSG_PING		2
#define MSG_PONG		3
#define MSG_PROFILE		4
#define MSG_REQ_DEBUGLEVEL	5
#define MSG_DEBUGLEVEL		6
#define MSG_REQ_PROFILELEVEL	7
#define MSG_PROFILELEVEL	8
#define MSG_REQ_POOL_USAGE	9
#define MSG_POOL_USAGE		10

/* If dmalloc is included, set a steady-state mark */
#define MSG_REQ_DMALLOC_MARK	11

/* If dmalloc is included, dump to the dmalloc log a description of
 * what has changed since the last MARK */
#define MSG_REQ_DMALLOC_LOG_CHANGED	12

#define MSG_SHUTDOWN		13

/* nmbd messages */
#define MSG_FORCE_ELECTION 1001
#define MSG_WINS_NEW_ENTRY 1002
#define MSG_SEND_PACKET    1003

/* printing messages */
/* #define MSG_PRINTER_NOTIFY  2001*/ /* Obsolete */
#define MSG_PRINTER_NOTIFY2		2002

#define MSG_PRINTER_DRVUPGRADE		2101
#define MSG_PRINTERDATA_INIT_RESET	2102
#define MSG_PRINTER_UPDATE		2103
#define MSG_PRINTER_MOD			2104

/* smbd messages */
#define MSG_SMB_CONF_UPDATED 3001
#define MSG_SMB_FORCE_TDIS   3002
#define MSG_SMB_SAM_SYNC     3003
#define MSG_SMB_SAM_REPL     3004
#define MSG_SMB_UNLOCK       3005
#define MSG_SMB_BREAK_REQUEST 3006
#define MSG_SMB_BREAK_RESPONSE 3007
#define MSG_SMB_ASYNC_LEVEL2_BREAK 3008
#define MSG_SMB_OPEN_RETRY   3009
#define MSG_SMB_KERNEL_BREAK 3010
#define MSG_SMB_FILE_RENAME  3011
#define MSG_SMB_INJECT_FAULT 3012

/* winbind messages */
#define MSG_WINBIND_FINISHED     4001
#define MSG_WINBIND_FORGET_STATE 4002
#define MSG_WINBIND_ONLINE       4003
#define MSG_WINBIND_OFFLINE      4004
#define MSG_WINBIND_ONLINESTATUS 4005

/* Flags to classify messages - used in message_send_all() */
/* Sender will filter by flag. */

#define FLAG_MSG_GENERAL 	0x0001
#define FLAG_MSG_SMBD		0x0002
#define FLAG_MSG_NMBD		0x0004
#define FLAG_MSG_PRINT_NOTIFY	0x0008
#define FLAG_MSG_PRINT_GENERAL	0x0010

struct process_id {
	pid_t pid;
};

#endif
