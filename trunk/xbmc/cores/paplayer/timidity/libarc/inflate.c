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
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
/* inflate.c -- Not copyrighted 1992 by Mark Adler
   version c10p1, 10 January 1993 */

/* You can do whatever you like with this source file, though I would
   prefer that if you modify it and redistribute it that you include
   comments to that effect with your name and the date.	 Thank you.
   [The history has been moved to the file ChangeLog.]
 */

/*
   Inflate deflated (PKZIP's method 8 compressed) data.	 The compression
   method searches for as much of the current string of bytes (up to a
   length of 258) in the previous 32K bytes.  If it doesn't find any
   matches (of at least length 3), it codes the next byte.  Otherwise, it
   codes the length of the matched string and its distance backwards from
   the current position.  There is a single Huffman code that codes both
   single bytes (called "literals") and match lengths.	A second Huffman
   code codes the distance information, which follows a length code.  Each
   length or distance code actually represents a base value and a number
   of "extra" (sometimes zero) bits to get to add to the base value.  At
   the end of each deflated block is a special end-of-block (EOB) literal/
   length code.	 The decoding process is basically: get a literal/length
   code; if EOB then done; if a literal, emit the decoded byte; if a
   length then get the distance and emit the referred-to bytes from the
   sliding window of previously emitted data.

   There are (currently) three kinds of inflate blocks: stored, fixed, and
   dynamic.  The compressor outputs a chunk of data at a time and decides
   which method to use on a chunk-by-chunk basis.  A chunk might typically
   be 32K to 64K, uncompressed.	 If the chunk is uncompressible, then the
   "stored" method is used.  In this case, the bytes are simply stored as
   is, eight bits per byte, with none of the above coding.  The bytes are
   preceded by a count, since there is no longer an EOB code.

   If the data are compressible, then either the fixed or dynamic methods
   are used.  In the dynamic method, the compressed data are preceded by
   an encoding of the literal/length and distance Huffman codes that are
   to be used to decode this block.  The representation is itself Huffman
   coded, and so is preceded by a description of that code.  These code
   descriptions take up a little space, and so for small blocks, there is
   a predefined set of codes, called the fixed codes.  The fixed method is
   used if the block ends up smaller that way (usually for quite small
   chunks); otherwise the dynamic method is used.  In the latter case, the
   codes are customized to the probabilities in the current block and so
   can code it much better than the pre-determined fixed codes can.

   The Huffman codes themselves are decoded using a multi-level table
   lookup, in order to maximize the speed of decoding plus the speed of
   building the decoding tables.  See the comments below that precede the
   lbits and dbits tuning parameters.
 */


/*
   Notes beyond the 1.93a appnote.txt:

   1. Distance pointers never point before the beginning of the output
      stream.
   2. Distance pointers can point back across blocks, up to 32k away.
   3. There is an implied maximum of 7 bits for the bit length table and
      15 bits for the actual data.
   4. If only one code exists, then it is encoded using one bit.  (Zero
      would be more efficient, but perhaps a little confusing.)	 If two
      codes exist, they are coded using one bit each (0 and 1).
   5. There is no way of sending zero distance codes--a dummy must be
      sent if there are none.  (History: a pre 2.0 version of PKZIP would
      store blocks with no distance codes, but this was discovered to be
      too harsh a criterion.)  Valid only for 1.93a.  2.04c does allow
      zero distance codes, which is sent as one code of zero bits in
      length.
   6. There are up to 286 literal/length codes.	 Code 256 represents the
      end-of-block.  Note however that the static length tree defines
      288 codes just to fill out the Huffman codes.  Codes 286 and 287
      cannot be used though, since there is no length base or extra bits
      defined for them.	 Similarily, there are up to 30 distance codes.
      However, static trees define 32 codes (all 5 bits) to fill out the
      Huffman codes, but the last two had better not show up in the data.
   7. Unzip can check dynamic Huffman blocks for complete code sets.
      The exception is that a single code would not be complete (see #4).
   8. The five bits following the block type is really the number of
      literal codes sent minus 257.
   9. Length codes 8,16,16 are interpreted as 13 length codes of 8 bits
      (1+6+6).	Therefore, to output three times the length, you output
      three codes (1+1+1), whereas to output four times the same length,
      you only need two codes (1+3).  Hmm.
  10. In the tree reconstruction algorithm, Code = Code + Increment
      only if BitLength(i) is not zero.	 (Pretty obvious.)
  11. Correction: 4 Bits: # of Bit Length codes - 4	(4 - 19)
  12. Note: length code 284 can represent 227-258, but length code 285
      really is 258.  The last length deserves its own, short code
      since it gets used a lot in very redundant files.	 The length
      258 is special since 258 - 3 (the min match length) is 255.
  13. The literal/length and distance code bit lengths are read as a
      single stream of lengths.	 It is possible (and advantageous) for
      a repeat code (16, 17, or 18) to go across the boundary between
      the two sets of lengths.
 */

