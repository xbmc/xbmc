/* filelib.h
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
#ifndef __FILELIB_H__
#define __FILELIB_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "srtypes.h"
#if WIN32
#include <windows.h>
#endif

#if WIN32
#define PATH_SLASH m_('\\')
#define PATH_SLASH_STR m_("\\")
#else
#define PATH_SLASH m_('/')
#define PATH_SLASH_STR m_("/")
#endif


/* Pathname support.
   Copyright (C) 1995-1999, 2000-2003 Free Software Foundation, Inc.
   Contributed by Ulrich Drepper <drepper@gnu.ai.mit.edu>, 1995.
   Licence: GNU LGPL */
/* ISSLASH(C)           tests whether C is a directory separator character.
   IS_ABSOLUTE_PATH(P)  tests whether P is an absolute path.  If it is not,
                        it may be concatenated to a directory pathname. */
#if defined _WIN32 || defined __WIN32__ || defined __EMX__ || defined __DJGPP__
  /* Win32, OS/2, DOS */
# define ISSLASH(C) ((C) == m_('/') || (C) == m_('\\'))
# define HAS_DEVICE(P) \
    ((((P)[0] >= m_('A') && (P)[0] <= m_('Z')) \
      || ((P)[0] >= m_('a') && (P)[0] <= m_('z'))) \
     && (P)[1] == m_(':'))
/* GCS: This is not correct, because it could be c:foo which is relative */
/* # define IS_ABSOLUTE_PATH(P) (ISSLASH ((P)[0]) || HAS_DEVICE (P)) */
# define IS_ABSOLUTE_PATH(P) ISSLASH ((P)[0])
#else
  /* Unix */
# define ISSLASH(C) ((C) == m_('/'))
# define HAS_DEVICE(P) (0)
# define IS_ABSOLUTE_PATH(P) ISSLASH ((P)[0])
#endif

#define SR_MIN_FILENAME		54     /* For files in incomplete */
#define SR_MIN_COMPLETEDIR      10     /* For dir with radio station name */
#define SR_DATE_LEN		11
#define SR_MIN_COMPLETE_W_DATE	(SR_MIN_COMPLETEDIR+SR_DATE_LEN)
/* Directory lengths, including trailing slash */
#define SR_MAX_INCOMPLETE  (SR_MAX_PATH-SR_MIN_FILENAME)
#define SR_MAX_COMPLETE    (SR_MAX_INCOMPLETE-strlen("incomplete/"))
#define SR_MAX_BASE        (SR_MAX_COMPLETE-SR_MIN_COMPLETEDIR-strlen("/"))
#define SR_MAX_BASE_W_DATE (SR_MAX_BASE-SR_MIN_COMPLETE_W_DATE)


error_code
filelib_init (BOOL do_individual_tracks,
	      BOOL do_count,
	      int count_start,
	      BOOL keep_incomplete,
	      BOOL do_show_file,
	      int content_type,
	      char* output_directory,
	      char* output_pattern,
	      char* showfile_pattern,
	      int get_separate_dirs,
	      int get_date_stamp,
	      char* icy_name);
error_code filelib_start (TRACK_INFO* ti);
error_code filelib_end (TRACK_INFO* ti, enum OverwriteOpt overwrite,
			BOOL truncate_dup, mchar *fullpath);
error_code filelib_write_track(char *buf, u_long size);
error_code filelib_write_show(char *buf, u_long size);
void filelib_shutdown();
error_code filelib_remove(char *filename);
error_code filelib_write_cue(TRACK_INFO* ti, int secs);

#ifdef __cplusplus
}
#endif

#endif //FILELIB
