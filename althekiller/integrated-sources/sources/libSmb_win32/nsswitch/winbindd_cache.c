/* 
   Unix SMB/CIFS implementation.

   Winbind cache backend functions

   Copyright (C) Andrew Tridgell 2001
   Copyright (C) Gerald Carter   2003
   Copyright (C) Volker Lendecke 2005
   Copyright (C) Guenther Deschner 2005
   
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
#include "winbindd.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_WINBIND

/* Global online/offline state - False when online. winbindd starts up online
   and sets this to true if the first query fails and there's an entry in
   the cache tdb telling us to stay offline. */

static BOOL global_winbindd_offline_state;

struct winbind_cache {
	TDB_CONTEXT *tdb;
};

struct cache_entry {
	NTSTATUS status;
	uint32 sequence_number;
	uint8 *data;
	uint32 len, ofs;
};

#define WINBINDD_MAX_CACHE_SIZE (50*1024*1024)

static struct winbind_cache *wcache;

void winbindd_check_cache_size(time_t t)
{
	static time_t last_check_time;
	struct stat st;

	if (last_check_time == (time_t)0)
		last_check_time = t;

	if (t - last_check_time < 60 && t - last_check_time > 0)
		return;

	if (wcache == NULL || wcache->tdb == NULL) {
		DEBUG(0, ("Unable to check size of tdb cache - cache not open !\n"));
		return;
	}

	if (fstat(wcache->tdb->fd, &st) == -1) {
		DEBUG(0, ("Unable to check size of tdb cache %s!\n", strerror(errno) ));
		return;
	}

	if (st.st_size > WINBINDD_MAX_CACHE_SIZE) {
		DEBUG(10,("flushing cache due to size (%lu) > (%lu)\n",
			(unsigned long)st.st_size,
			(unsigned long)WINBINDD_MAX_CACHE_SIZE));
		wcache_flush_cache();
	}
}

/* get the winbind_cache structure */
static struct winbind_cache *get_cache(struct winbindd_domain *domain)
{
	struct winbind_cache *ret = wcache;
#ifdef HAVE_ADS
	struct winbindd_domain *our_domain = domain;
#endif

	/* we have to know what type of domain we are dealing with first */

	if ( !domain->initialized )
		set_dc_type_and_flags( domain );

	/* 
	   OK.  listen up becasue I'm only going to say this once.
	   We have the following scenarios to consider
	   (a) trusted AD domains on a Samba DC,
	   (b) trusted AD domains and we are joined to a non-kerberos domain
	   (c) trusted AD domains and we are joined to a kerberos (AD) domain

	   For (a) we can always contact the trusted domain using krb5 
	   since we have the domain trust account password

	   For (b) we can only use RPC since we have no way of 
	   getting a krb5 ticket in our own domain

	   For (c) we can always use krb5 since we have a kerberos trust

	   --jerry
	 */

	if (!domain->backend) {
		extern struct winbindd_methods reconnect_methods;
#ifdef HAVE_ADS
		extern struct winbindd_methods ads_methods;

		/* find our domain first so we can figure out if we 
		   are joined to a kerberized domain */

		if ( !domain->primary )
			our_domain = find_our_domain();

		if ( (our_domain->active_directory || IS_DC) && domain->active_directory ) {
			DEBUG(5,("get_cache: Setting ADS methods for domain %s\n", domain->name));
			domain->backend = &ads_methods;
		} else {
#endif	/* HAVE_ADS */
			DEBUG(5,("get_cache: Setting MS-RPC methods for domain %s\n", domain->name));
			domain->backend = &reconnect_methods;
#ifdef HAVE_ADS
		}
#endif	/* HAVE_ADS */
	}

	if (ret)
		return ret;
	
	ret = SMB_XMALLOC_P(struct winbind_cache);
	ZERO_STRUCTP(ret);

	wcache = ret;
	wcache_flush_cache();

	return ret;
}

/*
  free a centry structure
*/
static void centry_free(struct cache_entry *centry)
{
	if (!centry)
		return;
	SAFE_FREE(centry->data);
	free(centry);
}

/*
  pull a uint32 from a cache entry 
*/
static uint32 centry_uint32(struct cache_entry *centry)
{
	uint32 ret;
	if (centry->len - centry->ofs < 4) {
		DEBUG(0,("centry corruption? needed 4 bytes, have %d\n", 
			 centry->len - centry->ofs));
		smb_panic("centry_uint32");
	}
	ret = IVAL(centry->data, centry->ofs);
	centry->ofs += 4;
	return ret;
}

/*
  pull a uint16 from a cache entry 
*/
static uint16 centry_uint16(struct cache_entry *centry)
{
	uint16 ret;
	if (centry->len - centry->ofs < 2) {
		DEBUG(0,("centry corruption? needed 2 bytes, have %d\n", 
			 centry->len - centry->ofs));
		smb_panic("centry_uint16");
	}
	ret = CVAL(centry->data, centry->ofs);
	centry->ofs += 2;
	return ret;
}

/*
  pull a uint8 from a cache entry 
*/
static uint8 centry_uint8(struct cache_entry *centry)
{
	uint8 ret;
	if (centry->len - centry->ofs < 1) {
		DEBUG(0,("centry corruption? needed 1 bytes, have %d\n", 
			 centry->len - centry->ofs));
		smb_panic("centry_uint32");
	}
	ret = CVAL(centry->data, centry->ofs);
	centry->ofs += 1;
	return ret;
}

/*
  pull a NTTIME from a cache entry 
*/
static NTTIME centry_nttime(struct cache_entry *centry)
{
	NTTIME ret;
	if (centry->len - centry->ofs < 8) {
		DEBUG(0,("centry corruption? needed 8 bytes, have %d\n", 
			 centry->len - centry->ofs));
		smb_panic("centry_nttime");
	}
	ret.low = IVAL(centry->data, centry->ofs);
	centry->ofs += 4;
	ret.high = IVAL(centry->data, centry->ofs);
	centry->ofs += 4;
	return ret;
}

/*
  pull a time_t from a cache entry 
*/
static time_t centry_time(struct cache_entry *centry)
{
	time_t ret;
	if (centry->len - centry->ofs < sizeof(time_t)) {
		DEBUG(0,("centry corruption? needed %u bytes, have %u\n", 
			 (unsigned int)sizeof(time_t), (unsigned int)(centry->len - centry->ofs)));
		smb_panic("centry_time");
	}
	ret = IVAL(centry->data, centry->ofs); /* FIXME: correct ? */
	centry->ofs += sizeof(time_t);
	return ret;
}

/* pull a string from a cache entry, using the supplied
   talloc context 
*/
static char *centry_string(struct cache_entry *centry, TALLOC_CTX *mem_ctx)
{
	uint32 len;
	char *ret;

	len = centry_uint8(centry);

	if (len == 0xFF) {
		/* a deliberate NULL string */
		return NULL;
	}

	if (centry->len - centry->ofs < len) {
		DEBUG(0,("centry corruption? needed %d bytes, have %d\n", 
			 len, centry->len - centry->ofs));
		smb_panic("centry_string");
	}

	ret = TALLOC(mem_ctx, len+1);
	if (!ret) {
		smb_panic("centry_string out of memory\n");
	}
	memcpy(ret,centry->data + centry->ofs, len);
	ret[len] = 0;
	centry->ofs += len;
	return ret;
}

/* pull a hash16 from a cache entry, using the supplied
   talloc context 
*/
static char *centry_hash16(struct cache_entry *centry, TALLOC_CTX *mem_ctx)
{
	uint32 len;
	char *ret;

	len = centry_uint8(centry);

	if (len != 16) {
		DEBUG(0,("centry corruption? hash len (%u) != 16\n", 
			len ));
		smb_panic("centry_hash16");
	}

	if (centry->len - centry->ofs < 16) {
		DEBUG(0,("centry corruption? needed 16 bytes, have %d\n", 
			 centry->len - centry->ofs));
		smb_panic("centry_hash16");
	}

	ret = TALLOC_ARRAY(mem_ctx, char, 16);
	if (!ret) {
		smb_panic("centry_hash out of memory\n");
	}
	memcpy(ret,centry->data + centry->ofs, 16);
	centry->ofs += 16;
	return ret;
}

/* pull a sid from a cache entry, using the supplied
   talloc context 
*/
static BOOL centry_sid(struct cache_entry *centry, TALLOC_CTX *mem_ctx, DOM_SID *sid)
{
	char *sid_string;
	sid_string = centry_string(centry, mem_ctx);
	if ((sid_string == NULL) || (!string_to_sid(sid, sid_string))) {
		return False;
	}
	return True;
}

/* the server is considered down if it can't give us a sequence number */
static BOOL wcache_server_down(struct winbindd_domain *domain)
{
	BOOL ret;

	if (!wcache->tdb)
		return False;

	ret = (domain->sequence_number == DOM_SEQUENCE_NONE);

	if (ret)
		DEBUG(10,("wcache_server_down: server for Domain %s down\n", 
			domain->name ));
	return ret;
}

