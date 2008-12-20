/* misc.hpp                                
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

/* based on Wei Dai's misc.h from CryptoPP */

#ifndef TAO_CRYPT_MISC_HPP
#define TAO_CRYPT_MISC_HPP


#if !defined(DO_TAOCRYPT_KERNEL_MODE)
    #include <stdlib.h>
    #include <assert.h>
    #include <string.h>
#else
    #include "kernelc.hpp"
#endif

#include "types.hpp"
#include "type_traits.hpp"



namespace TaoCrypt {


// Delete static singleton holders
void CleanUp();


#ifdef YASSL_PURE_C

    // library allocation
    struct new_t {};      // TaoCrypt New type
    extern new_t tc;      // pass in parameter

    } // namespace TaoCrypt

    void* operator new  (size_t, TaoCrypt::new_t);
    void* operator new[](size_t, TaoCrypt::new_t);

    void operator delete  (void*, TaoCrypt::new_t);
    void operator delete[](void*, TaoCrypt::new_t);


    namespace TaoCrypt {

    template<typename T>
    void tcDelete(T* ptr)
    {
        if (ptr) ptr->~T();
        ::operator delete(ptr, TaoCrypt::tc);
    }

    template<typename T>
    void tcArrayDelete(T* ptr)
    {
        // can't do array placement destruction since not tracking size in
        // allocation, only allow builtins to use array placement since they
        // don't need destructors called
        typedef char builtin[IsFundamentalType<T>::Yes ? 1 : -1];
        (void)sizeof(builtin);

        ::operator delete[](ptr, TaoCrypt::tc);
    }

    #define NEW_TC new (TaoCrypt::tc)


    // to resolve compiler generated operator delete on base classes with
    // virtual destructors (when on stack), make sure doesn't get called
    class virtual_base {
    public:
        static void operator delete(void*) { assert(0); }
    };

#else // YASSL_PURE_C


    template<typename T>
    void tcDelete(T* ptr)
    {
        delete ptr;
    }

    template<typename T>
    void tcArrayDelete(T* ptr)
    {
        delete[] ptr;
    }

    #define NEW_TC new

    class virtual_base {};
   
 
#endif // YASSL_PURE_C


#if defined(_MSC_VER) || defined(__BCPLUSPLUS__)
	#define INTEL_INTRINSICS
	#define FAST_ROTATE
#elif defined(__MWERKS__) && TARGET_CPU_PPC
	#define PPC_INTRINSICS
	#define FAST_ROTATE
#elif defined(__GNUC__) && defined(__i386__)
        // GCC does peephole optimizations which should result in using rotate
        // instructions
	#define FAST_ROTATE
#endif


// no gas on these systems ?, disable for now
#if defined(__sun__) || defined (__QNX__) || defined (__APPLE__)
    #define TAOCRYPT_DISABLE_X86ASM
#endif

// icc problem with -03 and integer, disable for now
#if defined(__INTEL_COMPILER)
    #define TAOCRYPT_DISABLE_X86ASM
#endif


// Turn on ia32 ASM for Big Integer
// CodeWarrior defines _MSC_VER
#if !defined(TAOCRYPT_DISABLE_X86ASM) && ((defined(_MSC_VER) && \
   !defined(__MWERKS__) && defined(_M_IX86)) || \
   (defined(__GNUC__) && defined(__i386__)))
    #define TAOCRYPT_X86ASM_AVAILABLE
#endif


#ifdef TAOCRYPT_X86ASM_AVAILABLE
    bool HaveCpuId();
    bool IsPentium();
    void CpuId(word32 input, word32 *output);

    extern bool isMMX;
#endif




// Turn on ia32 ASM for Ciphers and Message Digests
// Seperate define since these are more complex, use member offsets
// and user may want to turn off while leaving Big Integer optos on 
#if defined(TAOCRYPT_X86ASM_AVAILABLE) && !defined(DISABLE_TAO_ASM)
    #define TAO_ASM
#endif


//  Extra word in older vtable implementations, for ASM member offset
#if defined(__GNUC__) && __GNUC__ < 3
    #define OLD_GCC_OFFSET
