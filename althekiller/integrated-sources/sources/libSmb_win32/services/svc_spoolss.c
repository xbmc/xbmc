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

/* Implementation for internal spoolss service */

/*********************************************************************
*********************************************************************/

static WERROR spoolss_stop( const char *service, SERVICE_STATUS *service_status )
{
	ZERO_STRUCTP( service_status );
	
	lp_set_spoolss_state( SVCCTL_STOPPED );

	service_status->type              = 0x110;
	service_status->state             = SVCCTL_STOPPED;
	service_status->controls_accepted = SVCCTL_ACCEPT_STOP;

	DEBUG(6,("spoolss_stop: spooler stopped (not really)\n"));

	return WERR_OK;
}

/*********************************************************************
*********************************************************************/

static WERROR spoolss_start( const char *service )
{
	/* see if the smb.conf will support this anyways */
	
	if ( _lp_disable_spoolss() )
		return WERR_ACCESS_DENIED;
	
	lp_set_spoolss_state( SVCCTL_RUNNING );	
	
	return WERR_OK;
}

/*********************************************************************
*********************************************************************/

static WERROR spoolss_status( const char *service, SERVICE_STATUS *service_status )
{
	ZERO_STRUCTP( service_status );

	service_status->type              = 0x110;
	service_status->state             = lp_get_spoolss_state();
	service_status->controls_accepted = SVCCTL_ACCEPT_STOP;
	
	return WERR_OK;
}

/*********************************************************************
*********************************************************************/

/* struct for svcctl control to manipulate spoolss service */

SERVICE_CONTROL_OPS spoolss_svc_ops = {
	spoolss_stop,
	spoolss_start,
	spoolss_status
};
