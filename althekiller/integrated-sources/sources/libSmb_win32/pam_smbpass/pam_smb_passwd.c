/*
   Unix SMB/CIFS implementation.
   Use PAM to update user passwords in the local SAM
   Copyright (C) Steve Langasek		1998-2003
   
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

/* indicate the following groups are defined */
#define PAM_SM_PASSWORD

#include "includes.h"

/* This is only used in the Sun implementation.  FIXME: we really
   want a define here that distinguishes between the Solaris PAM
   and others (including FreeBSD). */

#ifndef LINUX
#include <security/pam_appl.h>
#endif

#include <security/pam_modules.h>

#include "general.h" 

#include "support.h"

int smb_update_db( pam_handle_t *pamh, int ctrl, const char *user,  const char *pass_new )
{
	int retval;
	pstring err_str;
	pstring msg_str;

	err_str[0] = '\0';
	msg_str[0] = '\0';

	retval = NT_STATUS_IS_OK(local_password_change( user, LOCAL_SET_PASSWORD, pass_new,
	                                err_str, sizeof(err_str),
	                                msg_str, sizeof(msg_str) ));

	if (!retval) {
		if (*err_str) {
			err_str[PSTRING_LEN-1] = '\0';
			make_remark( pamh, ctrl, PAM_ERROR_MSG, err_str );
		}

		/* FIXME: what value is appropriate here? */
		retval = PAM_AUTHTOK_ERR;
	} else {
		if (*msg_str) {
			msg_str[PSTRING_LEN-1] = '\0';
			make_remark( pamh, ctrl, PAM_TEXT_INFO, msg_str );
		}
		retval = PAM_SUCCESS;
	}

	return retval;      
}


/* data tokens */

#define _SMB_OLD_AUTHTOK  "-SMB-OLD-PASS"
#define _SMB_NEW_AUTHTOK  "-SMB-NEW-PASS"

/*
 * FUNCTION: pam_sm_chauthtok()
 *
 * This function is called twice by the PAM library, once with
 * PAM_PRELIM_CHECK set, and then again with PAM_UPDATE_AUTHTOK set.  With
 * Linux-PAM, these two passes generally involve first checking the old
 * token and then changing the token.  This is what we do here.
 *
 * Having obtained a new password. The function updates the
 * SMB_PASSWD_FILE file (normally, $(LIBDIR)/smbpasswd).
 */

