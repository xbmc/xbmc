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

#include "DVDInputStreamHttp.h"
#include "URL.h"
#include "filesystem/CurlFile.h"

using namespace XFILE;

CDVDInputStreamHttp::CDVDInputStreamHttp() : CDVDInputStream(DVDSTREAM_TYPE_HTTP)
{
  m_pFile = NULL;
  m_eof = true;
}

CDVDInputStreamHttp::~CDVDInputStreamHttp()
{
  Close();
}

bool CDVDInputStreamHttp::IsEOF()
{
  if(m_pFile && !m_eof)
  {
    int64_t size = m_pFile->GetLength();
    if( size > 0 && m_pFile->GetPosition() >= size )
    {
      m_eof = true;
      return true;
    }
    return false;
  }
  return true;
}

bool CDVDInputStreamHttp::Open(const char* strFile, const std::string& content)
{
  if (!CDVDInputStream::Open(strFile, content)) return false;

  m_pFile = new CCurlFile();
  if (!m_pFile) return false;

  std::string filename = strFile;

  // shout protocol is same thing as http, but curl doesn't know what it is
  if( filename.substr(0, 8) == "shout://" )
    filename.replace(0, 8, "http://");

  // this should go to the demuxer
  m_pFile->SetUserAgent("WinampMPEG/5.09");
  m_pFile->SetRequestHeader("Icy-MetaData", "1");
  m_eof = false;

  // open file in binary mode
  if (!m_pFile->Open(CURL(filename)))
  {
    delete m_pFile;
    m_pFile = NULL;
    return false;
  }

  return true;
}

void CDVDInputStreamHttp::Close()
{
  if (m_pFile)
  {
    m_pFile->Close();
    delete m_pFile;
  }

  CDVDInputStream::Close();
  m_pFile = NULL;
}

int CDVDInputStreamHttp::Read(BYTE* buf, int buf_size)
{
  unsigned int ret = 0;
  if (m_pFile) ret = m_pFile->Read(buf, buf_size);
  else return -1;

  if( ret <= 0 ) m_eof = true;

  return (int)(ret & 0xFFFFFFFF);
}

int64_t CDVDInputStreamHttp::Seek(int64_t offset, int whence)
{
  if(!m_pFile)
    return -1;

  if(whence == SEEK_POSSIBLE)
    return m_pFile->IoControl(IOCTRL_SEEK_POSSIBLE, NULL);

  int64_t ret = m_pFile->Seek(offset, whence);

  if( ret >= 0 ) m_eof = false;

  return ret;
}

CHttpHeader* CDVDInputStreamHttp::GetHttpHeader()
{
  if (m_pFile) return (CHttpHeader*)&m_pFile->GetHttpHeader();
  else return NULL;
}

int64_t CDVDInputStreamHttp::GetLength()
{
  if (m_pFile)
    return m_pFile->GetLength();
  return 0;
}
