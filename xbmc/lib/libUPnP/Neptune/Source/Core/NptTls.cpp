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

#if defined(NPT_CONFIG_ENABLE_TLS)

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptConfig.h"
#include "NptTls.h"
#include "NptLogging.h"
#include "NptUtils.h"
#include "NptSockets.h"

#include "ssl.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
//NPT_SET_LOCAL_LOGGER("neptune.tls")

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const unsigned int NPT_TLS_CONTEXT_DEFAULT_SESSION_CACHE = 16;

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef NPT_Reference<NPT_TlsSessionImpl> NPT_TlsSessionImplReference;

/*----------------------------------------------------------------------
|   NPT_Tls_MapResult
+---------------------------------------------------------------------*/
static NPT_Result
NPT_Tls_MapResult(int err)
{
    switch (err) {
        case SSL_ERROR_CONN_LOST:         return NPT_ERROR_CONNECTION_ABORTED;
        case SSL_ERROR_TIMEOUT:           return NPT_ERROR_TIMEOUT;
        case SSL_ERROR_EOS:               return NPT_ERROR_EOS;
        case SSL_ERROR_NOT_SUPPORTED:     return NPT_ERROR_NOT_SUPPORTED;
        case SSL_ERROR_INVALID_HANDSHAKE: return NPT_ERROR_TLS_INVALID_HANDSHAKE;
        case SSL_ERROR_INVALID_PROT_MSG:  return NPT_ERROR_TLS_INVALID_PROTOCOL_MESSAGE;
        case SSL_ERROR_INVALID_HMAC:      return NPT_ERROR_TLS_INVALID_HMAC;
        case SSL_ERROR_INVALID_VERSION:   return NPT_ERROR_TLS_INVALID_VERSION;
        case SSL_ERROR_INVALID_SESSION:   return NPT_ERROR_TLS_INVALID_SESSION;
        case SSL_ERROR_NO_CIPHER:         return NPT_ERROR_TLS_NO_CIPHER;
        case SSL_ERROR_BAD_CERTIFICATE:   return NPT_ERROR_TLS_BAD_CERTIFICATE;
        case SSL_ERROR_INVALID_KEY:       return NPT_ERROR_INVALID_KEY;
        case 0:                           return NPT_SUCCESS;
        default:                          return NPT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   NPT_TlsContextImpl
+---------------------------------------------------------------------*/
class NPT_TlsContextImpl {
public:
    NPT_TlsContextImpl() :
        m_SSL_CTX(ssl_ctx_new(SSL_SERVER_VERIFY_LATER, NPT_TLS_CONTEXT_DEFAULT_SESSION_CACHE)) {};
    ~NPT_TlsContextImpl() { ssl_ctx_free(m_SSL_CTX); }
    
   NPT_Result LoadKey(NPT_TlsKeyFormat     key_format, 
                      const unsigned char* key_data,
                      NPT_Size             key_data_size,
                      const char*          password);
    
    SSL_CTX* m_SSL_CTX;
};

/*----------------------------------------------------------------------
|   NPT_TlsContextImpl::LoadKey
+---------------------------------------------------------------------*/
NPT_Result 
NPT_TlsContextImpl::LoadKey(NPT_TlsKeyFormat     key_format, 
                            const unsigned char* key_data,
                            NPT_Size             key_data_size,
                            const char*          password)
{
    int object_type;
    switch (key_format) {
        case NPT_TLS_KEY_FORMAT_RSA_PRIVATE: object_type = SSL_OBJ_RSA_KEY; break;
        case NPT_TLS_KEY_FORMAT_PKCS8:       object_type = SSL_OBJ_PKCS8;   break;
        case NPT_TLS_KEY_FORMAT_PKCS12:      object_type = SSL_OBJ_PKCS12;  break;
        default: return NPT_ERROR_INVALID_PARAMETERS;
    }
    
    int result = ssl_obj_memory_load(m_SSL_CTX, object_type, key_data, key_data_size, password);
    return NPT_Tls_MapResult(result);
}

/*----------------------------------------------------------------------
|   NPT_TlsStreamAdapter
+---------------------------------------------------------------------*/
struct NPT_TlsStreamAdapter
{
    static int Read(SSL_SOCKET* _self, void* buffer, unsigned int size) {
        NPT_TlsStreamAdapter* self = (NPT_TlsStreamAdapter*)_self;
        NPT_Size bytes_read = 0;
        NPT_Result result = self->m_Input->Read(buffer, size, &bytes_read);
        if (NPT_FAILED(result)) {
            switch (result) {
                case NPT_ERROR_EOS:     return SSL_ERROR_EOS;
                case NPT_ERROR_TIMEOUT: return SSL_ERROR_TIMEOUT;
                default:                return SSL_ERROR_CONN_LOST;
            }
        }
        return bytes_read;
    }

    static int Write(SSL_SOCKET* _self, const void* buffer, unsigned int size) {
        NPT_TlsStreamAdapter* self = (NPT_TlsStreamAdapter*)_self;
        NPT_Size bytes_written = 0;
        NPT_Result result = self->m_Output->Write(buffer, size, &bytes_written);
        if (NPT_FAILED(result)) {
            switch (result) {
                case NPT_ERROR_EOS:     return SSL_ERROR_EOS;
                case NPT_ERROR_TIMEOUT: return SSL_ERROR_TIMEOUT;
                default:                return SSL_ERROR_CONN_LOST;
            }
        }
        return bytes_written;
    }
    
    NPT_TlsStreamAdapter(NPT_InputStreamReference  input, 
                         NPT_OutputStreamReference output) :
        m_Input(input), m_Output(output) {
        m_Base.Read  = Read;
        m_Base.Write = Write;
    }
    
    SSL_SOCKET                m_Base;
    NPT_InputStreamReference  m_Input;
    NPT_OutputStreamReference m_Output;
};


/*----------------------------------------------------------------------
|   NPT_TlsSessionImpl
+---------------------------------------------------------------------*/
class NPT_TlsSessionImpl {
public:
    NPT_TlsSessionImpl(SSL_CTX*                   context,
                       NPT_InputStreamReference&  input,
                       NPT_OutputStreamReference& output) :
        m_SSL_CTX(context),
        m_SSL(NULL),
        m_StreamAdapter(input, output) {}
    ~NPT_TlsSessionImpl() { ssl_free(m_SSL); }
    
    // methods
    NPT_Result Handshake();
    NPT_Result GetSessionId(NPT_DataBuffer& session_id);
    NPT_UInt32 GetCipherSuiteId();
    NPT_Result GetPeerCertificateInfo(NPT_TlsCertificateInfo& cert_info);
    
    // members
    SSL_CTX*             m_SSL_CTX;
    SSL*                 m_SSL;
    NPT_TlsStreamAdapter m_StreamAdapter;
};

/*----------------------------------------------------------------------
|   NPT_TlsSessionImpl::Handshake
+---------------------------------------------------------------------*/
NPT_Result
NPT_TlsSessionImpl::Handshake()
{
    if (m_SSL == NULL) {
        // we have not performed the handshake yet 
        m_SSL = ssl_client_new(m_SSL_CTX, &m_StreamAdapter.m_Base, NULL, 0);
    }
    
    int result = ssl_handshake_status(m_SSL);
    return NPT_Tls_MapResult(result);
}

/*----------------------------------------------------------------------
|   NPT_TlsSessionImpl::GetSessionId
+---------------------------------------------------------------------*/
NPT_Result
NPT_TlsSessionImpl::GetSessionId(NPT_DataBuffer& session_id)
{
    if (m_SSL == NULL) {
        // no handshake done
        session_id.SetDataSize(0);
        return NPT_ERROR_INVALID_STATE;
    }
    
    // return the session id
    session_id.SetData(ssl_get_session_id(m_SSL),
                       ssl_get_session_id_size(m_SSL));
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_TlsSessionImpl::GetCipherSuiteId
+---------------------------------------------------------------------*/
NPT_UInt32
NPT_TlsSessionImpl::GetCipherSuiteId()
{
    if (m_SSL == NULL) {
        // no handshake done
        return 0;
    }
    
    return ssl_get_cipher_id(m_SSL);
}

/*----------------------------------------------------------------------
|   NPT_TlsSessionImpl::GetPeerCertificateInfo
+---------------------------------------------------------------------*/
NPT_Result
NPT_TlsSessionImpl::GetPeerCertificateInfo(NPT_TlsCertificateInfo& cert_info)
{
    cert_info.subject.common_name         = ssl_get_cert_dn(m_SSL, SSL_X509_CERT_COMMON_NAME);
    cert_info.subject.organization        = ssl_get_cert_dn(m_SSL, SSL_X509_CERT_ORGANIZATION);
    cert_info.subject.organizational_name = ssl_get_cert_dn(m_SSL, SSL_X509_CERT_ORGANIZATIONAL_NAME);
    cert_info.issuer.common_name          = ssl_get_cert_dn(m_SSL, SSL_X509_CA_CERT_COMMON_NAME);
    cert_info.issuer.organization         = ssl_get_cert_dn(m_SSL, SSL_X509_CA_CERT_ORGANIZATION);
    cert_info.issuer.organizational_name  = ssl_get_cert_dn(m_SSL, SSL_X509_CA_CERT_ORGANIZATIONAL_NAME);
    
    ssl_get_cert_fingerprints(m_SSL, cert_info.fingerprint.md5, cert_info.fingerprint.sha1);
    
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_TlsInputStream
+---------------------------------------------------------------------*/
class NPT_TlsInputStream : public NPT_InputStream {
public:
    NPT_TlsInputStream(NPT_TlsSessionImplReference& session) :
        m_Session(session),
        m_Position(0),
        m_RecordCacheData(NULL),
        m_RecordCacheSize(0) {}
    
    // NPT_InputStream methods
    virtual NPT_Result Read(void*     buffer, 
                            NPT_Size  bytes_to_read, 
                            NPT_Size* bytes_read = NULL);
    virtual NPT_Result Seek(NPT_Position)           { return NPT_ERROR_NOT_SUPPORTED; }
    virtual NPT_Result Tell(NPT_Position& offset)   { offset = m_Position; return NPT_SUCCESS; }
    virtual NPT_Result GetSize(NPT_LargeSize& size) { size=0; return NPT_ERROR_NOT_SUPPORTED; }
    virtual NPT_Result GetAvailable(NPT_LargeSize& available);

private:
    NPT_TlsSessionImplReference m_Session;
    NPT_Position                m_Position;
    uint8_t*                    m_RecordCacheData;
    NPT_Size                    m_RecordCacheSize;
};

/*----------------------------------------------------------------------
|   NPT_TlsInputStream::Read
+---------------------------------------------------------------------*/
NPT_Result 
NPT_TlsInputStream::Read(void*     buffer, 
                         NPT_Size  bytes_to_read, 
                         NPT_Size* bytes_read)
{
    // setup default values
    if (bytes_read) *bytes_read = 0;
    
    // quick check for edge case
    if (bytes_to_read == 0) return NPT_SUCCESS;
    
    // read a new record if we don't have one cached
    if (m_RecordCacheData == NULL) {
        int ssl_result;
        do {
            ssl_result = ssl_read(m_Session->m_SSL, &m_RecordCacheData);
        } while (ssl_result == 0);
        if (ssl_result < 0) {
            return NPT_Tls_MapResult(ssl_result);
        } 
        m_RecordCacheSize = ssl_result;
    }
    
    // we now have data in cache
    if (bytes_to_read > m_RecordCacheSize) {
        // read at most what's in the cache
        bytes_to_read = m_RecordCacheSize;
    }
    NPT_CopyMemory(buffer, m_RecordCacheData, bytes_to_read);
    if (bytes_read) *bytes_read = bytes_to_read;

    // update the record cache
    m_RecordCacheSize -= bytes_to_read;
    if (m_RecordCacheSize == 0) {
        // nothing left in the cache
        m_RecordCacheData = NULL;
    } else {
        // move the cache pointer
        m_RecordCacheData += bytes_to_read;
    }
    
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_TlsInputStream::GetAvailable
+---------------------------------------------------------------------*/
NPT_Result 
NPT_TlsInputStream::GetAvailable(NPT_LargeSize& /*available*/)
{
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_TlsOutputStream
+---------------------------------------------------------------------*/
class NPT_TlsOutputStream : public NPT_OutputStream {
public:
    NPT_TlsOutputStream(NPT_TlsSessionImplReference& session) :
        m_Session(session),
        m_Position(0) {}
    
    // NPT_OutputStream methods
    virtual NPT_Result Write(const void* buffer, 
                             NPT_Size    bytes_to_write, 
                             NPT_Size*   bytes_written = NULL);
    virtual NPT_Result Seek(NPT_Position) { return NPT_ERROR_NOT_SUPPORTED; }
    virtual NPT_Result Tell(NPT_Position& offset) { offset = m_Position; return NPT_SUCCESS; }

private:
    NPT_TlsSessionImplReference m_Session;
    NPT_Position                m_Position;
};

/*----------------------------------------------------------------------
|   NPT_TlsOutputStream::Write
+---------------------------------------------------------------------*/
NPT_Result 
NPT_TlsOutputStream::Write(const void* buffer, 
                           NPT_Size    bytes_to_write, 
                           NPT_Size*   bytes_written)
{
    // setup default values
    if (bytes_written) *bytes_written = 0;
    
    // quick check for edge case 
    if (bytes_to_write == 0) return NPT_SUCCESS;
    
    // write some data
    int ssl_result;
    do {
        ssl_result = ssl_write(m_Session->m_SSL, (const uint8_t*)buffer, bytes_to_write);
    } while (ssl_result == 0);
    if (ssl_result < 0) {
        return NPT_Tls_MapResult(ssl_result);
    }
    m_Position += ssl_result;
    if (bytes_written) *bytes_written = (NPT_Size)ssl_result;
    
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_TlsContext::NPT_TlsContext
+---------------------------------------------------------------------*/
NPT_TlsContext::NPT_TlsContext() :
    m_Impl(new NPT_TlsContextImpl())
{
}

/*----------------------------------------------------------------------
|   NPT_TlsContext::~NPT_TlsContext
+---------------------------------------------------------------------*/
NPT_TlsContext::~NPT_TlsContext()
{
    delete m_Impl;
}

/*----------------------------------------------------------------------
|   NPT_TlsContext::LoadKey
+---------------------------------------------------------------------*/
NPT_Result 
NPT_TlsContext::LoadKey(NPT_TlsKeyFormat     key_format, 
                        const unsigned char* key_data,
                        NPT_Size             key_data_size,
                        const char*          password)
{
    return m_Impl->LoadKey(key_format, key_data, key_data_size, password);
}

/*----------------------------------------------------------------------
|   NPT_TlsClientSession::NPT_TlsClientSession
+---------------------------------------------------------------------*/
NPT_TlsClientSession::NPT_TlsClientSession(NPT_TlsContextReference&   context,
                                           NPT_InputStreamReference&  input,
                                           NPT_OutputStreamReference& output) :
    m_Context(context),
    m_Impl(new NPT_TlsSessionImpl(context->m_Impl->m_SSL_CTX, input, output))
{
}

/*----------------------------------------------------------------------
|   NPT_TlsClientSession::~NPT_TlsClientSession
+---------------------------------------------------------------------*/
NPT_TlsClientSession::~NPT_TlsClientSession()
{
}

/*----------------------------------------------------------------------
|   NPT_TlsClientSession::Handshake
+---------------------------------------------------------------------*/
NPT_Result 
NPT_TlsClientSession::Handshake()
{
    return m_Impl->Handshake();
}

/*----------------------------------------------------------------------
|   NPT_TlsClientSession::GetSessionId
+---------------------------------------------------------------------*/
NPT_Result 
NPT_TlsClientSession::GetSessionId(NPT_DataBuffer& session_id)
{
    return m_Impl->GetSessionId(session_id);
}

/*----------------------------------------------------------------------
|   NPT_TlsClientSession::GetCipherSuiteId
+---------------------------------------------------------------------*/
NPT_UInt32
NPT_TlsClientSession::GetCipherSuiteId()
{
    return m_Impl->GetCipherSuiteId();
}
              
/*----------------------------------------------------------------------
|   NPT_TlsSession::GetPeerCertificateInfo
+---------------------------------------------------------------------*/
NPT_Result
NPT_TlsClientSession::GetPeerCertificateInfo(NPT_TlsCertificateInfo& cert_info)
{
    return m_Impl->GetPeerCertificateInfo(cert_info);
}

/*----------------------------------------------------------------------
|   NPT_TlsClientSession::GetInputStream
+---------------------------------------------------------------------*/
NPT_Result 
NPT_TlsClientSession::GetInputStream(NPT_InputStreamReference& stream)
{
    stream = new NPT_TlsInputStream(m_Impl);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_TlsClientSession::GetOutputStream
+---------------------------------------------------------------------*/
NPT_Result 
NPT_TlsClientSession::GetOutputStream(NPT_OutputStreamReference& stream)
{
    stream = new NPT_TlsOutputStream(m_Impl);
    return NPT_SUCCESS;
}

#endif // NPT_CONFIG_ENABLE_TLS
