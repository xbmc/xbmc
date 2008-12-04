/* yassl_int.cpp                                
 *
 * Copyright (C) 2003 Sawtooth Consulting Ltd.
 *
 * This file is part of yaSSL, an SSL implementation written by Todd A Ouska
 * (todd at yassl.com, see www.yassl.com).
 *
 * yaSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * There are special exceptions to the terms and conditions of the GPL as it
 * is applied to yaSSL. View the full text of the exception in the file
 * FLOSS-EXCEPTIONS in the directory of this software distribution.
 *
 * yaSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */


/* yaSSL internal source implements SSL supporting types not specified in the
 * draft along with type conversion functions.
 */

#include "runtime.hpp"
#include "yassl_int.hpp"
#include "handshake.hpp"
#include "timer.hpp"

#ifdef _POSIX_THREADS
    #include "pthread.h"
#endif


#ifdef HAVE_LIBZ
    #include "zlib.h"
#endif


#ifdef YASSL_PURE_C

    void* operator new(size_t sz, yaSSL::new_t)
    {
        void* ptr = malloc(sz ? sz : 1);
        if (!ptr) abort();

        return ptr;
    }


    void operator delete(void* ptr, yaSSL::new_t)
    {
        if (ptr) free(ptr);
    }


    void* operator new[](size_t sz, yaSSL::new_t nt)
    {
        return ::operator new(sz, nt);
    }


    void operator delete[](void* ptr, yaSSL::new_t nt)
    {
        ::operator delete(ptr, nt);
    }

    namespace yaSSL {

        new_t ys;   // for yaSSL library new

    }

#endif // YASSL_PURE_C


