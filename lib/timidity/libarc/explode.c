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
/* explode.c -- put in the public domain by Mark Adler
   version c15, 6 July 1996 */


/* You can do whatever you like with this source file, though I would
   prefer that if you modify it and redistribute it that you include
   comments to that effect with your name and the date.  Thank you.

   History:
   vers    date          who           what
   ----  ---------  --------------  ------------------------------------
    c1   30 Mar 92  M. Adler        explode that uses huft_build from inflate
                                    (this gives over a 70% speed improvement
                                    over the original unimplode.c, which
                                    decoded a bit at a time)
    c2    4 Apr 92  M. Adler        fixed bug for file sizes a multiple of 32k.
    c3   10 Apr 92  M. Adler        added a little memory tracking if DEBUG
    c4   11 Apr 92  M. Adler        added NOMEMCPY do kill use of memcpy()
    c5   21 Apr 92  M. Adler        added the WSIZE #define to allow reducing
                                    the 32K window size for specialized
                                    applications.
    c6   31 May 92  M. Adler        added typecasts to eliminate some warnings
    c7   27 Jun 92  G. Roelofs      added more typecasts.
    c8   17 Oct 92  G. Roelofs      changed ULONG/UWORD/byte to ulg/ush/uch.
    c9   19 Jul 93  J. Bush         added more typecasts (to return values);
                                    made l[256] array static for Amiga.
    c10   8 Oct 93  G. Roelofs      added used_csize for diagnostics; added
                                    buf and unshrink arguments to flush();
                                    undef'd various macros at end for Turbo C;
                                    removed NEXTBYTE macro (now in unzip.h)
                                    and bytebuf variable (not used); changed
                                    memset() to memzero().
    c11   9 Jan 94  M. Adler        fixed incorrect used_csize calculation.
    c12   9 Apr 94  G. Roelofs      fixed split comments on preprocessor lines
                                    to avoid bug in Encore compiler.
    c13  25 Aug 94  M. Adler        fixed distance-length comment (orig c9 fix)
    c14  22 Nov 95  S. Maxwell      removed unnecessary "static" on auto array
    c15   6 Jul 96  W. Haidinger    added ulg typecasts to flush() calls
 */


/*
   Explode imploded (PKZIP method 6 compressed) data.  This compression
   method searches for as much of the current string of bytes (up to a length
   of ~320) in the previous 4K or 8K bytes.  If it doesn't find any matches
   (of at least length 2 or 3), it codes the next byte.  Otherwise, it codes
   the length of the matched string and its distance backwards from the
   current position.  Single bytes ("literals") are preceded by a one (a
   single bit) and are either uncoded (the eight bits go directly into the
   compressed stream for a total of nine bits) or Huffman coded with a
   supplied literal code tree.  If literals are coded, then the minimum match
   length is three, otherwise it is two.

   There are therefore four kinds of imploded streams: 8K search with coded
   literals (min match = 3), 4K search with coded literals (min match = 3),
   8K with uncoded literals (min match = 2), and 4K with uncoded literals
   (min match = 2).  The kind of stream is identified in two bits of a
   general purpose bit flag that is outside of the compressed stream.

   Distance-length pairs for matched strings are preceded by a zero bit (to
   distinguish them from literals) and are always coded.  The distance comes
   first and is either the low six (4K) or low seven (8K) bits of the
   distance (uncoded), followed by the high six bits of the distance coded.
   Then the length is six bits coded (0..63 + min match length), and if the
   maximum such length is coded, then it's followed by another eight bits
   (uncoded) to be added to the coded length.  This gives a match length
   range of 2..320 or 3..321 bytes.

   The literal, length, and distance codes are all represented in a slightly
   compressed form themselves.  What is sent are the lengths of the codes for
   each value, which is sufficient to construct the codes.  Each byte of the
   code representation is the code length (the low four bits representing
   1..16), and the number of values sequentially with that length (the high
   four bits also representing 1..16).  There are 256 literal code values (if
   literals are coded), 64 length code values, and 64 distance code values,
   in that order at the beginning of the compressed stream.  Each set of code
   values is preceded (redundantly) with a byte indicating how many bytes are
   in the code description that follows, in the range 1..256.

   The codes themselves are decoded using tables made by huft_build() from
   the bit lengths.  That routine and its comments are in the inflate.c
   module.
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
#include "explode.h"
#include "zip.h"

struct _ExplodeHandler
{
    void *user_val;
    long (* read_func)(char *buf, long size, void *user_val);
    int method;

    int initflag;
    unsigned insize;	/* valid bytes in inbuf */
    unsigned inptr;	/* index of next byte to be processed in inbuf */
    uch inbuf[INBUFSIZ];

    ulg bit_buf;	/* bit buffer */
    ulg bit_len;	/* bits in bit buffer */

    uch slide[WSIZE];
    struct huft *tb;		/* literal, length, and distance tables */
    struct huft *tl;
    struct huft *td;
    int bb;			/* number of bits decoded by those */
    int bl;
    int bd;

    unsigned u, n, d, w;
    long s;			/* original size */
    long csize;			/* compressed size */
    unsigned l[256];		/* bit lengths for codes */

    MBlockList pool;

    int eof;
};

