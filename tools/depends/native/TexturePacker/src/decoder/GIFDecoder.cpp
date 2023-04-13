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

#include "GIFDecoder.h"

#include "GifHelper.h"

#include <cstring>

GIFDecoder::GIFDecoder()
{
  m_extensions.emplace_back(".gif");
}

// returns true for gif files, otherwise returns false
bool GIFDecoder::CanDecode(const std::string &filename)
{
  return std::string::npos != filename.rfind(".gif",filename.length() - 4, 4);
}

bool GIFDecoder::LoadFile(const std::string &filename, DecodedFrames &frames)
{
  int n = 0;
  bool result = false;

  GifHelper gifImage;
  if (gifImage.LoadGif(filename))
  {
    auto extractedFrames = gifImage.GetFrames();
    n = extractedFrames.size();
    if (n > 0)
    {
      unsigned int height = gifImage.GetHeight();
      unsigned int width = gifImage.GetWidth();
      unsigned int pitch = gifImage.GetPitch();

      for (unsigned int i = 0; i < extractedFrames.size(); i++)
      {
        DecodedFrame frame;

        frame.rgbaImage.pixels = extractedFrames[i]->m_pImage;
        frame.rgbaImage.height = height;
        frame.rgbaImage.width = width;
        frame.rgbaImage.bbp = 32;
        frame.rgbaImage.pitch = pitch;
        frame.delay = extractedFrames[i]->m_delay;

        frames.frameList.push_back(frame);
      }
    }
    result = true;
  }

  return result;
}
