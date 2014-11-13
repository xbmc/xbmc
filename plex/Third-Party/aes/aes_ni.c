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

//#include <intrin.h>
#include "aes_ni.h"

#if defined( USE_INTEL_AES_IF_PRESENT )

#pragma intrinsic(__cpuid)

__inline int has_aes_ni()
{
	static int test = -1;
	int cpu_info[4];
	if(test < 0)
	{
		__cpuid(cpu_info, 1);
		test = cpu_info[2] & 0x02000000;
	}
	return test;
}

__inline __m128i aes_128_assist(__m128i t1, __m128i t2)
{
	__m128i t3;
	t2 = _mm_shuffle_epi32(t2, 0xff);
	t3 = _mm_slli_si128(t1, 0x4);
	t1 = _mm_xor_si128(t1, t3);
	t3 = _mm_slli_si128(t3, 0x4);
	t1 = _mm_xor_si128(t1, t3);
	t3 = _mm_slli_si128(t3, 0x4);
	t1 = _mm_xor_si128(t1, t3);
	t1 = _mm_xor_si128(t1, t2);
	return t1;
}

AES_RETURN aes_ni(encrypt_key128)(const unsigned char *key, aes_encrypt_ctx cx[1])
{
	__m128i t1, t2;
	__m128i *ks = (__m128i*)cx->ks;

	if(!has_aes_ni())
	{
		aes_xi(encrypt_key128)(key, cx);
		return EXIT_SUCCESS;
	}

	t1 = _mm_loadu_si128((__m128i*)key);

	ks[0] = t1;

	t2 = _mm_aeskeygenassist_si128(t1, 0x1);
	t1 = aes_128_assist(t1, t2);
	ks[1] = t1;
	
	t2 = _mm_aeskeygenassist_si128(t1, 0x2);
	t1 = aes_128_assist(t1, t2);
	ks[2] = t1;
	
	t2 = _mm_aeskeygenassist_si128(t1, 0x4);
	t1 = aes_128_assist(t1, t2);
	ks[3] = t1;
	
	t2 = _mm_aeskeygenassist_si128(t1, 0x8);
	t1 = aes_128_assist(t1, t2);
	ks[4] = t1;
	
	t2 = _mm_aeskeygenassist_si128(t1, 0x10);
	t1 = aes_128_assist(t1, t2);
	ks[5] = t1;
	
	t2 = _mm_aeskeygenassist_si128(t1, 0x20);
	t1 = aes_128_assist(t1, t2);
	ks[6] = t1;
	
	t2 = _mm_aeskeygenassist_si128(t1, 0x40);
	t1 = aes_128_assist(t1, t2);
	ks[7] = t1;
	
	t2 = _mm_aeskeygenassist_si128(t1, 0x80);
	t1 = aes_128_assist(t1, t2);
	ks[8] = t1;
	
	t2 = _mm_aeskeygenassist_si128(t1, 0x1b);
	t1 = aes_128_assist(t1, t2);
	ks[9] = t1;
	
	t2 = _mm_aeskeygenassist_si128(t1, 0x36);
	t1 = aes_128_assist(t1, t2);
	ks[10] = t1;

	cx->inf.l = 0;
	cx->inf.b[0] = 10 * 16;
	return EXIT_SUCCESS;
}

__inline void aes_192_assist(__m128i* t1, __m128i * t2, __m128i * t3)
{
	__m128i t4;
	*t2 = _mm_shuffle_epi32(*t2, 0x55);
	t4 = _mm_slli_si128(*t1, 0x4);
	*t1 = _mm_xor_si128(*t1, t4);
	t4 = _mm_slli_si128(t4, 0x4);
	*t1 = _mm_xor_si128(*t1, t4);
	t4 = _mm_slli_si128(t4, 0x4);
	*t1 = _mm_xor_si128(*t1, t4);
	*t1 = _mm_xor_si128(*t1, *t2);
	*t2 = _mm_shuffle_epi32(*t1, 0xff);
	t4 = _mm_slli_si128(*t3, 0x4);
	*t3 = _mm_xor_si128(*t3, t4);
	*t3 = _mm_xor_si128(*t3, *t2);
}

