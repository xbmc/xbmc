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
#include <stdio.h>
#include <stdlib.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include "timidity.h"
#include "unlzh.h"

#ifndef UCHAR_MAX
#define UCHAR_MAX 255		/* max value for an unsigned char */
#endif
#ifndef CHAR_BIT
#define CHAR_BIT  8
#endif
#define MAX_DICBIT    15
#define MAXMATCH 256    /* formerly F (not more than UCHAR_MAX + 1) */
#define THRESHOLD  3    /* choose optimal value */

#define NC (UCHAR_MAX + MAXMATCH + 2 - THRESHOLD)
			/* alphabet = {0, 1, 2, ..., NC - 1} */
#define CBIT 9  /* $\lfloor \log_2 NC \rfloor + 1$ */
#define USHRT_BIT 16	/* (CHAR_BIT * sizeof(ushort)) */
#define NP (MAX_DICBIT + 1)
#define NT (USHRT_BIT + 3)
#define PBIT 4  /* smallest integer such that (1 << PBIT) > NP */
#define TBIT 5  /* smallest integer such that (1 << TBIT) > NT */
#define NPT 0x80
#define N1 286  /* alphabet size */
#define N2 (2 * N1 - 1)  /* # of nodes in Huffman tree */
#define EXTRABITS 8
	/* >= log2(F-THRESHOLD+258-N1) */
#define BUFBITS  16  /* >= log2(MAXBUF) */
#define LENFIELD  4  /* bit size of length field for tree output */
#define SNP (8 * 1024 / 64)
#define SNP2 (SNP * 2 - 1)
#define N_CHAR      (256 + 60 - THRESHOLD + 1)
#define TREESIZE_C  (N_CHAR * 2)
#define TREESIZE_P  (128 * 2)
#define TREESIZE    (TREESIZE_C + TREESIZE_P)
#define ROOT_C      0
#define ROOT_P      TREESIZE_C
#define MAGIC0 18
#define MAGIC5 19
#define INBUFSIZ BUFSIZ

struct _UNLZHHandler
{
    void *user_val;
    long (* read_func)(char *buff, long buff_size, void *user_val);
    int method;

    unsigned char inbuf[INBUFSIZ];
    int inbuf_size;
    int inbuf_cnt;
    int initflag;

    int cpylen;
    int cpypos;
    unsigned long origsize;
    unsigned long compsize;
    void           (* decode_s)(UNLZHHandler decoder);
    unsigned short (* decode_c)(UNLZHHandler decoder);
    unsigned short (* decode_p)(UNLZHHandler decoder);
    int dicbit;
    unsigned short maxmatch;
    unsigned long count;
    unsigned short loc;
    unsigned char text[1L<<MAX_DICBIT];
    unsigned short bitbuf;
    unsigned char subbitbuf, bitcount;
    unsigned short left[2 * NC - 1], right[2 * NC - 1];
    unsigned char c_len[NC], pt_len[NPT];
    unsigned short c_table[4096], pt_table[256];
    unsigned short blocksize;
    unsigned int n_max;
    short	child [TREESIZE],
		parent[TREESIZE],
		block [TREESIZE],
		edge  [TREESIZE],
		stock [TREESIZE],
		node  [TREESIZE / 2];
    unsigned short freq[TREESIZE];
    unsigned short total_p;
    int avail, n1;
    int most_p, nn;
    unsigned long nextcount;
    unsigned int snp;
    int flag, flagcnt, matchpos;
    int offset;
    unsigned int pbit;
};

static unsigned short decode_c_cpy(UNLZHHandler decoder);
static unsigned short decode_p_cpy(UNLZHHandler decoder);
static void decode_start_cpy(UNLZHHandler decoder);
static void read_pt_len(UNLZHHandler decoder,
			short k, short nbit, short i_special);