#endif


#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#	define TAOCRYPT_MALLOC_ALIGNMENT_IS_16
#endif

#if defined(__linux__) || defined(__sun__) || defined(__CYGWIN__)
#	define TAOCRYPT_MEMALIGN_AVAILABLE
#endif


#if defined(_WIN32) || defined(__CYGWIN__)
    #define TAOCRYPT_WIN32_AVAILABLE
#endif

#if defined(__unix__) || defined(__MACH__)
    #define TAOCRYPT_UNIX_AVAILABLE
#endif


// VC60 workaround: it doesn't allow typename in some places
#if defined(_MSC_VER) && (_MSC_VER < 1300)
    #define CPP_TYPENAME
#else
    #define CPP_TYPENAME typename
#endif


#ifdef _MSC_VER
    #define TAOCRYPT_NO_VTABLE __declspec(novtable)
#else
    #define TAOCRYPT_NO_VTABLE
#endif


#ifdef USE_SYS_STL
    // use system STL
    #define STL_NAMESPACE       std
#else
    // use mySTL
    #define STL_NAMESPACE       mySTL
#endif


// ***************** DLL related ********************

#ifdef TAOCRYPT_WIN32_AVAILABLE

#ifdef TAOCRYPT_EXPORTS
    #define TAOCRYPT_IS_DLL
    #define TAOCRYPT_DLL __declspec(dllexport)
#elif defined(TAOCRYPT_IMPORTS)
    #define TAOCRYPT_IS_DLL
    #define TAOCRYPT_DLL __declspec(dllimport)
#else
    #define TAOCRYPT_DLL
#endif  // EXPORTS

#define TAOCRYPT_API __stdcall
#define TAOCRYPT_CDECL __cdecl

#else	// TAOCRYPT_WIN32_AVAILABLE

#define TAOCRYPT_DLL
#define TAOCRYPT_API
#define TAOCRYPT_CDECL

#endif	// TAOCRYPT_WIN32_AVAILABLE


// ****************** tempalte stuff *******************


#if defined(TAOCRYPT_MANUALLY_INSTANTIATE_TEMPLATES) && \
  !defined(TAOCRYPT_IMPORTS)
    #define TAOCRYPT_DLL_TEMPLATE_CLASS template class TAOCRYPT_DLL
#elif defined(__MWERKS__)
    #define TAOCRYPT_DLL_TEMPLATE_CLASS extern class TAOCRYPT_DLL
#else
    #define TAOCRYPT_DLL_TEMPLATE_CLASS extern template class TAOCRYPT_DLL
#endif


#if defined(TAOCRYPT_MANUALLY_INSTANTIATE_TEMPLATES) && \
  !defined(TAOCRYPT_EXPORTS)
    #define TAOCRYPT_STATIC_TEMPLATE_CLASS template class
#elif defined(__MWERKS__)
    #define TAOCRYPT_STATIC_TEMPLATE_CLASS extern class
#else
    #define TAOCRYPT_STATIC_TEMPLATE_CLASS extern template class
#endif


// ************** compile-time assertion ***************

template <bool b>
struct CompileAssert
{
	static char dummy[2*b-1];
};

#define TAOCRYPT_COMPILE_ASSERT(assertion) \
    TAOCRYPT_COMPILE_ASSERT_INSTANCE(assertion, __LINE__)

#if defined(TAOCRYPT_EXPORTS) || defined(TAOCRYPT_IMPORTS)
    #define TAOCRYPT_COMPILE_ASSERT_INSTANCE(assertion, instance)
#else
    #define TAOCRYPT_COMPILE_ASSERT_INSTANCE(assertion, instance) \
    (void)sizeof(CompileAssert<(assertion)>)
#endif

#define TAOCRYPT_ASSERT_JOIN(X, Y) TAOCRYPT_DO_ASSERT_JOIN(X, Y)

#define TAOCRYPT_DO_ASSERT_JOIN(X, Y) X##Y


/***************  helpers  *****************************/

inline unsigned int BitsToBytes(unsigned int bitCount)
{
    return ((bitCount+7)/(8));
}

