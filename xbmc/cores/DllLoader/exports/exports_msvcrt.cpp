#if defined(_XBOX) || defined(_LINUX)
#include "stdafx.h"
#endif
#ifdef _LINUX
#include "../DllLoaderContainer.h"
#include "../DllLoader.h"
#include "emu_msvcrt.h"

#else

#include "../DllLoader.h"
#ifdef _XBOX
#include <string.h>
#endif
#endif

#ifndef __GNUC__
#pragma warning (disable:4391)
#pragma warning (disable:4392)
#endif

#ifndef _LINUX
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
extern "C" void* dll_beginthread();
extern "C" void* clock();
extern "C" void* _hypot();
extern "C" void* asctime();
// extern "C" void* __security_error_handler();
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
#endif

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
  { "_close",                     -1, (void*)dll_close,                     (void*)track_close},
  { "_lseek",                     -1, (void*)dll_lseek,                     NULL },
  { "_read",                      -1, (void*)dll_read,                      NULL },
  { "_write",                     -1, (void*)dll_write,                     NULL },
  { "__dllonexit",                -1, (void*)dll__dllonexit,                NULL },
#ifndef _LINUX
  { "__mb_cur_max",               -1, (void*)__mb_cur_max,                  NULL },
  { "_assert",                    -1, (void*)_assert,                       NULL },
  { "_errno",                     -1, (void*)_errno,                        NULL },
  { "_ftime",                     -1, (void*)_ftime,                        NULL },
  { "_iob",                       -1, (void*)_iob,                          NULL },
  { "_isctype",                   -1, (void*)_isctype,                      NULL },
#else
  { "_errno",                     -1, (void*)dll_errno,                     NULL },
#endif
  { "_lseeki64",                  -1, (void*)dll_lseeki64,                  NULL },
  { "_open",                      -1, (void*)dll_open,                      (void*)track_open },
#ifdef _LINUX
  { "_snprintf",                  -1, (void*)snprintf,                     NULL },
  { "_stricmp",                   -1, (void*)stricmp,                      NULL },
  { "_strnicmp",                  -1, (void*)strnicmp,                     NULL },
  { "_vsnprintf",                 -1, (void*)vsnprintf,                    NULL },
#else
  { "_snprintf",                  -1, (void*)_snprintf,                     NULL },
  { "_stricmp",                   -1, (void*)_stricmp,                      NULL },
  { "_strnicmp",                  -1, (void*)_strnicmp,                     NULL },
  { "_vsnprintf",                 -1, (void*)_vsnprintf,                    NULL },

#endif
  { "abort",                      -1, (void*)dllabort,                      NULL },
  { "atof",                       -1, (void*)atof,                          NULL },
  { "atoi",                       -1, (void*)atoi,                          NULL },
  { "cos",                        -1, (void*)cos,                           NULL },
  { "cosh",                       -1, (void*)cosh,                          NULL },
  { "exp",                        -1, (void*)exp,                           NULL },
  { "fflush",                     -1, (void*)dll_fflush,                    NULL },
  { "floor",                      -1, (void*)floor,                         NULL },
  { "fprintf",                    -1, (void*)dll_fprintf,                   NULL },
  { "free",                       -1, (void*)dllfree,                       (void*)track_free},
  { "frexp",                      -1, (void*)frexp,                         NULL },
  { "fwrite",                     -1, (void*)dll_fwrite,                    NULL },
  { "gmtime",                     -1, (void*)gmtime,                        NULL },
  { "ldexp",                      -1, (void*)ldexp,                         NULL },
  { "localtime",                  -1, (void*)localtime,                     NULL },
  { "log",                        -1, (void*)log,                           NULL },
  { "log10",                      -1, (void*)log10,                         NULL },
  { "malloc",                     -1, (void*)dllmalloc,                     (void*)track_malloc},
  { "memcpy",                     -1, (void*)memcpy,                        NULL },
  { "memmove",                    -1, (void*)memmove,                       NULL },
  { "memset",                     -1, (void*)memset,                        NULL },
  { "mktime",                     -1, (void*)mktime,                        NULL },
  { "perror",                     -1, (void*)dllperror,                     NULL },
  { "printf",                     -1, (void*)dllprintf,                     NULL },
  { "putchar",                    -1, (void*)dll_putchar,                   NULL },
  { "puts",                       -1, (void*)dllputs,                       NULL },
  { "qsort",                      -1, (void*)qsort,                         NULL },
  { "realloc",                    -1, (void*)dllrealloc,                    (void*)track_realloc},
  { "sin",                        -1, (void*)sin,                           NULL },
  { "sinh",                       -1, (void*)sinh,                          NULL },
  { "sprintf",                    -1, (void*)sprintf,                       NULL },
  { "sqrt",                       -1, (void*)sqrt,                          NULL },
  { "sscanf",                     -1, (void*)sscanf,                        NULL },
  { "strchr",                     -1, (void*)::strchr,                      NULL },
  { "strcmp",                     -1, (void*)strcmp,                        NULL },
  { "strcpy",                     -1, (void*)strcpy,                        NULL },
  { "strlen",                     -1, (void*)strlen,                        NULL },
  { "strncpy",                    -1, (void*)strncpy,                       NULL },
  { "strrchr",                    -1, (void*)::strrchr,                     NULL },
  { "strtod",                     -1, (void*)strtod,                        NULL },
  { "strtok",                     -1, (void*)strtok,                        NULL },
  { "strtol",                     -1, (void*)strtol,                        NULL },
  { "strtoul",                    -1, (void*)strtoul,                       NULL },
  { "tan",                        -1, (void*)tan,                           NULL },
  { "tanh",                       -1, (void*)tanh,                          NULL },
  { "time",                       -1, (void*)time,                          NULL },
  { "toupper",                    -1, (void*)::toupper,                     NULL },
