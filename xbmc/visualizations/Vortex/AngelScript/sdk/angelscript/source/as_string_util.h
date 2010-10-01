/*
   AngelCode Scripting Library
   Copyright (c) 2003-2006 Andreas Jönsson

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

   Andreas Jönsson
   andreas@angelcode.com
*/


#ifndef AS_STRING_UTIL_H
#define AS_STRING_UTIL_H

typedef unsigned int acUINT;

int    asStringFormat(char *string, int maxLength, const char *format, ...);
double asStringScanDouble(const char *string, int *numScanned);
int    asStringScanInt(const char *string, int base, int *numScanned);
acUINT asStringScanUInt(const char *string, int base, int *numScanned);
acUINT asStringHash(const char *string);

void   asStringCopy(const char *source, int srcLength, char *dest, int destLength);

#define GETSTRING(name,src,len) \
  asCString name; \
  name.Assign((src), (len));

#endif
