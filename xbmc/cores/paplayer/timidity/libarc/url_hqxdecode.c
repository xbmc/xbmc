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
#define INFOBYTES 128

typedef struct _URL_hqxdecode
{
    char common[sizeof(struct _URL)];
    URL reader;
    long rpos;
    int beg, end, eof, eod;
    unsigned char decodebuf[DECODEBUFSIZ];
    long datalen, rsrclen, restlen;
    int dsoff, rsoff, zoff;
    int stage, dataonly, autoclose;
} URL_hqxdecode;

static long url_hqxdecode_read(URL url, void *buff, long n);
static int  url_hqxdecode_fgetc(URL url);
static long url_hqxdecode_tell(URL url);
static void url_hqxdecode_close(URL url);

URL url_hqxdecode_open(URL reader, int dataonly, int autoclose)
{
    URL_hqxdecode *url;

    url = (URL_hqxdecode *)alloc_url(sizeof(URL_hqxdecode));
    if(url == NULL)
    {
	if(autoclose)
	    url_close(reader);
	url_errno = errno;
	return NULL;
    }

    /* common members */
    URLm(url, type)      = URL_hqxdecode_t;
    URLm(url, url_read)  = url_hqxdecode_read;
    URLm(url, url_gets)  = NULL;
    URLm(url, url_fgetc) = url_hqxdecode_fgetc;
    URLm(url, url_seek)  = NULL;
    URLm(url, url_tell)  = url_hqxdecode_tell;
    URLm(url, url_close) = url_hqxdecode_close;

    /* private members */
    url->reader = reader;
    url->rpos = 0;
    url->beg = 0;
    url->end = 0;
    url->eof = url->eod = 0;
    memset(url->decodebuf, 0, sizeof(url->decodebuf));
    url->datalen = -1;
    url->rsrclen = -1;
    url->restlen = 0;
    url->stage = 0;
    url->dataonly = dataonly;
    url->autoclose = autoclose;

    return (URL)url;
}

