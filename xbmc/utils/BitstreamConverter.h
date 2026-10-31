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
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavcodec/avcodec.h>

#ifdef HAVE_LIBDOVI
#include <libdovi/rpu_parser.h>
#endif
}

typedef struct mpeg2_sequence
{
  uint32_t width;
  uint32_t height;
  uint32_t fps_rate;
  uint32_t fps_scale;
  float ratio;
  uint32_t ratio_info;
} mpeg2_sequence;

class CBitstreamConverter
{
public:
  CBitstreamConverter();
  ~CBitstreamConverter();

  bool Open(enum AVCodecID codec, uint8_t* in_extradata, int in_extrasize, bool to_annexb);
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
  static int avc_parse_nal_units(AVIOContext* pb, const uint8_t* buf_in, int size);
  static int avc_parse_nal_units_buf(const uint8_t* buf_in, uint8_t** buf, int* size);
  int isom_write_avcc(AVIOContext* pb, const uint8_t* data, int len);
  // bitstream to bytestream (Annex B) conversion support.
  bool IsIDR(uint8_t unit_type);
  bool IsSlice(uint8_t unit_type);
  bool BitstreamConvertInitAVC(void* in_extradata, int in_extrasize);
  bool BitstreamConvertInitHEVC(void* in_extradata, int in_extrasize);
  bool BitstreamConvert(uint8_t* pData, int iSize, uint8_t** poutbuf, int* poutbuf_size);
  static void BitstreamAllocAndCopy(uint8_t** poutbuf,
                                    int* poutbuf_size,
                                    const uint8_t* sps_pps,
                                    uint32_t sps_pps_size,
                                    const uint8_t* in,
                                    uint32_t in_size,
                                    uint8_t nal_type);

#ifdef HAVE_LIBDOVI
  const DoviData* processDoviRpu(uint8_t* buf, uint32_t nalSize);
#endif

  typedef struct omx_bitstream_ctx {
      uint8_t  length_size;
      uint8_t  first_idr;
      uint8_t  idr_sps_pps_seen;
      uint8_t *sps_pps_data;
      uint32_t size;
  } omx_bitstream_ctx;

  uint8_t* m_convertBuffer;
  int m_convertSize;
  uint8_t* m_inputBuffer;
  int m_inputSize;

  uint32_t m_sps_pps_size;
  omx_bitstream_ctx m_sps_pps_context;
  bool m_convert_bitstream;
  bool m_to_annexb;

  FFmpegExtraData m_extraData;
  bool m_convert_3byteTo4byteNALSize;
  bool m_convert_bytestream;
  AVCodecID m_codec;
  bool m_start_decode;
  bool m_convert_dovi;
  bool m_removeDovi;
  bool m_removeHdr10Plus;
  bool m_setDoviZeroLevel5;
};
