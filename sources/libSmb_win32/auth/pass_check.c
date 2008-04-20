/* 
   Unix SMB/CIFS implementation.
   Password checking
   Copyright (C) Andrew Tridgell 1992-1998
   
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

/* this module is for checking a username/password against a system
   password database. The SMB encrypted password support is elsewhere */

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_AUTH

/* these are kept here to keep the string_combinations function simple */
static fstring this_user;
#if !defined(WITH_PAM) 
static fstring this_salt;
static fstring this_crypted;
#endif

#ifdef WITH_AFS

#include <afs/stds.h>
#include <afs/kautils.h>

/*******************************************************************
check on AFS authentication
********************************************************************/
static BOOL afs_auth(char *user, char *password)
{
	long password_expires = 0;
	char *reason;

	/* For versions of AFS prior to 3.3, this routine has few arguments, */
	/* but since I can't find the old documentation... :-)               */
	setpag();
	if (ka_UserAuthenticateGeneral
	    (KA_USERAUTH_VERSION + KA_USERAUTH_DOSETPAG, user, (char *)0,	/* instance */
	     (char *)0,		/* cell */
	     password, 0,	/* lifetime, default */
	     &password_expires,	/*days 'til it expires */
	     0,			/* spare 2 */
	     &reason) == 0)
	{
		return (True);
	}
	DEBUG(1,
	      ("AFS authentication for \"%s\" failed (%s)\n", user, reason));
	return (False);
}
#endif


#ifdef WITH_DFS

#include <dce/dce_error.h>
#include <dce/sec_login.h>

/*****************************************************************
 This new version of the DFS_AUTH code was donated by Karsten Muuss
 <muuss@or.uni-bonn.de>. It fixes the following problems with the
 old code :

  - Server credentials may expire
  - Client credential cache files have wrong owner
  - purge_context() function is called with invalid argument

 This new code was modified to ensure that on exit the uid/gid is
 still root, and the original directory is restored. JRA.
******************************************************************/

sec_login_handle_t my_dce_sec_context;
int dcelogin_atmost_once = 0;

