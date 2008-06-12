/* pam_winbind module

   Copyright Andrew Tridgell <tridge@samba.org> 2000
   Copyright Tim Potter <tpot@samba.org> 2000
   Copyright Andrew Bartlett <abartlet@samba.org> 2002
   Copyright Guenther Deschner <gd@samba.org> 2005-2006

   largely based on pam_userdb by Cristian Gafton <gafton@redhat.com> 
   also contains large slabs of code from pam_unix by Elliot Lee <sopwith@redhat.com>
   (see copyright below for full details)
*/

#include "pam_winbind.h"

/* data tokens */

#define MAX_PASSWD_TRIES	3

/* some syslogging */
static void _pam_log(int err, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	openlog(MODULE_NAME, LOG_CONS|LOG_PID, LOG_AUTH);
	vsyslog(err, format, args);
	va_end(args);
	closelog();
}

static void _pam_log_debug(int ctrl, int err, const char *format, ...)
{
	va_list args;

	if (!(ctrl & WINBIND_DEBUG_ARG)) {
		return;
	}

	va_start(args, format);
	openlog(MODULE_NAME, LOG_CONS|LOG_PID, LOG_AUTH);
	vsyslog(err, format, args);
	va_end(args);
	closelog();
}

static int _pam_parse(int argc, const char **argv, dictionary **d)
{
	int ctrl = 0;
	const char *config_file = NULL;
	int i;
	const char **v;

	if (d == NULL) {
		goto config_from_pam;
	}

	for (i=argc,v=argv; i-- > 0; ++v) {
		if (!strncasecmp(*v, "config", strlen("config"))) {
			ctrl |= WINBIND_CONFIG_FILE;
			config_file = v[i];
			break;
		}
	}

	if (config_file == NULL) {
		config_file = PAM_WINBIND_CONFIG_FILE;
	}

	*d = iniparser_load(CONST_DISCARD(char *, config_file));
	if (*d == NULL) {
		goto config_from_pam;
	}

	if (iniparser_getboolean(*d, CONST_DISCARD(char *, "global:debug"), False)) {
		ctrl |= WINBIND_DEBUG_ARG;
	}

	if (iniparser_getboolean(*d, CONST_DISCARD(char *, "global:cached_login"), False)) {
		ctrl |= WINBIND_CACHED_LOGIN;
	}

	if (iniparser_getboolean(*d, CONST_DISCARD(char *, "global:krb5_auth"), False)) {
		ctrl |= WINBIND_KRB5_AUTH;
	}

	if (iniparser_getstr(*d, CONST_DISCARD(char *,"global:krb5_ccache_type")) != NULL) {
		ctrl |= WINBIND_KRB5_CCACHE_TYPE;
	}
	
	if ((iniparser_getstr(*d, CONST_DISCARD(char *, "global:require-membership-of")) != NULL) ||
	    (iniparser_getstr(*d, CONST_DISCARD(char *, "global:require_membership_of")) != NULL)) {
		ctrl |= WINBIND_REQUIRED_MEMBERSHIP;
	}

config_from_pam:
	/* step through arguments */
	for (i=argc,v=argv; i-- > 0; ++v) {

		/* generic options */
		if (!strcmp(*v,"debug"))
			ctrl |= WINBIND_DEBUG_ARG;
		else if (!strcasecmp(*v, "use_authtok"))
			ctrl |= WINBIND_USE_AUTHTOK_ARG;
		else if (!strcasecmp(*v, "use_first_pass"))
			ctrl |= WINBIND_USE_FIRST_PASS_ARG;
		else if (!strcasecmp(*v, "try_first_pass"))
			ctrl |= WINBIND_TRY_FIRST_PASS_ARG;
		else if (!strcasecmp(*v, "unknown_ok"))
			ctrl |= WINBIND_UNKNOWN_OK_ARG;
		else if (!strncasecmp(*v, "require_membership_of", strlen("require_membership_of")))
			ctrl |= WINBIND_REQUIRED_MEMBERSHIP;
		else if (!strncasecmp(*v, "require-membership-of", strlen("require-membership-of")))
			ctrl |= WINBIND_REQUIRED_MEMBERSHIP;
		else if (!strcasecmp(*v, "krb5_auth"))
			ctrl |= WINBIND_KRB5_AUTH;
		else if (!strncasecmp(*v, "krb5_ccache_type", strlen("krb5_ccache_type")))
			ctrl |= WINBIND_KRB5_CCACHE_TYPE;
		else if (!strcasecmp(*v, "cached_login"))
			ctrl |= WINBIND_CACHED_LOGIN;
		else {
			_pam_log(LOG_ERR, "pam_parse: unknown option; %s", *v);
		}

	}
	return ctrl;
};

static void _pam_winbind_cleanup_func(pam_handle_t *pamh, void *data, int error_status)
{
	SAFE_FREE(data);
}

static const struct ntstatus_errors {
	const char *ntstatus_string;
	const char *error_string;
} ntstatus_errors[] = {
	{"NT_STATUS_OK", "Success"},
	{"NT_STATUS_BACKUP_CONTROLLER", "No primary Domain Controler available"},
	{"NT_STATUS_PWD_TOO_SHORT", "Password too short"},
	{"NT_STATUS_PWD_TOO_RECENT", "The password of this user is too recent to change"},
	{"NT_STATUS_PWD_HISTORY_CONFLICT", "Password is already in password history"},
	{"NT_STATUS_PASSWORD_EXPIRED", "Your password has expired"},
	{"NT_STATUS_PASSWORD_MUST_CHANGE", "You need to change your password now"},
	{"NT_STATUS_INVALID_WORKSTATION", "You are not allowed to logon from this workstation"},
	{"NT_STATUS_INVALID_LOGON_HOURS", "You are not allowed to logon at this time"},
	{"NT_STATUS_ACCOUNT_EXPIRED", "Your account has expired. Please contact your System administrator"}, /* SCNR */
	{"NT_STATUS_ACCOUNT_DISABLED", "Your account is disabled. Please contact your System administrator"}, /* SCNR */
	{"NT_STATUS_ACCOUNT_LOCKED_OUT", "Your account has been locked. Please contact your System administrator"}, /* SCNR */
	{"NT_STATUS_NOLOGON_WORKSTATION_TRUST_ACCOUNT", "Invalid Trust Account"},
	{"NT_STATUS_NOLOGON_SERVER_TRUST_ACCOUNT", "Invalid Trust Account"},
	{"NT_STATUS_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT", "Invalid Trust Account"},
	{"NT_STATUS_ACCESS_DENIED", "Access is denied"},
	{NULL, NULL}
};

