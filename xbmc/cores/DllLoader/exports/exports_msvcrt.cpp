
//#include "stdafx.h"

#include "../DllLoader.h"
//#include "emu_msvcrt.h"

#ifndef __GNUC__
#pragma warning (disable:4391)
#pragma warning (disable:4392)
#endif

extern "C" void* dll_close();
extern "C" void* dll_lseek();
extern "C" void* dll_read();
extern "C" void* dll_write();
extern "C" void* dll__dllonexit();
extern "C" void* __mb_cur_max();
extern "C" void* _assert();
extern "C" void* _errno();
extern "C" void* _ftime();
extern "C" void* _iob();
extern "C" void* _isctype();
extern "C" void* dll_lseeki64();
extern "C" void* dll_open();
extern "C" void* _snprintf();
extern "C" void* _stricmp();
extern "C" void* _strnicmp();
extern "C" void* _vsnprintf();
extern "C" void* dllabort();
extern "C" void* atof();
extern "C" void* atoi();
extern "C" void* cos();
extern "C" void* cosh();
extern "C" void* exp();
extern "C" void* dll_fflush();
extern "C" void* floor();
extern "C" void* dll_fprintf();
extern "C" void* dllfree();
extern "C" void* frexp();
extern "C" void* gmtime();
extern "C" void* ldexp();
extern "C" void* localtime();
extern "C" void* log();
extern "C" void* log10();
extern "C" void* dllmalloc();
extern "C" void* memcpy();
extern "C" void* memmove();
extern "C" void* memset();
extern "C" void* mktime();
extern "C" void* dllperror();
extern "C" void* dllprintf();
extern "C" void* dll_putchar();
extern "C" void* dllputs();
extern "C" void* qsort();
extern "C" void* dllrealloc();
extern "C" void* sin();
extern "C" void* sinh();
extern "C" void* sprintf();
extern "C" void* sqrt();
extern "C" void* sscanf();
extern "C" void* strchr();
extern "C" void* strcmp();
extern "C" void* strcpy();
extern "C" void* strlen();
extern "C" void* strncpy();
extern "C" void* strrchr();
extern "C" void* strtod();
extern "C" void* strtok();
extern "C" void* strtol();
extern "C" void* strtoul();
extern "C" void* tan();
extern "C" void* tanh();
extern "C" void* time();
extern "C" void* toupper();
extern "C" void* _memccpy();
extern "C" void* dll_fstat();
extern "C" void* dll_mkdir();
extern "C" void* dll_pclose();
extern "C" void* dll_popen();
extern "C" void* dll_sleep();
extern "C" void* dll_stat();
extern "C" void* dll_strdup();
extern "C" void* _swab();
extern "C" void* dll_findclose();
extern "C" void* dll_findfirst();
extern "C" void* dll_findnext();
extern "C" void* _fullpath();
extern "C" void* _pctype();
extern "C" void* dllcalloc();
extern "C" void* ceil();
extern "C" void* ctime();
extern "C" void* dllexit();
extern "C" void* dll_fclose();
extern "C" void* dll_feof();
extern "C" void* dll_fgets();
extern "C" void* dll_fopen();
extern "C" void* dll_fputc();
extern "C" void* dll_fputs();
extern "C" void* dll_fread();
extern "C" void* dll_fseek();
extern "C" void* dll_ftell();
extern "C" void* dll_getc();
extern "C" void* dll_getenv();
extern "C" void* dll_putc();
extern "C" void* rand();
extern "C" void* remove();
extern "C" void* dll_rewind();
extern "C" void* setlocale();
extern "C" void* dll_signal();
extern "C" void* srand();
extern "C" void* strcat();
extern "C" void* strcoll();
extern "C" void* dllstrerror();
extern "C" void* strncat();
extern "C" void* strncmp();
extern "C" void* strpbrk();
extern "C" void* strstr();
extern "C" void* tolower();
extern "C" void* acos();
extern "C" void* atan();
extern "C" void* memchr();
extern "C" void* dll_getc();
extern "C" void* _CIpow();
extern "C" int _purecall();
extern "C" void* _adjust_fdiv();
extern "C" void* dll_initterm();
extern "C" void* swscanf();
extern "C" void* dllfree();
extern "C" void* iswspace();
extern "C" void* wcscmp();
extern "C" void* dll_vfprintf();
extern "C" void* vsprintf();
extern "C" void* longjmp();
extern "C" void* _ftol();
extern "C" void* strspn();
extern "C" void* strcspn();
extern "C" void* dll_fgetpos();
extern "C" void* dll_fsetpos();
extern "C" void* dll_stati64();
extern "C" void* dll_fstati64();
extern "C" void* dll_telli64();
extern "C" void* dll_tell();
extern "C" void* dll_setmode();
extern "C" void* dll_beginthreadex();
extern "C" void* dll_fileno();
extern "C" void* dll_getcwd();
extern "C" void* _isatty();
extern "C" void* dll_putenv();
extern "C" void* _atoi64();
extern "C" void* dll_ctype();
extern "C" void* _filbuf();
extern "C" void* _fmode();
extern "C" int _setjmp(int);
extern "C" void* asin();
extern "C" void* atol();
extern "C" void* atol();
extern "C" void* bsearch();
extern "C" void* dll_ferror();
extern "C" void* dll_freopen();
extern "C" void* fscanf();
extern "C" void* localeconv();
extern "C" void* raise();
extern "C" void* setvbuf();
extern "C" void* strftime();
extern "C" void* strxfrm();
extern "C" void* dll_ungetc();
extern "C" void* dll_fdopen();
extern "C" void* dll_system();
extern "C" void* _flsbuf();
extern "C" void* isdigit();
extern "C" void* isalnum();
extern "C" void* isxdigit();
extern "C" void* pow();
extern "C" void* dll_onexit();
extern "C" void* modf();
extern "C" void* _get_osfhandle();
extern "C" void* _itoa();
extern "C" void* memcmp();
extern "C" void* _except_handler3();
extern "C" void* __CxxFrameHandler();
extern "C" void* __stdcall __CxxLongjmpUnwind(void*);
//extern "C" void* __stdcall _CxxThrowException(void*, void*);
extern "C" void* abort();
extern "C" void* free();
extern "C" void* malloc();
extern "C" void* _strdup();
extern "C" void* exit();
extern "C" void* strerror();
extern "C" void* strcmpi();
extern "C" void* fabs();
extern "C" void* dllmalloc71();
extern "C" void* dllfree71();
extern "C" void* wcslen();
extern "C" void* _wcsicmp();
extern "C" void* _wcsnicmp();
extern "C" void* _CIacos();
extern "C" void* _CIasin();
extern "C" void* dllfree71();
extern "C" void* isalpha();
extern "C" void* _setjmp3();
extern "C" void* isprint();
extern "C" void* abs();
extern "C" void* labs();
extern "C" void* islower();
extern "C" void* isupper();
extern "C" void* wcscoll();
extern "C" void* _CIsinh();
extern "C" void* _CIcosh();
extern "C" void* _isnan();
extern "C" void* _finite();
extern "C" void* _CIfmod();
extern "C" void* atan2();
extern "C" void* fmod();
extern "C" void* _endthread();
extern "C" void* _beginthread();
extern "C" void* clock();
extern "C" void* _hypot();
extern "C" void* asctime();
extern "C" void* __security_error_handler();
extern "C" void* __CppXcptFilter();
extern "C" void* _tzset();
extern "C" void* _tzname();
extern "C" void* _daylight();
extern "C" void* _timezone();
extern "C" void* _sys_nerr();
extern "C" void* _sys_errlist();
extern "C" void* dll_getpid();
extern "C" void* _HUGE();
extern "C" void* isspace();
extern "C" void* dll_fwrite();
extern "C" void* fsetpos();
extern "C" void* _strtoi64();
extern "C" void* dll_clearerr();
extern "C" void* dll__commit();
extern "C" void* dll___p__environ();
extern "C" void* _tempnam();
extern "C" void* _aligned_malloc();
extern "C" void* _aligned_free();
extern "C" void* _aligned_realloc();
extern "C" void* _callnewh();