/*******************************************************************
check on a DCE/DFS authentication
********************************************************************/
static BOOL dfs_auth(char *user, char *password)
{
	struct tm *t;
	error_status_t err;
	int err2;
	int prterr;
	signed32 expire_time, current_time;
	boolean32 password_reset;
	struct passwd *pw;
	sec_passwd_rec_t passwd_rec;
	sec_login_auth_src_t auth_src = sec_login_auth_src_network;
	unsigned char dce_errstr[dce_c_error_string_len];
	gid_t egid;

	if (dcelogin_atmost_once)
		return (False);

#ifdef HAVE_CRYPT
	/*
	 * We only go for a DCE login context if the given password
	 * matches that stored in the local password file.. 
	 * Assumes local passwd file is kept in sync w/ DCE RGY!
	 */

	if (strcmp((char *)crypt(password, this_salt), this_crypted))
	{
		return (False);
	}
#endif

	sec_login_get_current_context(&my_dce_sec_context, &err);
	if (err != error_status_ok)
	{
		dce_error_inq_text(err, dce_errstr, &err2);
		DEBUG(0, ("DCE can't get current context. %s\n", dce_errstr));

		return (False);
	}

	sec_login_certify_identity(my_dce_sec_context, &err);
	if (err != error_status_ok)
	{
		dce_error_inq_text(err, dce_errstr, &err2);
		DEBUG(0, ("DCE can't get current context. %s\n", dce_errstr));

		return (False);
	}

	sec_login_get_expiration(my_dce_sec_context, &expire_time, &err);
	if (err != error_status_ok)
	{
		dce_error_inq_text(err, dce_errstr, &err2);
		DEBUG(0, ("DCE can't get expiration. %s\n", dce_errstr));

		return (False);
	}

	time(&current_time);

	if (expire_time < (current_time + 60))
	{
		struct passwd *pw;
		sec_passwd_rec_t *key;

		sec_login_get_pwent(my_dce_sec_context,
				    (sec_login_passwd_t *) & pw, &err);
		if (err != error_status_ok)
		{
			dce_error_inq_text(err, dce_errstr, &err2);
			DEBUG(0, ("DCE can't get pwent. %s\n", dce_errstr));

			return (False);
		}

		sec_login_refresh_identity(my_dce_sec_context, &err);
		if (err != error_status_ok)
		{
			dce_error_inq_text(err, dce_errstr, &err2);
			DEBUG(0, ("DCE can't refresh identity. %s\n",
				  dce_errstr));

			return (False);
		}

		sec_key_mgmt_get_key(rpc_c_authn_dce_secret, NULL,
				     (unsigned char *)pw->pw_name,
				     sec_c_key_version_none,
				     (void **)&key, &err);
		if (err != error_status_ok)
		{
			dce_error_inq_text(err, dce_errstr, &err2);
			DEBUG(0, ("DCE can't get key for %s. %s\n",
				  pw->pw_name, dce_errstr));

			return (False);
		}

		sec_login_valid_and_cert_ident(my_dce_sec_context, key,
					       &password_reset, &auth_src,
					       &err);
		if (err != error_status_ok)
		{
			dce_error_inq_text(err, dce_errstr, &err2);
			DEBUG(0,
			      ("DCE can't validate and certify identity for %s. %s\n",
			       pw->pw_name, dce_errstr));
		}

		sec_key_mgmt_free_key(key, &err);
		if (err != error_status_ok)
		{
			dce_error_inq_text(err, dce_errstr, &err2);
			DEBUG(0, ("DCE can't free key.\n", dce_errstr));
		}
	}

	if (sec_login_setup_identity((unsigned char *)user,
				     sec_login_no_flags,
				     &my_dce_sec_context, &err) == 0)
	{
		dce_error_inq_text(err, dce_errstr, &err2);
		DEBUG(0, ("DCE Setup Identity for %s failed: %s\n",
			  user, dce_errstr));
		return (False);
	}

	sec_login_get_pwent(my_dce_sec_context,
			    (sec_login_passwd_t *) & pw, &err);
	if (err != error_status_ok)
	{
		dce_error_inq_text(err, dce_errstr, &err2);
		DEBUG(0, ("DCE can't get pwent. %s\n", dce_errstr));

		return (False);
	}

	sec_login_purge_context(&my_dce_sec_context, &err);
	if (err != error_status_ok)
	{
		dce_error_inq_text(err, dce_errstr, &err2);
		DEBUG(0, ("DCE can't purge context. %s\n", dce_errstr));

		return (False);
	}

	/*
	 * NB. I'd like to change these to call something like change_to_user()
	 * instead but currently we don't have a connection
	 * context to become the correct user. This is already
	 * fairly platform specific code however, so I think
	 * this should be ok. I have added code to go
	 * back to being root on error though. JRA.
	 */

	egid = getegid();

	set_effective_gid(pw->pw_gid);
	set_effective_uid(pw->pw_uid);

	if (sec_login_setup_identity((unsigned char *)user,
				     sec_login_no_flags,
				     &my_dce_sec_context, &err) == 0)
	{
		dce_error_inq_text(err, dce_errstr, &err2);
		DEBUG(0, ("DCE Setup Identity for %s failed: %s\n",
			  user, dce_errstr));
		goto err;
	}

	sec_login_get_pwent(my_dce_sec_context,
			    (sec_login_passwd_t *) & pw, &err);
	if (err != error_status_ok)
	{
		dce_error_inq_text(err, dce_errstr, &err2);
		DEBUG(0, ("DCE can't get pwent. %s\n", dce_errstr));
		goto err;
	}

	passwd_rec.version_number = sec_passwd_c_version_none;
	passwd_rec.pepper = NULL;
	passwd_rec.key.key_type = sec_passwd_plain;
	passwd_rec.key.tagged_union.plain = (idl_char *) password;

	sec_login_validate_identity(my_dce_sec_context,
				    &passwd_rec, &password_reset,
				    &auth_src, &err);
	if (err != error_status_ok)
	{
		dce_error_inq_text(err, dce_errstr, &err2);
		DEBUG(0,
		      ("DCE Identity Validation failed for principal %s: %s\n",
		       user, dce_errstr));
		goto err;
	}

	sec_login_certify_identity(my_dce_sec_context, &err);
	if (err != error_status_ok)
	{
		dce_error_inq_text(err, dce_errstr, &err2);
		DEBUG(0, ("DCE certify identity failed: %s\n", dce_errstr));
		goto err;
	}

	if (auth_src != sec_login_auth_src_network)
	{
		DEBUG(0, ("DCE context has no network credentials.\n"));
	}

	sec_login_set_context(my_dce_sec_context, &err);
	if (err != error_status_ok)
	{
		dce_error_inq_text(err, dce_errstr, &err2);
		DEBUG(0,
		      ("DCE login failed for principal %s, cant set context: %s\n",
		       user, dce_errstr));

		sec_login_purge_context(&my_dce_sec_context, &err);
		goto err;
	}

	sec_login_get_pwent(my_dce_sec_context,
			    (sec_login_passwd_t *) & pw, &err);
	if (err != error_status_ok)
	{
		dce_error_inq_text(err, dce_errstr, &err2);
		DEBUG(0, ("DCE can't get pwent. %s\n", dce_errstr));
		goto err;
	}

	DEBUG(0, ("DCE login succeeded for principal %s on pid %d\n",
		  user, sys_getpid()));

	DEBUG(3, ("DCE principal: %s\n"
		  "          uid: %d\n"
		  "          gid: %d\n",
		  pw->pw_name, pw->pw_uid, pw->pw_gid));
	DEBUG(3, ("         info: %s\n"
		  "          dir: %s\n"
		  "        shell: %s\n",
		  pw->pw_gecos, pw->pw_dir, pw->pw_shell));

	sec_login_get_expiration(my_dce_sec_context, &expire_time, &err);
	if (err != error_status_ok)
	{
		dce_error_inq_text(err, dce_errstr, &err2);
		DEBUG(0, ("DCE can't get expiration. %s\n", dce_errstr));
		goto err;
	}

	set_effective_uid(0);
	set_effective_gid(0);

	t = localtime(&expire_time);
	if (t) {
		const char *asct = asctime(t);
		if (asct) {
			DEBUG(0,("DCE context expires: %s", asct));
		}
	}

	dcelogin_atmost_once = 1;
	return (True);

      err:

	/* Go back to root, JRA. */
	set_effective_uid(0);
	set_effective_gid(egid);
	return (False);
}

