/* gfileutils.c - File utility functions
 *
 *  Copyright 2000 Red Hat, Inc.
 *
 * GLib is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * GLib is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GLib; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *   Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include "glib.h"

#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#ifdef G_OS_WIN32
#include <windows.h>
#include <io.h>
#endif /* G_OS_WIN32 */

#ifndef S_ISLNK
#define S_ISLNK(x) 0
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#include "gstdio.h"
#include "glibintl.h"

#include "galias.h"

static gint create_temp_file (gchar *tmpl, 
			      int    permissions);

/**
 * g_mkdir_with_parents:
 * @pathname: a pathname in the GLib file name encoding
 * @mode: permissions to use for newly created directories
 *
 * Create a directory if it doesn't already exist. Create intermediate
 * parent directories as needed, too.
 *
 * Returns: 0 if the directory already exists, or was successfully
 * created. Returns -1 if an error occurred, with errno set.
 *
 * Since: 2.8
 */
int
g_mkdir_with_parents (const gchar *pathname,
		      int          mode)
{
  gchar *fn, *p;

  if (pathname == NULL || *pathname == '\0')
    {
      errno = EINVAL;
      return -1;
    }

  fn = g_strdup (pathname);

  if (g_path_is_absolute (fn))
    p = (gchar *) g_path_skip_root (fn);
  else
    p = fn;

  do
    {
      while (*p && !G_IS_DIR_SEPARATOR (*p))
	p++;
      
      if (!*p)
	p = NULL;
      else
	*p = '\0';
      
      if (!g_file_test (fn, G_FILE_TEST_EXISTS))
	{
	  if (g_mkdir (fn, mode) == -1)
	    {
	      int errno_save = errno;
	      g_free (fn);
	      errno = errno_save;
	      return -1;
	    }
	}
      else if (!g_file_test (fn, G_FILE_TEST_IS_DIR))
	{
	  g_free (fn);
	  errno = ENOTDIR;
	  return -1;
	}
      if (p)
	{
	  *p++ = G_DIR_SEPARATOR;
	  while (*p && G_IS_DIR_SEPARATOR (*p))
	    p++;
	}
    }
  while (p);

  g_free (fn);

  return 0;
}

/**
 * g_file_test:
 * @filename: a filename to test in the GLib file name encoding
 * @test: bitfield of #GFileTest flags
 * 
 * Returns %TRUE if any of the tests in the bitfield @test are
 * %TRUE. For example, <literal>(G_FILE_TEST_EXISTS | 
 * G_FILE_TEST_IS_DIR)</literal> will return %TRUE if the file exists; 
 * the check whether it's a directory doesn't matter since the existence 
 * test is %TRUE. With the current set of available tests, there's no point
 * passing in more than one test at a time.
 * 
 * Apart from %G_FILE_TEST_IS_SYMLINK all tests follow symbolic links,
 * so for a symbolic link to a regular file g_file_test() will return
 * %TRUE for both %G_FILE_TEST_IS_SYMLINK and %G_FILE_TEST_IS_REGULAR.
 *
 * Note, that for a dangling symbolic link g_file_test() will return
 * %TRUE for %G_FILE_TEST_IS_SYMLINK and %FALSE for all other flags.
 *
 * You should never use g_file_test() to test whether it is safe
 * to perform an operation, because there is always the possibility
 * of the condition changing before you actually perform the operation.
 * For example, you might think you could use %G_FILE_TEST_IS_SYMLINK
 * to know whether it is safe to write to a file without being
 * tricked into writing into a different location. It doesn't work!
 * |[
 * /&ast; DON'T DO THIS &ast;/
 *  if (!g_file_test (filename, G_FILE_TEST_IS_SYMLINK)) 
 *    {
 *      fd = g_open (filename, O_WRONLY);
 *      /&ast; write to fd &ast;/
 *    }
 * ]|
 *
 * Another thing to note is that %G_FILE_TEST_EXISTS and
 * %G_FILE_TEST_IS_EXECUTABLE are implemented using the access()
 * system call. This usually doesn't matter, but if your program
 * is setuid or setgid it means that these tests will give you
 * the answer for the real user ID and group ID, rather than the
 * effective user ID and group ID.
 *
 * On Windows, there are no symlinks, so testing for
 * %G_FILE_TEST_IS_SYMLINK will always return %FALSE. Testing for
 * %G_FILE_TEST_IS_EXECUTABLE will just check that the file exists and
 * its name indicates that it is executable, checking for well-known
 * extensions and those listed in the %PATHEXT environment variable.
 *
 * Return value: whether a test was %TRUE
 **/
