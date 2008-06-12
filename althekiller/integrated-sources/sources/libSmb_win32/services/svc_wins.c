/* 
 *  Unix SMB/CIFS implementation.
 *  Service Control API Implementation
 *  Copyright (C) Gerald Carter                   2005.
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "includes.h"

/* Implementation for internal wins service */

/*********************************************************************
*********************************************************************/

static WERROR wins_status( const char *service, SERVICE_STATUS *service_status )
{
	ZERO_STRUCTP( service_status );

	service_status->type              = 0x10;
	service_status->controls_accepted = SVCCTL_ACCEPT_NONE;

	if ( lp_wins_support() ) 
		service_status->state     = SVCCTL_RUNNING;
	else {
		service_status->state              = SVCCTL_STOPPED;
		service_status->win32_exit_code    = WERR_SERVICE_NEVER_STARTED;
	}
	
	return WERR_OK;
}

/*********************************************************************
*********************************************************************/

static WERROR wins_stop( const char *service, SERVICE_STATUS *service_status )
{
	wins_status( service, service_status );

	return WERR_ACCESS_DENIED;
}

/*********************************************************************
*********************************************************************/

static WERROR wins_start( const char *service )
{
	return WERR_ACCESS_DENIED;
}

/*********************************************************************
*********************************************************************/

/* struct for svcctl control to manipulate wins service */

SERVICE_CONTROL_OPS wins_svc_ops = {
	wins_stop,
	wins_start,
	wins_status
};