void dfs_unlogin(void)
{
	error_status_t err;
	int err2;
	unsigned char dce_errstr[dce_c_error_string_len];

	sec_login_purge_context(&my_dce_sec_context, &err);
	if (err != error_status_ok)
	{
		dce_error_inq_text(err, dce_errstr, &err2);
		DEBUG(0,
		      ("DCE purge login context failed for server instance %d: %s\n",
		       sys_getpid(), dce_errstr));
	}
}
#endif

#ifdef LINUX_BIGCRYPT
/****************************************************************************
an enhanced crypt for Linux to handle password longer than 8 characters
****************************************************************************/
static int linux_bigcrypt(char *password, char *salt1, char *crypted)
{
#define LINUX_PASSWORD_SEG_CHARS 8
	char salt[3];
	int i;

	StrnCpy(salt, salt1, 2);
	crypted += 2;

	for (i = strlen(password); i > 0; i -= LINUX_PASSWORD_SEG_CHARS) {
		char *p = crypt(password, salt) + 2;
		if (strncmp(p, crypted, LINUX_PASSWORD_SEG_CHARS) != 0)
			return (0);
		password += LINUX_PASSWORD_SEG_CHARS;
		crypted += strlen(p);
	}

	return (1);
}
#endif

#ifdef OSF1_ENH_SEC
/****************************************************************************
an enhanced crypt for OSF1
****************************************************************************/
static char *osf1_bigcrypt(char *password, char *salt1)
{
	static char result[AUTH_MAX_PASSWD_LENGTH] = "";
	char *p1;
	char *p2 = password;
	char salt[3];
	int i;
	int parts = strlen(password) / AUTH_CLEARTEXT_SEG_CHARS;
	if (strlen(password) % AUTH_CLEARTEXT_SEG_CHARS)
		parts++;

	StrnCpy(salt, salt1, 2);
	StrnCpy(result, salt1, 2);
	result[2] = '\0';

	for (i = 0; i < parts; i++) {
		p1 = crypt(p2, salt);
		strncat(result, p1 + 2,
			AUTH_MAX_PASSWD_LENGTH - strlen(p1 + 2) - 1);
		StrnCpy(salt, &result[2 + i * AUTH_CIPHERTEXT_SEG_CHARS], 2);
		p2 += AUTH_CLEARTEXT_SEG_CHARS;
	}

	return (result);
}
#endif


