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
 * h264_parser.c: Almost full-features H264 NAL-Parser
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "h264_parser.h"
#include "nal.h"
#include "cpb.h"

#include "utils/fastmemcpy.h"

/* default scaling_lists according to Table 7-2 */
uint8_t default_4x4_intra[16] = { 6, 13, 13, 20, 20, 20, 28, 28, 28, 28, 32,
    32, 32, 37, 37, 42 };

uint8_t default_4x4_inter[16] = { 10, 14, 14, 20, 20, 20, 24, 24, 24, 24, 27,
    27, 27, 30, 30, 34 };

uint8_t default_8x8_intra[64] = { 6, 10, 10, 13, 11, 13, 16, 16, 16, 16, 18,
    18, 18, 18, 18, 23, 23, 23, 23, 23, 23, 25, 25, 25, 25, 25, 25, 25, 27, 27,
    27, 27, 27, 27, 27, 27, 29, 29, 29, 29, 29, 29, 29, 31, 31, 31, 31, 31, 31,
    33, 33, 33, 33, 33, 36, 36, 36, 36, 38, 38, 38, 40, 40, 42 };

uint8_t default_8x8_inter[64] = { 9, 13, 13, 15, 13, 15, 17, 17, 17, 17, 19,
    19, 19, 19, 19, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 24, 24,
    24, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 27, 27, 27, 27, 27, 27,
    28, 28, 28, 28, 28, 30, 30, 30, 30, 32, 32, 32, 33, 33, 35 };

struct buf_reader
{
  uint8_t *buf;
  uint8_t *cur_pos;
  int len;
  int cur_offset;
};

struct h264_parser* init_parser();

static inline uint32_t read_bits(struct buf_reader *buf, int len);
uint32_t read_exp_golomb(struct buf_reader *buf);
int32_t read_exp_golomb_s(struct buf_reader *buf);

void calculate_pic_order(struct h264_parser *parser, struct coded_picture *pic,
    struct slice_header *slc);
void skip_scaling_list(struct buf_reader *buf, int size);
void parse_scaling_list(struct buf_reader *buf, uint8_t *scaling_list,
    int length, int index);

struct nal_unit* parse_nal_header(struct buf_reader *buf,
    struct coded_picture *pic, struct h264_parser *parser);
static void sps_scaling_list_fallback(struct seq_parameter_set_rbsp *sps,
    int i);
static void pps_scaling_list_fallback(struct seq_parameter_set_rbsp *sps,
    struct pic_parameter_set_rbsp *pps, int i);

uint8_t parse_sps(struct buf_reader *buf, struct seq_parameter_set_rbsp *sps);
void interpret_sps(struct coded_picture *pic, struct h264_parser *parser);

void parse_vui_parameters(struct buf_reader *buf,
    struct seq_parameter_set_rbsp *sps);
void parse_hrd_parameters(struct buf_reader *buf, struct hrd_parameters *hrd);

uint8_t parse_pps(struct buf_reader *buf, struct pic_parameter_set_rbsp *pps);
void interpret_pps(struct coded_picture *pic);

void parse_sei(struct buf_reader *buf, struct sei_message *sei,
    struct h264_parser *parser);
void interpret_sei(struct coded_picture *pic);

uint8_t parse_slice_header(struct buf_reader *buf, struct nal_unit *slc_nal,
    struct h264_parser *parser);
void interpret_slice_header(struct h264_parser *parser, struct nal_unit *slc_nal);

void parse_ref_pic_list_reordering(struct buf_reader *buf,
    struct slice_header *slc);

void calculate_pic_nums(struct h264_parser *parser, struct coded_picture *cpic);
void execute_ref_pic_marking(struct coded_picture *cpic,
    uint32_t memory_management_control_operation,
    uint32_t marking_nr,
    struct h264_parser *parser);
void parse_pred_weight_table(struct buf_reader *buf, struct slice_header *slc,
    struct h264_parser *parser);
void parse_dec_ref_pic_marking(struct buf_reader *buf,
    struct nal_unit *slc_nal);

/* here goes the parser implementation */

static void decode_nal(uint8_t **ret, int *len_ret, uint8_t *buf, int buf_len)
{
  // TODO: rework without copying
  uint8_t *end = &buf[buf_len];
  uint8_t *pos = malloc(buf_len);

  *ret = pos;
  while (buf < end) {
    if (buf < end - 3 && buf[0] == 0x00 && buf[1] == 0x00 && buf[2] == 0x03) {

      *pos++ = 0x00;
      *pos++ = 0x00;

      buf += 3;
      continue;
    }
    *pos++ = *buf++;
  }

  *len_ret = pos - *ret;
}

#if 0
static inline void dump_bits(const char *label, const struct buf_reader *buf, int bits)
{
  struct buf_reader lbuf;
  memcpy(&lbuf, buf, sizeof(struct buf_reader));

  int i;
  printf("%s: 0b", label);
  for(i=0; i < bits; i++)
    printf("%d", read_bits(&lbuf, 1));
  printf("\n");
}
#endif

/**
 * @return total number of bits read by the buf_reader
 */
static inline uint32_t bits_read(struct buf_reader *buf)
{
  int bits_read = 0;
  bits_read = (buf->cur_pos - buf->buf)*8;
  bits_read += (8-buf->cur_offset);

  return bits_read;
}

/*
 * read len bits from the buffer and return them
 * @return right aligned bits
 */
static inline uint32_t read_bits(struct buf_reader *buf, int len)
{
  static uint32_t i_mask[33] = { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f,
      0x7f, 0xff, 0x1ff, 0x3ff, 0x7ff, 0xfff, 0x1fff, 0x3fff, 0x7fff, 0xffff,
      0x1ffff, 0x3ffff, 0x7ffff, 0xfffff, 0x1fffff, 0x3fffff, 0x7fffff,
      0xffffff, 0x1ffffff, 0x3ffffff, 0x7ffffff, 0xfffffff, 0x1fffffff,
      0x3fffffff, 0x7fffffff, 0xffffffff };

  int i_shr;
  uint32_t bits = 0;

  while (len > 0 && (buf->cur_pos - buf->buf) < buf->len) {
    if ((i_shr = buf->cur_offset - len) >= 0) {
      bits |= (*buf->cur_pos >> i_shr) & i_mask[len];
      buf->cur_offset -= len;
      if (buf->cur_offset == 0) {
        buf->cur_pos++;
        buf->cur_offset = 8;
      }
      return bits;
    }
    else {
      bits |= (*buf->cur_pos & i_mask[buf->cur_offset]) << -i_shr;
      len -= buf->cur_offset;
      buf->cur_pos++;
      buf->cur_offset = 8;
    }
  }
  return bits;
}

/* determines if following bits are rtsb_trailing_bits */
static inline int rbsp_trailing_bits(uint8_t *buf, int buf_len)
{
  uint8_t *cur_buf = buf+(buf_len-1);
  uint8_t cur_val;
  int parsed_bits = 0;
  int i;

  while(buf_len > 0) {
    cur_val = *cur_buf;
    for(i = 0; i < 9; i++) {
      if (cur_val&1)
        return parsed_bits+i;
      cur_val>>=1;
    }
    parsed_bits += 8;
    cur_buf--;
  }

  fprintf(stderr, "rbsp trailing bits could not be found\n");
  return 0;
}

uint32_t read_exp_golomb(struct buf_reader *buf)
{
  int leading_zero_bits = 0;

  while (read_bits(buf, 1) == 0 && leading_zero_bits < 32)
    leading_zero_bits++;

  uint32_t code = (1 << leading_zero_bits) - 1 + read_bits(buf,
      leading_zero_bits);
  return code;
}

int32_t read_exp_golomb_s(struct buf_reader *buf)
{
  uint32_t ue = read_exp_golomb(buf);
  int32_t code = ue & 0x01 ? (ue + 1) / 2 : -(ue / 2);
  return code;
}


/**
 * parses the NAL header data and calls the subsequent
 * parser methods that handle specific NAL units
 */
struct nal_unit* parse_nal_header(struct buf_reader *buf,
    struct coded_picture *pic, struct h264_parser *parser)
{
  if (buf->len < 1)
    return NULL;


  struct nal_unit *nal = create_nal_unit();

  nal->nal_ref_idc = (buf->buf[0] >> 5) & 0x03;
  nal->nal_unit_type = buf->buf[0] & 0x1f;

  buf->cur_pos = buf->buf + 1;
  //printf("NAL: %d\n", nal->nal_unit_type);

  struct buf_reader ibuf;
  ibuf.cur_offset = 8;

  switch (nal->nal_unit_type) {
    case NAL_SPS:
      decode_nal(&ibuf.buf, &ibuf.len, buf->cur_pos, buf->len - 1);
      ibuf.cur_pos = ibuf.buf;

      parse_sps(&ibuf, &nal->sps);
      free(ibuf.buf);
      break;
    case NAL_PPS:
      parse_pps(buf, &nal->pps);
      break;
    case NAL_SLICE:
    case NAL_PART_A:
    case NAL_PART_B:
    case NAL_PART_C:
    case NAL_SLICE_IDR:
      parse_slice_header(buf, nal, parser);
      break;
    case NAL_SEI:
      memset(&(nal->sei), 0x00, sizeof(struct sei_message));
      parse_sei(buf, &nal->sei, parser);
      break;
    default:
      break;
  }

  return nal;
}

/**
 * calculates the picture order count according to ITU-T Rec. H.264 (11/2007)
 * chapter 8.2.1, p104f
 */
void calculate_pic_order(struct h264_parser *parser, struct coded_picture *pic,
    struct slice_header *slc)
{
  /* retrieve sps and pps from the buffers */
  struct nal_unit *pps_nal =
      nal_buffer_get_by_pps_id(parser->pps_buffer, slc->pic_parameter_set_id);

  if (pps_nal == NULL) {
    fprintf(stderr, "ERR: calculate_pic_order: pic_parameter_set_id %d not found in buffers\n",
            slc->pic_parameter_set_id);
        return;
  }

  struct pic_parameter_set_rbsp *pps = &pps_nal->pps;

  struct nal_unit *sps_nal =
      nal_buffer_get_by_sps_id(parser->sps_buffer, pps->seq_parameter_set_id);

