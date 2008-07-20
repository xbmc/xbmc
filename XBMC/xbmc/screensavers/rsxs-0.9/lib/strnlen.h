/* Find the length of STRING, but scan at most MAXLEN characters.
   Copyright (C) 2005 Free Software Foundation, Inc.
   Written by Simon Josefsson.

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

#ifndef STRNLEN_H
#define STRNLEN_H

/* Get strnlen declaration, if available.  */
#include <string.h>

#if defined HAVE_DECL_STRNLEN && !HAVE_DECL_STRNLEN
/* Find the length (number of bytes) of STRING, but scan at most
   MAXLEN bytes.  If no '\0' terminator is found in that many bytes,
   return MAXLEN.  */
extern size_t strnlen(const char *string, size_t maxlen);
#endif

#endif /* STRNLEN_H */
