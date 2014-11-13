/*
---------------------------------------------------------------------------
Copyright (c) 1998-2013, Brian Gladman, Worcester, UK. All rights reserved.

The redistribution and use of this software (with or without changes)
is allowed without the payment of fees or royalties provided that:

  source code distributions include the above copyright notice, this
  list of conditions and the following disclaimer;

  binary distributions include the above copyright notice, this list
  of conditions and the following disclaimer in their documentation.

This software is provided 'as is' with no explicit or implied warranties
in respect of its operation, including, but not limited to, correctness
and fitness for purpose.
---------------------------------------------------------------------------
Issue Date: 20/12/2007

 This file contains the definitions required to use AES (Rijndael) in C++.
*/

#ifndef _AESCPP_H
#define _AESCPP_H

#include "aes.h"

#if defined( AES_ENCRYPT )

class AESencrypt
{
public:
    aes_encrypt_ctx cx[1];
    AESencrypt(void) { aes_init(); };
#if defined(AES_128)
    AESencrypt(const unsigned char key[])
        {   aes_encrypt_key128(key, cx); }
    AES_RETURN key128(const unsigned char key[])
        {   return aes_encrypt_key128(key, cx); }
#endif
#if defined(AES_192)
    AES_RETURN key192(const unsigned char key[])
        {   return aes_encrypt_key192(key, cx); }
#endif
#if defined(AES_256)
    AES_RETURN key256(const unsigned char key[])
        {   return aes_encrypt_key256(key, cx); }
#endif
#if defined(AES_VAR)
    AES_RETURN key(const unsigned char key[], int key_len)
        {   return aes_encrypt_key(key, key_len, cx); }
#endif
    AES_RETURN encrypt(const unsigned char in[], unsigned char out[]) const
        {   return aes_encrypt(in, out, cx);  }
#ifndef AES_MODES
    AES_RETURN ecb_encrypt(const unsigned char in[], unsigned char out[], int nb) const
        {   while(nb--)
            {   aes_encrypt(in, out, cx), in += AES_BLOCK_SIZE, out += AES_BLOCK_SIZE; }
        }
#endif
#ifdef AES_MODES
    AES_RETURN mode_reset(void)   { return aes_mode_reset(cx); }

    AES_RETURN ecb_encrypt(const unsigned char in[], unsigned char out[], int nb) const
        {   return aes_ecb_encrypt(in, out, nb, cx);  }

    AES_RETURN cbc_encrypt(const unsigned char in[], unsigned char out[], int nb,
                                    unsigned char iv[]) const
        {   return aes_cbc_encrypt(in, out, nb, iv, cx);  }

    AES_RETURN cfb_encrypt(const unsigned char in[], unsigned char out[], int nb,
                                    unsigned char iv[])
        {   return aes_cfb_encrypt(in, out, nb, iv, cx);  }

    AES_RETURN cfb_decrypt(const unsigned char in[], unsigned char out[], int nb,
                                    unsigned char iv[])
        {   return aes_cfb_decrypt(in, out, nb, iv, cx);  }

    AES_RETURN ofb_crypt(const unsigned char in[], unsigned char out[], int nb,
                                    unsigned char iv[])
        {   return aes_ofb_crypt(in, out, nb, iv, cx);  }

    typedef void ctr_fn(unsigned char ctr[]);

    AES_RETURN ctr_crypt(const unsigned char in[], unsigned char out[], int nb,
                                    unsigned char iv[], ctr_fn cf)
        {   return aes_ctr_crypt(in, out, nb, iv, cf, cx);  }

#endif

};

#endif

#if defined( AES_DECRYPT )

class AESdecrypt
{
public:
    aes_decrypt_ctx cx[1];
    AESdecrypt(void) { aes_init(); };
#if defined(AES_128)
    AESdecrypt(const unsigned char key[])
            { aes_decrypt_key128(key, cx); }
    AES_RETURN key128(const unsigned char key[])
            { return aes_decrypt_key128(key, cx); }
#endif
#if defined(AES_192)
    AES_RETURN key192(const unsigned char key[])
            { return aes_decrypt_key192(key, cx); }
#endif
#if defined(AES_256)
    AES_RETURN key256(const unsigned char key[])
            { return aes_decrypt_key256(key, cx); }
#endif
#if defined(AES_VAR)
    AES_RETURN key(const unsigned char key[], int key_len)
            { return aes_decrypt_key(key, key_len, cx); }
#endif
    AES_RETURN decrypt(const unsigned char in[], unsigned char out[]) const
        {   return aes_decrypt(in, out, cx);  }
#ifndef AES_MODES
    AES_RETURN ecb_decrypt(const unsigned char in[], unsigned char out[], int nb) const
        {   while(nb--)
            {   aes_decrypt(in, out, cx), in += AES_BLOCK_SIZE, out += AES_BLOCK_SIZE; }
        }
#endif
#ifdef AES_MODES

    AES_RETURN ecb_decrypt(const unsigned char in[], unsigned char out[], int nb) const
        {   return aes_ecb_decrypt(in, out, nb, cx);  }

    AES_RETURN cbc_decrypt(const unsigned char in[], unsigned char out[], int nb,
                                    unsigned char iv[]) const
        {   return aes_cbc_decrypt(in, out, nb, iv, cx);  }
#endif
};

#endif

#endif
