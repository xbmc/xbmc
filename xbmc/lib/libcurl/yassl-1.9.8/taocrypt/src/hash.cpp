/* hash.cpp                                
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

/* hash.cpp implements a base for digest types
*/

#include "runtime.hpp"
#include <string.h>
#include <assert.h>

#include "hash.hpp"


namespace TaoCrypt {


HASHwithTransform::HASHwithTransform(word32 digSz, word32 buffSz)
{
    assert(digSz  <= MaxDigestSz);
    assert(buffSz <= MaxBufferSz);
}


void HASHwithTransform::AddLength(word32 len)
{
    HashLengthType tmp = loLen_;
    if ( (loLen_ += len) < tmp)
        hiLen_++;                       // carry low to high
    hiLen_ += SafeRightShift<8*sizeof(HashLengthType)>(len);
}


// Update digest with data of size len, do in blocks
void HASHwithTransform::Update(const byte* data, word32 len)
{
    // do block size increments
    word32 blockSz = getBlockSize();
    byte*  local   = reinterpret_cast<byte*>(buffer_);

    while (len) {
        word32 add = min(len, blockSz - buffLen_);
        memcpy(&local[buffLen_], data, add);

        buffLen_ += add;
        data     += add;
        len      -= add;

        if (buffLen_ == blockSz) {
            ByteReverseIf(local, local, blockSz, getByteOrder());
            Transform();
            AddLength(blockSz);
            buffLen_ = 0;
        }
    }
}


// Final process, place digest in hash
void HASHwithTransform::Final(byte* hash)
{
    word32    blockSz  = getBlockSize();
    word32    digestSz = getDigestSize();
    word32    padSz    = getPadSize();
    ByteOrder order    = getByteOrder();

    AddLength(buffLen_);                        // before adding pads
    HashLengthType preLoLen = GetBitCountLo();
    HashLengthType preHiLen = GetBitCountHi();
    byte*     local         = reinterpret_cast<byte*>(buffer_);

    local[buffLen_++] = 0x80;  // add 1

    // pad with zeros
    if (buffLen_ > padSz) {
        memset(&local[buffLen_], 0, blockSz - buffLen_);
        buffLen_ += blockSz - buffLen_;

        ByteReverseIf(local, local, blockSz, order);
        Transform();
        buffLen_ = 0;
    }
    memset(&local[buffLen_], 0, padSz - buffLen_);
   
    ByteReverseIf(local, local, blockSz, order);
    
    memcpy(&local[padSz],   order ? &preHiLen : &preLoLen, sizeof(preLoLen));
    memcpy(&local[padSz+4], order ? &preLoLen : &preHiLen, sizeof(preLoLen));

    Transform();
    ByteReverseIf(digest_, digest_, digestSz, order);
    memcpy(hash, digest_, digestSz);

    Init();  // reset state
}


#ifdef WORD64_AVAILABLE

HASH64withTransform::HASH64withTransform(word32 digSz, word32 buffSz)
{
    assert(digSz  <= MaxDigestSz);
    assert(buffSz <= MaxBufferSz);
}


void HASH64withTransform::AddLength(word32 len)
{
    HashLengthType tmp = loLen_;
    if ( (loLen_ += len) < tmp)
        hiLen_++;                       // carry low to high
    hiLen_ += SafeRightShift<8*sizeof(HashLengthType)>(len);
}


// Update digest with data of size len, do in blocks
void HASH64withTransform::Update(const byte* data, word32 len)
{
    // do block size increments
    word32 blockSz = getBlockSize();
    byte*  local   = reinterpret_cast<byte*>(buffer_);

    while (len) {
        word32 add = min(len, blockSz - buffLen_);
        memcpy(&local[buffLen_], data, add);

        buffLen_ += add;
        data     += add;
        len      -= add;

        if (buffLen_ == blockSz) {
            ByteReverseIf(buffer_, buffer_, blockSz, getByteOrder());
            Transform();
            AddLength(blockSz);
            buffLen_ = 0;
        }
    }
}


// Final process, place digest in hash
void HASH64withTransform::Final(byte* hash)
{
    word32    blockSz  = getBlockSize();
    word32    digestSz = getDigestSize();
    word32    padSz    = getPadSize();
    ByteOrder order    = getByteOrder();

    AddLength(buffLen_);                        // before adding pads
    HashLengthType preLoLen = GetBitCountLo();
    HashLengthType preHiLen = GetBitCountHi();
    byte*     local         = reinterpret_cast<byte*>(buffer_);

    local[buffLen_++] = 0x80;  // add 1

    // pad with zeros
    if (buffLen_ > padSz) {
        memset(&local[buffLen_], 0, blockSz - buffLen_);
        buffLen_ += blockSz - buffLen_;

        ByteReverseIf(buffer_, buffer_, blockSz, order);
        Transform();
        buffLen_ = 0;
    }
    memset(&local[buffLen_], 0, padSz - buffLen_);
   
    ByteReverseIf(buffer_, buffer_, padSz, order);
    
    buffer_[blockSz / sizeof(word64) - 2] = order ? preHiLen : preLoLen;
    buffer_[blockSz / sizeof(word64) - 1] = order ? preLoLen : preHiLen;

    Transform();
    ByteReverseIf(digest_, digest_, digestSz, order);
    memcpy(hash, digest_, digestSz);

    Init();  // reset state
}

#endif // WORD64_AVAILABLE


} // namespace
