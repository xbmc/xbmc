/* yassl_types.hpp                                
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

/*  yaSSL types  header defines all constants, enums, and typedefs
 *  from the SSL.v3 specification "draft-freier-ssl-version3-02.txt"
 */


#ifndef yaSSL_TYPES_HPP
#define yaSSL_TYPES_HPP

#include <stddef.h>
#include <assert.h>
#include "type_traits.hpp"


#ifdef _MSC_VER
    // disable conversion warning
    // 4996 warning to use MS extensions e.g., strcpy_s instead of strncpy
    #pragma warning(disable:4244 4996)
#endif


namespace yaSSL {

#define YASSL_LIB


#ifdef YASSL_PURE_C

    // library allocation
    struct new_t {};      // yaSSL New type
    extern new_t ys;      // pass in parameter

    } // namespace yaSSL

    void* operator new  (size_t, yaSSL::new_t);
    void* operator new[](size_t, yaSSL::new_t);

    void operator delete  (void*, yaSSL::new_t);
    void operator delete[](void*, yaSSL::new_t);


    namespace yaSSL {


    template<typename T>
    void ysDelete(T* ptr)
    {
        if (ptr) ptr->~T();
        ::operator delete(ptr, yaSSL::ys);
    }

    template<typename T>
    void ysArrayDelete(T* ptr)
    {
        // can't do array placement destruction since not tracking size in
        // allocation, only allow builtins to use array placement since they
        // don't need destructors called
        typedef char builtin[TaoCrypt::IsFundamentalType<T>::Yes ? 1 : -1];
        (void)sizeof(builtin);

        ::operator delete[](ptr, yaSSL::ys);
    }

    #define NEW_YS new (yaSSL::ys)

    // to resolve compiler generated operator delete on base classes with
    // virtual destructors (when on stack), make sure doesn't get called
    class virtual_base {
    public:
        static void operator delete(void*) { assert(0); }
    };


#else   // YASSL_PURE_C

    template<typename T>
    void ysDelete(T* ptr)
    {
        delete ptr;
    }

    template<typename T>
    void ysArrayDelete(T* ptr)
    {
        delete[] ptr;
    }

    #define NEW_YS new

    class virtual_base {};



#endif // YASSL_PURE_C


typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;
typedef uint8          uint24[3];
typedef uint32         uint64[2];

typedef uint8  opaque;
typedef opaque byte;

typedef unsigned int uint;


#ifdef USE_SYS_STL
    // use system STL
    #define STL_VECTOR_FILE    <vector>
    #define STL_LIST_FILE      <list>
    #define STL_ALGORITHM_FILE <algorithm>
    #define STL_MEMORY_FILE    <memory>
    #define STL_PAIR_FILE      <utility>
    
    #define STL_NAMESPACE       std
#else
    // use mySTL
    #define STL_VECTOR_FILE    "vector.hpp"
    #define STL_LIST_FILE      "list.hpp"
    #define STL_ALGORITHM_FILE "algorithm.hpp"
    #define STL_MEMORY_FILE    "memory.hpp"
    #define STL_PAIR_FILE      "pair.hpp"

    #define STL_NAMESPACE       mySTL
#endif


#ifdef min
    #undef min
#endif 

template <typename T>
T min(T a, T b)
{
    return a < b ? a : b;
}


 
// all length constants in bytes
const int ID_LEN            =  32;  // session id length
const int SUITE_LEN         =   2;  // cipher suite length
const int SECRET_LEN        =  48;  // pre RSA and all master secret length
const int MASTER_ROUNDS     =   3;  // master secret derivation rounds
const int RAN_LEN           =  32;  // client and server random length
const int MAC_BLOCK_SZ      =  64;  // MAC block size, & padding
const int MD5_LEN           =  16;  // MD5 digest length
const int SHA_LEN           =  20;  // SHA digest length
const int RMD_LEN           =  20;  // RIPEMD-160 digest length
const int PREFIX            =   3;  // up to 3 prefix letters for secret rounds
const int KEY_PREFIX        =   7;  // up to 7 prefix letters for key rounds
const int FORTEZZA_MAX      = 128;  // Maximum Fortezza Key length
const int MAX_SUITE_SZ      = 128;  // 64 max suites * sizeof(suite)
const int MAX_SUITE_NAME    =  48;  // max length of suite name
const int MAX_CIPHERS       =  32;  // max supported ciphers for cipher list
const int SIZEOF_ENUM       =   1;  // SSL considers an enum 1 byte, not 4
const int SIZEOF_SENDER     =   4;  // Sender constant, for finished generation
const int PAD_MD5           =  48;  // pad length 1 and 2 for md5 finished
const int PAD_SHA           =  40;  // should be 44, specd wrong by netscape
const int PAD_RMD           =  44;  // pad length for RIPEMD-160, some use 40??
const int CERT_HEADER       =   3;  // always use 3 bytes for certificate
const int CERT_TYPES        =   7;  // certificate request types
const int REQUEST_HEADER    =   2;  // request uses 2 bytes
const int VERIFY_HEADER     =   2;  // verify length field
const int MIN_CERT_TYPES    =   1;  // minimum certificate request types
const int MIN_DIS_NAMES     =   3;  // minimum distinguished names
const int MIN_DIS_SIZE      =   1;  // minimum distinguished name size
const int RECORD_HEADER     =   5;  // type + version + length(2)
const int HANDSHAKE_HEADER  =   4;  // type + length(3)
const int FINISHED_SZ       = MD5_LEN + SHA_LEN; // sizeof finished data
const int TLS_FINISHED_SZ   =  12;  // TLS verify data size
const int SEQ_SZ            =   8;  // 64 bit sequence number
const int LENGTH_SZ         =   2;  // length field for HMAC, data only
const int VERSION_SZ        = SIZEOF_ENUM * 2;  // SSL/TLS length of version
const int DES_KEY_SZ        =   8;  // DES Key length
const int DES_EDE_KEY_SZ    =  24;  // DES EDE Key length
const int DES_BLOCK         =   8;  // DES is always fixed block size 8
const int DES_IV_SZ         = DES_BLOCK;    // Init Vector length for DES
const int RC4_KEY_SZ        =  16;  // RC4 Key length
const int AES_128_KEY_SZ    =  16;  // AES 128bit Key length
const int AES_192_KEY_SZ    =  24;  // AES 192bit Key length
const int AES_256_KEY_SZ    =  32;  // AES 256bit Key length
const int AES_BLOCK_SZ      =  16;  // AES 128bit block size, rfc 3268
const int AES_IV_SZ         = AES_BLOCK_SZ; // AES Init Vector length
const int DSS_SIG_SZ        =  40;  // two 20 byte high byte first Integers
const int DSS_ENCODED_EXTRA =   6;  // seqID + len(1) + (intID + len(1)) * 2
const int EVP_SALT_SZ       =   8;
const int MASTER_LABEL_SZ   =  13;  // TLS master secret label size
const int KEY_LABEL_SZ      =  13;  // TLS key block expansion size
const int FINISHED_LABEL_SZ =  15;  // TLS finished lable length
const int SEED_LEN          = RAN_LEN * 2; // TLS seed, client + server random
const int DEFAULT_TIMEOUT   = 500;  // Default Session timeout in seconds
const int MAX_RECORD_SIZE   = 16384; // 2^14, max size by standard
const int COMPRESS_EXTRA    = 1024;  // extra compression possible addition
const int SESSION_FLUSH_COUNT = 256;  // when to flush session cache


typedef uint8 Cipher;             // first byte is always 0x00 for SSLv3 & TLS

typedef opaque Random[RAN_LEN];

typedef opaque* DistinguishedName;

typedef bool IsExportable;


enum CompressionMethod { no_compression = 0, zlib = 221 };

enum CipherType { stream, block };

enum CipherChoice { change_cipher_spec_choice = 1 };

enum PublicValueEncoding { implicit_encoding, explicit_encoding };

enum ConnectionEnd { server_end, client_end };

enum AlertLevel { warning = 1, fatal = 2 };



// Record Layer Header identifier from page 12
enum ContentType {
    no_type            = 0,
    change_cipher_spec = 20, 
    alert              = 21, 
    handshake          = 22, 
    application_data   = 23 
};


// HandShake Layer Header identifier from page 20
enum HandShakeType {
    no_shake            = -1,
    hello_request       = 0, 
    client_hello        = 1, 
    server_hello        = 2,
    certificate         = 11, 
    server_key_exchange = 12,
    certificate_request = 13, 
    server_hello_done   = 14,
    certificate_verify  = 15, 
    client_key_exchange = 16,
    finished            = 20
};


// Valid Alert types from page 16/17
enum AlertDescription {
    close_notify            = 0,
    unexpected_message      = 10,
    bad_record_mac          = 20,
    decompression_failure   = 30,
    handshake_failure       = 40,
    no_certificate          = 41,
    bad_certificate         = 42,
    unsupported_certificate = 43,
    certificate_revoked     = 44,
    certificate_expired     = 45,
    certificate_unknown     = 46,
    illegal_parameter       = 47
};


// Supported Key Exchange Protocols
enum KeyExchangeAlgorithm { 
    no_kea = 0,
    rsa_kea, 
    diffie_hellman_kea, 
    fortezza_kea 
};


// Supported Authentication Schemes
enum SignatureAlgorithm { 
    anonymous_sa_algo = 0, 
    rsa_sa_algo, 
    dsa_sa_algo 
};


// Valid client certificate request types from page 27
enum ClientCertificateType {    
    rsa_sign            = 1, 
    dss_sign            = 2,
    rsa_fixed_dh        = 3,
    dss_fixed_dh        = 4,
    rsa_ephemeral_dh    = 5,
    dss_ephemeral_dh    = 6,
    fortezza_kea_cert   = 20
};


// Supported Ciphers from page 43
enum BulkCipherAlgorithm { 
    cipher_null,
    rc4,
    rc2,
    des,
    triple_des,             // leading 3 (3des) not valid identifier
    des40,
    idea,
    aes
};


// Supported Message Authentication Codes from page 43
enum MACAlgorithm { 
    no_mac,
    md5,
    sha,
    rmd
};


// Certificate file Type
enum CertType { Cert = 0, PrivateKey, CA };


// all Cipher Suites from pages 41/42
const Cipher SSL_NULL_WITH_NULL_NULL                =  0; // { 0x00, 0x00 }
const Cipher SSL_RSA_WITH_NULL_MD5                  =  1; // { 0x00, 0x01 }
const Cipher SSL_RSA_WITH_NULL_SHA                  =  2; // { 0x00, 0x02 }
const Cipher SSL_RSA_EXPORT_WITH_RC4_40_MD5         =  3; // { 0x00, 0x03 }
const Cipher SSL_RSA_WITH_RC4_128_MD5               =  4; // { 0x00, 0x04 }
const Cipher SSL_RSA_WITH_RC4_128_SHA               =  5; // { 0x00, 0x05 }
const Cipher SSL_RSA_EXPORT_WITH_RC2_CBC_40_MD5     =  6; // { 0x00, 0x06 }
const Cipher SSL_RSA_WITH_IDEA_CBC_SHA              =  7; // { 0x00, 0x07 }
const Cipher SSL_RSA_EXPORT_WITH_DES40_CBC_SHA      =  8; // { 0x00, 0x08 }
const Cipher SSL_RSA_WITH_DES_CBC_SHA               =  9; // { 0x00, 0x09 }
const Cipher SSL_RSA_WITH_3DES_EDE_CBC_SHA          = 10; // { 0x00, 0x0A }
const Cipher SSL_DH_DSS_EXPORT_WITH_DES40_CBC_SHA   = 11; // { 0x00, 0x0B }
const Cipher SSL_DH_DSS_WITH_DES_CBC_SHA            = 12; // { 0x00, 0x0C }
const Cipher SSL_DH_DSS_WITH_3DES_EDE_CBC_SHA       = 13; // { 0x00, 0x0D }
const Cipher SSL_DH_RSA_EXPORT_WITH_DES40_CBC_SHA   = 14; // { 0x00, 0x0E }
const Cipher SSL_DH_RSA_WITH_DES_CBC_SHA            = 15; // { 0x00, 0x0F }
const Cipher SSL_DH_RSA_WITH_3DES_EDE_CBC_SHA       = 16; // { 0x00, 0x10 }
const Cipher SSL_DHE_DSS_EXPORT_WITH_DES40_CBC_SHA  = 17; // { 0x00, 0x11 }
const Cipher SSL_DHE_DSS_WITH_DES_CBC_SHA           = 18; // { 0x00, 0x12 }
const Cipher SSL_DHE_DSS_WITH_3DES_EDE_CBC_SHA      = 19; // { 0x00, 0x13 }
const Cipher SSL_DHE_RSA_EXPORT_WITH_DES40_CBC_SHA  = 20; // { 0x00, 0x14 }
const Cipher SSL_DHE_RSA_WITH_DES_CBC_SHA           = 21; // { 0x00, 0x15 }
const Cipher SSL_DHE_RSA_WITH_3DES_EDE_CBC_SHA      = 22; // { 0x00, 0x16 }
const Cipher SSL_DH_anon_EXPORT_WITH_RC4_40_MD5     = 23; // { 0x00, 0x17 }
const Cipher SSL_DH_anon_WITH_RC4_128_MD5           = 24; // { 0x00, 0x18 }
const Cipher SSL_DH_anon_EXPORT_WITH_DES40_CBC_SHA  = 25; // { 0x00, 0x19 }
const Cipher SSL_DH_anon_WITH_DES_CBC_SHA           = 26; // { 0x00, 0x1A }
const Cipher SSL_DH_anon_WITH_3DES_EDE_CBC_SHA      = 27; // { 0x00, 0x1B }
const Cipher SSL_FORTEZZA_KEA_WITH_NULL_SHA         = 28; // { 0x00, 0x1C }
const Cipher SSL_FORTEZZA_KEA_WITH_FORTEZZA_CBC_SHA = 29; // { 0x00, 0x1D }
const Cipher SSL_FORTEZZA_KEA_WITH_RC4_128_SHA      = 30; // { 0x00, 0x1E }

// .. to 0x2B uses Kerberos Authentication


// TLS AES extensions
const Cipher TLS_RSA_WITH_AES_128_CBC_SHA      = 47; // { 0x00, 0x2F }
const Cipher TLS_DH_DSS_WITH_AES_128_CBC_SHA   = 48; // { 0x00, 0x30 }
const Cipher TLS_DH_RSA_WITH_AES_128_CBC_SHA   = 49; // { 0x00, 0x31 }
const Cipher TLS_DHE_DSS_WITH_AES_128_CBC_SHA  = 50; // { 0x00, 0x32 }
const Cipher TLS_DHE_RSA_WITH_AES_128_CBC_SHA  = 51; // { 0x00, 0x33 }
const Cipher TLS_DH_anon_WITH_AES_128_CBC_SHA  = 52; // { 0x00, 0x34 }

const Cipher TLS_RSA_WITH_AES_256_CBC_SHA      = 53; // { 0x00, 0x35 }
const Cipher TLS_DH_DSS_WITH_AES_256_CBC_SHA   = 54; // { 0x00, 0x36 }
const Cipher TLS_DH_RSA_WITH_AES_256_CBC_SHA   = 55; // { 0x00, 0x37 }
const Cipher TLS_DHE_DSS_WITH_AES_256_CBC_SHA  = 56; // { 0x00, 0x38 }
const Cipher TLS_DHE_RSA_WITH_AES_256_CBC_SHA  = 57; // { 0x00, 0x39 }
const Cipher TLS_DH_anon_WITH_AES_256_CBC_SHA  = 58; // { 0x00, 0x3A }


// OpenPGP extensions

const Cipher TLS_DHE_DSS_WITH_3DES_EDE_CBC_RMD160 = 114; // { 0x00, 0x72 };
const Cipher TLS_DHE_DSS_WITH_AES_128_CBC_RMD160  = 115; // { 0x00, 0x73 };
const Cipher TLS_DHE_DSS_WITH_AES_256_CBC_RMD160  = 116; // { 0x00, 0x74 };
const Cipher TLS_DHE_RSA_WITH_3DES_EDE_CBC_RMD160 = 119; // { 0x00, 0x77 };
const Cipher TLS_DHE_RSA_WITH_AES_128_CBC_RMD160  = 120; // { 0x00, 0x78 };
const Cipher TLS_DHE_RSA_WITH_AES_256_CBC_RMD160  = 121; // { 0x00, 0x79 };
const Cipher TLS_RSA_WITH_3DES_EDE_CBC_RMD160     = 124; // { 0x00, 0x7C };
const Cipher TLS_RSA_WITH_AES_128_CBC_RMD160      = 125; // { 0x00, 0x7D };
const Cipher TLS_RSA_WITH_AES_256_CBC_RMD160      = 126; // { 0x00, 0x7E };


const char* const null_str = "";

const char* const cipher_names[128] =
{
    null_str, // SSL_NULL_WITH_NULL_NULL                =  0
    null_str, // SSL_RSA_WITH_NULL_MD5                  =  1
    null_str, // SSL_RSA_WITH_NULL_SHA                  =  2
    null_str, // SSL_RSA_EXPORT_WITH_RC4_40_MD5         =  3
    "RC4-MD5",  // SSL_RSA_WITH_RC4_128_MD5               =  4
    "RC4-SHA",  // SSL_RSA_WITH_RC4_128_SHA               =  5
    null_str, // SSL_RSA_EXPORT_WITH_RC2_CBC_40_MD5     =  6
    null_str, // SSL_RSA_WITH_IDEA_CBC_SHA              =  7
    null_str, // SSL_RSA_EXPORT_WITH_DES40_CBC_SHA      =  8
    "DES-CBC-SHA",  // SSL_RSA_WITH_DES_CBC_SHA               =  9
    "DES-CBC3-SHA", // SSL_RSA_WITH_3DES_EDE_CBC_SHA          = 10

    null_str, // SSL_DH_DSS_EXPORT_WITH_DES40_CBC_SHA   = 11
    null_str, // SSL_DH_DSS_WITH_DES_CBC_SHA            = 12
    null_str, // SSL_DH_DSS_WITH_3DES_EDE_CBC_SHA       = 13
    null_str, // SSL_DH_RSA_EXPORT_WITH_DES40_CBC_SHA   = 14
    null_str, // SSL_DH_RSA_WITH_DES_CBC_SHA            = 15
    null_str, // SSL_DH_RSA_WITH_3DES_EDE_CBC_SHA       = 16
    null_str, // SSL_DHE_DSS_EXPORT_WITH_DES40_CBC_SHA  = 17
    "EDH-DSS-DES-CBC-SHA",  // SSL_DHE_DSS_WITH_DES_CBC_SHA           = 18
    "EDH-DSS-DES-CBC3-SHA", // SSL_DHE_DSS_WITH_3DES_EDE_CBC_SHA      = 19
    null_str, // SSL_DHE_RSA_EXPORT_WITH_DES40_CBC_SHA  = 20

    "EDH-RSA-DES-CBC-SHA",  // SSL_DHE_RSA_WITH_DES_CBC_SHA           = 21
    "EDH-RSA-DES-CBC3-SHA", // SSL_DHE_RSA_WITH_3DES_EDE_CBC_SHA      = 22
    null_str, // SSL_DH_anon_EXPORT_WITH_RC4_40_MD5     = 23
    null_str, // SSL_DH_anon_WITH_RC4_128_MD5           = 24
    null_str, // SSL_DH_anon_EXPORT_WITH_DES40_CBC_SHA  = 25
    null_str, // SSL_DH_anon_WITH_DES_CBC_SHA           = 26
    null_str, // SSL_DH_anon_WITH_3DES_EDE_CBC_SHA      = 27
    null_str, // SSL_FORTEZZA_KEA_WITH_NULL_SHA         = 28
    null_str, // SSL_FORTEZZA_KEA_WITH_FORTEZZA_CBC_SHA = 29
    null_str, // SSL_FORTEZZA_KEA_WITH_RC4_128_SHA      = 30

    null_str, null_str, null_str, null_str, null_str, // 31 - 35
    null_str, null_str, null_str, null_str, null_str, // 36 - 40
    null_str, null_str, null_str, null_str, null_str, // 41 - 45
    null_str, // 46

    // TLS AES extensions
    "AES128-SHA", // TLS_RSA_WITH_AES_128_CBC_SHA      = 47
    null_str, // TLS_DH_DSS_WITH_AES_128_CBC_SHA   = 48
    null_str, // TLS_DH_RSA_WITH_AES_128_CBC_SHA   = 49
    "DHE-DSS-AES128-SHA", // TLS_DHE_DSS_WITH_AES_128_CBC_SHA  = 50
    "DHE-RSA-AES128-SHA", // TLS_DHE_RSA_WITH_AES_128_CBC_SHA  = 51
    null_str, // TLS_DH_anon_WITH_AES_128_CBC_SHA  = 52

    "AES256-SHA", // TLS_RSA_WITH_AES_256_CBC_SHA      = 53
    null_str, // TLS_DH_DSS_WITH_AES_256_CBC_SHA   = 54
    null_str, // TLS_DH_RSA_WITH_AES_256_CBC_SHA   = 55
    "DHE-DSS-AES256-SHA", // TLS_DHE_DSS_WITH_AES_256_CBC_SHA  = 56
    "DHE-RSA-AES256-SHA", // TLS_DHE_RSA_WITH_AES_256_CBC_SHA  = 57
    null_str, // TLS_DH_anon_WITH_AES_256_CBC_SHA  = 58
    
    null_str, // 59
    null_str, // 60
    null_str, null_str, null_str, null_str, null_str, // 61 - 65
    null_str, null_str, null_str, null_str, null_str, // 66 - 70
    null_str, null_str, null_str, null_str, null_str, // 71 - 75
    null_str, null_str, null_str, null_str, null_str, // 76 - 80
    null_str, null_str, null_str, null_str, null_str, // 81 - 85
    null_str, null_str, null_str, null_str, null_str, // 86 - 90
    null_str, null_str, null_str, null_str, null_str, // 91 - 95
    null_str, null_str, null_str, null_str, null_str, // 96 - 100
    null_str, null_str, null_str, null_str, null_str, // 101 - 105
    null_str, null_str, null_str, null_str, null_str, // 106 - 110
    null_str, null_str, null_str,                     // 111 - 113

    "DHE-DSS-DES-CBC3-RMD", //  TLS_DHE_DSS_WITH_3DES_EDE_CBC_RMD160 = 114
    "DHE-DSS-AES128-RMD",   //  TLS_DHE_DSS_WITH_AES_128_CBC_RMD160  = 115
    "DHE-DSS-AES256-RMD",   //  TLS_DHE_DSS_WITH_AES_256_CBC_RMD160  = 116
    null_str, // 117
    null_str, // 118
    "DHE-RSA-DES-CBC3-RMD", //  TLS_DHE_RSA_WITH_3DES_EDE_CBC_RMD160 = 119
    "DHE-RSA-AES128-RMD",   //  TLS_DHE_RSA_WITH_AES_128_CBC_RMD160  = 120
    "DHE-RSA-AES256-RMD",   //  TLS_DHE_RSA_WITH_AES_256_CBC_RMD160  = 121
    null_str, // 122
    null_str, // 123
    "DES-CBC3-RMD", //  TLS_RSA_WITH_3DES_EDE_CBC_RMD160     = 124
    "AES128-RMD",   //  TLS_RSA_WITH_AES_128_CBC_RMD160      = 125
    "AES256-RMD",   //  TLS_RSA_WITH_AES_256_CBC_RMD160      = 126
    null_str // 127
};

// fill with MD5 pad size since biggest required
const opaque PAD1[PAD_MD5] =  { 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
                                0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
                                0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
                                0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
                                0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
                                0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36
                              };
const opaque PAD2[PAD_MD5] =  { 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
                                0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
                                0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
                                0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
                                0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
                                0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c
                              };

const opaque client[SIZEOF_SENDER] = { 0x43, 0x4C, 0x4E, 0x54 };
const opaque server[SIZEOF_SENDER] = { 0x53, 0x52, 0x56, 0x52 };

const opaque tls_client[FINISHED_LABEL_SZ + 1] = "client finished";
const opaque tls_server[FINISHED_LABEL_SZ + 1] = "server finished";

const opaque master_label[MASTER_LABEL_SZ + 1] = "master secret";
const opaque key_label   [KEY_LABEL_SZ + 1]    = "key expansion";


} // naemspace

#if __GNUC__ == 2 && __GNUC_MINOR__ <= 96
/*
  gcc 2.96 bails out because of two declarations of byte: yaSSL::byte and
  TaoCrypt::byte. TODO: define global types.hpp and move the declaration of
  'byte' there.
*/
using yaSSL::byte;
#endif


#endif // yaSSL_TYPES_HPP
