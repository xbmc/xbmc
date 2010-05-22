/*
 *      MP3 bitstream Output interface for LAME
 *
 *      Copyright (c) 1999-2000 Mark Taylor
 *      Copyright (c) 1999-2002 Takehiro Tominaga
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * $Id: bitstream.c,v 1.82.2.3 2009/01/18 15:44:27 robert Exp $
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include "lame.h"
#include "machine.h"
#include "encoder.h"
#include "util.h"
#include "tables.h"
#include "quantize_pvt.h"
#include "lame_global_flags.h"
#include "gain_analysis.h"
#include "VbrTag.h"
#include "bitstream.h"



/* unsigned int is at least this large:  */
/* we work with ints, so when doing bit manipulation, we limit
 * ourselves to MAX_LENGTH-2 just to be on the safe side */
#define MAX_LENGTH      32



#ifdef DEBUG
static int hogege;
#endif





/***********************************************************************
 * compute bitsperframe and mean_bits for a layer III frame
 **********************************************************************/
int
getframebits(const lame_global_flags * gfp)
{
    lame_internal_flags const *const gfc = gfp->internal_flags;
    int     bit_rate;

    /* get bitrate in kbps [?] */
    if (gfc->bitrate_index)
        bit_rate = bitrate_table[gfp->version][gfc->bitrate_index];
    else
        bit_rate = gfp->brate;
    /*assert(bit_rate <= 550); */
    assert(8 <= bit_rate && bit_rate <= 640);

    /* main encoding routine toggles padding on and off */
    /* one Layer3 Slot consists of 8 bits */
    return 8 * ((gfp->version + 1) * 72000 * bit_rate / gfp->out_samplerate + gfc->padding);
}




static void
putheader_bits(lame_internal_flags * gfc)
{
    Bit_stream_struc *bs;
    bs = &gfc->bs;
#ifdef DEBUG
    hogege += gfc->sideinfo_len * 8;
#endif
    memcpy(&bs->buf[bs->buf_byte_idx], gfc->header[gfc->w_ptr].buf, gfc->sideinfo_len);
    bs->buf_byte_idx += gfc->sideinfo_len;
    bs->totbit += gfc->sideinfo_len * 8;
    gfc->w_ptr = (gfc->w_ptr + 1) & (MAX_HEADER_BUF - 1);
}




/*write j bits into the bit stream */
inline static void
putbits2(lame_internal_flags * gfc, int val, int j)
{
    Bit_stream_struc *bs;
    bs = &gfc->bs;

    assert(j < MAX_LENGTH - 2);

    while (j > 0) {
        int     k;
        if (bs->buf_bit_idx == 0) {
            bs->buf_bit_idx = 8;
            bs->buf_byte_idx++;
            assert(bs->buf_byte_idx < BUFFER_SIZE);
            assert(gfc->header[gfc->w_ptr].write_timing >= bs->totbit);
            if (gfc->header[gfc->w_ptr].write_timing == bs->totbit) {
                putheader_bits(gfc);
            }
            bs->buf[bs->buf_byte_idx] = 0;
        }

        k = Min(j, bs->buf_bit_idx);
        j -= k;

        bs->buf_bit_idx -= k;

        assert(j < MAX_LENGTH); /* 32 too large on 32 bit machines */
        assert(bs->buf_bit_idx < MAX_LENGTH);

        bs->buf[bs->buf_byte_idx] |= ((val >> j) << bs->buf_bit_idx);
        bs->totbit += k;
    }
}

/*write j bits into the bit stream, ignoring frame headers */
inline static void
putbits_noheaders(lame_internal_flags * gfc, int val, int j)
{
    Bit_stream_struc *bs;
    bs = &gfc->bs;

    assert(j < MAX_LENGTH - 2);

    while (j > 0) {
        int     k;
        if (bs->buf_bit_idx == 0) {
            bs->buf_bit_idx = 8;
            bs->buf_byte_idx++;
            assert(bs->buf_byte_idx < BUFFER_SIZE);
            bs->buf[bs->buf_byte_idx] = 0;
        }

        k = Min(j, bs->buf_bit_idx);
        j -= k;

        bs->buf_bit_idx -= k;

        assert(j < MAX_LENGTH); /* 32 too large on 32 bit machines */
        assert(bs->buf_bit_idx < MAX_LENGTH);

        bs->buf[bs->buf_byte_idx] |= ((val >> j) << bs->buf_bit_idx);
        bs->totbit += k;
    }
}


/*
  Some combinations of bitrate, Fs, and stereo make it impossible to stuff
  out a frame using just main_data, due to the limited number of bits to
  indicate main_data_length. In these situations, we put stuffing bits into
  the ancillary data...
*/