  if (sps_nal == NULL) {
    fprintf(stderr, "ERR: calculate_pic_order: seq_parameter_set_id %d not found in buffers\n",
        pps->seq_parameter_set_id);
    return;
  }

  struct seq_parameter_set_rbsp *sps = &sps_nal->sps;

  if (sps->pic_order_cnt_type == 0) {

    if (pic->flag_mask & IDR_PIC) {
      parser->prev_pic_order_cnt_lsb = 0;
      parser->prev_pic_order_cnt_msb = 0;


      // FIXME
      parser->frame_num_offset = 0;
    }

    const int max_poc_lsb = 1 << (sps->log2_max_pic_order_cnt_lsb_minus4 + 4);

    uint32_t pic_order_cnt_msb = 0;

    if (slc->pic_order_cnt_lsb < parser->prev_pic_order_cnt_lsb
        && parser->prev_pic_order_cnt_lsb - slc->pic_order_cnt_lsb
            >= max_poc_lsb / 2)
      pic_order_cnt_msb = parser->prev_pic_order_cnt_msb + max_poc_lsb;
    else if (slc->pic_order_cnt_lsb > parser->prev_pic_order_cnt_lsb
        && parser->prev_pic_order_cnt_lsb - slc->pic_order_cnt_lsb
            < -max_poc_lsb / 2)
      pic_order_cnt_msb = parser->prev_pic_order_cnt_msb - max_poc_lsb;
    else
      pic_order_cnt_msb = parser->prev_pic_order_cnt_msb;

    if(!slc->field_pic_flag || !slc->bottom_field_flag) {
      pic->top_field_order_cnt = pic_order_cnt_msb + slc->pic_order_cnt_lsb;
      parser->prev_top_field_order_cnt = pic->top_field_order_cnt;
    }

    if (pic->flag_mask & REFERENCE) {
      parser->prev_pic_order_cnt_msb =  pic_order_cnt_msb;
    }

    pic->bottom_field_order_cnt = 0;

    if(!slc->field_pic_flag)
      pic->bottom_field_order_cnt = pic->top_field_order_cnt + slc->delta_pic_order_cnt_bottom;
    else //if(slc->bottom_field_flag) //TODO: this is not spec compliant, but works...
      pic->bottom_field_order_cnt = pic_order_cnt_msb + slc->pic_order_cnt_lsb;

    if(slc->field_pic_flag && slc->bottom_field_flag)
      pic->top_field_order_cnt = parser->prev_top_field_order_cnt;

  } else if (sps->pic_order_cnt_type == 2) {
    uint32_t prev_frame_num = parser->last_vcl_nal->slc.frame_num;
    uint32_t prev_frame_num_offset = parser->frame_num_offset;
    uint32_t temp_pic_order_cnt = 0;

    if (parser->pic->flag_mask & IDR_PIC)
      parser->frame_num_offset = 0;
    else if (prev_frame_num > slc->frame_num)
      parser->frame_num_offset = prev_frame_num_offset + sps->max_frame_num;
    else
      parser->frame_num_offset = prev_frame_num_offset;

    if(parser->pic->flag_mask & IDR_PIC)
      temp_pic_order_cnt = 0;
    else if(!(parser->pic->flag_mask & REFERENCE))
      temp_pic_order_cnt = 2 * (parser->frame_num_offset + slc->frame_num)-1;
    else
      temp_pic_order_cnt = 2 * (parser->frame_num_offset + slc->frame_num);

    if(!slc->field_pic_flag)
      pic->top_field_order_cnt = pic->bottom_field_order_cnt = temp_pic_order_cnt;
    else if(slc->bottom_field_flag)
      pic->bottom_field_order_cnt = temp_pic_order_cnt;
    else
      pic->top_field_order_cnt = temp_pic_order_cnt;

  } else {
    fprintf(stderr, "FIXME: Unsupported poc_type: %d\n", sps->pic_order_cnt_type);
  }
}

void skip_scaling_list(struct buf_reader *buf, int size)
{
  int i;
  for (i = 0; i < size; i++) {
    read_exp_golomb_s(buf);
  }
}

void parse_scaling_list(struct buf_reader *buf, uint8_t *scaling_list,
    int length, int index)
{
  int last_scale = 8;
  int next_scale = 8;
  int32_t delta_scale;
  uint8_t use_default_scaling_matrix_flag = 0;
  int i;

  const uint8_t *zigzag = (length==64) ? zigzag_8x8 : zigzag_4x4;

  for (i = 0; i < length; i++) {
    if (next_scale != 0) {
      delta_scale = read_exp_golomb_s(buf);
      next_scale = (last_scale + delta_scale + 256) % 256;
      if (i == 0 && next_scale == 0) {
        use_default_scaling_matrix_flag = 1;
        break;
      }
    }
    scaling_list[zigzag[i]] = last_scale = (next_scale == 0) ? last_scale : next_scale;
  }

  if (use_default_scaling_matrix_flag) {
    switch (index) {
      case 0:
      case 1:
      case 2: {
        for(i = 0; i < sizeof(default_4x4_intra); i++) {
          scaling_list[zigzag_4x4[i]] = default_4x4_intra[i];
        }
        //memcpy(scaling_list, default_4x4_intra, sizeof(default_4x4_intra));
        break;
      }
      case 3:
      case 4:
      case 5: {
        for(i = 0; i < sizeof(default_4x4_inter); i++) {
          scaling_list[zigzag_4x4[i]] = default_4x4_inter[i];
        }
        //memcpy(scaling_list, default_4x4_inter, sizeof(default_4x4_inter));
        break;
      }
      case 6: {
        for(i = 0; i < sizeof(default_8x8_intra); i++) {
          scaling_list[zigzag_8x8[i]] = default_8x8_intra[i];
        }
        //memcpy(scaling_list, default_8x8_intra, sizeof(default_8x8_intra));
        break;
      }
      case 7: {
        for(i = 0; i < sizeof(default_8x8_inter); i++) {
          scaling_list[zigzag_8x8[i]] = default_8x8_inter[i];
        }
        //memcpy(scaling_list, default_8x8_inter, sizeof(default_8x8_inter));
        break;
      }
    }
  }
}

static void sps_scaling_list_fallback(struct seq_parameter_set_rbsp *sps, int i)
{
  int j;
  switch (i) {
    case 0: {
      for(j = 0; j < sizeof(default_4x4_intra); j++) {
        sps->scaling_lists_4x4[i][zigzag_4x4[j]] = default_4x4_intra[j];
      }
      //memcpy(sps->scaling_lists_4x4[i], default_4x4_intra, sizeof(sps->scaling_lists_4x4[i]));
      break;
    }
    case 3: {
      for(j = 0; j < sizeof(default_4x4_inter); j++) {
        sps->scaling_lists_4x4[i][zigzag_4x4[j]] = default_4x4_inter[j];
      }
      //memcpy(sps->scaling_lists_4x4[i], default_4x4_inter, sizeof(sps->scaling_lists_4x4[i]));
      break;
    }
    case 1:
    case 2:
    case 4:
    case 5:
      memcpy(sps->scaling_lists_4x4[i], sps->scaling_lists_4x4[i-1], sizeof(sps->scaling_lists_4x4[i]));
      break;
    case 6: {
      for(j = 0; j < sizeof(default_8x8_intra); j++) {
        sps->scaling_lists_8x8[i-6][zigzag_8x8[j]] = default_8x8_intra[j];
      }
      //memcpy(sps->scaling_lists_8x8[i-6], default_8x8_intra, sizeof(sps->scaling_lists_8x8[i-6]));
      break;
    }
    case 7: {
      for(j = 0; j < sizeof(default_8x8_inter); j++) {
        sps->scaling_lists_8x8[i-6][zigzag_8x8[j]] = default_8x8_inter[j];
      }
      //memcpy(sps->scaling_lists_8x8[i-6], default_8x8_inter, sizeof(sps->scaling_lists_8x8[i-6]));
      break;
    }

  }
}

static void pps_scaling_list_fallback(struct seq_parameter_set_rbsp *sps, struct pic_parameter_set_rbsp *pps, int i)
{
  switch (i) {
    case 0:
    case 3:
      memcpy(pps->scaling_lists_4x4[i], sps->scaling_lists_4x4[i], sizeof(pps->scaling_lists_4x4[i]));
      break;
    case 1:
    case 2:
    case 4:
    case 5:
      memcpy(pps->scaling_lists_4x4[i], pps->scaling_lists_4x4[i-1], sizeof(pps->scaling_lists_4x4[i]));
      break;
    case 6:
    case 7:
      memcpy(pps->scaling_lists_8x8[i-6], sps->scaling_lists_8x8[i-6], sizeof(pps->scaling_lists_8x8[i-6]));
      break;

  }
}


