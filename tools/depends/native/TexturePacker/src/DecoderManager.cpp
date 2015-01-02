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

std::vector<IDecoder *> DecoderManager::m_decoders;

// ADD new decoders here
// include decoders
#include "PNGDecoder.h"
#include "JPGDecoder.h"
#include "GIFDecoder.h"

void DecoderManager::InstantiateDecoders()
{
  m_decoders.push_back(new PNGDecoder());
  m_decoders.push_back(new JPGDecoder());
  m_decoders.push_back(new GIFDecoder());
}

void DecoderManager::FreeDecoders()
{
  for (unsigned int i = 0; i < m_decoders.size(); i++)
  {
    delete m_decoders[i];
  }
  m_decoders.clear();
}

// returns true for png, bmp, tga, jpg and dds files, otherwise returns false
bool DecoderManager::IsSupportedGraphicsFile(char *strFileName)
{
  std::string filename = strFileName;
  if (filename.length() < 4)
    return false;

  for (unsigned int i = 0; i < m_decoders.size(); i++)
  {
    const std::vector<std::string> extensions = m_decoders[i]->GetSupportedExtensions();
    for (unsigned int n = 0; n < extensions.size(); n++)
    {
      int extLen = extensions[n].length();
      if (std::string::npos != filename.rfind(extensions[n].c_str(), filename.length() - extLen, extLen))
      {
        return true;
      }
    }
  }
  return false;
}

bool DecoderManager::LoadFile(const std::string &filename, DecodedFrames &frames)
{
  for (unsigned int i = 0; i < m_decoders.size(); i++)
  {
    if (m_decoders[i]->CanDecode(filename))
    {
      fprintf(stdout, "This is a %s - lets load it via %s...\n", m_decoders[i]->GetImageFormatName(), m_decoders[i]->GetDecoderName());
      return m_decoders[i]->LoadFile(filename, frames);
    }
  }
  return false;
}

void DecoderManager::FreeDecodedFrames(DecodedFrames &frames)
{
  frames.clear();
}
