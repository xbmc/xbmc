/**
 *
 *  LZHXLib.C - Decompression module
 *
 *  Compression Library (LZH) from Haruhiko Okumura's "ar".
 *
 *  Copyright(c) 1996 Kerwin F. Medina
 *  Copyright(c) 1991 Haruhiko Okumura
 *
 *  In MSDOS, this MUST be compiled using compact, large,
 *      or huge memory models
 *
 *  To test, compile as below to create a stand-alone decompressor (will
 *      decompress standard input to standard output):
 *          bcc -O2 -mc -D__TEST__ lzhxlib.c
 *              or
 *          gcc -O2 -D__TEST__ lzhxlib.c
 *      then run as: lzhxlib <infile >outfile
 *
 *  To use as a library for your application, __TEST__ must
 *      not be defined.
 *
 */

#include "lzh.h"

/*
 * Additions
 */

static type_fnc_read   fnc_read;
static type_fnc_write  fnc_write;
static type_fnc_malloc fnc_malloc;
static type_fnc_free   fnc_free;
static int with_error;

#define BUFSIZE (1024 * 4)
static int fillbufsize;
static uchar far* buf;

/*
 * io.c
 */

static ushort far left [2 * NC - 1], far right[2 * NC - 1];

static BITBUFTYPE bitbuf;
static uint       subbitbuf;
static int        bitcount;

/** Shift bitbuf n bits left, read n bits */
static void fillbuf (int n)
{
    static uint i;
    bitbuf = (bitbuf << n) & 0xffff;
    while (n > bitcount)
    {
        bitbuf |= subbitbuf << (n -= bitcount);
        if (fillbufsize == 0)
        {
            i = 0;
            fillbufsize = fnc_read (buf, BUFSIZE - 32);
		}
		if (fillbufsize > 0)
            fillbufsize--, subbitbuf = buf[i++];
        else
            subbitbuf = 0;
        bitcount = CHAR_BIT;
    }
    bitbuf |= subbitbuf >> (bitcount -= n);
}

static ushort getbits (int n)
{
    ushort x;
    x = bitbuf >> (BITBUFSIZ - n);
    fillbuf (n);
    return x;
}

static void init_getbits (void)
{
    bitbuf = 0;
    subbitbuf = 0;
    bitcount = 0;
    fillbuf (BITBUFSIZ);
}

/*
 * maketbl.c
 */

static int make_table (int nchar, uchar far *bitlen,
                       int tablebits, ushort far *table)
{
    ushort count[17], weight[17], start[18], *p;
    uint i, k, len, ch, jutbits, avail, nextcode, mask;

    for (i = 1; i <= 16; i++)
        count[i] = 0;
    for (i = 0; i < nchar; i++)
        count[bitlen[i]]++;

    start[1] = 0;
    for (i = 1; i <= 16; i++)
        start[i + 1] = start[i] + (count[i] << (16 - i));
    if (start[17] != (ushort) (1U << 16))
        return (1); /* error: bad table */

    jutbits = 16 - tablebits;
    for (i = 1; i <= tablebits; i++)
    {
        start[i] >>= jutbits;
        weight[i] = 1U << (tablebits - i);
    }
    while (i <= 16)
        weight[i++] = 1U << (16 - i);

    i = start[tablebits + 1] >> jutbits;
    if (i != (ushort) (1U << 16))
    {
        k = 1U << tablebits;
        while (i != k)
            table[i++] = 0;
    }

    avail = nchar;
    mask = 1U << (15 - tablebits);
    for (ch = 0; ch < nchar; ch++)
    {
        if ((len = bitlen[ch]) == 0)
            continue;
        nextcode = start[len] + weight[len];
        if (len <= tablebits)
        {
            for (i = start[len]; i < nextcode; i++)
                table[i] = ch;
        }
        else
        {
            k = start[len];
            p = &table[k >> jutbits];
            i = len - tablebits;
            while (i != 0)
            {
                if (*p == 0)
                {
                    right[avail] = left[avail] = 0;
                    *p = avail++;
                }
                if (k & mask)
                    p = &right[*p];
                else
                    p = &left[*p];
                k <<= 1;
                i--;
            }
            *p = ch;
        }
        start[len] = nextcode;
    }
    return (0);
}

/*
 * huf.c
 */

#define NP (DICBIT + 1)
#define NT (CODE_BIT + 3)
#define PBIT 4      /* smallest integer such that (1U << PBIT) > NP */
#define TBIT 5      /* smallest integer such that (1U << TBIT) > NT */
#if NT > NP
    #define NPT NT
#else
    #define NPT NP
#endif

static uchar  far c_len[NC], far pt_len[NPT];
static uint   blocksize;
static ushort far c_table[4096], far pt_table[256];

static void read_pt_len (int nn, int nbit, int i_special)
{
    int i, n;
    short c;
    ushort mask;

    n = getbits (nbit);
    if (n == 0)
    {
        c = getbits (nbit);
        for (i = 0; i < nn; i++)
            pt_len[i] = 0;
        for (i = 0; i < 256; i++)
            pt_table[i] = c;
    }
    else
    {
        i = 0;
        while (i < n)
        {
            c = bitbuf >> (BITBUFSIZ - 3);
            if (c == 7)
            {
                mask = 1U << (BITBUFSIZ - 1 - 3);
                while (mask & bitbuf)
                {
                    mask >>= 1;
                    c++;
                }
            }
            fillbuf ((c < 7) ? 3 : c - 3);
            pt_len[i++] = c;
            if (i == i_special)
            {
                c = getbits (2);
                while (--c >= 0)
                    pt_len[i++] = 0;
            }
        }
        while (i < nn)
            pt_len[i++] = 0;
        make_table (nn, pt_len, 8, pt_table);
    }
}