const char *_get_ntstatus_error_string(const char *nt_status_string) 
{
	int i;
	for (i=0; ntstatus_errors[i].ntstatus_string != NULL; i++) {
		if (!strcasecmp(ntstatus_errors[i].ntstatus_string, nt_status_string)) {
			return ntstatus_errors[i].error_string;
		}
	}
	return NULL;
}

/* --- authentication management functions --- */

/* Attempt a conversation */

static int converse(pam_handle_t *pamh, int nargs,
		    struct pam_message **message,
		    struct pam_response **response)
{
	int retval;
	struct pam_conv *conv;

	retval = pam_get_item(pamh, PAM_CONV, (const void **) &conv );
	if (retval == PAM_SUCCESS) {
		retval = conv->conv(nargs, (const struct pam_message **)message,
				    response, conv->appdata_ptr);
	}
	
	return retval; /* propagate error status */
}


static int _make_remark(pam_handle_t * pamh, int type, const char *text)
{
	int retval = PAM_SUCCESS;

	struct pam_message *pmsg[1], msg[1];
	struct pam_response *resp;
	
	pmsg[0] = &msg[0];
	msg[0].msg = CONST_DISCARD(char *, text);
	msg[0].msg_style = type;
	
	resp = NULL;
	retval = converse(pamh, 1, pmsg, &resp);
	
	if (resp) {
		_pam_drop_reply(resp, 1);
	}
	return retval;
}

static int _make_remark_format(pam_handle_t * pamh, int type, const char *format, ...)
{
	va_list args;
	char *var;
	int ret;

	va_start(args, format);
	vasprintf(&var, format, args);
	va_end(args);

	ret = _make_remark(pamh, type, var);
	SAFE_FREE(var);
	return ret;
}

static int pam_winbind_request(pam_handle_t * pamh, int ctrl,
			       enum winbindd_cmd req_type,
			       struct winbindd_request *request,
			       struct winbindd_response *response)
{
	/* Fill in request and send down pipe */
	init_request(request, req_type);
	
	if (write_sock(request, sizeof(*request), 0) == -1) {
		_pam_log(LOG_ERR, "write to socket failed!");
		close_sock();
		return PAM_SERVICE_ERR;
	}
	
	/* Wait for reply */
	if (read_reply(response) == -1) {
		_pam_log(LOG_ERR, "read from socket failed!");
		close_sock();
		return PAM_SERVICE_ERR;
	}

	/* We are done with the socket - close it and avoid mischeif */
	close_sock();

	/* Copy reply data from socket */
	if (response->result != WINBINDD_OK) {
		if (response->data.auth.pam_error != PAM_SUCCESS) {
			_pam_log(LOG_ERR, "request failed: %s, PAM error was %s (%d), NT error was %s", 
				 response->data.auth.error_string,
				 pam_strerror(pamh, response->data.auth.pam_error),
				 response->data.auth.pam_error,
				 response->data.auth.nt_status_string);
			return response->data.auth.pam_error;
		} else {
			_pam_log(LOG_ERR, "request failed, but PAM error 0!");
			return PAM_SERVICE_ERR;
		}
	}

	return PAM_SUCCESS;
}

static int pam_winbind_request_log(pam_handle_t * pamh, 
				   int ctrl,
				   enum winbindd_cmd req_type,
				   struct winbindd_request *request,
				   struct winbindd_response *response,
				   const char *user)
{
	int retval;

	retval = pam_winbind_request(pamh, ctrl, req_type, request, response);

	switch (retval) {
	case PAM_AUTH_ERR:
		/* incorrect password */
		_pam_log(LOG_WARNING, "user `%s' denied access (incorrect password or invalid membership)", user);
		return retval;
	case PAM_ACCT_EXPIRED:
		/* account expired */
		_pam_log(LOG_WARNING, "user `%s' account expired", user);
		return retval;
	case PAM_AUTHTOK_EXPIRED:
		/* password expired */
		_pam_log(LOG_WARNING, "user `%s' password expired", user);
		return retval;
	case PAM_NEW_AUTHTOK_REQD:
		/* new password required */
		_pam_log(LOG_WARNING, "user `%s' new password required", user);
		return retval;
	case PAM_USER_UNKNOWN:
		/* the user does not exist */
		_pam_log_debug(ctrl, LOG_NOTICE, "user `%s' not found", user);
		if (ctrl & WINBIND_UNKNOWN_OK_ARG) {
			return PAM_IGNORE;
		}	 
		return retval;
	case PAM_SUCCESS:
		if (req_type == WINBINDD_PAM_AUTH) {
			/* Otherwise, the authentication looked good */
			_pam_log(LOG_NOTICE, "user '%s' granted access", user);
		} else if (req_type == WINBINDD_PAM_CHAUTHTOK) {
			/* Otherwise, the authentication looked good */
			_pam_log(LOG_NOTICE, "user '%s' password changed", user);
		} else { 
			/* Otherwise, the authentication looked good */
			_pam_log(LOG_NOTICE, "user '%s' OK", user);
		}
	
		return retval;
	default:
		/* we don't know anything about this return value */
		_pam_log(LOG_ERR, "internal module error (retval = %d, user = `%s')",
			 retval, user);
		return retval;
	}
}

