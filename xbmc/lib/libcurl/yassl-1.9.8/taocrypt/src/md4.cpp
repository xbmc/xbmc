/* md4.cpp                                
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


/* based on Wei Dai's md4.cpp from CryptoPP */

#include "runtime.hpp"
#include "md4.hpp"
#ifdef USE_SYS_STL
    #include <algorithm>
#else
    #include "algorithm.hpp"
#endif


namespace STL = STL_NAMESPACE;
   

namespace TaoCrypt {

void MD4::Init()
{
    digest_[0] = 0x67452301L;
    digest_[1] = 0xefcdab89L;
    digest_[2] = 0x98badcfeL;
    digest_[3] = 0x10325476L;

    buffLen_ = 0;
    loLen_  = 0;
    hiLen_  = 0;
}


MD4::MD4(const MD4& that) : HASHwithTransform(DIGEST_SIZE / sizeof(word32),
                                              BLOCK_SIZE) 
{ 
    buffLen_ = that.buffLen_;
    loLen_  =  that.loLen_;
    hiLen_  =  that.hiLen_;

    memcpy(digest_, that.digest_, DIGEST_SIZE);
    memcpy(buffer_, that.buffer_, BLOCK_SIZE);
}

MD4& MD4::operator= (const MD4& that)
{
    MD4 tmp(that);
    Swap(tmp);

    return *this;
}


void MD4::Swap(MD4& other)
{
    STL::swap(loLen_,   other.loLen_);
    STL::swap(hiLen_,   other.hiLen_);
    STL::swap(buffLen_, other.buffLen_);

    memcpy(digest_, other.digest_, DIGEST_SIZE);
    memcpy(buffer_, other.buffer_, BLOCK_SIZE);
}


void MD4::Transform()
{
#define F(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))
#define G(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))

    word32 A, B, C, D;

    A = digest_[0];
    B = digest_[1];
    C = digest_[2];
    D = digest_[3];

#define function(a,b,c,d,k,s) a=rotlFixed(a+F(b,c,d)+buffer_[k],s);
    function(A,B,C,D, 0, 3);
    function(D,A,B,C, 1, 7);
    function(C,D,A,B, 2,11);
    function(B,C,D,A, 3,19);
    function(A,B,C,D, 4, 3);
    function(D,A,B,C, 5, 7);
    function(C,D,A,B, 6,11);
    function(B,C,D,A, 7,19);
    function(A,B,C,D, 8, 3);
    function(D,A,B,C, 9, 7);
    function(C,D,A,B,10,11);
    function(B,C,D,A,11,19);
    function(A,B,C,D,12, 3);
    function(D,A,B,C,13, 7);
    function(C,D,A,B,14,11);
    function(B,C,D,A,15,19);

#undef function	  
#define function(a,b,c,d,k,s) a=rotlFixed(a+G(b,c,d)+buffer_[k]+0x5a827999,s);
    function(A,B,C,D, 0, 3);
    function(D,A,B,C, 4, 5);
    function(C,D,A,B, 8, 9);
    function(B,C,D,A,12,13);
    function(A,B,C,D, 1, 3);
    function(D,A,B,C, 5, 5);
    function(C,D,A,B, 9, 9);
    function(B,C,D,A,13,13);
    function(A,B,C,D, 2, 3);
    function(D,A,B,C, 6, 5);
    function(C,D,A,B,10, 9);
    function(B,C,D,A,14,13);
    function(A,B,C,D, 3, 3);
    function(D,A,B,C, 7, 5);
    function(C,D,A,B,11, 9);
    function(B,C,D,A,15,13);

#undef function	 
#define function(a,b,c,d,k,s) a=rotlFixed(a+H(b,c,d)+buffer_[k]+0x6ed9eba1,s);
    function(A,B,C,D, 0, 3);
    function(D,A,B,C, 8, 9);
    function(C,D,A,B, 4,11);
    function(B,C,D,A,12,15);
    function(A,B,C,D, 2, 3);
    function(D,A,B,C,10, 9);
    function(C,D,A,B, 6,11);
    function(B,C,D,A,14,15);
    function(A,B,C,D, 1, 3);
    function(D,A,B,C, 9, 9);
    function(C,D,A,B, 5,11);
    function(B,C,D,A,13,15);
    function(A,B,C,D, 3, 3);
    function(D,A,B,C,11, 9);
    function(C,D,A,B, 7,11);
    function(B,C,D,A,15,15);

    digest_[0] += A;
    digest_[1] += B;
    digest_[2] += C;
    digest_[3] += D;
}


} // namespace