namespace yaSSL {






// convert a 32 bit integer into a 24 bit one
void c32to24(uint32 u32, uint24& u24)
{
    u24[0] = (u32 >> 16) & 0xff;
    u24[1] = (u32 >>  8) & 0xff;
    u24[2] =  u32 & 0xff;
}


// convert a 24 bit integer into a 32 bit one
void c24to32(const uint24 u24, uint32& u32)
{
    u32 = 0;
    u32 = (u24[0] << 16) | (u24[1] << 8) | u24[2];
}


// convert with return for ease of use
uint32 c24to32(const uint24 u24)
{
    uint32 ret;
    c24to32(u24, ret);

    return ret;
}


// using a for opaque since underlying type is unsgined char and o is not a
// good leading identifier

// convert opaque to 16 bit integer
void ato16(const opaque* c, uint16& u16)
{
    u16 = 0;
    u16 = (c[0] << 8) | (c[1]);
}


// convert (copy) opaque to 24 bit integer
void ato24(const opaque* c, uint24& u24)
{
    u24[0] = c[0];
    u24[1] = c[1];
    u24[2] = c[2];
}


// convert 16 bit integer to opaque
void c16toa(uint16 u16, opaque* c)
{
    c[0] = (u16 >> 8) & 0xff;
    c[1] =  u16 & 0xff;
}


// convert 24 bit integer to opaque
void c24toa(const uint24 u24, opaque* c)
{
    c[0] =  u24[0]; 
    c[1] =  u24[1];
    c[2] =  u24[2];
}


// convert 32 bit integer to opaque
void c32toa(uint32 u32, opaque* c)
{
    c[0] = (u32 >> 24) & 0xff;
    c[1] = (u32 >> 16) & 0xff;
    c[2] = (u32 >>  8) & 0xff;
    c[3] =  u32 & 0xff;
}


States::States() : recordLayer_(recordReady), handshakeLayer_(preHandshake),
           clientState_(serverNull),  serverState_(clientNull),
           connectState_(CONNECT_BEGIN), acceptState_(ACCEPT_BEGIN),
           what_(no_error) {}

const RecordLayerState& States::getRecord() const 
{
    return recordLayer_;
}


const HandShakeState& States::getHandShake() const
{
    return handshakeLayer_;
}


const ClientState& States::getClient() const
{
    return clientState_;
}


const ServerState& States::getServer() const
{
    return serverState_;
}


const ConnectState& States::GetConnect() const
{
    return connectState_;
}


const AcceptState& States::GetAccept() const
{
    return acceptState_;
}


const char* States::getString() const
{
    return errorString_;
}


YasslError States::What() const
{
    return what_;
}


RecordLayerState& States::useRecord()
{
    return recordLayer_;
}


HandShakeState& States::useHandShake()
{
    return handshakeLayer_;
}


ClientState& States::useClient()
{
    return clientState_;
}


ServerState& States::useServer()
{
    return serverState_;
}


ConnectState& States::UseConnect()
{
    return connectState_;
}


AcceptState& States::UseAccept()
{
    return acceptState_;
}


char* States::useString()
{
    return errorString_;
}


void States::SetError(YasslError ye)
{
    what_ = ye;
}


sslFactory::sslFactory() :           
        messageFactory_(InitMessageFactory),
        handShakeFactory_(InitHandShakeFactory),
        serverKeyFactory_(InitServerKeyFactory),
        clientKeyFactory_(InitClientKeyFactory) 
{}


const MessageFactory& sslFactory::getMessage() const
{
    return messageFactory_;
}


const HandShakeFactory& sslFactory::getHandShake() const
{
    return handShakeFactory_;
}


const ServerKeyFactory& sslFactory::getServerKey() const
{
    return serverKeyFactory_;
}


const ClientKeyFactory& sslFactory::getClientKey() const
{
    return clientKeyFactory_;
}


// extract context parameters and store
SSL::SSL(SSL_CTX* ctx) 
    : secure_(ctx->getMethod()->getVersion(), crypto_.use_random(),
              ctx->getMethod()->getSide(), ctx->GetCiphers(), ctx,
              ctx->GetDH_Parms().set_), quietShutdown_(false), has_data_(false)
{
    if (int err = crypto_.get_random().GetError()) {
        SetError(YasslError(err));
        return;
    }

    CertManager& cm = crypto_.use_certManager();
    cm.CopySelfCert(ctx->getCert());

    bool serverSide = secure_.use_parms().entity_ == server_end;

    if (ctx->getKey()) {
        if (int err = cm.SetPrivateKey(*ctx->getKey())) {
            SetError(YasslError(err));
            return;
        }
    }
    else if (serverSide) {
        SetError(no_key_file);
        return;
    }

    if (ctx->getMethod()->verifyPeer())
        cm.setVerifyPeer();
    if (ctx->getMethod()->verifyNone())
        cm.setVerifyNone();
    if (ctx->getMethod()->failNoCert())
        cm.setFailNoCert();
    cm.setVerifyCallback(ctx->getVerifyCallback());

    if (serverSide)
        crypto_.SetDH(ctx->GetDH_Parms());

    const SSL_CTX::CertList& ca = ctx->GetCA_List();
    SSL_CTX::CertList::const_iterator first(ca.begin());
    SSL_CTX::CertList::const_iterator last(ca.end());

    while (first != last) {
        if (int err = cm.CopyCaCert(*first)) {
            SetError(YasslError(err));
            return;
        }
        ++first;
    }
}


// store pending security parameters from Server Hello
void SSL::set_pending(Cipher suite)
{
    Parameters& parms = secure_.use_parms();

    switch (suite) {

    case TLS_RSA_WITH_AES_256_CBC_SHA:
        parms.bulk_cipher_algorithm_ = aes;
        parms.mac_algorithm_         = sha;
        parms.kea_                   = rsa_kea;
        parms.hash_size_ = SHA_LEN;
        parms.key_size_  = AES_256_KEY_SZ;
        parms.iv_size_   = AES_BLOCK_SZ;
        parms.cipher_type_ = block;
        crypto_.setDigest(NEW_YS SHA);
        crypto_.setCipher(NEW_YS AES(AES_256_KEY_SZ));
        strncpy(parms.cipher_name_, cipher_names[TLS_RSA_WITH_AES_256_CBC_SHA],
                MAX_SUITE_NAME);
        break;

    case TLS_RSA_WITH_AES_128_CBC_SHA:
        parms.bulk_cipher_algorithm_ = aes;
        parms.mac_algorithm_         = sha;
        parms.kea_                   = rsa_kea;
        parms.hash_size_ = SHA_LEN;
        parms.key_size_  = AES_128_KEY_SZ;
        parms.iv_size_   = AES_BLOCK_SZ;
        parms.cipher_type_ = block;
        crypto_.setDigest(NEW_YS SHA);
        crypto_.setCipher(NEW_YS AES);
        strncpy(parms.cipher_name_, cipher_names[TLS_RSA_WITH_AES_128_CBC_SHA],
                MAX_SUITE_NAME);
        break;

    case SSL_RSA_WITH_3DES_EDE_CBC_SHA:
        parms.bulk_cipher_algorithm_ = triple_des;
        parms.mac_algorithm_         = sha;
        parms.kea_                   = rsa_kea;
        parms.hash_size_ = SHA_LEN;
        parms.key_size_  = DES_EDE_KEY_SZ;
        parms.iv_size_   = DES_IV_SZ;
        parms.cipher_type_ = block;
        crypto_.setDigest(NEW_YS SHA);
        crypto_.setCipher(NEW_YS DES_EDE);
        strncpy(parms.cipher_name_, cipher_names[SSL_RSA_WITH_3DES_EDE_CBC_SHA]
                , MAX_SUITE_NAME);
        break;

    case SSL_RSA_WITH_DES_CBC_SHA:
        parms.bulk_cipher_algorithm_ = des;
        parms.mac_algorithm_         = sha;
        parms.kea_                   = rsa_kea;
        parms.hash_size_ = SHA_LEN;
        parms.key_size_  = DES_KEY_SZ;
        parms.iv_size_   = DES_IV_SZ;
        parms.cipher_type_ = block;
        crypto_.setDigest(NEW_YS SHA);
        crypto_.setCipher(NEW_YS DES);
        strncpy(parms.cipher_name_, cipher_names[SSL_RSA_WITH_DES_CBC_SHA],
                MAX_SUITE_NAME);
        break;

    case SSL_RSA_WITH_RC4_128_SHA:
        parms.bulk_cipher_algorithm_ = rc4;
        parms.mac_algorithm_         = sha;
        parms.kea_                   = rsa_kea;
        parms.hash_size_ = SHA_LEN;
        parms.key_size_  = RC4_KEY_SZ;
        parms.iv_size_   = 0;
        parms.cipher_type_ = stream;
        crypto_.setDigest(NEW_YS SHA);
        crypto_.setCipher(NEW_YS RC4);
        strncpy(parms.cipher_name_, cipher_names[SSL_RSA_WITH_RC4_128_SHA],
                MAX_SUITE_NAME);
        break;

    case SSL_RSA_WITH_RC4_128_MD5:
        parms.bulk_cipher_algorithm_ = rc4;
        parms.mac_algorithm_         = md5;
        parms.kea_                   = rsa_kea;
        parms.hash_size_ = MD5_LEN;
        parms.key_size_  = RC4_KEY_SZ;
        parms.iv_size_   = 0;
        parms.cipher_type_ = stream;
        crypto_.setDigest(NEW_YS MD5);
        crypto_.setCipher(NEW_YS RC4);
        strncpy(parms.cipher_name_, cipher_names[SSL_RSA_WITH_RC4_128_MD5],
                MAX_SUITE_NAME);
        break;

    case SSL_DHE_RSA_WITH_DES_CBC_SHA:
        parms.bulk_cipher_algorithm_ = des;
        parms.mac_algorithm_         = sha;
        parms.kea_                   = diffie_hellman_kea;
        parms.sig_algo_              = rsa_sa_algo;
        parms.hash_size_ = SHA_LEN;
        parms.key_size_  = DES_KEY_SZ;
        parms.iv_size_   = DES_IV_SZ;
        parms.cipher_type_ = block;
        secure_.use_connection().send_server_key_  = true; // eph
        crypto_.setDigest(NEW_YS SHA);
        crypto_.setCipher(NEW_YS DES);
        strncpy(parms.cipher_name_, cipher_names[SSL_DHE_RSA_WITH_DES_CBC_SHA],
                MAX_SUITE_NAME);
        break;

    case SSL_DHE_RSA_WITH_3DES_EDE_CBC_SHA:
        parms.bulk_cipher_algorithm_ = triple_des;
        parms.mac_algorithm_         = sha;
        parms.kea_                   = diffie_hellman_kea;
        parms.sig_algo_              = rsa_sa_algo;
        parms.hash_size_ = SHA_LEN;
        parms.key_size_  = DES_EDE_KEY_SZ;
        parms.iv_size_   = DES_IV_SZ;
        parms.cipher_type_ = block;
        secure_.use_connection().send_server_key_  = true; // eph
        crypto_.setDigest(NEW_YS SHA);
        crypto_.setCipher(NEW_YS DES_EDE);
        strncpy(parms.cipher_name_,
              cipher_names[SSL_DHE_RSA_WITH_3DES_EDE_CBC_SHA], MAX_SUITE_NAME);
        break;

    case TLS_DHE_RSA_WITH_AES_256_CBC_SHA:
        parms.bulk_cipher_algorithm_ = aes;
        parms.mac_algorithm_         = sha;
        parms.kea_                   = diffie_hellman_kea;
        parms.sig_algo_              = rsa_sa_algo;
        parms.hash_size_ = SHA_LEN;
        parms.key_size_  = AES_256_KEY_SZ;
        parms.iv_size_   = AES_BLOCK_SZ;
        parms.cipher_type_ = block;
        secure_.use_connection().send_server_key_  = true; // eph
        crypto_.setDigest(NEW_YS SHA);
        crypto_.setCipher(NEW_YS AES(AES_256_KEY_SZ));
        strncpy(parms.cipher_name_,
               cipher_names[TLS_DHE_RSA_WITH_AES_256_CBC_SHA], MAX_SUITE_NAME);
        break;

    case TLS_DHE_RSA_WITH_AES_128_CBC_SHA:
        parms.bulk_cipher_algorithm_ = aes;
        parms.mac_algorithm_         = sha;
        parms.kea_                   = diffie_hellman_kea;
        parms.sig_algo_              = rsa_sa_algo;
        parms.hash_size_ = SHA_LEN;
        parms.key_size_  = AES_128_KEY_SZ;
        parms.iv_size_   = AES_BLOCK_SZ;
        parms.cipher_type_ = block;
        secure_.use_connection().send_server_key_  = true; // eph
        crypto_.setDigest(NEW_YS SHA);
        crypto_.setCipher(NEW_YS AES);
        strncpy(parms.cipher_name_,
               cipher_names[TLS_DHE_RSA_WITH_AES_128_CBC_SHA], MAX_SUITE_NAME);
        break;

    case SSL_DHE_DSS_WITH_DES_CBC_SHA:
        parms.bulk_cipher_algorithm_ = des;
        parms.mac_algorithm_         = sha;
        parms.kea_                   = diffie_hellman_kea;
        parms.sig_algo_              = dsa_sa_algo;
        parms.hash_size_ = SHA_LEN;
        parms.key_size_  = DES_KEY_SZ;
        parms.iv_size_   = DES_IV_SZ;
        parms.cipher_type_ = block;
        secure_.use_connection().send_server_key_  = true; // eph
        crypto_.setDigest(NEW_YS SHA);
        crypto_.setCipher(NEW_YS DES);
        strncpy(parms.cipher_name_, cipher_names[SSL_DHE_DSS_WITH_DES_CBC_SHA],
                MAX_SUITE_NAME);
        break;

    case SSL_DHE_DSS_WITH_3DES_EDE_CBC_SHA:
        parms.bulk_cipher_algorithm_ = triple_des;
        parms.mac_algorithm_         = sha;
        parms.kea_                   = diffie_hellman_kea;
        parms.sig_algo_              = dsa_sa_algo;
        parms.hash_size_ = SHA_LEN;
        parms.key_size_  = DES_EDE_KEY_SZ;
        parms.iv_size_   = DES_IV_SZ;
        parms.cipher_type_ = block;
        secure_.use_connection().send_server_key_  = true; // eph
        crypto_.setDigest(NEW_YS SHA);
        crypto_.setCipher(NEW_YS DES_EDE);
        strncpy(parms.cipher_name_,
              cipher_names[SSL_DHE_DSS_WITH_3DES_EDE_CBC_SHA], MAX_SUITE_NAME);
        break;

    case TLS_DHE_DSS_WITH_AES_256_CBC_SHA:
        parms.bulk_cipher_algorithm_ = aes;
        parms.mac_algorithm_         = sha;
        parms.kea_                   = diffie_hellman_kea;
        parms.sig_algo_              = dsa_sa_algo;
        parms.hash_size_ = SHA_LEN;
        parms.key_size_  = AES_256_KEY_SZ;
        parms.iv_size_   = AES_BLOCK_SZ;
        parms.cipher_type_ = block;
        secure_.use_connection().send_server_key_  = true; // eph
        crypto_.setDigest(NEW_YS SHA);
        crypto_.setCipher(NEW_YS AES(AES_256_KEY_SZ));
        strncpy(parms.cipher_name_,
               cipher_names[TLS_DHE_DSS_WITH_AES_256_CBC_SHA], MAX_SUITE_NAME);
        break;

    case TLS_DHE_DSS_WITH_AES_128_CBC_SHA:
        parms.bulk_cipher_algorithm_ = aes;
        parms.mac_algorithm_         = sha;
        parms.kea_                   = diffie_hellman_kea;
        parms.sig_algo_              = dsa_sa_algo;
        parms.hash_size_ = SHA_LEN;
        parms.key_size_  = AES_128_KEY_SZ;
        parms.iv_size_   = AES_BLOCK_SZ;
        parms.cipher_type_ = block;
        secure_.use_connection().send_server_key_  = true; // eph
        crypto_.setDigest(NEW_YS SHA);
        crypto_.setCipher(NEW_YS AES);
        strncpy(parms.cipher_name_,
               cipher_names[TLS_DHE_DSS_WITH_AES_128_CBC_SHA], MAX_SUITE_NAME);
        break;

    case TLS_RSA_WITH_AES_256_CBC_RMD160:
        parms.bulk_cipher_algorithm_ = aes;
        parms.mac_algorithm_         = rmd;
        parms.kea_                   = rsa_kea;
        parms.hash_size_ = RMD_LEN;
        parms.key_size_  = AES_256_KEY_SZ;
        parms.iv_size_   = AES_BLOCK_SZ;
        parms.cipher_type_ = block;
        crypto_.setDigest(NEW_YS RMD);
        crypto_.setCipher(NEW_YS AES(AES_256_KEY_SZ));
        strncpy(parms.cipher_name_,
                cipher_names[TLS_RSA_WITH_AES_256_CBC_RMD160], MAX_SUITE_NAME);
        break;

    case TLS_RSA_WITH_AES_128_CBC_RMD160:
        parms.bulk_cipher_algorithm_ = aes;
        parms.mac_algorithm_         = rmd;
        parms.kea_                   = rsa_kea;
        parms.hash_size_ = RMD_LEN;
        parms.key_size_  = AES_128_KEY_SZ;
        parms.iv_size_   = AES_BLOCK_SZ;
        parms.cipher_type_ = block;
        crypto_.setDigest(NEW_YS RMD);
        crypto_.setCipher(NEW_YS AES);
        strncpy(parms.cipher_name_,
                cipher_names[TLS_RSA_WITH_AES_128_CBC_RMD160], MAX_SUITE_NAME);
        break;

    case TLS_RSA_WITH_3DES_EDE_CBC_RMD160:
        parms.bulk_cipher_algorithm_ = triple_des;
        parms.mac_algorithm_         = rmd;
        parms.kea_                   = rsa_kea;
        parms.hash_size_ = RMD_LEN;
        parms.key_size_  = DES_EDE_KEY_SZ;
        parms.iv_size_   = DES_IV_SZ;
        parms.cipher_type_ = block;
        crypto_.setDigest(NEW_YS RMD);
        crypto_.setCipher(NEW_YS DES_EDE);
        strncpy(parms.cipher_name_,
               cipher_names[TLS_RSA_WITH_3DES_EDE_CBC_RMD160], MAX_SUITE_NAME);
        break;

    case TLS_DHE_RSA_WITH_3DES_EDE_CBC_RMD160:
        parms.bulk_cipher_algorithm_ = triple_des;
        parms.mac_algorithm_         = rmd;
        parms.kea_                   = diffie_hellman_kea;
        parms.sig_algo_              = rsa_sa_algo;
        parms.hash_size_ = RMD_LEN;
        parms.key_size_  = DES_EDE_KEY_SZ;
        parms.iv_size_   = DES_IV_SZ;
        parms.cipher_type_ = block;
        secure_.use_connection().send_server_key_  = true; // eph
        crypto_.setDigest(NEW_YS RMD);
        crypto_.setCipher(NEW_YS DES_EDE);
        strncpy(parms.cipher_name_,
                cipher_names[TLS_DHE_RSA_WITH_3DES_EDE_CBC_RMD160],
                MAX_SUITE_NAME);
        break;

    case TLS_DHE_RSA_WITH_AES_256_CBC_RMD160:
        parms.bulk_cipher_algorithm_ = aes;
        parms.mac_algorithm_         = rmd;
        parms.kea_                   = diffie_hellman_kea;
        parms.sig_algo_              = rsa_sa_algo;
        parms.hash_size_ = RMD_LEN;
        parms.key_size_  = AES_256_KEY_SZ;
        parms.iv_size_   = AES_BLOCK_SZ;
        parms.cipher_type_ = block;
        secure_.use_connection().send_server_key_  = true; // eph
        crypto_.setDigest(NEW_YS RMD);
        crypto_.setCipher(NEW_YS AES(AES_256_KEY_SZ));
        strncpy(parms.cipher_name_,
                cipher_names[TLS_DHE_RSA_WITH_AES_256_CBC_RMD160],
                MAX_SUITE_NAME);
        break;

    case TLS_DHE_RSA_WITH_AES_128_CBC_RMD160:
        parms.bulk_cipher_algorithm_ = aes;
        parms.mac_algorithm_         = rmd;
        parms.kea_                   = diffie_hellman_kea;
        parms.sig_algo_              = rsa_sa_algo;
        parms.hash_size_ = RMD_LEN;
        parms.key_size_  = AES_128_KEY_SZ;
        parms.iv_size_   = AES_BLOCK_SZ;
        parms.cipher_type_ = block;
        secure_.use_connection().send_server_key_  = true; // eph
        crypto_.setDigest(NEW_YS RMD);
        crypto_.setCipher(NEW_YS AES);
        strncpy(parms.cipher_name_,
                cipher_names[TLS_DHE_RSA_WITH_AES_128_CBC_RMD160],
                MAX_SUITE_NAME);
        break;

    case TLS_DHE_DSS_WITH_3DES_EDE_CBC_RMD160:
        parms.bulk_cipher_algorithm_ = triple_des;
        parms.mac_algorithm_         = rmd;
        parms.kea_                   = diffie_hellman_kea;
        parms.sig_algo_              = dsa_sa_algo;
        parms.hash_size_ = RMD_LEN;
        parms.key_size_  = DES_EDE_KEY_SZ;
        parms.iv_size_   = DES_IV_SZ;
        parms.cipher_type_ = block;
        secure_.use_connection().send_server_key_  = true; // eph
        crypto_.setDigest(NEW_YS RMD);
        crypto_.setCipher(NEW_YS DES_EDE);
        strncpy(parms.cipher_name_,
                cipher_names[TLS_DHE_DSS_WITH_3DES_EDE_CBC_RMD160],
                MAX_SUITE_NAME);
        break;

    case TLS_DHE_DSS_WITH_AES_256_CBC_RMD160:
        parms.bulk_cipher_algorithm_ = aes;
        parms.mac_algorithm_         = rmd;
        parms.kea_                   = diffie_hellman_kea;
        parms.sig_algo_              = dsa_sa_algo;
        parms.hash_size_ = RMD_LEN;
        parms.key_size_  = AES_256_KEY_SZ;
        parms.iv_size_   = AES_BLOCK_SZ;
        parms.cipher_type_ = block;
        secure_.use_connection().send_server_key_  = true; // eph
        crypto_.setDigest(NEW_YS RMD);
        crypto_.setCipher(NEW_YS AES(AES_256_KEY_SZ));
        strncpy(parms.cipher_name_,
                cipher_names[TLS_DHE_DSS_WITH_AES_256_CBC_RMD160],
                MAX_SUITE_NAME);
        break;

    case TLS_DHE_DSS_WITH_AES_128_CBC_RMD160:
        parms.bulk_cipher_algorithm_ = aes;
        parms.mac_algorithm_         = rmd;
        parms.kea_                   = diffie_hellman_kea;
        parms.sig_algo_              = dsa_sa_algo;
        parms.hash_size_ = RMD_LEN;
        parms.key_size_  = AES_128_KEY_SZ;
        parms.iv_size_   = AES_BLOCK_SZ;
        parms.cipher_type_ = block;
        secure_.use_connection().send_server_key_  = true; // eph
        crypto_.setDigest(NEW_YS RMD);
        crypto_.setCipher(NEW_YS AES);
        strncpy(parms.cipher_name_,
                cipher_names[TLS_DHE_DSS_WITH_AES_128_CBC_RMD160],
                MAX_SUITE_NAME);
        break;

    default:
        SetError(unknown_cipher);
    }
}


// store peer's random
void SSL::set_random(const opaque* random, ConnectionEnd sender)
{
    if (sender == client_end)
        memcpy(secure_.use_connection().client_random_, random, RAN_LEN);
    else
        memcpy(secure_.use_connection().server_random_, random, RAN_LEN);
}


// store client pre master secret
void SSL::set_preMaster(const opaque* pre, uint sz)
{
    secure_.use_connection().AllocPreSecret(sz);
    memcpy(secure_.use_connection().pre_master_secret_, pre, sz);
}


// set yaSSL zlib type compression
int SSL::SetCompression()
{
#ifdef HAVE_LIBZ
    secure_.use_connection().compression_ = true;
    return 0;
#else
    return -1;  // not built in
#endif
}


// unset yaSSL zlib type compression
void SSL::UnSetCompression()
{
    secure_.use_connection().compression_ = false;
}


// is yaSSL zlib compression on
bool SSL::CompressionOn() const
{
    return secure_.get_connection().compression_;
}


// store master secret
void SSL::set_masterSecret(const opaque* sec)
{
    memcpy(secure_.use_connection().master_secret_, sec, SECRET_LEN);
}

// store server issued id
void SSL::set_sessionID(const opaque* sessionID)
{
    memcpy(secure_.use_connection().sessionID_, sessionID, ID_LEN);
    secure_.use_connection().sessionID_Set_ = true;
}


// store error 
void SSL::SetError(YasslError ye)
{
    states_.SetError(ye);
    //strncpy(states_.useString(), e.what(), mySTL::named_exception::NAME_SIZE);
    // TODO: add string here
}


// set the quiet shutdown mode (close_nofiy not sent or received on shutdown)
void SSL::SetQuietShutdown(bool mode)
{
  quietShutdown_ = mode;
}


Buffers& SSL::useBuffers()
{
    return buffers_;
}


// locals
namespace {

// DeriveKeys and MasterSecret helper sets prefix letters
static bool setPrefix(opaque* sha_input, int i)
{
    switch (i) {
    case 0:
        memcpy(sha_input, "A", 1);
        break;
    case 1:
        memcpy(sha_input, "BB", 2);
        break;
    case 2:
        memcpy(sha_input, "CCC", 3);
        break;
    case 3:
        memcpy(sha_input, "DDDD", 4);
        break;
    case 4:
        memcpy(sha_input, "EEEEE", 5);
        break;
    case 5:
        memcpy(sha_input, "FFFFFF", 6);
        break;
    case 6:
        memcpy(sha_input, "GGGGGGG", 7);
        break;
    default:
        return false;  // prefix_error
    }
    return true;
}


const char handshake_order[] = "Out of order HandShake Message!";


} // namespcae for locals


void SSL::order_error()
{
    SetError(out_of_order);
}


// Create and store the master secret see page 32, 6.1
void SSL::makeMasterSecret()
{
    if (isTLS())
        makeTLSMasterSecret();
    else {
        opaque sha_output[SHA_LEN];

        const uint& preSz = secure_.get_connection().pre_secret_len_;
        output_buffer md5_input(preSz + SHA_LEN);
        output_buffer sha_input(PREFIX + preSz + 2 * RAN_LEN);

        MD5 md5;
        SHA sha;

        md5_input.write(secure_.get_connection().pre_master_secret_, preSz);

        for (int i = 0; i < MASTER_ROUNDS; ++i) {
            opaque prefix[PREFIX];
            if (!setPrefix(prefix, i)) {
                SetError(prefix_error);
                return;
            }

            sha_input.set_current(0);
            sha_input.write(prefix, i + 1);

            sha_input.write(secure_.get_connection().pre_master_secret_,preSz);
            sha_input.write(secure_.get_connection().client_random_, RAN_LEN);
            sha_input.write(secure_.get_connection().server_random_, RAN_LEN);
            sha.get_digest(sha_output, sha_input.get_buffer(),
                           sha_input.get_size());

            md5_input.set_current(preSz);
            md5_input.write(sha_output, SHA_LEN);
            md5.get_digest(&secure_.use_connection().master_secret_[i*MD5_LEN],
                           md5_input.get_buffer(), md5_input.get_size());
        }
        deriveKeys();
    }
    secure_.use_connection().CleanPreMaster();
}


// create TLSv1 master secret
void SSL::makeTLSMasterSecret()
{
    opaque seed[SEED_LEN];
    
    memcpy(seed, secure_.get_connection().client_random_, RAN_LEN);
    memcpy(&seed[RAN_LEN], secure_.get_connection().server_random_, RAN_LEN);

    PRF(secure_.use_connection().master_secret_, SECRET_LEN,
        secure_.get_connection().pre_master_secret_,
        secure_.get_connection().pre_secret_len_,
        master_label, MASTER_LABEL_SZ, 
        seed, SEED_LEN);

    deriveTLSKeys();
}


// derive mac, write, and iv keys for server and client, see page 34, 6.2.2
void SSL::deriveKeys()
{
    int length = 2 * secure_.get_parms().hash_size_ + 
                 2 * secure_.get_parms().key_size_  +
                 2 * secure_.get_parms().iv_size_;
    int rounds = (length + MD5_LEN - 1 ) / MD5_LEN;
    input_buffer key_data(rounds * MD5_LEN);

    opaque sha_output[SHA_LEN];
    opaque md5_input[SECRET_LEN + SHA_LEN];
    opaque sha_input[KEY_PREFIX + SECRET_LEN + 2 * RAN_LEN];
  
    MD5 md5;
    SHA sha;

    memcpy(md5_input, secure_.get_connection().master_secret_, SECRET_LEN);

    for (int i = 0; i < rounds; ++i) {
        int j = i + 1;
        if (!setPrefix(sha_input, i)) {
            SetError(prefix_error);
            return;
        }

        memcpy(&sha_input[j], secure_.get_connection().master_secret_,
               SECRET_LEN);
        memcpy(&sha_input[j+SECRET_LEN],
               secure_.get_connection().server_random_, RAN_LEN);
        memcpy(&sha_input[j + SECRET_LEN + RAN_LEN],
               secure_.get_connection().client_random_, RAN_LEN);
        sha.get_digest(sha_output, sha_input,
                       sizeof(sha_input) - KEY_PREFIX + j);

        memcpy(&md5_input[SECRET_LEN], sha_output, SHA_LEN);
        md5.get_digest(key_data.get_buffer() + i * MD5_LEN,
                       md5_input, sizeof(md5_input));
    }
    storeKeys(key_data.get_buffer());
}


// derive mac, write, and iv keys for server and client
void SSL::deriveTLSKeys()
{
    int length = 2 * secure_.get_parms().hash_size_ + 
                 2 * secure_.get_parms().key_size_  +
                 2 * secure_.get_parms().iv_size_;
    opaque       seed[SEED_LEN];
    input_buffer key_data(length);

    memcpy(seed, secure_.get_connection().server_random_, RAN_LEN);
    memcpy(&seed[RAN_LEN], secure_.get_connection().client_random_, RAN_LEN);

    PRF(key_data.get_buffer(), length, secure_.get_connection().master_secret_,
        SECRET_LEN, key_label, KEY_LABEL_SZ, seed, SEED_LEN);

    storeKeys(key_data.get_buffer());
}


// store mac, write, and iv keys for client and server
void SSL::storeKeys(const opaque* key_data)
{
    int sz = secure_.get_parms().hash_size_;
    memcpy(secure_.use_connection().client_write_MAC_secret_, key_data, sz);
    int i = sz;
    memcpy(secure_.use_connection().server_write_MAC_secret_,&key_data[i], sz);
    i += sz;

    sz = secure_.get_parms().key_size_;
    memcpy(secure_.use_connection().client_write_key_, &key_data[i], sz);
    i += sz;
    memcpy(secure_.use_connection().server_write_key_, &key_data[i], sz);
    i += sz;

    sz = secure_.get_parms().iv_size_;
    memcpy(secure_.use_connection().client_write_IV_, &key_data[i], sz);
    i += sz;
    memcpy(secure_.use_connection().server_write_IV_, &key_data[i], sz);

    setKeys();
}


// set encrypt/decrypt keys and ivs
void SSL::setKeys()
{
    Connection& conn = secure_.use_connection();

    if (secure_.get_parms().entity_ == client_end) {
        crypto_.use_cipher().set_encryptKey(conn.client_write_key_, 
                                            conn.client_write_IV_);
        crypto_.use_cipher().set_decryptKey(conn.server_write_key_,
                                            conn.server_write_IV_);
    }
    else {
        crypto_.use_cipher().set_encryptKey(conn.server_write_key_,
                                            conn.server_write_IV_);
        crypto_.use_cipher().set_decryptKey(conn.client_write_key_,
                                            conn.client_write_IV_);
    }
}



// local functors
namespace yassl_int_cpp_local1 {  // for explicit templates

struct SumData {
    uint total_;
    SumData() : total_(0) {}
    void operator()(input_buffer* data) { total_ += data->get_remaining(); }
};


struct SumBuffer {
    uint total_;
    SumBuffer() : total_(0) {}
    void operator()(output_buffer* buffer) { total_ += buffer->get_size(); }
};

} // namespace for locals
using namespace yassl_int_cpp_local1;


uint SSL::bufferedData()
{
    return STL::for_each(buffers_.getData().begin(),buffers_.getData().end(),
                           SumData()).total_;
}


// use input buffer to fill data
void SSL::fillData(Data& data)
{
    if (GetError()) return;
    uint dataSz   = data.get_length();        // input, data size to fill
    size_t elements = buffers_.getData().size();

    data.set_length(0);                         // output, actual data filled
    dataSz = min(dataSz, bufferedData());

    for (size_t i = 0; i < elements; i++) {
        input_buffer* front = buffers_.getData().front();
        uint frontSz = front->get_remaining();
        uint readSz  = min(dataSz - data.get_length(), frontSz);

        front->read(data.set_buffer() + data.get_length(), readSz);
        data.set_length(data.get_length() + readSz);

        if (readSz == frontSz) {
            buffers_.useData().pop_front();
            ysDelete(front);
        }
        if (data.get_length() == dataSz)
            break;
    }
    
    if (buffers_.getData().size() == 0) has_data_ = false;  // none left
}


// like Fill but keep data in buffer
void SSL::PeekData(Data& data)
{
    if (GetError()) return;
    uint   dataSz   = data.get_length();        // input, data size to fill
    size_t elements = buffers_.getData().size();

    data.set_length(0);                         // output, actual data filled
    dataSz = min(dataSz, bufferedData());

    Buffers::inputList::iterator front = buffers_.useData().begin();

    while (elements) {
        uint frontSz = (*front)->get_remaining();
        uint readSz  = min(dataSz - data.get_length(), frontSz);
        uint before  = (*front)->get_current();

        (*front)->read(data.set_buffer() + data.get_length(), readSz);
        data.set_length(data.get_length() + readSz);
        (*front)->set_current(before);

        if (data.get_length() == dataSz)
            break;

        elements--;
        front++;
    }
}


// flush output buffer
void SSL::flushBuffer()
{
    if (GetError()) return;

    uint sz = STL::for_each(buffers_.getHandShake().begin(),
                            buffers_.getHandShake().end(),
                            SumBuffer()).total_;
    output_buffer out(sz);
    size_t elements = buffers_.getHandShake().size();

    for (size_t i = 0; i < elements; i++) {
        output_buffer* front = buffers_.getHandShake().front();
        out.write(front->get_buffer(), front->get_size());

        buffers_.useHandShake().pop_front();
        ysDelete(front);
    }
    Send(out.get_buffer(), out.get_size());
}


void SSL::Send(const byte* buffer, uint sz)
{
    if (socket_.send(buffer, sz) != sz)
        SetError(send_error);
}


// get sequence number, if verify get peer's
uint SSL::get_SEQIncrement(bool verify) 
{ 
    if (verify)
        return secure_.use_connection().peer_sequence_number_++; 
    else
        return secure_.use_connection().sequence_number_++; 
}


const byte* SSL::get_macSecret(bool verify)
{
    if ( (secure_.get_parms().entity_ == client_end && !verify) ||
         (secure_.get_parms().entity_ == server_end &&  verify) )
        return secure_.get_connection().client_write_MAC_secret_;
    else
        return secure_.get_connection().server_write_MAC_secret_;
}


void SSL::verifyState(const RecordLayerHeader& rlHeader)
{
    if (GetError()) return;

    if (rlHeader.version_.major_ != 3 || rlHeader.version_.minor_ > 2) {
        SetError(badVersion_error);
        return;
    }

    if (states_.getRecord() == recordNotReady || 
            (rlHeader.type_ == application_data &&        // data and handshake
             states_.getHandShake() != handShakeReady) )  // isn't complete yet
              SetError(record_layer);
}


void SSL::verifyState(const HandShakeHeader& hsHeader)
{
    if (GetError()) return;

    if (states_.getHandShake() == handShakeNotReady) {
        SetError(handshake_layer);
        return;
    }

    if (secure_.get_parms().entity_ == client_end)
        verifyClientState(hsHeader.get_handshakeType());
    else
        verifyServerState(hsHeader.get_handshakeType());
}


void SSL::verifyState(ClientState cs)
{
    if (GetError()) return;
    if (states_.getClient() != cs) order_error();
}


void SSL::verifyState(ServerState ss)
{
    if (GetError()) return;
    if (states_.getServer() != ss) order_error();
}


void SSL::verfiyHandShakeComplete()
{
    if (GetError()) return;
    if (states_.getHandShake() != handShakeReady) order_error();
}


void SSL::verifyClientState(HandShakeType hsType)
{
    if (GetError()) return;

    switch(hsType) {
    case server_hello :
        if (states_.getClient() != serverNull)
            order_error();
        break;
    case certificate :
        if (states_.getClient() != serverHelloComplete)
            order_error();
        break;
    case server_key_exchange :
        if (states_.getClient() != serverCertComplete)
            order_error();
        break;
    case certificate_request :
        if (states_.getClient() != serverCertComplete &&
            states_.getClient() != serverKeyExchangeComplete)
            order_error();
        break;
    case server_hello_done :
        if (states_.getClient() != serverCertComplete &&
            states_.getClient() != serverKeyExchangeComplete)
            order_error();
        break;
    case finished :
        if (states_.getClient() != serverHelloDoneComplete || 
            secure_.get_parms().pending_)    // no change
                order_error();          // cipher yet
        break;
    default :
        order_error();
    };
}


void SSL::verifyServerState(HandShakeType hsType)
{
    if (GetError()) return;

    switch(hsType) {
    case client_hello :
        if (states_.getServer() != clientNull)
            order_error();
        break;
    case certificate :
        if (states_.getServer() != clientHelloComplete)
            order_error();
        break;
    case client_key_exchange :
        if (states_.getServer() != clientHelloComplete)
            order_error();
        break;
    case certificate_verify :
        if (states_.getServer() != clientKeyExchangeComplete)
            order_error();
        break;
    case finished :
        if (states_.getServer() != clientKeyExchangeComplete || 
            secure_.get_parms().pending_)    // no change
                order_error();               // cipher yet
        break;
    default :
        order_error();
    };
}


// try to find a suite match
void SSL::matchSuite(const opaque* peer, uint length)
{
    if (length == 0 || (length % 2) != 0) {
        SetError(bad_input);
        return;
    }

    // start with best, if a match we are good, Ciphers are at odd index
    // since all SSL and TLS ciphers have 0x00 first byte
    for (uint i = 1; i < secure_.get_parms().suites_size_; i += 2)
        for (uint j = 1; j < length; j+= 2)
            if (secure_.use_parms().suites_[i] == peer[j]) {
                secure_.use_parms().suite_[0] = 0x00;
                secure_.use_parms().suite_[1] = peer[j];
                return;
            }

    SetError(match_error);
}


void SSL::set_session(SSL_SESSION* s) 
{
    if (getSecurity().GetContext()->GetSessionCacheOff())
        return;

    if (s && GetSessions().lookup(s->GetID(), &secure_.use_resume())) {
        secure_.set_resuming(true);
        crypto_.use_certManager().setPeerX509(s->GetPeerX509());
    }
}


const Crypto& SSL::getCrypto() const
{
    return crypto_;
}


const Security& SSL::getSecurity() const
{
    return secure_;
}


const States& SSL::getStates() const
{
    return states_;
}


const sslHashes& SSL::getHashes() const
{
    return hashes_;
}


const sslFactory& SSL::getFactory() const
{
    return GetSSL_Factory();
}


const Socket& SSL::getSocket() const
{
    return socket_;
}


YasslError SSL::GetError() const
{
    return states_.What();
}


bool SSL::GetQuietShutdown() const
{
    return quietShutdown_;
}


bool SSL::GetMultiProtocol() const
{
    return secure_.GetContext()->getMethod()->multipleProtocol();
}


Crypto& SSL::useCrypto()
{
    return crypto_;
}


Security& SSL::useSecurity()
{
    return secure_;
}


States& SSL::useStates()
{
    return states_;
}


sslHashes& SSL::useHashes()
{
    return hashes_;
}


Socket& SSL::useSocket()
{
    return socket_;
}


Log& SSL::useLog()
{
    return log_;
}


bool SSL::isTLS() const
{
    return secure_.get_connection().TLS_;
}


bool SSL::isTLSv1_1() const
{
    return secure_.get_connection().TLSv1_1_;
}


// is there buffered data available, optimization to remove iteration on buffer
bool SSL::HasData() const
{ 
    return has_data_;
}


void SSL::addData(input_buffer* data)
{
    buffers_.useData().push_back(data);
    if (!has_data_) has_data_ = true;
}


void SSL::addBuffer(output_buffer* b)
{
    buffers_.useHandShake().push_back(b);
}


void SSL_SESSION::CopyX509(X509* x)
{
    assert(peerX509_ == 0);
    if (x == 0) return;

    X509_NAME* issuer   = x->GetIssuer();
    X509_NAME* subject  = x->GetSubject();
    ASN1_STRING* before = x->GetBefore();
    ASN1_STRING* after  = x->GetAfter();

    peerX509_ = NEW_YS X509(issuer->GetName(), issuer->GetLength(),
        subject->GetName(), subject->GetLength(), (const char*) before->data,
        before->length, (const char*) after->data, after->length);
}


// store connection parameters
SSL_SESSION::SSL_SESSION(const SSL& ssl, RandomPool& ran) 
    : timeout_(DEFAULT_TIMEOUT), random_(ran), peerX509_(0)
{
    const Connection& conn = ssl.getSecurity().get_connection();

    memcpy(sessionID_, conn.sessionID_, ID_LEN);
    memcpy(master_secret_, conn.master_secret_, SECRET_LEN);
    memcpy(suite_, ssl.getSecurity().get_parms().suite_, SUITE_LEN);

    bornOn_ = lowResTimer();

    CopyX509(ssl.getCrypto().get_certManager().get_peerX509());
}


// for resumption copy in ssl::parameters
SSL_SESSION::SSL_SESSION(RandomPool& ran) 
    : bornOn_(0), timeout_(0), random_(ran), peerX509_(0)
{
    memset(sessionID_, 0, ID_LEN);
    memset(master_secret_, 0, SECRET_LEN);
    memset(suite_, 0, SUITE_LEN);
}


SSL_SESSION& SSL_SESSION::operator=(const SSL_SESSION& that)
{
    memcpy(sessionID_, that.sessionID_, ID_LEN);
    memcpy(master_secret_, that.master_secret_, SECRET_LEN);
    memcpy(suite_, that.suite_, SUITE_LEN);
    
    bornOn_  = that.bornOn_;
    timeout_ = that.timeout_;

    if (peerX509_) {
        ysDelete(peerX509_);
        peerX509_ = 0;
    }
    CopyX509(that.peerX509_);

    return *this;
}


const opaque* SSL_SESSION::GetID() const
{
    return sessionID_;
}


const opaque* SSL_SESSION::GetSecret() const
{
    return master_secret_;
}


const Cipher* SSL_SESSION::GetSuite() const
{
    return suite_;
}


X509* SSL_SESSION::GetPeerX509() const
{
    return peerX509_;
}


uint SSL_SESSION::GetBornOn() const
{
    return bornOn_;
}


uint SSL_SESSION::GetTimeOut() const
{
    return timeout_;
}


void SSL_SESSION::SetTimeOut(uint t)
{
    timeout_ = t;
}


extern void clean(volatile opaque*, uint, RandomPool&);


// clean up secret data
SSL_SESSION::~SSL_SESSION()
{
    volatile opaque* p = master_secret_;
    clean(p, SECRET_LEN, random_);

    ysDelete(peerX509_);
}


static Sessions* sessionsInstance = 0;

Sessions& GetSessions()
{
    if (!sessionsInstance)
        sessionsInstance = NEW_YS Sessions;
    return *sessionsInstance;
}


static sslFactory* sslFactoryInstance = 0;

sslFactory& GetSSL_Factory()
{  
    if (!sslFactoryInstance)
        sslFactoryInstance = NEW_YS sslFactory;
    return *sslFactoryInstance;
}


static Errors* errorsInstance = 0;

Errors& GetErrors()
{
    if (!errorsInstance)
        errorsInstance = NEW_YS Errors;
    return *errorsInstance;
}


typedef Mutex::Lock Lock;


 
void Sessions::add(const SSL& ssl) 
{
    if (ssl.getSecurity().get_connection().sessionID_Set_) {
        Lock guard(mutex_);
        list_.push_back(NEW_YS SSL_SESSION(ssl, random_));
        count_++;
    }

    if (count_ > SESSION_FLUSH_COUNT)
        if (!ssl.getSecurity().GetContext()->GetSessionCacheFlushOff())
            Flush();
}


Sessions::~Sessions() 
{ 
    STL::for_each(list_.begin(), list_.end(), del_ptr_zero()); 
}


// locals
namespace yassl_int_cpp_local2 { // for explicit templates

typedef STL::list<SSL_SESSION*>::iterator sess_iterator;
typedef STL::list<ThreadError>::iterator  thr_iterator;

struct sess_match {
    const opaque* id_;
    explicit sess_match(const opaque* p) : id_(p) {}