uint8_t parse_sps(struct buf_reader *buf, struct seq_parameter_set_rbsp *sps)
{
  sps->profile_idc = read_bits(buf, 8);
  sps->constraint_setN_flag = read_bits(buf, 4);
  read_bits(buf, 4);
  sps->level_idc = read_bits(buf, 8);

  sps->seq_parameter_set_id = read_exp_golomb(buf);

  memset(sps->scaling_lists_4x4, 16, sizeof(sps->scaling_lists_4x4));
  memset(sps->scaling_lists_8x8, 16, sizeof(sps->scaling_lists_8x8));
  if (sps->profile_idc == 100 || sps->profile_idc == 110 || sps->profile_idc
      == 122 || sps->profile_idc == 244 || sps->profile_idc == 44 ||
      sps->profile_idc == 83 || sps->profile_idc == 86) {
    sps->chroma_format_idc = read_exp_golomb(buf);
    if (sps->chroma_format_idc == 3) {
      sps->separate_colour_plane_flag = read_bits(buf, 1);
    }

    sps->bit_depth_luma_minus8 = read_exp_golomb(buf);
    sps->bit_depth_chroma_minus8 = read_exp_golomb(buf);
    sps->qpprime_y_zero_transform_bypass_flag = read_bits(buf, 1);
    sps->seq_scaling_matrix_present_flag = read_bits(buf, 1);
    if (sps->seq_scaling_matrix_present_flag) {
      int i;
      for (i = 0; i < 8; i++) {
        sps->seq_scaling_list_present_flag[i] = read_bits(buf, 1);

        if (sps->seq_scaling_list_present_flag[i]) {
          if (i < 6)
            parse_scaling_list(buf, sps->scaling_lists_4x4[i], 16, i);
          else
            parse_scaling_list(buf, sps->scaling_lists_8x8[i - 6], 64, i);
        } else {
          sps_scaling_list_fallback(sps, i);
        }
      }
    }
  } else
    sps->chroma_format_idc = 1;

  sps->log2_max_frame_num_minus4 = read_exp_golomb(buf);
  sps->max_frame_num = 1 << (sps->log2_max_frame_num_minus4 + 4);

  sps->pic_order_cnt_type = read_exp_golomb(buf);
  if (!sps->pic_order_cnt_type)
    sps->log2_max_pic_order_cnt_lsb_minus4 = read_exp_golomb(buf);
  else if(sps->pic_order_cnt_type == 1) {
    sps->delta_pic_order_always_zero_flag = read_bits(buf, 1);
    sps->offset_for_non_ref_pic = read_exp_golomb_s(buf);
    sps->offset_for_top_to_bottom_field = read_exp_golomb_s(buf);
    sps->num_ref_frames_in_pic_order_cnt_cycle = read_exp_golomb(buf);
    int i;
    for (i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; i++) {
      sps->offset_for_ref_frame[i] = read_exp_golomb_s(buf);
    }
  }

  sps->num_ref_frames = read_exp_golomb(buf);
  sps->gaps_in_frame_num_value_allowed_flag = read_bits(buf, 1);

  /*sps->pic_width_in_mbs_minus1 = read_exp_golomb(buf);
   sps->pic_height_in_map_units_minus1 = read_exp_golomb(buf);*/
  sps->pic_width = 16 * (read_exp_golomb(buf) + 1);
  sps->pic_height = 16 * (read_exp_golomb(buf) + 1);

  sps->frame_mbs_only_flag = read_bits(buf, 1);

  /* compute the height correctly even for interlaced material */
  sps->pic_height = (2 - sps->frame_mbs_only_flag) * sps->pic_height;
  if (sps->pic_height == 1088)
    sps->pic_height = 1080;

  if (!sps->frame_mbs_only_flag)
    sps->mb_adaptive_frame_field_flag = read_bits(buf, 1);

  sps->direct_8x8_inference_flag = read_bits(buf, 1);
  sps->frame_cropping_flag = read_bits(buf, 1);
  if (sps->frame_cropping_flag) {
    sps->frame_crop_left_offset = read_exp_golomb(buf);
    sps->frame_crop_right_offset = read_exp_golomb(buf);
    sps->frame_crop_top_offset = read_exp_golomb(buf);
    sps->frame_crop_bottom_offset = read_exp_golomb(buf);
  }
  sps->vui_parameters_present_flag = read_bits(buf, 1);
  if (sps->vui_parameters_present_flag) {
    parse_vui_parameters(buf, sps);
  }

  return 0;
}

/* evaluates values parsed by sps and modifies the current
 * picture according to them
 */
void interpret_sps(struct coded_picture *pic, struct h264_parser *parser)
{
  if(pic->sps_nal == NULL) {
    fprintf(stderr, "WARNING: Picture contains no seq_parameter_set\n");
    return;
  }

  struct seq_parameter_set_rbsp *sps = &pic->sps_nal->sps;

  if(sps->vui_parameters_present_flag &&
        sps->vui_parameters.pic_struct_present_flag) {
    parser->flag_mask |= PIC_STRUCT_PRESENT;
  } else {
    parser->flag_mask &= ~PIC_STRUCT_PRESENT;
  }

  if(sps->vui_parameters_present_flag &&
      (sps->vui_parameters.nal_hrd_parameters_present_flag ||
       sps->vui_parameters.vc1_hrd_parameters_present_flag)) {
    parser->flag_mask |= CPB_DPB_DELAYS_PRESENT;
  } else {
    parser->flag_mask &= ~(CPB_DPB_DELAYS_PRESENT);
  }

  if(pic->slc_nal != NULL) {
    struct slice_header *slc = &pic->slc_nal->slc;
    if (slc->field_pic_flag == 0) {
      pic->max_pic_num = sps->max_frame_num;
      parser->curr_pic_num = slc->frame_num;
    } else {
      pic->max_pic_num = 2 * sps->max_frame_num;
      parser->curr_pic_num = 2 * slc->frame_num + 1;
    }
  }
}

void parse_sei(struct buf_reader *buf, struct sei_message *sei,
    struct h264_parser *parser)
{
  uint8_t tmp;

  struct nal_unit *sps_nal =
      nal_buffer_get_last(parser->sps_buffer);

  if (sps_nal == NULL) {
    fprintf(stderr, "ERR: parse_sei: seq_parameter_set_id not found in buffers\n");
    return;
  }

  struct seq_parameter_set_rbsp *sps = &sps_nal->sps;

  sei->payload_type = 0;
  while((tmp = read_bits(buf, 8)) == 0xff) {
    sei->payload_type += 255;
  }
  sei->last_payload_type_byte = tmp;
  sei->payload_type += sei->last_payload_type_byte;

  sei->payload_size = 0;
  while((tmp = read_bits(buf, 8)) == 0xff) {
    sei->payload_size += 255;
  }
  sei->last_payload_size_byte = tmp;
  sei->payload_size += sei->last_payload_size_byte;

  /* pic_timing */
  if(sei->payload_type == 1) {
    if(parser->flag_mask & CPB_DPB_DELAYS_PRESENT) {
      sei->pic_timing.cpb_removal_delay = read_bits(buf, 5);
      sei->pic_timing.dpb_output_delay = read_bits(buf, 5);
    }

    if(parser->flag_mask & PIC_STRUCT_PRESENT) {
      sei->pic_timing.pic_struct = read_bits(buf, 4);

      uint8_t NumClockTs = 0;
      switch(sei->pic_timing.pic_struct) {
        case 0:
        case 1:
        case 2:
          NumClockTs = 1;
          break;
        case 3:
        case 4:
        case 7:
          NumClockTs = 2;
          break;
        case 5:
        case 6:
        case 8:
          NumClockTs = 3;
          break;
      }

      int i;
      for(i = 0; i < NumClockTs; i++) {
        if(read_bits(buf, 1)) { /* clock_timestamp_flag == 1 */
          //printf("parse clock timestamp!\n");
          sei->pic_timing.ct_type = read_bits(buf, 2);
          sei->pic_timing.nuit_field_based_flag = read_bits(buf, 1);
          sei->pic_timing.counting_type = read_bits(buf, 5);
          sei->pic_timing.full_timestamp_flag = read_bits(buf, 1);
          sei->pic_timing.discontinuity_flag = read_bits(buf, 1);
          sei->pic_timing.cnt_dropped_flag = read_bits(buf, 1);
          sei->pic_timing.n_frames = read_bits(buf, 8);
          if(sei->pic_timing.full_timestamp_flag) {
            sei->pic_timing.seconds_value = read_bits(buf, 6);
            sei->pic_timing.minutes_value = read_bits(buf, 6);
            sei->pic_timing.hours_value = read_bits(buf, 5);
          } else {
            if(read_bits(buf, 1)) {
              sei->pic_timing.seconds_value = read_bits(buf, 6);

              if(read_bits(buf, 1)) {
                sei->pic_timing.minutes_value = read_bits(buf, 6);

                if(read_bits(buf, 1)) {
                  sei->pic_timing.hours_value = read_bits(buf, 5);
                }
              }
            }
          }

          if(sps->vui_parameters_present_flag &&
              sps->vui_parameters.nal_hrd_parameters_present_flag) {
            sei->pic_timing.time_offset =
                read_bits(buf,
                    sps->vui_parameters.nal_hrd_parameters.time_offset_length);
          }
        }
      }
    }
  } /*else {
    fprintf(stderr, "Unimplemented SEI payload: %d\n", sei->payload_type);
  }*/

}

void interpret_sei(struct coded_picture *pic)
{
  if(!pic->sps_nal || !pic->sei_nal)
    return;

  struct seq_parameter_set_rbsp *sps = &pic->sps_nal->sps;
  struct sei_message *sei = &pic->sei_nal->sei;

  if(sps && sps->vui_parameters_present_flag &&
      sps->vui_parameters.pic_struct_present_flag) {
    switch(sei->pic_timing.pic_struct) {
      case DISP_FRAME:
        pic->flag_mask &= ~INTERLACED;
        pic->repeat_pic = 0;
        break;
      case DISP_TOP:
      case DISP_BOTTOM:
      case DISP_TOP_BOTTOM:
      case DISP_BOTTOM_TOP:
        pic->flag_mask |= INTERLACED;
        break;
      case DISP_TOP_BOTTOM_TOP:
      case DISP_BOTTOM_TOP_BOTTOM:
        pic->flag_mask |= INTERLACED;
        pic->repeat_pic = 1;
        break;
      case DISP_FRAME_DOUBLING:
        pic->flag_mask &= ~INTERLACED;
        pic->repeat_pic = 2;
        break;
      case DISP_FRAME_TRIPLING:
        pic->flag_mask &= ~INTERLACED;
        pic->repeat_pic = 3;
    }
  }
}

void parse_vui_parameters(struct buf_reader *buf,
    struct seq_parameter_set_rbsp *sps)
{
  sps->vui_parameters.aspect_ration_info_present_flag = read_bits(buf, 1);
  if (sps->vui_parameters.aspect_ration_info_present_flag == 1) {
    sps->vui_parameters.aspect_ratio_idc = read_bits(buf, 8);
    if (sps->vui_parameters.aspect_ratio_idc == ASPECT_EXTENDED_SAR) {
      sps->vui_parameters.sar_width = read_bits(buf, 16);
      sps->vui_parameters.sar_height = read_bits(buf, 16);
    }
  }

  sps->vui_parameters.overscan_info_present_flag = read_bits(buf, 1);
  if (sps->vui_parameters.overscan_info_present_flag) {
    sps->vui_parameters.overscan_appropriate_flag = read_bits(buf, 1);
  }

