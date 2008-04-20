/* 
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines
 *
 *  Copyright (C) Gerald (Jerry) Carter             2005.
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

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_RPC_SRV

/********************************************************************
********************************************************************/

static char* get_device_path( const char *device )
{
	static pstring path;

	pstr_sprintf( path, "ROOT\\Legacy_%s\\0000", device );

	return path;
}

/********************************************************************
********************************************************************/

WERROR _ntsvcs_get_version( pipes_struct *p, NTSVCS_Q_GET_VERSION *q_u, NTSVCS_R_GET_VERSION *r_u )
{
	r_u->version = 0x00000400;	/* no idea what this means */
		
	return WERR_OK;
}

/********************************************************************
********************************************************************/

WERROR _ntsvcs_get_device_list_size( pipes_struct *p, NTSVCS_Q_GET_DEVICE_LIST_SIZE *q_u, NTSVCS_R_GET_DEVICE_LIST_SIZE *r_u )
{
	fstring device;
	const char *devicepath;

	if ( !q_u->devicename )
		return WERR_ACCESS_DENIED;

	rpcstr_pull(device, q_u->devicename->buffer, sizeof(device), q_u->devicename->uni_str_len*2, 0);
	devicepath = get_device_path( device );

	r_u->size = strlen(devicepath) + 2;

	return WERR_OK;
}


/********************************************************************
********************************************************************/

WERROR _ntsvcs_get_device_list( pipes_struct *p, NTSVCS_Q_GET_DEVICE_LIST *q_u, NTSVCS_R_GET_DEVICE_LIST *r_u )
{
	fstring device;
	const char *devicepath;

	if ( !q_u->devicename )
		return WERR_ACCESS_DENIED;

	rpcstr_pull(device, q_u->devicename->buffer, sizeof(device), q_u->devicename->uni_str_len*2, 0);
	devicepath = get_device_path( device );

	/* This has to be DOUBLE NULL terminated */

	init_unistr2( &r_u->devicepath, devicepath, UNI_STR_DBLTERMINATE );
	r_u->needed = r_u->devicepath.uni_str_len;

	return WERR_OK;
}

/********************************************************************
********************************************************************/

WERROR _ntsvcs_get_device_reg_property( pipes_struct *p, NTSVCS_Q_GET_DEVICE_REG_PROPERTY *q_u, NTSVCS_R_GET_DEVICE_REG_PROPERTY *r_u )
{
	fstring devicepath;
	char *ptr;
	REGVAL_CTR *values;
	REGISTRY_VALUE *val;

	rpcstr_pull(devicepath, q_u->devicepath.buffer, sizeof(devicepath), q_u->devicepath.uni_str_len*2, 0);

	switch( q_u->property ) {
	case DEV_REGPROP_DESC:
		/* just parse the service name from the device path and then 
		   lookup the display name */
		if ( !(ptr = strrchr_m( devicepath, '\\' )) )
			return WERR_GENERAL_FAILURE;	
		*ptr = '\0';
		
		if ( !(ptr = strrchr_m( devicepath, '_' )) )
			return WERR_GENERAL_FAILURE;	
		ptr++;
		
		if ( !(values = svcctl_fetch_regvalues( ptr, p->pipe_user.nt_user_token )) )
			return WERR_GENERAL_FAILURE;	
		
		if ( !(val = regval_ctr_getvalue( values, "DisplayName" )) ) {
			TALLOC_FREE( values );
			return WERR_GENERAL_FAILURE;
		}
		
		r_u->unknown1 = 0x1;	/* always 1...tested using a remove device manager connection */
		r_u->size = reg_init_regval_buffer( &r_u->value, val );
		r_u->needed = r_u->size;

		TALLOC_FREE(values);

		break;
		
	default:
		r_u->unknown1 = 0x00437c98;
		return WERR_CM_NO_SUCH_VALUE;
	}

	return WERR_OK;
}

/********************************************************************
********************************************************************/

WERROR _ntsvcs_validate_device_instance( pipes_struct *p, NTSVCS_Q_VALIDATE_DEVICE_INSTANCE *q_u, NTSVCS_R_VALIDATE_DEVICE_INSTANCE *r_u )
{
	/* whatever dude */
	return WERR_OK;
}

/********************************************************************
********************************************************************/

WERROR _ntsvcs_get_hw_profile_info( pipes_struct *p, NTSVCS_Q_GET_HW_PROFILE_INFO *q_u, NTSVCS_R_GET_HW_PROFILE_INFO *r_u )
{
	/* steal the incoming buffer */

	r_u->buffer_size = q_u->buffer_size;
	r_u->buffer = q_u->buffer;

	/* Take the 5th Ammentment */

	return WERR_CM_NO_MORE_HW_PROFILES;
}

/********************************************************************
********************************************************************/

WERROR _ntsvcs_hw_profile_flags( pipes_struct *p, NTSVCS_Q_HW_PROFILE_FLAGS *q_u, NTSVCS_R_HW_PROFILE_FLAGS *r_u )
{	
	/* just nod your head */
	
	return WERR_OK;
}