#include <stdio.h>
#include <stdlib.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include "timidity.h"
#include "mblock.h"
#include "zip.h"
#define local static

/* Save to local */
#define BITS_SAVE \
  ulg bit_buf = decoder->bit_buf; \
  ulg bit_len = decoder->bit_len;

/* Restore to decoder */
#define BITS_RESTORE \
  decoder->bit_buf = bit_buf; \
  decoder->bit_len = bit_len;

#define MASK_BITS(n) ((((ulg)1)<<(n))-1)
#define GET_BYTE()  (decoder->inptr < decoder->insize ? decoder->inbuf[decoder->inptr++] : fill_inbuf(decoder))
#define NEEDBITS(n) {while(bit_len<(n)){bit_buf|=((ulg)GET_BYTE())<<bit_len;bit_len+=8;}}
#define GETBITS(n)  (bit_buf & MASK_BITS(n))
#define DUMPBITS(n) {bit_buf>>=(n);bit_len-=(n);}

/* variables */
struct _InflateHandler
{
    void *user_val;
    long (* read_func)(char *buf, long size, void *user_val);

    uch slide[2L * WSIZE];
    uch inbuf[INBUFSIZ + INBUF_EXTRA];
    unsigned wp;	/* current position in slide */
    unsigned insize;	/* valid bytes in inbuf */
    unsigned inptr;	/* index of next byte to be processed in inbuf */
    struct huft *fixed_tl;	/* inflate static */
    struct huft *fixed_td;	/* inflate static */
    int fixed_bl, fixed_bd;	/* inflate static */
    ulg bit_buf;	/* bit buffer */
    ulg bit_len;	/* bits in bit buffer */
    int method;
    int eof;
    unsigned copy_leng;
    unsigned copy_dist;
    struct huft *tl, *td; /* literal/length and distance decoder tables */
    int bl, bd;		/* number of bits decoded by tl[] and td[] */
    MBlockList pool;	/* memory buffer for tl, td */
};

/* Function prototypes */
local int fill_inbuf(InflateHandler);
local int huft_free(struct huft *);
local long inflate_codes(InflateHandler, char *, long);
local long inflate_stored(InflateHandler, char *, long);
local long inflate_fixed(InflateHandler, char *, long);
local long inflate_dynamic(InflateHandler, char *, long);
local void inflate_start(InflateHandler);

/* The inflate algorithm uses a sliding 32K byte window on the uncompressed
   stream to find repeated byte strings.  This is implemented here as a
   circular buffer.  The index is updated simply by incrementing and then
   and'ing with 0x7fff (32K-1). */
/* It is left to other modules to supply the 32K area.	It is assumed
   to be usable as if it were declared "uch slide[32768];" or as just
   "uch *slide;" and then malloc'ed in the latter case.	 The definition
   must be in unzip.h, included above. */

#define lbits 9			/* bits in base literal/length lookup table */
#define dbits 6			/* bits in base distance lookup table */

/* Tables for deflate from PKZIP's appnote.txt. */
local ush cplens[] = {		/* Copy lengths for literal codes 257..285 */
	3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
	35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0};
	/* note: see note #13 above about the 258 in this list. */
local ush cplext[] = {		/* Extra bits for literal codes 257..285 */
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
	3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 99, 99}; /* 99==invalid */
local ush cpdist[] = {		/* Copy offsets for distance codes 0..29 */
	1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
	257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
	8193, 12289, 16385, 24577};
local ush cpdext[] = {		/* Extra bits for distance codes */
	0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
	7, 7, 8, 8, 9, 9, 10, 10, 11, 11,
	12, 12, 13, 13};