  sps->vui_parameters.video_signal_type_present_flag = read_bits(buf, 1);
  if (sps->vui_parameters.video_signal_type_present_flag) {
    sps->vui_parameters.video_format = read_bits(buf, 3);
    sps->vui_parameters.video_full_range_flag = read_bits(buf, 1);
    sps->vui_parameters.colour_description_present = read_bits(buf, 1);
    if (sps->vui_parameters.colour_description_present) {
      sps->vui_parameters.colour_primaries = read_bits(buf, 8);
      sps->vui_parameters.transfer_characteristics = read_bits(buf, 8);
      sps->vui_parameters.matrix_coefficients = read_bits(buf, 8);
    }
  }

  sps->vui_parameters.chroma_loc_info_present_flag = read_bits(buf, 1);
  if (sps->vui_parameters.chroma_loc_info_present_flag) {
    sps->vui_parameters.chroma_sample_loc_type_top_field = read_exp_golomb(buf);
    sps->vui_parameters.chroma_sample_loc_type_bottom_field = read_exp_golomb(
        buf);
  }

  sps->vui_parameters.timing_info_present_flag = read_bits(buf, 1);
  if (sps->vui_parameters.timing_info_present_flag) {
    uint32_t num_units_in_tick = read_bits(buf, 32);
    uint32_t time_scale = read_bits(buf, 32);
    sps->vui_parameters.num_units_in_tick = num_units_in_tick;
    sps->vui_parameters.time_scale = time_scale;
    sps->vui_parameters.fixed_frame_rate_flag = read_bits(buf, 1);
  }

  sps->vui_parameters.nal_hrd_parameters_present_flag = read_bits(buf, 1);
  if (sps->vui_parameters.nal_hrd_parameters_present_flag)
    parse_hrd_parameters(buf, &sps->vui_parameters.nal_hrd_parameters);

  sps->vui_parameters.vc1_hrd_parameters_present_flag = read_bits(buf, 1);
  if (sps->vui_parameters.vc1_hrd_parameters_present_flag)
    parse_hrd_parameters(buf, &sps->vui_parameters.vc1_hrd_parameters);

  if (sps->vui_parameters.nal_hrd_parameters_present_flag
      || sps->vui_parameters.vc1_hrd_parameters_present_flag)
    sps->vui_parameters.low_delay_hrd_flag = read_bits(buf, 1);

  sps->vui_parameters.pic_struct_present_flag = read_bits(buf, 1);
  sps->vui_parameters.bitstream_restriction_flag = read_bits(buf, 1);

  if (sps->vui_parameters.bitstream_restriction_flag) {
    sps->vui_parameters.motion_vectors_over_pic_boundaries = read_bits(buf, 1);
    sps->vui_parameters.max_bytes_per_pic_denom = read_exp_golomb(buf);
    sps->vui_parameters.max_bits_per_mb_denom = read_exp_golomb(buf);
    sps->vui_parameters.log2_max_mv_length_horizontal = read_exp_golomb(buf);
    sps->vui_parameters.log2_max_mv_length_vertical = read_exp_golomb(buf);
    sps->vui_parameters.num_reorder_frames = read_exp_golomb(buf);
    sps->vui_parameters.max_dec_frame_buffering = read_exp_golomb(buf);
  }
}

void parse_hrd_parameters(struct buf_reader *buf, struct hrd_parameters *hrd)
{
  hrd->cpb_cnt_minus1 = read_exp_golomb(buf);
  hrd->bit_rate_scale = read_bits(buf, 4);
  hrd->cpb_size_scale = read_bits(buf, 4);

  int i;
  for (i = 0; i <= hrd->cpb_cnt_minus1; i++) {
    hrd->bit_rate_value_minus1[i] = read_exp_golomb(buf);
    hrd->cpb_size_value_minus1[i] = read_exp_golomb(buf);
    hrd->cbr_flag[i] = read_bits(buf, 1);
  }

  hrd->initial_cpb_removal_delay_length_minus1 = read_bits(buf, 5);
  hrd->cpb_removal_delay_length_minus1 = read_bits(buf, 5);
  hrd->dpb_output_delay_length_minus1 = read_bits(buf, 5);
  hrd->time_offset_length = read_bits(buf, 5);
}

uint8_t parse_pps(struct buf_reader *buf, struct pic_parameter_set_rbsp *pps)
{
  pps->pic_parameter_set_id = read_exp_golomb(buf);
  pps->seq_parameter_set_id = read_exp_golomb(buf);
  pps->entropy_coding_mode_flag = read_bits(buf, 1);
  pps->pic_order_present_flag = read_bits(buf, 1);

  pps->num_slice_groups_minus1 = read_exp_golomb(buf);
  if (pps->num_slice_groups_minus1 > 0) {
    pps->slice_group_map_type = read_exp_golomb(buf);
    if (pps->slice_group_map_type == 0) {
      int i_group;
      for (i_group = 0; i_group <= pps->num_slice_groups_minus1; i_group++) {
        if (i_group < 64)
          pps->run_length_minus1[i_group] = read_exp_golomb(buf);
        else { // FIXME: skips if more than 64 groups exist
          fprintf(stderr, "Error: Only 64 slice_groups are supported\n");
          read_exp_golomb(buf);
        }
      }
    }
    else if (pps->slice_group_map_type == 3 || pps->slice_group_map_type == 4
        || pps->slice_group_map_type == 5) {
      pps->slice_group_change_direction_flag = read_bits(buf, 1);
      pps->slice_group_change_rate_minus1 = read_exp_golomb(buf);
    }
    else if (pps->slice_group_map_type == 6) {
      pps->pic_size_in_map_units_minus1 = read_exp_golomb(buf);
      int i_group;
      for (i_group = 0; i_group <= pps->num_slice_groups_minus1; i_group++) {
        pps->slice_group_id[i_group] = read_bits(buf, ceil(log(
            pps->num_slice_groups_minus1 + 1)));
      }
    }
  }

  pps->num_ref_idx_l0_active_minus1 = read_exp_golomb(buf);
  pps->num_ref_idx_l1_active_minus1 = read_exp_golomb(buf);
  pps->weighted_pred_flag = read_bits(buf, 1);
  pps->weighted_bipred_idc = read_bits(buf, 2);
  pps->pic_init_qp_minus26 = read_exp_golomb_s(buf);
  pps->pic_init_qs_minus26 = read_exp_golomb_s(buf);
  pps->chroma_qp_index_offset = read_exp_golomb_s(buf);
  pps->deblocking_filter_control_present_flag = read_bits(buf, 1);
  pps->constrained_intra_pred_flag = read_bits(buf, 1);
  pps->redundant_pic_cnt_present_flag = read_bits(buf, 1);

  int bit_length = (buf->len*8)-rbsp_trailing_bits(buf->buf, buf->len);
  int bit_read = bits_read(buf);

  memset(pps->scaling_lists_4x4, 16, sizeof(pps->scaling_lists_4x4));
  memset(pps->scaling_lists_8x8, 16, sizeof(pps->scaling_lists_8x8));
  if (bit_length-bit_read > 1) {
    pps->transform_8x8_mode_flag = read_bits(buf, 1);
    pps->pic_scaling_matrix_present_flag = read_bits(buf, 1);
    if (pps->pic_scaling_matrix_present_flag) {
      int i;
      for (i = 0; i < 8; i++) {
        if(i < 6 || pps->transform_8x8_mode_flag)
          pps->pic_scaling_list_present_flag[i] = read_bits(buf, 1);
        else
          pps->pic_scaling_list_present_flag[i] = 0;

        if (pps->pic_scaling_list_present_flag[i]) {
          if (i < 6)
            parse_scaling_list(buf, pps->scaling_lists_4x4[i], 16, i);
          else
            parse_scaling_list(buf, pps->scaling_lists_8x8[i - 6], 64, i);
        }
      }
    }

    pps->second_chroma_qp_index_offset = read_exp_golomb_s(buf);
  } else
    pps->second_chroma_qp_index_offset = pps->chroma_qp_index_offset;

  return 0;
}

void interpret_pps(struct coded_picture *pic)
{
  if(pic->sps_nal == NULL) {
    fprintf(stderr, "WARNING: Picture contains no seq_parameter_set\n");
    return;
  } else if(pic->pps_nal == NULL) {
    fprintf(stderr, "WARNING: Picture contains no pic_parameter_set\n");
    return;
  }

  struct seq_parameter_set_rbsp *sps = &pic->sps_nal->sps;
  struct pic_parameter_set_rbsp *pps = &pic->pps_nal->pps;

  int i;
  for (i = 0; i < 8; i++) {
    if (!pps->pic_scaling_list_present_flag[i]) {
      pps_scaling_list_fallback(sps, pps, i);
    }
  }

  if (!pps->pic_scaling_matrix_present_flag && sps != NULL) {
    memcpy(pps->scaling_lists_4x4, sps->scaling_lists_4x4,
        sizeof(pps->scaling_lists_4x4));
    memcpy(pps->scaling_lists_8x8, sps->scaling_lists_8x8,
        sizeof(pps->scaling_lists_8x8));
  }
}

uint8_t parse_slice_header(struct buf_reader *buf, struct nal_unit *slc_nal,
    struct h264_parser *parser)
{
  struct slice_header *slc = &slc_nal->slc;

  slc->first_mb_in_slice = read_exp_golomb(buf);
  /* we do some parsing on the slice type, because the list is doubled */
  slc->slice_type = slice_type(read_exp_golomb(buf));

  //print_slice_type(slc->slice_type);
  slc->pic_parameter_set_id = read_exp_golomb(buf);

  /* retrieve sps and pps from the buffers */
  struct nal_unit *pps_nal =
      nal_buffer_get_by_pps_id(parser->pps_buffer, slc->pic_parameter_set_id);

  if (pps_nal == NULL) {
    fprintf(stderr, "ERR: parse_slice_header: pic_parameter_set_id %d not found in buffers\n",
            slc->pic_parameter_set_id);
        return -1;
  }

  struct pic_parameter_set_rbsp *pps = &pps_nal->pps;