static void read_c_len(UNLZHHandler decoder);
static unsigned short decode_c_st1(UNLZHHandler decoder);
static unsigned short decode_p_st1(UNLZHHandler decoder);
static void decode_start_st1(UNLZHHandler decoder);
static void start_c_dyn(UNLZHHandler decoder);
static void start_p_dyn(UNLZHHandler decoder);
static void decode_start_dyn(UNLZHHandler decoder);
static void reconst(UNLZHHandler decoder, int start, int end);
static int swap_inc(UNLZHHandler decoder, int p);
static void update_c(UNLZHHandler decoder, int p);
static void update_p(UNLZHHandler decoder, int p);
static void make_new_node(UNLZHHandler decoder, int p);
static unsigned short decode_c_dyn(UNLZHHandler decoder);
static unsigned short decode_p_dyn(UNLZHHandler decoder);
static void decode_start_st0(UNLZHHandler decoder);
static void ready_made(UNLZHHandler decoder, int method);
static void read_tree_c(UNLZHHandler decoder);
static void read_tree_p(UNLZHHandler decoder);
static void decode_start_fix(UNLZHHandler decoder);
static unsigned short decode_c_st0(UNLZHHandler decoder);
static unsigned short decode_p_st0(UNLZHHandler decoder);
static unsigned short decode_c_lzs(UNLZHHandler decoder);
static unsigned short decode_p_lzs(UNLZHHandler decoder);
static void decode_start_lzs(UNLZHHandler decoder);
static unsigned short decode_c_lz5(UNLZHHandler decoder);
static unsigned short decode_p_lz5(UNLZHHandler decoder);
static void decode_start_lz5(UNLZHHandler decoder);
static int make_table(UNLZHHandler decoder,
		       int nchar, unsigned char bitlen[],
		       int tablebits, unsigned short table[]);
static int fill_inbuf(UNLZHHandler decoder);
static void fillbuf(UNLZHHandler decoder, unsigned char n);
static unsigned short getbits(UNLZHHandler decoder, unsigned char n);
static void init_getbits(UNLZHHandler decoder);

#define NEXTBYTE (decoder->inbuf_cnt < decoder->inbuf_size ? (int)decoder->inbuf[decoder->inbuf_cnt++] : fill_inbuf(decoder))

static struct
{
    char *id;
    int dicbit;
    void           (*decode_s)(UNLZHHandler decoder);
    unsigned short (*decode_c)(UNLZHHandler decoder);
    unsigned short (*decode_p)(UNLZHHandler decoder);
} method_table[] =
{
/* No compression */
    {"-lh0-",  0, decode_start_cpy, decode_c_cpy, decode_p_cpy},

/* 4k sliding dictionary(max 60 bytes)
   + dynamic Huffman + fixed encoding of position */
    {"-lh1-", 12, decode_start_fix, decode_c_dyn, decode_p_st0},

/* 8k sliding dictionary(max 256 bytes) + dynamic Huffman */
    {"-lh2-", 13, decode_start_dyn, decode_c_dyn, decode_p_dyn},

/* 8k sliding dictionary(max 256 bytes) + static Huffman */
    {"-lh3-", 13, decode_start_st0, decode_c_st0, decode_p_st0},

/* 4k sliding dictionary(max 256 bytes)
   + static Huffman + improved encoding of position and trees */
    {"-lh4-", 12, decode_start_st1, decode_c_st1, decode_p_st1},

/* 8k sliding dictionary(max 256 bytes)
   + static Huffman + improved encoding of position and trees */
    {"-lh5-", 13, decode_start_st1, decode_c_st1, decode_p_st1},

/* 2k sliding dictionary(max 17 bytes) */
    {"-lzs-", 11, decode_start_lzs, decode_c_lzs, decode_p_lzs},

/* No compression */
    {"-lz5-", 12, decode_start_lz5, decode_c_lz5, decode_p_lz5},

/* 4k sliding dictionary(max 17 bytes) */
    {"-lz4-",  0, decode_start_cpy, decode_c_cpy, decode_p_cpy},

/* Directory */
    {"-lhd-",  0, NULL, NULL, NULL},

/* 32k sliding dictionary + static Huffman */
    {"-lh6-", 15, decode_start_st1, decode_c_st1, decode_p_st1},

#if 0 /* not supported */
/* 64k sliding dictionary + static Huffman */
    {"-lh7-", 16, decode_start_st1, decode_c_st1, decode_p_st1},
#endif
    {NULL, 0, NULL, NULL}
};

char *lzh_methods[] =
{
    "-lh0-", "-lh1-", "-lh2-", "-lh3-", "-lh4-", "-lh5-",
    "-lzs-", "-lz5-", "-lz4-", "-lhd-", "-lh6-", "-lh7-", NULL
};

/*ARGSUSED*/
static long default_read_func(char *buf, long size, void *v)
{
    return (long)fread(buf, 1, size, stdin);
}