#ifndef _LINUX
  { "_memccpy",                   -1, (void*)_memccpy,                      NULL },
#endif
  { "_fstat",                     -1, (void*)dll_fstat,                     NULL },
  { "_mkdir",                     -1, (void*)dll_mkdir,                     NULL },
  { "_pclose",                    -1, (void*)dll_pclose,                    NULL },
  { "_popen",                     -1, (void*)dll_popen,                     NULL },
  { "_sleep",                     -1, (void*)dll_sleep,                     NULL },
  { "_stat",                      -1, (void*)dll_stat,                      NULL },
  { "_strdup",                    -1, (void*)dll_strdup,                    (void*)track_strdup},
#ifndef _LINUX
  { "_swab",                      -1, (void*)_swab,                         NULL },
  { "_findclose",                 -1, (void*)dll_findclose,                 NULL },
  { "_findfirst",                 -1, (void*)dll_findfirst,                 NULL },
  { "_findnext",                  -1, (void*)dll_findnext,                  NULL },
  { "_pctype",                    -1, (void*)_pctype,                       NULL },
#else
  { "_swab",                      -1, (void*)swab,                          NULL },
#endif
  { "_fullpath",                  -1, (void*)_fullpath,                     NULL },
  { "calloc",                     -1, (void*)dllcalloc,                     (void*)track_calloc},
  { "ceil",                       -1, (void*)ceil,                          NULL },
  { "ctime",                      -1, (void*)ctime,                         NULL },
  { "exit",                       -1, (void*)dllexit,                       NULL },
  { "fclose",                     -1, (void*)dll_fclose,                    (void*)track_fclose},
  { "feof",                       -1, (void*)dll_feof,                      NULL },
  { "fgets",                      -1, (void*)dll_fgets,                     NULL },
  { "fopen",                      -1, (void*)dll_fopen,                     (void*)track_fopen},
  { "putc",                       -1, (void*)dll_putc,                      NULL },
  { "fputc",                      -1, (void*)dll_fputc,                     NULL },
  { "fputs",                      -1, (void*)dll_fputs,                     NULL },
  { "fread",                      -1, (void*)dll_fread,                     NULL },
  { "fseek",                      -1, (void*)dll_fseek,                     NULL },
  { "ftell",                      -1, (void*)dll_ftell,                     NULL },
  { "getc",                       -1, (void*)dll_getc,                      NULL },
  { "getenv",                     -1, (void*)dll_getenv,                    NULL },
  { "rand",                       -1, (void*)rand,                          NULL },
  { "remove",                     -1, (void*)::remove,                      NULL },
  { "rewind",                     -1, (void*)dll_rewind,                    NULL },
  { "setlocale",                  -1, (void*)setlocale,                     NULL },
  { "signal",                     -1, (void*)dll_signal,                    NULL },
  { "srand",                      -1, (void*)srand,                         NULL },
  { "strcat",                     -1, (void*)strcat,                        NULL },
  { "strcoll",                    -1, (void*)strcoll,                       NULL },
  { "strerror",                   -1, (void*)dllstrerror,                   NULL },
  { "strncat",                    -1, (void*)strncat,                       NULL },
  { "strncmp",                    -1, (void*)strncmp,                       NULL },
  { "strpbrk",                    -1, (void*)::strpbrk,                     NULL },
  { "strstr",                     -1, (void*)::strstr,                      NULL },
  { "tolower",                    -1, (void*)::tolower,                     NULL },
  { "acos",                       -1, (void*)acos,                          NULL },
  { "atan",                       -1, (void*)atan,                          NULL },
  { "memchr",                     -1, (void*)::memchr,                      NULL },
  { "fgetc",                      -1, (void*)dll_getc,                      NULL },
