/*
   Unix SMB/CIFS implementation.
   kerberos keytab utility library
   Copyright (C) Andrew Tridgell 2001
   Copyright (C) Remus Koos 2001
   Copyright (C) Luke Howard 2003
   Copyright (C) Jim McDonough (jmcd@us.ibm.com) 2003
   Copyright (C) Guenther Deschner 2003
   Copyright (C) Rakesh Patel 2004
   Copyright (C) Dan Perry 2004
   Copyright (C) Jeremy Allison 2004
   Copyright (C) Gerald Carter 2006

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

/* This MAX_NAME_LEN is a constant defined in krb5.h */
#ifndef MAX_KEYTAB_NAME_LEN
#define MAX_KEYTAB_NAME_LEN 1100
#endif


/**********************************************************************
**********************************************************************/

static int smb_krb5_kt_add_entry( krb5_context context, krb5_keytab keytab,
                                  krb5_kvno kvno, const char *princ_s, 
				  krb5_enctype *enctypes, krb5_data password )
{
	krb5_error_code ret = 0;
	krb5_kt_cursor cursor;
	krb5_keytab_entry kt_entry;
	krb5_principal princ = NULL;
	int i;
	char *ktprinc = NULL;

	ZERO_STRUCT(kt_entry);
	ZERO_STRUCT(cursor);
	
	ret = smb_krb5_parse_name(context, princ_s, &princ);
	if (ret) {
		DEBUG(1,("ads_keytab_add_entry: smb_krb5_parse_name(%s) failed (%s)\n", princ_s, error_message(ret)));
		goto out;
	}

	/* Seek and delete old keytab entries */
	ret = krb5_kt_start_seq_get(context, keytab, &cursor);
	if (ret != KRB5_KT_END && ret != ENOENT ) {
		DEBUG(3,("ads_keytab_add_entry: Will try to delete old keytab entries\n"));
		while(!krb5_kt_next_entry(context, keytab, &kt_entry, &cursor)) {
			BOOL compare_name_ok = False;

			ret = smb_krb5_unparse_name(context, kt_entry.principal, &ktprinc);
			if (ret) {
				DEBUG(1,("ads_keytab_add_entry: smb_krb5_unparse_name failed (%s)\n",
					error_message(ret)));
				goto out;
			}

			/*---------------------------------------------------------------------------
			 * Save the entries with kvno - 1.   This is what microsoft does
			 * to allow people with existing sessions that have kvno - 1 to still
			 * work.   Otherwise, when the password for the machine changes, all
			 * kerberizied sessions will 'break' until either the client reboots or
			 * the client's session key expires and they get a new session ticket
			 * with the new kvno.
			 */

#ifdef HAVE_KRB5_KT_COMPARE
			compare_name_ok = (krb5_kt_compare(context, &kt_entry, princ, 0, 0) == True);
#else
			compare_name_ok = (strcmp(ktprinc, princ_s) == 0);
#endif

			if (!compare_name_ok) {
				DEBUG(10,("ads_keytab_add_entry: ignoring keytab entry principal %s, kvno = %d\n",
					ktprinc, kt_entry.vno));
			}

			SAFE_FREE(ktprinc);

			if (compare_name_ok) {
				if (kt_entry.vno == kvno - 1) {
					DEBUG(5,("ads_keytab_add_entry: Saving previous (kvno %d) entry for principal: %s.\n",
						kvno - 1, princ_s));
				} else {

					DEBUG(5,("ads_keytab_add_entry: Found old entry for principal: %s (kvno %d) - trying to remove it.\n",
						princ_s, kt_entry.vno));
					ret = krb5_kt_end_seq_get(context, keytab, &cursor);
					ZERO_STRUCT(cursor);
					if (ret) {
						DEBUG(1,("ads_keytab_add_entry: krb5_kt_end_seq_get() failed (%s)\n",
							error_message(ret)));
						goto out;
					}
					ret = krb5_kt_remove_entry(context, keytab, &kt_entry);
					if (ret) {
						DEBUG(1,("ads_keytab_add_entry: krb5_kt_remove_entry failed (%s)\n",
							error_message(ret)));
						goto out;
					}

					DEBUG(5,("ads_keytab_add_entry: removed old entry for principal: %s (kvno %d).\n",
						princ_s, kt_entry.vno));

					ret = krb5_kt_start_seq_get(context, keytab, &cursor);
					if (ret) {
						DEBUG(1,("ads_keytab_add_entry: krb5_kt_start_seq failed (%s)\n",
							error_message(ret)));
						goto out;
					}
					ret = smb_krb5_kt_free_entry(context, &kt_entry);
					ZERO_STRUCT(kt_entry);
					if (ret) {
						DEBUG(1,("ads_keytab_add_entry: krb5_kt_remove_entry failed (%s)\n",
							error_message(ret)));
						goto out;
					}
					continue;
				}
			}

			/* Not a match, just free this entry and continue. */
			ret = smb_krb5_kt_free_entry(context, &kt_entry);
			ZERO_STRUCT(kt_entry);
			if (ret) {
				DEBUG(1,("ads_keytab_add_entry: smb_krb5_kt_free_entry failed (%s)\n", error_message(ret)));
				goto out;
			}
		}

		ret = krb5_kt_end_seq_get(context, keytab, &cursor);
		ZERO_STRUCT(cursor);
		if (ret) {
			DEBUG(1,("ads_keytab_add_entry: krb5_kt_end_seq_get failed (%s)\n",error_message(ret)));
			goto out;
		}
	}

	/* Ensure we don't double free. */
	ZERO_STRUCT(kt_entry);
	ZERO_STRUCT(cursor);

	/* If we get here, we have deleted all the old entries with kvno's not equal to the current kvno-1. */

	/* Now add keytab entries for all encryption types */
	for (i = 0; enctypes[i]; i++) {
		krb5_keyblock *keyp;

#if !defined(HAVE_KRB5_KEYTAB_ENTRY_KEY) && !defined(HAVE_KRB5_KEYTAB_ENTRY_KEYBLOCK)
#error krb5_keytab_entry has no key or keyblock member
#endif
#ifdef HAVE_KRB5_KEYTAB_ENTRY_KEY               /* MIT */
		keyp = &kt_entry.key;
#endif
#ifdef HAVE_KRB5_KEYTAB_ENTRY_KEYBLOCK          /* Heimdal */
		keyp = &kt_entry.keyblock;
#endif
		if (create_kerberos_key_from_string(context, princ, &password, keyp, enctypes[i])) {
			continue;
		}

		kt_entry.principal = princ;
		kt_entry.vno       = kvno;

		DEBUG(3,("ads_keytab_add_entry: adding keytab entry for (%s) with encryption type (%d) and version (%d)\n",
			princ_s, enctypes[i], kt_entry.vno));
		ret = krb5_kt_add_entry(context, keytab, &kt_entry);
		krb5_free_keyblock_contents(context, keyp);
		ZERO_STRUCT(kt_entry);
		if (ret) {
			DEBUG(1,("ads_keytab_add_entry: adding entry to keytab failed (%s)\n", error_message(ret)));
			goto out;
		}
	}


out:
	{
		krb5_keytab_entry zero_kt_entry;
		ZERO_STRUCT(zero_kt_entry);
		if (memcmp(&zero_kt_entry, &kt_entry, sizeof(krb5_keytab_entry))) {
			smb_krb5_kt_free_entry(context, &kt_entry);
		}
	}
	if (princ) {
		krb5_free_principal(context, princ);
	}
	
	{
		krb5_kt_cursor zero_csr;
		ZERO_STRUCT(zero_csr);
		if ((memcmp(&cursor, &zero_csr, sizeof(krb5_kt_cursor)) != 0) && keytab) {
			krb5_kt_end_seq_get(context, keytab, &cursor);	
		}
	}
	
	return (int)ret;
}


