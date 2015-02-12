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
  GifHelper *gifImage = new GifHelper();
  if (gifImage->LoadGif(filename.c_str()))
  {
    std::vector<GifFrame> extractedFrames = gifImage->GetFrames();
    n = extractedFrames.size();
    if (n > 0)
    {
      for (unsigned int i = 0; i < extractedFrames.size(); i++)
      {
        DecodedFrame frame;
        
        frame.rgbaImage.pixels = (char *)new char[extractedFrames[i].m_imageSize];
        memcpy(frame.rgbaImage.pixels, extractedFrames[i].m_pImage, extractedFrames[i].m_imageSize);
        frame.rgbaImage.height = extractedFrames[i].m_height;
        frame.rgbaImage.width = extractedFrames[i].m_width;
        frame.rgbaImage.bbp = 32;
        frame.rgbaImage.pitch = extractedFrames[i].m_pitch;

        frame.delay = extractedFrames[i].m_delay;
        frame.disposal = extractedFrames[i].m_disposal;
        frame.x = extractedFrames[i].m_left;
        frame.y = extractedFrames[i].m_top;

        
        frames.frameList.push_back(frame);
      }
    }
    frames.user = gifImage;
    return true;
  }
  else
  {
    delete gifImage;
    return false;
  }
}

void GIFDecoder::FreeDecodedFrames(DecodedFrames &frames)
{
  for (unsigned int i = 0; i < frames.frameList.size(); i++)
  {
    delete [] frames.frameList[i].rgbaImage.pixels;
  }
  delete (GifHelper *)frames.user;
  frames.clear();
}

void GIFDecoder::FillSupportedExtensions()
{
  m_supportedExtensions.push_back(".gif");
}
