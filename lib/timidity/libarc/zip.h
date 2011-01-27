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

#ifndef ___ZIP_H_
#define ___ZIP_H_

/* zip.h -- common declarations for deflate/inflate routine */

#define INBUF_EXTRA  64
#define OUTBUF_EXTRA 2048

#ifdef SMALL_MEM
#  define INBUFSIZ  8192	/* input buffer size */
#  define OUTBUFSIZ 8192	/* output buffer size */
#else
#  define INBUFSIZ  32768	/* input buffer size */
#  define OUTBUFSIZ 16384	/* output buffer size */
#endif

typedef unsigned char  uch;
typedef unsigned short ush;
typedef unsigned long  ulg;

/* Huffman code lookup table entry--this entry is four bytes for machines
   that have 16-bit pointers (e.g. PC's in the small or medium model).
   Valid extra bits are 0..13.	e == 15 is EOB (end of block), e == 16
   means that v is a literal, 16 < e < 32 means that v is a pointer to
   the next table, which codes e - 16 bits, and lastly e == 99 indicates
   an unused code.  If a code with e == 99 is looked up, this implies an
   error in the data. */
struct huft {
    uch e;		/* number of extra bits or operation */
    uch b;		/* number of bits in this code or subcode */
    union {
	ush n;		/* literal, length base, or distance base */
	struct huft *t;	/* pointer to next level of table */
    } v;
};

int huft_build(unsigned *, unsigned, unsigned, ush *, ush *,
	       struct huft **, int *, MBlockList *pool);

#define STORED_BLOCK 0
#define STATIC_TREES 1
#define DYN_TREES    2
/* The three kinds of block type */

#define WSIZE 32768	/* window size--must be a power of two, and */
			/*  at least 32K for zip's deflate method */

/* Diagnostic functions */
#ifdef DEBUG
#  define Trace(x) fprintf x
#  define Tracev(x) {fprintf x ;}
#  define Tracevv(x) {fprintf x ;}
#  define Tracec(c,x) {fprintf x ;}
#  define Tracecv(c,x) {fprintf x ;}
#else
#  define Trace(x)
#  define Tracev(x)
#  define Tracevv(x)
#  define Tracec(c,x)
#  define Tracecv(c,x)
#endif

#define near

typedef struct _InflateHandler *InflateHandler;
typedef struct _DeflateHandler *DeflateHandler;

/*****************************************************************/
/*                      INTERFACE FUNCTIONS                      */
/*****************************************************************/

/* in deflate.c */
extern DeflateHandler open_deflate_handler(
	long (* read_func)(char *buf, long size, void *user_val),
	void *user_val,
	int compression_level);

extern long zip_deflate(DeflateHandler encoder,
		    char *decode_buff,
		    long decode_buff_size);

extern void close_deflate_handler(DeflateHandler encoder);


/* in inflate.c */
extern InflateHandler open_inflate_handler(
	long (* read_func)(char *buf, long size, void *user_val),
	void *user_val);

extern long zip_inflate(InflateHandler decoder,
		    char *decode_buff,
		    long decode_buff_size);

extern void close_inflate_handler(InflateHandler decoder);

#endif /* ___ZIP_H_ */
