#ifndef _EMU_KERNEL32_H_
#define _EMU_KERNEL32_H_

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#include <stdarg.h>

#define MAX_LEADBYTES             12
#define MAX_DEFAULTCHAR           2

#if defined (TARGET_POSIX)
typedef struct _STARTUPINFOA
{
  DWORD cb;
  LPSTR lpReserved;
  LPSTR lpDesktop;
  LPSTR lpTitle;
  DWORD dwX;
  DWORD dwY;
  DWORD dwXSize;
  DWORD dwYSize;
  DWORD dwXCountChars;
  DWORD dwYCountChars;
  DWORD dwFillAttribute;
  DWORD dwFlags;
  WORD wShowWindow;
  WORD cbReserved2;
  LPBYTE lpReserved2;
  HANDLE hStdInput;
  HANDLE hStdOutput;
  HANDLE hStdError;
}
STARTUPINFOA, *LPSTARTUPINFOA;

typedef struct _cpinfo
{
  UINT MaxCharSize;
  BYTE DefaultChar[MAX_DEFAULTCHAR];
  BYTE LeadByte[MAX_LEADBYTES];
}
CPINFO, *LPCPINFO;

#define STD_INPUT_HANDLE        ((DWORD) -10)
#define STD_OUTPUT_HANDLE       ((DWORD) -11)
#define STD_ERROR_HANDLE        ((DWORD) -12)

#define ERROR_INVALID_PARAMETER (87L)

//SYSTEM_INFO definition take from Winnt headers.
typedef struct _SYSTEM_INFO
{
  union {
    DWORD dwOemId;          // Obsolete field...do not use
    struct
    {
      WORD wProcessorArchitecture;
      WORD wReserved;
    } x;
  };
  DWORD dwPageSize;
  LPVOID lpMinimumApplicationAddress;
  LPVOID lpMaximumApplicationAddress;
  DWORD_PTR dwActiveProcessorMask;
  DWORD dwNumberOfProcessors;
  DWORD dwProcessorType;
  DWORD dwAllocationGranularity;
  WORD wProcessorLevel;
  WORD wProcessorRevision;
}
SYSTEM_INFO, *LPSYSTEM_INFO;
#endif

typedef DWORD LCTYPE;
#if defined (TARGET_POSIX)
typedef BOOL (*PHANDLER_ROUTINE)(DWORD);

typedef struct _OSVERSIONINFO
{
  DWORD dwOSVersionInfoSize;
  DWORD dwMajorVersion;
  DWORD dwMinorVersion;
  DWORD dwBuildNumber;
  DWORD dwPlatformId;
  char szCSDVersion[128];
}
OSVERSIONINFO, *LPOSVERSIONINFO;

typedef struct _OSVERSIONINFOW
{
  DWORD dwOSVersionInfoSize;
  DWORD dwMajorVersion;
  DWORD dwMinorVersion;
  DWORD dwBuildNumber;
  DWORD dwPlatformId;
  WCHAR szCSDVersion[128];
}
OSVERSIONINFOW, *LPOSVERSIONINFOW;

#ifdef TARGET_POSIX
#define EXCEPTION_MAXIMUM_PARAMETERS 15
typedef struct _EXCEPTION_RECORD {
  DWORD ExceptionCode;
  DWORD ExceptionFlags;
  struct _EXCEPTION_RECORD* ExceptionRecord;
  PVOID ExceptionAddress;
  DWORD NumberParameters;
  ULONG_PTR ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} EXCEPTION_RECORD,
 *PEXCEPTION_RECORD;

#define LPOVERLAPPED void *
#endif

#endif

#ifdef TARGET_POSIX
#define LANG_ENGLISH 0x09
#define SUBLANG_ENGLISH_US 0x01
#define MAKELCID(lgid, srtid)  ((DWORD)((((DWORD)((WORD  )(srtid))) << 16) | ((DWORD)((WORD  )(lgid)))))
#define MAKELANGID(p, s)       ((((WORD  )(s)) << 10) | (WORD  )(p))
#define SORT_DEFAULT 0x00

#define LANG_NEUTRAL 0x00
#define SUBLANG_NEUTRAL 0x00
#define SUBLANG_DEFAULT 0x01
#define SUBLANG_SYS_DEFAULT 0x02

#define LANG_SYSTEM_DEFAULT    (MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT))
#define LANG_USER_DEFAULT      (MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT))
#define LOCALE_SYSTEM_DEFAULT  (MAKELCID(LANG_SYSTEM_DEFAULT, SORT_DEFAULT))
#define LOCALE_USER_DEFAULT    (MAKELCID(LANG_USER_DEFAULT, SORT_DEFAULT))
#define LOCALE_NEUTRAL         (MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), SORT_DEFAULT))

