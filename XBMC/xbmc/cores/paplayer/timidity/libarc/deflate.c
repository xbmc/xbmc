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
/* deflate.c -- compress data using the deflation algorithm
 * Copyright (C) 1992-1993 Jean-loup Gailly
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License, see the file COPYING.
 */

/*
 *  PURPOSE
 *
 *	Identify new text as repetitions of old text within a fixed-
 *	length sliding window trailing behind the new text.
 *
 *  DISCUSSION
 *
 *	The "deflation" process depends on being able to identify portions
 *	of the input text which are identical to earlier input (within a
 *	sliding window trailing behind the input currently being processed).
 *
 *	The most straightforward technique turns out to be the fastest for
 *	most input files: try all possible matches and select the longest.
 *	The key feature of this algorithm is that insertions into the string
 *	dictionary are very simple and thus fast, and deletions are avoided
 *	completely. Insertions are performed at each input character, whereas
 *	string matches are performed only when the previous match ends. So it
 *	is preferable to spend more time in matches to allow very fast string
 *	insertions and avoid deletions. The matching algorithm for small
 *	strings is inspired from that of Rabin & Karp. A brute force approach
 *	is used to find longer strings when a small match has been found.
 *	A similar algorithm is used in comic (by Jan-Mark Wams) and freeze
 *	(by Leonid Broukhis).
 *	   A previous version of this file used a more sophisticated algorithm
 *	(by Fiala and Greene) which is guaranteed to run in linear amortized
 *	time, but has a larger average cost, uses more memory and is patented.
 *	However the F&G algorithm may be faster for some highly redundant
 *	files if the parameter max_chain_length (described below) is too large.
 *
 *  ACKNOWLEDGEMENTS
 *
 *	The idea of lazy evaluation of matches is due to Jan-Mark Wams, and
 *	I found it in 'freeze' written by Leonid Broukhis.
 *	Thanks to many info-zippers for bug reports and testing.
 *
 *  REFERENCES
 *
 *	APPNOTE.TXT documentation file in PKZIP 1.93a distribution.
 *
 *	A description of the Rabin and Karp algorithm is given in the book
 *	   "Algorithms" by R. Sedgewick, Addison-Wesley, p252.
 *
 *	Fiala,E.R., and Greene,D.H.
 *	   Data Compression with Finite Windows, Comm.ACM, 32,4 (1989) 490-595
 *
 *  INTERFACE
 *
 *	void lm_init (void)
 *	    Initialize the "longest match" routines for a new file
 *
 *	ulg deflate (void)
 *	    Processes a new input file and return its compressed length. Sets
 *	    the compressed length, crc, deflate flags and internal file
 *	    attributes.
 */

#include <stdio.h>
#include <stdlib.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <ctype.h>

#define FULL_SEARCH
/* #define UNALIGNED_OK */
/* #define DEBUG */
#include "timidity.h"
#include "common.h"
#include "mblock.h"
#include "zip.h"

#define local static

#define MIN_MATCH  3
#define MAX_MATCH  258
/* The minimum and maximum match lengths */

#define BITS 16

/* Compile with MEDIUM_MEM to reduce the memory requirements or
 * with SMALL_MEM to use as little memory as possible. Use BIG_MEM if the
 * entire input file can be held in memory (not possible on 16 bit systems).
 * Warning: defining these symbols affects HASH_BITS (see below) and thus
 * affects the compression ratio. The compressed output
 * is still correct, and might even be smaller in some cases.
 */
#ifdef SMALL_MEM
#  define LIT_BUFSIZE  0x2000
#  define HASH_BITS  13	 /* Number of bits used to hash strings */
#else
#ifdef MEDIUM_MEM
#  define LIT_BUFSIZE  0x4000
#  define HASH_BITS  14
#else
#  define LIT_BUFSIZE  0x8000
#  define HASH_BITS  15
/* For portability to 16 bit machines, do not use values above 15. */
#endif
#endif

#if LIT_BUFSIZE > INBUFSIZ
    error cannot overlay l_buf and inbuf
#endif
#if (WSIZE<<1) > (1<<BITS)
   error: cannot overlay window with tab_suffix and prev with tab_prefix0
#endif
#if HASH_BITS > BITS-1
   error: cannot overlay head with tab_prefix1
#endif

#define DIST_BUFSIZE  LIT_BUFSIZE
#define HASH_SIZE (unsigned)(1<<HASH_BITS)
#define HASH_MASK (HASH_SIZE-1)
#define WMASK	  (WSIZE-1)
/* HASH_SIZE and WSIZE must be powers of two */

#define NIL 0		/* Tail of hash chains */
#define EQUAL 0		/* result of memcmp for equal strings */

#define TOO_FAR 4096
/* Matches of length 3 are discarded if their distance exceeds TOO_FAR */

#define MIN_LOOKAHEAD (MAX_MATCH+MIN_MATCH+1)
/* Minimum amount of lookahead, except at the end of the input file.
 * See deflate.c for comments about the MIN_MATCH+1.
 */

#define MAX_DIST  (WSIZE-MIN_LOOKAHEAD)
/* In order to simplify the code, particularly on 16 bit machines, match
 * distances are limited to MAX_DIST instead of WSIZE.
 */

#define SMALLEST 1
/* Index within the heap array of least frequent node in the Huffman tree */
#define MAX_BITS 15 /* All codes must not exceed MAX_BITS bits */
#define MAX_BL_BITS 7 /* Bit length codes must not exceed MAX_BL_BITS bits */
#define LENGTH_CODES 29
/* number of length codes, not counting the special END_BLOCK code */
#define LITERALS  256 /* number of literal bytes 0..255 */
#define END_BLOCK 256 /* end of block literal code */
#define L_CODES (LITERALS+1+LENGTH_CODES)
/* number of Literal or Length codes, including the END_BLOCK code */
#define D_CODES	  30 /* number of distance codes */
#define BL_CODES  19 /* number of codes used to transfer the bit lengths */
#define REP_3_6	  16
/* repeat previous bit length 3-6 times (2 bits of repeat count) */
#define REPZ_3_10 17
/* repeat a zero length 3-10 times  (3 bits of repeat count) */
#define REPZ_11_138 18
/* repeat a zero length 11-138 times  (7 bits of repeat count) */
#define HEAP_SIZE (2*L_CODES+1)		/* maximum heap size */

#define H_SHIFT	 ((HASH_BITS+MIN_MATCH-1)/MIN_MATCH)
/* Number of bits by which ins_h and del_h must be shifted at each
 * input step. It must be such that after MIN_MATCH steps, the oldest
 * byte no longer takes part in the hash key, that is:
 *   H_SHIFT * MIN_MATCH >= HASH_BITS
 */

/* Data structure describing a single value and its code string. */
typedef struct ct_data {
    union {
	ush  freq;	 /* frequency count */
	ush  code;	 /* bit string */
    } fc;
    union {
	ush  dad;	 /* father node in Huffman tree */
	ush  len;	 /* length of bit string */
    } dl;
} ct_data;
#define Freq fc.freq
#define Code fc.code
#define Dad  dl.dad
#define Len  dl.len

typedef struct tree_desc {
    ct_data near *dyn_tree;	 /* the dynamic tree */
    ct_data near *static_tree;	 /* corresponding static tree or NULL */
    int	    near *extra_bits;	 /* extra bits for each code or NULL */
    int	    extra_base;		 /* base index for extra_bits */
    int	    elems;		 /* max number of elements in the tree */
    int	    max_length;		 /* max bit length for the codes */
    int	    max_code;		 /* largest code with non zero frequency */
} tree_desc;

local int near extra_lbits[LENGTH_CODES] /* extra bits for each length code */
   = {0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};
local int near extra_dbits[D_CODES] /* extra bits for each distance code */
   = {0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};
local int near extra_blbits[BL_CODES]/* extra bits for each bit length code */
   = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,3,7};

local uch near bl_order[BL_CODES]
   = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
/* The lengths of the bit length codes are sent in order of decreasing
 * probability, to avoid transmitting the lengths for unused bit length codes.
 */

/* Values for max_lazy_match, good_match and max_chain_length, depending on
 * the desired pack level (0..9). The values given below have been tuned to
 * exclude worst case performance for pathological files. Better values may be
 * found for specific files.
 */
local struct
{
   ush good_length; /* reduce lazy search above this match length */
   ush max_lazy;    /* do not perform lazy search above this match length */
   ush nice_length; /* quit search above this match length */
   ush max_chain;
} configuration_table[10] = {
/*	good lazy nice chain */
/* 0 */ {0,    0,  0,	 0},  /* store only */
/* 1 */ {4,    4,  8,	 4},  /* maximum speed, no lazy matches */
/* 2 */ {4,    5, 16,	 8},
/* 3 */ {4,    6, 32,	32},

/* 4 */ {4,    4, 16,	16},  /* lazy matches */
/* 5 */ {8,   16, 32,	32},
/* 6 */ {8,   16, 128, 128},
/* 7 */ {8,   32, 128, 256},
/* 8 */ {32, 128, 258, 1024},
/* 9 */ {32, 258, 258, 4096}}; /* maximum compression */