/* routines here */
static int get_tree(ExplodeHandler decoder, unsigned *l, unsigned n);
static int fill_inbuf(ExplodeHandler decoder);
static long explode_lit8(ExplodeHandler decoder,  char *buff, long size);
static long explode_lit4(ExplodeHandler decoder,  char *buff, long size);
static long explode_nolit8(ExplodeHandler decoder,char *buff, long size);
static long explode_nolit4(ExplodeHandler decoder,char *buff, long size);


/* The implode algorithm uses a sliding 4K or 8K byte window on the
   uncompressed stream to find repeated byte strings.  This is implemented
   here as a circular buffer.  The index is updated simply by incrementing
   and then and'ing with 0x0fff (4K-1) or 0x1fff (8K-1).  Here, the 32K
   buffer of inflate is used, and it works just as well to always have
   a 32K circular buffer, so the index is anded with 0x7fff.  This is
   done to allow the window to also be used as the output buffer. */
/* This must be supplied in an external module useable like "uch slide[8192];"
   or "uch *slide;", where the latter would be malloc'ed.  In unzip, slide[]
   is actually a 32K area for use by inflate, which uses a 32K sliding window.
 */


/* Tables for length and distance */
static ush cplen2[] =
        {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
        18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
        35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65};
static ush cplen3[] =
        {3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
        19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
        36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52,
        53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66};
static ush extra[] =
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        8};
static ush cpdist4[] =
        {1, 65, 129, 193, 257, 321, 385, 449, 513, 577, 641, 705,
        769, 833, 897, 961, 1025, 1089, 1153, 1217, 1281, 1345, 1409, 1473,
        1537, 1601, 1665, 1729, 1793, 1857, 1921, 1985, 2049, 2113, 2177,
        2241, 2305, 2369, 2433, 2497, 2561, 2625, 2689, 2753, 2817, 2881,
        2945, 3009, 3073, 3137, 3201, 3265, 3329, 3393, 3457, 3521, 3585,
        3649, 3713, 3777, 3841, 3905, 3969, 4033};
static ush cpdist8[] =
        {1, 129, 257, 385, 513, 641, 769, 897, 1025, 1153, 1281,
        1409, 1537, 1665, 1793, 1921, 2049, 2177, 2305, 2433, 2561, 2689,
        2817, 2945, 3073, 3201, 3329, 3457, 3585, 3713, 3841, 3969, 4097,
        4225, 4353, 4481, 4609, 4737, 4865, 4993, 5121, 5249, 5377, 5505,
        5633, 5761, 5889, 6017, 6145, 6273, 6401, 6529, 6657, 6785, 6913,
        7041, 7169, 7297, 7425, 7553, 7681, 7809, 7937, 8065};


