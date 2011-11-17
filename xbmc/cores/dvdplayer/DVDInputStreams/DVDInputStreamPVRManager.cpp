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

#include "DVDFactoryInputStream.h"
#include "DVDInputStreamPVRManager.h"
#include "filesystem/PVRFile.h"
#include "URL.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannel.h"
#include "utils/log.h"
#include "pvr/addons/PVRClients.h"

using namespace XFILE;
using namespace PVR;

/************************************************************************
 * Description: Class constructor, initialize member variables
 *              public class is CDVDInputStream
 */
CDVDInputStreamPVRManager::CDVDInputStreamPVRManager(IDVDPlayer* pPlayer) : CDVDInputStream(DVDSTREAM_TYPE_PVRMANAGER)
{
  m_pPlayer         = pPlayer;
  m_pFile           = NULL;
  m_pRecordable     = NULL;
  m_pLiveTV         = NULL;
  m_pOtherStream    = NULL;
  m_eof             = true;
}

/************************************************************************
 * Description: Class destructor
 */
CDVDInputStreamPVRManager::~CDVDInputStreamPVRManager()
{
  Close();
}

bool CDVDInputStreamPVRManager::IsEOF()
{
  if (m_pOtherStream)
    return m_pOtherStream->IsEOF();
  else
    return !m_pFile || m_eof;
}

bool CDVDInputStreamPVRManager::Open(const char* strFile, const std::string& content)
{
  /* Open PVR File for both cases, to have access to ILiveTVInterface and
   * IRecordable
   */
  m_pFile       = new CPVRFile();

  CURL url(strFile);
  if (!CDVDInputStream::Open(strFile, content)) return false;
  if (!m_pFile->Open(url))
  {
    delete m_pFile;
    m_pFile = NULL;
    return false;
  }
  m_eof = false;

  m_pLiveTV     = ((CPVRFile*)m_pFile)->GetLiveTV();
  m_pRecordable = ((CPVRFile*)m_pFile)->GetRecordable();

  /*
   * Translate the "pvr://....." entry.
   * The PVR Client can use http or whatever else is supported by DVDPlayer.
   * to access streams.
   * If after translation the file protocol is still "pvr://" use this class
   * to read the stream data over the CPVRFile class and the PVR Library itself.
   * Otherwise call CreateInputStream again with the translated filename and looks again
   * for the right protocol stream handler and swap every call to this input stream
   * handler.
   */
  std::string transFile = XFILE::CPVRFile::TranslatePVRFilename(strFile);
  if(transFile.substr(0, 6) != "pvr://")
  {
    m_pOtherStream = CDVDFactoryInputStream::CreateInputStream(m_pPlayer, transFile, content);
    if (!m_pOtherStream)
    {
      CLog::Log(LOGERROR, "CDVDInputStreamPVRManager::Open - unable to create input stream for [%s]", transFile.c_str());
      return false;
    }
    else
      m_pOtherStream->SetFileItem(m_item);

    if (!m_pOtherStream->Open(transFile.c_str(), content))
    {
      CLog::Log(LOGERROR, "CDVDInputStreamPVRManager::Open - error opening [%s]", transFile.c_str());
      delete m_pFile;
      m_pFile = NULL;
      delete m_pOtherStream;
      m_pOtherStream = NULL;
      return false;
    }
  }

  return true;
}

// close file and reset everyting
void CDVDInputStreamPVRManager::Close()
{
  if (m_pOtherStream)
  {
    m_pOtherStream->Close();
    delete m_pOtherStream;
  }

  if (m_pFile)
  {
    m_pFile->Close();
    delete m_pFile;
  }

  CDVDInputStream::Close();

  m_pPlayer         = NULL;
  m_pFile           = NULL;
  m_pLiveTV         = NULL;
  m_pRecordable     = NULL;
  m_pOtherStream    = NULL;
  m_eof             = true;
}

int CDVDInputStreamPVRManager::Read(BYTE* buf, int buf_size)
{
  if(!m_pFile) return -1;

  if (m_pOtherStream)
  {
    return m_pOtherStream->Read(buf, buf_size);
  }
  else
  {
    unsigned int ret = m_pFile->Read(buf, buf_size);

    /* we currently don't support non completing reads */
    if( ret <= 0 ) m_eof = true;

    return (int)(ret & 0xFFFFFFFF);
  }
}

__int64 CDVDInputStreamPVRManager::Seek(__int64 offset, int whence)
{
  if (!m_pFile)
    return -1;

  if (whence == SEEK_POSSIBLE)
    return m_pFile->IoControl(IOCTRL_SEEK_POSSIBLE, NULL);

  if (m_pOtherStream)
  {
    return m_pOtherStream->Seek(offset, whence);
  }
  else
  {
    __int64 ret = m_pFile->Seek(offset, whence);

    /* if we succeed, we are not eof anymore */
    if( ret >= 0 ) m_eof = false;

    return ret;
  }
}

__int64 CDVDInputStreamPVRManager::GetLength()
{
  if(!m_pFile) return -1;

  if (m_pOtherStream)
    return m_pOtherStream->GetLength();
  else
    return m_pFile->GetLength();
}

int CDVDInputStreamPVRManager::GetTotalTime()
{
  if (m_pLiveTV)
    return m_pLiveTV->GetTotalTime();
  return 0;
}

int CDVDInputStreamPVRManager::GetStartTime()
{
  if (m_pLiveTV)
    return m_pLiveTV->GetStartTime();
  return 0;
}

bool CDVDInputStreamPVRManager::NextChannel(bool preview/* = false*/)
{
  if (m_pLiveTV)
    return m_pLiveTV->NextChannel(preview);
  return false;
}

bool CDVDInputStreamPVRManager::PrevChannel(bool preview/* = false*/)
{
  if (m_pLiveTV)
    return m_pLiveTV->PrevChannel(preview);
  return false;
}

bool CDVDInputStreamPVRManager::SelectChannelByNumber(unsigned int channel)
{
  if (m_pLiveTV)
    return m_pLiveTV->SelectChannel(channel);
  return false;
}

bool CDVDInputStreamPVRManager::SelectChannel(const CPVRChannel &channel)
{
  if (m_pLiveTV)
    return m_pLiveTV->SelectChannel(channel.ChannelNumber());
  return false;
}

bool CDVDInputStreamPVRManager::GetSelectedChannel(CPVRChannel *channel)
{
  return g_PVRManager.GetCurrentChannel(channel);
}

bool CDVDInputStreamPVRManager::UpdateItem(CFileItem& item)
{
  if (m_pLiveTV)
    return m_pLiveTV->UpdateItem(item);
  return false;
}

bool CDVDInputStreamPVRManager::NextStream()
{
  if(!m_pFile) return false;

  if (m_pOtherStream)
    return m_pOtherStream->NextStream();
  else
  {
    if(m_pFile->SkipNext())
    {
      m_eof = false;
      return true;
    }
  }
  return false;
}

bool CDVDInputStreamPVRManager::CanRecord()
{
  if (m_pRecordable)
    return m_pRecordable->CanRecord();
  return false;
}

bool CDVDInputStreamPVRManager::IsRecording()
{
  if (m_pRecordable)
    return m_pRecordable->IsRecording();
  return false;
}

bool CDVDInputStreamPVRManager::Record(bool bOnOff)
{
  if (m_pRecordable)
    return m_pRecordable->Record(bOnOff);
  return false;
}

CStdString CDVDInputStreamPVRManager::GetInputFormat()
{
  if (m_pOtherStream)
    return "";
  else
    return g_PVRClients->GetCurrentInputFormat();
}
