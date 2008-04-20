	/* Unix NT password database implementation, version 0.6.
	 *
	 * This program is free software; you can redistribute it and/or modify it under
	 * the terms of the GNU General Public License as published by the Free
	 * Software Foundation; either version 2 of the License, or (at your option)
	 * any later version.
	 *
	 * This program is distributed in the hope that it will be useful, but WITHOUT
	 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
	 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
	 * more details.
	 *
	 * You should have received a copy of the GNU General Public License along with
	 * this program; if not, write to the Free Software Foundation, Inc., 675
	 * Mass Ave, Cambridge, MA 02139, USA.
	 */

	#include "includes.h"
	#include "general.h"

	#include "support.h"


	#define _pam_overwrite(x)        \
	do {                             \
	     register char *__xx__;      \
	     if ((__xx__=(x)))           \
		  while (*__xx__)        \
		       *__xx__++ = '\0'; \
	} while (0)

	/*
	 * Don't just free it, forget it too.
	 */

	#define _pam_drop(X) \
	do {                 \
	    if (X) {         \
		free(X);     \
		X=NULL;      \
	    }                \
	} while (0)

	#define _pam_drop_reply(/* struct pam_response * */ reply, /* int */ replies) \
	do {                                              \
	    int reply_i;                                  \
							  \
	    for (reply_i=0; reply_i<replies; ++reply_i) { \
		if (reply[reply_i].resp) {                \
		    _pam_overwrite(reply[reply_i].resp);  \
		    free(reply[reply_i].resp);            \
		}                                         \
	    }                                             \
	    if (reply)                                    \
		free(reply);                              \
	} while (0)


	int converse(pam_handle_t *, int, int, struct pam_message **,
				 struct pam_response **);
	int make_remark(pam_handle_t *, unsigned int, int, const char *);
	void _cleanup(pam_handle_t *, void *, int);
	char *_pam_delete(register char *);

	/* default configuration file location */

	char *servicesf = dyn_CONFIGFILE;

	/* syslogging function for errors and other information */

	void _log_err( int err, const char *format, ... )
	{
	    va_list args;

	    va_start( args, format );
	    openlog( "PAM_smbpass", LOG_CONS | LOG_PID, LOG_AUTH );
	    vsyslog( err, format, args );
	    va_end( args );
	    closelog();
	}

	/* this is a front-end for module-application conversations */

	int converse( pam_handle_t * pamh, int ctrl, int nargs
		      , struct pam_message **message
		      , struct pam_response **response )
	{
		int retval;
		struct pam_conv *conv;

		retval = pam_get_item(pamh, PAM_CONV, (const void **) &conv);
		if (retval == PAM_SUCCESS) {

			retval = conv->conv(nargs, (const struct pam_message **) message
								,response, conv->appdata_ptr);

			if (retval != PAM_SUCCESS && on(SMB_DEBUG, ctrl)) {
				_log_err(LOG_DEBUG, "conversation failure [%s]"
						 ,pam_strerror(pamh, retval));
			}
		} else {
			_log_err(LOG_ERR, "couldn't obtain coversation function [%s]"
					 ,pam_strerror(pamh, retval));
		}

		return retval;				/* propagate error status */
	}

	int make_remark( pam_handle_t * pamh, unsigned int ctrl
			 , int type, const char *text )
	{
		if (off(SMB__QUIET, ctrl)) {
			struct pam_message *pmsg[1], msg[1];
			struct pam_response *resp;

			pmsg[0] = &msg[0];
			msg[0].msg = CONST_DISCARD(char *, text);
			msg[0].msg_style = type;
			resp = NULL;

			return converse(pamh, ctrl, 1, pmsg, &resp);
		}
		return PAM_SUCCESS;
	}


	/* set the control flags for the SMB module. */