static NTSTATUS fetch_cache_seqnum( struct winbindd_domain *domain, time_t now )
{
	TDB_DATA data;
	fstring key;
	uint32 time_diff;
	
	if (!wcache->tdb) {
		DEBUG(10,("fetch_cache_seqnum: tdb == NULL\n"));
		return NT_STATUS_UNSUCCESSFUL;
	}
		
	fstr_sprintf( key, "SEQNUM/%s", domain->name );
	
	data = tdb_fetch_bystring( wcache->tdb, key );
	if ( !data.dptr || data.dsize!=8 ) {
		DEBUG(10,("fetch_cache_seqnum: invalid data size key [%s]\n", key ));
		return NT_STATUS_UNSUCCESSFUL;
	}
	
	domain->sequence_number = IVAL(data.dptr, 0);
	domain->last_seq_check  = IVAL(data.dptr, 4);
	
	SAFE_FREE(data.dptr);

	/* have we expired? */
	
	time_diff = now - domain->last_seq_check;
	if ( time_diff > lp_winbind_cache_time() ) {
		DEBUG(10,("fetch_cache_seqnum: timeout [%s][%u @ %u]\n",
			domain->name, domain->sequence_number,
			(uint32)domain->last_seq_check));
		return NT_STATUS_UNSUCCESSFUL;
	}

	DEBUG(10,("fetch_cache_seqnum: success [%s][%u @ %u]\n", 
		domain->name, domain->sequence_number, 
		(uint32)domain->last_seq_check));

	return NT_STATUS_OK;
}

static NTSTATUS store_cache_seqnum( struct winbindd_domain *domain )
{
	TDB_DATA data, key;
	fstring key_str;
	char buf[8];
	
	if (!wcache->tdb) {
		DEBUG(10,("store_cache_seqnum: tdb == NULL\n"));
		return NT_STATUS_UNSUCCESSFUL;
	}
		
	fstr_sprintf( key_str, "SEQNUM/%s", domain->name );
	key.dptr = key_str;
	key.dsize = strlen(key_str)+1;
	
	SIVAL(buf, 0, domain->sequence_number);
	SIVAL(buf, 4, domain->last_seq_check);
	data.dptr = buf;
	data.dsize = 8;
	
	if ( tdb_store( wcache->tdb, key, data, TDB_REPLACE) == -1 ) {
		DEBUG(10,("store_cache_seqnum: tdb_store fail key [%s]\n", key_str ));
		return NT_STATUS_UNSUCCESSFUL;
	}

	DEBUG(10,("store_cache_seqnum: success [%s][%u @ %u]\n", 
		domain->name, domain->sequence_number, 
		(uint32)domain->last_seq_check));
	
	return NT_STATUS_OK;
}

/*
  refresh the domain sequence number. If force is True
  then always refresh it, no matter how recently we fetched it
*/

static void refresh_sequence_number(struct winbindd_domain *domain, BOOL force)
{
	NTSTATUS status;
	unsigned time_diff;
	time_t t = time(NULL);
	unsigned cache_time = lp_winbind_cache_time();

	get_cache( domain );

#if 0	/* JERRY -- disable as the default cache time is now 5 minutes */
	/* trying to reconnect is expensive, don't do it too often */
	if (domain->sequence_number == DOM_SEQUENCE_NONE) {
		cache_time *= 8;
	}
#endif

	time_diff = t - domain->last_seq_check;

	/* see if we have to refetch the domain sequence number */
	if (!force && (time_diff < cache_time)) {
		DEBUG(10, ("refresh_sequence_number: %s time ok\n", domain->name));
		goto done;
	}
	
	/* try to get the sequence number from the tdb cache first */
	/* this will update the timestamp as well */
	
	status = fetch_cache_seqnum( domain, t );
	if ( NT_STATUS_IS_OK(status) )
		goto done;	

	/* important! make sure that we know if this is a native 
	   mode domain or not */

	status = domain->backend->sequence_number(domain, &domain->sequence_number);

	if (!NT_STATUS_IS_OK(status)) {
		DEBUG(10,("refresh_sequence_number: failed with %s\n", nt_errstr(status)));
		domain->sequence_number = DOM_SEQUENCE_NONE;
	}
	
	domain->last_status = status;
	domain->last_seq_check = time(NULL);
	
	/* save the new sequence number ni the cache */
	store_cache_seqnum( domain );

done:
	DEBUG(10, ("refresh_sequence_number: %s seq number is now %d\n", 
		   domain->name, domain->sequence_number));

	return;
}

/*
  decide if a cache entry has expired
*/
static BOOL centry_expired(struct winbindd_domain *domain, const char *keystr, struct cache_entry *centry)
{
	/* If we've been told to be offline - stay in that state... */
	if (lp_winbind_offline_logon() && global_winbindd_offline_state) {
		DEBUG(10,("centry_expired: Key %s for domain %s valid as winbindd is globally offline.\n",
			keystr, domain->name ));
		return False;
	}

	/* when the domain is offline and we havent checked in the last 30
	 * seconds if it has become online again, return the cached entry.
	 * This deals with transient offline states... */

	if (!domain->online && 
	    !NT_STATUS_IS_OK(check_negative_conn_cache(domain->name, domain->dcname))) {
		DEBUG(10,("centry_expired: Key %s for domain %s valid as domain is offline.\n",
			keystr, domain->name ));
		return False;
	}

	/* if the server is OK and our cache entry came from when it was down then
	   the entry is invalid */
	if ((domain->sequence_number != DOM_SEQUENCE_NONE) &&  
	    (centry->sequence_number == DOM_SEQUENCE_NONE)) {
		DEBUG(10,("centry_expired: Key %s for domain %s invalid sequence.\n",
			keystr, domain->name ));
		return True;
	}

	/* if the server is down or the cache entry is not older than the
	   current sequence number then it is OK */
	if (wcache_server_down(domain) || 
	    centry->sequence_number == domain->sequence_number) {
		DEBUG(10,("centry_expired: Key %s for domain %s is good.\n",
			keystr, domain->name ));
		return False;
	}

	DEBUG(10,("centry_expired: Key %s for domain %s expired\n",
		keystr, domain->name ));

	/* it's expired */
	return True;
}

static struct cache_entry *wcache_fetch_raw(char *kstr)
{
	TDB_DATA data;
	struct cache_entry *centry;
	TDB_DATA key;

	key.dptr = kstr;
	key.dsize = strlen(kstr);
	data = tdb_fetch(wcache->tdb, key);
	if (!data.dptr) {
		/* a cache miss */
		return NULL;
	}

	centry = SMB_XMALLOC_P(struct cache_entry);
	centry->data = (unsigned char *)data.dptr;
	centry->len = data.dsize;
	centry->ofs = 0;

	if (centry->len < 8) {
		/* huh? corrupt cache? */
		DEBUG(10,("wcache_fetch_raw: Corrupt cache for key %s (len < 8) ?\n", kstr));
		centry_free(centry);
		return NULL;
	}
	
	centry->status = NT_STATUS(centry_uint32(centry));
	centry->sequence_number = centry_uint32(centry);

	return centry;
}

/*
  fetch an entry from the cache, with a varargs key. auto-fetch the sequence
  number and return status
*/
static struct cache_entry *wcache_fetch(struct winbind_cache *cache, 
					struct winbindd_domain *domain,
					const char *format, ...) PRINTF_ATTRIBUTE(3,4);
static struct cache_entry *wcache_fetch(struct winbind_cache *cache, 
					struct winbindd_domain *domain,
					const char *format, ...)
{
	va_list ap;
	char *kstr;
	struct cache_entry *centry;

	extern BOOL opt_nocache;

	if (opt_nocache) {
		return NULL;
	}

	refresh_sequence_number(domain, False);

	va_start(ap, format);
	smb_xvasprintf(&kstr, format, ap);
	va_end(ap);

	centry = wcache_fetch_raw(kstr);
	if (centry == NULL) {
		free(kstr);
		return NULL;
	}

	if (centry_expired(domain, kstr, centry)) {

		DEBUG(10,("wcache_fetch: entry %s expired for domain %s\n",
			 kstr, domain->name ));

		centry_free(centry);
		free(kstr);
		return NULL;
	}

	DEBUG(10,("wcache_fetch: returning entry %s for domain %s\n",
		 kstr, domain->name ));

	free(kstr);
	return centry;
}

/*
  make sure we have at least len bytes available in a centry 
*/
static void centry_expand(struct cache_entry *centry, uint32 len)
{
	if (centry->len - centry->ofs >= len)
		return;
	centry->len *= 2;
	centry->data = SMB_REALLOC(centry->data, centry->len);
	if (!centry->data) {
		DEBUG(0,("out of memory: needed %d bytes in centry_expand\n", centry->len));
		smb_panic("out of memory in centry_expand");
	}
}

/*
  push a uint32 into a centry 
*/
static void centry_put_uint32(struct cache_entry *centry, uint32 v)
{
	centry_expand(centry, 4);
	SIVAL(centry->data, centry->ofs, v);
	centry->ofs += 4;
}

/*
  push a uint16 into a centry 
*/
static void centry_put_uint16(struct cache_entry *centry, uint16 v)
{
	centry_expand(centry, 2);
	SIVAL(centry->data, centry->ofs, v);
	centry->ofs += 2;
}

/*
  push a uint8 into a centry 
*/
static void centry_put_uint8(struct cache_entry *centry, uint8 v)
{
	centry_expand(centry, 1);
	SCVAL(centry->data, centry->ofs, v);
	centry->ofs += 1;
}

/* 
   push a string into a centry 
 */
