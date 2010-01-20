/* areadlink.c -- readlink wrapper to return the link name in malloc'd storage
   Unlike xreadlink and xreadlink_with_size, don't ever call exit.

   Copyright (C) 2001, 2003-2007 Free Software Foundation, Inc.

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

/* Written by Jim Meyering <jim@meyering.net>
   and Bruno Haible <bruno@clisp.org>.  */

#include <config.h>

/* Specification.  */
#include "areadlink.h"

#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef SIZE_MAX
# define SIZE_MAX ((size_t) -1)
#endif
#ifndef SSIZE_MAX
# define SSIZE_MAX ((ssize_t) (SIZE_MAX / 2))
#endif

/* Call readlink to get the symbolic link value of FILENAME.
   Return a pointer to that NUL-terminated string in malloc'd storage.
   If readlink fails, return NULL and set errno.
   If realloc fails, or if the link value is longer than SIZE_MAX :-),
   return NULL and set errno to ENOMEM.  */

char *
areadlink (char const *filename)
{
  /* The initial buffer size for the link value.  A power of 2
     detects arithmetic overflow earlier, but is not required.  */
#define INITIAL_BUF_SIZE 1024

  /* Allocate the initial buffer on the stack.  This way, in the common
     case of a symlink of small size, we get away with a single small malloc()
     instead of a big malloc() followed by a shrinking realloc().  */
  char initial_buf[INITIAL_BUF_SIZE];

  char *buffer = initial_buf;
  size_t buf_size = sizeof (initial_buf);

  while (1)
    {
      /* Attempt to read the link into the current buffer.  */
      ssize_t link_length = readlink (filename, buffer, buf_size);

      /* On AIX 5L v5.3 and HP-UX 11i v2 04/09, readlink returns -1
	 with errno == ERANGE if the buffer is too small.  */
      if (link_length < 0 && errno != ERANGE)
	{
	  if (buffer != initial_buf)
	    {
	      int saved_errno = errno;
	      free (buffer);
	      errno = saved_errno;
	    }
	  return NULL;
	}

      if ((size_t) link_length < buf_size)
	{
	  buffer[link_length++] = '\0';

	  /* Return it in a chunk of memory as small as possible.  */
	  if (buffer == initial_buf)
	    {
	      buffer = (char *) malloc (link_length);
	      if (buffer == NULL)
		/* errno is ENOMEM.  */
		return NULL;
	      memcpy (buffer, initial_buf, link_length);
	    }
	  else
	    {
	      /* Shrink buffer before returning it.  */
	      if ((size_t) link_length < buf_size)
		{
		  char *smaller_buffer = (char *) realloc (buffer, link_length);

		  if (smaller_buffer != NULL)
		    buffer = smaller_buffer;
		}
	    }
	  return buffer;
	}

      if (buffer != initial_buf)
	free (buffer);
      buf_size *= 2;
      if (SSIZE_MAX < buf_size || (SIZE_MAX / 2 < SSIZE_MAX && buf_size == 0))
	{
	  errno = ENOMEM;
	  return NULL;
	}
      buffer = (char *) malloc (buf_size);
      if (buffer == NULL)
	/* errno is ENOMEM.  */
	return NULL;
    }
}
