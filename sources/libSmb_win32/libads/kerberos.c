/* 
   Unix SMB/CIFS implementation.
   kerberos utility library
   Copyright (C) Andrew Tridgell 2001
   Copyright (C) Remus Koos 2001
   Copyright (C) Nalin Dahyabhai <nalin@redhat.com> 2004.
   Copyright (C) Jeremy Allison 2004.
   Copyright (C) Gerald Carter 2006.

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

#ifdef HAVE_KRB5

#define LIBADS_CCACHE_NAME "MEMORY:libads"

/*
  we use a prompter to avoid a crash bug in the kerberos libs when 
  dealing with empty passwords
  this prompter is just a string copy ...
*/
static krb5_error_code 
kerb_prompter(krb5_context ctx, void *data,
	       const char *name,
	       const char *banner,
	       int num_prompts,
	       krb5_prompt prompts[])
{
	if (num_prompts == 0) return 0;

	memset(prompts[0].reply->data, '\0', prompts[0].reply->length);
	if (prompts[0].reply->length > 0) {
		if (data) {
			strncpy(prompts[0].reply->data, data, prompts[0].reply->length-1);
			prompts[0].reply->length = strlen(prompts[0].reply->data);
		} else {
			prompts[0].reply->length = 0;
		}
	}
	return 0;
}

/*
  simulate a kinit, putting the tgt in the given cache location. If cache_name == NULL
  place in default cache location.
  remus@snapserver.com
*/
int kerberos_kinit_password_ext(const char *principal,
				const char *password,
				int time_offset,
				time_t *expire_time,
				time_t *renew_till_time,
				const char *cache_name,
				BOOL request_pac,
				BOOL add_netbios_addr,
				time_t renewable_time)
{
	krb5_context ctx = NULL;
	krb5_error_code code = 0;
	krb5_ccache cc = NULL;
	krb5_principal me;
	krb5_creds my_creds;
	krb5_get_init_creds_opt opt;
	smb_krb5_addresses *addr = NULL;

	initialize_krb5_error_table();
	if ((code = krb5_init_context(&ctx)))
		return code;

	if (time_offset != 0) {
		krb5_set_real_time(ctx, time(NULL) + time_offset, 0);
	}

	DEBUG(10,("kerberos_kinit_password: using %s as ccache\n",
			cache_name ? cache_name: krb5_cc_default_name(ctx)));

	if ((code = krb5_cc_resolve(ctx, cache_name ? cache_name : krb5_cc_default_name(ctx), &cc))) {
		krb5_free_context(ctx);
		return code;
	}
	
	if ((code = smb_krb5_parse_name(ctx, principal, &me))) {
		krb5_free_context(ctx);	
		return code;
	}

	krb5_get_init_creds_opt_init(&opt);
	krb5_get_init_creds_opt_set_renew_life(&opt, renewable_time);
	krb5_get_init_creds_opt_set_forwardable(&opt, 1);
	
	if (request_pac) {
#ifdef HAVE_KRB5_GET_INIT_CREDS_OPT_SET_PAC_REQUEST
		code = krb5_get_init_creds_opt_set_pac_request(ctx, &opt, True);
		if (code) {
			krb5_free_principal(ctx, me);
			krb5_free_context(ctx);
			return code;
		}
#endif
	}

	if (add_netbios_addr) {
		code = smb_krb5_gen_netbios_krb5_address(&addr);
		if (code) {
			krb5_free_principal(ctx, me);
			krb5_free_context(ctx);		
			return code;	
		}
		krb5_get_init_creds_opt_set_address_list(&opt, addr->addrs);
	}

	if ((code = krb5_get_init_creds_password(ctx, &my_creds, me, CONST_DISCARD(char *,password), 
						 kerb_prompter, NULL, 0, NULL, &opt)))
	{
		smb_krb5_free_addresses(ctx, addr);
		krb5_free_principal(ctx, me);
		krb5_free_context(ctx);		
		return code;
	}
	
	if ((code = krb5_cc_initialize(ctx, cc, me))) {
		smb_krb5_free_addresses(ctx, addr);
		krb5_free_cred_contents(ctx, &my_creds);
		krb5_free_principal(ctx, me);
		krb5_free_context(ctx);		
		return code;
	}
	
	if ((code = krb5_cc_store_cred(ctx, cc, &my_creds))) {
		krb5_cc_close(ctx, cc);
		smb_krb5_free_addresses(ctx, addr);
		krb5_free_cred_contents(ctx, &my_creds);
		krb5_free_principal(ctx, me);
		krb5_free_context(ctx);		
		return code;
	}

	if (expire_time) {
		*expire_time = (time_t) my_creds.times.endtime;
	}

	if (renew_till_time) {
		*renew_till_time = (time_t) my_creds.times.renew_till;
	}

	krb5_cc_close(ctx, cc);
	smb_krb5_free_addresses(ctx, addr);
	krb5_free_cred_contents(ctx, &my_creds);
	krb5_free_principal(ctx, me);
	krb5_free_context(ctx);		
	
	return 0;
}