#ifndef _LINUX
  { "_CIpow",                     -1, (void*)_CIpow,                        NULL },
  { "_purecall",                  -1, (void*)_purecall,                     NULL },
  { "_adjust_fdiv",               -1, (void*)_adjust_fdiv,                  NULL },
#else
  { "_CIpow",                     -1, (void*)pow,                           NULL },
#endif
  { "_initterm",                  -1, (void*)dll_initterm,                  NULL },
  { "swscanf",                    -1, (void*)swscanf,                       NULL },
  { "??2@YAPAXI@Z",               -1, (void*)dllmalloc,                     (void*)track_malloc},
  { "??3@YAXPAX@Z",               -1, (void*)dllfree,                       (void*)track_free},
  { "iswspace",                   -1, (void*)iswspace,                      NULL },
  { "wcscmp",                     -1, (void*)wcscmp,                        NULL },
  { "vfprintf",                   -1, (void*)dll_vfprintf,                  NULL },
  { "vsprintf",                   -1, (void*)vsprintf,                      NULL },
#ifndef _LINUX
  { "longjmp",                    -1, (void*)longjmp,                       NULL },
  { "_ftol",                      -1, (void*)_ftol,                         NULL },
#endif
  { "strspn",                     -1, (void*)strspn,                        NULL },
  { "strcspn",                    -1, (void*)strcspn,                       NULL },
  { "fgetpos",                    -1, (void*)dll_fgetpos,                   NULL },
  { "fsetpos",                    -1, (void*)dll_fsetpos,                   NULL },
  { "_stati64",                   -1, (void*)dll_stati64,                   NULL },
  { "_fstati64",                  -1, (void*)dll_fstati64,                  NULL },
  { "_telli64",                   -1, (void*)dll_telli64,                   NULL },
  { "_tell",                      -1, (void*)dll_tell,                      NULL },
  { "_setmode",                   -1, (void*)dll_setmode,                   NULL },
  { "_beginthreadex",             -1, (void*)dll_beginthreadex,             NULL },
  { "_fileno",                    -1, (void*)dll_fileno,                    NULL },
  { "_getcwd",                    -1, (void*)dll_getcwd,                    NULL },
  { "_putenv",                    -1, (void*)dll_putenv,                    NULL },
  { "_ctype",                     -1, (void*)dll_ctype,                     NULL },
#ifndef _LINUX
  { "_atoi64",                    -1, (void*)_atoi64,                       NULL },
  { "_isatty",                    -1, (void*)_isatty,                       NULL },
  { "_filbuf",                    -1, (void*)_filbuf,                       NULL },
  { "_fmode",                     -1, (void*)_fmode,                        NULL },
  { "_setjmp",                    -1, (void*)_setjmp,                       NULL },
#endif
  { "asin",                       -1, (void*)asin,                          NULL },
  { "atol",                       -1, (void*)atol,                          NULL },
  { "bsearch",                    -1, (void*)bsearch,                       NULL },
  { "ferror",                     -1, (void*)dll_ferror,                    NULL },
  { "freopen",                    -1, (void*)dll_freopen,                   (void*)track_freopen},
  { "fscanf",                     -1, (void*)fscanf,                        NULL },
  { "localeconv",                 -1, (void*)localeconv,                    NULL },
#ifndef _LINUX
  { "raise",                      -1, (void*)raise,                         NULL },