gboolean
g_file_test (const gchar *filename,
             GFileTest    test)
{
#ifdef G_OS_WIN32
/* stuff missing in std vc6 api */
#  ifndef INVALID_FILE_ATTRIBUTES
#    define INVALID_FILE_ATTRIBUTES -1
#  endif
#  ifndef FILE_ATTRIBUTE_DEVICE
#    define FILE_ATTRIBUTE_DEVICE 64
#  endif
  int attributes;
  wchar_t *wfilename = g_utf8_to_utf16 (filename, -1, NULL, NULL, NULL);

  if (wfilename == NULL)
    return FALSE;

  attributes = GetFileAttributesW (wfilename);

  g_free (wfilename);

  if (attributes == INVALID_FILE_ATTRIBUTES)
    return FALSE;

  if (test & G_FILE_TEST_EXISTS)
    return TRUE;
      
  if (test & G_FILE_TEST_IS_REGULAR)
    return (attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_DEVICE)) == 0;

  if (test & G_FILE_TEST_IS_DIR)
    return (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

  if (test & G_FILE_TEST_IS_EXECUTABLE)
    {
      const gchar *lastdot = strrchr (filename, '.');
      const gchar *pathext = NULL, *p;
      int extlen;

      if (lastdot == NULL)
	return FALSE;

      if (_stricmp (lastdot, ".exe") == 0 ||
	  _stricmp (lastdot, ".cmd") == 0 ||
	  _stricmp (lastdot, ".bat") == 0 ||
	  _stricmp (lastdot, ".com") == 0)
	return TRUE;

      /* Check if it is one of the types listed in %PATHEXT% */

      pathext = g_getenv ("PATHEXT");
      if (pathext == NULL)
	return FALSE;

      pathext = g_utf8_casefold (pathext, -1);

      lastdot = g_utf8_casefold (lastdot, -1);
      extlen = strlen (lastdot);

      p = pathext;
      while (TRUE)
	{
	  const gchar *q = strchr (p, ';');
	  if (q == NULL)
	    q = p + strlen (p);
	  if (extlen == q - p &&
	      memcmp (lastdot, p, extlen) == 0)
	    {
	      g_free ((gchar *) pathext);
	      g_free ((gchar *) lastdot);
	      return TRUE;
	    }
	  if (*q)
	    p = q + 1;
	  else
	    break;
	}

      g_free ((gchar *) pathext);
      g_free ((gchar *) lastdot);
      return FALSE;
    }

  return FALSE;
#else
  if ((test & G_FILE_TEST_EXISTS) && (access (filename, F_OK) == 0))
    return TRUE;
  
  if ((test & G_FILE_TEST_IS_EXECUTABLE) && (access (filename, X_OK) == 0))
    {
      if (getuid () != 0)
	return TRUE;

      /* For root, on some POSIX systems, access (filename, X_OK)
       * will succeed even if no executable bits are set on the
       * file. We fall through to a stat test to avoid that.
       */
    }
  else
    test &= ~G_FILE_TEST_IS_EXECUTABLE;

  if (test & G_FILE_TEST_IS_SYMLINK)
    {
      struct stat s;

      if ((lstat (filename, &s) == 0) && S_ISLNK (s.st_mode))
        return TRUE;
    }
  
  if (test & (G_FILE_TEST_IS_REGULAR |
	      G_FILE_TEST_IS_DIR |
	      G_FILE_TEST_IS_EXECUTABLE))
    {
      struct stat s;
      
      if (stat (filename, &s) == 0)
	{
	  if ((test & G_FILE_TEST_IS_REGULAR) && S_ISREG (s.st_mode))
	    return TRUE;
	  
	  if ((test & G_FILE_TEST_IS_DIR) && S_ISDIR (s.st_mode))
	    return TRUE;

	  /* The extra test for root when access (file, X_OK) succeeds.
	   */
	  if ((test & G_FILE_TEST_IS_EXECUTABLE) &&
	      ((s.st_mode & S_IXOTH) ||
	       (s.st_mode & S_IXUSR) ||
	       (s.st_mode & S_IXGRP)))
	    return TRUE;
	}
    }

  return FALSE;
#endif
}

GQuark
g_file_error_quark (void)
{
  return g_quark_from_static_string ("g-file-error-quark");
}

/**
 * g_file_error_from_errno:
 * @err_no: an "errno" value
 * 
 * Gets a #GFileError constant based on the passed-in @errno.
 * For example, if you pass in %EEXIST this function returns
 * #G_FILE_ERROR_EXIST. Unlike @errno values, you can portably
 * assume that all #GFileError values will exist.
 *
 * Normally a #GFileError value goes into a #GError returned
 * from a function that manipulates files. So you would use
 * g_file_error_from_errno() when constructing a #GError.
 * 
 * Return value: #GFileError corresponding to the given @errno
 **/
GFileError
g_file_error_from_errno (gint err_no)
{
  switch (err_no)
    {
#ifdef EEXIST
    case EEXIST:
      return G_FILE_ERROR_EXIST;
      break;
#endif

#ifdef EISDIR
    case EISDIR:
      return G_FILE_ERROR_ISDIR;
      break;
#endif

#ifdef EACCES
    case EACCES:
      return G_FILE_ERROR_ACCES;
      break;
#endif

#ifdef ENAMETOOLONG
    case ENAMETOOLONG:
      return G_FILE_ERROR_NAMETOOLONG;
      break;
#endif

#ifdef ENOENT
    case ENOENT:
      return G_FILE_ERROR_NOENT;
      break;
#endif

#ifdef ENOTDIR
    case ENOTDIR:
      return G_FILE_ERROR_NOTDIR;
      break;
#endif

#ifdef ENXIO
    case ENXIO:
      return G_FILE_ERROR_NXIO;
      break;
#endif

#ifdef ENODEV
    case ENODEV:
      return G_FILE_ERROR_NODEV;
      break;
#endif

#ifdef EROFS
    case EROFS:
      return G_FILE_ERROR_ROFS;
      break;
#endif

#ifdef ETXTBSY
    case ETXTBSY:
      return G_FILE_ERROR_TXTBSY;
      break;
#endif

#ifdef EFAULT
    case EFAULT:
      return G_FILE_ERROR_FAULT;
      break;
#endif

#ifdef ELOOP
    case ELOOP:
      return G_FILE_ERROR_LOOP;
      break;
#endif

#ifdef ENOSPC
    case ENOSPC:
      return G_FILE_ERROR_NOSPC;
      break;
#endif

#ifdef ENOMEM
    case ENOMEM:
      return G_FILE_ERROR_NOMEM;
      break;
#endif

#ifdef EMFILE
    case EMFILE:
      return G_FILE_ERROR_MFILE;
      break;
#endif

#ifdef ENFILE
    case ENFILE:
      return G_FILE_ERROR_NFILE;
      break;
#endif

#ifdef EBADF
    case EBADF:
      return G_FILE_ERROR_BADF;
      break;
#endif

#ifdef EINVAL
    case EINVAL:
      return G_FILE_ERROR_INVAL;
      break;
#endif

#ifdef EPIPE
    case EPIPE:
      return G_FILE_ERROR_PIPE;
      break;
#endif

#ifdef EAGAIN
    case EAGAIN:
      return G_FILE_ERROR_AGAIN;
      break;
#endif

#ifdef EINTR
    case EINTR:
      return G_FILE_ERROR_INTR;
      break;
#endif

#ifdef EIO
    case EIO:
      return G_FILE_ERROR_IO;
      break;
#endif

#ifdef EPERM
    case EPERM:
      return G_FILE_ERROR_PERM;
      break;
#endif

#ifdef ENOSYS
    case ENOSYS:
      return G_FILE_ERROR_NOSYS;
      break;
#endif

    default:
      return G_FILE_ERROR_FAILED;
      break;
    }
}

