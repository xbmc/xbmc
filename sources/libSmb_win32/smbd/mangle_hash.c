/* 
   Unix SMB/CIFS implementation.
   Name mangling
   Copyright (C) Andrew Tridgell 1992-2002
   Copyright (C) Simo Sorce 2001
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

/* -------------------------------------------------------------------------- **
 * Other stuff...
 *
 * magic_char     - This is the magic char used for mangling.  It's
 *                  global.  There is a call to lp_magicchar() in server.c
 *                  that is used to override the initial value.
 *
 * MANGLE_BASE    - This is the number of characters we use for name mangling.
 *
 * basechars      - The set characters used for name mangling.  This
 *                  is static (scope is this file only).
 *
 * mangle()       - Macro used to select a character from basechars (i.e.,
 *                  mangle(n) will return the nth digit, modulo MANGLE_BASE).
 *
 * chartest       - array 0..255.  The index range is the set of all possible
 *                  values of a byte.  For each byte value, the content is a
 *                  two nibble pair.  See BASECHAR_MASK below.
 *
 * ct_initialized - False until the chartest array has been initialized via
 *                  a call to init_chartest().
 *
 * BASECHAR_MASK  - Masks the upper nibble of a one-byte value.
 *
 * isbasecahr()   - Given a character, check the chartest array to see
 *                  if that character is in the basechars set.  This is
 *                  faster than using strchr_m().
 *
 */

char magic_char = '~';

static char basechars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_-!@#$%";
#define MANGLE_BASE       (sizeof(basechars)/sizeof(char)-1)

static unsigned char chartest[256]  = { 0 };
static BOOL          ct_initialized = False;

#define mangle(V) ((char)(basechars[(V) % MANGLE_BASE]))
#define BASECHAR_MASK 0xf0
#define isbasechar(C) ( (chartest[ ((C) & 0xff) ]) & BASECHAR_MASK )

static TDB_CONTEXT *tdb_mangled_cache;

/* -------------------------------------------------------------------- */

static NTSTATUS has_valid_83_chars(const smb_ucs2_t *s, BOOL allow_wildcards)
{
	if (!*s) {
		return NT_STATUS_INVALID_PARAMETER;
	}

	if (!allow_wildcards && ms_has_wild_w(s)) {
		return NT_STATUS_UNSUCCESSFUL;
	}

	while (*s) {
		if(!isvalid83_w(*s)) {
			return NT_STATUS_UNSUCCESSFUL;
		}
		s++;
	}

	return NT_STATUS_OK;
}

static NTSTATUS has_illegal_chars(const smb_ucs2_t *s, BOOL allow_wildcards)
{
	if (!allow_wildcards && ms_has_wild_w(s)) {
		return NT_STATUS_UNSUCCESSFUL;
	}

	while (*s) {
		if (*s <= 0x1f) {
			/* Control characters. */
			return NT_STATUS_UNSUCCESSFUL;
		}
		switch(*s) {
			case UCS2_CHAR('\\'):
			case UCS2_CHAR('/'):
			case UCS2_CHAR('|'):
			case UCS2_CHAR(':'):
				return NT_STATUS_UNSUCCESSFUL;
		}
		s++;
	}

	return NT_STATUS_OK;
}

/* return False if something fail and
 * return 2 alloced unicode strings that contain prefix and extension
 */

static NTSTATUS mangle_get_prefix(const smb_ucs2_t *ucs2_string, smb_ucs2_t **prefix,
		smb_ucs2_t **extension, BOOL allow_wildcards)
{
	size_t ext_len;
	smb_ucs2_t *p;

	*extension = 0;
	*prefix = strdup_w(ucs2_string);
	if (!*prefix) {
		return NT_STATUS_NO_MEMORY;
	}
	if ((p = strrchr_w(*prefix, UCS2_CHAR('.')))) {
		ext_len = strlen_w(p+1);
		if ((ext_len > 0) && (ext_len < 4) && (p != *prefix) &&
		    (NT_STATUS_IS_OK(has_valid_83_chars(p+1,allow_wildcards)))) /* check extension */ {
			*p = 0;
			*extension = strdup_w(p+1);
			if (!*extension) {
				SAFE_FREE(*prefix);
				return NT_STATUS_NO_MEMORY;
			}
		}
	}
	return NT_STATUS_OK;
}