inline static void
drain_into_ancillary(lame_global_flags const *gfp, int remainingBits)
{
    lame_internal_flags *const gfc = gfp->internal_flags;
    int     i;
    assert(remainingBits >= 0);

    if (remainingBits >= 8) {
        putbits2(gfc, 0x4c, 8);
        remainingBits -= 8;
    }
    if (remainingBits >= 8) {
        putbits2(gfc, 0x41, 8);
        remainingBits -= 8;
    }
    if (remainingBits >= 8) {
        putbits2(gfc, 0x4d, 8);
        remainingBits -= 8;
    }
    if (remainingBits >= 8) {
        putbits2(gfc, 0x45, 8);
        remainingBits -= 8;
    }

    if (remainingBits >= 32) {
        const char *const version = get_lame_short_version();
        if (remainingBits >= 32)
            for (i = 0; i < (int) strlen(version) && remainingBits >= 8; ++i) {
                remainingBits -= 8;
                putbits2(gfc, version[i], 8);
            }
    }

    for (; remainingBits >= 1; remainingBits -= 1) {
        putbits2(gfc, gfc->ancillary_flag, 1);
        gfc->ancillary_flag ^= !gfp->disable_reservoir;
    }

    assert(remainingBits == 0);

}

/*write N bits into the header */
inline static void
writeheader(lame_internal_flags * gfc, int val, int j)
{
    int     ptr = gfc->header[gfc->h_ptr].ptr;

    while (j > 0) {
        int const k = Min(j, 8 - (ptr & 7));
        j -= k;
        assert(j < MAX_LENGTH); /* >> 32  too large for 32 bit machines */
        gfc->header[gfc->h_ptr].buf[ptr >> 3]
            |= ((val >> j)) << (8 - (ptr & 7) - k);
        ptr += k;
    }
    gfc->header[gfc->h_ptr].ptr = ptr;
}


static int
CRC_update(int value, int crc)
{
    int     i;
    value <<= 8;
    for (i = 0; i < 8; i++) {
        value <<= 1;
        crc <<= 1;

        if (((crc ^ value) & 0x10000))
            crc ^= CRC16_POLYNOMIAL;
    }
    return crc;
}


void
CRC_writeheader(lame_internal_flags const *gfc, char *header)
{
    int     crc = 0xffff;    /* (jo) init crc16 for error_protection */
    int     i;

    crc = CRC_update(((unsigned char *) header)[2], crc);
    crc = CRC_update(((unsigned char *) header)[3], crc);
    for (i = 6; i < gfc->sideinfo_len; i++) {
        crc = CRC_update(((unsigned char *) header)[i], crc);
    }

    header[4] = crc >> 8;
    header[5] = crc & 255;
}

