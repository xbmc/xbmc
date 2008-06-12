/*
 * Unix SMB/CIFS implementation. 
 * secrets.tdb file format info
 * Copyright (C) Andrew Tridgell              2000
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
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

#ifndef _SECRETS_H
#define _SECRETS_H

/* the first one is for the hashed password (NT4 style) the latter
   for plaintext (ADS)
*/
#define SECRETS_MACHINE_ACCT_PASS "SECRETS/$MACHINE.ACC"
#define SECRETS_MACHINE_PASSWORD "SECRETS/MACHINE_PASSWORD"
#define SECRETS_MACHINE_LAST_CHANGE_TIME "SECRETS/MACHINE_LAST_CHANGE_TIME"
#define SECRETS_MACHINE_SEC_CHANNEL_TYPE "SECRETS/MACHINE_SEC_CHANNEL_TYPE"
#define SECRETS_MACHINE_TRUST_ACCOUNT_NAME "SECRETS/SECRETS_MACHINE_TRUST_ACCOUNT_NAME"
/* this one is for storing trusted domain account password */
#define SECRETS_DOMTRUST_ACCT_PASS "SECRETS/$DOMTRUST.ACC"

/* Store the principal name used for Kerberos DES key salt under this key name. */
#define SECRETS_SALTING_PRINCIPAL "SECRETS/SALTING_PRINCIPAL"

/* The domain sid and our sid are stored here even though they aren't
   really secret. */
#define SECRETS_DOMAIN_SID    "SECRETS/SID"
#define SECRETS_SAM_SID       "SAM/SID"

/* The domain GUID and server GUID (NOT the same) are also not secret */
#define SECRETS_DOMAIN_GUID   "SECRETS/DOMGUID"
#define SECRETS_SERVER_GUID   "SECRETS/GUID"

#define SECRETS_LDAP_BIND_PW "SECRETS/LDAP_BIND_PW"

/* Authenticated user info is stored in secrets.tdb under these keys */

#define SECRETS_AUTH_USER      "SECRETS/AUTH_USER"
#define SECRETS_AUTH_DOMAIN      "SECRETS/AUTH_DOMAIN"
#define SECRETS_AUTH_PASSWORD  "SECRETS/AUTH_PASSWORD"

/* structure for storing machine account password
   (ie. when samba server is member of a domain */
struct machine_acct_pass {
	uint8 hash[16];
	time_t mod_time;
};

/*
 * storage structure for trusted domain
 */
typedef struct trusted_dom_pass {
	size_t uni_name_len;
	smb_ucs2_t uni_name[32]; /* unicode domain name */
	size_t pass_len;
	fstring pass;		/* trust relationship's password */
	time_t mod_time;
	DOM_SID domain_sid;	/* remote domain's sid */
} TRUSTED_DOM_PASS;

/*
 * trusted domain entry/entries returned by secrets_get_trusted_domains
 * (used in _lsa_enum_trust_dom call)
 */
struct trustdom_info {
	char *name;
	DOM_SID sid;
};

/*
 * Format of an OpenAFS keyfile
 */

#define SECRETS_AFS_MAXKEYS 8

struct afs_key {
	uint32 kvno;
	char key[8];
};

struct afs_keyfile {
	uint32 nkeys;
	struct afs_key entry[SECRETS_AFS_MAXKEYS];
};

#define SECRETS_AFS_KEYFILE "SECRETS/AFS_KEYFILE"

#define SECRETS_SCHANNEL_STATE "SECRETS/SCHANNEL"

#endif /* _SECRETS_H */
