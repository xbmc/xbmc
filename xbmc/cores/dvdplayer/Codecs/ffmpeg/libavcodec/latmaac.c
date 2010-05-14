/*
 * copyright (c) 2008 Paul Kendall <paul@kcbbs.gen.nz>
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

/**
 * @file latmaac.c
 * LATM wrapped AAC decoder
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>

#include "parser.h"
#include "get_bits.h"
#include "put_bits.h"
#include "mpeg4audio.h"
#include "neaacdec.h"

#define min(a,b) ((a)<(b) ? (a) : (b))

/*
    Note: This decoder filter is intended to decode LATM streams transferred
    in MPEG transport streams which only contain one program.
    To do a more complex LATM demuxing a separate LATM demuxer should be used.
*/

#define SYNC_LATM   0x2b7       // 11 bits
#define MAX_SIZE    8*1024

typedef struct AACDecoder
{
    faacDecHandle   aac_decoder;
    uint8_t         initialized;

    // parser data
    uint8_t         audio_mux_version_A;
    uint8_t         frameLengthType;
    uint8_t         extra[64];            // should be way enough
    int             extrasize;
} AACDecoder;

static inline int64_t latm_get_value(GetBitContext *b)
{
    uint8_t bytesForValue = get_bits(b, 2);
    int64_t value = 0;
    int i;
    for (i=0; i<=bytesForValue; i++) {
        value <<= 8;
        value |= get_bits(b, 8);
    }
    return value;
}

static void readGASpecificConfig(int audioObjectType, GetBitContext *b, PutBitContext *o)
{
    int framelen_flag;
    int dependsOnCoder;
    int ext_flag;

    framelen_flag = get_bits(b, 1);
    put_bits(o, 1, framelen_flag);
    dependsOnCoder = get_bits(b, 1);
    put_bits(o, 1, dependsOnCoder);
    if (dependsOnCoder) {
        int delay = get_bits(b, 14);
        put_bits(o, 14, delay);
    }
    ext_flag = get_bits(b, 1);
    put_bits(o, 1, ext_flag);
    
    if (audioObjectType == 6 || audioObjectType == 20) {
        int layerNr = get_bits(b, 3);
        put_bits(o, 3, layerNr);
    }
    if (ext_flag) {
        if (audioObjectType == 22) {
            skip_bits(b, 5);                    // numOfSubFrame
            skip_bits(b, 11);                   // layer_length

            put_bits(o, 16, 0);
        }
        if (audioObjectType == 17 ||
                audioObjectType == 19 ||
                audioObjectType == 20 ||
                audioObjectType == 23) {

            skip_bits(b, 3);                    // stuff
            put_bits(o, 3, 0);
        }

        skip_bits(b, 1);                        // extflag3
        put_bits(o, 1, 0);
    }
}

static int readAudioSpecificConfig(struct AACDecoder *decoder, GetBitContext *b)
{
    PutBitContext o;
    int ret = 0;
    int audioObjectType;
    int samplingFrequencyIndex;
    int channelConfiguration;

    init_put_bits(&o, decoder->extra, sizeof(decoder->extra));

    audioObjectType = get_bits(b, 5);
    put_bits(&o, 5, audioObjectType);
    if (audioObjectType == 31) {
        uint8_t extended = get_bits(b, 6);
        put_bits(&o, 6, extended);
        audioObjectType = 32 + extended;
    }

    samplingFrequencyIndex = get_bits(b, 4);
    put_bits(&o, 4, samplingFrequencyIndex);
    if (samplingFrequencyIndex == 0x0f) {
        uint32_t f = get_bits_long(b, 24);
        put_bits(&o, 24, f);
    }
    channelConfiguration = get_bits(b, 4);
    put_bits(&o, 4, channelConfiguration);

    if (audioObjectType == 1 || audioObjectType == 2 || audioObjectType == 3
            || audioObjectType == 4 || audioObjectType == 6 || audioObjectType == 7) {
        readGASpecificConfig(audioObjectType, b, &o);
    } else if (audioObjectType == 5) {
        int sbr_present = 1;
        samplingFrequencyIndex = get_bits(b, 4);
        if (samplingFrequencyIndex == 0x0f) {
            uint32_t f = get_bits_long(b, 24);
            put_bits(&o, 24, f);
        }
        audioObjectType = get_bits(b, 5);
        put_bits(&o, 5, audioObjectType);
    } else if (audioObjectType >= 17) {
        int epConfig;
        readGASpecificConfig(audioObjectType, b, &o);
        epConfig = get_bits(b, 2);
        put_bits(&o, 2, epConfig);
    }

    // count the extradata
    ret = put_bits_count(&o);
    decoder->extrasize = (ret + 7) / 8;

    flush_put_bits(&o);
    return ret;
}

