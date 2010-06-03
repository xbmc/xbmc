#pragma once

/*
 *      Copyright (C) 2004-2009 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <cstdlib>
#include <ctype.h>
#include <cstdio>
#include <list>
#include "xbox.h"

class CBundler
{
	XPR_FILE_HEADER XPRHeader;
	struct FileHeader_t
	{
		// 128 bytes total
		char Name[116];
		DWORD Offset;
		DWORD UnpackedSize;
		DWORD PackedSize;
	};
	std::list<FileHeader_t> FileHeaders;
	BYTE* Data;
	DWORD DataSize;

public:
	CBundler() { Data = NULL; DataSize = 0; }
	~CBundler() {}

	bool StartBundle();
	int WriteBundle(const char* Filename, int NoProtect);

	bool AddFile(const char* Filename, int nBuffers, const void** Buffers, DWORD* Sizes);
};