/****************************************************************************
apply a function to upper/lower case combinations
of a string and return true if one of them returns true.
try all combinations with N uppercase letters.
offset is the first char to try and change (start with 0)
it assumes the string starts lowercased
****************************************************************************/
static NTSTATUS string_combinations2(char *s, int offset, NTSTATUS (*fn) (const char *),
				 int N)
{
	int len = strlen(s);
	int i;
	NTSTATUS nt_status;

#ifdef PASSWORD_LENGTH
	len = MIN(len, PASSWORD_LENGTH);
#endif

	if (N <= 0 || offset >= len)
		return (fn(s));

	for (i = offset; i < (len - (N - 1)); i++) {
		char c = s[i];
		if (!islower_ascii(c))
			continue;
		s[i] = toupper_ascii(c);
		if (!NT_STATUS_EQUAL(nt_status = string_combinations2(s, i + 1, fn, N - 1),NT_STATUS_WRONG_PASSWORD)) {
			return (nt_status);
		}
		s[i] = c;
	}
	return (NT_STATUS_WRONG_PASSWORD);
}

/****************************************************************************
apply a function to upper/lower case combinations
of a string and return true if one of them returns true.
try all combinations with up to N uppercase letters.
offset is the first char to try and change (start with 0)
it assumes the string starts lowercased
****************************************************************************/
static NTSTATUS string_combinations(char *s, NTSTATUS (*fn) (const char *), int N)
{
	int n;
	NTSTATUS nt_status;
	for (n = 1; n <= N; n++)
		if (!NT_STATUS_EQUAL(nt_status = string_combinations2(s, 0, fn, n), NT_STATUS_WRONG_PASSWORD))
			return nt_status;
	return NT_STATUS_WRONG_PASSWORD;
}


