#pragma once

#include <dbghelp.h>

enum BasicType  // Stolen from CVCONST.H in the DIA 2.0 SDK
{
    btNoType = 0,
    btVoid = 1,
    btChar = 2,
    btWChar = 3,
    btInt = 6,
    btUInt = 7,
    btFloat = 8,
    btBCD = 9,
    btBool = 10,
    btLong = 13,
    btULong = 14,
    btCurrency = 25,
    btDate = 26,
    btVariant = 27,
    btComplex = 28,
    btBit = 29,
    btBSTR = 30,
    btHresult = 31
};

class WheatyExceptionReport
{
    public:
    
    WheatyExceptionReport( );
    ~WheatyExceptionReport( );
    
    void SetLogFileName( PTSTR pszLogFileName );


    // entry point where control comes on an unhandled exception
    static LONG WINAPI WheatyUnhandledExceptionFilter(
                                PEXCEPTION_POINTERS pExceptionInfo );

    private:

    // where report info is extracted and generated 
    static void GenerateExceptionReport( PEXCEPTION_POINTERS pExceptionInfo );

    // Helper functions
    static LPTSTR GetExceptionString( DWORD dwCode );
    static BOOL GetLogicalAddress(  PVOID addr, PTSTR szModule, DWORD len,
                                    DWORD& section, DWORD& offset );

    static void WriteStackDetails( PCONTEXT pContext, bool bWriteVariables );

    static BOOL CALLBACK EnumerateSymbolsCallback(PSYMBOL_INFO,ULONG, PVOID);

    static bool FormatSymbolValue( PSYMBOL_INFO, STACKFRAME *, TCHAR * pszBuffer, unsigned cbBuffer );

    static TCHAR * DumpTypeIndex( TCHAR *, DWORD64, DWORD, unsigned, DWORD_PTR, bool & );

    static TCHAR * FormatOutputValue( TCHAR * pszCurrBuffer, BasicType basicType, DWORD64 length, PVOID pAddress );
    
    static BasicType GetBasicType( DWORD typeIndex, DWORD64 modBase );

    static int __cdecl _tprintf(const TCHAR * format, ...);

	static bool writeMiniDump(PEXCEPTION_POINTERS pExceptionInfo);

	static int sendMail();

    // Variables used by the class
    static TCHAR m_szLogFileName[MAX_PATH];
    static TCHAR m_szDmpFileName[MAX_PATH];
    static LPTOP_LEVEL_EXCEPTION_FILTER m_previousFilter;
    static HANDLE m_hReportFile;
    static HANDLE m_hDumpFile;
    static HANDLE m_hProcess;
};