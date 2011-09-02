// relstamp.cpp
//
// pa04 fixed bug in productversion aliases (/pv)
// pa03 fixed up for VC2008 Express

#include "stdafx.h"
#include "relstamp.h"
#include "vs_version.h"
#pragma comment(lib, "version")
#include "peExtras.h"

// Options
struct cmd_params {
	bool PatchMode;					// true: patch version descr. of the input file. false: create and replace.
	bool DbgImportOnly;				// debug: test import only, don't modify file
	bool DbgLoopbackTest;			// reparse generated blob
	// For import & patch mode
	bool DumpImportVerAsRC;			// dump input version info in RC format
	bool PreserveOriginalFilename;	// false: set to actual filename
	bool PreserveInternalFilename;	// false: set to actual filename
	bool PreserveStringVersionTail; // true: save appendix of fileversion, productversion strings

	bool fClearPdbPath;
	bool fNoPreserveExtraAppendedData;	// true = do not check for extra data appended to end of file

	bool cmd_arg_parse( int argc, _TCHAR *argv[], PCTSTR *fname, struct file_ver_data_s *fvd ); 
//	cmd_params() {} // ctor
} g_params;

// Struct to hold a resource:
struct ResDesc 
{
	PUCHAR m_data;
	DWORD m_cbdata;
	ULONG_PTR m_type; // string ptr or numeric id
	ULONG_PTR m_name; // string ptr or numeric id
	WORD m_language;

	ResDesc() : m_data(0), m_cbdata(0) {}

	ResDesc( void *data, unsigned cbData, ULONG_PTR typeId, 
		ULONG_PTR name_id, WORD langId = LANG_NEUTRAL ) 
		: m_data((PUCHAR)data), m_cbdata(cbData), m_type(typeId), 
			m_name(name_id), m_language(langId)	{}

	friend bool addResourceFromFile( PCTSTR resfile, UINT32 id_flags );
};

static 
ResDesc *aRes[_A_MAX_N_RES + 1];

static
void addUpdRes( ResDesc *rd ) 
{
	for (int i = 0; i < ARRAYSIZE(aRes) - 1; i++) {
		if ( NULL == aRes[i] ) {
			aRes[i] = rd;
			return;
		}
	}
	dprint("ERROR: Too many resources added\n");
	throw ":TooManyRes";
	return;
}


BOOL fillCompanyInfo( __out file_ver_data_s *fvd )
{
	// default
	fvd->addTwostr( L"CompanyName", DEF_COMPANY_NAME );
	fvd->addTwostr( L"LegalCopyright", DEF_COPYRGT ); //"Copyright (c) 2009"
	return TRUE;
}


BOOL fillProductInfo( __out file_ver_data_s *fvd )
{
	// default
	fvd->addTwostr( L"ProductName", DEF_PRODUCT_NAME );
	fvd->pv_1 = 1;
	fvd->pv_2 = 0;
	fvd->pv_3 = 0;
	fvd->pv_4 = 0;
	return TRUE;
}


// callback class for ParseBinaryVersionResource()
class VerCallback1 : public IParseVerStrCallback
{
	file_ver_data_s *m_vd;
	int m_numcalled;
	public:
	VerCallback1(file_ver_data_s *fvd) : m_numcalled(0), m_vd(fvd) {};
	void callback( PCWSTR name, PCWSTR value ) override;
};

