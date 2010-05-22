/*
 * interface.c
 *
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/* $Id: interface.c,v 1.52.2.4 2010/03/22 14:17:14 robert Exp $ */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "interface.h"
#include "tabinit.h"
#include "layer3.h"
#include "lame.h"
#include "machine.h"
#include "VbrTag.h"
#include "decode_i386.h"

#include "layer1.h"
#include "layer2.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

/* #define HIP_DEBUG */

int
InitMP3(PMPSTR mp)
{
    memset(mp, 0, sizeof(MPSTR));

    mp->framesize = 0;
    mp->num_frames = 0;
    mp->enc_delay = -1;
    mp->enc_padding = -1;
    mp->vbr_header = 0;
    mp->header_parsed = 0;
    mp->side_parsed = 0;
    mp->data_parsed = 0;
    mp->free_format = 0;
    mp->old_free_format = 0;
    mp->ssize = 0;
    mp->dsize = 0;
    mp->fsizeold = -1;
    mp->bsize = 0;
    mp->head = mp->tail = NULL;
    mp->fr.single = -1;
    mp->bsnum = 0;
    mp->wordpointer = mp->bsspace[mp->bsnum] + 512;
    mp->bitindex = 0;
    mp->synth_bo = 1;
    mp->sync_bitstream = 1;

    make_decode_tables(32767);

    init_layer3(SBLIMIT);

    init_layer2();

    return 1;
}

void
ExitMP3(PMPSTR mp)
{
    struct buf *b, *bn;

    b = mp->tail;
    while (b) {
        free(b->pnt);
        bn = b->next;
        free(b);
        b = bn;
    }
}

static struct buf *
addbuf(PMPSTR mp, unsigned char *buf, int size)
{
    struct buf *nbuf;

    nbuf = (struct buf *) malloc(sizeof(struct buf));
    if (!nbuf) {
        fprintf(stderr, "hip: addbuf() Out of memory!\n");
        return NULL;
    }
    nbuf->pnt = (unsigned char *) malloc((size_t) size);
    if (!nbuf->pnt) {
        free(nbuf);
        return NULL;
    }
    nbuf->size = size;
    memcpy(nbuf->pnt, buf, (size_t) size);
    nbuf->next = NULL;
    nbuf->prev = mp->head;
    nbuf->pos = 0;

    if (!mp->tail) {
        mp->tail = nbuf;
    }
    else {
        mp->head->next = nbuf;
    }

    mp->head = nbuf;
    mp->bsize += size;

    return nbuf;
}

void
remove_buf(PMPSTR mp)
{
    struct buf *buf = mp->tail;

    mp->tail = buf->next;
    if (mp->tail)
        mp->tail->prev = NULL;
    else {
        mp->tail = mp->head = NULL;
    }

    free(buf->pnt);
    free(buf);

}

static int
read_buf_byte(PMPSTR mp)
{
    unsigned int b;

    int     pos;


    pos = mp->tail->pos;
    while (pos >= mp->tail->size) {
        remove_buf(mp);
        if (!mp->tail) {
            fprintf(stderr, "hip: Fatal error! tried to read past mp buffer\n");
            exit(1);
        }
        pos = mp->tail->pos;
    }

    b = mp->tail->pnt[pos];
    mp->bsize--;
    mp->tail->pos++;


    return b;
}



static void
read_head(PMPSTR mp)
{
    unsigned long head;

    head = read_buf_byte(mp);
    head <<= 8;
    head |= read_buf_byte(mp);
    head <<= 8;
    head |= read_buf_byte(mp);
    head <<= 8;
    head |= read_buf_byte(mp);

    mp->header = head;
}




