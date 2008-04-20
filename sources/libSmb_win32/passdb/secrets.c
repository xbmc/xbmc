/* 
   Unix SMB/CIFS implementation.
   Copyright (C) Andrew Tridgell 1992-2001
   Copyright (C) Andrew Bartlett      2002
   Copyright (C) Rafal Szczesniak     2002
   Copyright (C) Tim Potter           2001

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

/* the Samba secrets database stores any generated, private information
   such as the local SID and machine trust password */

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_PASSDB

static TDB_CONTEXT *tdb;

/* Urrrg. global.... */
BOOL global_machine_password_needs_changing;

/**
 * Use a TDB to store an incrementing random seed.
 *
 * Initialised to the current pid, the very first time Samba starts,
 * and incremented by one each time it is needed.  
 * 
 * @note Not called by systems with a working /dev/urandom.
 */
static void get_rand_seed(int *new_seed) 
{
	*new_seed = sys_getpid();
	if (tdb) {
		tdb_change_int32_atomic(tdb, "INFO/random_seed", new_seed, 1);
	}
}

/* open up the secrets database */
BOOL secrets_init(void)
{
	pstring fname;
	unsigned char dummy;

	if (tdb)
		return True;

	pstrcpy(fname, lp_private_dir());
	pstrcat(fname,"/secrets.tdb");

	tdb = tdb_open_log(fname, 0, TDB_DEFAULT, O_RDWR|O_CREAT, 0600);

	if (!tdb) {
		DEBUG(0,("Failed to open %s\n", fname));
		return False;
	}

	/**
	 * Set a reseed function for the crypto random generator 
	 * 
	 * This avoids a problem where systems without /dev/urandom
	 * could send the same challenge to multiple clients
	 */
	set_rand_reseed_callback(get_rand_seed);

	/* Ensure that the reseed is done now, while we are root, etc */
	generate_random_buffer(&dummy, sizeof(dummy));

	return True;
}

/* read a entry from the secrets database - the caller must free the result
   if size is non-null then the size of the entry is put in there
 */
void *secrets_fetch(const char *key, size_t *size)
{
	TDB_DATA dbuf;
	secrets_init();
	if (!tdb)
		return NULL;
	dbuf = tdb_fetch(tdb, string_tdb_data(key));
	if (size)
		*size = dbuf.dsize;
	return dbuf.dptr;
}

/* store a secrets entry 
 */
BOOL secrets_store(const char *key, const void *data, size_t size)
{
	secrets_init();
	if (!tdb)
		return False;
	return tdb_store(tdb, string_tdb_data(key), make_tdb_data(data, size),
			 TDB_REPLACE) == 0;
}


/* delete a secets database entry
 */
BOOL secrets_delete(const char *key)
{
	secrets_init();
	if (!tdb)
		return False;
	return tdb_delete(tdb, string_tdb_data(key)) == 0;
}

BOOL secrets_store_domain_sid(const char *domain, const DOM_SID *sid)
{
	fstring key;
	BOOL ret;

	slprintf(key, sizeof(key)-1, "%s/%s", SECRETS_DOMAIN_SID, domain);
	strupper_m(key);
	ret = secrets_store(key, sid, sizeof(DOM_SID));

	/* Force a re-query, in case we modified our domain */
	if (ret)
		reset_global_sam_sid();
	return ret;
}

BOOL secrets_fetch_domain_sid(const char *domain, DOM_SID *sid)
{
	DOM_SID *dyn_sid;
	fstring key;
	size_t size = 0;

	slprintf(key, sizeof(key)-1, "%s/%s", SECRETS_DOMAIN_SID, domain);
	strupper_m(key);
	dyn_sid = (DOM_SID *)secrets_fetch(key, &size);

	if (dyn_sid == NULL)
		return False;

	if (size != sizeof(DOM_SID)) { 
		SAFE_FREE(dyn_sid);
		return False;
	}

	*sid = *dyn_sid;
	SAFE_FREE(dyn_sid);
	return True;
}

BOOL secrets_store_domain_guid(const char *domain, struct uuid *guid)
{
	fstring key;

	slprintf(key, sizeof(key)-1, "%s/%s", SECRETS_DOMAIN_GUID, domain);
	strupper_m(key);
	return secrets_store(key, guid, sizeof(struct uuid));
}