  struct nal_unit *sps_nal =
      nal_buffer_get_by_sps_id(parser->sps_buffer, pps->seq_parameter_set_id);

  if (sps_nal == NULL) {
    fprintf(stderr, "ERR: parse_slice_header: seq_parameter_set_id %d not found in buffers\n",
        pps->seq_parameter_set_id);
    return -1;
  }

  struct seq_parameter_set_rbsp *sps = &sps_nal->sps;

  if(sps->separate_colour_plane_flag)
    slc->colour_plane_id = read_bits(buf, 2);

  slc->frame_num = read_bits(buf, sps->log2_max_frame_num_minus4 + 4);
  if (!sps->frame_mbs_only_flag) {
    slc->field_pic_flag = read_bits(buf, 1);
    if (slc->field_pic_flag)
      slc->bottom_field_flag = read_bits(buf, 1);
    else
      slc->bottom_field_flag = 0;
  }
  else {
    slc->field_pic_flag = 0;
    slc->bottom_field_flag = 0;
  }

  if (slc_nal->nal_unit_type == NAL_SLICE_IDR)
    slc->idr_pic_id = read_exp_golomb(buf);

  if (!sps->pic_order_cnt_type) {
    slc->pic_order_cnt_lsb = read_bits(buf,
        sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
    if (pps->pic_order_present_flag && !slc->field_pic_flag)
      slc->delta_pic_order_cnt_bottom = read_exp_golomb_s(buf);
  }

  if (sps->pic_order_cnt_type == 1 && !sps->delta_pic_order_always_zero_flag) {
    slc->delta_pic_order_cnt[0] = read_exp_golomb_s(buf);
    if (pps->pic_order_present_flag && !slc->field_pic_flag)
      slc->delta_pic_order_cnt[1] = read_exp_golomb_s(buf);
  }

  if (pps->redundant_pic_cnt_present_flag == 1) {
    slc->redundant_pic_cnt = read_exp_golomb(buf);
  }

  if (slc->slice_type == SLICE_B)
    slc->direct_spatial_mv_pred_flag = read_bits(buf, 1);

  /* take default values in case they are not set here */
  slc->num_ref_idx_l0_active_minus1 = pps->num_ref_idx_l0_active_minus1;
  slc->num_ref_idx_l1_active_minus1 = pps->num_ref_idx_l1_active_minus1;

  if (slc->slice_type == SLICE_P || slc->slice_type == SLICE_SP
      || slc->slice_type == SLICE_B) {
    slc->num_ref_idx_active_override_flag = read_bits(buf, 1);

    if (slc->num_ref_idx_active_override_flag == 1) {
      slc->num_ref_idx_l0_active_minus1 = read_exp_golomb(buf);

      if (slc->slice_type == SLICE_B) {
        slc->num_ref_idx_l1_active_minus1 = read_exp_golomb(buf);
      }
    }
  }

  /* --- ref_pic_list_reordering --- */
  parse_ref_pic_list_reordering(buf, slc);

  /* --- pred_weight_table --- */
  if ((pps->weighted_pred_flag && (slc->slice_type == SLICE_P
      || slc->slice_type == SLICE_SP)) || (pps->weighted_bipred_idc == 1
      && slc->slice_type == SLICE_B)) {
    parse_pred_weight_table(buf, slc, parser);
  }

  /* --- dec_ref_pic_marking --- */
  if (slc_nal->nal_ref_idc != 0)
    parse_dec_ref_pic_marking(buf, slc_nal);
  else
    slc->dec_ref_pic_marking_count = 0;

  return 0;
}

void interpret_slice_header(struct h264_parser *parser, struct nal_unit *slc_nal)
{
  struct coded_picture *pic = parser->pic;
  struct slice_header *slc = &slc_nal->slc;

  /* retrieve sps and pps from the buffers */
  struct nal_unit *pps_nal =
      nal_buffer_get_by_pps_id(parser->pps_buffer, slc->pic_parameter_set_id);

  if (pps_nal == NULL) {
    fprintf(stderr, "ERR: interpret_slice_header: pic_parameter_set_id %d not found in buffers\n",
            slc->pic_parameter_set_id);
        return;
  }

  struct nal_unit *sps_nal =
      nal_buffer_get_by_sps_id(parser->sps_buffer, pps_nal->pps.seq_parameter_set_id);

  if (sps_nal == NULL) {
    fprintf(stderr, "ERR: interpret_slice_header: seq_parameter_set_id %d not found in buffers\n",
        pps_nal->pps.seq_parameter_set_id);
    return;
  }

  if (pic->sps_nal) {
    release_nal_unit(pic->sps_nal);
  }
  if (pic->pps_nal) {
    release_nal_unit(pic->pps_nal);
  }
  lock_nal_unit(sps_nal);
  pic->sps_nal = sps_nal;
  lock_nal_unit(pps_nal);
  pic->pps_nal = pps_nal;
}

void parse_ref_pic_list_reordering(struct buf_reader *buf, struct slice_header *slc)
{
  if (slc->slice_type != SLICE_I && slc->slice_type != SLICE_SI) {
    slc->ref_pic_list_reordering.ref_pic_list_reordering_flag_l0 = read_bits(
        buf, 1);

    if (slc->ref_pic_list_reordering.ref_pic_list_reordering_flag_l0 == 1) {
      do {
        slc->ref_pic_list_reordering.reordering_of_pic_nums_idc
            = read_exp_golomb(buf);

        if (slc->ref_pic_list_reordering.reordering_of_pic_nums_idc == 0
            || slc->ref_pic_list_reordering.reordering_of_pic_nums_idc == 1) {
          slc->ref_pic_list_reordering.abs_diff_pic_num_minus1
              = read_exp_golomb(buf);
        }
        else if (slc->ref_pic_list_reordering.reordering_of_pic_nums_idc == 2) {
          slc->ref_pic_list_reordering.long_term_pic_num = read_exp_golomb(buf);
        }
      } while (slc->ref_pic_list_reordering.reordering_of_pic_nums_idc != 3);
    }
  }

  if (slc->slice_type == SLICE_B) {
    slc->ref_pic_list_reordering.ref_pic_list_reordering_flag_l1 = read_bits(
        buf, 1);

    if (slc->ref_pic_list_reordering.ref_pic_list_reordering_flag_l1 == 1) {
      do {
        slc->ref_pic_list_reordering.reordering_of_pic_nums_idc
            = read_exp_golomb(buf);

        if (slc->ref_pic_list_reordering.reordering_of_pic_nums_idc == 0
            || slc->ref_pic_list_reordering.reordering_of_pic_nums_idc == 1) {
          slc->ref_pic_list_reordering.abs_diff_pic_num_minus1
              = read_exp_golomb(buf);
        }
        else if (slc->ref_pic_list_reordering.reordering_of_pic_nums_idc == 2) {
          slc->ref_pic_list_reordering.long_term_pic_num = read_exp_golomb(buf);
        }
      } while (slc->ref_pic_list_reordering.reordering_of_pic_nums_idc != 3);
    }
  }
}

void parse_pred_weight_table(struct buf_reader *buf, struct slice_header *slc,
    struct h264_parser *parser)
{
  /* retrieve sps and pps from the buffers */
  struct pic_parameter_set_rbsp *pps =
      &nal_buffer_get_by_pps_id(parser->pps_buffer, slc->pic_parameter_set_id)
      ->pps;

  struct seq_parameter_set_rbsp *sps =
      &nal_buffer_get_by_sps_id(parser->sps_buffer, pps->seq_parameter_set_id)
      ->sps;

  slc->pred_weight_table.luma_log2_weight_denom = read_exp_golomb(buf);

  uint32_t ChromaArrayType = sps->chroma_format_idc;
  if(sps->separate_colour_plane_flag)
    ChromaArrayType = 0;

  if (ChromaArrayType != 0)
    slc->pred_weight_table.chroma_log2_weight_denom = read_exp_golomb(buf);

  int i;
  for (i = 0; i <= slc->num_ref_idx_l0_active_minus1; i++) {
    uint8_t luma_weight_l0_flag = read_bits(buf, 1);

    if (luma_weight_l0_flag == 1) {
      slc->pred_weight_table.luma_weight_l0[i] = read_exp_golomb_s(buf);
      slc->pred_weight_table.luma_offset_l0[i] = read_exp_golomb_s(buf);
    }

    if (ChromaArrayType != 0) {
      uint8_t chroma_weight_l0_flag = read_bits(buf, 1);

      if (chroma_weight_l0_flag == 1) {
        int j;
        for (j = 0; j < 2; j++) {
          slc->pred_weight_table.chroma_weight_l0[i][j]
              = read_exp_golomb_s(buf);
          slc->pred_weight_table.chroma_offset_l0[i][j]
              = read_exp_golomb_s(buf);
        }
      }
    }
  }