struct deflate_buff_queue
{
    struct deflate_buff_queue *next;
    unsigned len;
    uch *ptr;
};
local struct deflate_buff_queue *free_queue = NULL;

local void reuse_queue(struct deflate_buff_queue *p)
{
    p->next = free_queue;
    free_queue = p;
}
local struct deflate_buff_queue *new_queue(void)
{
    struct deflate_buff_queue *p;

    if(free_queue)
    {
	p = free_queue;
	free_queue = free_queue->next;
    }
    else
	p = (struct deflate_buff_queue *)
	    safe_malloc(sizeof(struct deflate_buff_queue) + OUTBUFSIZ);
    p->next = NULL;
    p->len = 0;
    p->ptr = (uch *)p + sizeof(struct deflate_buff_queue);

    return p;
}

struct _DeflateHandler
{
    void *user_val;
    long (* read_func)(char *buf, long size, void *user_val);

    int initflag;
    struct deflate_buff_queue *qhead;
    struct deflate_buff_queue *qtail;
    uch outbuf[OUTBUFSIZ];
    unsigned outcnt, outoff;
    int complete;

#define window_size ((ulg)2*WSIZE)
    uch window[window_size];
    ush d_buf[DIST_BUFSIZE];		 /* buffer for distances */
    uch l_buf[INBUFSIZ + INBUF_EXTRA]; /* buffer for literals or lengths */
    ush prev[1L<<BITS];
    unsigned short bi_buf;
    int bi_valid;

    long block_start;
/* window position at the beginning of the current output block. Gets
 * negative when the window is moved backwards.
 */

    unsigned ins_h;		/* hash index of string to be inserted */

    unsigned hash_head;		/* head of hash chain */
    unsigned prev_match;	/* previous match */
    int match_available;	/* set if previous match exists */
    unsigned match_length;	/* length of best match */
    unsigned int near prev_length;
/* Length of the best match at previous step. Matches not greater than this
 * are discarded. This is used in the lazy match evaluation.
 */

    unsigned near strstart;	/* start of string to insert */
    unsigned near match_start;	/* start of matching string */
    int		  eofile;	/* flag set at end of input file */
    unsigned	  lookahead;	/* number of valid bytes ahead in window */

    unsigned near max_chain_length;
/* To speed up deflation, hash chains are never searched beyond this length.
 * A higher limit improves compression ratio but degrades the speed.
 */

    unsigned int max_lazy_match;
/* Attempt to find a better match only when the current match is strictly
 * smaller than this value. This mechanism is used only for compression
 * levels >= 4.
 */

    int compr_level;		/* compression level (1..9) */

    unsigned near good_match;
/* Use a faster search when the previous match is longer than this */

#ifndef FULL_SEARCH
    int near nice_match; /* Stop searching when current match exceeds this */
#endif

    ct_data near dyn_ltree[HEAP_SIZE];	 /* literal and length tree */
    ct_data near dyn_dtree[2*D_CODES+1]; /* distance tree */
    ct_data near static_ltree[L_CODES+2];
/* The static literal tree. Since the bit lengths are imposed, there is no
 * need for the L_CODES extra codes used during heap construction. However
 * The codes 286 and 287 are needed to build a canonical tree (see ct_init
 * below).
 */

    ct_data near static_dtree[D_CODES];
/* The static distance tree. (Actually a trivial tree since all codes use
 * 5 bits.)
 */

    ct_data near bl_tree[2*BL_CODES+1];/* Huffman tree for the bit lengths */

    tree_desc near l_desc;
    tree_desc near d_desc;
    tree_desc near bl_desc;

    ush near bl_count[MAX_BITS+1];
/* number of codes at each bit length for an optimal tree */

    int near heap[2*L_CODES+1];	/* heap used to build the Huffman trees */
    int heap_len;		/* number of elements in the heap */
    int heap_max;		/* element of largest frequency */
/* The sons of heap[n] are heap[2*n] and heap[2*n+1]. heap[0] is not used.
 * The same heap array is used to build all trees.
 */

    uch near depth[2*L_CODES+1];
/* Depth of each subtree used as tie breaker for trees of equal frequency */

    uch length_code[MAX_MATCH-MIN_MATCH+1];
/* length code for each normalized match length (0 == MIN_MATCH) */

    uch dist_code[512];
/* distance codes. The first 256 values correspond to the distances
 * 3 .. 258, the last 256 values correspond to the top 8 bits of
 * the 15 bit distances.
 */

    int near base_length[LENGTH_CODES];
/* First normalized length for each code (0 = MIN_MATCH) */

    int near base_dist[D_CODES];
/* First normalized distance for each code (0 = distance of 1) */

    uch near flag_buf[(LIT_BUFSIZE/8)];
/* flag_buf is a bit array distinguishing literals from lengths in
 * l_buf, thus indicating the presence or absence of a distance.
 */

    unsigned last_lit;	/* running index in l_buf */
    unsigned last_dist;	/* running index in d_buf */
    unsigned last_flags;/* running index in flag_buf */
    uch flags;		/* current flags not yet saved in flag_buf */
    uch flag_bit;	/* current bit used in flags */
    ulg opt_len;	/* bit length of current block with optimal trees */
    ulg static_len;	/* bit length of current block with static trees */
};

local void lm_init(DeflateHandler);
local int  longest_match(DeflateHandler,unsigned cur_match);
local void fill_window(DeflateHandler);
local void deflate_fast(DeflateHandler);
local void deflate_better(DeflateHandler);
local long qcopy(DeflateHandler encoder, char *buff, long buff_size);
local void ct_init(DeflateHandler);
local void init_block(DeflateHandler);
local void pqdownheap(DeflateHandler,ct_data near *, int);
local void gen_bitlen(DeflateHandler,tree_desc near *);
local void gen_codes(DeflateHandler,ct_data near *, int);
local void build_tree(DeflateHandler,tree_desc near *);
local void scan_tree(DeflateHandler,ct_data near *, int);
local void send_tree(DeflateHandler,ct_data near *, int);
local int  build_bl_tree(DeflateHandler);
local void send_all_trees(DeflateHandler,int,int,int);
local void flush_block(DeflateHandler,int);
local int  ct_tally(DeflateHandler,int,int);
local void compress_block(DeflateHandler,ct_data near *, ct_data near *);
local void send_bits(DeflateHandler,int,int);
local unsigned bi_reverse(unsigned, int);
local void bi_windup(DeflateHandler);
local void qoutbuf(DeflateHandler);

#ifdef DEBUG
local void error(char *m)
{
    fprintf(stderr, "%s\n", m);
    exit(1);
}
#define Assert(cond,msg) {if(!(cond)) error(msg);}
local int verbose = 0;		/* verbose */
local void check_match (DeflateHandler,unsigned, unsigned, int);
#else
#define Assert(cond,msg)
#endif

#ifndef MAX
#define MAX(a,b) (a >= b ? a : b)
#endif /* MAX */

#define head(i) ((encoder->prev+WSIZE)[i])

/* put_byte is used for the compressed output, put_ubyte for the
 * uncompressed output. However unlzw() uses window for its
 * suffix table instead of its output buffer, so it does not use put_ubyte
 * (to be cleaned up).
 */
#define put_byte(c) {encoder->outbuf[encoder->outoff + encoder->outcnt++] = \
  (uch)(c); if(encoder->outoff + encoder->outcnt == OUTBUFSIZ) \
  qoutbuf(encoder);}

/* Output a 16 bit value, lsb first */
#define put_short(w) \
{ if(encoder->outoff + encoder->outcnt < OUTBUFSIZ - 2) { \
    encoder->outbuf[encoder->outoff+encoder->outcnt++] = (uch) ((w) & 0xff); \
    encoder->outbuf[encoder->outoff+encoder->outcnt++] = (uch) ((ush)(w) >> 8); \
  } else { put_byte((uch)((w) & 0xff));	 put_byte((uch)((ush)(w) >> 8)); }}
/* ===========================================================================
 * Update a hash value with the given input byte
 * IN  assertion: all calls to to UPDATE_HASH are made with consecutive
 *    input characters, so that a running hash key can be computed from the
 *    previous key instead of complete recalculation each time.
 */
#define UPDATE_HASH(h,c) (h = (((h)<<H_SHIFT) ^ (c)) & HASH_MASK)

/* ===========================================================================
 * Insert string s in the dictionary and set match_head to the previous head
 * of the hash chain (the most recent string with same hash key). Return
 * the previous length of the hash chain.
 * IN  assertion: all calls to to INSERT_STRING are made with consecutive
 *    input characters and the first MIN_MATCH bytes of s are valid
 *    (except for the last MIN_MATCH-1 bytes of the input file).
 */
#define INSERT_STRING(s, match_head) \
   (UPDATE_HASH(encoder->ins_h, encoder->window[(s) + MIN_MATCH-1]), \
    encoder->prev[(s) & WMASK] =match_head = head(encoder->ins_h), \
    head(encoder->ins_h) = (s))

