/*
 * Ogg muxer
 * Copyright (c) 2007 Baptiste Coudurier <baptiste dot coudurier at free dot fr>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "libavutil/crc.h"
#include "libavutil/random_seed.h"
#include "libavcodec/xiph.h"
#include "libavcodec/bytestream.h"
#include "libavcodec/flac.h"
#include "avformat.h"
#include "internal.h"
#include "vorbiscomment.h"

#define MAX_PAGE_SIZE 65025

typedef struct {
    int64_t granule;
    int stream_index;
    uint8_t flags;
    uint8_t segments_count;
    uint8_t segments[255];
    uint8_t data[MAX_PAGE_SIZE];
    uint16_t size;
} OGGPage;

typedef struct {
    unsigned page_counter;
    uint8_t *header[3];
    int header_len[3];
    int free_header; // if the header needs to be freed outside of libvorbis
    /** for theora granule */
    int kfgshift;
    int64_t last_kf_pts;
    int vrev;
    int eos;
    unsigned page_count; ///< number of page buffered
    OGGPage page; ///< current page
    unsigned serial_num; ///< serial number
} OGGStreamContext;

typedef struct OGGPageList {
    OGGPage page;
    struct OGGPageList *next;
} OGGPageList;

typedef struct {
    OGGPageList *page_list;
} OGGContext;

static void ogg_update_checksum(AVFormatContext *s, int64_t crc_offset)
{
    int64_t pos = url_ftell(s->pb);
    uint32_t checksum = get_checksum(s->pb);
    url_fseek(s->pb, crc_offset, SEEK_SET);
    put_be32(s->pb, checksum);
    url_fseek(s->pb, pos, SEEK_SET);
}

static void ogg_write_page(AVFormatContext *s, OGGPage *page, int extra_flags)
{
    OGGStreamContext *oggstream = s->streams[page->stream_index]->priv_data;
    int64_t crc_offset;

    init_checksum(s->pb, ff_crc04C11DB7_update, 0);
    put_tag(s->pb, "OggS");
    put_byte(s->pb, 0);
    put_byte(s->pb, page->flags | extra_flags);
    put_le64(s->pb, page->granule);
    put_le32(s->pb, oggstream->serial_num);
    put_le32(s->pb, oggstream->page_counter++);
    crc_offset = url_ftell(s->pb);
    put_le32(s->pb, 0); // crc
    put_byte(s->pb, page->segments_count);
    put_buffer(s->pb, page->segments, page->segments_count);
    put_buffer(s->pb, page->data, page->size);

    ogg_update_checksum(s, crc_offset);
    put_flush_packet(s->pb);
    oggstream->page_count--;
}

static int64_t ogg_granule_to_timestamp(OGGStreamContext *oggstream, OGGPage *page)
{
    if (oggstream->kfgshift)
        return (page->granule>>oggstream->kfgshift) +
            (page->granule & ((1<<oggstream->kfgshift)-1));
    else
        return page->granule;
}

static int ogg_compare_granule(AVFormatContext *s, OGGPage *next, OGGPage *page)
{
    AVStream *st2 = s->streams[next->stream_index];
    AVStream *st  = s->streams[page->stream_index];
    int64_t next_granule, cur_granule;

    if (next->granule == -1 || page->granule == -1)
        return 0;

    next_granule = av_rescale_q(ogg_granule_to_timestamp(st2->priv_data, next),
                                st2->time_base, AV_TIME_BASE_Q);
    cur_granule  = av_rescale_q(ogg_granule_to_timestamp(st->priv_data, page),
                                st ->time_base, AV_TIME_BASE_Q);
    return next_granule > cur_granule;
}

static int ogg_reset_cur_page(OGGStreamContext *oggstream)
{
    oggstream->page.granule = -1;
    oggstream->page.flags = 0;
    oggstream->page.segments_count = 0;
    oggstream->page.size = 0;
    return 0;
}

