/*
 *      Copyright (C) 2013 Arne Morten Kvarving
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

#include "ImageDecoder.h"

#include "guilib/TextureFormats.h"
#include "utils/log.h"

static const std::map<int,int> KodiToAddonFormat = {{XB_FMT_A8R8G8B8, ADDON_IMG_FMT_A8R8G8B8},
                                                    {XB_FMT_A8,       ADDON_IMG_FMT_A8},
                                                    {XB_FMT_RGBA8,    ADDON_IMG_FMT_RGBA8},
                                                    {XB_FMT_RGB8,     ADDON_IMG_FMT_RGB8}};

namespace ADDON
{

CImageDecoder::CImageDecoder(const ADDON::AddonInfoPtr& addonInfo)
  : IAddonInstanceHandler(ADDON_IMAGEDECODER, addonInfo)
{
  memset(&m_struct, 0, sizeof(m_struct));
}

CImageDecoder::~CImageDecoder()
{
  DestroyInstance();
}

bool CImageDecoder::LoadImageFromMemory(unsigned char* buffer, unsigned int bufSize,
                                        unsigned int width, unsigned int height)
{
  if (!m_struct.toAddon.LoadImageFromMemory)
    return false;

  m_width = width;
  m_height = height;
  return m_struct.toAddon.LoadImageFromMemory(m_addonInstance, buffer, bufSize, &m_width, &m_height);
}

bool CImageDecoder::Decode(unsigned char* const pixels, unsigned int width,
                           unsigned int height, unsigned int pitch,
                           unsigned int format)
{
  if (!m_struct.toAddon.Decode)
    return false;

  auto it = KodiToAddonFormat.find(format & XB_FMT_MASK);
  if (it == KodiToAddonFormat.end())
    return false;

  bool result = m_struct.toAddon.Decode(m_addonInstance, pixels, width, height, pitch, it->second);
  m_width = width;
  m_height = height;

  return result;
}

bool CImageDecoder::Create(const std::string& mimetype)
{
  m_struct.props.mimetype = mimetype.c_str();
  m_struct.toKodi.kodiInstance = this;
  return CreateInstance(ADDON_INSTANCE_IMAGEDECODER, &m_struct, reinterpret_cast<KODI_HANDLE*>(&m_addonInstance));
}

} /*namespace ADDON*/
