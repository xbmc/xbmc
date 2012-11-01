#ifndef __OSXPLATFORM_H__
#define __OSXPLATFORM_H__
/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

// This is a catch all for GNU routines that do not exist under OSX.
#include <netdb.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

size_t strnlen(const char *s, size_t n);
char* strndup(char const *s, size_t n);
int gethostbyname_r(const char *name, struct hostent *ret, char *buf,
    size_t buflen, struct hostent **result, int *h_errnop);

#ifdef __cplusplus
}
#endif /* __cplusplus */
    
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
ssize_t getline (char **lineptr, size_t *n, FILE *stream);
int strverscmp (const char *s1, const char *s2);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