inline unsigned int BytesToWords(unsigned int byteCount)
{
    return ((byteCount+WORD_SIZE-1)/WORD_SIZE);
}

inline unsigned int BitsToWords(unsigned int bitCount)
{
    return ((bitCount+WORD_BITS-1)/(WORD_BITS));
}

inline void CopyWords(word* r, const word* a, word32 n)
{
    for (word32 i = 0; i < n; i++)
        r[i] = a[i];
}

inline unsigned int CountWords(const word* X, unsigned int N)
{
    while (N && X[N-1]==0)
        N--;
    return N;
}

inline void SetWords(word* r, word a, unsigned int n)
{
    for (unsigned int i=0; i<n; i++)
        r[i] = a;
}

enum ByteOrder { LittleEndianOrder = 0, BigEndianOrder = 1 };
enum CipherDir {ENCRYPTION,	DECRYPTION};

inline CipherDir ReverseDir(CipherDir dir)
{
    return (dir == ENCRYPTION) ? DECRYPTION : ENCRYPTION;
}

template <typename ENUM_TYPE, int VALUE>
struct EnumToType
{
    static ENUM_TYPE ToEnum() { return (ENUM_TYPE)VALUE; }
};

typedef EnumToType<ByteOrder, LittleEndianOrder> LittleEndian;
typedef EnumToType<ByteOrder, BigEndianOrder>    BigEndian;


#ifndef BIG_ENDIAN_ORDER
    typedef LittleEndian HostByteOrder;
#else
    typedef BigEndian    HostByteOrder;
#endif

inline ByteOrder GetHostByteOrder()
{
    return HostByteOrder::ToEnum();
}

inline bool HostByteOrderIs(ByteOrder order)
{
    return order == GetHostByteOrder();
}


void xorbuf(byte*, const byte*, unsigned int);


template <class T>
inline bool IsPowerOf2(T n)
{
    return n > 0 && (n & (n-1)) == 0;
}

template <class T1, class T2>
inline T2 ModPowerOf2(T1 a, T2 b)
{
    assert(IsPowerOf2(b));
    return T2(a) & (b-1);
}

template <class T>
inline T RoundDownToMultipleOf(T n, T m)
{
    return n - (IsPowerOf2(m) ? ModPowerOf2(n, m) : (n%m));
}

template <class T>
inline T RoundUpToMultipleOf(T n, T m)
{
    return RoundDownToMultipleOf(n+m-1, m);
}

template <class T>
inline unsigned int GetAlignment(T* dummy = 0)	// VC60 workaround
{
#if (_MSC_VER >= 1300)
    return __alignof(T);
#elif defined(__GNUC__)
    return __alignof__(T);
#else
    return sizeof(T);
#endif
}

inline bool IsAlignedOn(const void* p, unsigned int alignment)
{
    return IsPowerOf2(alignment) ? ModPowerOf2((size_t)p, alignment) == 0
        : (size_t)p % alignment == 0;
}

template <class T>
inline bool IsAligned(const void* p, T* dummy = 0)	// VC60 workaround
{
    return IsAlignedOn(p, GetAlignment<T>());
}


template <class T> inline T rotlFixed(T x, unsigned int y)
{
    assert(y < sizeof(T)*8);
        return (x<<y) | (x>>(sizeof(T)*8-y));
}

template <class T> inline T rotrFixed(T x, unsigned int y)
{
    assert(y < sizeof(T)*8);
        return (x>>y) | (x<<(sizeof(T)*8-y));
}

#ifdef INTEL_INTRINSICS

#pragma intrinsic(_lrotl, _lrotr)

template<> inline word32 rotlFixed(word32 x, word32 y)
{
    assert(y < 32);
    return y ? _lrotl(x, y) : x;
}

template<> inline word32 rotrFixed(word32 x, word32 y)
{
    assert(y < 32);
    return y ? _lrotr(x, y) : x;
}

#endif // INTEL_INTRINSICS

#ifdef min
#undef min
#endif 


template <class T>
inline const T& min(const T& a, const T& b)
{
    return a < b ? a : b;
}