// Callback to import version strings
void VerCallback1::callback( PCWSTR name, PCWSTR value )
{
	d3print( "callback #%d [%ws]= [%ws]\n", m_numcalled, name, value );
	++m_numcalled;

	// filter off FileVersion, Product Version. always take from FIXED_INFO.
	if ( 0 == _wcsicmp(name, L"FileVersion") ) {
		// Optional tail: "1.2.3.4 tail"
		PCTSTR tail = wcschr(value, _T(' '));
		if ( tail ) {
			while( *tail == _T(' ') ) tail++;
			if (*tail) m_vd->sFileVerTail = tail;
		}
		return;
	}

	if ( 0 == _wcsicmp(name, L"ProductVersion") ) {
		// Optional tail: "1.2.3.4 tail"
		PCTSTR tail = wcschr(value, _T(' '));
		if ( tail ) {
			while( *tail == _T(' ') ) tail++;
			if (*tail) m_vd->sProductVerTail = tail;
		}
		return;
	}

	if ( 0 == _wcsicmp(name, L"@LANG") ) {
		static const WCHAR sigLangNeutral[] = L"000004b0"; /* LANG_NEUTRAL */
		static const WCHAR sigLangEng[] = L"040904B0"; /* LANG_ENGLISH/SUBLANG_ENGLISH_US, Unicode CP */

		if ( 0 == _wcsicmp( value, sigLangEng ) )
			m_vd->langid = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
		else if ( 0 == _wcsicmp( value, sigLangNeutral ) )
			m_vd->langid = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
		else {
			dprint("Resource languages not supported yet! id=%ws", value);
			m_vd->langid = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
		}
		return;
	}

	m_vd->addTwostr( name, value );
}

BOOL fillFileInfo( __out file_ver_data_s *fvd, PCTSTR fpath )
{
	UINT xname, xdot_ext;
	if ( !fileGetNameExtFromPath( fpath, &xname, &xdot_ext) ) {
		dtprint(_T("Error parsing the file name\n") );
		return FALSE;
	}

	PCTSTR pfilename = (PCTSTR)( (PUCHAR)fpath + xname );
	PCTSTR pdot_ext  = (PCTSTR)( (PUCHAR)fpath + xdot_ext );

	if ( g_params.PatchMode ) 
	{
		// Import version blob from the file
		PUCHAR verinfo = (PUCHAR)calloc( _MAX_VERS_SIZE_CB, 1 );
		ASSERT(verinfo);
		if ( !fileReadVersionInfo( fpath, verinfo, _MAX_VERS_SIZE_CB ) ) {
			dprint("error reading version info from the file, err=%d\n", GetLastError());
			if (ERROR_RESOURCE_TYPE_NOT_FOUND == GetLastError() )
				dprint("The file does not have a version resource\n");
			if (ERROR_RESOURCE_DATA_NOT_FOUND == GetLastError() )
				dprint("The file could not be found or is not executable/dll\n");
			free(verinfo);
			return FALSE;
		}

		VS_FIXEDFILEINFO *fxi = NULL;
		VerCallback1 mycb(fvd);
		if ( !ParseBinaryVersionResource( verinfo, _MAX_VERS_SIZE_CB, 
			&fxi, &mycb, g_params.DumpImportVerAsRC ) ) {
			dprint("error parsing version info from the file\n");
			free(verinfo);
			return FALSE;
		}

		fvd->v_1 = HIWORD(fxi->dwFileVersionMS);
		fvd->v_2 = LOWORD(fxi->dwFileVersionMS);
		fvd->v_3 = HIWORD(fxi->dwFileVersionLS); 
		fvd->v_4 = LOWORD(fxi->dwFileVersionLS);

		fvd->pv_1 = HIWORD(fxi->dwProductVersionMS);
		fvd->pv_2 = LOWORD(fxi->dwProductVersionMS);
		fvd->pv_3 = HIWORD(fxi->dwProductVersionLS);
		fvd->pv_4 = LOWORD(fxi->dwProductVersionLS);

		fvd->dwFileFlags = (fxi->dwFileFlags & fxi->dwFileFlagsMask);
		fvd->dwFileType = fxi->dwFileType;
		fvd->dwFileSubType = fxi->dwFileSubtype;

		if (fxi->dwFileOS != VOS_NT_WINDOWS32 && fxi->dwFileOS != VOS__WINDOWS32 ) {
			dprint( "warning: imported dwFileOS not WINDOWS32 (=%#x)\n", fxi->dwFileOS);
		}

		free(verinfo);
	} // patch mode

	if ( !g_params.PreserveOriginalFilename ) {
		fvd->addTwostr(L"OriginalFilename", pfilename);
	}

	if ( !g_params.PreserveInternalFilename ) {
		fvd->addTwostr(L"InternalName", pfilename);
	}

	// Fill in per file info:
	if ( VFT_UNKNOWN == fvd->dwFileType ) {
		d3tprint(_T("file ext=%s\n"), pdot_ext);
		if ( 0 == _tcsicmp( pdot_ext, _T(".exe") ))
			fvd->dwFileType = VFT_APP;
		if ( 0 == _tcsicmp( pdot_ext, _T(".sys") ))
			fvd->dwFileType = VFT_DRV;
		if (  0 == _tcsicmp( pdot_ext, _T(".dll") ))
			fvd->dwFileType = VFT_DLL;
	}

	if ( !fvd->getValStr(L"FileDescription") ) {
		// Use filename as default FileDescription
		fvd->addTwostr( L"FileDescription", pfilename );
	}

	return TRUE;
}

