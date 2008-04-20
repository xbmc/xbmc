/* 
   Unix SMB/CIFS implementation.
   stat cache code
   Copyright (C) Andrew Tridgell 1992-2000
   Copyright (C) Jeremy Allison 1999-2004
   Copyright (C) Andrew Bartlett <abartlet@samba.org> 2003
   
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

/****************************************************************************
 Stat cache code used in unix_convert.
*****************************************************************************/

static TDB_CONTEXT *tdb_stat_cache;

/**
 * Add an entry into the stat cache.
 *
 * @param full_orig_name       The original name as specified by the client
 * @param orig_translated_path The name on our filesystem.
 * 
 * @note Only the first strlen(orig_translated_path) characters are stored 
 *       into the cache.  This means that full_orig_name will be internally
 *       truncated.
 *
 */

void stat_cache_add( const char *full_orig_name, const char *orig_translated_path, BOOL case_sensitive)
{
	char *translated_path;
	size_t translated_path_length;
	TDB_DATA data_val;
	char *original_path;
	size_t original_path_length;
	size_t sc_size = lp_max_stat_cache_size();

	if (!lp_stat_cache())
		return;

	if (sc_size && (tdb_stat_cache->map_size > sc_size*1024)) {
		reset_stat_cache();
	}

	ZERO_STRUCT(data_val);

	/*
	 * Don't cache trivial valid directory entries such as . and ..
	 */

	if((*full_orig_name == '\0') || (full_orig_name[0] == '.' && 
				((full_orig_name[1] == '\0') ||
				 (full_orig_name[1] == '.' && full_orig_name[2] == '\0'))))
		return;

	/*
	 * If we are in case insentive mode, we don't need to
	 * store names that need no translation - else, it
	 * would be a waste.
	 */

	if(case_sensitive && (strcmp(full_orig_name, orig_translated_path) == 0))
		return;

	/*
	 * Remove any trailing '/' characters from the
	 * translated path.
	 */

	translated_path = SMB_STRDUP(orig_translated_path);
	if (!translated_path)
		return;

	translated_path_length = strlen(translated_path);

	if(translated_path[translated_path_length-1] == '/') {
		translated_path[translated_path_length-1] = '\0';
		translated_path_length--;
	}

	if(case_sensitive) {
		original_path = SMB_STRDUP(full_orig_name);
	} else {
		original_path = strdup_upper(full_orig_name);
	}

	if (!original_path) {
		SAFE_FREE(translated_path);
		return;
	}

	original_path_length = strlen(original_path);

	if(original_path[original_path_length-1] == '/') {
		original_path[original_path_length-1] = '\0';
		original_path_length--;
	}

	if (original_path_length != translated_path_length) {
		if (original_path_length < translated_path_length) {
			DEBUG(0, ("OOPS - tried to store stat cache entry for weird length paths [%s] %lu and [%s] %lu)!\n",
				  original_path, (unsigned long)original_path_length, translated_path, (unsigned long)translated_path_length));
			SAFE_FREE(original_path);
			SAFE_FREE(translated_path);
			return;
		}

		/* we only want to index by the first part of original_path,
			up to the length of translated_path */

		original_path[translated_path_length] = '\0';
		original_path_length = translated_path_length;
	}

	/*
	 * New entry or replace old entry.
	 */
  
	data_val.dsize = translated_path_length + 1;
	data_val.dptr = translated_path;

	if (tdb_store_bystring(tdb_stat_cache, original_path, data_val, TDB_REPLACE) != 0) {
		DEBUG(0,("stat_cache_add: Error storing entry %s -> %s\n", original_path, translated_path));
	} else {
		DEBUG(5,("stat_cache_add: Added entry (%lx:size%x) %s -> %s\n",
			(unsigned long)data_val.dptr, (unsigned int)data_val.dsize, original_path, translated_path));
	}

	SAFE_FREE(original_path);
	SAFE_FREE(translated_path);
}

/**
 * Look through the stat cache for an entry
 *
 * @param conn    A connection struct to do the stat() with.
 * @param name    The path we are attempting to cache, modified by this routine
 *                to be correct as far as the cache can tell us
 * @param dirpath The path as far as the stat cache told us.
 * @param start   A pointer into name, for where to 'start' in fixing the rest of the name up.
 * @param psd     A stat buffer, NOT from the cache, but just a side-effect.
 *
 * @return True if we translated (and did a scuccessful stat on) the entire name.
 *
 */