/* ************************************************************************** **
 * Return NT_STATUS_UNSUCCESSFUL if a name is a special msdos reserved name.
 * or contains illegal characters.
 *
 *  Input:  fname - String containing the name to be tested.
 *
 *  Output: NT_STATUS_UNSUCCESSFUL, if the condition above is true.
 *
 *  Notes:  This is a static function called by is_8_3(), below.
 *
 * ************************************************************************** **
 */

static NTSTATUS is_valid_name(const smb_ucs2_t *fname, BOOL allow_wildcards, BOOL only_8_3)
{
	smb_ucs2_t *str, *p;
	size_t num_ucs2_chars;
	NTSTATUS ret = NT_STATUS_OK;

	if (!fname || !*fname)
		return NT_STATUS_INVALID_PARAMETER;

	/* . and .. are valid names. */
	if (strcmp_wa(fname, ".")==0 || strcmp_wa(fname, "..")==0)
		return NT_STATUS_OK;

	if (only_8_3) {
		ret = has_valid_83_chars(fname, allow_wildcards);
		if (!NT_STATUS_IS_OK(ret))
			return ret;
	}

	ret = has_illegal_chars(fname, allow_wildcards);
	if (!NT_STATUS_IS_OK(ret))
		return ret;

	/* Name can't end in '.' or ' ' */
	num_ucs2_chars = strlen_w(fname);
	if (fname[num_ucs2_chars-1] == UCS2_CHAR('.') || fname[num_ucs2_chars-1] == UCS2_CHAR(' ')) {
		return NT_STATUS_UNSUCCESSFUL;
	}

	str = strdup_w(fname);

	/* Truncate copy after the first dot. */
	p = strchr_w(str, UCS2_CHAR('.'));
	if (p) {
		*p = 0;
	}

	strupper_w(str);
	p = &str[1];

	switch(str[0])
	{
	case UCS2_CHAR('A'):
		if(strcmp_wa(p, "UX") == 0)
			ret = NT_STATUS_UNSUCCESSFUL;
		break;
	case UCS2_CHAR('C'):
		if((strcmp_wa(p, "LOCK$") == 0)
		|| (strcmp_wa(p, "ON") == 0)
		|| (strcmp_wa(p, "OM1") == 0)
		|| (strcmp_wa(p, "OM2") == 0)
		|| (strcmp_wa(p, "OM3") == 0)
		|| (strcmp_wa(p, "OM4") == 0)
		)
			ret = NT_STATUS_UNSUCCESSFUL;
		break;
	case UCS2_CHAR('L'):
		if((strcmp_wa(p, "PT1") == 0)
		|| (strcmp_wa(p, "PT2") == 0)
		|| (strcmp_wa(p, "PT3") == 0)
		)
			ret = NT_STATUS_UNSUCCESSFUL;
		break;
	case UCS2_CHAR('N'):
		if(strcmp_wa(p, "UL") == 0)
			ret = NT_STATUS_UNSUCCESSFUL;
		break;
	case UCS2_CHAR('P'):
		if(strcmp_wa(p, "RN") == 0)
			ret = NT_STATUS_UNSUCCESSFUL;
		break;
	default:
		break;
	}

	SAFE_FREE(str);
	return ret;
}

static NTSTATUS is_8_3_w(const smb_ucs2_t *fname, BOOL allow_wildcards)
{
	smb_ucs2_t *pref = 0, *ext = 0;
	size_t plen;
	NTSTATUS ret = NT_STATUS_UNSUCCESSFUL;

	if (!fname || !*fname)
		return NT_STATUS_INVALID_PARAMETER;

	if (strlen_w(fname) > 12)
		return NT_STATUS_UNSUCCESSFUL;
	
	if (strcmp_wa(fname, ".") == 0 || strcmp_wa(fname, "..") == 0)
		return NT_STATUS_OK;

	/* Name cannot start with '.' */
	if (*fname == UCS2_CHAR('.'))
		return NT_STATUS_UNSUCCESSFUL;
	
	if (!NT_STATUS_IS_OK(is_valid_name(fname, allow_wildcards, True)))
		goto done;

	if (!NT_STATUS_IS_OK(mangle_get_prefix(fname, &pref, &ext, allow_wildcards)))
		goto done;
	plen = strlen_w(pref);

	if (strchr_wa(pref, '.'))
		goto done;
	if (plen < 1 || plen > 8)
		goto done;
	if (ext && (strlen_w(ext) > 3))
		goto done;

	ret = NT_STATUS_OK;

done:
	SAFE_FREE(pref);
	SAFE_FREE(ext);
	return ret;
}

