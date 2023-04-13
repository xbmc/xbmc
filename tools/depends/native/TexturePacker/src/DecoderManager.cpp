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
#include <cstdio>
#include "DecoderManager.h"

// ADD new decoders here
// include decoders
#include "PNGDecoder.h"
#include "JPGDecoder.h"
#include "GIFDecoder.h"

DecoderManager::DecoderManager()
{
  m_decoders.emplace_back(std::make_unique<PNGDecoder>());
  m_decoders.emplace_back(std::make_unique<JPGDecoder>());
  m_decoders.emplace_back(std::make_unique<GIFDecoder>());
}

// returns true for png, bmp, tga, jpg and dds files, otherwise returns false
bool DecoderManager::IsSupportedGraphicsFile(std::string_view filename)
{
  if (filename.length() < 4)
    return false;

  for (const auto& decoder : m_decoders)
  {
    const std::vector<std::string>& extensions = decoder->GetSupportedExtensions();
    for (const auto& extension : extensions)
    {
      if (std::string::npos != filename.rfind(extension, filename.length() - extension.length()))
      {
        return true;
      }
    }
  }
  return false;
}

bool DecoderManager::LoadFile(const std::string &filename, DecodedFrames &frames)
{
  for (const auto& decoder : m_decoders)
  {
    if (decoder->CanDecode(filename))
    {
      if (verbose)
        fprintf(stdout, "This is a %s - lets load it via %s...\n", decoder->GetImageFormatName(),
                decoder->GetDecoderName());
      return decoder->LoadFile(filename, frames);
    }
  }
  return false;
}
