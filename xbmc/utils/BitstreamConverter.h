/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/FFmpeg.h"

#include <stdint.h>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavcodec/bsf.h>

#ifdef HAVE_LIBDOVI
#include <libdovi/rpu_parser.h>
#endif
}

typedef struct
{
  const uint8_t* data;
  const uint8_t* end;
  int head;
  uint64_t cache;
} nal_bitstream;

typedef struct mpeg2_sequence
{
  uint32_t width;
  uint32_t height;
  uint32_t fps_rate;
  uint32_t fps_scale;
  float ratio;
  uint32_t ratio_info;
} mpeg2_sequence;

typedef struct
{
  int profile_idc;
  int level_idc;
  int sps_id;

  int chroma_format_idc;
  int separate_colour_plane_flag;
  int bit_depth_luma_minus8;
  int bit_depth_chroma_minus8;
  int qpprime_y_zero_transform_bypass_flag;
  int seq_scaling_matrix_present_flag;

  int log2_max_frame_num_minus4;
  int pic_order_cnt_type;
  int log2_max_pic_order_cnt_lsb_minus4;

  int max_num_ref_frames;
  int gaps_in_frame_num_value_allowed_flag;
  int pic_width_in_mbs_minus1;
  int pic_height_in_map_units_minus1;

  int frame_mbs_only_flag;
  int mb_adaptive_frame_field_flag;

  int direct_8x8_inference_flag;

  int frame_cropping_flag;
  int frame_crop_left_offset;
  int frame_crop_right_offset;
  int frame_crop_top_offset;
  int frame_crop_bottom_offset;
} sps_info_struct;

class CBitstreamParser
{
public:
  CBitstreamParser();
  ~CBitstreamParser() = default;

  static bool Open() { return true; }
  static void Close();
  static bool CanStartDecode(const uint8_t* buf, int buf_size);
};

class CBitstreamConverter
{
public:
  CBitstreamConverter();
  ~CBitstreamConverter();

  bool Open(enum AVCodecID codec, const uint8_t* in_extradata, int in_extrasize);
  void Close();
  bool NeedConvert() const { return m_convert_bitstream; }
  bool Convert(uint8_t* pData, int iSize);
  uint8_t* GetConvertBuffer() const;
  int GetConvertSize() const;
  uint8_t* GetExtraData();
  const uint8_t* GetExtraData() const;
  int GetExtraSize() const;
  void ResetStartDecode();
  bool CanStartDecode() const;
  void SetConvertDovi(bool value) { m_convert_dovi = value; }
  void SetRemoveDovi(bool value) { m_removeDovi = value; }
  void SetRemoveHdr10Plus(bool value) { m_removeHdr10Plus = value; }
  void SetDoviZeroLevel5(bool value) { m_setDoviZeroLevel5 = value; }

  static bool mpeg2_sequence_header(const uint8_t* data,
                                    const uint32_t size,
                                    mpeg2_sequence* sequence);

protected:
  // bitstream to bytestream (Annex B) conversion support.
  bool IsIDR(uint8_t unit_type);

#ifdef HAVE_LIBDOVI
  const DoviData* processDoviRpu(uint8_t* buf, uint32_t nalSize);
#endif

  uint8_t* m_convertBuffer;
  int m_convertSize;

  bool m_convert_bitstream;

  FFmpegExtraData m_extraData;
  AVCodecID m_codec;
  bool m_start_decode;
  bool m_convert_dovi;
  bool m_removeDovi;
  bool m_removeHdr10Plus;
  bool m_setDoviZeroLevel5;

private:
  bool BitstreamConvertInit(const char* filterName, const uint8_t* extraData, int extraDataSize);
  bool BitstreamConvert(const uint8_t* pData, int iSize);
  bool ProcessAnnexB(uint8_t* data, int size);

  AVBSFContext* m_bitstreamFilter{nullptr};
};