/* Macros for inflate() bit peeking and grabbing.
   The usage is:

        NEEDBITS(j)
        x = GETBITS(j)
        DUMPBITS(j)

   where NEEDBITS makes sure that b has at least j bits in it, and
   DUMPBITS removes the bits from b.  The macros use the variable k
   for the number of bits in b.  Normally, b and k are register
   variables for speed.
 */

#define NEXTBYTE (decoder->inptr < decoder->insize ? decoder->inbuf[decoder->inptr++] : fill_inbuf(decoder))
#define MASK_BITS(n) ((((ulg)1)<<(n))-1)
#define NEEDBITS(n) {while(decoder->bit_len<(n)){decoder->bit_buf|=((ulg)NEXTBYTE)<<decoder->bit_len;decoder->bit_len+=8;}}
#define GETBITS(n)  ((ulg)decoder->bit_buf & MASK_BITS(n))
#define IGETBITS(n)  ((~((ulg)decoder->bit_buf)) & MASK_BITS(n))
#define DUMPBITS(n) {decoder->bit_buf>>=(n);decoder->bit_len-=(n);}


/*ARGSUSED*/
static long default_read_func(char *buf, long size, void *v)
{
    return (long)fread(buf, 1, size, stdin);
}

ExplodeHandler open_explode_handler(
	long (* read_func)(char *buf, long size, void *user_val),
	int method,
	long compsize, long origsize,
	void *user_val)
{
    ExplodeHandler decoder;

    decoder = (ExplodeHandler)malloc(sizeof(struct _ExplodeHandler));
    if(decoder == NULL)
	return NULL;
    memset(decoder, 0, sizeof(struct _ExplodeHandler));

    decoder->user_val = user_val;
    if(read_func == NULL)
	decoder->read_func = default_read_func;
    else
	decoder->read_func = read_func;
    decoder->insize = 0;
    decoder->method = method;
    decoder->bit_buf = 0;
    decoder->bit_len = 0;

    decoder->u = 1;
    decoder->n = 0;
    decoder->d = 0;
    decoder->w = 0;
    decoder->s = origsize;

    decoder->csize  = compsize;
    decoder->eof = 0;
    decoder->initflag = 0;
    init_mblock(&decoder->pool);

  /* Tune base table sizes.  Note: I thought that to truly optimize speed,
     I would have to select different bl, bd, and bb values for different
     compressed file sizes.  I was suprised to find out the the values of
     7, 7, and 9 worked best over a very wide range of sizes, except that
     bd = 8 worked marginally better for large compressed sizes. */
    decoder->bl = 7;
#if 0
    decoder->bd = (G.csize + G.incnt) > 200000L ? 8 : 7;
#else
    decoder->bd = (compsize > 200000L ? 8 : 7);
#endif

    return decoder;
}

static int explode_start(ExplodeHandler decoder)
{
    int method;

    method = decoder->method;

    /* With literal tree--minimum match length is 3 */
    if(method == EXPLODE_LIT8 || method == EXPLODE_LIT4)
    {
	decoder->bb = 9;	/* base table size for literals */
	if(get_tree(decoder, decoder->l, 256) != 0)
	    return 1;

	if(huft_build(decoder->l, 256, 256, NULL, NULL,
		      &decoder->tb, &decoder->bb, &decoder->pool) != 0)
	    return 1;

	if(get_tree(decoder, decoder->l, 64) != 0)
	    return 1;

	if(huft_build(decoder->l, 64, 0, cplen3, extra,
		      &decoder->tl, &decoder->bl, &decoder->pool) != 0)
	    return 1;

	if(get_tree(decoder, decoder->l, 64) != 0)
	    return 1;

	if(method == EXPLODE_LIT8)
	{
	    if(huft_build(decoder->l, 64, 0, cpdist8, extra,
			  &decoder->td, &decoder->bd, &decoder->pool) != 0)
		return 1;
	}
	else
	{
	    if(huft_build(decoder->l, 64, 0, cpdist4, extra,
			  &decoder->td, &decoder->bd, &decoder->pool) != 0)
		return 1;
	}
    }
    else /* EXPLODE_NOLIT8 or EXPLODE_NOLIT4 */
    {
	if(get_tree(decoder, decoder->l, 64) != 0)
	    return 1;

	if(huft_build(decoder->l, 64, 0, cplen2, extra,
		      &decoder->tl, &decoder->bl, &decoder->pool) != 0)
	    return 1;

	if(get_tree(decoder, decoder->l, 64) != 0)
	    return 1;

	if(method == EXPLODE_NOLIT8)
	{
	    if(huft_build(decoder->l, 64, 0, cpdist8, extra,
			  &decoder->td, &decoder->bd, &decoder->pool) != 0)
		return 1;
	}
	else
	{
	    if(huft_build(decoder->l, 64, 0, cpdist4, extra,
			  &decoder->td, &decoder->bd, &decoder->pool) != 0)
		return 1;
	}
    }

    return 0;
}


