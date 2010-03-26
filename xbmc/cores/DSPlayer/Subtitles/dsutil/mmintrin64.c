// In addition to the terms of the GPL, which necessarily cover the entire project, this rather trivial file is licensed as follows:

//  Copyright (C) 2007 Peter Bright
//
//  This software is provided 'as-is', without any express or implied
//  warranty. In no event will the authors be held liable for any damages
//  arising from the use of this software.
//
//  Permission is granted to anyone to use this software for any purpose,
//  including commercial applications, and to alter it and redistribute it
//  freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//     claim that you wrote the original software. If you use this software
//     in a product, an acknowledgment in the product documentation would be
//     appreciated but is not required.
//  2. Altered source versions must be plainly marked as such, and must not be
//     misrepresented as being the original software.
//  3. This notice may not be removed or altered from any source distribution.
//
//  Peter Bright <drpizza@quiscalusmexicanus.org>

// I'd release it into the public domain if my jurisdiction had an effective and meaningful way of doing so, but it doesn't, so I won't.

#include "simd_common.h"

// MMX
__m64 _mm_setzero_si64(void)
{
	__m64 rv = {0};
	return rv;
}
__m64 _mm_set_pi32(int _I1, int _I0)
{
	__m64 rv = {0};
	rv.m64_i32[0] = _I0;
	rv.m64_i32[1] = _I1;
	return rv;
}
__m64 _mm_set_pi16(short _S3, short _S2, short _S1, short _S0)
{
	__m64 rv = {0};
	rv.m64_i16[0] = _S0;
	rv.m64_i16[1] = _S1;
	rv.m64_i16[2] = _S2;
	rv.m64_i16[3] = _S3;
	return rv;
}
__m64 _mm_set_pi8(char _B7, char _B6, char _B5, char _B4, char _B3, char _B2, char _B1, char _B0)
{
	__m64 rv = {0};
	rv.m64_i8[0] = _B0;
	rv.m64_i8[1] = _B1;
	rv.m64_i8[2] = _B2;
	rv.m64_i8[3] = _B3;
	rv.m64_i8[4] = _B4;
	rv.m64_i8[5] = _B5;
	rv.m64_i8[6] = _B6;
	rv.m64_i8[7] = _B7;
	return rv;
}
__m64 _mm_set1_pi32(int _I)
{
	__m64 rv = {0};
	rv.m64_i32[0] = rv.m64_i32[1] = _I;
	return rv;
}
__m64 _mm_set1_pi16(short _S)
{
	__m64 rv = {0};
	rv.m64_i16[0] = rv.m64_i16[1] = rv.m64_i16[2] = rv.m64_i16[3] = _S;
	return rv;
}
__m64 _mm_set1_pi8(char _B)
{
	__m64 rv = {0};
	rv.m64_i8[0] = rv.m64_i8[1] = rv.m64_i8[2] = rv.m64_i8[3] = rv.m64_i8[4] = rv.m64_i8[5] = rv.m64_i8[6] = rv.m64_i8[7] = _B;
	return rv;
}
__m64 _m_psubb(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_sub_epi8(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_paddusb(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_adds_epu8(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_psubsw(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_subs_epi16(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_psubsb(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_subs_epi8(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_paddw(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_add_epi16(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
void _m_empty(void)
{
}
__m64 _m_packuswb(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;
	lhs.m128i_i64[1] = _MM2.m64_i64;

	lhs = _mm_packus_epi16(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_psrlwi(__m64 _M, int _Count)
{
	__m128i lhs = {0};
	lhs.m128i_i64[0] = _M.m64_i64;

	lhs = _mm_srli_epi16(lhs, _Count);

	_M.m64_i64 = lhs.m128i_i64[0];
	return _M;
}
__m64 _m_pmullw(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_mullo_epi16(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_from_int(int _I)
{
	__m64 rv = {0};
	rv.m64_i32[0] = _I;
	return rv;
}
int _m_to_int(__m64 _M)
{
	return (int)(_M.m64_u64 & 0xffffffff);
}
__m64 _m_psrlqi(__m64 _M, int _Count)
{
	__m128i lhs = {0};
	lhs.m128i_i64[0] = _M.m64_i64;

	lhs = _mm_srli_epi64(lhs, _Count);

	_M.m64_i64 = lhs.m128i_i64[0];
	return _M;
}
__m64 _m_paddd(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_add_epi32(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_pmaddwd(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_madd_epi16(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_punpcklbw(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_unpacklo_epi8(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_paddb(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_add_epi8(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_por(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_or_si128(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_pand(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs, rhs;
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_and_si128(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_pandn(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_andnot_si128(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_pcmpgtb(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_cmpgt_epi8(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_psubusb(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_subs_epu8(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_psrawi(__m64 _M, int _Count)
{
	__m128i lhs = {0};
	lhs.m128i_i64[0] = _M.m64_i64;

	lhs = _mm_srai_epi16(lhs, _Count);

	_M.m64_i64 = lhs.m128i_i64[0];
	return _M;
}
__m64 _m_psubw(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_sub_epi16(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_psllwi(__m64 _M, int _Count)
{
	__m128i lhs = {0};
	lhs.m128i_i64[0] = _M.m64_i64;

	lhs = _mm_slli_epi16(lhs, _Count);

	_M.m64_i64 = lhs.m128i_i64[0];
	return _M;
}
__m64 _m_paddusw(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_adds_epu16(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_pxor(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_xor_si128(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_pslldi(__m64 _M, int _Count)
{
	__m128i lhs = {0};
	lhs.m128i_i64[0] = _M.m64_i64;

	lhs = _mm_slli_epi32(lhs, _Count);

	_M.m64_i64 = lhs.m128i_i64[0];
	return _M;
}
__m64 _m_punpckhbw(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i8[ 8] = _MM1.m64_i8[4];
	lhs.m128i_i8[ 9] = _MM1.m64_i8[5];
	lhs.m128i_i8[10] = _MM1.m64_i8[6];
	lhs.m128i_i8[11] = _MM1.m64_i8[7];

	rhs.m128i_i8[ 8] = _MM2.m64_i8[4];
	rhs.m128i_i8[ 9] = _MM2.m64_i8[5];
	rhs.m128i_i8[10] = _MM2.m64_i8[6];
	rhs.m128i_i8[11] = _MM2.m64_i8[7];

	lhs = _mm_unpackhi_epi8(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_punpcklwd(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_unpacklo_epi16(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_punpckldq(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_unpacklo_epi32(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_pcmpgtw(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_cmpgt_epi16(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_pcmpgtd(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_cmpgt_epi32(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_pcmpeqb(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_cmpeq_epi8(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_pcmpeqd(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_cmpeq_epi32(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_punpckhwd(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i16[4] = _MM1.m64_i16[2];
	lhs.m128i_i16[5] = _MM1.m64_i16[3];

	rhs.m128i_i16[4] = _MM2.m64_i16[2];
	rhs.m128i_i16[5] = _MM2.m64_i16[3];

	lhs = _mm_unpackhi_epi16(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_punpckhdq(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i32[2] = _MM1.m64_i32[1];

	rhs.m128i_i32[2] = _MM2.m64_i32[1];

	lhs = _mm_unpackhi_epi32(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_psrldi(__m64 _M, int _Count)
{
	__m128i lhs = {0};
	lhs.m128i_i64[0] = _M.m64_i64;

	lhs = _mm_srli_epi32(lhs, _Count);

	_M.m64_i64 = lhs.m128i_i64[0];
	return _M;
}
__m64 _m_psubd(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_sub_epi32(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_pmulhw(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_mulhi_epi16(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_psllqi(__m64 _M, int _Count)
{
	__m128i lhs = {0};
	lhs.m128i_i64[0] = _M.m64_i64;

	lhs = _mm_slli_epi64(lhs, _Count);

	_M.m64_i64 = lhs.m128i_i64[0];
	return _M;
}
__m64 _m_pcmpeqw(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_cmpeq_epi16(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_paddsb(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_adds_epi8(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_packsswb(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_packs_epi16(lhs, rhs);

	_MM1.m64_i32[0] = lhs.m128i_i32[0];
	_MM1.m64_i32[1] = lhs.m128i_i32[2];
	return _MM1;
}
__m64 _m_psradi(__m64 _M, int _Count)
{
	__m128i lhs = {0};
	lhs.m128i_i64[0] = _M.m64_i64;

	lhs = _mm_srai_epi32(lhs, _Count);

	_M.m64_i64 = lhs.m128i_i64[0];
	return _M;
}
__m64 _m_paddsw(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_adds_epi16(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_psubusw(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_subs_epu16(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_packssdw(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i32[0] = _MM1.m64_i32[0];
	lhs.m128i_i32[1] = _MM1.m64_i32[1];
	lhs.m128i_i32[2] = _MM2.m64_i32[0];
	lhs.m128i_i32[3] = _MM2.m64_i32[1];

	lhs = _mm_packs_epi32(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_psraw(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_sra_epi16(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}

// SSE
__m64 _m_pmaxub(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_max_epu8(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_pminub(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_min_epu8(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_pavgb(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_avg_epu8(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_pshufw(__m64 _MM1, int _Imm)
{
	__m128i lhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	switch(_Imm)
	{
	case 0: lhs = _mm_shufflelo_epi16(lhs, 0); break;
	case 1: lhs = _mm_shufflelo_epi16(lhs, 1); break;
	case 2: lhs = _mm_shufflelo_epi16(lhs, 2); break;
	case 3: lhs = _mm_shufflelo_epi16(lhs, 3); break;
	case 4: lhs = _mm_shufflelo_epi16(lhs, 4); break;
	case 5: lhs = _mm_shufflelo_epi16(lhs, 5); break;
	case 6: lhs = _mm_shufflelo_epi16(lhs, 6); break;
	case 7: lhs = _mm_shufflelo_epi16(lhs, 7); break;
	case 8: lhs = _mm_shufflelo_epi16(lhs, 8); break;
	case 9: lhs = _mm_shufflelo_epi16(lhs, 9); break;
	case 10: lhs = _mm_shufflelo_epi16(lhs, 10); break;
	case 11: lhs = _mm_shufflelo_epi16(lhs, 11); break;
	case 12: lhs = _mm_shufflelo_epi16(lhs, 12); break;
	case 13: lhs = _mm_shufflelo_epi16(lhs, 13); break;
	case 14: lhs = _mm_shufflelo_epi16(lhs, 14); break;
	case 15: lhs = _mm_shufflelo_epi16(lhs, 15); break;
	case 16: lhs = _mm_shufflelo_epi16(lhs, 16); break;
	case 17: lhs = _mm_shufflelo_epi16(lhs, 17); break;
	case 18: lhs = _mm_shufflelo_epi16(lhs, 18); break;
	case 19: lhs = _mm_shufflelo_epi16(lhs, 19); break;
	case 20: lhs = _mm_shufflelo_epi16(lhs, 20); break;
	case 21: lhs = _mm_shufflelo_epi16(lhs, 21); break;
	case 22: lhs = _mm_shufflelo_epi16(lhs, 22); break;
	case 23: lhs = _mm_shufflelo_epi16(lhs, 23); break;
	case 24: lhs = _mm_shufflelo_epi16(lhs, 24); break;
	case 25: lhs = _mm_shufflelo_epi16(lhs, 25); break;
	case 26: lhs = _mm_shufflelo_epi16(lhs, 26); break;
	case 27: lhs = _mm_shufflelo_epi16(lhs, 27); break;
	case 28: lhs = _mm_shufflelo_epi16(lhs, 28); break;
	case 29: lhs = _mm_shufflelo_epi16(lhs, 29); break;
	case 30: lhs = _mm_shufflelo_epi16(lhs, 30); break;
	case 31: lhs = _mm_shufflelo_epi16(lhs, 31); break;
	case 32: lhs = _mm_shufflelo_epi16(lhs, 32); break;
	case 33: lhs = _mm_shufflelo_epi16(lhs, 33); break;
	case 34: lhs = _mm_shufflelo_epi16(lhs, 34); break;
	case 35: lhs = _mm_shufflelo_epi16(lhs, 35); break;
	case 36: lhs = _mm_shufflelo_epi16(lhs, 36); break;
	case 37: lhs = _mm_shufflelo_epi16(lhs, 37); break;
	case 38: lhs = _mm_shufflelo_epi16(lhs, 38); break;
	case 39: lhs = _mm_shufflelo_epi16(lhs, 39); break;
	case 40: lhs = _mm_shufflelo_epi16(lhs, 40); break;
	case 41: lhs = _mm_shufflelo_epi16(lhs, 41); break;
	case 42: lhs = _mm_shufflelo_epi16(lhs, 42); break;
	case 43: lhs = _mm_shufflelo_epi16(lhs, 43); break;
	case 44: lhs = _mm_shufflelo_epi16(lhs, 44); break;
	case 45: lhs = _mm_shufflelo_epi16(lhs, 45); break;
	case 46: lhs = _mm_shufflelo_epi16(lhs, 46); break;
	case 47: lhs = _mm_shufflelo_epi16(lhs, 47); break;
	case 48: lhs = _mm_shufflelo_epi16(lhs, 48); break;
	case 49: lhs = _mm_shufflelo_epi16(lhs, 49); break;
	case 50: lhs = _mm_shufflelo_epi16(lhs, 50); break;
	case 51: lhs = _mm_shufflelo_epi16(lhs, 51); break;
	case 52: lhs = _mm_shufflelo_epi16(lhs, 52); break;
	case 53: lhs = _mm_shufflelo_epi16(lhs, 53); break;
	case 54: lhs = _mm_shufflelo_epi16(lhs, 54); break;
	case 55: lhs = _mm_shufflelo_epi16(lhs, 55); break;
	case 56: lhs = _mm_shufflelo_epi16(lhs, 56); break;
	case 57: lhs = _mm_shufflelo_epi16(lhs, 57); break;
	case 58: lhs = _mm_shufflelo_epi16(lhs, 58); break;
	case 59: lhs = _mm_shufflelo_epi16(lhs, 59); break;
	case 60: lhs = _mm_shufflelo_epi16(lhs, 60); break;
	case 61: lhs = _mm_shufflelo_epi16(lhs, 61); break;
	case 62: lhs = _mm_shufflelo_epi16(lhs, 62); break;
	case 63: lhs = _mm_shufflelo_epi16(lhs, 63); break;
	case 64: lhs = _mm_shufflelo_epi16(lhs, 64); break;
	case 65: lhs = _mm_shufflelo_epi16(lhs, 65); break;
	case 66: lhs = _mm_shufflelo_epi16(lhs, 66); break;
	case 67: lhs = _mm_shufflelo_epi16(lhs, 67); break;
	case 68: lhs = _mm_shufflelo_epi16(lhs, 68); break;
	case 69: lhs = _mm_shufflelo_epi16(lhs, 69); break;
	case 70: lhs = _mm_shufflelo_epi16(lhs, 70); break;
	case 71: lhs = _mm_shufflelo_epi16(lhs, 71); break;
	case 72: lhs = _mm_shufflelo_epi16(lhs, 72); break;
	case 73: lhs = _mm_shufflelo_epi16(lhs, 73); break;
	case 74: lhs = _mm_shufflelo_epi16(lhs, 74); break;
	case 75: lhs = _mm_shufflelo_epi16(lhs, 75); break;
	case 76: lhs = _mm_shufflelo_epi16(lhs, 76); break;
	case 77: lhs = _mm_shufflelo_epi16(lhs, 77); break;
	case 78: lhs = _mm_shufflelo_epi16(lhs, 78); break;
	case 79: lhs = _mm_shufflelo_epi16(lhs, 79); break;
	case 80: lhs = _mm_shufflelo_epi16(lhs, 80); break;
	case 81: lhs = _mm_shufflelo_epi16(lhs, 81); break;
	case 82: lhs = _mm_shufflelo_epi16(lhs, 82); break;
	case 83: lhs = _mm_shufflelo_epi16(lhs, 83); break;
	case 84: lhs = _mm_shufflelo_epi16(lhs, 84); break;
	case 85: lhs = _mm_shufflelo_epi16(lhs, 85); break;
	case 86: lhs = _mm_shufflelo_epi16(lhs, 86); break;
	case 87: lhs = _mm_shufflelo_epi16(lhs, 87); break;
	case 88: lhs = _mm_shufflelo_epi16(lhs, 88); break;
	case 89: lhs = _mm_shufflelo_epi16(lhs, 89); break;
	case 90: lhs = _mm_shufflelo_epi16(lhs, 90); break;
	case 91: lhs = _mm_shufflelo_epi16(lhs, 91); break;
	case 92: lhs = _mm_shufflelo_epi16(lhs, 92); break;
	case 93: lhs = _mm_shufflelo_epi16(lhs, 93); break;
	case 94: lhs = _mm_shufflelo_epi16(lhs, 94); break;
	case 95: lhs = _mm_shufflelo_epi16(lhs, 95); break;
	case 96: lhs = _mm_shufflelo_epi16(lhs, 96); break;
	case 97: lhs = _mm_shufflelo_epi16(lhs, 97); break;
	case 98: lhs = _mm_shufflelo_epi16(lhs, 98); break;
	case 99: lhs = _mm_shufflelo_epi16(lhs, 99); break;
	case 100: lhs = _mm_shufflelo_epi16(lhs, 100); break;
	case 101: lhs = _mm_shufflelo_epi16(lhs, 101); break;
	case 102: lhs = _mm_shufflelo_epi16(lhs, 102); break;
	case 103: lhs = _mm_shufflelo_epi16(lhs, 103); break;
	case 104: lhs = _mm_shufflelo_epi16(lhs, 104); break;
	case 105: lhs = _mm_shufflelo_epi16(lhs, 105); break;
	case 106: lhs = _mm_shufflelo_epi16(lhs, 106); break;
	case 107: lhs = _mm_shufflelo_epi16(lhs, 107); break;
	case 108: lhs = _mm_shufflelo_epi16(lhs, 108); break;
	case 109: lhs = _mm_shufflelo_epi16(lhs, 109); break;
	case 110: lhs = _mm_shufflelo_epi16(lhs, 110); break;
	case 111: lhs = _mm_shufflelo_epi16(lhs, 111); break;
	case 112: lhs = _mm_shufflelo_epi16(lhs, 112); break;
	case 113: lhs = _mm_shufflelo_epi16(lhs, 113); break;
	case 114: lhs = _mm_shufflelo_epi16(lhs, 114); break;
	case 115: lhs = _mm_shufflelo_epi16(lhs, 115); break;
	case 116: lhs = _mm_shufflelo_epi16(lhs, 116); break;
	case 117: lhs = _mm_shufflelo_epi16(lhs, 117); break;
	case 118: lhs = _mm_shufflelo_epi16(lhs, 118); break;
	case 119: lhs = _mm_shufflelo_epi16(lhs, 119); break;
	case 120: lhs = _mm_shufflelo_epi16(lhs, 120); break;
	case 121: lhs = _mm_shufflelo_epi16(lhs, 121); break;
	case 122: lhs = _mm_shufflelo_epi16(lhs, 122); break;
	case 123: lhs = _mm_shufflelo_epi16(lhs, 123); break;
	case 124: lhs = _mm_shufflelo_epi16(lhs, 124); break;
	case 125: lhs = _mm_shufflelo_epi16(lhs, 125); break;
	case 126: lhs = _mm_shufflelo_epi16(lhs, 126); break;
	case 127: lhs = _mm_shufflelo_epi16(lhs, 127); break;
	case 128: lhs = _mm_shufflelo_epi16(lhs, 128); break;
	case 129: lhs = _mm_shufflelo_epi16(lhs, 129); break;
	case 130: lhs = _mm_shufflelo_epi16(lhs, 130); break;
	case 131: lhs = _mm_shufflelo_epi16(lhs, 131); break;
	case 132: lhs = _mm_shufflelo_epi16(lhs, 132); break;
	case 133: lhs = _mm_shufflelo_epi16(lhs, 133); break;
	case 134: lhs = _mm_shufflelo_epi16(lhs, 134); break;
	case 135: lhs = _mm_shufflelo_epi16(lhs, 135); break;
	case 136: lhs = _mm_shufflelo_epi16(lhs, 136); break;
	case 137: lhs = _mm_shufflelo_epi16(lhs, 137); break;
	case 138: lhs = _mm_shufflelo_epi16(lhs, 138); break;
	case 139: lhs = _mm_shufflelo_epi16(lhs, 139); break;
	case 140: lhs = _mm_shufflelo_epi16(lhs, 140); break;
	case 141: lhs = _mm_shufflelo_epi16(lhs, 141); break;
	case 142: lhs = _mm_shufflelo_epi16(lhs, 142); break;
	case 143: lhs = _mm_shufflelo_epi16(lhs, 143); break;
	case 144: lhs = _mm_shufflelo_epi16(lhs, 144); break;
	case 145: lhs = _mm_shufflelo_epi16(lhs, 145); break;
	case 146: lhs = _mm_shufflelo_epi16(lhs, 146); break;
	case 147: lhs = _mm_shufflelo_epi16(lhs, 147); break;
	case 148: lhs = _mm_shufflelo_epi16(lhs, 148); break;
	case 149: lhs = _mm_shufflelo_epi16(lhs, 149); break;
	case 150: lhs = _mm_shufflelo_epi16(lhs, 150); break;
	case 151: lhs = _mm_shufflelo_epi16(lhs, 151); break;
	case 152: lhs = _mm_shufflelo_epi16(lhs, 152); break;
	case 153: lhs = _mm_shufflelo_epi16(lhs, 153); break;
	case 154: lhs = _mm_shufflelo_epi16(lhs, 154); break;
	case 155: lhs = _mm_shufflelo_epi16(lhs, 155); break;
	case 156: lhs = _mm_shufflelo_epi16(lhs, 156); break;
	case 157: lhs = _mm_shufflelo_epi16(lhs, 157); break;
	case 158: lhs = _mm_shufflelo_epi16(lhs, 158); break;
	case 159: lhs = _mm_shufflelo_epi16(lhs, 159); break;
	case 160: lhs = _mm_shufflelo_epi16(lhs, 160); break;
	case 161: lhs = _mm_shufflelo_epi16(lhs, 161); break;
	case 162: lhs = _mm_shufflelo_epi16(lhs, 162); break;
	case 163: lhs = _mm_shufflelo_epi16(lhs, 163); break;
	case 164: lhs = _mm_shufflelo_epi16(lhs, 164); break;
	case 165: lhs = _mm_shufflelo_epi16(lhs, 165); break;
	case 166: lhs = _mm_shufflelo_epi16(lhs, 166); break;
	case 167: lhs = _mm_shufflelo_epi16(lhs, 167); break;
	case 168: lhs = _mm_shufflelo_epi16(lhs, 168); break;
	case 169: lhs = _mm_shufflelo_epi16(lhs, 169); break;
	case 170: lhs = _mm_shufflelo_epi16(lhs, 170); break;
	case 171: lhs = _mm_shufflelo_epi16(lhs, 171); break;
	case 172: lhs = _mm_shufflelo_epi16(lhs, 172); break;
	case 173: lhs = _mm_shufflelo_epi16(lhs, 173); break;
	case 174: lhs = _mm_shufflelo_epi16(lhs, 174); break;
	case 175: lhs = _mm_shufflelo_epi16(lhs, 175); break;
	case 176: lhs = _mm_shufflelo_epi16(lhs, 176); break;
	case 177: lhs = _mm_shufflelo_epi16(lhs, 177); break;
	case 178: lhs = _mm_shufflelo_epi16(lhs, 178); break;
	case 179: lhs = _mm_shufflelo_epi16(lhs, 179); break;
	case 180: lhs = _mm_shufflelo_epi16(lhs, 180); break;
	case 181: lhs = _mm_shufflelo_epi16(lhs, 181); break;
	case 182: lhs = _mm_shufflelo_epi16(lhs, 182); break;
	case 183: lhs = _mm_shufflelo_epi16(lhs, 183); break;
	case 184: lhs = _mm_shufflelo_epi16(lhs, 184); break;
	case 185: lhs = _mm_shufflelo_epi16(lhs, 185); break;
	case 186: lhs = _mm_shufflelo_epi16(lhs, 186); break;
	case 187: lhs = _mm_shufflelo_epi16(lhs, 187); break;
	case 188: lhs = _mm_shufflelo_epi16(lhs, 188); break;
	case 189: lhs = _mm_shufflelo_epi16(lhs, 189); break;
	case 190: lhs = _mm_shufflelo_epi16(lhs, 190); break;
	case 191: lhs = _mm_shufflelo_epi16(lhs, 191); break;
	case 192: lhs = _mm_shufflelo_epi16(lhs, 192); break;
	case 193: lhs = _mm_shufflelo_epi16(lhs, 193); break;
	case 194: lhs = _mm_shufflelo_epi16(lhs, 194); break;
	case 195: lhs = _mm_shufflelo_epi16(lhs, 195); break;
	case 196: lhs = _mm_shufflelo_epi16(lhs, 196); break;
	case 197: lhs = _mm_shufflelo_epi16(lhs, 197); break;
	case 198: lhs = _mm_shufflelo_epi16(lhs, 198); break;
	case 199: lhs = _mm_shufflelo_epi16(lhs, 199); break;
	case 200: lhs = _mm_shufflelo_epi16(lhs, 200); break;
	case 201: lhs = _mm_shufflelo_epi16(lhs, 201); break;
	case 202: lhs = _mm_shufflelo_epi16(lhs, 202); break;
	case 203: lhs = _mm_shufflelo_epi16(lhs, 203); break;
	case 204: lhs = _mm_shufflelo_epi16(lhs, 204); break;
	case 205: lhs = _mm_shufflelo_epi16(lhs, 205); break;
	case 206: lhs = _mm_shufflelo_epi16(lhs, 206); break;
	case 207: lhs = _mm_shufflelo_epi16(lhs, 207); break;
	case 208: lhs = _mm_shufflelo_epi16(lhs, 208); break;
	case 209: lhs = _mm_shufflelo_epi16(lhs, 209); break;
	case 210: lhs = _mm_shufflelo_epi16(lhs, 210); break;
	case 211: lhs = _mm_shufflelo_epi16(lhs, 211); break;
	case 212: lhs = _mm_shufflelo_epi16(lhs, 212); break;
	case 213: lhs = _mm_shufflelo_epi16(lhs, 213); break;
	case 214: lhs = _mm_shufflelo_epi16(lhs, 214); break;
	case 215: lhs = _mm_shufflelo_epi16(lhs, 215); break;
	case 216: lhs = _mm_shufflelo_epi16(lhs, 216); break;
	case 217: lhs = _mm_shufflelo_epi16(lhs, 217); break;
	case 218: lhs = _mm_shufflelo_epi16(lhs, 218); break;
	case 219: lhs = _mm_shufflelo_epi16(lhs, 219); break;
	case 220: lhs = _mm_shufflelo_epi16(lhs, 220); break;
	case 221: lhs = _mm_shufflelo_epi16(lhs, 221); break;
	case 222: lhs = _mm_shufflelo_epi16(lhs, 222); break;
	case 223: lhs = _mm_shufflelo_epi16(lhs, 223); break;
	case 224: lhs = _mm_shufflelo_epi16(lhs, 224); break;
	case 225: lhs = _mm_shufflelo_epi16(lhs, 225); break;
	case 226: lhs = _mm_shufflelo_epi16(lhs, 226); break;
	case 227: lhs = _mm_shufflelo_epi16(lhs, 227); break;
	case 228: lhs = _mm_shufflelo_epi16(lhs, 228); break;
	case 229: lhs = _mm_shufflelo_epi16(lhs, 229); break;
	case 230: lhs = _mm_shufflelo_epi16(lhs, 230); break;
	case 231: lhs = _mm_shufflelo_epi16(lhs, 231); break;
	case 232: lhs = _mm_shufflelo_epi16(lhs, 232); break;
	case 233: lhs = _mm_shufflelo_epi16(lhs, 233); break;
	case 234: lhs = _mm_shufflelo_epi16(lhs, 234); break;
	case 235: lhs = _mm_shufflelo_epi16(lhs, 235); break;
	case 236: lhs = _mm_shufflelo_epi16(lhs, 236); break;
	case 237: lhs = _mm_shufflelo_epi16(lhs, 237); break;
	case 238: lhs = _mm_shufflelo_epi16(lhs, 238); break;
	case 239: lhs = _mm_shufflelo_epi16(lhs, 239); break;
	case 240: lhs = _mm_shufflelo_epi16(lhs, 240); break;
	case 241: lhs = _mm_shufflelo_epi16(lhs, 241); break;
	case 242: lhs = _mm_shufflelo_epi16(lhs, 242); break;
	case 243: lhs = _mm_shufflelo_epi16(lhs, 243); break;
	case 244: lhs = _mm_shufflelo_epi16(lhs, 244); break;
	case 245: lhs = _mm_shufflelo_epi16(lhs, 245); break;
	case 246: lhs = _mm_shufflelo_epi16(lhs, 246); break;
	case 247: lhs = _mm_shufflelo_epi16(lhs, 247); break;
	case 248: lhs = _mm_shufflelo_epi16(lhs, 248); break;
	case 249: lhs = _mm_shufflelo_epi16(lhs, 249); break;
	case 250: lhs = _mm_shufflelo_epi16(lhs, 250); break;
	case 251: lhs = _mm_shufflelo_epi16(lhs, 251); break;
	case 252: lhs = _mm_shufflelo_epi16(lhs, 252); break;
	case 253: lhs = _mm_shufflelo_epi16(lhs, 253); break;
	case 254: lhs = _mm_shufflelo_epi16(lhs, 254); break;
	case 255: lhs = _mm_shufflelo_epi16(lhs, 255); break;
	}

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_pmulhuw(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_mulhi_epu16(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_psadbw(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_sad_epu8(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_pminsw(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_min_epi16(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
__m64 _m_pmaxsw(__m64 _MM1, __m64 _MM2)
{
	__m128i lhs = {0}, rhs = {0};
	lhs.m128i_i64[0] = _MM1.m64_i64;

	rhs.m128i_i64[0] = _MM2.m64_i64;

	lhs = _mm_max_epi16(lhs, rhs);

	_MM1.m64_i64 = lhs.m128i_i64[0];
	return _MM1;
}
void _mm_stream_pi(__m64* _MM1, __m64 _MM2)
{
	_mm_stream_si32(&(_MM1->m64_i32[0]), _MM2.m64_i32[0]);
	_mm_stream_si32(&(_MM1->m64_i32[1]), _MM2.m64_i32[1]);
}
__m64 _mm_cvt_ps2pi(__m128 _A)
{
	__m64 rv = {0};
	rv.m64_i32[0] = (int)_A.m128_f32[0];
	rv.m64_i32[1] = (int)_A.m128_f32[1];
	return rv;
}
__m128 _mm_cvt_pi2ps(__m128 _MM1, __m64 _MM2)
{
	_MM1.m128_f32[0] = (float)_MM2.m64_i32[0];
	_MM1.m128_f32[1] = (float)_MM2.m64_i32[1];
	return _MM1;
}

// SSE2
__m64 _mm_movepi64_pi64(__m128i _Q)
{
	__m64 rv = {0};
	rv.m64_i64 = _Q.m128i_i64[0];
	return rv;
}