int pam_sm_chauthtok(pam_handle_t *pamh, int flags,
                     int argc, const char **argv)
{
    unsigned int ctrl;
    int retval;

    extern BOOL in_client;

    struct samu *sampass = NULL;
    void (*oldsig_handler)(int);
    const char *user;
    char *pass_old;
    char *pass_new;

    /* Samba initialization. */
    load_case_tables();
    setup_logging( "pam_smbpass", False );
    in_client = True;

    ctrl = set_ctrl(flags, argc, argv);

    /*
     * First get the name of a user.  No need to do anything if we can't
     * determine this.
     */

    retval = pam_get_user( pamh, &user, "Username: " );
    if (retval != PAM_SUCCESS) {
        if (on( SMB_DEBUG, ctrl )) {
            _log_err( LOG_DEBUG, "password: could not identify user" );
        }
        return retval;
    }
    if (on( SMB_DEBUG, ctrl )) {
        _log_err( LOG_DEBUG, "username [%s] obtained", user );
    }

    /* Getting into places that might use LDAP -- protect the app
       from a SIGPIPE it's not expecting */
    oldsig_handler = CatchSignal(SIGPIPE, SIGNAL_CAST SIG_IGN);

    if (!initialize_password_db(False)) {
        _log_err( LOG_ALERT, "Cannot access samba password database" );
        CatchSignal(SIGPIPE, SIGNAL_CAST oldsig_handler);
        return PAM_AUTHINFO_UNAVAIL;
    }

    /* obtain user record */
    if ( !(sampass = samu_new( NULL )) ) {
        CatchSignal(SIGPIPE, SIGNAL_CAST oldsig_handler);
        return nt_status_to_pam(NT_STATUS_NO_MEMORY);
    }

    if (!pdb_getsampwnam(sampass,user)) {
        _log_err( LOG_ALERT, "Failed to find entry for user %s.", user );
        CatchSignal(SIGPIPE, SIGNAL_CAST oldsig_handler);
        return PAM_USER_UNKNOWN;
    }
    if (on( SMB_DEBUG, ctrl )) {
        _log_err( LOG_DEBUG, "Located account for %s", user );
    }

    if (flags & PAM_PRELIM_CHECK) {
        /*
         * obtain and verify the current password (OLDAUTHTOK) for
         * the user.
         */

        char *Announce;

        if (_smb_blankpasswd( ctrl, sampass )) {

            TALLOC_FREE(sampass);
            CatchSignal(SIGPIPE, SIGNAL_CAST oldsig_handler);
            return PAM_SUCCESS;
        }

	/* Password change by root, or for an expired token, doesn't
           require authentication.  Is this a good choice? */
        if (getuid() != 0 && !(flags & PAM_CHANGE_EXPIRED_AUTHTOK)) {

            /* tell user what is happening */
#define greeting "Changing password for "
            Announce = SMB_MALLOC_ARRAY(char, sizeof(greeting)+strlen(user));
            if (Announce == NULL) {
                _log_err(LOG_CRIT, "password: out of memory");
                TALLOC_FREE(sampass);
                CatchSignal(SIGPIPE, SIGNAL_CAST oldsig_handler);
                return PAM_BUF_ERR;
            }
            strncpy( Announce, greeting, sizeof(greeting) );
            strncpy( Announce+sizeof(greeting)-1, user, strlen(user)+1 );
#undef greeting

            set( SMB__OLD_PASSWD, ctrl );
            retval = _smb_read_password( pamh, ctrl, Announce, "Current SMB password: ",
                                         NULL, _SMB_OLD_AUTHTOK, &pass_old );
            SAFE_FREE( Announce );

            if (retval != PAM_SUCCESS) {
                _log_err( LOG_NOTICE
                          , "password - (old) token not obtained" );
                TALLOC_FREE(sampass);
                CatchSignal(SIGPIPE, SIGNAL_CAST oldsig_handler);
                return retval;
            }

            /* verify that this is the password for this user */

            retval = _smb_verify_password( pamh, sampass, pass_old, ctrl );

        } else {
	    pass_old = NULL;
            retval = PAM_SUCCESS;           /* root doesn't have to */
        }

        pass_old = NULL;
        TALLOC_FREE(sampass);
        CatchSignal(SIGPIPE, SIGNAL_CAST oldsig_handler);
        return retval;

    } else if (flags & PAM_UPDATE_AUTHTOK) {

        /*
         * obtain the proposed password
         */

        /*
         * get the old token back. NULL was ok only if root [at this
         * point we assume that this has already been enforced on a
         * previous call to this function].
         */

        if (off( SMB_NOT_SET_PASS, ctrl )) {
            retval = pam_get_item( pamh, PAM_OLDAUTHTOK,
                                   (const void **)&pass_old );
        } else {
            retval = pam_get_data( pamh, _SMB_OLD_AUTHTOK,
                                   (const void **)&pass_old );
            if (retval == PAM_NO_MODULE_DATA) {
		pass_old = NULL;
                retval = PAM_SUCCESS;
            }
        }

        if (retval != PAM_SUCCESS) {
            _log_err( LOG_NOTICE, "password: user not authenticated" );
            TALLOC_FREE(sampass);
            CatchSignal(SIGPIPE, SIGNAL_CAST oldsig_handler);
            return retval;
        }

        /*
         * use_authtok is to force the use of a previously entered
         * password -- needed for pluggable password strength checking
	 * or other module stacking
         */

        if (on( SMB_USE_AUTHTOK, ctrl )) {
            set( SMB_USE_FIRST_PASS, ctrl );
        }

        retval = _smb_read_password( pamh, ctrl
                                      , NULL
                                      , "Enter new SMB password: "
                                      , "Retype new SMB password: "
                                      , _SMB_NEW_AUTHTOK
                                      , &pass_new );

        if (retval != PAM_SUCCESS) {
            if (on( SMB_DEBUG, ctrl )) {
                _log_err( LOG_ALERT
                          , "password: new password not obtained" );
            }
            pass_old = NULL;                               /* tidy up */
            TALLOC_FREE(sampass);
            CatchSignal(SIGPIPE, SIGNAL_CAST oldsig_handler);
            return retval;
        }

        /*
         * At this point we know who the user is and what they
         * propose as their new password. Verify that the new
         * password is acceptable.
         */ 

        if (pass_new[0] == '\0') {     /* "\0" password = NULL */
            pass_new = NULL;
        }

        retval = _pam_smb_approve_pass(pamh, ctrl, pass_old, pass_new);

        if (retval != PAM_SUCCESS) {
            _log_err(LOG_NOTICE, "new password not acceptable");
            pass_new = pass_old = NULL;               /* tidy up */
            TALLOC_FREE(sampass);
            CatchSignal(SIGPIPE, SIGNAL_CAST oldsig_handler);
            return retval;
        }

        /*
         * By reaching here we have approved the passwords and must now
         * rebuild the smb password file.
         */

        /* update the password database */

        retval = smb_update_db(pamh, ctrl, user, pass_new);
        if (retval == PAM_SUCCESS) {
	    uid_t uid;
	    
            /* password updated */
		if (!sid_to_uid(pdb_get_user_sid(sampass), &uid)) {
			_log_err( LOG_NOTICE, "Unable to get uid for user %s",
				pdb_get_username(sampass));
			_log_err( LOG_NOTICE, "password for (%s) changed by (%s/%d)",
				user, uidtoname(getuid()), getuid());
		} else {
			_log_err( LOG_NOTICE, "password for (%s/%d) changed by (%s/%d)",
				user, uid, uidtoname(getuid()), getuid());
		}
	} else {
		_log_err( LOG_ERR, "password change failed for user %s", user);
	}

        pass_old = pass_new = NULL;
	if (sampass) {
		TALLOC_FREE(sampass);
		sampass = NULL;
	}

    } else {            /* something has broken with the library */

        _log_err( LOG_ALERT, "password received unknown request" );
        retval = PAM_ABORT;

    }
    
    if (sampass) {
    	TALLOC_FREE(sampass);
	sampass = NULL;
    }

    TALLOC_FREE(sampass);
    CatchSignal(SIGPIPE, SIGNAL_CAST oldsig_handler);
    return retval;
}

/* static module data */
#ifdef PAM_STATIC
struct pam_module _pam_smbpass_passwd_modstruct = {
     "pam_smbpass",
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     pam_sm_chauthtok
};
#endif

