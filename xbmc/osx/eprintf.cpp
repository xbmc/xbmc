/*
   Copyright (C) 1991, 1994-2007, 2008, 2009 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

/* __eprintf shouldn't have been made visible from libstdc++, or
+   anywhere, but on Mac OS X 10.4 it was defined in
+   libstdc++.6.0.3.dylib; so on that platform we have to keep defining
+   it to keep binary compatibility.  We can't just put the libgcc
+   version in the export list, because that doesn't work; once a
+   symbol is marked as hidden, it stays that way.  */
#if defined(__APPLE__) && (MAC_OS_X_VERSION_MAX_ALLOWED < 1050)
#include <cstdio>
#include <cstdlib>

using namespace std;

extern "C" void __attribute__ ((weak))
__eprintf(const char *string, const char *expression,
          unsigned int line, const char *filename)
  {
    fprintf(stderr, string, expression, line, filename);
    fflush(stderr);
    abort();
  }

#endif