BOOL secrets_fetch_domain_guid(const char *domain, struct uuid *guid)
{
	struct uuid *dyn_guid;
	fstring key;
	size_t size = 0;
	struct uuid new_guid;

	slprintf(key, sizeof(key)-1, "%s/%s", SECRETS_DOMAIN_GUID, domain);
	strupper_m(key);
	dyn_guid = (struct uuid *)secrets_fetch(key, &size);

	if (!dyn_guid) {
		if (lp_server_role() == ROLE_DOMAIN_PDC) {
			smb_uuid_generate_random(&new_guid);
			if (!secrets_store_domain_guid(domain, &new_guid))
				return False;
			dyn_guid = (struct uuid *)secrets_fetch(key, &size);
		}
		if (dyn_guid == NULL) {
			return False;
		}
	}

	if (size != sizeof(struct uuid)) { 
		DEBUG(1,("UUID size %d is wrong!\n", (int)size));
		SAFE_FREE(dyn_guid);
		return False;
	}

	*guid = *dyn_guid;
	SAFE_FREE(dyn_guid);
	return True;
}

/**
 * Form a key for fetching the machine trust account password
 *
 * @param domain domain name
 *
 * @return stored password's key
 **/
const char *trust_keystr(const char *domain)
{
	static fstring keystr;

	slprintf(keystr,sizeof(keystr)-1,"%s/%s", 
		 SECRETS_MACHINE_ACCT_PASS, domain);
	strupper_m(keystr);

	return keystr;
}

/**
 * Form a key for fetching a trusted domain password
 *
 * @param domain trusted domain name
 *
 * @return stored password's key
 **/
static char *trustdom_keystr(const char *domain)
{
	static pstring keystr;

	pstr_sprintf(keystr, "%s/%s", SECRETS_DOMTRUST_ACCT_PASS, domain);
	strupper_m(keystr);
		
	return keystr;
}

/************************************************************************
 Lock the trust password entry.
************************************************************************/

BOOL secrets_lock_trust_account_password(const char *domain, BOOL dolock)
{
	if (!tdb)
		return False;

	if (dolock)
		return (tdb_lock_bystring(tdb, trust_keystr(domain)) == 0);
	else
		tdb_unlock_bystring(tdb, trust_keystr(domain));
	return True;
}

/************************************************************************
 Routine to get the default secure channel type for trust accounts
************************************************************************/

uint32 get_default_sec_channel(void) 
{
	if (lp_server_role() == ROLE_DOMAIN_BDC || 
	    lp_server_role() == ROLE_DOMAIN_PDC) {
		return SEC_CHAN_BDC;
	} else {
		return SEC_CHAN_WKSTA;
	}
}

/************************************************************************
 Routine to get the trust account password for a domain.
 The user of this function must have locked the trust password file using
 the above secrets_lock_trust_account_password().
************************************************************************/

BOOL secrets_fetch_trust_account_password(const char *domain, uint8 ret_pwd[16],
					  time_t *pass_last_set_time,
					  uint32 *channel)
{
	struct machine_acct_pass *pass;
	char *plaintext;
	size_t size = 0;

	plaintext = secrets_fetch_machine_password(domain, pass_last_set_time, 
						   channel);
	if (plaintext) {
		DEBUG(4,("Using cleartext machine password\n"));
		E_md4hash(plaintext, ret_pwd);
		SAFE_FREE(plaintext);
		return True;
	}

	if (!(pass = secrets_fetch(trust_keystr(domain), &size))) {
		DEBUG(5, ("secrets_fetch failed!\n"));
		return False;
	}
	
	if (size != sizeof(*pass)) {
		DEBUG(0, ("secrets were of incorrect size!\n"));
		return False;
	}

	if (pass_last_set_time) {
		*pass_last_set_time = pass->mod_time;
	}
	memcpy(ret_pwd, pass->hash, 16);

	if (channel) {
		*channel = get_default_sec_channel();
	}

	/* Test if machine password has expired and needs to be changed */
	if (lp_machine_password_timeout()) {
		if (pass->mod_time > 0 && time(NULL) > (pass->mod_time +
				(time_t)lp_machine_password_timeout())) {
			global_machine_password_needs_changing = True;
		}
	}

	SAFE_FREE(pass);
	return True;
}