static void read_c_len (void)
{
    short i, c, n;
    ushort mask;

    n = getbits (CBIT);
    if (n == 0)
    {
        c = getbits (CBIT);
        for (i = 0; i < NC; i++)
            c_len[i] = 0;
        for (i = 0; i < 4096; i++)
            c_table[i] = c;
    }
    else
    {
        i = 0;
        while (i < n)
        {
            c = pt_table[bitbuf >> (BITBUFSIZ - 8)];
            if (c >= NT)
            {
                mask = 1U << (BITBUFSIZ - 1 - 8);
                do
                {
                    if (bitbuf & mask)
			c = right[c];
                    else
			c = left[c];
                    mask >>= 1;
                } while (c >= NT);
            }
            fillbuf (pt_len[c]);
            if (c <= 2)
            {
                if (c == 0)
                    c = 1;
                else if (c == 1)
                    c = getbits (4) + 3;
                else
                    c = getbits (CBIT) + 20;
                while (--c >= 0)
                    c_len[i++] = 0;
            }
            else
                c_len[i++] = c - 2;
        }
        while (i < NC)
            c_len[i++] = 0;
        make_table (NC, c_len, 12, c_table);
    }
}

ushort decode_c (void)
{
    ushort j, mask;

    if (blocksize == 0)
    {
        blocksize = getbits (16);
        read_pt_len (NT, TBIT, 3);
        read_c_len ();
        read_pt_len (NP, PBIT, -1);
    }
    blocksize--;
    j = c_table[bitbuf >> (BITBUFSIZ - 12)];
    if (j >= NC)
    {
        mask = 1U << (BITBUFSIZ - 1 - 12);
        do
        {
            if (bitbuf & mask)
				j = right[j];
            else
				j = left[j];
            mask >>= 1;
        }
		while (j >= NC);
    }
    fillbuf (c_len[j]);
    return j;
}

ushort decode_p (void)
{
    ushort j, mask;

    j = pt_table[bitbuf >> (BITBUFSIZ - 8)];
    if (j >= NP)
    {
        mask = 1U << (BITBUFSIZ - 1 - 8);
        do
        {
            if (bitbuf & mask)
		j = right[j];
            else
		j = left[j];
            mask >>= 1;
        } while (j >= NP);
    }
    fillbuf (pt_len[j]);
    if (j != 0)
        j = (1U << (j - 1)) + getbits (j - 1);
    return j;
}

void huf_decode_start (void)
{
    init_getbits ();
    blocksize = 0;
}

/*
 * decode.c
 */

static int decode_j;    /* remaining bytes to copy */

static void decode_start (void)
{
    fillbufsize = 0;
    huf_decode_start ();
    decode_j = 0;
}

/*
 * The calling function must keep the number of bytes to be processed.  This
 * function decodes either 'count' bytes or 'DICSIZ' bytes, whichever is
 * smaller, into the array 'buffer[]' of size 'DICSIZ' or more. Call
 * decode_start() once for each new file before calling this function.
 */
static void decode (uint count, uchar buffer[])
{
    static uint i;
    uint r, c;

    r = 0;
    while (--decode_j >= 0)
    {
        buffer[r] = buffer[i];
        i = (i + 1) & (DICSIZ - 1);
        if (++r == count)
            return;
    }
    for (;;)
    {
        c = decode_c ();
        if (c <= UCHAR_MAX)
        {
            buffer[r] = c;
            if (++r == count)
                return;
        }
        else
        {
            decode_j = c - (UCHAR_MAX + 1 - THRESHOLD);
            i = (r - decode_p () - 1) & (DICSIZ - 1);
            while (--decode_j >= 0)
            {
                buffer[r] = buffer[i];
                i = (i + 1) & (DICSIZ - 1);
                if (++r == count)
                    return;
            }
        }
    }
}

int lzh_melt (type_fnc_write  pfnc_read,
	      type_fnc_read   pfnc_write,
	      type_fnc_malloc pfnc_malloc,
	      type_fnc_free   pfnc_free,
	      ulong origsize)
{
    int n;
    uchar *outbuf;

    fnc_write   = pfnc_write;
    fnc_read	= pfnc_read;
    fnc_malloc	= pfnc_malloc;
    fnc_free	= pfnc_free;

    with_error = 0;

    if ((buf = (uchar*)fnc_malloc (BUFSIZE)) == 0L)
	return (1);
    if ((outbuf = (uchar*)fnc_malloc (DICSIZ)) == 0L)
    {
	fnc_free (buf);
	return (1);
    }

    decode_start ();
    while (origsize != 0)
    {
        n = (uint) ((origsize > DICSIZ) ? DICSIZ : origsize);
        decode (n, outbuf);
		if (with_error)
			break;
	
		fnc_write (outbuf, n);
        origsize -= n;
		if (with_error)
			break;
    }

	fnc_free(outbuf);
	fnc_free(buf);

    return (with_error ? 1 : 0);
}

/* end of LZHXLib.C */
