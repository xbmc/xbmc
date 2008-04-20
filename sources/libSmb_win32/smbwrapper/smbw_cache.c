/* 
   Unix SMB/CIFS implementation.
   SMB wrapper directory functions
   Copyright (C) Tim Potter 2000
   
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

/* We cache lists of workgroups, lists of servers in workgroups, and lists
   of shares exported by servers. */

#define CACHE_TIMEOUT 30

struct name_list {
	struct name_list *prev, *next;
	char *name;
	uint32 stype;
	char *comment;
};

struct cached_names {
	struct cached_names *prev, *next;
	char *key;
	struct name_list *name_list;
	time_t cache_timeout;
	int result;
};

static struct cached_names *cached_names = NULL;

/* Find a list of cached name for a workgroup, server or share list */

static struct cached_names *find_cached_names(char *key)
{
	struct cached_names *tmp;

	for (tmp = cached_names; tmp; tmp = tmp->next) {
		if (strequal(tmp->key, key)) {
			return tmp;
		}
	}

	return NULL;
}

/* Add a name to a list stored in the state variable */

static void add_cached_names(const char *name, uint32 stype, 
			     const char *comment, void *state)
{
	struct name_list **name_list = (struct name_list **)state;
	struct name_list *new_name;

	new_name = SMB_MALLOC_P(struct name_list);
	if (!new_name) return;

	ZERO_STRUCTP(new_name);

	new_name->name = SMB_STRDUP(name);
	new_name->stype = stype;
	new_name->comment = SMB_STRDUP(comment);

	DLIST_ADD(*name_list, new_name);
}

static void free_name_list(struct name_list *name_list)
{
	struct name_list *tmp = name_list;

	while(tmp) {
		struct name_list *next;

		next = tmp->next;

		SAFE_FREE(tmp->name);
		SAFE_FREE(tmp->comment);
		SAFE_FREE(tmp);
		
		tmp = next;
	}
}

/* Wrapper for NetServerEnum function */

BOOL smbw_NetServerEnum(struct cli_state *cli, char *workgroup, uint32 stype,
			void (*fn)(const char *, uint32, const char *, void *),
			void *state)
{
	struct cached_names *names;
	struct name_list *tmp;
	time_t now = time(NULL);
	char key[PATH_MAX];
	BOOL result = True;

	slprintf(key, PATH_MAX - 1, "%s/%s#%s", cli->desthost, 
		 workgroup, (stype == SV_TYPE_DOMAIN_ENUM ? "DOM" : "SRV"));

	names = find_cached_names(key);

	if (names == NULL || (now - names->cache_timeout) > CACHE_TIMEOUT) {
		struct cached_names *new_names = NULL;

		/* No names cached for this workgroup */

		if (names == NULL) {
			new_names = SMB_MALLOC_P(struct cached_names);

			ZERO_STRUCTP(new_names);
			DLIST_ADD(cached_names, new_names);

		} else {

			/* Dispose of out of date name list */

			free_name_list(names->name_list);
			names->name_list = NULL;

			new_names = names;
		}		

		result = cli_NetServerEnum(cli, workgroup, stype, 
					   add_cached_names, 
					   &new_names->name_list);
					   
		new_names->cache_timeout = now;
		new_names->result = result;
		new_names->key = SMB_STRDUP(key);

		names = new_names;
	}

	/* Return names by running callback function. */

	for (tmp = names->name_list; tmp; tmp = tmp->next)
		fn(tmp->name, stype, tmp->comment, state);
	
	return names->result;
}

/* Wrapper for RNetShareEnum function */

int smbw_RNetShareEnum(struct cli_state *cli, 
		       void (*fn)(const char *, uint32, const char *, void *), 
		       void *state)
{
	struct cached_names *names;
	struct name_list *tmp;
	time_t now = time(NULL);
	char key[PATH_MAX];

	slprintf(key, PATH_MAX - 1, "SHARE/%s", cli->desthost);

	names = find_cached_names(key);

	if (names == NULL || (now - names->cache_timeout) > CACHE_TIMEOUT) {
		struct cached_names *new_names = NULL;

		/* No names cached for this server */

		if (names == NULL) {
			new_names = SMB_MALLOC_P(struct cached_names);

			ZERO_STRUCTP(new_names);
			DLIST_ADD(cached_names, new_names);

		} else {

			/* Dispose of out of date name list */

			free_name_list(names->name_list);
			names->name_list = NULL;

			new_names = names;
		}

		new_names->result = cli_RNetShareEnum(cli, add_cached_names, 
						      &new_names->name_list);
		
		new_names->cache_timeout = now;
		new_names->key = SMB_STRDUP(key);

		names = new_names;
	}

	/* Return names by running callback function. */

	for (tmp = names->name_list; tmp; tmp = tmp->next)
		fn(tmp->name, tmp->stype, tmp->comment, state);
	
	return names->result;
}
