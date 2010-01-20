/* md2.cpp                                
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


/* based on Wei Dai's md2.cpp from CryptoPP */

#include "runtime.hpp"
#include "md2.hpp"
#include <string.h>

namespace TaoCrypt {


MD2::MD2()
    : X_(X_SIZE), C_(BLOCK_SIZE), buffer_(BLOCK_SIZE)
{
    Init();
}

void MD2::Init()
{
    memset(X_.get_buffer(), 0, X_SIZE);
    memset(C_.get_buffer(), 0, BLOCK_SIZE);
    memset(buffer_.get_buffer(), 0, BLOCK_SIZE);
    count_ = 0;
}


void MD2::Update(const byte* data, word32 len)
{

    static const byte S[256] = 
    {
        41, 46, 67, 201, 162, 216, 124, 1, 61, 54, 84, 161, 236, 240, 6,
        19, 98, 167, 5, 243, 192, 199, 115, 140, 152, 147, 43, 217, 188,
        76, 130, 202, 30, 155, 87, 60, 253, 212, 224, 22, 103, 66, 111, 24,
        138, 23, 229, 18, 190, 78, 196, 214, 218, 158, 222, 73, 160, 251,
        245, 142, 187, 47, 238, 122, 169, 104, 121, 145, 21, 178, 7, 63,
        148, 194, 16, 137, 11, 34, 95, 33, 128, 127, 93, 154, 90, 144, 50,
        39, 53, 62, 204, 231, 191, 247, 151, 3, 255, 25, 48, 179, 72, 165,
        181, 209, 215, 94, 146, 42, 172, 86, 170, 198, 79, 184, 56, 210,
        150, 164, 125, 182, 118, 252, 107, 226, 156, 116, 4, 241, 69, 157,
        112, 89, 100, 113, 135, 32, 134, 91, 207, 101, 230, 45, 168, 2, 27,
        96, 37, 173, 174, 176, 185, 246, 28, 70, 97, 105, 52, 64, 126, 15,
        85, 71, 163, 35, 221, 81, 175, 58, 195, 92, 249, 206, 186, 197,
        234, 38, 44, 83, 13, 110, 133, 40, 132, 9, 211, 223, 205, 244, 65,
        129, 77, 82, 106, 220, 55, 200, 108, 193, 171, 250, 36, 225, 123,
        8, 12, 189, 177, 74, 120, 136, 149, 139, 227, 99, 232, 109, 233,
        203, 213, 254, 59, 0, 29, 57, 242, 239, 183, 14, 102, 88, 208, 228,
        166, 119, 114, 248, 235, 117, 75, 10, 49, 68, 80, 180, 143, 237,
        31, 26, 219, 153, 141, 51, 159, 17, 131, 20
    };

    while (len) {
        word32 L = (PAD_SIZE - count_) < len ? (PAD_SIZE - count_) : len;
        memcpy(buffer_.get_buffer() + count_, data, L);
        count_ += L;
        data += L;
        len  -= L;

        if (count_==PAD_SIZE) {
            count_ = 0;
            memcpy(X_.get_buffer() + PAD_SIZE, buffer_.get_buffer(), PAD_SIZE);
            byte t = C_[15];

            int i;
            for(i = 0; i < PAD_SIZE; i++) {
                X_[32 + i] = X_[PAD_SIZE + i] ^ X_[i];
                t = C_[i] ^= S[buffer_[i] ^ t];
            }

            t=0;
            for(i = 0; i < 18; i++) {
                for(int j = 0; j < X_SIZE; j += 8) {
                    t = X_[j+0] ^= S[t];
                    t = X_[j+1] ^= S[t];
                    t = X_[j+2] ^= S[t];
                    t = X_[j+3] ^= S[t];
                    t = X_[j+4] ^= S[t];
                    t = X_[j+5] ^= S[t];
                    t = X_[j+6] ^= S[t];
                    t = X_[j+7] ^= S[t];
                }
                t = (t + i) & 0xFF;
            }
        }
    }
}


void MD2::Final(byte *hash)
{
    byte   padding[BLOCK_SIZE];
    word32 padLen = PAD_SIZE - count_;

    for (word32 i = 0; i < padLen; i++)
        padding[i] = static_cast<byte>(padLen);

    Update(padding, padLen);
    Update(C_.get_buffer(), BLOCK_SIZE);

    memcpy(hash, X_.get_buffer(), DIGEST_SIZE);

    Init();
}




} // namespace
