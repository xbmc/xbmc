/*
*  Copyright (C) 2010-2013 Eduard Kytmanov
*  http://www.avmedia.su
*
*  Copyright (C) 2015 Romank
*  https://github.com/Romank1
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
*  along with GNU Make; see the file COPYING.  If not, write to
*  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*  http://www.gnu.org/copyleft/gpl.html
*
*/

#ifdef HAS_DS_PLAYER

#include "DSInputStreamPVRManager.h"
#include "URL.h"

#include "filesystem/PVRFile.h "
#include "pvr/addons/PVRClients.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "settings/AdvancedSettings.h"

CDSInputStreamPVRManager* g_pPVRStream = NULL;

CDSInputStreamPVRManager::CDSInputStreamPVRManager(CDSPlayer *pPlayer)
  : m_pPlayer(pPlayer)
  , m_pFile(NULL)
  , m_pLiveTV(NULL)
  , m_pRecordable(NULL)
  , m_pPVRBackend(NULL)
{
}

CDSInputStreamPVRManager::~CDSInputStreamPVRManager(void)
{
  Close();
}

void CDSInputStreamPVRManager::Close()
{
  if (m_pFile)
  {
    m_pFile->Close();
    SAFE_DELETE(m_pFile);
  }
  SAFE_DELETE(m_pPVRBackend);
  m_pLiveTV = NULL;
  m_pRecordable = NULL;
}

bool CDSInputStreamPVRManager::CloseAndOpenFile(const CURL& url)
{
  if (!m_pFile)
    return false;

  // In case opened channel need to be closed before opening new channel
  bool bReturnVal = false;
  if ( CDSPlayer::PlayerState == DSPLAYER_PLAYING 
    || CDSPlayer::PlayerState == DSPLAYER_PAUSED 
    || CDSPlayer::PlayerState == DSPLAYER_STOPPED)
  {
    // New channel cannot be opened, try to close the file
    m_pPlayer->CloseFile(false);
    bReturnVal = m_pPlayer->WaitForFileClose();

    if (!bReturnVal)
      CLog::Log(LOGERROR, "%s Closing file failed", __FUNCTION__);

    if (bReturnVal)
    {
      // Try to open the file
      bReturnVal = false;
      for (int iRetry = 0; iRetry < 10 && !bReturnVal; iRetry++)
      {
        if (m_pFile->Open(url))
          bReturnVal = true;
        else
          Sleep(500);
      }
      if (!bReturnVal)
        CLog::Log(LOGERROR, "%s Opening file failed", __FUNCTION__);
    }
  }

  if (bReturnVal)
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, "DSPlayer", "File opened successfully", TOAST_MESSAGE_TIME, false);

  return bReturnVal;
}

bool CDSInputStreamPVRManager::Open(const CFileItem& file)
{
  Close();

  bool bReturnVal = false;
  std::string strTranslatedPVRFile;
  m_pFile = new CPVRFile;

  CURL url(file.GetPath());
  bReturnVal = m_pFile->Open(url);
  if (!bReturnVal)
    bReturnVal = CloseAndOpenFile(url);

  if (bReturnVal)
  {
    m_pLiveTV = ((CPVRFile*)m_pFile)->GetLiveTV();
    m_pRecordable = ((CPVRFile*)m_pFile)->GetRecordable();
    m_pPVRBackend = GetPVRBackend();

    if (file.IsLiveTV())
    {
      bReturnVal = true;
      strTranslatedPVRFile = XFILE::CPVRFile::TranslatePVRFilename(file.GetPath());
      if (strTranslatedPVRFile == file.GetPath())
      {
        if (file.HasPVRChannelInfoTag())
          strTranslatedPVRFile = g_PVRClients->GetStreamURL(file.GetPVRChannelInfoTag());
      }

      // Check if LiveTV file path is valid for DSPlayer.
      if (URIUtils::IsLiveTV(strTranslatedPVRFile))
        bReturnVal = false;

      // Convert Stream URL To TimeShift file path 
      if (bReturnVal && g_advancedSettings.m_bDSPlayerUseUNCPathsForLiveTV)
      {
        if (m_pPVRBackend && m_pPVRBackend->SupportsStreamConversion(strTranslatedPVRFile))
        {
          CStdString strTimeShiftFile;
          bReturnVal = m_pPVRBackend->ConvertStreamURLToTimeShiftFilePath(strTranslatedPVRFile, strTimeShiftFile);
          if (bReturnVal)
            strTranslatedPVRFile = strTimeShiftFile;
        }
        else
        {
          CLog::Log(LOGERROR, "%s Stream conversion is not supported for this PVR Backend url: %s", __FUNCTION__, strTranslatedPVRFile.c_str());
        }
      }
    }
    else if (m_pPVRBackend && file.IsPVRRecording())
    {
      if (file.HasPVRRecordingInfoTag())
      {
        CPVRRecordingPtr recordingPtr = file.GetPVRRecordingInfoTag();
        CStdString strRecordingUrl;
        bReturnVal = m_pPVRBackend->GetRecordingStreamURL(recordingPtr->m_strRecordingId, strRecordingUrl, g_advancedSettings.m_bDSPlayerUseUNCPathsForLiveTV);
        if (bReturnVal)
          strTranslatedPVRFile = strRecordingUrl;
      }
    }
  }

  if (bReturnVal)
  {
    CFileItem fileItem = file;
    fileItem.SetPath(strTranslatedPVRFile);
    bReturnVal = m_pPlayer->OpenFileInternal(fileItem);
  }
  else
  {
    std::string strMessage = StringUtils::Format("Opening %s failed", file.IsLiveTV() ? "channel" : "recording");
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, "DSPlayer", strMessage, TOAST_DISPLAY_TIME, false);
    CLog::Log(LOGERROR, "%s %s", __FUNCTION__, strMessage.c_str());
  }

  return bReturnVal;
}

