#pragma once

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
	CBundler() {}
	~CBundler() {}

	bool StartBundle();
	int WriteBundle(const char* Filename, int NoProtect);

	bool AddFile(const char* Filename, int nBuffers, const void** Buffers, DWORD* Sizes);
};
