/* 
   Unix SMB/CIFS implementation.

   Winbind daemon connection manager

   Copyright (C) Tim Potter 		2001
   Copyright (C) Andrew Bartlett 	2002
   Copyright (C) Gerald (Jerry) Carter 	2003
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#include "includes.h"

#define CONNCACHE_ADDR		1
#define CONNCACHE_NAME		2

/* cache entry contains either a server name **or** and IP address as 
   the key.  This means that a server could have two entries (one for each key) */
   
struct failed_connection_cache {
	fstring 	domain_name;
	fstring 	controller;
	time_t 		lookup_time;
	NTSTATUS 	nt_status;
	struct failed_connection_cache *prev, *next;
};

static struct failed_connection_cache *failed_connection_cache;

/**********************************************************************
 Check for a previously failed connection.
 failed_cache_timeout is an a absolute number of seconds after which
 we should time this out. If failed_cache_timeout == 0 then time out
 immediately. If failed_cache_timeout == -1 then never time out.
**********************************************************************/

NTSTATUS check_negative_conn_cache_timeout( const char *domain, const char *server, unsigned int failed_cache_timeout )
{
	struct failed_connection_cache *fcc;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	
	/* can't check if we don't have strings */
	
	if ( !domain || !server )
		return NT_STATUS_OK;

	for (fcc = failed_connection_cache; fcc; fcc = fcc->next) {
	
		if (!(strequal(domain, fcc->domain_name) && strequal(server, fcc->controller))) {
			continue; /* no match; check the next entry */
		}
		
		/* we have a match so see if it is still current */
		if (failed_cache_timeout != (unsigned int)-1) {
			if (failed_cache_timeout == 0 ||
					(time(NULL) - fcc->lookup_time) > (time_t)failed_cache_timeout) {
				/* Cache entry has expired, delete it */

				DEBUG(10, ("check_negative_conn_cache: cache entry expired for %s, %s\n", 
					domain, server ));

				DLIST_REMOVE(failed_connection_cache, fcc);
				SAFE_FREE(fcc);

				return NT_STATUS_OK;
			}
		}

		/* The timeout hasn't expired yet so return false */

		DEBUG(10, ("check_negative_conn_cache: returning negative entry for %s, %s\n", 
			domain, server ));

		result = fcc->nt_status;
		return result;
	}

	/* end of function means no cache entry */	
	return NT_STATUS_OK;
}

NTSTATUS check_negative_conn_cache( const char *domain, const char *server)
{
	return check_negative_conn_cache_timeout(domain, server, FAILED_CONNECTION_CACHE_TIMEOUT);
}

/**********************************************************************
 Add an entry to the failed conneciton cache (aither a name of dotted 
 decimal IP
**********************************************************************/

void add_failed_connection_entry(const char *domain, const char *server, NTSTATUS result) 
{
	struct failed_connection_cache *fcc;

	SMB_ASSERT(!NT_STATUS_IS_OK(result));

	/* Check we already aren't in the cache.  We always have to have 
	   a domain, but maybe not a specific DC name. */

	for (fcc = failed_connection_cache; fcc; fcc = fcc->next) {			
		if ( strequal(fcc->domain_name, domain) && strequal(fcc->controller, server) ) {
			DEBUG(10, ("add_failed_connection_entry: domain %s (%s) already tried and failed\n",
				   domain, server ));
			/* Update the failed time. */
			fcc->lookup_time = time(NULL);
			return;
		}
	}

	/* Create negative lookup cache entry for this domain and controller */

	if ( !(fcc = SMB_MALLOC_P(struct failed_connection_cache)) ) {
		DEBUG(0, ("malloc failed in add_failed_connection_entry!\n"));
		return;
	}
	
	ZERO_STRUCTP(fcc);
	
	fstrcpy( fcc->domain_name, domain );
	fstrcpy( fcc->controller, server );
	fcc->lookup_time = time(NULL);
	fcc->nt_status = result;
	
	DEBUG(10,("add_failed_connection_entry: added domain %s (%s) to failed conn cache\n",
		domain, server ));
	
	DLIST_ADD(failed_connection_cache, fcc);
}

/****************************************************************************
****************************************************************************/
 
void flush_negative_conn_cache( void )
{
	struct failed_connection_cache *fcc;
	
	fcc = failed_connection_cache;

	while (fcc) {
		struct failed_connection_cache *fcc_next;

		fcc_next = fcc->next;
		DLIST_REMOVE(failed_connection_cache, fcc);
		free(fcc);

		fcc = fcc_next;
	}

}