void close_explode_handler(ExplodeHandler decoder)
{
    free(decoder);
}


static int get_tree(
    ExplodeHandler decoder,
    unsigned *l,		/* bit lengths */
    unsigned n)			/* number expected */
/* Get the bit lengths for a code representation from the compressed
   stream.  If get_tree() returns 4, then there is an error in the data.
   Otherwise zero is returned. */
{
    unsigned i;			/* bytes remaining in list */
    unsigned k;			/* lengths entered */
    unsigned j;			/* number of codes */
    unsigned b;			/* bit length for those codes */

    /* get bit lengths */
    i = NEXTBYTE + 1;		/* length/count pairs to read */
    k = 0;			/* next code */
    do {
	b = ((j = NEXTBYTE) & 0xf) + 1;	/* bits in code (1..16) */
	j = ((j & 0xf0) >> 4) + 1; /* codes with those bits (1..16) */
	if (k + j > n)
	    return 4;		/* don't overflow l[] */
	do {
	    l[k++] = b;
	} while (--j);
    } while (--i);

    return k != n ? 4 : 0;	/* should have read n of them */
}



static long explode_lit8(ExplodeHandler decoder, char *buff, long size)
/* Decompress the imploded data using coded literals and an 8K sliding
   window. */
{
    long s;			/* bytes to decompress */
    register unsigned e;	/* table entry flag/number of extra bits */
    unsigned n, d;		/* length and index for copy */
    unsigned w;			/* current window position */
    struct huft *t;		/* pointer to table entry */
    unsigned u;			/* true if unflushed */
    long j;
    struct huft *tb, *tl, *td;	/* literal, length, and distance tables */
    int bb, bl, bd;		/* number of bits decoded by those */

    tb = decoder->tb;
    tl = decoder->tl;
    td = decoder->td;
    bb = decoder->bb;
    bl = decoder->bl;
    bd = decoder->bd;

    /* explode the coded data */
    s = decoder->s;
    w = decoder->w;
    u = decoder->u;
    j = 0;

    while(s > 0)		/* do until ucsize bytes uncompressed */
    {
	NEEDBITS(1);
	if(decoder->bit_buf & 1) /* then literal--decode it */
	{
	    DUMPBITS(1);
	    s--;
	    NEEDBITS((unsigned)bb); /* get coded literal */
	    t = tb + IGETBITS(bb);
	    e = t->e;
	    while(e > 16)
	    {
		if(e == 99)
		    return -1;
		DUMPBITS(t->b);
		e -= 16;
		NEEDBITS(e);
		t = t->v.t + IGETBITS(e);
		e = t->e;
	    }
	    DUMPBITS(t->b);
	    buff[j++] = decoder->slide[w++] = (uch)t->v.n;
	    if(w == WSIZE)
		w = u = 0;
	    if(j == size)
	    {
		decoder->u = u;
		decoder->w = w;
		decoder->s = s;
		return size;
	    }
	}
	else			/* else distance/length */
	{
	    DUMPBITS(1);
	    NEEDBITS(7);		/* get distance low bits */
	    d = GETBITS(7);
	    DUMPBITS(7);
	    NEEDBITS((unsigned)bd);	/* get coded distance high bits */
	    t = td + IGETBITS(bd);
	    e = t->e;
	    while(e > 16)
	    {
		if(e == 99)
		    return -1;
		DUMPBITS(t->b);
		e -= 16;
		NEEDBITS(e);
		t = t->v.t + IGETBITS(e);
		e = t->e;
	    }
	    DUMPBITS(t->b);
	    d = w - d - t->v.n;       /* construct offset */
	    NEEDBITS((unsigned)bl);    /* get coded length */
	    t = tl + IGETBITS(bl);
	    e = t->e;
	    while(e > 16)
	    {
		if(e == 99)
		    return -1;
		DUMPBITS(t->b);
		e -= 16;
		NEEDBITS(e);
		t = t->v.t + IGETBITS(e);
		e = t->e;
	    }
	    DUMPBITS(t->b);
	    n = t->v.n;
	    if(e)                    /* get length extra bits */
	    {
		NEEDBITS(8);
		n += GETBITS(8);
		DUMPBITS(8);
	    }

	    /* do the copy */
	    s -= n;
	    while(n > 0 && j < size)
	    {
		n--;
		d &= WSIZE - 1;
		w &= WSIZE - 1;
		if(u && w <= d)
		{
		    buff[j++] = 0;
		    w++;
		    d++;
		}
		else
		    buff[j++] = decoder->slide[w++] = decoder->slide[d++];
		if(w == WSIZE)
		    w = u = 0;
	    }
	    if(j == size)
	    {
		decoder->u = u;
		decoder->n = n;
		decoder->d = d;
		decoder->w = w;
		decoder->s = s;
		return size;
	    }
	    decoder->n = 0;
	}
    }

    decoder->n = 0;
    decoder->w = 0;
    decoder->eof = 1;
    return j;
}