static void centry_put_string(struct cache_entry *centry, const char *s)
{
	int len;

	if (!s) {
		/* null strings are marked as len 0xFFFF */
		centry_put_uint8(centry, 0xFF);
		return;
	}

	len = strlen(s);
	/* can't handle more than 254 char strings. Truncating is probably best */
	if (len > 254) {
		DEBUG(10,("centry_put_string: truncating len (%d) to: 254\n", len));
		len = 254;
	}
	centry_put_uint8(centry, len);
	centry_expand(centry, len);
	memcpy(centry->data + centry->ofs, s, len);
	centry->ofs += len;
}

/* 
   push a 16 byte hash into a centry - treat as 16 byte string.
 */
static void centry_put_hash16(struct cache_entry *centry, const uint8 val[16])
{
	centry_put_uint8(centry, 16);
	centry_expand(centry, 16);
	memcpy(centry->data + centry->ofs, val, 16);
	centry->ofs += 16;
}

static void centry_put_sid(struct cache_entry *centry, const DOM_SID *sid) 
{
	fstring sid_string;
	centry_put_string(centry, sid_to_string(sid_string, sid));
}

/*
  push a NTTIME into a centry 
*/
static void centry_put_nttime(struct cache_entry *centry, NTTIME nt)
{
	centry_expand(centry, 8);
	SIVAL(centry->data, centry->ofs, nt.low);
	centry->ofs += 4;
	SIVAL(centry->data, centry->ofs, nt.high);
	centry->ofs += 4;
}

/*
  push a time_t into a centry 
*/
static void centry_put_time(struct cache_entry *centry, time_t t)
{
	centry_expand(centry, sizeof(time_t));
	SIVAL(centry->data, centry->ofs, t); /* FIXME: is this correct ?? */
	centry->ofs += sizeof(time_t);
}

/*
  start a centry for output. When finished, call centry_end()
*/
struct cache_entry *centry_start(struct winbindd_domain *domain, NTSTATUS status)
{
	struct cache_entry *centry;

	if (!wcache->tdb)
		return NULL;

	centry = SMB_XMALLOC_P(struct cache_entry);

	centry->len = 8192; /* reasonable default */
	centry->data = SMB_XMALLOC_ARRAY(uint8, centry->len);
	centry->ofs = 0;
	centry->sequence_number = domain->sequence_number;
	centry_put_uint32(centry, NT_STATUS_V(status));
	centry_put_uint32(centry, centry->sequence_number);
	return centry;
}

/*
  finish a centry and write it to the tdb
*/
static void centry_end(struct cache_entry *centry, const char *format, ...) PRINTF_ATTRIBUTE(2,3);
static void centry_end(struct cache_entry *centry, const char *format, ...)
{
	va_list ap;
	char *kstr;
	TDB_DATA key, data;

	va_start(ap, format);
	smb_xvasprintf(&kstr, format, ap);
	va_end(ap);

	key.dptr = kstr;
	key.dsize = strlen(kstr);
	data.dptr = (char *)centry->data;
	data.dsize = centry->ofs;

	tdb_store(wcache->tdb, key, data, TDB_REPLACE);
	free(kstr);
}

static void wcache_save_name_to_sid(struct winbindd_domain *domain, 
				    NTSTATUS status, const char *domain_name,
				    const char *name, const DOM_SID *sid, 
				    enum SID_NAME_USE type)
{
	struct cache_entry *centry;
	fstring uname;

	centry = centry_start(domain, status);
	if (!centry)
		return;
	centry_put_uint32(centry, type);
	centry_put_sid(centry, sid);
	fstrcpy(uname, name);
	strupper_m(uname);
	centry_end(centry, "NS/%s/%s", domain_name, uname);
	DEBUG(10,("wcache_save_name_to_sid: %s\\%s -> %s\n", domain_name, uname,
		  sid_string_static(sid)));
	centry_free(centry);
}

static void wcache_save_sid_to_name(struct winbindd_domain *domain, NTSTATUS status, 
				    const DOM_SID *sid, const char *domain_name, const char *name, enum SID_NAME_USE type)
{
	struct cache_entry *centry;
	fstring sid_string;

	if (is_null_sid(sid)) {
		return;
	}

	centry = centry_start(domain, status);
	if (!centry)
		return;
	if (NT_STATUS_IS_OK(status)) {
		centry_put_uint32(centry, type);
		centry_put_string(centry, domain_name);
		centry_put_string(centry, name);
	}
	centry_end(centry, "SN/%s", sid_to_string(sid_string, sid));
	DEBUG(10,("wcache_save_sid_to_name: %s -> %s\n", sid_string, name));
	centry_free(centry);
}


static void wcache_save_user(struct winbindd_domain *domain, NTSTATUS status, WINBIND_USERINFO *info)
{
	struct cache_entry *centry;
	fstring sid_string;

	if (is_null_sid(&info->user_sid)) {
		return;
	}

	centry = centry_start(domain, status);
	if (!centry)
		return;
	centry_put_string(centry, info->acct_name);
	centry_put_string(centry, info->full_name);
	centry_put_string(centry, info->homedir);
	centry_put_string(centry, info->shell);
	centry_put_sid(centry, &info->user_sid);
	centry_put_sid(centry, &info->group_sid);
	centry_end(centry, "U/%s", sid_to_string(sid_string, &info->user_sid));
	DEBUG(10,("wcache_save_user: %s (acct_name %s)\n", sid_string, info->acct_name));
	centry_free(centry);
}

static void wcache_save_lockout_policy(struct winbindd_domain *domain, NTSTATUS status, SAM_UNK_INFO_12 *lockout_policy)
{
	struct cache_entry *centry;

	centry = centry_start(domain, status);
	if (!centry)
		return;

	centry_put_nttime(centry, lockout_policy->duration);
	centry_put_nttime(centry, lockout_policy->reset_count);
	centry_put_uint16(centry, lockout_policy->bad_attempt_lockout);

	centry_end(centry, "LOC_POL/%s", domain->name);
	
	DEBUG(10,("wcache_save_lockout_policy: %s\n", domain->name));

	centry_free(centry);
}

static void wcache_save_password_policy(struct winbindd_domain *domain, NTSTATUS status, SAM_UNK_INFO_1 *policy)
{
	struct cache_entry *centry;

	centry = centry_start(domain, status);
	if (!centry)
		return;

	centry_put_uint16(centry, policy->min_length_password);
	centry_put_uint16(centry, policy->password_history);
	centry_put_uint32(centry, policy->password_properties);
	centry_put_nttime(centry, policy->expire);
	centry_put_nttime(centry, policy->min_passwordage);

	centry_end(centry, "PWD_POL/%s", domain->name);
	
	DEBUG(10,("wcache_save_password_policy: %s\n", domain->name));

	centry_free(centry);
}

NTSTATUS wcache_cached_creds_exist(struct winbindd_domain *domain, const DOM_SID *sid)
{
	struct winbind_cache *cache = get_cache(domain);
	TDB_DATA data;
	fstring key_str;
	uint32 rid;

	if (!cache->tdb) {
		return NT_STATUS_INTERNAL_DB_ERROR;
	}

	if (is_null_sid(sid)) {
		return NT_STATUS_INVALID_SID;
	}

	if (!(sid_peek_rid(sid, &rid)) || (rid == 0)) {
		return NT_STATUS_INVALID_SID;
	}

	fstr_sprintf(key_str, "CRED/%s", sid_string_static(sid));

	data = tdb_fetch(cache->tdb, make_tdb_data(key_str, strlen(key_str)));
	if (!data.dptr) {
		return NT_STATUS_OBJECT_NAME_NOT_FOUND;
	}

	SAFE_FREE(data.dptr);
	return NT_STATUS_OK;
}

/* Lookup creds for a SID */
NTSTATUS wcache_get_creds(struct winbindd_domain *domain, 
			  TALLOC_CTX *mem_ctx, 
			  const DOM_SID *sid,
			  const uint8 **cached_nt_pass)
{
	struct winbind_cache *cache = get_cache(domain);
	struct cache_entry *centry = NULL;
	NTSTATUS status;
	time_t t;
	uint32 rid;

	if (!cache->tdb) {
		return NT_STATUS_INTERNAL_DB_ERROR;
	}

	if (is_null_sid(sid)) {
		return NT_STATUS_INVALID_SID;
	}

	if (!(sid_peek_rid(sid, &rid)) || (rid == 0)) {
		return NT_STATUS_INVALID_SID;
	}

	centry = wcache_fetch(cache, domain, "CRED/%s", sid_string_static(sid));
	
	if (!centry) {
		DEBUG(10,("wcache_get_creds: entry for [CRED/%s] not found\n", 
			sid_string_static(sid)));
		return NT_STATUS_OBJECT_NAME_NOT_FOUND;
	}

	t = centry_time(centry);
	*cached_nt_pass = (const uint8 *)centry_hash16(centry, mem_ctx);

#if DEBUG_PASSWORD
	dump_data(100, (const char *)cached_nt_pass, NT_HASH_LEN);
#endif
	status = centry->status;

	DEBUG(10,("wcache_get_creds: [Cached] - cached creds for user %s status: %s\n",
		sid_string_static(sid), nt_errstr(status) ));

	centry_free(centry);
	return status;
}

