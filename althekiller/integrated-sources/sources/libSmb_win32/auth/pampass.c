/* 
   Unix SMB/CIFS implementation.
   PAM Password checking
   Copyright (C) Andrew Tridgell 1992-2001
   Copyright (C) John H Terpsta 1999-2001
   Copyright (C) Andrew Bartlett 2001
   Copyright (C) Jeremy Allison 2001
   
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

/*
 * This module provides PAM based functions for validation of
 * username/password pairs, account managment, session and access control.
 * Note: SMB password checking is done in smbpass.c
 */

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_AUTH

#ifdef WITH_PAM

/*******************************************************************
 * Handle PAM authentication 
 * 	- Access, Authentication, Session, Password
 *   Note: See PAM Documentation and refer to local system PAM implementation
 *   which determines what actions/limitations/allowances become affected.
 *********************************************************************/

#include <security/pam_appl.h>

/*
 * Structure used to communicate between the conversation function
 * and the server_login/change password functions.
 */

struct smb_pam_userdata {
	const char *PAM_username;
	const char *PAM_password;
	const char *PAM_newpassword;
};

typedef int (*smb_pam_conv_fn)(int, const struct pam_message **, struct pam_response **, void *appdata_ptr);

/*
 *  Macros to help make life easy
 */
#define COPY_STRING(s) (s) ? SMB_STRDUP(s) : NULL

/*******************************************************************
 PAM error handler.
 *********************************************************************/

static BOOL smb_pam_error_handler(pam_handle_t *pamh, int pam_error, const char *msg, int dbglvl)
{

	if( pam_error != PAM_SUCCESS) {
		DEBUG(dbglvl, ("smb_pam_error_handler: PAM: %s : %s\n",
				msg, pam_strerror(pamh, pam_error)));
		
		return False;
	}
	return True;
}

/*******************************************************************
 This function is a sanity check, to make sure that we NEVER report
 failure as sucess.
*********************************************************************/

static BOOL smb_pam_nt_status_error_handler(pam_handle_t *pamh, int pam_error,
					    const char *msg, int dbglvl, 
					    NTSTATUS *nt_status)
{
	*nt_status = pam_to_nt_status(pam_error);

	if (smb_pam_error_handler(pamh, pam_error, msg, dbglvl))
		return True;

	if (NT_STATUS_IS_OK(*nt_status)) {
		/* Complain LOUDLY */
		DEBUG(0, ("smb_pam_nt_status_error_handler: PAM: BUG: PAM and NT_STATUS \
error MISMATCH, forcing to NT_STATUS_LOGON_FAILURE"));
		*nt_status = NT_STATUS_LOGON_FAILURE;
	}
	return False;
}

/*
 * PAM conversation function
 * Here we assume (for now, at least) that echo on means login name, and
 * echo off means password.
 */

static int smb_pam_conv(int num_msg,
		    const struct pam_message **msg,
		    struct pam_response **resp,
		    void *appdata_ptr)
{
	int replies = 0;
	struct pam_response *reply = NULL;
	struct smb_pam_userdata *udp = (struct smb_pam_userdata *)appdata_ptr;

	*resp = NULL;

	if (num_msg <= 0)
		return PAM_CONV_ERR;

	/*
	 * Apparantly HPUX has a buggy PAM that doesn't support the
	 * appdata_ptr. Fail if this is the case. JRA.
	 */

	if (udp == NULL) {
		DEBUG(0,("smb_pam_conv: PAM on this system is broken - appdata_ptr == NULL !\n"));
		return PAM_CONV_ERR;
	}

	reply = SMB_MALLOC_ARRAY(struct pam_response, num_msg);
	if (!reply)
		return PAM_CONV_ERR;

	memset(reply, '\0', sizeof(struct pam_response) * num_msg);

	for (replies = 0; replies < num_msg; replies++) {
		switch (msg[replies]->msg_style) {
			case PAM_PROMPT_ECHO_ON:
				reply[replies].resp_retcode = PAM_SUCCESS;
				reply[replies].resp = COPY_STRING(udp->PAM_username);
				/* PAM frees resp */
				break;

			case PAM_PROMPT_ECHO_OFF:
				reply[replies].resp_retcode = PAM_SUCCESS;
				reply[replies].resp = COPY_STRING(udp->PAM_password);
				/* PAM frees resp */
				break;

			case PAM_TEXT_INFO:
				/* fall through */

			case PAM_ERROR_MSG:
				/* ignore it... */
				reply[replies].resp_retcode = PAM_SUCCESS;
				reply[replies].resp = NULL;
				break;

			default:
				/* Must be an error of some sort... */
				SAFE_FREE(reply);
				return PAM_CONV_ERR;
		}
	}
	if (reply)
		*resp = reply;
	return PAM_SUCCESS;
}

