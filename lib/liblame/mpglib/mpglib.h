/*
 * Copyright (C) 1999-2010 The L.A.M.E. project
 *
 * Initially written by Michael Hipp, see also AUTHORS and README.
 *  
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef _MPGLIB_H_
#define _MPGLIB_H_

#ifndef plotting_data_defined
#define plotting_data_defined
struct plotting_data;
typedef struct plotting_data plotting_data;
#endif




struct buf {
    unsigned char *pnt;
    long    size;
    long    pos;
    struct buf *next;
    struct buf *prev;
};

struct framebuf {
    struct buf *buf;
    long    pos;
    struct frame *next;
    struct frame *prev;
};

typedef struct mpstr_tag {
    struct buf *head, *tail; /* buffer linked list pointers, tail points to oldest buffer */
    int     vbr_header;      /* 1 if valid Xing vbr header detected */
    int     num_frames;      /* set if vbr header present */
    int     enc_delay;       /* set if vbr header present */
    int     enc_padding;     /* set if vbr header present */
    /* header_parsed, side_parsed and data_parsed must be all set 1
       before the full frame has been parsed */
    int     header_parsed;   /* 1 = header of current frame has been parsed */
    int     side_parsed;     /* 1 = header of sideinfo of current frame has been parsed */
    int     data_parsed;
    int     free_format;     /* 1 = free format frame */
    int     old_free_format; /* 1 = last frame was free format */
    int     bsize;
    int     framesize;
    int     ssize;           /* number of bytes used for side information, including 2 bytes for CRC-16 if present */
    int     dsize;
    int     fsizeold;        /* size of previous frame, -1 for first */
    int     fsizeold_nopadding;
    struct frame fr;         /* holds the parameters decoded from the header */
    unsigned char bsspace[2][MAXFRAMESIZE + 1024]; /* bit stream space used ???? */ /* MAXFRAMESIZE */
    real    hybrid_block[2][2][SBLIMIT * SSLIMIT];
    int     hybrid_blc[2];
    unsigned long header;
    int     bsnum;
    real    synth_buffs[2][2][0x110];
    int     synth_bo;
    int     sync_bitstream;  /* 1 = bitstream is yet to be synchronized */

    int     bitindex;
    unsigned char *wordpointer;
    plotting_data *pinfo;
} MPSTR, *PMPSTR;


#define MP3_ERR -1
#define MP3_OK  0
#define MP3_NEED_MORE 1



#endif /* _MPGLIB_H_ */