AES_RETURN aes_ni(encrypt_key192)(const unsigned char *key, aes_encrypt_ctx cx[1])
{
	__m128i t1, t2, t3;
	__m128i *ks = (__m128i*)cx->ks;

	if(!has_aes_ni())
	{
		aes_xi(encrypt_key192)(key, cx);
		return EXIT_SUCCESS;
	}

	t1 = _mm_loadu_si128((__m128i*)key);
	t3 = _mm_loadu_si128((__m128i*)(key + 16));

	ks[0] = t1;
	ks[1] = t3;
	
	t2 = _mm_aeskeygenassist_si128(t3, 0x1);
	aes_192_assist(&t1, &t2, &t3);

	ks[1] = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(ks[1]), _mm_castsi128_pd(t1), 0));
	ks[2] = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(t1), _mm_castsi128_pd(t3), 1));
	
	t2 = _mm_aeskeygenassist_si128(t3, 0x2);
	aes_192_assist(&t1, &t2, &t3);
	ks[3] = t1;
	ks[4] = t3;
	
	t2 = _mm_aeskeygenassist_si128(t3, 0x4);
	aes_192_assist(&t1, &t2, &t3);
	ks[4] = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(ks[4]), _mm_castsi128_pd(t1), 0));
	ks[5] = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(t1), _mm_castsi128_pd(t3), 1));
	
	t2 = _mm_aeskeygenassist_si128(t3, 0x8);
	aes_192_assist(&t1, &t2, &t3);
	ks[6] = t1;
	ks[7] = t3;
	
	t2 = _mm_aeskeygenassist_si128(t3, 0x10);
	aes_192_assist(&t1, &t2, &t3);
	ks[7] = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(ks[7]), _mm_castsi128_pd(t1), 0));
	ks[8] = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(t1), _mm_castsi128_pd(t3), 1));
	
	t2 = _mm_aeskeygenassist_si128(t3, 0x20);
	aes_192_assist(&t1, &t2, &t3);
	ks[9] = t1;
	ks[10] = t3;
	
	t2 = _mm_aeskeygenassist_si128(t3, 0x40);
	aes_192_assist(&t1, &t2, &t3);
	ks[10] = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(ks[10]), _mm_castsi128_pd(t1), 0));
	ks[11] = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(t1), _mm_castsi128_pd(t3), 1));
	
	t2 = _mm_aeskeygenassist_si128(t3, 0x80);
	aes_192_assist(&t1, &t2, &t3);
	ks[12] = t1;

	cx->inf.l = 0;
	cx->inf.b[0] = 12 * 16;
	return EXIT_SUCCESS;
}

__inline void aes_256_assist1(__m128i* t1, __m128i * t2)
{
	__m128i t4;
	*t2 = _mm_shuffle_epi32(*t2, 0xff);
	t4 = _mm_slli_si128(*t1, 0x4);
	*t1 = _mm_xor_si128(*t1, t4);
	t4 = _mm_slli_si128(t4, 0x4);
	*t1 = _mm_xor_si128(*t1, t4);
	t4 = _mm_slli_si128(t4, 0x4);
	*t1 = _mm_xor_si128(*t1, t4);
	*t1 = _mm_xor_si128(*t1, *t2);
}

__inline void aes_256_assist2(__m128i* t1, __m128i * t3)
{
	__m128i t2, t4;
	t4 = _mm_aeskeygenassist_si128(*t1, 0x0);
	t2 = _mm_shuffle_epi32(t4, 0xaa);
	t4 = _mm_slli_si128(*t3, 0x4);
	*t3 = _mm_xor_si128(*t3, t4);
	t4 = _mm_slli_si128(t4, 0x4);
	*t3 = _mm_xor_si128(*t3, t4);
	t4 = _mm_slli_si128(t4, 0x4);
	*t3 = _mm_xor_si128(*t3, t4);
	*t3 = _mm_xor_si128(*t3, t2);
}

