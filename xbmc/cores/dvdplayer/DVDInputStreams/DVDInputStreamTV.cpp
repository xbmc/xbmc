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

#include "DVDInputStreamTV.h"
#include "filesystem/MythFile.h"
#include "filesystem/VTPFile.h"
#include "pvr/channels/PVRChannel.h"
#include "filesystem/VTPFile.h"
#include "filesystem/SlingboxFile.h"
#include "URL.h"

using namespace XFILE;

CDVDInputStreamTV::CDVDInputStreamTV() : CDVDInputStream(DVDSTREAM_TYPE_TV)
{
  m_pFile = NULL;
  m_pRecordable = NULL;
  m_pLiveTV = NULL;
  m_eof = true;
}

CDVDInputStreamTV::~CDVDInputStreamTV()
{
  Close();
}

bool CDVDInputStreamTV::IsEOF()
{
  return !m_pFile || m_eof;
}

bool CDVDInputStreamTV::Open(const char* strFile, const std::string& content)
{
  if (!CDVDInputStream::Open(strFile, content)) return false;

  if(strncmp(strFile, "vtp://", 6) == 0)
  {
    m_pFile       = new CVTPFile();
    m_pLiveTV     = ((CVTPFile*)m_pFile)->GetLiveTV();
    m_pRecordable = NULL;
  }
  else if (strncmp(strFile, "sling://", 8) == 0)
  {
    m_pFile       = new CSlingboxFile();
    m_pLiveTV     = ((CSlingboxFile*)m_pFile)->GetLiveTV();
    m_pRecordable = NULL;
  }
  else
  {
    m_pFile       = new CMythFile();
    m_pLiveTV     = ((CMythFile*)m_pFile)->GetLiveTV();
    m_pRecordable = ((CMythFile*)m_pFile)->GetRecordable();
  }

  CURL url(strFile);
  // open file in binary mode
  if (!m_pFile->Open(url))
  {
    delete m_pFile;
    m_pFile = NULL;
    return false;
  }
  m_eof = false;
  return true;
}

// close file and reset everyting
void CDVDInputStreamTV::Close()
{
  if (m_pFile)
  {
    m_pFile->Close();
    delete m_pFile;
  }

  CDVDInputStream::Close();
  m_pFile = NULL;
  m_pLiveTV = NULL;
  m_eof = true;
}

int CDVDInputStreamTV::Read(BYTE* buf, int buf_size)
{
  if(!m_pFile) return -1;

  unsigned int ret = m_pFile->Read(buf, buf_size);

  /* we currently don't support non completing reads */
  if( ret <= 0 ) m_eof = true;

  return (int)(ret & 0xFFFFFFFF);
}

int64_t CDVDInputStreamTV::Seek(int64_t offset, int whence)
{
  if(!m_pFile) return -1;
  int64_t ret = m_pFile->Seek(offset, whence);

  /* if we succeed, we are not eof anymore */
  if( ret >= 0 ) m_eof = false;

  return ret;
}

int64_t CDVDInputStreamTV::GetLength()
{
  if (!m_pFile) return 0;
  return m_pFile->GetLength();
}


int CDVDInputStreamTV::GetTotalTime()
{
  if(!m_pLiveTV) return -1;
  return m_pLiveTV->GetTotalTime();
}

int CDVDInputStreamTV::GetTime()
{
  if(!m_pLiveTV) return -1;
  return m_pLiveTV->GetStartTime();
}

bool CDVDInputStreamTV::NextChannel(bool preview/* = false*/)
{
  if(!m_pLiveTV) return false;
  return m_pLiveTV->NextChannel();
}

bool CDVDInputStreamTV::PrevChannel(bool preview/* = false*/)
{
  if(!m_pLiveTV) return false;
  return m_pLiveTV->PrevChannel();
}

bool CDVDInputStreamTV::SelectChannelByNumber(unsigned int channel)
{
  if(!m_pLiveTV) return false;
  return m_pLiveTV->SelectChannel(channel);
}

bool CDVDInputStreamTV::UpdateItem(CFileItem& item)
{
  if(m_pLiveTV)
    return m_pLiveTV->UpdateItem(item);
  return false;
}

bool CDVDInputStreamTV::SeekTime(int iTimeInMsec)
{
  return false;
}

CDVDInputStream::ENextStream CDVDInputStreamTV::NextStream()
{
  if(!m_pFile) return NEXTSTREAM_NONE;
  if(m_pFile->SkipNext())
  {
    m_eof = false;
    return NEXTSTREAM_OPEN;
  }
  return NEXTSTREAM_NONE;
}

bool CDVDInputStreamTV::CanRecord()
{
  if(m_pRecordable)
    return m_pRecordable->CanRecord();
  return false;
}
bool CDVDInputStreamTV::IsRecording()
{
  if(m_pRecordable)
    return m_pRecordable->IsRecording();
  return false;
}
bool CDVDInputStreamTV::Record(bool bOnOff)
{
  if(m_pRecordable)
    return m_pRecordable->Record(bOnOff);
  return false;
}

int CDVDInputStreamTV::GetBlockSize()
{
  if(m_pFile)
    return m_pFile->GetChunkSize();
  else
    return 0;
}

