/* des.hpp                                
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

/* des.hpp defines DES, DES_EDE2, and DES_EDE3
   see FIPS 46-2 and FIPS 81
*/


#ifndef TAO_CRYPT_DES_HPP
#define TAO_CRYPT_DES_HPP

#include "misc.hpp"
#include "modes.hpp"


#if defined(TAOCRYPT_X86ASM_AVAILABLE) && defined(TAO_ASM)
    #define DO_DES_ASM
#endif


namespace TaoCrypt {


enum { DES_BLOCK_SIZE = 8, DES_KEY_SIZE = 32 };


class BasicDES {
public:
    void SetKey(const byte*, word32, CipherDir dir);
    void RawProcessBlock(word32&, word32&) const;
protected:
    word32 k_[DES_KEY_SIZE];
};


// DES 
class DES : public Mode_BASE, public BasicDES {
public:
    DES(CipherDir DIR, Mode MODE) 
        : Mode_BASE(DES_BLOCK_SIZE, DIR, MODE) {}

private:
    void ProcessAndXorBlock(const byte*, const byte*, byte*) const;

    DES(const DES&);              // hide copy
    DES& operator=(const DES&);   // and assign
};


// DES_EDE2
class DES_EDE2 : public Mode_BASE {
public:
    DES_EDE2(CipherDir DIR, Mode MODE) 
        : Mode_BASE(DES_BLOCK_SIZE, DIR, MODE) {}

    void SetKey(const byte*, word32, CipherDir dir);
private:
    BasicDES  des1_;
    BasicDES  des2_;

    void ProcessAndXorBlock(const byte*, const byte*, byte*) const;

    DES_EDE2(const DES_EDE2&);              // hide copy
    DES_EDE2& operator=(const DES_EDE2&);   // and assign
};



// DES_EDE3
class DES_EDE3 : public Mode_BASE {
public:
    DES_EDE3(CipherDir DIR, Mode MODE)
        : Mode_BASE(DES_BLOCK_SIZE, DIR, MODE) {}

    void SetKey(const byte*, word32, CipherDir dir);
    void SetIV(const byte* iv) { memcpy(r_, iv, DES_BLOCK_SIZE); }
#ifdef DO_DES_ASM
    void Process(byte*, const byte*, word32);
#endif
private:
    BasicDES  des1_;
    BasicDES  des2_;
    BasicDES  des3_;

    void AsmProcess(const byte* in, byte* out, void* box) const;
    void ProcessAndXorBlock(const byte*, const byte*, byte*) const;

    DES_EDE3(const DES_EDE3&);              // hide copy
    DES_EDE3& operator=(const DES_EDE3&);   // and assign
};


typedef BlockCipher<ENCRYPTION, DES, ECB> DES_ECB_Encryption;
typedef BlockCipher<DECRYPTION, DES, ECB> DES_ECB_Decryption;

typedef BlockCipher<ENCRYPTION, DES, CBC> DES_CBC_Encryption;
typedef BlockCipher<DECRYPTION, DES, CBC> DES_CBC_Decryption;

typedef BlockCipher<ENCRYPTION, DES_EDE2, ECB> DES_EDE2_ECB_Encryption;
typedef BlockCipher<DECRYPTION, DES_EDE2, ECB> DES_EDE2_ECB_Decryption;

typedef BlockCipher<ENCRYPTION, DES_EDE2, CBC> DES_EDE2_CBC_Encryption;
typedef BlockCipher<DECRYPTION, DES_EDE2, CBC> DES_EDE2_CBC_Decryption;

typedef BlockCipher<ENCRYPTION, DES_EDE3, ECB> DES_EDE3_ECB_Encryption;
typedef BlockCipher<DECRYPTION, DES_EDE3, ECB> DES_EDE3_ECB_Decryption;

typedef BlockCipher<ENCRYPTION, DES_EDE3, CBC> DES_EDE3_CBC_Encryption;
typedef BlockCipher<DECRYPTION, DES_EDE3, CBC> DES_EDE3_CBC_Decryption;


} // namespace


#endif // TAO_CRYPT_DES_HPP
