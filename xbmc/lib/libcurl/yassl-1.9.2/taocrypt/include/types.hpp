/* types.hpp                                
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

/* based on Wei Dai's misc.h from CryptoPP, basic crypt types */


#ifndef TAO_CRYPT_TYPES_HPP
#define TAO_CRYPT_TYPES_HPP

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

namespace TaoCrypt {


#if defined(WORDS_BIGENDIAN) || (defined(__MWERKS__) && !defined(__INTEL__))
    #define BIG_ENDIAN_ORDER
#endif

#ifndef BIG_ENDIAN_ORDER
    #define LITTLE_ENDIAN_ORDER
#endif


typedef unsigned char  byte;
typedef unsigned short word16;
typedef unsigned int   word32;

#if defined(_MSC_VER) || defined(__BCPLUSPLUS__)
    #define WORD64_AVAILABLE
    #define WORD64_IS_DISTINCT_TYPE
    typedef unsigned __int64 word64;
    #define W64LIT(x) x##ui64
#elif SIZEOF_LONG == 8
    #define WORD64_AVAILABLE
    typedef unsigned long word64;
    #define W64LIT(x) x##LL
#elif SIZEOF_LONG_LONG == 8 
    #define WORD64_AVAILABLE
    #define WORD64_IS_DISTINCT_TYPE
    typedef unsigned long long word64;
    #define W64LIT(x) x##LL
#endif


// compilers we've found 64-bit multiply insructions for
#if defined(__GNUC__) || defined(_MSC_VER) || defined(__DECCXX)
    #if !(defined(__ICC) || defined(__INTEL_COMPILER))
        #define HAVE_64_MULTIPLY
    #endif
#endif

    
#if defined(HAVE_64_MULTIPLY) && (defined(__alpha__) || defined(__ia64__) \
    || defined(_ARCH_PPC64) || defined(__mips64)  || defined(__x86_64__) \
    || defined(_M_X64) || defined(_M_IA64)) 
// These platforms have 64-bit CPU registers. Unfortunately most C++ compilers
// don't allow any way to access the 64-bit by 64-bit multiply instruction
// without using assembly, so in order to use word64 as word, the assembly
// instruction must be defined in Dword::Multiply().
    typedef word32 hword;
    typedef word64 word;
#else
    #define TAOCRYPT_NATIVE_DWORD_AVAILABLE
    #ifdef WORD64_AVAILABLE
        #define TAOCRYPT_SLOW_WORD64
        typedef word16 hword;
        typedef word32 word;
        typedef word64 dword;
    #else
        typedef byte   hword;
        typedef word16 word;
        typedef word32 dword;
    #endif
#endif

const word32 WORD_SIZE = sizeof(word);
const word32 WORD_BITS = WORD_SIZE * 8;


}  // namespace

#endif // TAO_CRYPT_TYPES_HPP
