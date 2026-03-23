/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DoViRpuWriter.h"

#include "BitstreamIoWriter.h"
#include "Crc32.h"
#include "HDR10PlusConvert.h"
#include "HDR10Plus.h"
#include "HevcSei.h"

void write_dovi_rpu_payload(BitstreamIoWriter& writer, VdrDmData& vdr_dm_data) {

    writer.write_n<uint8_t>(2, 6);      // 6   (000010)         rpu_type           2 for the BDA Ultra HD Blu-ray Option-A HDR coding system
    writer.write_n<uint16_t>(18, 11);   // 17  (00000010010)    rpu_format         18
    writer.write_n<uint8_t>(1, 4);      // 21  (0001)           vdr_rpu_profile
    writer.write_n<uint8_t>(0, 4);      // 25  (0000)           vdr_rpu_level
    writer.write(true);                 // 26  (1)              vdr_seq_info_present_flag
    writer.write(false);                // 27  (0)              chroma_resampling_explicit_filter_flag
    writer.write_n<uint8_t>(0, 2);      // 29  (00)             coefficient_data_type
    writer.write_ue(23);                // 38  (000011000)      coefficient_log2_denom  23
    writer.write_n<uint8_t>(1, 2);      // 40  (01)             vdr_rpu_normalized_idc

    writer.write(false);                // 41  (0)              bl_video_full_range_flag
    writer.write_ue(2);                 // 44  (011)            bl_bit_depth_minus8
    writer.write_ue(2);                 // 47  (011)            el_bit_depth_minus8
    writer.write_ue(4);                 // 52  (00101)          vdr_bit_depth_minus8
    writer.write(false);                // 53  (0)              spatial_resampling_filter_flag
    writer.write_n<uint8_t>(0, 3);      // 56  (000)            reserved_zero_3bits

    writer.write(false);                // 57  (0)              el_spatial_resampling_filter_flag
    writer.write(true);                 // 58  (1)              disable_residual_flag

    writer.write(true);                 // 59  (1)              vdr_dm_metadata_present_flag
    writer.write(false);                // 60  (0)              use_prev_vdr_rpu_flag

    writer.write_ue(0);                 // 61  (1)              vdr_rpu_id

    writer.write_ue(0);                 // 62  (1)              mapping_color_space
    writer.write_ue(0);                 // 63  (1)              mapping_chroma_format_idc

    // 1.
    writer.write_ue(0);                 // 64  (1)              num_pivots_minus2
    writer.write_n<uint16_t>(0, 10);    // 74  (0000000000)     pivots[0]
    writer.write_n<uint16_t>(1023, 10); // 84  (1111111111)     pivots[1]

    // 2.
    writer.write_ue(0);                 // 85  (1)              num_pivots_minus2
    writer.write_n<uint16_t>(0, 10);    // 95  (0000000000)     pivots[0]
    writer.write_n<uint16_t>(1023, 10); // 105 (1111111111)     pivots[1]

    // 3.
    writer.write_ue(0);                 // 106 (1)              num_pivots_minus2
    writer.write_n<uint16_t>(0, 10);    // 116 (0000000000)     pivots[0]
    writer.write_n<uint16_t>(1023, 10); // 126 (1111111111)     pivots[1]

    writer.write_ue(0);                 // 127 (1)              num_x_partitions_minus1
    writer.write_ue(0);                 // 128 (1)              num_y_partitions_minus1

    // 1.
    writer.write_ue(0);                 // 129 (1)              mapping_idc Polynomial = 0
    writer.write_ue(0);                 // 130 (1)              poly_order_minus1
    writer.write(false);                // 131 (0)              linear_interp_flag
    writer.write_se(0);                 // 132 (1)              poly_coef_int 0
    writer.write_n<uint64_t>(0, 23);    // 155 (00000000000000000000000)    poly_coef :: coefficient_log2_denom_length
    writer.write_se(1);                 // 158 (010)            poly_coef_int 1
    writer.write_n<uint64_t>(0, 23);    // 181 (00000000000000000000000)    poly_coef :: coefficient_log2_denom_length

    // 152 OK.

    // 2.
    writer.write_ue(0);                 // 182 (1)              mapping_idc Polynomial = 0
    writer.write_ue(0);                 // 183 (1)              poly_order_minus1
    writer.write(false);                // 184 (0)              linear_interp_flag
    writer.write_se(0);                 // 185 (1)              poly_coef_int 0
    writer.write_n<uint64_t>(0, 23);    // 208 (00000000000000000000000)    poly_coef :: coefficient_log2_denom_length
    writer.write_se(1);                 // 211 (010)            poly_coef_int 1
    writer.write_n<uint64_t>(0, 23);    // 234 (00000000000000000000000)    poly_coef :: coefficient_log2_denom_length

    // 3.
    writer.write_ue(0);                 // 235 (1)              mapping_idc Polynomial = 0
    writer.write_ue(0);                 // 236 (1)              poly_order_minus1
    writer.write(false);                // 237 (0)              linear_interp_flag
    writer.write_se(0);                 // 238 (1)              poly_coef_int 0
    writer.write_n<uint64_t>(0, 23);    // 261 (00000000000000000000000)    poly_coef :: coefficient_log2_denom_length
    writer.write_se(1);                 // 264 (010)            poly_coef_int 1
    writer.write_n<uint64_t>(0, 23);    // 287 (00000000000000000000000)    poly_coef :: coefficient_log2_denom_length

    writer.write_ue(0);                 // 288 (1)              affected_dm_metadata_id
    writer.write_ue(0);                 // 289 (1)              current_dm_metadata_id
    writer.write_ue(1);                 // 292 (010)            scene_refresh_flag

    writer.write_signed_n<int16_t>( 9574, 16);  // (0010010101101100)  ycc_to_rgb_coef0
    writer.write_signed_n<int16_t>(    0, 16);  // (0000000000000000)  ycc_to_rgb_coef1
    writer.write_signed_n<int16_t>(13802, 16);  // (0011010111101010)  ycc_to_rgb_coef2
    writer.write_signed_n<int16_t>( 9574, 16);  // (0010010101101100)  ycc_to_rgb_coef3
    writer.write_signed_n<int16_t>(-1540, 16);  // (1111101000000100)  ycc_to_rgb_coef4
    writer.write_signed_n<int16_t>(-5348, 16);  // (1110101100011100)  ycc_to_rgb_coef5
    writer.write_signed_n<int16_t>( 9574, 16);  // (0010010101101100)  ycc_to_rgb_coef6
    writer.write_signed_n<int16_t>(17610, 16);  // (0100010010111010)  ycc_to_rgb_coef7
    writer.write_signed_n<int16_t>(    0, 16);  // (0000000000000000)  ycc_to_rgb_coef8

    writer.write_n<uint32_t>( 16777216, 32);    // (00000001000000000000000000000000) ycc_to_rgb_offset0
    writer.write_n<uint32_t>(134217728, 32);    // (00001000000000000000000000000000) ycc_to_rgb_offset1
    writer.write_n<uint32_t>(134217728, 32);    // (000010000000000000000000000000000 ycc_to_rgb_offset2

    writer.write_signed_n<int16_t>( 7222, 16);  // (0001110000110110) rgb_to_lms_coef0
    writer.write_signed_n<int16_t>( 8771, 16);  // (0010001001000011) rgb_to_lms_coef1
    writer.write_signed_n<int16_t>(  390, 16);  // (0000000110000110) rgb_to_lms_coef2
    writer.write_signed_n<int16_t>( 2654, 16);  // (0000101001011110) rgb_to_lms_coef3
    writer.write_signed_n<int16_t>(12430, 16);  // (0011000010001110) rgb_to_lms_coef4
    writer.write_signed_n<int16_t>( 1300, 16);  // (0000010100010100) rgb_to_lms_coef5
    writer.write_signed_n<int16_t>(    0, 16);  // (0000000000000000) rgb_to_lms_coef6
    writer.write_signed_n<int16_t>(  422, 16);  // (0000000110100110) rgb_to_lms_coef7
    writer.write_signed_n<int16_t>(15962, 16);  // (0011111001011010) rgb_to_lms_coef8

    writer.write_n<uint16_t>(65535, 16);        // (1111111111111111) signal_eotf                           Ultra-HD Blu-ray
    writer.write_n<uint16_t>(0, 16);            // (0000000000000000) signal_eotf_param0                    Ultra-HD Blu-ray
    writer.write_n<uint16_t>(0, 16);            // (0000000000000000) signal_eotf_param1                    Ultra-HD Blu-ray
    writer.write_n<uint32_t>(0, 32);            // (00000000000000000000000000000000) signal_eotf_param2    Ultra-HD Blu-ray

    writer.write_n<uint8_t>(12, 5);             // (01100)  signal_bit_depth
    writer.write_n<uint8_t>(0, 2);              // (00)     signal_color_space       YCbCr
    writer.write_n<uint8_t>(0, 2);              // (00)     signal_chroma_format     4:2:0 ?
    writer.write_n<uint8_t>(1, 2);              // (01)     signal_full_range_flag   Full Range

    writer.write_n<uint16_t>(vdr_dm_data.source_min_pq, 12);  // source min pq
    writer.write_n<uint16_t>(vdr_dm_data.source_max_pq, 12);  // source max pq

    writer.write_n<uint16_t>(42, 10);           // (0000101010) source_diagonal (display diagonal in inches - TODO: Any effect?)

    // Dolby Vision bitstream layout expects two sequential extension
    // metadata payloads: CM v2.9 first, then (optionally) CM v4.0.

    // CM v2.9 extension metadata (allowed levels: 1, 2, 4, 5, 6, 255)
    // ---------------------------------------------------------------
    writer.write_ue(3);                         // (00100) num_ext_blocks
    writer.byte_align();                        // dm_alignment_zero_bit

    // L1 ----------- (53 bits)
    writer.write_ue(5);                         // (00110)          length_bytes (payload only)
    writer.write_n<uint8_t>(1, 8);              // (00000001)       level
    writer.write_n<uint16_t>(vdr_dm_data.min_pq, 12);
    writer.write_n<uint16_t>(vdr_dm_data.max_pq, 12);
    writer.write_n<uint16_t>(vdr_dm_data.avg_pq, 12);
    writer.write_n<uint8_t>(0, 4);              // (0000)           alignment of 4 bits. (40)

    // L5 ----------- (71 bits)
    writer.write_ue(7);                         // (0001000)        length_bytes (payload only)
    writer.write_n<uint8_t>(5, 8);              // (00000101)       level
    writer.write_n<uint16_t>(0, 13);            // (0000000000000)  active_area_left_offset
    writer.write_n<uint16_t>(0, 13);            // (0000000000000)  active_area_right_offset
    writer.write_n<uint16_t>(0, 13);            // (0000000000000)  active_area_top_offset
    writer.write_n<uint16_t>(0, 13);            // (0000000000000)  active_area_bottom_offset
    writer.write_n<uint8_t>(0, 4);              // (0000)           alignment of 4 bits. (56)

    // L6 ----------- (79 bits)
    writer.write_ue(8);                         // (0001001)        length_bytes (payload only)
    writer.write_n<uint8_t>(6, 8);              // (00000110)       level
    writer.write_n<uint16_t>(vdr_dm_data.max_display_mastering_luminance, 16);
    writer.write_n<uint16_t>(vdr_dm_data.min_display_mastering_luminance, 16);
    writer.write_n<uint16_t>(vdr_dm_data.max_content_light_level, 16);
    writer.write_n<uint16_t>(vdr_dm_data.max_frame_average_light_level, 16);

    // CM v4.0 extension metadata (allowed levels: 3, 8, 9, 10, 11, 254)
    // -----------------------------------------------------------------
    writer.write_ue(4);                         // (00101) num_ext_blocks
    writer.byte_align();                        // dm_alignment_zero_bit

    // L3 ------------ (53 bits)
    writer.write_ue(5);                         // (00110)          length_bytes (payload only)
    writer.write_n<uint8_t>(3, 8);              // (00000011)       level
    writer.write_n<uint16_t>(2048, 12);         // (100000000000)   min_pq_offset
    writer.write_n<uint16_t>(2048, 12);         // (100000000000)   max_pq_offset
    writer.write_n<uint16_t>(2048, 12);         // (100000000000)   avg_pq_offset
    writer.write_n<uint8_t>(0, 4);              // (0000)           alignment of 4 bits. (40)

    // L9 ------------ (19 bits)
    writer.write_ue(1);                         // (010)            length_bytes (payload only)
    writer.write_n<uint8_t>(9, 8);              // (00001001)       level
    writer.write_n<uint8_t>(0, 8);              // (00000000)       source_primary_index

    // L11 ----------- (45 bits)
    writer.write_ue(4);                         // (00101)          length_bytes (payload only)
    writer.write_n<uint8_t>(11, 8);             // (00001011)       level
    writer.write_n<uint8_t>(1, 8);              // (00000001)       content_type
    writer.write_n<uint8_t>(0, 8);              // (00000000)       whitepoint
    writer.write_n<uint8_t>(0, 8);              // (00000000)       reserved_byte2
    writer.write_n<uint8_t>(0, 8);              // (00000000)       reserved_byte3

    // L254 ---------- (27 bits)
    writer.write_ue(2);                         // (011)            length_bytes (payload only)
    writer.write_n<uint8_t>(254, 8);            // (11111110)       level
    writer.write_n<uint8_t>(0, 8);              // (00000000)       dm_mode
    writer.write_n<uint8_t>(2, 8);              // (00000010)       dm_version_index

    writer.byte_align();                        // ext_dm_alignment_zero_bit
};

std::vector<uint8_t> create_dovi_rpu_nalu(VdrDmData& vdr_dm_data) {

  // Dolby Vision profile 8.1 (CMv2.9 133 Bytes long | CMv4.0 153 Bytes long)
  BitstreamIoWriter writer(153);

  writer.write_n<uint8_t>(0x19, 8);  // RPU prefix
  write_dovi_rpu_payload(writer, vdr_dm_data);
  writer.write_n<uint32_t>(Crc32::Compute(writer.as_slice() + 1, writer.as_slice_size() - 1), 32);
  writer.write_n<uint8_t>(0x80, 8);  // FINAL_BYTE

  std::vector<uint8_t> out = writer.into_inner();

  HevcAddStartCodeEmulationPrevention3Byte(out);

  // Add NAL unit type
  out.insert(out.begin(), 0x01);
  out.insert(out.begin(), 0x7C);

  return out;
}
