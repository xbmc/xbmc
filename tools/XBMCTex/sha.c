#include <stdlib.h>

typedef unsigned long u32;
typedef unsigned char u8;

#ifdef _LINUX
#include <string.h>
#define __forceinline inline
#define __int64 __int64_t
#endif

__forceinline static u32 rol(u32 x, u8 n)
{
	return (x << n) | (x >> (32-n));
}

static void bswapcpy(void* dst, const void* src, u32 n)
{
#ifdef _LINUX
  __asm__ ( 
    "mov %%ecx,%2\n\t"
    "mov %%esi,%1\n\t"
    "mov %%edi,%0\n\t"
    "shr $2,%%ecx\n\t"
    "jz  bswapcpy2\n"
  "bswapcpy1:\n\t"
    "mov %%eax,(%%esi)\n\t"
    "bswap %%eax\n\t"
    "mov (%%edi),%%eax\n\t"
    "add %%esi,4\n\t"
    "add %%edi,4\n\t"
    "dec %%ecx\n\t"
    "jnz bswapcpy1\n"
  "bswapcpy2:" : : "r"(dst), "r"(src), "r"(n)
  );
#else
	__asm {
		mov ecx,n
		mov esi,src
		mov edi,dst
		shr ecx,2
		jz  bswapcpy_2
bswapcpy_1:
		mov eax,dword ptr [esi]
		bswap eax
		mov dword ptr [edi],eax
		add esi,4
		add edi,4
		dec ecx
		jnz bswapcpy_1
bswapcpy_2:
  }
#endif
}

void SHA1(const u8* buf, u32 len, u8 hash[20])
{
	u32 a, b, c, d, e;
	u32 h[5] = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 };
	u32 w[80];
	u32 n = len;
	int t;
	int done = 0;

	while (done != 2)
	{
		if (n >= 64)
		{
			bswapcpy(w, buf, 64);
			buf += 64;
			n -= 64;
		}
		else
		{
			u8 tmpbuf[64];
			memcpy(tmpbuf, buf, n);
			memset(tmpbuf+n, 0, 64-n);
			if (!done)
				tmpbuf[n] = 0x80;
			bswapcpy(w, tmpbuf, 64);
			if (n <= 55)
			{
				__int64 bitlen = (__int64)len * 8;
				w[14] = *(((u32*)&bitlen)+1);
				w[15] = *((u32*)&bitlen);
				done = 2;
			}
			else
				done = 1;
			n = 0;
		}

		for (t = 16; t < 80; ++t)
		{
			w[t] = rol(w[t-3] ^ w[t-8] ^ w[t-14] ^ w[t-16], 1);
		}

		a = h[0]; b = h[1]; c = h[2]; d = h[3]; e = h[4];

		for (t = 0; t < 20; ++t)
		{
			u32 temp = rol(a, 5) + ((b & c) | ((~b) & d)) + e + w[t] + 0x5A827999;
			e = d; d = c; c = rol(b, 30); b = a; a = temp;
		}
		for ( ; t < 40; ++t)
		{
			u32 temp = rol(a, 5) + (b ^ c ^ d) + e + w[t] + 0x6ED9EBA1;
			e = d; d = c; c = rol(b, 30); b = a; a = temp;
		}
		for ( ; t < 60; ++t)
		{
			u32 temp = rol(a, 5) + ((b & c) | (b & d) | (c & d)) + e + w[t] + 0x8F1BBCDC;
			e = d; d = c; c = rol(b, 30); b = a; a = temp;
		}
		for ( ; t < 80; ++t)
		{
			u32 temp = rol(a, 5) + (b ^ c ^ d) + e + w[t] + 0xCA62C1D6;
			e = d; d = c; c = rol(b, 30); b = a; a = temp;
		}

		h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e;
	}

	bswapcpy(hash, h, 20);
}