#define SEND_CODE(c, tree) send_bits(encoder, (tree)[c].Code, (tree)[c].Len)
/* Send a code of the given tree. c and tree must not have side effects */

#define D_CODE(dist) ((dist)<256 ? encoder->dist_code[dist] : encoder->dist_code[256+((dist)>>7)])
/* Mapping from a distance to a distance code. dist is the distance - 1 and
 * must not have side effects. dist_code[256] and dist_code[257] are never
 * used.
 */

/* ===========================================================================
 * Compares to subtrees, using the tree depth as tie breaker when
 * the subtrees have equal frequency. This minimizes the worst case length.
 */
#define SMALLER(tree, n, m) \
   ((tree)[n].Freq < (tree)[m].Freq || \
   ((tree)[n].Freq == (tree)[m].Freq && encoder->depth[n] <= encoder->depth[m]))


/* ===========================================================================
 * Initialize the "longest match" routines for a new file
 */
local void lm_init(DeflateHandler encoder)
{
    register unsigned j;

    /* Initialize the hash table. */
#if defined(MAXSEG_64K) && HASH_BITS == 15
    for(j = 0; j < HASH_SIZE; j++) head(j) = NIL;
#else
    memset((char*)&head(0), 0, HASH_SIZE*sizeof(head(0)));
#endif
    /* prev will be initialized on the fly */

    /* Set the default configuration parameters:
     */
    encoder->max_lazy_match   = configuration_table[encoder->compr_level].max_lazy;
    encoder->good_match	      = configuration_table[encoder->compr_level].good_length;
#ifndef FULL_SEARCH
    encoder->nice_match	      = configuration_table[encoder->compr_level].nice_length;
#endif
    encoder->max_chain_length = configuration_table[encoder->compr_level].max_chain;

    encoder->strstart = 0;
    encoder->block_start = 0L;

    encoder->lookahead =
	encoder->read_func((char*)encoder->window,
			   (long)(sizeof(int)<=2 ? (unsigned)WSIZE : 2*WSIZE),
			   encoder->user_val);

    if(encoder->lookahead == 0 || encoder->lookahead == (unsigned)EOF) {
	encoder->eofile = 1;
	encoder->lookahead = 0;
	return;
    }
    encoder->eofile = 0;
    /* Make sure that we always have enough lookahead. This is important
     * if input comes from a device such as a tty.
     */
    while(encoder->lookahead < MIN_LOOKAHEAD && !encoder->eofile)
	fill_window(encoder);

    encoder->ins_h = 0;
    for(j=0; j<MIN_MATCH-1; j++)
	UPDATE_HASH(encoder->ins_h, encoder->window[j]);
    /* If lookahead < MIN_MATCH, ins_h is garbage, but this is
     * not important since only literal bytes will be emitted.
     */
}

/* ===========================================================================
 * Set match_start to the longest match starting at the given string and
 * return its length. Matches shorter or equal to prev_length are discarded,
 * in which case the result is equal to prev_length and match_start is
 * garbage.
 * IN assertions: cur_match is the head of the hash chain for the current
 *   string (strstart) and its distance is <= MAX_DIST, and prev_length >= 1
 */
local int longest_match(DeflateHandler encoder, unsigned cur_match)
{
    unsigned chain_length = encoder->max_chain_length;
    /* max hash chain length */

    register uch *scan = encoder->window + encoder->strstart;
    /* current string */

    register uch *match;			/* matched string */
    register int len;				/* length of current match */
    int best_len = encoder->prev_length;	 /* best match length so far */

    unsigned limit = (encoder->strstart > (unsigned)MAX_DIST ?
		      encoder->strstart - (unsigned)MAX_DIST : NIL);
    /* Stop when cur_match becomes <= limit. To simplify the code,
     * we prevent matches with the string of window index 0.
     */

#if HASH_BITS < 8 || MAX_MATCH != 258
   error: Code too clever
#endif

#ifdef UNALIGNED_OK
    /* Compare two bytes at a time. Note: this is not always beneficial.
     * Try with and without -DUNALIGNED_OK to check.
     */
    register uch *strend = encoder->window + encoder->strstart + MAX_MATCH - 1;
    register ush scan_start = *(ush*)scan;
    register ush scan_end   = *(ush*)(scan+best_len-1);
#else
    register uch *strend    = encoder->window + encoder->strstart + MAX_MATCH;
    register uch scan_end1  = scan[best_len-1];
    register uch scan_end   = scan[best_len];
#endif

    /* Do not waste too much time if we already have a good match: */
    if(encoder->prev_length >= encoder->good_match) {
	chain_length >>= 2;
    }
    Assert(encoder->strstart <= window_size-MIN_LOOKAHEAD, "insufficient lookahead");

    do {
	Assert(cur_match < encoder->strstart, "no future");
	match = encoder->window + cur_match;

	/* Skip to next match if the match length cannot increase
	 * or if the match length is less than 2:
	 */
#if (defined(UNALIGNED_OK) && MAX_MATCH == 258)
	/* This code assumes sizeof(unsigned short) == 2. Do not use
	 * UNALIGNED_OK if your compiler uses a different size.
	 */
	if(*(ush*)(match+best_len-1) != scan_end ||
	   *(ush*)match != scan_start) continue;

	/* It is not necessary to compare scan[2] and match[2] since they are
	 * always equal when the other bytes match, given that the hash keys
	 * are equal and that HASH_BITS >= 8. Compare 2 bytes at a time at
	 * strstart+3, +5, ... up to strstart+257. We check for insufficient
	 * lookahead only every 4th comparison; the 128th check will be made
	 * at strstart+257. If MAX_MATCH-2 is not a multiple of 8, it is
	 * necessary to put more guard bytes at the end of the window, or
	 * to check more often for insufficient lookahead.
	 */
	scan++, match++;
	do {
	} while(*(ush*)(scan+=2) == *(ush*)(match+=2) &&
		*(ush*)(scan+=2) == *(ush*)(match+=2) &&
		*(ush*)(scan+=2) == *(ush*)(match+=2) &&
		*(ush*)(scan+=2) == *(ush*)(match+=2) &&
		scan < strend);
	/* The funny "do {}" generates better code on most compilers */

	/* Here, scan <= window+strstart+257 */
	Assert(scan <= encoder->window+(unsigned)(window_size-1), "wild scan");
	if(*scan == *match) scan++;

	len = (MAX_MATCH - 1) - (int)(strend-scan);
	scan = strend - (MAX_MATCH-1);

#else /* UNALIGNED_OK */

	if(match[best_len]   != scan_end  ||
	   match[best_len-1] != scan_end1 ||
	   *match	      != *scan	   ||
	   *++match	      != scan[1])      continue;

	/* The check at best_len-1 can be removed because it will be made
	 * again later. (This heuristic is not always a win.)
	 * It is not necessary to compare scan[2] and match[2] since they
	 * are always equal when the other bytes match, given that
	 * the hash keys are equal and that HASH_BITS >= 8.
	 */
	scan += 2, match++;

	/* We check for insufficient lookahead only every 8th comparison;
	 * the 256th check will be made at strstart+258.
	 */
	do {
	} while(*++scan == *++match && *++scan == *++match &&
		*++scan == *++match && *++scan == *++match &&
		*++scan == *++match && *++scan == *++match &&
		*++scan == *++match && *++scan == *++match &&
		scan < strend);

	len = MAX_MATCH - (int)(strend - scan);
	scan = strend - MAX_MATCH;

#endif /* UNALIGNED_OK */

	if(len > best_len) {
	    encoder->match_start = cur_match;
	    best_len = len;
#ifdef FULL_SEARCH
	    if(len >= MAX_MATCH) break;
#else
	    if(len >= encoder->nice_match) break;
#endif /* FULL_SEARCH */


#ifdef UNALIGNED_OK
	    scan_end = *(ush*)(scan+best_len-1);
#else
	    scan_end1  = scan[best_len-1];
	    scan_end   = scan[best_len];
#endif
	}
    } while((cur_match = encoder->prev[cur_match & WMASK]) > limit
	    && --chain_length != 0);

    return best_len;
}

#ifdef DEBUG
/* ===========================================================================
 * Check that the match at match_start is indeed a match.
 */
local void check_match(DeflateHandler encoder,
		       unsigned start, unsigned match, int length)
{
    /* check that the match is indeed a match */
    if(memcmp((char*)encoder->window + match,
	      (char*)encoder->window + start, length) != EQUAL) {
	fprintf(stderr,
	    " start %d, match %d, length %d\n",
	    start, match, length);
	error("invalid match");
    }
    if(verbose > 1) {
	fprintf(stderr,"\\[%d,%d]", start-match, length);
	do { putc(encoder->window[start++], stderr); } while(--length != 0);
    }
}
#else
#  define check_match(encoder, start, match, length)
#endif

/* ===========================================================================
 * Fill the window when the lookahead becomes insufficient.
 * Updates strstart and lookahead, and sets eofile if end of input file.
 * IN assertion: lookahead < MIN_LOOKAHEAD && strstart + lookahead > 0
 * OUT assertions: at least one byte has been read, or eofile is set;
 *    file reads are performed for at least two bytes (required for the
 *    translate_eol option).
 */