    bool operator()(SSL_SESSION* sess)
    {
        if ( memcmp(sess->GetID(), id_, ID_LEN) == 0)
            return true;
        return false;
    }
};


THREAD_ID_T GetSelf()
{
#ifndef _POSIX_THREADS
    return GetCurrentThreadId();
#else
    return pthread_self();
#endif
}

struct thr_match {
    THREAD_ID_T id_;
    explicit thr_match() : id_(GetSelf()) {}

    bool operator()(ThreadError thr)
    {
        if (thr.threadID_ == id_)
            return true;
        return false;
    }
};


} // local namespace
using namespace yassl_int_cpp_local2;


// lookup session by id, return a copy if space provided
SSL_SESSION* Sessions::lookup(const opaque* id, SSL_SESSION* copy)
{
    Lock guard(mutex_);
    sess_iterator find = STL::find_if(list_.begin(), list_.end(),
                                        sess_match(id));
    if (find != list_.end()) {
        uint current = lowResTimer();
        if ( ((*find)->GetBornOn() + (*find)->GetTimeOut()) < current) {
            del_ptr_zero()(*find);
            list_.erase(find);
            return 0;
        }
        if (copy)
            *copy = *(*find);
        return *find;
    }
    return 0;
}


// remove a session by id
void Sessions::remove(const opaque* id)
{
    Lock guard(mutex_);
    sess_iterator find = STL::find_if(list_.begin(), list_.end(),
                                        sess_match(id));
    if (find != list_.end()) {
        del_ptr_zero()(*find);
        list_.erase(find);
    }
}


// flush expired sessions from cache 
void Sessions::Flush()
{
    Lock guard(mutex_);
    sess_iterator next = list_.begin();
    uint current = lowResTimer();

    while (next != list_.end()) {
        sess_iterator si = next;
        ++next;
        if ( ((*si)->GetBornOn() + (*si)->GetTimeOut()) < current) {
            del_ptr_zero()(*si);
            list_.erase(si);
        }
    }
    count_ = 0;  // reset flush counter
}


// remove a self thread error
void Errors::Remove()
{
    Lock guard(mutex_);
    thr_iterator find = STL::find_if(list_.begin(), list_.end(),
                                       thr_match());
    if (find != list_.end())
        list_.erase(find);
}


// lookup self error code
int Errors::Lookup(bool peek)
{
    Lock guard(mutex_);
    thr_iterator find = STL::find_if(list_.begin(), list_.end(),
                                       thr_match());
    if (find != list_.end()) {
        int ret = find->errorID_;
        if (!peek)
            list_.erase(find);
        return ret;
    }
    else
        return 0;
}


// add a new error code for self
void Errors::Add(int error)
{
    ThreadError add;
    add.errorID_  = error;
    add.threadID_ = GetSelf();

    Remove();   // may have old error

    Lock guard(mutex_);
    list_.push_back(add);
}


SSL_METHOD::SSL_METHOD(ConnectionEnd ce, ProtocolVersion pv, bool multiProto) 
    : version_(pv), side_(ce), verifyPeer_(false), verifyNone_(false),
      failNoCert_(false), multipleProtocol_(multiProto)
{}


ProtocolVersion SSL_METHOD::getVersion() const
{
    return version_;
}


ConnectionEnd SSL_METHOD::getSide() const
{
    return side_;
}


void SSL_METHOD::setVerifyPeer()
{
    verifyPeer_ = true;
}


void SSL_METHOD::setVerifyNone()
{
    verifyNone_ = true;
}


void SSL_METHOD::setFailNoCert()
{
    failNoCert_ = true;
}


bool SSL_METHOD::verifyPeer() const
{
    return verifyPeer_;
}


bool SSL_METHOD::verifyNone() const
{
    return verifyNone_;
}


bool SSL_METHOD::failNoCert() const
{
    return failNoCert_;
}


bool SSL_METHOD::multipleProtocol() const
{
    return multipleProtocol_;
}


SSL_CTX::SSL_CTX(SSL_METHOD* meth) 
    : method_(meth), certificate_(0), privateKey_(0), passwordCb_(0),
      userData_(0), sessionCacheOff_(false), sessionCacheFlushOff_(false),
      verifyCallback_(0)
{}


SSL_CTX::~SSL_CTX()
{
    ysDelete(method_);
    ysDelete(certificate_);
    ysDelete(privateKey_);

    STL::for_each(caList_.begin(), caList_.end(), del_ptr_zero());
}


void SSL_CTX::AddCA(x509* ca)
{
    caList_.push_back(ca);
}


const SSL_CTX::CertList& 
SSL_CTX::GetCA_List() const
{
    return caList_;
}


const VerifyCallback SSL_CTX::getVerifyCallback() const
{
    return verifyCallback_;
}


const x509* SSL_CTX::getCert() const
{
    return certificate_;
}


const x509* SSL_CTX::getKey() const
{
    return privateKey_;
}


const SSL_METHOD* SSL_CTX::getMethod() const
{
    return method_;
}


const Ciphers& SSL_CTX::GetCiphers() const
{
    return ciphers_;
}


const DH_Parms& SSL_CTX::GetDH_Parms() const
{
    return dhParms_;
}


const Stats& SSL_CTX::GetStats() const
{
    return stats_;
}


pem_password_cb SSL_CTX::GetPasswordCb() const
{
    return passwordCb_;
}


void SSL_CTX::SetPasswordCb(pem_password_cb cb)
{
    passwordCb_ = cb;
}


void* SSL_CTX::GetUserData() const
{
    return userData_;
}


bool SSL_CTX::GetSessionCacheOff() const
{
    return sessionCacheOff_;
}


bool SSL_CTX::GetSessionCacheFlushOff() const
{
    return sessionCacheFlushOff_;
}


void SSL_CTX::SetUserData(void* data)
{
    userData_ = data;
}


void SSL_CTX::SetSessionCacheOff()
{
    sessionCacheOff_ = true;
}


void SSL_CTX::SetSessionCacheFlushOff()
{
    sessionCacheFlushOff_ = true;
}


void SSL_CTX::setVerifyPeer()
{
    method_->setVerifyPeer();
}


void SSL_CTX::setVerifyNone()
{
    method_->setVerifyNone();
}


void SSL_CTX::setFailNoCert()
{
    method_->setFailNoCert();
}


void SSL_CTX::setVerifyCallback(VerifyCallback vc)
{
    verifyCallback_ = vc;
}


bool SSL_CTX::SetDH(const DH& dh)
{
    dhParms_.p_ = dh.p->int_;
    dhParms_.g_ = dh.g->int_;

    return dhParms_.set_ = true;
}


bool SSL_CTX::SetCipherList(const char* list)
{
    if (!list)
        return false;

    bool ret = false;
    char name[MAX_SUITE_NAME];

    char  needle[] = ":";
    char* haystack = const_cast<char*>(list);
    char* prev;

    const int suiteSz = sizeof(cipher_names) / sizeof(cipher_names[0]);
    int idx = 0;

    for(;;) {
        size_t len;
        prev = haystack;
        haystack = strstr(haystack, needle);

        if (!haystack)    // last cipher
            len = min(sizeof(name), strlen(prev));
        else
            len = min(sizeof(name), (size_t)(haystack - prev));

        strncpy(name, prev, len);
        name[(len == sizeof(name)) ? len - 1 : len] = 0;

        for (int i = 0; i < suiteSz; i++)
            if (strncmp(name, cipher_names[i], sizeof(name)) == 0) {

                ciphers_.suites_[idx++] = 0x00;  // first byte always zero
                ciphers_.suites_[idx++] = i;

                if (!ret) ret = true;   // found at least one
                break;
            }
        if (!haystack) break;
        haystack++;
    }

    if (ret) {
        ciphers_.setSuites_ = true;
        ciphers_.suiteSz_ = idx;
    }

    return ret;
}


void SSL_CTX::IncrementStats(StatsField fd)
{

    Lock guard(mutex_);
    
    switch (fd) {

	case Accept:
        ++stats_.accept_;
        break;

    case Connect:
        ++stats_.connect_;
        break;

    case AcceptGood:
        ++stats_.acceptGood_;
        break;

    case ConnectGood:
        ++stats_.connectGood_;
        break;

    case AcceptRenegotiate:
        ++stats_.acceptRenegotiate_;
        break;

    case ConnectRenegotiate:
        ++stats_.connectRenegotiate_;
        break;

    case Hits:
        ++stats_.hits_;
        break;

    case CbHits:
        ++stats_.cbHits_;
        break;

    case CacheFull:
        ++stats_.cacheFull_;
        break;

    case Misses:
        ++stats_.misses_;
        break;

    case Timeouts:
        ++stats_.timeouts_;
        break;

    case Number:
        ++stats_.number_;
        break;

    case GetCacheSize:
        ++stats_.getCacheSize_;
        break;

    case VerifyMode:
        ++stats_.verifyMode_;
        break;

    case VerifyDepth:
        ++stats_.verifyDepth_;
        break;

    default:
        break;
    }
}


Crypto::Crypto() 
    : digest_(0), cipher_(0), dh_(0) 
{}


Crypto::~Crypto()
{
    ysDelete(dh_);
    ysDelete(cipher_);
    ysDelete(digest_);
}


const Digest& Crypto::get_digest() const
{
    return *digest_;
}


const BulkCipher& Crypto::get_cipher() const
{
    return *cipher_;
}


const DiffieHellman& Crypto::get_dh() const
{
    return *dh_;
}


const RandomPool& Crypto::get_random() const
{
    return random_;
}


const CertManager& Crypto::get_certManager() const
{
    return cert_;
}


      
Digest& Crypto::use_digest()
{
    return *digest_;
}


BulkCipher& Crypto::use_cipher()
{
    return *cipher_;
}


DiffieHellman& Crypto::use_dh()
{
    return *dh_;
}


RandomPool& Crypto::use_random()
{
    return random_;
}


CertManager& Crypto::use_certManager()
{
    return cert_;
}



void Crypto::SetDH(DiffieHellman* dh)
{
    dh_ = dh;
}


void Crypto::SetDH(const DH_Parms& dh)
{
    if (dh.set_)
        dh_ = NEW_YS DiffieHellman(dh.p_, dh.g_, random_);
}


bool Crypto::DhSet()
{
    return dh_ != 0;
}


void Crypto::setDigest(Digest* digest)
{
    digest_ = digest;
}


void Crypto::setCipher(BulkCipher* c)
{
    cipher_ = c;
}


const MD5& sslHashes::get_MD5() const
{
    return md5HandShake_;
}


const SHA& sslHashes::get_SHA() const
{
    return shaHandShake_;
}


const Finished& sslHashes::get_verify() const
{
    return verify_;
}


const Hashes& sslHashes::get_certVerify() const
{
    return certVerify_;
}


MD5& sslHashes::use_MD5(){
    return md5HandShake_;
}


SHA& sslHashes::use_SHA()
{
    return shaHandShake_;
}


Finished& sslHashes::use_verify()
{
    return verify_;
}


Hashes& sslHashes::use_certVerify()
{
    return certVerify_;
}


Buffers::Buffers() : rawInput_(0)
{}


Buffers::~Buffers()
{
    STL::for_each(handShakeList_.begin(), handShakeList_.end(),
                  del_ptr_zero()) ;
    STL::for_each(dataList_.begin(), dataList_.end(),
                  del_ptr_zero()) ;
    ysDelete(rawInput_);
}


void Buffers::SetRawInput(input_buffer* ib)
{
    assert(rawInput_ == 0);
    rawInput_ = ib;
}


input_buffer* Buffers::TakeRawInput()
{
    input_buffer* ret = rawInput_;
    rawInput_ = 0;

    return ret;
}


const Buffers::inputList& Buffers::getData() const
{
    return dataList_;
}


const Buffers::outputList& Buffers::getHandShake() const
{
    return handShakeList_;
}


Buffers::inputList& Buffers::useData()
{
    return dataList_;
}


Buffers::outputList& Buffers::useHandShake()
{
    return handShakeList_;
}


Security::Security(ProtocolVersion pv, RandomPool& ran, ConnectionEnd ce,
                   const Ciphers& ciphers, SSL_CTX* ctx, bool haveDH)
   : conn_(pv, ran), parms_(ce, ciphers, pv, haveDH), resumeSession_(ran),
     ctx_(ctx), resuming_(false)
{}


const Connection& Security::get_connection() const
{
    return conn_;
}


const SSL_CTX* Security::GetContext() const
{
    return ctx_;
}


const Parameters& Security::get_parms() const
{
    return parms_;
}


const SSL_SESSION& Security::get_resume() const
{
    return resumeSession_;
}


bool Security::get_resuming() const
{
    return resuming_;
}


Connection& Security::use_connection()
{
    return conn_;
}


Parameters& Security::use_parms()
{
    return parms_;
}


SSL_SESSION& Security::use_resume()
{
    return resumeSession_;
}


void Security::set_resuming(bool b)
{
    resuming_ = b;
}


X509_NAME::X509_NAME(const char* n, size_t sz)
    : name_(0), sz_(sz)
{
    if (sz) {
        name_ = NEW_YS char[sz];
        memcpy(name_, n, sz);
    }
    entry_.data = 0;
}


X509_NAME::~X509_NAME()
{
    ysArrayDelete(name_);
    ysArrayDelete(entry_.data);
}


const char* X509_NAME::GetName() const
{
    return name_;
}


size_t X509_NAME::GetLength() const
{
    return sz_;
}


X509::X509(const char* i, size_t iSz, const char* s, size_t sSz,
           const char* b, int bSz, const char* a, int aSz)
    : issuer_(i, iSz), subject_(s, sSz),
      beforeDate_(b, bSz), afterDate_(a, aSz)
{}


X509_NAME* X509::GetIssuer()
{
    return &issuer_;
}


X509_NAME* X509::GetSubject()
{
    return &subject_;
}


ASN1_STRING* X509::GetBefore()
{
    return beforeDate_.GetString();
}


ASN1_STRING* X509::GetAfter()
{
    return afterDate_.GetString();
}


ASN1_STRING* X509_NAME::GetEntry(int i)
{
    if (i < 0 || i >= int(sz_))
        return 0;

    if (entry_.data)
        ysArrayDelete(entry_.data);
    entry_.data = NEW_YS byte[sz_];       // max size;

    memcpy(entry_.data, &name_[i], sz_ - i);
    if (entry_.data[sz_ -i - 1]) {
        entry_.data[sz_ - i] = 0;
        entry_.length = int(sz_) - i;
    }
    else
        entry_.length = int(sz_) - i - 1;
    entry_.type = 0;

    return &entry_;
}


StringHolder::StringHolder(const char* str, int sz)
{
    asnString_.length = sz;
    asnString_.data = NEW_YS byte[sz + 1];
    memcpy(asnString_.data, str, sz);
    asnString_.type = 0;  // not used for now
}


StringHolder::~StringHolder()
{
    ysArrayDelete(asnString_.data);
}


ASN1_STRING* StringHolder::GetString()
{
    return &asnString_;
}


#ifdef HAVE_LIBZ