/*
   Huffman code decoding is performed using a multi-level table lookup.
   The fastest way to decode is to simply build a lookup table whose
   size is determined by the longest code.  However, the time it takes
   to build this table can also be a factor if the data being decoded
   are not very long.  The most common codes are necessarily the
   shortest codes, so those codes dominate the decoding time, and hence
   the speed.  The idea is you can have a shorter table that decodes the
   shorter, more probable codes, and then point to subsidiary tables for
   the longer codes.  The time it costs to decode the longer codes is
   then traded against the time it takes to make longer tables.

   This results of this trade are in the variables lbits and dbits
   below.  lbits is the number of bits the first level table for literal/
   length codes can decode in one step, and dbits is the same thing for
   the distance codes.	Subsequent tables are also less than or equal to
   those sizes.	 These values may be adjusted either when all of the
   codes are shorter than that, in which case the longest code length in
   bits is used, or when the shortest code is *longer* than the requested
   table size, in which case the length of the shortest code in bits is
   used.

   There are two different values for the two tables, since they code a
   different number of possibilities each.  The literal/length table
   codes 286 possible values, or in a flat code, a little over eight
   bits.  The distance table codes 30 possible values, or a little less
   than five bits, flat.  The optimum values for speed end up being
   about one bit more than those, so lbits is 8+1 and dbits is 5+1.
   The optimum values may differ though from machine to machine, and
   possibly even between compilers.  Your mileage may vary.
 */

/* If BMAX needs to be larger than 16, then h and x[] should be ulg. */
#define BMAX 16		/* maximum bit length of any code (16 for explode) */
#define N_MAX 288	/* maximum number of codes in any set */

int huft_build(
    unsigned *b,	/* code lengths in bits (all assumed <= BMAX) */
    unsigned n,		/* number of codes (assumed <= N_MAX) */
    unsigned s,		/* number of simple-valued codes (0..s-1) */
    ush *d,		/* list of base values for non-simple codes */
    ush *e,		/* list of extra bits for non-simple codes */
    struct huft **t,	/* result: starting table */
    int *m,		/* maximum lookup bits, returns actual */
    MBlockList *pool)	/* memory pool */
