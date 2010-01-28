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
 * nal.h: H264 NAL structures
 */
#define NOVDPAU

#ifndef NAL_H_
#define NAL_H_
#include <stdint.h>
#ifndef NOVDPAU
#include <vdpau/vdpau.h>
#endif

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

enum nal_unit_types
{
  NAL_UNSPECIFIED = 0,
  NAL_SLICE,
  NAL_PART_A,
  NAL_PART_B,
  NAL_PART_C,
  NAL_SLICE_IDR,
  NAL_SEI,
  NAL_SPS,
  NAL_PPS,
  NAL_AU_DELIMITER,
  NAL_END_OF_SEQUENCE,
  NAL_END_OF_STREAM,
  NAL_FILLER_DATA,
  NAL_SPS_EXT
};

enum pic_struct {
  DISP_FRAME = 0,
  DISP_TOP,
  DISP_BOTTOM,
  DISP_TOP_BOTTOM,
  DISP_BOTTOM_TOP,
  DISP_TOP_BOTTOM_TOP,
  DISP_TOP_TOP_BOTTOM,
  DISP_BOTTOM_TOP_BOTTOM,
  DISP_FRAME_DOUBLING,
  DISP_FRAME_TRIPLING
};

/* slice types repeat from 5-9, we
 * need a helper function for comparison
 */
enum slice_types
{
  SLICE_P = 0, SLICE_B, SLICE_I, SLICE_SP, SLICE_SI
};

enum aspect_ratio
{
  ASPECT_UNSPECIFIED = 0,
  ASPECT_1_1,
  ASPECT_12_11,
  ASPECT_10_11,
  ASPECT_16_11,
  ASPECT_40_33,
  ASPECT_24_11,
  ASPECT_20_11,
  ASPECT_32_11,
  ASPECT_80_33,
  ASPECT_18_11,
  ASPECT_15_11,
  ASPECT_64_33,
  ASPECT_160_99,
  ASPECT_4_3,
  ASPECT_3_2,
  ASPECT_2_1,
  ASPECT_RESERVED,
  ASPECT_EXTENDED_SAR=255
};

static const uint8_t zigzag_4x4[16] = {
  0+0*4, 1+0*4, 0+1*4, 0+2*4,
  1+1*4, 2+0*4, 3+0*4, 2+1*4,
  1+2*4, 0+3*4, 1+3*4, 2+2*4,
  3+1*4, 3+2*4, 2+3*4, 3+3*4,
};

static const uint8_t zigzag_8x8[64] = {
  0+0*8, 1+0*8, 0+1*8, 0+2*8,
  1+1*8, 2+0*8, 3+0*8, 2+1*8,
  1+2*8, 0+3*8, 0+4*8, 1+3*8,
  2+2*8, 3+1*8, 4+0*8, 5+0*8,
  4+1*8, 3+2*8, 2+3*8, 1+4*8,
  0+5*8, 0+6*8, 1+5*8, 2+4*8,
  3+3*8, 4+2*8, 5+1*8, 6+0*8,
  7+0*8, 6+1*8, 5+2*8, 4+3*8,
  3+4*8, 2+5*8, 1+6*8, 0+7*8,
  1+7*8, 2+6*8, 3+5*8, 4+4*8,
  5+3*8, 6+2*8, 7+1*8, 7+2*8,
  6+3*8, 5+4*8, 4+5*8, 3+6*8,
  2+7*8, 3+7*8, 4+6*8, 5+5*8,
  6+4*8, 7+3*8, 7+4*8, 6+5*8,
  5+6*8, 4+7*8, 5+7*8, 6+6*8,
  7+5*8, 7+6*8, 6+7*8, 7+7*8,
};

static inline uint32_t slice_type(uint32_t slice_type)
{
  return (slice_type < 10 ? slice_type % 5 : slice_type);
}

#if 0
static inline void print_slice_type(uint32_t slice_type)
{
  switch(slice_type) {
    case SLICE_P:
      printf("SLICE_P\n");
      break;
    case SLICE_B:
      printf("SLICE_B\n");
      break;
    case SLICE_I:
      printf("SLICE_I\n");
      break;
    case SLICE_SP:
      printf("SLICE_SP\n");
      break;
    case SLICE_SI:
      printf("SLICE_SI\n");
      break;
    default:
      printf("Unknown SLICE\n");
  }
}
#endif