/*
 * PAM password change conversation function
 * Here we assume (for now, at least) that echo on means login name, and
 * echo off means password.
 */

static void special_char_sub(char *buf)
{
	all_string_sub(buf, "\\n", "", 0);
	all_string_sub(buf, "\\r", "", 0);
	all_string_sub(buf, "\\s", " ", 0);
	all_string_sub(buf, "\\t", "\t", 0);
}

static void pwd_sub(char *buf, const char *username, const char *oldpass, const char *newpass)
{
	fstring_sub(buf, "%u", username);
	all_string_sub(buf, "%o", oldpass, sizeof(fstring));
	all_string_sub(buf, "%n", newpass, sizeof(fstring));
}


struct chat_struct {
	struct chat_struct *next, *prev;
	fstring prompt;
	fstring reply;
};

/**************************************************************
 Create a linked list containing chat data.
***************************************************************/

static struct chat_struct *make_pw_chat(const char *p) 
{
	fstring prompt;
	fstring reply;
	struct chat_struct *list = NULL;
	struct chat_struct *t;
	struct chat_struct *tmp;

	while (1) {
		t = SMB_MALLOC_P(struct chat_struct);
		if (!t) {
			DEBUG(0,("make_pw_chat: malloc failed!\n"));
			return NULL;
		}

		ZERO_STRUCTP(t);

		DLIST_ADD_END(list, t, tmp);

		if (!next_token(&p, prompt, NULL, sizeof(fstring)))
			break;

		if (strequal(prompt,"."))
			fstrcpy(prompt,"*");

		special_char_sub(prompt);
		fstrcpy(t->prompt, prompt);
		strlower_m(t->prompt);
		trim_char(t->prompt, ' ', ' ');

		if (!next_token(&p, reply, NULL, sizeof(fstring)))
			break;

		if (strequal(reply,"."))
				fstrcpy(reply,"");

		special_char_sub(reply);
		fstrcpy(t->reply, reply);
		strlower_m(t->reply);
		trim_char(t->reply, ' ', ' ');

	}
	return list;
}

static void free_pw_chat(struct chat_struct *list)
{
    while (list) {
        struct chat_struct *old_head = list;
        DLIST_REMOVE(list, list);
        SAFE_FREE(old_head);
    }
}