static int ogg_buffer_page(AVFormatContext *s, OGGStreamContext *oggstream)
{
    OGGContext *ogg = s->priv_data;
    OGGPageList **p = &ogg->page_list;
    OGGPageList *l = av_mallocz(sizeof(*l));

    if (!l)
        return AVERROR(ENOMEM);
    l->page = oggstream->page;

    oggstream->page_count++;
    ogg_reset_cur_page(oggstream);

    while (*p) {
        if (ogg_compare_granule(s, &(*p)->page, &l->page))
            break;
        p = &(*p)->next;
    }
    l->next = *p;
    *p = l;

    return 0;
}

static int ogg_buffer_data(AVFormatContext *s, AVStream *st,
                           uint8_t *data, unsigned size, int64_t granule)
{
    OGGStreamContext *oggstream = st->priv_data;
    int total_segments = size / 255 + 1;
    uint8_t *p = data;
    int i, segments, len;

    for (i = 0; i < total_segments; ) {
        OGGPage *page = &oggstream->page;

        segments = FFMIN(total_segments - i, 255 - page->segments_count);

        if (i && !page->segments_count)
            page->flags |= 1; // continued packet

        memset(page->segments+page->segments_count, 255, segments - 1);
        page->segments_count += segments - 1;

        len = FFMIN(size, segments*255);
        page->segments[page->segments_count++] = len - (segments-1)*255;
        memcpy(page->data+page->size, p, len);
        p += len;
        size -= len;
        i += segments;
        page->size += len;

        if (i == total_segments)
            page->granule = granule;

        if (page->segments_count == 255) {
            ogg_buffer_page(s, oggstream);
        }
    }
    return 0;
}

static uint8_t *ogg_write_vorbiscomment(int offset, int bitexact,
                                        int *header_len, AVMetadata *m)
{
    const char *vendor = bitexact ? "ffmpeg" : LIBAVFORMAT_IDENT;
    int size;
    uint8_t *p, *p0;
    unsigned int count;

    size = offset + ff_vorbiscomment_length(m, vendor, &count);
    p = av_mallocz(size);
    if (!p)
        return NULL;
    p0 = p;

    p += offset;
    ff_vorbiscomment_write(&p, m, vendor, count);

    *header_len = size;
    return p0;
}