static int hqxgetchar(URL reader)
{
    int c;
    static int hqx_decode_table[256] =
    {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
	0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x00, 0x00,
	0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x00,
	0x14, 0x15, EOF,  0x00, 0x00, 0x00, 0x00, 0x00,
	0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
	0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x00,
	0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x00,
	0x2c, 0x2d, 0x2e, 0x2f, 0x00, 0x00, 0x00, 0x00,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x00,
	0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x00, 0x00,
	0x3d, 0x3e, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    do
    {
	if((c = url_getc(reader)) == EOF)
	    return EOF;
    } while(c == '\r' || c == '\n');
    return hqx_decode_table[c];
}

static int hqxdecode_chunk(URL url, unsigned char *p)
{
    int c1, c2, c3, c4;
    int n;

    n = 0;
    if((c1 = hqxgetchar(url)) == EOF)
	return 0;
    if((c2 = hqxgetchar(url)) == EOF)
	return 0;
    p[n++] = ((c1 << 2) | ((c2 & 0x30) >> 4));
    if((c3 = hqxgetchar(url)) == EOF)
	return n;
    p[n++] = (((c2 & 0xf) << 4) | ((c3 & 0x3c) >> 2));
    if((c4 = hqxgetchar(url)) == EOF)
	return n;
    p[n++] = (((c3 & 0x03) << 6) | c4);
    return n;
}

static uint32 convert_int32(unsigned char *p)
{
    return ((uint32)p[3])		|
	   (((uint32)p[2]) << 8)	|
	   (((uint32)p[1]) << 16)	|
	   (((uint32)p[0]) << 24);
}

static int hqxdecode_header(URL_hqxdecode *urlp)
{
    int i, n;
    unsigned char *p, *q;
    URL url;
    int hlen, nlen;

    n = 0;
    p = urlp->decodebuf;
    q = urlp->decodebuf + INFOBYTES;
    url = urlp->reader;
    while(n < DECODEBUFSIZ - INFOBYTES - 2)
    {
	i = hqxdecode_chunk(url, q + n);
	n += i;
	if(i != 3)
	{
	    urlp->eod = 1;
	    break;
	}
    }

    memset(p, 0, INFOBYTES);
    nlen = q[0];
    hlen = nlen + 22;

    if(n < hlen)
    {
	urlp->eof = 1;
	return -1; /* Error */
    }

    urlp->datalen = (long)convert_int32(q + hlen - 10);
    urlp->rsrclen = (long)convert_int32(q + hlen - 6);
    urlp->dsoff = (((urlp->datalen + 127) >> 7) << 7) - urlp->datalen;
    urlp->rsoff = (((urlp->rsrclen + 127) >> 7) << 7) - urlp->rsrclen;
    urlp->zoff = 0;

    p[1] = nlen;
    memcpy(p + 2, q + 1, nlen);
    memcpy(p + 65, q + hlen - 20, 4+4+2); /* type, author, flags */
    memcpy(p + 83, q + hlen - 10, 4+4);	/* datalen, rsrclen */
    /* 91: create time (4) */
    /* 95: modify time (4) */

    q += hlen;
    n -= hlen;
    for(i = 0; i < n; i++)
	p[INFOBYTES + i] = q[i];
    return INFOBYTES + n;
}

static int hqxdecode(URL_hqxdecode *urlp)
{
    int i, n;
    unsigned char *p;
    URL url;

    if(urlp->eod)
    {
	urlp->eof = 1;
	return 1;
    }

    if(urlp->stage == 0)
    {
	n = hqxdecode_header(urlp);
	if(n == -1)
	    return 1;
	urlp->end = n;

	if(urlp->dataonly)
	{
	    urlp->beg = INFOBYTES;
	    urlp->restlen = urlp->datalen;
	}
	else
	{
	    urlp->beg = 0;
	    urlp->restlen = urlp->datalen + INFOBYTES;
	}

	urlp->stage = 1;
	return 0;
    }

    p = urlp->decodebuf;
    url = urlp->reader;
    n = 0;

    if(urlp->restlen == 0)
    {
	if(urlp->dataonly)
	{
	    urlp->eof = 1;
	    return 1;
	}

	if(urlp->stage == 2)
	{
	    urlp->zoff = urlp->rsoff;
	    urlp->eof = 1;
	    return 1;
	}

	urlp->zoff = urlp->dsoff;
	urlp->stage = 2;

	n = urlp->end - urlp->beg;
	if(n <= 2)
	{
	    for(i = 0; i < n; i++)
		p[i] = p[i + urlp->beg];
	    n += hqxdecode_chunk(url, p + n);
	    if(n <= 2)
	    {
		urlp->eof = 1;
		return 1;
	    }
	    urlp->rpos += urlp->beg;
	    urlp->beg = 0;
	    urlp->end = n;
	}
	urlp->restlen = urlp->rsrclen;

	/* skip 2 byte (crc) */
	urlp->beg += 2;
	urlp->rpos -= 2;

	n = urlp->beg;
    }

    while(n < DECODEBUFSIZ)
    {
	i = hqxdecode_chunk(url, p + n);
	n += i;
	if(i != 3)
	{
	    urlp->eod = 1;
	    break;
	}
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

static long url_hqxdecode_read(URL url, void *buff, long size)
{
    URL_hqxdecode *urlp = (URL_hqxdecode *)url;
    char *p = (char *)buff;
    long n;
    int i;

    n = 0;
    while(n < size)
    {
	if(urlp->zoff > 0)
	{
	    i = urlp->zoff;
	    if(i > size - n)
		i = size - n;
	    memset(p + n, 0, i);
	    urlp->zoff -= i;
	    urlp->rpos += i;
	    n += i;
	    continue;
	}

	if(urlp->eof)
	    break;

	if(urlp->restlen == 0 || urlp->beg == urlp->end)
	{
	    hqxdecode(urlp);
	    continue;
	}

	i = urlp->end - urlp->beg;
	if(i > urlp->restlen)
	    i = urlp->restlen;
	if(i > size - n)
	    i = size - n;
	memcpy(p + n, urlp->decodebuf + urlp->beg, i);
	urlp->beg += i;
	n += i;
	urlp->restlen -= i;
    }

    return n;
}

static int url_hqxdecode_fgetc(URL url)
{
    URL_hqxdecode *urlp = (URL_hqxdecode *)url;
    int c;

  retry_read:
    if(urlp->zoff > 0)
    {
	urlp->zoff--;
	urlp->rpos++;
	return 0;
    }

    if(urlp->eof)
	return EOF;

    if(urlp->restlen == 0 || urlp->beg == urlp->end)
    {
	hqxdecode(urlp);
	goto retry_read;
    }

    c = (int)urlp->decodebuf[urlp->beg++];
    urlp->restlen--;

    return c;
}

static long url_hqxdecode_tell(URL url)
{
    URL_hqxdecode *urlp = (URL_hqxdecode *)url;

    if(urlp->dataonly)
	return urlp->rpos + urlp->beg - INFOBYTES;
    return urlp->rpos + urlp->beg;
}

static void url_hqxdecode_close(URL url)
{
    URL_hqxdecode *urlp = (URL_hqxdecode *)url;

    if(urlp->autoclose)
	url_close(urlp->reader);
    free(url);
}

#ifdef HQXDECODE_MAIN
void main(int argc, char** argv)
{
    URL hqxdecoder;
    char buff[256], *filename;
    int c;

    if(argc != 2)
    {
	fprintf(stderr, "Usage: %s hqx-filename\n", argv[0]);
	exit(1);
    }
    filename = argv[1];

    if((hqxdecoder = url_file_open(filename)) == NULL)
    {
	perror(argv[1]);
	exit(1);
    }

    for(;;)
    {
	if(url_readline(hqxdecoder, buff, sizeof(buff)) == NULL)
	{
	    fprintf(stderr, "%s: Not a hqx-file\n", filename);
	    url_close(hqxdecoder);
	    exit(1);
	}
	if((strncmp(buff, "(This file", 10) == 0) ||
	   (strncmp(buff, "(Convert with", 13) == 0))
	    break;
    }

    while((c = url_getc(hqxdecoder)) != EOF)
	if(c == ':')
	    break;
    if(c == EOF)
    {
	fprintf(stderr, "%s: Not a hqx-file\n", filename);
	url_close(hqxdecoder);
	exit(1);
    }

    hqxdecoder = url_hqxdecode_open(hqxdecoder, 0, 1);
#if HQXDECODE_MAIN
    while((c = url_getc(hqxdecoder)) != EOF)
	putchar(c);
#else
    while((c = url_read(hqxdecoder, buff, sizeof(buff))) > 0)
	write(1, buff, c);
#endif
    url_close(hqxdecoder);
    exit(0);
}

#endif /* HQXDECODE_MAIN */