/* talk to winbindd */
static int winbind_auth_request(pam_handle_t * pamh, 
				int ctrl, 
				const char *user, 
				const char *pass, 
				const char *member, 
				const char *cctype,
				int process_result,
				time_t *pwd_last_set)
{
	struct winbindd_request request;
	struct winbindd_response response;
	int ret;

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	if (pwd_last_set) {
		*pwd_last_set = 0;
	}

	strncpy(request.data.auth.user, user, 
		sizeof(request.data.auth.user)-1);

	strncpy(request.data.auth.pass, pass, 
		sizeof(request.data.auth.pass)-1);

	request.data.auth.krb5_cc_type[0] = '\0';
	request.data.auth.uid = -1;
	
	request.flags = WBFLAG_PAM_INFO3_TEXT | WBFLAG_PAM_CONTACT_TRUSTDOM;

	if (ctrl & WINBIND_KRB5_AUTH) {

		struct passwd *pwd = NULL;

		_pam_log_debug(ctrl, LOG_DEBUG, "enabling krb5 login flag\n"); 

		request.flags |= WBFLAG_PAM_KRB5 | WBFLAG_PAM_FALLBACK_AFTER_KRB5;

		pwd = getpwnam(user);
		if (pwd == NULL) {
			return PAM_USER_UNKNOWN;
		}
		request.data.auth.uid = pwd->pw_uid;
	}

	if (ctrl & WINBIND_CACHED_LOGIN) {
		_pam_log_debug(ctrl, LOG_DEBUG, "enabling cached login flag\n"); 
		request.flags |= WBFLAG_PAM_CACHED_LOGIN;
	}

	if (cctype != NULL) {
		strncpy(request.data.auth.krb5_cc_type, cctype, 
			sizeof(request.data.auth.krb5_cc_type) - 1);
		_pam_log_debug(ctrl, LOG_DEBUG, "enabling request for a %s krb5 ccache\n", cctype); 
	}

	request.data.auth.require_membership_of_sid[0] = '\0';

	if (member != NULL) {
		strncpy(request.data.auth.require_membership_of_sid, member, 
		        sizeof(request.data.auth.require_membership_of_sid)-1);
	}

	/* lookup name? */ 
	if ( (member != NULL) && (strncmp("S-", member, 2) != 0) ) {
		
		struct winbindd_request sid_request;
		struct winbindd_response sid_response;

		ZERO_STRUCT(sid_request);
		ZERO_STRUCT(sid_response);

		_pam_log_debug(ctrl, LOG_DEBUG, "no sid given, looking up: %s\n", member);

		/* fortunatly winbindd can handle non-separated names */
		strncpy(sid_request.data.name.name, member,
			sizeof(sid_request.data.name.name) - 1);

		if (pam_winbind_request_log(pamh, ctrl, WINBINDD_LOOKUPNAME, &sid_request, &sid_response, user)) {
			_pam_log(LOG_INFO, "could not lookup name: %s\n", member); 
			return PAM_AUTH_ERR;
		}

		member = sid_response.data.sid.sid;

		strncpy(request.data.auth.require_membership_of_sid, member, 
		        sizeof(request.data.auth.require_membership_of_sid)-1);
	}
	
	ret = pam_winbind_request_log(pamh, ctrl, WINBINDD_PAM_AUTH, &request, &response, user);

	if (pwd_last_set) {
		*pwd_last_set = response.data.auth.info3.pass_last_set_time;
	}

	if ((ctrl & WINBIND_KRB5_AUTH) && 
	    response.data.auth.krb5ccname[0] != '\0') {

		char var[PATH_MAX];

		_pam_log_debug(ctrl, LOG_DEBUG, "request returned KRB5CCNAME: %s", 
			       response.data.auth.krb5ccname);
	
		snprintf(var, sizeof(var), "KRB5CCNAME=%s", response.data.auth.krb5ccname);
	
		ret = pam_putenv(pamh, var);
		if (ret != PAM_SUCCESS) {
			_pam_log(LOG_ERR, "failed to set KRB5CCNAME to %s", var);
			return ret;
		}
	}

	if (!process_result) {
		return ret;
	}

	if (ret) {
		PAM_WB_REMARK_CHECK_RESPONSE_RET(pamh, response, "NT_STATUS_PASSWORD_EXPIRED");
		PAM_WB_REMARK_CHECK_RESPONSE_RET(pamh, response, "NT_STATUS_PASSWORD_MUST_CHANGE");
		PAM_WB_REMARK_CHECK_RESPONSE_RET(pamh, response, "NT_STATUS_INVALID_WORKSTATION");
		PAM_WB_REMARK_CHECK_RESPONSE_RET(pamh, response, "NT_STATUS_INVALID_LOGON_HOURS");
		PAM_WB_REMARK_CHECK_RESPONSE_RET(pamh, response, "NT_STATUS_ACCOUNT_EXPIRED");
		PAM_WB_REMARK_CHECK_RESPONSE_RET(pamh, response, "NT_STATUS_ACCOUNT_DISABLED");
		PAM_WB_REMARK_CHECK_RESPONSE_RET(pamh, response, "NT_STATUS_ACCOUNT_LOCKED_OUT");
		PAM_WB_REMARK_CHECK_RESPONSE_RET(pamh, response, "NT_STATUS_NOLOGON_WORKSTATION_TRUST_ACCOUNT");
		PAM_WB_REMARK_CHECK_RESPONSE_RET(pamh, response, "NT_STATUS_NOLOGON_SERVER_TRUST_ACCOUNT");
		PAM_WB_REMARK_CHECK_RESPONSE_RET(pamh, response, "NT_STATUS_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT");
	}

	/* handle the case where the auth was ok, but the password must expire right now */
	/* good catch from Ralf Haferkamp: an expiry of "never" is translated to -1 */
	if ( ! (response.data.auth.info3.acct_flags & ACB_PWNOEXP) &&
	    (response.data.auth.policy.expire > 0) && 
	    (response.data.auth.info3.pass_last_set_time + response.data.auth.policy.expire < time(NULL))) {

		ret = PAM_AUTHTOK_EXPIRED;

		_pam_log_debug(ctrl, LOG_DEBUG,"Password has expired (Password was last set: %d, "
			       "the policy says it should expire here %d (now it's: %d)\n",
			       response.data.auth.info3.pass_last_set_time,
			       response.data.auth.info3.pass_last_set_time + response.data.auth.policy.expire,
			       time(NULL));

		PAM_WB_REMARK_DIRECT_RET(pamh, "NT_STATUS_PASSWORD_EXPIRED");

	}

	/* warn a user if the password is about to expire soon */
	if ( ! (response.data.auth.info3.acct_flags & ACB_PWNOEXP) &&
	    (response.data.auth.policy.expire) && 
	    (response.data.auth.info3.pass_last_set_time + response.data.auth.policy.expire > time(NULL) ) ) {

		int days = response.data.auth.policy.expire / SECONDS_PER_DAY;
		if (days <= DAYS_TO_WARN_BEFORE_PWD_EXPIRES) {
			_make_remark_format(pamh, PAM_TEXT_INFO, "Your password will expire in %d days", days);
		}
	}

	if (response.data.auth.info3.user_flgs & LOGON_CACHED_ACCOUNT) {
		_make_remark(pamh, PAM_ERROR_MSG, "Logging on using cached account. Network ressources can be unavailable");
		_pam_log_debug(ctrl, LOG_DEBUG,"User %s logged on using cached account\n", user);
	}

	/* save the CIFS homedir for pam_cifs / pam_mount */
	if (response.data.auth.info3.home_dir[0] != '\0') {

		int ret2 = pam_set_data(pamh, PAM_WINBIND_HOMEDIR,
					(void *) strdup(response.data.auth.info3.home_dir),
					_pam_winbind_cleanup_func);
		if (ret2) {
			_pam_log_debug(ctrl, LOG_DEBUG, "Could not set data: %s", 
				       pam_strerror(pamh, ret2));
		}

	}

	/* save the logon script path for other PAM modules */
	if (response.data.auth.info3.logon_script[0] != '\0') {

		int ret2 = pam_set_data(pamh, PAM_WINBIND_LOGONSCRIPT, 
					(void *) strdup(response.data.auth.info3.logon_script), 
					_pam_winbind_cleanup_func);
		if (ret2) {
			_pam_log_debug(ctrl, LOG_DEBUG, "Could not set data: %s", 
				       pam_strerror(pamh, ret2));
		}
	}

	/* save the profile path for other PAM modules */
	if (response.data.auth.info3.profile_path[0] != '\0') {

		int ret2 = pam_set_data(pamh, PAM_WINBIND_PROFILEPATH, 
					(void *) strdup(response.data.auth.info3.profile_path), 
					_pam_winbind_cleanup_func);
		if (ret2) {
			_pam_log_debug(ctrl, LOG_DEBUG, "Could not set data: %s", 
				       pam_strerror(pamh, ret2));
		}
	}

	return ret;
}