/* Given a list of code lengths and a maximum table size, make a set of
   tables to decode that set of codes.	Return zero on success, one if
   the given code set is incomplete (the tables are still built in this
   case), two if the input is invalid (all zero length codes or an
   oversubscribed set of lengths), and three if not enough memory.
   The code with value 256 is special, and the tables are constructed
   so that no bits beyond that code are fetched when that code is
   decoded. */
{
    unsigned a;			/* counter for codes of length k */
    unsigned c[BMAX+1];		/* bit length count table */
    unsigned el;		/* length of EOB code (value 256) */
    unsigned f;			/* i repeats in table every f entries */
    int g;			/* maximum code length */
    int h;			/* table level */
    register unsigned i;	/* counter, current code */
    register unsigned j;	/* counter */
    register int k;		/* number of bits in current code */
    int lx[BMAX+1];		/* memory for l[-1..BMAX-1] */
    int *l = lx+1;		/* stack of bits per table */
    register unsigned *p;	/* pointer into c[], b[], or v[] */
    register struct huft *q;	/* points to current table */
    struct huft r;		/* table entry for structure assignment */
    struct huft *u[BMAX];	/* table stack */
    unsigned v[N_MAX];		/* values in order of bit length */
    register int w;		/* bits before this table == (l * h) */
    unsigned x[BMAX+1];		/* bit offsets, then code stack */
    unsigned *xp;		/* pointer into x */
    int y;			/* number of dummy codes added */
    unsigned z;			/* number of entries in current table */

    /* Generate counts for each bit length */
    el = n > 256 ? b[256] : BMAX; /* set length of EOB code, if any */
    memset(c, 0, sizeof(c));
    p = b;
    i = n;
    do
    {
	Tracecv(*p, (stderr, (n-i >= ' ' && n-i <= '~' ? "%c %d\n" :
			      "0x%x %d\n"), n-i, *p));
	c[*p]++;	/* assume all entries <= BMAX */
	p++;		/* Can't combine with above line (Solaris bug) */
    } while(--i);
    if(c[0] == n)	/* null input--all zero length codes */
    {
	*t = (struct huft *)NULL;
	*m = 0;
	return 0;
    }

    /* Find minimum and maximum length, bound *m by those */
    for(j = 1; j <= BMAX; j++)
	if(c[j])
	    break;
    k = j;			/* minimum code length */
    if((unsigned)*m < j)
	*m = j;
    for(i = BMAX; i; i--)
	if(c[i])
	    break;
    g = i;			/* maximum code length */
    if((unsigned)*m > i)
	*m = i;

    /* Adjust last length count to fill out codes, if needed */
    for(y = 1 << j; j < i; j++, y <<= 1)
	if((y -= c[j]) < 0)
	    return 2;		/* bad input: more codes than bits */
    if((y -= c[i]) < 0)
	return 2;
    c[i] += y;

    /* Generate starting offsets into the value table for each length */
    x[1] = j = 0;
    p = c + 1;  xp = x + 2;
    while(--i)			/* note that i == g from above */
	*xp++ = (j += *p++);

    /* Make a table of values in order of bit lengths */
    memset(v, 0, sizeof(v));
    p = b;
    i = 0;
    do
    {
	if((j = *p++) != 0)
	    v[x[j]++] = i;
    } while(++i < n);
    n = x[g];			/* set n to length of v */

    /* Generate the Huffman codes and for each, make the table entries */
    x[0] = i = 0;		/* first Huffman code is zero */
    p = v;			/* grab values in bit order */
    h = -1;			/* no tables yet--level -1 */
    w = l[-1] = 0;		/* no bits decoded yet */
    u[0] = (struct huft *)NULL;	/* just to keep compilers happy */
    q = (struct huft *)NULL;	/* ditto */
    z = 0;			/* ditto */

    /* go through the bit lengths (k already is bits in shortest code) */
    for(; k <= g; k++)
    {
	a = c[k];
	while(a--)
	{
	    /* here i is the Huffman code of length k bits for value *p */
	    /* make tables up to required level */
	    while(k > w + l[h])
	    {
		w += l[h++];	/* add bits already decoded */

		/* compute minimum size table less than or equal to *m bits */
		z = (z = g - w) > (unsigned)*m ? *m : z; /* upper limit */
		if((f = 1 << (j = k - w)) > a + 1) /* try a k-w bit table */
		{		/* too few codes for k-w bit table */
		    f -= a + 1;	/* deduct codes from patterns left */
		    xp = c + k;
		    while(++j < z)/* try smaller tables up to z bits */
		    {
			if((f <<= 1) <= *++xp)
			    break;	/* enough codes to use up j bits */
			f -= *xp;	/* else deduct codes from patterns */
		    }
		}
		if((unsigned)w + j > el && (unsigned)w < el)
		    j = el - w;	/* make EOB code end at table */
		z = 1 << j;	/* table entries for j-bit table */
		l[h] = j;	/* set table size in stack */

		/* allocate and link in new table */
		if(pool == NULL)
		    q = (struct huft *)malloc((z + 1)*sizeof(struct huft));
		else
		    q = (struct huft *)
			new_segment(pool, (z + 1)*sizeof(struct huft));
		if(q == NULL)
		{
		    if(h && pool == NULL)
			huft_free(u[0]);
		    return 3;	/* not enough memory */
		}

		*t = q + 1;	/* link to list for huft_free() */
		*(t = &(q->v.t)) = (struct huft *)NULL;
		u[h] = ++q;	/* table starts after link */

		/* connect to last table, if there is one */
		if(h)
		{
		    x[h] = i;		/* save pattern for backing up */
		    r.b = (uch)l[h-1];	/* bits to dump before this table */
		    r.e = (uch)(16 + j);/* bits in this table */
		    r.v.t = q;		/* pointer to this table */
		    j = (i & ((1 << w) - 1)) >> (w - l[h-1]);
		    u[h-1][j] = r;	/* connect to last table */
		}
	    }

	    /* set up table entry in r */
	    r.b = (uch)(k - w);
	    if(p >= v + n)
		r.e = 99;		/* out of values--invalid code */
	    else if(*p < s)
	    {
		r.e = (uch)(*p < 256 ? 16 : 15); /* 256 is end-of-block code */
		r.v.n = (ush)*p++;	/* simple code is just the value */
	    }
	    else
	    {
		r.e = (uch)e[*p - s];	/* non-simple--look up in lists */
		r.v.n = d[*p++ - s];
	    }

	    /* fill code-like entries with r */
	    f = 1 << (k - w);
	    for(j = i >> w; j < z; j += f)
		q[j] = r;

	    /* backwards increment the k-bit code i */
	    for(j = 1 << (k - 1); i & j; j >>= 1)
		i ^= j;
	    i ^= j;

	    /* backup over finished tables */
	    while((i & ((1 << w) - 1)) != x[h])
		w -= l[--h];		/* don't need to update q */
	}
    }

    /* return actual size of base table */
    *m = l[0];

    /* Return true (1) if we were given an incomplete table */
    return y != 0 && g != 1;
}