    void* myAlloc(void* /* opaque */, unsigned int item, unsigned int size)
    {
        return NEW_YS unsigned char[item * size];
    }


    void myFree(void* /* opaque */, void* memory)
    {
        unsigned char* ptr = static_cast<unsigned char*>(memory);
        yaSSL::ysArrayDelete(ptr);
    }


    // put size in front of compressed data
    int Compress(const byte* in, int sz, input_buffer& buffer)
    {
        byte     tmp[LENGTH_SZ];
        z_stream c_stream; /* compression stream */

        buffer.allocate(sz + sizeof(uint16) + COMPRESS_EXTRA);

        c_stream.zalloc = myAlloc;
        c_stream.zfree  = myFree;
        c_stream.opaque = (voidpf)0;

        c_stream.next_in   = const_cast<byte*>(in);
        c_stream.avail_in  = sz;
        c_stream.next_out  = buffer.get_buffer() + sizeof(tmp);
        c_stream.avail_out = buffer.get_capacity() - sizeof(tmp);

        if (deflateInit(&c_stream, 8) != Z_OK) return -1;
        int err = deflate(&c_stream, Z_FINISH);
        deflateEnd(&c_stream);
        if (err != Z_OK && err != Z_STREAM_END) return -1;

        c16toa(sz, tmp);
        memcpy(buffer.get_buffer(), tmp, sizeof(tmp));
        buffer.add_size(c_stream.total_out + sizeof(tmp));

        return 0;
    }