NTSTATUS wcache_save_creds(struct winbindd_domain *domain, 
			   TALLOC_CTX *mem_ctx, 
			   const DOM_SID *sid, 
			   const uint8 nt_pass[NT_HASH_LEN])
{
	struct cache_entry *centry;
	fstring sid_string;
	uint32 rid;

	if (is_null_sid(sid)) {
		return NT_STATUS_INVALID_SID;
	}

	if (!(sid_peek_rid(sid, &rid)) || (rid == 0)) {
		return NT_STATUS_INVALID_SID;
	}

	centry = centry_start(domain, NT_STATUS_OK);
	if (!centry) {
		return NT_STATUS_INTERNAL_DB_ERROR;
	}

#if DEBUG_PASSWORD
	dump_data(100, (const char *)nt_pass, NT_HASH_LEN);
#endif

	centry_put_time(centry, time(NULL));
	centry_put_hash16(centry, nt_pass);
	centry_end(centry, "CRED/%s", sid_to_string(sid_string, sid));

	DEBUG(10,("wcache_save_creds: %s\n", sid_string));

	centry_free(centry);

	return NT_STATUS_OK;
}


/* Query display info. This is the basic user list fn */
static NTSTATUS query_user_list(struct winbindd_domain *domain,
				TALLOC_CTX *mem_ctx,
				uint32 *num_entries, 
				WINBIND_USERINFO **info)
{
	struct winbind_cache *cache = get_cache(domain);
	struct cache_entry *centry = NULL;
	NTSTATUS status;
	unsigned int i, retry;

	if (!cache->tdb)
		goto do_query;

	centry = wcache_fetch(cache, domain, "UL/%s", domain->name);
	if (!centry)
		goto do_query;

	*num_entries = centry_uint32(centry);
	
	if (*num_entries == 0)
		goto do_cached;

	(*info) = TALLOC_ARRAY(mem_ctx, WINBIND_USERINFO, *num_entries);
	if (! (*info))
		smb_panic("query_user_list out of memory");
	for (i=0; i<(*num_entries); i++) {
		(*info)[i].acct_name = centry_string(centry, mem_ctx);
		(*info)[i].full_name = centry_string(centry, mem_ctx);
		(*info)[i].homedir = centry_string(centry, mem_ctx);
		(*info)[i].shell = centry_string(centry, mem_ctx);
		centry_sid(centry, mem_ctx, &(*info)[i].user_sid);
		centry_sid(centry, mem_ctx, &(*info)[i].group_sid);
	}

do_cached:	
	status = centry->status;

	DEBUG(10,("query_user_list: [Cached] - cached list for domain %s status: %s\n",
		domain->name, nt_errstr(status) ));

	centry_free(centry);
	return status;

do_query:
	*num_entries = 0;
	*info = NULL;

	/* Return status value returned by seq number check */

	if (!NT_STATUS_IS_OK(domain->last_status))
		return domain->last_status;

	/* Put the query_user_list() in a retry loop.  There appears to be
	 * some bug either with Windows 2000 or Samba's handling of large
	 * rpc replies.  This manifests itself as sudden disconnection
	 * at a random point in the enumeration of a large (60k) user list.
	 * The retry loop simply tries the operation again. )-:  It's not
	 * pretty but an acceptable workaround until we work out what the
	 * real problem is. */

	retry = 0;
	do {

		DEBUG(10,("query_user_list: [Cached] - doing backend query for list for domain %s\n",
			domain->name ));

		status = domain->backend->query_user_list(domain, mem_ctx, num_entries, info);
		if (!NT_STATUS_IS_OK(status))
			DEBUG(3, ("query_user_list: returned 0x%08x, "
				  "retrying\n", NT_STATUS_V(status)));
			if (NT_STATUS_EQUAL(status, NT_STATUS_UNSUCCESSFUL)) {
				DEBUG(3, ("query_user_list: flushing "
					  "connection cache\n"));
				invalidate_cm_connection(&domain->conn);
			}

	} while (NT_STATUS_V(status) == NT_STATUS_V(NT_STATUS_UNSUCCESSFUL) && 
		 (retry++ < 5));

	/* and save it */
	refresh_sequence_number(domain, False);
	centry = centry_start(domain, status);
	if (!centry)
		goto skip_save;
	centry_put_uint32(centry, *num_entries);
	for (i=0; i<(*num_entries); i++) {
		centry_put_string(centry, (*info)[i].acct_name);
		centry_put_string(centry, (*info)[i].full_name);
		centry_put_string(centry, (*info)[i].homedir);
		centry_put_string(centry, (*info)[i].shell);
		centry_put_sid(centry, &(*info)[i].user_sid);
		centry_put_sid(centry, &(*info)[i].group_sid);
		if (domain->backend->consistent) {
			/* when the backend is consistent we can pre-prime some mappings */
			wcache_save_name_to_sid(domain, NT_STATUS_OK, 
						domain->name,
						(*info)[i].acct_name, 
						&(*info)[i].user_sid,
						SID_NAME_USER);
			wcache_save_sid_to_name(domain, NT_STATUS_OK, 
						&(*info)[i].user_sid,
						domain->name,
						(*info)[i].acct_name, 
						SID_NAME_USER);
			wcache_save_user(domain, NT_STATUS_OK, &(*info)[i]);
		}
	}	
	centry_end(centry, "UL/%s", domain->name);
	centry_free(centry);

skip_save:
	return status;
}

/* list all domain groups */
static NTSTATUS enum_dom_groups(struct winbindd_domain *domain,
				TALLOC_CTX *mem_ctx,
				uint32 *num_entries, 
				struct acct_info **info)
{
	struct winbind_cache *cache = get_cache(domain);
	struct cache_entry *centry = NULL;
	NTSTATUS status;
	unsigned int i;

	if (!cache->tdb)
		goto do_query;

	centry = wcache_fetch(cache, domain, "GL/%s/domain", domain->name);
	if (!centry)
		goto do_query;

	*num_entries = centry_uint32(centry);
	
	if (*num_entries == 0)
		goto do_cached;

	(*info) = TALLOC_ARRAY(mem_ctx, struct acct_info, *num_entries);
	if (! (*info))
		smb_panic("enum_dom_groups out of memory");
	for (i=0; i<(*num_entries); i++) {
		fstrcpy((*info)[i].acct_name, centry_string(centry, mem_ctx));
		fstrcpy((*info)[i].acct_desc, centry_string(centry, mem_ctx));
		(*info)[i].rid = centry_uint32(centry);
	}

do_cached:	
	status = centry->status;

	DEBUG(10,("enum_dom_groups: [Cached] - cached list for domain %s status: %s\n",
		domain->name, nt_errstr(status) ));

	centry_free(centry);
	return status;

do_query:
	*num_entries = 0;
	*info = NULL;

	/* Return status value returned by seq number check */

	if (!NT_STATUS_IS_OK(domain->last_status))
		return domain->last_status;

	DEBUG(10,("enum_dom_groups: [Cached] - doing backend query for list for domain %s\n",
		domain->name ));

	status = domain->backend->enum_dom_groups(domain, mem_ctx, num_entries, info);

	/* and save it */
	refresh_sequence_number(domain, False);
	centry = centry_start(domain, status);
	if (!centry)
		goto skip_save;
	centry_put_uint32(centry, *num_entries);
	for (i=0; i<(*num_entries); i++) {
		centry_put_string(centry, (*info)[i].acct_name);
		centry_put_string(centry, (*info)[i].acct_desc);
		centry_put_uint32(centry, (*info)[i].rid);
	}	
	centry_end(centry, "GL/%s/domain", domain->name);
	centry_free(centry);

skip_save:
	return status;
}

/* list all domain groups */
static NTSTATUS enum_local_groups(struct winbindd_domain *domain,
				TALLOC_CTX *mem_ctx,
				uint32 *num_entries, 
				struct acct_info **info)
{
	struct winbind_cache *cache = get_cache(domain);
	struct cache_entry *centry = NULL;
	NTSTATUS status;
	unsigned int i;

	if (!cache->tdb)
		goto do_query;

	centry = wcache_fetch(cache, domain, "GL/%s/local", domain->name);
	if (!centry)
		goto do_query;

	*num_entries = centry_uint32(centry);
	
	if (*num_entries == 0)
		goto do_cached;

	(*info) = TALLOC_ARRAY(mem_ctx, struct acct_info, *num_entries);
	if (! (*info))
		smb_panic("enum_dom_groups out of memory");
	for (i=0; i<(*num_entries); i++) {
		fstrcpy((*info)[i].acct_name, centry_string(centry, mem_ctx));
		fstrcpy((*info)[i].acct_desc, centry_string(centry, mem_ctx));
		(*info)[i].rid = centry_uint32(centry);
	}

do_cached:	

	/* If we are returning cached data and the domain controller
	   is down then we don't know whether the data is up to date
	   or not.  Return NT_STATUS_MORE_PROCESSING_REQUIRED to
	   indicate this. */

	if (wcache_server_down(domain)) {
		DEBUG(10, ("enum_local_groups: returning cached user list and server was down\n"));
		status = NT_STATUS_MORE_PROCESSING_REQUIRED;
	} else
		status = centry->status;

	DEBUG(10,("enum_local_groups: [Cached] - cached list for domain %s status: %s\n",
		domain->name, nt_errstr(status) ));

	centry_free(centry);
	return status;

do_query:
	*num_entries = 0;
	*info = NULL;

	/* Return status value returned by seq number check */

	if (!NT_STATUS_IS_OK(domain->last_status))
		return domain->last_status;