static int smb_pam_passchange_conv(int num_msg,
				const struct pam_message **msg,
				struct pam_response **resp,
				void *appdata_ptr)
{
	int replies = 0;
	struct pam_response *reply = NULL;
	fstring current_prompt;
	fstring current_reply;
	struct smb_pam_userdata *udp = (struct smb_pam_userdata *)appdata_ptr;
	struct chat_struct *pw_chat= make_pw_chat(lp_passwd_chat());
	struct chat_struct *t;
	BOOL found; 
	*resp = NULL;
	
	DEBUG(10,("smb_pam_passchange_conv: starting converstation for %d messages\n", num_msg));

	if (num_msg <= 0)
		return PAM_CONV_ERR;

	if (pw_chat == NULL)
		return PAM_CONV_ERR;

	/*
	 * Apparantly HPUX has a buggy PAM that doesn't support the
	 * appdata_ptr. Fail if this is the case. JRA.
	 */

	if (udp == NULL) {
		DEBUG(0,("smb_pam_passchange_conv: PAM on this system is broken - appdata_ptr == NULL !\n"));
		free_pw_chat(pw_chat);
		return PAM_CONV_ERR;
	}

	reply = SMB_MALLOC_ARRAY(struct pam_response, num_msg);
	if (!reply) {
		DEBUG(0,("smb_pam_passchange_conv: malloc for reply failed!\n"));
		free_pw_chat(pw_chat);
		return PAM_CONV_ERR;
	}

	for (replies = 0; replies < num_msg; replies++) {
		found = False;
		DEBUG(10,("smb_pam_passchange_conv: Processing message %d\n", replies));
		switch (msg[replies]->msg_style) {
		case PAM_PROMPT_ECHO_ON:
			DEBUG(10,("smb_pam_passchange_conv: PAM_PROMPT_ECHO_ON: PAM said: %s\n", msg[replies]->msg));
			fstrcpy(current_prompt, msg[replies]->msg);
			trim_char(current_prompt, ' ', ' ');
			for (t=pw_chat; t; t=t->next) {

				DEBUG(10,("smb_pam_passchange_conv: PAM_PROMPT_ECHO_ON: trying to match |%s| to |%s|\n",
						t->prompt, current_prompt ));

				if (unix_wild_match(t->prompt, current_prompt)) {
					fstrcpy(current_reply, t->reply);
					DEBUG(10,("smb_pam_passchange_conv: PAM_PROMPT_ECHO_ON: We sent: %s\n", current_reply));
					pwd_sub(current_reply, udp->PAM_username, udp->PAM_password, udp->PAM_newpassword);
#ifdef DEBUG_PASSWORD
					DEBUG(100,("smb_pam_passchange_conv: PAM_PROMPT_ECHO_ON: We actualy sent: %s\n", current_reply));
#endif
					reply[replies].resp_retcode = PAM_SUCCESS;
					reply[replies].resp = COPY_STRING(current_reply);
					found = True;
					break;
				}
			}
			/* PAM frees resp */
			if (!found) {
				DEBUG(3,("smb_pam_passchange_conv: Could not find reply for PAM prompt: %s\n",msg[replies]->msg));
				free_pw_chat(pw_chat);
				SAFE_FREE(reply);
				return PAM_CONV_ERR;
			}
			break;

		case PAM_PROMPT_ECHO_OFF:
			DEBUG(10,("smb_pam_passchange_conv: PAM_PROMPT_ECHO_OFF: PAM said: %s\n", msg[replies]->msg));
			fstrcpy(current_prompt, msg[replies]->msg);
			trim_char(current_prompt, ' ', ' ');
			for (t=pw_chat; t; t=t->next) {

				DEBUG(10,("smb_pam_passchange_conv: PAM_PROMPT_ECHO_OFF: trying to match |%s| to |%s|\n",
						t->prompt, current_prompt ));

				if (unix_wild_match(t->prompt, current_prompt)) {
					fstrcpy(current_reply, t->reply);
					DEBUG(10,("smb_pam_passchange_conv: PAM_PROMPT_ECHO_OFF: We sent: %s\n", current_reply));
					pwd_sub(current_reply, udp->PAM_username, udp->PAM_password, udp->PAM_newpassword);
					reply[replies].resp_retcode = PAM_SUCCESS;
					reply[replies].resp = COPY_STRING(current_reply);
#ifdef DEBUG_PASSWORD
					DEBUG(100,("smb_pam_passchange_conv: PAM_PROMPT_ECHO_OFF: We actualy sent: %s\n", current_reply));
#endif
					found = True;
					break;
				}
			}
			/* PAM frees resp */
			
			if (!found) {
				DEBUG(3,("smb_pam_passchange_conv: Could not find reply for PAM prompt: %s\n",msg[replies]->msg));
				free_pw_chat(pw_chat);
				SAFE_FREE(reply);
				return PAM_CONV_ERR;
			}
			break;

		case PAM_TEXT_INFO:
			/* fall through */

		case PAM_ERROR_MSG:
			/* ignore it... */
			reply[replies].resp_retcode = PAM_SUCCESS;
			reply[replies].resp = NULL;
			break;
			
		default:
			/* Must be an error of some sort... */
			free_pw_chat(pw_chat);
			SAFE_FREE(reply);
			return PAM_CONV_ERR;
		}
	}
		
	free_pw_chat(pw_chat);
	if (reply)
		*resp = reply;
	return PAM_SUCCESS;
}

/***************************************************************************
 Free up a malloced pam_conv struct.
****************************************************************************/

static void smb_free_pam_conv(struct pam_conv *pconv)
{
	if (pconv)
		SAFE_FREE(pconv->appdata_ptr);

	SAFE_FREE(pconv);
}

/***************************************************************************
 Allocate a pam_conv struct.
****************************************************************************/

