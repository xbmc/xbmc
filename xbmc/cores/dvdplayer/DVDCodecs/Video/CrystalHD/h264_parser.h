/*
 * Copyright (C) 2008 Julian Scheel
 *
 * This file is part of xine, a free video player.
 *
 * xine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * xine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 * h264_parser.h: Almost full-features H264 NAL-Parser
 */

#ifndef NAL_PARSER_H_
#define NAL_PARSER_H_

#include <stdlib.h>

#include "nal.h"
//#include "dpb.h"

#define MAX_FRAME_SIZE  1024*1024

/* specifies wether the parser last parsed
 * non-vcl or vcl nal units. depending on
 * this the access unit boundaries are detected
 */
enum parser_position {
    NON_VCL,
    VCL
};

enum parser_flags {
    CPB_DPB_DELAYS_PRESENT = 0x01,
    PIC_STRUCT_PRESENT = 0x02
};

struct h264_parser {
#ifdef NOVDPAU
    uint8_t privatebuf[MAX_FRAME_SIZE];
    uint32_t privatebuf_len;
#endif
    uint8_t buf[MAX_FRAME_SIZE];
    uint32_t buf_len;


    /* prebuf is used to store the currently
     * processed nal unit */
    uint8_t prebuf[MAX_FRAME_SIZE];
    uint32_t prebuf_len;
    uint32_t next_nal_position;

    uint8_t last_nal_res;

    uint8_t nal_size_length;
    uint32_t next_nal_size;
    uint8_t *nal_size_length_buf;
    uint8_t have_nal_size_length_buf;

    enum parser_position position;

    struct coded_picture *pic;

    struct nal_unit *last_vcl_nal;
    struct nal_buffer *sps_buffer;
    struct nal_buffer *pps_buffer;

    uint32_t prev_pic_order_cnt_lsb;
    uint32_t prev_pic_order_cnt_msb;
    uint32_t frame_num_offset;

    int32_t prev_top_field_order_cnt;

    uint32_t curr_pic_num;

    uint16_t flag_mask;
#if 0
    /* this is dpb used for reference frame
     * heading to vdpau + unordered frames
     */
    struct dpb dpb;
#endif
};

int parse_nal(uint8_t *buf, int buf_len, struct h264_parser *parser,
    struct coded_picture **completed_picture);

int seek_for_nal(uint8_t *buf, int buf_len, struct h264_parser *parser);

struct h264_parser* init_parser();
void free_parser(struct h264_parser *parser);
void reset_parser(struct h264_parser *parser);
int parse_frame(struct h264_parser *parser, uint8_t *inbuf, int inbuf_len,
    int64_t pts,
    uint8_t **ret_buf, uint32_t *ret_len, struct coded_picture **ret_pic);

/* this has to be called after decoding the frame delivered by parse_frame,
 * but before adding a decoded frame to the dpb.
 */
void process_mmc_operations(struct h264_parser *parser, struct coded_picture *picture);

int parse_codec_private(struct h264_parser *parser, uint8_t *inbuf, int inbuf_len);

#endif
