/*
 *      Copyright (C) 2014 Team Kodi
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <cstring>
#include "GIFDecoder.h"
#include "GifHelper.h"

// returns true for gif files, otherwise returns false
bool GIFDecoder::CanDecode(const std::string &filename)
{
  return std::string::npos != filename.rfind(".gif",filename.length() - 4, 4);
}

bool GIFDecoder::LoadFile(const std::string &filename, DecodedFrames &frames)
{
  int n = 0;
  bool result = false;

  GifHelper *gifImage = new GifHelper();
  if (gifImage->LoadGif(filename.c_str()))
  {
    auto extractedFrames = gifImage->GetFrames();
    n = extractedFrames.size();
    if (n > 0)
    {
      unsigned int height = gifImage->GetHeight();
      unsigned int width = gifImage->GetWidth();
      unsigned int pitch = gifImage->GetPitch();
      unsigned int frameSize = pitch * height;
      for (unsigned int i = 0; i < extractedFrames.size(); i++)
      {
        DecodedFrame frame;

        frame.rgbaImage.pixels = (char *)new char[frameSize];
        memcpy(frame.rgbaImage.pixels, extractedFrames[i]->m_pImage, frameSize);
        frame.rgbaImage.height = height;
        frame.rgbaImage.width = width;
        frame.rgbaImage.bbp = 32;
        frame.rgbaImage.pitch = pitch;
        frame.delay = extractedFrames[i]->m_delay;
        frame.decoder = this;

        frames.frameList.push_back(frame);
      }
    }
    result = true;
  }
  delete gifImage;
  return result;
}

void GIFDecoder::FreeDecodedFrame(DecodedFrame &frame)
{
  delete [] frame.rgbaImage.pixels;
}

void GIFDecoder::FillSupportedExtensions()
{
  m_supportedExtensions.push_back(".gif");
}