static long explode_lit4(ExplodeHandler decoder, char *buff, long size)
/* Decompress the imploded data using coded literals and a 4K sliding
   window. */
{
    long s;               /* bytes to decompress */
    register unsigned e;  /* table entry flag/number of extra bits */
    unsigned n, d;        /* length and index for copy */
    unsigned w;           /* current window position */
    struct huft *t;       /* pointer to table entry */
    unsigned u;           /* true if unflushed */
    long j;
    struct huft *tb, *tl, *td;	/* literal, length, and distance tables */
    int bb, bl, bd;		/* number of bits decoded by those */

    tb = decoder->tb;
    tl = decoder->tl;
    td = decoder->td;
    bb = decoder->bb;
    bl = decoder->bl;
    bd = decoder->bd;

  /* explode the coded data */
    s = decoder->s;
    w = decoder->w;
    u = decoder->u;
    j = 0;

    while(s > 0)                 /* do until ucsize bytes uncompressed */
    {
	NEEDBITS(1);
	if(decoder->bit_buf & 1)                  /* then literal--decode it */
	{
	    DUMPBITS(1);
	    s--;
	    NEEDBITS((unsigned)bb);    /* get coded literal */
	    t = tb + IGETBITS(bb);
	    e = t->e;
	    while(e > 16)
	    {
		if(e == 99)
		    return -1;
		DUMPBITS(t->b);
		e -= 16;
		NEEDBITS(e);
		t = t->v.t + IGETBITS(e);
	    }
	    DUMPBITS(t->b);
	    buff[j++] = decoder->slide[w++] = (uch)t->v.n;
	    if(w == WSIZE)
		w = u = 0;
	    if(j == size)
	    {
		decoder->u = u;
		decoder->w = w;
		decoder->s = s;
		return size;
	    }
	}
	else                        /* else distance/length */
	{
	    DUMPBITS(1);
	    NEEDBITS(6);               /* get distance low bits */
	    d = GETBITS(6);
	    DUMPBITS(6);
	    NEEDBITS((unsigned)bd);    /* get coded distance high bits */
	    t = td + IGETBITS(bd);
	    e = t->e;
	    while(e > 16)
	    {
		if(e == 99)
		    return -1;
		DUMPBITS(t->b);
		e -= 16;
		NEEDBITS(e);
		t = t->v.t + IGETBITS(e);
		e = t->e;
	    }
	    DUMPBITS(t->b);
	    d = w - d - t->v.n;       /* construct offset */
	    NEEDBITS((unsigned)bl);    /* get coded length */
	    t = tl + IGETBITS(bl);
	    e = t->e;
	    while(e > 16)
	    {
		if(e == 99)
		    return -1;
		DUMPBITS(t->b);
		e -= 16;
		NEEDBITS(e);
		t = t->v.t + IGETBITS(e);
		e = t->e;
	    }
	    DUMPBITS(t->b);
	    n = t->v.n;
	    if(e)                    /* get length extra bits */
	    {
		NEEDBITS(8);
		n += GETBITS(8);
		DUMPBITS(8);
	    }

	    /* do the copy */
	    s -= n;
	    while(n > 0 && j < size)
	    {
		n--;
		d &= WSIZE - 1;
		w &= WSIZE - 1;
		if(u && w <= d)
		{
		    buff[j++] = 0;
		    w++;
		    d++;
		}
		else
		    buff[j++] = decoder->slide[w++] = decoder->slide[d++];
		if(w == WSIZE)
		    w = u = 0;
	    }
	    if(j == size)
	    {
		decoder->u = u;
		decoder->n = n;
		decoder->d = d;
		decoder->w = w;
		decoder->s = s;
		return size;
	    }
	    decoder->n = 0;
	}
    }

    decoder->n = 0;
    decoder->w = 0;
    decoder->eof = 1;
    return j;
}



