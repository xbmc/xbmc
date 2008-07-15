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

/*----------------------------------------------------------------------
|   named types       
+---------------------------------------------------------------------*/
typedef int           NPT_Result;
typedef unsigned int  NPT_Cardinal;
typedef unsigned int  NPT_Ordinal;
typedef unsigned long NPT_Size;
typedef signed   long NPT_Offset;
typedef unsigned long NPT_Position;
typedef long          NPT_Timeout;
typedef void          NPT_Interface;
typedef unsigned char NPT_Byte;
typedef unsigned int  NPT_Flags;
typedef int           NPT_Integer;
typedef void*         NPT_Any;
typedef const void*   NPT_AnyConst;

/*----------------------------------------------------------------------
|   sized types
+---------------------------------------------------------------------*/
typedef unsigned int   NPT_UInt32;
typedef int            NPT_Int32;
typedef unsigned short NPT_UInt16;
typedef short          NPT_Int16;
typedef unsigned char  NPT_UInt8;
typedef char           NPT_Int8;
typedef float          NPT_Float;

#endif // _NPT_TYPES_H_
