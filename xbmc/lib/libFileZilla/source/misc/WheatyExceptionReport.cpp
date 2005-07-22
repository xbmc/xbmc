//==========================================
// Based upon code from
// Matt Pietrek
// MSDN Magazine, 2002
// FILE: WheatyExceptionReport.CPP
//==========================================
#define WIN32_LEAN_AND_MEAN
#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <dbghelp.h>
#include "WheatyExceptionReport.h"
#include "..\version.h"
#include "ProcessorInfo.h"
#include "WindowsVersion.h"
#include "mailmsg.h"

typedef BOOL
(_stdcall *tSymFromAddr)(
    IN  HANDLE              hProcess,
    IN  DWORD64             Address,
    OUT PDWORD64            Displacement,
    IN OUT PSYMBOL_INFO     Symbol
    );

typedef DWORD
(_stdcall *tSymGetOptions)(
	);

typedef DWORD
(_stdcall *tSymSetOptions)(
    IN DWORD   SymOptions
    );

typedef BOOL
(_stdcall *tSymCleanup)(
    IN HANDLE hProcess
    );

typedef BOOL
(_stdcall *tSymInitialize)(
    IN HANDLE   hProcess,
    IN PSTR     UserSearchPath,
    IN BOOL     fInvadeProcess
    );

typedef BOOL
(_stdcall *tSymEnumSymbols)(
    IN HANDLE                       hProcess,
    IN ULONG64                      BaseOfDll,
    IN PCSTR                        Mask,
    IN PSYM_ENUMERATESYMBOLS_CALLBACK    EnumSymbolsCallback,
    IN PVOID                        UserContext
    );

typedef ULONG
(_stdcall *tSymSetContext)(
    HANDLE hProcess,
    PIMAGEHLP_STACK_FRAME StackFrame,
    PIMAGEHLP_CONTEXT Context
    );

typedef BOOL
(_stdcall *tSymGetLineFromAddr)(
    IN  HANDLE                hProcess,
    IN  DWORD                 dwAddr,
    OUT PDWORD                pdwDisplacement,
    OUT PIMAGEHLP_LINE        Line
    );

typedef BOOL
(_stdcall *tStackWalk)(
    DWORD                             MachineType,
    HANDLE                            hProcess,
    HANDLE                            hThread,
    LPSTACKFRAME                      StackFrame,
    PVOID                             ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE      ReadMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE    FunctionTableAccessRoutine,
    PGET_MODULE_BASE_ROUTINE          GetModuleBaseRoutine,
    PTRANSLATE_ADDRESS_ROUTINE        TranslateAddress
    );

typedef PVOID
(_stdcall *tSymFunctionTableAccess)(
    HANDLE  hProcess,
    DWORD   AddrBase
    );

typedef DWORD
(_stdcall *tSymGetModuleBase)(
    IN  HANDLE              hProcess,
    IN  DWORD               dwAddr
    );

typedef BOOL
(_stdcall *tSymGetTypeInfo)(
    IN  HANDLE          hProcess,
    IN  DWORD64         ModBase,
    IN  ULONG           TypeId,
    IN  IMAGEHLP_SYMBOL_TYPE_INFO GetType,
    OUT PVOID           pInfo
    );

typedef BOOL
(_stdcall *tMiniDumpWriteDump)(
	HANDLE hProcess,
	DWORD ProcessId,
	HANDLE hFile,
	MINIDUMP_TYPE DumpType,
	PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
	PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	PMINIDUMP_CALLBACK_INFORMATION CallbackParam
	);

static tSymCleanup				pSymCleanup;
static tSymInitialize			pSymInitialize;
static tSymGetOptions			pSymGetOptions;
static tSymSetOptions			pSymSetOptions;
static tSymEnumSymbols			pSymEnumSymbols;
static tSymSetContext			pSymSetContext;
static tSymGetLineFromAddr		pSymGetLineFromAddr;
static tSymFromAddr			pSymFromAddr;
static tStackWalk				pStackWalk;
static tSymFunctionTableAccess	pSymFunctionTableAccess;
static tSymGetModuleBase		pSymGetModuleBase;
static tSymGetTypeInfo			pSymGetTypeInfo;
static tMiniDumpWriteDump		pMiniDumpWriteDump;



//============================== Global Variables =============================

//
// Declare the static variables of the WheatyExceptionReport class
//
TCHAR WheatyExceptionReport::m_szLogFileName[MAX_PATH];
TCHAR WheatyExceptionReport::m_szDmpFileName[MAX_PATH];
LPTOP_LEVEL_EXCEPTION_FILTER WheatyExceptionReport::m_previousFilter;
HANDLE WheatyExceptionReport::m_hReportFile;
HANDLE WheatyExceptionReport::m_hDumpFile;
HANDLE WheatyExceptionReport::m_hProcess;

