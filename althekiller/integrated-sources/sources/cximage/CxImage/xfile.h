/*
 * File:	xfile.h
 * Purpose:	General Purpose File Class 
 */
/* === C R E D I T S  &  D I S C L A I M E R S ==============
 * Permission is given by the author to freely redistribute and include
 * this code in any program as long as this credit is given where due.
 *
 * CxFile (c)  11/May/2002 <ing.davide.pizzolato@libero.it>
 * CxFile version 2.00 23/Aug/2002
 * See the file history.htm for the complete bugfix and news report.
 *
 * Special thanks to Chris Shearer Cooper for new features, enhancements and bugfixes
 *
 * COVERED CODE IS PROVIDED UNDER THIS LICENSE ON AN "AS IS" BASIS, WITHOUT WARRANTY
 * OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, WITHOUT LIMITATION, WARRANTIES
 * THAT THE COVERED CODE IS FREE OF DEFECTS, MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE
 * OR NON-INFRINGING. THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE COVERED
 * CODE IS WITH YOU. SHOULD ANY COVERED CODE PROVE DEFECTIVE IN ANY RESPECT, YOU (NOT
 * THE INITIAL DEVELOPER OR ANY OTHER CONTRIBUTOR) ASSUME THE COST OF ANY NECESSARY
 * SERVICING, REPAIR OR CORRECTION. THIS DISCLAIMER OF WARRANTY CONSTITUTES AN ESSENTIAL
 * PART OF THIS LICENSE. NO USE OF ANY COVERED CODE IS AUTHORIZED HEREUNDER EXCEPT UNDER
 * THIS DISCLAIMER.
 *
 * Use at your own risk!
 * ==========================================================
 */
#if !defined(__xfile_h)
#define __xfile_h

#ifdef WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "ximadefs.h"

class DLL_EXP CxFile
{
public:
	CxFile(void) { };
	virtual ~CxFile() { };

	virtual bool	Close() = 0;
	virtual size_t	Read(void *buffer, size_t size, size_t count) = 0;
	virtual size_t	Write(const void *buffer, size_t size, size_t count) = 0;
	virtual bool	Seek(long offset, int origin) = 0;
	virtual long	Tell() = 0;
	virtual long	Size() = 0;
	virtual bool	Flush() = 0;
	virtual bool	Eof() = 0;
	virtual long	Error() = 0;
	virtual bool	PutC(unsigned char c)
		{
		// Default implementation
		size_t nWrote = Write(&c, 1, 1);
		return (bool)(nWrote == 1);
		}
	virtual long	GetC() = 0;
};

#endif //__xfile_h
