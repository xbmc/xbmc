#pragma once

class CBundler
{
	XPR_HEADER XPRHeader;
	struct FileHeader_t
	{
		// 64 bytes total
		char Name[52];
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
	int WriteBundle(const char* Filename);

	bool AddFile(const char* Filename, int nBuffers, const void** Buffers, DWORD* Sizes);
};