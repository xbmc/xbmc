#ifdef __APPLE__
#ifndef _GET_DELIM_H_
#define _GET_DELIM_H_
/* getdelim.h --- Prototype for replacement getdelim function.
   Copyright (C) 2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.  */

/* Written by Simon Josefsson. */

/* Get size_t, FILE, ssize_t.  And getdelim, if available.  */
# include <stddef.h>
# include <stdio.h>
# include <sys/types.h>

extern "C" ssize_t getdelim(char **lineptr, size_t *n, int delimiter, FILE *stream);
#endif
#endif