AES_RETURN aes_ni(encrypt_key256)(const unsigned char *key, aes_encrypt_ctx cx[1])
{
	__m128i t1, t2, t3;
	__m128i *ks = (__m128i*)cx->ks;

	if(!has_aes_ni())
	{
		aes_xi(encrypt_key256)(key, cx);
		return EXIT_SUCCESS;
	}

	t1 = _mm_loadu_si128((__m128i*)key);
	t3 = _mm_loadu_si128((__m128i*)(key + 16));

	ks[0] = t1;
	ks[1] = t3;
	
	t2 = _mm_aeskeygenassist_si128(t3, 0x01);
	aes_256_assist1(&t1, &t2);
	ks[2] = t1;
	aes_256_assist2(&t1, &t3);
	ks[3] = t3;

	t2 = _mm_aeskeygenassist_si128(t3, 0x02);
	aes_256_assist1(&t1, &t2);
	ks[4] = t1;
	aes_256_assist2(&t1, &t3);
	ks[5] = t3;
	
	t2 = _mm_aeskeygenassist_si128(t3, 0x04);
	aes_256_assist1(&t1, &t2);
	ks[6] = t1;
	aes_256_assist2(&t1, &t3);
	ks[7] = t3;
	
	t2 = _mm_aeskeygenassist_si128(t3, 0x08);
	aes_256_assist1(&t1, &t2);
	ks[8] = t1;
	aes_256_assist2(&t1, &t3);
	ks[9] = t3;
	
	t2 = _mm_aeskeygenassist_si128(t3, 0x10);
	aes_256_assist1(&t1, &t2);
	ks[10] = t1;
	aes_256_assist2(&t1, &t3);
	ks[11] = t3;
	
	t2 = _mm_aeskeygenassist_si128(t3, 0x20);
	aes_256_assist1(&t1, &t2);
	ks[12] = t1;
	aes_256_assist2(&t1, &t3);
	ks[13] = t3;
	
	t2 = _mm_aeskeygenassist_si128(t3, 0x40);
	aes_256_assist1(&t1, &t2);
	ks[14] = t1;

	cx->inf.l = 0;
	cx->inf.b[0] = 14 * 16;
	return EXIT_SUCCESS;
}

__inline void enc_to_dec(aes_decrypt_ctx cx[1])
{
	__m128i *ks = (__m128i*)cx->ks;
	int j;

	for( j = 1 ; j < (cx->inf.b[0] >> 4) ; ++j )
		ks[j] = _mm_aesimc_si128(ks[j]);
}

AES_RETURN aes_ni(decrypt_key128)(const unsigned char *key, aes_decrypt_ctx cx[1])
{
	if(!has_aes_ni())
	{
		aes_xi(decrypt_key128)(key, cx);
		return EXIT_SUCCESS;
	}

	if(aes_ni(encrypt_key128)(key, (aes_encrypt_ctx*)cx) == EXIT_SUCCESS)
	{
		enc_to_dec(cx);
		return EXIT_SUCCESS;
	}
	else
		return EXIT_FAILURE;

}

AES_RETURN aes_ni(decrypt_key192)(const unsigned char *key, aes_decrypt_ctx cx[1])
{
	if(!has_aes_ni())
	{
		aes_xi(decrypt_key192)(key, cx);
		return EXIT_SUCCESS;
	}

	if(aes_ni(encrypt_key192)(key, (aes_encrypt_ctx*)cx) == EXIT_SUCCESS)
	{
		enc_to_dec(cx);
		return EXIT_SUCCESS;
	}
	else
		return EXIT_FAILURE;
}