/* run kinit to setup our ccache */
int ads_kinit_password(ADS_STRUCT *ads)
{
	char *s;
	int ret;
	const char *account_name;
	fstring acct_name;

	if ( IS_DC ) {
		/* this will end up getting a ticket for DOMAIN@RUSTED.REA.LM */
		account_name = lp_workgroup();
	} else {
		/* always use the sAMAccountName for security = domain */
		/* global_myname()$@REA.LM */
		if ( lp_security() == SEC_DOMAIN ) {
			fstr_sprintf( acct_name, "%s$", global_myname() );
			account_name = acct_name;
		}
		else 
			/* This looks like host/global_myname()@REA.LM */
			account_name = ads->auth.user_name;
	}

	if (asprintf(&s, "%s@%s", account_name, ads->auth.realm) == -1) {
		return KRB5_CC_NOMEM;
	}

	if (!ads->auth.password) {
		SAFE_FREE(s);
		return KRB5_LIBOS_CANTREADPWD;
	}
	
	ret = kerberos_kinit_password_ext(s, ads->auth.password, ads->auth.time_offset,
			&ads->auth.expire, NULL, NULL, False, False, ads->auth.renewable);

	if (ret) {
		DEBUG(0,("kerberos_kinit_password %s failed: %s\n", 
			 s, error_message(ret)));
	}
	SAFE_FREE(s);
	return ret;
}

int ads_kdestroy(const char *cc_name)
{
	krb5_error_code code;
	krb5_context ctx = NULL;
	krb5_ccache cc = NULL;

	initialize_krb5_error_table();
	if ((code = krb5_init_context (&ctx))) {
		DEBUG(3, ("ads_kdestroy: kdb5_init_context failed: %s\n", 
			error_message(code)));
		return code;
	}
  
	if (!cc_name) {
		if ((code = krb5_cc_default(ctx, &cc))) {
			krb5_free_context(ctx);
			return code;
		}
	} else {
		if ((code = krb5_cc_resolve(ctx, cc_name, &cc))) {
			DEBUG(3, ("ads_kdestroy: krb5_cc_resolve failed: %s\n",
				  error_message(code)));
			krb5_free_context(ctx);
			return code;
		}
	}

	if ((code = krb5_cc_destroy (ctx, cc))) {
		DEBUG(3, ("ads_kdestroy: krb5_cc_destroy failed: %s\n", 
			error_message(code)));
	}

	krb5_free_context (ctx);
	return code;
}

/************************************************************************
 Routine to fetch the salting principal for a service.  Active
 Directory may use a non-obvious principal name to generate the salt
 when it determines the key to use for encrypting tickets for a service,
 and hopefully we detected that when we joined the domain.
 ************************************************************************/

static char *kerberos_secrets_fetch_salting_principal(const char *service, int enctype)
{
	char *key = NULL;
	char *ret = NULL;

	asprintf(&key, "%s/%s/enctype=%d", SECRETS_SALTING_PRINCIPAL, service, enctype);
	if (!key) {
		return NULL;
	}
	ret = (char *)secrets_fetch(key, NULL);
	SAFE_FREE(key);
	return ret;
}

/************************************************************************
 Return the standard DES salt key
************************************************************************/

char* kerberos_standard_des_salt( void )
{
	fstring salt;

	fstr_sprintf( salt, "host/%s.%s@", global_myname(), lp_realm() );
	strlower_m( salt );
	fstrcat( salt, lp_realm() );

	return SMB_STRDUP( salt );
}

/************************************************************************
************************************************************************/

static char* des_salt_key( void )
{
	char *key;

	asprintf(&key, "%s/DES/%s", SECRETS_SALTING_PRINCIPAL, lp_realm());

	return key;
}

/************************************************************************
************************************************************************/

