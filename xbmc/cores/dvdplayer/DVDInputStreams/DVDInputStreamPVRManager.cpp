/*
 *      Copyright (C) 2012 Team XBMC
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

#include "DVDFactoryInputStream.h"
#include "DVDInputStreamPVRManager.h"
#include "filesystem/PVRFile.h"
#include "URL.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannel.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "settings/GUISettings.h"

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
  m_bReopened       = false;
  m_iScanTimeout    = 0;
}

/************************************************************************
 * Description: Class destructor
 */
CDVDInputStreamPVRManager::~CDVDInputStreamPVRManager()
{
  Close();
}

void CDVDInputStreamPVRManager::ResetScanTimeout(unsigned int iTimeoutMs)
{
  m_iScanTimeout = iTimeoutMs > 0 ?
      XbmcThreads::SystemClockMillis() + iTimeoutMs :
      0;
}

bool CDVDInputStreamPVRManager::IsEOF()
{
  // don't mark as eof while within the scan timeout
  if (m_iScanTimeout && XbmcThreads::SystemClockMillis() < m_iScanTimeout)
    return false;

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
  m_pFile       = new CPVRFile;
  m_pLiveTV     = ((CPVRFile*)m_pFile)->GetLiveTV();
  m_pRecordable = ((CPVRFile*)m_pFile)->GetRecordable();

  CURL url(strFile);
  if (!CDVDInputStream::Open(strFile, content)) return false;
  if (!m_pFile->Open(url))
  {
    delete m_pFile;
    m_pFile = NULL;
    m_pLiveTV = NULL;
    m_pRecordable = NULL;
    return false;
  }
  m_eof = false;

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
      m_pLiveTV = NULL;
      m_pRecordable = NULL;
      delete m_pOtherStream;
      m_pOtherStream = NULL;
      return false;
    }
  }

  ResetScanTimeout((unsigned int) g_guiSettings.GetInt("pvrplayback.scantime") * 1000);
  m_content = content;
  CLog::Log(LOGDEBUG, "CDVDInputStreamPVRManager::Open - stream opened: %s", transFile.c_str());

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

  CLog::Log(LOGDEBUG, "CDVDInputStreamPVRManager::Close - stream closed");
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

int64_t CDVDInputStreamPVRManager::Seek(int64_t offset, int whence)
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
    int64_t ret = m_pFile->Seek(offset, whence);

    /* if we succeed, we are not eof anymore */
    if( ret >= 0 ) m_eof = false;

    return ret;
  }
}

int64_t CDVDInputStreamPVRManager::GetLength()
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

int CDVDInputStreamPVRManager::GetTime()
{
  if (m_pLiveTV)
    return m_pLiveTV->GetStartTime();
  return 0;
}

bool CDVDInputStreamPVRManager::NextChannel(bool preview/* = false*/)
{
  PVR_CLIENT client;
  if (!preview && !SupportsChannelSwitch())
  {
    CPVRChannelPtr channel;
    g_PVRManager.GetCurrentChannel(channel);
    CFileItemPtr item = g_PVRChannelGroups->Get(channel->IsRadio())->GetSelectedGroup()->GetByChannelUp(*channel);
    if (item.get())
      return CloseAndOpen(item->GetPath().c_str());
  }
  else if (m_pLiveTV)
    return m_pLiveTV->NextChannel(preview);
  return false;
}

bool CDVDInputStreamPVRManager::PrevChannel(bool preview/* = false*/)
{
  PVR_CLIENT client;
  if (!preview && !SupportsChannelSwitch())
  {
    CPVRChannelPtr channel;
    g_PVRManager.GetCurrentChannel(channel);
    CFileItemPtr item = g_PVRChannelGroups->Get(channel->IsRadio())->GetSelectedGroup()->GetByChannelDown(*channel);
    if (item.get())
      return CloseAndOpen(item->GetPath().c_str());
  }
  else if (m_pLiveTV)
    return m_pLiveTV->PrevChannel(preview);
  return false;
}

bool CDVDInputStreamPVRManager::SelectChannelByNumber(unsigned int iChannelNumber)
{
  PVR_CLIENT client;
  if (!SupportsChannelSwitch())
  {
    CPVRChannelPtr channel;
    g_PVRManager.GetCurrentChannel(channel);
    CFileItemPtr item = g_PVRChannelGroups->Get(channel->IsRadio())->GetSelectedGroup()->GetByChannelNumber(iChannelNumber);
    if (item.get())
      return CloseAndOpen(item->GetPath().c_str());
  }
  else if (m_pLiveTV)
    return m_pLiveTV->SelectChannel(iChannelNumber);

  return false;
}

bool CDVDInputStreamPVRManager::SelectChannel(const CPVRChannel &channel)
{
  PVR_CLIENT client;
  if (!SupportsChannelSwitch())
  {
    CFileItem item(channel);
    return CloseAndOpen(item.GetPath().c_str());
  }
  else if (m_pLiveTV)
  {
    return m_pLiveTV->SelectChannel(channel.ChannelNumber());
  }

  return false;
}

bool CDVDInputStreamPVRManager::GetSelectedChannel(CPVRChannelPtr& channel) const
{
  return g_PVRManager.GetCurrentChannel(channel);
}

bool CDVDInputStreamPVRManager::UpdateItem(CFileItem& item)
{
  if (m_pLiveTV)
    return m_pLiveTV->UpdateItem(item);
  return false;
}

CDVDInputStream::ENextStream CDVDInputStreamPVRManager::NextStream()
{
  if(!m_pFile) return NEXTSTREAM_NONE;

  if(m_bReopened)
  {
    m_bReopened = false;
    m_eof       = false;
    return NEXTSTREAM_RETRY;
  }

  if (m_pOtherStream)
    return m_pOtherStream->NextStream();
  else if(m_pFile->SkipNext())
  {
    m_eof = false;
    return NEXTSTREAM_OPEN;
  }

  return NEXTSTREAM_NONE;
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

bool CDVDInputStreamPVRManager::CanPause()
{
  return g_PVRClients->CanPauseStream();
}

bool CDVDInputStreamPVRManager::CanSeek()
{
  return g_PVRClients->CanSeekStream();
}

void CDVDInputStreamPVRManager::Pause(bool bPaused)
{
  g_PVRClients->PauseStream(bPaused);
}

CStdString CDVDInputStreamPVRManager::GetInputFormat()
{
  if (!m_pOtherStream && g_PVRManager.IsStarted())
    return g_PVRClients->GetCurrentInputFormat();
  return StringUtils::EmptyString;
}

bool CDVDInputStreamPVRManager::CloseAndOpen(const char* strFile)
{
  Close();

  if (Open(strFile, m_content))
  {
    m_bReopened = true;
    return true;
  }

  return false;
}

bool CDVDInputStreamPVRManager::SupportsChannelSwitch(void) const
{
  PVR_CLIENT client;
  return g_PVRClients->GetPlayingClient(client) &&
         client->HandlesInputStream();
}