  if ((slc->slice_type % 5) == SLICE_B) {
    /* FIXME: Being spec-compliant here and loop to num_ref_idx_l0_active_minus1
     * will break Divx7 files. Keep this in mind if any other streams are broken
     */
    for (i = 0; i <= slc->num_ref_idx_l1_active_minus1; i++) {
      uint8_t luma_weight_l1_flag = read_bits(buf, 1);

      if (luma_weight_l1_flag == 1) {
        slc->pred_weight_table.luma_weight_l1[i] = read_exp_golomb_s(buf);
        slc->pred_weight_table.luma_offset_l1[i] = read_exp_golomb_s(buf);
      }

      if (ChromaArrayType != 0) {
        uint8_t chroma_weight_l1_flag = read_bits(buf, 1);

        if (chroma_weight_l1_flag == 1) {
          int j;
          for (j = 0; j < 2; j++) {
            slc->pred_weight_table.chroma_weight_l1[i][j]
                = read_exp_golomb_s(buf);
            slc->pred_weight_table.chroma_offset_l1[i][j]
                = read_exp_golomb_s(buf);
          }
        }
      }
    }
  }
}

/**
 * PicNum calculation following ITU-T H264 11/2007
 * 8.2.4.1 p112f
 */
#if 0
void calculate_pic_nums(struct h264_parser *parser, struct coded_picture *cpic)
{
  struct decoded_picture *pic = NULL;
  struct slice_header *cslc = &cpic->slc_nal->slc;

  while((pic = dpb_get_next_ref_pic(&parser->dpb, pic)) != NULL) {
    int i;
    for (i=0; i<2; i++) {
      if(pic->coded_pic[i] == NULL)
        continue;

      struct slice_header *slc = &pic->coded_pic[i]->slc_nal->slc;
      struct seq_parameter_set_rbsp *sps = &pic->coded_pic[i]->sps_nal->sps;

      if (!pic->coded_pic[i]->used_for_long_term_ref) {
        int32_t frame_num_wrap = 0;
        if (slc->frame_num > cslc->frame_num)
          frame_num_wrap = slc->frame_num - sps->max_frame_num;
        else
          frame_num_wrap = slc->frame_num;

        if(i == 0) {
          pic->frame_num_wrap = frame_num_wrap;
        }

        if (cslc->field_pic_flag == 0) {
          pic->coded_pic[i]->pic_num = frame_num_wrap;
        } else {
          pic->coded_pic[i]->pic_num = 2 * frame_num_wrap;
          if((slc->field_pic_flag == 1 &&
              cslc->bottom_field_flag == slc->bottom_field_flag) ||
              (slc->field_pic_flag == 0 && !cslc->bottom_field_flag))
            pic->coded_pic[i]->pic_num++;
        }
      } else {
        pic->coded_pic[i]->long_term_pic_num = pic->coded_pic[i]->long_term_frame_idx;
        if(slc->bottom_field_flag == cslc->bottom_field_flag)
          pic->coded_pic[i]->long_term_pic_num++;
      }
    }
  }
}

void execute_ref_pic_marking(struct coded_picture *cpic,
    uint32_t memory_management_control_operation,
    uint32_t marking_nr,
    struct h264_parser *parser)
{
  /**
   * according to NOTE 6, p83 the dec_ref_pic_marking
   * structure is identical for all slice headers within
   * a coded picture, so we can simply use the last
   * slice_header we saw in the pic
   */
  if (!cpic->slc_nal)
    return;
  struct slice_header *slc = &cpic->slc_nal->slc;
  struct dpb *dpb = &parser->dpb;

  calculate_pic_nums(parser, cpic);

  if (cpic->flag_mask & IDR_PIC) {
    if(slc->dec_ref_pic_marking[marking_nr].long_term_reference_flag) {
      cpic->used_for_long_term_ref = 1;
      dpb_set_unused_ref_picture_lidx_gt(&parser->dpb, 0);
    } else {
      dpb_set_unused_ref_picture_lidx_gt(&parser->dpb, -1);
    }
    return;
  }

  /* MMC operation == 1 : 8.2.5.4.1, p. 120 */
  if (memory_management_control_operation == 1) {
    // short-term -> unused for reference
    int32_t pic_num_x = (parser->curr_pic_num
        - (slc->dec_ref_pic_marking[marking_nr].difference_of_pic_nums_minus1 + 1));
        //% cpic->max_pic_num;
    struct decoded_picture* pic = NULL;
    if ((pic = dpb_get_picture(dpb, pic_num_x)) != NULL) {
      if (cpic->slc_nal->slc.field_pic_flag == 0) {
        dpb_set_unused_ref_picture_a(dpb, pic);
      } else {

        if (pic->coded_pic[0]->slc_nal->slc.field_pic_flag == 1) {
          if (pic->top_is_reference)
            pic->top_is_reference = 0;
          else if (pic->bottom_is_reference)
            pic->bottom_is_reference = 0;

          if(!pic->top_is_reference && !pic->bottom_is_reference)
            dpb_set_unused_ref_picture_a(dpb, pic);
        } else {
          pic->top_is_reference = pic->bottom_is_reference = 0;
          dpb_set_unused_ref_picture_a(dpb, pic);
        }
      }
    } else {
        fprintf(stderr, "H264: mmc 1 failed: %d not existent - curr_pic: %d\n", pic_num_x, parser->curr_pic_num);
    }
  } else if (memory_management_control_operation == 2) {
    // long-term -> unused for reference
    struct decoded_picture* pic = dpb_get_picture_by_ltpn(dpb,
        slc->dec_ref_pic_marking[marking_nr].long_term_pic_num);
    if (pic != NULL) {
      if (cpic->slc_nal->slc.field_pic_flag == 0)
        dpb_set_unused_ref_picture_byltpn(dpb,
            slc->dec_ref_pic_marking[marking_nr].long_term_pic_num);
      else {

        if (pic->coded_pic[0]->slc_nal->slc.field_pic_flag == 1) {
          if (pic->top_is_reference)
            pic->top_is_reference = 0;
          else if (pic->bottom_is_reference)
            pic->bottom_is_reference = 0;

          if(!pic->top_is_reference && !pic->bottom_is_reference) {
            dpb_set_unused_ref_picture_byltpn(dpb,
                slc->dec_ref_pic_marking[marking_nr].long_term_pic_num);
          }
        } else {
          pic->top_is_reference = pic->bottom_is_reference = 0;
          dpb_set_unused_ref_picture_byltpn(dpb,
              slc->dec_ref_pic_marking[marking_nr].long_term_pic_num);
        }
      }
    }
  } else if (memory_management_control_operation == 3) {
    // short-term -> long-term, set long-term frame index
    uint32_t pic_num_x = parser->curr_pic_num
        - (slc->dec_ref_pic_marking[marking_nr].difference_of_pic_nums_minus1 + 1);
    struct decoded_picture* pic = dpb_get_picture_by_ltidx(dpb,
        slc->dec_ref_pic_marking[marking_nr].long_term_pic_num);
    if (pic != NULL)
      dpb_set_unused_ref_picture_bylidx(dpb,
          slc->dec_ref_pic_marking[marking_nr].long_term_frame_idx);

    pic = dpb_get_picture(dpb, pic_num_x);
    if (pic) {
      pic = dpb_get_picture(dpb, pic_num_x);

      if (pic->coded_pic[0]->slc_nal->slc.field_pic_flag == 0) {
        pic->coded_pic[0]->long_term_frame_idx
            = slc->dec_ref_pic_marking[marking_nr].long_term_frame_idx;
        pic->coded_pic[0]->long_term_pic_num = pic->coded_pic[0]->long_term_frame_idx;
      }
      else {
        if(pic->coded_pic[0]->pic_num == pic_num_x) {
          pic->coded_pic[0]->long_term_frame_idx
              = slc->dec_ref_pic_marking[marking_nr].long_term_frame_idx;
          pic->coded_pic[0]->long_term_pic_num = pic->coded_pic[0]->long_term_frame_idx * 2 + 1;
        } else if(pic->coded_pic[1] != NULL &&
            pic->coded_pic[1]->pic_num == pic_num_x) {
          pic->coded_pic[1]->long_term_frame_idx
              = slc->dec_ref_pic_marking[marking_nr].long_term_frame_idx;
          pic->coded_pic[1]->long_term_pic_num = pic->coded_pic[1]->long_term_frame_idx * 2 + 1;
        }
        printf("FIXME: B Set frame %d to long-term ref\n", pic_num_x);
      }
    }
    else {
      fprintf(stderr, "memory_management_control_operation: 3 failed. No such picture.\n");
    }

  } else if (memory_management_control_operation == 4) {
    // set max-long-term frame index,
    // mark all long-term pictures with long-term frame idx
    // greater max-long-term farme idx as unused for ref
    if (slc->dec_ref_pic_marking[marking_nr].max_long_term_frame_idx_plus1 == 0)
      dpb_set_unused_ref_picture_lidx_gt(dpb, 0);
    else
      dpb_set_unused_ref_picture_lidx_gt(dpb,
          slc->dec_ref_pic_marking[marking_nr].max_long_term_frame_idx_plus1 - 1);
  } else if (memory_management_control_operation == 5) {
    // mark all ref pics as unused for reference,
    // set max-long-term frame index = no long-term frame idxs
    dpb_flush(dpb);

    if (!slc->bottom_field_flag) {
      parser->prev_pic_order_cnt_lsb = cpic->top_field_order_cnt;
      parser->prev_pic_order_cnt_msb = 0;
    } else {
      parser->prev_pic_order_cnt_lsb = 0;
      parser->prev_pic_order_cnt_msb = 0;
    }
  } else if (memory_management_control_operation == 6) {
    // mark current picture as used for long-term ref,
    // assing long-term frame idx to it
    struct decoded_picture* pic = dpb_get_picture_by_ltidx(dpb,
        slc->dec_ref_pic_marking[marking_nr].long_term_frame_idx);
    if (pic != NULL)
      dpb_set_unused_ref_picture_bylidx(dpb,
          slc->dec_ref_pic_marking[marking_nr].long_term_frame_idx);

    cpic->long_term_frame_idx = slc->dec_ref_pic_marking[marking_nr].long_term_frame_idx;
    cpic->used_for_long_term_ref = 1;

    if (slc->field_pic_flag == 0) {
      cpic->long_term_pic_num = cpic->long_term_frame_idx;
    }
    else {
      cpic->long_term_pic_num = cpic->long_term_frame_idx * 2 + 1;
    }

  }
}
#endif

void parse_dec_ref_pic_marking(struct buf_reader *buf,
    struct nal_unit *slc_nal)
{
  struct slice_header *slc = &slc_nal->slc;

  if (!slc)
    return;

  slc->dec_ref_pic_marking_count = 0;
  int i = slc->dec_ref_pic_marking_count;

