/* Return the canonical absolute name of a given file.
   Copyright (C) 1996-2003, 2005-2008 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>

/* Avoid a clash of our rpl_realpath() function with the prototype in
   <stdlib.h> on Solaris 2.5.1.  */
#undef realpath

#if !HAVE_CANONICALIZE_FILE_NAME || defined _LIBC

#include <alloca.h>

/* Specification.  */
#include "canonicalize.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#if HAVE_UNISTD_H || defined _LIBC
# include <unistd.h>
#endif

#include <limits.h>

#if HAVE_SYS_PARAM_H || defined _LIBC
# include <sys/param.h>
#endif
#ifndef MAXSYMLINKS
# define MAXSYMLINKS 20
#endif

#include <sys/stat.h>

#include <errno.h>
#ifndef _LIBC
# define __set_errno(e) errno = (e)
# ifndef ENAMETOOLONG
#  define ENAMETOOLONG EINVAL
# endif
#endif

#ifdef _LIBC
# include <shlib-compat.h>
#else
# define SHLIB_COMPAT(lib, introduced, obsoleted) 0
# define versioned_symbol(lib, local, symbol, version)
# define compat_symbol(lib, local, symbol, version)
# define weak_alias(local, symbol)
# define __canonicalize_file_name canonicalize_file_name
# define __realpath rpl_realpath
# include "pathmax.h"
# include "malloca.h"
# if HAVE_GETCWD
#  ifdef VMS
    /* We want the directory in Unix syntax, not in VMS syntax.  */
#   define __getcwd(buf, max) getcwd (buf, max, 0)
#  else
#   define __getcwd getcwd
#  endif
# else
#  define __getcwd(buf, max) getwd (buf)
# endif
# define __readlink readlink
  /* On systems without symbolic links, call stat() instead of lstat().  */
# if !defined S_ISLNK && !HAVE_READLINK
#  define lstat stat
# endif
#endif

/* Return the canonical absolute name of file NAME.  A canonical name
   does not contain any `.', `..' components nor any repeated path
   separators ('/') or symlinks.  All path components must exist.  If
   RESOLVED is null, the result is malloc'd; otherwise, if the
   canonical name is PATH_MAX chars or more, returns null with `errno'
   set to ENAMETOOLONG; if the name fits in fewer than PATH_MAX chars,
   returns the name in RESOLVED.  If the name cannot be resolved and
   RESOLVED is non-NULL, it contains the path of the first component
   that cannot be resolved.  If the path can be resolved, RESOLVED
   holds the same value as the value returned.  */