static struct pam_conv *smb_setup_pam_conv(smb_pam_conv_fn smb_pam_conv_fnptr, const char *user,
					const char *passwd, const char *newpass)
{
	struct pam_conv *pconv = SMB_MALLOC_P(struct pam_conv);
	struct smb_pam_userdata *udp = SMB_MALLOC_P(struct smb_pam_userdata);

	if (pconv == NULL || udp == NULL) {
		SAFE_FREE(pconv);
		SAFE_FREE(udp);
		return NULL;
	}

	udp->PAM_username = user;
	udp->PAM_password = passwd;
	udp->PAM_newpassword = newpass;

	pconv->conv = smb_pam_conv_fnptr;
	pconv->appdata_ptr = (void *)udp;
	return pconv;
}

/* 
 * PAM Closing out cleanup handler
 */

static BOOL smb_pam_end(pam_handle_t *pamh, struct pam_conv *smb_pam_conv_ptr)
{
	int pam_error;

	smb_free_pam_conv(smb_pam_conv_ptr);
       
	if( pamh != NULL ) {
		pam_error = pam_end(pamh, 0);
		if(smb_pam_error_handler(pamh, pam_error, "End Cleanup Failed", 2) == True) {
			DEBUG(4, ("smb_pam_end: PAM: PAM_END OK.\n"));
			return True;
		}
	}
	DEBUG(2,("smb_pam_end: PAM: not initialised"));
	return False;
}

/*
 * Start PAM authentication for specified account
 */

static BOOL smb_pam_start(pam_handle_t **pamh, const char *user, const char *rhost, struct pam_conv *pconv)
{
	int pam_error;
	const char *our_rhost;

	*pamh = (pam_handle_t *)NULL;

	DEBUG(4,("smb_pam_start: PAM: Init user: %s\n", user));

	pam_error = pam_start("samba", user, pconv, pamh);
	if( !smb_pam_error_handler(*pamh, pam_error, "Init Failed", 0)) {
		*pamh = (pam_handle_t *)NULL;
		return False;
	}

	if (rhost == NULL) {
		our_rhost = client_name();
		if (strequal(our_rhost,"UNKNOWN"))
			our_rhost = client_addr();
	} else {
		our_rhost = rhost;
	}

#ifdef PAM_RHOST
	DEBUG(4,("smb_pam_start: PAM: setting rhost to: %s\n", our_rhost));
	pam_error = pam_set_item(*pamh, PAM_RHOST, our_rhost);
	if(!smb_pam_error_handler(*pamh, pam_error, "set rhost failed", 0)) {
		smb_pam_end(*pamh, pconv);
		*pamh = (pam_handle_t *)NULL;
		return False;
	}
#endif
#ifdef PAM_TTY
	DEBUG(4,("smb_pam_start: PAM: setting tty\n"));
	pam_error = pam_set_item(*pamh, PAM_TTY, "samba");
	if (!smb_pam_error_handler(*pamh, pam_error, "set tty failed", 0)) {
		smb_pam_end(*pamh, pconv);
		*pamh = (pam_handle_t *)NULL;
		return False;
	}
#endif
	DEBUG(4,("smb_pam_start: PAM: Init passed for user: %s\n", user));
	return True;
}

/*
 * PAM Authentication Handler
 */
static NTSTATUS smb_pam_auth(pam_handle_t *pamh, const char *user)
{
	int pam_error;
	NTSTATUS nt_status = NT_STATUS_LOGON_FAILURE;

	/*
	 * To enable debugging set in /etc/pam.d/samba:
	 *	auth required /lib/security/pam_pwdb.so nullok shadow audit
	 */
	
	DEBUG(4,("smb_pam_auth: PAM: Authenticate User: %s\n", user));
	pam_error = pam_authenticate(pamh, PAM_SILENT | lp_null_passwords() ? 0 : PAM_DISALLOW_NULL_AUTHTOK);
	switch( pam_error ){
		case PAM_AUTH_ERR:
			DEBUG(2, ("smb_pam_auth: PAM: Athentication Error for user %s\n", user));
			break;
		case PAM_CRED_INSUFFICIENT:
			DEBUG(2, ("smb_pam_auth: PAM: Insufficient Credentials for user %s\n", user));
			break;
		case PAM_AUTHINFO_UNAVAIL:
			DEBUG(2, ("smb_pam_auth: PAM: Authentication Information Unavailable for user %s\n", user));
			break;
		case PAM_USER_UNKNOWN:
			DEBUG(2, ("smb_pam_auth: PAM: Username %s NOT known to Authentication system\n", user));
			break;
		case PAM_MAXTRIES:
			DEBUG(2, ("smb_pam_auth: PAM: One or more authentication modules reports user limit for user %s exceeeded\n", user));
			break;
		case PAM_ABORT:
			DEBUG(0, ("smb_pam_auth: PAM: One or more PAM modules failed to load for user %s\n", user));
			break;
		case PAM_SUCCESS:
			DEBUG(4, ("smb_pam_auth: PAM: User %s Authenticated OK\n", user));
			break;
		default:
			DEBUG(0, ("smb_pam_auth: PAM: UNKNOWN ERROR while authenticating user %s\n", user));
			break;
	}

	smb_pam_nt_status_error_handler(pamh, pam_error, "Authentication Failure", 2, &nt_status);
	return nt_status;
}

