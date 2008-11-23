/* coding.cpp                                
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

/* coding.cpp implements hex and base64 encoding/decoing
*/

#include "runtime.hpp"
#include "coding.hpp"
#include "file.hpp"


namespace TaoCrypt {


namespace { // locals

const byte bad = 0xFF;  // invalid encoding

const byte hexEncode[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                           'A', 'B', 'C', 'D', 'E', 'F'
                         };

const byte hexDecode[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                           bad, bad, bad, bad, bad, bad, bad,
                           10, 11, 12, 13, 14, 15 
                         };  // A starts at 0x41 not 0x3A


const byte base64Encode[] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
                              'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
                              'U', 'V', 'W', 'X', 'Y', 'Z',
                              'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
                              'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
                              'u', 'v', 'w', 'x', 'y', 'z',
                              '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                              '+', '/'
                            };

const byte base64Decode[] = { 62, bad, bad, bad, 63,   // + starts at 0x2B
                              52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
                              bad, bad, bad, bad, bad, bad, bad,
                              0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                              10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
                              20, 21, 22, 23, 24, 25,
                              bad, bad, bad, bad, bad, bad,
                              26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
                              36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
                              46, 47, 48, 49, 50, 51
                            };

const byte pad = '=';
const int pemLineSz = 64;

}  // local namespace


// Hex Encode
void HexEncoder::Encode()
{
    word32 bytes = plain_.size();
    encoded_.New(bytes * 2);

    word32 i = 0;

    while (bytes--) {
        byte p = plain_.next();

        byte b  = p >> 4;
        byte b2 = p & 0xF;

        encoded_[i++] = hexEncode[b];
        encoded_[i++] = hexEncode[b2];
    }

    plain_.reset(encoded_);
}


// Hex Decode
void HexDecoder::Decode()
{
    word32 bytes = coded_.size();
    assert((bytes % 2) == 0);
    decoded_.New(bytes / 2);

    word32 i(0);

    while (bytes) {
        byte b  = coded_.next() - 0x30;  // 0 starts at 0x30
        byte b2 = coded_.next() - 0x30;

        // sanity checks
        assert( b  < sizeof(hexDecode)/sizeof(hexDecode[0]) );
        assert( b2 < sizeof(hexDecode)/sizeof(hexDecode[0]) );

        b  = hexDecode[b];
        b2 = hexDecode[b2];

        assert( b != bad && b2 != bad );
        
        decoded_[i++] = (b << 4) | b2;
        bytes -= 2;
    }

    coded_.reset(decoded_);
}


// Base 64 Encode
void Base64Encoder::Encode()
{
    word32 bytes = plain_.size();
    word32 outSz = (bytes + 3 - 1) / 3 * 4;

    outSz += (outSz + pemLineSz - 1) / pemLineSz;  // new lines
    encoded_.New(outSz);

    word32 i = 0;
    word32 j = 0;
    
    while (bytes > 2) {
        byte b1 = plain_.next();
        byte b2 = plain_.next();
        byte b3 = plain_.next();

        // encoded idx
        byte e1 = b1 >> 2;
        byte e2 = ((b1 & 0x3) << 4) | (b2 >> 4);
        byte e3 = ((b2 & 0xF) << 2) | (b3 >> 6);
        byte e4 = b3 & 0x3F;

        // store
        encoded_[i++] = base64Encode[e1];
        encoded_[i++] = base64Encode[e2];
        encoded_[i++] = base64Encode[e3];
        encoded_[i++] = base64Encode[e4];

        bytes -= 3;

        if ((++j % 16) == 0 && bytes)
            encoded_[i++] = '\n';
    }

    // last integral
    if (bytes) {
        bool twoBytes = (bytes == 2);

        byte b1 = plain_.next();
        byte b2 = (twoBytes) ? plain_.next() : 0;

        byte e1 = b1 >> 2;
        byte e2 = ((b1 & 0x3) << 4) | (b2 >> 4);
        byte e3 =  (b2 & 0xF) << 2;

        encoded_[i++] = base64Encode[e1];
        encoded_[i++] = base64Encode[e2];
        encoded_[i++] = (twoBytes) ? base64Encode[e3] : pad;
        encoded_[i++] = pad;
    } 

    encoded_[i++] = '\n';
    assert(i == outSz);

    plain_.reset(encoded_);
}


// Base 64 Decode
void Base64Decoder::Decode()
{
    word32 bytes = coded_.size();
    word32 plainSz = bytes - ((bytes + (pemLineSz - 1)) / pemLineSz); 
    plainSz = (plainSz * 3 + 3) / 4;
    decoded_.New(plainSz);

    word32 i = 0;
    word32 j = 0;

    while (bytes > 3) {
        byte e1 = coded_.next();
        byte e2 = coded_.next();
        byte e3 = coded_.next();
        byte e4 = coded_.next();

        // do asserts first
        if (e1 == 0)            // end file 0's
            break;

        bool pad3 = false;
        bool pad4 = false;
        if (e3 == pad)
            pad3 = true;
        if (e4 == pad)
            pad4 = true;

        e1 = base64Decode[e1 - 0x2B];
        e2 = base64Decode[e2 - 0x2B];
        e3 = (e3 == pad) ? 0 : base64Decode[e3 - 0x2B];
        e4 = (e4 == pad) ? 0 : base64Decode[e4 - 0x2B];

        byte b1 = (e1 << 2) | (e2 >> 4);
        byte b2 = ((e2 & 0xF) << 4) | (e3 >> 2);
        byte b3 = ((e3 & 0x3) << 6) | e4;

        decoded_[i++] = b1;
        if (!pad3)
            decoded_[i++] = b2;
        if (!pad4)
            decoded_[i++] = b3;
        else
            break;
        
        bytes -= 4;
        if ((++j % 16) == 0) {
            byte endLine = coded_.next();
            bytes--;
            while (endLine == ' ') {        // remove possible whitespace
                endLine = coded_.next();
                bytes--;
            }
            if (endLine == '\r') {
                endLine = coded_.next();
                bytes--;
            }
            if (endLine != '\n') {
                coded_.SetError(PEM_E); 
                return;
            }
        }
    }

    if (i != decoded_.size())
        decoded_.resize(i);
    coded_.reset(decoded_);
}


} // namespace