/* talk to winbindd */
static int winbind_chauthtok_request(pam_handle_t * pamh,
				     int ctrl,
				     const char *user, 
				     const char *oldpass,
				     const char *newpass,
				     time_t pwd_last_set) 
{
	struct winbindd_request request;
	struct winbindd_response response;
	int ret;

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	if (request.data.chauthtok.user == NULL) return -2;

	strncpy(request.data.chauthtok.user, user, 
		sizeof(request.data.chauthtok.user) - 1);

	if (oldpass != NULL) {
		strncpy(request.data.chauthtok.oldpass, oldpass, 
			sizeof(request.data.chauthtok.oldpass) - 1);
	} else {
		request.data.chauthtok.oldpass[0] = '\0';
	}
	
	if (newpass != NULL) {
		strncpy(request.data.chauthtok.newpass, newpass, 
			sizeof(request.data.chauthtok.newpass) - 1);
	} else {
		request.data.chauthtok.newpass[0] = '\0';
	}

	if (ctrl & WINBIND_KRB5_AUTH) {
		request.flags = WBFLAG_PAM_KRB5 | WBFLAG_PAM_CONTACT_TRUSTDOM;
	}

	ret = pam_winbind_request_log(pamh, ctrl, WINBINDD_PAM_CHAUTHTOK, &request, &response, user);

	if (ret == PAM_SUCCESS) {
		return ret;
	}

	PAM_WB_REMARK_CHECK_RESPONSE_RET(pamh, response, "NT_STATUS_BACKUP_CONTROLLER");
	PAM_WB_REMARK_CHECK_RESPONSE_RET(pamh, response, "NT_STATUS_ACCESS_DENIED");

	/* TODO: tell the min pwd length ? */
	PAM_WB_REMARK_CHECK_RESPONSE_RET(pamh, response, "NT_STATUS_PWD_TOO_SHORT");

	/* TODO: tell the minage ? */
	PAM_WB_REMARK_CHECK_RESPONSE_RET(pamh, response, "NT_STATUS_PWD_TOO_RECENT");

	/* TODO: tell the history length ? */
	PAM_WB_REMARK_CHECK_RESPONSE_RET(pamh, response, "NT_STATUS_PWD_HISTORY_CONFLICT");

	if (!strcasecmp(response.data.auth.nt_status_string, "NT_STATUS_PASSWORD_RESTRICTION")) {

		/* FIXME: avoid to send multiple PAM messages after another */
		switch (response.data.auth.reject_reason) {
			case -1:
				break;
			case REJECT_REASON_OTHER:
				if ((response.data.auth.policy.min_passwordage > 0) &&
				    (pwd_last_set + response.data.auth.policy.min_passwordage > time(NULL))) {
					PAM_WB_REMARK_DIRECT(pamh, "NT_STATUS_PWD_TOO_RECENT");
				}
				break;
			case REJECT_REASON_TOO_SHORT:
				PAM_WB_REMARK_DIRECT(pamh, "NT_STATUS_PWD_TOO_SHORT");
				break;
			case REJECT_REASON_IN_HISTORY:
				PAM_WB_REMARK_DIRECT(pamh, "NT_STATUS_PWD_HISTORY_CONFLICT");
				break;
			case REJECT_REASON_NOT_COMPLEX:
				_make_remark(pamh, PAM_ERROR_MSG, "Password does not meet complexity requirements");
				break;
			default:
				_pam_log_debug(ctrl, LOG_DEBUG,
					       "unknown password change reject reason: %d", 
					       response.data.auth.reject_reason);
				break;
		}

		_make_remark_format(pamh, PAM_ERROR_MSG,  
			"Your password must be at least %d characters; "
			"cannot repeat any of the your previous %d passwords"
			"%s. "
			"Please type a different password. "
			"Type a password which meets these requirements in both text boxes.",
			response.data.auth.policy.min_length_password,
			response.data.auth.policy.password_history,
			(response.data.auth.policy.password_properties & DOMAIN_PASSWORD_COMPLEX) ? 
				"; must contain capitals, numerals or punctuation; and cannot contain your account or full name" : 
				"");

	}

	return ret;
}