static BOOL is_8_3(const char *fname, BOOL check_case, BOOL allow_wildcards, int snum)
{
	const char *f;
	smb_ucs2_t *ucs2name;
	NTSTATUS ret = NT_STATUS_UNSUCCESSFUL;
	size_t size;

	magic_char = lp_magicchar(snum);

	if (!fname || !*fname)
		return False;
	if ((f = strrchr(fname, '/')) == NULL)
		f = fname;
	else
		f++;

	if (strlen(f) > 12)
		return False;
	
	size = push_ucs2_allocate(&ucs2name, f);
	if (size == (size_t)-1) {
		DEBUG(0,("is_8_3: internal error push_ucs2_allocate() failed!\n"));
		goto done;
	}

	ret = is_8_3_w(ucs2name, allow_wildcards);

done:
	SAFE_FREE(ucs2name);

	if (!NT_STATUS_IS_OK(ret)) { 
		return False;
	}
	
	return True;
}



/* -------------------------------------------------------------------------- **
 * Functions...
 */

/* ************************************************************************** **
 * Initialize the static character test array.
 *
 *  Input:  none
 *
 *  Output: none
 *
 *  Notes:  This function changes (loads) the contents of the <chartest>
 *          array.  The scope of <chartest> is this file.
 *
 * ************************************************************************** **
 */
static void init_chartest( void )
{
	const unsigned char *s;
  
	memset( (char *)chartest, '\0', 256 );

	for( s = (const unsigned char *)basechars; *s; s++ ) {
		chartest[*s] |= BASECHAR_MASK;
	}

	ct_initialized = True;
}

/* ************************************************************************** **
 * Return True if the name *could be* a mangled name.
 *
 *  Input:  s - A path name - in UNIX pathname format.
 *
 *  Output: True if the name matches the pattern described below in the
 *          notes, else False.
 *
 *  Notes:  The input name is *not* tested for 8.3 compliance.  This must be
 *          done separately.  This function returns true if the name contains
 *          a magic character followed by excactly two characters from the
 *          basechars list (above), which in turn are followed either by the
 *          nul (end of string) byte or a dot (extension) or by a '/' (end of
 *          a directory name).
 *
 * ************************************************************************** **
 */
static BOOL is_mangled(const char *s, int snum)
{
	char *magic;

	magic_char = lp_magicchar(snum);

	if( !ct_initialized )
		init_chartest();

	magic = strchr_m( s, magic_char );
	while( magic && magic[1] && magic[2] ) {         /* 3 chars, 1st is magic. */
		if( ('.' == magic[3] || '/' == magic[3] || !(magic[3]))          /* Ends with '.' or nul or '/' ?  */
				&& isbasechar( toupper_ascii(magic[1]) )           /* is 2nd char basechar?  */
				&& isbasechar( toupper_ascii(magic[2]) ) )         /* is 3rd char basechar?  */
			return( True );                           /* If all above, then true, */
		magic = strchr_m( magic+1, magic_char );      /*    else seek next magic. */
	}
	return( False );
}

/***************************************************************************
 Initializes or clears the mangled cache.
***************************************************************************/

static void mangle_reset( void )
{
	/* We could close and re-open the tdb here... should we ? The old code did
	   the equivalent... JRA. */
}

/***************************************************************************
 Add a mangled name into the cache.
 If the extension of the raw name maps directly to the
 extension of the mangled name, then we'll store both names
 *without* extensions.  That way, we can provide consistent
 reverse mangling for all names that match.  The test here is
 a bit more careful than the one done in earlier versions of
 mangle.c:

    - the extension must exist on the raw name,
    - it must be all lower case
    - it must match the mangled extension (to prove that no
      mangling occurred).
  crh 07-Apr-1998
**************************************************************************/

