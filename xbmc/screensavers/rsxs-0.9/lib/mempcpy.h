/* Copy memory area and return pointer after last written byte.
   Copyright (C) 2003, 2004 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef mempcpy

# if HAVE_MEMPCPY

/* Get mempcpy() declaration.  */
#  include <string.h>

# else

/* Get size_t */
#  include <stddef.h>

/* Copy N bytes of SRC to DEST, return pointer to bytes after the
   last written byte.  */
extern void *mempcpy (void *dest, const void *src, size_t n);

# endif

#endif