local void fill_window(DeflateHandler encoder)
{
    register unsigned n, m;
    unsigned more = (unsigned)(window_size - (ulg)encoder->lookahead - (ulg)encoder->strstart);
    /* Amount of free space at the end of the window. */

    /* If the window is almost full and there is insufficient lookahead,
     * move the upper half to the lower one to make room in the upper half.
     */
    if(more == (unsigned)EOF) {
	/* Very unlikely, but possible on 16 bit machine if strstart == 0
	 * and lookahead == 1 (input done one byte at time)
	 */
	more--;
    } else if(encoder->strstart >= WSIZE+MAX_DIST) {
	/* By the IN assertion, the window is not empty so we can't confuse
	 * more == 0 with more == 64K on a 16 bit machine.
	 */
	Assert(window_size == (ulg)2*WSIZE, "no sliding with BIG_MEM");

	memcpy((char*)encoder->window, (char*)encoder->window+WSIZE, (unsigned)WSIZE);
	encoder->match_start -= WSIZE;
	encoder->strstart    -= WSIZE; /* we now have strstart >= MAX_DIST: */
	encoder->block_start -= (long) WSIZE;

	for(n = 0; n < HASH_SIZE; n++) {
	    m = head(n);
	    head(n) = (ush)(m >= WSIZE ? m-WSIZE : NIL);
	}
	for(n = 0; n < WSIZE; n++) {
	    m = encoder->prev[n];
	    encoder->prev[n] = (ush)(m >= WSIZE ? m-WSIZE : NIL);
	    /* If n is not on any hash chain, prev[n] is garbage but
	     * its value will never be used.
	     */
	}
	more += WSIZE;
    }
    /* At this point, more >= 2 */
    if(!encoder->eofile) {
	n = encoder->read_func((char*)encoder->window+encoder->strstart
			       + encoder->lookahead,
			       (long)more,
			       encoder->user_val);
	if(n == 0 || n == (unsigned)EOF) {
	    encoder->eofile = 1;
	} else {
	    encoder->lookahead += n;
	}
    }
}

/* ===========================================================================
 * Processes a new input file and return its compressed length. This
 * function does not perform lazy evaluationof matches and inserts
 * new strings in the dictionary only for unmatched strings or for short
 * matches. It is used only for the fast compression options.
 */
local void deflate_fast(DeflateHandler encoder)
{
/*
    encoder->prev_length = MIN_MATCH-1;
    encoder->match_length = 0;
*/
    while(encoder->lookahead != 0 && encoder->qhead == NULL) {
	int flush; /* set if current block must be flushed */

	/* Insert the string window[strstart .. strstart+2] in the
	 * dictionary, and set hash_head to the head of the hash chain:
	 */
	INSERT_STRING(encoder->strstart, encoder->hash_head);

	/* Find the longest match, discarding those <= prev_length.
	 * At this point we have always match_length < MIN_MATCH
	 */
	if(encoder->hash_head != NIL &&
	   encoder->strstart - encoder->hash_head <= MAX_DIST) {
	    /* To simplify the code, we prevent matches with the string
	     * of window index 0 (in particular we have to avoid a match
	     * of the string with itself at the start of the input file).
	     */
	    encoder->match_length = longest_match(encoder, encoder->hash_head);
	    /* longest_match() sets match_start */
	    if(encoder->match_length > encoder->lookahead)
		encoder->match_length = encoder->lookahead;
	}
	if(encoder->match_length >= MIN_MATCH) {
	    check_match(encoder, encoder->strstart,
			encoder->match_start, encoder->match_length);

	    flush = ct_tally(encoder, encoder->strstart - encoder->match_start,
			     encoder->match_length - MIN_MATCH);

	    encoder->lookahead -= encoder->match_length;

	    /* Insert new strings in the hash table only if the match length
	     * is not too large. This saves time but degrades compression.
	     */
	    if(encoder->match_length <= encoder->max_lazy_match) {
		encoder->match_length--; /* string at strstart already in hash table */
		do {
		    encoder->strstart++;
		    INSERT_STRING(encoder->strstart, encoder->hash_head);
		    /* strstart never exceeds WSIZE-MAX_MATCH, so there are
		     * always MIN_MATCH bytes ahead. If lookahead < MIN_MATCH
		     * these bytes are garbage, but it does not matter since
		     * the next lookahead bytes will be emitted as literals.
		     */
		} while(--encoder->match_length != 0);
		encoder->strstart++;
	    } else {
		encoder->strstart += encoder->match_length;
		encoder->match_length = 0;
		encoder->ins_h = encoder->window[encoder->strstart];
		UPDATE_HASH(encoder->ins_h,
			    encoder->window[encoder->strstart + 1]);
#if MIN_MATCH != 3
		Call UPDATE_HASH() MIN_MATCH-3 more times
#endif
	    }
	} else {
	    /* No match, output a literal byte */
	    Tracevv((stderr,"%c",encoder->window[encoder->strstart]));
	    flush = ct_tally (encoder, 0, encoder->window[encoder->strstart]);
	    encoder->lookahead--;
	    encoder->strstart++;
	}
	if(flush)
	{
	    flush_block(encoder, 0);
	    encoder->block_start = (long)encoder->strstart;
	}

	/* Make sure that we always have enough lookahead, except
	 * at the end of the input file. We need MAX_MATCH bytes
	 * for the next match, plus MIN_MATCH bytes to insert the
	 * string following the next match.
	 */
	while(encoder->lookahead < MIN_LOOKAHEAD && !encoder->eofile)
	    fill_window(encoder);
    }
}

local void deflate_better(DeflateHandler encoder) {
    /* Process the input block. */
    while(encoder->lookahead != 0 && encoder->qhead == NULL) {
	/* Insert the string window[strstart .. strstart+2] in the
	 * dictionary, and set hash_head to the head of the hash chain:
	 */
	INSERT_STRING(encoder->strstart, encoder->hash_head);

	/* Find the longest match, discarding those <= prev_length.
	 */
	encoder->prev_length = encoder->match_length;
	encoder->prev_match = encoder->match_start;
	encoder->match_length = MIN_MATCH-1;

	if(encoder->hash_head != NIL &&
	   encoder->prev_length < encoder->max_lazy_match &&
	   encoder->strstart - encoder->hash_head <= MAX_DIST) {
	    /* To simplify the code, we prevent matches with the string
	     * of window index 0 (in particular we have to avoid a match
	     * of the string with itself at the start of the input file).
	     */
	    encoder->match_length = longest_match(encoder, encoder->hash_head);
	    /* longest_match() sets match_start */
	    if(encoder->match_length > encoder->lookahead)
		encoder->match_length = encoder->lookahead;

	    /* Ignore a length 3 match if it is too distant: */
	    if(encoder->match_length == MIN_MATCH &&
		encoder->strstart - encoder->match_start > TOO_FAR){
		/* If prev_match is also MIN_MATCH, match_start is garbage
		 * but we will ignore the current match anyway.
		 */
		encoder->match_length--;
	    }
	}
	/* If there was a match at the previous step and the current
	 * match is not better, output the previous match:
	 */
	if(encoder->prev_length >= MIN_MATCH &&
	   encoder->match_length <= encoder->prev_length) {
	    int flush; /* set if current block must be flushed */

	    check_match(encoder, encoder->strstart-1,
			encoder->prev_match, encoder->prev_length);

	    flush = ct_tally(encoder, encoder->strstart-1-encoder->prev_match,
			     encoder->prev_length - MIN_MATCH);

	    /* Insert in hash table all strings up to the end of the match.
	     * strstart-1 and strstart are already inserted.
	     */
	    encoder->lookahead -= encoder->prev_length-1;
	    encoder->prev_length -= 2;
	    do {
		encoder->strstart++;
		INSERT_STRING(encoder->strstart, encoder->hash_head);
		/* strstart never exceeds WSIZE-MAX_MATCH, so there are
		 * always MIN_MATCH bytes ahead. If lookahead < MIN_MATCH
		 * these bytes are garbage, but it does not matter since the
		 * next lookahead bytes will always be emitted as literals.
		 */
	    } while(--encoder->prev_length != 0);
	    encoder->match_available = 0;
	    encoder->match_length = MIN_MATCH-1;
	    encoder->strstart++;
	    if(flush) {
		flush_block(encoder, 0);
		encoder->block_start = (long)encoder->strstart;
	    }

	} else if(encoder->match_available) {
	    /* If there was no match at the previous position, output a
	     * single literal. If there was a match but the current match
	     * is longer, truncate the previous match to a single literal.
	     */
	    Tracevv((stderr,"%c",encoder->window[encoder->strstart-1]));
	    if(ct_tally (encoder, 0, encoder->window[encoder->strstart-1])) {
		flush_block(encoder, 0);
		encoder->block_start = (long)encoder->strstart;
	    }
	    encoder->strstart++;
	    encoder->lookahead--;
	} else {
	    /* There is no previous match to compare with, wait for
	     * the next step to decide.
	     */
	    encoder->match_available = 1;
	    encoder->strstart++;
	    encoder->lookahead--;
	}

	/* Make sure that we always have enough lookahead, except
	 * at the end of the input file. We need MAX_MATCH bytes
	 * for the next match, plus MIN_MATCH bytes to insert the
	 * string following the next match.
	 */
	while(encoder->lookahead < MIN_LOOKAHEAD && !encoder->eofile)
	    fill_window(encoder);
    }
}