static void
copy_mp(PMPSTR mp, int size, unsigned char *ptr)
{
    int     len = 0;

    while (len < size && mp->tail) {
        int     nlen;
        int     blen = mp->tail->size - mp->tail->pos;
        if ((size - len) <= blen) {
            nlen = size - len;
        }
        else {
            nlen = blen;
        }
        memcpy(ptr + len, mp->tail->pnt + mp->tail->pos, (size_t) nlen);
        len += nlen;
        mp->tail->pos += nlen;
        mp->bsize -= nlen;
        if (mp->tail->pos == mp->tail->size) {
            remove_buf(mp);
        }
    }
}

/* number of bytes needed by GetVbrTag to parse header */
#define XING_HEADER_SIZE 194

/*
traverse mp data structure without changing it
(just like sync_buffer)
pull out Xing bytes
call vbr header check code from LAME
if we find a header, parse it and also compute the VBR header size
if no header, do nothing.

bytes = number of bytes before MPEG header.  skip this many bytes
before starting to read
return value: number of bytes in VBR header, including syncword
*/
static int
check_vbr_header(PMPSTR mp, int bytes)
{
    int     i, pos;
    struct buf *buf = mp->tail;
    unsigned char xing[XING_HEADER_SIZE];
    VBRTAGDATA pTagData;

    pos = buf->pos;
    /* skip to valid header */
    for (i = 0; i < bytes; ++i) {
        while (pos >= buf->size) {
            buf = buf->next;
            if (!buf)
                return -1; /* fatal error */
            pos = buf->pos;
        }
        ++pos;
    }
    /* now read header */
    for (i = 0; i < XING_HEADER_SIZE; ++i) {
        while (pos >= buf->size) {
            buf = buf->next;
            if (!buf)
                return -1; /* fatal error */
            pos = buf->pos;
        }
        xing[i] = buf->pnt[pos];
        ++pos;
    }

    /* check first bytes for Xing header */
    mp->vbr_header = GetVbrTag(&pTagData, xing);
    if (mp->vbr_header) {
        mp->num_frames = pTagData.frames;
        mp->enc_delay = pTagData.enc_delay;
        mp->enc_padding = pTagData.enc_padding;

        /* fprintf(stderr,"hip: delays: %i %i \n",mp->enc_delay,mp->enc_padding); */
        /* fprintf(stderr,"hip: Xing VBR header dectected.  MP3 file has %i frames\n", pTagData.frames); */
        if (pTagData.headersize < 1)
            return 1;
        return pTagData.headersize;
    }
    return 0;
}





static int
sync_buffer(PMPSTR mp, int free_match)
{
    /* traverse mp structure without modifying pointers, looking
     * for a frame valid header.
     * if free_format, valid header must also have the same
     * samplerate.   
     * return number of bytes in mp, before the header
     * return -1 if header is not found
     */
    unsigned int b[4] = { 0, 0, 0, 0 };
    int     i, h, pos;
    struct buf *buf = mp->tail;
    if (!buf)
        return -1;

    pos = buf->pos;
    for (i = 0; i < mp->bsize; i++) {
        /* get 4 bytes */

        b[0] = b[1];
        b[1] = b[2];
        b[2] = b[3];
        while (pos >= buf->size) {
            buf = buf->next;
            pos = buf->pos;
            if (!buf) {
                return -1;
                /* not enough data to read 4 bytes */
            }
        }
        b[3] = buf->pnt[pos];
        ++pos;

        if (i >= 3) {
            struct frame *fr = &mp->fr;
            unsigned long head;

            head = b[0];
            head <<= 8;
            head |= b[1];
            head <<= 8;
            head |= b[2];
            head <<= 8;
            head |= b[3];
            h = head_check(head, fr->lay);

            if (h && free_match) {
                /* just to be even more thorough, match the sample rate */
                int     mode, stereo, sampling_frequency, mpeg25, lsf;

                if (head & (1 << 20)) {
                    lsf = (head & (1 << 19)) ? 0x0 : 0x1;
                    mpeg25 = 0;
                }
                else {
                    lsf = 1;
                    mpeg25 = 1;
                }

                mode = ((head >> 6) & 0x3);
                stereo = (mode == MPG_MD_MONO) ? 1 : 2;

                if (mpeg25)
                    sampling_frequency = 6 + ((head >> 10) & 0x3);
                else
                    sampling_frequency = ((head >> 10) & 0x3) + (lsf * 3);
                h = ((stereo == fr->stereo) && (lsf == fr->lsf) && (mpeg25 == fr->mpeg25) &&
                     (sampling_frequency == fr->sampling_frequency));
            }

            if (h) {
                return i - 3;
            }
        }
    }
    return -1;
}