static long explode_nolit8(ExplodeHandler decoder, char *buff, long size)
/* Decompress the imploded data using uncoded literals and an 8K sliding
   window. */
{
    long s;               /* bytes to decompress */
    register unsigned e;  /* table entry flag/number of extra bits */
    unsigned n, d;        /* length and index for copy */
    unsigned w;           /* current window position */
    struct huft *t;       /* pointer to table entry */
    unsigned u;           /* true if unflushed */
    long j;
    struct huft *tl, *td;   /* length and distance decoder tables */
    int bl, bd;             /* number of bits decoded by tl[] and td[] */

    tl = decoder->tl;
    td = decoder->td;
    bl = decoder->bl;
    bd = decoder->bd;

  /* explode the coded data */
#if 0
  b = k = w = 0;                /* initialize bit buffer, window */
  u = 1;                        /* buffer unflushed */
  ml = mask_bits[bl];           /* precompute masks for speed */
  md = mask_bits[bd];
  s = G.ucsize;
#endif

    s = decoder->s;
    w = decoder->w;
    u = decoder->u;
    j = 0;

    while(s > 0)                 /* do until ucsize bytes uncompressed */
    {
	NEEDBITS(1);
	if(decoder->bit_buf & 1) /* then literal--get eight bits */
	{
	    DUMPBITS(1);
	    s--;
	    NEEDBITS(8);
	    buff[j++] = decoder->slide[w++] = (uch)decoder->bit_buf;;
	    DUMPBITS(8);
	    if(w == WSIZE)
		w = u = 0;
	    if(j == size)
	    {
		decoder->u = u;
		decoder->w = w;
		decoder->s = s;
		return size;
	    }
	}
	else                        /* else distance/length */
	{
	    DUMPBITS(1);
	    NEEDBITS(7);               /* get distance low bits */
	    d = GETBITS(7);
	    DUMPBITS(7);
	    NEEDBITS((unsigned)bd);    /* get coded distance high bits */
	    t = td + IGETBITS(bd);
	    e = t->e;
	    while(e > 16)
	    {
		if(e == 99)
		    return -1;
		DUMPBITS(t->b);
		e -= 16;
		NEEDBITS(e);
		t = t->v.t + IGETBITS(e);
		e = t->e;
	    }
	    DUMPBITS(t->b);
	    d = w - d - t->v.n;       /* construct offset */
	    NEEDBITS((unsigned)bl);    /* get coded length */
	    t = tl + IGETBITS(bl);
	    e = t->e;
	    while(e > 16)
	    {
		if(e == 99)
		    return -1;
		DUMPBITS(t->b);
		e -= 16;
		NEEDBITS(e);
		t = t->v.t + IGETBITS(e);
		e = t->e;
	    }
	    DUMPBITS(t->b);
	    n = t->v.n;
	    if(e)                    /* get length extra bits */
	    {
		NEEDBITS(8);
		n += GETBITS(8);
		DUMPBITS(8);
	    }

	    /* do the copy */
	    s -= n;
	    while(n > 0 && j < size)
	    {
		n--;
		d &= WSIZE - 1;
		w &= WSIZE - 1;
		if(u && w <= d)
		{
		    buff[j++] = 0;
		    w++;
		    d++;
		}
		else
		    buff[j++] = decoder->slide[w++] = decoder->slide[d++];
		if(w == WSIZE)
		    w = u = 0;
	    }
	    if(j == size)
	    {
		decoder->u = u;
		decoder->n = n;
		decoder->d = d;
		decoder->w = w;
		decoder->s = s;
		return size;
	    }
	    decoder->n = 0;
	}
    }

    decoder->n = 0;
    decoder->w = 0;
    decoder->eof = 1;
    return j;
}