// tracker functions
extern "C" void* track_close();
extern "C" void* track_open();
extern "C" void* track_free();
extern "C" void* track_malloc();
extern "C" void* track_realloc();
extern "C" void* track_strdup();
extern "C" void* track_calloc();
extern "C" void* track_fclose();
extern "C" void* track_fopen();
extern "C" void* track_freopen();

Export export_msvcrt[] =
{
  { "_close",                     -1, dll_close,                     track_close },
  { "_lseek",                     -1, dll_lseek,                     NULL },
  { "_read",                      -1, dll_read,                      NULL },
  { "_write",                     -1, dll_write,                     NULL },
  { "__dllonexit",                -1, dll__dllonexit,                NULL },
  { "__mb_cur_max",               -1, __mb_cur_max,                  NULL },
  { "_assert",                    -1, _assert,                       NULL },
  { "_errno",                     -1, _errno,                        NULL },
  { "_ftime",                     -1, _ftime,                        NULL },
  { "_iob",                       -1, _iob,                          NULL },
  { "_isctype",                   -1, _isctype,                      NULL },
  { "_lseeki64",                  -1, dll_lseeki64,                  NULL },
  { "_open",                      -1, dll_open,                      track_open },
  { "_snprintf",                  -1, _snprintf,                     NULL },
  { "_stricmp",                   -1, _stricmp,                      NULL },
  { "_strnicmp",                  -1, _strnicmp,                     NULL },
  { "_vsnprintf",                 -1, _vsnprintf,                    NULL },
  { "abort",                      -1, dllabort,                      NULL },
  { "atof",                       -1, atof,                          NULL },
  { "atoi",                       -1, atoi,                          NULL },
  { "_itoa",                      -1, _itoa,                         NULL },
  { "cos",                        -1, cos,                           NULL },
  { "cosh",                       -1, cosh,                          NULL },
  { "exp",                        -1, exp,                           NULL },
  { "fflush",                     -1, dll_fflush,                    NULL },
  { "floor",                      -1, floor,                         NULL },
  { "fprintf",                    -1, dll_fprintf,                   NULL },
  { "free",                       -1, free,                          track_free },
  { "frexp",                      -1, frexp,                         NULL },
  { "fwrite",                     -1, dll_fwrite,                    NULL },
  { "gmtime",                     -1, gmtime,                        NULL },
  { "ldexp",                      -1, ldexp,                         NULL },
  { "localtime",                  -1, localtime,                     NULL },
  { "log",                        -1, log,                           NULL },
  { "log10",                      -1, log10,                         NULL },
  { "malloc",                     -1, malloc,                        track_malloc },
  { "memcpy",                     -1, memcpy,                        NULL },
  { "memmove",                    -1, memmove,                       NULL },
  { "memset",                     -1, memset,                        NULL },
  { "mktime",                     -1, mktime,                        NULL },
  { "perror",                     -1, dllperror,                     NULL },
  { "printf",                     -1, dllprintf,                     NULL },
  { "putchar",                    -1, dll_putchar,                   NULL },
  { "puts",                       -1, dllputs,                       NULL },
  { "qsort",                      -1, qsort,                         NULL },
  { "realloc",                    -1, dllrealloc,                    track_realloc },
  { "sin",                        -1, sin,                           NULL },
  { "sinh",                       -1, sinh,                          NULL },
  { "sprintf",                    -1, sprintf,                       NULL },
  { "sqrt",                       -1, sqrt,                          NULL },
  { "sscanf",                     -1, sscanf,                        NULL },
  { "strchr",                     -1, strchr,                        NULL },
  { "strcmp",                     -1, strcmp,                        NULL },
  { "strcpy",                     -1, strcpy,                        NULL },
  { "strlen",                     -1, strlen,                        NULL },
  { "strncpy",                    -1, strncpy,                       NULL },
  { "strrchr",                    -1, strrchr,                       NULL },
  { "strtod",                     -1, strtod,                        NULL },
  { "strtok",                     -1, strtok,                        NULL },
  { "strtol",                     -1, strtol,                        NULL },
  { "strtoul",                    -1, strtoul,                       NULL },
  { "tan",                        -1, tan,                           NULL },
  { "tanh",                       -1, tanh,                          NULL },
  { "time",                       -1, time,                          NULL },
  { "toupper",                    -1, toupper,                       NULL },
  { "_memccpy",                   -1, _memccpy,                      NULL },
  { "_fstat",                     -1, dll_fstat,                     NULL },
  { "_memccpy",                   -1, _memccpy,                      NULL },
  { "_mkdir",                     -1, dll_mkdir,                     NULL },
  { "_pclose",                    -1, dll_pclose,                    NULL },
  { "_popen",                     -1, dll_popen,                     NULL },
  { "_sleep",                     -1, dll_sleep,                     NULL },
  { "_stat",                      -1, dll_stat,                      NULL },
  { "_strdup",                    -1, _strdup,                       track_strdup },
  { "_swab",                      -1, _swab,                         NULL },
  { "_findclose",                 -1, dll_findclose,                 NULL },
  { "_findfirst",                 -1, dll_findfirst,                 NULL },
  { "_findnext",                  -1, dll_findnext,                  NULL },
  { "_fullpath",                  -1, _fullpath,                     NULL },
  { "_pctype",                    -1, _pctype,                       NULL },
  { "calloc",                     -1, dllcalloc,                     track_calloc },
  { "ceil",                       -1, ceil,                          NULL },
  { "ctime",                      -1, ctime,                         NULL },
  { "exit",                       -1, dllexit,                       NULL },
  { "fclose",                     -1, dll_fclose,                    track_fclose },
  { "feof",                       -1, dll_feof,                      NULL },
  { "fgets",                      -1, dll_fgets,                     NULL },
  { "fopen",                      -1, dll_fopen,                     track_fopen },
  { "fgetc",                      -1, dll_getc,                      NULL },
  { "putc",                       -1, dll_putc,                      NULL },
  { "fputc",                      -1, dll_fputc,                     NULL },
  { "fputs",                      -1, dll_fputs,                     NULL },
  { "fread",                      -1, dll_fread,                     NULL },
  { "fseek",                      -1, dll_fseek,                     NULL },
  { "ftell",                      -1, dll_ftell,                     NULL },
  { "getc",                       -1, dll_getc,                      NULL },
  { "getenv",                     -1, dll_getenv,                    NULL },
  { "rand",                       -1, rand,                          NULL },
  { "remove",                     -1, remove,                        NULL },
  { "rewind",                     -1, dll_rewind,                    NULL },
  { "setlocale",                  -1, setlocale,                     NULL },
  { "signal",                     -1, dll_signal,                    NULL },
  { "srand",                      -1, srand,                         NULL },
  { "strcat",                     -1, strcat,                        NULL },
  { "strcoll",                    -1, strcoll,                       NULL },
  { "strerror",                   -1, strerror,                      NULL },
  { "strncat",                    -1, strncat,                       NULL },
  { "strncmp",                    -1, strncmp,                       NULL },
  { "strpbrk",                    -1, strpbrk,                       NULL },
  { "strstr",                     -1, strstr,                        NULL },
  { "tolower",                    -1, tolower,                       NULL },
  { "acos",                       -1, acos,                          NULL },
  { "atan",                       -1, atan,                          NULL },
  { "memchr",                     -1, memchr,                        NULL },
  { "isdigit",                    -1, isdigit,                       NULL },
  { "_strcmpi",                   -1, strcmpi,                       NULL },
  { "_CIpow",                     -1, _CIpow,                        NULL },
  { "_adjust_fdiv",               -1, _adjust_fdiv,                  NULL },
  { "pow",                        -1, pow,                           NULL },
  { "fabs",                       -1, fabs,                          NULL },
  { "swscanf",                    -1, swscanf,                       NULL },
  { "??2@YAPAXI@Z",               -1, dllmalloc,                     track_malloc },
  { "??3@YAXPAX@Z",               -1, dllfree,                       track_free },
  { "??_U@YAPAXI@Z",              -1, (void*)(operator new),         NULL },
  { "iswspace",                   -1, iswspace,                      NULL },
  { "wcscmp",                     -1, wcscmp,                        NULL },
  { "_ftol",                      -1, _ftol,                         NULL },
  { "_telli64",                   -1, dll_telli64,                   NULL },
  { "_tell",                      -1, dll_tell,                      NULL },
  { "_setmode",                   -1, dll_setmode,                   NULL },
  { "_beginthreadex",             -1, dll_beginthreadex,             NULL },
  { "_fdopen",                    -1, dll_fdopen,                    NULL },
  { "_fileno",                    -1, dll_fileno,                    NULL },
  { "_getcwd",                    -1, dll_getcwd,                    NULL },
  { "_isatty",                    -1, _isatty,                       NULL },
  { "_putenv",                    -1, dll_putenv,                    NULL },
  { "_atoi64",                    -1, _atoi64,                       NULL },
  { "_ctype",                     -1, dll_ctype,                     NULL },
  { "_filbuf",                    -1, _filbuf,                       NULL },
  { "_fmode",                     -1, _fmode,                        NULL },
  { "_setjmp",                    -1, _setjmp,                       NULL },
  { "asin",                       -1, asin,                          NULL },
  { "atol",                       -1, atol,                          NULL },
  { "atol",                       -1, atol,                          NULL },
  { "bsearch",                    -1, bsearch,                       NULL },
  { "ferror",                     -1, dll_ferror,                    NULL },
  { "freopen",                    -1, dll_freopen,                   track_freopen},
  { "fscanf",                     -1, fscanf,                        NULL },
  { "localeconv",                 -1, localeconv,                    NULL },
  { "raise",                      -1, raise,                         NULL },
  { "setvbuf",                    -1, setvbuf,                       NULL },
  { "strftime",                   -1, strftime,                      NULL },
  { "strxfrm",                    -1, strxfrm,                       NULL },
  { "ungetc",                     -1, dll_ungetc,                    NULL },
  { "system",                     -1, dll_system,                    NULL },
  { "_flsbuf",                    -1, _flsbuf,                       NULL },
  { "_get_osfhandle",             -1, _get_osfhandle,                NULL },
  { "strspn",                     -1, strspn,                        NULL },
  { "strcspn",                    -1, strcspn,                       NULL },
  { "wcslen",                     -1, wcslen,                        NULL },
  { "_wcsicmp",                   -1, _wcsicmp,                      NULL },
  { "fgetpos",                    -1, dll_fgetpos,                   NULL },
  { "_wcsnicmp",                  -1, _wcsnicmp,                     NULL },
  { "_CIacos",                    -1, _CIacos,                       NULL },
  { "_CIasin",                    -1, _CIasin,                       NULL },
  { "??_V@YAXPAX@Z",              -1, dllfree,                       track_free},
  { "isalpha",                    -1, isalpha,                       NULL },
  { "_CxxThrowException",         -1, _CxxThrowException,            NULL },
  { "__CxxFrameHandler",          -1, __CxxFrameHandler,             NULL },
  { "__CxxLongjmpUnwind",         -1, __CxxLongjmpUnwind,            NULL },
  { "memcmp",                     -1, memcmp,                        NULL },
  { "fsetpos",                    -1, dll_fsetpos,                   NULL },
  { "_setjmp3",                   -1, _setjmp3,                      NULL },
  { "longjmp",                    -1, longjmp,                       NULL },
  { "isprint",                    -1, isprint,                       NULL },
  { "vsprintf",                   -1, vsprintf,                      NULL },
  { "abs",                        -1, abs,                           NULL },
  { "labs",                       -1, labs,                          NULL },
  { "islower",                    -1, islower,                       NULL },
  { "isupper",                    -1, isupper,                       NULL },
  { "wcscoll",                    -1, wcscoll,                       NULL },
  { "_CIsinh",                    -1, _CIsinh,                       NULL },
  { "_CIcosh",                    -1, _CIcosh,                       NULL },
  { "modf",                       -1, modf,                          NULL },
  { "_isnan",                     -1, _isnan,                        NULL },
  { "_finite",                    -1, _finite,                       NULL },
  { "_CIfmod",                    -1, _CIfmod,                       NULL },
  { "atan2",                      -1, atan2,                         NULL },
  { "fmod",                       -1, fmod,                          NULL },
  { "isxdigit",                   -1, isxdigit,                      NULL },
  { "_endthread",                 -1, _endthread,                    NULL },
  { "_beginthread",               -1, _beginthread,                  NULL },
  { "clock",                      -1, clock,                         NULL },
  { "_hypot",                     -1, _hypot,                        NULL },
  { "_except_handler3",           -1, _except_handler3,              NULL },
  { "asctime",                    -1, asctime,                       NULL },
  { "__security_error_handler",   -1, __security_error_handler,      NULL },
  { "__CppXcptFilter",            -1, __CppXcptFilter,               NULL },
  { "_tzset",                     -1, _tzset,                        NULL },
  { "_tzname",                    -1, &_tzname,                      NULL },
  { "_daylight",                  -1, &_daylight,                    NULL },
  { "_timezone",                  -1, &_timezone,                    NULL },
  { "_sys_nerr",                  -1, &_sys_nerr,                    NULL },
  { "_sys_errlist",               -1, &_sys_errlist,                 NULL },
  { "_getpid",                    -1, dll_getpid,                    NULL },
  { "_exit",                      -1, dllexit,                       NULL },
  { "_onexit",                    -1, dll_onexit,                    NULL },
  { "_HUGE",                      -1, _HUGE,                         NULL },
  { "_initterm",                  -1, dll_initterm,                  NULL },
  { "_purecall",                  -1, _purecall,                     NULL },
  { "isalnum",                    -1, isalnum,                       NULL },
  { "isspace",                    -1, isspace,                       NULL },
  { "_stati64",                   -1, dll_stati64,                   NULL },
  { "_fstati64",                  -1, dll_fstati64,                  NULL },
  { "_strtoi64",                  -1, _strtoi64,                     NULL },
  { "clearerr",                   -1, dll_clearerr,                  NULL },
  { "_commit",                    -1, dll__commit,                   NULL },
  { "__p__environ",               -1, dll___p__environ,              NULL },
  { "vfprintf",                   -1, dll_vfprintf,                  NULL },
  { "_tempnam",                   -1, _tempnam,                      NULL },
  { "_aligned_malloc",            -1, _aligned_malloc,               NULL },
  { "_aligned_free",              -1, _aligned_free,                 NULL },
  { "_aligned_realloc",           -1, _aligned_realloc,              NULL },
  { "_callnewh",                  -1, _callnewh,                     NULL },
  { NULL,                         -1, NULL,                          NULL }
};