local int huft_free(struct huft *t)
/* Free the malloc'ed tables built by huft_build(), which makes a linked
   list of the tables it made, with the links in a dummy first entry of
   each table. */
{
    register struct huft *p, *q;

    /* Go through linked list, freeing from the malloced (t[-1]) address. */
    p = t;
    while(p != (struct huft *)NULL)
    {
	q = (--p)->v.t;
	free((char*)p);
	p = q;
    }
    return 0;
}

local long inflate_codes(InflateHandler decoder, char *buff, long size)
/* inflate (decompress) the codes in a deflated (compressed) block.
   Return an error code or zero if it all goes ok. */
{
    register unsigned e;/* table entry flag/number of extra bits */
    struct huft *t;	/* pointer to table entry */
    int n;
    struct huft *tl, *td;/* literal/length and distance decoder tables */
    int bl, bd;		/* number of bits decoded by tl[] and td[] */
    unsigned l, w, d;
    uch *slide;

    BITS_SAVE;

    if(size == 0)
	return 0;

    slide = decoder->slide;
    tl = decoder->tl;
    td = decoder->td;
    bl = decoder->bl;
    bd = decoder->bd;

#ifdef DEBUG
    if(decoder->copy_leng != 0)
    {
	fprintf(stderr, "What ? (decoder->copy_leng = %d)\n",
		decoder->copy_leng);
	abort();
    }
#endif /* DEBUG */
    w = decoder->wp;

    /* inflate the coded data */
    n = 0;
    for(;;)			/* do until end of block */
    {
	NEEDBITS((unsigned)bl);
	t = tl + GETBITS(bl);
	e = t->e;
	while(e > 16)
	{
	    if(e == 99)
		return -1;
	    DUMPBITS(t->b);
	    e -= 16;
	    NEEDBITS(e);
	    t = t->v.t + GETBITS(e);
	    e = t->e;
	}
	DUMPBITS(t->b);

	if(e == 16)		/* then it's a literal */
	{
	    w &= WSIZE - 1;
	    buff[n++] = slide[w++] = (uch)t->v.n;
	    if(n == size)
	    {
		decoder->wp = w;
		BITS_RESTORE;
		return size;
	    }
	    continue;
	}

	/* exit if end of block */
	if(e == 15)
	    break;

	/* it's an EOB or a length */

	/* get length of block to copy */
	NEEDBITS(e);
	l = t->v.n + GETBITS(e);
	DUMPBITS(e);

	/* decode distance of block to copy */
	NEEDBITS((unsigned)bd);
	t = td + GETBITS(bd);
	e = t->e;
	while(e > 16)
	{
	    if(e == 99)
		return -1;
	    DUMPBITS(t->b);
	    e -= 16;
	    NEEDBITS(e);
	    t = t->v.t + GETBITS(e);
	    e = t->e;
	}
	DUMPBITS(t->b);
	NEEDBITS(e);
	d = w - t->v.n - GETBITS(e);
	DUMPBITS(e);

	/* do the copy */
	while(l > 0 && n < size)
	{
	    l--;
	    d &= WSIZE - 1;
	    w &= WSIZE - 1;
	    buff[n++] = slide[w++] = slide[d++];
	}

	if(n == size)
	{
	    decoder->copy_leng = l;
	    decoder->wp = w;
	    decoder->copy_dist = d;
	    BITS_RESTORE;
	    return n;
	}
    }

    decoder->wp = w;
    decoder->method = -1; /* done */
    BITS_RESTORE;
    return n;
}