/************************************************************************
 Routine to get account password to trusted domain
************************************************************************/

BOOL secrets_fetch_trusted_domain_password(const char *domain, char** pwd,
                                           DOM_SID *sid, time_t *pass_last_set_time)
{
	struct trusted_dom_pass pass;
	size_t size = 0;
	
	/* unpacking structures */
	char* pass_buf;
	int pass_len = 0;

	ZERO_STRUCT(pass);

	/* fetching trusted domain password structure */
	if (!(pass_buf = secrets_fetch(trustdom_keystr(domain), &size))) {
		DEBUG(5, ("secrets_fetch failed!\n"));
		return False;
	}

	/* unpack trusted domain password */
	pass_len = tdb_trusted_dom_pass_unpack(pass_buf, size, &pass);
	SAFE_FREE(pass_buf);

	if (pass_len != size) {
		DEBUG(5, ("Invalid secrets size. Unpacked data doesn't match trusted_dom_pass structure.\n"));
		return False;
	}
			
	/* the trust's password */	
	if (pwd) {
		*pwd = SMB_STRDUP(pass.pass);
		if (!*pwd) {
			return False;
		}
	}

	/* last change time */
	if (pass_last_set_time) *pass_last_set_time = pass.mod_time;

	/* domain sid */
	if (sid != NULL) sid_copy(sid, &pass.domain_sid);
		
	return True;
}

/************************************************************************
 Routine to set the trust account password for a domain.
************************************************************************/

BOOL secrets_store_trust_account_password(const char *domain, uint8 new_pwd[16])
{
	struct machine_acct_pass pass;

	pass.mod_time = time(NULL);
	memcpy(pass.hash, new_pwd, 16);

	return secrets_store(trust_keystr(domain), (void *)&pass, sizeof(pass));
}

/**
 * Routine to store the password for trusted domain
 *
 * @param domain remote domain name
 * @param pwd plain text password of trust relationship
 * @param sid remote domain sid
 *
 * @return true if succeeded
 **/

BOOL secrets_store_trusted_domain_password(const char* domain, const char* pwd,
                                           const DOM_SID *sid)
{
	smb_ucs2_t *uni_dom_name;

	/* packing structures */
	pstring pass_buf;
	int pass_len = 0;
	int pass_buf_len = sizeof(pass_buf);
	
	struct trusted_dom_pass pass;
	ZERO_STRUCT(pass);

	if (push_ucs2_allocate(&uni_dom_name, domain) == (size_t)-1) {
		DEBUG(0, ("Could not convert domain name %s to unicode\n",
			  domain));
		return False;
	}
	
	strncpy_w(pass.uni_name, uni_dom_name, sizeof(pass.uni_name) - 1);
	pass.uni_name_len = strlen_w(uni_dom_name)+1;
	SAFE_FREE(uni_dom_name);

	/* last change time */
	pass.mod_time = time(NULL);

	/* password of the trust */
	pass.pass_len = strlen(pwd);
	fstrcpy(pass.pass, pwd);

	/* domain sid */
	sid_copy(&pass.domain_sid, sid);
	
	pass_len = tdb_trusted_dom_pass_pack(pass_buf, pass_buf_len, &pass);

	return secrets_store(trustdom_keystr(domain), (void *)&pass_buf, pass_len);
}

/************************************************************************
 Routine to set the plaintext machine account password for a realm
the password is assumed to be a null terminated ascii string
************************************************************************/

