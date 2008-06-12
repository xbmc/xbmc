/* 
   Unix SMB/CIFS implementation.

   Generic, persistent and shared between processes cache mechanism for use
   by various parts of the Samba code

   Copyright (C) Rafal Szczesniak    2002
   
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

#undef  DBGC_CLASS
#define DBGC_CLASS DBGC_TDB

#define TIMEOUT_LEN 12
#define CACHE_DATA_FMT	"%12u/%s"
#define READ_CACHE_DATA_FMT_TEMPLATE "%%12u/%%%us"

static TDB_CONTEXT *cache;
static BOOL cache_readonly;

/**
 * @file gencache.c
 * @brief Generic, persistent and shared between processes cache mechanism
 *        for use by various parts of the Samba code
 *
 **/


/**
 * Cache initialisation function. Opens cache tdb file or creates
 * it if does not exist.
 *
 * @return true on successful initialisation of the cache or
 *         false on failure
 **/

BOOL gencache_init(void)
{
	char* cache_fname = NULL;
	
	/* skip file open if it's already opened */
	if (cache) return True;

	cache_fname = lock_path("gencache.tdb");

	DEBUG(5, ("Opening cache file at %s\n", cache_fname));

	cache = tdb_open_log(cache_fname, 0, TDB_DEFAULT,
	                     O_RDWR|O_CREAT, 0644);

	if (!cache && (errno == EACCES)) {
		cache = tdb_open_log(cache_fname, 0, TDB_DEFAULT, O_RDONLY, 0644);
		if (cache) {
			cache_readonly = True;
			DEBUG(5, ("gencache_init: Opening cache file %s read-only.\n", cache_fname));
		}
	}

	if (!cache) {
		DEBUG(5, ("Attempt to open gencache.tdb has failed.\n"));
		return False;
	}
	return True;
}


/**
 * Cache shutdown function. Closes opened cache tdb file.
 *
 * @return true on successful closing the cache or
 *         false on failure during cache shutdown
 **/
 
BOOL gencache_shutdown(void)
{
	int ret;
	/* tdb_close routine returns -1 on error */
	if (!cache) return False;
	DEBUG(5, ("Closing cache file\n"));
	ret = tdb_close(cache);
	cache = NULL;
	cache_readonly = False;
	return ret != -1;
}


/**
 * Set an entry in the cache file. If there's no such
 * one, then add it.
 *
 * @param keystr string that represents a key of this entry
 * @param value text representation value being cached
 * @param timeout time when the value is expired
 *
 * @retval true when entry is successfuly stored
 * @retval false on failure
 **/
 
BOOL gencache_set(const char *keystr, const char *value, time_t timeout)
{
	int ret;
	TDB_DATA keybuf, databuf;
	char* valstr = NULL;
	
	/* fail completely if get null pointers passed */
	SMB_ASSERT(keystr && value);

	if (!gencache_init()) return False;
	
	if (cache_readonly) {
		return False;
	}

	asprintf(&valstr, CACHE_DATA_FMT, (int)timeout, value);
	if (!valstr)
		return False;

	keybuf.dptr = SMB_STRDUP(keystr);
	keybuf.dsize = strlen(keystr)+1;
	databuf.dptr = SMB_STRDUP(valstr);
	databuf.dsize = strlen(valstr)+1;
	DEBUG(10, ("Adding cache entry with key = %s; value = %s and timeout ="
	           " %s (%d seconds %s)\n", keybuf.dptr, value,ctime(&timeout),
		   (int)(timeout - time(NULL)), 
		   timeout > time(NULL) ? "ahead" : "in the past"));

	ret = tdb_store(cache, keybuf, databuf, 0);
	SAFE_FREE(valstr);
	SAFE_FREE(keybuf.dptr);
	SAFE_FREE(databuf.dptr);
	
	return ret == 0;
}


/**
 * Set existing entry to the cache file.
 *
 * @param keystr string that represents a key of this entry
 * @param valstr text representation value being cached
 * @param timeout time when the value is expired
 *
 * @retval true when entry is successfuly set
 * @retval false on failure
 **/