BOOL parseFileVer( __out file_ver_data_s *fvd, PCTSTR arg )
{
	unsigned n1=0,n2=0,n3=0,n4=0;
	int nf = _stscanf_s( arg, _T("%d.%d.%d.%d"), &n1, &n2, &n3, &n4);

	// if less than 4 numbers given, they are minor
	switch( nf ) {
		case 1:
			fvd->v_4 = n1;
			break;
		case 2:
			fvd->v_3 = n1; fvd->v_4 = n2;
			break;
		case 3:
			fvd->v_2 = n1; fvd->v_3 = n2; fvd->v_4 = n3; 
			break;
		case 4:
			fvd->v_1 = n1; fvd->v_2 = n2; fvd->v_3 = n3; fvd->v_4 = n4;
			break;
		default:
			dprint("error parsing version arg\n");
			return FALSE;
	}

	// Optional tail: "1.2.3.4 tail"
	// If no tail found, don't replace existing.
	PCTSTR tail = _tcschr(arg, _T(' '));
	if ( tail ) {
		while( *tail == _T(' ') ) tail++;
		fvd->sFileVerTail = (*tail) ? strUnEscape(tail) : NULL;
	}

	return TRUE;
}

BOOL parseProductVer( __out file_ver_data_s *fvd, PCTSTR arg )
{
	unsigned n1=0,n2=0,n3=0,n4=0;
	int nf = _stscanf_s( arg, _T("%d.%d.%d.%d"), &n1, &n2, &n3, &n4);

	// if less than 4 numbers given, they are minor
	switch( nf ) {
		case 1:
			fvd->pv_4 = n1;
			break;
		case 2:
			fvd->pv_3 = n1; fvd->pv_4 = n2;
			break;
		case 3:
			fvd->pv_2 = n1; fvd->pv_3 = n2; fvd->pv_4 = n3; 
			break;
		case 4:
			fvd->pv_1 = n1; fvd->pv_2 = n2; fvd->pv_3 = n3; fvd->pv_4 = n4;
			break;
		default:
			dprint("error parsing version arg\n");
			return FALSE;
	}

	// Optional tail: "1.2.3.4 tail"
	// If no tail found, don't replace existing.
	PCTSTR tail = _tcschr(arg, _T(' '));
	if ( tail ) {
		while( *tail == _T(' ') ) tail++;
		fvd->sProductVerTail = (*tail) ? strUnEscape(tail) : NULL;
	}

	return TRUE;
}