    // get uncompressed size in front
    int DeCompress(input_buffer& in, int sz, input_buffer& out)
    {
        byte tmp[LENGTH_SZ];
    
        in.read(tmp, sizeof(tmp));

        uint16 len;
        ato16(tmp, len);

        out.allocate(len);

        z_stream d_stream; /* decompression stream */

        d_stream.zalloc = myAlloc;
        d_stream.zfree  = myFree;
        d_stream.opaque = (voidpf)0;

        d_stream.next_in   = in.get_buffer() + in.get_current();
        d_stream.avail_in  = sz - sizeof(tmp);
        d_stream.next_out  = out.get_buffer();
        d_stream.avail_out = out.get_capacity();

        if (inflateInit(&d_stream) != Z_OK) return -1;
        int err = inflate(&d_stream, Z_FINISH);
        inflateEnd(&d_stream);
        if (err != Z_OK && err != Z_STREAM_END) return -1;

        out.add_size(d_stream.total_out);
        in.set_current(in.get_current() + sz - sizeof(tmp));

        return 0;
    }


#else  // LIBZ

    // these versions should never get called
    int Compress(const byte* in, int sz, input_buffer& buffer)
    {
        assert(0);  
        return -1;
    } 


    int DeCompress(input_buffer& in, int sz, input_buffer& out)
    {
        assert(0);  
        return -1;
    } 


#endif // LIBZ


} // namespace



