/*****************************************************************************
 * bits.h : Bit handling helpers
 *****************************************************************************
 * Copyright (C) 2003 the VideoLAN team
 * $Id: 0f9cbfe93686319fc2285767b8c4019555451f4c $
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef BD_BITS_H
#define BD_BITS_H

//#include <stdio.h>
#include <fcntl.h>
#include "../file/file.h"

/**
 * \file
 * This file defines functions, structures for handling streams of bits
 */

#define BF_BUF_SIZE   (1024*32)

typedef struct {
    uint8_t *p_start;
    uint8_t *p;
    uint8_t *p_end;

    ssize_t  i_left;    /* i_count number of available bits */
} BITBUFFER;

typedef struct {
    FILE_H    *fp;
    uint8_t    buf[BF_BUF_SIZE];
    BITBUFFER  bb;
    off_t      pos;
    off_t      end;
    int        size;
} BITSTREAM;

static inline void bb_init( BITBUFFER *bb, const void *p_data, size_t i_data )
{
    bb->p_start = (void *)p_data;
    bb->p       = bb->p_start;
    bb->p_end   = bb->p_start + i_data;
    bb->i_left  = 8;
}

static inline void bs_init( BITSTREAM *bs, FILE_H *fp )
{
    bs->fp = fp;
    bs->pos = 0;
    file_seek(bs->fp, 0, SEEK_END);
    bs->end = file_tell(bs->fp);
    bs->size = file_read(bs->fp, bs->buf, BF_BUF_SIZE);
    bb_init(&bs->bb, bs->buf, bs->size);
}

static inline int bb_pos( const BITBUFFER *bb )
{
    return 8 * ( bb->p - bb->p_start ) + 8 - bb->i_left;
}

static inline int bs_pos( const BITSTREAM *bs )
{
    return bs->pos * 8 + bb_pos(&bs->bb);
}

static inline int bb_eof( const BITBUFFER *bb )
{
    return bb->p >= bb->p_end ? 1: 0 ;
}

static inline int bs_eof( const BITSTREAM *bs )
{
    return file_eof(bs->fp) && bb_eof(&bs->bb);
}

static inline void bb_seek( BITBUFFER *bb, off_t off, int whence)
{
    off_t b;

    switch (whence) {
        case SEEK_CUR:
            off = (bb->p - bb->p_start) + off;
            break;
        case SEEK_END:
            off = (bb->p_end - bb->p_start) - off;
            break;
        case SEEK_SET:
        default:
            break;
    }
    b = off >> 3;
    bb->p = &bb->p_start[b];
    bb->i_left = 8 - (off & 0x07);
}

static inline void bs_seek( BITSTREAM *bs, off_t off, int whence)
{
    off_t b;

    switch (whence) {
        case SEEK_CUR:
            off = bs->pos * 8 + (bs->bb.p - bs->bb.p_start) + off;
            break;
        case SEEK_END:
            off = bs->end * 8 - off;
            break;
        case SEEK_SET:
        default:
            break;
    }
    b = off >> 3;
    if (b >= bs->end)
    {
        if (BF_BUF_SIZE < bs->end) {
            bs->pos = bs->end - BF_BUF_SIZE;
            file_seek(bs->fp, BF_BUF_SIZE, SEEK_END);
        } else {
            bs->pos = 0;
            file_seek(bs->fp, 0, SEEK_SET);
        }
    	bs->size = file_read(bs->fp, bs->buf, BF_BUF_SIZE);
        bb_init(&bs->bb, bs->buf, bs->size);
        bs->bb.p = bs->bb.p_end;
    } else if (b < bs->pos || b >= (bs->pos + BF_BUF_SIZE)) {
        file_seek(bs->fp, b, SEEK_SET);
        bs->pos = b;
    	bs->size = file_read(bs->fp, bs->buf, BF_BUF_SIZE);
        bb_init(&bs->bb, bs->buf, bs->size);
    } else {
        b -= bs->pos;
        bs->bb.p = &bs->bb.p_start[b];
        bs->bb.i_left = 8 - (off & 0x07);
    }
}

static inline void bb_seek_byte( BITBUFFER *bb, off_t off)
{
    bb_seek(bb, off << 3, SEEK_SET);
}

