/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * gdir.c: Simplified wrapper around the DIRENT functions.
 *
 * Copyright 2001 Hans Breuer
 * Copyright 2004 Tor Lillqvist
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#ifdef HAVE_DIRENT_H
#include <sys/types.h>
#include <dirent.h>
#endif

#include "glib.h"
#include "gdir.h"

#include "glibintl.h"

#include "galias.h"

#if defined (_MSC_VER) && !defined (HAVE_DIRENT_H)
#include "../build/win32/dirent/dirent.h"
#include "../build/win32/dirent/wdirent.c"
#endif

struct _GDir
{
#ifdef G_OS_WIN32
  _WDIR *wdirp;
#else
  DIR *dirp;
#endif
#ifdef G_OS_WIN32
  gchar utf8_buf[FILENAME_MAX*4];
#endif
};

/**
 * g_dir_open:
 * @path: the path to the directory you are interested in. On Unix
 *         in the on-disk encoding. On Windows in UTF-8
 * @flags: Currently must be set to 0. Reserved for future use.
 * @error: return location for a #GError, or %NULL.
 *         If non-%NULL, an error will be set if and only if
 *         g_dir_open() fails.
 *
 * Opens a directory for reading. The names of the files in the
 * directory can then be retrieved using g_dir_read_name().
 *
 * Return value: a newly allocated #GDir on success, %NULL on failure.
 *   If non-%NULL, you must free the result with g_dir_close()
 *   when you are finished with it.
 **/
GDir *
g_dir_open (const gchar  *path,
            guint         flags,
            GError      **error)
{
  GDir *dir;
#ifdef G_OS_WIN32
  wchar_t *wpath;
#else
  gchar *utf8_path;
#endif

  g_return_val_if_fail (path != NULL, NULL);

#ifdef G_OS_WIN32
  wpath = g_utf8_to_utf16 (path, -1, NULL, NULL, error);

  if (wpath == NULL)
    return NULL;

  dir = g_new (GDir, 1);

  dir->wdirp = _wopendir (wpath);
  g_free (wpath);

  if (dir->wdirp)
    return dir;

  /* error case */

  g_set_error (error,
	       G_FILE_ERROR,
	       g_file_error_from_errno (errno),
	       _("Error opening directory '%s': %s"),
	       path, g_strerror (errno));
  
  g_free (dir);
      
  return NULL;
#else
  dir = g_new (GDir, 1);

  dir->dirp = opendir (path);

  if (dir->dirp)
    return dir;

  /* error case */
  utf8_path = g_filename_to_utf8 (path, -1,
				  NULL, NULL, NULL);
  g_set_error (error,
               G_FILE_ERROR,
               g_file_error_from_errno (errno),
               _("Error opening directory '%s': %s"),
	       utf8_path, g_strerror (errno));

  g_free (utf8_path);
  g_free (dir);

  return NULL;
#endif
}

#if defined (G_OS_WIN32) && !defined (_WIN64)

/* The above function actually is called g_dir_open_utf8, and it's
 * that what applications compiled with this GLib version will
 * use.
 */

#undef g_dir_open

/* Binary compatibility version. Not for newly compiled code. */

GDir *
g_dir_open (const gchar  *path,
            guint         flags,
            GError      **error)
{
  gchar *utf8_path = g_locale_to_utf8 (path, -1, NULL, NULL, error);
  GDir *retval;

  if (utf8_path == NULL)
    return NULL;

  retval = g_dir_open_utf8 (utf8_path, flags, error);

  g_free (utf8_path);

  return retval;
}
#endif

/**
 * g_dir_read_name:
 * @dir: a #GDir* created by g_dir_open()
 *
 * Retrieves the name of the next entry in the directory.  The '.' and
 * '..' entries are omitted. On Windows, the returned name is in
 * UTF-8. On Unix, it is in the on-disk encoding.
 *
 * Return value: The entry's name or %NULL if there are no 
 *   more entries. The return value is owned by GLib and
 *   must not be modified or freed.
 **/
G_CONST_RETURN gchar*
g_dir_read_name (GDir *dir)
{
#ifdef G_OS_WIN32
  gchar *utf8_name;
  struct _wdirent *wentry;
#else
  struct dirent *entry;
#endif

  g_return_val_if_fail (dir != NULL, NULL);

#ifdef G_OS_WIN32
  while (1)
    {
      wentry = _wreaddir (dir->wdirp);
      while (wentry 
	     && (0 == wcscmp (wentry->d_name, L".") ||
		 0 == wcscmp (wentry->d_name, L"..")))
	wentry = _wreaddir (dir->wdirp);

      if (wentry == NULL)
	return NULL;

      utf8_name = g_utf16_to_utf8 (wentry->d_name, -1, NULL, NULL, NULL);

      if (utf8_name == NULL)
	continue;		/* Huh, impossible? Skip it anyway */

      strcpy (dir->utf8_buf, utf8_name);
      g_free (utf8_name);

      return dir->utf8_buf;
    }
#else
  entry = readdir (dir->dirp);
  while (entry 
         && (0 == strcmp (entry->d_name, ".") ||
             0 == strcmp (entry->d_name, "..")))
    entry = readdir (dir->dirp);

  if (entry)
    return entry->d_name;
  else
    return NULL;
#endif
}

#if defined (G_OS_WIN32) && !defined (_WIN64)

/* Ditto for g_dir_read_name */

#undef g_dir_read_name

/* Binary compatibility version. Not for newly compiled code. */

G_CONST_RETURN gchar*
g_dir_read_name (GDir *dir)
{
  while (1)
    {
      const gchar *utf8_name = g_dir_read_name_utf8 (dir);
      gchar *retval;
      
      if (utf8_name == NULL)
	return NULL;

      retval = g_locale_from_utf8 (utf8_name, -1, NULL, NULL, NULL);

      if (retval != NULL)
	{
	  strcpy (dir->utf8_buf, retval);
	  g_free (retval);

	  return dir->utf8_buf;
	}
    }
}

#endif

/**
 * g_dir_rewind:
 * @dir: a #GDir* created by g_dir_open()
 *
 * Resets the given directory. The next call to g_dir_read_name()
 * will return the first entry again.
 **/
void
g_dir_rewind (GDir *dir)
{
  g_return_if_fail (dir != NULL);
  
#ifdef G_OS_WIN32
  _wrewinddir (dir->wdirp);
#else
  rewinddir (dir->dirp);
#endif
}

/**
 * g_dir_close:
 * @dir: a #GDir* created by g_dir_open()
 *
 * Closes the directory and deallocates all related resources.
 **/
void
g_dir_close (GDir *dir)
{
  g_return_if_fail (dir != NULL);

#ifdef G_OS_WIN32
  _wclosedir (dir->wdirp);
#else
  closedir (dir->dirp);
#endif
  g_free (dir);
}

#define __G_DIR_C__
#include "galiasdef.c"
