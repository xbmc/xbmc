/* yassl_error.cpp                                
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


/* yaSSL error implements and an exception class
 */

#include "runtime.hpp"
#include "yassl_error.hpp"
#include "error.hpp"        // TaoCrypt error numbers
#include "openssl/ssl.h"    // SSL_ERROR_WANT_READ
#include <string.h>         // strncpy

#ifdef _MSC_VER
    // 4996 warning to use MS extensions e.g., strcpy_s instead of strncpy
    #pragma warning(disable: 4996)
#endif

namespace yaSSL {


/* may bring back in future
Error::Error(const char* s, YasslError e, Library l) 
    : mySTL::runtime_error(s), error_(e), lib_(l) 
{
}


YasslError Error::get_number() const
{
    return error_;
}


Library Error::get_lib() const
{

    return lib_;
}
*/


void SetErrorString(YasslError error, char* buffer)
{
    using namespace TaoCrypt;
    const int max = MAX_ERROR_SZ;  // shorthand

    switch (error) {

        // yaSSL proper errors
    case range_error :
        strncpy(buffer, "buffer index error, out of range", max);
        break; 

    case realloc_error :
        strncpy(buffer, "trying to realloc a fixed buffer", max);
        break; 

    case factory_error : 
        strncpy(buffer, "unknown factory create request", max);
        break; 

    case unknown_cipher :
        strncpy(buffer, "trying to use an unknown cipher", max);
        break; 

    case prefix_error : 
        strncpy(buffer, "bad master secret derivation, prefix too big", max);
        break; 

    case record_layer : 
        strncpy(buffer, "record layer not ready yet", max);
        break; 
        
    case handshake_layer :
        strncpy(buffer, "handshake layer not ready yet", max);
        break; 

    case out_of_order :
        strncpy(buffer, "handshake message received in wrong order", max);
        break; 

    case bad_input : 
        strncpy(buffer, "bad cipher suite input", max);
        break; 

    case match_error :
        strncpy(buffer, "unable to match a supported cipher suite", max);
        break; 

    case no_key_file : 
        strncpy(buffer, "the server needs a private key file", max);
        break; 

    case verify_error :
        strncpy(buffer, "unable to verify peer checksum", max);
        break; 

    case send_error :
        strncpy(buffer, "socket layer send error", max);
        break; 

    case receive_error :
        strncpy(buffer, "socket layer receive error", max);
        break; 

    case certificate_error :
        strncpy(buffer, "unable to proccess cerificate", max);
        break;

    case privateKey_error :
        strncpy(buffer, "unable to proccess private key, bad format", max);
        break;

    case badVersion_error :
        strncpy(buffer, "protocl version mismatch", max);
        break;

    case compress_error :
        strncpy(buffer, "compression error", max);
        break;

    case decompress_error :
        strncpy(buffer, "decompression error", max);
        break;

    case pms_version_error :
        strncpy(buffer, "bad PreMasterSecret version error", max);
        break;

        // openssl errors
    case SSL_ERROR_WANT_READ :
        strncpy(buffer, "the read operation would block", max);
        break;

    case CERTFICATE_ERROR :
        strncpy(buffer, "Unable to verify certificate", max);
        break;

        // TaoCrypt errors
    case NO_ERROR_E :
        strncpy(buffer, "not in error state", max);
        break;

    case WINCRYPT_E :
        strncpy(buffer, "bad wincrypt acquire", max);
        break;

    case CRYPTGEN_E :
        strncpy(buffer, "CryptGenRandom error", max);
        break;

    case OPEN_RAN_E :
        strncpy(buffer, "unable to use random device", max);
        break;

    case READ_RAN_E :
        strncpy(buffer, "unable to use random device", max);
        break;

    case INTEGER_E :
        strncpy(buffer, "ASN: bad DER Integer Header", max);
        break;

    case SEQUENCE_E :
        strncpy(buffer, "ASN: bad Sequence Header", max);
        break;

    case SET_E :
        strncpy(buffer, "ASN: bad Set Header", max);
        break;

    case VERSION_E :
        strncpy(buffer, "ASN: version length not 1", max);
        break;

    case SIG_OID_E :
        strncpy(buffer, "ASN: signature OID mismatch", max);
        break;

    case BIT_STR_E :
        strncpy(buffer, "ASN: bad BitString Header", max);
        break;

    case UNKNOWN_OID_E :
        strncpy(buffer, "ASN: unknown key OID type", max);
        break;

    case OBJECT_ID_E :
        strncpy(buffer, "ASN: bad Ojbect ID Header", max);
        break;

    case TAG_NULL_E :
        strncpy(buffer, "ASN: expected TAG NULL", max);
        break;

    case EXPECT_0_E :
        strncpy(buffer, "ASN: expected 0", max);
        break;

    case OCTET_STR_E :
        strncpy(buffer, "ASN: bad Octet String Header", max);
        break;

    case TIME_E :
        strncpy(buffer, "ASN: bad TIME", max);
        break;

    case DATE_SZ_E :
        strncpy(buffer, "ASN: bad Date Size", max);
        break;

    case SIG_LEN_E :
        strncpy(buffer, "ASN: bad Signature Length", max);
        break;

    case UNKOWN_SIG_E :
        strncpy(buffer, "ASN: unknown signature OID", max);
        break;

    case UNKOWN_HASH_E :
        strncpy(buffer, "ASN: unknown hash OID", max);
        break;

    case DSA_SZ_E :
        strncpy(buffer, "ASN: bad DSA r or s size", max);
        break;

    case BEFORE_DATE_E :
        strncpy(buffer, "ASN: before date in the future", max);
        break;

    case AFTER_DATE_E :
        strncpy(buffer, "ASN: after date in the past", max);
        break;

    case SIG_CONFIRM_E :
        strncpy(buffer, "ASN: bad self signature confirmation", max);
        break;

    case SIG_OTHER_E :
        strncpy(buffer, "ASN: bad other signature confirmation", max);
        break;

    case CONTENT_E :
        strncpy(buffer, "bad content processing", max);
        break;

    case PEM_E :
        strncpy(buffer, "bad PEM format processing", max);
        break;

    default :
        strncpy(buffer, "unknown error number", max);
    }
}



}  // namespace yaSSL
