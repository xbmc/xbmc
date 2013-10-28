//
// Code for VS_VERSION resource
//
#include "stdafx.h"
#include "relstamp.h"
#include "vs_version.h"

/// Make version resource
BOOL makeVersionResource( __in file_ver_data_s const * fvd, __out PUCHAR *retp )
{
	PUCHAR palloc = (PUCHAR)calloc(_MAX_VERS_SIZE_CB, 1);
	yybuf vbuf( palloc, _MAX_VERS_SIZE_CB );
	unsigned cbWritten = 0;
	BOOL ok = FALSE;
	PCWSTR sigLang = L"040904B0"; /* LANG_ENGLISH/SUBLANG_ENGLISH_US, Unicode CP */
	WORD vtransl[2] = {0x0409, 0x04B0};
	WCHAR temps[_MAX_VER_STRING_LEN_CCH + 1];

	if ( fvd->langid == LANG_NEUTRAL ) {
		//TODO support any language. currently tested only with en or neutral
		vtransl[0] = LANG_NEUTRAL;
		sigLang = L"000004b0";
	}

	try {

		// Fill the res header
//		struct VS_VERSIONINFO { 
//		WORD  wLength; 
//		WORD  wValueLength; 
//		WORD  wType; 
//		WCHAR szKey[]; 
//		WORD  Padding1[]; 
//		VS_FIXEDFILEINFO Value; 
//		WORD  Padding2[]; 
//		WORD  Children[]; 
//		};

		PWORD pTotalLen = vbuf.marksize(); // FIXUP LATER
		vbuf.pushw(~0); // FIXUP LATER
		vbuf.pushw( sizeof(VS_FIXEDFILEINFO) ); //0x34
		vbuf.pushw( 0 ); //type
		vbuf.pushstr( L"VS_VERSION_INFO" );

		// Fixed info
		VS_FIXEDFILEINFO *fxi = (VS_FIXEDFILEINFO *)vbuf.getptr();
		fxi->dwSignature = 0xfeef04bd; // magic
		fxi->dwStrucVersion = VS_FFI_STRUCVERSION; //0x00010000
		fxi->dwFileVersionMS = MAKELONG( fvd->v_2, fvd->v_1 );
		fxi->dwFileVersionLS = MAKELONG( fvd->v_4, fvd->v_3 );
		fxi->dwProductVersionMS = MAKELONG( fvd->pv_2, fvd->pv_1 );
		fxi->dwProductVersionLS = MAKELONG( fvd->pv_4, fvd->pv_3 );
		fxi->dwFileFlagsMask = VS_FFI_FILEFLAGSMASK; //0x3F;
		fxi->dwFileFlags = fvd->dwFileFlags;
		fxi->dwFileOS = VOS_NT_WINDOWS32;
		fxi->dwFileType = fvd->dwFileType;
		fxi->dwFileSubtype = fvd->dwFileSubType;
		if ( 0 == fxi->dwFileType && 0 != fxi->dwFileSubtype )
			fxi->dwFileType = VFT_DRV;
		fxi->dwFileDateLS = 0; //unused?
		fxi->dwFileDateMS = 0; //
		vbuf.incptr( sizeof(VS_FIXEDFILEINFO) );
		vbuf.align4();

		// String File Info

		PWORD stringStart = vbuf.marksize();
		vbuf.pushw(~0); //wLength FIXUP LATER
		vbuf.pushw(0); //wValueLength
		vbuf.pushw(1); //wType
		vbuf.pushstr( L"StringFileInfo" );

		PWORD stringTableStart = vbuf.marksize();
		vbuf.pushw(~0); //wLength FIXUP LATER
		vbuf.pushw(0); // ?
		vbuf.pushw(1); //wType
		vbuf.pushstr( sigLang );

		// File version as string. Not shown by Vista, Win7.
		HRESULT hr = ::StringCbPrintf( &temps[0], sizeof(temps), _T("%d.%d.%d.%d"),
			fvd->v_1, fvd->v_2, fvd->v_3, fvd->v_4 );
		if ( !SUCCEEDED(hr) ) temps[0] = 0;
		if ( fvd->sFileVerTail ) {
			hr = ::StringCbCatW(&temps[0], sizeof(temps), L" ");
			hr = ::StringCbCatW(&temps[0], sizeof(temps), fvd->sFileVerTail);
			if ( !SUCCEEDED(hr) ) temps[0] = 0;
		}
		vbuf.pushTwostr( L"FileVersion", &temps[0] );

		hr = ::StringCbPrintf( &temps[0], sizeof(temps), _T("%d.%d.%d.%d"),
			fvd->pv_1, fvd->pv_2, fvd->pv_3, fvd->pv_4 );
		if ( !SUCCEEDED(hr) ) temps[0] = 0;
		if ( fvd->sProductVerTail ) {
			hr = ::StringCbCatW(&temps[0], sizeof(temps), L" ");
			hr = ::StringCbCatW(&temps[0], sizeof(temps), fvd->sProductVerTail);
			if ( !SUCCEEDED(hr) ) temps[0] = 0;
		}
		vbuf.pushTwostr( L"ProductVersion", &temps[0] );

		// Strings
		for ( int k = 0; k < ARRAYSIZE(fvd->CustomStrNames); k++ ) {
			if ( fvd->CustomStrNames[k] != NULL ) {

				vbuf.pushTwostr( fvd->CustomStrNames[k], fvd->CustomStrVals[k] );
				
				if ( 0 == _wcsicmp( L"SpecialBuild", fvd->CustomStrNames[k] ) )
					fxi->dwFileFlags |= VS_FF_SPECIALBUILD;
				if ( 0 == _wcsicmp( L"PrivateBuild",fvd->CustomStrNames[k] ) )
					fxi->dwFileFlags |= VS_FF_PRIVATEBUILD;
			}
		}

		vbuf.patchsize( stringTableStart );
		vbuf.patchsize( stringStart );
		vbuf.align4();

		// Var info
//struct VarFileInfo { 
//  WORD  wLength; 
//  WORD  wValueLength; 
//  WORD  wType; 
//  WCHAR szKey[]; 
//  WORD  Padding[]; 
//  Var   Children[]; 
		PWORD varStart = vbuf.marksize();
		vbuf.pushw(~0); // size, patch
		vbuf.pushw(0);
		vbuf.pushw(1);
		vbuf.pushstr( L"VarFileInfo" );

		vbuf.pushw(0x24);
		vbuf.pushw(0x04);
		vbuf.pushw(0x00);
		vbuf.pushstr( L"Translation" );
		vbuf.pushw( vtransl[0] );
		vbuf.pushw( vtransl[1] );
		vbuf.patchsize( varStart );

		/////////////////////////////
		vbuf.patchsize(pTotalLen);
		vbuf.checkspace(); 

		ok = TRUE;
	} catch(...) {
		ok = FALSE;
	}

	if (ok) {
		d3print("ver size= %d\n", vbuf.cbwritten() );
		*retp = palloc;
	} else {
		dprint("error in %s\n", __FUNCTION__);
		free( palloc );
	}

	return ok;
}


