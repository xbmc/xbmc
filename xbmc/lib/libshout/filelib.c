/* filelib.c
 * library routines for file operations
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "compat.h"
#include "filelib.h"
#include "mchar.h"
#include "debug.h"
#include <assert.h>
#include <sys/types.h>
#include "uce_dirent.h"

#define TEMP_STR_LEN	(SR_MAX_PATH*2)

/*****************************************************************************
 * Public functions
 *****************************************************************************/
error_code filelib_write_track(char *buf, u_long size);
error_code filelib_write_show(char *buf, u_long size);
void filelib_shutdown();
error_code filelib_remove(char *filename);


/*****************************************************************************
 * Private Functions
 *****************************************************************************/
static error_code device_split (mchar* dirname, mchar* device, mchar* path);
static error_code mkdir_if_needed (mchar *str);
static error_code mkdir_recursive (mchar *str, int make_last);
static void close_file (FHANDLE* fp);
static void close_files ();
static error_code filelib_write (FHANDLE fp, char *buf, u_long size);
static BOOL file_exists (mchar *filename);
static void trim_filename (mchar* out, mchar *filename);
static void trim_mp3_suffix (mchar *filename);
static error_code filelib_open_for_write (FHANDLE* fp, mchar *filename);
static void
parse_and_subst_dir (mchar* pattern_head, mchar* pattern_tail, 
		     mchar* opat_path, int is_for_showfile);
static void
parse_and_subst_pat (mchar* newfile,
		     TRACK_INFO* ti,
		     mchar* directory,
		     mchar* pattern,
		     mchar* extension);
static void set_default_pattern (BOOL get_separate_dirs, BOOL do_count);
static error_code 
set_output_directory (mchar* global_output_directory,
		      mchar* global_output_pattern,
		      mchar* output_pattern,
		      mchar* output_directory,
		      mchar* default_pattern,
		      mchar* default_pattern_tail,
		      int get_separate_dirs,
		      int get_date_stamp,
		      int is_for_showfile
		      );
static error_code sr_getcwd (mchar* dirbuf);
static error_code add_trailing_slash (mchar *str);
static int get_next_sequence_number (mchar* fn_base);
static void fill_date_buf (mchar* datebuf, int datebuf_len);
static error_code filelib_open_showfiles ();
static void move_file (mchar* new_filename, mchar* old_filename);
static mchar* replace_invalid_chars (mchar *str);

/*****************************************************************************
 * Private Vars
 *****************************************************************************/
#define DATEBUF_LEN 50
static FHANDLE 	m_file;
static FHANDLE 	m_show_file;
static FHANDLE  m_cue_file;
static int 	m_count;
static int      m_do_show;
static mchar 	m_default_pattern[SR_MAX_PATH];
static mchar 	m_default_showfile_pattern[SR_MAX_PATH];
static mchar 	m_output_directory[SR_MAX_PATH];
static mchar 	m_output_pattern[SR_MAX_PATH];
static mchar 	m_incomplete_directory[SR_MAX_PATH];
static mchar    m_incomplete_filename[SR_MAX_PATH];
static mchar 	m_showfile_directory[SR_MAX_PATH];
static mchar    m_showfile_pattern[SR_MAX_PATH];
static BOOL	m_keep_incomplete = TRUE;
static int      m_max_filename_length;
static mchar 	m_show_name[SR_MAX_PATH];
static mchar 	m_cue_name[SR_MAX_PATH];
static mchar 	m_icy_name[SR_MAX_PATH];
static mchar*	m_extension;
static BOOL	m_do_individual_tracks;
static mchar    m_session_datebuf[DATEBUF_LEN];
static mchar    m_stripped_icy_name[SR_MAX_PATH];

// For now we're not going to care. If it makes it good. it not, will know 
// When we try to create a file in the path.
static error_code
mkdir_if_needed (mchar *str)
{
    char s[SR_MAX_PATH];
    string_from_mstring (s, SR_MAX_PATH, str, CODESET_FILESYS);
#if WIN32
    mkdir (s);
#else
    mkdir (s, 0777);
#endif
    return SR_SUCCESS;
}

/* Recursively make directories.  If make_last == 1, then the final 
   substring (after the last '/') is considered a directory rather 
   than a file name */
#ifndef XBMC
static error_code
mkdir_recursive (mchar *str, int make_last)
{
    mchar buf[SR_MAX_PATH];
    mchar* p = buf;
    mchar q;

    buf[0] = 0;
    while ((q = *p++ = *str++) != 0) {
	if (ISSLASH(q)) {
	    *p = 0;
	    mkdir_if_needed (buf);
	}
    }
    if (make_last) {
	mkdir_if_needed (str);
    }
    return SR_SUCCESS;
}
#endif