BOOL secrets_store_machine_password(const char *pass, const char *domain, uint32 sec_channel)
{
	char *key = NULL;
	BOOL ret;
	uint32 last_change_time;
	uint32 sec_channel_type;

	asprintf(&key, "%s/%s", SECRETS_MACHINE_PASSWORD, domain);
	if (!key) 
		return False;
	strupper_m(key);

	ret = secrets_store(key, pass, strlen(pass)+1);
	SAFE_FREE(key);

	if (!ret)
		return ret;
	
	asprintf(&key, "%s/%s", SECRETS_MACHINE_LAST_CHANGE_TIME, domain);
	if (!key) 
		return False;
	strupper_m(key);

	SIVAL(&last_change_time, 0, time(NULL));
	ret = secrets_store(key, &last_change_time, sizeof(last_change_time));
	SAFE_FREE(key);

	asprintf(&key, "%s/%s", SECRETS_MACHINE_SEC_CHANNEL_TYPE, domain);
	if (!key) 
		return False;
	strupper_m(key);

	SIVAL(&sec_channel_type, 0, sec_channel);
	ret = secrets_store(key, &sec_channel_type, sizeof(sec_channel_type));
	SAFE_FREE(key);

	return ret;
}

/************************************************************************
 Routine to fetch the plaintext machine account password for a realm
 the password is assumed to be a null terminated ascii string.
************************************************************************/

char *secrets_fetch_machine_password(const char *domain, 
				     time_t *pass_last_set_time,
				     uint32 *channel)
{
	char *key = NULL;
	char *ret;
	asprintf(&key, "%s/%s", SECRETS_MACHINE_PASSWORD, domain);
	strupper_m(key);
	ret = (char *)secrets_fetch(key, NULL);
	SAFE_FREE(key);
	
	if (pass_last_set_time) {
		size_t size;
		uint32 *last_set_time;
		asprintf(&key, "%s/%s", SECRETS_MACHINE_LAST_CHANGE_TIME, domain);
		strupper_m(key);
		last_set_time = secrets_fetch(key, &size);
		if (last_set_time) {
			*pass_last_set_time = IVAL(last_set_time,0);
			SAFE_FREE(last_set_time);
		} else {
			*pass_last_set_time = 0;
		}
		SAFE_FREE(key);
	}
	
	if (channel) {
		size_t size;
		uint32 *channel_type;
		asprintf(&key, "%s/%s", SECRETS_MACHINE_SEC_CHANNEL_TYPE, domain);
		strupper_m(key);
		channel_type = secrets_fetch(key, &size);
		if (channel_type) {
			*channel = IVAL(channel_type,0);
			SAFE_FREE(channel_type);
		} else {
			*channel = get_default_sec_channel();
		}
		SAFE_FREE(key);
	}
	
	return ret;
}

/*******************************************************************
 Wrapper around retrieving the trust account password
*******************************************************************/
                                                                                                                     
BOOL get_trust_pw(const char *domain, uint8 ret_pwd[16], uint32 *channel)
{
	DOM_SID sid;
	char *pwd;
	time_t last_set_time;
                                                                                                                     
	/* if we are a DC and this is not our domain, then lookup an account
		for the domain trust */
                                                                                                                     
	if ( IS_DC && !strequal(domain, lp_workgroup()) && lp_allow_trusted_domains() ) {
		if (!secrets_fetch_trusted_domain_password(domain, &pwd, &sid,
							&last_set_time)) {
			DEBUG(0, ("get_trust_pw: could not fetch trust "
				"account password for trusted domain %s\n",
				domain));
			return False;
		}
                                                                                                                     
		*channel = SEC_CHAN_DOMAIN;
		E_md4hash(pwd, ret_pwd);
		SAFE_FREE(pwd);

		return True;
	}
                                                                                                                     
	/* Just get the account for the requested domain. In the future this
	 * might also cover to be member of more than one domain. */
                                                                                                                     
	if (secrets_fetch_trust_account_password(domain, ret_pwd,
						&last_set_time, channel))
		return True;

	DEBUG(5, ("get_trust_pw: could not fetch trust account "
		"password for domain %s\n", domain));
	return False;
}

/************************************************************************
 Routine to delete the machine trust account password file for a domain.
************************************************************************/

BOOL trust_password_delete(const char *domain)
{
	return secrets_delete(trust_keystr(domain));
}

/************************************************************************
 Routine to delete the password for trusted domain
************************************************************************/

BOOL trusted_domain_password_delete(const char *domain)
{
	return secrets_delete(trustdom_keystr(domain));
}

