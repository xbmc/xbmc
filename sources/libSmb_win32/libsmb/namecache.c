/* 
   Unix SMB/CIFS implementation.

   NetBIOS name cache module on top of gencache mechanism.
   
   Copyright (C) Tim Potter         2002
   Copyright (C) Rafal Szczesniak   2002
   
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

#define NBTKEY_FMT  "NBT/%s#%02X"


/**
 * Initialise namecache system. Function calls gencache
 * initialisation function to perform necessary actions
 * 
 * @return true upon successful initialisation of the cache or
 *         false on failure
 **/

BOOL namecache_enable(void)
{
	/*
	 * Check if name caching disabled by setting the name cache
	 * timeout to zero.
	 */ 

	if (lp_name_cache_timeout() == 0) {
		DEBUG(5, ("namecache_enable: disabling netbios name cache\n"));
		return False;
	}

	/* Init namecache by calling gencache initialisation */

	if (!gencache_init()) {
		DEBUG(2, ("namecache_enable: Couldn't initialise namecache on top of gencache.\n"));
		return False;
	}

	/* I leave it for now, though I don't think we really need this (mimir, 27.09.2002) */
	DEBUG(5, ("namecache_enable: enabling netbios namecache, timeout %d "
		  "seconds\n", lp_name_cache_timeout()));

	return True;
}


/**
 * Shutdown namecache. Routine calls gencache close function
 * to safely close gencache file.
 *
 * @return true upon successful shutdown of the cache or
 *         false on failure
 **/
 
BOOL namecache_shutdown(void)
{
	if (!gencache_shutdown()) {
		DEBUG(2, ("namecache_shutdown: Couldn't close namecache on top of gencache.\n"));
		return False;
	}
	
	DEBUG(5, ("namecache_shutdown: netbios namecache closed successfully.\n"));
	return True;
}


/**
 * Generates a key for netbios name lookups on basis of
 * netbios name and type.
 * The caller must free returned key string when finished.
 *
 * @param name netbios name string (case insensitive)
 * @param name_type netbios type of the name being looked up
 *
 * @return string consisted of uppercased name and appended
 *         type number
 */

static char* namecache_key(const char *name, int name_type)
{
	char *keystr;
	asprintf(&keystr, NBTKEY_FMT, strupper_static(name), name_type);

	return keystr;
}


/**
 * Store a name(s) in the name cache
 *
 * @param name netbios names array
 * @param name_type integer netbios name type
 * @param num_names number of names being stored
 * @param ip_list array of in_addr structures containing
 *        ip addresses being stored
 **/

BOOL namecache_store(const char *name, int name_type,
                     int num_names, struct ip_service *ip_list)
{
	time_t expiry;
	char *key, *value_string;
	int i;
	BOOL ret;

	/*
	 * we use gecache call to avoid annoying debug messages about
	 * initialised namecache again and again...
	 */
	if (!gencache_init()) return False;

	if ( DEBUGLEVEL >= 5 ) {
		DEBUG(5, ("namecache_store: storing %d address%s for %s#%02x: ",
			num_names, num_names == 1 ? "": "es", name, name_type));

		for (i = 0; i < num_names; i++) 
			DEBUGADD(5, ("%s:%d%s", inet_ntoa(ip_list[i].ip), 
				ip_list[i].port, (i == (num_names - 1) ? "" : ",")));
			
		DEBUGADD(5, ("\n"));
	}
	
	key = namecache_key(name, name_type);
	expiry = time(NULL) + lp_name_cache_timeout();

	/*
	 * Generate string representation of ip addresses list
	 * First, store the number of ip addresses and then
	 * place each single ip
	 */
	if (!ipstr_list_make(&value_string, ip_list, num_names)) {
		SAFE_FREE(key);
		SAFE_FREE(value_string);
		return False;
	}
	
	/* set the entry */
	ret = gencache_set(key, value_string, expiry);
	SAFE_FREE(key);
	SAFE_FREE(value_string);
	return ret;
}


