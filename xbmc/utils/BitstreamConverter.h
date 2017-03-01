#pragma once

/*
 *      Copyright (C) 2010-2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <stdint.h>

extern "C" {
#include "libavutil/avutil.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
#include "libavcodec/avcodec.h"
}

typedef struct
{
  const uint8_t *data;
  const uint8_t *end;
  int head;
  uint64_t cache;
} nal_bitstream;

typedef struct mpeg2_sequence
{
  uint32_t  width;
  uint32_t  height;
  float     rate;
  uint32_t  rate_info;
  float     ratio;
  uint32_t  ratio_info;
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
  ~CBitstreamParser();

  static bool Open();
  static void Close();
  static bool CanStartDecode(const uint8_t *buf, int buf_size);
};

class CBitstreamConverter
{
public:
  CBitstreamConverter();
  ~CBitstreamConverter();

  bool              Open(enum AVCodecID codec, uint8_t *in_extradata, int in_extrasize, bool to_annexb);
  void              Close(void);
  bool              NeedConvert(void) const { return m_convert_bitstream; };
  bool              Convert(uint8_t *pData, int iSize);
  uint8_t*          GetConvertBuffer(void) const;
  int               GetConvertSize() const;
  uint8_t*          GetExtraData(void) const;
  int               GetExtraSize() const;
  void              ResetStartDecode(void);
  bool              CanStartDecode() const;

  static void       parseh264_sps(const uint8_t *sps, const uint32_t sps_size, bool *interlaced, int32_t *max_ref_frames);
  static bool       mpeg2_sequence_header(const uint8_t *data, const uint32_t size, mpeg2_sequence *sequence);

protected:
  static const int  avc_parse_nal_units(AVIOContext *pb, const uint8_t *buf_in, int size);
  static const int  avc_parse_nal_units_buf(const uint8_t *buf_in, uint8_t **buf, int *size);
  const int         isom_write_avcc(AVIOContext *pb, const uint8_t *data, int len);
  // bitstream to bytestream (Annex B) conversion support.
  bool              IsIDR(uint8_t unit_type);
  bool              IsSlice(uint8_t unit_type);
  bool              BitstreamConvertInitAVC(void *in_extradata, int in_extrasize);
  bool              BitstreamConvertInitHEVC(void *in_extradata, int in_extrasize);
  bool              BitstreamConvert(uint8_t* pData, int iSize, uint8_t **poutbuf, int *poutbuf_size);
  static void       BitstreamAllocAndCopy(uint8_t **poutbuf, int *poutbuf_size,
                      const uint8_t *sps_pps, uint32_t sps_pps_size, const uint8_t *in, uint32_t in_size);

  typedef struct omx_bitstream_ctx {
      uint8_t  length_size;
      uint8_t  first_idr;
      uint8_t  idr_sps_pps_seen;
      uint8_t *sps_pps_data;
      uint32_t size;
  } omx_bitstream_ctx;

  uint8_t          *m_convertBuffer;
  int               m_convertSize;
  uint8_t          *m_inputBuffer;
  int               m_inputSize;

  uint32_t          m_sps_pps_size;
  omx_bitstream_ctx m_sps_pps_context;
  bool              m_convert_bitstream;
  bool              m_to_annexb;

  uint8_t          *m_extradata;
  int               m_extrasize;
  bool              m_convert_3byteTo4byteNALSize;
  bool              m_convert_bytestream;
  AVCodecID         m_codec;
  bool              m_start_decode;
};