/**********************************************************************
 Adds a single service principal, i.e. 'host' to the system keytab
***********************************************************************/

int ads_keytab_add_entry(ADS_STRUCT *ads, const char *srvPrinc)
{
	krb5_error_code ret = 0;
	krb5_context context = NULL;
	krb5_keytab keytab = NULL;
	krb5_data password;
	krb5_kvno kvno;
        krb5_enctype enctypes[4] = { ENCTYPE_DES_CBC_CRC, ENCTYPE_DES_CBC_MD5, 0, 0 };
	char *princ_s = NULL, *short_princ_s = NULL;
	char *password_s = NULL;
	char *my_fqdn;
	char keytab_name[MAX_KEYTAB_NAME_LEN];          
	TALLOC_CTX *ctx = NULL;
	char *machine_name;

#if defined(ENCTYPE_ARCFOUR_HMAC)
        enctypes[2] = ENCTYPE_ARCFOUR_HMAC;
#endif

	initialize_krb5_error_table();
	ret = krb5_init_context(&context);
	if (ret) {
		DEBUG(1,("ads_keytab_add_entry: could not krb5_init_context: %s\n",error_message(ret)));
		return -1;
	}
	
#ifdef HAVE_WRFILE_KEYTAB       /* MIT */
	keytab_name[0] = 'W';
	keytab_name[1] = 'R';
	ret = krb5_kt_default_name(context, (char *) &keytab_name[2], MAX_KEYTAB_NAME_LEN - 4);
#else                           /* Heimdal */
	ret = krb5_kt_default_name(context, (char *) &keytab_name[0], MAX_KEYTAB_NAME_LEN - 2);
#endif
	if (ret) {
		DEBUG(1,("ads_keytab_add_entry: krb5_kt_default_name failed (%s)\n", error_message(ret)));
		goto out;
	}
	DEBUG(2,("ads_keytab_add_entry: Using default system keytab: %s\n", (char *) &keytab_name));
	ret = krb5_kt_resolve(context, (char *) &keytab_name, &keytab);
	if (ret) {
		DEBUG(1,("ads_keytab_add_entry: krb5_kt_resolve failed (%s)\n", error_message(ret)));
		goto out;
	}

	/* retrieve the password */
	if (!secrets_init()) {
		DEBUG(1,("ads_keytab_add_entry: secrets_init failed\n"));
		ret = -1;
		goto out;
	}
	password_s = secrets_fetch_machine_password(lp_workgroup(), NULL, NULL);
	if (!password_s) {
		DEBUG(1,("ads_keytab_add_entry: failed to fetch machine password\n"));
		ret = -1;
		goto out;
	}
	password.data = password_s;
	password.length = strlen(password_s);

	/* we need the dNSHostName value here */
	
	if ( (ctx = talloc_init("ads_keytab_add_entry")) == NULL ) {
		DEBUG(0,("ads_keytab_add_entry: talloc() failed!\n"));
		ret = -1;
		goto out;
	}
	
	if ( (my_fqdn = ads_get_dnshostname( ads, ctx, global_myname())) == NULL ) {
		DEBUG(0,("ads_keytab_add_entry: unable to determine machine account's dns name in AD!\n"));
		ret = -1;
		goto out;	
	}
	
	if ( (machine_name = ads_get_samaccountname( ads, ctx, global_myname())) == NULL ) {
		DEBUG(0,("ads_keytab_add_entry: unable to determine machine account's short name in AD!\n"));
		ret = -1;
		goto out;	
	}
	/*strip the trailing '$' */
	machine_name[strlen(machine_name)-1] = '\0';
		
	/* Construct our principal */

	if (strchr_m(srvPrinc, '@')) {
		/* It's a fully-named principal. */
		asprintf(&princ_s, "%s", srvPrinc);
	} else if (srvPrinc[strlen(srvPrinc)-1] == '$') {
		/* It's the machine account, as used by smbclient clients. */
		asprintf(&princ_s, "%s@%s", srvPrinc, lp_realm());
	} else {
		/* It's a normal service principal.  Add the SPN now so that we
		 * can obtain credentials for it and double-check the salt value
		 * used to generate the service's keys. */
		 
		asprintf(&princ_s, "%s/%s@%s", srvPrinc, my_fqdn, lp_realm());
		asprintf(&short_princ_s, "%s/%s@%s", srvPrinc, machine_name, lp_realm());
		
		/* According to http://support.microsoft.com/kb/326985/en-us, 
		   certain principal names are automatically mapped to the host/...
		   principal in the AD account.  So only create these in the 
		   keytab, not in AD.  --jerry */
		   
		if ( !strequal( srvPrinc, "cifs" ) && !strequal(srvPrinc, "host" ) ) {
			DEBUG(3,("ads_keytab_add_entry: Attempting to add/update '%s'\n", princ_s));
			
			if (!ADS_ERR_OK(ads_add_service_principal_name(ads, global_myname(), my_fqdn, srvPrinc))) {
				DEBUG(1,("ads_keytab_add_entry: ads_add_service_principal_name failed.\n"));
				goto out;
			}
		}
	}

	kvno = (krb5_kvno) ads_get_kvno(ads, global_myname());
	if (kvno == -1) {       /* -1 indicates failure, everything else is OK */
		DEBUG(1,("ads_keytab_add_entry: ads_get_kvno failed to determine the system's kvno.\n"));
		ret = -1;
		goto out;
	}
	
	/* add the fqdn principal to the keytab */
	
	ret = smb_krb5_kt_add_entry( context, keytab, kvno, princ_s, enctypes, password );
	if ( ret ) {
		DEBUG(1,("ads_keytab_add_entry: Failed to add entry to keytab file\n"));
		goto out;
	}
	
	/* add the short principal name if we have one */
	
	if ( short_princ_s ) {
		ret = smb_krb5_kt_add_entry( context, keytab, kvno, short_princ_s, enctypes, password );
		if ( ret ) {
			DEBUG(1,("ads_keytab_add_entry: Failed to add short entry to keytab file\n"));
			goto out;
		}
	}

out:
	SAFE_FREE( princ_s );
	SAFE_FREE( short_princ_s );
	TALLOC_FREE( ctx );
	
	if (keytab) {
		krb5_kt_close(context, keytab);
	}
	if (context) {
		krb5_free_context(context);
	}
	return (int)ret;
}