#define PROCESSOR_ARCHITECTURE_UNKNOWN 0xFFFF

#define PF_FLOATING_POINT_PRECISION_ERRATA  0
#define PF_FLOATING_POINT_EMULATED          1
#define PF_COMPARE_EXCHANGE_DOUBLE          2
#define PF_MMX_INSTRUCTIONS_AVAILABLE       3
#define PF_PPC_MOVEMEM_64BIT_OK             4
#define PF_ALPHA_BYTE_INSTRUCTIONS          5
#define PF_XMMI_INSTRUCTIONS_AVAILABLE      6
#define PF_3DNOW_INSTRUCTIONS_AVAILABLE     7
#define PF_RDTSC_INSTRUCTION_AVAILABLE      8
#define PF_PAE_ENABLED                      9

#define ERROR_INVALID_FUNCTION 1

#define HGLOBAL void *
#define HRSRC   void *

#endif

#define ATOM unsigned short

// LOCAL defines from mingw
#define MAX_LEADBYTES 	12
#define MAX_DEFAULTCHAR	2
#if defined (TARGET_POSIX)
#define LOCALE_NOUSEROVERRIDE	0x80000000
#define LOCALE_USE_CP_ACP	0x40000000
#if (WINVER >= 0x0400)
#define LOCALE_RETURN_NUMBER	0x20000000
#endif
#define LOCALE_ILANGUAGE	1
#define LOCALE_SLANGUAGE	2
#define LOCALE_SENGLANGUAGE	0x1001
#define LOCALE_SABBREVLANGNAME	3
#define LOCALE_SNATIVELANGNAME	4
#define LOCALE_ICOUNTRY	5
#define LOCALE_SCOUNTRY	6
#define LOCALE_SENGCOUNTRY	0x1002
#define LOCALE_SABBREVCTRYNAME	7
#define LOCALE_SNATIVECTRYNAME	8
#define LOCALE_IDEFAULTLANGUAGE	9
#define LOCALE_IDEFAULTCOUNTRY	10
#define LOCALE_IDEFAULTCODEPAGE	11
#define LOCALE_IDEFAULTANSICODEPAGE 0x1004
#define LOCALE_SLIST	12
#define LOCALE_IMEASURE	13
#define LOCALE_SDECIMAL	14
#define LOCALE_STHOUSAND	15
#define LOCALE_SGROUPING	16
#define LOCALE_IDIGITS	17
#define LOCALE_ILZERO	18
#define LOCALE_INEGNUMBER	0x1010
#define LOCALE_SNATIVEDIGITS	19
#define LOCALE_SCURRENCY	20
#define LOCALE_SINTLSYMBOL	21
#define LOCALE_SMONDECIMALSEP	22
#define LOCALE_SMONTHOUSANDSEP	23
#define LOCALE_SMONGROUPING	24
#define LOCALE_ICURRDIGITS	25
#define LOCALE_IINTLCURRDIGITS	26
#define LOCALE_ICURRENCY	27
#define LOCALE_INEGCURR	28
#define LOCALE_SDATE	29
#define LOCALE_STIME	30
#define LOCALE_SSHORTDATE	31
#define LOCALE_SLONGDATE	32
#define LOCALE_STIMEFORMAT	0x1003
#define LOCALE_IDATE	33
#define LOCALE_ILDATE	34
#define LOCALE_ITIME	35
#define LOCALE_ITIMEMARKPOSN	0x1005
#define LOCALE_ICENTURY	36
#define LOCALE_ITLZERO	37
#define LOCALE_IDAYLZERO	38
#define LOCALE_IMONLZERO	39
#define LOCALE_S1159	40
#define LOCALE_S2359	41
#define LOCALE_ICALENDARTYPE	0x1009
#define LOCALE_IOPTIONALCALENDAR	0x100B
#define LOCALE_IFIRSTDAYOFWEEK	0x100C
#define LOCALE_IFIRSTWEEKOFYEAR	0x100D
#define LOCALE_SDAYNAME1	42
#define LOCALE_SDAYNAME2	43
#define LOCALE_SDAYNAME3	44
#define LOCALE_SDAYNAME4	45
#define LOCALE_SDAYNAME5	46
#define LOCALE_SDAYNAME6	47
#define LOCALE_SDAYNAME7	48
#define LOCALE_SABBREVDAYNAME1	49
#define LOCALE_SABBREVDAYNAME2	50
#define LOCALE_SABBREVDAYNAME3	51
#define LOCALE_SABBREVDAYNAME4	52
#define LOCALE_SABBREVDAYNAME5	53
#define LOCALE_SABBREVDAYNAME6	54
#define LOCALE_SABBREVDAYNAME7	55
#define LOCALE_SMONTHNAME1	56
#define LOCALE_SMONTHNAME2	57
#define LOCALE_SMONTHNAME3	58
#define LOCALE_SMONTHNAME4	59
#define LOCALE_SMONTHNAME5	60
#define LOCALE_SMONTHNAME6	61
#define LOCALE_SMONTHNAME7	62
#define LOCALE_SMONTHNAME8	63
#define LOCALE_SMONTHNAME9	64
#define LOCALE_SMONTHNAME10	65
#define LOCALE_SMONTHNAME11	66
#define LOCALE_SMONTHNAME12	67
#define LOCALE_SMONTHNAME13	0x100E
#define LOCALE_SABBREVMONTHNAME1	68
#define LOCALE_SABBREVMONTHNAME2	69
#define LOCALE_SABBREVMONTHNAME3	70
#define LOCALE_SABBREVMONTHNAME4	71
#define LOCALE_SABBREVMONTHNAME5	72
#define LOCALE_SABBREVMONTHNAME6	73
#define LOCALE_SABBREVMONTHNAME7	74
#define LOCALE_SABBREVMONTHNAME8	75
#define LOCALE_SABBREVMONTHNAME9	76
#define LOCALE_SABBREVMONTHNAME10	77
#define LOCALE_SABBREVMONTHNAME11	78
#define LOCALE_SABBREVMONTHNAME12	79
#define LOCALE_SABBREVMONTHNAME13	0x100F
#define LOCALE_SPOSITIVESIGN	80
#define LOCALE_SNEGATIVESIGN	81
#define LOCALE_IPOSSIGNPOSN	82
#define LOCALE_INEGSIGNPOSN	83
#define LOCALE_IPOSSYMPRECEDES	84
#define LOCALE_IPOSSEPBYSPACE	85
#define LOCALE_INEGSYMPRECEDES	86
#define LOCALE_INEGSEPBYSPACE	87
#define LOCALE_FONTSIGNATURE	88
#define LOCALE_SISO639LANGNAME 89
#define LOCALE_SISO3166CTRYNAME 90
//#define LOCALE_SYSTEM_DEFAULT	0x800
//#define LOCALE_USER_DEFAULT	0x400
#define NORM_IGNORECASE	1
#define NORM_IGNOREKANATYPE	65536
#define NORM_IGNORENONSPACE	2
#define NORM_IGNORESYMBOLS	4
#define NORM_IGNOREWIDTH	131072
#define SORT_STRINGSORT	4096
#define LCMAP_LOWERCASE 0x00000100
#define LCMAP_UPPERCASE 0x00000200
#define LCMAP_SORTKEY 0x00000400
#define LCMAP_BYTEREV 0x00000800
#define LCMAP_HIRAGANA 0x00100000
#define LCMAP_KATAKANA 0x00200000
#define LCMAP_HALFWIDTH 0x00400000
#define LCMAP_FULLWIDTH 0x00800000
#define LCMAP_LINGUISTIC_CASING 0x01000000
#define LCMAP_SIMPLIFIED_CHINESE 0x02000000
#define LCMAP_TRADITIONAL_CHINESE 0x04000000
#define ENUM_ALL_CALENDARS (-1)
#define DATE_SHORTDATE 1
#define DATE_LONGDATE 2
#define DATE_USE_ALT_CALENDAR 4
#define CP_INSTALLED 1
#define CP_SUPPORTED 2
#define LCID_INSTALLED 1
#define LCID_SUPPORTED 2
#define LCID_ALTERNATE_SORTS 4
#define MAP_FOLDCZONE 16
#define MAP_FOLDDIGITS 128
#define MAP_PRECOMPOSED 32
#define MAP_COMPOSITE 64
#define CP_ACP 0
#define CP_OEMCP 1
#define CP_MACCP 2
#define CP_THREAD_ACP 3
#define CP_SYMBOL 42
#define CP_UTF7 65000
#define CP_UTF8 65001
#define CT_CTYPE1 1
#define CT_CTYPE2 2
#define CT_CTYPE3 4
#define C1_UPPER 1
#define C1_LOWER 2
#define C1_DIGIT 4
#define C1_SPACE 8
#define C1_PUNCT 16
#define C1_CNTRL 32
#define C1_BLANK 64
#define C1_XDIGIT 128
#define C1_ALPHA 256
#define C2_LEFTTORIGHT 1
#define C2_RIGHTTOLEFT 2
#define C2_EUROPENUMBER 3
#define C2_EUROPESEPARATOR 4
#define C2_EUROPETERMINATOR 5
#define C2_ARABICNUMBER 6
#define C2_COMMONSEPARATOR 7
#define C2_BLOCKSEPARATOR 8
#define C2_SEGMENTSEPARATOR 9
#define C2_WHITESPACE 10
#define C2_OTHERNEUTRAL 11
#define C2_NOTAPPLICABLE 0
#define C3_NONSPACING 1
#define C3_DIACRITIC 2
#define C3_VOWELMARK 4
#define C3_SYMBOL 8
#define C3_KATAKANA 16
#define C3_HIRAGANA 32
#define C3_HALFWIDTH 64
#define C3_FULLWIDTH 128
#define C3_IDEOGRAPH 256
#define C3_KASHIDA 512
#define C3_LEXICAL 1024
#define C3_ALPHA 32768
#define C3_NOTAPPLICABLE 0
#define TIME_NOMINUTESORSECONDS 1
#define TIME_NOSECONDS 2
#define TIME_NOTIMEMARKER 4
#define TIME_FORCE24HOURFORMAT 8
#define MB_PRECOMPOSED 1
#define MB_COMPOSITE 2
#define MB_ERR_INVALID_CHARS 8
#define MB_USEGLYPHCHARS 4
#define WC_COMPOSITECHECK 512
#define WC_DISCARDNS 16
#define WC_SEPCHARS 32
#define WC_DEFAULTCHAR 64
#define CTRY_DEFAULT 0
#define CTRY_ALBANIA 355
#define CTRY_ALGERIA 213
#define CTRY_ARGENTINA 54
#define CTRY_ARMENIA 374
#define CTRY_AUSTRALIA 61
#define CTRY_AUSTRIA 43
#define CTRY_AZERBAIJAN 994
#define CTRY_BAHRAIN 973
#define CTRY_BELARUS 375
#define CTRY_BELGIUM 32
#define CTRY_BELIZE 501
#define CTRY_BOLIVIA 591
#define CTRY_BRAZIL 55
#define CTRY_BRUNEI_DARUSSALAM 673
#define CTRY_BULGARIA 359
#define CTRY_CANADA 2
#define CTRY_CARIBBEAN 1
#define CTRY_CHILE 56
#define CTRY_COLOMBIA 57
#define CTRY_COSTA_RICA 506
#define CTRY_CROATIA 385
#define CTRY_CZECH 420
#define CTRY_DENMARK 45
#define CTRY_DOMINICAN_REPUBLIC 1
#define CTRY_ECUADOR 593
#define CTRY_EGYPT 20
#define CTRY_EL_SALVADOR 503
#define CTRY_ESTONIA 372
#define CTRY_FAEROE_ISLANDS 298
#define CTRY_FINLAND 358
#define CTRY_FRANCE 33
#define CTRY_GEORGIA 995
#define CTRY_GERMANY 49
#define CTRY_GREECE 30
#define CTRY_GUATEMALA 502
#define CTRY_HONDURAS 504
#define CTRY_HONG_KONG 852
#define CTRY_HUNGARY 36
#define CTRY_ICELAND 354
#define CTRY_INDIA 91
#define CTRY_INDONESIA 62
#define CTRY_IRAN 981
#define CTRY_IRAQ 964
#define CTRY_IRELAND 353
#define CTRY_ISRAEL 972
#define CTRY_ITALY 39
#define CTRY_JAMAICA 1
#define CTRY_JAPAN 81
#define CTRY_JORDAN 962
#define CTRY_KAZAKSTAN 7
#define CTRY_KENYA 254
#define CTRY_KUWAIT 965
#define CTRY_KYRGYZSTAN 996
#define CTRY_LATVIA 371
#define CTRY_LEBANON 961
#define CTRY_LIBYA 218
#define CTRY_LIECHTENSTEIN 41
#define CTRY_LITHUANIA 370
#define CTRY_LUXEMBOURG 352
#define CTRY_MACAU 853
#define CTRY_MACEDONIA 389
#define CTRY_MALAYSIA 60
#define CTRY_MALDIVES 960
#define CTRY_MEXICO 52
#define CTRY_MONACO 33
#define CTRY_MONGOLIA 976
#define CTRY_MOROCCO 212
#define CTRY_NETHERLANDS 31
#define CTRY_NEW_ZEALAND 64
#define CTRY_NICARAGUA 505
#define CTRY_NORWAY 47
#define CTRY_OMAN 968
#define CTRY_PAKISTAN 92
#define CTRY_PANAMA 507
#define CTRY_PARAGUAY 595
#define CTRY_PERU 51
#define CTRY_PHILIPPINES 63
#define CTRY_POLAND 48
#define CTRY_PORTUGAL 351
#define CTRY_PRCHINA 86
#define CTRY_PUERTO_RICO 1
#define CTRY_QATAR 974
#define CTRY_ROMANIA 40
#define CTRY_RUSSIA 7
#define CTRY_SAUDI_ARABIA 966
#define CTRY_SERBIA 381
#define CTRY_SINGAPORE 65
#define CTRY_SLOVAK 421
#define CTRY_SLOVENIA 386
#define CTRY_SOUTH_AFRICA 27
#define CTRY_SOUTH_KOREA 82
#define CTRY_SPAIN 34
#define CTRY_SWEDEN 46
#define CTRY_SWITZERLAND 41
#define CTRY_SYRIA 963
#define CTRY_TAIWAN 886
#define CTRY_TATARSTAN 7
#define CTRY_THAILAND 66
#define CTRY_TRINIDAD_Y_TOBAGO 1
#define CTRY_TUNISIA 216
#define CTRY_TURKEY 90
#define CTRY_UAE 971
#define CTRY_UKRAINE 380
#define CTRY_UNITED_KINGDOM 44
#define CTRY_UNITED_STATES 1
#define CTRY_URUGUAY 598
#define CTRY_UZBEKISTAN 7
#define CTRY_VENEZUELA 58
#define CTRY_VIET_NAM 84
#define CTRY_YEMEN 967
#define CTRY_ZIMBABWE 263
#define CAL_ICALINTVALUE 1
#define CAL_SCALNAME 2
#define CAL_IYEAROFFSETRANGE 3
#define CAL_SERASTRING 4
#define CAL_SSHORTDATE 5
#define CAL_SLONGDATE 6
#define CAL_SDAYNAME1 7
#define CAL_SDAYNAME2 8
#define CAL_SDAYNAME3 9
#define CAL_SDAYNAME4 10
#define CAL_SDAYNAME5 11
#define CAL_SDAYNAME6 12
#define CAL_SDAYNAME7 13
#define CAL_SABBREVDAYNAME1 14
#define CAL_SABBREVDAYNAME2 15
#define CAL_SABBREVDAYNAME3 16
#define CAL_SABBREVDAYNAME4 17
#define CAL_SABBREVDAYNAME5 18
#define CAL_SABBREVDAYNAME6 19
#define CAL_SABBREVDAYNAME7 20
#define CAL_SMONTHNAME1 21
#define CAL_SMONTHNAME2 22
#define CAL_SMONTHNAME3 23
#define CAL_SMONTHNAME4 24
#define CAL_SMONTHNAME5 25
#define CAL_SMONTHNAME6 26
#define CAL_SMONTHNAME7 27
#define CAL_SMONTHNAME8 28
#define CAL_SMONTHNAME9 29
#define CAL_SMONTHNAME10 30
#define CAL_SMONTHNAME11 31
#define CAL_SMONTHNAME12 32
#define CAL_SMONTHNAME13 33
#define CAL_SABBREVMONTHNAME1 34
#define CAL_SABBREVMONTHNAME2 35
#define CAL_SABBREVMONTHNAME3 36
#define CAL_SABBREVMONTHNAME4 37
#define CAL_SABBREVMONTHNAME5 38
#define CAL_SABBREVMONTHNAME6 39
#define CAL_SABBREVMONTHNAME7 40
#define CAL_SABBREVMONTHNAME8 41
#define CAL_SABBREVMONTHNAME9 42
#define CAL_SABBREVMONTHNAME10 43
#define CAL_SABBREVMONTHNAME11 44
#define CAL_SABBREVMONTHNAME12 45
#define CAL_SABBREVMONTHNAME13 46
#define CAL_GREGORIAN 1
#define CAL_GREGORIAN_US 2
#define CAL_JAPAN 3
#define CAL_TAIWAN 4
#define CAL_KOREA 5
#define CAL_HIJRI 6
#define CAL_THAI 7
#define CAL_HEBREW 8
#define CAL_GREGORIAN_ME_FRENCH 9
#define CAL_GREGORIAN_ARABIC 10
#define CAL_GREGORIAN_XLIT_ENGLISH 11
#define CAL_GREGORIAN_XLIT_FRENCH 12
#define CSTR_LESS_THAN 1
#define CSTR_EQUAL 2
#define CSTR_GREATER_THAN 3
#define LGRPID_INSTALLED 1
#define LGRPID_SUPPORTED 2
#define LGRPID_WESTERN_EUROPE 1
#define LGRPID_CENTRAL_EUROPE 2
#define LGRPID_BALTIC 3
#define LGRPID_GREEK 4
#define LGRPID_CYRILLIC 5
#define LGRPID_TURKISH 6
#define LGRPID_JAPANESE 7
#define LGRPID_KOREAN 8
#define LGRPID_TRADITIONAL_CHINESE 9
#define LGRPID_SIMPLIFIED_CHINESE 10
#define LGRPID_THAI 11
#define LGRPID_HEBREW 12
#define LGRPID_ARABIC 13
#define LGRPID_VIETNAMESE 14
#define LGRPID_INDIC 15
#define LGRPID_GEORGIAN 16
#define LGRPID_ARMENIAN 17