local long inflate_stored(InflateHandler decoder, char *buff, long size)
/* "decompress" an inflated type 0 (stored) block. */
{
    unsigned n, l, w;
    BITS_SAVE;

    /* go to byte boundary */
    n = bit_len & 7;
    DUMPBITS(n);

    /* get the length and its complement */
    NEEDBITS(16);
    n = GETBITS(16);
    DUMPBITS(16);
    NEEDBITS(16);
    if(n != (unsigned)((~bit_buf) & 0xffff))
    {
	BITS_RESTORE;
	return -1;			/* error in compressed data */
    }
    DUMPBITS(16);

    /* read and output the compressed data */
    decoder->copy_leng = n;

    n = 0;
    l = decoder->copy_leng;
    w = decoder->wp;
    while(l > 0 && n < size)
    {
	l--;
	w &= WSIZE - 1;
	NEEDBITS(8);
	buff[n++] = decoder->slide[w++] = (uch)GETBITS(8);
	DUMPBITS(8);
    }
    if(l == 0)
	decoder->method = -1; /* done */
    decoder->copy_leng = l;
    decoder->wp = w;
    BITS_RESTORE;
    return (long)n;
}

local long inflate_fixed(InflateHandler decoder, char *buff, long size)
/* decompress an inflated type 1 (fixed Huffman codes) block.  We should
   either replace this with a custom decoder, or at least precompute the
   Huffman tables. */
{
    /* if first time, set up tables for fixed blocks */
    if(decoder->fixed_tl == NULL)
    {
	int i;		  /* temporary variable */
	unsigned l[288];	  /* length list for huft_build */

	/* literal table */
	for(i = 0; i < 144; i++)
	    l[i] = 8;
	for(; i < 256; i++)
	    l[i] = 9;
	for(; i < 280; i++)
	    l[i] = 7;
	for(; i < 288; i++)	  /* make a complete, but wrong code set */
	    l[i] = 8;
	decoder->fixed_bl = 7;
	if((i = huft_build(l, 288, 257, cplens, cplext,
			   &decoder->fixed_tl, &decoder->fixed_bl, NULL))
	    != 0)
	{
	    decoder->fixed_tl = NULL;
	    return -1;
	}

	/* distance table */
	for(i = 0; i < 30; i++)	  /* make an incomplete code set */
	    l[i] = 5;
	decoder->fixed_bd = 5;
	if((i = huft_build(l, 30, 0, cpdist, cpdext,
			   &decoder->fixed_td, &decoder->fixed_bd, NULL)) > 1)
	{
	    huft_free(decoder->fixed_tl);
	    decoder->fixed_tl = NULL;
	    return -1;
	}
    }

    decoder->tl = decoder->fixed_tl;
    decoder->td = decoder->fixed_td;
    decoder->bl = decoder->fixed_bl;
    decoder->bd = decoder->fixed_bd;
    return inflate_codes(decoder, buff, size);
}