/**********************************************************************
 Flushes all entries from the system keytab.
***********************************************************************/

int ads_keytab_flush(ADS_STRUCT *ads)
{
	krb5_error_code ret = 0;
	krb5_context context = NULL;
	krb5_keytab keytab = NULL;
	krb5_kt_cursor cursor;
	krb5_keytab_entry kt_entry;
	krb5_kvno kvno;
	char keytab_name[MAX_KEYTAB_NAME_LEN];

	ZERO_STRUCT(kt_entry);
	ZERO_STRUCT(cursor);

	initialize_krb5_error_table();
	ret = krb5_init_context(&context);
	if (ret) {
		DEBUG(1,("ads_keytab_flush: could not krb5_init_context: %s\n",error_message(ret)));
		return ret;
	}
#ifdef HAVE_WRFILE_KEYTAB
	keytab_name[0] = 'W';
	keytab_name[1] = 'R';
	ret = krb5_kt_default_name(context, (char *) &keytab_name[2], MAX_KEYTAB_NAME_LEN - 4);
#else
	ret = krb5_kt_default_name(context, (char *) &keytab_name[0], MAX_KEYTAB_NAME_LEN - 2);
#endif
	if (ret) {
		DEBUG(1,("ads_keytab_flush: krb5_kt_default failed (%s)\n", error_message(ret)));
		goto out;
	}
	DEBUG(3,("ads_keytab_flush: Using default keytab: %s\n", (char *) &keytab_name));
	ret = krb5_kt_resolve(context, (char *) &keytab_name, &keytab);
	if (ret) {
		DEBUG(1,("ads_keytab_flush: krb5_kt_default failed (%s)\n", error_message(ret)));
		goto out;
	}
	ret = krb5_kt_resolve(context, (char *) &keytab_name, &keytab);
	if (ret) {
		DEBUG(1,("ads_keytab_flush: krb5_kt_default failed (%s)\n", error_message(ret)));
		goto out;
	}

	kvno = (krb5_kvno) ads_get_kvno(ads, global_myname());
	if (kvno == -1) {       /* -1 indicates a failure */
		DEBUG(1,("ads_keytab_flush: Error determining the system's kvno.\n"));
		goto out;
	}

	ret = krb5_kt_start_seq_get(context, keytab, &cursor);
	if (ret != KRB5_KT_END && ret != ENOENT) {
		while (!krb5_kt_next_entry(context, keytab, &kt_entry, &cursor)) {
			ret = krb5_kt_end_seq_get(context, keytab, &cursor);
			ZERO_STRUCT(cursor);
			if (ret) {
				DEBUG(1,("ads_keytab_flush: krb5_kt_end_seq_get() failed (%s)\n",error_message(ret)));
				goto out;
			}
			ret = krb5_kt_remove_entry(context, keytab, &kt_entry);
			if (ret) {
				DEBUG(1,("ads_keytab_flush: krb5_kt_remove_entry failed (%s)\n",error_message(ret)));
				goto out;
			}
			ret = krb5_kt_start_seq_get(context, keytab, &cursor);
			if (ret) {
				DEBUG(1,("ads_keytab_flush: krb5_kt_start_seq failed (%s)\n",error_message(ret)));
				goto out;
			}
			ret = smb_krb5_kt_free_entry(context, &kt_entry);
			ZERO_STRUCT(kt_entry);
			if (ret) {
				DEBUG(1,("ads_keytab_flush: krb5_kt_remove_entry failed (%s)\n",error_message(ret)));
				goto out;
			}
		}
	}

	/* Ensure we don't double free. */
	ZERO_STRUCT(kt_entry);
	ZERO_STRUCT(cursor);

	if (!ADS_ERR_OK(ads_clear_service_principal_names(ads, global_myname()))) {
		DEBUG(1,("ads_keytab_flush: Error while clearing service principal listings in LDAP.\n"));
		goto out;
	}

out:

	{
		krb5_keytab_entry zero_kt_entry;
		ZERO_STRUCT(zero_kt_entry);
		if (memcmp(&zero_kt_entry, &kt_entry, sizeof(krb5_keytab_entry))) {
			smb_krb5_kt_free_entry(context, &kt_entry);
		}
	}
	{
		krb5_kt_cursor zero_csr;
		ZERO_STRUCT(zero_csr);
		if ((memcmp(&cursor, &zero_csr, sizeof(krb5_kt_cursor)) != 0) && keytab) {
			krb5_kt_end_seq_get(context, keytab, &cursor);	
		}
	}
	if (keytab) {
		krb5_kt_close(context, keytab);
	}
	if (context) {
		krb5_free_context(context);
	}
	return ret;
}