BOOL kerberos_secrets_store_des_salt( const char* salt )
{
	char* key;
	BOOL ret;

	if ( (key = des_salt_key()) == NULL ) {
		DEBUG(0,("kerberos_secrets_store_des_salt: failed to generate key!\n"));
		return False;
	}

	if ( !salt ) {
		DEBUG(8,("kerberos_secrets_store_des_salt: deleting salt\n"));
		secrets_delete( key );
		return True;
	}

	DEBUG(3,("kerberos_secrets_store_des_salt: Storing salt \"%s\"\n", salt));

	ret = secrets_store( key, salt, strlen(salt)+1 );

	SAFE_FREE( key );

	return ret;
}

/************************************************************************
************************************************************************/

char* kerberos_secrets_fetch_des_salt( void )
{
	char *salt, *key;

	if ( (key = des_salt_key()) == NULL ) {
		DEBUG(0,("kerberos_secrets_fetch_des_salt: failed to generate key!\n"));
		return False;
	}

	salt = (char*)secrets_fetch( key, NULL );

	SAFE_FREE( key );

	return salt;
}


/************************************************************************
 Routine to get the salting principal for this service.  This is 
 maintained for backwards compatibilty with releases prior to 3.0.24.
 Since we store the salting principal string only at join, we may have 
 to look for the older tdb keys.  Caller must free if return is not null.
 ************************************************************************/

krb5_principal kerberos_fetch_salt_princ_for_host_princ(krb5_context context,
							krb5_principal host_princ,
							int enctype)
{
	char *unparsed_name = NULL, *salt_princ_s = NULL;
	krb5_principal ret_princ = NULL;
	
	/* lookup new key first */

	if ( (salt_princ_s = kerberos_secrets_fetch_des_salt()) == NULL ) {
	
		/* look under the old key.  If this fails, just use the standard key */

		if (smb_krb5_unparse_name(context, host_princ, &unparsed_name) != 0) {
			return (krb5_principal)NULL;
		}
		if ((salt_princ_s = kerberos_secrets_fetch_salting_principal(unparsed_name, enctype)) == NULL) {
			/* fall back to host/machine.realm@REALM */
			salt_princ_s = kerberos_standard_des_salt();
		}
	}

	if (smb_krb5_parse_name(context, salt_princ_s, &ret_princ) != 0) {
		ret_princ = NULL;
	}
	
	SAFE_FREE(unparsed_name);
	SAFE_FREE(salt_princ_s);
	
	return ret_princ;
}

/************************************************************************
 Routine to set the salting principal for this service.  Active
 Directory may use a non-obvious principal name to generate the salt
 when it determines the key to use for encrypting tickets for a service,
 and hopefully we detected that when we joined the domain.
 Setting principal to NULL deletes this entry.
 ************************************************************************/

BOOL kerberos_secrets_store_salting_principal(const char *service,
					      int enctype,
					      const char *principal)
{
	char *key = NULL;
	BOOL ret = False;
	krb5_context context = NULL;
	krb5_principal princ = NULL;
	char *princ_s = NULL;
	char *unparsed_name = NULL;

	krb5_init_context(&context);
	if (!context) {
		return False;
	}
	if (strchr_m(service, '@')) {
		asprintf(&princ_s, "%s", service);
	} else {
		asprintf(&princ_s, "%s@%s", service, lp_realm());
	}

	if (smb_krb5_parse_name(context, princ_s, &princ) != 0) {
		goto out;
		
	}
	if (smb_krb5_unparse_name(context, princ, &unparsed_name) != 0) {
		goto out;
	}

	asprintf(&key, "%s/%s/enctype=%d", SECRETS_SALTING_PRINCIPAL, unparsed_name, enctype);
	if (!key)  {
		goto out;
	}

	if ((principal != NULL) && (strlen(principal) > 0)) {
		ret = secrets_store(key, principal, strlen(principal) + 1);
	} else {
		ret = secrets_delete(key);
	}

 out:

	SAFE_FREE(key);
	SAFE_FREE(princ_s);
	SAFE_FREE(unparsed_name);

	if (context) {
		krb5_free_context(context);
	}

	return ret;
}


/************************************************************************
************************************************************************/

int kerberos_kinit_password(const char *principal,
			    const char *password,
			    int time_offset,
			    const char *cache_name)
{
	return kerberos_kinit_password_ext(principal, 
					   password, 
					   time_offset, 
					   0, 
					   0,
					   cache_name,
					   False,
					   False,
					   0);
}

#endif