static gboolean
get_contents_stdio (const gchar  *display_filename,
                    FILE         *f,
                    gchar       **contents,
                    gsize        *length,
                    GError      **error)
{
  gchar buf[4096];
  gsize bytes;
  gchar *str = NULL;
  gsize total_bytes = 0;
  gsize total_allocated = 0;
  gchar *tmp;

  g_assert (f != NULL);

  while (!feof (f))
    {
      gint save_errno;

      bytes = fread (buf, 1, sizeof (buf), f);
      save_errno = errno;

      while ((total_bytes + bytes + 1) > total_allocated)
        {
          if (str)
            total_allocated *= 2;
          else
            total_allocated = MIN (bytes + 1, sizeof (buf));

          tmp = g_try_realloc (str, total_allocated);

          if (tmp == NULL)
            {
              g_set_error (error,
                           G_FILE_ERROR,
                           G_FILE_ERROR_NOMEM,
                           _("Could not allocate %lu bytes to read file \"%s\""),
                           (gulong) total_allocated,
			   display_filename);

              goto error;
            }

	  str = tmp;
        }

      if (ferror (f))
        {
          g_set_error (error,
                       G_FILE_ERROR,
                       g_file_error_from_errno (save_errno),
                       _("Error reading file '%s': %s"),
                       display_filename,
		       g_strerror (save_errno));

          goto error;
        }

      memcpy (str + total_bytes, buf, bytes);

      if (total_bytes + bytes < total_bytes) 
        {
          g_set_error (error,
                       G_FILE_ERROR,
                       G_FILE_ERROR_FAILED,
                       _("File \"%s\" is too large"),
                       display_filename);

          goto error;
        }

      total_bytes += bytes;
    }

  fclose (f);

  if (total_allocated == 0)
    {
      str = g_new (gchar, 1);
      total_bytes = 0;
    }

  str[total_bytes] = '\0';

  if (length)
    *length = total_bytes;

  *contents = str;

  return TRUE;

 error:

  g_free (str);
  fclose (f);

  return FALSE;
}

#ifndef G_OS_WIN32

static gboolean
get_contents_regfile (const gchar  *display_filename,
                      struct stat  *stat_buf,
                      gint          fd,
                      gchar       **contents,
                      gsize        *length,
                      GError      **error)
{
  gchar *buf;
  gsize bytes_read;
  gsize size;
  gsize alloc_size;
  
  size = stat_buf->st_size;

  alloc_size = size + 1;
  buf = g_try_malloc (alloc_size);

  if (buf == NULL)
    {
      g_set_error (error,
                   G_FILE_ERROR,
                   G_FILE_ERROR_NOMEM,
                   _("Could not allocate %lu bytes to read file \"%s\""),
                   (gulong) alloc_size, 
		   display_filename);

      goto error;
    }
  
  bytes_read = 0;
  while (bytes_read < size)
    {
      gssize rc;
          
      rc = read (fd, buf + bytes_read, size - bytes_read);

      if (rc < 0)
        {
          if (errno != EINTR) 
            {
	      int save_errno = errno;

              g_free (buf);
              g_set_error (error,
                           G_FILE_ERROR,
                           g_file_error_from_errno (save_errno),
                           _("Failed to read from file '%s': %s"),
                           display_filename, 
			   g_strerror (save_errno));

	      goto error;
            }
        }
      else if (rc == 0)
        break;
      else
        bytes_read += rc;
    }
      
  buf[bytes_read] = '\0';

  if (length)
    *length = bytes_read;
  
  *contents = buf;

  close (fd);

  return TRUE;

 error:

  close (fd);
  
  return FALSE;
}

static gboolean
get_contents_posix (const gchar  *filename,
                    gchar       **contents,
                    gsize        *length,
                    GError      **error)
{
  struct stat stat_buf;
  gint fd;
  gchar *display_filename = g_filename_display_name (filename);

  /* O_BINARY useful on Cygwin */
  fd = open (filename, O_RDONLY|O_BINARY);

  if (fd < 0)
    {
      int save_errno = errno;

      g_set_error (error,
                   G_FILE_ERROR,
                   g_file_error_from_errno (save_errno),
                   _("Failed to open file '%s': %s"),
                   display_filename, 
		   g_strerror (save_errno));
      g_free (display_filename);

      return FALSE;
    }

  /* I don't think this will ever fail, aside from ENOMEM, but. */
  if (fstat (fd, &stat_buf) < 0)
    {
      int save_errno = errno;

      close (fd);
      g_set_error (error,
                   G_FILE_ERROR,
                   g_file_error_from_errno (save_errno),
                   _("Failed to get attributes of file '%s': fstat() failed: %s"),
                   display_filename, 
		   g_strerror (save_errno));
      g_free (display_filename);

      return FALSE;
    }

  if (stat_buf.st_size > 0 && S_ISREG (stat_buf.st_mode))
    {
      gboolean retval = get_contents_regfile (display_filename,
					      &stat_buf,
					      fd,
					      contents,
					      length,
					      error);
      g_free (display_filename);

      return retval;
    }
  else
    {
      FILE *f;
      gboolean retval;

      f = fdopen (fd, "r");
      
      if (f == NULL)
        {
	  int save_errno = errno;

          g_set_error (error,
                       G_FILE_ERROR,
                       g_file_error_from_errno (save_errno),
                       _("Failed to open file '%s': fdopen() failed: %s"),
                       display_filename, 
		       g_strerror (save_errno));
          g_free (display_filename);

          return FALSE;
        }
  
      retval = get_contents_stdio (display_filename, f, contents, length, error);
      g_free (display_filename);

      return retval;
    }
}

#else  /* G_OS_WIN32 */

static gboolean
get_contents_win32 (const gchar  *filename,
		    gchar       **contents,
		    gsize        *length,
		    GError      **error)
{
  FILE *f;
  gboolean retval;
  gchar *display_filename = g_filename_display_name (filename);
  int save_errno;
  
  f = g_fopen (filename, "rb");
  save_errno = errno;

  if (f == NULL)
    {
      g_set_error (error,
                   G_FILE_ERROR,
                   g_file_error_from_errno (save_errno),
                   _("Failed to open file '%s': %s"),
                   display_filename,
		   g_strerror (save_errno));
      g_free (display_filename);

      return FALSE;
    }
  
  retval = get_contents_stdio (display_filename, f, contents, length, error);
  g_free (display_filename);

  return retval;
}

#endif