/*
 * Checks if a user has an account
 *
 * return values:
 *	 1  = User not found
 *	 0  = OK
 * 	-1  = System error
 */
static int valid_user(const char *user, pam_handle_t *pamh, int ctrl)
{
	/* check not only if the user is available over NSS calls, also make
	 * sure it's really a winbind user, this is important when stacking PAM
	 * modules in the 'account' or 'password' facility. */

	struct passwd *pwd = NULL;
	struct winbindd_request request;
	struct winbindd_response response;
	int ret;

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	pwd = getpwnam(user);
	if (pwd == NULL) {
		return 1;
	}

	strncpy(request.data.username, user,
		sizeof(request.data.username) - 1);

	ret = pam_winbind_request_log(pamh, ctrl, WINBINDD_GETPWNAM, &request, &response, user);

	switch (ret) {
		case PAM_USER_UNKNOWN:
			return 1;
		case PAM_SUCCESS:
			return 0;
		default:
			break;
	}
	return -1;
}

static char *_pam_delete(register char *xx)
{
	_pam_overwrite(xx);
	_pam_drop(xx);
	return NULL;
}

/*
 * obtain a password from the user
 */

static int _winbind_read_password(pam_handle_t * pamh,
				  unsigned int ctrl,
				  const char *comment,
				  const char *prompt1,
				  const char *prompt2,
				  const char **pass)
{
	int authtok_flag;
	int retval;
	const char *item;
	char *token;

	/*
	 * make sure nothing inappropriate gets returned
	 */

	*pass = token = NULL;

	/*
	 * which authentication token are we getting?
	 */

	authtok_flag = on(WINBIND__OLD_PASSWORD, ctrl) ? PAM_OLDAUTHTOK : PAM_AUTHTOK;

	/*
	 * should we obtain the password from a PAM item ?
	 */

	if (on(WINBIND_TRY_FIRST_PASS_ARG, ctrl) || on(WINBIND_USE_FIRST_PASS_ARG, ctrl)) {
		retval = pam_get_item(pamh, authtok_flag, (const void **) &item);
		if (retval != PAM_SUCCESS) {
			/* very strange. */
			_pam_log(LOG_ALERT, 
				 "pam_get_item returned error to unix-read-password"
			    );
			return retval;
		} else if (item != NULL) {	/* we have a password! */
			*pass = item;
			item = NULL;
			return PAM_SUCCESS;
		} else if (on(WINBIND_USE_FIRST_PASS_ARG, ctrl)) {
			return PAM_AUTHTOK_RECOVER_ERR;		/* didn't work */
		} else if (on(WINBIND_USE_AUTHTOK_ARG, ctrl)
			   && off(WINBIND__OLD_PASSWORD, ctrl)) {
			return PAM_AUTHTOK_RECOVER_ERR;
		}
	}
	/*
	 * getting here implies we will have to get the password from the
	 * user directly.
	 */

	{
		struct pam_message msg[3], *pmsg[3];
		struct pam_response *resp;
		int i, replies;

		/* prepare to converse */

		if (comment != NULL) {
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
		replies = 1;

		if (prompt2 != NULL) {
			pmsg[i] = &msg[i];
			msg[i].msg_style = PAM_PROMPT_ECHO_OFF;
			msg[i++].msg = CONST_DISCARD(char *, prompt2);
			++replies;
		}
		/* so call the conversation expecting i responses */
		resp = NULL;
		retval = converse(pamh, i, pmsg, &resp);

		if (resp != NULL) {

			/* interpret the response */

			if (retval == PAM_SUCCESS) {	/* a good conversation */

				token = x_strdup(resp[i - replies].resp);
				if (token != NULL) {
					if (replies == 2) {
						/* verify that password entered correctly */
						if (!resp[i - 1].resp
						    || strcmp(token, resp[i - 1].resp)) {
							_pam_delete(token);	/* mistyped */
							retval = PAM_AUTHTOK_RECOVER_ERR;
							_make_remark(pamh, PAM_ERROR_MSG, MISTYPED_PASS);
						}
					}
				} else {
					_pam_log(LOG_NOTICE, "could not recover authentication token");
					retval = PAM_AUTHTOK_RECOVER_ERR;
				}

			}
			/*
			 * tidy up the conversation (resp_retcode) is ignored
			 * -- what is it for anyway? AGM
			 */

			_pam_drop_reply(resp, i);

		} else {
			retval = (retval == PAM_SUCCESS)
			    ? PAM_AUTHTOK_RECOVER_ERR : retval;
		}
	}

	if (retval != PAM_SUCCESS) {
		_pam_log_debug(ctrl, LOG_DEBUG,
			         "unable to obtain a password");
		return retval;
	}
	/* 'token' is the entered password */

	/* we store this password as an item */
	
	retval = pam_set_item(pamh, authtok_flag, token);
	_pam_delete(token);	/* clean it up */
	if (retval != PAM_SUCCESS || 
	    (retval = pam_get_item(pamh, authtok_flag, (const void **) &item)) != PAM_SUCCESS) {
		
		_pam_log(LOG_CRIT, "error manipulating password");
		return retval;
		
	}

	*pass = item;
	item = NULL;		/* break link to password */

	return PAM_SUCCESS;
}

const char *get_conf_item_string(int argc, 
				 const char **argv, 
				 int ctrl,
				 dictionary *d,
				 const char *item, 
				 int flag)
{
	int i = 0;
	const char *parm_opt = NULL;
	char *key = NULL;

	if (!(ctrl & flag)) {
		goto out;
	}

	/* let the pam opt take precedence over the pam_winbind.conf option */

	if (d != NULL) {

		if (!asprintf(&key, "global:%s", item)) {
			goto out;
		}

		parm_opt = iniparser_getstr(d, key);
		SAFE_FREE(key);
	}

	for ( i=0; i<argc; i++ ) {

		if ((strncmp(argv[i], item, strlen(item)) == 0)) {
			char *p;

			if ( (p = strchr( argv[i], '=' )) == NULL) {
				_pam_log(LOG_INFO, "no \"=\" delimiter for \"%s\" found\n", item);
				goto out;
			}
			_pam_log_debug(ctrl, LOG_INFO, "PAM config: %s '%s'\n", item, p+1);
			return p + 1;
		}
	}

	if (d != NULL) {
		_pam_log_debug(ctrl, LOG_INFO, "CONFIG file: %s '%s'\n", item, parm_opt);
	}
out:
	return parm_opt;
}

const char *get_krb5_cc_type_from_config(int argc, const char **argv, int ctrl, dictionary *d)
{
	return get_conf_item_string(argc, argv, ctrl, d, "krb5_ccache_type", WINBIND_KRB5_CCACHE_TYPE);
}

const char *get_member_from_config(int argc, const char **argv, int ctrl, dictionary *d)
{
	const char *ret = NULL;
	ret = get_conf_item_string(argc, argv, ctrl, d, "require_membership_of", WINBIND_REQUIRED_MEMBERSHIP);
	if (ret) {
		return ret;
	}
	return get_conf_item_string(argc, argv, ctrl, d, "require-membership-of", WINBIND_REQUIRED_MEMBERSHIP);
}

PAM_EXTERN
int pam_sm_authenticate(pam_handle_t *pamh, int flags,
			int argc, const char **argv)
{
	const char *username;
	const char *password;
	const char *member = NULL;
	const char *cctype = NULL;
	int retval = PAM_AUTH_ERR;
	dictionary *d = NULL;

	/* parse arguments */
	int ctrl = _pam_parse(argc, argv, &d);
	if (ctrl == -1) {
		retval = PAM_SYSTEM_ERR;
		goto out;
	}

	_pam_log_debug(ctrl, LOG_DEBUG,"pam_winbind: pam_sm_authenticate (flags: 0x%04x)", flags);

	/* Get the username */
	retval = pam_get_user(pamh, &username, NULL);
	if ((retval != PAM_SUCCESS) || (!username)) {
		_pam_log_debug(ctrl, LOG_DEBUG, "can not get the username");
		retval = PAM_SERVICE_ERR;
		goto out;
	}

	retval = _winbind_read_password(pamh, ctrl, NULL, 
					"Password: ", NULL,
					&password);

	if (retval != PAM_SUCCESS) {
		_pam_log(LOG_ERR, "Could not retrieve user's password");
		retval = PAM_AUTHTOK_ERR;
		goto out;
	}

	/* Let's not give too much away in the log file */

#ifdef DEBUG_PASSWORD
	_pam_log_debug(ctrl, LOG_INFO, "Verify user `%s' with password `%s'", 
		       username, password);
#else
	_pam_log_debug(ctrl, LOG_INFO, "Verify user `%s'", username);
#endif

	member = get_member_from_config(argc, argv, ctrl, d);

	cctype = get_krb5_cc_type_from_config(argc, argv, ctrl, d);

	/* Now use the username to look up password */
	retval = winbind_auth_request(pamh, ctrl, username, password, member, cctype, True, NULL);

	if (retval == PAM_NEW_AUTHTOK_REQD ||
	    retval == PAM_AUTHTOK_EXPIRED) {

		char *buf;

		if (!asprintf(&buf, "%d", retval)) {
			retval = PAM_BUF_ERR;
			goto out;
		}

		pam_set_data( pamh, PAM_WINBIND_NEW_AUTHTOK_REQD, (void *)buf, _pam_winbind_cleanup_func);

		retval = PAM_SUCCESS;
		goto out;
	}

out:
	if (d) {
		iniparser_freedict(d);
	}
	return retval;
}

PAM_EXTERN
int pam_sm_setcred(pam_handle_t *pamh, int flags,
		   int argc, const char **argv)
{
	/* parse arguments */
	int ctrl = _pam_parse(argc, argv, NULL);
	if (ctrl == -1) {
		return PAM_SYSTEM_ERR;
	}

	_pam_log_debug(ctrl, LOG_DEBUG,"pam_winbind: pam_sm_setcred (flags: 0x%04x)", flags);

	if (flags & PAM_DELETE_CRED) {
		return pam_sm_close_session(pamh, flags, argc, argv);
	}

	return PAM_SUCCESS;
}

/*
 * Account management. We want to verify that the account exists 
 * before returning PAM_SUCCESS
 */
PAM_EXTERN
int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags,
		   int argc, const char **argv)
{
	const char *username;
	int retval = PAM_USER_UNKNOWN;
	void *tmp = NULL;

	/* parse arguments */
	int ctrl = _pam_parse(argc, argv, NULL);
	if (ctrl == -1) {
		return PAM_SYSTEM_ERR;
	}

	_pam_log_debug(ctrl, LOG_DEBUG,"pam_winbind: pam_sm_acct_mgmt (flags: 0x%04x)", flags);


	/* Get the username */
	retval = pam_get_user(pamh, &username, NULL);
	if ((retval != PAM_SUCCESS) || (!username)) {
		_pam_log_debug(ctrl, LOG_DEBUG,"can not get the username");
		return PAM_SERVICE_ERR;
	}

	/* Verify the username */
	retval = valid_user(username, pamh, ctrl);
	switch (retval) {
	case -1:
		/* some sort of system error. The log was already printed */
		return PAM_SERVICE_ERR;
	case 1:
		/* the user does not exist */
		_pam_log_debug(ctrl, LOG_NOTICE, "user `%s' not found", username);
		if (ctrl & WINBIND_UNKNOWN_OK_ARG) {
			return PAM_IGNORE;
		}
		return PAM_USER_UNKNOWN;
	case 0:
		pam_get_data( pamh, PAM_WINBIND_NEW_AUTHTOK_REQD, (const void **)&tmp);
		if (tmp != NULL) {
			retval = atoi(tmp);
			switch (retval) {
			case PAM_AUTHTOK_EXPIRED:
				/* fall through, since new token is required in this case */
			case PAM_NEW_AUTHTOK_REQD:
				_pam_log(LOG_WARNING, "pam_sm_acct_mgmt success but %s is set", 
					 PAM_WINBIND_NEW_AUTHTOK_REQD);
				_pam_log(LOG_NOTICE, "user '%s' needs new password", username);
				/* PAM_AUTHTOKEN_REQD does not exist, but is documented in the manpage */
				return PAM_NEW_AUTHTOK_REQD; 
			default:
				_pam_log(LOG_WARNING, "pam_sm_acct_mgmt success");
				_pam_log(LOG_NOTICE, "user '%s' granted access", username);
				return PAM_SUCCESS;
			}
		}

		/* Otherwise, the authentication looked good */
		_pam_log(LOG_NOTICE, "user '%s' granted access", username);
		return PAM_SUCCESS;
	default:
		/* we don't know anything about this return value */
		_pam_log(LOG_ERR, "internal module error (retval = %d, user = `%s')", 
			 retval, username);
		return PAM_SERVICE_ERR;
	}

	/* should not be reached */
	return PAM_IGNORE;
}