error_code
filelib_init (BOOL do_individual_tracks,
	      BOOL do_count,
	      int count_start,
	      BOOL keep_incomplete,
	      BOOL do_show_file,
	      int content_type,
	      char* output_directory,  /* Locale encoded - from command line */
	      char* output_pattern,    /* Locale encoded - from command line */
	      char* showfile_pattern,  /* Locale encoded - from command line */
	      int get_separate_dirs,
	      int get_date_stamp,
	      char* icy_name)
{
#ifndef XBMC
    mchar tmp_output_directory[SR_MAX_PATH];
    mchar tmp_output_pattern[SR_MAX_PATH];
    mchar tmp_showfile_pattern[SR_MAX_PATH];

    m_file = INVALID_FHANDLE;
    m_show_file = INVALID_FHANDLE;
    m_cue_file = INVALID_FHANDLE;
    m_count = do_count ? count_start : -1;
    m_keep_incomplete = keep_incomplete;
    memset(&m_output_directory, 0, SR_MAX_PATH);
    m_show_name[0] = 0;
    m_do_show = do_show_file;
    m_do_individual_tracks = do_individual_tracks;

    debug_printf ("FILELIB_INIT: output_directory=%s\n",
		  output_directory ? output_directory : "");
    debug_printf ("FILELIB_INIT: output_pattern=%s\n",
		  output_pattern ? output_pattern : "");
    debug_printf ("FILELIB_INIT: showfile_pattern=%s\n",
		  showfile_pattern ? showfile_pattern : "");

    mstring_from_string (tmp_output_directory, SR_MAX_PATH, output_directory, 
			 CODESET_LOCALE);
    mstring_from_string (tmp_output_pattern, SR_MAX_PATH, output_pattern, 
			 CODESET_LOCALE);
    mstring_from_string (tmp_showfile_pattern, SR_MAX_PATH, showfile_pattern, 
			 CODESET_LOCALE);
    mstring_from_string (m_icy_name, SR_MAX_PATH, icy_name, 
			 CODESET_METADATA);
    debug_printf ("Converted output directory: len=%d\n", 
		  mstrlen (tmp_output_directory));
    mstrcpy (m_stripped_icy_name, m_icy_name);
    
    replace_invalid_chars (m_stripped_icy_name);

    switch (content_type) {
    case CONTENT_TYPE_MP3:
	m_extension = m_(".mp3");
	break;
    case CONTENT_TYPE_NSV:
    case CONTENT_TYPE_ULTRAVOX:
	m_extension = m_(".nsv");
	break;
    case CONTENT_TYPE_OGG:
	m_extension = m_(".ogg");
	break;
    case CONTENT_TYPE_AAC:
	m_extension = m_(".aac");
	break;
    default:
	fprintf (stderr, "Error (wrong suggested content type: %d)\n", 
		 content_type);
	return SR_ERROR_PROGRAM_ERROR;
    }

    /* Initialize session date */
    fill_date_buf (m_session_datebuf, DATEBUF_LEN);

    /* Set up the proper pattern if we're using -q and -s flags */
    set_default_pattern (get_separate_dirs, do_count);

    /* Get the path to the "parent" directory.  This is the directory
       that contains the incomplete dir and the show files.
       It might not contain the complete files if an output_pattern
       was specified. */
    set_output_directory (m_output_directory,
			  m_output_pattern,
			  tmp_output_pattern,
			  tmp_output_directory,
			  m_default_pattern,
			  m_("%A - %T"),
			  get_separate_dirs,
			  get_date_stamp,
			  0);

    msnprintf (m_incomplete_directory, SR_MAX_PATH, m_S m_S m_C, 
	       m_output_directory, m_("incomplete"), PATH_SLASH);

    /* Recursively make the output directory & incomplete directory */
    if (m_do_individual_tracks) {
	debug_mprintf (m_("Trying to make output_directory: ") m_S m_("\n"), 
		       m_output_directory);
	mkdir_recursive (m_output_directory, 1);

	/* Next, make the incomplete directory */
	if (m_do_individual_tracks) {
	    debug_mprintf (m_("Trying to make incomplete_directory: ") 
			   m_S m_("\n"), m_incomplete_directory);
	    mkdir_if_needed (m_incomplete_directory);
	}
    }

    /* Compute the amount of remaining path length for the filenames */
    m_max_filename_length = SR_MAX_PATH - mstrlen(m_incomplete_directory);

    /* Get directory and pattern of showfile */
    if (do_show_file) {
	if (*tmp_showfile_pattern) {
	    trim_mp3_suffix (tmp_showfile_pattern);
	    if (mstrlen(m_show_name) > SR_MAX_PATH - 5) {
		return SR_ERROR_DIR_PATH_TOO_LONG;
	    }
	}
	set_output_directory (m_showfile_directory,
			      m_showfile_pattern,
			      tmp_showfile_pattern,
			      tmp_output_directory,
			      m_default_showfile_pattern,
			      m_(""),
			      get_separate_dirs,
			      get_date_stamp,
			      1);
	mkdir_recursive (m_showfile_directory, 1);
	filelib_open_showfiles ();
    }
#endif

    return SR_SUCCESS;
}

/* This sets the value for m_default_pattern and m_default_showfile_pattern,
   using the -q & -s flags.  This function cannot overflow 
   these static buffers. */
