/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2010 Live Networks, Inc.  All rights reserved.
// Support for temporarily setting the locale (e.g., to POSIX) for (e.g.) parsing or printing
// floating-point numbers in protocol headers, or calling toupper()/tolower() on human-input strings.
// C++ header

#ifndef _LOCALE_HH
#define _LOCALE_HH

// If you're on a system that (for whatever reason) doesn't have the "setlocale()" function, then
// add "-DLOCALE_NOT_USED" to your "config.*" file.

#ifndef LOCALE_NOT_USED
#include <locale.h>
#else
#ifndef LC_ALL
#define LC_ALL 0
#endif
#ifndef LC_NUMERIC
#define LC_NUMERIC 4
#endif
#endif

class Locale {
public:
  Locale(char const* newLocale, int category = LC_ALL);
  virtual ~Locale();

private:
  int fCategory;
  char* fPrevLocale;
};

#endif
