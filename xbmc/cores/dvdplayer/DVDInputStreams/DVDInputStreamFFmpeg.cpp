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

#include "DVDInputStreamFFmpeg.h"

using namespace XFILE;

CDVDInputStreamFFmpeg::CDVDInputStreamFFmpeg()
  : CDVDInputStream(DVDSTREAM_TYPE_FFMPEG)
  , m_aborted(false)
{

}

CDVDInputStreamFFmpeg::~CDVDInputStreamFFmpeg()
{
  Close();
}

bool CDVDInputStreamFFmpeg::IsEOF()
{
  if(m_aborted)
    return true;
  else
    return false;
}

bool CDVDInputStreamFFmpeg::Open(const char* strFile, const std::string& content)
{
  if (!CDVDInputStream::Open(strFile, content))
    return false;

  m_can_pause = true;
  m_can_seek  = true;

  if(strnicmp(strFile, "udp://", 6) == 0
  || strnicmp(strFile, "rtp://", 6) == 0)
  {
      m_can_pause = false;
      m_can_seek  = false;
  }
  
  if(strnicmp(strFile, "tcp://", 6) == 0)
  {
      m_can_pause = true;
      m_can_seek  = false;
  }

  m_aborted   = false;

  return true;
}

// close file and reset everyting
void CDVDInputStreamFFmpeg::Close()
{
  CDVDInputStream::Close();
}

int CDVDInputStreamFFmpeg::Read(BYTE* buf, int buf_size)
{
  return -1;
}

int64_t CDVDInputStreamFFmpeg::GetLength()
{
  return 0;
}

int64_t CDVDInputStreamFFmpeg::Seek(int64_t offset, int whence)
{
  return -1;
}

