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

#include "IPixelConverter.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFormats.h"

#include <stdint.h>

struct VideoPicture;
struct SwsContext;

class CPixelConverter : public IPixelConverter
{
public:
  CPixelConverter();
  ~CPixelConverter() override { Dispose(); }

  // implementation of IPixelConverter
  bool Open(AVPixelFormat pixfmt, AVPixelFormat target, unsigned int width, unsigned int height) override;
  void Dispose() override;
  bool Decode(const uint8_t* pData, unsigned int size) override;
  void GetPicture(VideoPicture& dvdVideoPicture) override;

protected:
  //ERenderFormat m_renderFormat;
  unsigned int m_width;
  unsigned int m_height;
  SwsContext* m_swsContext;
  VideoPicture* m_buf;
};
