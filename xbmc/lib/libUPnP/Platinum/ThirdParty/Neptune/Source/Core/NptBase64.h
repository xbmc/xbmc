/*****************************************************************
|
|   Neptune - Base64
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
****************************************************************/

#ifndef _NPT_BASE64_H_
#define _NPT_BASE64_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptDataBuffer.h"
#include "NptStrings.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const NPT_Cardinal NPT_BASE64_MIME_BLOCKS_PER_LINE = 19;
const NPT_Cardinal NPT_BASE64_PEM_BLOCKS_PER_LINE  = 16;

/*----------------------------------------------------------------------
|   NPT_Base64
+---------------------------------------------------------------------*/
class NPT_Base64 {
public:
    // class methods
    static NPT_Result Decode(const char*     base64, 
                             NPT_Size        size,
                             NPT_DataBuffer& data,
                             bool            url_safe = false);
    static NPT_Result Encode(const NPT_Byte* data, 
                             NPT_Size        size, 
                             NPT_String&     base64, 
                             NPT_Cardinal    max_blocks_per_line = 0, 
                             bool            url_safe = false);

private: 
    // this class is purely static
    NPT_Base64();
};

#endif // _NPT_BASE64_H_