static int ogg_build_comment_header(OGGStreamContext *oggstream, AVMetadata *m) {
    uint8_t *p = NULL;
    uint32_t i, c;
    size_t   size;

    uint8_t  *packet_type    = NULL;
    char     *packet_magic   = NULL;
    uint32_t vendor_length   = 0;
    char     *vendor_string  = NULL;
    uint32_t list_length     = 0;
    uint32_t *comment_length = NULL;
    char     *comment_string = NULL;
    uint8_t  framing_bit     = 0;

    int32_t  name_size, value_size;
    char     *name, *value;

    // load the old header in if it is set
    if (oggstream->header_len[1] > 0) {
        p = oggstream->header[1];
        packet_type  = (uint8_t*)p;
        p += sizeof(uint8_t);
        packet_magic = (char*)p;
        p += 6;

        // if the header is a valid vorbis header then read it
        if (*packet_type == 3 && strncmp(packet_magic, "vorbis", 6) == 0) {
            // copy the vendor so we can re-use it later
            vendor_length = *(uint32_t*)p;
            p += sizeof(uint32_t);
            vendor_string = (char*)av_mallocz(vendor_length + 1);
            if (!vendor_string)
              return AVERROR_NOMEM;
            memcpy(vendor_string, p, vendor_length);
            p += vendor_length;

            list_length = *(uint32_t*)p;
            p += sizeof(uint32_t);

            for(i = list_length; i > 0; --i) {
                comment_length = (uint32_t*)p;
                p += sizeof(uint32_t);

                // copy the comment so we can use strtok on it
                comment_string = (char*)av_mallocz(*comment_length + 1);
                if (!comment_string) {
                    av_free(vendor_string);
                    return AVERROR_NOMEM;
                }

                memcpy(comment_string, p, *comment_length);
                p += *comment_length;

                //split up the comment name & value
                name  = strtok(comment_string, "=");
                value = name + strlen(name) + 1;

                //set the name & value
                av_metadata_set2(&m, name, value, 0);

                //clean up
                av_free(comment_string);
            }

            framing_bit = *(uint8_t*)p;
        }
    }

    if (!vendor_length) {
        vendor_string = (char*)av_mallocz(sizeof(LIBAVFORMAT_IDENT));
        memcpy(vendor_string, LIBAVFORMAT_IDENT, sizeof(LIBAVFORMAT_IDENT) - 1);
        vendor_length = sizeof(LIBAVFORMAT_IDENT) - 1;
        framing_bit = 1;
    }

    // calculate the total header size
    size  = 7; // packet_type + packet_magic
    size += sizeof(uint32_t) + vendor_length;
    size += sizeof(uint32_t); // list length
    if (m)
        for(c = 0; c < m->count; ++c)
            size += sizeof(uint32_t) + strlen(m->elems[c].key) + 1 + strlen(m->elems[c].value);
    size += 1; // framing bit

    // allocate a new header
    oggstream->header_len[1] = size;
    oggstream->header    [1] = (uint8_t*)av_mallocz(size);
    if (!oggstream->header[1]) {
        oggstream->header_len[1] = 0;
        av_free(vendor_string);
        return AVERROR_NOMEM;
    }

    oggstream->free_header = 1;

    // setup the new header
    p = oggstream->header[1];
    *(uint8_t*)p = 3;
    p += sizeof(uint8_t);
    memcpy(p, "vorbis", 6);
    p += 6;

    // write the vendor length and string
    *(uint32_t*)p = vendor_length;
    p += sizeof(uint32_t);
    memcpy(p, vendor_string, vendor_length);
    p += vendor_length;

    // write the comments
    *(uint32_t*)p = m ? m->count : 0;
    p += sizeof(uint32_t);
    if (m)
        for(c = 0; c < m->count; ++c) {
          name_size  = strlen(m->elems[c].key  );
          value_size = strlen(m->elems[c].value);
          *(uint32_t*)p = name_size + 1 + value_size;
          p += sizeof(uint32_t);
          memcpy(p, m->elems[c].key, name_size);
          p += name_size;
          *(char*)p = '=';
          ++p;
          memcpy(p, m->elems[c].value, value_size);
          p += value_size;
        }

    // write the framing bit
    *(uint8_t*)p = framing_bit;

    // clean up and return success
    av_free(vendor_string);
    return 0;
}

static int ogg_build_flac_headers(AVCodecContext *avctx,
                                  OGGStreamContext *oggstream, int bitexact,
                                  AVMetadata *m)
{
    enum FLACExtradataFormat format;
    uint8_t *streaminfo;
    uint8_t *p;

    if (!ff_flac_is_extradata_valid(avctx, &format, &streaminfo))
        return -1;

    // first packet: STREAMINFO
    oggstream->header_len[0] = 51;
    oggstream->header[0] = av_mallocz(51); // per ogg flac specs
    p = oggstream->header[0];
    if (!p)
        return AVERROR(ENOMEM);
    bytestream_put_byte(&p, 0x7F);
    bytestream_put_buffer(&p, "FLAC", 4);
    bytestream_put_byte(&p, 1); // major version
    bytestream_put_byte(&p, 0); // minor version
    bytestream_put_be16(&p, 1); // headers packets without this one
    bytestream_put_buffer(&p, "fLaC", 4);
    bytestream_put_byte(&p, 0x00); // streaminfo
    bytestream_put_be24(&p, 34);
    bytestream_put_buffer(&p, streaminfo, FLAC_STREAMINFO_SIZE);

    // second packet: VorbisComment
    p = ogg_write_vorbiscomment(4, bitexact, &oggstream->header_len[1], m);
    if (!p)
        return AVERROR(ENOMEM);
    oggstream->header[1] = p;
    bytestream_put_byte(&p, 0x84); // last metadata block and vorbis comment
    bytestream_put_be24(&p, oggstream->header_len[1] - 4);

    return 0;
}

#define SPEEX_HEADER_SIZE 80