#ifndef XBMC
static void
set_default_pattern (BOOL get_separate_dirs, BOOL do_count)
{
    /* m_default_pattern */
    m_default_pattern[0] = 0;
    if (get_separate_dirs) {
	mstrcpy (m_default_pattern, m_("%S") PATH_SLASH_STR);
    }
    if (do_count) {
	if (m_count < 0) {
	    mstrncat (m_default_pattern, m_("%q_"), SR_MAX_PATH);
	} else {
	    msnprintf (&m_default_pattern[mstrlen(m_default_pattern)], 
		       SR_MAX_PATH - mstrlen(m_default_pattern), 
		       m_("%%%dq_"), m_count);
	}
    }
    mstrncat (m_default_pattern, m_("%A - %T"), SR_MAX_PATH);

    /* m_default_showfile_pattern */
    m_default_showfile_pattern[0] = 0;
    if (get_separate_dirs) {
	mstrcpy (m_default_showfile_pattern, m_("%S") PATH_SLASH_STR);
    }
    mstrncat (m_default_showfile_pattern, m_("sr_program_%d"), SR_MAX_PATH);
}

/* This function sets the value of m_output_directory or 
   m_showfile_directory. */
static error_code 
set_output_directory (mchar* global_output_directory,
		      mchar* global_output_pattern,
		      mchar* output_pattern,
		      mchar* output_directory,
		      mchar* default_pattern,
		      mchar* default_pattern_tail,
		      int get_separate_dirs,
		      int get_date_stamp,
		      int is_for_showfile
		      )
{
    error_code ret;
    mchar opat_device[3];
    mchar odir_device[3];
    mchar cwd_device[3];
    mchar* device;
    mchar opat_path[SR_MAX_PATH];
    mchar odir_path[SR_MAX_PATH];
    mchar cwd_path[SR_MAX_PATH];
    mchar cwd[SR_MAX_PATH];

    mchar pattern_head[SR_MAX_PATH];
    mchar pattern_tail[SR_MAX_PATH];

    /* Initialize strings */
    cwd[0] = 0;
    odir_device[0] = 0;
    opat_device[0] = 0;
    odir_path[0] = 0;
    opat_path[0] = 0;
    ret = sr_getcwd (cwd);
    if (ret != SR_SUCCESS) return ret;

    if (!output_pattern || !(*output_pattern)) {
	output_pattern = default_pattern;
    }

    /* Get the device. It can be empty. */
    if (output_directory && *output_directory) {
	device_split (output_directory, odir_device, odir_path);
	debug_printf ("devicesplit: %d -> %d %d\n", 
		      mstrlen (output_directory), 
		      mstrlen (odir_device),
		      mstrlen (odir_path));
    }
    device_split (output_pattern, opat_device, opat_path);
    device_split (cwd, cwd_device, cwd_path);
    if (*opat_device) {
	device = opat_device;
    } else if (*odir_device) {
	device = odir_device;
    } else {
	/* No device */
	device = m_("");
    }

    /* Generate the output file pattern. */
    if (IS_ABSOLUTE_PATH(opat_path)) {
	cwd_path[0] = 0;
	odir_path[0] = 0;
	debug_printf ("Got opat_path absolute path\n");
    } else if (IS_ABSOLUTE_PATH(odir_path)) {
	cwd_path[0] = 0;
	debug_printf ("Got odir_path absolute path\n");
    }
    if (*odir_path) {
	ret = add_trailing_slash(odir_path);
	if (ret != SR_SUCCESS) return ret;
    }
    if (*cwd_path) {
	ret = add_trailing_slash(cwd_path);
	if (ret != SR_SUCCESS) return ret;
    }
    if (mstrlen(device) + mstrlen(cwd_path) + mstrlen(opat_path) 
	+ mstrlen(odir_path) > SR_MAX_PATH-1) {
	return SR_ERROR_DIR_PATH_TOO_LONG;
    }

    /* Fill in %S and %d patterns */
    msnprintf (pattern_head, SR_MAX_PATH, m_S m_S m_S, device, 
	       cwd_path, odir_path);
    debug_printf ("Composed pattern head (%d) <- (%d,%d,%d)\n",
		  mstrlen(pattern_head), mstrlen(device), 
		  mstrlen(cwd_path), mstrlen(odir_path));
    parse_and_subst_dir (pattern_head, pattern_tail, opat_path, 
			 is_for_showfile);

    /* In case there is no %A, no %T, etc., use the default pattern */
    if (!*pattern_tail) {
	mstrcpy (pattern_tail, default_pattern_tail);
    }

    /* Set the global variables */
    mstrcpy (global_output_directory, pattern_head);
    add_trailing_slash (global_output_directory);
    mstrcpy (global_output_pattern, pattern_tail);

    return SR_SUCCESS;
}
#endif