static long explode_nolit4(ExplodeHandler decoder, char *buff, long size)
/* Decompress the imploded data using uncoded literals and a 4K sliding
   window. */
{
    long s;               /* bytes to decompress */
    register unsigned e;  /* table entry flag/number of extra bits */
    unsigned n, d;        /* length and index for copy */
    unsigned w;           /* current window position */
    struct huft *t;       /* pointer to table entry */
    unsigned u;           /* true if unflushed */
    long j;
    struct huft *tl, *td;   /* length and distance decoder tables */
    int bl, bd;             /* number of bits decoded by tl[] and td[] */

    tl = decoder->tl;
    td = decoder->td;
    bl = decoder->bl;
    bd = decoder->bd;

  /* explode the coded data */
#if 0
  b = k = w = 0;                /* initialize bit buffer, window */
  u = 1;                        /* buffer unflushed */
  ml = mask_bits[bl];           /* precompute masks for speed */
  md = mask_bits[bd];
  s = G.ucsize;
#endif
    s = decoder->s;
    w = decoder->w;
    u = decoder->u;
    j = 0;

    while(s > 0)                 /* do until ucsize bytes uncompressed */
    {
	NEEDBITS(1);
	if(decoder->bit_buf & 1) /* then literal--get eight bits */
	{
	    DUMPBITS(1);
	    s--;
	    NEEDBITS(8);
	    buff[j++] = decoder->slide[w++] = (uch)decoder->bit_buf;
	    DUMPBITS(8);
	    if(w == WSIZE)
		w = u = 0;
	    if(j == size)
	    {
		decoder->u = u;
		decoder->w = w;
		decoder->s = s;
		return size;
	    }
	}
	else                        /* else distance/length */
	{
	    DUMPBITS(1);
	    NEEDBITS(6);               /* get distance low bits */
#if 0
	    d = (unsigned)b & 0x3f;
#else
	    d = GETBITS(6);
#endif
	    DUMPBITS(6);
	    NEEDBITS((unsigned)bd);    /* get coded distance high bits */
	    t = td + IGETBITS(bd);
	    e = t->e;
	    while(e > 16)
	    {
		if(e == 99)
		    return -1;
		DUMPBITS(t->b);
		e -= 16;
		NEEDBITS(e);
		t = t->v.t + IGETBITS(e);
		e = t->e;
	    }
	    DUMPBITS(t->b);
	    d = w - d - t->v.n;       /* construct offset */
	    NEEDBITS((unsigned)bl);    /* get coded length */
	    /*t =*/ t = tl + IGETBITS(bl);
	    e = t->e;
	    while(e > 16)
	    {
		if(e == 99)
		    return -1;
		DUMPBITS(t->b);
		e -= 16;
		NEEDBITS(e);
		t = t->v.t + IGETBITS(e);
		e = t->e;
	    }
	    DUMPBITS(t->b);
	    n = t->v.n;
	    if(e)                    /* get length extra bits */
	    {
		NEEDBITS(8);
		n += GETBITS(8);
		DUMPBITS(8);
	    }

	    /* do the copy */
	    s -= n;
	    while(n > 0 && j < size)
	    {
		n--;
		d &= WSIZE - 1;
		w &= WSIZE - 1;
		if(u && w <= d)
		{
		    buff[j++] = 0;
		    w++;
		    d++;
		}
		else
		    buff[j++] = decoder->slide[w++] = decoder->slide[d++];
		if(w == WSIZE)
		    w = u = 0;
	    }
	    if(j == size)
	    {
		decoder->u = u;
		decoder->n = n;
		decoder->d = d;
		decoder->w = w;
		decoder->s = s;
		return size;
	    }
	    decoder->n = 0;
	}
    }

    decoder->n = 0;
    decoder->w = 0;
    decoder->eof = 1;
    return j;
}