PAM_EXTERN
int pam_sm_open_session(pam_handle_t *pamh, int flags,
			int argc, const char **argv)
{
	/* parse arguments */
	int ctrl = _pam_parse(argc, argv, NULL);
	if (ctrl == -1) {
		return PAM_SYSTEM_ERR;
	}

	_pam_log_debug(ctrl, LOG_DEBUG,"pam_winbind: pam_sm_open_session handler (flags: 0x%04x)", flags);

	return PAM_SUCCESS;
}

PAM_EXTERN
int pam_sm_close_session(pam_handle_t *pamh, int flags,
			 int argc, const char **argv)
{
	dictionary *d = NULL;
	int retval = PAM_SUCCESS;

	/* parse arguments */
	int ctrl = _pam_parse(argc, argv, &d);
	if (ctrl == -1) {
		retval = PAM_SYSTEM_ERR;
		goto out;
	}

	_pam_log_debug(ctrl, LOG_DEBUG,"pam_winbind: pam_sm_close_session handler (flags: 0x%04x)", flags);

	if (!(flags & PAM_DELETE_CRED)) {
		retval = PAM_SUCCESS;
		goto out;
	}

	if (ctrl & WINBIND_KRB5_AUTH) {

		/* destroy the ccache here */
		struct winbindd_request request;
		struct winbindd_response response;
		const char *user;
		const char *ccname = NULL;
		struct passwd *pwd = NULL;

		ZERO_STRUCT(request);
		ZERO_STRUCT(response);

		retval = pam_get_user(pamh, &user, "Username: ");
		if (retval == PAM_SUCCESS) {
			if (user == NULL) {
				_pam_log(LOG_ERR, "username was NULL!");
				retval = PAM_USER_UNKNOWN;
				goto out;
			}
			if (retval == PAM_SUCCESS) {
				_pam_log_debug(ctrl, LOG_DEBUG, "username [%s] obtained", user);
			}
		} else {
			_pam_log_debug(ctrl, LOG_DEBUG, "could not identify user");
			goto out;
		}

		ccname = pam_getenv(pamh, "KRB5CCNAME");
		if (ccname == NULL) {
			_pam_log_debug(ctrl, LOG_DEBUG, "user has no KRB5CCNAME environment");
			retval = PAM_SUCCESS;
			goto out;
		}

		strncpy(request.data.logoff.user, user,
			sizeof(request.data.logoff.user) - 1);

		strncpy(request.data.logoff.krb5ccname, ccname,
			sizeof(request.data.logoff.krb5ccname) - 1);

		pwd = getpwnam(user);
		if (pwd == NULL) {
			retval = PAM_USER_UNKNOWN;
			goto out;
		}
		request.data.logoff.uid = pwd->pw_uid;

		request.flags = WBFLAG_PAM_KRB5 | WBFLAG_PAM_CONTACT_TRUSTDOM;

	        retval = pam_winbind_request_log(pamh, ctrl, WINBINDD_PAM_LOGOFF, &request, &response, user);
	}

out:
	if (d) {
		iniparser_freedict(d);
	}
	return retval;
}



