/* Setup for Microsoft Visual C++ Version 5 */
#ifndef _sidconfig_h_
#define _sidconfig_h_

/* Define the sizeof of types in bytes */
#define SID_SIZEOF_CHAR      1
#define SID_SIZEOF_SHORT_INT 2
#define SID_SIZEOF_INT       4
#define SID_SIZEOF_LONG_INT  4

/* Define if your processor stores words with the most significant
   byte first (like Motorola and SPARC, unlike Intel and VAX).  */
#define SID_WORDS_LITTLEENDIAN

/* Define if your compiler supports type "bool".
   If not, a user-defined signed integral type will be used.  */
#define SID_HAVE_BOOL

/* Define if your compiler supports AC99 header "stdint.h" */
//#define SID_HAVE_STDINT_H

/* Define if your compiler supports AC99 header "stdbool.h" */
//#define SID_HAVE_STDBOOL_H

#ifdef _MSC_VER
    /* Microsoft specific compiler problem */
#   define SID_HAVE_BAD_COMPILER
#endif

/* DLL building support on win32 hosts */
#ifndef SID_EXTERN
#   ifdef DLL_EXPORT      /* defined by libtool (if required) */
#       define SID_EXTERN __declspec(dllexport)
#   endif
#   ifdef SID_DLL_IMPORT  /* define if linking with this dll */
#       define SID_EXTERN __declspec(dllimport)
#   endif
#   ifndef SID_EXTERN     /* static linking or !_WIN32 */
#       define SID_EXTERN
#   endif
#endif

/* Namespace support */
#define SIDPLAY2_NAMESPACE __sidplay2__
#ifdef  SIDPLAY2_NAMESPACE
#   define SIDPLAY2_NAMESPACE_START \
    namespace SIDPLAY2_NAMESPACE    \
    {
#   define SIDPLAY2_NAMESPACE_STOP  \
    }
#else
#   define SIDPLAY2_NAMESPACE_START
#   define SIDPLAY2_NAMESPACE_STOP
#endif

#endif // _sidconfig_h_