/****************************************************************************
core of password checking routine
****************************************************************************/
static NTSTATUS password_check(const char *password)
{
#ifdef WITH_PAM
	return smb_pam_passcheck(this_user, password);
#else

	BOOL ret;

#ifdef WITH_AFS
	if (afs_auth(this_user, password))
		return NT_STATUS_OK;
#endif /* WITH_AFS */

#ifdef WITH_DFS
	if (dfs_auth(this_user, password))
		return NT_STATUS_OK;
#endif /* WITH_DFS */

#ifdef OSF1_ENH_SEC
	
	ret = (strcmp(osf1_bigcrypt(password, this_salt),
		      this_crypted) == 0);
	if (!ret) {
		DEBUG(2,
		      ("OSF1_ENH_SEC failed. Trying normal crypt.\n"));
		ret = (strcmp((char *)crypt(password, this_salt), this_crypted) == 0);
	}
	if (ret) {
		return NT_STATUS_OK;
	} else {
		return NT_STATUS_WRONG_PASSWORD;
	}
	
#endif /* OSF1_ENH_SEC */
	
#ifdef ULTRIX_AUTH
	ret = (strcmp((char *)crypt16(password, this_salt), this_crypted) == 0);
	if (ret) {
		return NT_STATUS_OK;
        } else {
		return NT_STATUS_WRONG_PASSWORD;
	}
	
#endif /* ULTRIX_AUTH */
	
#ifdef LINUX_BIGCRYPT
	ret = (linux_bigcrypt(password, this_salt, this_crypted));
        if (ret) {
		return NT_STATUS_OK;
	} else {
		return NT_STATUS_WRONG_PASSWORD;
	}
#endif /* LINUX_BIGCRYPT */
	
#if defined(HAVE_BIGCRYPT) && defined(HAVE_CRYPT) && defined(USE_BOTH_CRYPT_CALLS)
	
	/*
	 * Some systems have bigcrypt in the C library but might not
	 * actually use it for the password hashes (HPUX 10.20) is
	 * a noteable example. So we try bigcrypt first, followed
	 * by crypt.
	 */

	if (strcmp(bigcrypt(password, this_salt), this_crypted) == 0)
		return NT_STATUS_OK;
	else
		ret = (strcmp((char *)crypt(password, this_salt), this_crypted) == 0);
	if (ret) {
		return NT_STATUS_OK;
	} else {
		return NT_STATUS_WRONG_PASSWORD;
	}
#else /* HAVE_BIGCRYPT && HAVE_CRYPT && USE_BOTH_CRYPT_CALLS */
	
#ifdef HAVE_BIGCRYPT
	ret = (strcmp(bigcrypt(password, this_salt), this_crypted) == 0);
        if (ret) {
		return NT_STATUS_OK;
	} else {
		return NT_STATUS_WRONG_PASSWORD;
	}
#endif /* HAVE_BIGCRYPT */
	
#ifndef HAVE_CRYPT
	DEBUG(1, ("Warning - no crypt available\n"));
	return NT_STATUS_LOGON_FAILURE;
#else /* HAVE_CRYPT */
	ret = (strcmp((char *)crypt(password, this_salt), this_crypted) == 0);
        if (ret) {
		return NT_STATUS_OK;
	} else {
		return NT_STATUS_WRONG_PASSWORD;
	}
#endif /* HAVE_CRYPT */
#endif /* HAVE_BIGCRYPT && HAVE_CRYPT && USE_BOTH_CRYPT_CALLS */
#endif /* WITH_PAM */
}



/****************************************************************************
CHECK if a username/password is OK
the function pointer fn() points to a function to call when a successful
match is found and is used to update the encrypted password file 
return NT_STATUS_OK on correct match, appropriate error otherwise
****************************************************************************/

