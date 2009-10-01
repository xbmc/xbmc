/* error.hpp                                
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

/* error.hpp provides a taocrypt error numbers
 *
 */


#ifndef TAO_CRYPT_ERROR_HPP
#define TAO_CRYPT_ERROR_HPP


namespace TaoCrypt {


enum ErrorNumber {

NO_ERROR_E   =    0, // "not in error state"

// RandomNumberGenerator
WINCRYPT_E      = 1001, // "bad wincrypt acquire"
CRYPTGEN_E      = 1002, // "CryptGenRandom error"
OPEN_RAN_E      = 1003, // "open /dev/urandom error"
READ_RAN_E      = 1004, // "read /dev/urandom error"

// Integer
INTEGER_E       = 1010, // "bad DER Integer Header"


// ASN.1
SEQUENCE_E      = 1020, // "bad Sequence Header"
SET_E           = 1021, // "bad Set Header"
VERSION_E       = 1022, // "version length not 1"
SIG_OID_E       = 1023, // "signature OID mismatch"
BIT_STR_E       = 1024, // "bad BitString Header"
UNKNOWN_OID_E   = 1025, // "unknown key OID type"
OBJECT_ID_E     = 1026, // "bad Ojbect ID Header"
TAG_NULL_E      = 1027, // "expected TAG NULL"
EXPECT_0_E      = 1028, // "expected 0"
OCTET_STR_E     = 1029, // "bad Octet String Header"
TIME_E          = 1030, // "bad TIME"

DATE_SZ_E       = 1031, // "bad Date Size"
SIG_LEN_E       = 1032, // "bad Signature Length"
UNKOWN_SIG_E    = 1033, // "unknown signature OID"
UNKOWN_HASH_E   = 1034, // "unknown hash OID"
DSA_SZ_E        = 1035, // "bad DSA r or s size"
BEFORE_DATE_E   = 1036, // "before date in the future"
AFTER_DATE_E    = 1037, // "after date in the past"
SIG_CONFIRM_E   = 1038, // "bad self  signature confirmation"
SIG_OTHER_E     = 1039, // "bad other signature confirmation"

CONTENT_E       = 1040, // "bad content processing"
PEM_E           = 1041  // "bad pem format error"

    //  add error string to yassl/src/yassl_error.cpp !!! 
};


struct Error {
    ErrorNumber  what_;    // description number, 0 for no error

    explicit Error(ErrorNumber w = NO_ERROR_E) : what_(w) {}

    ErrorNumber What()            const  { return what_; }
    void        SetError(ErrorNumber w)  { what_ = w; }
};



} // namespace TaoCrypt

#endif // TAO_CRYPT_ERROR_HPP