#if (WINVER >= 0x0500)
#define LOCALE_SYEARMONTH 0x1006
#define LOCALE_SENGCURRNAME 0x1007
#define LOCALE_SNATIVECURRNAME 0x1008
#define LOCALE_IDEFAULTEBCDICCODEPAGE 0x1012
#define LOCALE_SSORTNAME 0x1013
#define LOCALE_IDIGITSUBSTITUTION 0x1014
#define LOCALE_IPAPERSIZE 0x100A
#define DATE_YEARMONTH 8
#define DATE_LTRREADING 16
#define DATE_RTLREADING 32
#define MAP_EXPAND_LIGATURES   0x2000
#define WC_NO_BEST_FIT_CHARS 1024
#define CAL_SYEARMONTH 47
#define CAL_ITWODIGITYEARMAX 48
#define CAL_NOUSEROVERRIDE LOCALE_NOUSEROVERRIDE
#define CAL_RETURN_NUMBER LOCALE_RETURN_NUMBER
#define CAL_USE_CP_ACP LOCALE_USE_CP_ACP
#endif /* (WINVER >= 0x0500) */
#ifndef  _BASETSD_H
typedef long LONG_PTR;
#endif
#endif

//All kernel32 function should use WINAPI calling convention.
//When doing emulation or interception, the calling convention should
//match exactly the target dlls suppose to use.   Monkeyhappy
extern "C" HANDLE WINAPI dllFindFirstFileA(LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData);
extern "C" BOOL WINAPI dllFindNextFileA(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData);
extern "C" BOOL WINAPI dllFindClose(HANDLE hFile);
extern "C" UINT WINAPI dllGetAtomNameA( ATOM nAtom, LPTSTR lpBuffer, int nSize);
extern "C" ATOM WINAPI dllFindAtomA( LPCTSTR lpString);
extern "C" ATOM WINAPI dllAddAtomA( LPCTSTR lpString);
extern "C" DWORD WINAPI dllGetCurrentProcessId(void);
extern "C" BOOL WINAPI dllDisableThreadLibraryCalls(HMODULE);