inline word32 ByteReverse(word32 value)
{
#ifdef PPC_INTRINSICS
    // PPC: load reverse indexed instruction
    return (word32)__lwbrx(&value,0);
#elif defined(FAST_ROTATE)
    // 5 instructions with rotate instruction, 9 without
    return (rotrFixed(value, 8U) & 0xff00ff00) |
           (rotlFixed(value, 8U) & 0x00ff00ff);
#else
    // 6 instructions with rotate instruction, 8 without
    value = ((value & 0xFF00FF00) >> 8) | ((value & 0x00FF00FF) << 8);
    return rotlFixed(value, 16U);
#endif
}


#ifdef WORD64_AVAILABLE

inline word64 ByteReverse(word64 value)
{
#ifdef TAOCRYPT_SLOW_WORD64
	return (word64(ByteReverse(word32(value))) << 32) | 
                   ByteReverse(word32(value>>32));
#else
	value = ((value & W64LIT(0xFF00FF00FF00FF00)) >> 8) |
            ((value & W64LIT(0x00FF00FF00FF00FF)) << 8);
	value = ((value & W64LIT(0xFFFF0000FFFF0000)) >> 16) |
            ((value & W64LIT(0x0000FFFF0000FFFF)) << 16);
	return rotlFixed(value, 32U);
#endif
}

#endif // WORD64_AVAILABLE


template <typename T>
inline void ByteReverse(T* out, const T* in, word32 byteCount)
{
    assert(byteCount % sizeof(T) == 0);
    word32 count = byteCount/sizeof(T);
    for (word32 i=0; i<count; i++)
        out[i] = ByteReverse(in[i]);
}

inline void ByteReverse(byte* out, const byte* in, word32 byteCount)
{
    word32* o       = reinterpret_cast<word32*>(out);
    const word32* i = reinterpret_cast<const word32*>(in);
    ByteReverse(o, i, byteCount);
}


template <class T>
inline T ByteReverseIf(T value, ByteOrder order)
{
    return HostByteOrderIs(order) ? value : ByteReverse(value);
}


template <typename T>
inline void ByteReverseIf(T* out, const T* in, word32 bc, ByteOrder order)
{
    if (!HostByteOrderIs(order)) 
        ByteReverse(out, in, bc);
    else if (out != in)
        memcpy(out, in, bc);
}



// do Asm Reverse is host is Little and x86asm 
#ifdef LITTLE_ENDIAN_ORDER
    #ifdef TAOCRYPT_X86ASM_AVAILABLE
        #define LittleReverse AsmReverse
    #else
        #define LittleReverse ByteReverse
    #endif
#else
    #define LittleReverse
#endif


// do Asm Reverse is host is Big and x86asm 
#ifdef BIG_ENDIAN_ORDER
    #ifdef TAOCRYPT_X86ASM_AVAILABLE
        #define BigReverse AsmReverse
    #else
        #define BigReverse ByteReverse
    #endif
#else
    #define BigReverse
#endif


#ifdef TAOCRYPT_X86ASM_AVAILABLE

    // faster than rotate, use bswap

    inline word32 AsmReverse(word32 wd)
    {
    #ifdef __GNUC__
        __asm__ 
        (
            "bswap %1"
            : "=r"(wd)
            : "0"(wd)
        );
    #else
        __asm 
        {
            mov   eax, wd
            bswap eax
            mov   wd, eax
        }
    #endif
        return wd;
    }

#endif 


template <class T>
inline void GetUserKey(ByteOrder order, T* out, word32 outlen, const byte* in,
                       word32 inlen)
{
    const unsigned int U = sizeof(T);
    assert(inlen <= outlen*U);
    memcpy(out, in, inlen);
    memset((byte *)out+inlen, 0, outlen*U-inlen);
    ByteReverseIf(out, out, RoundUpToMultipleOf(inlen, U), order);
}


#ifdef _MSC_VER
    // disable conversion warning
    // 4996 warning to use MS extensions e.g., strcpy_s instead of strncpy
    #pragma warning(disable:4244 4996)
#endif


