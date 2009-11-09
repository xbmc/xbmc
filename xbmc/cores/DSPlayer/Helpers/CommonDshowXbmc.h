//------------------------------------------------------------------------------
// File: stdafx.h
//
// Desc: DirectShow sample code - main header for standard include files
//      
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
#define AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Windows Header Files:

#include <intsafe.h>
#include <objbase.h>
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <comdef.h>
#include <commdlg.h>

// DirectShow Header Files
//#include <dshow.h>
//#include <streams.h>
#define _ATL_EX_CONVERSION_MACROS_ONLY
#include "Helpers/streamsxbmc.h"
#include <atlcoll.h>
//#include <Helpers/streamsxbmc.h>//its streams.h modified without the int TIMER causing too much problems to compile with xbmc

#include <d3d9.h>
#include <vmr9.h>

#ifndef _UTIL_HH__
#define _UTIL_HH__

#define FAIL_RET(x) do { if( FAILED( hr = ( x  ) ) ) \
    return hr; } while(0)

#endif

// Local Header Files

// TODO: reference additional headers your program requires here

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
