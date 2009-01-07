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
#include "url.h"

#define DECODEBUFSIZ 255 /* Must be power of 3 */

typedef struct _URL_b64decode
{
    char common[sizeof(struct _URL)];
    URL reader;
    long rpos;
    int beg, end, eof, eod;
    unsigned char decodebuf[DECODEBUFSIZ];
    int autoclose;
} URL_b64decode;

static long url_b64decode_read(URL url, void *buff, long n);
static int  url_b64decode_fgetc(URL url);
static long url_b64decode_tell(URL url);
static void url_b64decode_close(URL url);

URL url_b64decode_open(URL reader, int autoclose)
{
    URL_b64decode *url;

    url = (URL_b64decode *)alloc_url(sizeof(URL_b64decode));
    if(url == NULL)
    {
	if(autoclose)
	    url_close(reader);
	url_errno = errno;
	return NULL;
    }

    /* common members */
    URLm(url, type)      = URL_b64decode_t;
    URLm(url, url_read)  = url_b64decode_read;
    URLm(url, url_gets)  = NULL;
    URLm(url, url_fgetc) = url_b64decode_fgetc;
    URLm(url, url_seek)  = NULL;
    URLm(url, url_tell)  = url_b64decode_tell;
    URLm(url, url_close) = url_b64decode_close;

    /* private members */
    url->reader = reader;
    url->rpos = 0;
    url->beg = 0;
    url->end = 0;
    url->eof = url->eod = 0;
    memset(url->decodebuf, 0, sizeof(url->decodebuf));
    url->autoclose = autoclose;

    return (URL)url;
}

static int b64getchar(URL reader)
{
    int c;
    static int b64_decode_table[256] =
    {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 62, 0, 0,
	0, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0, 0, 0, EOF, 0, 0, 0,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
	20, 21, 22, 23, 24, 25, 0, 0, 0, 0, 0, 0, 26, 27, 28, 29, 30, 31, 32,
	33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
	50, 51, 0, 0, 0, 0, 0
    };

    do
    {
	if((c = url_getc(reader)) == EOF)
	    return EOF;
    } while(c == '\r' || c == '\n');
    return b64_decode_table[c];
}

static int b64decode(URL_b64decode *urlp)
{
    int c1, c2, c3, c4;
    int n;
    unsigned char *p;
    URL url;

    if(urlp->eod)
    {
	urlp->eof = 1;
	return 1;
    }

    p = urlp->decodebuf;
    url = urlp->reader;
    n = 0;
    while(n < DECODEBUFSIZ)
    {
	if((c1 = b64getchar(url)) == EOF)
	{
	    urlp->eod = 1;
	    break;
	}
	if((c2 = b64getchar(url)) == EOF)
	{
	    urlp->eod = 1;
	    break;
	}
	p[n++] = ((c1 << 2) | ((c2 & 0x30) >> 4));

	if((c3 = b64getchar(url)) == EOF)
	{
	    urlp->eod = 1;
	    break;
	}
	p[n++] = (((c2 & 0xf) << 4) | ((c3 & 0x3c) >> 2));

	if((c4 = b64getchar(url)) == EOF)
	{
	    urlp->eod = 1;
	    break;
	}
        p[n++] = (((c3 & 0x03) << 6) | c4);
    }

    urlp->rpos += urlp->beg;
    urlp->beg = 0;
    urlp->end = n;

    if(n == 0)
    {
	urlp->eof = 1;
	return 1;
    }

    return 0;
}

static long url_b64decode_read(URL url, void *buff, long size)
{
    URL_b64decode *urlp = (URL_b64decode *)url;
    unsigned char *p = (unsigned char *)buff;
    long n;

    if(urlp->eof)
	return 0;

    n = 0;
    while(n < size)
    {
	int i;

	if(urlp->beg == urlp->end)
	    if(b64decode(urlp))
		break;
	i = urlp->end - urlp->beg;
	if(i > size - n)
	    i = size - n;
	memcpy(p + n, urlp->decodebuf + urlp->beg, i);
	n += i;
	urlp->beg += i;
    }
    return n;
}

static int url_b64decode_fgetc(URL url)
{
    URL_b64decode *urlp = (URL_b64decode *)url;

    if(urlp->eof)
	return EOF;
    if(urlp->beg == urlp->end)
	if(b64decode(urlp))
	    return EOF;

    return (int)urlp->decodebuf[urlp->beg++];
}

static long url_b64decode_tell(URL url)
{
    URL_b64decode *urlp = (URL_b64decode *)url;

    return urlp->rpos + urlp->beg;
}

static void url_b64decode_close(URL url)
{
    URL_b64decode *urlp = (URL_b64decode *)url;

    if(urlp->autoclose)
	url_close(urlp->reader);
    free(url);
}

#ifdef B64DECODE_MAIN
void main(int argc, char** argv)
{
    URL b64decoder;
    char buff[256], *filename;
    int c;

    if(argc != 2)
    {
	fprintf(stderr, "Usage: %s b64-filename\n", argv[0]);
	exit(1);
    }
    filename = argv[1];

    if((b64decoder = url_file_open(filename)) == NULL)
    {
	perror(argv[1]);
	url_close(b64decoder);
	exit(1);
    }

    b64decoder = url_b64decode_open(b64decoder, 1);
#if B64DECODE_MAIN
    while((c = url_getc(b64decoder)) != EOF)
	putchar(c);
#else
    while((c = url_read(b64decoder, buff, sizeof(buff))) > 0)
	write(1, buff, c);
#endif
    url_close(b64decoder);
    exit(0);
}

#endif /* B64DECODE_MAIN */
