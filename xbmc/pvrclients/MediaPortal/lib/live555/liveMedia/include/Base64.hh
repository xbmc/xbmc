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
// Base64 encoding and decoding
// C++ header

#ifndef _BASE64_HH
#define _BASE64_HH

#ifndef _BOOLEAN_HH
#include "Boolean.hh"
#endif

unsigned char* base64Decode(char* in, unsigned& resultSize,
			    Boolean trimTrailingZeros = True);
    // returns a newly allocated array - of size "resultSize" - that
    // the caller is responsible for delete[]ing.

char* base64Encode(char const* orig, unsigned origLength);
    // returns a 0-terminated string that
    // the caller is responsible for delete[]ing.

#endif