struct hrd_parameters
{
  uint32_t cpb_cnt_minus1;
  uint8_t bit_rate_scale;
  uint8_t cpb_size_scale;

  uint32_t bit_rate_value_minus1[32];
  uint32_t cpb_size_value_minus1[32];
  uint8_t cbr_flag[32];

  uint8_t initial_cpb_removal_delay_length_minus1;
  uint8_t cpb_removal_delay_length_minus1;
  uint8_t dpb_output_delay_length_minus1;
  uint8_t time_offset_length;
};

struct seq_parameter_set_rbsp
{
  uint8_t profile_idc; // 0xff
  uint8_t constraint_setN_flag; // 0x0f
  uint8_t level_idc; // 0xff
  uint32_t seq_parameter_set_id;
  uint32_t chroma_format_idc;
  uint8_t separate_colour_plane_flag; // 0x01
  uint32_t bit_depth_luma_minus8;
  uint32_t bit_depth_chroma_minus8;
  uint8_t qpprime_y_zero_transform_bypass_flag;
  uint8_t seq_scaling_matrix_present_flag;

  /* if(seq_scaling_matrix_present_flag) */
  uint8_t seq_scaling_list_present_flag[8];

  uint8_t scaling_lists_4x4[6][16];
  uint8_t scaling_lists_8x8[2][64];
  /* endif */

  uint32_t log2_max_frame_num_minus4;
  uint32_t max_frame_num;
  uint32_t pic_order_cnt_type;
  // if pic_order_cnt_type==0
  uint32_t log2_max_pic_order_cnt_lsb_minus4;
  // else
  uint8_t delta_pic_order_always_zero_flag;
  int32_t offset_for_non_ref_pic;
  int32_t offset_for_top_to_bottom_field;
  uint8_t num_ref_frames_in_pic_order_cnt_cycle;
  int32_t offset_for_ref_frame[256];
  // TODO: some more ignored here
  uint32_t num_ref_frames;
  uint8_t gaps_in_frame_num_value_allowed_flag;
  /*uint32_t    pic_width_in_mbs_minus1;
   uint32_t    pic_height_in_map_units_minus1;*/
  uint32_t pic_width;
  uint32_t pic_height;
  uint8_t frame_mbs_only_flag;
  uint8_t mb_adaptive_frame_field_flag;
  uint8_t direct_8x8_inference_flag;
  uint8_t frame_cropping_flag;
  uint32_t frame_crop_left_offset;
  uint32_t frame_crop_right_offset;
  uint32_t frame_crop_top_offset;
  uint32_t frame_crop_bottom_offset;
  uint8_t vui_parameters_present_flag;

  /* vui_parameters */
  struct
  {
    uint8_t aspect_ration_info_present_flag;

    /* aspect_ration_info_present_flag == 1 */
    uint8_t aspect_ratio_idc;
    uint16_t sar_width;
    uint16_t sar_height;

    uint8_t overscan_info_present_flag;
    /* overscan_info_present_flag == 1 */
    uint8_t overscan_appropriate_flag;

    uint8_t video_signal_type_present_flag;
    /* video_signal_type_present_flag == 1 */
    uint8_t video_format;
    uint8_t video_full_range_flag;
    uint8_t colour_description_present;
    /* colour_description_present == 1 */
    uint8_t colour_primaries;
    uint8_t transfer_characteristics;
    uint8_t matrix_coefficients;

    uint8_t chroma_loc_info_present_flag;
    /* chroma_loc_info_present_flag == 1 */
    uint8_t chroma_sample_loc_type_top_field;
    uint8_t chroma_sample_loc_type_bottom_field;

    uint8_t timing_info_present_flag;
    /* timing_info_present_flag == 1 */
    uint32_t num_units_in_tick;
    uint32_t time_scale;
    uint8_t fixed_frame_rate_flag;

    uint8_t nal_hrd_parameters_present_flag;
    struct hrd_parameters nal_hrd_parameters;