static int ogg_build_speex_headers(AVCodecContext *avctx,
                                   OGGStreamContext *oggstream, int bitexact,
                                   AVMetadata *m)
{
    uint8_t *p;

    if (avctx->extradata_size < SPEEX_HEADER_SIZE)
        return -1;

    // first packet: Speex header
    p = av_mallocz(SPEEX_HEADER_SIZE);
    if (!p)
        return AVERROR(ENOMEM);
    oggstream->header[0] = p;
    oggstream->header_len[0] = SPEEX_HEADER_SIZE;
    bytestream_put_buffer(&p, avctx->extradata, SPEEX_HEADER_SIZE);
    AV_WL32(&oggstream->header[0][68], 0);  // set extra_headers to 0

    // second packet: VorbisComment
    p = ogg_write_vorbiscomment(0, bitexact, &oggstream->header_len[1], m);
    if (!p)
        return AVERROR(ENOMEM);
    oggstream->header[1] = p;

    return 0;
}

static int ogg_write_header(AVFormatContext *s)
{
    OGGStreamContext *oggstream;
    int i, j, err;

    for (i = 0; i < s->nb_streams; i++) {
        AVStream *st = s->streams[i];
        unsigned serial_num = i;

        if (st->codec->codec_type == AVMEDIA_TYPE_AUDIO)
            av_set_pts_info(st, 64, 1, st->codec->sample_rate);
        else if (st->codec->codec_type == AVMEDIA_TYPE_VIDEO)
            av_set_pts_info(st, 64, st->codec->time_base.num, st->codec->time_base.den);
        if (st->codec->codec_id != CODEC_ID_VORBIS &&
            st->codec->codec_id != CODEC_ID_THEORA &&
            st->codec->codec_id != CODEC_ID_SPEEX  &&
            st->codec->codec_id != CODEC_ID_FLAC) {
            av_log(s, AV_LOG_ERROR, "Unsupported codec id in stream %d\n", i);
            return -1;
        }

        if (!st->codec->extradata || !st->codec->extradata_size) {
            av_log(s, AV_LOG_ERROR, "No extradata present\n");
            return -1;
        }
        oggstream = av_mallocz(sizeof(*oggstream));
        oggstream->page.stream_index = i;

        if (!(st->codec->flags & CODEC_FLAG_BITEXACT))
            do {
                serial_num = av_get_random_seed();
                for (j = 0; j < i; j++) {
                    OGGStreamContext *sc = s->streams[j]->priv_data;
                    if (serial_num == sc->serial_num)
                        break;
                }
            } while (j < i);
        oggstream->serial_num = serial_num;

        st->priv_data = oggstream;
        if (st->codec->codec_id == CODEC_ID_FLAC) {
            err = ogg_build_flac_headers(st->codec, oggstream,
                                             st->codec->flags & CODEC_FLAG_BITEXACT,
                                             s->metadata);
            if (err) {
                av_log(s, AV_LOG_ERROR, "Error writing FLAC headers\n");
                av_freep(&st->priv_data);
                return err;
            }
        } else if (st->codec->codec_id == CODEC_ID_SPEEX) {
            err = ogg_build_speex_headers(st->codec, oggstream,
                                              st->codec->flags & CODEC_FLAG_BITEXACT,
                                              s->metadata);
            if (err) {
                av_log(s, AV_LOG_ERROR, "Error writing Speex headers\n");
                av_freep(&st->priv_data);
                return err;
            }
        } else {
            if (ff_split_xiph_headers(st->codec->extradata, st->codec->extradata_size,
                                      st->codec->codec_id == CODEC_ID_VORBIS ? 30 : 42,
                                      oggstream->header, oggstream->header_len) < 0) {
                av_log(s, AV_LOG_ERROR, "Extradata corrupted\n");
                av_freep(&st->priv_data);
                return -1;
            }
            if (st->codec->codec_id == CODEC_ID_THEORA) {
                /** KFGSHIFT is the width of the less significant section of the granule position
                    The less significant section is the frame count since the last keyframe */
                oggstream->kfgshift = ((oggstream->header[0][40]&3)<<3)|(oggstream->header[0][41]>>5);
                oggstream->vrev = oggstream->header[0][9];
                av_log(s, AV_LOG_DEBUG, "theora kfgshift %d, vrev %d\n",
                       oggstream->kfgshift, oggstream->vrev);
            }

            err = ogg_build_comment_header(oggstream, s->metadata);
            if (err) {
                av_log(s, AV_LOG_ERROR, "Error writing Vorbis comment header\n");
                av_freep(&st->priv_data);
                return err;
            }
        }
    }

    for (j = 0; j < s->nb_streams; j++) {
        OGGStreamContext *oggstream = s->streams[j]->priv_data;
        ogg_buffer_data(s, s->streams[j], oggstream->header[0],
                        oggstream->header_len[0], 0);
        oggstream->page.flags |= 2; // bos
        ogg_buffer_page(s, oggstream);
    }
    for (j = 0; j < s->nb_streams; j++) {
        AVStream *st = s->streams[j];
        OGGStreamContext *oggstream = st->priv_data;
        for (i = 1; i < 3; i++) {
            if (oggstream && oggstream->header_len[i])
                ogg_buffer_data(s, st, oggstream->header[i],
                                oggstream->header_len[i], 0);
        }
        ogg_buffer_page(s, oggstream);
    }
    return 0;
}