  if (slc_nal->nal_unit_type == NAL_SLICE_IDR) {
    slc->dec_ref_pic_marking[i].no_output_of_prior_pics_flag = read_bits(buf, 1);
    slc->dec_ref_pic_marking[i].long_term_reference_flag = read_bits(buf, 1);
    i+=2;
  } else {
    slc->dec_ref_pic_marking[i].adaptive_ref_pic_marking_mode_flag = read_bits(
        buf, 1);

    if (slc->dec_ref_pic_marking[i].adaptive_ref_pic_marking_mode_flag) {
      do {
        slc->dec_ref_pic_marking[i].memory_management_control_operation
            = read_exp_golomb(buf);

        if (slc->dec_ref_pic_marking[i].memory_management_control_operation == 1
            || slc->dec_ref_pic_marking[i].memory_management_control_operation
                == 3)
          slc->dec_ref_pic_marking[i].difference_of_pic_nums_minus1
              = read_exp_golomb(buf);

        if (slc->dec_ref_pic_marking[i].memory_management_control_operation == 2)
          slc->dec_ref_pic_marking[i].long_term_pic_num = read_exp_golomb(buf);

        if (slc->dec_ref_pic_marking[i].memory_management_control_operation == 3
            || slc->dec_ref_pic_marking[i].memory_management_control_operation
                == 6)
          slc->dec_ref_pic_marking[i].long_term_frame_idx = read_exp_golomb(buf);

        if (slc->dec_ref_pic_marking[i].memory_management_control_operation == 4)
          slc->dec_ref_pic_marking[i].max_long_term_frame_idx_plus1
              = read_exp_golomb(buf);

        i++;
        if(i >= 10) {
          fprintf(stderr, "Error: Not more than 10 MMC operations supported per slice. Dropping some.\n");
          i = 0;
        }
      } while (slc->dec_ref_pic_marking[i-1].memory_management_control_operation
          != 0);
    }
  }

  slc->dec_ref_pic_marking_count = (i>0) ? (i-1) : 0;
}

/* ----------------- NAL parser ----------------- */

struct h264_parser* init_parser()
{
  struct h264_parser *parser = calloc(1, sizeof(struct h264_parser));
  parser->pic = create_coded_picture();
  parser->position = NON_VCL;
  parser->last_vcl_nal = NULL;
  parser->sps_buffer = create_nal_buffer(32);
  parser->pps_buffer = create_nal_buffer(32);
  // sdd memset(&parser->dpb, 0x00, sizeof(struct dpb));

  /* no idea why we do that. inspired by libavcodec,
   * as we couldn't figure in the specs....
   */
  parser->prev_pic_order_cnt_msb = 1 << 16;