/* 
 * PAM Account Handler
 */
static NTSTATUS smb_pam_account(pam_handle_t *pamh, const char * user)
{
	int pam_error;
	NTSTATUS nt_status = NT_STATUS_ACCOUNT_DISABLED;

	DEBUG(4,("smb_pam_account: PAM: Account Management for User: %s\n", user));
	pam_error = pam_acct_mgmt(pamh, PAM_SILENT); /* Is user account enabled? */
	switch( pam_error ) {
		case PAM_AUTHTOK_EXPIRED:
			DEBUG(2, ("smb_pam_account: PAM: User %s is valid but password is expired\n", user));
			break;
		case PAM_ACCT_EXPIRED:
			DEBUG(2, ("smb_pam_account: PAM: User %s no longer permitted to access system\n", user));
			break;
		case PAM_AUTH_ERR:
			DEBUG(2, ("smb_pam_account: PAM: There was an authentication error for user %s\n", user));
			break;
		case PAM_PERM_DENIED:
			DEBUG(0, ("smb_pam_account: PAM: User %s is NOT permitted to access system at this time\n", user));
			break;
		case PAM_USER_UNKNOWN:
			DEBUG(0, ("smb_pam_account: PAM: User \"%s\" is NOT known to account management\n", user));
			break;
		case PAM_SUCCESS:
			DEBUG(4, ("smb_pam_account: PAM: Account OK for User: %s\n", user));
			break;
		default:
			DEBUG(0, ("smb_pam_account: PAM: UNKNOWN PAM ERROR (%d) during Account Management for User: %s\n", pam_error, user));
			break;
	}

	smb_pam_nt_status_error_handler(pamh, pam_error, "Account Check Failed", 2, &nt_status);
	return nt_status;
}

/*
 * PAM Credential Setting
 */

static NTSTATUS smb_pam_setcred(pam_handle_t *pamh, const char * user)
{
	int pam_error;
	NTSTATUS nt_status = NT_STATUS_NO_TOKEN;

	/*
	 * This will allow samba to aquire a kerberos token. And, when
	 * exporting an AFS cell, be able to /write/ to this cell.
	 */

	DEBUG(4,("PAM: Account Management SetCredentials for User: %s\n", user));
	pam_error = pam_setcred(pamh, (PAM_ESTABLISH_CRED|PAM_SILENT)); 
	switch( pam_error ) {
		case PAM_CRED_UNAVAIL:
			DEBUG(0, ("smb_pam_setcred: PAM: Credentials not found for user:%s\n", user ));
			break;
		case PAM_CRED_EXPIRED:
			DEBUG(0, ("smb_pam_setcred: PAM: Credentials for user: \"%s\" EXPIRED!\n", user ));
			break;
		case PAM_USER_UNKNOWN:
			DEBUG(0, ("smb_pam_setcred: PAM: User: \"%s\" is NOT known so can not set credentials!\n", user ));
			break;
		case PAM_CRED_ERR:
			DEBUG(0, ("smb_pam_setcred: PAM: Unknown setcredentials error - unable to set credentials for %s\n", user ));
			break;
		case PAM_SUCCESS:
			DEBUG(4, ("smb_pam_setcred: PAM: SetCredentials OK for User: %s\n", user));
			break;
		default:
			DEBUG(0, ("smb_pam_setcred: PAM: UNKNOWN PAM ERROR (%d) during SetCredentials for User: %s\n", pam_error, user));
			break;
	}

	smb_pam_nt_status_error_handler(pamh, pam_error, "Set Credential Failure", 2, &nt_status);
	return nt_status;
}