bool  CDSInputStreamPVRManager::PrepareForChannelSwitch(const CPVRChannelPtr &channel)
{
  bool bReturnVal = true;
  // Workaround for MediaPortal addon, running in ffmpeg mode.
  if (m_pPVRBackend && m_pPVRBackend->GetBackendName().find("MediaPortal TV-server") != std::string::npos)
  {
    // Opened new channel manually.
    // This will prevent from SwitchChannel() method to fail.
    bReturnVal = !((g_PVRClients->GetStreamURL(channel)).empty());

    // Clear StreamURL field of CurrentChannel 
    // that's used in CPVRClients::SwitchChannel()
    g_PVRManager.GetCurrentChannel()->SetStreamURL("");

    // Clear StreamURL field of new channel that's
    // used in CPVRManager::ChannelSwitch()
    channel->SetStreamURL("");
  }

  return bReturnVal;
}

bool CDSInputStreamPVRManager::PerformChannelSwitch()
{
  bool bResult = false;
  CFileItem fileItem;
  bResult = GetNewChannel(fileItem);
  if (bResult)
  {
    if (m_pPlayer->currentFileItem.GetPath() != fileItem.GetPath())
    {
      // File changed - fast channel switching is not possible
      CLog::Log(LOGNOTICE, "%s - File changed - fast channel switching is not possible, opening new channel...", __FUNCTION__);
      bResult = m_pPlayer->OpenFileInternal(fileItem);
    }
    else
    {
      // File not changed - channel switch complete successfully.
      m_pPlayer->UpdateApplication();
      m_pPlayer->UpdateChannelSwitchSettings();
      CLog::Log(LOGNOTICE, "%s - Channel switch complete successfully", __FUNCTION__);
    }
  }

  return bResult;
}

bool CDSInputStreamPVRManager::GetNewChannel(CFileItem& fileItem)
{
  bool bResult = false;
  CPVRChannelPtr channelPtr(g_PVRManager.GetCurrentChannel());
  if (channelPtr)
  {
    std::string strNewFile = g_PVRClients->GetStreamURL(channelPtr);
    if (!strNewFile.empty())
    {
      bResult = true;
      // Convert Stream URL To TimeShift file
      if (g_advancedSettings.m_bDSPlayerUseUNCPathsForLiveTV && m_pPVRBackend && m_pPVRBackend->SupportsStreamConversion(strNewFile))
      {
        CStdString timeShiftFile = "";
        if (m_pPVRBackend->ConvertStreamURLToTimeShiftFilePath(strNewFile, timeShiftFile))
          strNewFile = timeShiftFile;
      }

      CFileItem newFileItem(channelPtr);
      fileItem = newFileItem;
      fileItem.SetPath(strNewFile);
      CLog::Log(LOGNOTICE, "%s - New channel file path: %s", __FUNCTION__, strNewFile.c_str());
    }
  }

  if (!bResult)
    CLog::Log(LOGERROR, "%s - Failed to get file path of the new channel", __FUNCTION__);

  return bResult;
}

bool CDSInputStreamPVRManager::SelectChannel(const CPVRChannelPtr &channel)
{
  bool bResult = false;

  if (!SupportsChannelSwitch())
  {
    CFileItem item(channel);
    bResult = Open(item);
  }
  else if (m_pLiveTV && PrepareForChannelSwitch(channel))
  {
    bResult = m_pLiveTV->SelectChannel(channel->ChannelNumber());
    if (bResult)
      bResult = PerformChannelSwitch();
  }

  return bResult;
}

