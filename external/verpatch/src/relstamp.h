// relstamp R2

#pragma once

// Defs for Windows version resource
// http://msdn.microsoft.com/en-us/library/ms646997(VS.85).aspx
#define _MAX_VERS_SIZE_CB 4096
#define _MAX_VER_STRING_LEN_CCH 255
#define _MAX_VER_CUSTOM_STRINGS 16
#define _A_MAX_N_RES 8
#define _A_MAX_RES_CB (500*1024)

#if ( 1 && !defined(DEF_COMPANY_NAME) )
#define DEF_COMPANY_NAME	_T(" ")
#define DEF_COPYRGT			_T("Copyright (c) 2009")
#define DEF_PRODUCT_NAME	_T(" ")
#endif

#define dprint(fmt, ...) printf(fmt, __VA_ARGS__)
#define dtprint(tfmt, ...) _tprintf(tfmt, __VA_ARGS__)

#ifdef NDEBUG
#undef NDEBUG
#endif
#define ASSERT assert

#ifndef _A_NOISE_DBG
#define _A_NOISE_DBG 1
#endif

#if _A_NOISE_DBG
#define d2print(fmt, ...) dprint(fmt, __VA_ARGS__)
#define d2tprint(tfmt, ...) dtprint(tfmt,  __VA_ARGS__)
#else
#define d2print(fmt, ...) __noop(fmt, __VA_ARGS__)
#define d2tprint(tfmt, ...) __noop(tfmt, __VA_ARGS__)
#endif //_A_NOISE_DBG

#if ( _A_NOISE_DBG > 1 )
#define d3print d2print
#define d3tprint d2tprint
#else
#define d3print(fmt, ...) __noop(fmt, __VA_ARGS__)
#define d3tprint(tfmt, ...) __noop(tfmt, __VA_ARGS__)
#endif //_A_NOISE_DBG

// Format a string escaped for RC: quotes, (R), (C) and so on
PCWSTR strEscape( __in PCWSTR ws );
PCWSTR strUnEscape( __in PCWSTR ws );

// strdup likes: 
LPWSTR stralloc( __in PCSTR s );
LPWSTR stralloc( __in PCWSTR s );
// Get name, ext from full filename
BOOL fileGetNameExtFromPath( __in PCTSTR path, __out PUINT pname, __out PUINT pext );
BOOL fileReadVersionInfo( __in PCTSTR fname, __out PUCHAR buf, __in unsigned size);

// 3state flag:
enum f3state { F3NOTSET, F3FALSE, F3TRUE };

void showUsage();
bool argmatch(__in PCTSTR sw, __in PCTSTR cmp );

BOOL updFileChecksum( LPCTSTR fname, bool fRemovePdbPath = false );