AES_RETURN aes_ni(decrypt_key256)(const unsigned char *key, aes_decrypt_ctx cx[1])
{
	if(!has_aes_ni())
	{
		aes_xi(decrypt_key256)(key, cx);
		return EXIT_SUCCESS;
	}

	if(aes_ni(encrypt_key256)(key, (aes_encrypt_ctx*)cx) == EXIT_SUCCESS)
	{
		enc_to_dec(cx);
		return EXIT_SUCCESS;
	}
	else
		return EXIT_FAILURE;
}

AES_RETURN aes_ni(encrypt)(const unsigned char *in, unsigned char *out, const aes_encrypt_ctx cx[1])
{
	__m128i *key = (__m128i*)cx->ks, t;

	if(cx->inf.b[0] != 10 * 16 && cx->inf.b[0] != 12 * 16 && cx->inf.b[0] != 14 * 16)
		return EXIT_FAILURE;

	if(!has_aes_ni())
	{
		aes_xi(encrypt)(in, out, cx);
		return EXIT_SUCCESS;
	}

	t = _mm_xor_si128(_mm_loadu_si128((__m128i*)in), *(__m128i*)key);

	switch(cx->inf.b[0])
	{
	case 14 * 16:
		t = _mm_aesenc_si128(t, *(__m128i*)++key);
		t = _mm_aesenc_si128(t, *(__m128i*)++key);
	case 12 * 16:
		t = _mm_aesenc_si128(t, *(__m128i*)++key);
		t = _mm_aesenc_si128(t, *(__m128i*)++key);
	case 10 * 16:
		t = _mm_aesenc_si128(t, *(__m128i*)++key);
		t = _mm_aesenc_si128(t, *(__m128i*)++key);
		t = _mm_aesenc_si128(t, *(__m128i*)++key);
		t = _mm_aesenc_si128(t, *(__m128i*)++key);
		t = _mm_aesenc_si128(t, *(__m128i*)++key);
		t = _mm_aesenc_si128(t, *(__m128i*)++key);
		t = _mm_aesenc_si128(t, *(__m128i*)++key);
		t = _mm_aesenc_si128(t, *(__m128i*)++key);
		t = _mm_aesenc_si128(t, *(__m128i*)++key);
		t = _mm_aesenclast_si128(t, *(__m128i*)++key);
	}

	_mm_storeu_si128(&((__m128i*)out)[0], t);
	return EXIT_SUCCESS;
}

AES_RETURN aes_ni(decrypt)(const unsigned char *in, unsigned char *out, const aes_decrypt_ctx cx[1])
{
	__m128i *key = (__m128i*)cx->ks + (cx->inf.b[0] >> 4), t;

	if(cx->inf.b[0] != 10 * 16 && cx->inf.b[0] != 12 * 16 && cx->inf.b[0] != 14 * 16)
		return EXIT_FAILURE;

	if(!has_aes_ni())
	{
		aes_xi(decrypt)(in, out, cx);
		return EXIT_SUCCESS;
	}

	t = _mm_xor_si128(_mm_loadu_si128((__m128i*)in), *(__m128i*)key);

	switch(cx->inf.b[0])
	{
	case 14 * 16:
		t = _mm_aesdec_si128(t, *(__m128i*)--key);
		t = _mm_aesdec_si128(t, *(__m128i*)--key);
	case 12 * 16:
		t = _mm_aesdec_si128(t, *(__m128i*)--key);
		t = _mm_aesdec_si128(t, *(__m128i*)--key);
	case 10 * 16:
		t = _mm_aesdec_si128(t, *(__m128i*)--key);
		t = _mm_aesdec_si128(t, *(__m128i*)--key);
		t = _mm_aesdec_si128(t, *(__m128i*)--key);
		t = _mm_aesdec_si128(t, *(__m128i*)--key);
		t = _mm_aesdec_si128(t, *(__m128i*)--key);
		t = _mm_aesdec_si128(t, *(__m128i*)--key);
		t = _mm_aesdec_si128(t, *(__m128i*)--key);
		t = _mm_aesdec_si128(t, *(__m128i*)--key);
		t = _mm_aesdec_si128(t, *(__m128i*)--key);
		t = _mm_aesdeclast_si128(t, *(__m128i*)--key);
	}

	_mm_storeu_si128((__m128i*)out, t);
	return EXIT_SUCCESS;
}