/*ARGSUSED*/
static long default_read_func(char *buf, long size, void *v)
{
    return (long)fread(buf, 1, size, stdin);
}

DeflateHandler open_deflate_handler(
    long (* read_func)(char *buf, long size, void *user_val),
    void *user_val,
    int level)
{
    DeflateHandler encoder;

    if(level < 1 || level > 9)
	return NULL; /* error("bad compression level"); */

    encoder = (DeflateHandler)safe_malloc(sizeof(struct _DeflateHandler));
    if(encoder == NULL)
	return NULL;
    memset(encoder, 0, sizeof(struct _DeflateHandler));
    encoder->compr_level = level;
    if(read_func == NULL)
	encoder->read_func = default_read_func;
    else
	encoder->read_func = read_func;
    encoder->user_val = user_val;

    return encoder;
}

void close_deflate_handler(DeflateHandler encoder)
{
    free(encoder);
}

local void init_deflate(DeflateHandler encoder)
{
    if(encoder->eofile)
	return;
    encoder->bi_buf = 0;
    encoder->bi_valid = 0;
    ct_init(encoder);
    lm_init(encoder);

    encoder->qhead = NULL;
    encoder->outcnt = 0;

    if(encoder->compr_level <= 3)
    {
	encoder->prev_length = MIN_MATCH - 1;
	encoder->match_length = 0;
    }
    else
    {
	encoder->match_length = MIN_MATCH - 1;
	encoder->match_available = 0;
    }

    encoder->complete = 0;
}

/* ===========================================================================
 * Same as above, but achieves better compression. We use a lazy
 * evaluation for matches: a match is finally adopted only if there is
 * no better match at the next window position.
 */
long zip_deflate(DeflateHandler encoder, char *buff, long buff_size)
{
    long n;

    if(!encoder->initflag)
    {
	init_deflate(encoder);
	encoder->initflag = 1;
	if(encoder->lookahead == 0) { /* empty */
	    encoder->complete = 1;
	    return 0;
	}
    }

    if((n = qcopy(encoder, buff, buff_size)) == buff_size)
	return buff_size;

    if(encoder->complete)
	return n;

    if(encoder->compr_level <= 3) /* optimized for speed */
	deflate_fast(encoder);
    else
	deflate_better(encoder);
    if(encoder->lookahead == 0)
    {
	if(encoder->match_available)
	    ct_tally(encoder, 0, encoder->window[encoder->strstart - 1]);
	flush_block(encoder, 1);
	encoder->complete = 1;
    }
    return n + qcopy(encoder, buff + n, buff_size - n);
}

local long qcopy(DeflateHandler encoder, char *buff, long buff_size)
{
  struct deflate_buff_queue *q;
    long n, i;

    n = 0;
    q = encoder->qhead;
    while(q != NULL && n < buff_size)
    {
	i = buff_size - n;
	if(i > q->len)
	    i = q->len;
	memcpy(buff + n, q->ptr, i);
	q->ptr += i;
	q->len -= i;
	n += i;

	if(q->len == 0)
	{
	    struct deflate_buff_queue *p;
	    p = q;
	    q = q->next;
	    reuse_queue(p);
	}
    }
    encoder->qhead = q;
    if(n == buff_size)
      return n;

    if(encoder->outoff < encoder->outcnt)
    {
	i = buff_size - n;
	if(i > encoder->outcnt - encoder->outoff)
	    i = encoder->outcnt - encoder->outoff;
	memcpy(buff + n, encoder->outbuf + encoder->outoff, i);
	encoder->outoff += i;
	n += i;
	if(encoder->outcnt == encoder->outoff)
	    encoder->outcnt = encoder->outoff = 0;
    }
    return n;
}


/* ===========================================================================
 * Allocate the match buffer, initialize the various tables and save the
 * location of the internal file attribute (ascii/binary) and method
 * (DEFLATE/STORE).
 */
local void ct_init(DeflateHandler encoder)
{
    int n;	/* iterates over tree elements */
    int bits;	/* bit counter */
    int length;	/* length value */
    int code;	/* code value */
    int dist;	/* distance index */

    if(encoder->static_dtree[0].Len != 0) return; /* ct_init already called */

    encoder->l_desc.dyn_tree	= encoder->dyn_ltree;
    encoder->l_desc.static_tree = encoder->static_ltree;
    encoder->l_desc.extra_bits	= extra_lbits;
    encoder->l_desc.extra_base	= LITERALS + 1;
    encoder->l_desc.elems	= L_CODES;
    encoder->l_desc.max_length	= MAX_BITS;
    encoder->l_desc.max_code	= 0;

    encoder->d_desc.dyn_tree	= encoder->dyn_dtree;
    encoder->d_desc.static_tree = encoder->static_dtree;
    encoder->d_desc.extra_bits	= extra_dbits;
    encoder->d_desc.extra_base	= 0;
    encoder->d_desc.elems	= D_CODES;
    encoder->d_desc.max_length	= MAX_BITS;
    encoder->d_desc.max_code	= 0;

    encoder->bl_desc.dyn_tree	 = encoder->bl_tree;
    encoder->bl_desc.static_tree = NULL;
    encoder->bl_desc.extra_bits	 = extra_blbits;
    encoder->bl_desc.extra_base	 = 0;
    encoder->bl_desc.elems	 = BL_CODES;
    encoder->bl_desc.max_length	 = MAX_BL_BITS;
    encoder->bl_desc.max_code	 = 0;

    /* Initialize the mapping length (0..255) -> length code (0..28) */
    length = 0;
    for(code = 0; code < LENGTH_CODES-1; code++) {
	encoder->base_length[code] = length;
	for(n = 0; n < (1<<extra_lbits[code]); n++) {
	    encoder->length_code[length++] = (uch)code;
	}
    }
    Assert (length == 256, "ct_init: length != 256");
    /* Note that the length 255 (match length 258) can be represented
     * in two different ways: code 284 + 5 bits or code 285, so we
     * overwrite length_code[255] to use the best encoding:
     */
    encoder->length_code[length-1] = (uch)code;

    /* Initialize the mapping dist (0..32K) -> dist code (0..29) */
    dist = 0;
    for(code = 0 ; code < 16; code++) {
	encoder->base_dist[code] = dist;
	for(n = 0; n < (1<<extra_dbits[code]); n++) {
	    encoder->dist_code[dist++] = (uch)code;
	}
    }
    Assert (dist == 256, "ct_init: dist != 256");
    dist >>= 7; /* from now on, all distances are divided by 128 */
    for( ; code < D_CODES; code++) {
	encoder->base_dist[code] = dist << 7;
	for(n = 0; n < (1<<(extra_dbits[code]-7)); n++) {
	    encoder->dist_code[256 + dist++] = (uch)code;
	}
    }
    Assert (dist == 256, "ct_init: 256+dist != 512");

    /* Construct the codes of the static literal tree */
    for(bits = 0; bits <= MAX_BITS; bits++) encoder->bl_count[bits] = 0;
    n = 0;
    while(n <= 143) encoder->static_ltree[n++].Len = 8, encoder->bl_count[8]++;
    while(n <= 255) encoder->static_ltree[n++].Len = 9, encoder->bl_count[9]++;
    while(n <= 279) encoder->static_ltree[n++].Len = 7, encoder->bl_count[7]++;
    while(n <= 287) encoder->static_ltree[n++].Len = 8, encoder->bl_count[8]++;
    /* Codes 286 and 287 do not exist, but we must include them in the
     * tree construction to get a canonical Huffman tree (longest code
     * all ones)
     */
    gen_codes(encoder, (ct_data near *)encoder->static_ltree, L_CODES+1);

    /* The static distance tree is trivial: */
    for(n = 0; n < D_CODES; n++) {
	encoder->static_dtree[n].Len = 5;
	encoder->static_dtree[n].Code = bi_reverse(n, 5);
    }

    /* Initialize the first block of the first file: */
    init_block(encoder);
}

/* ===========================================================================
 * Initialize a new block.
 */
local void init_block(DeflateHandler encoder)
{
    int n; /* iterates over tree elements */

    /* Initialize the trees. */
    for(n = 0; n < L_CODES;  n++) encoder->dyn_ltree[n].Freq = 0;
    for(n = 0; n < D_CODES;  n++) encoder->dyn_dtree[n].Freq = 0;
    for(n = 0; n < BL_CODES; n++) encoder->bl_tree[n].Freq = 0;

    encoder->dyn_ltree[END_BLOCK].Freq = 1;
    encoder->opt_len = encoder->static_len = 0L;
    encoder->last_lit = encoder->last_dist = encoder->last_flags = 0;
    encoder->flags = 0;
    encoder->flag_bit = 1;
}