BOOL secrets_store_ldap_pw(const char* dn, char* pw)
{
	char *key = NULL;
	BOOL ret;
	
	if (asprintf(&key, "%s/%s", SECRETS_LDAP_BIND_PW, dn) < 0) {
		DEBUG(0, ("secrets_store_ldap_pw: asprintf failed!\n"));
		return False;
	}
		
	ret = secrets_store(key, pw, strlen(pw)+1);
	
	SAFE_FREE(key);
	return ret;
}

/*******************************************************************
 Find the ldap password.
******************************************************************/

BOOL fetch_ldap_pw(char **dn, char** pw)
{
	char *key = NULL;
	size_t size = 0;
	
	*dn = smb_xstrdup(lp_ldap_admin_dn());
	
	if (asprintf(&key, "%s/%s", SECRETS_LDAP_BIND_PW, *dn) < 0) {
		SAFE_FREE(*dn);
		DEBUG(0, ("fetch_ldap_pw: asprintf failed!\n"));
	}
	
	*pw=secrets_fetch(key, &size);
	SAFE_FREE(key);

	if (!size) {
		/* Upgrade 2.2 style entry */
		char *p;
	        char* old_style_key = SMB_STRDUP(*dn);
		char *data;
		fstring old_style_pw;
		
		if (!old_style_key) {
			DEBUG(0, ("fetch_ldap_pw: strdup failed!\n"));
			return False;
		}

		for (p=old_style_key; *p; p++)
			if (*p == ',') *p = '/';
	
		data=secrets_fetch(old_style_key, &size);
		if (!size && size < sizeof(old_style_pw)) {
			DEBUG(0,("fetch_ldap_pw: neither ldap secret retrieved!\n"));
			SAFE_FREE(old_style_key);
			SAFE_FREE(*dn);
			return False;
		}

		size = MIN(size, sizeof(fstring)-1);
		strncpy(old_style_pw, data, size);
		old_style_pw[size] = 0;

		SAFE_FREE(data);

		if (!secrets_store_ldap_pw(*dn, old_style_pw)) {
			DEBUG(0,("fetch_ldap_pw: ldap secret could not be upgraded!\n"));
			SAFE_FREE(old_style_key);
			SAFE_FREE(*dn);
			return False;			
		}
		if (!secrets_delete(old_style_key)) {
			DEBUG(0,("fetch_ldap_pw: old ldap secret could not be deleted!\n"));
		}

		SAFE_FREE(old_style_key);

		*pw = smb_xstrdup(old_style_pw);		
	}
	
	return True;
}

/**
 * Get trusted domains info from secrets.tdb.
 **/ 

NTSTATUS secrets_trusted_domains(TALLOC_CTX *mem_ctx, uint32 *num_domains,
				 struct trustdom_info ***domains)
{
	TDB_LIST_NODE *keys, *k;
	char *pattern;

	if (!secrets_init()) return NT_STATUS_ACCESS_DENIED;
	
	/* generate searching pattern */
	pattern = talloc_asprintf(mem_ctx, "%s/*", SECRETS_DOMTRUST_ACCT_PASS);
	if (pattern == NULL) {
		DEBUG(0, ("secrets_trusted_domains: talloc_asprintf() "
			  "failed!\n"));
		return NT_STATUS_NO_MEMORY;
	}

	*domains = NULL;
	*num_domains = 0;

	/* fetching trusted domains' data and collecting them in a list */
	keys = tdb_search_keys(tdb, pattern);

