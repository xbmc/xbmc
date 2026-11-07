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
  bool HasRecoveryPoint(const uint8_t* data, int size);
  bool BitstreamConvert(const uint8_t* pData, int iSize);
  bool ProcessAnnexB(uint8_t* data, int size);

  AVBSFContext* m_bitstreamFilter{nullptr};
  uint8_t m_nalLengthSize{0};
};