/* Parse & substitute the output pattern.  What we're trying to
   get is everything up to the pattern specifiers that change 
   from track to track: %A, %T, %a, %D, %q, or %Q. 
   If %S or %d appear before this, substitute in. 
   If it's for the showfile, then we don't advance pattern_head 
   If there is no %A, no %T, etc.
*/
static void
parse_and_subst_dir (mchar* pattern_head, mchar* pattern_tail, 
		     mchar* opat_path, int is_for_showfile)
{
    int opi = 0;
    unsigned int phi = 0;
    int ph_base_len;
    int op_tail_idx;

    phi = mstrlen(pattern_head);
    opi = 0;
    ph_base_len = phi;
    op_tail_idx = opi;

    while (phi < SR_MAX_BASE) {
	if (ISSLASH(opat_path[opi])) {
	    pattern_head[phi++] = PATH_SLASH;
	    opi++;
	    ph_base_len = phi;
	    op_tail_idx = opi;
	    continue;
	}
	if (opat_path[opi] == 0) {
	    /* This means there are no artist/title info in the filename.
	       In this case, we fall back on the default pattern. */
	    if (!is_for_showfile) {
		ph_base_len = phi;
		op_tail_idx = opi;
	    }
	    break;
	}
	if (opat_path[opi] != m_('%')) {
	    pattern_head[phi++] = opat_path[opi++];
	    continue;
	}
	/* If we got here, we have a '%' */
	switch (opat_path[opi+1]) {
	case m_('%'):
	    pattern_head[phi++]=m_('%');
	    opi+=2;
	    continue;
	case m_('S'):
	    /* append stream name */
	    mstrncpy (&pattern_head[phi], m_stripped_icy_name, 
		      SR_MAX_BASE-phi);
	    phi = mstrlen (pattern_head);
	    opi+=2;
	    continue;
	case m_('d'):
	    /* append date info */
	    mstrncpy (&pattern_head[phi], m_session_datebuf, SR_MAX_BASE-phi);
	    phi = mstrlen (pattern_head);
	    opi+=2;
	    continue;
	case m_('0'): case m_('1'): case m_('2'): case m_('3'): case m_('4'): 
	case m_('5'): case m_('6'): case m_('7'): case m_('8'): case m_('9'): 
	case m_('a'):
	case m_('A'):
	case m_('D'):
	case m_('q'):
	case m_('T'):
	    /* These are track specific patterns */
	    break;
	case 0:
	    /* This means there are no artist/title info in the filename.
	       In this case, we fall back on the default pattern. */
	    pattern_head[phi++] = opat_path[opi++];
	    if (!is_for_showfile) {
		ph_base_len = phi;
		op_tail_idx = opi;
	    }
	    break;
	default:
	    /* This is an illegal pattern, so copy the '%' and continue */
	    pattern_head[phi++] = opat_path[opi++];
	    continue;
	}
	/* If we got to here, it means that we hit something like %A or %T */
	break;
    }
    /* Terminate the pattern_head string */
    pattern_head[ph_base_len] = 0;

    mstrcpy (pattern_tail, &opat_path[op_tail_idx]);
}

static void
fill_date_buf (mchar* datebuf, int datebuf_len)
{
    char tmp[DATEBUF_LEN];
    time_t now = time(NULL);
    strftime (tmp, datebuf_len, "%Y_%m_%d_%H_%M_%S", localtime(&now));
    mstring_from_string (datebuf, DATEBUF_LEN, tmp, CODESET_FILESYS);
}

static error_code
add_trailing_slash (mchar *str)
{
    int len = mstrlen(str);
    if (len >= SR_MAX_PATH-1)
	return SR_ERROR_DIR_PATH_TOO_LONG;
    if (!ISSLASH(str[mstrlen(str)-1]))
	mstrncat (str,  PATH_SLASH_STR, SR_MAX_PATH);
    return SR_SUCCESS;
}

/* Split off the device */
static error_code 
device_split (mchar* dirname,
	      mchar* device,
	      mchar* path
	      )
{
    int di;

    if (HAS_DEVICE(dirname)) {
	device[0] = dirname[0];
	device[1] = dirname[1];
	device[2] = 0;
	di = 2;
    } else {
	device[0] = 0;
	di = 0;
    }
    mstrcpy (path, &dirname[di]);
    return SR_SUCCESS;
}

static error_code 
sr_getcwd (mchar* dirbuf)
{
    char db[SR_MAX_PATH];
#if defined (WIN32)
    if (!_getcwd (db, SR_MAX_PATH)) {
	debug_printf ("getcwd returned zero?\n");
	return SR_ERROR_DIR_PATH_TOO_LONG;
    }
#else
    if (!getcwd (db, SR_MAX_PATH)) {
	debug_printf ("getcwd returned zero?\n");
	return SR_ERROR_DIR_PATH_TOO_LONG;
    }
#endif
    mstring_from_string (dirbuf, SR_MAX_PATH, db, CODESET_FILESYS);
    return SR_SUCCESS;
}

void close_file (FHANDLE* fp)
{
    if (*fp != INVALID_FHANDLE) {
#if defined WIN32
	CloseHandle (*fp);
#else
	close (*fp);
#endif
	*fp = INVALID_FHANDLE;
    }
}

#ifndef XBMC
void close_files()
{
    close_file (&m_file);
    close_file (&m_show_file);
    close_file (&m_cue_file);
}

BOOL
file_exists (mchar *filename)
{
    FHANDLE f;
    char fn[SR_MAX_PATH];
    string_from_mstring (fn, SR_MAX_PATH, filename, CODESET_FILESYS);
#if defined (WIN32)
    f = CreateFile (fn, GENERIC_READ,
	    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
	    FILE_ATTRIBUTE_NORMAL, NULL);
#else
    f = open (fn, O_RDONLY);
#endif
    if (f == INVALID_FHANDLE) {
	return FALSE;
    }
    close_file (&f);
    return TRUE;
}
#endif

