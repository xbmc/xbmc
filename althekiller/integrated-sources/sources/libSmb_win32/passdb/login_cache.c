/* 
   Unix SMB/CIFS implementation.
   struct samu local cache for 
   Copyright (C) Jim McDonough (jmcd@us.ibm.com) 2004.
      
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

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_PASSDB

#define LOGIN_CACHE_FILE "login_cache.tdb"

#define SAM_CACHE_FORMAT "dwwd"

static TDB_CONTEXT *cache;

BOOL login_cache_init(void)
{
	char* cache_fname = NULL;
	
	/* skip file open if it's already opened */
	if (cache) return True;

	asprintf(&cache_fname, "%s/%s", lp_lockdir(), LOGIN_CACHE_FILE);
	if (cache_fname)
		DEBUG(5, ("Opening cache file at %s\n", cache_fname));
	else {
		DEBUG(0, ("Filename allocation failed.\n"));
		return False;
	}

	cache = tdb_open_log(cache_fname, 0, TDB_DEFAULT,
	                     O_RDWR|O_CREAT, 0644);

	if (!cache)
		DEBUG(5, ("Attempt to open %s failed.\n", cache_fname));

	SAFE_FREE(cache_fname);

	return (cache ? True : False);
}

BOOL login_cache_shutdown(void)
{
	/* tdb_close routine returns -1 on error */
	if (!cache) return False;
	DEBUG(5, ("Closing cache file\n"));
	return tdb_close(cache) != -1;
}

/* if we can't read the cache, oh well, no need to return anything */
LOGIN_CACHE * login_cache_read(struct samu *sampass)
{
	TDB_DATA keybuf, databuf;
	LOGIN_CACHE *entry;

	if (!login_cache_init())
		return NULL;

	if (pdb_get_nt_username(sampass) == NULL) {
		return NULL;
	}

	keybuf.dptr = SMB_STRDUP(pdb_get_nt_username(sampass));
	if (!keybuf.dptr || !strlen(keybuf.dptr)) {
		SAFE_FREE(keybuf.dptr);
		return NULL;
	}
	keybuf.dsize = strlen(keybuf.dptr) + 1;

	DEBUG(7, ("Looking up login cache for user %s\n",
		  keybuf.dptr));
	databuf = tdb_fetch(cache, keybuf);
	SAFE_FREE(keybuf.dptr);

	if (!(entry = SMB_MALLOC_P(LOGIN_CACHE))) {
		DEBUG(1, ("Unable to allocate cache entry buffer!\n"));
		SAFE_FREE(databuf.dptr);
		return NULL;
	}

	if (tdb_unpack (databuf.dptr, databuf.dsize, SAM_CACHE_FORMAT,
			&entry->entry_timestamp, &entry->acct_ctrl, 
			&entry->bad_password_count, 
			&entry->bad_password_time) == -1) {
		DEBUG(7, ("No cache entry found\n"));
		SAFE_FREE(entry);
		SAFE_FREE(databuf.dptr);
		return NULL;
	}

	SAFE_FREE(databuf.dptr);

	DEBUG(5, ("Found login cache entry: timestamp %12u, flags 0x%x, count %d, time %12u\n",
		  (unsigned int)entry->entry_timestamp, entry->acct_ctrl, 
		  entry->bad_password_count, (unsigned int)entry->bad_password_time));
	return entry;
}

BOOL login_cache_write(const struct samu *sampass, LOGIN_CACHE entry)
{

	TDB_DATA keybuf, databuf;
	BOOL ret;

	if (!login_cache_init())
		return False;

	if (pdb_get_nt_username(sampass) == NULL) {
		return False;
	}

	keybuf.dptr = SMB_STRDUP(pdb_get_nt_username(sampass));
	if (!keybuf.dptr || !strlen(keybuf.dptr)) {
		SAFE_FREE(keybuf.dptr);
		return False;
	}
	keybuf.dsize = strlen(keybuf.dptr) + 1;

	entry.entry_timestamp = time(NULL);

	databuf.dsize = 
		tdb_pack(NULL, 0, SAM_CACHE_FORMAT,
			 entry.entry_timestamp,
			 entry.acct_ctrl,
			 entry.bad_password_count,
			 entry.bad_password_time);
	databuf.dptr = SMB_MALLOC(databuf.dsize);
	if (!databuf.dptr) {
		SAFE_FREE(keybuf.dptr);
		return False;
	}
			 
	if (tdb_pack(databuf.dptr, databuf.dsize, SAM_CACHE_FORMAT,
			 entry.entry_timestamp,
			 entry.acct_ctrl,
			 entry.bad_password_count,
			 entry.bad_password_time)
	    != databuf.dsize) {
		SAFE_FREE(keybuf.dptr);
		SAFE_FREE(databuf.dptr);
		return False;
	}

	ret = tdb_store(cache, keybuf, databuf, 0);
	SAFE_FREE(keybuf.dptr);
	SAFE_FREE(databuf.dptr);
	return ret == 0;
}

BOOL login_cache_delentry(const struct samu *sampass)
{
	int ret;
	TDB_DATA keybuf;
	
	if (!login_cache_init()) 
		return False;	

	if (pdb_get_nt_username(sampass) == NULL) {
		return False;
	}

	keybuf.dptr = SMB_STRDUP(pdb_get_nt_username(sampass));
	if (!keybuf.dptr || !strlen(keybuf.dptr)) {
		SAFE_FREE(keybuf.dptr);
		return False;
	}
	keybuf.dsize = strlen(keybuf.dptr) + 1;
	DEBUG(9, ("About to delete entry for %s\n", keybuf.dptr));
	ret = tdb_delete(cache, keybuf);
	DEBUG(9, ("tdb_delete returned %d\n", ret));
	
	SAFE_FREE(keybuf.dptr);
	return ret == 0;
}

