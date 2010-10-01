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

#include <memory.h>
#include <stdarg.h>		// va_list, va_start(), etc
#include <stdlib.h>     // strtod(), strtol()
#include <assert.h>     // assert()

#include "as_config.h"
#include "as_string.h"

asCString::asCString()
{
	length = 0;
	bufferSize = 0;
	buffer = 0;

//	Assign("", 0);
}

// Copy constructor
asCString::asCString(const asCString &str)
{
	length = 0;
	bufferSize = 0;
	buffer = 0;

	Assign(str.buffer, str.length);
}

asCString::asCString(const char *str, asUINT len)
{
	length = 0;
	bufferSize = 0;
	buffer = 0;

	Assign(str, len);
}

asCString::asCString(const char *str)
{
	length = 0;
	bufferSize = 0;
	buffer = 0;

	int len = strlen(str);
	Assign(str, len);
}

asCString::asCString(char ch)
{
	length = 0;
	bufferSize = 0;
	buffer = 0;

	Assign(&ch, 1);
}

asCString::~asCString()
{
	if( buffer )
		delete[] buffer;
}

char *asCString::AddressOf()
{
	if( buffer == 0 ) Assign("", 0);
	return buffer;
}

const char *asCString::AddressOf() const
{
	if( buffer == 0 ) return "";
	return buffer;
}

void asCString::SetLength(asUINT len)
{
	if( len >= bufferSize )
		Allocate(len, true);

	buffer[len] = 0;
	length = len;
}

void asCString::Allocate(asUINT len, bool keepData)
{
	char *buf = new char[len+1];
	bufferSize = len+1;

	if( buffer )
	{
		if( keepData )
		{
			if( len < length )
				length = len;
			memcpy(buf, buffer, length);
		}

		delete[] buffer;
	}

	buffer = buf;

	// If no earlier data, set to empty string
	if( !keepData )
		length = 0;

	// Make sure the buffer is null terminated
	buffer[length] = 0;
}

void asCString::Assign(const char *str, asUINT len)
{
	// Allocate memory for the string
	if( bufferSize < len + 1 )
		Allocate(len, false);

	// Copy the string
	length = len;
	memcpy(buffer, str, length);
	buffer[length] = 0;
}

asCString &asCString::operator =(const char *str)
{
	int len = str ? strlen(str) : 0;
	Assign(str, len);

	return *this;
}

asCString &asCString::operator =(const asCString &str)
{
	Assign(str.buffer, str.length);

	return *this;
}

asCString &asCString::operator =(char ch)
{
	Assign(&ch, 1);

	return *this;
}

void asCString::Concatenate(const char *str, asUINT len)
{
	// Allocate memory for the string
	if( bufferSize < length + len + 1 )
		Allocate(length + len, true);

	memcpy(&buffer[length], str, len);
	length = length + len;
	buffer[length] = 0;
}

asCString &asCString::operator +=(const char *str)
{
	int len = strlen(str);
	Concatenate(str, len);

	return *this;
}

asCString &asCString::operator +=(const asCString &str)
{
	Concatenate(str.buffer, str.length);

	return *this;
}

asCString &asCString::operator +=(char ch)
{
	Concatenate(&ch, 1);

	return *this;
}

asUINT asCString::GetLength() const
{
	return length;
}

// Returns the length
int asCString::Format(const char *format, ...)
{
	va_list args;
	va_start(args, format);

	char tmp[256];
	int r = vsnprintf(tmp, 255, format, args);

	if( r > 0 )
	{
		Assign(tmp, r);
	}
	else
	{
		int n = 512;
		asCString str; // Use temporary string in case the current buffer is a parameter
		str.Allocate(n, false);

		while( (r = vsnprintf(str.buffer, n, format, args)) < 0 )
		{
			n *= 2;
			str.Allocate(n, false);
		}

		Assign(str.buffer, r);
	}

	va_end(args);

	return length;
}

char &asCString::operator [](asUINT index) 
{
	assert(index < length);

	return buffer[index];
}

const char &asCString::operator [](asUINT index) const
{
	assert(index < length);

	return buffer[index];
}

asCString asCString::SubString(asUINT start, int length) const
{
	if( start >= GetLength() || length == 0 )
		return asCString("");

	if( length == -1 ) length = GetLength() - start;

	asCString tmp;
	tmp.Assign(buffer + start, length);

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

int asCString::Compare(const char *str, asUINT len) const
{
	if( buffer == 0 ) 
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
		int result = memcmp(buffer, str, len);
		if( result == 0 ) return -1; // The other string is smaller than this

		return result;
	}

	int result = memcmp(buffer, str, length);
	if( result == 0 && length < len ) return 1; // The other string is larger than this

	return result;
}

int asCString::RecalculateLength()
{
	if( buffer == 0 ) Assign("", 0);
	length = strlen(buffer);

	assert(length < bufferSize);

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