/**
 * Look up a name in the cache.
 *
 * @param name netbios name to look up for
 * @param name_type netbios name type of @param name
 * @param ip_list mallocated list of IP addresses if found in the cache,
 *        NULL otherwise
 * @param num_names number of entries found
 *
 * @return true upon successful fetch or
 *         false if name isn't found in the cache or has expired
 **/

BOOL namecache_fetch(const char *name, int name_type, struct ip_service **ip_list,
                     int *num_names)
{
	char *key, *value;
	time_t timeout;

	/* exit now if null pointers were passed as they're required further */
	if (!ip_list || !num_names) return False;

	if (!gencache_init())
		return False;

	*num_names = 0;

	/* 
	 * Use gencache interface - lookup the key
	 */
	key = namecache_key(name, name_type);

	if (!gencache_get(key, &value, &timeout)) {
		DEBUG(5, ("no entry for %s#%02X found.\n", name, name_type));
		gencache_del(key);
		SAFE_FREE(key);
		SAFE_FREE(value);		 
		return False;
	} else {
		DEBUG(5, ("name %s#%02X found.\n", name, name_type));
	}
	
	/*
	 * Split up the stored value into the list of IP adresses
	 */
	*num_names = ipstr_list_parse(value, ip_list);
	
	SAFE_FREE(key);
	SAFE_FREE(value);
			 
	return *num_names > 0;		/* true only if some ip has been fetched */
}


/**
 * Delete single namecache entry. Look at the
 * gencache_iterate definition.
 *
 **/

static void flush_netbios_name(const char* key, const char *value, time_t timeout, void* dptr)
{
	gencache_del(key);
	DEBUG(5, ("Deleting entry %s\n", key));
}


/**
 * Flush all names from the name cache.
 * It's done by gencache_iterate()
 *
 * @return True upon successful deletion or
 *         False in case of an error
 **/

void namecache_flush(void)
{
	if (!gencache_init())
		return;

	/* 
	 * iterate through each NBT cache's entry and flush it
	 * by flush_netbios_name function
	 */
	gencache_iterate(flush_netbios_name, NULL, "NBT/*");
	DEBUG(5, ("Namecache flushed\n"));
}

/* Construct a name status record key. */

static char *namecache_status_record_key(const char *name, int name_type1,
				int name_type2, struct in_addr keyip)
{
	char *keystr;

	asprintf(&keystr, "NBT/%s#%02X.%02X.%s",
			strupper_static(name), name_type1, name_type2, inet_ntoa(keyip));
	return keystr;
}

/* Store a name status record. */

BOOL namecache_status_store(const char *keyname, int keyname_type,
		int name_type, struct in_addr keyip,
		const char *srvname)
{
	char *key;
	time_t expiry;
	BOOL ret;

	if (!gencache_init())
		return False;

	key = namecache_status_record_key(keyname, keyname_type, name_type, keyip);
	if (!key)
		return False;

	expiry = time(NULL) + lp_name_cache_timeout();
	ret = gencache_set(key, srvname, expiry);

	if (ret)
		DEBUG(5, ("namecache_status_store: entry %s -> %s\n", key, srvname ));
	else
		DEBUG(5, ("namecache_status_store: entry %s store failed.\n", key ));

	SAFE_FREE(key);
	return ret;
}

/* Fetch a name status record. */

BOOL namecache_status_fetch(const char *keyname, int keyname_type,
			int name_type, struct in_addr keyip, char *srvname_out)
{
	char *key = NULL;
	char *value = NULL;
	time_t timeout;

	if (!gencache_init())
		return False;

	key = namecache_status_record_key(keyname, keyname_type, name_type, keyip);
	if (!key)
		return False;

	if (!gencache_get(key, &value, &timeout)) {
		DEBUG(5, ("namecache_status_fetch: no entry for %s found.\n", key));
		gencache_del(key);
		SAFE_FREE(key);
		SAFE_FREE(value);
		return False;
	} else {
		DEBUG(5, ("namecache_status_fetch: key %s -> %s\n", key, value ));
	}

	strlcpy(srvname_out, value, 16);
	SAFE_FREE(key);
	SAFE_FREE(value);
	return True;
}
