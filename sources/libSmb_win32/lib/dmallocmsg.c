/* 
   samba -- Unix SMB/CIFS implementation.
   Copyright (C) 2002 by Martin Pool
   
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

#include "includes.h"

/**
 * @file dmallocmsg.c
 *
 * Glue code to cause dmalloc info to come out when we receive a prod
 * over samba messaging.
 **/

#ifdef ENABLE_DMALLOC
static unsigned long our_dm_mark = 0;
#endif /* ENABLE_DMALLOC */


/**
 * Respond to a POOL_USAGE message by sending back string form of memory
 * usage stats.
 **/
static void msg_req_dmalloc_mark(int UNUSED(msg_type), struct process_id UNUSED(src_pid),
			  void *UNUSED(buf), size_t UNUSED(len))
{
#ifdef ENABLE_DMALLOC
	our_dm_mark = dmalloc_mark();
	DEBUG(2,("Got MSG_REQ_DMALLOC_MARK: mark set\n"));
#else
	DEBUG(2,("Got MSG_REQ_DMALLOC_MARK but dmalloc not in this process\n"));
#endif
}



static void msg_req_dmalloc_log_changed(int UNUSED(msg_type),
					struct process_id UNUSED(src_pid),
					void *UNUSED(buf), size_t UNUSED(len))
{
#ifdef ENABLE_DMALLOC
	dmalloc_log_changed(our_dm_mark, True, True, True);
	DEBUG(2,("Got MSG_REQ_DMALLOC_LOG_CHANGED: done\n"));
#else
	DEBUG(2,("Got MSG_REQ_DMALLOC_LOG_CHANGED but dmalloc not in this process\n"));
#endif
}


/**
 * Register handler for MSG_REQ_POOL_USAGE
 **/
void register_dmalloc_msgs(void)
{
	message_register(MSG_REQ_DMALLOC_MARK, msg_req_dmalloc_mark);
	message_register(MSG_REQ_DMALLOC_LOG_CHANGED, msg_req_dmalloc_log_changed);
	DEBUG(2, ("Registered MSG_REQ_DMALLOC_MARK and LOG_CHANGED\n"));
}	