inline static void
encodeSideInfo2(lame_global_flags const *gfp, int bitsPerFrame)
{
    lame_internal_flags *const gfc = gfp->internal_flags;
    III_side_info_t *l3_side;
    int     gr, ch;

    l3_side = &gfc->l3_side;
    gfc->header[gfc->h_ptr].ptr = 0;
    memset(gfc->header[gfc->h_ptr].buf, 0, gfc->sideinfo_len);
    if (gfp->out_samplerate < 16000)
        writeheader(gfc, 0xffe, 12);
    else
        writeheader(gfc, 0xfff, 12);
    writeheader(gfc, (gfp->version), 1);
    writeheader(gfc, 4 - 3, 2);
    writeheader(gfc, (!gfp->error_protection), 1);
    writeheader(gfc, (gfc->bitrate_index), 4);
    writeheader(gfc, (gfc->samplerate_index), 2);
    writeheader(gfc, (gfc->padding), 1);
    writeheader(gfc, (gfp->extension), 1);
    writeheader(gfc, (gfp->mode), 2);
    writeheader(gfc, (gfc->mode_ext), 2);
    writeheader(gfc, (gfp->copyright), 1);
    writeheader(gfc, (gfp->original), 1);
    writeheader(gfc, (gfp->emphasis), 2);
    if (gfp->error_protection) {
        writeheader(gfc, 0, 16); /* dummy */
    }

    if (gfp->version == 1) {
        /* MPEG1 */
        assert(l3_side->main_data_begin >= 0);
        writeheader(gfc, (l3_side->main_data_begin), 9);

        if (gfc->channels_out == 2)
            writeheader(gfc, l3_side->private_bits, 3);
        else
            writeheader(gfc, l3_side->private_bits, 5);

        for (ch = 0; ch < gfc->channels_out; ch++) {
            int     band;
            for (band = 0; band < 4; band++) {
                writeheader(gfc, l3_side->scfsi[ch][band], 1);
            }
        }

        for (gr = 0; gr < 2; gr++) {
            for (ch = 0; ch < gfc->channels_out; ch++) {
                gr_info *const gi = &l3_side->tt[gr][ch];
                writeheader(gfc, gi->part2_3_length + gi->part2_length, 12);
                writeheader(gfc, gi->big_values / 2, 9);
                writeheader(gfc, gi->global_gain, 8);
                writeheader(gfc, gi->scalefac_compress, 4);

                if (gi->block_type != NORM_TYPE) {
                    writeheader(gfc, 1, 1); /* window_switching_flag */
                    writeheader(gfc, gi->block_type, 2);
                    writeheader(gfc, gi->mixed_block_flag, 1);

                    if (gi->table_select[0] == 14)
                        gi->table_select[0] = 16;
                    writeheader(gfc, gi->table_select[0], 5);
                    if (gi->table_select[1] == 14)
                        gi->table_select[1] = 16;
                    writeheader(gfc, gi->table_select[1], 5);

                    writeheader(gfc, gi->subblock_gain[0], 3);
                    writeheader(gfc, gi->subblock_gain[1], 3);
                    writeheader(gfc, gi->subblock_gain[2], 3);
                }
                else {
                    writeheader(gfc, 0, 1); /* window_switching_flag */
                    if (gi->table_select[0] == 14)
                        gi->table_select[0] = 16;
                    writeheader(gfc, gi->table_select[0], 5);
                    if (gi->table_select[1] == 14)
                        gi->table_select[1] = 16;
                    writeheader(gfc, gi->table_select[1], 5);
                    if (gi->table_select[2] == 14)
                        gi->table_select[2] = 16;
                    writeheader(gfc, gi->table_select[2], 5);

                    assert(0 <= gi->region0_count && gi->region0_count < 16);
                    assert(0 <= gi->region1_count && gi->region1_count < 8);
                    writeheader(gfc, gi->region0_count, 4);
                    writeheader(gfc, gi->region1_count, 3);
                }
                writeheader(gfc, gi->preflag, 1);
                writeheader(gfc, gi->scalefac_scale, 1);
                writeheader(gfc, gi->count1table_select, 1);
            }
        }
    }
    else {
        /* MPEG2 */
        assert(l3_side->main_data_begin >= 0);
        writeheader(gfc, (l3_side->main_data_begin), 8);
        writeheader(gfc, l3_side->private_bits, gfc->channels_out);

        gr = 0;
        for (ch = 0; ch < gfc->channels_out; ch++) {
            gr_info *const gi = &l3_side->tt[gr][ch];
            writeheader(gfc, gi->part2_3_length + gi->part2_length, 12);
            writeheader(gfc, gi->big_values / 2, 9);
            writeheader(gfc, gi->global_gain, 8);
            writeheader(gfc, gi->scalefac_compress, 9);

            if (gi->block_type != NORM_TYPE) {
                writeheader(gfc, 1, 1); /* window_switching_flag */
                writeheader(gfc, gi->block_type, 2);
                writeheader(gfc, gi->mixed_block_flag, 1);

                if (gi->table_select[0] == 14)
                    gi->table_select[0] = 16;
                writeheader(gfc, gi->table_select[0], 5);
                if (gi->table_select[1] == 14)
                    gi->table_select[1] = 16;
                writeheader(gfc, gi->table_select[1], 5);

                writeheader(gfc, gi->subblock_gain[0], 3);
                writeheader(gfc, gi->subblock_gain[1], 3);
                writeheader(gfc, gi->subblock_gain[2], 3);
            }
            else {
                writeheader(gfc, 0, 1); /* window_switching_flag */
                if (gi->table_select[0] == 14)
                    gi->table_select[0] = 16;
                writeheader(gfc, gi->table_select[0], 5);
                if (gi->table_select[1] == 14)
                    gi->table_select[1] = 16;
                writeheader(gfc, gi->table_select[1], 5);
                if (gi->table_select[2] == 14)
                    gi->table_select[2] = 16;
                writeheader(gfc, gi->table_select[2], 5);

                assert(0 <= gi->region0_count && gi->region0_count < 16);
                assert(0 <= gi->region1_count && gi->region1_count < 8);
                writeheader(gfc, gi->region0_count, 4);
                writeheader(gfc, gi->region1_count, 3);
            }

            writeheader(gfc, gi->scalefac_scale, 1);
            writeheader(gfc, gi->count1table_select, 1);
        }
    }

    if (gfp->error_protection) {
        /* (jo) error_protection: add crc16 information to header */
        CRC_writeheader(gfc, gfc->header[gfc->h_ptr].buf);
    }

    {
        int const old = gfc->h_ptr;
        assert(gfc->header[old].ptr == gfc->sideinfo_len * 8);

        gfc->h_ptr = (old + 1) & (MAX_HEADER_BUF - 1);
        gfc->header[gfc->h_ptr].write_timing = gfc->header[old].write_timing + bitsPerFrame;

        if (gfc->h_ptr == gfc->w_ptr) {
            /* yikes! we are out of header buffer space */
            ERRORF(gfc, "Error: MAX_HEADER_BUF too small in bitstream.c \n");
        }

    }
}


