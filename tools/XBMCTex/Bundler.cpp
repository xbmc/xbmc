#include "StdAfx.h"
#include "Bundler.h"
#include "../../xbmc/lib/liblzo/lzo1x.h"

// alignment of file blocks - should be a multiple of the sector size of the disk and a power of 2
// HDD sector = 512 bytes, DVD/CD sector = 2048 bytes
// XBMC supports caching of texures on the HDD for DVD loads, so this can be 512
#define ALIGN (512)

bool CBundler::StartBundle()
{
	Data = (BYTE*)VirtualAlloc(0, 512 * 1024 * 1024, MEM_RESERVE, PAGE_NOACCESS);
	if (!Data)
	{
		printf("Memory allocation failure: %08x\n", GetLastError());
		return false;
	}
	DataSize = 0;
	FileHeaders.clear();

	lzo_init();

	return true;
}

int CBundler::WriteBundle(const char* Filename, int NoProtect)
{
	// calc data offset
	DWORD Offset = sizeof(XPR_FILE_HEADER) + FileHeaders.size() * sizeof(FileHeader_t);

	// setup header
	XPRHeader.dwMagic = XPR_MAGIC_HEADER_VALUE | ((2+(NoProtect << 7)) << 24); // version 2
	XPRHeader.dwHeaderSize = Offset;

	Offset = (Offset + (ALIGN-1)) & ~(ALIGN-1);
	XPRHeader.dwTotalSize = Offset + DataSize;

	// buffer data
	if (!VirtualAlloc(Data+DataSize, Offset, MEM_COMMIT, PAGE_READWRITE))
		return -1;
	BYTE* buf = Data+DataSize;
	memcpy(buf, &XPRHeader, sizeof(XPR_FILE_HEADER));
	buf += sizeof(XPR_FILE_HEADER);

	int j = 0;
	for (std::list<FileHeader_t>::iterator i = FileHeaders.begin(); i != FileHeaders.end(); ++i)
	{
		i->Offset += Offset;
		memcpy(buf, &(*i), sizeof(FileHeader_t));
		buf += sizeof(FileHeader_t);
	}

	// write file
	HANDLE hFile = CreateFile(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING, 0);
	if (hFile == INVALID_HANDLE_VALUE)
		return -1;
	if (SetFilePointer(hFile, DataSize + Offset, 0, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		return -1;
	if (!SetEndOfFile(hFile))
		return -1;
	SetFilePointer(hFile, 0, 0, FILE_BEGIN);
	DWORD n;
	if (!WriteFile(hFile, Data+DataSize, Offset, &n, NULL) || n != Offset)
		return -1;
	if (!WriteFile(hFile, Data, DataSize, &n, NULL) || n != DataSize)
		return -1;
	CloseHandle(hFile);
	VirtualFree(Data, 0, MEM_RELEASE);

	return DataSize + Offset;
}

bool CBundler::AddFile(const char* Filename, int nBuffers, const void** Buffers, DWORD* Sizes)
{
	FileHeader_t Header;
	
	memset(Header.Name, 0, sizeof(Header.Name));
	for (int i = 0; i < sizeof(Header.Name) && Filename[i]; ++i)
		Header.Name[i] = tolower(Filename[i]);
	Header.Name[sizeof(Header.Name)-1] = 0;

	Header.Offset = DataSize;
	Header.UnpackedSize = 0;
	for (int i = 0; i < nBuffers; ++i)
		Header.UnpackedSize += Sizes[i];

	BYTE* buf = (BYTE*)VirtualAlloc(0, Header.UnpackedSize, MEM_COMMIT, PAGE_READWRITE);
	BYTE* p = buf;
	for (int i = 0; i < nBuffers; ++i)
	{
		memcpy(p, Buffers[i], Sizes[i]);
		p += Sizes[i];
	}

	VirtualAlloc(Data+DataSize, Header.UnpackedSize, MEM_COMMIT, PAGE_READWRITE);

	lzo_voidp tmp = VirtualAlloc(0, LZO1X_999_MEM_COMPRESS, MEM_COMMIT, PAGE_READWRITE);
	if (lzo1x_999_compress(buf, Header.UnpackedSize, Data+DataSize, (lzo_uint*)&Header.PackedSize, tmp) != LZO_E_OK)
	{
		printf("Compression failure\n");
		VirtualFree(buf, 0, MEM_RELEASE);
		VirtualFree(tmp, 0, MEM_RELEASE);
		return false;
	}
	VirtualFree(tmp, 0, MEM_RELEASE);

	lzo_uint s;
	lzo1x_optimize(Data+DataSize, Header.PackedSize, buf, &s, NULL);

	VirtualFree(buf, 0, MEM_RELEASE);

	DataSize += (Header.PackedSize + (ALIGN-1)) & ~(ALIGN-1);
	FileHeaders.push_back(Header);
	return true;
}