UNLZHHandler open_unlzh_handler(long (* read_func)(char *, long, void *),
				const char *method,
				long compsize, long origsize, void *user_val)
{
    UNLZHHandler decoder;
    int i;

    for(i = 0; method_table[i].id != NULL; i++)
	if(!strcmp(method_table[i].id, method))
	    break;
    if(method_table[i].id == NULL)
	return NULL; /* Invalid method */

    if((decoder = (UNLZHHandler)malloc(sizeof(struct _UNLZHHandler))) == NULL)
	return NULL;
    memset(decoder, 0, sizeof(struct _UNLZHHandler));

    if(strcmp(method, "-lhd-") == 0)
	origsize = 0;

    decoder->method = i;
    decoder->dicbit = method_table[i].dicbit;
    decoder->decode_s = method_table[i].decode_s;
    decoder->decode_c = method_table[i].decode_c;
    decoder->decode_p = method_table[i].decode_p;
    decoder->compsize = compsize;
    decoder->origsize = origsize;
    decoder->user_val = user_val;
    decoder->cpylen = 0;
    decoder->cpypos = 0;
    decoder->offset = (decoder->method == 6) ? 0x100 - 2 : 0x100 - 3;
    decoder->count = 0;
    decoder->loc = 0;
    decoder->initflag = 0;

    if(read_func == NULL)
	decoder->read_func = default_read_func;
    else
	decoder->read_func = read_func;

    return decoder;
}

void close_unlzh_handler(UNLZHHandler decoder)
{
    free(decoder);
}

long unlzh(UNLZHHandler decoder, char *buff, long buff_size)
{
    long n;
    unsigned short dicsiz1;
    int offset;
    int cpylen, cpypos, loc;
    unsigned char *text;
    unsigned long origsize;

    if((origsize = decoder->origsize) == 0 || buff_size <= 0)
	return 0;

    if(!decoder->initflag)
    {
	decoder->initflag = 1;
	decoder->decode_s(decoder);
    }

    dicsiz1 = (1 << decoder->dicbit) - 1;
    n = 0;
    text = decoder->text;

    if(decoder->cpylen > 0)
    {
	cpylen = decoder->cpylen;
	cpypos = decoder->cpypos;
	loc    = decoder->loc;
	while(cpylen > 0 && n < buff_size)
	{
	    buff[n++] = text[loc++] = text[cpypos++];
	    loc &= dicsiz1;
	    cpypos &= dicsiz1;
	    cpylen--;
	}
	decoder->cpylen = cpylen;
	decoder->cpypos = cpypos;
	decoder->loc = loc;
    }

    if(n == buff_size)
	return buff_size;

    offset = decoder->offset;
    while(decoder->count < origsize && n < buff_size)
    {
	int c;

	c = decoder->decode_c(decoder);
	if(c <= UCHAR_MAX)
	{
	    buff[n++] = decoder->text[decoder->loc++] = c;
	    decoder->loc &= dicsiz1;
	    decoder->count++;
	}
	else
	{
	    int i, j, k, m;

	    j = c - offset;
	    i = (decoder->loc - decoder->decode_p(decoder) - 1) & dicsiz1;
	    decoder->count += j;
	    loc = decoder->loc;
	    m = buff_size - n;
	    if(m > j)
		m = j;
	    for(k = 0; k < m; k++)
	    {
		buff[n++] = text[loc++] = text[i++];
		loc &= dicsiz1;
		i &= dicsiz1;
	    }
	    decoder->loc = loc;
	    if(k < j)
	    {
		decoder->cpylen = j - k;
		decoder->cpypos = i;
		break;
	    }
	}
    }

    return n;
}

static unsigned short decode_c_cpy(UNLZHHandler decoder)
{
    int c;

    if((c = NEXTBYTE) == EOF)
	return 0;
    return (unsigned short)c;
}

/*ARGSUSED*/
static unsigned short decode_p_cpy(UNLZHHandler decoder)
{
    return 0;
}

static void decode_start_cpy(UNLZHHandler decoder)
{
    init_getbits(decoder);
}

static void read_pt_len(UNLZHHandler decoder,
			short k, short nbit, short i_special)
{
    short i, c, n;

    n = getbits(decoder, nbit);
    if(n == 0)
    {
	c = getbits(decoder, nbit);
	for(i = 0; i < k; i++)
	    decoder->pt_len[i] = 0;
	for(i = 0; i < 256; i++)
	    decoder->pt_table[i] = c;
    }
    else
    {
	i = 0;
	while(i < n)
	{
	    c = decoder->bitbuf >> (16 - 3);
	    if (c == 7)
	    {
		unsigned short mask = 1 << (16 - 4);
		while(mask & decoder->bitbuf)
		{
		    mask >>= 1;
		    c++;
		}
	    }
	    fillbuf(decoder, (c < 7) ? 3 : c - 3);
	    decoder->pt_len[i++] = c;
	    if(i == i_special)
	    {
		c = getbits(decoder, 2);
		while(--c >= 0)
		    decoder->pt_len[i++] = 0;
	    }
	}
	while(i < k)
	    decoder->pt_len[i++] = 0;
	make_table(decoder, k, decoder->pt_len, 8, decoder->pt_table);
    }
}