///////////////////////////////////////////////////////////////////////////
// Simple parser for binary version resource
///////////////////////////////////////////////////////////////////////////


BOOL ParseBinaryVersionResource( 
	__in const PUCHAR verres,
	unsigned size,
	__out VS_FIXEDFILEINFO **pfxi,
	IParseVerStrCallback *strCallback,
	bool b_dump_rc 
	)
{
	BOOL ok = FALSE;
	xybuf vbuf( verres, size );
	WCHAR sigLang[8+1];

	try {

	// Res header
	PWORD pTotalLen = vbuf.marksize();
	
	vbuf.chkword( sizeof(VS_FIXEDFILEINFO) ); //0x34
	vbuf.chkword(0); //type
	vbuf.chkstr(L"VS_VERSION_INFO");
	// Fixed info
	VS_FIXEDFILEINFO *fxi = (VS_FIXEDFILEINFO *)vbuf.getptr();
	if ( fxi->dwSignature != 0xfeef04bd )
		throw ":fxi.sig";
	if ( fxi->dwStrucVersion > 0x00010000 || (fxi->dwStrucVersion == 0) )
		throw ":fxi.version";

	*pfxi = fxi;

	if (b_dump_rc) {
		// Dump in RC format:
		dtprint( _T("#ifdef RC_INVOKED\n\n"));
		dtprint( _T("1\tVERSIONINFO\n"));
		dtprint( _T("FILEVERSION\t%u,%u,%u,%u\n"), 
			HIWORD(fxi->dwFileVersionMS), LOWORD(fxi->dwFileVersionMS), HIWORD(fxi->dwFileVersionLS), LOWORD(fxi->dwFileVersionLS) );
		dtprint( _T("PRODUCTVERSION\t%u,%u,%u,%u\n"),
			HIWORD(fxi->dwProductVersionMS), LOWORD(fxi->dwProductVersionMS), HIWORD(fxi->dwProductVersionLS), LOWORD(fxi->dwProductVersionLS) );
		dtprint( _T("FILEFLAGSMASK\t%#XL\n"),  fxi->dwFileFlagsMask);
		dtprint( _T("FILEFLAGS\t%#XL\n"),  fxi->dwFileFlags);
		dtprint( _T("FILEOS\t\t%#XL\n"),  fxi->dwFileOS);
		dtprint( _T("FILETYPE\t%#X\n"),  fxi->dwFileType);
		dtprint( _T("FILESUBTYPE\t%#X\n"),  fxi->dwFileSubtype);
	}

	vbuf.incptr( sizeof(VS_FIXEDFILEINFO) );
	vbuf.align4();

	// String File Info
	PWORD stringStart = vbuf.marksize();
	vbuf.chkword(0); //wValueLength
	vbuf.chkword(1); //wType

	try {
		vbuf.chkstr(L"StringFileInfo");
	} catch( char *exs ) {
		// !!! VarFileInfo can go before StringFileInfo!
		vbuf.chkstr(L"VarFileInfo");
		// ok so here is "VarFileInfo". Skip it and resync at StringFileInfo
		vbuf.checkspace(0x40);
		PUCHAR q = (PUCHAR)memchr( vbuf.getptr(), 'S', 0x30 );
		if ( !q ) throw(":parse_err2");
		vbuf.incptr( q - vbuf.getptr() - 3 *sizeof(WORD));
		// Retry:
		stringStart = vbuf.marksize();
		vbuf.chkword(0); //wValueLength
		vbuf.chkword(1); //wType
		vbuf.chkstr(L"StringFileInfo");
	}

	PWORD stringTableStart = vbuf.marksize();
	vbuf.chkword(0); // ?
	vbuf.chkword(1); //wType

	// Language string: ex. "040904B0"
	vbuf.checkspace( 10 * sizeof(WCHAR) );
	WORD n = wcslen( (PCWSTR)vbuf.getptr() );
	if (n != 8)
		throw(":bad_lang_str");
	memcpy( sigLang, vbuf.getptr(), 9*sizeof(WCHAR) ); //incl term. 0
	vbuf.incptr( (n + 1) * sizeof(WCHAR) );
	vbuf.align4();

	strCallback->callback( L"@LANG", sigLang ); // revise
	
	if (b_dump_rc) {
		// Dump in RC format:
		dtprint(_T("BEGIN\n"));
		dtprint(_T("\tBLOCK \"StringFileInfo\"\n"));
		dtprint(_T("\tBEGIN\n"));
		dtprint(_T("\t\tBLOCK \"%ws\"\n"), sigLang);
		dtprint(_T("\t\tBEGIN\n"));
	}

	// Loop for strings:
	int cntstrings = 0;
	do {
		PCWSTR wsname, wsval;
		vbuf.pullTwoStr( &wsname, &wsval );
		cntstrings++;
		//- dprint(" str#%d [%ws]=[%ws]\n", cntstrings, wsname, wsval);
		if (b_dump_rc) {
			dtprint(_T("\t\t\tVALUE \"%ws\", \"%ws\"\n"), wsname, strEscape(wsval));
		}
		strCallback->callback( wsname, wsval );	
	} while( vbuf.getptr() < ((PUCHAR)stringStart + *stringStart) );
	
	vbuf.align4();
	d3print("strings counted:%d\n", cntstrings);

	if (b_dump_rc) {
		dtprint(_T("\t\tEND\n"));
		dtprint(_T("\tEND\n"));
		// VarFileInfo.....
		dtprint(_T("\tBLOCK \"VarFileInfo\"\n"));
		dtprint(_T("\tBEGIN\n"));
		dtprint(_T("\t\tVALUE \"Translation\", 0x%4.4ws, 0x%4.4ws\n"), &sigLang[0], &sigLang[4]);
		dtprint(_T("\tEND\n"));
		dtprint(_T("END\n\n"));
		dtprint( _T("#endif /*RC_INVOKED*/\n\n"));
	}

	ok = TRUE;

	} catch(...) {
		dprint("Exception in %s\n", __FUNCTION__);
	}

	if (!ok) dprint("Error in %s\n", __FUNCTION__);

	return ok;	
}