NTSTATUS pass_check(const struct passwd *pass, const char *user, const char *password, 
		    int pwlen, BOOL (*fn) (const char *, const char *), BOOL run_cracker)
{
	pstring pass2;
	int level = lp_passwordlevel();

	NTSTATUS nt_status;

#ifdef DEBUG_PASSWORD
	DEBUG(100, ("checking user=[%s] pass=[%s]\n", user, password));
#endif

	if (!password)
		return NT_STATUS_LOGON_FAILURE;

	if (((!*password) || (!pwlen)) && !lp_null_passwords())
		return NT_STATUS_LOGON_FAILURE;

#if defined(WITH_PAM) 

	/*
	 * If we're using PAM we want to short-circuit all the 
	 * checks below and dive straight into the PAM code.
	 */

	fstrcpy(this_user, user);

	DEBUG(4, ("pass_check: Checking (PAM) password for user %s (l=%d)\n", user, pwlen));

#else /* Not using PAM */

	DEBUG(4, ("pass_check: Checking password for user %s (l=%d)\n", user, pwlen));

	if (!pass) {
		DEBUG(3, ("Couldn't find user %s\n", user));
		return NT_STATUS_NO_SUCH_USER;
	}


	/* Copy into global for the convenience of looping code */
	/* Also the place to keep the 'password' no matter what
	   crazy struct it started in... */
	fstrcpy(this_crypted, pass->pw_passwd);
	fstrcpy(this_salt, pass->pw_passwd);

#ifdef HAVE_GETSPNAM
	{
		struct spwd *spass;

		/* many shadow systems require you to be root to get
		   the password, in most cases this should already be
		   the case when this function is called, except
		   perhaps for IPC password changing requests */

		spass = getspnam(pass->pw_name);
		if (spass && spass->sp_pwdp) {
			fstrcpy(this_crypted, spass->sp_pwdp);
			fstrcpy(this_salt, spass->sp_pwdp);
		}
	}
#elif defined(IA_UINFO)
	{
		/* Need to get password with SVR4.2's ia_ functions
		   instead of get{sp,pw}ent functions. Required by
		   UnixWare 2.x, tested on version
		   2.1. (tangent@cyberport.com) */
		uinfo_t uinfo;
		if (ia_openinfo(pass->pw_name, &uinfo) != -1)
			ia_get_logpwd(uinfo, &(pass->pw_passwd));
	}
#endif

#ifdef HAVE_GETPRPWNAM
	{
		struct pr_passwd *pr_pw = getprpwnam(pass->pw_name);
		if (pr_pw && pr_pw->ufld.fd_encrypt)
			fstrcpy(this_crypted, pr_pw->ufld.fd_encrypt);
	}
#endif

#ifdef HAVE_GETPWANAM
	{
		struct passwd_adjunct *pwret;
		pwret = getpwanam(s);
		if (pwret && pwret->pwa_passwd)
			fstrcpy(this_crypted, pwret->pwa_passwd);
	}
#endif

#ifdef OSF1_ENH_SEC
	{
		struct pr_passwd *mypasswd;
		DEBUG(5, ("Checking password for user %s in OSF1_ENH_SEC\n",
			  user));
		mypasswd = getprpwnam(user);
		if (mypasswd) {
			fstrcpy(this_user, mypasswd->ufld.fd_name);
			fstrcpy(this_crypted, mypasswd->ufld.fd_encrypt);
		} else {
			DEBUG(5,
			      ("OSF1_ENH_SEC: No entry for user %s in protected database !\n",
			       user));
		}
	}
#endif

#ifdef ULTRIX_AUTH
	{
		AUTHORIZATION *ap = getauthuid(pass->pw_uid);
		if (ap) {
			fstrcpy(this_crypted, ap->a_password);
			endauthent();
		}
	}
#endif

#if defined(HAVE_TRUNCATED_SALT)
	/* crypt on some platforms (HPUX in particular)
	   won't work with more than 2 salt characters. */
	this_salt[2] = 0;
#endif

	if (!*this_crypted) {
		if (!lp_null_passwords()) {
			DEBUG(2, ("Disallowing %s with null password\n",
				  this_user));
			return NT_STATUS_LOGON_FAILURE;
		}
		if (!*password) {
			DEBUG(3,
			      ("Allowing access to %s with null password\n",
			       this_user));
			return NT_STATUS_OK;
		}
	}

#endif /* defined(WITH_PAM) */

	/* try it as it came to us */
	nt_status = password_check(password);
        if NT_STATUS_IS_OK(nt_status) {
                if (fn) {
                        fn(user, password);
		}
		return (nt_status);
	} else if (!NT_STATUS_EQUAL(nt_status, NT_STATUS_WRONG_PASSWORD)) {
                /* No point continuing if its not the password thats to blame (ie PAM disabled). */
                return (nt_status);
        }

	if (!run_cracker) {
		return (nt_status);
	}

	/* if the password was given to us with mixed case then we don't
	 * need to proceed as we know it hasn't been case modified by the
	 * client */
	if (strhasupper(password) && strhaslower(password)) {
		return nt_status;
	}

	/* make a copy of it */
	pstrcpy(pass2, password);

	/* try all lowercase if it's currently all uppercase */
	if (strhasupper(pass2)) {
		strlower_m(pass2);
		if NT_STATUS_IS_OK(nt_status = password_check(pass2)) {
		        if (fn)
				fn(user, pass2);
			return (nt_status);
		}
	}

	/* give up? */
	if (level < 1) {
		return NT_STATUS_WRONG_PASSWORD;
	}

	/* last chance - all combinations of up to level chars upper! */
	strlower_m(pass2);
 
        if (NT_STATUS_IS_OK(nt_status = string_combinations(pass2, password_check, level))) {
                if (fn)
			fn(user, pass2);
		return nt_status;
	}
        
	return NT_STATUS_WRONG_PASSWORD;
}
