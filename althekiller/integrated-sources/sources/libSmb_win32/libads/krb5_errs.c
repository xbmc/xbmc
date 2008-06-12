/* 
 *  Unix SMB/CIFS implementation.
 *  Kerberos error mapping functions
 *  Copyright (C) Guenther Deschner 2005
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

#ifdef HAVE_KRB5

static const struct {
	krb5_error_code krb5_code;
	NTSTATUS ntstatus;
} krb5_to_nt_status_map[] = {
	{KRB5_CC_IO, NT_STATUS_UNEXPECTED_IO_ERROR},
	{KRB5KDC_ERR_BADOPTION, NT_STATUS_INVALID_PARAMETER},
	{KRB5KDC_ERR_CLIENT_REVOKED, NT_STATUS_ACCESS_DENIED},
	{KRB5KDC_ERR_C_PRINCIPAL_UNKNOWN, NT_STATUS_INVALID_ACCOUNT_NAME},
	{KRB5KDC_ERR_ETYPE_NOSUPP, NT_STATUS_LOGON_FAILURE},
#if defined(KRB5KDC_ERR_KEY_EXPIRED) /* Heimdal */
	{KRB5KDC_ERR_KEY_EXPIRED, NT_STATUS_PASSWORD_EXPIRED},
#elif defined(KRB5KDC_ERR_KEY_EXP) /* MIT */
	{KRB5KDC_ERR_KEY_EXP, NT_STATUS_PASSWORD_EXPIRED},
#else 
#error Neither KRB5KDC_ERR_KEY_EXPIRED nor KRB5KDC_ERR_KEY_EXP available
#endif
	{25, NT_STATUS_PASSWORD_EXPIRED}, /* FIXME: bug in heimdal 0.7 krb5_get_init_creds_password (Inappropriate ioctl for device (25)) */
	{KRB5KDC_ERR_NULL_KEY, NT_STATUS_LOGON_FAILURE},
	{KRB5KDC_ERR_POLICY, NT_STATUS_INVALID_WORKSTATION},
	{KRB5KDC_ERR_PREAUTH_FAILED, NT_STATUS_LOGON_FAILURE},
	{KRB5KDC_ERR_SERVICE_REVOKED, NT_STATUS_ACCESS_DENIED},
	{KRB5KDC_ERR_S_PRINCIPAL_UNKNOWN, NT_STATUS_INVALID_ACCOUNT_NAME},
	{KRB5KDC_ERR_SUMTYPE_NOSUPP, NT_STATUS_LOGON_FAILURE},
	{KRB5KDC_ERR_TGT_REVOKED, NT_STATUS_ACCESS_DENIED},
	{KRB5_KDC_UNREACH, NT_STATUS_NO_LOGON_SERVERS},
	{KRB5KRB_AP_ERR_BAD_INTEGRITY, NT_STATUS_LOGON_FAILURE},
	{KRB5KRB_AP_ERR_MODIFIED, NT_STATUS_LOGON_FAILURE},
	{KRB5KRB_AP_ERR_SKEW, NT_STATUS_TIME_DIFFERENCE_AT_DC},
	{KRB5KRB_AP_ERR_TKT_EXPIRED, NT_STATUS_LOGON_FAILURE},
	{KRB5KRB_ERR_GENERIC, NT_STATUS_UNSUCCESSFUL},
#if defined(KRB5KRB_ERR_RESPONSE_TOO_BIG)
	{KRB5KRB_ERR_RESPONSE_TOO_BIG, NT_STATUS_PROTOCOL_UNREACHABLE},
#endif
	{0, NT_STATUS_OK}
};

static const struct {
	NTSTATUS ntstatus;
	krb5_error_code krb5_code;
} nt_status_to_krb5_map[] = {
	{NT_STATUS_LOGON_FAILURE, KRB5KDC_ERR_PREAUTH_FAILED},
	{NT_STATUS_NO_LOGON_SERVERS, KRB5_KDC_UNREACH},
	{NT_STATUS_OK, 0}
};

/*****************************************************************************
convert a KRB5 error to a NT status32 code
 *****************************************************************************/
 NTSTATUS krb5_to_nt_status(krb5_error_code kerberos_error)
{
	int i;
	
	if (kerberos_error == 0) {
		return NT_STATUS_OK;
	}
	
	for (i=0; NT_STATUS_V(krb5_to_nt_status_map[i].ntstatus); i++) {
		if (kerberos_error == krb5_to_nt_status_map[i].krb5_code)
			return krb5_to_nt_status_map[i].ntstatus;
	}

	return NT_STATUS_UNSUCCESSFUL;
}

/*****************************************************************************
convert an NT status32 code to a KRB5 error
 *****************************************************************************/
 krb5_error_code nt_status_to_krb5(NTSTATUS nt_status)
{
	int i;
	
	if NT_STATUS_IS_OK(nt_status) {
		return 0;
	}
	
	for (i=0; NT_STATUS_V(nt_status_to_krb5_map[i].ntstatus); i++) {
		if (NT_STATUS_EQUAL(nt_status,nt_status_to_krb5_map[i].ntstatus))
			return nt_status_to_krb5_map[i].krb5_code;
	}

	return KRB5KRB_ERR_GENERIC;
}

#endif