/*
 * PAM Internal Session Handler
 */
static BOOL smb_internal_pam_session(pam_handle_t *pamh, const char *user, const char *tty, BOOL flag)
{
	int pam_error;

#ifdef PAM_TTY
	DEBUG(4,("smb_internal_pam_session: PAM: tty set to: %s\n", tty));
	pam_error = pam_set_item(pamh, PAM_TTY, tty);
	if (!smb_pam_error_handler(pamh, pam_error, "set tty failed", 0))
		return False;
#endif

	if (flag) {
		pam_error = pam_open_session(pamh, PAM_SILENT);
		if (!smb_pam_error_handler(pamh, pam_error, "session setup failed", 0))
			return False;
	} else {
		pam_setcred(pamh, (PAM_DELETE_CRED|PAM_SILENT)); /* We don't care if this fails */
		pam_error = pam_close_session(pamh, PAM_SILENT); /* This will probably pick up the error anyway */
		if (!smb_pam_error_handler(pamh, pam_error, "session close failed", 0))
			return False;
	}
	return (True);
}

/*
 * Internal PAM Password Changer.
 */

static BOOL smb_pam_chauthtok(pam_handle_t *pamh, const char * user)
{
	int pam_error;

	DEBUG(4,("smb_pam_chauthtok: PAM: Password Change for User: %s\n", user));

	pam_error = pam_chauthtok(pamh, PAM_SILENT); /* Change Password */

	switch( pam_error ) {
	case PAM_AUTHTOK_ERR:
		DEBUG(2, ("PAM: unable to obtain the new authentication token - is password to weak?\n"));
		break;

	/* This doesn't seem to be defined on Solaris. JRA */
#ifdef PAM_AUTHTOK_RECOVER_ERR
	case PAM_AUTHTOK_RECOVER_ERR:
		DEBUG(2, ("PAM: unable to obtain the old authentication token - was the old password wrong?.\n"));
		break;
#endif

	case PAM_AUTHTOK_LOCK_BUSY:
		DEBUG(2, ("PAM: unable to change the authentication token since it is currently locked.\n"));
		break;
	case PAM_AUTHTOK_DISABLE_AGING:
		DEBUG(2, ("PAM: Authentication token aging has been disabled.\n"));
		break;
	case PAM_PERM_DENIED:
		DEBUG(0, ("PAM: Permission denied.\n"));
		break;
	case PAM_TRY_AGAIN:
		DEBUG(0, ("PAM: Could not update all authentication token(s). No authentication tokens were updated.\n"));
		break;
	case PAM_USER_UNKNOWN:
		DEBUG(0, ("PAM: User not known to PAM\n"));
		break;
	case PAM_SUCCESS:
		DEBUG(4, ("PAM: Account OK for User: %s\n", user));
		break;
	default:
		DEBUG(0, ("PAM: UNKNOWN PAM ERROR (%d) for User: %s\n", pam_error, user));
	}
 
	if(!smb_pam_error_handler(pamh, pam_error, "Password Change Failed", 2)) {
		return False;
	}

	/* If this point is reached, the password has changed. */
	return True;
}

/*
 * PAM Externally accessible Session handler
 */

BOOL smb_pam_claim_session(char *user, char *tty, char *rhost)
{
	pam_handle_t *pamh = NULL;
	struct pam_conv *pconv = NULL;

	/* Ignore PAM if told to. */

	if (!lp_obey_pam_restrictions())
		return True;

	if ((pconv = smb_setup_pam_conv(smb_pam_conv, user, NULL, NULL)) == NULL)
		return False;

	if (!smb_pam_start(&pamh, user, rhost, pconv))
		return False;

	if (!smb_internal_pam_session(pamh, user, tty, True)) {
		smb_pam_end(pamh, pconv);
		return False;
	}

	return smb_pam_end(pamh, pconv);
}

/*
 * PAM Externally accessible Session handler
 */

BOOL smb_pam_close_session(char *user, char *tty, char *rhost)
{
	pam_handle_t *pamh = NULL;
	struct pam_conv *pconv = NULL;

	/* Ignore PAM if told to. */

	if (!lp_obey_pam_restrictions())
		return True;

	if ((pconv = smb_setup_pam_conv(smb_pam_conv, user, NULL, NULL)) == NULL)
		return False;

	if (!smb_pam_start(&pamh, user, rhost, pconv))
		return False;

	if (!smb_internal_pam_session(pamh, user, tty, False)) {
		smb_pam_end(pamh, pconv);
		return False;
	}

	return smb_pam_end(pamh, pconv);
}