	/* searching for keys in secrets db -- way to go ... */
	for (k = keys; k; k = k->next) {
		char *packed_pass;
		size_t size = 0, packed_size = 0;
		struct trusted_dom_pass pass;
		char *secrets_key;
		struct trustdom_info *dom_info;
		
		/* important: ensure null-termination of the key string */
		secrets_key = talloc_strndup(mem_ctx,
					     k->node_key.dptr,
					     k->node_key.dsize);
		if (!secrets_key) {
			DEBUG(0, ("strndup failed!\n"));
			tdb_search_list_free(keys);
			return NT_STATUS_NO_MEMORY;
		}

		packed_pass = secrets_fetch(secrets_key, &size);
		packed_size = tdb_trusted_dom_pass_unpack(packed_pass, size,
							  &pass);
		/* packed representation isn't needed anymore */
		SAFE_FREE(packed_pass);
		
		if (size != packed_size) {
			DEBUG(2, ("Secrets record %s is invalid!\n",
				  secrets_key));
			continue;
		}

		if (pass.domain_sid.num_auths != 4) {
			DEBUG(0, ("SID %s is not a domain sid, has %d "
				  "auths instead of 4\n",
				  sid_string_static(&pass.domain_sid),
				  pass.domain_sid.num_auths));
			continue;
		}

		dom_info = TALLOC_P(mem_ctx, struct trustdom_info);
		if (dom_info == NULL) {
			DEBUG(0, ("talloc failed\n"));
			tdb_search_list_free(keys);
			return NT_STATUS_NO_MEMORY;
		}

		if (pull_ucs2_talloc(mem_ctx, &dom_info->name,
				     pass.uni_name) == (size_t)-1) {
			DEBUG(2, ("pull_ucs2_talloc failed\n"));
			tdb_search_list_free(keys);
			return NT_STATUS_NO_MEMORY;
		}

		sid_copy(&dom_info->sid, &pass.domain_sid);

		ADD_TO_ARRAY(mem_ctx, struct trustdom_info *, dom_info,
			     domains, num_domains);

		if (*domains == NULL) {
			tdb_search_list_free(keys);
			return NT_STATUS_NO_MEMORY;
		}
		talloc_steal(*domains, dom_info);
	}
	
	DEBUG(5, ("secrets_get_trusted_domains: got %d domains\n",
		  *num_domains));

	/* free the results of searching the keys */
	tdb_search_list_free(keys);

	return NT_STATUS_OK;
}

/*******************************************************************************
 Lock the secrets tdb based on a string - this is used as a primitive form of mutex
 between smbd instances.
*******************************************************************************/

BOOL secrets_named_mutex(const char *name, unsigned int timeout)
{
	int ret = 0;

	if (!secrets_init())
		return False;

	ret = tdb_lock_bystring_with_timeout(tdb, name, timeout);
	if (ret == 0)
		DEBUG(10,("secrets_named_mutex: got mutex for %s\n", name ));

	return (ret == 0);
}

/*******************************************************************************
 Unlock a named mutex.
*******************************************************************************/

void secrets_named_mutex_release(const char *name)
{
	tdb_unlock_bystring(tdb, name);
	DEBUG(10,("secrets_named_mutex: released mutex for %s\n", name ));
}

/*******************************************************************************
 Store a complete AFS keyfile into secrets.tdb.
*******************************************************************************/

BOOL secrets_store_afs_keyfile(const char *cell, const struct afs_keyfile *keyfile)
{
	fstring key;

	if ((cell == NULL) || (keyfile == NULL))
		return False;

	if (ntohl(keyfile->nkeys) > SECRETS_AFS_MAXKEYS)
		return False;

	slprintf(key, sizeof(key)-1, "%s/%s", SECRETS_AFS_KEYFILE, cell);
	return secrets_store(key, keyfile, sizeof(struct afs_keyfile));
}

/*******************************************************************************
 Fetch the current (highest) AFS key from secrets.tdb
*******************************************************************************/
BOOL secrets_fetch_afs_key(const char *cell, struct afs_key *result)
{
	fstring key;
	struct afs_keyfile *keyfile;
	size_t size = 0;
	uint32 i;

	slprintf(key, sizeof(key)-1, "%s/%s", SECRETS_AFS_KEYFILE, cell);

	keyfile = (struct afs_keyfile *)secrets_fetch(key, &size);

	if (keyfile == NULL)
		return False;

	if (size != sizeof(struct afs_keyfile)) {
		SAFE_FREE(keyfile);
		return False;
	}

	i = ntohl(keyfile->nkeys);

	if (i > SECRETS_AFS_MAXKEYS) {
		SAFE_FREE(keyfile);
		return False;
	}

	*result = keyfile->entry[i-1];

	result->kvno = ntohl(result->kvno);

	return True;
}

