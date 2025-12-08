/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDCodecUtils.h"

#include "cores/FFmpeg.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "utils/log.h"

#include <array>
#include <assert.h>

extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/intreadwrite.h>
}

bool CDVDCodecUtils::IsVP3CompatibleWidth(int width)
{
  // known hardware limitation of purevideo 3 (VP3). (the Nvidia 9400 is a purevideo 3 chip)
  // from nvidia's linux vdpau README: All current third generation PureVideo hardware
  // (G98, MCP77, MCP78, MCP79, MCP7A) cannot decode H.264 for the following horizontal resolutions:
  // 769-784, 849-864, 929-944, 1009–1024, 1793–1808, 1873–1888, 1953–1968 and 2033-2048 pixel.
  // This relates to the following macroblock sizes.
  int unsupported[] = {49, 54, 59, 64, 113, 118, 123, 128};
  for (int u : unsupported)
  {
    if (u == (width + 15) / 16)
      return false;
  }
  return true;
}

double CDVDCodecUtils::NormalizeFrameduration(double frameduration, bool *match)
{
  //if the duration is within 20 microseconds of a common duration, use that
  // clang-format off
  constexpr std::array<double, 8> durations = {
    DVD_TIME_BASE * 1.001 / 24.0,
    DVD_TIME_BASE / 24.0,
    DVD_TIME_BASE / 25.0,
    DVD_TIME_BASE * 1.001 / 30.0,
    DVD_TIME_BASE / 30.0,
    DVD_TIME_BASE / 50.0,
    DVD_TIME_BASE * 1.001 / 60.0,
    DVD_TIME_BASE / 60.0
  };
  // clang-format on

  double lowestdiff = DVD_TIME_BASE;
  int    selected   = -1;
  for (size_t i = 0; i < durations.size(); i++)
  {
    double diff = fabs(frameduration - durations[i]);
    if (diff < DVD_MSEC_TO_TIME(0.02) && diff < lowestdiff)
    {
      selected = i;
      lowestdiff = diff;
    }
  }

  if (selected != -1)
  {
    if (match)
      *match = true;
    return durations[selected];
  }
  else
  {
    if (match)
      *match = false;
    return frameduration;
  }
}

bool CDVDCodecUtils::IsH264AnnexB(std::string format, AVStream *avstream)
{
  assert(avstream->codecpar->codec_id == AV_CODEC_ID_H264 || avstream->codecpar->codec_id == AV_CODEC_ID_H264_MVC);
  if (avstream->codecpar->extradata_size < 4)
    return true;
  if (avstream->codecpar->extradata[0] == 1)
    return false;
  if (format == "avi")
  {
    uint8_t *src = avstream->codecpar->extradata;
    unsigned startcode = AV_RB32(src);
    if (startcode == 0x00000001 || (startcode & 0xffffff00) == 0x00000100)
      return true;
    if (avstream->codecpar->codec_tag == MKTAG('A', 'V', 'C', '1') || avstream->codecpar->codec_tag == MKTAG('a', 'v', 'c', '1'))
      return false;
  }
  return true;
}

bool CDVDCodecUtils::ProcessH264MVCExtradata(uint8_t *data, uint32_t data_size, uint8_t **mvc_data, uint32_t *mvc_data_size)
{
  uint8_t* extradata = data;
  uint32_t extradata_size = data_size;

  if (extradata_size > 4 && *(char *)extradata == 1)
  {
    // Find "mvcC" atom
    uint32_t state = -1;
    uint32_t i = 0;
    for (; i < extradata_size; i++)
    {
      state = (state << 8) | extradata[i];
      if (state == MKBETAG('m', 'v', 'c', 'C'))
        break;
    }
    if (i >= 8 && i < extradata_size)
    {
      // Update pointers to the start of the mvcC atom
      extradata = extradata + i - 7;
      extradata_size = extradata_size - i + 7;
      // verify size atom and actual size
      if (extradata_size >= 14 && (AV_RB32(extradata) + 4) <= extradata_size)
      {
        extradata += 8;
        extradata_size -= 8;
        if (*(char *)extradata == 1)
        {
          if (mvc_data)
            *mvc_data = extradata;
          if (mvc_data_size)
            *mvc_data_size = extradata_size;
          return true;
        }
      }
    }
  }
  return false;
}

bool CDVDCodecUtils::GetH264MvcStreamIndex(AVFormatContext *fmt, int *mvcIndex)
{
  *mvcIndex = -1;

  for (size_t i = 0; i < fmt->nb_streams; i++)
  {
    AVStream *st = fmt->streams[i];

    if (st->codecpar->codec_id == AV_CODEC_ID_H264_MVC)
    {
      if (*mvcIndex != -1)
      {
        CLog::Log(LOGDEBUG, "multiple h264 mvc extension streams aren't supported");
        return false;
      }

      *mvcIndex = i;
    }
  }

  return *mvcIndex >= 0;
}