static void read_c_len(UNLZHHandler decoder)
{
    short i, c, n;

    n = getbits(decoder, CBIT);
    if(n == 0)
    {
	c = getbits(decoder, CBIT);
	for(i = 0; i < NC; i++)
	    decoder->c_len[i] = 0;
	for(i = 0; i < 4096; i++)
	    decoder->c_table[i] = c;
    }
    else
    {
	i = 0;
	while(i < n)
	{
	    c = decoder->pt_table[decoder->bitbuf >> (16 - 8)];
	    if(c >= NT)
	    {
		unsigned short mask = 1 << (16 - 9);
		do
		{
		    if(decoder->bitbuf & mask)
			c = decoder->right[c];
		    else
			c = decoder->left[c];
		    mask >>= 1;
		} while(c >= NT);
	    }
	    fillbuf(decoder, decoder->pt_len[c]);
	    if(c <= 2)
	    {
		if(c == 0)
		    c = 1;
		else if(c == 1)
		    c = getbits(decoder, 4) + 3;
		else
		    c = getbits(decoder, CBIT) + 20;
		while(--c >= 0)
		    decoder->c_len[i++] = 0;
	    }
	    else
		decoder->c_len[i++] = c - 2;
	}
	while(i < NC)
	    decoder->c_len[i++] = 0;
	make_table(decoder, NC, decoder->c_len, 12, decoder->c_table);
    }
}

static unsigned short decode_c_st1(UNLZHHandler decoder)
{
    unsigned short j, mask;

    if(decoder->blocksize == 0)
    {
	decoder->blocksize = getbits(decoder, 16);
	read_pt_len(decoder, NT, TBIT, 3);
	read_c_len(decoder);
	read_pt_len(decoder, decoder->snp, decoder->pbit, -1);
    }
    decoder->blocksize--;
    j = decoder->c_table[decoder->bitbuf >> 4];
    if(j < NC)
	fillbuf(decoder, decoder->c_len[j]);
    else
    {
	fillbuf(decoder, 12);
	mask = 1 << (16 - 1);
	do
	{
	    if(decoder->bitbuf & mask)
		j = decoder->right[j];
	    else
		j = decoder->left[j];
	    mask >>= 1;
	} while(j >= NC);
	fillbuf(decoder, decoder->c_len[j] - 12);
    }
    return j;
}

static unsigned short decode_p_st1(UNLZHHandler decoder)
{
    unsigned short j, mask;
    unsigned int np = decoder->snp;

    j = decoder->pt_table[decoder->bitbuf >> (16 - 8)];
    if(j < np)
	fillbuf(decoder, decoder->pt_len[j]);
    else
    {
	fillbuf(decoder, 8);
	mask = 1 << (16 - 1);
	do
	{
	    if(decoder->bitbuf & mask)
		j = decoder->right[j];
	    else
		j = decoder->left[j];
	    mask >>= 1;
	} while(j >= np);
	fillbuf(decoder, decoder->pt_len[j] - 8);
    }
    if(j != 0)
	j = (1 << (j - 1)) + getbits(decoder, j - 1);
    return j;
}


static void decode_start_st1(UNLZHHandler decoder)
{
    if(decoder->dicbit <= (MAX_DICBIT - 2)) {
	decoder->snp = 14;
	decoder->pbit = 4;
    } else {
	decoder->snp = 16;
	decoder->pbit = 5;
    }

    init_getbits(decoder);
    decoder->blocksize = 0;
}

static void start_c_dyn(UNLZHHandler decoder)
{
    int i, j, f;

    decoder->n1 = (decoder->n_max >= 256 + decoder->maxmatch - THRESHOLD + 1)
	? 512 : decoder->n_max - 1;
    for(i = 0; i < TREESIZE_C; i++)
    {
	decoder->stock[i] = i;
	decoder->block[i] = 0;
    }
    for(i = 0, j = decoder->n_max * 2 - 2; i < decoder->n_max; i++, j--)
    {
	decoder->freq[j] = 1;
	decoder->child[j] = ~i;
	decoder->node[i] = j;
	decoder->block[j] = 1;
    }
    decoder->avail = 2;
    decoder->edge[1] = decoder->n_max - 1;
    i = decoder->n_max * 2 - 2;
    while(j >= 0)
    {
	f = decoder->freq[j] = decoder->freq[i] + decoder->freq[i - 1];
	decoder->child[j] = i;
	decoder->parent[i] = decoder->parent[i - 1] = j;
	if(f == decoder->freq[j + 1])
	{
	    decoder->edge[decoder->block[j] = decoder->block[j + 1]] = j;
	}
	else
	{
	    decoder->edge[decoder->block[j] =
			  decoder->stock[decoder->avail++]] = j;
	}
	i -= 2;
	j--;
    }
}