static void ogg_write_pages(AVFormatContext *s, int flush)
{
    OGGContext *ogg = s->priv_data;
    OGGPageList *next, *p;

    if (!ogg->page_list)
        return;

    for (p = ogg->page_list; p; ) {
        OGGStreamContext *oggstream =
            s->streams[p->page.stream_index]->priv_data;
        if (oggstream->page_count < 2 && !flush)
            break;
        ogg_write_page(s, &p->page,
                       flush && oggstream->page_count == 1 ? 4 : 0); // eos
        next = p->next;
        av_freep(&p);
        p = next;
    }
    ogg->page_list = p;
}

static int ogg_write_packet(AVFormatContext *s, AVPacket *pkt)
{
    AVStream *st = s->streams[pkt->stream_index];
    OGGStreamContext *oggstream = st->priv_data;
    int ret;
    int64_t granule;

    if (st->codec->codec_id == CODEC_ID_THEORA) {
        int64_t pts = oggstream->vrev < 1 ? pkt->pts : pkt->pts + pkt->duration;
        int pframe_count;
        if (pkt->flags & AV_PKT_FLAG_KEY)
            oggstream->last_kf_pts = pts;
        pframe_count = pts - oggstream->last_kf_pts;
        // prevent frame count from overflow if key frame flag is not set
        if (pframe_count >= (1<<oggstream->kfgshift)) {
            oggstream->last_kf_pts += pframe_count;
            pframe_count = 0;
        }
        granule = (oggstream->last_kf_pts<<oggstream->kfgshift) | pframe_count;
    } else
        granule = pkt->pts + pkt->duration;

    ret = ogg_buffer_data(s, st, pkt->data, pkt->size, granule);
    if (ret < 0)
        return ret;

    ogg_write_pages(s, 0);

    return 0;
}

static int ogg_write_trailer(AVFormatContext *s)
{
    int i;

    /* flush current page */
    for (i = 0; i < s->nb_streams; i++)
        ogg_buffer_page(s, s->streams[i]->priv_data);

    ogg_write_pages(s, 1);

    for (i = 0; i < s->nb_streams; i++) {
        AVStream *st = s->streams[i];
        OGGStreamContext *oggstream = st->priv_data;
        if (oggstream->free_header)
            av_free(oggstream->header[1]);
        else
        if (st->codec->codec_id == CODEC_ID_FLAC ||
            st->codec->codec_id == CODEC_ID_SPEEX) {
            av_free(oggstream->header[0]);
            av_free(oggstream->header[1]);
        }
        av_freep(&st->priv_data);
    }
    return 0;
}

AVOutputFormat ogg_muxer = {
    "ogg",
    NULL_IF_CONFIG_SMALL("Ogg"),
    "application/ogg",
    "ogg,ogv,spx",
    sizeof(OGGContext),
    CODEC_ID_FLAC,
    CODEC_ID_THEORA,
    ogg_write_header,
    ogg_write_packet,
    ogg_write_trailer,
    .metadata_conv = ff_vorbiscomment_metadata_conv,
};