BOOL updFileResources( LPCTSTR fname, __in ResDesc *ard[] )
{
	if ( !ard[0] ) {
		d2print("No resources to update\n");
		return TRUE;
	}

	// open file, start update
    BOOL bDeleteExistingResources = FALSE;
	BOOL ok = FALSE;
	int n_updated = 0;
	HANDLE rhandle = ::BeginUpdateResource( fname, bDeleteExistingResources );
	if ( !rhandle ) {
		dprint( "Error opening file for update resources, err=%d\n", GetLastError() );
		return FALSE;
	}

	for (int i = 0; ; i++) {
		ResDesc *rd = ard[i];
		if ( !rd )
			break;

		ok = ::UpdateResource(
			rhandle,
			(LPCTSTR)rd->m_type,
			(LPCTSTR)rd->m_name,
			rd->m_language,
			(LPVOID)rd->m_data,
			rd->m_cbdata
			);

		if (!ok) {
			dprint("UpdateResource #%d err=%d\n", i,  GetLastError());
			break;
		}
		n_updated++;
	}

	d2print("Resources updated: %d\n", n_updated);

	BOOL r2 = ::EndUpdateResource( rhandle, !ok );
	if (!r2) {
		dprint("EndUpdateResource err=%d\n", GetLastError());
		ok = FALSE;
	}

	return ok;
}

long parseFileSubType( PCTSTR ap )
{
	if ( isdigit(*ap) )
		return _tcstol( ap, NULL, 16 );
	if ( 0 == _tcsicmp(ap, _T("DRV_SYSTEM")) ) 
		return VFT2_DRV_SYSTEM;
	if ( 0 == _tcsicmp(ap, _T("DRV_NETWORK")) ) 
		return VFT2_DRV_NETWORK;
	if ( 0 == _tcsicmp(ap, _T("DRV_COMM")) ) 
			return VFT2_DRV_COMM;
	return 0;
}


////////////////////////////////////////////////////////////////////////
// main
////////////////////////////////////////////////////////////////////////
int _tmain(int argc, _TCHAR* argv[])
{
	static file_ver_data_s file_ver_data;
	PCTSTR fname;
	BOOL r;

	SetErrorMode( SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX );
	r = SetThreadLocale( LOCALE_INVARIANT );

	if ( !g_params.cmd_arg_parse( argc, argv, &fname, NULL ) ) {
		return 1;
	}

	d2tprint(_T("relstamp file=[%s]\n"), fname );

	r &= fillFileInfo( &file_ver_data, fname );

	if ( !g_params.PatchMode ) { // if not patch mode, read from INI
		r &= fillCompanyInfo( &file_ver_data );
		r &= fillProductInfo( &file_ver_data );
	}

	// Parse again, put the args into the data
	if ( !g_params.cmd_arg_parse( argc, argv, &fname, &file_ver_data ) ) {
		dprint("Error parsing command args 2 pass!\n");
		r = FALSE;
	}

	if ( !r ) {
		dprint("Some of actions failed, exiting\n");
		return 1;
	}

	PUCHAR verdata = NULL;
	r = makeVersionResource( &file_ver_data, &verdata );
	if ( !r ) return 1;

	if ( g_params.DbgLoopbackTest )
	{	// loopback test for vers res parser
		VS_FIXEDFILEINFO *fxi = NULL;
		VerCallback1 mycb(&file_ver_data);
		dprint("dbg: begin reparse\n");
		r = ParseBinaryVersionResource( verdata, _MAX_VERS_SIZE_CB, &fxi, &mycb, g_params.DumpImportVerAsRC );
		dprint("dbg: end reparse\n");
	}

	// Save extra data possibly appended to the exe file
	DWORD extrasSize = 0;
	void *extras;
	if ( !g_params.fNoPreserveExtraAppendedData ) {
		r = getFileExtraData(fname, &extras, &extrasSize);
		if ( !r ) {
			dprint("Failed to read extra data\n");
			return 2;
		}
		if ( extrasSize ) {
			d2print("Found extra %d bytes appended to the file\n", extrasSize);
		}
	}

	if( g_params.DbgImportOnly ) {
		dprint("read only, exiting\n");
		return 0;
	}

	// Add the version to resources update:
	addUpdRes( 
		new ResDesc( 
			(PVOID)verdata, 
			*((PWORD)verdata), 
			(ULONG_PTR)RT_VERSION, 
			(ULONG_PTR)MAKEINTRESOURCE(VS_VERSION_INFO), 
			 file_ver_data.langid
			)
	);

	r = updFileResources( fname, &aRes[0] );
	if ( !r ) {
		dprint("Update file resources failed, the file may be damaged\n");
		return 2;
	}

	if ( extrasSize ) {

		// Restore extra data appended to end of exec
		r = appendFileExtraData(fname, extras, extrasSize);
		if ( !r ) {
			dprint("Failed to restore extra data, the file may be damaged\n");
			return 3;
		}

	} else {

		// Re-checksum
		r = updFileChecksum( fname, g_params.fClearPdbPath );
		if ( !r ) {
			dprint("Update file checksum failed, the file may be damaged\n");
			return 3;
		}

	}

	d2print("ok\n");
	return 0;
}