	DEBUG(10,("enum_local_groups: [Cached] - doing backend query for list for domain %s\n",
		domain->name ));

	status = domain->backend->enum_local_groups(domain, mem_ctx, num_entries, info);

	/* and save it */
	refresh_sequence_number(domain, False);
	centry = centry_start(domain, status);
	if (!centry)
		goto skip_save;
	centry_put_uint32(centry, *num_entries);
	for (i=0; i<(*num_entries); i++) {
		centry_put_string(centry, (*info)[i].acct_name);
		centry_put_string(centry, (*info)[i].acct_desc);
		centry_put_uint32(centry, (*info)[i].rid);
	}
	centry_end(centry, "GL/%s/local", domain->name);
	centry_free(centry);

skip_save:
	return status;
}

/* convert a single name to a sid in a domain */
static NTSTATUS name_to_sid(struct winbindd_domain *domain,
			    TALLOC_CTX *mem_ctx,
			    const char *domain_name,
			    const char *name,
			    DOM_SID *sid,
			    enum SID_NAME_USE *type)
{
	struct winbind_cache *cache = get_cache(domain);
	struct cache_entry *centry = NULL;
	NTSTATUS status;
	fstring uname;

	if (!cache->tdb)
		goto do_query;

	fstrcpy(uname, name);
	strupper_m(uname);
	centry = wcache_fetch(cache, domain, "NS/%s/%s", domain_name, uname);
	if (!centry)
		goto do_query;
	*type = (enum SID_NAME_USE)centry_uint32(centry);
	status = centry->status;
	if (NT_STATUS_IS_OK(status)) {
		centry_sid(centry, mem_ctx, sid);
	}

	DEBUG(10,("name_to_sid: [Cached] - cached name for domain %s status: %s\n",
		domain->name, nt_errstr(status) ));

	centry_free(centry);
	return status;

do_query:
	ZERO_STRUCTP(sid);

	/* If the seq number check indicated that there is a problem
	 * with this DC, then return that status... except for
	 * access_denied.  This is special because the dc may be in
	 * "restrict anonymous = 1" mode, in which case it will deny
	 * most unauthenticated operations, but *will* allow the LSA
	 * name-to-sid that we try as a fallback. */

	if (!(NT_STATUS_IS_OK(domain->last_status)
	      || NT_STATUS_EQUAL(domain->last_status, NT_STATUS_ACCESS_DENIED)))
		return domain->last_status;

	DEBUG(10,("name_to_sid: [Cached] - doing backend query for name for domain %s\n",
		domain->name ));

	status = domain->backend->name_to_sid(domain, mem_ctx, domain_name, name, sid, type);

	/* and save it */
	refresh_sequence_number(domain, False);

	if (domain->online && !is_null_sid(sid)) {
		wcache_save_name_to_sid(domain, status, domain_name, name, sid, *type);
	}

	if (NT_STATUS_IS_OK(status)) {
		strupper_m(CONST_DISCARD(char *,domain_name));
		strlower_m(CONST_DISCARD(char *,name));
		wcache_save_sid_to_name(domain, status, sid, domain_name, name, *type);
	}

	return status;
}

/* convert a sid to a user or group name. The sid is guaranteed to be in the domain
   given */
static NTSTATUS sid_to_name(struct winbindd_domain *domain,
			    TALLOC_CTX *mem_ctx,
			    const DOM_SID *sid,
			    char **domain_name,
			    char **name,
			    enum SID_NAME_USE *type)
{
	struct winbind_cache *cache = get_cache(domain);
	struct cache_entry *centry = NULL;
	NTSTATUS status;
	fstring sid_string;

	if (!cache->tdb)
		goto do_query;

	centry = wcache_fetch(cache, domain, "SN/%s", sid_to_string(sid_string, sid));
	if (!centry)
		goto do_query;
	if (NT_STATUS_IS_OK(centry->status)) {
		*type = (enum SID_NAME_USE)centry_uint32(centry);
		*domain_name = centry_string(centry, mem_ctx);
		*name = centry_string(centry, mem_ctx);
	}
	status = centry->status;

	DEBUG(10,("sid_to_name: [Cached] - cached name for domain %s status: %s\n",
		domain->name, nt_errstr(status) ));

	centry_free(centry);
	return status;

do_query:
	*name = NULL;
	*domain_name = NULL;

	/* If the seq number check indicated that there is a problem
	 * with this DC, then return that status... except for
	 * access_denied.  This is special because the dc may be in
	 * "restrict anonymous = 1" mode, in which case it will deny
	 * most unauthenticated operations, but *will* allow the LSA
	 * sid-to-name that we try as a fallback. */

	if (!(NT_STATUS_IS_OK(domain->last_status)
	      || NT_STATUS_EQUAL(domain->last_status, NT_STATUS_ACCESS_DENIED)))
		return domain->last_status;

	DEBUG(10,("sid_to_name: [Cached] - doing backend query for name for domain %s\n",
		domain->name ));

	status = domain->backend->sid_to_name(domain, mem_ctx, sid, domain_name, name, type);

	/* and save it */
	refresh_sequence_number(domain, False);
	wcache_save_sid_to_name(domain, status, sid, *domain_name, *name, *type);

	/* We can't save the name to sid mapping here, as with sid history a
	 * later name2sid would give the wrong sid. */

	return status;
}

/* Lookup user information from a rid */
static NTSTATUS query_user(struct winbindd_domain *domain, 
			   TALLOC_CTX *mem_ctx, 
			   const DOM_SID *user_sid, 
			   WINBIND_USERINFO *info)
{
	struct winbind_cache *cache = get_cache(domain);
	struct cache_entry *centry = NULL;
	NTSTATUS status;

	if (!cache->tdb)
		goto do_query;

	centry = wcache_fetch(cache, domain, "U/%s", sid_string_static(user_sid));
	
	/* If we have an access denied cache entry and a cached info3 in the
           samlogon cache then do a query.  This will force the rpc back end
           to return the info3 data. */

	if (NT_STATUS_V(domain->last_status) == NT_STATUS_V(NT_STATUS_ACCESS_DENIED) &&
	    netsamlogon_cache_have(user_sid)) {
		DEBUG(10, ("query_user: cached access denied and have cached info3\n"));
		domain->last_status = NT_STATUS_OK;
		centry_free(centry);
		goto do_query;
	}
	
	if (!centry)
		goto do_query;

	info->acct_name = centry_string(centry, mem_ctx);
	info->full_name = centry_string(centry, mem_ctx);
	info->homedir = centry_string(centry, mem_ctx);
	info->shell = centry_string(centry, mem_ctx);
	centry_sid(centry, mem_ctx, &info->user_sid);
	centry_sid(centry, mem_ctx, &info->group_sid);
	status = centry->status;

	DEBUG(10,("query_user: [Cached] - cached info for domain %s status: %s\n",
		domain->name, nt_errstr(status) ));

	centry_free(centry);
	return status;

do_query:
	ZERO_STRUCTP(info);

	/* Return status value returned by seq number check */

	if (!NT_STATUS_IS_OK(domain->last_status))
		return domain->last_status;
	
	DEBUG(10,("sid_to_name: [Cached] - doing backend query for info for domain %s\n",
		domain->name ));

	status = domain->backend->query_user(domain, mem_ctx, user_sid, info);

	/* and save it */
	refresh_sequence_number(domain, False);
	wcache_save_user(domain, status, info);

	return status;
}


/* Lookup groups a user is a member of. */
static NTSTATUS lookup_usergroups(struct winbindd_domain *domain,
				  TALLOC_CTX *mem_ctx,
				  const DOM_SID *user_sid, 
				  uint32 *num_groups, DOM_SID **user_gids)
{
	struct winbind_cache *cache = get_cache(domain);
	struct cache_entry *centry = NULL;
	NTSTATUS status;
	unsigned int i;
	fstring sid_string;

	if (!cache->tdb)
		goto do_query;

	centry = wcache_fetch(cache, domain, "UG/%s", sid_to_string(sid_string, user_sid));
	
	/* If we have an access denied cache entry and a cached info3 in the
           samlogon cache then do a query.  This will force the rpc back end
           to return the info3 data. */

	if (NT_STATUS_V(domain->last_status) == NT_STATUS_V(NT_STATUS_ACCESS_DENIED) &&
	    netsamlogon_cache_have(user_sid)) {
		DEBUG(10, ("lookup_usergroups: cached access denied and have cached info3\n"));
		domain->last_status = NT_STATUS_OK;
		centry_free(centry);
		goto do_query;
	}
	
	if (!centry)
		goto do_query;

	*num_groups = centry_uint32(centry);
	
	if (*num_groups == 0)
		goto do_cached;

	(*user_gids) = TALLOC_ARRAY(mem_ctx, DOM_SID, *num_groups);
	if (! (*user_gids))
		smb_panic("lookup_usergroups out of memory");
	for (i=0; i<(*num_groups); i++) {
		centry_sid(centry, mem_ctx, &(*user_gids)[i]);
	}

do_cached:	
	status = centry->status;

	DEBUG(10,("lookup_usergroups: [Cached] - cached info for domain %s status: %s\n",
		domain->name, nt_errstr(status) ));

	centry_free(centry);
	return status;

do_query:
	(*num_groups) = 0;
	(*user_gids) = NULL;

	/* Return status value returned by seq number check */

	if (!NT_STATUS_IS_OK(domain->last_status))
		return domain->last_status;

	DEBUG(10,("lookup_usergroups: [Cached] - doing backend query for info for domain %s\n",
		domain->name ));

	status = domain->backend->lookup_usergroups(domain, mem_ctx, user_sid, num_groups, user_gids);

	/* and save it */
	refresh_sequence_number(domain, False);
	centry = centry_start(domain, status);
	if (!centry)
		goto skip_save;
	centry_put_uint32(centry, *num_groups);
	for (i=0; i<(*num_groups); i++) {
		centry_put_sid(centry, &(*user_gids)[i]);
	}	
	centry_end(centry, "UG/%s", sid_to_string(sid_string, user_sid));
	centry_free(centry);

skip_save:
	return status;
}

