#ifndef _FSTRCMP_H
#define _FSTRCMP_H

  /* GNU gettext - internationalization aids
  Copyright (C) 1995 Free Software Foundation, Inc.

  This file was written by Peter Miller <pmiller@agso.gov.au>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XBMC; see the file COPYING.  If not, see
<http://www.gnu.org/licenses/>.
*/
#define PARAMS(proto) proto

#ifdef __cplusplus
extern "C"
{
#endif

double fstrcmp (const char *__s1, const char *__s2, double __minimum);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