static void readStreamMuxConfig(struct AACDecoder *parser, GetBitContext *b)
{
    int audio_mux_version = get_bits(b, 1);
    parser->audio_mux_version_A = 0;
    if (audio_mux_version == 1) {                // audioMuxVersion
        parser->audio_mux_version_A = get_bits(b, 1);
    }

    if (parser->audio_mux_version_A == 0) {
        int frame_length_type;

        if (audio_mux_version == 1) {
            // taraFullness
            latm_get_value(b);
        }
        get_bits(b, 1);                    // allStreamSameTimeFraming = 1
        get_bits(b, 6);                    // numSubFrames = 0
        get_bits(b, 4);                    // numPrograms = 0

        // for each program (which there is only on in DVB)
        get_bits(b, 3);                    // numLayer = 0

        // for each layer (which there is only on in DVB)
        if (audio_mux_version == 0) {
            readAudioSpecificConfig(parser, b);
        } else {
            int ascLen = latm_get_value(b);
            ascLen -= readAudioSpecificConfig(parser, b);

            // skip left over bits
            while (ascLen > 16) {
                skip_bits(b, 16);
                ascLen -= 16;
            }
            skip_bits(b, ascLen);
        }

        // these are not needed... perhaps
        frame_length_type = get_bits(b, 3);
        parser->frameLengthType = frame_length_type;
        if (frame_length_type == 0) {
            get_bits(b, 8);
        } else if (frame_length_type == 1) {
            get_bits(b, 9);
        } else if (frame_length_type == 3 || frame_length_type == 4 || frame_length_type == 5) {
            // celp_table_index
            get_bits(b, 6);
        } else if (frame_length_type == 6 || frame_length_type == 7) {
            // hvxc_table_index
            get_bits(b, 1);
        }

        // other data
        if (get_bits(b, 1)) {
            // other data present
            if (audio_mux_version == 1) {
                // other_data_bits
                latm_get_value(b);
            } else {
                int esc, tmp;
                // other data bits
                int64_t other_data_bits = 0;
                do {
                    esc = get_bits(b, 1);
                    tmp = get_bits(b, 8);
                    other_data_bits = other_data_bits << 8 | tmp;
                } while (esc);
            }
        }

        // CRC if necessary
        if (get_bits(b, 1)) {
            // config_crc
            get_bits(b, 8);
        }
    } else {
        // TBD
    }
}

static int readPayloadLengthInfo(struct AACDecoder *parser, GetBitContext *b)
{
    if (parser->frameLengthType == 0) {
        uint8_t tmp;
        int muxSlotLengthBytes = 0;
        do {
            tmp = get_bits(b, 8);
            muxSlotLengthBytes += tmp;
        } while (tmp == 255);
        return muxSlotLengthBytes;
    } else {
        if (parser->frameLengthType == 3 ||
                parser->frameLengthType == 5 ||
                parser->frameLengthType == 7) {
            get_bits(b, 2);
        }
        return 0;
    }
}

static void readAudioMuxElement(struct AACDecoder *parser, GetBitContext *b, uint8_t *payload, int *payloadsize)
{
    uint8_t use_same_mux = get_bits(b, 1);
    if (!use_same_mux) {
        readStreamMuxConfig(parser, b);
    }
    if (parser->audio_mux_version_A == 0) {
        int j;
        int muxSlotLengthBytes = readPayloadLengthInfo(parser, b);
        muxSlotLengthBytes = min(muxSlotLengthBytes, *payloadsize);
        for (j=0; j<muxSlotLengthBytes; j++) {
            *payload++ = get_bits(b, 8);
        }
        *payloadsize = muxSlotLengthBytes;
    }
}