/******************************************************************************
  When kerberos is not available, choose between anonymous or
  authenticated connections.  

  We need to use an authenticated connection if DCs have the
  RestrictAnonymous registry entry set > 0, or the "Additional
  restrictions for anonymous connections" set in the win2k Local
  Security Policy.

  Caller to free() result in domain, username, password
*******************************************************************************/
void secrets_fetch_ipc_userpass(char **username, char **domain, char **password)
{
	*username = secrets_fetch(SECRETS_AUTH_USER, NULL);
	*domain = secrets_fetch(SECRETS_AUTH_DOMAIN, NULL);
	*password = secrets_fetch(SECRETS_AUTH_PASSWORD, NULL);
	
	if (*username && **username) {

		if (!*domain || !**domain)
			*domain = smb_xstrdup(lp_workgroup());
		
		if (!*password || !**password)
			*password = smb_xstrdup("");

		DEBUG(3, ("IPC$ connections done by user %s\\%s\n", 
			  *domain, *username));

	} else {
		DEBUG(3, ("IPC$ connections done anonymously\n"));
		*username = smb_xstrdup("");
		*domain = smb_xstrdup("");
		*password = smb_xstrdup("");
	}
}

/******************************************************************************
 Open or create the schannel session store tdb.
*******************************************************************************/

static TDB_CONTEXT *open_schannel_session_store(TALLOC_CTX *mem_ctx)
{
	TDB_DATA vers;
	uint32 ver;
	TDB_CONTEXT *tdb_sc = NULL;
	char *fname = talloc_asprintf(mem_ctx, "%s/schannel_store.tdb", lp_private_dir());

	if (!fname) {
		return NULL;
	}

        tdb_sc = tdb_open_log(fname, 0, TDB_DEFAULT, O_RDWR|O_CREAT, 0600);

        if (!tdb_sc) {
                DEBUG(0,("open_schannel_session_store: Failed to open %s\n", fname));
		TALLOC_FREE(fname);
                return NULL;
        }

	vers = tdb_fetch_bystring(tdb_sc, "SCHANNEL_STORE_VERSION");
	if (vers.dptr == NULL) {
		/* First opener, no version. */
		SIVAL(&ver,0,1);
		vers.dptr = (char *)&ver;
		vers.dsize = 4;
		tdb_store_bystring(tdb_sc, "SCHANNEL_STORE_VERSION", vers, TDB_REPLACE);
		vers.dptr = NULL;
	} else if (vers.dsize == 4) {
		ver = IVAL(vers.dptr,0);
		if (ver != 1) {
			tdb_close(tdb_sc);
			tdb_sc = NULL;
			DEBUG(0,("open_schannel_session_store: wrong version number %d in %s\n",
				(int)ver, fname ));
		}
	} else {
		tdb_close(tdb_sc);
		tdb_sc = NULL;
		DEBUG(0,("open_schannel_session_store: wrong version number size %d in %s\n",
			(int)vers.dsize, fname ));
	}

	SAFE_FREE(vers.dptr);
	TALLOC_FREE(fname);

	return tdb_sc;
}

/******************************************************************************
 Store the schannel state after an AUTH2 call.
 Note we must be root here.
*******************************************************************************/

BOOL secrets_store_schannel_session_info(TALLOC_CTX *mem_ctx,
				const char *remote_machine,
				const struct dcinfo *pdc)
{
	TDB_CONTEXT *tdb_sc = NULL;
	TDB_DATA value;
	BOOL ret;
	char *keystr = talloc_asprintf(mem_ctx, "%s/%s", SECRETS_SCHANNEL_STATE,
				remote_machine);
	if (!keystr) {
		return False;
	}

	strupper_m(keystr);

	/* Work out how large the record is. */
	value.dsize = tdb_pack(NULL, 0, "dBBBBBfff",
				pdc->sequence,
				8, pdc->seed_chal.data,
				8, pdc->clnt_chal.data,
				8, pdc->srv_chal.data,
				16, pdc->sess_key,
				16, pdc->mach_pw,
				pdc->mach_acct,
				pdc->remote_machine,
				pdc->domain);

	value.dptr = TALLOC(mem_ctx, value.dsize);
	if (!value.dptr) {
		TALLOC_FREE(keystr);
		return False;
	}

	value.dsize = tdb_pack(value.dptr, value.dsize, "dBBBBBfff",
				pdc->sequence,
				8, pdc->seed_chal.data,
				8, pdc->clnt_chal.data,
				8, pdc->srv_chal.data,
				16, pdc->sess_key,
				16, pdc->mach_pw,
				pdc->mach_acct,
				pdc->remote_machine,
				pdc->domain);

	tdb_sc = open_schannel_session_store(mem_ctx);
	if (!tdb_sc) {
		TALLOC_FREE(keystr);
		TALLOC_FREE(value.dptr);
		return False;
	}

	ret = (tdb_store_bystring(tdb_sc, keystr, value, TDB_REPLACE) == 0 ? True : False);

	DEBUG(3,("secrets_store_schannel_session_info: stored schannel info with key %s\n",
		keystr ));

	tdb_close(tdb_sc);
	TALLOC_FREE(keystr);
	TALLOC_FREE(value.dptr);
	return ret;
}