    uint8_t vc1_hrd_parameters_present_flag;
    struct hrd_parameters vc1_hrd_parameters;

    uint8_t low_delay_hrd_flag;

    uint8_t pic_struct_present_flag;
    uint8_t bitstream_restriction_flag;

    /* bitstream_restriction_flag == 1 */
    uint8_t motion_vectors_over_pic_boundaries;
    uint32_t max_bytes_per_pic_denom;
    uint32_t max_bits_per_mb_denom;
    uint32_t log2_max_mv_length_horizontal;
    uint32_t log2_max_mv_length_vertical;
    uint32_t num_reorder_frames;
    uint32_t max_dec_frame_buffering;
  } vui_parameters;

};

struct pic_parameter_set_rbsp
{
  uint32_t pic_parameter_set_id;
  uint32_t seq_parameter_set_id;
  uint8_t entropy_coding_mode_flag;
  uint8_t pic_order_present_flag;

  uint32_t num_slice_groups_minus1;

  /* num_slice_groups_minus1 > 0 */
  uint32_t slice_group_map_type;

  /* slice_group_map_type == 1 */
  uint32_t run_length_minus1[64];

  /* slice_group_map_type == 2 */
  uint32_t top_left[64];
  uint32_t bottom_right[64];

  /* slice_group_map_type == 3,4,5 */
  uint8_t slice_group_change_direction_flag;
  uint32_t slice_group_change_rate_minus1;

  /* slice_group_map_type == 6 */
  uint32_t pic_size_in_map_units_minus1;
  uint8_t slice_group_id[64];

  uint32_t num_ref_idx_l0_active_minus1;
  uint32_t num_ref_idx_l1_active_minus1;
  uint8_t weighted_pred_flag;
  uint8_t weighted_bipred_idc;
  int32_t pic_init_qp_minus26;
  int32_t pic_init_qs_minus26;
  int32_t chroma_qp_index_offset;
  uint8_t deblocking_filter_control_present_flag;
  uint8_t constrained_intra_pred_flag;
  uint8_t redundant_pic_cnt_present_flag;

  /* if(more_rbsp_data) */
  uint8_t transform_8x8_mode_flag;
  uint8_t pic_scaling_matrix_present_flag;

  /* if(pic_scaling_matrix_present_flag) */
  uint8_t pic_scaling_list_present_flag[8];

  uint8_t scaling_lists_4x4[6][16];
  uint8_t scaling_lists_8x8[2][64];

  int32_t second_chroma_qp_index_offset;
};

/*struct clock_timestamp {
  uint8_t ct_type;
  uint8_t nuit_fiel_based_flag;
  uint8_t counting_type;
  uint8_t full_timestamp_flag;
  uint8_t discontinuity_flag;
  uint8_t cnt_dropped_flag;
  uint8_t n_frames
};*/

/* sei contains several additional info, we do
 * only care for pic_timing, to handle display
 * reordering
 */
struct sei_message
{
  uint32_t payload_type;
  uint8_t last_payload_type_byte;
  uint32_t payload_size;
  uint8_t last_payload_size_byte;

  struct
  {
    /* cpb_dpb_delays_present_flag == 1 */
    uint8_t cpb_removal_delay;
    uint8_t dpb_output_delay;

    uint8_t pic_struct;
    uint8_t ct_type : 1;
    uint8_t nuit_field_based_flag : 1;
    uint8_t counting_type : 5;
    uint8_t full_timestamp_flag : 1;
    uint8_t discontinuity_flag : 1;
    uint8_t cnt_dropped_flag : 1;
    uint8_t n_frames;

    uint8_t seconds_value : 6;
    uint8_t minutes_value : 6;
    uint8_t hours_value : 5;

    int32_t time_offset;
  } pic_timing;
};

struct slice_header
{
  uint32_t first_mb_in_slice;
  uint32_t slice_type;
  uint32_t pic_parameter_set_id;
  uint8_t colour_plane_id;
  uint32_t frame_num;
  uint8_t field_pic_flag;
  uint8_t bottom_field_flag;
  uint32_t idr_pic_id;