// Check if the string key name is alias, return the correct invariant key name:
// See ver-res.txt
LPCTSTR aliasToStringKey( LPCTSTR key )
{
	if ( 0 == _tcsicmp(key, _T("comment"))) return _T("Comments");
	if ( 0 == _tcsicmp(key, _T("company"))) return _T("CompanyName");
	if ( 0 == _tcsicmp(key, _T("description"))) return _T("FileDescription");
	if ( 0 == _tcsicmp(key, _T("desc"))) return _T("FileDescription");
	if ( 0 == _tcsicmp(key, _T("title"))) return _T("InternalName");
	if ( 0 == _tcsicmp(key, _T("copyright"))) return _T("LegalCopyright");
	if ( 0 == _tcsicmp(key, _T("(c)"))) return _T("LegalCopyright");
	if ( 0 == _tcsicmp(key, _T("trademarks"))) return _T("LegalTrademarks");
	if ( 0 == _tcsicmp(key, _T("(tm)"))) return _T("LegalTrademarks");
	if ( 0 == _tcsicmp(key, _T("tm"))) return _T("LegalTrademarks");
	if ( 0 == _tcsicmp(key, _T("product"))) return _T("ProductName");
	if ( 0 == _tcsicmp(key, _T("pb"))) return _T("PrivateBuild");
	if ( 0 == _tcsicmp(key, _T("sb"))) return _T("SpecialBuild");
	if ( 0 == _tcsicmp(key, _T("build"))) return _T("SpecialBuild");

	if ( 0 == _tcsicmp(key, _T("fileversion"))) {
		dprint("do NOT use FileVersion with /s option, see usage!\n");
	}

	if ( 0 == _tcsicmp(key, _T("language"))) {
		dprint("do NOT use Language with /s option, this won't work.\n");
	}

	//d3tprint(_T("alias not found %s\n"), key);
	return key;
}


