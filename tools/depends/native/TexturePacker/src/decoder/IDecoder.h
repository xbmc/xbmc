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

#pragma once

#include <cstdint>
#include <string>
#include <vector>

/* forward declarations */

class DecodedFrame;
class DecodedFrames;

class IDecoder
{
  public:
    virtual ~IDecoder() = default;
    virtual bool CanDecode(const std::string &filename) = 0;
    virtual bool LoadFile(const std::string& filename, DecodedFrames& frames) = 0;
    virtual const char* GetImageFormatName() = 0;
    virtual const char* GetDecoderName() = 0;

    const std::vector<std::string>& GetSupportedExtensions() const { return m_extensions; }

  protected:
    std::vector<std::string> m_extensions;
};

class RGBAImage
{
public:
  RGBAImage() = default;

  std::vector<uint8_t> pixels;
  int width = 0; // width
  int height = 0; // height
  int bbp = 0; // bits per pixel
  int pitch = 0; // rowsize in bytes
};

class DecodedFrame
{
public:
  DecodedFrame() = default;
  RGBAImage rgbaImage; /* rgbaimage for this frame */
  int delay = 0; /* Frame delay in ms */
};

class DecodedFrames
{
  public:
    DecodedFrames() = default;
    std::vector<DecodedFrame> frameList;
};