static NTSTATUS lookup_useraliases(struct winbindd_domain *domain,
				   TALLOC_CTX *mem_ctx,
				   uint32 num_sids, const DOM_SID *sids,
				   uint32 *num_aliases, uint32 **alias_rids)
{
	struct winbind_cache *cache = get_cache(domain);
	struct cache_entry *centry = NULL;
	NTSTATUS status;
	char *sidlist = talloc_strdup(mem_ctx, "");
	int i;

	if (!cache->tdb)
		goto do_query;

	if (num_sids == 0) {
		*num_aliases = 0;
		*alias_rids = NULL;
		return NT_STATUS_OK;
	}

	/* We need to cache indexed by the whole list of SIDs, the aliases
	 * resulting might come from any of the SIDs. */

	for (i=0; i<num_sids; i++) {
		sidlist = talloc_asprintf(mem_ctx, "%s/%s", sidlist,
					  sid_string_static(&sids[i]));
		if (sidlist == NULL)
			return NT_STATUS_NO_MEMORY;
	}

	centry = wcache_fetch(cache, domain, "UA%s", sidlist);

	if (!centry)
		goto do_query;

	*num_aliases = centry_uint32(centry);
	*alias_rids = NULL;

	(*alias_rids) = TALLOC_ARRAY(mem_ctx, uint32, *num_aliases);

	if ((*num_aliases != 0) && ((*alias_rids) == NULL)) {
		centry_free(centry);
		return NT_STATUS_NO_MEMORY;
	}

	for (i=0; i<(*num_aliases); i++)
		(*alias_rids)[i] = centry_uint32(centry);

	status = centry->status;

	DEBUG(10,("lookup_useraliases: [Cached] - cached info for domain: %s "
		  "status %s\n", domain->name, nt_errstr(status)));

	centry_free(centry);
	return status;

 do_query:
	(*num_aliases) = 0;
	(*alias_rids) = NULL;

	if (!NT_STATUS_IS_OK(domain->last_status))
		return domain->last_status;

	DEBUG(10,("lookup_usergroups: [Cached] - doing backend query for info "
		  "for domain %s\n", domain->name ));

	status = domain->backend->lookup_useraliases(domain, mem_ctx,
						     num_sids, sids,
						     num_aliases, alias_rids);

	/* and save it */
	refresh_sequence_number(domain, False);
	centry = centry_start(domain, status);
	if (!centry)
		goto skip_save;
	centry_put_uint32(centry, *num_aliases);
	for (i=0; i<(*num_aliases); i++)
		centry_put_uint32(centry, (*alias_rids)[i]);
	centry_end(centry, "UA%s", sidlist);
	centry_free(centry);

 skip_save:
	return status;
}


static NTSTATUS lookup_groupmem(struct winbindd_domain *domain,
				TALLOC_CTX *mem_ctx,
				const DOM_SID *group_sid, uint32 *num_names, 
				DOM_SID **sid_mem, char ***names, 
				uint32 **name_types)
{
	struct winbind_cache *cache = get_cache(domain);
	struct cache_entry *centry = NULL;
	NTSTATUS status;
	unsigned int i;
	fstring sid_string;

	if (!cache->tdb)
		goto do_query;

	centry = wcache_fetch(cache, domain, "GM/%s", sid_to_string(sid_string, group_sid));
	if (!centry)
		goto do_query;

	*num_names = centry_uint32(centry);
	
	if (*num_names == 0)
		goto do_cached;

	(*sid_mem) = TALLOC_ARRAY(mem_ctx, DOM_SID, *num_names);
	(*names) = TALLOC_ARRAY(mem_ctx, char *, *num_names);
	(*name_types) = TALLOC_ARRAY(mem_ctx, uint32, *num_names);

	if (! (*sid_mem) || ! (*names) || ! (*name_types)) {
		smb_panic("lookup_groupmem out of memory");
	}

	for (i=0; i<(*num_names); i++) {
		centry_sid(centry, mem_ctx, &(*sid_mem)[i]);
		(*names)[i] = centry_string(centry, mem_ctx);
		(*name_types)[i] = centry_uint32(centry);
	}

do_cached:	
	status = centry->status;

	DEBUG(10,("lookup_groupmem: [Cached] - cached info for domain %s status: %s\n",
		domain->name, nt_errstr(status)));

	centry_free(centry);
	return status;

do_query:
	(*num_names) = 0;
	(*sid_mem) = NULL;
	(*names) = NULL;
	(*name_types) = NULL;
	
	/* Return status value returned by seq number check */

	if (!NT_STATUS_IS_OK(domain->last_status))
		return domain->last_status;

	DEBUG(10,("lookup_groupmem: [Cached] - doing backend query for info for domain %s\n",
		domain->name ));

	status = domain->backend->lookup_groupmem(domain, mem_ctx, group_sid, num_names, 
						  sid_mem, names, name_types);

	/* and save it */
	refresh_sequence_number(domain, False);
	centry = centry_start(domain, status);
	if (!centry)
		goto skip_save;
	centry_put_uint32(centry, *num_names);
	for (i=0; i<(*num_names); i++) {
		centry_put_sid(centry, &(*sid_mem)[i]);
		centry_put_string(centry, (*names)[i]);
		centry_put_uint32(centry, (*name_types)[i]);
	}	
	centry_end(centry, "GM/%s", sid_to_string(sid_string, group_sid));
	centry_free(centry);

skip_save:
	return status;
}

/* find the sequence number for a domain */
static NTSTATUS sequence_number(struct winbindd_domain *domain, uint32 *seq)
{
	refresh_sequence_number(domain, False);

	*seq = domain->sequence_number;

	return NT_STATUS_OK;
}

/* enumerate trusted domains 
 * (we need to have the list of trustdoms in the cache when we go offline) -
 * Guenther */
static NTSTATUS trusted_domains(struct winbindd_domain *domain,
				TALLOC_CTX *mem_ctx,
				uint32 *num_domains,
				char ***names,
				char ***alt_names,
				DOM_SID **dom_sids)
{
 	struct winbind_cache *cache = get_cache(domain);
 	struct cache_entry *centry = NULL;
 	NTSTATUS status;
	int i;
 
	if (!cache->tdb)
		goto do_query;
 
	centry = wcache_fetch(cache, domain, "TRUSTDOMS/%s", domain->name);
	
	if (!centry) {
 		goto do_query;
	}
 
	*num_domains = centry_uint32(centry);
	
	(*names) 	= TALLOC_ARRAY(mem_ctx, char *, *num_domains);
	(*alt_names) 	= TALLOC_ARRAY(mem_ctx, char *, *num_domains);
	(*dom_sids) 	= TALLOC_ARRAY(mem_ctx, DOM_SID, *num_domains);
 
	if (! (*dom_sids) || ! (*names) || ! (*alt_names)) {
		smb_panic("trusted_domains out of memory");
 	}
 
	for (i=0; i<(*num_domains); i++) {
		(*names)[i] = centry_string(centry, mem_ctx);
		(*alt_names)[i] = centry_string(centry, mem_ctx);
		centry_sid(centry, mem_ctx, &(*dom_sids)[i]);
	}

 	status = centry->status;
 
	DEBUG(10,("trusted_domains: [Cached] - cached info for domain %s (%d trusts) status: %s\n",
		domain->name, *num_domains, nt_errstr(status) ));
 
 	centry_free(centry);
 	return status;
 
do_query:
	(*num_domains) = 0;
	(*dom_sids) = NULL;
	(*names) = NULL;
	(*alt_names) = NULL;
 
	/* Return status value returned by seq number check */

 	if (!NT_STATUS_IS_OK(domain->last_status))
 		return domain->last_status;
	
	DEBUG(10,("trusted_domains: [Cached] - doing backend query for info for domain %s\n",
		domain->name ));
 
	status = domain->backend->trusted_domains(domain, mem_ctx, num_domains,
						names, alt_names, dom_sids);

	/* no trusts gives NT_STATUS_NO_MORE_ENTRIES resetting to NT_STATUS_OK
	 * so that the generic centry handling still applies correctly -
	 * Guenther*/

	if (!NT_STATUS_IS_ERR(status)) {
		status = NT_STATUS_OK;
	}

	/* and save it */
	refresh_sequence_number(domain, False);
 
 	centry = centry_start(domain, status);
	if (!centry)
		goto skip_save;

	centry_put_uint32(centry, *num_domains);

	for (i=0; i<(*num_domains); i++) {
		centry_put_string(centry, (*names)[i]);
		centry_put_string(centry, (*alt_names)[i]);
		centry_put_sid(centry, &(*dom_sids)[i]);
 	}
	
	centry_end(centry, "TRUSTDOMS/%s", domain->name);
 
 	centry_free(centry);
 
skip_save:
 	return status;
}	

