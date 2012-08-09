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
# include <stddef.h>
# include <stdio.h>
# include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

ssize_t getdelim(char **lineptr, size_t *n, int delimiter, FILE *stream);

#ifdef __cplusplus
}
#endif /* __cplusplus */
