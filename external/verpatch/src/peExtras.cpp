//
// PE image related utils
//

#include "stdafx.h"
#include "peExtras.h"
#include "relstamp.h"
#include <imagehlp.h>
#pragma comment(lib, "imagehlp")

#if UNICODE
// only non-unicode form of MapAndLoad exists in imagehlp. Grr.
BOOL MapAndLoadW(
    __in PCWSTR ImageName,
    __in_opt PCWSTR DllPath,
    __out PLOADED_IMAGE LoadedImage,
    __in BOOL DotDll,
    __in BOOL ReadOnly
    );
#define MapAndLoadT MapAndLoadW
#else
#define MapAndLoadT MapAndLoad
#endif

BOOL getFileExtraData(LPCTSTR fname, PVOID *extraData, LPDWORD dwSize)
{
	*dwSize = 0;
	*extraData = NULL;

	BOOL r;
	LOADED_IMAGE im;

	r = ::MapAndLoadT(
		fname,
		_T("\\no-implicit-paths"),
		&im,
		FALSE, // .exe by default
		TRUE // readonly
		);

	if (!r) {
		dprint("err open file for reading extra data %d\n", GetLastError() );
		return FALSE;
	}

	ULONG endOfImage = 0;

	for (ULONG i = 0; i < im.NumberOfSections; i++)
	{
		if (endOfImage < im.Sections[i].PointerToRawData + im.Sections[i].SizeOfRawData)
			endOfImage = im.Sections[i].PointerToRawData + im.Sections[i].SizeOfRawData;
	}

	if (im.SizeOfImage > endOfImage)
	{
		*dwSize = im.SizeOfImage - endOfImage;
		*extraData = malloc(*dwSize);
		ASSERT(*extraData);
		memcpy(*extraData, &im.MappedAddress[endOfImage], *dwSize);
	}

	r = ::UnMapAndLoad( &im );
	if (!r) {
		dprint("err unloading file %d\n", GetLastError() );
		return FALSE;
	}

	return TRUE;
}

BOOL appendFileExtraData( PCTSTR fname, PVOID extraData, DWORD dwSize)
{
	DWORD bytesWritten = 0;

	if (extraData == NULL)
		return TRUE;

	HANDLE fh = CreateFile(fname, GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, NULL);

	if (INVALID_HANDLE_VALUE == fh ) {
		dtprint(_T("Error opening executable %s err=%d\n"), fname, GetLastError());
		return false;
	}

	DWORD pos = SetFilePointer(fh, 0, NULL, FILE_END);
	if ( INVALID_SET_FILE_POINTER == pos ) {
		dtprint(_T("Error seeking executable %s err=%d\n"), fname, GetLastError());
		CloseHandle(fh);
		return false;
	}

	if ( !WriteFile(fh, extraData, dwSize, &bytesWritten, NULL) || (bytesWritten != dwSize) ) {
		dtprint(_T("Error writing extra data %s err=%d\n"), fname, GetLastError());
		CloseHandle(fh);
		return false;
	}

	CloseHandle(fh);
	return TRUE;
}