/* ===========================================================================
 * Restore the heap property by moving down the tree starting at node k,
 * exchanging a node with the smallest of its two sons if necessary, stopping
 * when the heap property is re-established (each father smaller than its
 * two sons).
 */
local void pqdownheap(
    DeflateHandler encoder,
    ct_data near *tree,	/* the tree to restore */
    int k)		/* node to move down */
{
    int v = encoder->heap[k];
    int j = k << 1;  /* left son of k */
    while(j <= encoder->heap_len) {
	/* Set j to the smallest of the two sons: */
	if(j < encoder->heap_len &&
	   SMALLER(tree, encoder->heap[j+1], encoder->heap[j]))
	    j++;

	/* Exit if v is smaller than both sons */
	if(SMALLER(tree, v, encoder->heap[j]))
	    break;

	/* Exchange v with the smallest son */
	encoder->heap[k] = encoder->heap[j];
	k = j;

	/* And continue down the tree, setting j to the left son of k */
	j <<= 1;
    }
    encoder->heap[k] = v;
}

/* ===========================================================================
 * Compute the optimal bit lengths for a tree and update the total bit length
 * for the current block.
 * IN assertion: the fields freq and dad are set, heap[heap_max] and
 *    above are the tree nodes sorted by increasing frequency.
 * OUT assertions: the field len is set to the optimal bit length, the
 *     array bl_count contains the frequencies for each bit length.
 *     The length opt_len is updated; static_len is also updated if stree is
 *     not null.
 */
local void gen_bitlen(
    DeflateHandler encoder,
    tree_desc near *desc) /* the tree descriptor */
{
    ct_data near *tree	= desc->dyn_tree;
    int near *extra	= desc->extra_bits;
    int base		= desc->extra_base;
    int max_code	= desc->max_code;
    int max_length	= desc->max_length;
    ct_data near *stree = desc->static_tree;
    int h;		/* heap index */
    int n, m;		/* iterate over the tree elements */
    int bits;		/* bit length */
    int xbits;		/* extra bits */
    ush f;		/* frequency */
    int overflow = 0;	/* number of elements with bit length too large */

    for(bits = 0; bits <= MAX_BITS; bits++)
	encoder->bl_count[bits] = 0;

    /* In a first pass, compute the optimal bit lengths (which may
     * overflow in the case of the bit length tree).
     */
    tree[encoder->heap[encoder->heap_max]].Len = 0; /* root of the heap */

    for(h = encoder->heap_max+1; h < HEAP_SIZE; h++) {
	n = encoder->heap[h];
	bits = tree[tree[n].Dad].Len + 1;
	if(bits > max_length)
	    bits = max_length, overflow++;
	tree[n].Len = (ush)bits;
	/* We overwrite tree[n].Dad which is no longer needed */

	if(n > max_code)
	    continue; /* not a leaf node */

	encoder->bl_count[bits]++;
	xbits = 0;
	if(n >= base)
	    xbits = extra[n-base];
	f = tree[n].Freq;
	encoder->opt_len += (ulg)f * (bits + xbits);
	if(stree)
	    encoder->static_len += (ulg)f * (stree[n].Len + xbits);
    }
    if(overflow == 0) return;

    Trace((stderr,"\nbit length overflow\n"));
    /* This happens for example on obj2 and pic of the Calgary corpus */

    /* Find the first bit length which could increase: */
    do {
	bits = max_length-1;
	while(encoder->bl_count[bits] == 0) bits--;
	encoder->bl_count[bits]--;	/* move one leaf down the tree */
	encoder->bl_count[bits+1] += 2; /* move one overflow item as its brother */
	encoder->bl_count[max_length]--;
	/* The brother of the overflow item also moves one step up,
	 * but this does not affect bl_count[max_length]
	 */
	overflow -= 2;
    } while(overflow > 0);

    /* Now recompute all bit lengths, scanning in increasing frequency.
     * h is still equal to HEAP_SIZE. (It is simpler to reconstruct all
     * lengths instead of fixing only the wrong ones. This idea is taken
     * from 'ar' written by Haruhiko Okumura.)
     */
    for(bits = max_length; bits != 0; bits--) {
	n = encoder->bl_count[bits];
	while(n != 0) {
	    m = encoder->heap[--h];
	    if(m > max_code) continue;
	    if(tree[m].Len != (unsigned) bits) {
		Trace((stderr,"code %d bits %d->%d\n", m, tree[m].Len, bits));
		encoder->opt_len +=
		    ((long)bits-(long)tree[m].Len)*(long)tree[m].Freq;
		tree[m].Len = (ush)bits;
	    }
	    n--;
	}
    }
}

/* ===========================================================================
 * Generate the codes for a given tree and bit counts (which need not be
 * optimal).
 * IN assertion: the array bl_count contains the bit length statistics for
 * the given tree and the field len is set for all tree elements.
 * OUT assertion: the field code is set for all tree elements of non
 *     zero code length.
 */
local void gen_codes(
    DeflateHandler encoder,
    ct_data near *tree,	/* the tree to decorate */
    int max_code)	/* largest code with non zero frequency */
{
    ush next_code[MAX_BITS+1];	/* next code value for each bit length */
    ush code = 0;		/* running code value */
    int bits;			/* bit index */
    int n;			/* code index */

    /* The distribution counts are first used to generate the code values
     * without bit reversal.
     */
    for(bits = 1; bits <= MAX_BITS; bits++) {
	next_code[bits] = code = (code + encoder->bl_count[bits-1]) << 1;
    }
    /* Check that the bit counts in bl_count are consistent. The last code
     * must be all ones.
     */
    Assert (code + encoder->bl_count[MAX_BITS]-1 == (1<<MAX_BITS)-1,
	    "inconsistent bit counts");
    Tracev((stderr,"\ngen_codes: max_code %d ", max_code));

    for(n = 0;	 n <= max_code; n++) {
	int len = tree[n].Len;
	if(len == 0)
	    continue;
	/* Now reverse the bits */
	tree[n].Code = bi_reverse(next_code[len]++, len);

	Tracec(tree != static_ltree, (stderr,"\nn %3d %c l %2d c %4x (%x) ",
	     n, (isgraph(n) ? n : ' '), len, tree[n].Code, next_code[len]-1));
    }
}

/* ===========================================================================
 * Construct one Huffman tree and assigns the code bit strings and lengths.
 * Update the total bit length for the current block.
 * IN assertion: the field freq is set for all tree elements.
 * OUT assertions: the fields len and code are set to the optimal bit length
 *     and corresponding code. The length opt_len is updated; static_len is
 *     also updated if stree is not null. The field max_code is set.
 */
local void build_tree(
    DeflateHandler encoder,
    tree_desc near *desc) /* the tree descriptor */
{
    ct_data near *tree	= desc->dyn_tree;
    ct_data near *stree	= desc->static_tree;
    int elems		= desc->elems;
    int n, m;		/* iterate over heap elements */
    int max_code = -1;	/* largest code with non zero frequency */
    int node = elems;	/* next internal node of the tree */

    /* Construct the initial heap, with least frequent element in
     * heap[SMALLEST]. The sons of heap[n] are heap[2*n] and heap[2*n+1].
     * heap[0] is not used.
     */
    encoder->heap_len = 0;
    encoder->heap_max = HEAP_SIZE;

    for(n = 0; n < elems; n++) {
	if(tree[n].Freq != 0) {
	    encoder->heap[++encoder->heap_len] = max_code = n;
	    encoder->depth[n] = 0;
	} else {
	    tree[n].Len = 0;
	}
    }

    /* The pkzip format requires that at least one distance code exists,
     * and that at least one bit should be sent even if there is only one
     * possible code. So to avoid special checks later on we force at least
     * two codes of non zero frequency.
     */
    while(encoder->heap_len < 2) {
	int new = encoder->heap[++encoder->heap_len] =
	    (max_code < 2 ? ++max_code : 0);
	tree[new].Freq = 1;
	encoder->depth[new] = 0;
	encoder->opt_len--;
	if(stree)
	    encoder->static_len -= stree[new].Len;
	/* new is 0 or 1 so it does not have extra bits */
    }
    desc->max_code = max_code;

    /* The elements heap[heap_len/2+1 .. heap_len] are leaves of the tree,
     * establish sub-heaps of increasing lengths:
     */
    for(n = encoder->heap_len/2; n >= 1; n--)
	pqdownheap(encoder, tree, n);

    /* Construct the Huffman tree by repeatedly combining the least two
     * frequent nodes.
     */
    do {
	n = encoder->heap[SMALLEST];
	encoder->heap[SMALLEST] = encoder->heap[encoder->heap_len--];
	pqdownheap(encoder, tree, SMALLEST);

	m = encoder->heap[SMALLEST];  /* m = node of next least frequency */

	/* keep the nodes sorted by frequency */
	encoder->heap[--encoder->heap_max] = n;
	encoder->heap[--encoder->heap_max] = m;

	/* Create a new node father of n and m */
	tree[node].Freq = tree[n].Freq + tree[m].Freq;
	encoder->depth[node] =
	    (uch)(MAX(encoder->depth[n], encoder->depth[m]) + 1);
	tree[n].Dad = tree[m].Dad = (ush)node;

	/* and insert the new node in the heap */
	encoder->heap[SMALLEST] = node++;
	pqdownheap(encoder, tree, SMALLEST);

    } while(encoder->heap_len >= 2);

    encoder->heap[--encoder->heap_max] = encoder->heap[SMALLEST];

    /* At this point, the fields freq and dad are set. We can now
     * generate the bit lengths.
     */
    gen_bitlen(encoder, (tree_desc near *)desc);

    /* The field len is now set, we can generate the bit codes */
    gen_codes (encoder, (ct_data near *)tree, max_code);
}