void aes_CBC_encrypt(const unsigned char *in,
	unsigned char *out,
	unsigned char ivec[16],
	unsigned long length,
	unsigned char *key,
	int number_of_rounds)
{
	__m128i feedback, data;
	int i, j;
	if(length % 16)
		length = length / 16 + 1;
	else length /= 16;
	feedback = _mm_loadu_si128((__m128i*)ivec);
	for(i = 0; i < length; i++)
	{
		data = _mm_loadu_si128(&((__m128i*)in)[i]);
		feedback = _mm_xor_si128(data, feedback);
		feedback = _mm_xor_si128(feedback, ((__m128i*)key)[0]);
		for(j = 1; j <number_of_rounds; j++)
			feedback = _mm_aesenc_si128(feedback, ((__m128i*)key)[j]);
		feedback = _mm_aesenclast_si128(feedback, ((__m128i*)key)[j]);
		_mm_storeu_si128(&((__m128i*)out)[i], feedback);
	}
}

void aes_CBC_decrypt(const unsigned char *in,
	unsigned char *out,
	unsigned char ivec[16],
	unsigned long length,
	unsigned char *key,
	int number_of_rounds)
{
	__m128i data, feedback, last_in;
	int i, j;
	if(length % 16)
		length = length / 16 + 1;
	else length /= 16;
	feedback = _mm_loadu_si128((__m128i*)ivec);
	for(i = 0; i < length; i++)
	{
		last_in = _mm_loadu_si128(&((__m128i*)in)[i]);
		data = _mm_xor_si128(last_in, ((__m128i*)key)[0]);
		for(j = 1; j <number_of_rounds; j++)
		{
			data = _mm_aesdec_si128(data, ((__m128i*)key)[j]);
		}
		data = _mm_aesdeclast_si128(data, ((__m128i*)key)[j]);
		data = _mm_xor_si128(data, feedback);
		_mm_storeu_si128(&((__m128i*)out)[i], data);
		feedback = last_in;
	}
}

void AES_CTR_encrypt(const unsigned char *in,
	unsigned char *out,
	const unsigned char ivec[8],
	const unsigned char nonce[4],
	unsigned long length,
	const unsigned char *key,
	int number_of_rounds)
{
	__m128i ctr_block = { 0 }, tmp, ONE, BSWAP_EPI64;
	int i, j;
	if(length % 16)
		length = length / 16 + 1;
	else length /= 16;
	ONE = _mm_set_epi32(0, 1, 0, 0);
	BSWAP_EPI64 = _mm_setr_epi8(7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8);
	ctr_block = _mm_insert_epi64(ctr_block, *(long long*)ivec, 1);
	ctr_block = _mm_insert_epi32(ctr_block, *(long*)nonce, 1);
	ctr_block = _mm_srli_si128(ctr_block, 4);
	ctr_block = _mm_shuffle_epi8(ctr_block, BSWAP_EPI64);
	ctr_block = _mm_add_epi64(ctr_block, ONE);
	for(i = 0; i < length; i++)
	{
		tmp = _mm_shuffle_epi8(ctr_block, BSWAP_EPI64);
		ctr_block = _mm_add_epi64(ctr_block, ONE);
		tmp = _mm_xor_si128(tmp, ((__m128i*)key)[0]);
		for(j = 1; j <number_of_rounds; j++)
		{
			tmp = _mm_aesenc_si128(tmp, ((__m128i*)key)[j]);
		};
		tmp = _mm_aesenclast_si128(tmp, ((__m128i*)key)[j]);
		tmp = _mm_xor_si128(tmp, _mm_loadu_si128(&((__m128i*)in)[i]));
		_mm_storeu_si128(&((__m128i*)out)[i], tmp);
	}
}

#endif
