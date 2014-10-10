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

#include <stdarg.h>		// va_list, va_start(), etc
#include <stdlib.h>     // strtod(), strtol()
#include <string.h> // some compilers declare memcpy() here

#if !defined(AS_NO_MEMORY_H)
#include <memory.h>
#endif

#include "as_string.h"

asCString::asCString()
{
	length = 0;
	local[0] = 0;

//	Assign("", 0);
}

// Copy constructor
asCString::asCString(const asCString &str)
{
	length = 0;
	local[0] = 0;

	Assign(str.AddressOf(), str.length);
}

asCString::asCString(const char *str, size_t len)
{
	length = 0;
	local[0] = 0;

	Assign(str, len);
}

asCString::asCString(const char *str)
{
	length = 0;
	local[0] = 0;

	size_t len = strlen(str);
	Assign(str, len);
}

asCString::asCString(char ch)
{
	length = 0;
	local[0] = 0;

	Assign(&ch, 1);
}

asCString::~asCString()
{
	if( length > 11 && dynamic )
	{
		asDELETEARRAY(dynamic);
	}
}

char *asCString::AddressOf()
{
	if( length <= 11 )
		return local;
	else
		return dynamic;
}

const char *asCString::AddressOf() const
{
	if( length <= 11 )
		return local;
	else
		return dynamic;
}

void asCString::SetLength(size_t len)
{
	Allocate(len, true);
}

void asCString::Allocate(size_t len, bool keepData)
{
	if( len > 11 )
	{
		char *buf = asNEWARRAY(char,len+1);

		if( keepData )
		{
			int l = (int)len < (int)length ? (int)len : (int)length;
			memcpy(buf, AddressOf(), l);
		}

		if( length > 11 )
		{
			asDELETEARRAY(dynamic);
		}

		dynamic = buf;
	}
	else
	{
		if( length > 11 )
		{
			if( keepData )
			{
				memcpy(&local, dynamic, len);
			}
			asDELETEARRAY(dynamic);
		}
	}

	length = (int)len;

	// Make sure the buffer is null terminated
	AddressOf()[length] = 0;
}

void asCString::Assign(const char *str, size_t len)
{
	Allocate(len, false);

	// Copy the string
	memcpy(AddressOf(), str, length);
	AddressOf()[length] = 0;
}

asCString &asCString::operator =(const char *str)
{
	size_t len = str ? strlen(str) : 0;
	Assign(str, len);

	return *this;
}

asCString &asCString::operator =(const asCString &str)
{
	Assign(str.AddressOf(), str.length);

	return *this;
}

asCString &asCString::operator =(char ch)
{
	Assign(&ch, 1);

	return *this;
}

void asCString::Concatenate(const char *str, size_t len)
{
	asUINT oldLength = length;
	SetLength(length + len);

	memcpy(AddressOf() + oldLength, str, len);
	AddressOf()[length] = 0;
}

asCString &asCString::operator +=(const char *str)
{
	size_t len = strlen(str);
	Concatenate(str, len);

	return *this;
}

asCString &asCString::operator +=(const asCString &str)
{
	Concatenate(str.AddressOf(), str.length);

	return *this;
}

asCString &asCString::operator +=(char ch)
{
	Concatenate(&ch, 1);

	return *this;
}

size_t asCString::GetLength() const
{
	return length;
}

// Returns the length
size_t asCString::Format(const char *format, ...)
{
	va_list args;
	va_start(args, format);

	char tmp[256];
	int r = asVSNPRINTF(tmp, 255, format, args);

	if( r > 0 )
	{
		Assign(tmp, r);
	}
	else
	{
		size_t n = 512;
		asCString str; // Use temporary string in case the current buffer is a parameter
		str.Allocate(n, false);

		while( (r = asVSNPRINTF(str.AddressOf(), n, format, args)) < 0 )
		{
			n *= 2;
			str.Allocate(n, false);
		}

		Assign(str.AddressOf(), r);
	}

	va_end(args);

	return length;
}

char &asCString::operator [](size_t index) 
{
	asASSERT(index < length);

	return AddressOf()[index];
}

const char &asCString::operator [](size_t index) const
{
	asASSERT(index < length);

	return AddressOf()[index];
}

asCString asCString::SubString(size_t start, size_t length) const
{
	if( start >= GetLength() || length == 0 )
		return asCString("");

	if( length == (size_t)(-1) ) length = GetLength() - start;

	asCString tmp;
	tmp.Assign(AddressOf() + start, length);

	return tmp;
}

int asCString::Compare(const char *str) const
{
	return Compare(str, strlen(str));
}

int asCString::Compare(const asCString &str) const
{
	return Compare(str.AddressOf(), str.GetLength());
}

int asCString::Compare(const char *str, size_t len) const
{
	if( length == 0 ) 
	{
		if( str == 0 || len == 0 ) return 0; // Equal

		return 1; // The other string is larger than this
	}

	if( str == 0 )
	{
		if( length == 0 ) 
			return 0; // Equal

		return -1; // The other string is smaller than this
	}

	if( len < length )
	{
		int result = memcmp(AddressOf(), str, len);
		if( result == 0 ) return -1; // The other string is smaller than this

		return result;
	}

	int result = memcmp(AddressOf(), str, length);
	if( result == 0 && length < len ) return 1; // The other string is larger than this

	return result;
}

size_t asCString::RecalculateLength()
{
	SetLength(strlen(AddressOf()));

	return length;
}

//-----------------------------------------------------------------------------
// Helper functions

bool operator ==(const asCString &a, const char *b)
{
	return a.Compare(b) == 0;
}

bool operator !=(const asCString &a, const char *b)
{
	return a.Compare(b) != 0;
}

bool operator ==(const asCString &a, const asCString &b)
{
	return a.Compare(b) == 0;
}

bool operator !=(const asCString &a, const asCString &b)
{
	return a.Compare(b) != 0;
}

bool operator ==(const char *a, const asCString &b)
{
	return b.Compare(a) == 0;
}

bool operator !=(const char *a, const asCString &b)
{
	return b.Compare(a) != 0;
}

bool operator <(const asCString &a, const asCString &b)
{
	return a.Compare(b) < 0;
}

asCString operator +(const asCString &a, const asCString &b)
{
	asCString res = a;
	res += b;

	return res;
}

asCString operator +(const char *a, const asCString &b)
{
	asCString res = a;
	res += b;

	return res;
}

asCString operator +(const asCString &a, const char *b)
{
	asCString res = a;
	res += b;

	return res;
}

