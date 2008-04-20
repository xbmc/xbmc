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

/*********************************************************************
*********************************************************************/

static WERROR rcinit_stop( const char *service, SERVICE_STATUS *status )
{
	pstring command;
	int ret, fd;
	
	pstr_sprintf( command, "%s/%s/%s stop", dyn_LIBDIR, SVCCTL_SCRIPT_DIR, service );
	
	/* we've already performed the access check when the service was opened */
	
	become_root();
	ret = smbrun( command , &fd );
	unbecome_root();
	
	DEBUGADD(5, ("rcinit_start: [%s] returned [%d]\n", command, ret));
	close(fd);
	
	ZERO_STRUCTP( status );
	status->type = 0x0020;
	status->state = (ret == 0 ) ? 0x0001 : 0x0004;
	status->controls_accepted = 0x0005;

	return ( ret == 0 ) ? WERR_OK : WERR_ACCESS_DENIED;
}

/*********************************************************************
*********************************************************************/

static WERROR rcinit_start( const char *service )
{
	pstring command;
	int ret, fd;
	
	pstr_sprintf( command, "%s/%s/%s start", dyn_LIBDIR, SVCCTL_SCRIPT_DIR, service );
	
	/* we've already performed the access check when the service was opened */
	
	become_root();
	ret = smbrun( command , &fd );
	unbecome_root();
	
	DEBUGADD(5, ("rcinit_start: [%s] returned [%d]\n", command, ret));
	close(fd);	

	return ( ret == 0 ) ? WERR_OK : WERR_ACCESS_DENIED;
}

/*********************************************************************
*********************************************************************/

static WERROR rcinit_status( const char *service, SERVICE_STATUS *status )
{
	pstring command;
	int ret, fd;
	
	pstr_sprintf( command, "%s/%s/%s status", dyn_LIBDIR, SVCCTL_SCRIPT_DIR, service );
	
	/* we've already performed the access check when the service was opened */
	/* assume as return code of 0 means that the service is ok.  Anything else
	   is STOPPED */
	
	become_root();
	ret = smbrun( command , &fd );
	unbecome_root();
	
	DEBUGADD(5, ("rcinit_start: [%s] returned [%d]\n", command, ret));
	close(fd);
	
	ZERO_STRUCTP( status );
	status->type = 0x0020;
	status->state = (ret == 0 ) ? 0x0004 : 0x0001;
	status->controls_accepted = 0x0005;

	return WERR_OK;
}

/*********************************************************************
*********************************************************************/

/* struct for svcctl control to manipulate rcinit service */

SERVICE_CONTROL_OPS rcinit_svc_ops = {
	rcinit_stop,
	rcinit_start,
	rcinit_status
};