bool CDSInputStreamPVRManager::NextChannel(bool preview /* = false */)
{
  bool bResult = false;
  PVR_CLIENT client;

  CPVRChannelPtr channel(g_PVRManager.GetCurrentChannel());
  CFileItemPtr item = g_PVRChannelGroups->Get(channel->IsRadio())->GetSelectedGroup()->GetByChannelUp(channel);
  if (!item || !item->HasPVRChannelInfoTag())
  {
    CLog::Log(LOGERROR, "%s - Cannot find the channel", __FUNCTION__);
    return false;
  }

  if (!preview && !SupportsChannelSwitch())
  {
    if (item.get())
      bResult = Open(*item.get());
  }
  else if (m_pLiveTV && PrepareForChannelSwitch(item->GetPVRChannelInfoTag()))
  {
    bResult = m_pLiveTV->NextChannel(preview);
    if (bResult)
      bResult = PerformChannelSwitch();
  }

  return bResult;
}

bool CDSInputStreamPVRManager::PrevChannel(bool preview/* = false*/)
{
  bool bResult = false;
  PVR_CLIENT client;

  CPVRChannelPtr channel(g_PVRManager.GetCurrentChannel());
  CFileItemPtr item = g_PVRChannelGroups->Get(channel->IsRadio())->GetSelectedGroup()->GetByChannelDown(channel);
  if (!item || !item->HasPVRChannelInfoTag())
  {
    CLog::Log(LOGERROR, "%s - Cannot find the channel", __FUNCTION__);
    return false;
  }

  if (!preview && !SupportsChannelSwitch())
  {
    if (item.get())
      bResult = Open(*item.get());
  }
  else if (m_pLiveTV && PrepareForChannelSwitch(item->GetPVRChannelInfoTag()))
  {
    bResult = m_pLiveTV->PrevChannel(preview);
    if (bResult)
      bResult = PerformChannelSwitch();
  }

  return bResult;
}

bool CDSInputStreamPVRManager::SelectChannelByNumber(unsigned int iChannelNumber)
{
  bool bResult = false;
  PVR_CLIENT client;

  CPVRChannelPtr channel(g_PVRManager.GetCurrentChannel());
  CFileItemPtr item = g_PVRChannelGroups->Get(channel->IsRadio())->GetSelectedGroup()->GetByChannelNumber(iChannelNumber);
  if (!item || !item->HasPVRChannelInfoTag())
  {
    CLog::Log(LOGERROR, "%s - Cannot find the channel %d", __FUNCTION__, iChannelNumber);
    return false;
  }

  if (!SupportsChannelSwitch())
  {
    if (item.get())
      bResult = Open(*item.get());
  }
  else if (m_pLiveTV && PrepareForChannelSwitch(item->GetPVRChannelInfoTag()))
  {
    bResult = m_pLiveTV->SelectChannel(iChannelNumber);
    if (bResult)
      bResult = PerformChannelSwitch();
  }

  return bResult;
}

bool CDSInputStreamPVRManager::SupportsChannelSwitch()const
{
  if (!m_pPVRBackend || !g_advancedSettings.m_bDSPlayerFastChannelSwitching)
    return false;

  PVR_CLIENT pvrClient;
  if (!g_PVRClients->GetPlayingClient(pvrClient))
    return false;

  // Check if active PVR Backend addon has changed or PVR Backend addon does not support channel switching
  if (pvrClient->GetBackendName() != m_pPVRBackend->GetBackendName() || !m_pPVRBackend->SupportsFastChannelSwitch())
    return false;

  return pvrClient->HandlesInputStream();
}

bool CDSInputStreamPVRManager::UpdateItem(CFileItem& item)
{
  if (m_pLiveTV)
    return m_pLiveTV->UpdateItem(item);
  return false;
}

CDSPVRBackend* CDSInputStreamPVRManager::GetPVRBackend()
{
  PVR_CLIENT pvrClient;
  if (!g_PVRClients->GetPlayingClient(pvrClient))
  {
    CLog::Log(LOGERROR, "%s - Failed to get PVR Client", __FUNCTION__);
    return NULL;
  }

  CDSPVRBackend* pPVRBackend = NULL;
  if (pvrClient->GetBackendName().find("MediaPortal TV-server") != std::string::npos)
  {
    pPVRBackend = new CDSMediaPortal(pvrClient->GetConnectionString(), pvrClient->GetBackendName());
  }
  else if (pvrClient->GetBackendName().find("ARGUS TV") != std::string::npos)
  {
    pPVRBackend = new CDSArgusTV(pvrClient->GetConnectionString(), pvrClient->GetBackendName());
  }

  return pPVRBackend;
}

uint64_t CDSInputStreamPVRManager::GetTotalTime()
{
  if (m_pLiveTV)
    return m_pLiveTV->GetTotalTime();
  return 0;
}

uint64_t CDSInputStreamPVRManager::GetTime()
{
  if (m_pLiveTV)
    return m_pLiveTV->GetStartTime();
  return 0;
}

#endif
