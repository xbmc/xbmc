/* random.hpp                                
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

/* random.hpp provides a crypto secure Random Number Generator using an OS
   specific seed
*/


#ifndef TAO_CRYPT_RANDOM_HPP
#define TAO_CRYPT_RANDOM_HPP

#include "arc4.hpp"
#include "error.hpp"

namespace TaoCrypt {


// OS specific seeder
class OS_Seed {
public:
    OS_Seed();
    ~OS_Seed();

    void   GenerateSeed(byte*, word32 sz);
    Error  GetError() const { return error_; }
private:
#if defined(_WIN32)
    #if defined(_WIN64)
        typedef unsigned __int64 ProviderHandle;
        // type HCRYPTPROV, avoid #include <windows.h>
    #else
        typedef unsigned long ProviderHandle;
    #endif
    ProviderHandle handle_;
#else
    int fd_;
#endif
    Error error_;

    OS_Seed(const OS_Seed&);              // hide copy
    OS_Seed& operator=(const OS_Seed&);   // hide assign
};


// secure Random Nnumber Generator
class RandomNumberGenerator {
public:
    RandomNumberGenerator();
    ~RandomNumberGenerator() {}

    void GenerateBlock(byte*, word32 sz);
    byte GenerateByte();

    ErrorNumber GetError() const { return seed_.GetError().What(); }
private:
    OS_Seed seed_;
    ARC4    cipher_;

    RandomNumberGenerator(const RandomNumberGenerator&);           // hide copy
    RandomNumberGenerator operator=(const RandomNumberGenerator&); // && assign
};




}  // namespace

#endif // TAO_CRYPT_RANDOM_HPP