/**
 * g_file_get_contents:
 * @filename: name of a file to read contents from, in the GLib file name encoding
 * @contents: location to store an allocated string, use g_free() to free
 *     the returned string
 * @length: location to store length in bytes of the contents, or %NULL
 * @error: return location for a #GError, or %NULL
 *
 * Reads an entire file into allocated memory, with good error
 * checking.
 *
 * If the call was successful, it returns %TRUE and sets @contents to the file
 * contents and @length to the length of the file contents in bytes. The string
 * stored in @contents will be nul-terminated, so for text files you can pass
 * %NULL for the @length argument. If the call was not successful, it returns
 * %FALSE and sets @error. The error domain is #G_FILE_ERROR. Possible error
 * codes are those in the #GFileError enumeration. In the error case,
 * @contents is set to %NULL and @length is set to zero.
 *
 * Return value: %TRUE on success, %FALSE if an error occurred
 **/
gboolean
g_file_get_contents (const gchar  *filename,
                     gchar       **contents,
                     gsize        *length,
                     GError      **error)
{  
  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (contents != NULL, FALSE);

  *contents = NULL;
  if (length)
    *length = 0;

#ifdef G_OS_WIN32
  return get_contents_win32 (filename, contents, length, error);
#else
  return get_contents_posix (filename, contents, length, error);
#endif
}

static gboolean
rename_file (const char  *old_name,
	     const char  *new_name,
	     GError     **err)
{
  errno = 0;
  if (g_rename (old_name, new_name) == -1)
    {
      int save_errno = errno;
      gchar *display_old_name = g_filename_display_name (old_name);
      gchar *display_new_name = g_filename_display_name (new_name);

      g_set_error (err,
		   G_FILE_ERROR,
		   g_file_error_from_errno (save_errno),
		   _("Failed to rename file '%s' to '%s': g_rename() failed: %s"),
		   display_old_name,
		   display_new_name,
		   g_strerror (save_errno));

      g_free (display_old_name);
      g_free (display_new_name);
      
      return FALSE;
    }
  
  return TRUE;
}

static gchar *
write_to_temp_file (const gchar  *contents,
		    gssize        length,
		    const gchar  *dest_file,
		    GError      **err)
{
  gchar *tmp_name;
  gchar *display_name;
  gchar *retval;
  FILE *file;
  gint fd;
  int save_errno;

  retval = NULL;
  
  tmp_name = g_strdup_printf ("%s.XXXXXX", dest_file);

  errno = 0;
  fd = create_temp_file (tmp_name, 0666);
  save_errno = errno;

  display_name = g_filename_display_name (tmp_name);
      
  if (fd == -1)
    {
      g_set_error (err,
		   G_FILE_ERROR,
		   g_file_error_from_errno (save_errno),
		   _("Failed to create file '%s': %s"),
		   display_name, g_strerror (save_errno));
      
      goto out;
    }

  errno = 0;
  file = fdopen (fd, "wb");
  if (!file)
    {
      save_errno = errno;
      g_set_error (err,
		   G_FILE_ERROR,
		   g_file_error_from_errno (save_errno),
		   _("Failed to open file '%s' for writing: fdopen() failed: %s"),
		   display_name,
		   g_strerror (save_errno));

      close (fd);
      g_unlink (tmp_name);
      
      goto out;
    }

  if (length > 0)
    {
      gsize n_written;
      
      errno = 0;

      n_written = fwrite (contents, 1, length, file);

      if (n_written < length)
	{
	  save_errno = errno;
      
 	  g_set_error (err,
		       G_FILE_ERROR,
		       g_file_error_from_errno (save_errno),
		       _("Failed to write file '%s': fwrite() failed: %s"),
		       display_name,
		       g_strerror (save_errno));

	  fclose (file);
	  g_unlink (tmp_name);
	  
	  goto out;
	}
    }

  errno = 0;
  if (fflush (file) != 0)
    { 
      save_errno = errno;
      
      g_set_error (err,
		   G_FILE_ERROR,
		   g_file_error_from_errno (save_errno),
		   _("Failed to write file '%s': fflush() failed: %s"),
		   display_name, 
		   g_strerror (save_errno));

      g_unlink (tmp_name);
      
      goto out;
    }
  
#ifdef HAVE_FSYNC
  errno = 0;
  /* If the final destination exists, we want to sync the newly written
   * file to ensure the data is on disk when we rename over the destination.
   * otherwise if we get a system crash we can lose both the new and the
   * old file on some filesystems. (I.E. those that don't guarantee the
   * data is written to the disk before the metadata.)
   */
  if (g_file_test (dest_file, G_FILE_TEST_EXISTS) &&
      fsync (fileno (file)) != 0)
    { 
      save_errno = errno;
      
      g_set_error (err,
		   G_FILE_ERROR,
		   g_file_error_from_errno (save_errno),
		   _("Failed to write file '%s': fsync() failed: %s"),
		   display_name, 
		   g_strerror (save_errno));

      g_unlink (tmp_name);
      
      goto out;
    }
#endif
  
  errno = 0;
  if (fclose (file) == EOF)
    { 
      save_errno = errno;
      
      g_set_error (err,
		   G_FILE_ERROR,
		   g_file_error_from_errno (save_errno),
		   _("Failed to close file '%s': fclose() failed: %s"),
		   display_name, 
		   g_strerror (save_errno));

      g_unlink (tmp_name);
      
      goto out;
    }

  retval = g_strdup (tmp_name);
  
 out:
  g_free (tmp_name);
  g_free (display_name);
  
  return retval;
}

/**
 * g_file_set_contents:
 * @filename: name of a file to write @contents to, in the GLib file name
 *   encoding
 * @contents: string to write to the file
 * @length: length of @contents, or -1 if @contents is a nul-terminated string
 * @error: return location for a #GError, or %NULL
 *
 * Writes all of @contents to a file named @filename, with good error checking.
 * If a file called @filename already exists it will be overwritten.
 *
 * This write is atomic in the sense that it is first written to a temporary
 * file which is then renamed to the final name. Notes:
 * <itemizedlist>
 * <listitem>
 *    On Unix, if @filename already exists hard links to @filename will break.
 *    Also since the file is recreated, existing permissions, access control
 *    lists, metadata etc. may be lost. If @filename is a symbolic link,
 *    the link itself will be replaced, not the linked file.
 * </listitem>
 * <listitem>
 *   On Windows renaming a file will not remove an existing file with the
 *   new name, so on Windows there is a race condition between the existing
 *   file being removed and the temporary file being renamed.
 * </listitem>
 * <listitem>
 *   On Windows there is no way to remove a file that is open to some
 *   process, or mapped into memory. Thus, this function will fail if
 *   @filename already exists and is open.
 * </listitem>
 * </itemizedlist>
 *
 * If the call was sucessful, it returns %TRUE. If the call was not successful,
 * it returns %FALSE and sets @error. The error domain is #G_FILE_ERROR.
 * Possible error codes are those in the #GFileError enumeration.
 *
 * Return value: %TRUE on success, %FALSE if an error occurred
 *
 * Since: 2.8
 **/