int set_ctrl( int flags, int argc, const char **argv )
{
    int i = 0;
    const char *service_file = dyn_CONFIGFILE;
    unsigned int ctrl;

    ctrl = SMB_DEFAULTS;	/* the default selection of options */

    /* set some flags manually */

    /* A good, sane default (matches Samba's behavior). */
    set( SMB__NONULL, ctrl );

    /* initialize service file location */
    service_file=servicesf;

    if (flags & PAM_SILENT) {
        set( SMB__QUIET, ctrl );
    }

    /* Run through the arguments once, looking for an alternate smb config
       file location */
    while (i < argc) {
	int j;

	for (j = 0; j < SMB_CTRLS_; ++j) {
	    if (smb_args[j].token
	        && !strncmp(argv[i], smb_args[j].token, strlen(smb_args[j].token)))
	    {
		break;
	    }
	}

	if (j == SMB_CONF_FILE) {
	    service_file = argv[i] + 8;
	}
	i++;
    }

    /* Read some options from the Samba config. Can be overridden by
       the PAM config. */
    if(lp_load(service_file,True,False,False,True) == False) {
	_log_err( LOG_ERR, "Error loading service file %s", service_file );
    }

    secrets_init();

    if (lp_null_passwords()) {
        set( SMB__NULLOK, ctrl );
    }

    /* now parse the rest of the arguments to this module */

    while (argc-- > 0) {
        int j;

        for (j = 0; j < SMB_CTRLS_; ++j) {
            if (smb_args[j].token
	        && !strncmp(*argv, smb_args[j].token, strlen(smb_args[j].token)))
            {
                break;
            }
        }

        if (j >= SMB_CTRLS_) {
            _log_err( LOG_ERR, "unrecognized option [%s]", *argv );
        } else {
            ctrl &= smb_args[j].mask;	/* for turning things off */
            ctrl |= smb_args[j].flag;	/* for turning things on  */
        }

        ++argv;				/* step to next argument */
    }

    /* auditing is a more sensitive version of debug */

    if (on( SMB_AUDIT, ctrl )) {
        set( SMB_DEBUG, ctrl );
    }
    /* return the set of flags */

    return ctrl;
}

/* use this to free strings. ESPECIALLY password strings */

char * _pam_delete( register char *xx )
{
    _pam_overwrite( xx );
    _pam_drop( xx );
    return NULL;
}

void _cleanup( pam_handle_t * pamh, void *x, int error_status )
{
    x = _pam_delete( (char *) x );
}

/* JHT
 *
 * Safe duplication of character strings. "Paranoid"; don't leave
 * evidence of old token around for later stack analysis.
 *
 */
char * smbpXstrDup( const char *x )
{
    register char *newstr = NULL;

    if (x != NULL) {
        register int i;

        for (i = 0; x[i]; ++i); /* length of string */
        if ((newstr = SMB_MALLOC_ARRAY(char, ++i)) == NULL) {
            i = 0;
            _log_err( LOG_CRIT, "out of memory in smbpXstrDup" );
        } else {
            while (i-- > 0) {
                newstr[i] = x[i];
            }
        }
        x = NULL;
    }
    return newstr;			/* return the duplicate or NULL on error */
}

/* ************************************************************** *
 * Useful non-trivial functions                                   *
 * ************************************************************** */

