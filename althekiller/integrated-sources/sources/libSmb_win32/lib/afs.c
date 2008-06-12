/* 
 *  Unix SMB/CIFS implementation.
 *  Generate AFS tickets
 *  Copyright (C) Volker Lendecke 2003
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

#ifdef WITH_FAKE_KASERVER

#define NO_ASN1_TYPEDEFS 1

#include <afs/stds.h>
#include <afs/afs.h>
#include <afs/auth.h>
#include <afs/venus.h>
#include <asm/unistd.h>
#include <openssl/des.h>

struct ClearToken {
	uint32 AuthHandle;
	char HandShakeKey[8];
	uint32 ViceId;
	uint32 BeginTimestamp;
	uint32 EndTimestamp;
};

static char *afs_encode_token(const char *cell, const DATA_BLOB ticket,
			      const struct ClearToken *ct)
{
	char *base64_ticket;
	char *result;

	DATA_BLOB key = data_blob(ct->HandShakeKey, 8);
	char *base64_key;

	base64_ticket = base64_encode_data_blob(ticket);
	if (base64_ticket == NULL)
		return NULL;

	base64_key = base64_encode_data_blob(key);
	if (base64_key == NULL) {
		free(base64_ticket);
		return NULL;
	}

	asprintf(&result, "%s\n%u\n%s\n%u\n%u\n%u\n%s\n", cell,
		 ct->AuthHandle, base64_key, ct->ViceId, ct->BeginTimestamp,
		 ct->EndTimestamp, base64_ticket);

	DEBUG(10, ("Got ticket string:\n%s\n", result));

	free(base64_ticket);
	free(base64_key);

	return result;
}

/* Create a ClearToken and an encrypted ticket. ClearToken has not yet the
 * ViceId set, this should be set by the caller. */

static BOOL afs_createtoken(const char *username, const char *cell,
			    DATA_BLOB *ticket, struct ClearToken *ct)
{
	fstring clear_ticket;
	char *p = clear_ticket;
	uint32 len;
	uint32 now;

	struct afs_key key;
	des_key_schedule key_schedule;

	if (!secrets_init()) 
		return False;

	if (!secrets_fetch_afs_key(cell, &key)) {
		DEBUG(1, ("Could not fetch AFS service key\n"));
		return False;
	}

	ct->AuthHandle = key.kvno;

	/* Build the ticket. This is going to be encrypted, so in our
           way we fill in ct while we still have the unencrypted
           form. */

	p = clear_ticket;

	/* The byte-order */
	*p = 1;
	p += 1;

	/* "Alice", the client username */
	strncpy(p, username, sizeof(clear_ticket)-PTR_DIFF(p,clear_ticket)-1);
	p += strlen(p)+1;
	strncpy(p, "", sizeof(clear_ticket)-PTR_DIFF(p,clear_ticket)-1);
	p += strlen(p)+1;
	strncpy(p, cell, sizeof(clear_ticket)-PTR_DIFF(p,clear_ticket)-1);
	p += strlen(p)+1;

	/* Alice's network layer address. At least Openafs-1.2.10
           ignores this, so we fill in a dummy value here. */
	SIVAL(p, 0, 0);
	p += 4;

	/* We need to create a session key */
	generate_random_buffer(p, 8);

	/* Our client code needs the the key in the clear, it does not
           know the server-key ... */
	memcpy(ct->HandShakeKey, p, 8);

	p += 8;

	/* This is a kerberos 4 life time. The life time is expressed
	 * in units of 5 minute intervals up to 38400 seconds, after
	 * that a table is used up to lifetime 0xBF. Values between
	 * 0xC0 and 0xFF is undefined. 0xFF is defined to be the
	 * infinite time that never expire.
	 *
	 * So here we cheat and use the infinite time */
	*p = 255;
	p += 1;

	/* Ticket creation time */
	now = time(NULL);
	SIVAL(p, 0, now);
	ct->BeginTimestamp = now;

	if(lp_afs_token_lifetime() == 0)
		ct->EndTimestamp = NEVERDATE;
	else
		ct->EndTimestamp = now + lp_afs_token_lifetime();

	if (((ct->EndTimestamp - ct->BeginTimestamp) & 1) == 1) {
		ct->BeginTimestamp += 1; /* Lifetime must be even */
	}
	p += 4;

	/* And here comes Bob's name and instance, in this case the
           AFS server. */
	strncpy(p, "afs", sizeof(clear_ticket)-PTR_DIFF(p,clear_ticket)-1);
	p += strlen(p)+1;
	strncpy(p, "", sizeof(clear_ticket)-PTR_DIFF(p,clear_ticket)-1);
	p += strlen(p)+1;

	/* And zero-pad to a multiple of 8 bytes */
	len = PTR_DIFF(p, clear_ticket);
	if (len & 7) {
		uint32 extra_space = 8-(len & 7);
		memset(p, 0, extra_space);
		p+=extra_space;
	}
	len = PTR_DIFF(p, clear_ticket);

	des_key_sched((const_des_cblock *)key.key, key_schedule);
	des_pcbc_encrypt(clear_ticket, clear_ticket,
			 len, key_schedule, (C_Block *)key.key, 1);

	ZERO_STRUCT(key);

	*ticket = data_blob(clear_ticket, len);

	return True;
}

