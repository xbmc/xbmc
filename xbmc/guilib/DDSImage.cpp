/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <algorithm>
#include "DDSImage.h"
#include "XBTF.h"
#include "utils/log.h"
#include <string.h>

#ifndef NO_XBMC_FILESYSTEM
#include "filesystem/File.h"
using namespace XFILE;
#else
#include "SimpleFS.h"
#endif

CDDSImage::CDDSImage()
{
  m_data = NULL;
  memset(&m_desc, 0, sizeof(m_desc));
}

CDDSImage::CDDSImage(unsigned int width, unsigned int height, unsigned int format)
{
  m_data = NULL;
  Allocate(width, height, format);
}

CDDSImage::~CDDSImage()
{
  delete[] m_data;
}

unsigned int CDDSImage::GetWidth() const
{
  return m_desc.width;
}

unsigned int CDDSImage::GetHeight() const
{
  return m_desc.height;
}

unsigned int CDDSImage::GetFormat() const
{
  if (m_desc.pixelFormat.flags & DDPF_RGB)
    return 0; // Not supported
  if (m_desc.pixelFormat.flags & DDPF_FOURCC)
  {
    if (strncmp((const char *)&m_desc.pixelFormat.fourcc, "DXT1", 4) == 0)
      return XB_FMT_DXT1;
    if (strncmp((const char *)&m_desc.pixelFormat.fourcc, "DXT3", 4) == 0)
      return XB_FMT_DXT3;
    if (strncmp((const char *)&m_desc.pixelFormat.fourcc, "DXT5", 4) == 0)
      return XB_FMT_DXT5;
    if (strncmp((const char *)&m_desc.pixelFormat.fourcc, "ARGB", 4) == 0)
      return XB_FMT_A8R8G8B8;
  }
  return 0;
}

unsigned int CDDSImage::GetSize() const
{
  return m_desc.linearSize;
}

unsigned char *CDDSImage::GetData() const
{
  return m_data;
}

bool CDDSImage::ReadFile(const std::string &inputFile)
{
  // open the file
  CFile file;
  if (!file.Open(inputFile))
    return false;

  // read the header
  uint32_t magic;
  if (file.Read(&magic, 4) != 4)
    return false;
  if (file.Read(&m_desc, sizeof(m_desc)) != sizeof(m_desc))
    return false;
  if (!GetFormat())
    return false;  // not supported

  // allocate our data
  m_data = new unsigned char[m_desc.linearSize];
  if (!m_data)
    return false;

  // and read it in
  if (file.Read(m_data, m_desc.linearSize) != m_desc.linearSize)
    return false;

  file.Close();
  return true;
}

unsigned int CDDSImage::GetStorageRequirements(unsigned int width, unsigned int height, unsigned int format)
{
  switch (format)
  {
  case XB_FMT_DXT1:
    return ((width + 3) / 4) * ((height + 3) / 4) * 8;
  case XB_FMT_DXT3:
  case XB_FMT_DXT5:
    return ((width + 3) / 4) * ((height + 3) / 4) * 16;
  case XB_FMT_A8R8G8B8:
  default:
    return width * height * 4;
  }
}

void CDDSImage::Allocate(unsigned int width, unsigned int height, unsigned int format)
{
  memset(&m_desc, 0, sizeof(m_desc));
  m_desc.size = sizeof(m_desc);
  m_desc.flags = ddsd_caps | ddsd_pixelformat | ddsd_width | ddsd_height | ddsd_linearsize;
  m_desc.height = height;
  m_desc.width = width;
  m_desc.linearSize = GetStorageRequirements(width, height, format);
  m_desc.pixelFormat.size = sizeof(m_desc.pixelFormat);
  m_desc.pixelFormat.flags = ddpf_fourcc;
  memcpy(&m_desc.pixelFormat.fourcc, GetFourCC(format), 4);
  m_desc.caps.flags1 = ddscaps_texture;
  delete[] m_data;
  m_data = new unsigned char[m_desc.linearSize];
}

const char *CDDSImage::GetFourCC(unsigned int format)
{
  switch (format)
  {
  case XB_FMT_DXT1:
    return "DXT1";
  case XB_FMT_DXT3:
    return "DXT3";
  case XB_FMT_DXT5:
    return "DXT5";
  case XB_FMT_A8R8G8B8:
  default:
    return "ARGB";
  }
}