//dllLoadLibraryA, dllFreeLibrary, dllGetProcAddress are from dllLoader,
//they are wrapper functions of COFF/PE32 loader.
extern "C" HMODULE WINAPI dllLoadLibraryA(LPCSTR libname);
extern "C" HMODULE WINAPI dllLoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
extern "C" BOOL WINAPI dllFreeLibrary(HINSTANCE hLibModule);
extern "C" FARPROC WINAPI dllGetProcAddress(HMODULE hModule, LPCSTR function);
extern "C" HMODULE WINAPI dllGetModuleHandleA(LPCSTR lpModuleName);
extern "C" DWORD WINAPI dllGetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize);

//Current just a dummy function
extern "C" UINT WINAPI dllGetPrivateProfileIntA(LPCSTR lpAppName, LPCSTR lpKeyName,
      INT nDefault, LPCSTR lpFileName);

extern "C" BOOL WINAPI dllGetVersionExA(LPOSVERSIONINFO lpVersionInfo);
extern "C" BOOL WINAPI dllGetVersionExW(LPOSVERSIONINFOW lpVersionInfo);
extern "C" DWORD WINAPI dllGetVersion();
extern "C" UINT WINAPI dllGetProfileIntA(LPCTSTR lpAppName, LPCTSTR lpKeyName, INT nDefault);