inline byte UnalignedGetWordNonTemplate(ByteOrder order, const byte *block,
                                        byte*)
{
    return block[0];
}

inline word16 UnalignedGetWordNonTemplate(ByteOrder order, const byte* block,
                                          word16*)
{
    return (order == BigEndianOrder)
        ? block[1] | (block[0] << 8)
        : block[0] | (block[1] << 8);
}

inline word32 UnalignedGetWordNonTemplate(ByteOrder order, const byte* block,
                                          word32*)
{
    return (order == BigEndianOrder)
        ? word32(block[3]) | (word32(block[2]) << 8) | (word32(block[1]) << 16)
            | (word32(block[0]) << 24)
        : word32(block[0]) | (word32(block[1]) << 8) | (word32(block[2]) << 16)
            | (word32(block[3]) << 24);
}

template <class T>
inline T UnalignedGetWord(ByteOrder order, const byte *block, T* dummy = 0)
{
    return UnalignedGetWordNonTemplate(order, block, dummy);
}

inline void UnalignedPutWord(ByteOrder order, byte *block, byte value,
                             const byte *xorBlock = 0)
{
    block[0] = xorBlock ? (value ^ xorBlock[0]) : value;
}

#define GETBYTE(x, y) (unsigned int)byte((x)>>(8*(y)))

inline void UnalignedPutWord(ByteOrder order, byte *block, word16 value,
                             const byte *xorBlock = 0)
{
    if (order == BigEndianOrder)
    {
        block[0] = GETBYTE(value, 1);
        block[1] = GETBYTE(value, 0);
    }
    else
    {
        block[0] = GETBYTE(value, 0);
        block[1] = GETBYTE(value, 1);
    }

    if (xorBlock)
    {
        block[0] ^= xorBlock[0];
        block[1] ^= xorBlock[1];
    }
}

inline void UnalignedPutWord(ByteOrder order, byte* block, word32 value,
                             const byte* xorBlock = 0)
{
    if (order == BigEndianOrder)
    {
        block[0] = GETBYTE(value, 3);
        block[1] = GETBYTE(value, 2);
        block[2] = GETBYTE(value, 1);
        block[3] = GETBYTE(value, 0);
    }
    else
    {
        block[0] = GETBYTE(value, 0);
        block[1] = GETBYTE(value, 1);
        block[2] = GETBYTE(value, 2);
        block[3] = GETBYTE(value, 3);
    }

    if (xorBlock)
    {
        block[0] ^= xorBlock[0];
        block[1] ^= xorBlock[1];
        block[2] ^= xorBlock[2];
        block[3] ^= xorBlock[3];
    }
}


template <class T>
inline T GetWord(bool assumeAligned, ByteOrder order, const byte *block)
{
    if (assumeAligned)
    {
        assert(IsAligned<T>(block));
        return ByteReverseIf(*reinterpret_cast<const T *>(block), order);
    }
    else
        return UnalignedGetWord<T>(order, block);
}

template <class T>
inline void GetWord(bool assumeAligned, ByteOrder order, T &result,
                    const byte *block)
{
    result = GetWord<T>(assumeAligned, order, block);
}

template <class T>
inline void PutWord(bool assumeAligned, ByteOrder order, byte* block, T value,
                    const byte *xorBlock = 0)
{
    if (assumeAligned)
    {
        assert(IsAligned<T>(block));
        if (xorBlock)
            *reinterpret_cast<T *>(block) = ByteReverseIf(value, order) 
                ^ *reinterpret_cast<const T *>(xorBlock);
        else
            *reinterpret_cast<T *>(block) = ByteReverseIf(value, order);
    }
    else
        UnalignedPutWord(order, block, value, xorBlock);
}

template <class T, class B, bool A=true>
class GetBlock
{
public:
    GetBlock(const void *block)
        : m_block((const byte *)block) {}

    template <class U>
    inline GetBlock<T, B, A> & operator()(U &x)
    {
        TAOCRYPT_COMPILE_ASSERT(sizeof(U) >= sizeof(T));
        x = GetWord<T>(A, B::ToEnum(), m_block);
        m_block += sizeof(T);
        return *this;
    }

private:
    const byte *m_block;
};