gboolean
g_file_set_contents (const gchar  *filename,
		     const gchar  *contents,
		     gssize	   length,
		     GError	 **error)
{
  gchar *tmp_filename;
  gboolean retval;
  GError *rename_error = NULL;
  
  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
  g_return_val_if_fail (contents != NULL || length == 0, FALSE);
  g_return_val_if_fail (length >= -1, FALSE);
  
  if (length == -1)
    length = strlen (contents);

  tmp_filename = write_to_temp_file (contents, length, filename, error);
  
  if (!tmp_filename)
    {
      retval = FALSE;
      goto out;
    }

  if (!rename_file (tmp_filename, filename, &rename_error))
    {
#ifndef G_OS_WIN32

      g_unlink (tmp_filename);
      g_propagate_error (error, rename_error);
      retval = FALSE;
      goto out;

#else /* G_OS_WIN32 */
      
      /* Renaming failed, but on Windows this may just mean
       * the file already exists. So if the target file
       * exists, try deleting it and do the rename again.
       */
      if (!g_file_test (filename, G_FILE_TEST_EXISTS))
	{
	  g_unlink (tmp_filename);
	  g_propagate_error (error, rename_error);
	  retval = FALSE;
	  goto out;
	}

      g_error_free (rename_error);
      
      if (g_unlink (filename) == -1)
	{
          gchar *display_filename = g_filename_display_name (filename);

	  int save_errno = errno;
	  
	  g_set_error (error,
		       G_FILE_ERROR,
		       g_file_error_from_errno (save_errno),
		       _("Existing file '%s' could not be removed: g_unlink() failed: %s"),
		       display_filename,
		       g_strerror (save_errno));

	  g_free (display_filename);
	  g_unlink (tmp_filename);
	  retval = FALSE;
	  goto out;
	}
      
      if (!rename_file (tmp_filename, filename, error))
	{
	  g_unlink (tmp_filename);
	  retval = FALSE;
	  goto out;
	}

#endif
    }

  retval = TRUE;
  
 out:
  g_free (tmp_filename);
  return retval;
}

/*
 * create_temp_file based on the mkstemp implementation from the GNU C library.
 * Copyright (C) 1991,92,93,94,95,96,97,98,99 Free Software Foundation, Inc.
 */