/* ===========================================================================
 * Scan a literal or distance tree to determine the frequencies of the codes
 * in the bit length tree. Updates opt_len to take into account the repeat
 * counts. (The contribution of the bit length codes will be added later
 * during the construction of bl_tree.)
 */
local void scan_tree(
    DeflateHandler encoder,
    ct_data near *tree,	/* the tree to be scanned */
    int max_code)	/* and its largest code of non zero frequency */
{
    int n;			/* iterates over all tree elements */
    int prevlen = -1;		/* last emitted length */
    int curlen;			/* length of current code */
    int nextlen = tree[0].Len;	/* length of next code */
    int count = 0;		/* repeat count of the current code */
    int max_count = 7;		/* max repeat count */
    int min_count = 4;		/* min repeat count */

    if(nextlen == 0)
	max_count = 138, min_count = 3;
    tree[max_code+1].Len = (ush)0xffff; /* guard */

    for(n = 0; n <= max_code; n++) {
	curlen = nextlen; nextlen = tree[n+1].Len;
	if(++count < max_count && curlen == nextlen) {
	    continue;
	} else if(count < min_count) {
	    encoder->bl_tree[curlen].Freq += count;
	} else if(curlen != 0) {
	    if(curlen != prevlen)
		encoder->bl_tree[curlen].Freq++;
	    encoder->bl_tree[REP_3_6].Freq++;
	} else if(count <= 10) {
	    encoder->bl_tree[REPZ_3_10].Freq++;
	} else {
	    encoder->bl_tree[REPZ_11_138].Freq++;
	}
	count = 0; prevlen = curlen;
	if(nextlen == 0) {
	    max_count = 138, min_count = 3;
	} else if(curlen == nextlen) {
	    max_count = 6, min_count = 3;
	} else {
	    max_count = 7, min_count = 4;
	}
    }
}

/* ===========================================================================
 * Send a literal or distance tree in compressed form, using the codes in
 * bl_tree.
 */
local void send_tree(
    DeflateHandler encoder,
    ct_data near *tree,	/* the tree to be scanned */
    int max_code)	/* and its largest code of non zero frequency */
{
    int n;			/* iterates over all tree elements */
    int prevlen = -1;		/* last emitted length */
    int curlen;			/* length of current code */
    int nextlen = tree[0].Len;	/* length of next code */
    int count = 0;		/* repeat count of the current code */
    int max_count = 7;		/* max repeat count */
    int min_count = 4;		/* min repeat count */

    /* tree[max_code+1].Len = -1; */  /* guard already set */
    if(nextlen == 0) max_count = 138, min_count = 3;

    for(n = 0; n <= max_code; n++) {
	curlen = nextlen; nextlen = tree[n+1].Len;
	if(++count < max_count && curlen == nextlen) {
	    continue;
	} else if(count < min_count) {
	    do { SEND_CODE(curlen, encoder->bl_tree); } while(--count != 0);

	} else if(curlen != 0) {
	    if(curlen != prevlen) {
		SEND_CODE(curlen, encoder->bl_tree);
		count--;
	    }
	    Assert(count >= 3 && count <= 6, " 3_6?");
	    SEND_CODE(REP_3_6, encoder->bl_tree);
	    send_bits(encoder, count-3, 2);

	} else if(count <= 10) {
	    SEND_CODE(REPZ_3_10, encoder->bl_tree);
	    send_bits(encoder, count-3, 3);

	} else {
	    SEND_CODE(REPZ_11_138, encoder->bl_tree);
	    send_bits(encoder, count-11, 7);
	}
	count = 0; prevlen = curlen;
	if(nextlen == 0) {
	    max_count = 138, min_count = 3;
	} else if(curlen == nextlen) {
	    max_count = 6, min_count = 3;
	} else {
	    max_count = 7, min_count = 4;
	}
    }
}

/* ===========================================================================
 * Construct the Huffman tree for the bit lengths and return the index in
 * bl_order of the last bit length code to send.
 */
local int build_bl_tree(DeflateHandler encoder)
{
    int max_blindex;  /* index of last bit length code of non zero freq */

    /* Determine the bit length frequencies for literal and distance trees */
    scan_tree(encoder,
	      (ct_data near *)encoder->dyn_ltree, encoder->l_desc.max_code);
    scan_tree(encoder,
	      (ct_data near *)encoder->dyn_dtree, encoder->d_desc.max_code);

    /* Build the bit length tree: */
    build_tree(encoder, (tree_desc near *)(&encoder->bl_desc));
    /* opt_len now includes the length of the tree representations, except
     * the lengths of the bit lengths codes and the 5+5+4 bits for the counts.
     */

    /* Determine the number of bit length codes to send. The pkzip format
     * requires that at least 4 bit length codes be sent. (appnote.txt says
     * 3 but the actual value used is 4.)
     */
    for(max_blindex = BL_CODES-1; max_blindex >= 3; max_blindex--) {
	if(encoder->bl_tree[bl_order[max_blindex]].Len != 0) break;
    }
    /* Update opt_len to include the bit length tree and counts */
    encoder->opt_len += 3*(max_blindex+1) + 5+5+4;
    Tracev((stderr, "\ndyn trees: dyn %ld, stat %ld",
	    encoder->opt_len, encoder->static_len));

    return max_blindex;
}

/* ===========================================================================
 * Send the header for a block using dynamic Huffman trees: the counts, the
 * lengths of the bit length codes, the literal tree and the distance tree.
 * IN assertion: lcodes >= 257, dcodes >= 1, blcodes >= 4.
 */
local void send_all_trees(
    DeflateHandler encoder,
    int lcodes, int dcodes, int blcodes) /* number of codes for each tree */
{
    int rank; /* index in bl_order */

    Assert (lcodes >= 257 && dcodes >= 1 && blcodes >= 4, "not enough codes");
    Assert (lcodes <= L_CODES && dcodes <= D_CODES && blcodes <= BL_CODES,
	    "too many codes");
    Tracev((stderr, "\nbl counts: "));
    send_bits(encoder, lcodes-257, 5); /* not +255 as stated in appnote.txt */
    send_bits(encoder, dcodes-1,   5);
    send_bits(encoder, blcodes-4,  4); /* not -3 as stated in appnote.txt */
    for(rank = 0; rank < blcodes; rank++) {
	Tracev((stderr, "\nbl code %2d ", bl_order[rank]));
	send_bits(encoder, encoder->bl_tree[bl_order[rank]].Len, 3);
    }

    /* send the literal tree */
    send_tree(encoder, (ct_data near *)encoder->dyn_ltree,lcodes-1);

    /* send the distance tree */
    send_tree(encoder, (ct_data near *)encoder->dyn_dtree,dcodes-1);
}

/* ===========================================================================
 * Determine the best encoding for the current block: dynamic trees, static
 * trees or store, and output the encoded block to the zip file.
 */
