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

// This class has been designed to be easy to use, but not necessarily efficiency.
// It doesn't use shared string memory, or reference counting. It keeps track of
// string length, memory size. It also makes sure that the string is null-terminated.

#ifndef AS_STRING_H
#define AS_STRING_H

#include <stdio.h>
#include <string.h>

class asCString
{
public:
	asCString();
	~asCString();

	asCString(const asCString &);
	asCString(const char *);
	asCString(const char *, size_t length);
	explicit asCString(char);

	void   Allocate(size_t len, bool keepData);
	void   SetLength(size_t len);
	size_t GetLength() const;

	void Concatenate(const char *str, size_t length);
	asCString &operator +=(const asCString &);
	asCString &operator +=(const char *);
	asCString &operator +=(char);

	void Assign(const char *str, size_t length);
	asCString &operator =(const asCString &);
	asCString &operator =(const char *);
	asCString &operator =(char);

	asCString SubString(size_t start, size_t length = (size_t)(-1)) const;

	size_t Format(const char *fmt, ...);

	int Compare(const char *str) const;
	int Compare(const asCString &str) const;
	int Compare(const char *str, size_t length) const;

	char *AddressOf();
	const char *AddressOf() const;
	char &operator [](size_t index);
	const char &operator[](size_t index) const;
	size_t RecalculateLength();

protected:
	unsigned int length;
	union
	{
		char *dynamic;
		char local[12];
	};
};

// Helper functions

bool operator ==(const asCString &, const asCString &);
bool operator !=(const asCString &, const asCString &);

bool operator ==(const asCString &, const char *);
bool operator !=(const asCString &, const char *);

bool operator ==(const char *, const asCString &);
bool operator !=(const char *, const asCString &);

bool operator <(const asCString &, const asCString &);

asCString operator +(const asCString &, const char *);
asCString operator +(const char *, const asCString &);
asCString operator +(const asCString &, const asCString &);

#endif
