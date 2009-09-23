/* arc4.cpp                                
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

/* based on Wei Dai's arc4.cpp from CryptoPP */

#include "runtime.hpp"
#include "arc4.hpp"


#if defined(TAOCRYPT_X86ASM_AVAILABLE) && defined(TAO_ASM)
    #define DO_ARC4_ASM
#endif


namespace TaoCrypt {

void ARC4::SetKey(const byte* key, word32 length)
{
    x_ = 1;
    y_ = 0;

    word32 i;

    for (i = 0; i < STATE_SIZE; i++)
        state_[i] = i;

    word32 keyIndex = 0, stateIndex = 0;

    for (i = 0; i < STATE_SIZE; i++) {
        word32 a = state_[i];
        stateIndex += key[keyIndex] + a;
        stateIndex &= 0xFF;
        state_[i] = state_[stateIndex];
        state_[stateIndex] = a;

        if (++keyIndex >= length)
            keyIndex = 0;
    }
}


// local
namespace {

inline unsigned int MakeByte(word32& x, word32& y, byte* s)
{
    word32 a = s[x];
    y = (y+a) & 0xff;

    word32 b = s[y];
    s[x] = b;
    s[y] = a;
    x = (x+1) & 0xff;

    return s[(a+b) & 0xff];
}

} // namespace



void ARC4::Process(byte* out, const byte* in, word32 length)
{
    if (length == 0) return;

#ifdef DO_ARC4_ASM
    if (isMMX) {
        AsmProcess(out, in, length);
        return;
    } 
#endif

    byte *const s = state_;
    word32 x = x_;
    word32 y = y_;

    if (in == out)
        while (length--)
            *out++ ^= MakeByte(x, y, s);
    else
        while(length--)
            *out++ = *in++ ^ MakeByte(x, y, s);
    x_ = x;
    y_ = y;
}



#ifdef DO_ARC4_ASM

#ifdef _MSC_VER
    __declspec(naked)
#else
    __attribute__ ((noinline))
#endif
void ARC4::AsmProcess(byte* out, const byte* in, word32 length)
{
#ifdef __GNUC__
    #define AS1(x)    asm(#x);
    #define AS2(x, y) asm(#x ", " #y);

    #define PROLOG()  \
        asm(".intel_syntax noprefix"); \
        AS2(    movd  mm3, edi                      )   \
        AS2(    movd  mm4, ebx                      )   \
        AS2(    movd  mm5, esi                      )   \
        AS2(    movd  mm6, ebp                      )   \
        AS2(    mov   ecx, DWORD PTR [ebp +  8]     )   \
        AS2(    mov   edi, DWORD PTR [ebp + 12]     )   \
        AS2(    mov   esi, DWORD PTR [ebp + 16]     )   \
        AS2(    mov   ebp, DWORD PTR [ebp + 20]     )

    #define EPILOG()  \
        AS2(    movd  ebp, mm6                  )   \
        AS2(    movd  esi, mm5                  )   \
        AS2(    movd  ebx, mm4                  )   \
        AS2(    mov   esp, ebp                  )   \
        AS2(    movd  edi, mm3                  )   \
        AS1(    emms                            )   \
        asm(".att_syntax");
#else
    #define AS1(x)    __asm x
    #define AS2(x, y) __asm x, y

    #define PROLOG() \
        AS1(    push  ebp                       )   \
        AS2(    mov   ebp, esp                  )   \
        AS2(    movd  mm3, edi                  )   \
        AS2(    movd  mm4, ebx                  )   \
        AS2(    movd  mm5, esi                  )   \
        AS2(    movd  mm6, ebp                  )   \
        AS2(    mov   edi, DWORD PTR [ebp +  8] )   \
        AS2(    mov   esi, DWORD PTR [ebp + 12] )   \
        AS2(    mov   ebp, DWORD PTR [ebp + 16] )

    #define EPILOG() \
        AS2(    movd  ebp, mm6                  )   \
        AS2(    movd  esi, mm5                  )   \
        AS2(    movd  ebx, mm4                  )   \
        AS2(    movd  edi, mm3                  )   \
        AS2(    mov   esp, ebp                  )   \
        AS1(    pop   ebp                       )   \
        AS1(    emms                            )   \
        AS1(    ret 12                          )
        
#endif

    PROLOG()

    AS2(    sub    esp, 4                   )   // make room 

    AS2(    cmp    ebp, 0                   )
    AS1(    jz     nothing                  )

    AS2(    mov    [esp], ebp               )   // length

    AS2(    movzx  edx, BYTE PTR [ecx + 1]  )   // y
    AS2(    lea    ebp, [ecx + 2]           )   // state_
    AS2(    movzx  ecx, BYTE PTR [ecx]      )   // x

    // setup loop
    // a = s[x];
    AS2(    movzx  eax, BYTE PTR [ebp + ecx]    )


AS1( begin:                             )

    // y = (y+a) & 0xff;
    AS2(    add    edx, eax                     )
    AS2(    and    edx, 255                     )

    // b = s[y];
    AS2(    movzx  ebx, BYTE PTR [ebp + edx]    )

    // s[x] = b;
    AS2(    mov    [ebp + ecx], bl              )

    // s[y] = a;
    AS2(    mov    [ebp + edx], al              )

    // x = (x+1) & 0xff;
    AS1(    inc    ecx                          )
    AS2(    and    ecx, 255                     )

    //return s[(a+b) & 0xff];
    AS2(    add    eax, ebx                     )
    AS2(    and    eax, 255                     )
    
    AS2(    movzx  ebx, BYTE PTR [ebp + eax]    )

    // a = s[x];   for next round
    AS2(    movzx  eax, BYTE PTR [ebp + ecx]    )

    // xOr w/ inByte
    AS2(    xor    bl,  BYTE PTR [esi]          )
    AS1(    inc    esi                          )

    // write to outByte
    AS2(    mov    [edi], bl                    )
    AS1(    inc    edi                          )

    AS1(    dec    DWORD PTR [esp]              )
    AS1(    jnz    begin                        )


    // write back to x_ and y_
    AS2(    mov    [ebp - 2], cl            )
    AS2(    mov    [ebp - 1], dl            )


AS1( nothing:                           )


    EPILOG()
}

#endif // DO_ARC4_ASM


}  // namespace