#endif
  { "setvbuf",                    -1, (void*)setvbuf,                       NULL },
  { "strftime",                   -1, (void*)strftime,                      NULL },
  { "strxfrm",                    -1, (void*)strxfrm,                       NULL },
  { "ungetc",                     -1, (void*)dll_ungetc,                    NULL },
  { "_fdopen",                    -1, (void*)dll_fdopen,                    NULL },
  { "system",                     -1, (void*)dll_system,                    NULL },
#ifndef _LINUX
  { "_flsbuf",                    -1, (void*)_flsbuf,                       NULL },
#endif
  { "isdigit",                    -1, (void*)::isdigit,                     NULL },
  { "isalnum",                    -1, (void*)::isalnum,                     NULL },
  { "isxdigit",                   -1, (void*)::isxdigit,                    NULL },
  { "pow",                        -1, (void*)pow,                           NULL },
  { "_onexit",                    -1, (void*)dll_onexit,                    NULL },
  { "modf",                       -1, (void*)modf,                          NULL },
  { "memcmp",                     -1, (void*)memcmp,                        NULL },
#ifndef _LINUX
  { "_get_osfhandle",             -1, (void*)_get_osfhandle,                NULL },
  { "_itoa",                      -1, (void*)_itoa,                         NULL },
  { "_except_handler3",           -1, (void*)_except_handler3,              NULL },
  { "_CxxThrowException",         -1, (void*)_CxxThrowException,            NULL },
  { "__CxxFrameHandler",          -1, (void*)__CxxFrameHandler,             NULL },
  { "__CxxLongjmpUnwind",         -1, (void*)__CxxLongjmpUnwind,            NULL },
  { "_tempnam",                   -1, (void*)_tempnam,                      NULL },
#else
  { "_itoa",                      -1, (void*)itoa,                          NULL },
#endif  
  { "clearerr",                   -1, (void*)dll_clearerr,                  NULL },
  { "_sys_nerr",                  -1, (void*)&_sys_nerr,                    NULL },
  { NULL,                         -1, (void*)NULL,                          NULL }
};

