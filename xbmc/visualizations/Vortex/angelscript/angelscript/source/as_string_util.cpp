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

#include "as_config.h"

#include <stdarg.h>     // va_list, va_start(), etc
#include <stdlib.h>     // strtod(), strtol()
#include <stdio.h>      // _vsnprintf()
#include <string.h>     // some compilers declare memcpy() here
#include <locale.h>     // setlocale()

#if !defined(AS_NO_MEMORY_H)
#include <memory.h>
#endif

#include "as_string.h"
#include "as_string_util.h"

BEGIN_AS_NAMESPACE

double asStringScanDouble(const char *string, size_t *numScanned)
{
	char *end;

    // WinCE doesn't have setlocale. Some quick testing on my current platform
    // still manages to parse the numbers such as "3.14" even if the decimal for the
    // locale is ",".
#if !defined(_WIN32_WCE) && !defined(ANDROID)
	// Set the locale to C so that we are guaranteed to parse the float value correctly
	asCString orig = setlocale(LC_NUMERIC, 0);
	setlocale(LC_NUMERIC, "C");
#endif

	double res = strtod(string, &end);

#if !defined(_WIN32_WCE) && !defined(ANDROID)
	// Restore the locale
	setlocale(LC_NUMERIC, orig.AddressOf());
#endif

	if( numScanned )
		*numScanned = end - string;

	return res;
}

asQWORD asStringScanUInt64(const char *string, int base, size_t *numScanned)
{
	asASSERT(base == 10 || base == 16);

	const char *end = string;

	asQWORD res = 0;
	if( base == 10 )
	{
		while( *end >= '0' && *end <= '9' )
		{
			res *= 10;
			res += *end++ - '0';
		}
	}
	else if( base == 16 )
	{
		while( (*end >= '0' && *end <= '9') ||
		       (*end >= 'a' && *end <= 'f') ||
		       (*end >= 'A' && *end <= 'F') )
		{
			res *= 16;
			if( *end >= '0' && *end <= '9' )
				res += *end++ - '0';
			else if( *end >= 'a' && *end <= 'f' )
				res += *end++ - 'a' + 10;
			else if( *end >= 'A' && *end <= 'F' )
				res += *end++ - 'A' + 10;
		}
	}

	if( numScanned )
		*numScanned = end - string;

	return res;
}

//
// The function will encode the unicode code point into the outEncodedBuffer, and then
// return the length of the encoded value. If the input value is not a valid unicode code 
// point, then the function will return -1.
//
// This function is taken from the AngelCode ToolBox.
//
int asStringEncodeUTF8(unsigned int value, char *outEncodedBuffer)
{
	unsigned char *buf = (unsigned char*)outEncodedBuffer;

	int length = -1;

	if( value <= 0x7F )
	{
		buf[0] = static_cast<unsigned char>(value);
		return 1;
	}
	else if( value >= 0x80 && value <= 0x7FF )
	{
		// Encode it with 2 characters
		buf[0] = static_cast<unsigned char>(0xC0 + (value >> 6));
		length = 2;
	}
	else if( (value >= 0x800 && value <= 0xD7FF) || (value >= 0xE000 && value <= 0xFFFF) )
	{
		// Note: Values 0xD800 to 0xDFFF are not valid unicode characters
		buf[0] = static_cast<unsigned char>(0xE0 + (value >> 12));
		length = 3;
	}
	else if( value >= 0x10000 && value <= 0x10FFFF )
	{
		buf[0] = static_cast<unsigned char>(0xF0 + (value >> 18));
		length = 4;
	}

	int n = length-1;
	for( ; n > 0; n-- )
	{
		buf[n] = static_cast<unsigned char>(0x80 + (value & 0x3F));
		value >>= 6;
	}

	return length;
}

//
// The function will decode an UTF8 character and return the unicode code point.
// outLength will receive the number of bytes that were decoded.
//
// This function is taken from the AngelCode ToolBox.
//
int asStringDecodeUTF8(const char *encodedBuffer, unsigned int *outLength)
{
	const unsigned char *buf = (const unsigned char*)encodedBuffer;
	
	int value = 0;
	int length = -1;
	unsigned char byte = buf[0];
	if( (byte & 0x80) == 0 )
	{
		// This is the only byte
		if( outLength ) *outLength = 1;
		return byte;
	}
	else if( (byte & 0xE0) == 0xC0 )
	{
		// There is one more byte
		value = int(byte & 0x1F);
		length = 2;

		// The value at this moment must not be less than 2, because 
		// that should have been encoded with one byte only.
		if( value < 2 )
			length = -1;
	}
	else if( (byte & 0xF0) == 0xE0 )
	{
		// There are two more bytes
		value = int(byte & 0x0F);
		length = 3;
	}
	else if( (byte & 0xF8) == 0xF0 )
	{
		// There are three more bytes
		value = int(byte & 0x07);
		length = 4;
	}

	int n = 1;
	for( ; n < length; n++ )
	{
		byte = buf[n];
		if( (byte & 0xC0) == 0x80 )
			value = (value << 6) + int(byte & 0x3F);
		else 
			break;
	}

	if( n == length )
	{
		if( outLength ) *outLength = (unsigned)length;
		return value;
	}

	// The byte sequence isn't a valid UTF-8 byte sequence.
	return -1;
}

//
// The function will encode the unicode code point into the outEncodedBuffer, and then
// return the length of the encoded value. If the input value is not a valid unicode code 
// point, then the function will return -1.
//
// This function is taken from the AngelCode ToolBox.
//
int asStringEncodeUTF16(unsigned int value, char *outEncodedBuffer)
{
	if( value < 0x10000 )
	{
#ifndef AS_BIG_ENDIAN
		outEncodedBuffer[0] = (value & 0xFF);
		outEncodedBuffer[1] = ((value >> 8) & 0xFF);
#else
		outEncodedBuffer[1] = (value & 0xFF);
		outEncodedBuffer[0] = ((value >> 8) & 0xFF);
#endif
		return 2;
	}
	else
	{
		value -= 0x10000;
		int surrogate1 = ((value >> 10) & 0x3FF) + 0xD800;
		int surrogate2 = (value & 0x3FF) + 0xDC00;

#ifndef AS_BIG_ENDIAN
		outEncodedBuffer[0] = (surrogate1 & 0xFF);
		outEncodedBuffer[1] = ((surrogate1 >> 8) & 0xFF);
		outEncodedBuffer[2] = (surrogate2 & 0xFF);
		outEncodedBuffer[3] = ((surrogate2 >> 8) & 0xFF);
#else
		outEncodedBuffer[1] = (surrogate1 & 0xFF);
		outEncodedBuffer[0] = ((surrogate1 >> 8) & 0xFF);
		outEncodedBuffer[3] = (surrogate2 & 0xFF);
		outEncodedBuffer[2] = ((surrogate2 >> 8) & 0xFF);
#endif

		return 4;
	}
}


END_AS_NAMESPACE
