/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

   bitset.c

   Author: Masanao Izumo <mo@goice.co.jp>
   Create: Sun Mar 02 1997
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include "timidity.h"
#include "common.h"
#include "bitset.h"

#define CUTUP(n)   (((n) + BIT_CHUNK_SIZE - 1) & ~(BIT_CHUNK_SIZE - 1))
#define CUTDOWN(n) ((n) & ~(BIT_CHUNK_SIZE - 1))

/* 右から n ビットを 1 に */
#define RFILLBITS(n) ((1u << (n)) - 1)

/* 左から n ビットを 1 に */
#define LFILLBITS(n) (RFILLBITS(n) << (BIT_CHUNK_SIZE - (n)))

static void print_uibits(unsigned int x)
{
    unsigned int mask;
    for(mask = (1u << (BIT_CHUNK_SIZE-1)); mask; mask >>= 1)
	if(mask & x)
	    putchar('1');
	else
	    putchar('0');
}

/* bitset の中を表示 */
void print_bitset(Bitset *bitset)
{
    int i, n;
    unsigned int mask;

    n = CUTDOWN(bitset->nbits)/BIT_CHUNK_SIZE;
    for(i = 0; i < n; i++)
    {
	print_uibits(bitset->bits[i]);
#if 0
	putchar(',');
#endif
    }

    n = bitset->nbits - CUTDOWN(bitset->nbits);
    mask = (1u << (BIT_CHUNK_SIZE-1));
    while(n--)
    {
	if(mask & bitset->bits[i])
	    putchar('1');
	else
	    putchar('0');
	mask >>= 1;
    }
}

/*
 * Bitset の初期化
 * 初期化後、全てのビットは 0 に初期化される
 */
void init_bitset(Bitset *bitset, int nbits)
{
    bitset->bits = (unsigned int *)
	safe_malloc((CUTUP(nbits) / BIT_CHUNK_SIZE)
		    * sizeof(unsigned int));
    bitset->nbits = nbits;
    memset(bitset->bits, 0, (CUTUP(nbits) / BIT_CHUNK_SIZE) * sizeof(int));
}

/*
 * start 番目のビットから、nbit 分、0 にセットする。
 */
void clear_bitset(Bitset *bitset, int start, int nbits)
{
    int i, j, sbitoff, ebitoff;
    unsigned int mask;

    if(nbits == 0 || start < 0 || start >= bitset->nbits)
	return;

    if(start + nbits > bitset->nbits)
	nbits = bitset->nbits - start;

    i = CUTDOWN(start);
    sbitoff = start - i;	/* 左側 n ビット目からスタート */
    i /= BIT_CHUNK_SIZE;	/* i 番目から */

    j = CUTDOWN(start + nbits - 1);
    ebitoff = start + nbits - j; /* 左側 n ビット目まで  */
    /* ebitoff := [1 ... BIT_CHUNK_SIZE] */

    j /= BIT_CHUNK_SIZE;	/* j 番目まで */

    /* 左側 sbitoff ビットまでを 1, それ以外は 0 */
    mask = LFILLBITS(sbitoff);

    if(i == j)			/* 1 つの Chunk の中に入る  */
    {
	/* 左側 ebitoff 以降は 1 */
	mask |= RFILLBITS(BIT_CHUNK_SIZE - ebitoff);
	bitset->bits[i] &= mask;
	return;
    }

    bitset->bits[i] &= mask;
    for(i++; i < j; i++)
	bitset->bits[i] = 0;
    mask = RFILLBITS(BIT_CHUNK_SIZE - ebitoff);
    bitset->bits[i] &= mask;
}

/*
 * start ビットから、nbits 分、bits にセットする
 */
void set_bitset(Bitset *bitset, const unsigned int *bits,
		int start, int nbits)
{
    int i, j, lsbitoff, rsbitoff, ebitoff;
    unsigned int mask;

    if(nbits == 0 || start < 0 || start >= bitset->nbits)
	return;

    if(start + nbits > bitset->nbits)
	nbits = bitset->nbits - start;

    i = CUTDOWN(start);
    lsbitoff = start - i;	/* 左側 n ビット目からスタート */
    rsbitoff = BIT_CHUNK_SIZE - lsbitoff; /* 右側 n ビット目からスタート */
    i /= BIT_CHUNK_SIZE;	/* i 番目から */

    j = CUTDOWN(start + nbits - 1);
    ebitoff = start + nbits - j; /* 左側 n ビット目まで  */
    /* ebitoff := [1 ... BIT_CHUNK_SIZE] */

    j /= BIT_CHUNK_SIZE;	/* j 番目まで */

    /* 左側 lsbitoff ビットまでを 1, それ以外は 0 */
    mask = LFILLBITS(lsbitoff);

    if(i == j)			/* 1 つの Chunk の中に入る  */
    {
	mask |= RFILLBITS(BIT_CHUNK_SIZE - ebitoff);
	bitset->bits[i] = ((~mask & (*bits >> lsbitoff))
			   | (mask & bitset->bits[i]));
	return;
    }

    /* |1 2 3 4|1 2 3[...]
     *  \       \     \
     *   \       \     \
     * |1 2 3 4|1 2 3 4|1 2 3 4|...
     */
    bitset->bits[i] = ((~mask & (*bits >> lsbitoff))
		       | (mask & bitset->bits[i]));
    i++;
    bits++;
    for(; i < j; i++)
    {
	bitset->bits[i] = ((bits[-1] << rsbitoff) | (bits[0] >> lsbitoff));
	bits++;
    }
    mask = LFILLBITS(ebitoff);
    bitset->bits[i] = ((bits[-1] << rsbitoff) |
		       ((mask & bits[0]) >> lsbitoff) |
		       (~mask & bitset->bits[i]));
}