//vp6vfw.dll
extern "C" BOOL WINAPI dllFreeEnvironmentStringsW(LPWSTR lpString);
extern "C" HMODULE WINAPI dllGetOEMCP();
extern "C" HMODULE WINAPI dllRtlUnwind(PVOID TargetFrame OPTIONAL, PVOID TargetIp OPTIONAL, PEXCEPTION_RECORD ExceptionRecord OPTIONAL, PVOID ReturnValue);
extern "C" LPTSTR WINAPI dllGetCommandLineA();
extern "C" HMODULE WINAPI dllExitProcess(UINT uExitCode);
extern "C" HMODULE WINAPI dllTerminateProcess(HANDLE hProcess, UINT uExitCode);
extern "C" HANDLE WINAPI dllGetCurrentProcess();
extern "C" UINT WINAPI dllGetACP();
extern "C" UINT WINAPI dllSetHandleCount(UINT uNumber);
extern "C" HANDLE WINAPI dllGetStdHandle(DWORD nStdHandle);
extern "C" DWORD WINAPI dllGetFileType(HANDLE hFile);
extern "C" int WINAPI dllGetStartupInfoA(LPSTARTUPINFOA lpStartupInfo);
extern "C" BOOL WINAPI dllFreeEnvironmentStringsA(LPSTR lpString);
extern "C" LPVOID WINAPI dllGetEnvironmentStrings();
extern "C" LPVOID WINAPI dllGetEnvironmentStringsW();
extern "C" int WINAPI dllGetEnvironmentVariableA(LPCSTR lpName, LPSTR lpBuffer, DWORD nSize);
extern "C" HMODULE WINAPI dllLCMapStringA(LCID Locale, DWORD dwMapFlags, LPCSTR lpSrcStr, int cchSrc, LPSTR lpDestStr, int cchDest);
extern "C" HMODULE WINAPI dllLCMapStringW(LCID Locale, DWORD dwMapFlags, LPCWSTR lpSrcStr, int cchSrc, LPWSTR lpDestStr, int cchDest);
extern "C" HMODULE WINAPI dllSetStdHandle(DWORD nStdHandle, HANDLE hHandle);
extern "C" HMODULE WINAPI dllGetStringTypeA(LCID Locale, DWORD dwInfoType, LPCSTR lpSrcStr, int cchSrc, LPWORD lpCharType);
extern "C" HMODULE WINAPI dllGetStringTypeW(DWORD dwInfoType, LPCWSTR lpSrcStr, int cchSrc, LPWORD lpCharType);
extern "C" HMODULE WINAPI dllGetCPInfo(UINT CodePage, LPCPINFO lpCPInfo);