inline static int
huffman_coder_count1(lame_internal_flags * gfc, gr_info const *gi)
{
    /* Write count1 area */
    struct huffcodetab const *const h = &ht[gi->count1table_select + 32];
    int     i, bits = 0;
#ifdef DEBUG
    int     gegebo = gfc->bs.totbit;
#endif

    int const *ix = &gi->l3_enc[gi->big_values];
    FLOAT const *xr = &gi->xr[gi->big_values];
    assert(gi->count1table_select < 2);

    for (i = (gi->count1 - gi->big_values) / 4; i > 0; --i) {
        int     huffbits = 0;
        int     p = 0, v;

        v = ix[0];
        if (v) {
            p += 8;
            if (xr[0] < 0)
                huffbits++;
            assert(v <= 1);
        }

        v = ix[1];
        if (v) {
            p += 4;
            huffbits *= 2;
            if (xr[1] < 0)
                huffbits++;
            assert(v <= 1);
        }

        v = ix[2];
        if (v) {
            p += 2;
            huffbits *= 2;
            if (xr[2] < 0)
                huffbits++;
            assert(v <= 1);
        }

        v = ix[3];
        if (v) {
            p++;
            huffbits *= 2;
            if (xr[3] < 0)
                huffbits++;
            assert(v <= 1);
        }

        ix += 4;
        xr += 4;
        putbits2(gfc, huffbits + h->table[p], h->hlen[p]);
        bits += h->hlen[p];
    }
#ifdef DEBUG
    DEBUGF(gfc, "count1: real: %ld counted:%d (bigv %d count1len %d)\n",
           gfc->bs.totbit - gegebo, gi->count1bits, gi->big_values, gi->count1);
#endif
    return bits;
}



/*
  Implements the pseudocode of page 98 of the IS
  */
inline static int
Huffmancode(lame_internal_flags * const gfc, const unsigned int tableindex,
            int start, int end, gr_info const *gi)
{
    struct huffcodetab const *const h = &ht[tableindex];
    int     i, bits = 0;

    assert(tableindex < 32u);
    if (!tableindex)
        return bits;

    for (i = start; i < end; i += 2) {
        int     cbits = 0;
        int     xbits = 0;
        int const linbits = h->xlen;
        int     xlen = h->xlen;
        int     ext = 0;
        int     x1 = gi->l3_enc[i];
        int     x2 = gi->l3_enc[i + 1];

        if (x1 != 0) {
            if (gi->xr[i] < 0)
                ext++;
            cbits--;
        }

        if (tableindex > 15) {
            /* use ESC-words */
            if (x1 > 14) {
                int const linbits_x1 = x1 - 15;
                assert(linbits_x1 <= h->linmax);
                ext |= linbits_x1 << 1;
                xbits = linbits;
                x1 = 15;
            }

            if (x2 > 14) {
                int const linbits_x2 = x2 - 15;
                assert(linbits_x2 <= h->linmax);
                ext <<= linbits;
                ext |= linbits_x2;
                xbits += linbits;
                x2 = 15;
            }
            xlen = 16;
        }

        if (x2 != 0) {
            ext <<= 1;
            if (gi->xr[i + 1] < 0)
                ext++;
            cbits--;
        }

        assert((x1 | x2) < 16);

        x1 = x1 * xlen + x2;
        xbits -= cbits;
        cbits += h->hlen[x1];

        assert(cbits <= MAX_LENGTH);
        assert(xbits <= MAX_LENGTH);

        putbits2(gfc, h->table[x1], cbits);
        putbits2(gfc, ext, xbits);
        bits += cbits + xbits;
    }
    return bits;
}

/*
  Note the discussion of huffmancodebits() on pages 28
  and 29 of the IS, as well as the definitions of the side
  information on pages 26 and 27.
  */