BOOL gencache_set_only(const char *keystr, const char *valstr, time_t timeout)
{
	int ret = -1;
	TDB_DATA keybuf, databuf;
	char *old_valstr, *datastr;
	time_t old_timeout;
	
	/* fail completely if get null pointers passed */
	SMB_ASSERT(keystr && valstr);

	if (!gencache_init()) return False;
			
	if (cache_readonly) {
		return False;
	}

	/* 
	 * Check whether entry exists in the cache
	 * Don't verify gencache_get exit code, since the entry may be expired
	 */	
	gencache_get(keystr, &old_valstr, &old_timeout);
	
	if (!(old_valstr && old_timeout)) return False;
		
	DEBUG(10, ("Setting cache entry with key = %s; old value = %s and old timeout \
	           = %s\n", keystr, old_valstr, ctime(&old_timeout)));

	asprintf(&datastr, CACHE_DATA_FMT, (int)timeout, valstr);
	keybuf.dptr = SMB_STRDUP(keystr);
	keybuf.dsize = strlen(keystr)+1;
	databuf.dptr = SMB_STRDUP(datastr);
	databuf.dsize = strlen(datastr)+1;
	DEBUGADD(10, ("New value = %s, new timeout = %s (%d seconds %s)", valstr,
	              ctime(&timeout), (int)(timeout - time(NULL)),
	              timeout > time(NULL) ? "ahead" : "in the past"));

		
	ret = tdb_store(cache, keybuf, databuf, TDB_REPLACE);

	SAFE_FREE(datastr);
	SAFE_FREE(old_valstr);
	SAFE_FREE(keybuf.dptr);
	SAFE_FREE(databuf.dptr);
	
	return ret == 0;
}
 

/**
 * Delete one entry from the cache file.
 *
 * @param keystr string that represents a key of this entry
 *
 * @retval true upon successful deletion
 * @retval false in case of failure
 **/

BOOL gencache_del(const char *keystr)
{
	int ret;
	TDB_DATA keybuf;
	
	/* fail completely if get null pointers passed */
	SMB_ASSERT(keystr);

	if (!gencache_init()) return False;	
	
	if (cache_readonly) {
		return False;
	}

	keybuf.dptr = SMB_STRDUP(keystr);
	keybuf.dsize = strlen(keystr)+1;
	DEBUG(10, ("Deleting cache entry (key = %s)\n", keystr));
	ret = tdb_delete(cache, keybuf);
	
	SAFE_FREE(keybuf.dptr);
	return ret == 0;
}


/**
 * Get existing entry from the cache file.
 *
 * @param keystr string that represents a key of this entry
 * @param valstr buffer that is allocated and filled with the entry value
 *        buffer's disposing must be done outside
 * @param timeout pointer to a time_t that is filled with entry's
 *        timeout
 *
 * @retval true when entry is successfuly fetched
 * @retval False for failure
 **/

BOOL gencache_get(const char *keystr, char **valstr, time_t *timeout)
{
	TDB_DATA keybuf, databuf;

	/* fail completely if get null pointers passed */
	SMB_ASSERT(keystr);

	if (!gencache_init()) {
		return False;
	}
	
	keybuf.dptr = SMB_STRDUP(keystr);
	keybuf.dsize = strlen(keystr)+1;
	databuf = tdb_fetch(cache, keybuf);
	SAFE_FREE(keybuf.dptr);
	
	if (databuf.dptr && databuf.dsize > TIMEOUT_LEN) {
		char* entry_buf = SMB_STRNDUP(databuf.dptr, databuf.dsize);
		char *v;
		time_t t;
		unsigned u;
		int status;
		char *fmt;

		v = SMB_MALLOC(databuf.dsize + 1 - TIMEOUT_LEN);
		if (!v) {
			return False;
		}

		SAFE_FREE(databuf.dptr);

		asprintf(&fmt, READ_CACHE_DATA_FMT_TEMPLATE, (unsigned int)databuf.dsize - TIMEOUT_LEN);
		if (!fmt) {
			SAFE_FREE(v);
			return False;
		}

		status = sscanf(entry_buf, fmt, &u, v);
		SAFE_FREE(fmt);

		if ( status != 2 ) {
			DEBUG(0, ("gencache_get: Invalid return %d from sscanf\n", status ));
		}
		t = u;
		SAFE_FREE(entry_buf);

		DEBUG(10, ("Returning %s cache entry: key = %s, value = %s, "
			   "timeout = %s", t > time(NULL) ? "valid" :
			   "expired", keystr, v, ctime(&t)));

		if (valstr) {
			*valstr = v;
		} else {
			SAFE_FREE(v);
		}

		if (timeout) {
			*timeout = t;
		}

		return t > time(NULL);

	} 

	SAFE_FREE(databuf.dptr);

	if (valstr) {
		*valstr = NULL;
	}
	if (timeout) {
		timeout = NULL;
	}

	DEBUG(10, ("Cache entry with key = %s couldn't be found\n", keystr));
	return False;
}