BOOL stat_cache_lookup(connection_struct *conn, pstring name, pstring dirpath, 
		       char **start, SMB_STRUCT_STAT *pst)
{
	char *chk_name;
	size_t namelen;
	BOOL sizechanged = False;
	unsigned int num_components = 0;

	if (!lp_stat_cache())
		return False;
 
	namelen = strlen(name);

	*start = name;

	DO_PROFILE_INC(statcache_lookups);

	/*
	 * Don't lookup trivial valid directory entries.
	 */
	if((*name == '\0') || (name[0] == '.' && 
				((name[1] == '\0') ||
				 (name[1] == '.' && name[1] == '\0'))))
		return False;

	if (conn->case_sensitive) {
		chk_name = SMB_STRDUP(name);
		if (!chk_name) {
			DEBUG(0, ("stat_cache_lookup: strdup failed!\n"));
			return False;
		}

	} else {
		chk_name = strdup_upper(name);
		if (!chk_name) {
			DEBUG(0, ("stat_cache_lookup: strdup_upper failed!\n"));
			return False;
		}

		/*
		 * In some language encodings the length changes
		 * if we uppercase. We need to treat this differently
		 * below.
		 */
		if (strlen(chk_name) != namelen)
			sizechanged = True;
	}

	while (1) {
		TDB_DATA data_val;
		char *sp;

		data_val = tdb_fetch_bystring(tdb_stat_cache, chk_name);
		if(data_val.dptr == NULL || data_val.dsize == 0) {
			DEBUG(10,("stat_cache_lookup: lookup failed for name [%s]\n", chk_name ));
			/*
			 * Didn't find it - remove last component for next try.
			 */
			sp = strrchr_m(chk_name, '/');
			if (sp) {
				*sp = '\0';
				/*
				 * Count the number of times we have done this,
				 * we'll need it when reconstructing the string.
				 */
				if (sizechanged)
					num_components++;

			} else {
				/*
				 * We reached the end of the name - no match.
				 */
				DO_PROFILE_INC(statcache_misses);
				SAFE_FREE(chk_name);
				return False;
			}
			if((*chk_name == '\0') || (strcmp(chk_name, ".") == 0)
					|| (strcmp(chk_name, "..") == 0)) {
				DO_PROFILE_INC(statcache_misses);
				SAFE_FREE(chk_name);
				return False;
			}
		} else {
			BOOL retval;
			char *translated_path = data_val.dptr;
			size_t translated_path_length = data_val.dsize - 1;

			DEBUG(10,("stat_cache_lookup: lookup succeeded for name [%s] -> [%s]\n", chk_name, translated_path ));
			DO_PROFILE_INC(statcache_hits);
			if(SMB_VFS_STAT(conn,translated_path, pst) != 0) {
				/* Discard this entry - it doesn't exist in the filesystem.  */
				tdb_delete_bystring(tdb_stat_cache, chk_name);
				SAFE_FREE(chk_name);
				SAFE_FREE(data_val.dptr);
				return False;
			}

			if (!sizechanged) {
				memcpy(name, translated_path, MIN(sizeof(pstring)-1, translated_path_length));
			} else if (num_components == 0) {
				pstrcpy(name, translated_path);
			} else {
				sp = strnrchr_m(name, '/', num_components);
				if (sp) {
					pstring last_component;
					pstrcpy(last_component, sp);
					pstrcpy(name, translated_path);
					pstrcat(name, last_component);
				} else {
					pstrcpy(name, translated_path);
				}
			}

			/* set pointer for 'where to start' on fixing the rest of the name */
			*start = &name[translated_path_length];
			if(**start == '/')
				++*start;

			pstrcpy(dirpath, translated_path);
			retval = (namelen == translated_path_length) ? True : False;
			SAFE_FREE(chk_name);
			SAFE_FREE(data_val.dptr);
			return retval;
		}
	}
}

/***************************************************************
 Compute a hash value based on a string key value.
 The function returns the bucket index number for the hashed key.
 JRA. Use a djb-algorithm hash for speed.
***************************************************************/
                                                                                                     
u32 fast_string_hash(TDB_DATA *key)
{
        u32 n = 0;
        const char *p;
        for (p = key->dptr; *p != '\0'; p++) {
                n = ((n << 5) + n) ^ (u32)(*p);
        }
        return n;
}

/***************************************************************************
 Initializes or clears the stat cache.
**************************************************************************/

BOOL reset_stat_cache( void )
{
	if (!lp_stat_cache())
		return True;

	if (tdb_stat_cache) {
		tdb_close(tdb_stat_cache);
	}

	/* Create the in-memory tdb using our custom hash function. */
	tdb_stat_cache = tdb_open_ex("statcache", 1031, TDB_INTERNAL,
                                    (O_RDWR|O_CREAT), 0644, NULL, fast_string_hash);

	if (!tdb_stat_cache)
		return False;
	return True;
}