static int
ShortHuffmancodebits(lame_internal_flags * gfc, gr_info const *gi)
{
    int     bits;
    int     region1Start;

    region1Start = 3 * gfc->scalefac_band.s[3];
    if (region1Start > gi->big_values)
        region1Start = gi->big_values;

    /* short blocks do not have a region2 */
    bits = Huffmancode(gfc, gi->table_select[0], 0, region1Start, gi);
    bits += Huffmancode(gfc, gi->table_select[1], region1Start, gi->big_values, gi);
    return bits;
}

static int
LongHuffmancodebits(lame_internal_flags * gfc, gr_info const *gi)
{
    int     i, bigvalues, bits;
    int     region1Start, region2Start;

    bigvalues = gi->big_values;
    assert(0 <= bigvalues && bigvalues <= 576);

    i = gi->region0_count + 1;
    assert(0 <= i);
    assert((size_t) i < dimension_of(gfc->scalefac_band.l));
    region1Start = gfc->scalefac_band.l[i];
    i += gi->region1_count + 1;
    assert(0 <= i);
    assert((size_t) i < dimension_of(gfc->scalefac_band.l));
    region2Start = gfc->scalefac_band.l[i];

    if (region1Start > bigvalues)
        region1Start = bigvalues;

    if (region2Start > bigvalues)
        region2Start = bigvalues;

    bits = Huffmancode(gfc, gi->table_select[0], 0, region1Start, gi);
    bits += Huffmancode(gfc, gi->table_select[1], region1Start, region2Start, gi);
    bits += Huffmancode(gfc, gi->table_select[2], region2Start, bigvalues, gi);
    return bits;
}

inline static int
writeMainData(lame_global_flags const *const gfp)
{
    int     gr, ch, sfb, data_bits, tot_bits = 0;
    lame_internal_flags *const gfc = gfp->internal_flags;
    III_side_info_t const *const l3_side = &gfc->l3_side;

    if (gfp->version == 1) {
        /* MPEG 1 */
        for (gr = 0; gr < 2; gr++) {
            for (ch = 0; ch < gfc->channels_out; ch++) {
                gr_info const *const gi = &l3_side->tt[gr][ch];
                int const slen1 = slen1_tab[gi->scalefac_compress];
                int const slen2 = slen2_tab[gi->scalefac_compress];
                data_bits = 0;
#ifdef DEBUG
                hogege = gfc->bs.totbit;
#endif
                for (sfb = 0; sfb < gi->sfbdivide; sfb++) {
                    if (gi->scalefac[sfb] == -1)
                        continue; /* scfsi is used */
                    putbits2(gfc, gi->scalefac[sfb], slen1);
                    data_bits += slen1;
                }
                for (; sfb < gi->sfbmax; sfb++) {
                    if (gi->scalefac[sfb] == -1)
                        continue; /* scfsi is used */
                    putbits2(gfc, gi->scalefac[sfb], slen2);
                    data_bits += slen2;
                }
                assert(data_bits == gi->part2_length);

                if (gi->block_type == SHORT_TYPE) {
                    data_bits += ShortHuffmancodebits(gfc, gi);
                }
                else {
                    data_bits += LongHuffmancodebits(gfc, gi);
                }
                data_bits += huffman_coder_count1(gfc, gi);
#ifdef DEBUG
                DEBUGF(gfc, "<%ld> ", gfc->bs.totbit - hogege);
#endif
                /* does bitcount in quantize.c agree with actual bit count? */
                assert(data_bits == gi->part2_3_length + gi->part2_length);
                tot_bits += data_bits;
            }           /* for ch */
        }               /* for gr */
    }
    else {
        /* MPEG 2 */
        gr = 0;
        for (ch = 0; ch < gfc->channels_out; ch++) {
            gr_info const *const gi = &l3_side->tt[gr][ch];
            int     i, sfb_partition, scale_bits = 0;
            assert(gi->sfb_partition_table);
            data_bits = 0;
#ifdef DEBUG
            hogege = gfc->bs.totbit;
#endif
            sfb = 0;
            sfb_partition = 0;

            if (gi->block_type == SHORT_TYPE) {
                for (; sfb_partition < 4; sfb_partition++) {
                    int const sfbs = gi->sfb_partition_table[sfb_partition] / 3;
                    int const slen = gi->slen[sfb_partition];
                    for (i = 0; i < sfbs; i++, sfb++) {
                        putbits2(gfc, Max(gi->scalefac[sfb * 3 + 0], 0), slen);
                        putbits2(gfc, Max(gi->scalefac[sfb * 3 + 1], 0), slen);
                        putbits2(gfc, Max(gi->scalefac[sfb * 3 + 2], 0), slen);
                        scale_bits += 3 * slen;
                    }
                }
                data_bits += ShortHuffmancodebits(gfc, gi);
            }
            else {
                for (; sfb_partition < 4; sfb_partition++) {
                    int const sfbs = gi->sfb_partition_table[sfb_partition];
                    int const slen = gi->slen[sfb_partition];
                    for (i = 0; i < sfbs; i++, sfb++) {
                        putbits2(gfc, Max(gi->scalefac[sfb], 0), slen);
                        scale_bits += slen;
                    }
                }
                data_bits += LongHuffmancodebits(gfc, gi);
            }
            data_bits += huffman_coder_count1(gfc, gi);
#ifdef DEBUG
            DEBUGF(gfc, "<%ld> ", gfc->bs.totbit - hogege);
#endif
            /* does bitcount in quantize.c agree with actual bit count? */
            assert(data_bits == gi->part2_3_length);
            assert(scale_bits == gi->part2_length);
            tot_bits += scale_bits + data_bits;
        }               /* for ch */
    }                   /* for gf */
    return tot_bits;
}                       /* main_data */



