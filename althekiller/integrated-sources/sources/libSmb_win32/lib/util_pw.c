/* 
   Unix SMB/CIFS implementation.

   Safe versions of getpw* calls

   Copyright (C) Andrew Bartlett 2002
   
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

struct passwd *tcopy_passwd(TALLOC_CTX *mem_ctx, const struct passwd *from) 
{
	struct passwd *ret = TALLOC_P(mem_ctx, struct passwd);
	if (!ret) {
		return NULL;
	}
	ret->pw_name = talloc_strdup(ret, from->pw_name);
	ret->pw_passwd = talloc_strdup(ret, from->pw_passwd);
	ret->pw_uid = from->pw_uid;
	ret->pw_gid = from->pw_gid;
	ret->pw_gecos = talloc_strdup(ret, from->pw_gecos);
	ret->pw_dir = talloc_strdup(ret, from->pw_dir);
	ret->pw_shell = talloc_strdup(ret, from->pw_shell);
	return ret;
}

#define PWNAMCACHE_SIZE 4
static struct passwd **pwnam_cache = NULL;

static void init_pwnam_cache(void)
{
	if (pwnam_cache != NULL)
		return;

	pwnam_cache = TALLOC_ZERO_ARRAY(NULL, struct passwd *,
					PWNAMCACHE_SIZE);
	if (pwnam_cache == NULL) {
		smb_panic("Could not init pwnam_cache\n");
	}

	return;
}

void flush_pwnam_cache(void)
{
	TALLOC_FREE(pwnam_cache);
	pwnam_cache = NULL;
	init_pwnam_cache();
}

struct passwd *getpwnam_alloc(TALLOC_CTX *mem_ctx, const char *name)
{
	int i;

	struct passwd *temp;

	init_pwnam_cache();

	for (i=0; i<PWNAMCACHE_SIZE; i++) {
		if ((pwnam_cache[i] != NULL) && 
		    (strcmp(name, pwnam_cache[i]->pw_name) == 0)) {
			DEBUG(10, ("Got %s from pwnam_cache\n", name));
			return talloc_reference(mem_ctx, pwnam_cache[i]);
		}
	}

	temp = sys_getpwnam(name);
	
	if (!temp) {
#if 0
		if (errno == ENOMEM) {
			/* what now? */
		}
#endif
		return NULL;
	}

	for (i=0; i<PWNAMCACHE_SIZE; i++) {
		if (pwnam_cache[i] == NULL)
			break;
	}

	if (i == PWNAMCACHE_SIZE)
		i = rand() % PWNAMCACHE_SIZE;

	if (pwnam_cache[i] != NULL) {
		TALLOC_FREE(pwnam_cache[i]);
	}

	pwnam_cache[i] = tcopy_passwd(pwnam_cache, temp);
	if (pwnam_cache[i]!= NULL && mem_ctx != NULL) {
		return talloc_reference(mem_ctx, pwnam_cache[i]);
	}

	return tcopy_passwd(NULL, pwnam_cache[i]);
}

struct passwd *getpwuid_alloc(TALLOC_CTX *mem_ctx, uid_t uid) 
{
	struct passwd *temp;

	temp = sys_getpwuid(uid);
	
	if (!temp) {
#if 0
		if (errno == ENOMEM) {
			/* what now? */
		}
#endif
		return NULL;
	}

	return tcopy_passwd(mem_ctx, temp);
}
