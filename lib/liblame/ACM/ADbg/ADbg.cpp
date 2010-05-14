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

#include <stdio.h>
#include <stdarg.h>
#include <windows.h>

#include "ADbg.h"

#if !defined(NDEBUG)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ADbg::ADbg(int level)
:my_level(level)
,my_time_included(false)
,my_use_file(false)
,my_debug_output(true)
,hFile(NULL)
{
	prefix[0] = '\0';
	OutPut(-1,"ADbg Creation at debug level = %d (0x%08X)",my_level,this);
}

ADbg::~ADbg()
{
	unsetDebugFile();
	OutPut(-1,"ADbg Deletion (0x%08X)",this);
}

inline int ADbg::_OutPut(const char * format,va_list params) const
{
	int result;

	char tst[1000];
	char myformat[256];

	if (my_time_included) {
		SYSTEMTIME time;
		GetSystemTime(&time);
		if (prefix[0] == '\0')
			wsprintf(myformat,"%04d/%02d/%02d %02d:%02d:%02d.%03d UTC : %s\r\n",
							time.wYear,
							time.wMonth,
							time.wDay,
							time.wHour,
							time.wMinute,
							time.wSecond,
							time.wMilliseconds,
							format);
		else
			wsprintf(myformat,"%04d/%02d/%02d %02d:%02d:%02d.%03d UTC : %s - %s\r\n",
							time.wYear,
							time.wMonth,
							time.wDay,
							time.wHour,
							time.wMinute,
							time.wSecond,
							time.wMilliseconds,
							prefix,
							format);
	} else {
		if (prefix[0] == '\0')
			wsprintf( myformat, "%s\r\n", format);
		else
			wsprintf( myformat, "%s - %s\r\n", prefix, format);
	}

	result = vsprintf(tst,myformat,params);
	
	if (my_debug_output)
		OutputDebugString(tst);

	if (my_use_file && (hFile != NULL)) {
		SetFilePointer( hFile, 0, 0, FILE_END );
		DWORD written;
		WriteFile( hFile, tst, lstrlen(tst), &written, NULL );
	}

	return result;
}

int ADbg::OutPut(int forLevel, const char * format,...) const
{
	int result=0;
	
	if (forLevel >= my_level) {
		va_list tstlist;
		int result;

		va_start(tstlist, format);

		result = _OutPut(format,tstlist);

	}

	return result;
}

int ADbg::OutPut(const char * format,...) const
{
	va_list tstlist;

	va_start(tstlist, format);

	return _OutPut(format,tstlist);
}

bool ADbg::setDebugFile(const char * NewFilename) {
	bool result;
	result = unsetDebugFile();

	if (result) {
		result = false;

		hFile = CreateFile(NewFilename, GENERIC_WRITE, FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		
		if (hFile != INVALID_HANDLE_VALUE) {
			SetFilePointer( hFile, 0, 0, FILE_END );

			result = true;

			OutPut(-1,"Debug file Opening succeeded");

		}
		else
			OutPut(-1,"Debug file %s Opening failed",NewFilename);
	}

	return result;
}

bool ADbg::unsetDebugFile() {
	bool result = (hFile == NULL);
	
	if (hFile != NULL) {
		result = (CloseHandle(hFile) != 0);
		
		if (result) {
			OutPut(-1,"Debug file Closing succeeded");
			hFile = NULL;
		}
	}

	return result;
}

#endif // !defined(NDEBUG)