// Declare global instance of class
static WheatyExceptionReport g_WheatyExceptionReport;

//============================== Class Methods =============================

WheatyExceptionReport::WheatyExceptionReport( )   // Constructor
{
    // Install the unhandled exception filter function
    m_previousFilter =
        SetUnhandledExceptionFilter(WheatyUnhandledExceptionFilter);

    // Figure out what the report file will be named, and store it away
    GetModuleFileName( 0, m_szLogFileName, MAX_PATH );

    // Look for the '.' before the "EXE" extension.  Replace the extension
    // with "RPT"
    PTSTR pszDot = _tcsrchr( m_szLogFileName, _T('.') );
    if ( pszDot )
    {
        pszDot++;   // Advance past the '.'
		*pszDot = 0;
		_tcscpy( m_szDmpFileName, m_szLogFileName );
        _tcscpy( pszDot, _T("rpt") );   // "rpt" -> "Report"
		_tcscat( m_szDmpFileName, _T("dmp") );   // "dmp" -> "Dump"
    }

    m_hProcess = GetCurrentProcess();
}

//============
// Destructor 
//============
WheatyExceptionReport::~WheatyExceptionReport( )
{
    SetUnhandledExceptionFilter( m_previousFilter );
}

//==============================================================
// Lets user change the name of the report file to be generated 
//==============================================================
void WheatyExceptionReport::SetLogFileName( PTSTR pszLogFileName )
{
    _tcscpy( m_szLogFileName, pszLogFileName );
}

