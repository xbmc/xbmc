#pragma once
/*
*      Copyright (C) 2012-2015 Team Kodi
*      http://kodi.tv
*
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

#include "iimage.h"

extern "C"
{
#include "libavutil/pixfmt.h"
}

struct AVFrame;
struct AVPicture;
struct AVIOContext;
struct AVCodecContext;

class CFFmpegImage : public IImage
{
public:
  CFFmpegImage(const std::string& strMimeType);
  virtual ~CFFmpegImage();

  virtual bool LoadImageFromMemory(unsigned char* buffer, unsigned int bufSize,
                                   unsigned int width, unsigned int height);
  virtual bool Decode(unsigned char * const pixels, unsigned int width, unsigned int height,
                      unsigned int pitch, unsigned int format);
  virtual bool CreateThumbnailFromSurface(unsigned char* bufferin, unsigned int width,
                                          unsigned int height, unsigned int format,
                                          unsigned int pitch, const std::string& destFile, 
                                          unsigned char* &bufferout,
                                          unsigned int &bufferoutSize);
  virtual void ReleaseThumbnailBuffer();
private:
  static void FreeIOCtx(AVIOContext* ioctx);
  static AVPixelFormat ConvertFormats(AVFrame* frame);
  std::string m_strMimeType;
  void CleanupLocalOutputBuffer();

  AVFrame* m_pFrame;
  uint8_t* m_outputBuffer;
};