long explode(ExplodeHandler decoder, char *buff, long size)
/* Explode an imploded compressed stream.  Based on the general purpose
   bit flag, decide on coded or uncoded literals, and an 8K or 4K sliding
   window.  Construct the literal (if any), length, and distance codes and
   the tables needed to decode them (using huft_build() from inflate.c),
   and call the appropriate routine for the type of data in the remainder
   of the stream.  The four routines are nearly identical, differing only
   in whether the literal is decoded or simply read in, and in how many
   bits are read in, uncoded, for the low distance bits. */
{
    long j, i;

    if(size <= 0)
	return size;

    if(!decoder->initflag)
    {
	decoder->initflag = 1;
	if(explode_start(decoder) != 0)
	    return 0;
    }

    j = 0;
    while(j < size)
    {
	if(decoder->n > 0) /* do the copy */
	{
	    unsigned u, n, w, d;

	    u = decoder->u;
	    n = decoder->n;
	    d = decoder->d;
	    w = decoder->w;
	    while(n > 0 && j < size)
	    {
		n--;
		d &= WSIZE - 1;
		w &= WSIZE - 1;
		if(u && w <= d)
		{
		    buff[j++] = 0;
		    w++;
		    d++;
		}
		else
		    buff[j++] = decoder->slide[w++] = decoder->slide[d++];
		if(w == WSIZE)
		    w = u = 0;
	    }

	    decoder->u = u;
	    decoder->n = n;
	    decoder->d = d;
	    decoder->w = w;
	    if(j == size)
		return size;
	}

	/* decoder->n == 0 */
	if(decoder->eof)
	    return j;

	switch(decoder->method)
	{
	  case EXPLODE_LIT8:
	    i = explode_lit8(decoder, buff + j, size - j);
	    break;
	  case EXPLODE_LIT4:
	    i = explode_lit4(decoder, buff + j, size - j);
	    break;
	  case EXPLODE_NOLIT8:
	    i = explode_nolit8(decoder, buff + j, size - j);
	    break;
	  case EXPLODE_NOLIT4:
	    i = explode_nolit4(decoder, buff + j, size - j);
	    break;
	  default:
	    i = -1;
	    break;
	}
	if(i == -1)
	    return -1;
	j += i;
    }
    return j;
}



static int fill_inbuf(ExplodeHandler decoder)
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

