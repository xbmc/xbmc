/*
 *  Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All rights reserved.
 *  
 *  License to copy and use this software is granted provided that it is identified 
 *  as the "RSA Data Security, Inc. MD5 Message-Digest Algorithm" in all material 
 *  mentioning or referencing this software or this function.
 *
 *  License is also granted to make and use derivative works provided that such 
 *  works are identified as "derived from the RSA Data Security, Inc. MD5 Message-
 *  Digest Algorithm" in all material mentioning or referencing the derived work.
 *  
 *  RSA Data Security, Inc. makes no representations concerning either the 
 *  merchantability of this software or the suitability of this software for any 
 *  particular purpose. It is provided "as is" without express or implied warranty 
 *  of any kind. These notices must be retained in any copies of any part of this
 *  documentation and/or software.
 */

#ifndef MD5SUM_MD5_H
#define MD5SUM_MD5_H

//typedef unsigned int uint32_t;
//typedef unsigned char uint8_t;
//
#include <inttypes.h>

/*
 *  Define the MD5 context structure
 *  Please DO NOT change the order or contents of the structure as various assembler files depend on it !!
 */
 
typedef struct {
   uint32_t  state  [ 4];     /* state (ABCD) */
   uint32_t  count  [ 2];     /* number of bits, modulo 2^64 (least sig word first) */
   uint8_t   buffer [64];     /* input buffer for incomplete buffer data */
} MD5_CTX;

void   MD5Init   ( MD5_CTX* ctx );
void   MD5Update ( MD5_CTX* ctx, const uint8_t* buf, size_t len );
void   MD5Final  ( uint8_t digest [16], MD5_CTX* ctx );

class CMD5Helper
{
public:

    CMD5Helper(BOOL bInitialize = TRUE)
    {
        if (bInitialize)
            Initialize();
    }

    BOOL Initialize()
    {
        memset(&m_MD5Context, 0, sizeof(m_MD5Context));
        MD5Init(&m_MD5Context);
        m_nTotalBytes = 0;
        return TRUE;
    }

    inline void AddData(const void * pData, int nBytes)
    {
        MD5Update(&m_MD5Context, (const unsigned char *) pData, nBytes);
        m_nTotalBytes += nBytes;
    }

    BOOL GetResult(unsigned char cResult[16])
    {
        memset(cResult, 0, 16);
        MD5Final(cResult, &m_MD5Context);
        return TRUE;
    }

protected:

    MD5_CTX m_MD5Context;
    BOOL m_bStopped;
    int m_nTotalBytes;
};


#endif /* MD5SUM_MD5_H */
