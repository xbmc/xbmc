/*
 * libacm - Interplay ACM audio decoder.
 *
 * Copyright (c) 2004-2008, Marko Kreen
 * Copyright (c) 2008, Adam Gashlin
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef __LIBACM_H
#define __LIBACM_H

#include "../streamfile.h"

#define LIBACM_VERSION "1.0-svn"

#define ACM_ID		0x032897
#define ACM_WORD	2

#define ACM_HEADER_LEN  14
#define ACM_OK			 0
#define ACM_ERR_OTHER		-1
#define ACM_ERR_OPEN		-2
#define ACM_ERR_NOT_ACM		-3
#define ACM_ERR_READ_ERR	-4
#define ACM_ERR_BADFMT		-5
#define ACM_ERR_CORRUPT		-6
#define ACM_ERR_UNEXPECTED_EOF	-7
#define ACM_ERR_NOT_SEEKABLE	-8

typedef struct ACMInfo {
	unsigned channels;
	unsigned rate;
	unsigned acm_id;
	unsigned acm_version;
	unsigned acm_level;
	unsigned acm_cols;		/* 1 << acm_level */
	unsigned acm_rows;
} ACMInfo;

struct ACMStream {
	ACMInfo info;
	unsigned total_values;

	/* acm data stream */
    STREAMFILE *streamfile;
	unsigned data_len;

	/* acm stream buffer */
	unsigned bit_avail;
	unsigned bit_data;
	unsigned buf_start_ofs;

	/* block lengths (in samples) */
	unsigned block_len;
	unsigned wrapbuf_len;
	/* buffers */
	int *block;
	int *wrapbuf;
	int *ampbuf;
	int *midbuf;			/* pointer into ampbuf */
	/* result */
	unsigned block_ready:1;
	unsigned file_eof:1;
	unsigned stream_pos;			/* in words. absolute */
	unsigned block_pos;			/* in words, relative */
};
typedef struct ACMStream ACMStream;

/* decode.c */
int acm_open_decoder(ACMStream **res, STREAMFILE *facilitator_file, const char *const filename);
int acm_read(ACMStream *acm, void *buf, unsigned nbytes,
		int bigendianp, int wordlen, int sgned);
void acm_close(ACMStream *acm);
void acm_reset(ACMStream *acm);

#endif
