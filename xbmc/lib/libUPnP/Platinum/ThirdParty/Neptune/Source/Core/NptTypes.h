/*****************************************************************
|
|   Neptune - Types
|
|      (c) 2001-2006 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (boK@bok.net)
|
 ****************************************************************/

#ifndef _NPT_TYPES_H_
#define _NPT_TYPES_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptConfig.h"

/*----------------------------------------------------------------------
|   sized types (this assumes that ints are 32 bits)
+---------------------------------------------------------------------*/
typedef NPT_CONFIG_INT64_TYPE          NPT_Int64;
typedef unsigned NPT_CONFIG_INT64_TYPE NPT_UInt64;
typedef unsigned int                   NPT_UInt32;
typedef int                            NPT_Int32;
typedef unsigned short                 NPT_UInt16;
typedef short                          NPT_Int16;
typedef unsigned char                  NPT_UInt8;
typedef char                           NPT_Int8;
typedef float                          NPT_Float;

/*----------------------------------------------------------------------
|   named types       
+---------------------------------------------------------------------*/
typedef int           NPT_Result;
typedef unsigned int  NPT_Cardinal;
typedef unsigned int  NPT_Ordinal;
typedef unsigned long NPT_Size;
typedef NPT_UInt64    NPT_LargeSize;
typedef signed   long NPT_Offset;
typedef NPT_UInt64    NPT_Position;
typedef long          NPT_Timeout;
typedef void          NPT_Interface;
typedef unsigned char NPT_Byte;
typedef unsigned int  NPT_Flags;
typedef void*         NPT_Any;
typedef const void*   NPT_AnyConst;


#endif // _NPT_TYPES_H_
