/********************************************************************
 * Copyright(c) 2006 Broadcom Corporation, all rights reserved.
 * Contains proprietary and confidential information.
 *
 * This source file is the property of Broadcom Corporation, and
 * may not be copied or distributed in any isomorphic form without
 * the prior written consent of Broadcom Corporation.
 *
 *
 *  Name: bc_dts_utils.h
 *
 *  Description: Utils.
 *
 *  AU
 *
 *  HISTORY:
 *
 *******************************************************************/
#ifndef _BC_DTS_UTILS_H_
#define _BC_DTS_UTILS_H_

#include <windows.h>
#include <stdio.h>
#include <crtdbg.h>
#include <strsafe.h>  

HRESULT DebugLog_Initialize(const WCHAR *sFileName);
void    DebugLog_Trace(const WCHAR *sFormatString, ...);
void    DebugLog_Close();


#ifdef _DEBUG

    //--------------------------------------------------------------------------------------
    // Debug logging functions
    // Description: Contains debug logging functions.
    //
    //     Initialize: Opens a logging file with the specified file name.
    //     Trace: Writes a sprintf-formatted string to the logging file.
    //     Close: Closes the logging file and reports any memory leaks.
    //
    // The TRACE_INIT, TRACE, and TRACE_CLOSE macros are mapped to the logging functions
    // in debug builds, and defined as nothing in retail builds.
    //--------------------------------------------------------------------------------------


    #define TRACE_INIT(x) DebugLog_Initialize(x)
    #define TRACE(x) DebugLog_Trace x
    #define TRACE_CLOSE() DebugLog_Close()

    // Log HRESULTs on failure.
    #define LOG_IF_FAILED(x,hr) if (FAILED(hr)) { TRACE((L"%s hr=0x%X", x, hr)); }

#else

    #define TRACE_INIT(x) 
    #define TRACE(x) 
    #define TRACE_CLOSE() 

    #define LOG_IF_FAILED(x, hr)

#endif

#endif