extern "C" LCID WINAPI dllGetThreadLocale(void);
extern "C" BOOL WINAPI dllSetPriorityClass(HANDLE hProcess, DWORD dwPriorityClass);
extern "C" DWORD WINAPI dllFormatMessageA(DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId, DWORD dwLanguageId, LPTSTR lpBuffer, DWORD nSize, va_list* Arguments);
extern "C" DWORD WINAPI dllGetFullPathNameA(LPCTSTR lpFileName, DWORD nBufferLength, LPTSTR lpBuffer, LPTSTR* lpFilePart);
extern "C" DWORD WINAPI dllGetFullPathNameW(LPCWSTR lpFileName, DWORD nBufferLength, LPWSTR lpBuffer, LPWSTR* lpFilePart);
extern "C" DWORD WINAPI dllExpandEnvironmentStringsA(LPCTSTR lpSrc, LPTSTR lpDst, DWORD nSize);
extern "C" UINT WINAPI dllGetWindowsDirectoryA(LPTSTR lpBuffer, UINT uSize);
extern "C" UINT WINAPI dllGetSystemDirectoryA(LPTSTR lpBuffer, UINT uSize);

//
extern "C" HANDLE WINAPI dllHeapCreate(DWORD flOptions, SIZE_T dwInitialSize, SIZE_T dwMaximumSize);
extern "C" LPVOID WINAPI dllHeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
extern "C" LPVOID WINAPI dllHeapReAlloc(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, SIZE_T dwBytes);
extern "C" BOOL WINAPI dllHeapFree(HANDLE heap, DWORD flags, LPVOID mem);
extern "C" HANDLE WINAPI dllGetProcessHeap(void);

