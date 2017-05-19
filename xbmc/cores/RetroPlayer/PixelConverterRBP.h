/*
 *      Copyright (C) 2016-2017 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "PixelConverter.h"

#include <memory>
#include <stdint.h>
#include <vector>

namespace MMAL {
  class CMMALPool;
  class CMMALYUVBuffer;
};

struct AVFrame;
struct VideoPicture;
struct SwsContext;

class CPixelConverterRBP : public CPixelConverter
{
public:
  CPixelConverterRBP();
  ~CPixelConverterRBP() override { Dispose(); }

  // implementation of IPixelConverter
  virtual bool Open(AVPixelFormat pixfmt, AVPixelFormat target, unsigned int width, unsigned int height) override;
  virtual void Dispose() override;
  virtual bool Decode(const uint8_t* pData, unsigned int size) override;
  virtual void GetPicture(VideoPicture& picture) override;

protected:
  struct PixelFormatTargetTable
  {
    AVPixelFormat pixfmt;
    AVPixelFormat targetfmt;
  };
  static std::vector<PixelFormatTargetTable> pixfmt_target_table;

  static AVPixelFormat TranslateTargetFormat(AVPixelFormat pixfmt);

  AVPixelFormat m_targetFormat = AV_PIX_FMT_NONE;
  SwsContext* m_swsContext = nullptr;
  MMAL::CMMALYUVBuffer *m_renderBuffer = nullptr;
  std::shared_ptr<MMAL::CMMALPool> m_pixelBufferPool;
};