//===========================================================
// Entry point where control comes on an unhandled exception 
//===========================================================
LONG WINAPI WheatyExceptionReport::WheatyUnhandledExceptionFilter(
                                    PEXCEPTION_POINTERS pExceptionInfo )
{
	HMODULE hDll=LoadLibrary( _T("dbghelp.dll") );
	if (!hDll)
	{
		if ( m_previousFilter )
			return m_previousFilter( pExceptionInfo );
		else
			return EXCEPTION_CONTINUE_SEARCH;
	}
	
	pSymCleanup				= (tSymCleanup)GetProcAddress(hDll, "SymCleanup");
	pSymInitialize			= (tSymInitialize)GetProcAddress(hDll, "SymInitialize");
	pSymGetOptions			= (tSymGetOptions)GetProcAddress(hDll, "SymGetOptions");
	pSymSetOptions			= (tSymSetOptions)GetProcAddress(hDll, "SymSetOptions");
	pSymEnumSymbols			= (tSymEnumSymbols)GetProcAddress(hDll, "SymEnumSymbols");
	pSymSetContext			= (tSymSetContext)GetProcAddress(hDll, "SymSetContext");
	pSymGetLineFromAddr		= (tSymGetLineFromAddr)GetProcAddress(hDll, "SymGetLineFromAddr");
	pSymFromAddr			= (tSymFromAddr)GetProcAddress(hDll, "SymFromAddr");
	pStackWalk				= (tStackWalk)GetProcAddress(hDll, "StackWalk");
	pSymFunctionTableAccess	= (tSymFunctionTableAccess)GetProcAddress(hDll, "SymFunctionTableAccess");
	pSymGetModuleBase		= (tSymGetModuleBase)GetProcAddress(hDll, "SymGetModuleBase");
	pSymGetTypeInfo			= (tSymGetTypeInfo)GetProcAddress(hDll, "SymGetTypeInfo");
	pMiniDumpWriteDump		= (tMiniDumpWriteDump)GetProcAddress(hDll, "MiniDumpWriteDump");
	
	if (!pSymCleanup			||
		!pSymInitialize			||
		!pSymGetOptions			||
		!pSymSetOptions			||
		!pSymEnumSymbols		||
		!pSymSetContext			||
		!pSymGetLineFromAddr	||
		!pSymFromAddr			||
		!pStackWalk				||
		!pSymFunctionTableAccess||
		!pSymGetModuleBase		||
		!pSymGetTypeInfo		||
		!pMiniDumpWriteDump)
	{
		FreeLibrary(hDll);
		if ( m_previousFilter )
			return m_previousFilter( pExceptionInfo );
		else
			return EXCEPTION_CONTINUE_SEARCH;
	}
	
	if (::MessageBox( NULL, 
_T("An unhandled exception has occurred in FileZilla Server\r\n\
FileZilla Server has to be closed.\r\n\r\n\
Would you like to generate an exception report?\r\n\
The report contains all neccessary information about the exception,\r\n\
including a call stack with function parameters and local variables.\r\n\r\n\
If you're using the latest version of FileZilla Server, please send the generated exception record to the following mail address: Tim.Kosse@gmx.de\r\n\
The report will be analyzed and the reason for this exception will be fixed in the next version of FileZilla Server.\r\n\r\n\
Please note: It may be possible - though unlikely - that the exception report may contain personal and or confidential information. All exception reports will be processed higly confidential and solely to analyze the crash. The reports will be deleted immediately after processing.\r\n"),
		_T("FileZilla Server - Unhandled exception"), MB_APPLMODAL | MB_YESNO | MB_ICONSTOP)==IDYES)
	{
		m_hReportFile = CreateFile( m_szLogFileName,
										GENERIC_WRITE,
										FILE_SHARE_READ,
										0,
										CREATE_ALWAYS,
										FILE_FLAG_WRITE_THROUGH,
										0 );
		
		m_hDumpFile = CreateFile( m_szDmpFileName,
										GENERIC_WRITE,
										FILE_SHARE_READ,
										0,
										CREATE_ALWAYS,
										FILE_FLAG_WRITE_THROUGH,
										0 );
		
		if (!m_hReportFile)
		{
			TCHAR tmp[MAX_PATH];
			_tcscpy(tmp, m_szLogFileName);
			TCHAR *pos=_tcsrchr(tmp, '\\');
			if (pos)
			{
				pos++;
				_stprintf(m_szLogFileName, _T("c:\\%s"), pos);
			}
			else
				_stprintf(m_szLogFileName, _T("c:\\%s"), tmp);
			
			m_hReportFile = CreateFile( m_szLogFileName,
										GENERIC_WRITE,
										0,
										0,
										CREATE_ALWAYS,
										FILE_FLAG_WRITE_THROUGH,
										0 );
		
		}
		if (!m_hDumpFile && m_hReportFile)
		{
			TCHAR tmp[MAX_PATH];
			_tcscpy(tmp, m_szDmpFileName);
			TCHAR *pos=_tcsrchr(tmp, '\\');
			if (pos)
			{
				pos++;
				_stprintf(m_szDmpFileName, _T("c:\\%s"), pos);
			}
			else
				_stprintf(m_szDmpFileName, _T("c:\\%s"), tmp);
			
			m_hDumpFile = CreateFile( m_szDmpFileName,
										GENERIC_WRITE,
										0,
										0,
										CREATE_ALWAYS,
										FILE_FLAG_WRITE_THROUGH,
										0 );
		
		}
	
		int nError=0;
	    if ( m_hReportFile && m_hDumpFile)
	    {
		  #ifdef TRY
			TRY
		  #endif
			{
				writeMiniDump(pExceptionInfo);
				GenerateExceptionReport( pExceptionInfo );
	
				CloseHandle( m_hReportFile );
				m_hReportFile = 0;
			}
          #ifdef TRY
			CATCH_ALL(e);
			{
				nError=GetLastError();
				CloseHandle( m_hReportFile );
				m_hReportFile = 0;
			}
			END_CATCH_ALL
		  #endif
		}
		else
			nError=GetLastError();
		if (nError)
		{
			
			TCHAR tmp[1000];
			_stprintf(tmp, _T("Unable to create exception report, error code %d."), nError);
			MessageBox(0, tmp, _T("FileZilla Server - Unhandled eception"), MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
		}
		else
		{
			sendMail();

			TCHAR tmp[1000];
			_stprintf(tmp, _T("The exception report has been saved to \"%s\" and \"%s\".\n\
Please make sure that you are using the latest version of FileZilla Server.\n\
You can download the latest version from http://sourceforge.net/projects/filezilla/.\n\
If you do use the latest version, please send the exception report to Tim.Kosse@gmx.de along with a brief explanation what you did before FileZilla Server crashed."), m_szLogFileName, m_szDmpFileName);
			MessageBox(0, tmp, _T("FileZilla Server - Unhandled eception"), MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
		}
	}
	FreeLibrary(hDll);
    if ( m_previousFilter )
		return m_previousFilter( pExceptionInfo );
	else
        return EXCEPTION_CONTINUE_SEARCH;

}

//===========================================================================
// Open the report file, and write the desired information to it.  Called by 
// WheatyUnhandledExceptionFilter                                               
//===========================================================================
void WheatyExceptionReport::GenerateExceptionReport(
    PEXCEPTION_POINTERS pExceptionInfo )
{
    // Start out with a banner
    _tprintf(_T("//=====================================================\r\n"));
	_tprintf(_T("Exception report created by %s\r\n\r\n"), (LPCTSTR)GetVersionString());
	_tprintf(_T("System details:\r\n"));
	TCHAR buffer[200];
	if (DisplaySystemVersion(buffer))
		_tprintf(_T("%s\r\n"), buffer);
	CProcessorInfo pi;
	CMemoryInfo mi;
	_tprintf(_T("%s\r\n"), (LPCTSTR)pi.GetProcessorName());
	_tprintf(_T("%s\r\n\r\n"), (LPCTSTR)mi.GetMemoryInfo());
	


    PEXCEPTION_RECORD pExceptionRecord = pExceptionInfo->ExceptionRecord;

    // First print information about the type of fault
    _tprintf(   _T("Exception code: %08X %s\r\n"),
                pExceptionRecord->ExceptionCode,
                GetExceptionString(pExceptionRecord->ExceptionCode) );

    // Now print information about where the fault occured
    TCHAR szFaultingModule[MAX_PATH];
    DWORD section, offset;
    GetLogicalAddress(  pExceptionRecord->ExceptionAddress,
                        szFaultingModule,
                        sizeof( szFaultingModule ),
                        section, offset );

    _tprintf( _T("Fault address:  %08X %02X:%08X %s\r\n"),
                pExceptionRecord->ExceptionAddress,
                section, offset, szFaultingModule );

    PCONTEXT pCtx = pExceptionInfo->ContextRecord;

    // Show the registers
    #ifdef _M_IX86  // X86 Only!
    _tprintf( _T("\r\nRegisters:\r\n") );

    _tprintf(_T("EAX:%08X\r\nEBX:%08X\r\nECX:%08X\r\nEDX:%08X\r\nESI:%08X\r\nEDI:%08X\r\n")
            ,pCtx->Eax, pCtx->Ebx, pCtx->Ecx, pCtx->Edx,
            pCtx->Esi, pCtx->Edi );

    _tprintf( _T("CS:EIP:%04X:%08X\r\n"), pCtx->SegCs, pCtx->Eip );
    _tprintf( _T("SS:ESP:%04X:%08X  EBP:%08X\r\n"),
                pCtx->SegSs, pCtx->Esp, pCtx->Ebp );
    _tprintf( _T("DS:%04X  ES:%04X  FS:%04X  GS:%04X\r\n"),
                pCtx->SegDs, pCtx->SegEs, pCtx->SegFs, pCtx->SegGs );
    _tprintf( _T("Flags:%08X\r\n"), pCtx->EFlags );

    #endif

	// Set up the symbol engine.
	DWORD dwOpts = pSymGetOptions() ;

	// Turn on line loading and deferred loading.
	pSymSetOptions( dwOpts                |
                        SYMOPT_DEFERRED_LOADS |
                        SYMOPT_LOAD_LINES      ) ;
	
    // Initialize DbgHelp
    if ( !pSymInitialize( GetCurrentProcess(), 0, TRUE ) )
        return;

    CONTEXT trashableContext = *pCtx;

    WriteStackDetails( &trashableContext, false );

    #ifdef _M_IX86  // X86 Only!

    _tprintf( _T("========================\r\n") );
    _tprintf( _T("Local Variables And Parameters\r\n") );

    trashableContext = *pCtx;
    WriteStackDetails( &trashableContext, true );

    /*_tprintf( _T("========================\r\n") );
    _tprintf( _T("Global Variables\r\n") );

    SymEnumSymbols( GetCurrentProcess(),
                    (DWORD64)GetModuleHandle(szFaultingModule),
                    0, EnumerateSymbolsCallback, 0 );
    */
    #endif      // X86 Only!

    pSymCleanup( GetCurrentProcess() );

    _tprintf( _T("\r\n") );
}

//======================================================================
// Given an exception code, returns a pointer to a static string with a 
// description of the exception                                         
//======================================================================
LPTSTR WheatyExceptionReport::GetExceptionString( DWORD dwCode )
{
    #define EXCEPTION( x ) case EXCEPTION_##x: return _T(#x);

    switch ( dwCode )
    {
        EXCEPTION( ACCESS_VIOLATION )
        EXCEPTION( DATATYPE_MISALIGNMENT )
        EXCEPTION( BREAKPOINT )
        EXCEPTION( SINGLE_STEP )
        EXCEPTION( ARRAY_BOUNDS_EXCEEDED )
        EXCEPTION( FLT_DENORMAL_OPERAND )
        EXCEPTION( FLT_DIVIDE_BY_ZERO )
        EXCEPTION( FLT_INEXACT_RESULT )
        EXCEPTION( FLT_INVALID_OPERATION )
        EXCEPTION( FLT_OVERFLOW )
        EXCEPTION( FLT_STACK_CHECK )
        EXCEPTION( FLT_UNDERFLOW )
        EXCEPTION( INT_DIVIDE_BY_ZERO )
        EXCEPTION( INT_OVERFLOW )
        EXCEPTION( PRIV_INSTRUCTION )
        EXCEPTION( IN_PAGE_ERROR )
        EXCEPTION( ILLEGAL_INSTRUCTION )
        EXCEPTION( NONCONTINUABLE_EXCEPTION )
        EXCEPTION( STACK_OVERFLOW )
        EXCEPTION( INVALID_DISPOSITION )
        EXCEPTION( GUARD_PAGE )
        EXCEPTION( INVALID_HANDLE )
    }

    // If not one of the "known" exceptions, try to get the string
    // from NTDLL.DLL's message table.

    static TCHAR szBuffer[512] = { 0 };

    FormatMessage( FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE,
                   GetModuleHandle( _T("NTDLL.DLL") ),
                   dwCode, 0, szBuffer, sizeof( szBuffer ), 0 );

    return szBuffer;
}

//=============================================================================
// Given a linear address, locates the module, section, and offset containing  
// that address.                                                               
//                                                                             
// Note: the szModule paramater buffer is an output buffer of length specified 
// by the len parameter (in characters!)                                       
//=============================================================================
BOOL WheatyExceptionReport::GetLogicalAddress(
        PVOID addr, PTSTR szModule, DWORD len, DWORD& section, DWORD& offset )
{
    MEMORY_BASIC_INFORMATION mbi;

    if ( !VirtualQuery( addr, &mbi, sizeof(mbi) ) )
        return FALSE;

    DWORD hMod = (DWORD)mbi.AllocationBase;

    if ( !GetModuleFileName( (HMODULE)hMod, szModule, len ) )
        return FALSE;

    // Point to the DOS header in memory
    PIMAGE_DOS_HEADER pDosHdr = (PIMAGE_DOS_HEADER)hMod;

    // From the DOS header, find the NT (PE) header
    PIMAGE_NT_HEADERS pNtHdr = (PIMAGE_NT_HEADERS)(hMod + pDosHdr->e_lfanew);

    PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION( pNtHdr );

    DWORD rva = (DWORD)addr - hMod; // RVA is offset from module load address

    // Iterate through the section table, looking for the one that encompasses
    // the linear address.
    for (   unsigned i = 0;
            i < pNtHdr->FileHeader.NumberOfSections;
            i++, pSection++ )
    {
        DWORD sectionStart = pSection->VirtualAddress;
        DWORD sectionEnd = sectionStart
                    + max(pSection->SizeOfRawData, pSection->Misc.VirtualSize);

        // Is the address in this section???
        if ( (rva >= sectionStart) && (rva <= sectionEnd) )
        {
            // Yes, address is in the section.  Calculate section and offset,
            // and store in the "section" & "offset" params, which were
            // passed by reference.
            section = i+1;
            offset = rva - sectionStart;
            return TRUE;
        }
    }

    return FALSE;   // Should never get here!
}

//============================================================
// Walks the stack, and writes the results to the report file 
//============================================================
void WheatyExceptionReport::WriteStackDetails(
        PCONTEXT pContext,
        bool bWriteVariables )  // true if local/params should be output
{
	USES_CONVERSION;
    _tprintf( _T("\r\nCall stack:\r\n") );

    _tprintf( _T("Address   Frame     Function            SourceFile\r\n") );

    DWORD dwMachineType = 0;
    // Could use SymSetOptions here to add the SYMOPT_DEFERRED_LOADS flag

    STACKFRAME sf;
    memset( &sf, 0, sizeof(sf) );

    #ifdef _M_IX86
    // Initialize the STACKFRAME structure for the first call.  This is only
    // necessary for Intel CPUs, and isn't mentioned in the documentation.
    sf.AddrPC.Offset       = pContext->Eip;
    sf.AddrPC.Mode         = AddrModeFlat;
    sf.AddrStack.Offset    = pContext->Esp;
    sf.AddrStack.Mode      = AddrModeFlat;
    sf.AddrFrame.Offset    = pContext->Ebp;
    sf.AddrFrame.Mode      = AddrModeFlat;

    dwMachineType = IMAGE_FILE_MACHINE_I386;
    #endif

    while ( 1 )
    {
        // Get the next stack frame
        if ( ! pStackWalk(  dwMachineType,
                            m_hProcess,
                            GetCurrentThread(),
                            &sf,
                            pContext,
                            0,
                            pSymFunctionTableAccess,
                            pSymGetModuleBase,
                            0 ) )
            break;

        if ( 0 == sf.AddrFrame.Offset ) // Basic sanity check to make sure
            break;                      // the frame is OK.  Bail if not.

        _tprintf( _T("%08X  %08X  "), sf.AddrPC.Offset, sf.AddrFrame.Offset );

        // Get the name of the function for this stack frame entry
        BYTE symbolBuffer[ sizeof(SYMBOL_INFO) + 1024 ];
        PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)symbolBuffer;
        pSymbol->SizeOfStruct = sizeof(symbolBuffer);
        pSymbol->MaxNameLen = 1024;
                        
        DWORD64 symDisplacement = 0;    // Displacement of the input address,
                                        // relative to the start of the symbol

        if ( pSymFromAddr(m_hProcess,sf.AddrPC.Offset,&symDisplacement,pSymbol))
        {
            _tprintf( _T("%hs+%I64X"), pSymbol->Name, symDisplacement );
            
        }
        else    // No symbol found.  Print out the logical address instead.
        {
            TCHAR szModule[MAX_PATH] = _T("");
            DWORD section = 0, offset = 0;

            GetLogicalAddress(  (PVOID)sf.AddrPC.Offset,
                                szModule, sizeof(szModule), section, offset );

            _tprintf( _T("%04X:%08X %s"), section, offset, szModule );
        }

        // Get the source line for this stack frame entry
        IMAGEHLP_LINE lineInfo = { sizeof(IMAGEHLP_LINE) };
        DWORD dwLineDisplacement;
        if ( pSymGetLineFromAddr( m_hProcess, sf.AddrPC.Offset,
                                &dwLineDisplacement, &lineInfo ) )
        {
            _tprintf(_T("  %s line %u"), A2T(lineInfo.FileName), lineInfo.LineNumber); 
        }

        _tprintf( _T("\r\n") );

        // Write out the variables, if desired
        if ( bWriteVariables )
        {
            // Use SymSetContext to get just the locals/params for this frame
            IMAGEHLP_STACK_FRAME imagehlpStackFrame;
            imagehlpStackFrame.InstructionOffset = sf.AddrPC.Offset;
            pSymSetContext( m_hProcess, &imagehlpStackFrame, 0 );

            // Enumerate the locals/parameters
            pSymEnumSymbols( m_hProcess, 0, 0, EnumerateSymbolsCallback, &sf );

            _tprintf( _T("\r\n") );
        }
    }

}

//////////////////////////////////////////////////////////////////////////////
// The function invoked by SymEnumSymbols
//////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK
WheatyExceptionReport::EnumerateSymbolsCallback(
    PSYMBOL_INFO  pSymInfo,
    ULONG         SymbolSize,
    PVOID         UserContext )
{
	USES_CONVERSION;
    TCHAR szBuffer[4096];

    __try
    {
        if ( FormatSymbolValue( pSymInfo, (STACKFRAME*)UserContext,
                                szBuffer, sizeof(szBuffer) ) )  
            _tprintf( _T("\t%s\r\n"), szBuffer );
    }
    __except( 1 )
    {
        _tprintf( _T("punting on symbol %s\r\n"), A2T(pSymInfo->Name) );
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
// Given a SYMBOL_INFO representing a particular variable, displays its
// contents.  If it's a user defined type, display the members and their
// values.
//////////////////////////////////////////////////////////////////////////////
bool WheatyExceptionReport::FormatSymbolValue(
            PSYMBOL_INFO pSym,
            STACKFRAME * sf,
            TCHAR * pszBuffer,
            unsigned cbBuffer )
{
	USES_CONVERSION;
    TCHAR * pszCurrBuffer = pszBuffer;

    // Indicate if the variable is a local or parameter
    if ( pSym->Flags & IMAGEHLP_SYMBOL_INFO_PARAMETER )
        pszCurrBuffer += _stprintf( pszCurrBuffer, _T("Parameter ") );
    else if ( pSym->Flags & IMAGEHLP_SYMBOL_INFO_LOCAL )
        pszCurrBuffer += _stprintf( pszCurrBuffer, _T("Local ") );

    // If it's a function, don't do anything.
    if ( pSym->Tag == 5 )   // SymTagFunction from CVCONST.H from the DIA SDK
        return false;

    // Emit the variable name
    pszCurrBuffer += _stprintf( pszCurrBuffer, _T("\'%s\'"), A2T(pSym->Name) );

    DWORD_PTR pVariable = 0;    // Will point to the variable's data in memory

    if ( pSym->Flags & IMAGEHLP_SYMBOL_INFO_REGRELATIVE )
    {
        if ( pSym->Register == 8 )   // EBP is the value 8 (in DBGHELP 5.1)
        {                               //  This may change!!!
            pVariable = sf->AddrFrame.Offset;
            pVariable += (DWORD_PTR)pSym->Address;
        }
        else
          return false;
    }
    else if ( pSym->Flags & IMAGEHLP_SYMBOL_INFO_REGISTER )
    {
        return false;   // Don't try to report register variable
    }
    else
    {
        pVariable = (DWORD_PTR)pSym->Address;   // It must be a global variable
    }

    // Determine if the variable is a user defined type (UDT).  IF so, bHandled
    // will return true.
    bool bHandled;
    pszCurrBuffer = DumpTypeIndex(pszCurrBuffer, pSym->ModBase, pSym->TypeIndex,
                                    0, pVariable, bHandled );

    if ( !bHandled )
    {
	    // The symbol wasn't a UDT, so do basic, stupid formatting of the
        // variable.  Based on the size, we're assuming it's a char, WORD, or
        // DWORD.
        BasicType basicType = GetBasicType( pSym->TypeIndex, pSym->ModBase );
        
        pszCurrBuffer = FormatOutputValue(pszCurrBuffer, basicType, pSym->Size,
                                            (PVOID)pVariable ); 
    }


    return true;
}

//////////////////////////////////////////////////////////////////////////////
// If it's a user defined type (UDT), recurse through its members until we're
// at fundamental types.  When he hit fundamental types, return
// bHandled = false, so that FormatSymbolValue() will format them.
//////////////////////////////////////////////////////////////////////////////
TCHAR * WheatyExceptionReport::DumpTypeIndex(
        TCHAR * pszCurrBuffer,
        DWORD64 modBase,
        DWORD dwTypeIndex,
        unsigned nestingLevel,
        DWORD_PTR offset,
        bool & bHandled )
{
	USES_CONVERSION;
    bHandled = false;

    // Get the name of the symbol.  This will either be a Type name (if a UDT),
    // or the structure member name.
    WCHAR * pwszTypeName;
    if ( pSymGetTypeInfo( m_hProcess, modBase, dwTypeIndex, TI_GET_SYMNAME,
                        &pwszTypeName ) )
    {
        pszCurrBuffer += _stprintf( pszCurrBuffer, _T(" %ls"), W2T(pwszTypeName) );
        LocalFree( pwszTypeName );
    }

    // Determine how many children this type has.
    DWORD dwChildrenCount = 0;
    pSymGetTypeInfo( m_hProcess, modBase, dwTypeIndex, TI_GET_CHILDRENCOUNT,
                    &dwChildrenCount );

    if ( !dwChildrenCount )     // If no children, we're done
	{
        return pszCurrBuffer;
	}
    // Prepare to get an array of "TypeIds", representing each of the children.
    // SymGetTypeInfo(TI_FINDCHILDREN) expects more memory than just a
    // TI_FINDCHILDREN_PARAMS struct has.  Use derivation to accomplish this.
    struct FINDCHILDREN : TI_FINDCHILDREN_PARAMS
    {
        ULONG   MoreChildIds[1024];
        FINDCHILDREN(){Count = sizeof(MoreChildIds) / sizeof(MoreChildIds[0]);}
    } children;

    children.Count = dwChildrenCount;
    children.Start= 0;

    // Get the array of TypeIds, one for each child type
    if ( !pSymGetTypeInfo( m_hProcess, modBase, dwTypeIndex, TI_FINDCHILDREN,
                            &children ) )
    {
        return pszCurrBuffer;
    }

    // Append a line feed
    pszCurrBuffer += _stprintf( pszCurrBuffer, _T("\r\n") );

    // Iterate through each of the children
    for ( unsigned i = 0; i < dwChildrenCount; i++ )
    {
        // Add appropriate indentation level (since this routine is recursive)
        for ( unsigned j = 0; j <= nestingLevel+1; j++ )
            pszCurrBuffer += _stprintf( pszCurrBuffer, _T("\t") );

        // Recurse for each of the child types
        bool bHandled2;
        pszCurrBuffer = DumpTypeIndex( pszCurrBuffer, modBase,
                                        children.ChildId[i], nestingLevel+1,
                                        offset, bHandled2 );

        // If the child wasn't a UDT, format it appropriately
        if ( !bHandled2 )
        {
            // Get the offset of the child member, relative to its parent
            DWORD dwMemberOffset;
            pSymGetTypeInfo( m_hProcess, modBase, children.ChildId[i],
                            TI_GET_OFFSET, &dwMemberOffset );

            // Get the real "TypeId" of the child.  We need this for the
            // SymGetTypeInfo( TI_GET_TYPEID ) call below.
            DWORD typeId;
            pSymGetTypeInfo( m_hProcess, modBase, children.ChildId[i],
                            TI_GET_TYPEID, &typeId );

            // Get the size of the child member
            ULONG64 length;
            pSymGetTypeInfo(m_hProcess, modBase, typeId, TI_GET_LENGTH,&length);

            // Calculate the address of the member
            DWORD_PTR dwFinalOffset = offset + dwMemberOffset;

            BasicType basicType = GetBasicType(children.ChildId[i], modBase );

            pszCurrBuffer = FormatOutputValue( pszCurrBuffer, basicType,
                                                length, (PVOID)dwFinalOffset ); 

            pszCurrBuffer += _stprintf( pszCurrBuffer, _T("\r\n") );
        }
    }

    bHandled = true;
    return pszCurrBuffer;
}

TCHAR * WheatyExceptionReport::FormatOutputValue(   TCHAR * pszCurrBuffer,
                                                    BasicType basicType,
                                                    DWORD64 length,
                                                    PVOID pAddress )
{
    // Format appropriately (assuming it's a 1, 2, or 4 bytes (!!!)
    if ( length == 1 )
        pszCurrBuffer += _stprintf( pszCurrBuffer, _T(" = %X"), *(PBYTE)pAddress );
    else if ( length == 2 )
        pszCurrBuffer += _stprintf( pszCurrBuffer, _T(" = %X"), *(PWORD)pAddress );
    else if ( length == 4 )
    {
        if ( basicType == btFloat )
        {
            pszCurrBuffer += _stprintf(pszCurrBuffer, _T(" = %f"), *(PFLOAT)pAddress);
        }
        else if ( basicType == btChar )
        {
            if ( !IsBadStringPtr( *(LPTSTR*)pAddress, 32) )
            {
                pszCurrBuffer += _stprintf( pszCurrBuffer, _T(" = \"%.31s\""),
                                            *(PDWORD)pAddress );
            }
            else
                pszCurrBuffer += _stprintf( pszCurrBuffer, _T(" = %X"),
                                            *(PDWORD)pAddress );
        }
        else
		{
            pszCurrBuffer += _stprintf(pszCurrBuffer, _T(" = %X"), *(PDWORD)pAddress);
		}
    }
    else if ( length == 8 )
    {
        if ( basicType == btFloat )
        {
            pszCurrBuffer += _stprintf( pszCurrBuffer, _T(" = %lf"),
                                        *(double *)pAddress );
        }
        else
            pszCurrBuffer += _stprintf( pszCurrBuffer, _T(" = %I64X"),
                                        *(DWORD64*)pAddress );
    }

    return pszCurrBuffer;
}

BasicType
WheatyExceptionReport::GetBasicType( DWORD typeIndex, DWORD64 modBase )
{
    BasicType basicType;
    if ( pSymGetTypeInfo( m_hProcess, modBase, typeIndex,
                        TI_GET_BASETYPE, &basicType ) )
    {
        return basicType;
    }

    // Get the real "TypeId" of the child.  We need this for the
    // SymGetTypeInfo( TI_GET_TYPEID ) call below.
    DWORD typeId;
    if (pSymGetTypeInfo(m_hProcess,modBase, typeIndex, TI_GET_TYPEID, &typeId))
    {
        if ( pSymGetTypeInfo( m_hProcess, modBase, typeId, TI_GET_BASETYPE,
                            &basicType ) )
        {
            return basicType;
        }
    }

    return btNoType;
}

//============================================================================
// Helper function that writes to the report file, and allows the user to use 
// printf style formating                                                     
//============================================================================
int __cdecl WheatyExceptionReport::_tprintf(const TCHAR * format, ...)
{
    TCHAR szBuff[1024];
    int retValue;
    DWORD cbWritten;
    va_list argptr;
          
    va_start( argptr, format );
    retValue = _vstprintf( szBuff, format, argptr );
    va_end( argptr );

    WriteFile(m_hReportFile, szBuff, retValue * sizeof(TCHAR), &cbWritten, 0 );

    return retValue;
}

bool WheatyExceptionReport::writeMiniDump(PEXCEPTION_POINTERS pExceptionInfo)
{
	//
	// Write the minidump to the file
	//
	MINIDUMP_EXCEPTION_INFORMATION eInfo;
	eInfo.ThreadId = GetCurrentThreadId();
	eInfo.ExceptionPointers = pExceptionInfo;
	eInfo.ClientPointers = FALSE;

	MINIDUMP_CALLBACK_INFORMATION cbMiniDump;
	cbMiniDump.CallbackRoutine = 0;
	cbMiniDump.CallbackParam = 0;


	pMiniDumpWriteDump(
		GetCurrentProcess(),
		GetCurrentProcessId(),
		m_hDumpFile,
		MiniDumpNormal,
		pExceptionInfo ? &eInfo : NULL,
		NULL,
		&cbMiniDump);

	// Close file
	CloseHandle(m_hDumpFile);

	return true;
}

int WheatyExceptionReport::sendMail()
{
	CMailMsg mail;

	mail.SetTo(_T("tim.kosse@gmx.de"), _T("Tim Kosse"));

	TCHAR str[4096];
	_stprintf(str, _T("Exception report created by %s\r\n\r\n"), (LPCTSTR)GetVersionString());
	mail.SetSubject(str);

	mail.SetMessage(_T("Enter your comments here, what did you do with FileZilla Server before it crashed?"));

	mail.AddAttachment(m_szLogFileName, _T("FileZilla Server.rpt"));
	mail.AddAttachment(m_szDmpFileName, _T("FileZilla Server.dmp"));

	return mail.Send();
}