local void flush_block(
    DeflateHandler encoder,
    int eof) /* true if this is the last block for a file */
{
    ulg opt_lenb, static_lenb; /* opt_len and static_len in bytes */
    int max_blindex;	/* index of last bit length code of non zero freq */
    ulg stored_len;	/* length of input block */

    stored_len = (ulg)(encoder->strstart - encoder->block_start);
    encoder->flag_buf[encoder->last_flags] = encoder->flags; /* Save the flags for the last 8 items */

    /* Construct the literal and distance trees */
    build_tree(encoder, (tree_desc near *)(&encoder->l_desc));
    Tracev((stderr, "\nlit data: dyn %ld, stat %ld",
	    encoder->opt_len, encoder->static_len));

    build_tree(encoder, (tree_desc near *)(&encoder->d_desc));
    Tracev((stderr, "\ndist data: dyn %ld, stat %ld",
	    encoder->opt_len, encoder->static_len));
    /* At this point, opt_len and static_len are the total bit lengths of
     * the compressed block data, excluding the tree representations.
     */

    /* Build the bit length tree for the above two trees, and get the index
     * in bl_order of the last bit length code to send.
     */
    max_blindex = build_bl_tree(encoder);

    /* Determine the best encoding. Compute first the block length in bytes */
    opt_lenb	= (encoder->opt_len   +3+7)>>3;
    static_lenb = (encoder->static_len+3+7)>>3;

    Trace((stderr, "\nopt %lu(%lu) stat %lu(%lu) stored %lu lit %u dist %u ",
	   opt_lenb, encoder->opt_len,
	   static_lenb, encoder->static_len, stored_len,
	   encoder->last_lit, encoder->last_dist));

    if(static_lenb <= opt_lenb)
	opt_lenb = static_lenb;
    if(stored_len + 4 <= opt_lenb /* 4: two words for the lengths */
       && encoder->block_start >= 0L) {
	unsigned int i;
	uch *p;

	/* The test buf != NULL is only necessary if LIT_BUFSIZE > WSIZE.
	 * Otherwise we can't have processed more than WSIZE input bytes since
	 * the last block flush, because compression would have been
	 * successful. If LIT_BUFSIZE <= WSIZE, it is never too late to
	 * transform a block into a stored block.
	 */
	send_bits(encoder, (STORED_BLOCK<<1)+eof, 3);  /* send block type */
	bi_windup(encoder);		 /* align on byte boundary */
	put_short((ush)stored_len);
	put_short((ush)~stored_len);

	/* copy block */
	p = &encoder->window[(unsigned)encoder->block_start];
	for(i = 0; i < stored_len; i++)
	    put_byte(p[i]);
    } else if(static_lenb == opt_lenb) {
	send_bits(encoder, (STATIC_TREES<<1)+eof, 3);
	compress_block(encoder,
		       (ct_data near *)encoder->static_ltree,
		       (ct_data near *)encoder->static_dtree);
    } else {
	send_bits(encoder, (DYN_TREES<<1)+eof, 3);
	send_all_trees(encoder,
		       encoder->l_desc.max_code+1,
		       encoder->d_desc.max_code+1,
		       max_blindex+1);
	compress_block(encoder,
		       (ct_data near *)encoder->dyn_ltree,
		       (ct_data near *)encoder->dyn_dtree);
    }

    init_block(encoder);

    if(eof)
	bi_windup(encoder);
}

/* ===========================================================================
 * Save the match info and tally the frequency counts. Return true if
 * the current block must be flushed.
 */
local int ct_tally(
    DeflateHandler encoder,
    int dist,	/* distance of matched string */
    int lc)	/* match length-MIN_MATCH or unmatched char (if dist==0) */
{
    encoder->l_buf[encoder->last_lit++] = (uch)lc;
    if(dist == 0) {
	/* lc is the unmatched char */
	encoder->dyn_ltree[lc].Freq++;
    } else {
	/* Here, lc is the match length - MIN_MATCH */
	dist--;		    /* dist = match distance - 1 */
	Assert((ush)dist < (ush)MAX_DIST &&
	       (ush)lc <= (ush)(MAX_MATCH-MIN_MATCH) &&
	       (ush)D_CODE(dist) < (ush)D_CODES,  "ct_tally: bad match");

	encoder->dyn_ltree[encoder->length_code[lc]+LITERALS+1].Freq++;
	encoder->dyn_dtree[D_CODE(dist)].Freq++;

	encoder->d_buf[encoder->last_dist++] = (ush)dist;
	encoder->flags |= encoder->flag_bit;
    }
    encoder->flag_bit <<= 1;

    /* Output the flags if they fill a byte: */
    if((encoder->last_lit & 7) == 0) {
	encoder->flag_buf[encoder->last_flags++] = encoder->flags;
	encoder->flags = 0;
	encoder->flag_bit = 1;
    }
    /* Try to guess if it is profitable to stop the current block here */
    if(encoder->compr_level > 2 && (encoder->last_lit & 0xfff) == 0) {
	/* Compute an upper bound for the compressed length */
	ulg out_length = (ulg)encoder->last_lit*8L;
	ulg in_length = (ulg)encoder->strstart - encoder->block_start;
	int dcode;

	for(dcode = 0; dcode < D_CODES; dcode++) {
	    out_length +=
		(ulg)encoder->dyn_dtree[dcode].Freq *
		    (5L + extra_dbits[dcode]);
	}
	out_length >>= 3;
	Trace((stderr,"\nlast_lit %u, last_dist %u, in %ld, out ~%ld(%ld%%) ",
	       encoder->last_lit, encoder->last_dist, in_length, out_length,
	       100L - out_length*100L/in_length));
	if(encoder->last_dist < encoder->last_lit/2 &&
	    out_length < in_length/2)
	    return 1;
    }
    return (encoder->last_lit == LIT_BUFSIZE-1 ||
	    encoder->last_dist == DIST_BUFSIZE);
    /* We avoid equality with LIT_BUFSIZE because of wraparound at 64K
     * on 16 bit machines and because stored blocks are restricted to
     * 64K-1 bytes.
     */
}

/* ===========================================================================
 * Send the block data compressed using the given Huffman trees
 */
local void compress_block(
    DeflateHandler encoder,
    ct_data near *ltree, /* literal tree */
    ct_data near *dtree) /* distance tree */
{
    unsigned dist;	/* distance of matched string */
    int lc;		/* match length or unmatched char (if dist == 0) */
    unsigned lx = 0;	/* running index in l_buf */
    unsigned dx = 0;	/* running index in d_buf */
    unsigned fx = 0;	/* running index in flag_buf */
    uch flag = 0;	/* current flags */
    unsigned code;	/* the code to send */
    int extra;		/* number of extra bits to send */

    if(encoder->last_lit != 0) do {
	if((lx & 7) == 0)
	    flag = encoder->flag_buf[fx++];
	lc = encoder->l_buf[lx++];
	if((flag & 1) == 0) {
	    SEND_CODE(lc, ltree); /* send a literal byte */
	    Tracecv(isgraph(lc), (stderr," '%c' ", lc));
	} else {
	    /* Here, lc is the match length - MIN_MATCH */
	    code = encoder->length_code[lc];
	    SEND_CODE(code+LITERALS+1, ltree); /* send the length code */
	    extra = extra_lbits[code];
	    if(extra != 0) {
		lc -= encoder->base_length[code];
		send_bits(encoder, lc, extra); /* send the extra length bits */
	    }
	    dist = encoder->d_buf[dx++];
	    /* Here, dist is the match distance - 1 */
	    code = D_CODE(dist);
	    Assert (code < D_CODES, "bad d_code");

	    SEND_CODE(code, dtree);	  /* send the distance code */
	    extra = extra_dbits[code];
	    if(extra != 0) {
		dist -= encoder->base_dist[code];
		send_bits(encoder, dist, extra);   /* send the extra distance bits */
	    }
	} /* literal or match pair ? */
	flag >>= 1;
    } while(lx < encoder->last_lit);

    SEND_CODE(END_BLOCK, ltree);
}

/* ===========================================================================
 * Send a value on a given number of bits.
 * IN assertion: length <= 16 and value fits in length bits.
 */
#define Buf_size (8 * sizeof(ush)) /* bit size of bi_buf */
local void send_bits(
    DeflateHandler encoder,
    int value,	/* value to send */
    int length)	/* number of bits */
{
    /* If not enough room in bi_buf, use (valid) bits from bi_buf and
     * (16 - bi_valid) bits from value, leaving (width - (16-bi_valid))
     * unused bits in value.
     */
    if(encoder->bi_valid > Buf_size - length) {
	encoder->bi_buf |= (value << encoder->bi_valid);
	put_short(encoder->bi_buf);
	encoder->bi_buf = (ush)value >> (Buf_size - encoder->bi_valid);
	encoder->bi_valid += length - Buf_size;
    } else {
	encoder->bi_buf |= value << encoder->bi_valid;
	encoder->bi_valid += length;
    }
}

/* ===========================================================================
 * Reverse the first len bits of a code, using straightforward code (a faster
 * method would use a table)
 * IN assertion: 1 <= len <= 15
 */
local unsigned bi_reverse(
    unsigned code,	/* the value to invert */
    int len)		/* its bit length */
{
    register unsigned res = 0;
    do {
	res |= code & 1;
	code >>= 1, res <<= 1;
    } while(--len > 0);
    return res >> 1;
}

/* ===========================================================================
 * Write out any remaining bits in an incomplete byte.
 */
local void bi_windup(DeflateHandler encoder)
{
    if(encoder->bi_valid > 8) {
	put_short(encoder->bi_buf);
    } else if(encoder->bi_valid > 0) {
	put_byte(encoder->bi_buf);
    }
    encoder->bi_buf = 0;
    encoder->bi_valid = 0;
}

local void qoutbuf(DeflateHandler encoder)
{
    if(encoder->outcnt != 0)
    {
	struct deflate_buff_queue *q;
	q = new_queue();
	if(encoder->qhead == NULL)
	    encoder->qhead = encoder->qtail = q;
	else
	    encoder->qtail = encoder->qtail->next = q;
	q->len = encoder->outcnt - encoder->outoff;
	memcpy(q->ptr, encoder->outbuf + encoder->outoff, q->len);
	encoder->outcnt = encoder->outoff = 0;
    }
}