///////////////////////////////////////////////////////////////////////
// Command args
//
//TODO: handle escapes in strings passed on command line
//      mode without version res: only bin patches 
// 
///////////////////////////////////////////////////////////////////////
bool cmd_params::cmd_arg_parse( int argc, _TCHAR *argv[], PCTSTR *fname, 
	__out_opt struct file_ver_data_s *fvd ) 
{
	bool firstPass = false;
	static file_ver_data_s my_fvd;
	if ( !fvd ) {
		firstPass = true;
		fvd = &my_fvd;
	}

	int pos_args = 0;
	int patch_actions = 0;

	PatchMode = true; //default mode -pa04

	for( int i = 1; i < argc; i++ ) {
		PCTSTR ap = argv[i];
		if (!ap) 
			break; //done

		if ( *ap == _T('/') || *ap == _T('-') ) {
			ap++;

			if ( argmatch(_T("?"), ap ) )
				{ showUsage(); return false; }
			else if ( argmatch(_T("fn"), ap) )  // keep original filename
				PreserveInternalFilename = PreserveOriginalFilename = true;
			else if ( argmatch(_T("vo"), ap) ) // dump input res. desc. as RC source
				DumpImportVerAsRC = true;
			else if ( argmatch(_T("xi"), ap) ) {  // read only, don't patch
				DbgImportOnly = true;	// dbg
			}
			else if ( argmatch(_T("xlb"), ap) )  // reparse created res.desc. (self test)
				DbgLoopbackTest = true;  //dbg 
			else if ( argmatch(_T("sc"), ap) ) { // /sc "comment"
				ap = argv[++i];	ASSERT(ap);
				patch_actions++;
				if( !fvd->addTwostr( _T("Comments"), strUnEscape(ap) ) ) {
					dtprint(_T("Error adding string:[%s]\n"), ap);
					return false;
				}
			}
			else if ( argmatch(_T("s"), ap) ) { 
				//Add a string to version res: /s name "comment"
				PCTSTR ns = argv[++i];	ASSERT(ns);
				ap = argv[++i];	ASSERT(ap);
				patch_actions++;

				if( !fvd->addTwostr( aliasToStringKey(ns), strUnEscape(ap) ) ) {
					dtprint(_T("Error adding string:[%s]\n"), ap);
					return false;
				}
			}
			else if( argmatch(_T("pv"), ap) ||
					argmatch(_T("prodver"), ap)  ||
					argmatch(_T("productver"), ap)  ||
					argmatch(_T("productversion"), ap) ) {
				// product version string has same form as the file version arg (positional)
				ap = argv[++i];	ASSERT(ap);
				if ( !parseProductVer( fvd, ap ) ) {
					dprint("bad product version arg, see usage (/?)\n");
					return false;
				}
				patch_actions++;
			}
			else if (argmatch(_T("vft2"), ap) ) { // version subtype
				ap = argv[++i];	ASSERT(ap);
				long n = parseFileSubType(ap);
				if ( !(n >= 0 && n <= 0xFFFF) ) {
					dtprint(_T("Bad subtype \"%s\". For usage: /?\n"), ap);
					return false;
				}
				fvd->dwFileSubType = (USHORT)n;
				patch_actions++;
			}
			else if ( argmatch(_T("rpdb"), ap) ) {
				g_params.fClearPdbPath = true;
				patch_actions++;
			}
			else if ( argmatch(_T("va"), ap) ) { // auto generate version desc.
				PatchMode = false;
				patch_actions++;
			}
			else if ( argmatch(_T("rf"), ap) ) {
				// Add a resource from file: /rf #<number> filename
				// For now, use numeric ids only. High 16 bits can be used for type, etc.
				ULONG res_id = 0;
				ap = argv[++i];	ASSERT(ap);
				if ( *ap++ != _T('#') || 0 == (res_id = _tcstol( ap, NULL, 16 )) ) {
					dtprint(_T("Resource id must be #hex_number\n"), ap);
					return false;
				}

				ap = argv[++i];	ASSERT( ap && *ap != _T('/') && *ap != _T('-') );
				// only during 2nd pass:
				if ( !firstPass && !addResourceFromFile( ap, res_id ) ) {
					dtprint(_T("Error adding resource file [%s] id=%#X\n"), ap, res_id);
					return false;
				}
			}
			else if ( argmatch(_T("noed"), ap) ) {
				// Do not check for extra data appended to exe file
				g_params.fNoPreserveExtraAppendedData = true;
			}
			else {
				dtprint(_T("Unknown option \"%s\". For usage: /?\n"), ap);
				return false;
			}
		} else {
			// position args:
			switch(pos_args++) {
				case 0:
					*fname = ap; //ex d:/stuff/foo.sys
					break; 
				case 1:
					if ( parseFileVer( fvd, ap ) ) {
						patch_actions++;
						break;
					}
					dprint("bad version arg, see usage (/?)\n");
					return false;
				default:
					dprint("Too many args, see usage (/?)\n");
					return false;
			}
		}
	}

	if ( 0 == pos_args ) {
		showUsage();
		return false;
	}

	if ( 0 == patch_actions )
		DbgImportOnly = true; //dbg

	// flags: SpecialBuild fVerBeta fVerDebug?

	return true;
}