/******************************************************************************
 Restore the schannel state on a client reconnect.
 Note we must be root here.
*******************************************************************************/

BOOL secrets_restore_schannel_session_info(TALLOC_CTX *mem_ctx,
				const char *remote_machine,
				struct dcinfo **ppdc)
{
	TDB_CONTEXT *tdb_sc = NULL;
	TDB_DATA value;
	unsigned char *pseed_chal = NULL;
	unsigned char *pclnt_chal = NULL;
	unsigned char *psrv_chal = NULL;
	unsigned char *psess_key = NULL;
	unsigned char *pmach_pw = NULL;
	uint32 l1, l2, l3, l4, l5;
	int ret;
	struct dcinfo *pdc = NULL;
	char *keystr = talloc_asprintf(mem_ctx, "%s/%s", SECRETS_SCHANNEL_STATE,
				remote_machine);

	*ppdc = NULL;

	if (!keystr) {
		return False;
	}

	strupper_m(keystr);

	tdb_sc = open_schannel_session_store(mem_ctx);
	if (!tdb_sc) {
		TALLOC_FREE(keystr);
		return False;
	}

	value = tdb_fetch_bystring(tdb_sc, keystr);
	if (!value.dptr) {
		DEBUG(0,("secrets_restore_schannel_session_info: Failed to find entry with key %s\n",
			keystr ));
		tdb_close(tdb_sc);
		return False;
	}

	pdc = TALLOC_ZERO_P(mem_ctx, struct dcinfo);

	/* Retrieve the record. */
	ret = tdb_unpack(value.dptr, value.dsize, "dBBBBBfff",
				&pdc->sequence,
				&l1, &pseed_chal,
				&l2, &pclnt_chal,
				&l3, &psrv_chal,
				&l4, &psess_key,
				&l5, &pmach_pw,
				&pdc->mach_acct,
				&pdc->remote_machine,
				&pdc->domain);

	if (ret == -1 || l1 != 8 || l2 != 8 || l3 != 8 || l4 != 16 || l5 != 16) {
		/* Bad record - delete it. */
		tdb_delete_bystring(tdb_sc, keystr);
		tdb_close(tdb_sc);
		TALLOC_FREE(keystr);
		TALLOC_FREE(pdc);
		SAFE_FREE(pseed_chal);
		SAFE_FREE(pclnt_chal);
		SAFE_FREE(psrv_chal);
		SAFE_FREE(psess_key);
		SAFE_FREE(pmach_pw);
		SAFE_FREE(value.dptr);
		return False;
	}

	tdb_close(tdb_sc);

	memcpy(pdc->seed_chal.data, pseed_chal, 8);
	memcpy(pdc->clnt_chal.data, pclnt_chal, 8);
	memcpy(pdc->srv_chal.data, psrv_chal, 8);
	memcpy(pdc->sess_key, psess_key, 16);
	memcpy(pdc->mach_pw, pmach_pw, 16);

	/* We know these are true so didn't bother to store them. */
	pdc->challenge_sent = True;
	pdc->authenticated = True;

	DEBUG(3,("secrets_restore_schannel_session_info: restored schannel info key %s\n",
		keystr ));

	SAFE_FREE(pseed_chal);
	SAFE_FREE(pclnt_chal);
	SAFE_FREE(psrv_chal);
	SAFE_FREE(psess_key);
	SAFE_FREE(pmach_pw);

	TALLOC_FREE(keystr);
	SAFE_FREE(value.dptr);

	*ppdc = pdc;

	return True;
}
