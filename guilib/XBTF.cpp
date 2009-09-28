/*
 *      Copyright (C) 2005-2009 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#include <strings.h>
#include "XBTF.h"

CXBTFFrame::CXBTFFrame()
{
  m_width = 0;
  m_height = 0;
  m_packedSize = 0;
  m_unpackedSize = 0;
  m_offset = 0;
}

unsigned int CXBTFFrame::GetWidth() const
{
  return m_width;
}

void CXBTFFrame::SetWidth(unsigned int width)
{
  m_width = width;
}

unsigned int CXBTFFrame::GetHeight() const
{
  return m_height;
}

void CXBTFFrame::SetHeight(unsigned int height)
{
  m_height = height;
}

unsigned long long CXBTFFrame::GetPackedSize() const
{
  return m_packedSize;
}

void CXBTFFrame::SetPackedSize(unsigned long long size)
{
  m_packedSize = size;
}

unsigned long long CXBTFFrame::GetUnpackedSize() const
{
  return m_unpackedSize;
}

void CXBTFFrame::SetUnpackedSize(unsigned long long size)
{
  m_unpackedSize = size;
}

void CXBTFFrame::SetX(unsigned int x)
{
  m_x = x;
}

unsigned int CXBTFFrame::GetX() const
{
  return m_x;
}

void CXBTFFrame::SetY(unsigned int y)
{
  m_y = y;
}

unsigned int CXBTFFrame::GetY() const
{
  return m_y;
}

unsigned long long CXBTFFrame::GetOffset() const
{
  return m_offset;
}

void CXBTFFrame::SetOffset(unsigned long long offset)
{
  m_offset = offset;
}

unsigned int CXBTFFrame::GetDuration() const
{
  return m_duration;
}

void CXBTFFrame::SetDuration(unsigned int duration)
{
  m_duration = duration;
}


unsigned long long CXBTFFrame::GetHeaderSize() const
{
  unsigned long long result = 
    sizeof(m_width) +
    sizeof(m_height) +
    sizeof(m_x) +
    sizeof(m_y) +
    sizeof(m_packedSize) +
    sizeof(m_unpackedSize) +
    sizeof(m_offset) + 
    sizeof(m_duration); 
  
  return result;
}

CXBTFFile::CXBTFFile()
{
  bzero(m_path, sizeof(m_path));
  m_loop = 0;
  m_format = XB_FMT_UNKNOWN;  
}

CXBTFFile::CXBTFFile(const CXBTFFile& ref)
{
  strcpy(m_path, ref.m_path);
  m_loop = ref.m_loop;
  m_format = ref.m_format;
  m_frames = ref.m_frames;
}

char* CXBTFFile::GetPath()
{
  return m_path;
}

void CXBTFFile::SetPath(const std::string& path)
{
  bzero(m_path, sizeof(m_path));
  strncpy(m_path, path.c_str(), sizeof(m_path) - 1);
}

int CXBTFFile::GetLoop() const
{
  return m_loop;
}

void CXBTFFile::SetLoop(int loop)
{
  m_loop = loop;
}

unsigned int CXBTFFile::GetFormat() const
{
  return m_format;
}

void CXBTFFile::SetFormat(unsigned int format)
{
  m_format = format;
}

std::vector<CXBTFFrame>& CXBTFFile::GetFrames()
{
  return m_frames;
}

unsigned long long CXBTFFile::GetHeaderSize() const
{
  unsigned long long result = 
    sizeof(m_path) +
    sizeof(m_loop) +
    sizeof(m_format) +    
    sizeof(unsigned int); /* Number of frames */
  
  for (size_t i = 0; i < m_frames.size(); i++)
  {
    result += m_frames[i].GetHeaderSize();
  }
  
  return result;
}

CXBTF::CXBTF()
{
}

unsigned long long CXBTF::GetHeaderSize() const
{
  unsigned long long result = 
    4 /* Magic */ + 
    1 /* Vesion */ +
    sizeof(unsigned int) /* Number of Files */;
  
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
