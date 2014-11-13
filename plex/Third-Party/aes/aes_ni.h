/*
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
Issue Date: 13/11/2013
*/

#ifndef AES_NI_H
#define AES_NI_H

//#include <intrin.h>
#include "aesopt.h"

#if defined( USE_INTEL_AES_IF_PRESENT )

/* map names in C code to make them internal ('name' -> 'aes_name_i') */
#define aes_xi(x) aes_ ## x ## _i

/* map names here to provide the external API ('name' -> 'aes_name') */
#define aes_ni(x) aes_ ## x

AES_RETURN aes_ni(encrypt_key128)(const unsigned char *key, aes_encrypt_ctx cx[1]);
AES_RETURN aes_ni(encrypt_key192)(const unsigned char *key, aes_encrypt_ctx cx[1]);
AES_RETURN aes_ni(encrypt_key256)(const unsigned char *key, aes_encrypt_ctx cx[1]);

AES_RETURN aes_ni(decrypt_key128)(const unsigned char *key, aes_decrypt_ctx cx[1]);
AES_RETURN aes_ni(decrypt_key192)(const unsigned char *key, aes_decrypt_ctx cx[1]);
AES_RETURN aes_ni(decrypt_key256)(const unsigned char *key, aes_decrypt_ctx cx[1]);

AES_RETURN aes_ni(encrypt)(const unsigned char *in, unsigned char *out, const aes_encrypt_ctx cx[1]);
AES_RETURN aes_ni(decrypt)(const unsigned char *in, unsigned char *out, const aes_decrypt_ctx cx[1]);

AES_RETURN aes_xi(encrypt_key128)(const unsigned char *key, aes_encrypt_ctx cx[1]);
AES_RETURN aes_xi(encrypt_key192)(const unsigned char *key, aes_encrypt_ctx cx[1]);
AES_RETURN aes_xi(encrypt_key256)(const unsigned char *key, aes_encrypt_ctx cx[1]);

AES_RETURN aes_xi(decrypt_key128)(const unsigned char *key, aes_decrypt_ctx cx[1]);
AES_RETURN aes_xi(decrypt_key192)(const unsigned char *key, aes_decrypt_ctx cx[1]);
AES_RETURN aes_xi(decrypt_key256)(const unsigned char *key, aes_decrypt_ctx cx[1]);

AES_RETURN aes_xi(encrypt)(const unsigned char *in, unsigned char *out, const aes_encrypt_ctx cx[1]);
AES_RETURN aes_xi(decrypt)(const unsigned char *in, unsigned char *out, const aes_decrypt_ctx cx[1]);

#endif

#endif