  return parser;
}

void reset_parser(struct h264_parser *parser)
{
  parser->position = NON_VCL;
  parser->buf_len = parser->prebuf_len = 0;
  parser->next_nal_position = 0;
  parser->last_nal_res = 0;

  if(parser->last_vcl_nal) {
    release_nal_unit(parser->last_vcl_nal);
  }
  parser->last_vcl_nal = NULL;

  parser->prev_pic_order_cnt_msb = 1 << 16;
  parser->prev_pic_order_cnt_lsb = 0;
  parser->frame_num_offset = 0;
  parser->prev_top_field_order_cnt = 0;
  parser->curr_pic_num = 0;
  parser->flag_mask = 0;
}

void free_parser(struct h264_parser *parser)
{
  // sdd dpb_free_all(&parser->dpb);
  free_nal_buffer(parser->pps_buffer);
  free_nal_buffer(parser->sps_buffer);
  free(parser);
}

int parse_codec_private(struct h264_parser *parser, uint8_t *inbuf, int inbuf_len)
{
  struct buf_reader bufr;
#ifdef NOVDPAU
	int nFirst = 1;
  parser->privatebuf_len = 0;
#endif

  bufr.buf = inbuf;
  bufr.cur_pos = inbuf;
  bufr.cur_offset = 8;
  bufr.len = inbuf_len;

  // FIXME: Might be broken!
  struct nal_unit *nal = calloc(1, sizeof(struct nal_unit));


  /* reserved */
  read_bits(&bufr, 8);
  nal->sps.profile_idc = read_bits(&bufr, 8);
  read_bits(&bufr, 8);
  nal->sps.level_idc = read_bits(&bufr, 8);
  read_bits(&bufr, 6);

  parser->nal_size_length = read_bits(&bufr, 2) + 1;
  parser->nal_size_length_buf = calloc(1, parser->nal_size_length);
  read_bits(&bufr, 3);
  uint8_t sps_count = read_bits(&bufr, 5);

  inbuf += 6;
  inbuf_len -= 6;
  int i;

  struct coded_picture *dummy = NULL;
  for(i = 0; i < sps_count; i++) {
    uint16_t sps_size = read_bits(&bufr, 16);
    
    if (sps_size > inbuf_len) {
      free(nal);
      free(parser->nal_size_length_buf);
      parser->nal_size_length = 0;
      return 1;
    }
    if (sps_size && sps_size <= inbuf_len) {
      inbuf += 2;
      inbuf_len -= 2;
#ifdef NOVDPAU
      printf("parse codec private SPS\n");
      /* copy SPS NALU's to privatebuf */
      static const uint8_t start_seq[4] = { 0x00, 0x00, 0x00, 0x01 };
      if(nFirst) {
        nFirst = 0;
        fast_memcpy(parser->privatebuf+parser->privatebuf_len, start_seq, 4);
        parser->privatebuf_len+=4;
      } else {
        fast_memcpy(parser->privatebuf+parser->privatebuf_len, start_seq + 1, 3);
        parser->privatebuf_len+=3;
      }

      fast_memcpy(parser->privatebuf+parser->privatebuf_len, inbuf, sps_size);
      parser->privatebuf_len+=sps_size;
#endif
      parse_nal(inbuf, sps_size, parser, &dummy);
      inbuf += sps_size;
      inbuf_len -= sps_size;
    }
  }

  bufr.buf = inbuf;
  bufr.cur_pos = inbuf;
  bufr.cur_offset = 8;
  bufr.len = inbuf_len;

  uint8_t pps_count = read_bits(&bufr, 8);
  inbuf += 1;
  for(i = 0; i < pps_count; i++) {
    uint16_t pps_size = read_bits(&bufr, 16);
    inbuf += 2;
    inbuf_len -= 2;
#ifdef NOVDPAU
    printf("parse codec private PPS\n");
    /* copy PPS NALU's to privatebuf */
    static const uint8_t start_seq[4] = { 0x00, 0x00, 0x00, 0x01 };
		if(nFirst) {
			nFirst = 0;
	    fast_memcpy(parser->privatebuf+parser->privatebuf_len, start_seq, 4);
			parser->privatebuf_len+=4;
		} else {
	    fast_memcpy(parser->privatebuf+parser->privatebuf_len, start_seq + 1, 3);
			parser->privatebuf_len+=3;
		}
		fast_memcpy(parser->privatebuf+parser->privatebuf_len, inbuf, pps_size);
		parser->privatebuf_len+=pps_size;
#endif
    parse_nal(inbuf, pps_size, parser, &dummy);
    inbuf += pps_size;
    inbuf_len -= pps_size;
  }

  nal_buffer_append(parser->sps_buffer, nal);
  
  return 0;
}

#if 0
void process_mmc_operations(struct h264_parser *parser, struct coded_picture *picture)
{
  if (picture->flag_mask & REFERENCE) {
    parser->prev_pic_order_cnt_lsb
          = picture->slc_nal->slc.pic_order_cnt_lsb;
  }

  int i;
  for(i = 0; i < picture->slc_nal->slc.
      dec_ref_pic_marking_count; i++) {
    execute_ref_pic_marking(
        picture,
        picture->slc_nal->slc.dec_ref_pic_marking[i].
        memory_management_control_operation,
        i,
        parser);
  }
}
#endif

int parse_frame(struct h264_parser *parser, uint8_t *inbuf, int inbuf_len,
    int64_t pts,
    uint8_t **ret_buf, uint32_t *ret_len, struct coded_picture **ret_pic)
{
  int32_t next_nal = 0;
  int32_t offset = 0;
  int start_seq_len = 3;

  *ret_pic = NULL;
  *ret_buf = NULL;
  *ret_len = 0;

  if(parser->nal_size_length > 0)
    start_seq_len = offset = parser->nal_size_length;

  if (parser->prebuf_len + inbuf_len > MAX_FRAME_SIZE) {
    fprintf(stderr, "h264_parser: prebuf underrung\n");
    *ret_len = 0;
    *ret_buf = NULL;
    parser->prebuf_len = 0;
    return inbuf_len;
  }

  /* copy the whole inbuf to the prebuf,
   * then search for a nal-start sequence in the prebuf,
   * if it's in there, parse the nal and append to parser->buf
   * or return a frame */

  fast_memcpy(parser->prebuf + parser->prebuf_len, inbuf, inbuf_len);
  parser->prebuf_len += inbuf_len;

  while((next_nal = seek_for_nal(parser->prebuf+start_seq_len-offset, parser->prebuf_len-start_seq_len+offset, parser)) > 0) {

    struct coded_picture *completed_pic = NULL;

    if(!parser->nal_size_length &&
        (parser->prebuf[0] != 0x00 || parser->prebuf[1] != 0x00 ||
            parser->prebuf[2] != 0x01)) {
      fprintf(stderr, "Broken NAL, skip it.\n");
      parser->last_nal_res = 2;
    } else {
      parser->last_nal_res = parse_nal(parser->prebuf+start_seq_len,
          next_nal, parser, &completed_pic);
    }

    if (completed_pic != NULL &&
        completed_pic->slice_cnt > 0 &&
        parser->buf_len > 0) {

      //printf("Frame complete: %d bytes\n", parser->buf_len);
#ifdef NOVDPAU
      uint8_t *p;
      *ret_len = parser->buf_len + parser->privatebuf_len;
      *ret_buf = malloc(*ret_len);
      p = *ret_buf;

      //if(parser->pic->flag_mask & IDR_PIC) {
        if(parser->privatebuf_len >  0) {
          fast_memcpy(p, parser->privatebuf, parser->privatebuf_len);
          p += parser->privatebuf_len;
          parser->privatebuf_len = 0;
        }
      //}
      fast_memcpy(p, parser->buf, parser->buf_len);
#else
      *ret_len = parser->buf_len;
      *ret_buf = malloc(*ret_len);
      fast_memcpy(*ret_buf, parser->buf, parser->buf_len);
#endif
      *ret_pic = completed_pic;

      parser->buf_len = 0;

      if (pts != 0 && (parser->pic->pts == 0 || parser->pic->pts != pts)) {
        parser->pic->pts = pts;
      }

      /**
       * if the new coded picture started with a VCL nal
       * we have to copy this to buffer for the next picture
       * now.
       */
      if(parser->last_nal_res == 1) {
        if(parser->nal_size_length > 0) {
          static const uint8_t start_seq[3] = { 0x00, 0x00, 0x01 };
          fast_memcpy(parser->buf, start_seq, 3);
          parser->buf_len += 3;
        }

        fast_memcpy(parser->buf+parser->buf_len, parser->prebuf+offset, next_nal+start_seq_len-2*offset);
        parser->buf_len += next_nal+start_seq_len-2*offset;
      }

      memmove(parser->prebuf, parser->prebuf+(next_nal+start_seq_len-offset), parser->prebuf_len-(next_nal+start_seq_len-offset));
      parser->prebuf_len -= next_nal+start_seq_len-offset;

      return inbuf_len;
    }

    /* got a new nal, which is part of the current
     * coded picture. add it to buf
     */
#ifdef NOVDPAU
    if (parser->last_nal_res < 3) {
#else
    if (parser->last_nal_res < 2) {
#endif
      if (parser->buf_len + next_nal+start_seq_len-offset > MAX_FRAME_SIZE) {
        fprintf(stderr, "h264_parser: buf underrun!\n");
        parser->buf_len = 0;
        *ret_len = 0;
        *ret_buf = NULL;
        return inbuf_len;
      }

      if(parser->nal_size_length > 0) {
        static const uint8_t start_seq[3] = { 0x00, 0x00, 0x01 };
        fast_memcpy(parser->buf+parser->buf_len, start_seq, 3);
        parser->buf_len += 3;
      }

      fast_memcpy(parser->buf+parser->buf_len, parser->prebuf+offset, next_nal+start_seq_len-2*offset);
      parser->buf_len += next_nal+start_seq_len-2*offset;

      memmove(parser->prebuf, parser->prebuf+(next_nal+start_seq_len-offset), parser->prebuf_len-(next_nal+start_seq_len-offset));
      parser->prebuf_len -= next_nal+start_seq_len-offset;
    } else {
      /* got a non-relevant nal, just remove it */
      memmove(parser->prebuf, parser->prebuf+(next_nal+start_seq_len-offset), parser->prebuf_len-(next_nal+start_seq_len-offset));
      parser->prebuf_len -= next_nal+start_seq_len-offset;
    }
  }

  if (pts != 0 && (parser->pic->pts == 0 || parser->pic->pts != pts)) {
    parser->pic->pts = pts;
  }

  *ret_buf = NULL;
  *ret_len = 0;
  return inbuf_len;
}


/**
 * @return 0: NAL is part of coded picture
 *         2: NAL is not part of coded picture
 *         1: NAL is the beginning of a new coded picture
 *         3: NAL is marked as END_OF_SEQUENCE
 */
int parse_nal(uint8_t *buf, int buf_len, struct h264_parser *parser,
    struct coded_picture **completed_picture)
{
  int ret = 0;

  struct buf_reader bufr;

  bufr.buf = buf;
  bufr.cur_pos = buf;
  bufr.cur_offset = 8;
  bufr.len = buf_len;

  *completed_picture = NULL;

  struct nal_unit *nal = parse_nal_header(&bufr, parser->pic, parser);

  /**
   * we detect the start of a new access unit if
   * a non-vcl nal unit is received after a vcl
   * nal unit
   */
  if (nal->nal_unit_type >= NAL_SLICE &&
      nal->nal_unit_type <= NAL_SLICE_IDR) {
    parser->position = VCL;
  } else if ((parser->position == VCL &&
      nal->nal_unit_type >= NAL_SEI &&
      nal->nal_unit_type <= NAL_PPS) ||
      nal->nal_unit_type == NAL_AU_DELIMITER) {
    /* start of a new access unit! */
    *completed_picture = parser->pic;
    parser->pic = create_coded_picture();

    if(parser->last_vcl_nal != NULL) {
      release_nal_unit(parser->last_vcl_nal);
      parser->last_vcl_nal = NULL;
    }
    parser->position = NON_VCL;
  } else {
    parser->position = NON_VCL;
  }

  switch(nal->nal_unit_type) {
    case NAL_SPS:
      nal_buffer_append(parser->sps_buffer, nal);
      break;
    case NAL_PPS:
      nal_buffer_append(parser->pps_buffer, nal);
      break;
    case NAL_SEI: {
      if (parser->pic != NULL) {
        if(parser->pic->sei_nal) {
          release_nal_unit(parser->pic->sei_nal);
        }
        lock_nal_unit(nal);
        parser->pic->sei_nal = nal;
        interpret_sei(parser->pic);
      }
    }
    default:
      break;
  }

  /**
   * in case of an access unit which does not contain any
   * non-vcl nal units we have to detect the new access
   * unit through the algorithm for detecting first vcl nal
   * units of a primary coded picture
   */
  if (parser->position == VCL && parser->last_vcl_nal != NULL &&
      nal->nal_unit_type >= NAL_SLICE && nal->nal_unit_type <= NAL_SLICE_IDR) {
    /**
     * frame boundary detection according to
     * ITU-T Rec. H264 (11/2007) chapt 7.4.1.2.4, p65
     */
    struct nal_unit* last_nal = parser->last_vcl_nal;

    if (nal == NULL || last_nal == NULL) {
      ret = 1;
    } else if (nal->slc.frame_num != last_nal->slc.frame_num) {
      ret = 1;
    } else if (nal->slc.pic_parameter_set_id
        != last_nal->slc.pic_parameter_set_id) {
      ret = 1;
    } else if (nal->slc.field_pic_flag
        != last_nal->slc.field_pic_flag) {
      ret = 1;
    } else if (nal->slc.bottom_field_flag
        != last_nal->slc.bottom_field_flag) {
      ret = 1;
    } else if (nal->nal_ref_idc != last_nal->nal_ref_idc &&
        (nal->nal_ref_idc == 0 || last_nal->nal_ref_idc == 0)) {
      ret = 1;
    } else if (nal->sps.pic_order_cnt_type == 0
            && last_nal->sps.pic_order_cnt_type == 0
            && (nal->slc.pic_order_cnt_lsb != last_nal->slc.pic_order_cnt_lsb
                || nal->slc.delta_pic_order_cnt_bottom
                != last_nal->slc.delta_pic_order_cnt_bottom)) {
      ret = 1;
    } else if (nal->sps.pic_order_cnt_type == 1
        && last_nal->sps.pic_order_cnt_type == 1
        && (nal->slc.delta_pic_order_cnt[0]
            != last_nal->slc.delta_pic_order_cnt[0]
            || nal->slc.delta_pic_order_cnt[1]
                != last_nal->slc.delta_pic_order_cnt[1])) {
      ret = 1;
    } else if (nal->nal_unit_type != last_nal->nal_unit_type && (nal->nal_unit_type
        == NAL_SLICE_IDR || last_nal->nal_unit_type == NAL_SLICE_IDR)) {
      ret = 1;
    } else if (nal->nal_unit_type == NAL_SLICE_IDR
        && last_nal->nal_unit_type == NAL_SLICE_IDR && nal->slc.idr_pic_id
        != last_nal->slc.idr_pic_id) {
      ret = 1;
    }

    /* increase the slice_cnt until a new frame is detected */
    if (ret) {
      *completed_picture = parser->pic;
      parser->pic = create_coded_picture();
    }

  } else if (nal->nal_unit_type == NAL_PPS || nal->nal_unit_type == NAL_SPS) {
    ret = 2;
  } else if (nal->nal_unit_type == NAL_AU_DELIMITER) {
    ret = 2;
  } else if (nal->nal_unit_type == NAL_END_OF_SEQUENCE) {
    ret = 3;
  } else if (nal->nal_unit_type >= NAL_SEI) {
    ret = 2;
  }

  if (parser->pic) {

    if (nal->nal_unit_type == NAL_SLICE_IDR) {
      parser->pic->flag_mask |= IDR_PIC;
    }

    if (nal->nal_ref_idc) {
      parser->pic->flag_mask |= REFERENCE;
    }

    if (nal->nal_unit_type >= NAL_SLICE &&
        nal->nal_unit_type <= NAL_SLICE_IDR) {
      lock_nal_unit(nal);
      if(parser->last_vcl_nal) {
        release_nal_unit(parser->last_vcl_nal);
      }
      parser->last_vcl_nal = nal;

      parser->pic->slice_cnt++;
      if(parser->pic->slc_nal) {
        release_nal_unit(parser->pic->slc_nal);
      }
      lock_nal_unit(nal);
      parser->pic->slc_nal = nal;

      interpret_slice_header(parser, nal);
    }

    if (*completed_picture != NULL &&
        (*completed_picture)->slice_cnt > 0) {
      calculate_pic_order(parser, *completed_picture,
          &((*completed_picture)->slc_nal->slc));
      interpret_sps(*completed_picture, parser);
      interpret_pps(*completed_picture);
    }
  }

  release_nal_unit(nal);
  return ret;
}

int seek_for_nal(uint8_t *buf, int buf_len, struct h264_parser *parser)
{
  if(buf_len <= 0)
    return -1;

  if(parser->nal_size_length > 0) {
    if(buf_len < parser->nal_size_length) {
      return -1;
    }

    uint32_t next_nal = parser->next_nal_position;
    if(!next_nal) {
      struct buf_reader bufr;

      bufr.buf = buf;
      bufr.cur_pos = buf;
      bufr.cur_offset = 8;
      bufr.len = buf_len;

      next_nal = read_bits(&bufr, parser->nal_size_length*8)+parser->nal_size_length;
    }

    if(next_nal > buf_len) {
      parser->next_nal_position = next_nal;
      return -1;
    } else
      parser->next_nal_position = 0;

    return next_nal;
  }

  /* NAL_END_OF_SEQUENCE has only 1 byte, so
   * we do not need to search for the next start sequence */
  if(buf[0] == NAL_END_OF_SEQUENCE)
    return 1;

  int i;
  for (i = 0; i < buf_len - 2; i++) {
    if (buf[i] == 0x00 && buf[i + 1] == 0x00 && buf[i + 2] == 0x01) {
      //printf("found nal at: %d\n", i);
      return i;
    }
  }

  return -1;
}