static gint
create_temp_file (gchar *tmpl, 
		  int    permissions)
{
  char *XXXXXX;
  int count, fd;
  static const char letters[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  static const int NLETTERS = sizeof (letters) - 1;
  glong value;
  GTimeVal tv;
  static int counter = 0;

  /* find the last occurrence of "XXXXXX" */
  XXXXXX = g_strrstr (tmpl, "XXXXXX");

  if (!XXXXXX || strncmp (XXXXXX, "XXXXXX", 6))
    {
      errno = EINVAL;
      return -1;
    }

  /* Get some more or less random data.  */
  g_get_current_time (&tv);
  value = (tv.tv_usec ^ tv.tv_sec) + counter++;

  for (count = 0; count < 100; value += 7777, ++count)
    {
      glong v = value;

      /* Fill in the random bits.  */
      XXXXXX[0] = letters[v % NLETTERS];
      v /= NLETTERS;
      XXXXXX[1] = letters[v % NLETTERS];
      v /= NLETTERS;
      XXXXXX[2] = letters[v % NLETTERS];
      v /= NLETTERS;
      XXXXXX[3] = letters[v % NLETTERS];
      v /= NLETTERS;
      XXXXXX[4] = letters[v % NLETTERS];
      v /= NLETTERS;
      XXXXXX[5] = letters[v % NLETTERS];

      /* tmpl is in UTF-8 on Windows, thus use g_open() */
      fd = g_open (tmpl, O_RDWR | O_CREAT | O_EXCL | O_BINARY, permissions);

      if (fd >= 0)
	return fd;
      else if (errno != EEXIST)
	/* Any other error will apply also to other names we might
	 *  try, and there are 2^32 or so of them, so give up now.
	 */
	return -1;
    }

  /* We got out of the loop because we ran out of combinations to try.  */
  errno = EEXIST;
  return -1;
}

/**
 * g_mkstemp:
 * @tmpl: template filename
 *
 * Opens a temporary file. See the mkstemp() documentation
 * on most UNIX-like systems. 
 *
 * The parameter is a string that should follow the rules for
 * mkstemp() templates, i.e. contain the string "XXXXXX". 
 * g_mkstemp() is slightly more flexible than mkstemp()
 * in that the sequence does not have to occur at the very end of the 
 * template. The X string will 
 * be modified to form the name of a file that didn't exist.
 * The string should be in the GLib file name encoding. Most importantly, 
 * on Windows it should be in UTF-8.
 *
 * Return value: A file handle (as from open()) to the file
 * opened for reading and writing. The file is opened in binary mode
 * on platforms where there is a difference. The file handle should be
 * closed with close(). In case of errors, -1 is returned.  
 */ 
gint
g_mkstemp (gchar *tmpl)
{
  return create_temp_file (tmpl, 0600);
}

/**
 * g_file_open_tmp:
 * @tmpl: Template for file name, as in g_mkstemp(), basename only,
 *        or %NULL, to a default template
 * @name_used: location to store actual name used, or %NULL
 * @error: return location for a #GError
 *
 * Opens a file for writing in the preferred directory for temporary
 * files (as returned by g_get_tmp_dir()). 
 *
 * @tmpl should be a string in the GLib file name encoding containing 
 * a sequence of six 'X' characters, as the parameter to g_mkstemp().
 * However, unlike these functions, the template should only be a
 * basename, no directory components are allowed. If template is
 * %NULL, a default template is used.
 *
 * Note that in contrast to g_mkstemp() (and mkstemp()) 
 * @tmpl is not modified, and might thus be a read-only literal string.
 *
 * The actual name used is returned in @name_used if non-%NULL. This
 * string should be freed with g_free() when not needed any longer.
 * The returned name is in the GLib file name encoding.
 *
 * Return value: A file handle (as from open()) to 
 * the file opened for reading and writing. The file is opened in binary 
 * mode on platforms where there is a difference. The file handle should be
 * closed with close(). In case of errors, -1 is returned 
 * and @error will be set.
 **/
gint
g_file_open_tmp (const gchar  *tmpl,
		 gchar       **name_used,
		 GError      **error)
{
  int retval;
  const char *tmpdir;
  const char *sep;
  char *fulltemplate;
  const char *slash;

  if (tmpl == NULL)
    tmpl = ".XXXXXX";

  if ((slash = strchr (tmpl, G_DIR_SEPARATOR)) != NULL
#ifdef G_OS_WIN32
      || (strchr (tmpl, '/') != NULL && (slash = "/"))
#endif
      )
    {
      gchar *display_tmpl = g_filename_display_name (tmpl);
      char c[2];
      c[0] = *slash;
      c[1] = '\0';

      g_set_error (error,
		   G_FILE_ERROR,
		   G_FILE_ERROR_FAILED,
		   _("Template '%s' invalid, should not contain a '%s'"),
		   display_tmpl, c);
      g_free (display_tmpl);

      return -1;
    }
  
  if (strstr (tmpl, "XXXXXX") == NULL)
    {
      gchar *display_tmpl = g_filename_display_name (tmpl);
      g_set_error (error,
		   G_FILE_ERROR,
		   G_FILE_ERROR_FAILED,
		   _("Template '%s' doesn't contain XXXXXX"),
		   display_tmpl);
      g_free (display_tmpl);
      return -1;
    }

  tmpdir = g_get_tmp_dir ();

  if (G_IS_DIR_SEPARATOR (tmpdir [strlen (tmpdir) - 1]))
    sep = "";
  else
    sep = G_DIR_SEPARATOR_S;

  fulltemplate = g_strconcat (tmpdir, sep, tmpl, NULL);

  retval = g_mkstemp (fulltemplate);

  if (retval == -1)
    {
      int save_errno = errno;
      gchar *display_fulltemplate = g_filename_display_name (fulltemplate);

      g_set_error (error,
		   G_FILE_ERROR,
		   g_file_error_from_errno (save_errno),
		   _("Failed to create file '%s': %s"),
		   display_fulltemplate, g_strerror (save_errno));
      g_free (display_fulltemplate);
      g_free (fulltemplate);
      return -1;
    }

  if (name_used)
    *name_used = fulltemplate;
  else
    g_free (fulltemplate);

  return retval;
}

static gchar *
g_build_path_va (const gchar  *separator,
		 const gchar  *first_element,
		 va_list      *args,
		 gchar       **str_array)
{
  GString *result;
  gint separator_len = strlen (separator);
  gboolean is_first = TRUE;
  gboolean have_leading = FALSE;
  const gchar *single_element = NULL;
  const gchar *next_element;
  const gchar *last_trailing = NULL;
  gint i = 0;

  result = g_string_new (NULL);

  if (str_array)
    next_element = str_array[i++];
  else
    next_element = first_element;

  while (TRUE)
    {
      const gchar *element;
      const gchar *start;
      const gchar *end;

      if (next_element)
	{
	  element = next_element;
	  if (str_array)
	    next_element = str_array[i++];
	  else
	    next_element = va_arg (*args, gchar *);
	}
      else
	break;

      /* Ignore empty elements */
      if (!*element)
	continue;
      
      start = element;

      if (separator_len)
	{
	  while (start &&
		 strncmp (start, separator, separator_len) == 0)
	    start += separator_len;
      	}

      end = start + strlen (start);
      
      if (separator_len)
	{
	  while (end >= start + separator_len &&
		 strncmp (end - separator_len, separator, separator_len) == 0)
	    end -= separator_len;
	  
	  last_trailing = end;
	  while (last_trailing >= element + separator_len &&
		 strncmp (last_trailing - separator_len, separator, separator_len) == 0)
	    last_trailing -= separator_len;

	  if (!have_leading)
	    {
	      /* If the leading and trailing separator strings are in the
	       * same element and overlap, the result is exactly that element
	       */
	      if (last_trailing <= start)
		single_element = element;
		  
	      g_string_append_len (result, element, start - element);
	      have_leading = TRUE;
	    }
	  else
	    single_element = NULL;
	}

      if (end == start)
	continue;

      if (!is_first)
	g_string_append (result, separator);
      
      g_string_append_len (result, start, end - start);
      is_first = FALSE;
    }

  if (single_element)
    {
      g_string_free (result, TRUE);
      return g_strdup (single_element);
    }
  else
    {
      if (last_trailing)
	g_string_append (result, last_trailing);
  
      return g_string_free (result, FALSE);
    }
}

/**
 * g_build_pathv:
 * @separator: a string used to separator the elements of the path.
 * @args: %NULL-terminated array of strings containing the path elements.
 * 
 * Behaves exactly like g_build_path(), but takes the path elements 
 * as a string array, instead of varargs. This function is mainly
 * meant for language bindings.
 *
 * Return value: a newly-allocated string that must be freed with g_free().
 *
 * Since: 2.8
 */
gchar *
g_build_pathv (const gchar  *separator,
	       gchar       **args)
{
  if (!args)
    return NULL;

  return g_build_path_va (separator, NULL, NULL, args);
}


/**
 * g_build_path:
 * @separator: a string used to separator the elements of the path.
 * @first_element: the first element in the path
 * @Varargs: remaining elements in path, terminated by %NULL
 * 
 * Creates a path from a series of elements using @separator as the
 * separator between elements. At the boundary between two elements,
 * any trailing occurrences of separator in the first element, or
 * leading occurrences of separator in the second element are removed
 * and exactly one copy of the separator is inserted.
 *
 * Empty elements are ignored.
 *
 * The number of leading copies of the separator on the result is
 * the same as the number of leading copies of the separator on
 * the first non-empty element.
 *
 * The number of trailing copies of the separator on the result is
 * the same as the number of trailing copies of the separator on
 * the last non-empty element. (Determination of the number of
 * trailing copies is done without stripping leading copies, so
 * if the separator is <literal>ABA</literal>, <literal>ABABA</literal>
 * has 1 trailing copy.)
 *
 * However, if there is only a single non-empty element, and there
 * are no characters in that element not part of the leading or
 * trailing separators, then the result is exactly the original value
 * of that element.
 *
 * Other than for determination of the number of leading and trailing
 * copies of the separator, elements consisting only of copies
 * of the separator are ignored.
 * 
 * Return value: a newly-allocated string that must be freed with g_free().
 **/
gchar *
g_build_path (const gchar *separator,
	      const gchar *first_element,
	      ...)
{
  gchar *str;
  va_list args;

  g_return_val_if_fail (separator != NULL, NULL);

  va_start (args, first_element);
  str = g_build_path_va (separator, first_element, &args, NULL);
  va_end (args);

  return str;
}

#ifdef G_OS_WIN32

static gchar *
g_build_pathname_va (const gchar  *first_element,
		     va_list      *args,
		     gchar       **str_array)
{
  /* Code copied from g_build_pathv(), and modified to use two
   * alternative single-character separators.
   */
  GString *result;
  gboolean is_first = TRUE;
  gboolean have_leading = FALSE;
  const gchar *single_element = NULL;
  const gchar *next_element;
  const gchar *last_trailing = NULL;
  gchar current_separator = '\\';
  gint i = 0;

  result = g_string_new (NULL);

  if (str_array)
    next_element = str_array[i++];
  else
    next_element = first_element;
  
  while (TRUE)
    {
      const gchar *element;
      const gchar *start;
      const gchar *end;

      if (next_element)
	{
	  element = next_element;
	  if (str_array)
	    next_element = str_array[i++];
	  else
	    next_element = va_arg (*args, gchar *);
	}
      else
	break;

      /* Ignore empty elements */
      if (!*element)
	continue;
      
      start = element;

      if (TRUE)
	{
	  while (start &&
		 (*start == '\\' || *start == '/'))
	    {
	      current_separator = *start;
	      start++;
	    }
	}

      end = start + strlen (start);
      
      if (TRUE)
	{
	  while (end >= start + 1 &&
		 (end[-1] == '\\' || end[-1] == '/'))
	    {
	      current_separator = end[-1];
	      end--;
	    }
	  
	  last_trailing = end;
	  while (last_trailing >= element + 1 &&
		 (last_trailing[-1] == '\\' || last_trailing[-1] == '/'))
	    last_trailing--;

	  if (!have_leading)
	    {
	      /* If the leading and trailing separator strings are in the
	       * same element and overlap, the result is exactly that element
	       */
	      if (last_trailing <= start)
		single_element = element;
		  
	      g_string_append_len (result, element, start - element);
	      have_leading = TRUE;
	    }
	  else
	    single_element = NULL;
	}

      if (end == start)
	continue;

      if (!is_first)
	g_string_append_len (result, &current_separator, 1);
      
      g_string_append_len (result, start, end - start);
      is_first = FALSE;
    }

  if (single_element)
    {
      g_string_free (result, TRUE);
      return g_strdup (single_element);
    }
  else
    {
      if (last_trailing)
	g_string_append (result, last_trailing);
  
      return g_string_free (result, FALSE);
    }
}

#endif

/**
 * g_build_filenamev:
 * @args: %NULL-terminated array of strings containing the path elements.
 * 
 * Behaves exactly like g_build_filename(), but takes the path elements 
 * as a string array, instead of varargs. This function is mainly
 * meant for language bindings.
 *
 * Return value: a newly-allocated string that must be freed with g_free().
 * 
 * Since: 2.8
 */
gchar *
g_build_filenamev (gchar **args)
{
  gchar *str;

#ifndef G_OS_WIN32
  str = g_build_path_va (G_DIR_SEPARATOR_S, NULL, NULL, args);
#else
  str = g_build_pathname_va (NULL, NULL, args);
#endif

  return str;
}

/**
 * g_build_filename:
 * @first_element: the first element in the path
 * @Varargs: remaining elements in path, terminated by %NULL
 * 
 * Creates a filename from a series of elements using the correct
 * separator for filenames.
 *
 * On Unix, this function behaves identically to <literal>g_build_path
 * (G_DIR_SEPARATOR_S, first_element, ....)</literal>.
 *
 * On Windows, it takes into account that either the backslash
 * (<literal>\</literal> or slash (<literal>/</literal>) can be used
 * as separator in filenames, but otherwise behaves as on Unix. When
 * file pathname separators need to be inserted, the one that last
 * previously occurred in the parameters (reading from left to right)
 * is used.
 *
 * No attempt is made to force the resulting filename to be an absolute
 * path. If the first element is a relative path, the result will
 * be a relative path. 
 * 
 * Return value: a newly-allocated string that must be freed with g_free().
 **/
gchar *
g_build_filename (const gchar *first_element, 
		  ...)
{
  gchar *str;
  va_list args;

  va_start (args, first_element);
#ifndef G_OS_WIN32
  str = g_build_path_va (G_DIR_SEPARATOR_S, first_element, &args, NULL);
#else
  str = g_build_pathname_va (first_element, &args, NULL);
#endif
  va_end (args);

  return str;
}

#define KILOBYTE_FACTOR 1024.0
#define MEGABYTE_FACTOR (1024.0 * 1024.0)
#define GIGABYTE_FACTOR (1024.0 * 1024.0 * 1024.0)

/**
 * g_format_size_for_display:
 * @size: a size in bytes.
 * 
 * Formats a size (for example the size of a file) into a human readable string.
 * Sizes are rounded to the nearest size prefix (KB, MB, GB) and are displayed 
 * rounded to the nearest  tenth. E.g. the file size 3292528 bytes will be
 * converted into the string "3.1 MB".
 *
 * The prefix units base is 1024 (i.e. 1 KB is 1024 bytes).
 *
 * This string should be freed with g_free() when not needed any longer.
 *
 * Returns: a newly-allocated formatted string containing a human readable
 *          file size.
 *
 * Since: 2.16
 **/
char *
g_format_size_for_display (goffset size)
{
  if (size < (goffset) KILOBYTE_FACTOR)
    return g_strdup_printf (g_dngettext(GETTEXT_PACKAGE, "%u byte", "%u bytes",(guint) size), (guint) size);
  else
    {
      gdouble displayed_size;
      
      if (size < (goffset) MEGABYTE_FACTOR)
	{
	  displayed_size = (gdouble) size / KILOBYTE_FACTOR;
	  return g_strdup_printf (_("%.1f KB"), displayed_size);
	}
      else if (size < (goffset) GIGABYTE_FACTOR)
	{
	  displayed_size = (gdouble) size / MEGABYTE_FACTOR;
	  return g_strdup_printf (_("%.1f MB"), displayed_size);
	}
      else
	{
	  displayed_size = (gdouble) size / GIGABYTE_FACTOR;
	  return g_strdup_printf (_("%.1f GB"), displayed_size);
	}
    }
}


/**
 * g_file_read_link:
 * @filename: the symbolic link
 * @error: return location for a #GError
 *
 * Reads the contents of the symbolic link @filename like the POSIX
 * readlink() function.  The returned string is in the encoding used
 * for filenames. Use g_filename_to_utf8() to convert it to UTF-8.
 *
 * Returns: A newly-allocated string with the contents of the symbolic link, 
 *          or %NULL if an error occurred.
 *
 * Since: 2.4
 */
gchar *
g_file_read_link (const gchar  *filename,
	          GError      **error)
{
#ifdef HAVE_READLINK
  gchar *buffer;
  guint size;
  gint read_size;    
  
  size = 256; 
  buffer = g_malloc (size);
  
  while (TRUE) 
    {
      read_size = readlink (filename, buffer, size);
      if (read_size < 0) {
	int save_errno = errno;
	gchar *display_filename = g_filename_display_name (filename);

	g_free (buffer);
	g_set_error (error,
		     G_FILE_ERROR,
		     g_file_error_from_errno (save_errno),
		     _("Failed to read the symbolic link '%s': %s"),
		     display_filename, 
		     g_strerror (save_errno));
	g_free (display_filename);
	
	return NULL;
      }
    
      if (read_size < size) 
	{
	  buffer[read_size] = 0;
	  return buffer;
	}
      
      size *= 2;
      buffer = g_realloc (buffer, size);
    }
#else
  g_set_error_literal (error,
                       G_FILE_ERROR,
                       G_FILE_ERROR_INVAL,
                       _("Symbolic links not supported"));
	
  return NULL;
#endif
}

/* NOTE : Keep this part last to ensure nothing in this file uses the
 * below binary compatibility versions.
 */
#if defined (G_OS_WIN32) && !defined (_WIN64)

/* Binary compatibility versions. Will be called by code compiled
 * against quite old (pre-2.8, I think) headers only, not from more
 * recently compiled code.
 */

#undef g_file_test

gboolean
g_file_test (const gchar *filename,
             GFileTest    test)
{
  gchar *utf8_filename = g_locale_to_utf8 (filename, -1, NULL, NULL, NULL);
  gboolean retval;

  if (utf8_filename == NULL)
    return FALSE;

  retval = g_file_test_utf8 (utf8_filename, test);

  g_free (utf8_filename);

  return retval;
}

#undef g_file_get_contents

gboolean
g_file_get_contents (const gchar  *filename,
                     gchar       **contents,
                     gsize        *length,
                     GError      **error)
{
  gchar *utf8_filename = g_locale_to_utf8 (filename, -1, NULL, NULL, error);
  gboolean retval;

  if (utf8_filename == NULL)
    return FALSE;

  retval = g_file_get_contents_utf8 (utf8_filename, contents, length, error);

  g_free (utf8_filename);

  return retval;
}

#undef g_mkstemp

gint
g_mkstemp (gchar *tmpl)
{
  char *XXXXXX;
  int count, fd;
  static const char letters[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  static const int NLETTERS = sizeof (letters) - 1;
  glong value;
  GTimeVal tv;
  static int counter = 0;

  /* find the last occurrence of 'XXXXXX' */
  XXXXXX = g_strrstr (tmpl, "XXXXXX");

  if (!XXXXXX)
    {
      errno = EINVAL;
      return -1;
    }

  /* Get some more or less random data.  */
  g_get_current_time (&tv);
  value = (tv.tv_usec ^ tv.tv_sec) + counter++;

  for (count = 0; count < 100; value += 7777, ++count)
    {
      glong v = value;

      /* Fill in the random bits.  */
      XXXXXX[0] = letters[v % NLETTERS];
      v /= NLETTERS;
      XXXXXX[1] = letters[v % NLETTERS];
      v /= NLETTERS;
      XXXXXX[2] = letters[v % NLETTERS];
      v /= NLETTERS;
      XXXXXX[3] = letters[v % NLETTERS];
      v /= NLETTERS;
      XXXXXX[4] = letters[v % NLETTERS];
      v /= NLETTERS;
      XXXXXX[5] = letters[v % NLETTERS];

      /* This is the backward compatibility system codepage version,
       * thus use normal open().
       */
      fd = open (tmpl, O_RDWR | O_CREAT | O_EXCL | O_BINARY, 0600);

      if (fd >= 0)
	return fd;
      else if (errno != EEXIST)
	/* Any other error will apply also to other names we might
	 *  try, and there are 2^32 or so of them, so give up now.
	 */
	return -1;
    }

  /* We got out of the loop because we ran out of combinations to try.  */
  errno = EEXIST;
  return -1;
}

#undef g_file_open_tmp

gint
g_file_open_tmp (const gchar  *tmpl,
		 gchar       **name_used,
		 GError      **error)
{
  gchar *utf8_tmpl = g_locale_to_utf8 (tmpl, -1, NULL, NULL, error);
  gchar *utf8_name_used;
  gint retval;

  if (utf8_tmpl == NULL)
    return -1;

  retval = g_file_open_tmp_utf8 (utf8_tmpl, &utf8_name_used, error);
  
  if (retval == -1)
    return -1;

  if (name_used)
    *name_used = g_locale_from_utf8 (utf8_name_used, -1, NULL, NULL, NULL);

  g_free (utf8_name_used);

  return retval;
}

#endif

#define __G_FILEUTILS_C__
#include "galiasdef.c"
