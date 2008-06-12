/*
   Unix SMB/Netbios implementation.
   Version 1.9.
   VFS module to perform read-only limitation based on a time period
   Copyright (C) Alexander Bokovoy 2003

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

   This work was sponsored by Optifacio Software Services, Inc.
*/

#include "includes.h"
#include "getdate.h"

/*
  This module performs a read-only limitation for specified share 
  (or all of them if it is loaded in a [global] section) based on period
  definition in smb.conf. You can stack this module multiple times under 
  different names to get multiple limit intervals.

  The module uses get_date() function from coreutils' date utility to parse
  specified dates according to date(1) rules. Look into info page for date(1)
  to understand the syntax.

  The module accepts one parameter: 

  readonly: period = "begin date","end date"

  where "begin date" and "end date" are mandatory and should comply with date(1) 
  syntax for date strings.

  Example:

  readonly: period = "today 14:00","today 15:00"

  Default:

  readonly: period = "today 0:0:0","tomorrow 0:0:0"

  The default covers whole day thus making the share readonly

 */

#define MODULE_NAME "readonly"
static int readonly_connect(vfs_handle_struct *handle, 
			    connection_struct *conn, 
			    const char *service, 
			    const char *user)    
{
  const char *period_def[] = {"today 0:0:0", "tomorrow 0:0:0"};

  const char **period = lp_parm_string_list(SNUM(handle->conn),
					     (handle->param ? handle->param : MODULE_NAME),
					     "period", period_def); 

  if (period && period[0] && period[1]) {
    time_t current_time = time(NULL);
    time_t begin_period = get_date(period[0], &current_time);
    time_t end_period   = get_date(period[1], &current_time);

    if ((current_time >= begin_period) && (current_time <= end_period)) {
      conn->read_only = True;
    }

    return SMB_VFS_NEXT_CONNECT(handle, conn, service, user);

  } else {
    
    return 1;
    
  }
}


/* VFS operations structure */

static vfs_op_tuple readonly_op_tuples[] = {
	/* Disk operations */
  {SMB_VFS_OP(readonly_connect),	SMB_VFS_OP_CONNECT, SMB_VFS_LAYER_TRANSPARENT},
  {SMB_VFS_OP(NULL),   		SMB_VFS_OP_NOOP,    SMB_VFS_LAYER_NOOP}
};

NTSTATUS vfs_readonly_init(void)
{
  return smb_register_vfs(SMB_VFS_INTERFACE_VERSION, MODULE_NAME, readonly_op_tuples);
}