static int readAudioSyncStream(struct AACDecoder *parser, GetBitContext *b, int size, uint8_t *payload, int *payloadsize)
{
    int muxlength;

    if (get_bits(b, 11) != SYNC_LATM) return -1;    // not LATM

    muxlength = get_bits(b, 13);
    if (muxlength+3 > size) return -1;          // not enough data, the parser should have sorted this

    readAudioMuxElement(parser, b, payload, payloadsize);

    return 0;
}

static void channel_setup(AVCodecContext *avctx)
{
    AACDecoder *decoder = avctx->priv_data;
    
    if (avctx->request_channels == 2 && avctx->channels > 2) {
        NeAACDecConfigurationPtr faac_cfg;
        avctx->channels = 2;
        faac_cfg = NeAACDecGetCurrentConfiguration(decoder->aac_decoder);
        if (faac_cfg) {
            faac_cfg->downMatrix = 1;
            faac_cfg->defSampleRate = (!avctx->sample_rate) ? 44100 : avctx->sample_rate;
            NeAACDecSetConfiguration(decoder->aac_decoder, faac_cfg);
        }
    }
}

static int latm_decode_frame(AVCodecContext *avctx, void *out, int *out_size, AVPacket *avpkt)
{
    AACDecoder          *decoder = avctx->priv_data;
    uint8_t             tempbuf[MAX_SIZE];
    int                 bufsize = sizeof(tempbuf);
    int                 max_size = *out_size;
    NeAACDecFrameInfo   info;
    GetBitContext       b;
    
    init_get_bits(&b, avpkt->data, avpkt->size * 8);
    if (readAudioSyncStream(decoder, &b, avpkt->size, tempbuf, &bufsize)) {
        return -1;
    }

    if (!decoder->initialized) {
        // we are going to initialize from decoder specific info when available
        if (decoder->extrasize > 0) {
            if (NeAACDecInit2(decoder->aac_decoder, decoder->extra, decoder->extrasize, &avctx->sample_rate, &avctx->channels)) {
                return -1;
            }
            channel_setup(avctx);
            decoder->initialized = 1;
        } else {
            *out_size = 0;
            return avpkt->size;
        }
    }

    if (!NeAACDecDecode2(decoder->aac_decoder, &info, tempbuf, bufsize, &out, max_size)) {
        return -1;
    }
    *out_size = info.samples * sizeof(short);
    return avpkt->size;
}

static int latm_decode_init(AVCodecContext *avctx)
{
    AACDecoder *decoder = avctx->priv_data;
    NeAACDecConfigurationPtr faac_cfg;

    avctx->bit_rate = 0;
    avctx->sample_fmt = SAMPLE_FMT_S16;
    decoder->aac_decoder = NeAACDecOpen();
    if (!decoder->aac_decoder) {
        return -1;
    }

    faac_cfg = NeAACDecGetCurrentConfiguration(decoder->aac_decoder);
    if (faac_cfg) {
        faac_cfg->outputFormat = FAAD_FMT_16BIT;
        faac_cfg->defSampleRate = (!avctx->sample_rate) ? 44100 : avctx->sample_rate;
        faac_cfg->defObjectType = LC;
        NeAACDecSetConfiguration(decoder->aac_decoder, faac_cfg);
    }

    decoder->initialized = 0;
    return 0;
}

static int latm_decode_end(AVCodecContext *avctx)
{
    AACDecoder *decoder = avctx->priv_data;
    NeAACDecClose(decoder->aac_decoder);
    return 0;
}

AVCodec libfaad_latm_decoder = {
    .name = "AAC/LATM",
    .type = CODEC_TYPE_AUDIO,
    .id = CODEC_ID_AAC_LATM,
    .priv_data_size = sizeof (AACDecoder),
    .init = latm_decode_init,
    .close = latm_decode_end,
    .decode = latm_decode_frame,
    .long_name = "AAC over LATM",
};
