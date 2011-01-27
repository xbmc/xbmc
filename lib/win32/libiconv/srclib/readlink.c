/* Stub for readlink().
   Copyright (C) 2003-2007 Free Software Foundation, Inc.

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

/* Specification.  */
#include <unistd.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stddef.h>

#if !HAVE_READLINK

/* readlink() substitute for systems that don't have a readlink() function,
   such as DJGPP 2.03 and mingw32.  */

/* The official POSIX return type of readlink() is ssize_t, but since here
   we have no declaration in a public header file, we use 'int' as return
   type.  */

int
readlink (const char *path, char *buf, size_t bufsize)
{
  struct stat statbuf;

  /* In general we should use lstat() here, not stat().  But on platforms
     without symbolic links lstat() - if it exists - would be equivalent to
     stat(), therefore we can use stat().  This saves us a configure check.  */
  if (stat (path, &statbuf) >= 0)
    errno = EINVAL;
  return -1;
}

#endif