extern "C" UINT WINAPI dllGetShortPathName(LPTSTR lpszLongPath, LPTSTR lpszShortPath, UINT cchBuffer);
extern "C" UINT WINAPI dllSetErrorMode(UINT i);
extern "C" BOOL WINAPI dllIsProcessorFeaturePresent(DWORD ProcessorFeature);

extern "C" BOOL WINAPI dllFileTimeToLocalFileTime(CONST FILETIME *lpFileTime, LPFILETIME lpLocalFileTime);
extern "C" BOOL WINAPI dllFileTimeToSystemTime(CONST FILETIME *lpFileTime, LPSYSTEMTIME lpSystemTime);
extern "C" DWORD WINAPI dllGetTimeZoneInformation(LPTIME_ZONE_INFORMATION lpTimeZoneInformation);

extern "C" DWORD WINAPI dllGetFileAttributesA(LPCSTR lpFileName);

extern "C" UINT WINAPI dllGetCurrentDirectoryA(UINT c, LPSTR s);
extern "C" UINT WINAPI dllSetCurrentDirectoryA(const char *pathname);

extern "C" int WINAPI dllDuplicateHandle(HANDLE hSourceProcessHandle, HANDLE hSourceHandle, HANDLE hTargetProcessHandle, HANDLE* lpTargetHandle, DWORD dwDesiredAccess, int bInheritHandle, DWORD dwOptions);