local long inflate_dynamic(InflateHandler decoder, char *buff, long size)
/* decompress an inflated type 2 (dynamic Huffman codes) block. */
{
    int i;		/* temporary variables */
    unsigned j;
    unsigned l;		/* last length */
    unsigned n;		/* number of lengths to get */
    struct huft *tl;	/* literal/length code table */
    struct huft *td;	/* distance code table */
    int bl;		/* lookup bits for tl */
    int bd;		/* lookup bits for td */
    unsigned nb;	/* number of bit length codes */
    unsigned nl;	/* number of literal/length codes */
    unsigned nd;	/* number of distance codes */
#ifdef PKZIP_BUG_WORKAROUND
    unsigned ll[288+32];/* literal/length and distance code lengths */
#else
    unsigned ll[286+30];/* literal/length and distance code lengths */
#endif
    static unsigned border[] = {  /* Order of the bit length code lengths */
	16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
    BITS_SAVE;

    reuse_mblock(&decoder->pool);

    /* read in table lengths */
    NEEDBITS(5);
    nl = 257 + GETBITS(5);	/* number of literal/length codes */
    DUMPBITS(5);
    NEEDBITS(5);
    nd = 1 + GETBITS(5);	/* number of distance codes */
    DUMPBITS(5);
    NEEDBITS(4);
    nb = 4 + GETBITS(4);	/* number of bit length codes */
    DUMPBITS(4);
#ifdef PKZIP_BUG_WORKAROUND
    if(nl > 288 || nd > 32)
#else
    if(nl > 286 || nd > 30)
#endif
    {
	BITS_RESTORE;
	return -1;		/* bad lengths */
    }

    /* read in bit-length-code lengths */
    for(j = 0; j < nb; j++)
    {
	NEEDBITS(3);
	ll[border[j]] = GETBITS(3);
	DUMPBITS(3);
    }
    for(; j < 19; j++)
	ll[border[j]] = 0;

    /* build decoding table for trees--single level, 7 bit lookup */
    bl = 7;
    if((i = huft_build(ll, 19, 19, NULL, NULL, &tl, &bl, &decoder->pool)) != 0)
    {
	reuse_mblock(&decoder->pool);
	BITS_RESTORE;
	return -1;		/* incomplete code set */
    }

    /* read in literal and distance code lengths */
    n = nl + nd;
    i = l = 0;
    while((unsigned)i < n)
    {
	NEEDBITS((unsigned)bl);
	j = (td = tl + (GETBITS(bl)))->b;
	DUMPBITS(j);
	j = td->v.n;
	if(j < 16)		/* length of code in bits (0..15) */
	    ll[i++] = l = j;	/* save last length in l */
	else if(j == 16)	/* repeat last length 3 to 6 times */
	{
	    NEEDBITS(2);
	    j = 3 + GETBITS(2);
	    DUMPBITS(2);
	    if((unsigned)i + j > n)
	    {
		BITS_RESTORE;
		return -1;
	    }
	    while(j--)
		ll[i++] = l;
	}
	else if(j == 17)	/* 3 to 10 zero length codes */
	{
	    NEEDBITS(3);
	    j = 3 + GETBITS(3);
	    DUMPBITS(3);
	    if((unsigned)i + j > n)
	    {
		BITS_RESTORE;
		return -1;
	    }
	    while(j--)
		ll[i++] = 0;
	    l = 0;
	}
	else			/* j == 18: 11 to 138 zero length codes */
	{
	    NEEDBITS(7);
	    j = 11 + GETBITS(7);
	    DUMPBITS(7);
	    if((unsigned)i + j > n)
	    {
		BITS_RESTORE;
		return -1;
	    }
	    while(j--)
		ll[i++] = 0;
	    l = 0;
	}
    }

    BITS_RESTORE;

    /* free decoding table for trees */
    reuse_mblock(&decoder->pool);

    /* build the decoding tables for literal/length and distance codes */
    bl = lbits;
    i = huft_build(ll, nl, 257, cplens, cplext, &tl, &bl, &decoder->pool);
    if(bl == 0)			      /* no literals or lengths */
      i = 1;
    if(i)
    {
	if(i == 1)
	    fprintf(stderr, " incomplete literal tree\n");
	reuse_mblock(&decoder->pool);
	return -1;		/* incomplete code set */
    }
    bd = dbits;
    i = huft_build(ll + nl, nd, 0, cpdist, cpdext, &td, &bd, &decoder->pool);
    if(bd == 0 && nl > 257)    /* lengths but no distances */
    {
	fprintf(stderr, " incomplete distance tree\n");
	reuse_mblock(&decoder->pool);
	return -1;
    }

    if(i == 1) {
#ifdef PKZIP_BUG_WORKAROUND
	i = 0;
#else
	fprintf(stderr, " incomplete distance tree\n");
#endif
    }
    if(i)
    {
	reuse_mblock(&decoder->pool);
	return -1;
    }

    /* decompress until an end-of-block code */
    decoder->tl = tl;
    decoder->td = td;
    decoder->bl = bl;
    decoder->bd = bd;

    i = inflate_codes(decoder, buff, size);

    if(i == -1) /* error */
    {
	reuse_mblock(&decoder->pool);
	return -1;
    }

    /* free the decoding tables, return */
    return i;
}

local void inflate_start(InflateHandler decoder)
/* initialize window, bit buffer */
{
    decoder->wp = 0;
    decoder->bit_buf = 0;
    decoder->bit_len = 0;
    decoder->insize = decoder->inptr = 0;
    decoder->fixed_td = decoder->fixed_tl = NULL;
    decoder->method = -1;
    decoder->eof = 0;
    decoder->copy_leng = decoder->copy_dist = 0;
    decoder->tl = NULL;

    init_mblock(&decoder->pool);
}

/*ARGSUSED*/
static long default_read_func(char *buf, long size, void *v)
{
    return (long)fread(buf, 1, size, stdin);
}

InflateHandler open_inflate_handler(
    long (* read_func)(char *buf, long size, void *user_val),
    void *user_val)
{
    InflateHandler decoder;

    decoder = (InflateHandler)
	malloc(sizeof(struct _InflateHandler));
    inflate_start(decoder);
    decoder->user_val = user_val;
    if(read_func == NULL)
	decoder->read_func = default_read_func;
    else
	decoder->read_func = read_func;
    return decoder;
}

void close_inflate_handler(InflateHandler decoder)
{
    if(decoder->fixed_tl != NULL)
    {
	huft_free(decoder->fixed_td);
	huft_free(decoder->fixed_tl);
	decoder->fixed_td = decoder->fixed_tl = NULL;
    }
    reuse_mblock(&decoder->pool);
    free(decoder);
}

/* decompress an inflated entry */
long zip_inflate(
    InflateHandler decoder,
    char *buff,
    long size)
{
    long n, i;

    n = 0;
    while(n < size)
    {
	if(decoder->eof && decoder->method == -1)
	    return n;

	if(decoder->copy_leng > 0)
	{
	    unsigned l, w, d;

	    l = decoder->copy_leng;
	    w = decoder->wp;
	    if(decoder->method != STORED_BLOCK)
	    {
		/* STATIC_TREES or DYN_TREES */
		d = decoder->copy_dist;
		while(l > 0 && n < size)
		{
		    l--;
		    d &= WSIZE - 1;
		    w &= WSIZE - 1;
		    buff[n++] = decoder->slide[w++] = decoder->slide[d++];
		}
		decoder->copy_dist = d;
	    }
	    else /* STATIC_TREES or DYN_TREES */
	    {
		BITS_SAVE;
		while(l > 0 && n < size)
		{
		    l--;
		    w &= WSIZE - 1;
		    NEEDBITS(8);
		    buff[n++] = decoder->slide[w++] = (uch)GETBITS(8);
		    DUMPBITS(8);
		}
		BITS_RESTORE;
		if(l == 0)
		    decoder->method = -1; /* done */
	    }
	    decoder->copy_leng = l;
	    decoder->wp = w;
	    if(n == size)
		return n;
	}

	if(decoder->method == -1)
	{
	    BITS_SAVE;
	    if(decoder->eof)
	    {
		BITS_RESTORE;
		break;
	    }
	    /* read in last block bit */
	    NEEDBITS(1);
	    if(GETBITS(1))
		decoder->eof = 1;
	    DUMPBITS(1);

	    /* read in block type */
	    NEEDBITS(2);
	    decoder->method = (int)GETBITS(2);
	    DUMPBITS(2);
	    decoder->tl = NULL;
	    decoder->copy_leng = 0;
	    BITS_RESTORE;
	}

	switch(decoder->method)
	{
	  case STORED_BLOCK:
	    i = inflate_stored(decoder, buff + n, size - n);
	    break;

	  case STATIC_TREES:
	    if(decoder->tl != NULL)
		i = inflate_codes(decoder, buff + n, size - n);
	    else
		i = inflate_fixed(decoder, buff + n, size - n);
	    break;

	  case DYN_TREES:
	    if(decoder->tl != NULL)
		i = inflate_codes(decoder, buff + n, size - n);
	    else
		i = inflate_dynamic(decoder, buff + n, size - n);
	    break;

	  default: /* error */
	    i = -1;
	    break;
	}

	if(i == -1)
	{
	    if(decoder->eof)
		return 0;
	    return -1; /* error */
	}
	n += i;
    }
    return n;
}

/* ===========================================================================
 * Fill the input buffer. This is called only when the buffer is empty.
 */
local int fill_inbuf(InflateHandler decoder)
{
    int len;

    /* Read as much as possible */
    decoder->insize = 0;
    errno = 0;
    do {
	len = decoder->read_func((char*)decoder->inbuf + decoder->insize,
				 (long)(INBUFSIZ - decoder->insize),
				 decoder->user_val);
	if(len == 0 || len == EOF) break;
	decoder->insize += len;
    } while(decoder->insize < INBUFSIZ);

    if(decoder->insize == 0)
	return EOF;
    decoder->inptr = 1;
    return decoder->inbuf[0];
}
