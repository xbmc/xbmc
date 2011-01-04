/*****************************************************************
|
|   Neptune - TLS/SSL Support
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

#ifndef _NPT_TLS_H_
#define _NPT_TLS_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptConfig.h"
#include "NptStreams.h"

/*----------------------------------------------------------------------
|   error codes
+---------------------------------------------------------------------*/
const NPT_Result NPT_ERROR_INVALID_PASSWORD             = (NPT_ERROR_BASE_TLS-1);
const NPT_Result NPT_ERROR_TLS_INVALID_HANDSHAKE        = (NPT_ERROR_BASE_TLS-2);
const NPT_Result NPT_ERROR_TLS_INVALID_PROTOCOL_MESSAGE = (NPT_ERROR_BASE_TLS-3);
const NPT_Result NPT_ERROR_TLS_INVALID_HMAC             = (NPT_ERROR_BASE_TLS-4);
const NPT_Result NPT_ERROR_TLS_INVALID_VERSION          = (NPT_ERROR_BASE_TLS-5);
const NPT_Result NPT_ERROR_TLS_INVALID_SESSION          = (NPT_ERROR_BASE_TLS-6);
const NPT_Result NPT_ERROR_TLS_NO_CIPHER                = (NPT_ERROR_BASE_TLS-7);
const NPT_Result NPT_ERROR_TLS_BAD_CERTIFICATE          = (NPT_ERROR_BASE_TLS-8);
const NPT_Result NPT_ERROR_INVALID_KEY                  = (NPT_ERROR_BASE_TLS-9);

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const unsigned int NPT_TLS_NULL_WITH_NULL_NULL      = 0x00;
const unsigned int NPT_TLS_RSA_WITH_RC4_128_MD5     = 0x04;
const unsigned int NPT_TLS_RSA_WITH_RC4_128_SHA     = 0x05;
const unsigned int NPT_TLS_RSA_WITH_AES_128_CBC_SHA = 0x2F;
const unsigned int NPT_TLS_RSA_WITH_AES_256_CBC_SHA = 0x35;

/*----------------------------------------------------------------------
|   class references
+---------------------------------------------------------------------*/
class NPT_TlsContextImpl;
class NPT_TlsSessionImpl;

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef enum {
    NPT_TLS_KEY_FORMAT_RSA_PRIVATE,
    NPT_TLS_KEY_FORMAT_PKCS8,
    NPT_TLS_KEY_FORMAT_PKCS12
} NPT_TlsKeyFormat;

/*----------------------------------------------------------------------
|   NPT_TlsContext
+---------------------------------------------------------------------*/
class NPT_TlsContext
{
public:
    NPT_TlsContext();
   ~NPT_TlsContext();
   
   // methods
   NPT_Result LoadKey(NPT_TlsKeyFormat     key_format, 
                      const unsigned char* key_data,
                      NPT_Size             key_data_size,
                      const char*          password);
                   
      
protected:
    NPT_TlsContextImpl* m_Impl;
    
    // friends
    friend class NPT_TlsClientSession;
    friend class NPT_TlsServerSession;
};

typedef NPT_Reference<NPT_TlsContext> NPT_TlsContextReference;

/*----------------------------------------------------------------------
|   NPT_TlsCertificateInfo
+---------------------------------------------------------------------*/
struct NPT_TlsCertificateInfo
{
    struct {
        NPT_String common_name;
        NPT_String organization;
        NPT_String organizational_name;
    } subject;
    struct {
        NPT_String common_name;
        NPT_String organization;
        NPT_String organizational_name;
    } issuer;
    struct {
        unsigned char sha1[20];
        unsigned char md5[16];
    } fingerprint;
};

/*----------------------------------------------------------------------
|   NPT_TlsClientSession
+---------------------------------------------------------------------*/
class NPT_TlsClientSession
{
public:
    NPT_TlsClientSession(NPT_TlsContextReference&   context,
                         NPT_InputStreamReference&  input,
                         NPT_OutputStreamReference& output);
   ~NPT_TlsClientSession();
    NPT_Result Handshake();
    NPT_Result GetSessionId(NPT_DataBuffer& session_id);
    NPT_UInt32 GetCipherSuiteId();
    NPT_Result GetPeerCertificateInfo(NPT_TlsCertificateInfo& info);
    NPT_Result GetInputStream(NPT_InputStreamReference& stream);
    NPT_Result GetOutputStream(NPT_OutputStreamReference& stream);
    
protected:
    NPT_TlsContextReference           m_Context;
    NPT_Reference<NPT_TlsSessionImpl> m_Impl;
};

/*----------------------------------------------------------------------
|   NPT_TlsServerSession
+---------------------------------------------------------------------*/
class NPT_TlsServerSession
{
public:
    NPT_TlsServerSession(NPT_TlsContext&           context,
                         NPT_InputStreamReference  input,
                         NPT_OutputStreamReference output);

protected:
    NPT_TlsSessionImpl* m_Impl;
};

#endif // _NPT_TLS_H_