/* compute the number of bits required to flush all mp3 frames
   currently in the buffer.  This should be the same as the
   reservoir size.  Only call this routine between frames - i.e.
   only after all headers and data have been added to the buffer
   by format_bitstream().

   Also compute total_bits_output =
       size of mp3 buffer (including frame headers which may not
       have yet been send to the mp3 buffer) +
       number of bits needed to flush all mp3 frames.

   total_bytes_output is the size of the mp3 output buffer if
   lame_encode_flush_nogap() was called right now.

 */
int
compute_flushbits(const lame_global_flags * gfp, int *total_bytes_output)
{
    lame_internal_flags const *const gfc = gfp->internal_flags;
    int     flushbits, remaining_headers;
    int     bitsPerFrame;
    int     last_ptr, first_ptr;
    first_ptr = gfc->w_ptr; /* first header to add to bitstream */
    last_ptr = gfc->h_ptr - 1; /* last header to add to bitstream */
    if (last_ptr == -1)
        last_ptr = MAX_HEADER_BUF - 1;

    /* add this many bits to bitstream so we can flush all headers */
    flushbits = gfc->header[last_ptr].write_timing - gfc->bs.totbit;
    *total_bytes_output = flushbits;

    if (flushbits >= 0) {
        /* if flushbits >= 0, some headers have not yet been written */
        /* reduce flushbits by the size of the headers */
        remaining_headers = 1 + last_ptr - first_ptr;
        if (last_ptr < first_ptr)
            remaining_headers = 1 + last_ptr - first_ptr + MAX_HEADER_BUF;
        flushbits -= remaining_headers * 8 * gfc->sideinfo_len;
    }


    /* finally, add some bits so that the last frame is complete
     * these bits are not necessary to decode the last frame, but
     * some decoders will ignore last frame if these bits are missing
     */
    bitsPerFrame = getframebits(gfp);
    flushbits += bitsPerFrame;
    *total_bytes_output += bitsPerFrame;
    /* round up:   */
    if (*total_bytes_output % 8)
        *total_bytes_output = 1 + (*total_bytes_output / 8);
    else
        *total_bytes_output = (*total_bytes_output / 8);
    *total_bytes_output += gfc->bs.buf_byte_idx + 1;


    if (flushbits < 0) {
#if 0
        /* if flushbits < 0, this would mean that the buffer looks like:
         * (data...)  last_header  (data...)  (extra data that should not be here...)
         */
        DEBUGF(gfc, "last header write_timing = %i \n", gfc->header[last_ptr].write_timing);
        DEBUGF(gfc, "first header write_timing = %i \n", gfc->header[first_ptr].write_timing);
        DEBUGF(gfc, "bs.totbit:                 %i \n", gfc->bs.totbit);
        DEBUGF(gfc, "first_ptr, last_ptr        %i %i \n", first_ptr, last_ptr);
        DEBUGF(gfc, "remaining_headers =        %i \n", remaining_headers);
        DEBUGF(gfc, "bitsperframe:              %i \n", bitsPerFrame);
        DEBUGF(gfc, "sidelen:                   %i \n", gfc->sideinfo_len);
#endif
        ERRORF(gfc, "strange error flushing buffer ... \n");
    }
    return flushbits;
}