PAM_EXTERN 
int pam_sm_chauthtok(pam_handle_t * pamh, int flags,
		     int argc, const char **argv)
{
	unsigned int lctrl;
	int retval;
	unsigned int ctrl;

	/* <DO NOT free() THESE> */
	const char *user;
	char *pass_old, *pass_new;
	/* </DO NOT free() THESE> */

	char *Announce;
	
	int retry = 0;
	dictionary *d = NULL;

	ctrl = _pam_parse(argc, argv, &d);
	if (ctrl == -1) {
		retval = PAM_SYSTEM_ERR;
		goto out;
	}

	_pam_log_debug(ctrl, LOG_DEBUG,"pam_winbind: pam_sm_chauthtok (flags: 0x%04x)", flags);

	/* clearing offline bit for the auth in the password change */
	ctrl &= ~WINBIND_CACHED_LOGIN;

	/*
	 * First get the name of a user
	 */
	retval = pam_get_user(pamh, &user, "Username: ");
	if (retval == PAM_SUCCESS) {
		if (user == NULL) {
			_pam_log(LOG_ERR, "username was NULL!");
			retval = PAM_USER_UNKNOWN;
			goto out;
		}
		if (retval == PAM_SUCCESS) {
			_pam_log_debug(ctrl, LOG_DEBUG, "username [%s] obtained",
				 user);
		}
	} else {
		_pam_log_debug(ctrl, LOG_DEBUG,
			 "password - could not identify user");
		goto out;
	}

	/* check if this is really a user in winbindd, not only in NSS */
	retval = valid_user(user, pamh, ctrl);
	switch (retval) {
		case 1:
			retval = PAM_USER_UNKNOWN;
			goto out;
		case -1:
			retval = PAM_SYSTEM_ERR;
			goto out;
		default:
			break;
	}
		
	/*
	 * obtain and verify the current password (OLDAUTHTOK) for
	 * the user.
	 */

	if (flags & PAM_PRELIM_CHECK) {
		
		time_t pwdlastset_prelim = 0;
		
		/* instruct user what is happening */
#define greeting "Changing password for "
		Announce = (char *) malloc(sizeof(greeting) + strlen(user));
		if (Announce == NULL) {
			_pam_log(LOG_CRIT, "password - out of memory");
			retval = PAM_BUF_ERR;
			goto out;
		}
		(void) strcpy(Announce, greeting);
		(void) strcpy(Announce + sizeof(greeting) - 1, user);
#undef greeting
		
		lctrl = ctrl | WINBIND__OLD_PASSWORD;
		retval = _winbind_read_password(pamh, lctrl,
						Announce,
						"(current) NT password: ",
						NULL,
						(const char **) &pass_old);
		if (retval != PAM_SUCCESS) {
			_pam_log(LOG_NOTICE, "password - (old) token not obtained");
			goto out;
		}
		/* verify that this is the password for this user */
		
		retval = winbind_auth_request(pamh, ctrl, user, pass_old, NULL, NULL, False, &pwdlastset_prelim);

		if (retval != PAM_ACCT_EXPIRED && 
		    retval != PAM_AUTHTOK_EXPIRED &&
		    retval != PAM_NEW_AUTHTOK_REQD &&
		    retval != PAM_SUCCESS) {
			pass_old = NULL;
			goto out;
		}
		
		pam_set_data(pamh, PAM_WINBIND_PWD_LAST_SET, (void *)pwdlastset_prelim, NULL);

		retval = pam_set_item(pamh, PAM_OLDAUTHTOK, (const void *) pass_old);
		pass_old = NULL;
		if (retval != PAM_SUCCESS) {
			_pam_log(LOG_CRIT, "failed to set PAM_OLDAUTHTOK");
		}
	} else if (flags & PAM_UPDATE_AUTHTOK) {
	
		time_t pwdlastset_update = 0;
		
		/*
		 * obtain the proposed password
		 */
		
		/*
		 * get the old token back. 
		 */
		
		retval = pam_get_item(pamh, PAM_OLDAUTHTOK,
				      (const void **) &pass_old);
		
		if (retval != PAM_SUCCESS) {
			_pam_log(LOG_NOTICE, "user not authenticated");
			goto out;
		}
		
		lctrl = ctrl;
		
		if (on(WINBIND_USE_AUTHTOK_ARG, lctrl)) {
			lctrl |= WINBIND_USE_FIRST_PASS_ARG;
		}
		retry = 0;
		retval = PAM_AUTHTOK_ERR;
		while ((retval != PAM_SUCCESS) && (retry++ < MAX_PASSWD_TRIES)) {
			/*
			 * use_authtok is to force the use of a previously entered
			 * password -- needed for pluggable password strength checking
			 */
			
			retval = _winbind_read_password(pamh, lctrl,
							NULL,
							"Enter new NT password: ",
							"Retype new NT password: ",
							(const char **) &pass_new);
			
			if (retval != PAM_SUCCESS) {
				_pam_log_debug(ctrl, LOG_ALERT
					 ,"password - new password not obtained");
				pass_old = NULL;/* tidy up */
				goto out;
			}

			/*
			 * At this point we know who the user is and what they
			 * propose as their new password. Verify that the new
			 * password is acceptable.
			 */
			
			if (pass_new[0] == '\0') {/* "\0" password = NULL */
				pass_new = NULL;
			}
		}
		
		/*
		 * By reaching here we have approved the passwords and must now
		 * rebuild the password database file.
		 */
		pam_get_data( pamh, PAM_WINBIND_PWD_LAST_SET, (const void **)&pwdlastset_update);

		retval = winbind_chauthtok_request(pamh, ctrl, user, pass_old, pass_new, pwdlastset_update);
		if (retval) {
			_pam_overwrite(pass_new);
			_pam_overwrite(pass_old);
			pass_old = pass_new = NULL;
			goto out;
		}

		/* just in case we need krb5 creds after a password change over msrpc */

		if (ctrl & WINBIND_KRB5_AUTH) {

			const char *member = get_member_from_config(argc, argv, ctrl, d);
			const char *cctype = get_krb5_cc_type_from_config(argc, argv, ctrl, d);

			retval = winbind_auth_request(pamh, ctrl, user, pass_new, member, cctype, False, NULL);
			_pam_overwrite(pass_new);
			_pam_overwrite(pass_old);
			pass_old = pass_new = NULL;
		}
	} else {
		retval = PAM_SERVICE_ERR;
	}

out:
	if (d) {
		iniparser_freedict(d);
	}
	return retval;
}