Export export_msvcr71[] =
{
  { "_close",                     -1, (void*)dll_close,                     (void*)track_close },
  { "_lseek",                     -1, (void*)dll_lseek,                     NULL },
  { "_read",                      -1, (void*)dll_read,                      NULL },
  { "_write",                     -1, (void*)dll_write,                     NULL },
  { "__dllonexit",                -1, (void*)dll__dllonexit,                NULL },
#ifndef _LINUX
  { "__mb_cur_max",               -1, (void*)__mb_cur_max,                  NULL },
  { "_assert",                    -1, (void*)_assert,                       NULL },
  { "_errno",                     -1, (void*)_errno,                        NULL },
  { "_ftime",                     -1, (void*)_ftime,                        NULL },
  { "_iob",                       -1, (void*)_iob,                          NULL },
  { "_isctype",                   -1, (void*)_isctype,                      NULL },
#endif
  { "_lseeki64",                  -1, (void*)dll_lseeki64,                  NULL },
  { "_open",                      -1, (void*)dll_open,                      (void*)track_open },
#ifndef _LINUX
  { "_snprintf",                  -1, (void*)_snprintf,                     NULL },
  { "_stricmp",                   -1, (void*)_stricmp,                      NULL },
  { "_strnicmp",                  -1, (void*)_strnicmp,                     NULL },
  { "_vsnprintf",                 -1, (void*)_vsnprintf,                    NULL },
#else
  { "_snprintf",                  -1, (void*)snprintf,                      NULL },
  { "_stricmp",                   -1, (void*)stricmp,                       NULL },
  { "_strnicmp",                  -1, (void*)strnicmp,                      NULL },
  { "_vsnprintf",                 -1, (void*)vsnprintf,                     NULL },
#endif
  { "abort",                      -1, (void*)abort,                         NULL },
  { "atof",                       -1, (void*)atof,                          NULL },
  { "atoi",                       -1, (void*)atoi,                          NULL },
  { "cos",                        -1, (void*)cos,                           NULL },
  { "cosh",                       -1, (void*)cosh,                          NULL },
  { "exp",                        -1, (void*)exp,                           NULL },
  { "fflush",                     -1, (void*)dll_fflush,                    NULL },
  { "floor",                      -1, (void*)floor,                         NULL },
  { "fprintf",                    -1, (void*)dll_fprintf,                   NULL },
  { "free",                       -1, (void*)free,                          (void*)track_free },
  { "frexp",                      -1, (void*)frexp,                         NULL },
  { "fwrite",                     -1, (void*)dll_fwrite,                    NULL },
  { "gmtime",                     -1, (void*)gmtime,                        NULL },
  { "ldexp",                      -1, (void*)ldexp,                         NULL },
  { "localtime",                  -1, (void*)localtime,                     NULL },
  { "log",                        -1, (void*)log,                           NULL },
  { "log10",                      -1, (void*)log10,                         NULL },
  { "malloc",                     -1, (void*)malloc,                        (void*)track_malloc },
  { "memcpy",                     -1, (void*)memcpy,                        NULL },
  { "memmove",                    -1, (void*)memmove,                       NULL },
  { "memset",                     -1, (void*)memset,                        NULL },
  { "mktime",                     -1, (void*)mktime,                        NULL },
  { "perror",                     -1, (void*)dllperror,                     NULL },
  { "printf",                     -1, (void*)dllprintf,                     NULL },
  { "putchar",                    -1, (void*)dll_putchar,                   NULL },
  { "puts",                       -1, (void*)dllputs,                       NULL },
  { "qsort",                      -1, (void*)qsort,                         NULL },
  { "realloc",                    -1, (void*)dllrealloc,                    (void*)track_realloc },
  { "sin",                        -1, (void*)sin,                           NULL },
  { "sinh",                       -1, (void*)sinh,                          NULL },
  { "sprintf",                    -1, (void*)sprintf,                       NULL },
  { "sqrt",                       -1, (void*)sqrt,                          NULL },
  { "sscanf",                     -1, (void*)sscanf,                        NULL },
  { "strchr",                     -1, (void*)::strchr,                      NULL },
  { "strcmp",                     -1, (void*)strcmp,                        NULL },
  { "strcpy",                     -1, (void*)strcpy,                        NULL },
  { "strlen",                     -1, (void*)strlen,                        NULL },
  { "strncpy",                    -1, (void*)strncpy,                       NULL },
  { "strrchr",                    -1, (void*)::strrchr,                     NULL },
  { "strtod",                     -1, (void*)strtod,                        NULL },
  { "strtok",                     -1, (void*)strtok,                        NULL },
  { "strtol",                     -1, (void*)strtol,                        NULL },
  { "strtoul",                    -1, (void*)strtoul,                       NULL },
  { "tan",                        -1, (void*)tan,                           NULL },
  { "tanh",                       -1, (void*)tanh,                          NULL },
  { "time",                       -1, (void*)time,                          NULL },
  { "toupper",                    -1, (void*)::toupper,                       NULL },
  { "_fstat",                     -1, (void*)dll_fstat,                     NULL },
  { "_mkdir",                     -1, (void*)dll_mkdir,                     NULL },
  { "_pclose",                    -1, (void*)dll_pclose,                    NULL },
  { "_popen",                     -1, (void*)dll_popen,                     NULL },
  { "_sleep",                     -1, (void*)dll_sleep,                     NULL },
  { "_stat",                      -1, (void*)dll_stat,                      NULL },
#ifndef _LINUX
  { "_memccpy",                   -1, (void*)_memccpy,                      NULL },
  { "_strdup",                    -1, (void*)_strdup,                       (void*)track_strdup },
  { "_swab",                      -1, (void*)_swab,                         NULL },
  { "_findclose",                 -1, (void*)dll_findclose,                 NULL },
  { "_findfirst",                 -1, (void*)dll_findfirst,                 NULL },
  { "_findnext",                  -1, (void*)dll_findnext,                  NULL },
  { "_pctype",                    -1, (void*)_pctype,                       NULL },
#else
  { "_strdup",                    -1, (void*)strdup,                        (void*)track_strdup },
  { "_swab",                      -1, (void*)swab,                          NULL },
#endif
  { "_fullpath",                  -1, (void*)_fullpath,                     NULL },
  { "calloc",                     -1, (void*)dllcalloc,                     (void*)track_calloc },
  { "ceil",                       -1, (void*)ceil,                          NULL },
  { "ctime",                      -1, (void*)ctime,                         NULL },
  { "exit",                       -1, (void*)exit,                          NULL },
  { "fclose",                     -1, (void*)dll_fclose,                    (void*)track_fclose },
  { "feof",                       -1, (void*)dll_feof,                      NULL },
  { "fgets",                      -1, (void*)dll_fgets,                     NULL },
  { "fopen",                      -1, (void*)dll_fopen,                     (void*)track_fopen },
  { "fgetc",                      -1, (void*)dll_getc,                      NULL },
  { "putc",                       -1, (void*)dll_putc,                      NULL },
  { "fputc",                      -1, (void*)dll_fputc,                     NULL },
  { "fputs",                      -1, (void*)dll_fputs,                     NULL },
  { "fread",                      -1, (void*)dll_fread,                     NULL },
  { "fseek",                      -1, (void*)dll_fseek,                     NULL },
  { "ftell",                      -1, (void*)dll_ftell,                     NULL },
  { "getc",                       -1, (void*)dll_getc,                      NULL },
  { "getenv",                     -1, (void*)dll_getenv,                    NULL },
  { "rand",                       -1, (void*)rand,                          NULL },
  { "remove",                     -1, (void*)::remove,                      NULL },
  { "rewind",                     -1, (void*)dll_rewind,                    NULL },
  { "setlocale",                  -1, (void*)setlocale,                     NULL },
  { "signal",                     -1, (void*)dll_signal,                    NULL },
  { "srand",                      -1, (void*)srand,                         NULL },
  { "strcat",                     -1, (void*)strcat,                        NULL },
  { "strcoll",                    -1, (void*)strcoll,                       NULL },
  { "strerror",                   -1, (void*)strerror,                      NULL },
  { "strncat",                    -1, (void*)strncat,                       NULL },
  { "strncmp",                    -1, (void*)strncmp,                       NULL },
  { "strpbrk",                    -1, (void*)::strpbrk,                     NULL },
  { "strstr",                     -1, (void*)::strstr,                        NULL },
  { "tolower",                    -1, (void*)::tolower,                     NULL },
  { "acos",                       -1, (void*)acos,                          NULL },
  { "atan",                       -1, (void*)atan,                          NULL },
  { "memchr",                     -1, (void*)::memchr,                      NULL },
  { "isdigit",                    -1, (void*)::isdigit,                     NULL },
  { "_strcmpi",                   -1, (void*)strcmpi,                       NULL },
#ifndef _LINUX
  { "_CIpow",                     -1, (void*)_CIpow,                        NULL },
  { "_adjust_fdiv",               -1, (void*)_adjust_fdiv,                  NULL },
#endif
  { "pow",                        -1, (void*)pow,                           NULL },
  { "fabs",                       -1, (void*)fabs,                          NULL },
  { "??2@YAPAXI@Z",               -1, (void*)dllmalloc,                     (void*)track_malloc },
  { "??3@YAXPAX@Z",               -1, (void*)dllfree,                       (void*)track_free },
#ifndef _LINUX
  { "??_U@YAPAXI@Z",              -1, (void*)(::operator new),              NULL },
#endif
  { "_beginthreadex",             -1, (void*)dll_beginthreadex,             NULL },
  { "_fdopen",                    -1, (void*)dll_fdopen,                    NULL },
  { "_fileno",                    -1, (void*)dll_fileno,                    NULL },
  { "_getcwd",                    -1, (void*)dll_getcwd,                    NULL },
  { "_putenv",                    -1, (void*)dll_putenv,                    NULL },
  { "_ctype",                     -1, (void*)dll_ctype,                     NULL },
#ifndef _LINUX
  { "_isatty",                    -1, (void*)_isatty,                       NULL },
  { "_atoi64",                    -1, (void*)_atoi64,                       NULL },
  { "_filbuf",                    -1, (void*)_filbuf,                       NULL },
  { "_fmode",                     -1, (void*)_fmode,                        NULL },
  { "_setjmp",                    -1, (void*)_setjmp,                       NULL },
#endif
  { "asin",                       -1, (void*)asin,                          NULL },
  { "atol",                       -1, (void*)atol,                          NULL },
  { "bsearch",                    -1, (void*)bsearch,                       NULL },
  { "ferror",                     -1, (void*)dll_ferror,                    NULL },
  { "freopen",                    -1, (void*)dll_freopen,                   (void*)track_freopen},
  { "fscanf",                     -1, (void*)fscanf,                        NULL },
  { "localeconv",                 -1, (void*)localeconv,                    NULL },
  { "setvbuf",                    -1, (void*)setvbuf,                       NULL },
  { "strftime",                   -1, (void*)strftime,                      NULL },
  { "strxfrm",                    -1, (void*)strxfrm,                       NULL },
  { "ungetc",                     -1, (void*)dll_ungetc,                    NULL },
  { "system",                     -1, (void*)dll_system,                    NULL },
  { "strspn",                     -1, (void*)strspn,                        NULL },
  { "strcspn",                    -1, (void*)strcspn,                       NULL },
#ifndef _LINUX
  { "raise",                      -1, (void*)raise,                         NULL },
  { "_flsbuf",                    -1, (void*)_flsbuf,                       NULL },
  { "wcslen",                     -1, (void*)wcslen,                        NULL },
  { "_wcsicmp",                   -1, (void*)_wcsicmp,                      NULL },
  { "_wcsnicmp",                  -1, (void*)_wcsnicmp,                     NULL },
#endif
  { "fgetpos",                    -1, (void*)dll_fgetpos,                   NULL },
  { "??_V@YAXPAX@Z",              -1, (void*)dllfree,                       (void*)track_free},
  { "isalpha",                    -1, (void*)::isalpha,                       NULL },
#ifndef _LINUX
  { "_CIacos",                    -1, (void*)_CIacos,                       NULL },
  { "_CIasin",                    -1, (void*)_CIasin,                       NULL },
  { "_CxxThrowException",         -1, (void*)_CxxThrowException,            NULL },
  { "__CxxFrameHandler",          -1, (void*)__CxxFrameHandler,             NULL },
  { "__CxxLongjmpUnwind",         -1, (void*)__CxxLongjmpUnwind,            NULL },
  { "_setjmp3",                   -1, (void*)_setjmp3,                      NULL },
  { "longjmp",                    -1, (void*)longjmp,                       NULL },
#endif
  { "memcmp",                     -1, (void*)memcmp,                        NULL },
  { "fsetpos",                    -1, (void*)dll_fsetpos,                   NULL },
  { "isprint",                    -1, (void*)::isprint,                     NULL },
  { "vsprintf",                   -1, (void*)vsprintf,                      NULL },
  { "abs",                        -1, (void*)::abs,                         NULL },
  { "labs",                       -1, (void*)::labs,                        NULL },
  { "islower",                    -1, (void*)::islower,                     NULL },
  { "isupper",                    -1, (void*)::isupper,                     NULL },
  { "wcscoll",                    -1, (void*)wcscoll,                       NULL },
  { "modf",                       -1, (void*)modf,                          NULL },
#ifndef _LINUX
  { "_CIsinh",                    -1, (void*)_CIsinh,                       NULL },
  { "_CIcosh",                    -1, (void*)_CIcosh,                       NULL },
  { "_isnan",                     -1, (void*)_isnan,                        NULL },
  { "_finite",                    -1, (void*)_finite,                       NULL },
  { "_CIfmod",                    -1, (void*)_CIfmod,                       NULL },
#else
  { "_CIsinh",                    -1, (void*)sinh,                       NULL },
  { "_CIcosh",                    -1, (void*)cosh,                       NULL },
  { "_isnan",                     -1, (void*)isnan,                        NULL },
  { "_finite",                    -1, (void*)finite,                       NULL },
  { "_CIfmod",                    -1, (void*)fmod,                       NULL },
#endif
  { "atan2",                      -1, (void*)atan2,                         NULL },
  { "fmod",                       -1, (void*)fmod,                          NULL },
  { "isxdigit",                   -1, (void*)::isxdigit,                      NULL },
  { "clock",                      -1, (void*)clock,                         NULL },
  { "asctime",                    -1, (void*)asctime,                       NULL },
  { "_beginthread",               -1, (void*)dll_beginthread,               NULL },
#ifndef _LINUX
  { "_endthread",                 -1, (void*)_endthread,                    NULL },
  { "_hypot",                     -1, (void*)_hypot,                        NULL },
  { "_except_handler3",           -1, (void*)_except_handler3,              NULL },
  //{ "__security_error_handler",   -1, (void*)__security_error_handler,      NULL },
  { "__CppXcptFilter",            -1, (void*)__CppXcptFilter,               NULL },
  { "_tzset",                     -1, (void*)_tzset,                        NULL },
  { "_tzname",                    -1, (void*)&_tzname,                      NULL },
#else
  { "_hypot",                     -1, (void*)hypot,                        NULL },
  { "_tzset",                     -1, (void*)tzset,                        NULL },
  { "_tzname",                    -1, (void*)&tzname,                      NULL },
#endif
  { "_sys_nerr",                  -1, (void*)&_sys_nerr,                    NULL },
  { "_sys_errlist",               -1, (void*)&_sys_errlist,                 NULL },
  { "_getpid",                    -1, (void*)dll_getpid,                    NULL },
  { "_exit",                      -1, (void*)dllexit,                       NULL },
  { "_onexit",                    -1, (void*)dll_onexit,                    NULL },
  { "_initterm",                  -1, (void*)dll_initterm,                  NULL },
#ifndef _LINUX
  { "_daylight",                  -1, (void*)&_daylight,                    NULL },
  { "_timezone",                  -1, (void*)&_timezone,                    NULL },
  { "_HUGE",                      -1, (void*)_HUGE,                         NULL },
  { "_purecall",                  -1, (void*)_purecall,                     NULL },
#else
  { "_daylight",                  -1, (void*)&daylight,                    NULL },
  { "_timezone",                  -1, (void*)&timezone,                    NULL },
#endif
  { "isalnum",                    -1, (void*)::isalnum,                       NULL },
  { "isspace",                    -1, (void*)::isspace,                       NULL },
  { "_stati64",                   -1, (void*)dll_stati64,                   NULL },
  { "_fstati64",                  -1, (void*)dll_fstati64,                  NULL },
  { "clearerr",                   -1, (void*)dll_clearerr,                  NULL },
  { "_commit",                    -1, (void*)dll__commit,                   NULL },
  { "__p__environ",               -1, (void*)dll___p__environ,              NULL },
  { "vfprintf",                   -1, (void*)dll_vfprintf,                  NULL },
#ifndef _LINUX
  { "_strtoi64",                  -1, (void*)_strtoi64,                     NULL },
  { "_tempnam",                   -1, (void*)_tempnam,                      NULL },
  { "_aligned_malloc",            -1, (void*)_aligned_malloc,               NULL },
  { "_aligned_free",              -1, (void*)_aligned_free,                 NULL },
  { "_aligned_realloc",           -1, (void*)_aligned_realloc,              NULL },
  { "_callnewh",                  -1, (void*)_callnewh,                     NULL },
#endif
  { NULL,                         -1, (void*)NULL,                          NULL }
};