void
flush_bitstream(lame_global_flags const *gfp)
{
    lame_internal_flags *const gfc = gfp->internal_flags;
    III_side_info_t *l3_side;
    int     nbytes;
    int     flushbits;
    int     last_ptr = gfc->h_ptr - 1; /* last header to add to bitstream */
    if (last_ptr == -1)
        last_ptr = MAX_HEADER_BUF - 1;
    l3_side = &gfc->l3_side;


    if ((flushbits = compute_flushbits(gfp, &nbytes)) < 0)
        return;
    drain_into_ancillary(gfp, flushbits);

    /* check that the 100% of the last frame has been written to bitstream */
    assert(gfc->header[last_ptr].write_timing + getframebits(gfp)
           == gfc->bs.totbit);

    /* we have padded out all frames with ancillary data, which is the
       same as filling the bitreservoir with ancillary data, so : */
    gfc->ResvSize = 0;
    l3_side->main_data_begin = 0;


    /* save the ReplayGain value */
    if (gfc->findReplayGain) {
        FLOAT const RadioGain = (FLOAT) GetTitleGain(gfc->rgdata);
        assert(NEQ(RadioGain, GAIN_NOT_ENOUGH_SAMPLES));
        gfc->RadioGain = (int) floor(RadioGain * 10.0 + 0.5); /* round to nearest */
    }

    /* find the gain and scale change required for no clipping */
    if (gfc->findPeakSample) {
        gfc->noclipGainChange = (int) ceil(log10(gfc->PeakSample / 32767.0) * 20.0 * 10.0); /* round up */

        if (gfc->noclipGainChange > 0) { /* clipping occurs */
            if (EQ(gfp->scale, 1.0) || EQ(gfp->scale, 0.0))
                gfc->noclipScale = floor((32767.0 / gfc->PeakSample) * 100.0) / 100.0; /* round down */
            else
                /* the user specified his own scaling factor. We could suggest
                 * the scaling factor of (32767.0/gfp->PeakSample)*(gfp->scale)
                 * but it's usually very inaccurate. So we'd rather not advice him
                 * on the scaling factor. */
                gfc->noclipScale = -1;
        }
        else            /* no clipping */
            gfc->noclipScale = -1;
    }
}




void
add_dummy_byte(lame_global_flags const *const gfp, unsigned char val, unsigned int n)
{
    lame_internal_flags *const gfc = gfp->internal_flags;
    int     i;

    while (n-- > 0) {
        putbits_noheaders(gfc, val, 8);

        for (i = 0; i < MAX_HEADER_BUF; ++i)
            gfc->header[i].write_timing += 8;
    }
}


/*
  format_bitstream()

  This is called after a frame of audio has been quantized and coded.
  It will write the encoded audio to the bitstream. Note that
  from a layer3 encoder's perspective the bit stream is primarily
  a series of main_data() blocks, with header and side information
  inserted at the proper locations to maintain framing. (See Figure A.7
  in the IS).
  */
int
format_bitstream(lame_global_flags const *gfp)
{
    lame_internal_flags *const gfc = gfp->internal_flags;
    int     bits, nbytes;
    III_side_info_t *l3_side;
    int     bitsPerFrame;
    l3_side = &gfc->l3_side;

    bitsPerFrame = getframebits(gfp);
    drain_into_ancillary(gfp, l3_side->resvDrain_pre);

    encodeSideInfo2(gfp, bitsPerFrame);
    bits = 8 * gfc->sideinfo_len;
    bits += writeMainData(gfp);
    drain_into_ancillary(gfp, l3_side->resvDrain_post);
    bits += l3_side->resvDrain_post;

    l3_side->main_data_begin += (bitsPerFrame - bits) / 8;

    /* compare number of bits needed to clear all buffered mp3 frames
     * with what we think the resvsize is: */
    if (compute_flushbits(gfp, &nbytes) != gfc->ResvSize) {
        ERRORF(gfc, "Internal buffer inconsistency. flushbits <> ResvSize");
    }


    /* compare main_data_begin for the next frame with what we
     * think the resvsize is: */
    if ((l3_side->main_data_begin * 8) != gfc->ResvSize) {
        ERRORF(gfc, "bit reservoir error: \n"
               "l3_side->main_data_begin: %i \n"
               "Resvoir size:             %i \n"
               "resv drain (post)         %i \n"
               "resv drain (pre)          %i \n"
               "header and sideinfo:      %i \n"
               "data bits:                %i \n"
               "total bits:               %i (remainder: %i) \n"
               "bitsperframe:             %i \n",
               8 * l3_side->main_data_begin,
               gfc->ResvSize,
               l3_side->resvDrain_post,
               l3_side->resvDrain_pre,
               8 * gfc->sideinfo_len,
               bits - l3_side->resvDrain_post - 8 * gfc->sideinfo_len,
               bits, bits % 8, bitsPerFrame);

        ERRORF(gfc, "This is a fatal error.  It has several possible causes:");
        ERRORF(gfc, "90%%  LAME compiled with buggy version of gcc using advanced optimizations");
        ERRORF(gfc, " 9%%  Your system is overclocked");
        ERRORF(gfc, " 1%%  bug in LAME encoding library");

        gfc->ResvSize = l3_side->main_data_begin * 8;
    };
    assert(gfc->bs.totbit % 8 == 0);

    if (gfc->bs.totbit > 1000000000) {
        /* to avoid totbit overflow, (at 8h encoding at 128kbs) lets reset bit counter */
        int     i;
        for (i = 0; i < MAX_HEADER_BUF; ++i)
            gfc->header[i].write_timing -= gfc->bs.totbit;
        gfc->bs.totbit = 0;
    }


    return 0;
}





