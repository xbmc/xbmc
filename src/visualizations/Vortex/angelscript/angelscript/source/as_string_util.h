/*
   AngelCode Scripting Library
   Copyright (c) 2003-2009 Andreas Jonsson

   This software is provided 'as-is', without any express or implied 
   warranty. In no event will the authors be held liable for any 
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any 
   purpose, including commercial applications, and to alter it and 
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you 
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product 
      documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and 
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source 
      distribution.

   The original version of this library can be located at:
   http://www.angelcode.com/angelscript/

   Andreas Jonsson
   andreas@angelcode.com
*/


#ifndef AS_STRING_UTIL_H
#define AS_STRING_UTIL_H

#include "as_config.h"

BEGIN_AS_NAMESPACE

double  asStringScanDouble(const char *string, size_t *numScanned);
asQWORD asStringScanUInt64(const char *string, int base, size_t *numScanned);

int     asStringEncodeUTF8(unsigned int value, char *outEncodedBuffer);
int     asStringDecodeUTF8(const char *encodedBuffer, unsigned int *outLength);

int     asStringEncodeUTF16(unsigned int value, char *outEncodedBuffer);

END_AS_NAMESPACE

#endif