Export export_pncrt[] =
{
#ifndef _LINUX
  { "_purecall",                  -1, (void*)_purecall,                     NULL },
  { "_ftol",                      -1, (void*)_ftol,                         NULL },
  { "_CIpow",                     -1, (void*)_CIpow,                        NULL },
  { "_adjust_fdiv",               -1, (void*)&_adjust_fdiv,                 NULL },
  { "_iob",                       -1, (void*)&_iob,                         NULL },
  { "_assert",                    -1, (void*)_assert,                       NULL },
#endif
  { "malloc",                     -1, (void*)malloc,                        (void*)track_malloc },
  { "??3@YAXPAX@Z",               -1, (void*)free,                          (void*)track_free },
  { "memmove",                    -1, (void*)memmove,                       NULL },
  { "??2@YAPAXI@Z",               -1, (void*)malloc,                        (void*)track_malloc },
  { "free",                       -1, (void*)free,                          (void*)track_free },
  { "_initterm",                  -1, (void*)dll_initterm,                  NULL },
  { "_beginthreadex",             -1, (void*)dll_beginthreadex,             NULL },
  { "fprintf",                    -1, (void*)dll_fprintf,                   NULL },
  { "floor",                      -1, (void*)floor,                         NULL },
  { "__dllonexit",                -1, (void*)dll__dllonexit,                NULL },
  { "calloc",                     -1, (void*)dllcalloc,                     (void*)track_calloc },
  { "strncpy",                    -1, (void*)strncpy,                       NULL },
  { "ldexp",                      -1, (void*)ldexp,                         NULL },
  { "frexp",                      -1, (void*)frexp,                         NULL },
  { "rand",                       -1, (void*)rand,                          NULL },
  { NULL,                         -1, (void*)NULL,                          NULL }
};