static void start_p_dyn(UNLZHHandler decoder)
{
    decoder->freq[ROOT_P] = 1;
    decoder->child[ROOT_P] = ~(N_CHAR);
    decoder->node[N_CHAR] = ROOT_P;
    decoder->edge[decoder->block[ROOT_P] =
		  decoder->stock[decoder->avail++]] = ROOT_P;
    decoder->most_p = ROOT_P;
    decoder->total_p = 0;
    decoder->nn = 1 << decoder->dicbit;
    decoder->nextcount = 64;
}

static void decode_start_dyn(UNLZHHandler decoder)
{
    decoder->n_max = 286;
    decoder->maxmatch = MAXMATCH;
    init_getbits(decoder);
    start_c_dyn(decoder);
    start_p_dyn(decoder);
}

static void reconst(UNLZHHandler decoder, int start, int end)
{
    int i, j, k, l, b;
    unsigned int f, g;

    b = 0;
    for(i = j = start; i < end; i++)
    {
	if((k = decoder->child[i]) < 0)
	{
	    decoder->freq[j] = (decoder->freq[i] + 1) / 2;
	    decoder->child[j] = k;
	    j++;
	}
	if(decoder->edge[b = decoder->block[i]] == i)
	{
	    decoder->stock[--decoder->avail] = b;
	}
    }
    j--;
    i = end - 1;
    l = end - 2;
    while(i >= start)
    {
	while (i >= l)
	{
	    decoder->freq[i] = decoder->freq[j];
	    decoder->child[i] = decoder->child[j];
	    i--;
	    j--;
	}
	f = decoder->freq[l] + decoder->freq[l + 1];
	for(k = start; f < decoder->freq[k]; k++)
	    ;
	while(j >= k)
	{
	    decoder->freq[i] = decoder->freq[j];
	    decoder->child[i] = decoder->child[j];
	    i--;
	    j--;
	}
	decoder->freq[i] = f;
	decoder->child[i] = l + 1;
	i--;
	l -= 2;
    }
    f = 0;
    for(i = start; i < end; i++)
    {
	if((j = decoder->child[i]) < 0)
	    decoder->node[~j] = i;
	else
	    decoder->parent[j] = decoder->parent[j - 1] = i;
	if((g = decoder->freq[i]) == f)
	{
	    decoder->block[i] = b;
	}
	else
	{
	    decoder->edge[b = decoder->block[i]
			  = decoder->stock[decoder->avail++]] = i;
	    f = g;
	}
    }
}

static int swap_inc(UNLZHHandler decoder, int p)
{
    int b, q, r, s;

    b = decoder->block[p];
    if((q = decoder->edge[b]) != p)
    {	/* swap for leader */
	r = decoder->child[p];
	s = decoder->child[q];
	decoder->child[p] = s;
	decoder->child[q] = r;
	if(r >= 0)
	    decoder->parent[r] = decoder->parent[r - 1] = q;
	else
	    decoder->node[~r] = q;
	if(s >= 0)
	    decoder->parent[s] = decoder->parent[s - 1] = p;
	else
	    decoder->node[~s] = p;
	p = q;
	goto Adjust;
    }
    else if(b == decoder->block[p + 1])
    {
      Adjust:
	decoder->edge[b]++;
	if(++decoder->freq[p] == decoder->freq[p - 1])
	{
	    decoder->block[p] = decoder->block[p - 1];
	}
	else
	{
	    decoder->edge[decoder->block[p] =
		 decoder->stock[decoder->avail++]] = p;	/* create block */
	}
    }
    else if (++decoder->freq[p] == decoder->freq[p - 1])
    {
	decoder->stock[--decoder->avail] = b;		/* delete block */
	decoder->block[p] = decoder->block[p - 1];
    }
    return decoder->parent[p];
}

static void update_c(UNLZHHandler decoder, int p)
{
    int q;

    if(decoder->freq[ROOT_C] == 0x8000)
    {
	reconst(decoder, 0, decoder->n_max * 2 - 1);
    }
    decoder->freq[ROOT_C]++;
    q = decoder->node[p];
    do
    {
	q = swap_inc(decoder, q);
    } while(q != ROOT_C);
}

