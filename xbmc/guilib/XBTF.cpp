/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
#include "XBTF.h"

CXBTFFrame::CXBTFFrame()
{
  m_width = 0;
  m_height = 0;
  m_packedSize = 0;
  m_unpackedSize = 0;
  m_offset = 0;
  m_format = XB_FMT_UNKNOWN;
  m_duration = 0;
}

uint32_t CXBTFFrame::GetWidth() const
{
  return m_width;
}

void CXBTFFrame::SetWidth(uint32_t width)
{
  m_width = width;
}

uint32_t CXBTFFrame::GetHeight() const
{
  return m_height;
}

void CXBTFFrame::SetHeight(uint32_t height)
{
  m_height = height;
}

uint64_t CXBTFFrame::GetPackedSize() const
{
  return m_packedSize;
}

void CXBTFFrame::SetPackedSize(uint64_t size)
{
  m_packedSize = size;
}

bool CXBTFFrame::IsPacked() const
{
  return m_unpackedSize != m_packedSize;
}

bool CXBTFFrame::HasAlpha() const
{
  return (m_format & XB_FMT_OPAQUE) == 0;
}

uint64_t CXBTFFrame::GetUnpackedSize() const
{
  return m_unpackedSize;
}

void CXBTFFrame::SetUnpackedSize(uint64_t size)
{
  m_unpackedSize = size;
}

void CXBTFFrame::SetFormat(uint32_t format)
{
  m_format = format;
}

uint32_t CXBTFFrame::GetFormat(bool raw) const
{
  return raw ? m_format : (m_format & XB_FMT_MASK);
}

uint64_t CXBTFFrame::GetOffset() const
{
  return m_offset;
}

void CXBTFFrame::SetOffset(uint64_t offset)
{
  m_offset = offset;
}

uint32_t CXBTFFrame::GetDuration() const
{
  return m_duration;
}

void CXBTFFrame::SetDuration(uint32_t duration)
{
  m_duration = duration;
}

uint64_t CXBTFFrame::GetHeaderSize() const
{
  uint64_t result =
    sizeof(m_width) +
    sizeof(m_height) +
    sizeof(m_format) +
    sizeof(m_packedSize) +
    sizeof(m_unpackedSize) +
    sizeof(m_offset) +
    sizeof(m_duration);

  return result;
}

CXBTFFile::CXBTFFile()
{
  memset(m_path, 0, sizeof(m_path));
  m_loop = 0;
}

CXBTFFile::CXBTFFile(const CXBTFFile& ref)
{
  strcpy(m_path, ref.m_path);
  m_loop = ref.m_loop;
  m_frames = ref.m_frames;
}

char* CXBTFFile::GetPath()
{
  return m_path;
}

void CXBTFFile::SetPath(const std::string& path)
{
  memset(m_path, 0, sizeof(m_path));
  strncpy(m_path, path.c_str(), sizeof(m_path) - 1);
}

uint32_t CXBTFFile::GetLoop() const
{
  return m_loop;
}

void CXBTFFile::SetLoop(uint32_t loop)
{
  m_loop = loop;
}

std::vector<CXBTFFrame>& CXBTFFile::GetFrames()
{
  return m_frames;
}

uint64_t CXBTFFile::GetHeaderSize() const
{
  uint64_t result =
    sizeof(m_path) +
    sizeof(m_loop) +
    sizeof(uint32_t); /* Number of frames */

  for (size_t i = 0; i < m_frames.size(); i++)
  {
    result += m_frames[i].GetHeaderSize();
  }

  return result;
}

CXBTF::CXBTF()
{
}

uint64_t CXBTF::GetHeaderSize() const
{
  uint64_t result =
    4 /* Magic */ +
    1 /* Vesion */ +
    sizeof(uint32_t) /* Number of Files */;

  for (size_t i = 0; i < m_files.size(); i++)
  {
    result += m_files[i].GetHeaderSize();
  }

  return result;
}

std::vector<CXBTFFile>& CXBTF::GetFiles()
{
  return m_files;
}
