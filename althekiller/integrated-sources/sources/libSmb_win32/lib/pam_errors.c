/* 
 *  Unix SMB/CIFS implementation.
 *  PAM error mapping functions
 *  Copyright (C) Andrew Bartlett 2002
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

#ifdef WITH_PAM
#include <security/pam_appl.h>

#if defined(PAM_AUTHTOK_RECOVERY_ERR) && !defined(PAM_AUTHTOK_RECOVER_ERR)
#define PAM_AUTHTOK_RECOVER_ERR PAM_AUTHTOK_RECOVERY_ERR
#endif	

/* PAM -> NT_STATUS map */
static const struct {
	int pam_code;
	NTSTATUS ntstatus;
} pam_to_nt_status_map[] = {
	{PAM_OPEN_ERR, NT_STATUS_UNSUCCESSFUL},
	{PAM_SYMBOL_ERR, NT_STATUS_UNSUCCESSFUL},
	{PAM_SERVICE_ERR, NT_STATUS_UNSUCCESSFUL},
	{PAM_SYSTEM_ERR,  NT_STATUS_UNSUCCESSFUL},
	{PAM_BUF_ERR, NT_STATUS_NO_MEMORY},
	{PAM_PERM_DENIED, NT_STATUS_ACCESS_DENIED},
	{PAM_AUTH_ERR, NT_STATUS_WRONG_PASSWORD},
	{PAM_CRED_INSUFFICIENT, NT_STATUS_INSUFFICIENT_LOGON_INFO}, /* FIXME:  Is this correct? */
	{PAM_AUTHINFO_UNAVAIL, NT_STATUS_LOGON_FAILURE},
	{PAM_USER_UNKNOWN, NT_STATUS_NO_SUCH_USER},
	{PAM_MAXTRIES, NT_STATUS_REMOTE_SESSION_LIMIT}, /* FIXME:  Is this correct? */
	{PAM_NEW_AUTHTOK_REQD, NT_STATUS_PASSWORD_MUST_CHANGE},
	{PAM_ACCT_EXPIRED, NT_STATUS_ACCOUNT_EXPIRED},
	{PAM_SESSION_ERR, NT_STATUS_INSUFFICIENT_RESOURCES},
	{PAM_CRED_UNAVAIL, NT_STATUS_NO_TOKEN},  /* FIXME:  Is this correct? */
	{PAM_CRED_EXPIRED, NT_STATUS_PASSWORD_EXPIRED},  /* FIXME:  Is this correct? */
	{PAM_CRED_ERR, NT_STATUS_UNSUCCESSFUL},
	{PAM_AUTHTOK_ERR, NT_STATUS_UNSUCCESSFUL},
#ifdef PAM_AUTHTOK_RECOVER_ERR
	{PAM_AUTHTOK_RECOVER_ERR, NT_STATUS_UNSUCCESSFUL},
#endif
	{PAM_AUTHTOK_EXPIRED, NT_STATUS_PASSWORD_EXPIRED},
	{PAM_SUCCESS, NT_STATUS_OK}
};

/* NT_STATUS -> PAM map */
static const struct {
	NTSTATUS ntstatus;
	int pam_code;
} nt_status_to_pam_map[] = {
	{NT_STATUS_UNSUCCESSFUL, PAM_SYSTEM_ERR},
	{NT_STATUS_NO_SUCH_USER, PAM_USER_UNKNOWN},
	{NT_STATUS_WRONG_PASSWORD, PAM_AUTH_ERR},
	{NT_STATUS_LOGON_FAILURE, PAM_AUTH_ERR},
	{NT_STATUS_ACCOUNT_EXPIRED, PAM_ACCT_EXPIRED},
	{NT_STATUS_PASSWORD_EXPIRED, PAM_AUTHTOK_EXPIRED},
	{NT_STATUS_PASSWORD_MUST_CHANGE, PAM_NEW_AUTHTOK_REQD},
	{NT_STATUS_ACCOUNT_LOCKED_OUT, PAM_MAXTRIES},
	{NT_STATUS_NO_MEMORY, PAM_BUF_ERR},
	{NT_STATUS_PASSWORD_RESTRICTION, PAM_PERM_DENIED},
	{NT_STATUS_OK, PAM_SUCCESS}
};

/*****************************************************************************
convert a PAM error to a NT status32 code
 *****************************************************************************/
NTSTATUS pam_to_nt_status(int pam_error)
{
	int i;
	if (pam_error == 0) return NT_STATUS_OK;
	
	for (i=0; NT_STATUS_V(pam_to_nt_status_map[i].ntstatus); i++) {
		if (pam_error == pam_to_nt_status_map[i].pam_code)
			return pam_to_nt_status_map[i].ntstatus;
	}
	return NT_STATUS_UNSUCCESSFUL;
}

/*****************************************************************************
convert an NT status32 code to a PAM error
 *****************************************************************************/
int nt_status_to_pam(NTSTATUS nt_status)
{
	int i;
	if NT_STATUS_IS_OK(nt_status) return PAM_SUCCESS;
	
	for (i=0; NT_STATUS_V(nt_status_to_pam_map[i].ntstatus); i++) {
		if (NT_STATUS_EQUAL(nt_status,nt_status_to_pam_map[i].ntstatus))
			return nt_status_to_pam_map[i].pam_code;
	}
	return PAM_SYSTEM_ERR;
}

#else 

/*****************************************************************************
convert a PAM error to a NT status32 code
 *****************************************************************************/
NTSTATUS pam_to_nt_status(int pam_error)
{
	if (pam_error == 0) return NT_STATUS_OK;
	return NT_STATUS_UNSUCCESSFUL;
}

/*****************************************************************************
convert an NT status32 code to a PAM error
 *****************************************************************************/
int nt_status_to_pam(NTSTATUS nt_status)
{
	if (NT_STATUS_EQUAL(nt_status, NT_STATUS_OK)) return 0;
	return 4; /* PAM_SYSTEM_ERR */
}

#endif