static inline void bs_seek_byte( BITSTREAM *s, off_t off)
{
    bs_seek(s, off << 3, SEEK_SET);
}

static inline uint32_t bb_read( BITBUFFER *bb, int i_count )
{
    static const uint32_t i_mask[33] = {  
        0x00,
        0x01,      0x03,      0x07,      0x0f,
        0x1f,      0x3f,      0x7f,      0xff,
        0x1ff,     0x3ff,     0x7ff,     0xfff,
        0x1fff,    0x3fff,    0x7fff,    0xffff,
        0x1ffff,   0x3ffff,   0x7ffff,   0xfffff,
        0x1fffff,  0x3fffff,  0x7fffff,  0xffffff,
        0x1ffffff, 0x3ffffff, 0x7ffffff, 0xfffffff,
        0x1fffffff,0x3fffffff,0x7fffffff,0xffffffff
    };
    int      i_shr;
    uint32_t i_result = 0;

    while( i_count > 0 ) {

        if( bb->p >= bb->p_end ) {
            break;
        }

        i_shr = bb->i_left - i_count;
        if( i_shr >= 0 ) {
            /* more in the buffer than requested */
            i_result |= ( *bb->p >> i_shr )&i_mask[i_count];
            bb->i_left -= i_count;
            if( bb->i_left == 0 ) {
                bb->p++;
                bb->i_left = 8;
            }
            return( i_result );
        } else {
            /* less in the buffer than requested */
           i_result |= (*bb->p&i_mask[bb->i_left]) << -i_shr;
           i_count  -= bb->i_left;
           bb->p++;
           bb->i_left = 8;
        }
    }

    return( i_result );
}

static inline uint32_t bs_read( BITSTREAM *bs, int i_count )
{
    int left;
    int bytes = (i_count + 7) >> 3;

    if (bs->bb.p + bytes >= bs->bb.p_end) {
        bs->pos = bs->pos + (bs->bb.p - bs->bb.p_start);
        left = bs->bb.i_left;
        file_seek(bs->fp, bs->pos, SEEK_SET);
    	bs->size = file_read(bs->fp, bs->buf, BF_BUF_SIZE);
        bb_init(&bs->bb, bs->buf, bs->size);
        bs->bb.i_left = left;
    }
    return bb_read(&bs->bb, i_count);
}

static inline void bb_read_bytes( BITBUFFER *bb, uint8_t *buf, int i_count )
{
    int ii;

    for (ii = 0; ii < i_count; ii++) {
        buf[ii] = bb_read(bb, 8);
    }
}

static inline void bs_read_bytes( BITSTREAM *s, uint8_t *buf, int i_count )
{
    int ii;

    for (ii = 0; ii < i_count; ii++) {
        buf[ii] = bs_read(s, 8);
    }
}

static inline uint32_t bb_show( BITBUFFER *bb, int i_count )
{
    BITBUFFER     bb_tmp = *bb;
    return bb_read( &bb_tmp, i_count );
}

static inline void bb_skip( BITBUFFER *bb, ssize_t i_count )
{
    bb->i_left -= i_count;

    if( bb->i_left <= 0 ) {
        const int i_bytes = ( -bb->i_left + 8 ) / 8;

        bb->p += i_bytes;
        bb->i_left += 8 * i_bytes;
    }
}

static inline void bs_skip( BITSTREAM *bs, ssize_t i_count )
{
    int left;
    int bytes = i_count >> 3;

    if (bs->bb.p + bytes >= bs->bb.p_end) {
        bs->pos = bs->pos + (bs->bb.p - bs->bb.p_start);
        left = bs->bb.i_left;
        file_seek(bs->fp, bs->pos, SEEK_SET);
    	bs->size = file_read(bs->fp, bs->buf, BF_BUF_SIZE);
        bb_init(&bs->bb, bs->buf, bs->size);
        bs->bb.i_left = left;
    }
    bb_skip(&bs->bb, i_count);
}

static inline int bb_is_align( BITBUFFER *bb, uint32_t mask )
{
    off_t off = bb_pos(bb);

    return !(off & mask);
}

static inline int bs_is_align( BITSTREAM *s, uint32_t mask )
{
    off_t off = bs_pos(s);

    return !(off & mask);
}

#endif
