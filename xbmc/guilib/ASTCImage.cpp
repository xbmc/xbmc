/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ASTCImage.h"

#include "filesystem/File.h"
#include "TextureFormats.h"
#include "Texture.h"
#include "utils/log.h"

#include <algorithm>
#include <string.h>

using namespace XFILE;

bool CASTCImage::ReadFile(const std::string &inputFile)
{
  CFile file;
  uint8_t descriptor[4];
  uint8_t block_x;
  uint8_t block_y;
  uint8_t block_z;
  uint8_t dim_x[3];
  uint8_t dim_y[3];
  uint8_t dim_z[3];
  uint32_t depth;

  if (!file.Open(inputFile))
    return false;

  if (file.GetLength() <= 16)
    return false;

  if (file.Read(&descriptor, 4) != 4)
    return false;
  if (file.Read(&block_x, 1) != 1)
    return false;
  if (file.Read(&block_y, 1) != 1)
    return false;
  if (file.Read(&block_z, 1) != 1)
    return false;
  if (file.Read(&dim_x[0], 1) != 1)
    return false;
  if (file.Read(&dim_x[1], 1) != 1)
    return false;
  if (file.Read(&dim_x[2], 1) != 1)
    return false;

  if (file.Read(&dim_y[0], 1) != 1)
    return false;
  if (file.Read(&dim_y[1], 1) != 1)
    return false;
  if (file.Read(&dim_y[2], 1) != 1)
    return false;

  if (file.Read(&dim_z[0], 1) != 1)
    return false;
  if (file.Read(&dim_z[1], 1) != 1)
    return false;
  if (file.Read(&dim_z[2], 1) != 1)
    return false;

  uint8_t magic[4] = {0x13, 0xAB, 0xA1, 0x5C};

  if (!std::equal(descriptor, descriptor+4, magic))
  {
    CLog::Log(LOGERROR, "{}: Error parsing ASTC file: wrong magic", __PRETTY_FUNCTION__);
    return false;
  }

  if (block_x == 4 && block_y == 4 && block_z == 1)
    m_format = XB_FMT_ASTC_4x4;
  else if (block_x == 8 && block_y == 8 && block_z == 1)
    m_format = XB_FMT_ASTC_8x8;
  else
  {
    CLog::Log(LOGERROR, "{}: Error parsing ASTC file: unsupported block size - {}x{}x{}", __PRETTY_FUNCTION__, block_x, block_y, block_z);
    return false;
  }

  m_width = dim_x[0] + (dim_x[1] << 8) + (dim_x[2] << 16);
  m_height = dim_y[0] + (dim_y[1] << 8) + (dim_y[2] << 16);
  depth = dim_z[0] + (dim_z[1] << 8) + (dim_z[2] << 16);

  if (depth != 1)
  {
    CLog::Log(LOGERROR, "{}: Error parsing ASTC file: depth invalid", __PRETTY_FUNCTION__);
    return false;
  }

  m_size = file.GetLength() - 16;
  m_data.resize(m_size);
  file.Read(m_data.data(), m_size);
  return true;
}
