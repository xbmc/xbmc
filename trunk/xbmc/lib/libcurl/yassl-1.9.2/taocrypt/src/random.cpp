/* random.cpp                                
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


/* random.cpp implements a crypto secure Random Number Generator using an OS
   specific seed, switch to /dev/random for more security but may block
*/

#include "runtime.hpp"
#include "random.hpp"
#include <string.h>
#include <time.h>

#if defined(_WIN32)
    #define _WIN32_WINNT 0x0400
    #include <windows.h>
    #include <wincrypt.h>
#else
    #include <errno.h>
    #include <fcntl.h>
    #include <unistd.h>
#endif // _WIN32

namespace TaoCrypt {


// Get seed and key cipher
RandomNumberGenerator::RandomNumberGenerator()
{
    byte key[32];
    byte junk[256];

    seed_.GenerateSeed(key, sizeof(key));
    cipher_.SetKey(key, sizeof(key));
    GenerateBlock(junk, sizeof(junk));  // rid initial state
}


// place a generated block in output
void RandomNumberGenerator::GenerateBlock(byte* output, word32 sz)
{
    memset(output, 0, sz);
    cipher_.Process(output, output, sz);
}


byte RandomNumberGenerator::GenerateByte()
{
    byte b;
    GenerateBlock(&b, 1);

    return b;
}


#if defined(_WIN32)

/* The OS_Seed implementation for windows */

OS_Seed::OS_Seed()
{
    if(!CryptAcquireContext(&handle_, 0, 0, PROV_RSA_FULL,
                             CRYPT_VERIFYCONTEXT))
        error_.SetError(WINCRYPT_E);
}


OS_Seed::~OS_Seed()
{
    CryptReleaseContext(handle_, 0);
}


void OS_Seed::GenerateSeed(byte* output, word32 sz)
{
    if (!CryptGenRandom(handle_, sz, output))
        error_.SetError(CRYPTGEN_E);
}


#elif defined(__NETWARE__)

/* The OS_Seed implementation for Netware */

#include <nks/thread.h>
#include <nks/plat.h>

// Loop on high resulution Read Time Stamp Counter
static void NetwareSeed(byte* output, word32 sz)
{
    word32 tscResult;

    for (word32 i = 0; i < sz; i += sizeof(tscResult)) {
        #if defined(__GNUC__)
            asm volatile("rdtsc" : "=A" (tscResult));
        #else
            #ifdef __MWERKS__
                asm {
            #else
                __asm {
            #endif
                    rdtsc
                    mov tscResult, eax
            }
        #endif

        memcpy(output, &tscResult, sizeof(tscResult));
        output += sizeof(tscResult);

        NXThreadYield();   // induce more variance
    }
}


OS_Seed::OS_Seed()
{
}


OS_Seed::~OS_Seed()
{
}


void OS_Seed::GenerateSeed(byte* output, word32 sz)
{
  /*
    Try to use NXSeedRandom as it will generate a strong
    seed using the onboard 82802 chip

    As it's not always supported, fallback to default
    implementation if an error is returned
  */

  if (NXSeedRandom(sz, output) != 0)
  {
    NetwareSeed(output, sz);
  }
}


#else

/* The default OS_Seed implementation */

OS_Seed::OS_Seed()
{
    fd_ = open("/dev/urandom",O_RDONLY);
    if (fd_ == -1) {
        fd_ = open("/dev/random",O_RDONLY);
        if (fd_ == -1)
            error_.SetError(OPEN_RAN_E);
    }
}


OS_Seed::~OS_Seed() 
{
    close(fd_);
}


// may block
void OS_Seed::GenerateSeed(byte* output, word32 sz)
{
    while (sz) {
        int len = read(fd_, output, sz);
        if (len == -1) {
            error_.SetError(READ_RAN_E);
            return;
        }

        sz     -= len;
        output += len;

        if (sz)
            sleep(1);
    }
}

#endif // _WIN32



} // namespace
