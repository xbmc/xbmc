// XprPack.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../../xbmc/lib/liblzo/lzo1x.h"

#pragma comment(lib, "../../xbmc/lib/liblzo/lzo.lib")

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		puts("Usage: XprPack xprfile");
		return 1;
	}

	printf("Compressing %s: ", argv[1]);

	HANDLE hFile = CreateFile(argv[1], GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("Unable to open file: %x\n", GetLastError());
		return 1;
	}

	DWORD n;
	DWORD XprHeader[3]; // "XPR0", totalsize, headersize
	if (!ReadFile(hFile, XprHeader, 12, &n, 0) || n != 12)
	{
		printf("Unable to read header: %x\n", GetLastError());
		return 1;
	}

	SetFilePointer(hFile, XprHeader[2], 0, FILE_BEGIN);

	lzo_uint SrcSize = XprHeader[1] - XprHeader[2];
	lzo_byte* src = (lzo_byte*)VirtualAlloc(0, SrcSize, MEM_COMMIT, PAGE_READWRITE);

	if (!ReadFile(hFile, src, SrcSize, &n, 0) || n != SrcSize)
	{
		printf("Unable to read data: %x\n", GetLastError());
		return 1;
	}

	lzo_uint DstSize;
	lzo_byte* dst = (lzo_byte*)VirtualAlloc(0, SrcSize, MEM_COMMIT, PAGE_READWRITE);

	lzo_voidp tmp = VirtualAlloc(0, LZO1X_999_MEM_COMPRESS, MEM_COMMIT, PAGE_READWRITE);
	if (lzo1x_999_compress(src, SrcSize, dst + 4, &DstSize, tmp) != LZO_E_OK)
	{
		printf("Compression failure\n");
		return 1;
	}

	lzo_uint s;
	lzo1x_optimize(dst + 4, DstSize, src, &s, NULL);

	SetFilePointer(hFile, XprHeader[2], 0, FILE_BEGIN);

	*((lzo_uint*)dst) = SrcSize;
	if (!WriteFile(hFile, dst, DstSize + 4, &n, 0) || n != DstSize + 4)
	{
		printf("Unable to write data: %x\n", GetLastError());
		return 1;
	}
	SetEndOfFile(hFile);

	SetFilePointer(hFile, 0, 0, FILE_BEGIN);

	XprHeader[1] = XprHeader[2] + DstSize + 4;
	if (!WriteFile(hFile, XprHeader, 12, &n, 0) || n != 12)
	{
		printf("Unable to write data: %x\n", GetLastError());
		return 1;
	}

	printf("%.1f:1\n", double(SrcSize) / double(DstSize));

	return 0;
}