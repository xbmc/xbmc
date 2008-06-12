/* 
   Unix SMB/CIFS implementation.
   ads (active directory) utility library
   Copyright (C) Andrew Tridgell 2001
   Copyright (C) Remus Koos 2001
   Copyright (C) Andrew Bartlett 2001
   
   
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

/*
  build a ADS_STATUS structure
*/
ADS_STATUS ads_build_error(enum ads_error_type etype, 
			   int rc, int minor_status)
{
	ADS_STATUS ret;

	if (etype == ENUM_ADS_ERROR_NT) {
		DEBUG(0,("don't use ads_build_error with ENUM_ADS_ERROR_NT!\n"));
		ret.err.rc = -1;
		ret.error_type = ENUM_ADS_ERROR_SYSTEM;
		ret.minor_status = 0;
		return ret;	
	}	
		
	ret.err.rc = rc;
	ret.error_type = etype;		
	ret.minor_status = minor_status;
	return ret;
}

ADS_STATUS ads_build_nt_error(enum ads_error_type etype, 
			   NTSTATUS nt_status)
{
	ADS_STATUS ret;

	if (etype != ENUM_ADS_ERROR_NT) {
		DEBUG(0,("don't use ads_build_nt_error without ENUM_ADS_ERROR_NT!\n"));
		ret.err.rc = -1;
		ret.error_type = ENUM_ADS_ERROR_SYSTEM;
		ret.minor_status = 0;
		return ret;	
	}
	ret.err.nt_status = nt_status;
	ret.error_type = etype;		
	ret.minor_status = 0;
	return ret;
}

/*
  do a rough conversion between ads error codes and NT status codes
  we'll need to fill this in more
*/
NTSTATUS ads_ntstatus(ADS_STATUS status)
{
	if (status.error_type == ENUM_ADS_ERROR_NT){
		return status.err.nt_status;	
	}
#ifdef HAVE_LDAP
	if ((status.error_type == ENUM_ADS_ERROR_LDAP) 
	    && (status.err.rc == LDAP_NO_MEMORY)) {
		return NT_STATUS_NO_MEMORY;
	}
#endif
#ifdef HAVE_KRB5
	if (status.error_type == ENUM_ADS_ERROR_KRB5) { 
		if (status.err.rc == KRB5KDC_ERR_PREAUTH_FAILED) {
			return NT_STATUS_LOGON_FAILURE;
		} else if (status.err.rc == KRB5_KDC_UNREACH) {
			return NT_STATUS_NO_LOGON_SERVERS;
		}
	}
#endif
	if (ADS_ERR_OK(status)) return NT_STATUS_OK;
	return NT_STATUS_UNSUCCESSFUL;
}

/*
  return a string for an error from a ads routine
*/
const char *ads_errstr(ADS_STATUS status)
{
	static char *ret;

	SAFE_FREE(ret);

	switch (status.error_type) {
	case ENUM_ADS_ERROR_SYSTEM:
		return strerror(status.err.rc);
#ifdef HAVE_LDAP
	case ENUM_ADS_ERROR_LDAP:
		return ldap_err2string(status.err.rc);
#endif
#ifdef HAVE_KRB5
	case ENUM_ADS_ERROR_KRB5: 
		return error_message(status.err.rc);
#endif
#ifdef HAVE_GSSAPI
	case ENUM_ADS_ERROR_GSS:
	{
		uint32 msg_ctx;
		uint32 minor;
		gss_buffer_desc msg1, msg2;

		msg_ctx = 0;
		
		msg1.value = NULL;
		msg2.value = NULL;
		gss_display_status(&minor, status.err.rc, GSS_C_GSS_CODE,
				   GSS_C_NULL_OID, &msg_ctx, &msg1);
		gss_display_status(&minor, status.minor_status, GSS_C_MECH_CODE,
				   GSS_C_NULL_OID, &msg_ctx, &msg2);
		asprintf(&ret, "%s : %s", (char *)msg1.value, (char *)msg2.value);
		gss_release_buffer(&minor, &msg1);
		gss_release_buffer(&minor, &msg2);
		return ret;
	}
#endif
	case ENUM_ADS_ERROR_NT: 
		return get_friendly_nt_error_msg(ads_ntstatus(status));
	default:
		return "Unknown ADS error type!? (not compiled in?)";
	}

}


