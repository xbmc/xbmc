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

#include "Bundler.h"
#include "EndianSwap.h"

#ifdef _LINUX
#include <lzo1x.h>
#else
#include "../../xbmc/lib/liblzo/LZO1X.H"
#endif

// alignment of file blocks - should be a multiple of the sector size of the disk and a power of 2
// HDD sector = 512 bytes, DVD/CD sector = 2048 bytes
// XBMC supports caching of texures on the HDD for DVD loads, so this can be 512
#undef ALIGN
#define ALIGN (512)

bool CBundler::StartBundle()
{
	DataSize = 0;
	FileHeaders.clear();

	lzo_init();

	return true;
}

// On big-endian systems, to generate the same bundle on little-endian systems,
// we need to swap all the values in the header

int CBundler::WriteBundle(const char* Filename, int NoProtect)
{
	// calc data offset
	DWORD headerSize = sizeof(XPR_FILE_HEADER) + FileHeaders.size() * sizeof(FileHeader_t);

	// setup header
	XPRHeader.dwMagic = Endian_SwapLE32(XPR_MAGIC_HEADER_VALUE | ((2+(NoProtect << 7)) << 24)); // version 2
	XPRHeader.dwHeaderSize = Endian_SwapLE32(headerSize);

	headerSize = (headerSize + (ALIGN-1)) & ~(ALIGN-1);
	XPRHeader.dwTotalSize = Endian_SwapLE32(headerSize + DataSize);

	// create our header in memory
	BYTE *headerBuf = (BYTE *)malloc(headerSize);
	if (!headerBuf) return -1;

	BYTE* buf = headerBuf;
	memcpy(buf, &XPRHeader, sizeof(XPR_FILE_HEADER));
	buf += sizeof(XPR_FILE_HEADER);

	for (std::list<FileHeader_t>::iterator i = FileHeaders.begin(); i != FileHeaders.end(); ++i)
	{
		// Swap values on big-endian systems
		i->Offset = Endian_SwapLE32(i->Offset + headerSize);
		i->UnpackedSize = Endian_SwapLE32(i->UnpackedSize);
		i->PackedSize = Endian_SwapLE32(i->PackedSize);
		memcpy(buf, &(*i), sizeof(FileHeader_t));
		buf += sizeof(FileHeader_t);
	}
	memset(buf, 0, headerBuf + headerSize - buf);

	// write file
	FILE *file = fopen(Filename, "wb");
	if (!file)
	{
		free(Data);
		free(headerBuf);
		return -1;
	}

	size_t n = fwrite(headerBuf, 1, headerSize, file);
	if (n != headerSize)
	{
		free(Data);
		free(headerBuf);
		fclose(file);
		return -1;
	}

	n = fwrite(Data, 1, DataSize, file);
	if (n != DataSize)
	{
		free(Data);
		free(headerBuf);
		fclose(file);
		return -1;
	}

	free(Data);
	free(headerBuf);
	fclose(file);

	return DataSize + headerSize;
}

bool CBundler::AddFile(const char* Filename, int nBuffers, const void** Buffers, DWORD* Sizes)
{
	FileHeader_t Header;

	memset(Header.Name, 0, sizeof(Header.Name));
	for (int i = 0; i < (int)sizeof(Header.Name) && Filename[i]; ++i)
		Header.Name[i] = tolower(Filename[i]);
	Header.Name[sizeof(Header.Name)-1] = 0;

	Header.Offset = DataSize;
	Header.PackedSize = 0;
	Header.UnpackedSize = 0;
	for (int i = 0; i < nBuffers; ++i)
		Header.UnpackedSize += Sizes[i];

	// allocate enough memory for the total unpacked size
	BYTE* buf = (BYTE*)malloc(Header.UnpackedSize);
	if (!buf) return false; // failure to allocate memory

	BYTE* p = buf;
	for (int i = 0; i < nBuffers; ++i)
	{
		memcpy(p, Buffers[i], Sizes[i]);
		p += Sizes[i];
	}

	// grab a temporary buffer for unpacking into
	BYTE *compressedBuf = (BYTE*)malloc(Header.UnpackedSize);
	if (!compressedBuf)
	{
		free(buf);
		return false;
	}

	// and a working buffer for lzo
	lzo_voidp workingBuf = malloc(LZO1X_999_MEM_COMPRESS);
	if (!workingBuf)
	{
		free(buf);
		free(compressedBuf);
		return false;
	}

	if (lzo1x_999_compress(buf, Header.UnpackedSize, compressedBuf, (lzo_uint*)&Header.PackedSize, workingBuf) != LZO_E_OK)
	{
		printf("Compression failure\n");
		free(buf);
		free(compressedBuf);
		free(workingBuf);
		return false;
	}
	free(workingBuf);

	lzo_uint s = Header.UnpackedSize;
	lzo1x_optimize(compressedBuf, Header.PackedSize, buf, &s, NULL);
	free(buf);

	// now increase the size of our buffer
	DWORD ExtraNeeded = (Header.PackedSize + (ALIGN-1)) & ~(ALIGN-1);

	// reallocate our data dump
	Data = (BYTE*)realloc(Data, DataSize + ExtraNeeded);

	memcpy(Data + DataSize, compressedBuf, Header.PackedSize);
	free(compressedBuf);
  
	memset(Data + DataSize + Header.PackedSize, 0, ExtraNeeded - Header.PackedSize);
	DataSize += ExtraNeeded;
	FileHeaders.push_back(Header);

	return true;
}
