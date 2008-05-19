/* BranchTypes.h */

#ifndef __BRANCHTYPES_H
#define __BRANCHTYPES_H

#ifndef _7ZIP_BYTE_DEFINED
#define _7ZIP_BYTE_DEFINED
typedef unsigned char Byte;
#endif 

#ifndef _7ZIP_UINT16_DEFINED
#define _7ZIP_UINT16_DEFINED
typedef unsigned short UInt16;
#endif 

#ifndef _7ZIP_UINT32_DEFINED
#define _7ZIP_UINT32_DEFINED
#ifdef _LZMA_UINT32_IS_ULONG
typedef unsigned long UInt32;
#else
typedef unsigned int UInt32;
#endif
#endif

#ifndef _7ZIP_UINT64_DEFINED
#define _7ZIP_UINT64_DEFINED
#ifdef _SZ_NO_INT_64
typedef unsigned long UInt64;
#else
#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef unsigned __int64 UInt64;
#else
typedef unsigned long long int UInt64;
#endif
#endif
#endif

/* #define _LZMA_NO_SYSTEM_SIZE_T */
/* You can use it, if you don't want <stddef.h> */

#ifndef _7ZIP_SIZET_DEFINED
#define _7ZIP_SIZET_DEFINED
#ifdef _LZMA_NO_SYSTEM_SIZE_T
typedef UInt32 SizeT;
#else
#include <stddef.h>
typedef size_t SizeT;
#endif
#endif

#endif