void
decode_reset(PMPSTR mp)
{
#if 0
    remove_buf(mp);
    /* start looking for next frame */
    /* mp->fsizeold = mp->framesize; */
    mp->fsizeold = -1;
    mp->old_free_format = mp->free_format;
    mp->framesize = 0;
    mp->header_parsed = 0;
    mp->side_parsed = 0;
    mp->data_parsed = 0;
    mp->sync_bitstream = 1; /* TODO check if this is right */
#else
    InitMP3(mp);        /* Less error prone to just to reinitialise. */
#endif
}

int
audiodata_precedesframes(PMPSTR mp)
{
    if (mp->fr.lay == 3)
        return layer3_audiodata_precedesframes(mp);
    else
        return 0;       /* For Layer 1 & 2 the audio data starts at the frame that describes it, so no audio data precedes. */
}

static int
decodeMP3_clipchoice(PMPSTR mp, unsigned char *in, int isize, char *out, int *done,
                     int (*synth_1to1_mono_ptr) (PMPSTR, real *, unsigned char *, int *),
                     int (*synth_1to1_ptr) (PMPSTR, real *, int, unsigned char *, int *))
{
    int     i, iret, bits, bytes;

    if (in && isize && addbuf(mp, in, isize) == NULL)
        return MP3_ERR;

    /* First decode header */
    if (!mp->header_parsed) {

        if (mp->fsizeold == -1 || mp->sync_bitstream) {
            int     vbrbytes;
            mp->sync_bitstream = 0;

            /* This is the very first call.   sync with anything */
            /* bytes= number of bytes before header */
            bytes = sync_buffer(mp, 0);

            /* now look for Xing VBR header */
            if (mp->bsize >= bytes + XING_HEADER_SIZE) {
                /* vbrbytes = number of bytes in entire vbr header */
                vbrbytes = check_vbr_header(mp, bytes);
            }
            else {
                /* not enough data to look for Xing header */
#ifdef HIP_DEBUG
                fprintf(stderr, "hip: not enough data to look for Xing header\n");
#endif
                return MP3_NEED_MORE;
            }

            if (mp->vbr_header) {
                /* do we have enough data to parse entire Xing header? */
                if (bytes + vbrbytes > mp->bsize) {
                    /* fprintf(stderr,"hip: not enough data to parse entire Xing header\n"); */
                    return MP3_NEED_MORE;
                }

                /* read in Xing header.  Buffer data in case it
                 * is used by a non zero main_data_begin for the next
                 * frame, but otherwise dont decode Xing header */
#ifdef HIP_DEBUG
                fprintf(stderr, "hip: found xing header, skipping %i bytes\n", vbrbytes + bytes);
#endif
                for (i = 0; i < vbrbytes + bytes; ++i)
                    read_buf_byte(mp);
                /* now we need to find another syncword */
                /* just return and make user send in more data */

                return MP3_NEED_MORE;
            }
        }
        else {
            /* match channels, samplerate, etc, when syncing */
            bytes = sync_buffer(mp, 1);
        }

        /* buffer now synchronized */
        if (bytes < 0) {
            /* fprintf(stderr,"hip: need more bytes %d\n", bytes); */
            return MP3_NEED_MORE;
        }
        if (bytes > 0) {
            /* there were some extra bytes in front of header.
             * bitstream problem, but we are now resynced 
             * should try to buffer previous data in case new
             * frame has nonzero main_data_begin, but we need
             * to make sure we do not overflow buffer
             */
            int     size;
            fprintf(stderr, "hip: bitstream problem, resyncing skipping %d bytes...\n", bytes);
            mp->old_free_format = 0;
#if 1
            /* FIXME: correct ??? */
            mp->sync_bitstream = 1;
#endif
            /* skip some bytes, buffer the rest */
            size = (int) (mp->wordpointer - (mp->bsspace[mp->bsnum] + 512));

            if (size > MAXFRAMESIZE) {
                /* wordpointer buffer is trashed.  probably cant recover, but try anyway */
                fprintf(stderr, "hip: wordpointer trashed.  size=%i (%i)  bytes=%i \n",
                        size, MAXFRAMESIZE, bytes);
                size = 0;
                mp->wordpointer = mp->bsspace[mp->bsnum] + 512;
            }

            /* buffer contains 'size' data right now 
               we want to add 'bytes' worth of data, but do not 
               exceed MAXFRAMESIZE, so we through away 'i' bytes */
            i = (size + bytes) - MAXFRAMESIZE;
            for (; i > 0; --i) {
                --bytes;
                read_buf_byte(mp);
            }

            copy_mp(mp, bytes, mp->wordpointer);
            mp->fsizeold += bytes;
        }

        read_head(mp);
        decode_header(&mp->fr, mp->header);
        mp->header_parsed = 1;
        mp->framesize = mp->fr.framesize;
        mp->free_format = (mp->framesize == 0);

        if (mp->fr.lsf)
            mp->ssize = (mp->fr.stereo == 1) ? 9 : 17;
        else
            mp->ssize = (mp->fr.stereo == 1) ? 17 : 32;
        if (mp->fr.error_protection)
            mp->ssize += 2;

        mp->bsnum = 1 - mp->bsnum; /* toggle buffer */
        mp->wordpointer = mp->bsspace[mp->bsnum] + 512;
        mp->bitindex = 0;

        /* for very first header, never parse rest of data */
        if (mp->fsizeold == -1) {
#ifdef HIP_DEBUG
            fprintf(stderr, "hip: not parsing the rest of the data of the first header\n");
#endif
            return MP3_NEED_MORE;
        }
    }                   /* end of header parsing block */

    /* now decode side information */
    if (!mp->side_parsed) {

        /* Layer 3 only */
        if (mp->fr.lay == 3) {
            if (mp->bsize < mp->ssize)
                return MP3_NEED_MORE;

            copy_mp(mp, mp->ssize, mp->wordpointer);

            if (mp->fr.error_protection)
                getbits(mp, 16);
            bits = do_layer3_sideinfo(mp);
            /* bits = actual number of bits needed to parse this frame */
            /* can be negative, if all bits needed are in the reservoir */
            if (bits < 0)
                bits = 0;

            /* read just as many bytes as necessary before decoding */
            mp->dsize = (bits + 7) / 8;

#ifdef HIP_DEBUG
            fprintf(stderr,
                    "hip: %d bits needed to parse layer III frame, number of bytes to read before decoding dsize = %d\n",
                    bits, mp->dsize);
#endif

            /* this will force mpglib to read entire frame before decoding */
            /* mp->dsize= mp->framesize - mp->ssize; */

        }
        else {
            /* Layers 1 and 2 */

            /* check if there is enough input data */
            if (mp->fr.framesize > mp->bsize)
                return MP3_NEED_MORE;

            /* takes care that the right amount of data is copied into wordpointer */
            mp->dsize = mp->fr.framesize;
            mp->ssize = 0;
        }

        mp->side_parsed = 1;
    }

    /* now decode main data */
    iret = MP3_NEED_MORE;
    if (!mp->data_parsed) {
        if (mp->dsize > mp->bsize) {
            return MP3_NEED_MORE;
        }

        copy_mp(mp, mp->dsize, mp->wordpointer);

        *done = 0;

        /*do_layer3(&mp->fr,(unsigned char *) out,done); */
        switch (mp->fr.lay) {
        case 1:
            if (mp->fr.error_protection)
                getbits(mp, 16);

            do_layer1(mp, (unsigned char *) out, done);
            break;

        case 2:
            if (mp->fr.error_protection)
                getbits(mp, 16);

            do_layer2(mp, (unsigned char *) out, done);
            break;

        case 3:
            do_layer3(mp, (unsigned char *) out, done, synth_1to1_mono_ptr, synth_1to1_ptr);
            break;
        default:
            fprintf(stderr, "hip: invalid layer %d\n", mp->fr.lay);
        }

        mp->wordpointer = mp->bsspace[mp->bsnum] + 512 + mp->ssize + mp->dsize;

        mp->data_parsed = 1;
        iret = MP3_OK;
    }


    /* remaining bits are ancillary data, or reservoir for next frame 
     * If free format, scan stream looking for next frame to determine
     * mp->framesize */
    if (mp->free_format) {
        if (mp->old_free_format) {
            /* free format.  bitrate must not vary */
            mp->framesize = mp->fsizeold_nopadding + (mp->fr.padding);
        }
        else {
            bytes = sync_buffer(mp, 1);
            if (bytes < 0)
                return iret;
            mp->framesize = bytes + mp->ssize + mp->dsize;
            mp->fsizeold_nopadding = mp->framesize - mp->fr.padding;
            /*
               fprintf(stderr,"hip: freeformat bitstream:  estimated bitrate=%ikbs  \n",
               8*(4+mp->framesize)*freqs[mp->fr.sampling_frequency]/
               (1000*576*(2-mp->fr.lsf)));
             */
        }
    }

    /* buffer the ancillary data and reservoir for next frame */
    bytes = mp->framesize - (mp->ssize + mp->dsize);
    if (bytes > mp->bsize) {
        return iret;
    }

    if (bytes > 0) {
        int     size;
#if 0
        /* FIXME: while loop OK ??? */
        while (bytes > 512) {
            read_buf_byte(mp);
            bytes--;
            mp->framesize--;
        }
#endif
        copy_mp(mp, bytes, mp->wordpointer);
        mp->wordpointer += bytes;

        size = (int) (mp->wordpointer - (mp->bsspace[mp->bsnum] + 512));
        if (size > MAXFRAMESIZE) {
            fprintf(stderr, "hip: fatal error.  MAXFRAMESIZE not large enough.\n");
        }

    }

    /* the above frame is completely parsed.  start looking for next frame */
    mp->fsizeold = mp->framesize;
    mp->old_free_format = mp->free_format;
    mp->framesize = 0;
    mp->header_parsed = 0;
    mp->side_parsed = 0;
    mp->data_parsed = 0;

    return iret;
}

int
decodeMP3(PMPSTR mp, unsigned char *in, int isize, char *out, int osize, int *done)
{
    if (osize < 4608) {
        fprintf(stderr, "hip: Insufficient memory for decoding buffer %d\n", osize);
        return MP3_ERR;
    }

    /* passing pointers to the functions which clip the samples */
    return decodeMP3_clipchoice(mp, in, isize, out, done, synth_1to1_mono, synth_1to1);
}

int
decodeMP3_unclipped(PMPSTR mp, unsigned char *in, int isize, char *out, int osize, int *done)
{
    /* we forbid input with more than 1152 samples per channel for output in unclipped mode */
    if (osize < (int) (1152 * 2 * sizeof(real))) {
        fprintf(stderr, "hip: out space too small for unclipped mode\n");
        return MP3_ERR;
    }

    /* passing pointers to the functions which don't clip the samples */
    return decodeMP3_clipchoice(mp, in, isize, out, done, synth_1to1_mono_unclipped,
                                synth_1to1_unclipped);
}