#ifdef PAM_STATIC

/* static module data */

struct pam_module _pam_winbind_modstruct = {
	MODULE_NAME,
	pam_sm_authenticate,
	pam_sm_setcred,
	pam_sm_acct_mgmt,
	pam_sm_open_session,
	pam_sm_close_session,
	pam_sm_chauthtok
};

#endif

/*
 * Copyright (c) Andrew Tridgell  <tridge@samba.org>   2000
 * Copyright (c) Tim Potter       <tpot@samba.org>     2000
 * Copyright (c) Andrew Bartlettt <abartlet@samba.org> 2002
 * Copyright (c) Guenther Deschner <gd@samba.org>      2005-2006
 * Copyright (c) Jan Rêkorajski 1999.
 * Copyright (c) Andrew G. Morgan 1996-8.
 * Copyright (c) Alex O. Yuriev, 1996.
 * Copyright (c) Cristian Gafton 1996.
 * Copyright (C) Elliot Lee <sopwith@redhat.com> 1996, Red Hat Software. 
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, and the entire permission notice in its entirety,
 *    including the disclaimer of warranties.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * ALTERNATIVELY, this product may be distributed under the terms of
 * the GNU Public License, in which case the provisions of the GPL are
 * required INSTEAD OF the above restrictions.  (This clause is
 * necessary due to a potential bad interaction between the GPL and
 * the restrictions contained in a BSD-style copyright.)
 *
 * THIS SOFTWARE IS PROVIDED `AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