static void update_p(UNLZHHandler decoder, int p)
{
    int q;

    if(decoder->total_p == 0x8000)
    {
	reconst(decoder, ROOT_P, decoder->most_p + 1);
	decoder->total_p = decoder->freq[ROOT_P];
	decoder->freq[ROOT_P] = 0xffff;
    }
    q = decoder->node[p + N_CHAR];
    while(q != ROOT_P)
    {
	q = swap_inc(decoder, q);
    }
    decoder->total_p++;
}

static void make_new_node(UNLZHHandler decoder, int p)
{
    int q, r;

    r = decoder->most_p + 1;
    q = r + 1;
    decoder->node[~(decoder->child[r] = decoder->child[decoder->most_p])] = r;
    decoder->child[q] = ~(p + N_CHAR);
    decoder->child[decoder->most_p] = q;
    decoder->freq[r] = decoder->freq[decoder->most_p];
    decoder->freq[q] = 0;
    decoder->block[r] = decoder->block[decoder->most_p];
    if(decoder->most_p == ROOT_P)
    {
	decoder->freq[ROOT_P] = 0xffff;
	decoder->edge[decoder->block[ROOT_P]]++;
    }
    decoder->parent[r] = decoder->parent[q] = decoder->most_p;
    decoder->edge[decoder->block[q] = decoder->stock[decoder->avail++]]
	= decoder->node[p + N_CHAR] = decoder->most_p = q;
    update_p(decoder, p);
}

static unsigned short decode_c_dyn(UNLZHHandler decoder)
{
    int c;
    short buf, cnt;

    c = decoder->child[ROOT_C];
    buf = decoder->bitbuf;
    cnt = 0;
    do
    {
	c = decoder->child[c - (buf < 0)];
	buf <<= 1;
	if(++cnt == 16)
	{
	    fillbuf(decoder, 16);
	    buf = decoder->bitbuf;
	    cnt = 0;
	}
    }while (c > 0);
    fillbuf(decoder, cnt);
    c = ~c;
    update_c(decoder, c);
    if(c == decoder->n1)
	c += getbits(decoder, 8);
    return c;
}

static unsigned short decode_p_dyn(UNLZHHandler decoder)
{
    int c;
    short buf, cnt;

    while(decoder->count > decoder->nextcount)
    {
	make_new_node(decoder, (int)(decoder->nextcount / 64));
	if((decoder->nextcount += 64) >= decoder->nn)
	    decoder->nextcount = 0xffffffff;
    }
    c = decoder->child[ROOT_P];
    buf = decoder->bitbuf;
    cnt = 0;
    while(c > 0)
    {
	c = decoder->child[c - (buf < 0)];
	buf <<= 1;
	if(++cnt == 16)
	{
	    fillbuf(decoder, 16);
	    buf = decoder->bitbuf;
	    cnt = 0;
	}
    }
    fillbuf(decoder, cnt);
    c = (~c) - N_CHAR;
    update_p(decoder, c);

    return (c << 6) + getbits(decoder, 6);
}

static void decode_start_st0(UNLZHHandler decoder)
{
    decoder->n_max = 286;
    decoder->maxmatch = MAXMATCH;
    init_getbits(decoder);
    decoder->snp = 1 << (MAX_DICBIT - 6);
    decoder->blocksize = 0;
}

static int fixed[2][16] = {
    {3, 0x01, 0x04, 0x0c, 0x18, 0x30, 0},		/* old compatible */
    {2, 0x01, 0x01, 0x03, 0x06, 0x0D, 0x1F, 0x4E, 0}	/* 8K buf */
};

static void ready_made(UNLZHHandler decoder, int method)
{
    int i, j;
    unsigned int code, weight;
    int *tbl;

    tbl = fixed[method];
    j = *tbl++;
    weight = 1 << (16 - j);
    code = 0;
    for(i = 0; i < decoder->snp; i++)
    {
	while(*tbl == i)
	{
	    j++;
	    tbl++;
	    weight >>= 1;
	}
	decoder->pt_len[i] = j;
	code += weight;
    }
}

static void read_tree_c(UNLZHHandler decoder)  /* read tree from file */
{
    int i, c;

    i = 0;
    while(i < N1)
    {
	if(getbits(decoder, 1))
	    decoder->c_len[i] = getbits(decoder, LENFIELD) + 1;
	else
	    decoder->c_len[i] = 0;
	if(++i == 3 && decoder->c_len[0] == 1 &&
	   decoder->c_len[1] == 1 && decoder->c_len[2] == 1)
	{
	    c = getbits(decoder, CBIT);
	    for(i = 0; i < N1; i++)
		decoder->c_len[i] = 0;
	    for(i = 0; i < 4096; i++)
		decoder->c_table[i] = c;
	    return;
	}
    }
    make_table(decoder, N1, decoder->c_len, 12, decoder->c_table);
}

