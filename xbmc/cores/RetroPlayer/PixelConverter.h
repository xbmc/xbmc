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

#include <memory>
#include <stdint.h>

class CPixelBufferPoolFFmpeg;
struct AVFrame;
struct VideoPicture;
struct SwsContext;

class CPixelConverter : public IPixelConverter
{
public:
  CPixelConverter();
  virtual ~CPixelConverter() { Dispose(); }

  // implementation of IPixelConverter
  virtual bool Open(AVPixelFormat pixfmt, AVPixelFormat target, unsigned int width, unsigned int height) override;
  virtual void Dispose() override;
  virtual bool Decode(const uint8_t* pData, unsigned int size) override;
  virtual void GetPicture(VideoPicture& dvdVideoPicture) override;

protected:
  bool AllocateBuffers(AVFrame *pFrame) const;

  struct delete_buffer_pool
  {
    void operator()(CPixelBufferPoolFFmpeg *p) const;
  };

  AVPixelFormat m_targetFormat;
  unsigned int m_width;
  unsigned int m_height;
  SwsContext* m_swsContext;
  AVFrame *m_pFrame;
  std::unique_ptr<CPixelBufferPoolFFmpeg, delete_buffer_pool> m_pixelBufferPool;
};
