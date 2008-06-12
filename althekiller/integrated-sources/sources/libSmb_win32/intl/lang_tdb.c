/* 
   Unix SMB/CIFS implementation.
   tdb based replacement for gettext 
   Copyright (C) Andrew Tridgell 2001
   
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

static TDB_CONTEXT *tdb;

/* the currently selected language */
static char *current_lang;


/* load a msg file into the tdb */
static BOOL load_msg(const char *msg_file)
{
	char **lines;
	int num_lines, i;
	char *msgid, *msgstr;
	TDB_DATA key, data;

	lines = file_lines_load(msg_file, &num_lines,0);

	if (!lines) {
		return False;
	}

	if (tdb_lockall(tdb) != 0) {
		file_lines_free(lines);
		return False;
	}

	/* wipe the db */
	tdb_traverse(tdb, tdb_traverse_delete_fn, NULL);

	msgid = NULL;
	
	for (i=0;i<num_lines;i++) {
		if (strncmp(lines[i], "msgid \"", 7) == 0) {
			msgid = lines[i] + 7;
		}
		if (msgid && strncmp(lines[i], "msgstr \"", 8) == 0) {
			msgstr = lines[i] + 8;
			trim_char(msgid, '\0', '\"');
			trim_char(msgstr, '\0', '\"');
			if (*msgstr == 0) {
				msgstr = msgid;
			}
			all_string_sub(msgid, "\\n", "\n", 0);
			all_string_sub(msgstr, "\\n", "\n", 0);
			key.dptr = msgid;
			key.dsize = strlen(msgid)+1;
			data.dptr = msgstr;
			data.dsize = strlen(msgstr)+1;
			tdb_store(tdb, key, data, 0);
			msgid = NULL;
		}
	}

	file_lines_free(lines);
	tdb_unlockall(tdb);

	return True;
}


/* work out what language to use from locale variables */
static const char *get_lang(void)
{
	const char *vars[] = {"LANGUAGE", "LC_ALL", "LC_LANG", "LANG", NULL};
	int i;
	char *p;

	for (i=0; vars[i]; i++) {
		if ((p = getenv(vars[i]))) {
			return p;
		}
	}

	return NULL;
}

/* initialise the message translation subsystem. If the "lang" argument
   is NULL then get the language from the normal environment variables */
BOOL lang_tdb_init(const char *lang)
{
	char *path = NULL;
	char *msg_path = NULL;
	struct stat st;
	static int initialised;
	time_t loadtime;
	BOOL result = False;

	/* we only want to init once per process, unless given
	   an override */
	if (initialised && !lang) 
		return True;

	if (initialised) {
		/* we are re-initialising, free up any old init */
		if (tdb) {
			tdb_close(tdb);
			tdb = NULL;
		}
		SAFE_FREE(current_lang);
	}

	initialised = 1;

	if (!lang) {
		/* no lang given, use environment */
		lang = get_lang();
	}

	/* if no lang then we don't translate */
	if (!lang) 
		return True;

	asprintf(&msg_path, "%s.msg", lib_path((const char *)lang));
	if (stat(msg_path, &st) != 0) {
		/* the msg file isn't available */
		DEBUG(10, ("lang_tdb_init: %s: %s\n", msg_path, 
			   strerror(errno)));
		goto done;
	}
	
	asprintf(&path, "%s%s.tdb", lock_path("lang_"), lang);

	DEBUG(10, ("lang_tdb_init: loading %s\n", path));

	tdb = tdb_open_log(path, 0, TDB_DEFAULT, O_RDWR|O_CREAT, 0644);
	if (!tdb) {
		tdb = tdb_open_log(path, 0, TDB_DEFAULT, O_RDONLY, 0);
		if (!tdb) {
			DEBUG(10, ("lang_tdb_init: %s: %s\n", path,
				   strerror(errno)));
			goto done;
		}
		current_lang = SMB_STRDUP(lang);
		result = True;
		goto done;
	}

	loadtime = tdb_fetch_int32(tdb, "/LOADTIME/");

	if (loadtime == -1 || loadtime < st.st_mtime) {
		load_msg(msg_path);
		tdb_store_int32(tdb, "/LOADTIME/", (int)time(NULL));
	}

	current_lang = SMB_STRDUP(lang);
	result = True;

 done:
	SAFE_FREE(msg_path);
	SAFE_FREE(path);

	return result;
}

/* translate a msgid to a message string in the current language 
   returns a string that must be freed by calling lang_msg_free()
*/
const char *lang_msg(const char *msgid)
{
	TDB_DATA key, data;
	const char *p;
	char *q, *msgid_quoted;
	int count;

	lang_tdb_init(NULL);

	if (!tdb) return msgid;

	/* Due to the way quotes in msgids are escaped in the msg file we
	   must replace " with \" before doing a lookup in the tdb. */

	count = 0;

	for(p = msgid; *p; p++) {
		if (*p == '\"')
			count++;
	}

	if (!(msgid_quoted = SMB_MALLOC(strlen(msgid) + count + 1)))
		return msgid;

	/* string_sub() is unsuitable here as it replaces some punctuation
	   chars with underscores. */

	for(p = msgid, q = msgid_quoted; *p; p++) {
		if (*p == '\"') {
			*q = '\\';
			q++;
		}
		*q = *p;
		q++;
	}

	*q = 0;

	key.dptr = (char *)msgid_quoted;
	key.dsize = strlen(msgid_quoted)+1;
	
	data = tdb_fetch(tdb, key);

	free(msgid_quoted);

	/* if the message isn't found then we still need to return a pointer
	   that can be freed. Pity. */
	if (!data.dptr)
		return SMB_STRDUP(msgid);

	return (const char *)data.dptr;
}


/* free up a string from lang_msg() */
void lang_msg_free(const char *msgstr)
{
	if (!tdb) return;
	free((void *)msgstr);
}


/*
  when the _() translation macro is used there is no obvious place to free
  the resulting string and there is no easy way to give a static pointer.
  All we can do is rotate between some static buffers and hope a single d_printf() 
  doesn't have more calls to _() than the number of buffers 
*/
const char *lang_msg_rotate(const char *msgid)
{
#define NUM_LANG_BUFS 16
	char *msgstr;
	static pstring bufs[NUM_LANG_BUFS];
	static int next;

	msgstr = (char *)lang_msg(msgid);
	if (!msgstr) return msgid;

	pstrcpy(bufs[next], msgstr);
	msgstr = bufs[next];

	next = (next+1) % NUM_LANG_BUFS;
	
	return msgstr;
}


/* 
   return the current language - needed for language file mappings 
*/
char *lang_tdb_current(void)
{
	return current_lang;
}
