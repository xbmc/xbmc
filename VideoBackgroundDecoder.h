#pragma once
/*
 *      Copyright (C) 2005-2017 Team XBMC
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

#include <string>
#include <stdint.h>

extern "C"
{
#include "libavutil/pixfmt.h"
}

struct AVFormatContext;
struct AVCodecContext;
struct SwsContext;
struct AVFrame;

class CVideoBackgroundDecoder
{
public:
  CVideoBackgroundDecoder();
  ~CVideoBackgroundDecoder();

  bool Open(const std::string& filename);
  void Close();
  bool IsOpen() const;
  bool Update(unsigned int currentTimeMs);
  const uint8_t* GetCurrentFrame(int& width, int& height) const;

private:
  bool DecodeNextFrame();
  void SeekToStart();

  AVFormatContext* m_fmtCtx;
  AVCodecContext* m_codecCtx;
  SwsContext* m_swsCtx;
  AVFrame* m_avFrame;
  AVFrame* m_rgbFrame;
  uint8_t* m_rgbBuffer;
  int m_videoStream;
  int m_width;
  int m_height;
  double m_timeBase;
  unsigned int m_nextFrameMs;
  bool m_isOpen;
};