/* get lockout policy */
static NTSTATUS lockout_policy(struct winbindd_domain *domain,
 			       TALLOC_CTX *mem_ctx,
			       SAM_UNK_INFO_12 *policy){
 	struct winbind_cache *cache = get_cache(domain);
 	struct cache_entry *centry = NULL;
 	NTSTATUS status;
 
	if (!cache->tdb)
		goto do_query;
 
	centry = wcache_fetch(cache, domain, "LOC_POL/%s", domain->name);
	
	if (!centry)
 		goto do_query;
 
	policy->duration = centry_nttime(centry);
	policy->reset_count = centry_nttime(centry);
	policy->bad_attempt_lockout = centry_uint16(centry);
 
 	status = centry->status;
 
	DEBUG(10,("lockout_policy: [Cached] - cached info for domain %s status: %s\n",
		domain->name, nt_errstr(status) ));
 
 	centry_free(centry);
 	return status;
 
do_query:
	ZERO_STRUCTP(policy);
 
	/* Return status value returned by seq number check */

 	if (!NT_STATUS_IS_OK(domain->last_status))
 		return domain->last_status;
	
	DEBUG(10,("lockout_policy: [Cached] - doing backend query for info for domain %s\n",
		domain->name ));
 
	status = domain->backend->lockout_policy(domain, mem_ctx, policy); 
 
	/* and save it */
 	refresh_sequence_number(domain, False);
	wcache_save_lockout_policy(domain, status, policy);
 
 	return status;
}
 
/* get password policy */
static NTSTATUS password_policy(struct winbindd_domain *domain,
				TALLOC_CTX *mem_ctx,
				SAM_UNK_INFO_1 *policy)
{
	struct winbind_cache *cache = get_cache(domain);
	struct cache_entry *centry = NULL;
	NTSTATUS status;

	if (!cache->tdb)
		goto do_query;
 
	centry = wcache_fetch(cache, domain, "PWD_POL/%s", domain->name);
	
	if (!centry)
		goto do_query;

	policy->min_length_password = centry_uint16(centry);
	policy->password_history = centry_uint16(centry);
	policy->password_properties = centry_uint32(centry);
	policy->expire = centry_nttime(centry);
	policy->min_passwordage = centry_nttime(centry);

	status = centry->status;

	DEBUG(10,("lockout_policy: [Cached] - cached info for domain %s status: %s\n",
		domain->name, nt_errstr(status) ));

	centry_free(centry);
	return status;

do_query:
	ZERO_STRUCTP(policy);

	/* Return status value returned by seq number check */

	if (!NT_STATUS_IS_OK(domain->last_status))
		return domain->last_status;
	
	DEBUG(10,("password_policy: [Cached] - doing backend query for info for domain %s\n",
		domain->name ));

	status = domain->backend->password_policy(domain, mem_ctx, policy); 

	/* and save it */
	refresh_sequence_number(domain, False);
	wcache_save_password_policy(domain, status, policy);

	return status;
}


/* Invalidate cached user and group lists coherently */

static int traverse_fn(TDB_CONTEXT *the_tdb, TDB_DATA kbuf, TDB_DATA dbuf, 
		       void *state)
{
	if (strncmp(kbuf.dptr, "UL/", 3) == 0 ||
	    strncmp(kbuf.dptr, "GL/", 3) == 0)
		tdb_delete(the_tdb, kbuf);

	return 0;
}

/* Invalidate the getpwnam and getgroups entries for a winbindd domain */

void wcache_invalidate_samlogon(struct winbindd_domain *domain, 
				NET_USER_INFO_3 *info3)
{
	struct winbind_cache *cache;
	
	if (!domain)
		return;

	cache = get_cache(domain);
	netsamlogon_clear_cached_user(cache->tdb, info3);
}

void wcache_invalidate_cache(void)
{
	struct winbindd_domain *domain;

	for (domain = domain_list(); domain; domain = domain->next) {
		struct winbind_cache *cache = get_cache(domain);

		DEBUG(10, ("wcache_invalidate_cache: invalidating cache "
			   "entries for %s\n", domain->name));
		if (cache)
			tdb_traverse(cache->tdb, traverse_fn, NULL);
	}
}

static BOOL init_wcache(void)
{
	if (wcache == NULL) {
		wcache = SMB_XMALLOC_P(struct winbind_cache);
		ZERO_STRUCTP(wcache);
	}

	if (wcache->tdb != NULL)
		return True;

	/* when working offline we must not clear the cache on restart */
	wcache->tdb = tdb_open_log(lock_path("winbindd_cache.tdb"),
				WINBINDD_CACHE_TDB_DEFAULT_HASH_SIZE, 
				lp_winbind_offline_logon() ? TDB_DEFAULT : (TDB_DEFAULT | TDB_CLEAR_IF_FIRST), 
				O_RDWR|O_CREAT, 0600);

	if (wcache->tdb == NULL) {
		DEBUG(0,("Failed to open winbindd_cache.tdb!\n"));
		return False;
	}

	return True;
}

void cache_store_response(pid_t pid, struct winbindd_response *response)
{
	fstring key_str;

	if (!init_wcache())
		return;

	DEBUG(10, ("Storing response for pid %d, len %d\n",
		   pid, response->length));

	fstr_sprintf(key_str, "DR/%d", pid);
	if (tdb_store(wcache->tdb, string_tdb_data(key_str), 
		      make_tdb_data((void *)response, sizeof(*response)),
		      TDB_REPLACE) == -1)
		return;

	if (response->length == sizeof(*response))
		return;

	/* There's extra data */

	DEBUG(10, ("Storing extra data: len=%d\n",
		   (int)(response->length - sizeof(*response))));

	fstr_sprintf(key_str, "DE/%d", pid);
	if (tdb_store(wcache->tdb, string_tdb_data(key_str),
		      make_tdb_data(response->extra_data.data,
				    response->length - sizeof(*response)),
		      TDB_REPLACE) == 0)
		return;

	/* We could not store the extra data, make sure the tdb does not
	 * contain a main record with wrong dangling extra data */

	fstr_sprintf(key_str, "DR/%d", pid);
	tdb_delete(wcache->tdb, string_tdb_data(key_str));

	return;
}

BOOL cache_retrieve_response(pid_t pid, struct winbindd_response * response)
{
	TDB_DATA data;
	fstring key_str;

	if (!init_wcache())
		return False;

	DEBUG(10, ("Retrieving response for pid %d\n", pid));

	fstr_sprintf(key_str, "DR/%d", pid);
	data = tdb_fetch(wcache->tdb, string_tdb_data(key_str));

	if (data.dptr == NULL)
		return False;

	if (data.dsize != sizeof(*response))
		return False;

	memcpy(response, data.dptr, data.dsize);
	SAFE_FREE(data.dptr);

	if (response->length == sizeof(*response)) {
		response->extra_data.data = NULL;
		return True;
	}

	/* There's extra data */

	DEBUG(10, ("Retrieving extra data length=%d\n",
		   (int)(response->length - sizeof(*response))));

	fstr_sprintf(key_str, "DE/%d", pid);
	data = tdb_fetch(wcache->tdb, string_tdb_data(key_str));

	if (data.dptr == NULL) {
		DEBUG(0, ("Did not find extra data\n"));
		return False;
	}

	if (data.dsize != (response->length - sizeof(*response))) {
		DEBUG(0, ("Invalid extra data length: %d\n", (int)data.dsize));
		SAFE_FREE(data.dptr);
		return False;
	}

	dump_data(11, data.dptr, data.dsize);

	response->extra_data.data = data.dptr;
	return True;
}

void cache_cleanup_response(pid_t pid)
{
	fstring key_str;

	if (!init_wcache())
		return;

	fstr_sprintf(key_str, "DR/%d", pid);
	tdb_delete(wcache->tdb, string_tdb_data(key_str));

	fstr_sprintf(key_str, "DE/%d", pid);
	tdb_delete(wcache->tdb, string_tdb_data(key_str));

	return;
}


BOOL lookup_cached_sid(TALLOC_CTX *mem_ctx, const DOM_SID *sid,
		       const char **domain_name, const char **name,
		       enum SID_NAME_USE *type)
{
	struct winbindd_domain *domain;
	struct winbind_cache *cache;
	struct cache_entry *centry = NULL;
	NTSTATUS status;

	domain = find_lookup_domain_from_sid(sid);
	if (domain == NULL) {
		return False;
	}

	cache = get_cache(domain);

	if (cache->tdb == NULL) {
		return False;
	}

	centry = wcache_fetch(cache, domain, "SN/%s", sid_string_static(sid));
	if (centry == NULL) {
		return False;
	}

	if (NT_STATUS_IS_OK(centry->status)) {
		*type = (enum SID_NAME_USE)centry_uint32(centry);
		*domain_name = centry_string(centry, mem_ctx);
		*name = centry_string(centry, mem_ctx);
	}

	status = centry->status;
	centry_free(centry);
	return NT_STATUS_IS_OK(status);
}

BOOL lookup_cached_name(TALLOC_CTX *mem_ctx,
			const char *domain_name,
			const char *name,
			DOM_SID *sid,
			enum SID_NAME_USE *type)
{
	struct winbindd_domain *domain;
	struct winbind_cache *cache;
	struct cache_entry *centry = NULL;
	NTSTATUS status;
	fstring uname;