char *
__realpath (const char *name, char *resolved)
{
  char *rpath, *dest, *extra_buf = NULL;
  const char *start, *end, *rpath_limit;
  long int path_max;
#if HAVE_READLINK
  int num_links = 0;
#endif

  if (name == NULL)
    {
      /* As per Single Unix Specification V2 we must return an error if
	 either parameter is a null pointer.  We extend this to allow
	 the RESOLVED parameter to be NULL in case the we are expected to
	 allocate the room for the return value.  */
      __set_errno (EINVAL);
      return NULL;
    }

  if (name[0] == '\0')
    {
      /* As per Single Unix Specification V2 we must return an error if
	 the name argument points to an empty string.  */
      __set_errno (ENOENT);
      return NULL;
    }

#ifdef PATH_MAX
  path_max = PATH_MAX;
#else
  path_max = pathconf (name, _PC_PATH_MAX);
  if (path_max <= 0)
    path_max = 1024;
#endif

  if (resolved == NULL)
    {
      rpath = malloc (path_max);
      if (rpath == NULL)
	{
	  /* It's easier to set errno to ENOMEM than to rely on the
	     'malloc-posix' gnulib module.  */
	  errno = ENOMEM;
	  return NULL;
	}
    }
  else
    rpath = resolved;
  rpath_limit = rpath + path_max;

  if (name[0] != '/')
    {
      if (!__getcwd (rpath, path_max))
	{
	  rpath[0] = '\0';
	  goto error;
	}
      dest = strchr (rpath, '\0');
    }
  else
    {
      rpath[0] = '/';
      dest = rpath + 1;
    }

  for (start = end = name; *start; start = end)
    {
#ifdef _LIBC
      struct stat64 st;
#else
      struct stat st;
#endif

      /* Skip sequence of multiple path-separators.  */
      while (*start == '/')
	++start;

      /* Find end of path component.  */
      for (end = start; *end && *end != '/'; ++end)
	/* Nothing.  */;

      if (end - start == 0)
	break;
      else if (end - start == 1 && start[0] == '.')
	/* nothing */;
      else if (end - start == 2 && start[0] == '.' && start[1] == '.')
	{
	  /* Back up to previous component, ignore if at root already.  */
	  if (dest > rpath + 1)
	    while ((--dest)[-1] != '/');
	}
      else
	{
	  size_t new_size;

	  if (dest[-1] != '/')
	    *dest++ = '/';

	  if (dest + (end - start) >= rpath_limit)
	    {
	      ptrdiff_t dest_offset = dest - rpath;
	      char *new_rpath;

	      if (resolved)
		{
		  __set_errno (ENAMETOOLONG);
		  if (dest > rpath + 1)
		    dest--;
		  *dest = '\0';
		  goto error;
		}
	      new_size = rpath_limit - rpath;
	      if (end - start + 1 > path_max)
		new_size += end - start + 1;
	      else
		new_size += path_max;
	      new_rpath = (char *) realloc (rpath, new_size);
	      if (new_rpath == NULL)
		{
		  /* It's easier to set errno to ENOMEM than to rely on the
		     'realloc-posix' gnulib module.  */
		  errno = ENOMEM;
		  goto error;
		}
	      rpath = new_rpath;
	      rpath_limit = rpath + new_size;

	      dest = rpath + dest_offset;
	    }

#ifdef _LIBC
	  dest = __mempcpy (dest, start, end - start);
#else
	  memcpy (dest, start, end - start);
	  dest += end - start;
#endif
	  *dest = '\0';

#ifdef _LIBC
	  if (__lxstat64 (_STAT_VER, rpath, &st) < 0)
#else
	  if (lstat (rpath, &st) < 0)
#endif
	    goto error;

#if HAVE_READLINK
	  if (S_ISLNK (st.st_mode))
	    {
	      char *buf;
	      size_t len;
	      int n;

	      if (++num_links > MAXSYMLINKS)
		{
		  __set_errno (ELOOP);
		  goto error;
		}

	      buf = malloca (path_max);
	      if (!buf)
		{
		  errno = ENOMEM;
		  goto error;
		}

	      n = __readlink (rpath, buf, path_max - 1);
	      if (n < 0)
		{
		  int saved_errno = errno;
		  freea (buf);
		  errno = saved_errno;
		  goto error;
		}
	      buf[n] = '\0';

	      if (!extra_buf)
		{
		  extra_buf = malloca (path_max);
		  if (!extra_buf)
		    {
		      freea (buf);
		      errno = ENOMEM;
		      goto error;
		    }
		}

	      len = strlen (end);
	      if ((long int) (n + len) >= path_max)
		{
		  freea (buf);
		  __set_errno (ENAMETOOLONG);
		  goto error;
		}

	      /* Careful here, end may be a pointer into extra_buf... */
	      memmove (&extra_buf[n], end, len + 1);
	      name = end = memcpy (extra_buf, buf, n);

	      if (buf[0] == '/')
		dest = rpath + 1;	/* It's an absolute symlink */
	      else
		/* Back up to previous component, ignore if at root already: */
		if (dest > rpath + 1)
		  while ((--dest)[-1] != '/');
	    }
#endif
	}
    }
  if (dest > rpath + 1 && dest[-1] == '/')
    --dest;
  *dest = '\0';

  if (extra_buf)
    freea (extra_buf);

  return resolved ? memcpy (resolved, rpath, dest - rpath + 1) : rpath;

error:
  {
    int saved_errno = errno;
    if (extra_buf)
      freea (extra_buf);
    if (resolved)
      strcpy (resolved, rpath);
    else
      free (rpath);
    errno = saved_errno;
  }
  return NULL;
}
#ifdef _LIBC
versioned_symbol (libc, __realpath, realpath, GLIBC_2_3);
#endif


#if SHLIB_COMPAT(libc, GLIBC_2_0, GLIBC_2_3)
char *
__old_realpath (const char *name, char *resolved)
{
  if (resolved == NULL)
    {
      __set_errno (EINVAL);
      return NULL;
    }

  return __realpath (name, resolved);
}
compat_symbol (libc, __old_realpath, realpath, GLIBC_2_0);
#endif


char *
__canonicalize_file_name (const char *name)
{
  return __realpath (name, NULL);
}
weak_alias (__canonicalize_file_name, canonicalize_file_name)

#else

/* This declaration is solely to ensure that after preprocessing
   this file is never empty.  */
typedef int dummy;

#endif