static void read_tree_p(UNLZHHandler decoder)  /* read tree from file */
{
    int i, c;

    i = 0;
    while(i < SNP)
    {
	decoder->pt_len[i] = getbits(decoder, LENFIELD);
	if(++i == 3 && decoder->pt_len[0] == 1 &&
	   decoder->pt_len[1] == 1 && decoder->pt_len[2] == 1)
	{
	    c = getbits(decoder, MAX_DICBIT - 6);
	    for(i = 0; i < SNP; i++)
		decoder->c_len[i] = 0;
	    for(i = 0; i < 256; i++)
		decoder->c_table[i] = c;
	    return;
	}
    }
}

static void decode_start_fix(UNLZHHandler decoder)
{
    decoder->n_max = 314;
    decoder->maxmatch = 60;
    init_getbits(decoder);
    decoder->snp = 1 << (12 - 6);
    start_c_dyn(decoder);
    ready_made(decoder, 0);
    make_table(decoder, decoder->snp, decoder->pt_len, 8, decoder->pt_table);
}

static unsigned short decode_c_st0(UNLZHHandler decoder)
{
    int i, j;

    if(decoder->blocksize == 0) /* read block head */
    {
	/* read block blocksize */
	decoder->blocksize = getbits(decoder, BUFBITS);
	read_tree_c(decoder);
	if(getbits(decoder, 1))
	{
	    read_tree_p(decoder);
	}
	else
	{
	    ready_made(decoder, 1);
	}
	make_table(decoder, SNP, decoder->pt_len, 8, decoder->pt_table);
    }
    decoder->blocksize--;
    j = decoder->c_table[decoder->bitbuf >> 4];
    if(j < N1)
	fillbuf(decoder, decoder->c_len[j]);
    else
    {
	fillbuf(decoder, 12);
	i = decoder->bitbuf;
	do
	{
	    if((short)i < 0)
		j = decoder->right[j];
	    else
		j = decoder->left [j];
	    i <<= 1;
	} while(j >= N1);
	fillbuf(decoder, decoder->c_len[j] - 12);
    }
    if(j == N1 - 1)
	j += getbits(decoder, EXTRABITS);
    return j;
}

static unsigned short decode_p_st0(UNLZHHandler decoder)
{
    int i, j;

    j = decoder->pt_table[decoder->bitbuf >> 8];
    if(j < decoder->snp)
    {
	fillbuf(decoder, decoder->pt_len[j]);
    }
    else
    {
	fillbuf(decoder, 8);
	i = decoder->bitbuf;
	do
	{
	    if((short)i < 0)
		j = decoder->right[j];
	    else
		j = decoder->left[j];
	    i <<= 1;
	} while(j >= decoder->snp);
	fillbuf(decoder, decoder->pt_len[j] - 8);
    }
    return (j << 6) + getbits(decoder, 6);
}

static unsigned short decode_c_lzs(UNLZHHandler decoder)
{
    if(getbits(decoder, 1))
	return getbits(decoder, 8);
    decoder->matchpos = getbits(decoder, 11);
    return getbits(decoder, 4) + 0x100;
}

static unsigned short decode_p_lzs(UNLZHHandler decoder)
{
    return (decoder->loc - decoder->matchpos - MAGIC0) & 0x7ff;
}

static void decode_start_lzs(UNLZHHandler decoder)
{
    init_getbits(decoder);
}

static unsigned short decode_c_lz5(UNLZHHandler decoder)
{
    int c;

    if(decoder->flagcnt == 0)
    {
	decoder->flagcnt = 8;
	decoder->flag = NEXTBYTE;
    }
    decoder->flagcnt--;
    c = NEXTBYTE;
    if((decoder->flag & 1) == 0)
    {
	decoder->matchpos = c;
	c = NEXTBYTE;
	decoder->matchpos += (c & 0xf0) << 4;
	c &= 0x0f;
	c += 0x100;
    }
    decoder->flag >>= 1;
    return c;
}

static unsigned short decode_p_lz5(UNLZHHandler decoder)
{
    return (decoder->loc - decoder->matchpos - MAGIC5) & 0xfff;
}

static void decode_start_lz5(UNLZHHandler decoder)
{
    int i;

    decoder->flagcnt = 0;
    for(i = 0; i < 256; i++)
	memset(&decoder->text[i * 13 + 18], i, 13);
    for(i = 0; i < 256; i++)
	decoder->text[256 * 13 + 18 + i] = i;
    for(i = 0; i < 256; i++)
	decoder->text[256 * 13 + 256 + 18 + i] = 255 - i;
    memset(&decoder->text[256 * 13 + 512 + 18], 0, 128);
    memset(&decoder->text[256 * 13 + 512 + 128 + 18], ' ', 128 - 18);
}