  /* sps->pic_order_cnt_type == 0 */
  uint32_t pic_order_cnt_lsb;
  int32_t delta_pic_order_cnt_bottom;
  /* sps->pic_order_cnt_type == 1 && !sps->delta_pic_order_always_zero_flag */
  int32_t delta_pic_order_cnt[2];

  /* pps->redundant_pic_cnt_present_flag == 1 */
  int32_t redundant_pic_cnt;

  /* slice_type == B */
  uint8_t direct_spatial_mv_pred_flag;

  /* slice_type == P, SP, B */
  uint8_t num_ref_idx_active_override_flag;
  /* num_ref_idx_active_override_flag == 1 */
  uint32_t num_ref_idx_l0_active_minus1;
  /* slice type == B */
  uint32_t num_ref_idx_l1_active_minus1;

  /* ref_pic_list_reordering */
  struct
  {
    /* slice_type != I && slice_type != SI */
    uint8_t ref_pic_list_reordering_flag_l0;

    /* slice_type == B */
    uint8_t ref_pic_list_reordering_flag_l1;

    /* ref_pic_list_reordering_flag_l0 == 1 */
    uint32_t reordering_of_pic_nums_idc;

    /* reordering_of_pic_nums_idc == 0, 1 */
    uint32_t abs_diff_pic_num_minus1;

    /* reordering_of_pic_nums_idc == 2) */
    uint32_t long_term_pic_num;
  } ref_pic_list_reordering;

  /* pred_weight_table */
  struct
  {
    uint32_t luma_log2_weight_denom;

    /* chroma_format_idc != 0 */
    uint32_t chroma_log2_weight_denom;

    int32_t luma_weight_l0[32];
    int32_t luma_offset_l0[32];

    int32_t chroma_weight_l0[32][2];
    int32_t chroma_offset_l0[32][2];

    int32_t luma_weight_l1[32];
    int32_t luma_offset_l1[32];

    int32_t chroma_weight_l1[32][2];
    int32_t chroma_offset_l1[32][2];
  } pred_weight_table;

  /* def_rec_pic_marking */
  struct
  {

    /* nal_unit_type == NAL_SLICE_IDR */
    uint8_t no_output_of_prior_pics_flag;
    uint8_t long_term_reference_flag;

    /* else */
    uint8_t adaptive_ref_pic_marking_mode_flag;
    uint32_t memory_management_control_operation;

    uint32_t difference_of_pic_nums_minus1;
    uint32_t long_term_pic_num;
    uint32_t long_term_frame_idx;
    uint32_t max_long_term_frame_idx_plus1;
  } dec_ref_pic_marking[10];
  uint32_t dec_ref_pic_marking_count;
};

struct nal_unit {
    uint8_t nal_ref_idc; // 0x03
    enum nal_unit_types nal_unit_type; // 0x1f

    //union {
      struct sei_message sei;
      struct seq_parameter_set_rbsp sps;
      struct pic_parameter_set_rbsp pps;
      struct slice_header slc;
    //};

    struct nal_unit *prev;
    struct nal_unit *next;

    uint32_t lock_counter;
};

struct nal_buffer {
    struct nal_unit *first;
    struct nal_unit *last;

    uint8_t max_size;
    uint8_t used;
};

struct nal_buffer* create_nal_buffer(uint8_t max_size);
void free_nal_buffer(struct nal_buffer *nal_buffer);
void nal_buffer_append(struct nal_buffer *nal_buffer, struct nal_unit *nal);
void nal_buffer_remove(struct nal_buffer *nal_buffer, struct nal_unit *nal);
void nal_buffer_flush(struct nal_buffer *nal_buffer);

struct nal_unit* nal_buffer_get_by_sps_id(struct nal_buffer *nal_buffer,
    uint32_t seq_parameter_set_id);
struct nal_unit* nal_buffer_get_by_pps_id(struct nal_buffer *nal_buffer,
    uint32_t pic_parameter_set_id);
struct nal_unit* nal_buffer_get_last(struct nal_buffer *nal_buffer);

struct nal_unit* create_nal_unit();
void lock_nal_unit(struct nal_unit *nal);
void release_nal_unit(struct nal_unit *nal);
void copy_nal_unit(struct nal_unit *dest, struct nal_unit *src);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* NAL_H_ */