////////////////////////////////////////////////////////////////////////
// misc. utils
////////////////////////////////////////////////////////////////////////


LPWSTR stralloc( __in PCSTR s )
{
	ASSERT(s);
	size_t n = strlen(s);
	ASSERT( n < (256));
	LPWSTR p = (LPWSTR)malloc( (n + 1) * sizeof(WCHAR) );
	ASSERT( p );
	for ( size_t i = 0; i <= n; i++ )
		p[i] = s[i];
	return p;
}

LPWSTR stralloc( __in PCWSTR s )
{
	ASSERT(s);
	PWSTR p = _wcsdup(s);
	ASSERT(p);
	return p;
}

// return byte offset to name and .ext parts of filename
BOOL fileGetNameExtFromPath( __in PCTSTR path, __out PUINT pname, __out PUINT pext )
{
	ASSERT(path);
	PTSTR name = (PTSTR)calloc( 2*MAX_PATH, sizeof(TCHAR));
	ASSERT(name);
	PTSTR ext = name + MAX_PATH;

	errno_t e;
	e = _tsplitpath_s(path, NULL, 0 , NULL, 0, name, MAX_PATH, ext, 10);
	if ( e == ERROR_SUCCESS ) {
		size_t lname = _tcslen(name);
		size_t lext = _tcslen(ext);
		*pname = (UINT)(_tcslen(path) - lname - lext) * sizeof(TCHAR);
		*pext = *pname + (lname * sizeof(TCHAR));
	} else {
		dtprint(_T("Error parsing filename: err=%d path=[%s]\n"), e, path);
	}

	if ( name ) free( name );

	return e == ERROR_SUCCESS;
}

// Read VS_VERSION resource blob from a file by name
BOOL fileReadVersionInfo( __in PCTSTR fname, __out PUCHAR buf, __in unsigned size)
{
	BOOL r;
	r = GetFileVersionInfo(  fname, NULL /*reserved*/, (DWORD)size, (LPVOID)buf );
	return r;
}

// Format a string escaped for RC: quotes, symbols (R) (C) and so on
PCWSTR strEscape( __in PCWSTR ws )
{
	if ( !ws || !*ws ) {
		return (PCWSTR)L"\\0";
	}
	return ws; //$$$ TODO. for now, unescaped text will be printed, edit manually
}

// Unescape a string escaped for RC: quotes, symbols (R) (C) and so on
PCWSTR strUnEscape( __in PCWSTR ws )
{
	return ws; //$$$ TODO
}

bool argmatch(PCTSTR sw, PCTSTR cmp )
{
	if ( 0 == _tcsicmp(sw, cmp) ) return true;
	return false;
}


//////////////////////////////////////////////////////////////////////////
// Add raw binary resource from file.
// Low 16 bit of id_flags = resource ID. Bitmask FF0000 = type (0=RCDATA). High byte reserved.
//////////////////////////////////////////////////////////////////////////
bool addResourceFromFile( PCTSTR resfile, UINT32 id_flags )
{
	UINT64 xFileSize;
	HANDLE fh = CreateFile(resfile, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, NULL);

	if (INVALID_HANDLE_VALUE == fh ) {
		dtprint(_T("Error opening res. file %s err=%d\n"), resfile, GetLastError());
		return false;
	}

	if ( !GetFileSizeEx( fh, (PLARGE_INTEGER)&xFileSize ) ) {
		dtprint(_T("Error get file size %s\n"), resfile);
		CloseHandle(fh);
		return false;
	}

	if ( xFileSize > _A_MAX_RES_CB ) {
		dtprint(_T("Error: file size too large %s %I64d K\n"), resfile, (xFileSize/1024));
		CloseHandle(fh);
		return false;
	}

	DWORD dwFileSize = (DWORD)xFileSize;
	PUCHAR dp = (PUCHAR)calloc( dwFileSize + 4, 1 ); // round up to 4 bytes
	ASSERT(dp);
	DWORD cbread;
	if ( !ReadFile( fh, (LPVOID)dp, dwFileSize, &cbread, NULL ) || cbread != dwFileSize ) {
		dtprint(_T("Error reading file %s\n"), resfile);
		CloseHandle(fh);
		free(dp);
		return false;
	}

	CloseHandle(fh);

	// round up to 4 bytes
	dwFileSize = (dwFileSize + 3) & ~3; 

	ULONG restype = (id_flags >> 16) & 0xFF;
	if ( 0 == restype ) restype = (ULONG)RT_RCDATA;

	addUpdRes( new ResDesc( dp, dwFileSize, restype, id_flags & 0xFFFF ) );

	return true;	
}