Export export_pncrt[] =
{
  { "malloc",                     -1, malloc,                        track_malloc },
  { "??3@YAXPAX@Z",               -1, free,                          track_free },
  { "memmove",                    -1, memmove,                       NULL },
  { "_purecall",                  -1, _purecall,                     NULL },
  { "_ftol",                      -1, _ftol,                         NULL },
  { "_CIpow",                     -1, _CIpow,                        NULL },
  { "??2@YAPAXI@Z",               -1, malloc,                        track_malloc },
  { "free",                       -1, free,                          track_free },
  { "_initterm",                  -1, dll_initterm,                  NULL },
  { "_adjust_fdiv",               -1, &_adjust_fdiv,                 NULL },
  { "_beginthreadex",             -1, dll_beginthreadex,             NULL },
  { "_iob",                       -1, &_iob,                         NULL },
  { "fprintf",                    -1, dll_fprintf,                   NULL },
  { "floor",                      -1, floor,                         NULL },
  { "_assert",                    -1, _assert,                       NULL },
  { "__dllonexit",                -1, dll__dllonexit,                NULL },
  { "calloc",                     -1, dllcalloc,                     track_calloc },
  { "strncpy",                    -1, strncpy,                       NULL },
  { "ldexp",                      -1, ldexp,                         NULL },
  { "frexp",                      -1, frexp,                         NULL },
  { "rand",                       -1, rand,                          NULL },
  { NULL,                         -1, NULL,                          NULL }
};
