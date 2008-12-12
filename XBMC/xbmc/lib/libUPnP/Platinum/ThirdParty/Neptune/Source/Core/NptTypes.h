/*****************************************************************
|
|   Neptune - Types
|
| Copyright (c) 2002-2008, Axiomatic Systems, LLC.
| All rights reserved.
|
| Redistribution and use in source and binary forms, with or without
| modification, are permitted provided that the following conditions are met:
|     * Redistributions of source code must retain the above copyright
|       notice, this list of conditions and the following disclaimer.
|     * Redistributions in binary form must reproduce the above copyright
|       notice, this list of conditions and the following disclaimer in the
|       documentation and/or other materials provided with the distribution.
|     * Neither the name of Axiomatic Systems nor the
|       names of its contributors may be used to endorse or promote products
|       derived from this software without specific prior written permission.
|
| THIS SOFTWARE IS PROVIDED BY AXIOMATIC SYSTEMS ''AS IS'' AND ANY
| EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
| WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
| DISCLAIMED. IN NO EVENT SHALL AXIOMATIC SYSTEMS BE LIABLE FOR ANY
| DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
| (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
| LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
| ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
| (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
| SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
