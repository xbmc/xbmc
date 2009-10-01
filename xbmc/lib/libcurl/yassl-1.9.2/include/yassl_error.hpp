/* yassl_error.hpp                                
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


/* yaSSL error header defines error codes and an exception class
 */

#ifndef yaSSL_ERROR_HPP
#define yaSSL_ERROR_HPP



namespace yaSSL {


enum YasslError {
    no_error            = 0,

    // 10 - 47 from AlertDescription, 0 also close_notify

    range_error         = 101,
    realloc_error       = 102,
    factory_error       = 103,
    unknown_cipher      = 104,
    prefix_error        = 105,
    record_layer        = 106,
    handshake_layer     = 107,
    out_of_order        = 108,
    bad_input           = 109,
    match_error         = 110,
    no_key_file         = 111,
    verify_error        = 112,
    send_error          = 113,
    receive_error       = 114,
    certificate_error   = 115,
    privateKey_error    = 116,
    badVersion_error    = 117,
    compress_error      = 118,
    decompress_error    = 119,
    pms_version_error   = 120

    // !!!! add error message to .cpp !!!!

    // 1000+ from TaoCrypt error.hpp

};


enum Library { yaSSL_Lib = 0, CryptoLib, SocketLib };
enum { MAX_ERROR_SZ = 80 };

void SetErrorString(YasslError, char*);

/* remove for now, if go back to exceptions use this wrapper
// Base class for all yaSSL exceptions
class Error : public mySTL::runtime_error {
    YasslError  error_;
    Library     lib_;
public:
    explicit Error(const char* s = "", YasslError e = no_error,
                   Library l = yaSSL_Lib);

    YasslError  get_number() const;
    Library     get_lib()    const;
};
*/


} // naemspace

#endif // yaSSL_ERROR_HPP