error_code
filelib_write_cue (TRACK_INFO* ti, int secs)
{
#ifndef XBMC
    static int track_no = 1;
    int rc;
    char buf1[MAX_TRACK_LEN];
    char buf2[MAX_TRACK_LEN];

    if (!m_do_show) return SR_SUCCESS;
    if (!m_cue_file) return SR_SUCCESS;

    rc = snprintf (buf2, MAX_TRACK_LEN, "  TRACK %02d AUDIO\n", track_no++);
    filelib_write (m_cue_file, buf2, rc);
    string_from_mstring (buf1, MAX_TRACK_LEN, ti->title, CODESET_ID3);
    rc = snprintf (buf2, MAX_TRACK_LEN, "    TITLE \"%s\"\n", buf1);
    filelib_write (m_cue_file, buf2, rc);
    string_from_mstring (buf1, MAX_TRACK_LEN, ti->artist, CODESET_ID3);
    rc = snprintf (buf2, MAX_TRACK_LEN, "    PERFORMER \"%s\"\n", buf1);
    filelib_write (m_cue_file, buf2, rc);
    rc = snprintf (buf2, MAX_TRACK_LEN, "    INDEX 01 %02d:%02d:00\n", 
		   secs / 60, secs % 60);
    filelib_write (m_cue_file, buf2, rc);
#endif
    return SR_SUCCESS;
}

/* It's a bit touch and go here. My artist substitution might 
   be into a directory, in which case I don't have enough 
   room for a legit file name */
/* Also, what about versioning of completed filenames? */
/* If (TRACK_INFO* ti) is NULL, that means we're being called for the 
   showfile, and therefore some parts don't apply */
static void
parse_and_subst_pat (mchar* newfile,
		     TRACK_INFO* ti,
		     mchar* directory,
		     mchar* pattern,
		     mchar* extension)
{
    mchar stripped_artist[SR_MAX_PATH];
    mchar stripped_title[SR_MAX_PATH];
    mchar stripped_album[SR_MAX_PATH];
#define DATEBUF_LEN 50
    mchar temp[DATEBUF_LEN];
    mchar datebuf[DATEBUF_LEN];
    int opi = 0;
    int nfi = 0;
    int done;
    mchar* pat = pattern;

    /* Reserve 5 bytes: 4 for the .mp3 extension, and 1 for null char */
    int MAX_FILEBASELEN = SR_MAX_PATH-5;

    mstrcpy (newfile, directory);
    opi = 0;
    nfi = mstrlen(newfile);
    done = 0;

    /* Strip artist, title, album */
    debug_printf ("parse_and_subst_pat: stripping\n");
    if (ti) {
	mstrncpy (stripped_artist, ti->artist, SR_MAX_PATH);
	mstrncpy (stripped_title, ti->title, SR_MAX_PATH);
	mstrncpy (stripped_album, ti->album, SR_MAX_PATH);
	replace_invalid_chars (stripped_artist);
	replace_invalid_chars (stripped_title);
	replace_invalid_chars (stripped_album);
    }

    debug_printf ("parse_and_subst_pat: substitute pattern\n");
    while (nfi < MAX_FILEBASELEN) {
	if (pat[opi] == 0) {
	    done = 1;
	    break;
	}
	if (pat[opi] != m_('%')) {
	    newfile[nfi++] = pat[opi++];
	    newfile[nfi] = 0;
	    continue;
	}
	/* If we got here, we have a '%' */
	switch (pat[opi+1]) {
	case m_('%'):
	    newfile[nfi++] = m_('%');
	    newfile[nfi] = 0;
	    opi+=2;
	    continue;
	case m_('S'):
	    /* stream name */
	    /* GCS FIX: Not sure here */
	    mstrncat (newfile, m_icy_name, MAX_FILEBASELEN-nfi);
	    nfi = mstrlen (newfile);
	    opi+=2;
	    continue;
	case m_('d'):
	    /* append date info */
	    mstrncat (newfile, m_session_datebuf, MAX_FILEBASELEN-nfi);
	    nfi = mstrlen (newfile);
	    opi+=2;
	    continue;
	case m_('D'):
	    /* current timestamp */
	    fill_date_buf (datebuf, DATEBUF_LEN);
	    mstrncat (newfile, datebuf, MAX_FILEBASELEN-nfi);
	    nfi = mstrlen (newfile);
	    opi+=2;
	    continue;
	case m_('a'):
	    /* album */
	    if (!ti) goto illegal_pattern;
	    mstrncat (newfile, stripped_album, MAX_FILEBASELEN-nfi);
	    nfi = mstrlen (newfile);
	    opi+=2;
	    continue;
	case m_('A'):
	    /* artist */
	    if (!ti) goto illegal_pattern;
	    mstrncat (newfile, stripped_artist, MAX_FILEBASELEN-nfi);
	    nfi = mstrlen (newfile);
	    opi+=2;
	    continue;
	case m_('q'):
	    /* automatic sequence number */
	    msnprintf (temp, DATEBUF_LEN, m_("%04d"), 
		       get_next_sequence_number (newfile));
	    mstrncat (newfile, temp, MAX_FILEBASELEN-nfi);
	    nfi = mstrlen (newfile);
	    opi+=2;
	    continue;
	case m_('T'):
	    /* title */
	    if (!ti) goto illegal_pattern;
	    mstrncat (newfile, stripped_title, MAX_FILEBASELEN-nfi);
	    nfi = mstrlen (newfile);
	    opi+=2;
	    continue;
	case 0:
	    /* The pattern ends in '%', but that's ok. */
	    newfile[nfi++] = pat[opi++];
	    newfile[nfi] = 0;
	    done = 1;
	    break;
	case m_('0'): case m_('1'): case m_('2'): case m_('3'): case m_('4'): 
	case m_('5'): case m_('6'): case m_('7'): case m_('8'): case m_('9'): 
	    {
		/* Get integer */
		int ai = 0;
		mchar ascii_buf[7];      /* max 6 chars */
		while (isdigit (pat[opi+1+ai]) && ai < 6) {
		    ascii_buf[ai] = pat[opi+1+ai];
		    ai ++;
		}
		ascii_buf[ai] = 0;
		/* If we got a q, get starting number */
		if (pat[opi+1+ai] == m_('q')) {
		    if (m_count == -1) {
			m_count = mtol(ascii_buf);
		    }
		    msnprintf (temp, DATEBUF_LEN, m_("%04d"), m_count);
		    mstrncat (newfile, temp, MAX_FILEBASELEN-nfi);
		    nfi = mstrlen (newfile);
		    opi+=ai+2;
		    continue;
		}
		/* Otherwise, no 'q', so drop through to default case */
	    }
	default:
	illegal_pattern:
	    /* Illegal pattern, but that's ok. */
	    newfile[nfi++] = pat[opi++];
	    newfile[nfi] = 0;
	    continue;
	}
    }

    /* Pop on the extension */
    /* GCS FIX - is SR_MAX_PATH right here? */
    debug_printf ("parse_and_subst_pat: pop on the extension\n");
    mstrncat (newfile, extension, SR_MAX_PATH);
}