/**********************************************************************
 Adds all the required service principals to the system keytab.
***********************************************************************/

int ads_keytab_create_default(ADS_STRUCT *ads)
{
	krb5_error_code ret = 0;
	krb5_context context = NULL;
	krb5_keytab keytab = NULL;
	krb5_kt_cursor cursor;
	krb5_keytab_entry kt_entry;
	krb5_kvno kvno;
	int i, found = 0;
	char *sam_account_name, *upn;
	char **oldEntries = NULL, *princ_s[26];
	TALLOC_CTX *ctx = NULL;
	fstring machine_name;

	memset(princ_s, '\0', sizeof(princ_s));

	fstrcpy( machine_name, global_myname() );

	/* these are the main ones we need */
	
	if ( (ret = ads_keytab_add_entry(ads, "host") ) != 0 ) {
		DEBUG(1,("ads_keytab_create_default: ads_keytab_add_entry failed while adding 'host'.\n"));
		return ret;
	}


#if 0	/* don't create the CIFS/... keytab entries since no one except smbd 
	   really needs them and we will fall back to verifying against secrets.tdb */
	   
	if ( (ret = ads_keytab_add_entry(ads, "cifs")) != 0 ) {
		DEBUG(1,("ads_keytab_create_default: ads_keytab_add_entry failed while adding 'cifs'.\n"));
		return ret;
	}
#endif

	if ( (ctx = talloc_init("ads_keytab_create_default")) == NULL ) {
		DEBUG(0,("ads_keytab_create_default: talloc() failed!\n"));
		return -1;
	}

	/* now add the userPrincipalName and sAMAccountName entries */
	
	if ( (sam_account_name = ads_get_samaccountname( ads, ctx, machine_name)) == NULL ) {
		DEBUG(0,("ads_keytab_add_entry: unable to determine machine account's name in AD!\n"));
		TALLOC_FREE( ctx );
		return -1;	
	}

	if ( (ret = ads_keytab_add_entry(ads, sam_account_name )) != 0 ) {
		DEBUG(1,("ads_keytab_create_default: ads_keytab_add_entry failed while adding sAMAccountName (%s)\n",
			sam_account_name));
		return ret;
	}
	
	/* remember that not every machine account will have a upn */
		
	upn = ads_get_upn( ads, ctx, machine_name);
	if ( upn ) {
		if ( (ret = ads_keytab_add_entry(ads, upn)) != 0 ) {
			DEBUG(1,("ads_keytab_create_default: ads_keytab_add_entry failed while adding UPN (%s)\n",
				upn));
			TALLOC_FREE( ctx );
			return ret;
		}
	}

	TALLOC_FREE( ctx );

	/* Now loop through the keytab and update any other existing entries... */
	
	kvno = (krb5_kvno) ads_get_kvno(ads, machine_name);
	if (kvno == -1) {
		DEBUG(1,("ads_keytab_create_default: ads_get_kvno failed to determine the system's kvno.\n"));
		return -1;
	}
	
	DEBUG(3,("ads_keytab_create_default: Searching for keytab entries to "
		"preserve and update.\n"));

	ZERO_STRUCT(kt_entry);
	ZERO_STRUCT(cursor);

	initialize_krb5_error_table();
	ret = krb5_init_context(&context);
	if (ret) {
		DEBUG(1,("ads_keytab_create_default: could not krb5_init_context: %s\n",error_message(ret)));
		return ret;
	}
	ret = krb5_kt_default(context, &keytab);
	if (ret) {
		DEBUG(1,("ads_keytab_create_default: krb5_kt_default failed (%s)\n",error_message(ret)));
		goto done;
	}

	ret = krb5_kt_start_seq_get(context, keytab, &cursor);
	if (ret != KRB5_KT_END && ret != ENOENT ) {
		while ((ret = krb5_kt_next_entry(context, keytab, &kt_entry, &cursor)) == 0) {
			smb_krb5_kt_free_entry(context, &kt_entry);
			ZERO_STRUCT(kt_entry);
			found++;
		}
	}
	krb5_kt_end_seq_get(context, keytab, &cursor);
	ZERO_STRUCT(cursor);

	/*
	 * Hmmm. There is no "rewind" function for the keytab. This means we have a race condition
	 * where someone else could add entries after we've counted them. Re-open asap to minimise
	 * the race. JRA.
	 */
	
	DEBUG(3, ("ads_keytab_create_default: Found %d entries in the keytab.\n", found));
	if (!found) {
		goto done;
	}
	oldEntries = SMB_MALLOC_ARRAY(char *, found );
	if (!oldEntries) {
		DEBUG(1,("ads_keytab_create_default: Failed to allocate space to store the old keytab entries (malloc failed?).\n"));
		ret = -1;
		goto done;
	}
	memset(oldEntries, '\0', found * sizeof(char *));

	ret = krb5_kt_start_seq_get(context, keytab, &cursor);
	if (ret != KRB5_KT_END && ret != ENOENT ) {
		while (krb5_kt_next_entry(context, keytab, &kt_entry, &cursor) == 0) {
			if (kt_entry.vno != kvno) {
				char *ktprinc = NULL;
				char *p;

				/* This returns a malloc'ed string in ktprinc. */
				ret = smb_krb5_unparse_name(context, kt_entry.principal, &ktprinc);
				if (ret) {
					DEBUG(1,("smb_krb5_unparse_name failed (%s)\n", error_message(ret)));
					goto done;
				}
				/*
				 * From looking at the krb5 source they don't seem to take locale
				 * or mb strings into account. Maybe this is because they assume utf8 ?
				 * In this case we may need to convert from utf8 to mb charset here ? JRA.
				 */
				p = strchr_m(ktprinc, '@');
				if (p) {
					*p = '\0';
				}

				p = strchr_m(ktprinc, '/');
				if (p) {
					*p = '\0';
				}
				for (i = 0; i < found; i++) {
					if (!oldEntries[i]) {
						oldEntries[i] = ktprinc;
						break;
					}
					if (!strcmp(oldEntries[i], ktprinc)) {
						SAFE_FREE(ktprinc);
						break;
					}
				}
				if (i == found) {
					SAFE_FREE(ktprinc);
				}
			}
			smb_krb5_kt_free_entry(context, &kt_entry);
			ZERO_STRUCT(kt_entry);
		}
		ret = 0;
		for (i = 0; oldEntries[i]; i++) {
			ret |= ads_keytab_add_entry(ads, oldEntries[i]);
			SAFE_FREE(oldEntries[i]);
		}
		krb5_kt_end_seq_get(context, keytab, &cursor);
	}
	ZERO_STRUCT(cursor);

done:

	SAFE_FREE(oldEntries);

	{
		krb5_keytab_entry zero_kt_entry;
		ZERO_STRUCT(zero_kt_entry);
		if (memcmp(&zero_kt_entry, &kt_entry, sizeof(krb5_keytab_entry))) {
			smb_krb5_kt_free_entry(context, &kt_entry);
		}
	}
	{
		krb5_kt_cursor zero_csr;
		ZERO_STRUCT(zero_csr);
		if ((memcmp(&cursor, &zero_csr, sizeof(krb5_kt_cursor)) != 0) && keytab) {
			krb5_kt_end_seq_get(context, keytab, &cursor);	
		}
	}
	if (keytab) {
		krb5_kt_close(context, keytab);
	}
	if (context) {
		krb5_free_context(context);
	}
	return ret;
}
#endif /* HAVE_KRB5 */