	domain = find_lookup_domain_from_name(domain_name);
	if (domain == NULL) {
		return False;
	}

	cache = get_cache(domain);

	if (cache->tdb == NULL) {
		return False;
	}

	fstrcpy(uname, name);
	strupper_m(uname);
	
	centry = wcache_fetch(cache, domain, "NS/%s/%s", domain_name, uname);
	if (centry == NULL) {
		return False;
	}

	if (NT_STATUS_IS_OK(centry->status)) {
		*type = (enum SID_NAME_USE)centry_uint32(centry);
		centry_sid(centry, mem_ctx, sid);
	}

	status = centry->status;
	centry_free(centry);
	
	return NT_STATUS_IS_OK(status);
}

void cache_name2sid(struct winbindd_domain *domain, 
		    const char *domain_name, const char *name,
		    enum SID_NAME_USE type, const DOM_SID *sid)
{
	refresh_sequence_number(domain, False);
	wcache_save_name_to_sid(domain, NT_STATUS_OK, domain_name, name,
				sid, type);
}

/* delete all centries that don't have NT_STATUS_OK set */
static int traverse_fn_cleanup(TDB_CONTEXT *the_tdb, TDB_DATA kbuf, 
			       TDB_DATA dbuf, void *state)
{
	struct cache_entry *centry;

	centry = wcache_fetch_raw(kbuf.dptr);
	if (!centry) {
		return 0;
	}

	if (!NT_STATUS_IS_OK(centry->status)) {
		DEBUG(10,("deleting centry %s\n", kbuf.dptr));
		tdb_delete(the_tdb, kbuf);
	}

	centry_free(centry);
	return 0;
}

/* flush the cache */
void wcache_flush_cache(void)
{
	extern BOOL opt_nocache;

	if (!wcache)
		return;
	if (wcache->tdb) {
		tdb_close(wcache->tdb);
		wcache->tdb = NULL;
	}
	if (opt_nocache)
		return;

	/* when working offline we must not clear the cache on restart */
	wcache->tdb = tdb_open_log(lock_path("winbindd_cache.tdb"),
				WINBINDD_CACHE_TDB_DEFAULT_HASH_SIZE, 
				lp_winbind_offline_logon() ? TDB_DEFAULT : (TDB_DEFAULT | TDB_CLEAR_IF_FIRST), 
				O_RDWR|O_CREAT, 0600);

	if (!wcache->tdb) {
		DEBUG(0,("Failed to open winbindd_cache.tdb!\n"));
		return;
	}

	tdb_traverse(wcache->tdb, traverse_fn_cleanup, NULL);

	DEBUG(10,("wcache_flush_cache success\n"));
}

/* Count cached creds */

static int traverse_fn_cached_creds(TDB_CONTEXT *the_tdb, TDB_DATA kbuf, TDB_DATA dbuf, 
			 	    void *state)
{
	int *cred_count = (int*)state;
 
	if (strncmp(kbuf.dptr, "CRED/", 5) == 0) {
		(*cred_count)++;
	}
	return 0;
}

NTSTATUS wcache_count_cached_creds(struct winbindd_domain *domain, int *count)
{
	struct winbind_cache *cache = get_cache(domain);

	*count = 0;

	if (!cache->tdb) {
		return NT_STATUS_INTERNAL_DB_ERROR;
	}
 
	tdb_traverse(cache->tdb, traverse_fn_cached_creds, (void *)count);

	return NT_STATUS_OK;
}

struct cred_list {
	struct cred_list *prev, *next;
	TDB_DATA key;
	fstring name;
	time_t created;
};
static struct cred_list *wcache_cred_list;

static int traverse_fn_get_credlist(TDB_CONTEXT *the_tdb, TDB_DATA kbuf, TDB_DATA dbuf, 
				    void *state)
{
	struct cred_list *cred;

	if (strncmp(kbuf.dptr, "CRED/", 5) == 0) {

		cred = SMB_MALLOC_P(struct cred_list);
		if (cred == NULL) {
			DEBUG(0,("traverse_fn_remove_first_creds: failed to malloc new entry for list\n"));
			return -1;
		}

		ZERO_STRUCTP(cred);
		
		/* save a copy of the key */
		
		fstrcpy(cred->name, kbuf.dptr);		
		DLIST_ADD(wcache_cred_list, cred);
	}
	
	return 0;
}

NTSTATUS wcache_remove_oldest_cached_creds(struct winbindd_domain *domain, const DOM_SID *sid) 
{
	struct winbind_cache *cache = get_cache(domain);
	NTSTATUS status;
	int ret;
	struct cred_list *cred, *oldest = NULL;

	if (!cache->tdb) {
		return NT_STATUS_INTERNAL_DB_ERROR;
	}

	/* we possibly already have an entry */
 	if (sid && NT_STATUS_IS_OK(wcache_cached_creds_exist(domain, sid))) {
	
		fstring key_str;

		DEBUG(11,("we already have an entry, deleting that\n"));

		fstr_sprintf(key_str, "CRED/%s", sid_string_static(sid));

		tdb_delete(cache->tdb, string_tdb_data(key_str));

		return NT_STATUS_OK;
	}

	ret = tdb_traverse(cache->tdb, traverse_fn_get_credlist, NULL);
	if (ret == 0) {
		return NT_STATUS_OK;
	} else if ((ret == -1) || (wcache_cred_list == NULL)) {
		return NT_STATUS_OBJECT_NAME_NOT_FOUND;
	}

	ZERO_STRUCTP(oldest);

	for (cred = wcache_cred_list; cred; cred = cred->next) {

		TDB_DATA data;
		time_t t;

		data = tdb_fetch(cache->tdb, make_tdb_data(cred->name, strlen(cred->name)));
		if (!data.dptr) {
			DEBUG(10,("wcache_remove_oldest_cached_creds: entry for [%s] not found\n", 
				cred->name));
			status = NT_STATUS_OBJECT_NAME_NOT_FOUND;
			goto done;
		}
	
		t = IVAL(data.dptr, 0);
		SAFE_FREE(data.dptr);

		if (!oldest) {
			oldest = SMB_MALLOC_P(struct cred_list);
			if (oldest == NULL) {
				status = NT_STATUS_NO_MEMORY;
				goto done;
			}

			fstrcpy(oldest->name, cred->name);
			oldest->created = t;
			continue;
		}

		if (t < oldest->created) {
			fstrcpy(oldest->name, cred->name);
			oldest->created = t;
		}
	}

	if (tdb_delete(cache->tdb, string_tdb_data(oldest->name)) == 0) {
		status = NT_STATUS_OK;
	} else {
		status = NT_STATUS_UNSUCCESSFUL;
	}
done:
	SAFE_FREE(wcache_cred_list);
	SAFE_FREE(oldest);
	
	return status;
}

/* Change the global online/offline state. */
BOOL set_global_winbindd_state_offline(void)
{
	TDB_DATA data;
	int err;

	DEBUG(10,("set_global_winbindd_state_offline: offline requested.\n"));

	/* Only go offline if someone has created
	   the key "WINBINDD_OFFLINE" in the cache tdb. */

	if (wcache == NULL || wcache->tdb == NULL) {
		DEBUG(10,("set_global_winbindd_state_offline: wcache not open yet.\n"));
		return False;
	}

	if (!lp_winbind_offline_logon()) {
		DEBUG(10,("set_global_winbindd_state_offline: rejecting.\n"));
		return False;
	}

	if (global_winbindd_offline_state) {
		/* Already offline. */
		return True;
	}

	wcache->tdb->ecode = 0;

	data = tdb_fetch_bystring( wcache->tdb, "WINBINDD_OFFLINE" );

	/* As this is a key with no data we don't need to free, we
	   check for existence by looking at tdb_err. */

	err = tdb_error(wcache->tdb);

	if (err == TDB_ERR_NOEXIST) {
		DEBUG(10,("set_global_winbindd_state_offline: offline state not set.\n"));
		return False;
	} else {
		DEBUG(10,("set_global_winbindd_state_offline: offline state set.\n"));
		global_winbindd_offline_state = True;
		return True;
	}
}

void set_global_winbindd_state_online(void)
{
	DEBUG(10,("set_global_winbindd_state_online: online requested.\n"));

	if (!lp_winbind_offline_logon()) {
		DEBUG(10,("set_global_winbindd_state_online: rejecting.\n"));
		return;
	}

	if (!global_winbindd_offline_state) {
		/* Already online. */
		return;
	}
	global_winbindd_offline_state = False;

	if (!wcache->tdb) {
		return;
	}

	/* Ensure there is no key "WINBINDD_OFFLINE" in the cache tdb. */
	tdb_delete_bystring(wcache->tdb, "WINBINDD_OFFLINE");
}

BOOL get_global_winbindd_state_online(void)
{
	return global_winbindd_offline_state;
}

/* the cache backend methods are exposed via this structure */
struct winbindd_methods cache_methods = {
	True,
	query_user_list,
	enum_dom_groups,
	enum_local_groups,
	name_to_sid,
	sid_to_name,
	query_user,
	lookup_usergroups,
	lookup_useraliases,
	lookup_groupmem,
	sequence_number,
	lockout_policy,
	password_policy,
	trusted_domains
};