template <class T, class B, bool A = true>
class PutBlock
{
public:
    PutBlock(const void *xorBlock, void *block)
        : m_xorBlock((const byte *)xorBlock), m_block((byte *)block) {}

    template <class U>
    inline PutBlock<T, B, A> & operator()(U x)
    {
        PutWord(A, B::ToEnum(), m_block, (T)x, m_xorBlock);
        m_block += sizeof(T);
        if (m_xorBlock)
            m_xorBlock += sizeof(T);
        return *this;
    }

private:
    const byte *m_xorBlock;
    byte *m_block;
};

template <class T, class B, bool A=true>
struct BlockGetAndPut
{
    // function needed because of C++ grammatical ambiguity between
    // expression-statements and declarations
    static inline GetBlock<T, B, A> Get(const void *block) 
        {return GetBlock<T, B, A>(block);}
    typedef PutBlock<T, B, A> Put;
};



template <bool overflow> struct SafeShifter;

template<> struct SafeShifter<true>
{
    template <class T>
    static inline T RightShift(T value, unsigned int bits)
    {
        return 0;
    }

    template <class T>
    static inline T LeftShift(T value, unsigned int bits)
    {
        return 0;
    }
};

template<> struct SafeShifter<false>
{
    template <class T>
    static inline T RightShift(T value, unsigned int bits)
    {
        return value >> bits;
    }

    template <class T>
    static inline T LeftShift(T value, unsigned int bits)
    {
        return value << bits;
    }
};

template <unsigned int bits, class T>
inline T SafeRightShift(T value)
{
    return SafeShifter<(bits>=(8*sizeof(T)))>::RightShift(value, bits);
}

template <unsigned int bits, class T>
inline T SafeLeftShift(T value)
{
    return SafeShifter<(bits>=(8*sizeof(T)))>::LeftShift(value, bits);
}


inline
word ShiftWordsLeftByBits(word* r, unsigned int n, unsigned int shiftBits)
{
    assert (shiftBits<WORD_BITS);
    word u, carry=0;
    if (shiftBits)
        for (unsigned int i=0; i<n; i++)
        {
            u = r[i];
            r[i] = (u << shiftBits) | carry;
            carry = u >> (WORD_BITS-shiftBits);
        }
    return carry;
}


inline
word ShiftWordsRightByBits(word* r, unsigned int n, unsigned int shiftBits)
{
    assert (shiftBits<WORD_BITS);
    word u, carry=0;
    if (shiftBits)
        for (int i=n-1; i>=0; i--)
        {
            u = r[i];
            r[i] = (u >> shiftBits) | carry;
            carry = u << (WORD_BITS-shiftBits);
        }
    return carry;
}


inline
void ShiftWordsLeftByWords(word* r, unsigned int n, unsigned int shiftWords)
{
    shiftWords = min(shiftWords, n);
    if (shiftWords)
    {
        for (unsigned int i=n-1; i>=shiftWords; i--)
            r[i] = r[i-shiftWords];
        SetWords(r, 0, shiftWords);
    }
}


inline
void ShiftWordsRightByWords(word* r, unsigned int n, unsigned int shiftWords)
{
    shiftWords = min(shiftWords, n);
    if (shiftWords)
    {
        for (unsigned int i=0; i+shiftWords<n; i++)
            r[i] = r[i+shiftWords];
        SetWords(r+n-shiftWords, 0, shiftWords);
    }
}


template <class T1, class T2>
inline T1 SaturatingSubtract(T1 a, T2 b)
{
    TAOCRYPT_COMPILE_ASSERT_INSTANCE(T1(-1)>0, 0);  // T1 is unsigned type
    TAOCRYPT_COMPILE_ASSERT_INSTANCE(T2(-1)>0, 1);  // T2 is unsigned type
    return T1((a > b) ? (a - b) : 0);
}


// declares
unsigned int  BytePrecision(word value);
unsigned int  BitPrecision(word);
word Crop(word value, unsigned int size);



} // namespace

#endif // TAO_CRYPT_MISC_HPP
