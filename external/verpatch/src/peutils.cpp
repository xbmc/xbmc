//
// PE image related utils
//

#include "stdafx.h"
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


bool clearPdbPath( PLOADED_IMAGE pim )
{
	bool ret = false;

	__try {

	PIMAGE_NT_HEADERS pnth = pim->FileHeader;
	PIMAGE_FILE_HEADER pfh = &(pnth->FileHeader);
	WORD mtype = pfh->Machine;
	PIMAGE_DEBUG_DIRECTORY pdebug;
	PIMAGE_DATA_DIRECTORY pd;

	switch (mtype) {
		case IMAGE_FILE_MACHINE_I386: {
			PIMAGE_OPTIONAL_HEADER32 pih = 
				&(((PIMAGE_NT_HEADERS32)pnth)->OptionalHeader);
			pd = pih->DataDirectory;
			break;
			}
		case IMAGE_FILE_MACHINE_AMD64: 	{
			// EXPERIMENTAL for x64 image mapped on x86 host:
			PIMAGE_OPTIONAL_HEADER64 pih = 
				&(((PIMAGE_NT_HEADERS64)pnth)->OptionalHeader);
			pd = pih->DataDirectory;
			break;
			}
		case IMAGE_FILE_MACHINE_IA64:
			dprint("not tested for IA64\n"); return false;
		default:
			dprint("Unsupported arch\n"); return false;
	}

	ULONG32 adebug = pd[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
	ULONG i;
	for ( i = 0; i < pim->NumberOfSections; i++ ) {
		PIMAGE_SECTION_HEADER sh = &pim->Sections[i];
		if ( (sh->VirtualAddress <= adebug) && 
			(sh->SizeOfRawData + sh->VirtualAddress > adebug) ) {
				adebug -= sh->VirtualAddress;
				adebug += sh->PointerToRawData;
				break;
		}
	}

	if ( i >= pim->NumberOfSections ) {
		d2print("virt address %#x not found\n", adebug);
		__leave;
	}
	pdebug = (PIMAGE_DEBUG_DIRECTORY)( adebug + (PUCHAR)(pim->MappedAddress) );

	if ( pdebug->Type != IMAGE_DEBUG_TYPE_CODEVIEW /*2*/ )
		__leave;
	char *pdd = pdebug->PointerToRawData + (char*)(pim->MappedAddress);
	if ( *((PULONG32)pdd) != 'SDSR' )
		__leave;
	
	unsigned dsize = pdebug->SizeOfData;
	if ( dsize <= 0x18 ) //?
		__leave;
	pdd += 0x18; //skip header to the pdb name string
	dsize -= 0x18;
	unsigned dsize2 = strnlen( pdd, dsize );
	if ( dsize2 == 0 || dsize2 >= dsize )
		__leave;
	d2print("PDB path in image:[%hs]\n", pdd);

	char * pdbname = strrchr( pdd, '\\' );

	if ( !pdbname || (pdbname - pdd) <= 4 || strchr( pdd, '\\' ) == pdbname ) {
		d2print("PDB path too short, not changing\n");
		ret = true; __leave;
	}

	memcpy( pdd, "\\x\\", 3 );
	strcpy_s( pdd + 3, dsize2 - 3, pdbname );
	char *t = pdd + strlen(pdd);
	SecureZeroMemory( t, dsize2 - (t - pdd) );

	ret = true;

	} __except( EXCEPTION_EXECUTE_HANDLER ) {
		ret = false;
		dprint("%s - exception\n", __FUNCTION__);
	}

	return ret;
}

BOOL updFileChecksum( LPCTSTR fname, bool fRemovePdbPath )
{
	BOOL r;
	LOADED_IMAGE im;

	r = ::MapAndLoadT(
		fname,
		_T("\\no-implicit-paths"),
		&im,
		FALSE, // .exe by default
		FALSE // readonly
		);

	if (!r) {
		dprint("err open file for rechecksum %d\n", GetLastError() );
		return FALSE;
	}

	if ( !(im.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) ) {
		dprint("error: the file not marked as executable (build errors?)\n");
		::UnMapAndLoad( &im );
		return FALSE;
	}

	// begin image patches 
	if ( fRemovePdbPath && !clearPdbPath( &im ) ) {
		dprint("error while editing pdb path\n");
		::UnMapAndLoad( &im );
		return FALSE;
	}
	// end image patches

	// Checksum offset is same for IMAGE_OPTIONAL_HEADER32 and 64.
	DWORD old_sum, new_sum;
	PIMAGE_NT_HEADERS pimh = ::CheckSumMappedFile( im.MappedAddress, im.SizeOfImage, &old_sum, &new_sum );
	if (!pimh ) {
		dprint( "err CheckSumMappedFile %d\n", GetLastError() );
		::UnMapAndLoad( &im );
		return FALSE;
	}

	// d3print( "old cksm=%4.4X new=%4.4X\n", old_sum, new_sum );
	pimh->OptionalHeader.CheckSum = new_sum;

	r = ::UnMapAndLoad( &im );
	if (!r) {
		dprint("err writing file back after patching %d\n", GetLastError() );
		return FALSE;
	}

	return TRUE;
}

#if UNICODE

PSTR strFilePathDeUnicode( PCWSTR tstr );

//---------------------------------------------------------------------------------
// Only a non-unicode form of MapAndLoad exists in imagehlp. Grr.
//---------------------------------------------------------------------------------
BOOL 
MapAndLoadW( 
	__in PCWSTR fname,
	__in_opt PCWSTR path,
	__out PLOADED_IMAGE LoadedImage, 
	BOOL DotDll, 
	BOOL Readonly ) 
{
	BOOL r;
	UNREFERENCED_PARAMETER(path); // fake this arg to not search, and not deunicode
	// Convert PWSTR to something acceptable for CreateFileA
	// Another way: Require the file *name* only be ascii, then path can be gibberish.
	//  => cd to the path and use ./filename form.
	PSTR a_fname = strFilePathDeUnicode(fname);
	if (!a_fname) {
		dprint("err opening file for rechecksum (unicode path)\n");
		return FALSE;
	}
	r = MapAndLoad( a_fname, "\\dont-search-path", LoadedImage, DotDll, Readonly );
	free( a_fname );
	return r;
}

//---------------------------------------------------------------------------------
// Convert unicode file path to something acceptable for non-unicode file APIs.
// Caller should free returned pointer with free()
//---------------------------------------------------------------------------------
PSTR strFilePathDeUnicode( __in PCWSTR tstr)
{
	//- dprint("orig. path [%ws]\n", tstr );
	DWORD r;
	PWSTR p = (PWSTR)calloc( MAX_PATH + 1, sizeof(WCHAR) );
	if ( !p )
		return NULL;

	r = ::GetShortPathName( tstr, p, MAX_PATH );
	if ( r == 0 || r >= MAX_PATH ) {
		dprint("err GetShortPathName, gle=%d\n", GetLastError() );
		free(p);
		return NULL;
	}
	// now convert to single bytes
	for (int i = 0; i < MAX_PATH; i++ ) {
		WCHAR w = p[i];
		if ( !w )
			break;
		if ( w > (WCHAR)0xFF ) {
			dprint("error: GetShortPathName returned unicode??\n");
			free(p);
			SetLastError(ERROR_INVALID_NAME);
			return NULL;
		}
		p[i] = 0;
		((PUCHAR)p)[i] = w & 0xFF;
	}

	d3print("de-unicoded name [%ws]=[%hs]\n", tstr, p );

	return (PSTR)p;
}
#endif //UNICODE

