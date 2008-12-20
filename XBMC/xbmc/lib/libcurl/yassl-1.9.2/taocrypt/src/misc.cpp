/* misc.cpp                                
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

/* based on Wei Dai's misc.cpp from CryptoPP */


#include "runtime.hpp"
#include "misc.hpp"


#ifdef __GNUC__
    #include <signal.h>
    #include <setjmp.h>
#endif

#ifdef USE_SYS_STL
    #include <algorithm>
#else
    #include "algorithm.hpp"
#endif

namespace STL = STL_NAMESPACE;


#ifdef YASSL_PURE_C

    void* operator new(size_t sz, TaoCrypt::new_t)
    {
        void* ptr = malloc(sz ? sz : 1);
        if (!ptr) abort();

        return ptr;
    }


    void operator delete(void* ptr, TaoCrypt::new_t)
    {
        if (ptr) free(ptr);
    }


    void* operator new[](size_t sz, TaoCrypt::new_t nt)
    {
        return ::operator new(sz, nt);
    }


    void operator delete[](void* ptr, TaoCrypt::new_t nt)
    {
        ::operator delete(ptr, nt);
    }


    /* uncomment to test
    // make sure not using globals anywhere by forgetting to use overloaded
    void* operator new(size_t sz);

    void operator delete(void* ptr);

    void* operator new[](size_t sz);

    void operator delete[](void* ptr);
    */


    namespace TaoCrypt {

        new_t tc;   // for library new

    }

#if defined(__ICC) || defined(__INTEL_COMPILER)

extern "C" {

    int __cxa_pure_virtual() {
      assert("Pure virtual method called." == "Aborted");
      return 0;
    }

}  // extern "C"

#endif

#endif // YASSL_PURE_C


namespace TaoCrypt {


inline void XorWords(word* r, const word* a, unsigned int n)
{
    for (unsigned int i=0; i<n; i++)
        r[i] ^= a[i];
}


void xorbuf(byte* buf, const byte* mask, unsigned int count)
{
    if (((size_t)buf | (size_t)mask | count) % WORD_SIZE == 0)
        XorWords((word *)buf, (const word *)mask, count/WORD_SIZE);
    else
    {
        for (unsigned int i=0; i<count; i++)
            buf[i] ^= mask[i];
    }
}


unsigned int BytePrecision(word value)
{
    unsigned int i;
    for (i=sizeof(value); i; --i)
        if (value >> (i-1)*8)
            break;

    return i;
}


unsigned int BitPrecision(word value)
{
    if (!value)
        return 0;

    unsigned int l = 0,
                 h = 8 * sizeof(value);

    while (h-l > 1)
    {
        unsigned int t = (l+h)/2;
        if (value >> t)
            l = t;
        else
            h = t;
    }

    return h;
}


word Crop(word value, unsigned int size)
{
    if (size < 8*sizeof(value))
        return (value & ((1L << size) - 1));
    else
        return value;
}



#ifdef TAOCRYPT_X86ASM_AVAILABLE

#ifndef _MSC_VER
    static jmp_buf s_env;
    static void SigIllHandler(int)
    {
        longjmp(s_env, 1);
    }
#endif


bool HaveCpuId()
{
#ifdef _MSC_VER
    __try
    {
        __asm
        {
            mov eax, 0
            cpuid
        }            
    }
    __except (1)
    {
        return false;
    }
    return true;
#else
    word32 eax, ebx;
    __asm__ __volatile
    (
        /* Put EFLAGS in eax and ebx */
        "pushf;"
        "pushf;"
        "pop %0;"
        "movl %0,%1;"

        /* Flip the cpuid bit and store back in EFLAGS */
        "xorl $0x200000,%0;"
        "push %0;"
        "popf;"

        /* Read EFLAGS again */
        "pushf;"
        "pop %0;"
        "popf"
        : "=r" (eax), "=r" (ebx)
        :
        : "cc"
    );

    if (eax == ebx)
        return false;
    return true;
#endif
}


void CpuId(word32 input, word32 *output)
{
#ifdef __GNUC__
    __asm__
    (
        // save ebx in case -fPIC is being used
        "push %%ebx; cpuid; mov %%ebx, %%edi; pop %%ebx"
        : "=a" (output[0]), "=D" (output[1]), "=c" (output[2]), "=d"(output[3])
        : "a" (input)
    );
#else
    __asm
    {
        mov eax, input
        cpuid
        mov edi, output
        mov [edi], eax
        mov [edi+4], ebx
        mov [edi+8], ecx
        mov [edi+12], edx
    }
#endif
}


bool IsPentium()
{
    if (!HaveCpuId())
        return false;

    word32 cpuid[4];

    CpuId(0, cpuid);
    STL::swap(cpuid[2], cpuid[3]);
    if (memcmp(cpuid+1, "GenuineIntel", 12) != 0)
        return false;

    CpuId(1, cpuid);
    byte family = ((cpuid[0] >> 8) & 0xf);
    if (family < 5)
        return false;

    return true;
}



static bool IsMmx()
{
    if (!IsPentium())
        return false;

    word32 cpuid[4];

    CpuId(1, cpuid);
    if ((cpuid[3] & (1 << 23)) == 0)
        return false;

    return true;
}


bool isMMX = IsMmx();


#endif // TAOCRYPT_X86ASM_AVAILABLE




}  // namespace