/*
 * start ビットから、nbits 分、得る
 */
void get_bitset(const Bitset *bitset, unsigned int *bits,
		int start, int nbits)
{
    int i, j, lsbitoff, rsbitoff, ebitoff;

    memset(bits, 0, CUTUP(nbits) / 8);

    if(nbits == 0 || start < 0 || start >= bitset->nbits)
	return;

    if(start + nbits > bitset->nbits)
	nbits = bitset->nbits - start;

    i = CUTDOWN(start);
    lsbitoff = start - i;	/* 左側 n ビット目からスタート */
    rsbitoff = BIT_CHUNK_SIZE - lsbitoff; /* 右側 n ビット目からスタート */
    i /= BIT_CHUNK_SIZE;	/* i 番目から */

    j = CUTDOWN(start + nbits - 1);
    ebitoff = start + nbits - j; /* 左側 n ビット目まで  */
    /* ebitoff := [1 ... BIT_CHUNK_SIZE] */

    j /= BIT_CHUNK_SIZE;	/* j 番目まで */

    if(i == j)			/* 1 つの Chunk の中に入る  */
    {
	unsigned int mask;
	mask = LFILLBITS(lsbitoff) | RFILLBITS(BIT_CHUNK_SIZE - ebitoff);
	*bits = (~mask & bitset->bits[i]) << lsbitoff;
	return;
    }

    /* |1 2 3 4|1 2 3 4|1 2 3 4|...
     *     /       /   .   /
     *   /       /   .   /
     * |1 2 3 4|1 2 3[...]
     */

    for(; i < j; i++)
    {
	*bits = ((bitset->bits[i]     << lsbitoff) |
		 (bitset->bits[i + 1] >> rsbitoff));
	bits++;
    }

    if(ebitoff < lsbitoff)
	bits[-1] &= LFILLBITS(BIT_CHUNK_SIZE + ebitoff - lsbitoff);
    else
	*bits = (bitset->bits[i] << lsbitoff) & LFILLBITS(ebitoff - lsbitoff);
}

/*
 * bitset の中に 1 ビットも含まれていなければ 0 を返し，
 * 1 ビットでも含まれている場合は 1 を返す．
 */
unsigned int has_bitset(const Bitset *bitset)
{
    int i, n;
    const unsigned int *p;

    n = CUTUP(bitset->nbits) / BIT_CHUNK_SIZE;
    p = bitset->bits;

    for(i = 0; i < n; i++)
	if(p[i])
	    return 1;
    return 0;
}

int get_bitset1(Bitset *bitset, int n)
{
    int i;
    if(n < 0 || n >= bitset->nbits)
	return 0;
    i = BIT_CHUNK_SIZE - n - 1;
    return (bitset->bits[n / BIT_CHUNK_SIZE] & (1u << i)) >> i;
}

void set_bitset1(Bitset *bitset, int n, int bit)
{
    if(n < 0 || n >= bitset->nbits)
	return;
    if(bit)
	bitset->bits[n / BIT_CHUNK_SIZE] |=
	    (1 << (BIT_CHUNK_SIZE - n - 1));
    else
	bitset->bits[n / BIT_CHUNK_SIZE] &=
	   ~(1 << (BIT_CHUNK_SIZE - n - 1));
}

#if 0
void main(void)
{
    int i, j;
    Bitset b;
    unsigned int bits[3];

    init_bitset(&b, 96);

    b.bits[0] = 0x12345678;
    b.bits[1] = 0x9abcdef1;
    b.bits[2] = 0xaaaaaaaa;
    print_bitset(&b);

    for(i = 0; i <= 96; i++)
    {
	bits[0] = 0xffffffff;
	bits[1] = 0xffffffff;
	get_bitset(&b, bits, i, 60);
	print_uibits(bits[0]);putchar(',');
	print_uibits(bits[1]);
	putchar('\n');
    }
}
#endif