static void cache_mangled_name( const char mangled_name[13], char *raw_name )
{
	TDB_DATA data_val;
	char mangled_name_key[13];
	char *s1;
	char *s2;

	/* If the cache isn't initialized, give up. */
	if( !tdb_mangled_cache )
		return;

	/* Init the string lengths. */
	safe_strcpy(mangled_name_key, mangled_name, sizeof(mangled_name_key)-1);

	/* See if the extensions are unmangled.  If so, store the entry
	 * without the extension, thus creating a "group" reverse map.
	 */
	s1 = strrchr( mangled_name_key, '.' );
	if( s1 && (s2 = strrchr( raw_name, '.' )) ) {
		size_t i = 1;
		while( s1[i] && (tolower_ascii( s1[i] ) == s2[i]) )
			i++;
		if( !s1[i] && !s2[i] ) {
			/* Truncate at the '.' */
			*s1 = '\0';
			*s2 = '\0';
		}
	}

	/* Allocate a new cache entry.  If the allocation fails, just return. */
	data_val.dptr = raw_name;
	data_val.dsize = strlen(raw_name)+1;
	if (tdb_store_bystring(tdb_mangled_cache, mangled_name_key, data_val, TDB_REPLACE) != 0) {
		DEBUG(0,("cache_mangled_name: Error storing entry %s -> %s\n", mangled_name_key, raw_name));
	} else {
		DEBUG(5,("cache_mangled_name: Stored entry %s -> %s\n", mangled_name_key, raw_name));
	}
}

/* ************************************************************************** **
 * Check for a name on the mangled name stack
 *
 *  Input:  s - Input *and* output string buffer.
 *	    maxlen - space in i/o string buffer.
 *  Output: True if the name was found in the cache, else False.
 *
 *  Notes:  If a reverse map is found, the function will overwrite the string
 *          space indicated by the input pointer <s>.  This is frightening.
 *          It should be rewritten to return NULL if the long name was not
 *          found, and a pointer to the long name if it was found.
 *
 * ************************************************************************** **
 */

static BOOL check_cache( char *s, size_t maxlen, int snum )
{
	TDB_DATA data_val;
	char *ext_start = NULL;
	char *saved_ext = NULL;

	magic_char = lp_magicchar(snum);

	/* If the cache isn't initialized, give up. */
	if( !tdb_mangled_cache )
		return( False );

	data_val = tdb_fetch_bystring(tdb_mangled_cache, s);

	/* If we didn't find the name *with* the extension, try without. */
	if(data_val.dptr == NULL || data_val.dsize == 0) {
		ext_start = strrchr( s, '.' );
		if( ext_start ) {
			if((saved_ext = SMB_STRDUP(ext_start)) == NULL)
				return False;

			*ext_start = '\0';
			data_val = tdb_fetch_bystring(tdb_mangled_cache, s);
			/* 
			 * At this point s is the name without the
			 * extension. We re-add the extension if saved_ext
			 * is not null, before freeing saved_ext.
			 */
		}
	}

	/* Okay, if we haven't found it we're done. */
	if(data_val.dptr == NULL || data_val.dsize == 0) {
		if(saved_ext) {
			/* Replace the saved_ext as it was truncated. */
			(void)safe_strcat( s, saved_ext, maxlen );
			SAFE_FREE(saved_ext);
		}
		return( False );
	}

	/* If we *did* find it, we need to copy it into the string buffer. */
	(void)safe_strcpy( s, data_val.dptr, maxlen );
	if( saved_ext ) {
		/* Replace the saved_ext as it was truncated. */
		(void)safe_strcat( s, saved_ext, maxlen );
		SAFE_FREE(saved_ext);
	}
	SAFE_FREE(data_val.dptr);
	return( True );
}

/*****************************************************************************
 * do the actual mangling to 8.3 format
 * the buffer must be able to hold 13 characters (including the null)
 *****************************************************************************
 */
