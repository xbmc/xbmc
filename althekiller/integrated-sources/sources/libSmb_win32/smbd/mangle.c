/* 
   Unix SMB/CIFS implementation.
   Name mangling interface
   Copyright (C) Andrew Tridgell 2002
   
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

static struct mangle_fns *mangle_fns;

/* this allows us to add more mangling backends */
static const struct {
	const char *name;
	struct mangle_fns *(*init_fn)(void);
} mangle_backends[] = {
	{ "hash", mangle_hash_init },
	{ "hash2", mangle_hash2_init },
	{ "posix", posix_mangle_init },
	/*{ "tdb", mangle_tdb_init }, */
	{ NULL, NULL }
};

/*
  initialise the mangling subsystem
*/
static void mangle_init(void)
{
	int i;
	const char *method;

	if (mangle_fns)
		return;

	method = lp_mangling_method();

	/* find the first mangling method that manages to initialise and
	   matches the "mangling method" parameter */
	for (i=0; mangle_backends[i].name && !mangle_fns; i++) {
		if (!method || !*method || strcmp(method, mangle_backends[i].name) == 0) {
			mangle_fns = mangle_backends[i].init_fn();
		}
	}

	if (!mangle_fns) {
		DEBUG(0,("Failed to initialise mangling system '%s'\n", method));
		exit_server("mangling init failed");
	}
}


/*
  reset the cache. This is called when smb.conf has been reloaded
*/
void mangle_reset_cache(void)
{
	mangle_init();
	mangle_fns->reset();
}

void mangle_change_to_posix(void)
{
	mangle_fns = NULL;
	lp_set_mangling_method("posix");
	mangle_reset_cache();
}

/*
  see if a filename has come out of our mangling code
*/
BOOL mangle_is_mangled(const char *s, int snum)
{
	return mangle_fns->is_mangled(s, snum);
}

/*
  see if a filename matches the rules of a 8.3 filename
*/
BOOL mangle_is_8_3(const char *fname, BOOL check_case, int snum)
{
	return mangle_fns->is_8_3(fname, check_case, False, snum);
}

BOOL mangle_is_8_3_wildcards(const char *fname, BOOL check_case, int snum)
{
	return mangle_fns->is_8_3(fname, check_case, True, snum);
}

/*
  try to reverse map a 8.3 name to the original filename. This doesn't have to 
  always succeed, as the directory handling code in smbd will scan the directory
  looking for a matching name if it doesn't. It should succeed most of the time
  or there will be a huge performance penalty
*/
BOOL mangle_check_cache(char *s, size_t maxlen, int snum)
{
	return mangle_fns->check_cache(s, maxlen, snum);
}

/* 
   map a long filename to a 8.3 name. 
 */

void mangle_map(pstring OutName, BOOL need83, BOOL cache83, int snum)
{
	/* name mangling can be disabled for speed, in which case
	   we just truncate the string */
	if (!lp_manglednames(snum)) {
		if (need83) {
			string_truncate(OutName, 12);
		}
		return;
	}

	/* invoke the inane "mangled map" code */
	mangle_map_filename(OutName, snum);
	mangle_fns->name_map(OutName, need83, cache83, lp_defaultcase(snum), snum);
}