extern "C" void yaSSL_CleanUp()
{
    TaoCrypt::CleanUp();
    yaSSL::ysDelete(yaSSL::sslFactoryInstance);
    yaSSL::ysDelete(yaSSL::sessionsInstance);
    yaSSL::ysDelete(yaSSL::errorsInstance);

    // In case user calls more than once, prevent seg fault
    yaSSL::sslFactoryInstance = 0;
    yaSSL::sessionsInstance = 0;
    yaSSL::errorsInstance = 0;
}


#ifdef HAVE_EXPLICIT_TEMPLATE_INSTANTIATION
namespace mySTL {
template yaSSL::yassl_int_cpp_local1::SumData for_each<mySTL::list<yaSSL::input_buffer*>::iterator, yaSSL::yassl_int_cpp_local1::SumData>(mySTL::list<yaSSL::input_buffer*>::iterator, mySTL::list<yaSSL::input_buffer*>::iterator, yaSSL::yassl_int_cpp_local1::SumData);
template yaSSL::yassl_int_cpp_local1::SumBuffer for_each<mySTL::list<yaSSL::output_buffer*>::iterator, yaSSL::yassl_int_cpp_local1::SumBuffer>(mySTL::list<yaSSL::output_buffer*>::iterator, mySTL::list<yaSSL::output_buffer*>::iterator, yaSSL::yassl_int_cpp_local1::SumBuffer);
template mySTL::list<yaSSL::SSL_SESSION*>::iterator find_if<mySTL::list<yaSSL::SSL_SESSION*>::iterator, yaSSL::yassl_int_cpp_local2::sess_match>(mySTL::list<yaSSL::SSL_SESSION*>::iterator, mySTL::list<yaSSL::SSL_SESSION*>::iterator, yaSSL::yassl_int_cpp_local2::sess_match);
template mySTL::list<yaSSL::ThreadError>::iterator find_if<mySTL::list<yaSSL::ThreadError>::iterator, yaSSL::yassl_int_cpp_local2::thr_match>(mySTL::list<yaSSL::ThreadError>::iterator, mySTL::list<yaSSL::ThreadError>::iterator, yaSSL::yassl_int_cpp_local2::thr_match);
}
#endif