/*
 * PAM Externally accessible Account handler
 */

NTSTATUS smb_pam_accountcheck(const char * user)
{
	NTSTATUS nt_status = NT_STATUS_ACCOUNT_DISABLED;
	pam_handle_t *pamh = NULL;
	struct pam_conv *pconv = NULL;

	/* Ignore PAM if told to. */

	if (!lp_obey_pam_restrictions())
		return NT_STATUS_OK;

	if ((pconv = smb_setup_pam_conv(smb_pam_conv, user, NULL, NULL)) == NULL)
		return NT_STATUS_NO_MEMORY;

	if (!smb_pam_start(&pamh, user, NULL, pconv))
		return NT_STATUS_ACCOUNT_DISABLED;

	if (!NT_STATUS_IS_OK(nt_status = smb_pam_account(pamh, user)))
		DEBUG(0, ("smb_pam_accountcheck: PAM: Account Validation Failed - Rejecting User %s!\n", user));

	smb_pam_end(pamh, pconv);
	return nt_status;
}

/*
 * PAM Password Validation Suite
 */

NTSTATUS smb_pam_passcheck(const char * user, const char * password)
{
	pam_handle_t *pamh = NULL;
	NTSTATUS nt_status = NT_STATUS_LOGON_FAILURE;
	struct pam_conv *pconv = NULL;

	/*
	 * Note we can't ignore PAM here as this is the only
	 * way of doing auths on plaintext passwords when
	 * compiled --with-pam.
	 */

	if ((pconv = smb_setup_pam_conv(smb_pam_conv, user, password, NULL)) == NULL)
		return NT_STATUS_LOGON_FAILURE;

	if (!smb_pam_start(&pamh, user, NULL, pconv))
		return NT_STATUS_LOGON_FAILURE;

	if (!NT_STATUS_IS_OK(nt_status = smb_pam_auth(pamh, user))) {
		DEBUG(0, ("smb_pam_passcheck: PAM: smb_pam_auth failed - Rejecting User %s !\n", user));
		smb_pam_end(pamh, pconv);
		return nt_status;
	}

	if (!NT_STATUS_IS_OK(nt_status = smb_pam_account(pamh, user))) {
		DEBUG(0, ("smb_pam_passcheck: PAM: smb_pam_account failed - Rejecting User %s !\n", user));
		smb_pam_end(pamh, pconv);
		return nt_status;
	}

	if (!NT_STATUS_IS_OK(nt_status = smb_pam_setcred(pamh, user))) {
		DEBUG(0, ("smb_pam_passcheck: PAM: smb_pam_setcred failed - Rejecting User %s !\n", user));
		smb_pam_end(pamh, pconv);
		return nt_status;
	}

	smb_pam_end(pamh, pconv);
	return nt_status;
}

/*
 * PAM Password Change Suite
 */

BOOL smb_pam_passchange(const char * user, const char * oldpassword, const char * newpassword)
{
	/* Appropriate quantities of root should be obtained BEFORE calling this function */
	struct pam_conv *pconv = NULL;
	pam_handle_t *pamh = NULL;

	if ((pconv = smb_setup_pam_conv(smb_pam_passchange_conv, user, oldpassword, newpassword)) == NULL)
		return False;

	if(!smb_pam_start(&pamh, user, NULL, pconv))
		return False;

	if (!smb_pam_chauthtok(pamh, user)) {
		DEBUG(0, ("smb_pam_passchange: PAM: Password Change Failed for user %s!\n", user));
		smb_pam_end(pamh, pconv);
		return False;
	}

	return smb_pam_end(pamh, pconv);
}

#else

/* If PAM not used, no PAM restrictions on accounts. */
NTSTATUS smb_pam_accountcheck(const char * user)
{
	return NT_STATUS_OK;
}

/* If PAM not used, also no PAM restrictions on sessions. */
BOOL smb_pam_claim_session(char *user, char *tty, char *rhost)
{
	return True;
}

/* If PAM not used, also no PAM restrictions on sessions. */
BOOL smb_pam_close_session(char *in_user, char *tty, char *rhost)
{
	return True;
}
#endif /* WITH_PAM */
