/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PKMImage.h"

#include "filesystem/File.h"
#include "TextureFormats.h"
#include "Texture.h"
#include "utils/log.h"

#include <algorithm>
#include <string.h>

using namespace XFILE;

bool CPKMImage::ReadFile(const std::string &inputFile)
{
  CFile file;
  uint8_t descriptor[6];
  uint16_t format;
  uint16_t width;
  uint16_t height;
  uint16_t originalWidth;
  uint16_t originalHeight;

  if (!file.Open(inputFile))
    return false;

  if (file.GetLength() <= 16)
    return false;
  if (file.Read(&descriptor, 6) != 6)
    return false;
  if (file.Read(&format, 2) != 2)
    return false;
  if (file.Read(&width, 2) != 2)
    return false;
  if (file.Read(&height, 2) != 2)
    return false;
  if (file.Read(&originalWidth, 2) != 2)
    return false;
  if (file.Read(&originalHeight, 2) != 2)
    return false;

  std::swap(reinterpret_cast<uint8_t *>(&format)[0], reinterpret_cast<uint8_t *>(&format)[1]);
  std::swap(reinterpret_cast<uint8_t *>(&height)[0], reinterpret_cast<uint8_t *>(&height)[1]);
  std::swap(reinterpret_cast<uint8_t *>(&width)[0], reinterpret_cast<uint8_t *>(&width)[1]);

  uint8_t magicPKM10[6] = {0x50, 0x4B, 0x4D, 0x20, 0x31, 0x30};
  uint8_t magicPKM20[6] = {0x50, 0x4B, 0x4D, 0x20, 0x32, 0x30};

  if (std::equal(descriptor, descriptor+6, magicPKM10))
  {
    if (format == 0)
      m_format = XB_FMT_ETC1;
    else
    {
      CLog::Log(LOGERROR, "{}: Error parsing PKM10 file: unknown format {}", __PRETTY_FUNCTION__, format);
      return false;
    }
  }
  else if (std::equal(descriptor, descriptor+6, magicPKM20))
  {
    if (format == 1)
      m_format = XB_FMT_ETC2_RGB;
    else if (format == 3)
      m_format = XB_FMT_ETC2_RGBA;
    else if (format == 5)
      m_format = XB_FMT_ETC2_R;
    else
    {
      CLog::Log(LOGERROR, "{}: Error parsing PKM20 file: unknown format {}", __PRETTY_FUNCTION__, format);
      return false;
    }
  }
  else
  {
    CLog::Log(LOGERROR, "{}: Error parsing PKM file: wrong magic", __PRETTY_FUNCTION__);
    return false;
  }

  m_width = width;
  m_height = height;
  m_size = file.GetLength() - 16;
  m_data.resize(m_size);
  file.Read(m_data.data(), m_size);
  return true;
}
