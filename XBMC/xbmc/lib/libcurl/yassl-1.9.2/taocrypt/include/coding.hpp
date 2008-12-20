/* coding.hpp                                
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

/* coding.hpp defines hex and base64 encoding/decoing
*/

#ifndef TAO_CRYPT_CODING_HPP
#define TAO_CRYPT_CODING_HPP

#include "misc.hpp"
#include "block.hpp"

namespace TaoCrypt {

class Source;


// Hex Encoding, see RFC 3548
class HexEncoder {
    ByteBlock encoded_;
    Source&     plain_;
public:
    explicit HexEncoder(Source& s) : plain_(s) { Encode(); }
private:
    void Encode();

    HexEncoder(const HexEncoder&);              // hide copy
    HexEncoder& operator=(const HexEncoder&);   // and assign
};


// Hex Decoding, see RFC 3548
class HexDecoder {
    ByteBlock decoded_;
    Source&     coded_;
public:
    explicit HexDecoder(Source& s) : coded_(s) { Decode(); }
private:
    void Decode();

    HexDecoder(const HexDecoder&);              // hide copy
    HexDecoder& operator=(const HexDecoder&);   // and assign
};


// Base 64 encoding, see RFC 3548
class Base64Encoder {
    ByteBlock encoded_;
    Source&     plain_;
public:
    explicit Base64Encoder(Source& s) : plain_(s) { Encode(); }
private:
    void Encode();

    Base64Encoder(const Base64Encoder&);              // hide copy
    Base64Encoder& operator=(const Base64Encoder&);   // and assign
};


// Base 64 decoding, see RFC 3548
class Base64Decoder {
    ByteBlock decoded_;
    Source&     coded_;
public:
    explicit Base64Decoder(Source& s) : coded_(s) { Decode(); }
private:
    void Decode();

    Base64Decoder(const Base64Decoder&);              // hide copy
    Base64Decoder& operator=(const Base64Decoder&);   // and assign
};


}  // namespace

#endif // TAO_CRYPT_CODING_HPP
