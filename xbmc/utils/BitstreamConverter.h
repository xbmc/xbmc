/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _BITSTREAMCONVERTER_H_
#define _BITSTREAMCONVERTER_H_

#include <stdint.h>

extern "C" {
#include "libavutil/avutil.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
#include "libavcodec/avcodec.h"
}

typedef struct {
  int       writer_le;
  uint32_t  bit_buf;
  int       bit_left;
  uint8_t   *buf, *buf_ptr, *buf_end;
  int       size_in_bits;
} bits_writer_t;

typedef struct {
  uint8_t *buffer, *start;
  int      offbits, length, oflow;
} bits_reader_t;

////////////////////////////////////////////////////////////////////////////////////////////
// TODO: refactor this so as not to need these ffmpeg routines.
// These are not exposed in ffmpeg's API so we dupe them here.
// AVC helper functions for muxers,
//  * Copyright (c) 2006 Baptiste Coudurier <baptiste.coudurier@smartjog.com>
// This is part of FFmpeg
//  * License as published by the Free Software Foundation; either
//  * version 2.1 of the License, or (at your option) any later version.
#define BS_RB16(x)                          \
  ((((const uint8_t*)(x))[0] <<  8) |        \
   ((const uint8_t*)(x)) [1])

#define BS_RB24(x)                          \
  ((((const uint8_t*)(x))[0] << 16) |        \
   (((const uint8_t*)(x))[1] <<  8) |        \
   ((const uint8_t*)(x))[2])

#define BS_RB32(x)                          \
  ((((const uint8_t*)(x))[0] << 24) |        \
   (((const uint8_t*)(x))[1] << 16) |        \
   (((const uint8_t*)(x))[2] <<  8) |        \
   ((const uint8_t*)(x))[3])

#define BS_WB32(p, d) { \
  ((uint8_t*)(p))[3] = (d); \
  ((uint8_t*)(p))[2] = (d) >> 8; \
  ((uint8_t*)(p))[1] = (d) >> 16; \
  ((uint8_t*)(p))[0] = (d) >> 24; }

#define BS_WL32(p, d) { \
  ((uint8_t*)(p))[0] = (d); \
  ((uint8_t*)(p))[1] = (d) >> 8; \
  ((uint8_t*)(p))[2] = (d) >> 16; \
  ((uint8_t*)(p))[3] = (d) >> 24; }

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
  static bool FindIdrSlice(const uint8_t *buf, int buf_size);

protected:
  static const uint8_t* find_start_code(const uint8_t *p, const uint8_t *end, uint32_t *state);
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

  static void       bits_reader_set( bits_reader_t *br, uint8_t *buf, int len );
  static uint32_t   read_bits( bits_reader_t *br, int nbits );
  static void       skip_bits( bits_reader_t *br, int nbits );
  static uint32_t   get_bits( bits_reader_t *br, int nbits );

  static void       init_bits_writer(bits_writer_t *s, uint8_t *buffer, int buffer_size, int writer_le);
  static void       write_bits(bits_writer_t *s, int n, unsigned int value);
  static void       skip_bits( bits_writer_t *s, int n);
  static void       flush_bits(bits_writer_t *s);

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
};

#endif