static void to_8_3(char *s, int default_case)
{
	int csum;
	char *p;
	char extension[4];
	char base[9];
	int baselen = 0;
	int extlen = 0;

	extension[0] = 0;
	base[0] = 0;

	p = strrchr(s,'.');  
	if( p && (strlen(p+1) < (size_t)4) ) {
		BOOL all_normal = ( strisnormal(p+1, default_case) ); /* XXXXXXXXX */

		if( all_normal && p[1] != 0 ) {
			*p = 0;
			csum = str_checksum( s );
			*p = '.';
		} else
			csum = str_checksum(s);
	} else
		csum = str_checksum(s);

	strupper_m( s );

	if( p ) {
		if( p == s )
			safe_strcpy( extension, "___", 3 );
		else {
			*p++ = 0;
			while( *p && extlen < 3 ) {
				if ( *p != '.') {
					extension[extlen++] = p[0];
				}
				p++;
			}
			extension[extlen] = 0;
		}
	}
  
	p = s;

	while( *p && baselen < 5 ) {
		if (isbasechar(*p)) {
			base[baselen++] = p[0];
		}
		p++;
	}
	base[baselen] = 0;
  
	csum = csum % (MANGLE_BASE*MANGLE_BASE);
  
	(void)slprintf(s, 12, "%s%c%c%c",
		base, magic_char, mangle( csum/MANGLE_BASE ), mangle( csum ) );
  
	if( *extension ) {
		(void)pstrcat( s, "." );
		(void)pstrcat( s, extension );
	}
}

/*****************************************************************************
 * Convert a filename to DOS format.  Return True if successful.
 *
 *  Input:  OutName - Source *and* destination buffer. 
 *
 *                    NOTE that OutName must point to a memory space that
 *                    is at least 13 bytes in size!
 *
 *          need83  - If False, name mangling will be skipped unless the
 *                    name contains illegal characters.  Mapping will still
 *                    be done, if appropriate.  This is probably used to
 *                    signal that a client does not require name mangling,
 *                    thus skipping the name mangling even on shares which
 *                    have name-mangling turned on.
 *          cache83 - If False, the mangled name cache will not be updated.
 *                    This is usually used to prevent that we overwrite
 *                    a conflicting cache entry prematurely, i.e. before
 *                    we know whether the client is really interested in the
 *                    current name.  (See PR#13758).  UKD.
 *
 *  Output: Returns False only if the name wanted mangling but the share does
 *          not have name mangling turned on.
 *
 * ****************************************************************************
 */

static void name_map(char *OutName, BOOL need83, BOOL cache83, int default_case, int snum)
{
	smb_ucs2_t *OutName_ucs2;
	magic_char = lp_magicchar(snum);

	DEBUG(5,("name_map( %s, need83 = %s, cache83 = %s)\n", OutName,
		 need83 ? "True" : "False", cache83 ? "True" : "False"));
	
	if (push_ucs2_allocate(&OutName_ucs2, OutName) == (size_t)-1) {
		DEBUG(0, ("push_ucs2_allocate failed!\n"));
		return;
	}

	if( !need83 && !NT_STATUS_IS_OK(is_valid_name(OutName_ucs2, False, False)))
		need83 = True;

	/* check if it's already in 8.3 format */
	if (need83 && !NT_STATUS_IS_OK(is_8_3_w(OutName_ucs2, False))) {
		char *tmp = NULL; 

		/* mangle it into 8.3 */
		if (cache83)
			tmp = SMB_STRDUP(OutName);

		to_8_3(OutName, default_case);

		if(tmp != NULL) {
			cache_mangled_name(OutName, tmp);
			SAFE_FREE(tmp);
		}
	}

	DEBUG(5,("name_map() ==> [%s]\n", OutName));
	SAFE_FREE(OutName_ucs2);
}

/*
  the following provides the abstraction layer to make it easier
  to drop in an alternative mangling implementation
*/
static struct mangle_fns mangle_fns = {
	mangle_reset,
	is_mangled,
	is_8_3,
	check_cache,
	name_map
};

/* return the methods for this mangling implementation */
struct mangle_fns *mangle_hash_init(void)
{
	mangle_reset();

	/* Create the in-memory tdb using our custom hash function. */
	tdb_mangled_cache = tdb_open_ex("mangled_cache", 1031, TDB_INTERNAL,
				(O_RDWR|O_CREAT), 0644, NULL, fast_string_hash);

	return &mangle_fns;
}