#if 1
// Get a resource (pointer and size)
// hm: module handle. 0 is the exe file
bool getResource( HMODULE hm, DWORD rtype, DWORD rid, __out LPCVOID *p, __out PDWORD size )
{
	HRSRC hrs = FindResourceEx( hm, (LPCTSTR)rtype, (LPCTSTR)rid, 0 /*MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)*/ );
	if ( !hrs ) {
		d2print("cannot find res. %#x, err=%d\n", rid, GetLastError());
		return false;
	}

	DWORD rsize = SizeofResource(hm, hrs);
	if ( rsize < sizeof (DWORD) ) {
		d2print("res. size = 0??\n");
		return false;
	}

	HGLOBAL hg = LoadResource( hm, hrs );
	if ( !hg ) {
		d2print("err LoadResource %d\n", GetLastError() );
		return false;
	}
	
	*p = LockResource( hg );
	*size = rsize;
	return !!(*p) ;
}


////////////////////////////////////////////////////////////////////////////////
// USAGE
// How to add the help text:
// - Put the text in a file (ex. usage.txt)
// - make a copy of this exe file
// - use /rf #64 <file.txt> to attach this text file to the copy of the program
// See ver-self.cmd for a working example (this is also a kind of unit test :)
////////////////////////////////////////////////////////////////////////////////
void showUsage()
{
	const ULONG usage_txt_id = 0x64; //100
	const ULONG usage_txt_type = (ULONG)(ULONG_PTR)RT_RCDATA;
	const void *p;
	DWORD len;
	if ( !getResource( 0, usage_txt_type, usage_txt_id, &p, &len ) ) {
		dprint("No usage text! See readme.txt for instructions how to add it.\n");
		return;
	}
	fwrite(p, len, 1, stderr);
}

#else
void showUsage()
{
	dprint("verpatch r2 (2009/05/31)\n");
	dprint("Usage: verpatch filename [version] [/options]\n");
	dprint("\nOptions:\n");
	dprint(" /sc \"comment\"\t- add Comments string\n"); // todo: escapes
	dprint(" /s name \"value\"\t- add/replace any version resource string\n"); // todo: escapes
	dprint(" /va\t- create a default version resource\n");
	dprint(" /vft2 num\t- specify driver type (VFT2_xxx, see winver.h)\n");
	dprint(" /fn\t- preserve Original filename, Internal name in the file version info\n");
	dprint(" /vo\t- output the file version info in RC format\n");
	dprint(" /xi\t- test mode, do not patch the file\n");
	dprint(" /rpdb\t- remove path to .pdb in debug information\n");
	dprint(" /rf #hex-id file - add a resource from file (see readme)\n");
	dprint("\n\nExamples:\n");
	dprint("  verpatch d:\\foo.dll 1.2.33.44 /sc \"holy cow, it works!\"\n");
	dprint("  verpatch d:\\foo.sys \"33.44 release\" /fn\n");
	dprint("  verpatch d:\\foo.exe 1.2.3.4 /rf #9 driver.sys\n");
}
#endif