void _cleanup_failures( pam_handle_t * pamh, void *fl, int err )
{
    int quiet;
    const char *service = NULL;
    struct _pam_failed_auth *failure;

#ifdef PAM_DATA_SILENT
    quiet = err & PAM_DATA_SILENT;	/* should we log something? */
#else
    quiet = 0;
#endif
#ifdef PAM_DATA_REPLACE
    err &= PAM_DATA_REPLACE;	/* are we just replacing data? */
#endif
    failure = (struct _pam_failed_auth *) fl;

    if (failure != NULL) {

#ifdef PAM_DATA_SILENT
        if (!quiet && !err) {	/* under advisement from Sun,may go away */
#else
        if (!quiet) {	/* under advisement from Sun,may go away */
#endif

            /* log the number of authentication failures */
            if (failure->count != 0) {
                pam_get_item( pamh, PAM_SERVICE, (const void **) &service );
                _log_err( LOG_NOTICE
                          , "%d authentication %s "
                            "from %s for service %s as %s(%d)"
                          , failure->count
                          , failure->count == 1 ? "failure" : "failures"
                          , failure->agent
                          , service == NULL ? "**unknown**" : service 
                          , failure->user, failure->id );
                if (failure->count > SMB_MAX_RETRIES) {
                    _log_err( LOG_ALERT
                              , "service(%s) ignoring max retries; %d > %d"
                              , service == NULL ? "**unknown**" : service
                              , failure->count
                              , SMB_MAX_RETRIES );
                }
            }
        }
        _pam_delete( failure->agent );	/* tidy up */
        _pam_delete( failure->user );	/* tidy up */
	SAFE_FREE( failure );
    }
}

int _smb_verify_password( pam_handle_t * pamh, struct samu *sampass,
			  const char *p, unsigned int ctrl )
{
    uchar lm_pw[16];
    uchar nt_pw[16];
    int retval = PAM_AUTH_ERR;
    char *data_name;
    const char *name;

    if (!sampass)
        return PAM_ABORT;

    name = pdb_get_username(sampass);

#ifdef HAVE_PAM_FAIL_DELAY
    if (off( SMB_NODELAY, ctrl )) {
        (void) pam_fail_delay( pamh, 1000000 );	/* 1 sec delay for on failure */
    }
#endif

    if (!pdb_get_lanman_passwd(sampass))
    {
        _log_err( LOG_DEBUG, "user %s has null SMB password"
                  , name );

        if (off( SMB__NONULL, ctrl )
            && (pdb_get_acct_ctrl(sampass) & ACB_PWNOTREQ))
        { /* this means we've succeeded */
            return PAM_SUCCESS;
        } else {
            const char *service;

            pam_get_item( pamh, PAM_SERVICE, (const void **)&service );
            _log_err( LOG_NOTICE, "failed auth request by %s for service %s as %s",
                      uidtoname(getuid()), service ? service : "**unknown**", name);
            return PAM_AUTH_ERR;
        }
    }

    data_name = SMB_MALLOC_ARRAY(char, sizeof(FAIL_PREFIX) + strlen( name ));
    if (data_name == NULL) {
        _log_err( LOG_CRIT, "no memory for data-name" );
    }
    strncpy( data_name, FAIL_PREFIX, sizeof(FAIL_PREFIX) );
    strncpy( data_name + sizeof(FAIL_PREFIX) - 1, name, strlen( name ) + 1 );

    /*
     * The password we were given wasn't an encrypted password, or it
     * didn't match the one we have.  We encrypt the password now and try
     * again.
     */

    nt_lm_owf_gen(p, nt_pw, lm_pw);

    /* the moment of truth -- do we agree with the password? */

    if (!memcmp( nt_pw, pdb_get_nt_passwd(sampass), 16 )) {

        retval = PAM_SUCCESS;
        if (data_name) {		/* reset failures */
            pam_set_data(pamh, data_name, NULL, _cleanup_failures);
        }
    } else {

        const char *service;

        pam_get_item( pamh, PAM_SERVICE, (const void **)&service );

        if (data_name != NULL) {
            struct _pam_failed_auth *newauth = NULL;
            const struct _pam_failed_auth *old = NULL;

            /* get a failure recorder */

            newauth = SMB_MALLOC_P( struct _pam_failed_auth );

            if (newauth != NULL) {

                /* any previous failures for this user ? */
                pam_get_data(pamh, data_name, (const void **) &old);

                if (old != NULL) {
                    newauth->count = old->count + 1;
                    if (newauth->count >= SMB_MAX_RETRIES) {
                        retval = PAM_MAXTRIES;
                    }
                } else {
                    _log_err(LOG_NOTICE,
                      "failed auth request by %s for service %s as %s",
                      uidtoname(getuid()),
                      service ? service : "**unknown**", name);
                    newauth->count = 1;
                }
		if (!sid_to_uid(pdb_get_user_sid(sampass), &(newauth->id))) {
                    _log_err(LOG_NOTICE,
                      "failed auth request by %s for service %s as %s",
                      uidtoname(getuid()),
                      service ? service : "**unknown**", name);
		}		
                newauth->user = smbpXstrDup( name );
                newauth->agent = smbpXstrDup( uidtoname( getuid() ) );
                pam_set_data( pamh, data_name, newauth, _cleanup_failures );

            } else {
                _log_err( LOG_CRIT, "no memory for failure recorder" );
                _log_err(LOG_NOTICE,
                      "failed auth request by %s for service %s as %s(%d)",
                      uidtoname(getuid()),
                      service ? service : "**unknown**", name);
            }
        } else {
            _log_err(LOG_NOTICE,
                      "failed auth request by %s for service %s as %s(%d)",
                      uidtoname(getuid()),
                      service ? service : "**unknown**", name);
            retval = PAM_AUTH_ERR;
        }
    }

    _pam_delete( data_name );
    
    return retval;
}


/*
 * _smb_blankpasswd() is a quick check for a blank password
 *
 * returns TRUE if user does not have a password
 * - to avoid prompting for one in such cases (CG)
 */

int _smb_blankpasswd( unsigned int ctrl, struct samu *sampass )
{
	int retval;

	/*
	 * This function does not have to be too smart if something goes
	 * wrong, return FALSE and let this case to be treated somewhere
	 * else (CG)
	 */

	if (on( SMB__NONULL, ctrl ))
		return 0;		/* will fail but don't let on yet */

	if (pdb_get_lanman_passwd(sampass) == NULL)
		retval = 1;
	else
		retval = 0;

	return retval;
}

/*
 * obtain a password from the user
 */

int _smb_read_password( pam_handle_t * pamh, unsigned int ctrl,
                        const char *comment, const char *prompt1,
                        const char *prompt2, const char *data_name, char **pass )
{
    int authtok_flag;
    int retval;
    char *item = NULL;
    char *token;

    struct pam_message msg[3], *pmsg[3];
    struct pam_response *resp;
    int i, expect;


    /* make sure nothing inappropriate gets returned */

    *pass = token = NULL;

    /* which authentication token are we getting? */

    authtok_flag = on(SMB__OLD_PASSWD, ctrl) ? PAM_OLDAUTHTOK : PAM_AUTHTOK;

    /* should we obtain the password from a PAM item ? */

    if (on(SMB_TRY_FIRST_PASS, ctrl) || on(SMB_USE_FIRST_PASS, ctrl)) {
        retval = pam_get_item( pamh, authtok_flag, (const void **) &item );
        if (retval != PAM_SUCCESS) {
            /* very strange. */
            _log_err( LOG_ALERT
                      , "pam_get_item returned error to smb_read_password" );
            return retval;
        } else if (item != NULL) {	/* we have a password! */
            *pass = item;
            item = NULL;
            return PAM_SUCCESS;
        } else if (on( SMB_USE_FIRST_PASS, ctrl )) {
            return PAM_AUTHTOK_RECOVER_ERR;		/* didn't work */
        } else if (on( SMB_USE_AUTHTOK, ctrl )
                   && off( SMB__OLD_PASSWD, ctrl ))
        {
            return PAM_AUTHTOK_RECOVER_ERR;
        }
    }

    /*
     * getting here implies we will have to get the password from the
     * user directly.
     */

    /* prepare to converse */
    if (comment != NULL && off(SMB__QUIET, ctrl)) {
        pmsg[0] = &msg[0];
        msg[0].msg_style = PAM_TEXT_INFO;
        msg[0].msg = CONST_DISCARD(char *, comment);
        i = 1;
    } else {
        i = 0;
    }

    pmsg[i] = &msg[i];
    msg[i].msg_style = PAM_PROMPT_ECHO_OFF;
    msg[i++].msg = CONST_DISCARD(char *, prompt1);

    if (prompt2 != NULL) {
        pmsg[i] = &msg[i];
        msg[i].msg_style = PAM_PROMPT_ECHO_OFF;
        msg[i++].msg = CONST_DISCARD(char *, prompt2);
        expect = 2;
    } else
        expect = 1;

    resp = NULL;

    retval = converse( pamh, ctrl, i, pmsg, &resp );

    if (resp != NULL) {
        int j = comment ? 1 : 0;
        /* interpret the response */

        if (retval == PAM_SUCCESS) {	/* a good conversation */

            token = smbpXstrDup(resp[j++].resp);
            if (token != NULL) {
                if (expect == 2) {
                    /* verify that password entered correctly */
                    if (!resp[j].resp || strcmp( token, resp[j].resp )) {
                        _pam_delete( token );
                        retval = PAM_AUTHTOK_RECOVER_ERR;
                        make_remark( pamh, ctrl, PAM_ERROR_MSG
                                     , MISTYPED_PASS );
                    }
                }
            } else {
                _log_err(LOG_NOTICE, "could not recover authentication token");
            }
        }

        /* tidy up */
        _pam_drop_reply( resp, expect );

    } else {
        retval = (retval == PAM_SUCCESS) ? PAM_AUTHTOK_RECOVER_ERR : retval;
    }

    if (retval != PAM_SUCCESS) {
        if (on( SMB_DEBUG, ctrl ))
            _log_err( LOG_DEBUG, "unable to obtain a password" );
        return retval;
    }
    /* 'token' is the entered password */

    if (off( SMB_NOT_SET_PASS, ctrl )) {

        /* we store this password as an item */

        retval = pam_set_item( pamh, authtok_flag, (const void *)token );
        _pam_delete( token );		/* clean it up */
        if (retval != PAM_SUCCESS
            || (retval = pam_get_item( pamh, authtok_flag
                            ,(const void **)&item )) != PAM_SUCCESS)
        {
            _log_err( LOG_CRIT, "error manipulating password" );
            return retval;
        }
    } else {
        /*
         * then store it as data specific to this module. pam_end()
         * will arrange to clean it up.
         */

        retval = pam_set_data( pamh, data_name, (void *) token, _cleanup );
        if (retval != PAM_SUCCESS
            || (retval = pam_get_data( pamh, data_name, (const void **)&item ))
                             != PAM_SUCCESS)
        {
            _log_err( LOG_CRIT, "error manipulating password data [%s]"
                      , pam_strerror( pamh, retval ));
            _pam_delete( token );
            item = NULL;
            return retval;
        }
        token = NULL;			/* break link to password */
    }

    *pass = item;
    item = NULL;			/* break link to password */

    return PAM_SUCCESS;
}

int _pam_smb_approve_pass(pam_handle_t * pamh,
		unsigned int ctrl,
		const char *pass_old,
		const char *pass_new )
{

    /* Further checks should be handled through module stacking. -SRL */
    if (pass_new == NULL || (pass_old && !strcmp( pass_old, pass_new )))
    {
	if (on(SMB_DEBUG, ctrl)) {
	    _log_err( LOG_DEBUG,
	              "passwd: bad authentication token (null or unchanged)" );
	}
	make_remark( pamh, ctrl, PAM_ERROR_MSG, pass_new == NULL ?
				"No password supplied" : "Password unchanged" );
	return PAM_AUTHTOK_ERR;
    }

    return PAM_SUCCESS;
}