extern "C" int WINAPI dllSetUnhandledExceptionFilter(void* filter);

extern "C" int WINAPI dllSetEnvironmentVariableA(const char *name, const char *value);
extern "C" int WINAPI dllCreateDirectoryA(const char *pathname, void *sa);

extern "C" BOOL WINAPI dllGetProcessAffinityMask(HANDLE hProcess, LPDWORD lpProcessAffinityMask, LPDWORD lpSystemAffinityMask);

extern "C" HGLOBAL WINAPI dllLoadResource(HMODULE module, HRSRC res);
extern "C" HRSRC WINAPI dllFindResourceA(HMODULE module, LPCTSTR name, LPCTSTR type);
extern "C" int WINAPI dllGetLocaleInfoA(LCID Locale, LCTYPE LCType, LPTSTR lpLCData, int cchData);
extern "C" UINT WINAPI dllGetConsoleCP();
extern "C" UINT WINAPI dllGetConsoleOutputCP();
extern "C" UINT WINAPI dllSetConsoleCtrlHandler(PHANDLER_ROUTINE HandlerRoutine, BOOL Add);

extern "C" void WINAPI dllSleep(DWORD dwTime);
extern "C" PVOID WINAPI dllEncodePointer(PVOID ptr);
extern "C" PVOID WINAPI dllDecodePointer(PVOID ptr);

extern "C" int WINAPI dllMultiByteToWideChar(UINT CodePage, DWORD dwFlags, LPCSTR lpMultiByteStr, int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar);
extern "C" int WINAPI dllWideCharToMultiByte(UINT CodePage, DWORD dwFlags, LPCWSTR lpWideCharStr, int cchWideChar, LPSTR lpMultiByteStr, int cbMultiByte, LPCSTR lpDefaultChar, LPBOOL lpUsedDefaultChar);

extern "C" BOOL WINAPI dllLockFile(HANDLE hFile, DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh, DWORD nNumberOffBytesToLockLow, DWORD nNumberOffBytesToLockHigh);
extern "C" BOOL WINAPI dllLockFileEx(HANDLE hFile, DWORD dwFlags, DWORD dwReserved, DWORD nNumberOffBytesToLockLow, DWORD nNumberOffBytesToLockHigh, LPOVERLAPPED lpOverlapped);
extern "C" BOOL WINAPI dllUnlockFile(HANDLE hFile, DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh, DWORD nNumberOfBytesToUnlockLow, DWORD nNumberOfBytesToUnlockHigh);

extern "C" HANDLE WINAPI dllCreateFileA(
    IN LPCSTR lpFileName,
    IN DWORD dwDesiredAccess,
    IN DWORD dwShareMode,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    IN DWORD dwCreationDisposition,
    IN DWORD dwFlagsAndAttributes,
    IN HANDLE hTemplateFile
    );

extern "C" DWORD WINAPI dllGetTempPathA(DWORD nBufferLength, LPTSTR lpBuffer);



extern "C" BOOL WINAPI dllDVDReadFileLayerChangeHack(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);

extern "C" LPVOID WINAPI dllLockResource(HGLOBAL hResData);
extern "C" SIZE_T WINAPI dllGlobalSize(HGLOBAL hMem);
extern "C" DWORD  WINAPI dllSizeofResource(HMODULE hModule, HRSRC hResInfo);

#endif // _EMU_KERNEL32_H_