error_code
filelib_start (TRACK_INFO* ti)
{
#ifndef XBMC
    mchar newfile[TEMP_STR_LEN];
    mchar fnbase[TEMP_STR_LEN];
    mchar fnbase1[TEMP_STR_LEN];
    mchar fnbase2[TEMP_STR_LEN];

    if (!m_do_individual_tracks) return SR_SUCCESS;

    close_file(&m_file);

    /* Compose and trim filename (not including directory) */
    msnprintf (fnbase1, TEMP_STR_LEN, m_S m_(" - ") m_S, 
	       ti->artist, ti->title);
    trim_filename (fnbase, fnbase1);
    msnprintf (newfile, TEMP_STR_LEN, m_S m_S m_S, 
	       m_incomplete_directory, fnbase, m_extension);
    if (m_keep_incomplete) {
	int n = 1;
	mchar oldfile[TEMP_STR_LEN];
	msnprintf (oldfile, TEMP_STR_LEN, m_S m_S m_S, 
		   m_incomplete_directory, fnbase, m_extension);
	mstrcpy (fnbase1, fnbase);
	while (file_exists (oldfile)) {
	    msnprintf (fnbase1, TEMP_STR_LEN, m_("(%d)") m_S, 
		       n, fnbase);
	    trim_filename (fnbase2, fnbase1);
	    msnprintf (oldfile, TEMP_STR_LEN, m_S m_S m_S, 
		       m_incomplete_directory,
		       fnbase2, m_extension);
	    n++;
	}
	if (mstrcmp (newfile, oldfile) != 0) {
	    move_file (oldfile, newfile);
	}
    }
    mstrcpy (m_incomplete_filename, newfile);
    return filelib_open_for_write(&m_file, newfile);
#else
    return SR_SUCCESS;
#endif
}

static long
get_file_size (mchar *filename)
{
    FILE* fp;
    long len;
    char fn[SR_MAX_PATH];

    string_from_mstring (fn, SR_MAX_PATH, filename, CODESET_FILESYS);
    fp = fopen (fn, "r");
    if (!fp) return 0;

    if (fseek (fp, 0, SEEK_END)) {
	fclose(fp);
	return 0;
    }

    len = ftell (fp);
    if (len < 0) {
	fclose(fp);
	return 0;
    }

    fclose (fp);
    return len;
}

/*
 * Added by Daniel Lord 29.06.2005 to only overwrite files with better 
 * captures, modified by GCS to get file size from file system 
 */
static BOOL
new_file_is_better (mchar *oldfile, mchar *newfile)
{
    long oldfilesize=0;
    long newfilesize=0;

    oldfilesize = get_file_size (oldfile);
    newfilesize = get_file_size (newfile);
    
    /*
     * simple size check for now. Newfile should have at least 1Meg. Else it's
     * not very usefull most of the time.
     */
    /* GCS: This isn't quite true for low bitrate streams.  */
#if defined (commentout)
    if (newfilesize <= 524288) {
	debug_printf("NFB: newfile smaller as 524288\n");
	return FALSE;
    }
#endif

    if (oldfilesize == -1) {
	/* make sure we get the file in case of errors */
	debug_printf("NFB: could not get old filesize\n");
	return TRUE;
    }

    if (oldfilesize == newfilesize) {
	debug_printf("NFB: Size Match\n");
	return FALSE;
    }

    if (newfilesize < oldfilesize) {
	debug_printf("NFB:newfile bigger as oldfile\n");
	return FALSE;
    }

    debug_printf ("NFB:oldfilesize = %li, newfilesize = %li, "
		  "overwriting file\n", oldfilesize, newfilesize);
    return TRUE;
}

