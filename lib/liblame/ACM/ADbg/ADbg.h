/************************************************************************
Project               : C++ debugging class
File version          : 0.4

BSD License post 1999 : 

Copyright (c) 2001, Steve Lhomme
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met: 

- Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution. 

- The name of the author may not be used to endorse or promote products derived
from this software without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO 
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
OF SUCH DAMAGE. 
************************************************************************/

#if !defined(_DBG_H__INCLUDED_)
#define _DBG_H__INCLUDED_

#include <windows.h>

static const int MAX_PREFIX_LENGTH = 128;

#if !defined(NDEBUG)
// define the working debugging class

class ADbg  
{
public:
	ADbg(int level = 0);
	virtual ~ADbg();

	/// \todo make an inline function to test the level first and the process
	int OutPut(int level, const char * format,...) const;

	int OutPut(const char * format,...) const;

	inline int setLevel(const int level) {
		return my_level = level;
	}

	inline bool setIncludeTime(const bool included = true) {
		return my_time_included = included;
	}

	bool setDebugFile(const char * NewFilename);
	bool unsetDebugFile();

	inline bool setUseFile(const bool usefile = true) {
		return my_use_file = usefile;
	}

	inline const char * setPrefix(const char * string) {
		return strncpy(prefix, string, MAX_PREFIX_LENGTH);
	}

private:
	int my_level;
	bool my_time_included;
	bool my_use_file;
	bool my_debug_output;

	int _OutPut(const char * format,va_list params) const;

	char prefix[MAX_PREFIX_LENGTH];

	HANDLE hFile;
};

#else // !defined(NDEBUG)

// define a class that does nothing (no output)

class ADbg  
{
public:
	ADbg(int level = 0){}
	virtual ~ADbg() {}

	inline int OutPut(int level, const char * format,...) const {
		return 0;
	}

	inline int OutPut(const char * format,...) const {
		return 0;
	}

	inline int setLevel(const int level) {
		return level;
	}

	inline bool setIncludeTime(const bool included = true) {
		return true;
	}

	inline bool setDebugFile(const char * NewFilename) {
		return true;
	}

	inline bool unsetDebugFile() {
		return true;
	}

	inline bool setUseFile(const bool usefile = true) {
		return true;
	}

	inline const char * setPrefix(const char * string) {
		return string;
	}
};

#endif // !defined(NDEBUG)

#endif // !defined(_DBG_H__INCLUDED_)