static int make_table(UNLZHHandler decoder,
		       int nchar, unsigned char bitlen[],
		       int tablebits, unsigned short table[])
{
    unsigned short cnttable[17];  /* count of bitlen */
    unsigned short weight[17]; /* 0x10000ul >> bitlen */
    unsigned short start[17];  /* first code of bitlen */
    unsigned short total;
    unsigned int i;
    int j, k, l, m, n, available;
    unsigned short *p;

    available = nchar;

/* initialize */
    for(i = 1; i <= 16; i++)
    {
	cnttable[i] = 0;
	weight[i] = 1 << (16 - i);
    }

/* cnttable */
    for(i = 0; i < nchar; i++)
	cnttable[bitlen[i]]++;

/* calculate first code */
    total = 0;
    for(i = 1; i <= 16; i++)
    {
	start[i] = total;
	total += weight[i] * cnttable[i];
    }
    if((total & 0xffff) != 0)
    {
	fprintf(stderr, "Decode: Bad table (5)\n");
	/*exit(1);*/ /* for win32gui i/f  2002/8/17 */
	return 1;
    }

/* shift data for make table. */
    m = 16 - tablebits;
    for(i = 1; i <= tablebits; i++)
    {
	start[i] >>= m;
	weight[i] >>= m;
    }

/* initialize */
    j = start[tablebits + 1] >> m;
    k = 1 << tablebits;
    if(j != 0)
	for(i = j; i < k; i++)
	    table[i] = 0;

/* create table and tree */
    for(j = 0; j < nchar; j++)
    {
	k = bitlen[j];
	if(k == 0)
	    continue;
	l = start[k] + weight[k];
	if(k <= tablebits)
	{
	    /* code in table */
	    for(i = start[k]; i < l; i++)
		table[i] = j;
	}
	else
	{
	    /* code not in table */
	    p = &table[(i = start[k]) >> m];
	    i <<= tablebits;
	    n = k - tablebits;
	    /* make tree (n length) */
	    while(--n >= 0)
	    {
		if(*p == 0)
		{
		    decoder->right[available] = decoder->left[available] = 0;
		    *p = available++;
		}
		if(i & 0x8000)
		    p = &decoder->right[*p];
		else
		    p = &decoder->left[*p];
		i <<= 1;
	    }
	    *p = j;
	}
	start[k] = l;
    }
    return 0;
}

static int fill_inbuf(UNLZHHandler decoder)
{
    long n, i;

    if(decoder->compsize == 0)
	return EOF;
    i = INBUFSIZ;
    if(i > decoder->compsize)
	i = (long)decoder->compsize;
    n = decoder->read_func((char *)decoder->inbuf, i, decoder->user_val);
    if(n <= 0)
	return EOF;
    decoder->inbuf_size = n;
    decoder->inbuf_cnt = 1;
    decoder->compsize -= n;
    return (int)decoder->inbuf[0];
}

/* Shift bitbuf n bits left, read n bits */
static void fillbuf(UNLZHHandler decoder, unsigned char n)
{
    unsigned char bitcount;
    unsigned short bitbuf;

    bitcount = decoder->bitcount;
    bitbuf   = decoder->bitbuf;
    while(n > bitcount)
    {
	n -= bitcount;
	bitbuf = (bitbuf << bitcount)
	    + (decoder->subbitbuf >> (CHAR_BIT - bitcount));
	decoder->subbitbuf = (unsigned char)NEXTBYTE;
	bitcount = CHAR_BIT;
    }
    bitcount -= n;
    bitbuf = (bitbuf << n) + (decoder->subbitbuf >> (CHAR_BIT - n));
    decoder->subbitbuf <<= n;
    decoder->bitcount = bitcount;
    decoder->bitbuf   = bitbuf;
}

static unsigned short getbits(UNLZHHandler decoder, unsigned char n)
{
    unsigned short x;

    x = decoder->bitbuf >> (2 * CHAR_BIT - n);
    fillbuf(decoder, n);
    return x;
}

static void init_getbits(UNLZHHandler decoder)
{
    decoder->bitbuf = 0;
    decoder->subbitbuf = 0;
    decoder->bitcount = 0;
    decoder->inbuf_cnt = 0;
    decoder->inbuf_size = 0;
    fillbuf(decoder, 2 * CHAR_BIT);
}