void
truncate_file (mchar* filename)
{
    char fn[SR_MAX_PATH];
    string_from_mstring (fn, SR_MAX_PATH, filename, CODESET_FILESYS);
    debug_printf ("Trying to truncate file: %s\n", fn);
#if defined WIN32
    CloseHandle (CreateFile(fn, GENERIC_WRITE, 
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, 
		TRUNCATE_EXISTING, 
		FILE_ATTRIBUTE_NORMAL, NULL));
#else
    close (open (fn, O_RDWR | O_CREAT | O_TRUNC, 
		 S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
#endif
}

void
move_file (mchar* new_filename, mchar* old_filename)
{
    char old_fn[SR_MAX_PATH];
    char new_fn[SR_MAX_PATH];
    string_from_mstring (old_fn, SR_MAX_PATH, old_filename, CODESET_FILESYS);
    string_from_mstring (new_fn, SR_MAX_PATH, new_filename, CODESET_FILESYS);
#if defined WIN32
    MoveFile(old_fn, new_fn);
#else
    rename (old_fn, new_fn);
#endif
}

void
delete_file (mchar* filename)
{
    char fn[SR_MAX_PATH];
    string_from_mstring (fn, SR_MAX_PATH, filename, CODESET_FILESYS);
#if defined WIN32
    DeleteFile (fn);
#else
    unlink (fn);
#endif
}

// Moves the file from incomplete to complete directory
// fullpath is an output parameter
error_code
filelib_end (TRACK_INFO* ti,
	     enum OverwriteOpt overwrite,
	     BOOL truncate_dup,
	     mchar *fullpath)
{
#ifndef XBMC
    BOOL ok_to_write = TRUE;
    mchar newfile[TEMP_STR_LEN];

    if (!m_do_individual_tracks) return SR_SUCCESS;

    close_file (&m_file);

    /* Construct filename for completed file */
    parse_and_subst_pat (newfile, ti, m_output_directory, 
			 m_output_pattern, m_extension);

    /* Build up the output directory */
    mkdir_recursive (newfile, 0);

    // If we are over writing existing tracks
    switch (overwrite) {
    case OVERWRITE_ALWAYS:
	ok_to_write = TRUE;
	break;
    case OVERWRITE_NEVER:
	if (file_exists (newfile)) {
	    ok_to_write = FALSE;
	} else {
	    ok_to_write = TRUE;
	}
	break;
    case OVERWRITE_LARGER:
    default:
	/* Smart overwriting -- only overwrite if new file is bigger */
	ok_to_write = new_file_is_better (newfile, m_incomplete_filename);
	break;
    }

    if (ok_to_write) {
	if (file_exists (newfile)) {
	    delete_file (newfile);
	}
	move_file (newfile, m_incomplete_filename);
    } else {
	if (truncate_dup && file_exists (m_incomplete_filename)) {
	    // TruncateFile(m_incomplete_filename);
	    truncate_file (m_incomplete_filename);
	}
    }

    if (fullpath) {
	mstrcpy (fullpath, newfile);
    }
    if (m_count != -1)
	m_count++;
#endif
    return SR_SUCCESS;
}

static error_code
filelib_open_for_write (FHANDLE* fp, mchar* filename)
{
    char fn[SR_MAX_PATH];
    string_from_mstring (fn, SR_MAX_PATH, filename, CODESET_FILESYS);
    debug_printf ("Trying to create file: %s\n", fn);
#if WIN32
    *fp = CreateFile (fn, GENERIC_WRITE,         // open for reading 
		      FILE_SHARE_READ,           // share for reading 
		      NULL,                      // no security 
		      CREATE_ALWAYS,             // existing file only 
		      FILE_ATTRIBUTE_NORMAL,     // normal file 
		      NULL);                     // no attr. template 
    if (*fp == INVALID_FHANDLE) {
	int r = GetLastError();
	r = strlen(fn);
	printf ("ERROR creating file: %s\n",filename);
	return SR_ERROR_CANT_CREATE_FILE;
    }
#else
    /* For unix, we need to convert to char, and just open. 
       http://mail.nl.linux.org/linux-utf8/2001-02/msg00103.html
    */
    *fp = open (fn, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (*fp == INVALID_FHANDLE) {
	/* GCS FIX -- need error message here! */
	// printf ("ERROR creating file: %s\n",filename);
	return SR_ERROR_CANT_CREATE_FILE;
    }
#endif
    return SR_SUCCESS;
}

error_code
filelib_write (FHANDLE fp, char *buf, u_long size)
{
    if (!fp) {
	debug_printf("filelib_write: fp = 0\n");
	return SR_ERROR_CANT_WRITE_TO_FILE;
    }
#if WIN32
    {
	DWORD bytes_written = 0;
	if (!WriteFile(fp, buf, size, &bytes_written, NULL))
	    return SR_ERROR_CANT_WRITE_TO_FILE;
    }
#else
    if (write(fp, buf, size) == -1)
	return SR_ERROR_CANT_WRITE_TO_FILE;
#endif

    return SR_SUCCESS;
}

error_code
filelib_write_track(char *buf, u_long size)
{
#ifndef XBMC
    return filelib_write (m_file, buf, size);
#else
    return SR_SUCCESS;
#endif
}

static error_code
filelib_open_showfiles ()
{
    int rc;
    mchar mcue_buf[1024];
    char cue_buf[1024];

    parse_and_subst_pat (m_show_name, 0, m_showfile_directory, 
			 m_showfile_pattern, m_extension);
    parse_and_subst_pat (m_cue_name, 0, m_showfile_directory, 
			 m_showfile_pattern, 
			 m_(".cue"));

    rc = filelib_open_for_write (&m_cue_file, m_cue_name);
    if (rc != SR_SUCCESS) {
	m_do_show = 0;
	return rc;
    }

    /* Write cue header here */
    /* GCS FIX: What encoding should the FILE line be? */
    rc = msnprintf (mcue_buf, 1024, m_("FILE \"") m_S m_("\" MP3\n"), 
		    m_show_name);
    rc = string_from_mstring (cue_buf, 1024, mcue_buf, CODESET_FILESYS);
    rc = filelib_write (m_cue_file, cue_buf, rc);
    if (rc != SR_SUCCESS) {
	m_do_show = 0;
	return rc;
    }
    rc = filelib_open_for_write (&m_show_file, m_show_name);
    if (rc != SR_SUCCESS) {
	m_do_show = 0;
	return rc;
    }
    return rc;
}

#ifndef XBMC
error_code
filelib_write_show(char *buf, u_long size)
{
    error_code rc;
    if (!m_do_show) {
	return SR_SUCCESS;
    }
    rc = filelib_write (m_show_file, buf, size);
    if (rc != SR_SUCCESS) {
	m_do_show = 0;
    }
    return rc;
}
#endif

void
filelib_shutdown()
{
#ifndef XBMC
    close_files();
#endif
}

/* GCS: This should get only the name, not the directory */
#ifndef XBMC
static void
trim_filename (mchar* out, mchar *filename)
{
    long maxlen = m_max_filename_length;
    mstrncpy (out, filename, MAX_TRACK_LEN);
    replace_invalid_chars (out);
    out[maxlen-4] = 0;	// -4 = make room for ".mp3"
}

static void
trim_mp3_suffix (mchar *filename)
{
    mchar* suffix_ptr;
    if (mstrlen(filename) <= 4) return;
    suffix_ptr = filename + mstrlen(filename) - 4;  // -4 for ".mp3"
    if (mstrcmp (suffix_ptr, m_extension) == 0) {
	*suffix_ptr = 0;
    }
}
#endif

/* GCS FIX: This may not work with filesystem charsets where 0-9 are 
   not ascii compatible? */
static int
get_next_sequence_number (mchar* fn_base)
{
    int rc;
    int di = 0;
    int edi = 0;
    int seq;
    mchar dir_name[SR_MAX_PATH];
    mchar fn_prefix[SR_MAX_PATH];
    char dname[SR_MAX_PATH];
    char fnp[SR_MAX_PATH];
    DIR* dp;
    struct dirent* de;

    /* Get directory from fn_base */
    while (fn_base[di]) {
	if (ISSLASH(fn_base[di])) {
	    edi = di;
	}
	di++;
    }
    mstrncpy (dir_name, fn_base, SR_MAX_PATH);
    dir_name[edi] = 0;

    /* Get fn prefix from fn_base */
    fn_prefix[0] = 0;
    mstrcpy (fn_prefix, &fn_base[edi+1]);

    rc = string_from_mstring (dname, SR_MAX_PATH, dir_name, CODESET_FILESYS);
    rc = string_from_mstring (fnp, SR_MAX_PATH, fn_prefix, CODESET_FILESYS);

    /* Look through directory for a filenames that match prefix */
    if ((dp = opendir (dname)) == 0) {
	return 0;
    }
    seq = 0;
    while ((de = readdir (dp)) != 0) {
	if (strncmp(de->d_name, fnp, strlen(fnp)) == 0) {
	    if (isdigit(de->d_name[strlen(fnp)])) {
		int this_seq = atoi(&de->d_name[strlen(fnp)]);
		if (seq <= this_seq) {
		    seq = this_seq + 1;
		}
	    }
	}
    }
    closedir (dp);
    return seq;
}

/* GCS FIX: This should only strip "." at beginning of path */
mchar* 
replace_invalid_chars (mchar *str)
{
# if defined (WIN32)
    mchar invalid_chars[] = m_("\\/:*?\"<>|~");
# else
    mchar invalid_chars[] = m_("\\/:*?\"<>|.~");
# endif
    mchar replacement = m_('-');

    mchar *oldstr = str;
    mchar *newstr = str;

    if (!str) return NULL;

    for (;*oldstr; oldstr++) {
	if (mstrchr(invalid_chars, *oldstr) == NULL) {
	    *newstr++ = *oldstr;
	} else {
	    *newstr++ = replacement;
	}
    }
    *newstr = '\0';

    return str;
}