/**
 * Iterate through all entries which key matches to specified pattern
 *
 * @param fn pointer to the function that will be supplied with each single
 *        matching cache entry (key, value and timeout) as an arguments
 * @param data void pointer to an arbitrary data that is passed directly to the fn
 *        function on each call
 * @param keystr_pattern pattern the existing entries' keys are matched to
 *
 **/

void gencache_iterate(void (*fn)(const char* key, const char *value, time_t timeout, void* dptr),
                      void* data, const char* keystr_pattern)
{
	TDB_LIST_NODE *node, *first_node;
	TDB_DATA databuf;
	char *keystr = NULL, *valstr = NULL, *entry = NULL;
	time_t timeout = 0;
	int status;
	unsigned u;

	/* fail completely if get null pointers passed */
	SMB_ASSERT(fn && keystr_pattern);

	if (!gencache_init()) return;

	DEBUG(5, ("Searching cache keys with pattern %s\n", keystr_pattern));
	node = tdb_search_keys(cache, keystr_pattern);
	first_node = node;
	
	while (node) {
		char *fmt;

		/* ensure null termination of the key string */
		keystr = SMB_STRNDUP(node->node_key.dptr, node->node_key.dsize);
		if (!keystr) {
			break;
		}
		
		/* 
		 * We don't use gencache_get function, because we need to iterate through
		 * all of the entries. Validity verification is up to fn routine.
		 */
		databuf = tdb_fetch(cache, node->node_key);
		if (!databuf.dptr || databuf.dsize <= TIMEOUT_LEN) {
			SAFE_FREE(databuf.dptr);
			SAFE_FREE(keystr);
			node = node->next;
			continue;
		}
		entry = SMB_STRNDUP(databuf.dptr, databuf.dsize);
		if (!entry) {
			SAFE_FREE(databuf.dptr);
			SAFE_FREE(keystr);
			break;
		}

		SAFE_FREE(databuf.dptr);

		valstr = SMB_MALLOC(databuf.dsize + 1 - TIMEOUT_LEN);
		if (!valstr) {
			SAFE_FREE(entry);
			SAFE_FREE(keystr);
			break;
		}

		asprintf(&fmt, READ_CACHE_DATA_FMT_TEMPLATE, (unsigned int)databuf.dsize - TIMEOUT_LEN);
		if (!fmt) {
			SAFE_FREE(valstr);
			SAFE_FREE(entry);
			SAFE_FREE(keystr);
			break;
		}
		status = sscanf(entry, fmt, &u, valstr);
		SAFE_FREE(fmt);

		if ( status != 2 ) {
			DEBUG(0,("gencache_iterate: invalid return from sscanf %d\n",status));
		}
		timeout = u;
		
		DEBUG(10, ("Calling function with arguments (key = %s, value = %s, timeout = %s)\n",
		           keystr, valstr, ctime(&timeout)));
		fn(keystr, valstr, timeout, data);
		
		SAFE_FREE(valstr);
		SAFE_FREE(entry);
		SAFE_FREE(keystr);
		node = node->next;
	}
	
	tdb_search_list_free(first_node);
}

/********************************************************************
 lock a key
********************************************************************/

int gencache_lock_entry( const char *key )
{
	if (!gencache_init())
		return -1;
	
	return tdb_lock_bystring(cache, key);
}

/********************************************************************
 unlock a key
********************************************************************/

void gencache_unlock_entry( const char *key )
{
	if (!gencache_init())
		return;
	
	tdb_unlock_bystring(cache, key);
	return;
}