char *afs_createtoken_str(const char *username, const char *cell)
{
	DATA_BLOB ticket;
	struct ClearToken ct;
	char *result;

	if (!afs_createtoken(username, cell, &ticket, &ct))
		return NULL;

	result = afs_encode_token(cell, ticket, &ct);

	data_blob_free(&ticket);

	return result;
}

/*
  This routine takes a radical approach completely bypassing the
  Kerberos idea of security and using AFS simply as an intelligent
  file backend. Samba has persuaded itself somehow that the user is
  actually correctly identified and then we create a ticket that the
  AFS server hopefully accepts using its KeyFile that the admin has
  kindly stored to our secrets.tdb.

  Thanks to the book "Network Security -- PRIVATE Communication in a
  PUBLIC World" by Charlie Kaufman, Radia Perlman and Mike Speciner
  Kerberos 4 tickets are not really hard to construct.

  For the comments "Alice" is the User to be auth'ed, and "Bob" is the
  AFS server.  */

BOOL afs_login(connection_struct *conn)
{
	extern struct current_user current_user;
	DATA_BLOB ticket;
	pstring afs_username;
	char *cell;
	BOOL result;
	char *ticket_str;
	const DOM_SID *user_sid;

	struct ClearToken ct;

	pstrcpy(afs_username, lp_afs_username_map());
	standard_sub_conn(conn, afs_username, sizeof(afs_username));

	user_sid = &current_user.nt_user_token->user_sids[0];
	pstring_sub(afs_username, "%s", sid_string_static(user_sid));

	/* The pts command always generates completely lower-case user
	 * names. */
	strlower_m(afs_username);

	cell = strchr(afs_username, '@');

	if (cell == NULL) {
		DEBUG(1, ("AFS username doesn't contain a @, "
			  "could not find cell\n"));
		return False;
	}

	*cell = '\0';
	cell += 1;

	DEBUG(10, ("Trying to log into AFS for user %s@%s\n", 
		   afs_username, cell));

	if (!afs_createtoken(afs_username, cell, &ticket, &ct))
		return False;

	/* For which Unix-UID do we want to set the token? */
	ct.ViceId = getuid();

	ticket_str = afs_encode_token(cell, ticket, &ct);

	result = afs_settoken_str(ticket_str);

	SAFE_FREE(ticket_str);

	data_blob_free(&ticket);

	return result;
}

#else

BOOL afs_login(connection_struct *conn)
{
	return True;
}

char *afs_createtoken_str(const char *username, const char *cell)
{
	return False;
}

#endif /* WITH_FAKE_KASERVER */