/* copy data out of the internal MP3 bit buffer into a user supplied
   unsigned char buffer.

   mp3data=0      indicates data in buffer is an id3tags and VBR tags
   mp3data=1      data is real mp3 frame data.


*/
int
copy_buffer(lame_internal_flags * gfc, unsigned char *buffer, int size, int mp3data)
{
    Bit_stream_struc *const bs = &gfc->bs;
    int const minimum = bs->buf_byte_idx + 1;
    if (minimum <= 0)
        return 0;
    if (size != 0 && minimum > size)
        return -1;      /* buffer is too small */
    memcpy(buffer, bs->buf, minimum);
    bs->buf_byte_idx = -1;
    bs->buf_bit_idx = 0;

    if (mp3data) {
        UpdateMusicCRC(&gfc->nMusicCRC, buffer, minimum);

        /** sum number of bytes belonging to the mp3 stream
         *  this info will be written into the Xing/LAME header for seeking
         */
        if (minimum > 0) {
            gfc->VBR_seek_table.nBytesWritten += minimum;
        }

#ifdef DECODE_ON_THE_FLY
        if (gfc->decode_on_the_fly) { /* decode the frame */
            sample_t pcm_buf[2][1152];
            int     mp3_in = minimum;
            int     samples_out = -1;
            int     i;

            /* re-synthesis to pcm.  Repeat until we get a samples_out=0 */
            while (samples_out != 0) {

                samples_out = hip_decode1_unclipped(gfc->hip, buffer, mp3_in, pcm_buf[0], pcm_buf[1]);
                /* samples_out = 0:  need more data to decode
                 * samples_out = -1:  error.  Lets assume 0 pcm output
                 * samples_out = number of samples output */

                /* set the lenght of the mp3 input buffer to zero, so that in the
                 * next iteration of the loop we will be querying mpglib about
                 * buffered data */
                mp3_in = 0;

                if (samples_out == -1) {
                    /* error decoding. Not fatal, but might screw up
                     * the ReplayGain tag. What should we do? Ignore for now */
                    samples_out = 0;
                }
                if (samples_out > 0) {
                    /* process the PCM data */

                    /* this should not be possible, and indicates we have
                     * overflown the pcm_buf buffer */
                    assert(samples_out <= 1152);

                    if (gfc->findPeakSample) {
                        for (i = 0; i < samples_out; i++) {
                            if (pcm_buf[0][i] > gfc->PeakSample)
                                gfc->PeakSample = pcm_buf[0][i];
                            else if (-pcm_buf[0][i] > gfc->PeakSample)
                                gfc->PeakSample = -pcm_buf[0][i];
                        }
                        if (gfc->channels_out > 1)
                            for (i = 0; i < samples_out; i++) {
                                if (pcm_buf[1][i] > gfc->PeakSample)
                                    gfc->PeakSample = pcm_buf[1][i];
                                else if (-pcm_buf[1][i] > gfc->PeakSample)
                                    gfc->PeakSample = -pcm_buf[1][i];
                            }
                    }

                    if (gfc->findReplayGain)
                        if (AnalyzeSamples
                            (gfc->rgdata, pcm_buf[0], pcm_buf[1], samples_out,
                             gfc->channels_out) == GAIN_ANALYSIS_ERROR)
                            return -6;

                }       /* if (samples_out>0) */
            }           /* while (samples_out!=0) */
        }               /* if (gfc->decode_on_the_fly) */
#endif

    }                   /* if (mp3data) */
    return minimum;
}


void
init_bit_stream_w(lame_internal_flags * gfc)
{
    gfc->bs.buf = (unsigned char *) malloc(BUFFER_SIZE);
    gfc->bs.buf_size = BUFFER_SIZE;

    gfc->h_ptr = gfc->w_ptr = 0;
    gfc->header[gfc->h_ptr].write_timing = 0;
    gfc->bs.buf_byte_idx = -1;
    gfc->bs.buf_bit_idx = 0;
    gfc->bs.totbit = 0;
}

/* end of bitstream.c */
