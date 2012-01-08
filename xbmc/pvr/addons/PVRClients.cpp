/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include "PVRClients.h"
#include "PVRClient.h"

#include "Application.h"
#include "settings/GUISettings.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogSelect.h"
#include "threads/SingleLock.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRDatabase.h"
#include "guilib/GUIWindowManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/channels/PVRChannelGroupInternal.h"
#include "utils/StringUtils.h"

#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif

using namespace std;
using namespace ADDON;
using namespace PVR;
using namespace EPG;

CPVRClients::CPVRClients(void) :
    CThread("PVR add-on updater"),
    m_bChannelScanRunning(false),
    m_bAllClientsConnected(false),
    m_bIsSwitchingChannels(false),
    m_bIsValidChannelSettings(false),
    m_bIsPlayingLiveTV(false),
    m_bIsPlayingRecording(false),
    m_scanStart(0)
{
  ResetQualityData(m_qualityInfo);
}

CPVRClients::~CPVRClients(void)
{
  Unload();
}

void CPVRClients::Start(void)
{
  Stop();

  ResetQualityData(m_qualityInfo);

  Create();
  SetPriority(-1);
}

void CPVRClients::Stop(void)
{
  StopThread();
}

bool CPVRClients::IsConnectedClient(int iClientId)
{
  boost::shared_ptr<CPVRClient> client;
  return GetConnectedClient(iClientId, client);
}

bool CPVRClients::GetConnectedClient(int iClientId, boost::shared_ptr<CPVRClient> &addon)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  CLIENTMAPITR itr = m_clientMap.find(iClientId);
  if (itr != m_clientMap.end() && itr->second->ReadyToUse())
  {
    addon = itr->second;
    bReturn = true;
  }
  else
  {
    CLog::Log(LOGDEBUG, "%s - client %d is not connected", __FUNCTION__, iClientId);
  }

  return bReturn;
}

bool CPVRClients::RequestRestart(AddonPtr addon, bool bDataChanged)
{
  return StopClient(addon, true);
}

bool CPVRClients::RequestRemoval(AddonPtr addon)
{
  return StopClient(addon, false);
}

void CPVRClients::Unload(void)
{
  Stop();

  CSingleLock lock(m_critSection);

  /* destroy all clients */
  for (CLIENTMAPITR itr = m_clientMap.begin(); itr != m_clientMap.end(); itr++)
    m_clientMap[(*itr).first]->Destroy();

  /* reset class properties */
  m_bChannelScanRunning  = false;
  m_bAllClientsConnected = false;
  m_bIsPlayingLiveTV     = false;
  m_bIsPlayingRecording  = false;
  m_strPlayingClientName = StringUtils::EmptyString;

  m_clientMap.clear();
}

int CPVRClients::GetFirstConnectedClientID(void)
{
  int iReturn(-1);
  CSingleLock lock(m_critSection);

  for (CLIENTMAPITR itr = m_clientMap.begin(); itr != m_clientMap.end(); itr++)
  {
    boost::shared_ptr<CPVRClient> client = m_clientMap[(*itr).first];
    if (client->ReadyToUse())
    {
      iReturn = client->GetID();
      break;
    }
  }

  return iReturn;
}

bool CPVRClients::AllClientsConnected(void) const
{
  CSingleLock lock(m_critSection);
  return m_bAllClientsConnected;
}

int CPVRClients::EnabledClientAmount(void) const
{
  CSingleLock lock(m_critSection);
  return m_clientMap.size();
}

bool CPVRClients::HasEnabledClients(void) const
{
  CSingleLock lock(m_critSection);
  return !m_clientMap.empty();
}

bool CPVRClients::StopClient(AddonPtr client, bool bRestart)
{
  bool bFoundClient(false);
  if (!client)
    return bFoundClient;

  boost::shared_ptr<CPVRClient> mappedClient;
  {
    CSingleLock lock(m_critSection);
    CLIENTMAPITR itr = m_clientMap.begin();
    while (!bFoundClient && itr != m_clientMap.end())
    {
      if (m_clientMap[(*itr).first]->ID() == client->ID())
      {
        bFoundClient = true;
        mappedClient = m_clientMap[(*itr).first];

        if (!bRestart)
          m_clientMap.erase((*itr).first);
      }
      itr++;
    }
  }

  if (bFoundClient)
  {
    g_PVRManager.StopUpdateThreads();
    if (bRestart)
    {
      mappedClient->ReCreate();
    }
    else
    {
      mappedClient->Destroy();
      CPVRDatabase *database = OpenPVRDatabase();
      if (database)
      {
        database->DeleteClient(mappedClient->ID());
        database->Close();
      }
    }
    g_PVRManager.StartUpdateThreads();
  }

  return bFoundClient;
}

int CPVRClients::ConnectedClientAmount(void)
{
  int iReturn(0);
  CSingleLock lock(m_critSection);

  if (!m_clientMap.empty())
  {
    CLIENTMAPITR itr = m_clientMap.begin();
    while (itr != m_clientMap.end())
    {
      if (m_clientMap[(*itr).first]->ReadyToUse())
        ++iReturn;
      itr++;
    }
  }

  return iReturn;
}

bool CPVRClients::HasConnectedClients(void)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  if (!m_clientMap.empty())
  {
    CLIENTMAPITR itr = m_clientMap.begin();
    while (itr != m_clientMap.end())
    {
      if (m_clientMap[(*itr).first]->ReadyToUse())
      {
        bReturn = true;
        break;
      }
      itr++;
    }
  }

  return bReturn;
}

bool CPVRClients::GetClientName(int iClientId, CStdString &strName)
{
  bool bReturn(false);
  boost::shared_ptr<CPVRClient> client;
  if ((bReturn = GetConnectedClient(iClientId, client)) == true)
    strName = client->GetFriendlyName();

  return bReturn;
}

int CPVRClients::GetConnectedClients(CLIENTMAP *clients)
{
  int iReturn(0);
  CSingleLock lock(m_critSection);

  if (m_clientMap.size() > 0)
  {
    CLIENTMAPITR itr = m_clientMap.begin();
    while (itr != m_clientMap.end())
    {
      boost::shared_ptr<CPVRClient> client = m_clientMap[(*itr).first];
      if (client->ReadyToUse())
      {
        clients->insert(std::make_pair(client->GetID(), client));
        ++iReturn;
        break;
      }
      itr++;
    }
  }

  return iReturn;
}

int CPVRClients::GetPlayingClientID(void) const
{
  int iReturn(-1);
  CSingleLock lock(m_critSection);

  if (m_bIsPlayingLiveTV)
    iReturn = m_currentChannel.ClientID();
  else if (m_bIsPlayingRecording)
    iReturn = m_currentRecording.m_iClientId;

  return iReturn;
}

PVR_ADDON_CAPABILITIES CPVRClients::GetAddonCapabilities(int iClientId)
{
  PVR_ADDON_CAPABILITIES props;
  memset(&props, 0, sizeof(PVR_ADDON_CAPABILITIES));

  boost::shared_ptr<CPVRClient> client;
  if (GetConnectedClient(iClientId, client))
    props = client->GetAddonCapabilities();

  return props;
}

PVR_ADDON_CAPABILITIES CPVRClients::GetCurrentAddonCapabilities(void)
{
  PVR_ADDON_CAPABILITIES props;
  memset(&props, 0, sizeof(PVR_ADDON_CAPABILITIES));

  CSingleLock lock(m_critSection);
  if (m_bIsPlayingLiveTV)
    props = m_clientMap[m_currentChannel.ClientID()]->GetAddonCapabilities();
  else if (m_bIsPlayingRecording)
    props = m_clientMap[m_currentRecording.m_iClientId]->GetAddonCapabilities();

  return props;
}

bool CPVRClients::IsPlaying(void) const
{
  CSingleLock lock(m_critSection);
  return m_bIsPlayingRecording || m_bIsPlayingLiveTV;
}

const CStdString CPVRClients::GetPlayingClientName(void) const
{
  CSingleLock lock(m_critSection);
  return m_strPlayingClientName;
}

int CPVRClients::ReadStream(void* lpBuf, int64_t uiBufSize)
{
  CSingleLock lock(m_critSection);

  if (m_bIsPlayingLiveTV)
    return m_clientMap[m_currentChannel.ClientID()]->ReadLiveStream(lpBuf, uiBufSize);
  else if (m_bIsPlayingRecording)
    return m_clientMap[m_currentRecording.m_iClientId]->ReadRecordedStream(lpBuf, uiBufSize);

  return 0;
}

int64_t CPVRClients::LengthStream(void)
{
  int64_t streamLength(0);
  CSingleLock lock(m_critSection);

  if (m_bIsPlayingLiveTV)
    streamLength = 0;
  else if (m_bIsPlayingRecording)
    streamLength = m_clientMap[m_currentRecording.m_iClientId]->LengthRecordedStream();

  return streamLength;
}

int64_t CPVRClients::SeekStream(int64_t iFilePosition, int iWhence/* = SEEK_SET*/)
{
  int64_t streamNewPos(0);
  CSingleLock lock(m_critSection);

  if (m_bIsPlayingLiveTV)
    streamNewPos = 0;
  else if (m_bIsPlayingRecording)
    streamNewPos = m_clientMap[m_currentRecording.m_iClientId]->SeekRecordedStream(iFilePosition, iWhence);

  return streamNewPos;
}

int64_t CPVRClients::GetStreamPosition(void)
{
  int64_t streamPos(0);
  CSingleLock lock(m_critSection);

  if (m_bIsPlayingLiveTV)
    streamPos = 0;
  else if (m_bIsPlayingRecording)
    streamPos = m_clientMap[m_currentRecording.m_iClientId]->PositionRecordedStream();

  return streamPos;
}

void CPVRClients::CloseStream(void)
{
  CSingleLock lock(m_critSection);
  CloseLiveStream() || CloseRecordedStream();
  m_strPlayingClientName = StringUtils::EmptyString;
}

PVR_STREAM_PROPERTIES *CPVRClients::GetCurrentStreamProperties(void)
{
  PVR_STREAM_PROPERTIES *props = NULL;
  CSingleLock lock(m_critSection);

  if (m_bIsPlayingLiveTV)
  {
    int iChannelId = m_currentChannel.ClientID();
    m_clientMap[iChannelId]->GetStreamProperties(&m_streamProps[iChannelId]);

    props = &m_streamProps[iChannelId];
  }

  return props;
}

CStdString CPVRClients::GetCurrentInputFormat(void) const
{
  CStdString strReturn;
  CPVRChannel currentChannel;
  if (GetPlayingChannel(currentChannel))
    strReturn = currentChannel.InputFormat();

  return strReturn;
}

bool CPVRClients::IsReadingLiveStream(void) const
{
  CSingleLock lock(m_critSection);
  return m_bIsPlayingLiveTV;
}

bool CPVRClients::IsPlayingTV(void) const
{
  CSingleLock lock(m_critSection);
  return m_bIsPlayingLiveTV && !m_currentChannel.IsRadio();
}

bool CPVRClients::IsPlayingRadio(void) const
{
  CSingleLock lock(m_critSection);
  return m_bIsPlayingLiveTV && m_currentChannel.IsRadio();
}

bool CPVRClients::IsEncrypted(void) const
{
  CSingleLock lock(m_critSection);
  return m_bIsPlayingLiveTV && m_currentChannel.IsEncrypted();
}

bool CPVRClients::OpenLiveStream(const CPVRChannel &tag)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  m_bIsPlayingLiveTV = false;
  m_bIsPlayingRecording = false;

  ResetQualityData(m_qualityInfo);

  /* try to open the stream on the client */
  boost::shared_ptr<CPVRClient> client;
  if (tag.StreamURL().IsEmpty() == false ||
      (GetConnectedClient(tag.ClientID(), client) &&
      client->GetAddonCapabilities().bHandlesInputStream &&
      client->OpenLiveStream(tag)))
  {
    m_currentChannel = tag;
    m_bIsPlayingLiveTV = true;
    if (tag.ClientID() == XBMC_VIRTUAL_CLIENTID)
      m_strPlayingClientName = g_localizeStrings.Get(19209);
    else if (!tag.IsVirtual())
      GetClientName(tag.ClientID(), m_strPlayingClientName);
    else
      m_strPlayingClientName = g_localizeStrings.Get(13205);

    bReturn = true;
  }

  return bReturn;
}

bool CPVRClients::CloseLiveStream(void)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);
  ResetQualityData(m_qualityInfo);

  if (!m_bIsPlayingLiveTV)
    return bReturn;

  if ((m_currentChannel.StreamURL().IsEmpty()) || (m_currentChannel.StreamURL().compare(0,13, "pvr://stream/") == 0))
  {
    m_clientMap[m_currentChannel.ClientID()]->CloseLiveStream();
    bReturn = true;
  }

  m_bIsPlayingLiveTV = false;
  return bReturn;
}

CStdString CPVRClients::GetStreamURL(const CPVRChannel &tag)
{
  CStdString strReturn;
  boost::shared_ptr<CPVRClient> client;
  if (GetConnectedClient(tag.ClientID(), client))
    strReturn = client->GetLiveStreamURL(tag);
  else
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, tag.ClientID());

  return strReturn;
}

bool CPVRClients::SwitchChannel(const CPVRChannel &channel)
{
  bool bSwitchSuccessful(false);
  bool bNewStreamOpened(false);

  {
    CSingleLock lock(m_critSection);
    if (m_bIsSwitchingChannels)
    {
      CLog::Log(LOGDEBUG, "PVRClients - %s - can't switch to channel '%s'. waiting for the previous switch to complete", __FUNCTION__, channel.ChannelName().c_str());
      return false;
    }
    m_bIsSwitchingChannels = true;
  }

  CPVRChannel currentChannel;
  bool bGotPlayingChannel = GetPlayingChannel(currentChannel);
  if (bGotPlayingChannel)
  {
    if (currentChannel != channel)
    {
      /* different client add-on */
      if (currentChannel.ClientID() != channel.ClientID() ||
      /* switch from radio -> tv or tv -> radio */
      currentChannel.IsRadio() != channel.IsRadio())
      {
        CloseStream();
        bSwitchSuccessful = OpenLiveStream(channel);
      }
      else if (!channel.StreamURL().IsEmpty() || !currentChannel.StreamURL().IsEmpty())
      {
        // StreamURL should always be opened as a new file
        CFileItem m_currentFile(channel);
        g_application.getApplicationMessenger().PlayFile(m_currentFile, false);
        bSwitchSuccessful = true;
        bNewStreamOpened = true;
      }
      else
      {
        boost::shared_ptr<CPVRClient> client;
        if (GetConnectedClient(channel.ClientID(), client))
          bSwitchSuccessful = client->SwitchChannel(channel);
      }
    }
    else
    {
      bSwitchSuccessful = true;
    }
  }

  {
    CSingleLock lock(m_critSection);
    m_bIsSwitchingChannels = false;
    if (bSwitchSuccessful && !bNewStreamOpened)
    {
      m_currentChannel = channel;
      m_bIsPlayingLiveTV = true;
      ResetQualityData(m_qualityInfo);
      m_bIsValidChannelSettings = false;
    }
  }

  if (!bSwitchSuccessful)
    CLog::Log(LOGERROR, "PVR - %s - cannot switch channel on client %d",__FUNCTION__, channel.ClientID());

  return bSwitchSuccessful;
}

bool CPVRClients::GetPlayingChannel(CPVRChannel &channel) const
{
  CSingleLock lock(m_critSection);

  if (m_bIsPlayingLiveTV)
    channel = m_currentChannel;
  return m_bIsPlayingLiveTV;
}

bool CPVRClients::IsPlayingRecording(void) const
{
  CSingleLock lock(m_critSection);
  return m_bIsPlayingRecording;
}

bool CPVRClients::OpenRecordedStream(const CPVRRecording &tag)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  m_bIsPlayingLiveTV = false;
  m_bIsPlayingRecording = false;

  /* try to open the recording stream on the client */
  boost::shared_ptr<CPVRClient> client;
  if (GetConnectedClient(tag.m_iClientId, client) &&
      client->OpenRecordedStream(tag))
  {
    m_currentRecording = tag;
    m_bIsPlayingRecording = true;
    m_strPlayingClientName = client->GetFriendlyName();
    bReturn = true;
  }

  return bReturn;
}

bool CPVRClients::CloseRecordedStream(void)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  if (!m_bIsPlayingRecording)
    return bReturn;

  if (m_currentRecording.m_iClientId > 0 && m_clientMap[m_currentRecording.m_iClientId])
  {
    m_clientMap[m_currentRecording.m_iClientId]->CloseRecordedStream();
    bReturn = true;
  }

  m_bIsPlayingRecording = false;
  return bReturn;
}

bool CPVRClients::GetPlayingRecording(CPVRRecording &recording) const
{
  CSingleLock lock(m_critSection);
  if (m_bIsPlayingRecording)
    recording = m_currentRecording;

  return m_bIsPlayingRecording;
}

void CPVRClients::DemuxReset(void)
{
  /* don't lock here cause it'll cause a dead lock when the client connection is dropped while playing */
  if (m_bIsPlayingLiveTV)
    m_clientMap[m_currentChannel.ClientID()]->DemuxReset();
}

void CPVRClients::DemuxAbort(void)
{
  /* don't lock here cause it'll cause a dead lock when the client connection is dropped while playing */
  if (m_bIsPlayingLiveTV)
    m_clientMap[m_currentChannel.ClientID()]->DemuxAbort();
}

void CPVRClients::DemuxFlush(void)
{
  /* don't lock here cause it'll cause a dead lock when the client connection is dropped while playing */
  if (m_bIsPlayingLiveTV)
    m_clientMap[m_currentChannel.ClientID()]->DemuxFlush();
}

DemuxPacket* CPVRClients::ReadDemuxStream(void)
{
  /* don't lock here cause it'll cause a dead lock when the client connection is dropped while playing */
  if (m_bIsPlayingLiveTV)
    return m_clientMap[m_currentChannel.ClientID()]->DemuxRead();

  return NULL;
}

void CPVRClients::GetQualityData(PVR_SIGNAL_STATUS *status) const
{
  CSingleLock lock(m_critSection);
  *status = m_qualityInfo;
}

int CPVRClients::GetSignalLevel(void) const
{
  CSingleLock lock(m_critSection);
  return (int) ((float) m_qualityInfo.iSignal / 0xFFFF * 100);
}

int CPVRClients::GetSNR(void) const
{
  CSingleLock lock(m_critSection);
  return (int) ((float) m_qualityInfo.iSNR / 0xFFFF * 100);
}

bool CPVRClients::HasTimerSupport(int iClientId)
{
  CSingleLock lock(m_critSection);

  return IsConnectedClient(iClientId) && m_clientMap[iClientId]->GetAddonCapabilities().bSupportsTimers;
}

int CPVRClients::GetTimers(CPVRTimers *timers)
{
  int iCurSize = timers->size();
  CLIENTMAP clients;
  GetConnectedClients(&clients);

  /* get the timer list from each client */
  boost::shared_ptr<CPVRClient> client;
  CLIENTMAPITR itrClients = clients.begin();
  while (itrClients != clients.end())
  {
    client = (*itrClients).second;
    if (client->GetAddonCapabilities().bSupportsTimers)
      client->GetTimers(timers);

    ++itrClients;
  }

  return timers->size() - iCurSize;
}

bool CPVRClients::AddTimer(const CPVRTimerInfoTag &timer, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKNOWN;
  boost::shared_ptr<CPVRClient> client;
  if (GetConnectedClient(timer.m_iClientId, client) && client->GetAddonCapabilities().bSupportsTimers)
    *error = client->AddTimer(timer);
  else
    CLog::Log(LOGERROR, "PVR - %s - client %d does not support timers",__FUNCTION__, timer.m_iClientId);

  return *error == PVR_ERROR_NO_ERROR;
}

bool CPVRClients::UpdateTimer(const CPVRTimerInfoTag &timer, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKNOWN;
  boost::shared_ptr<CPVRClient> client;
  if (GetConnectedClient(timer.m_iClientId, client) && client->GetAddonCapabilities().bSupportsTimers)
    *error = client->UpdateTimer(timer);
  else
    CLog::Log(LOGERROR, "PVR - %s - client %d doest not support timers",__FUNCTION__, timer.m_iClientId);

  return *error == PVR_ERROR_NO_ERROR;
}

bool CPVRClients::DeleteTimer(const CPVRTimerInfoTag &timer, bool bForce, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKNOWN;
  boost::shared_ptr<CPVRClient> client;
  if (GetConnectedClient(timer.m_iClientId, client) && client->GetAddonCapabilities().bSupportsTimers)
    *error = client->DeleteTimer(timer, bForce);
  else
    CLog::Log(LOGERROR, "PVR - %s - client %d does not support timers",__FUNCTION__, timer.m_iClientId);

  return *error == PVR_ERROR_NO_ERROR;
}

bool CPVRClients::RenameTimer(const CPVRTimerInfoTag &timer, const CStdString &strNewName, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKNOWN;
  boost::shared_ptr<CPVRClient> client;
  if (GetConnectedClient(timer.m_iClientId, client) && client->GetAddonCapabilities().bSupportsTimers)
    *error = client->RenameTimer(timer, strNewName);
  else
    CLog::Log(LOGERROR, "PVR - %s - client %d does not support timers",__FUNCTION__, timer.m_iClientId);

  return *error == PVR_ERROR_NO_ERROR;
}

bool CPVRClients::HasRecordingsSupport(int iClientId)
{
  CSingleLock lock(m_critSection);

  return IsConnectedClient(iClientId) && m_clientMap[iClientId]->GetAddonCapabilities().bSupportsRecordings;
}

int CPVRClients::GetRecordings(CPVRRecordings *recordings)
{
  int iCurSize = recordings->size();
  CLIENTMAP clients;
  GetConnectedClients(&clients);

  boost::shared_ptr<CPVRClient> client;
  CLIENTMAPITR itrClients = clients.begin();
  while (itrClients != clients.end())
  {
    client = (*itrClients).second;
    if (client->GetAddonCapabilities().bSupportsRecordings)
      client->GetRecordings(recordings);

    itrClients++;
  }

  return recordings->size() - iCurSize;
}

bool CPVRClients::RenameRecording(const CPVRRecording &recording, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKNOWN;
  boost::shared_ptr<CPVRClient> client;
  if (GetConnectedClient(recording.m_iClientId, client) && client->GetAddonCapabilities().bSupportsRecordings)
    *error = client->RenameRecording(recording);
  else
    CLog::Log(LOGERROR, "PVR - %s - client %d does not support recordings",__FUNCTION__, recording.m_iClientId);

  return *error == PVR_ERROR_NO_ERROR;
}

bool CPVRClients::DeleteRecording(const CPVRRecording &recording, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKNOWN;
  boost::shared_ptr<CPVRClient> client;
  if (GetConnectedClient(recording.m_iClientId, client) && client->GetAddonCapabilities().bSupportsRecordings)
    *error = client->DeleteRecording(recording);
  else
    CLog::Log(LOGERROR, "PVR - %s - client %d does not support recordings",__FUNCTION__, recording.m_iClientId);

  return *error == PVR_ERROR_NO_ERROR;
}

bool CPVRClients::IsRecordingOnPlayingChannel(void) const
{
  CPVRChannel currentChannel;
  return GetPlayingChannel(currentChannel) && currentChannel.IsRecording();
}

bool CPVRClients::CanRecordInstantly(void)
{
  CPVRChannel currentChannel;
  return GetPlayingChannel(currentChannel) && HasRecordingsSupport(currentChannel.ClientID());
}

bool CPVRClients::HasEPGSupport(int iClientId)
{
  CSingleLock lock(m_critSection);

  return IsConnectedClient(iClientId) && m_clientMap[iClientId]->GetAddonCapabilities().bSupportsEPG;
}

bool CPVRClients::GetEPGForChannel(const CPVRChannel &channel, CEpg *epg, time_t start, time_t end, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKNOWN;
  boost::shared_ptr<CPVRClient> client;
  if (GetConnectedClient(channel.ClientID(), client) && client->GetAddonCapabilities().bSupportsEPG)
    *error = client->GetEPGForChannel(channel, epg, start, end);
  else
    CLog::Log(LOGERROR, "PVR - %s - client %d does not support EPG",__FUNCTION__, channel.ClientID());

  return *error == PVR_ERROR_NO_ERROR;
}

int CPVRClients::GetChannels(CPVRChannelGroupInternal *group, PVR_ERROR *error)
{
  *error = PVR_ERROR_NO_ERROR;
  int iCurSize = group->Size();
  CLIENTMAP clients;
  GetConnectedClients(&clients);

  /* get the channel list from each client */
  boost::shared_ptr<CPVRClient> client;
  for (CLIENTMAPITR itrClients = clients.begin(); itrClients != clients.end(); itrClients++)
  {
    PVR_ERROR currentError;
    client = (*itrClients).second;

    if (group->IsRadio() && !client->GetAddonCapabilities().bSupportsRadio)
      continue;
    else if (!group->IsRadio() && !client->GetAddonCapabilities().bSupportsTV)
      continue;
    else if ((currentError = client->GetChannels(*group, group->IsRadio())) != PVR_ERROR_NO_ERROR)
      *error = currentError;
  }

  return group->Size() - iCurSize;
}

bool CPVRClients::HasChannelGroupSupport(int iClientId)
{
  CSingleLock lock(m_critSection);

  return IsConnectedClient(iClientId) && m_clientMap[iClientId]->GetAddonCapabilities().bSupportsChannelGroups;
}

int CPVRClients::GetChannelGroups(CPVRChannelGroups *groups, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKNOWN;
  int iCurSize = groups->size();
  CLIENTMAP clients;
  GetConnectedClients(&clients);

  /* get the channel groups list from each client */
  boost::shared_ptr<CPVRClient> client;
  CLIENTMAPITR itrClients = clients.begin();
  while (itrClients != clients.end())
  {
    client = (*itrClients).second;
    if (client->GetAddonCapabilities().bSupportsChannelGroups)
      client->GetChannelGroups(groups);

    itrClients++;
  }

  return groups->size() - iCurSize;
}

int CPVRClients::GetChannelGroupMembers(CPVRChannelGroup *group, PVR_ERROR *error)
{
  *error = PVR_ERROR_NO_ERROR;
  int iCurSize = group->Size();
  CLIENTMAP clients;
  GetConnectedClients(&clients);

  /* get the member list from each client */
  boost::shared_ptr<CPVRClient> client;
  CLIENTMAPITR itrClients = clients.begin();
  while (itrClients != clients.end())
  {
    client = (*itrClients).second;
    if (client->GetAddonCapabilities().bSupportsChannelGroups)
    {
      PVR_ERROR currentError;
      if ((currentError = client->GetChannelGroupMembers(group)) != PVR_ERROR_NO_ERROR)
        *error = currentError;
    }

    itrClients++;
  }

  return group->Size() - iCurSize;
}

bool CPVRClients::HasMenuHooks(int iClientID)
{
  if (iClientID < 0)
    iClientID = GetPlayingClientID();

  boost::shared_ptr<CPVRClient> client;
  return (GetConnectedClient(iClientID, client) &&
      client->HaveMenuHooks());
}

bool CPVRClients::GetMenuHooks(int iClientID, PVR_MENUHOOKS *hooks)
{
  bool bReturn(false);

  if (iClientID < 0)
    iClientID = GetPlayingClientID();

  boost::shared_ptr<CPVRClient> client;
  if (GetConnectedClient(iClientID, client) && client->HaveMenuHooks())
  {
    hooks = client->GetMenuHooks();
    bReturn = true;
  }

  return bReturn;
}

void CPVRClients::ProcessMenuHooks(int iClientID)
{
  PVR_MENUHOOKS *hooks = NULL;

  if (iClientID < 0)
    iClientID = GetPlayingClientID();

  boost::shared_ptr<CPVRClient> client;
  if (GetConnectedClient(iClientID, client) && client->HaveMenuHooks())
  {
    hooks = client->GetMenuHooks();
    std::vector<int> hookIDs;

    CGUIDialogSelect* pDialog = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
    pDialog->Reset();
    pDialog->SetHeading(19196);
    for (unsigned int i = 0; i < hooks->size(); i++)
      pDialog->Add(client->GetString(hooks->at(i).iLocalizedStringId));
    pDialog->DoModal();

    int selection = pDialog->GetSelectedLabel();
    if (selection >= 0)
      client->CallMenuHook(hooks->at(selection));
  }
}

bool CPVRClients::IsRunningChannelScan(void) const
{
  CSingleLock lock(m_critSection);
  return m_bChannelScanRunning;
}

void CPVRClients::StartChannelScan(void)
{
  CLIENTMAP clients;
  vector< boost::shared_ptr<CPVRClient> > possibleScanClients;
  boost::shared_ptr<CPVRClient> scanClient;
  CSingleLock lock(m_critSection);
  GetConnectedClients(&clients);
  m_bChannelScanRunning = true;

  /* get clients that support channel scanning */
  CLIENTMAPITR itr = m_clientMap.begin();
  while (itr != m_clientMap.end())
  {
    if (m_clientMap[(*itr).first]->ReadyToUse() && m_clientMap[(*itr).first]->GetAddonCapabilities().bSupportsChannelScan)
      possibleScanClients.push_back(m_clientMap[(*itr).first]);

    itr++;
  }

  /* multiple clients found */
  if (possibleScanClients.size() > 1)
  {
    CGUIDialogSelect* pDialog= (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);

    pDialog->Reset();
    pDialog->SetHeading(19119);

    for (unsigned int i = 0; i < possibleScanClients.size(); i++)
      pDialog->Add(possibleScanClients[i]->GetFriendlyName());

    pDialog->DoModal();

    int selection = pDialog->GetSelectedLabel();
    if (selection >= 0)
      scanClient = possibleScanClients[selection];
  }
  /* one client found */
  else if (possibleScanClients.size() == 1)
  {
    scanClient = possibleScanClients[0];
  }
  /* no clients found */
  else if (!scanClient)
  {
    CGUIDialogOK::ShowAndGetInput(19033,0,19192,0);
    return;
  }

  /* start the channel scan */
  CLog::Log(LOGNOTICE,"PVR - %s - starting to scan for channels on client %s",
      __FUNCTION__, scanClient->GetFriendlyName().c_str());
  long perfCnt = XbmcThreads::SystemClockMillis();

  /* stop the supervisor thread */
  g_PVRManager.StopUpdateThreads();

  /* do the scan */
  if (scanClient->StartChannelScan() != PVR_ERROR_NO_ERROR)
    /* an error occured */
    CGUIDialogOK::ShowAndGetInput(19111,0,19193,0);

  /* restart the supervisor thread */
  g_PVRManager.StartUpdateThreads();

  CLog::Log(LOGNOTICE, "PVRManager - %s - channel scan finished after %li.%li seconds",
      __FUNCTION__, (XbmcThreads::SystemClockMillis()-perfCnt)/1000, (XbmcThreads::SystemClockMillis()-perfCnt)%1000);
  m_bChannelScanRunning = false;
}

int CPVRClients::AddClientToDb(const AddonPtr client)
{
  /* add this client to the database if it's not in there yet */
  CPVRDatabase *database = OpenPVRDatabase();
  int iClientDbId = database ? database->AddClient(client->Name(), client->ID()) : -1;
  if (iClientDbId == -1)
  {
    CLog::Log(LOGERROR, "PVR - %s - can't add client '%s' to the database",
        __FUNCTION__, client->Name().c_str());
  }

  return iClientDbId;
}

bool CPVRClients::IsKnownClient(const AddonPtr client)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  for (CLIENTMAPITR itr = m_clientMap.begin(); itr != m_clientMap.end(); itr++)
  {
    if (m_clientMap[(*itr).first]->ID().Equals(client->ID()) &&
        m_clientMap[(*itr).first]->Name().Equals(client->Name()))
    {
      bReturn = true;
      break;
    }
  }

  return bReturn;
}

bool CPVRClients::InitialiseClient(AddonPtr client)
{
  bool bReturn(false);
  if (!client->Enabled())
    return bReturn;

  CLog::Log(LOGDEBUG, "%s - initialising add-on '%s'", __FUNCTION__, client->Name().c_str());

  /* register this client in the db */
  int iClientId = AddClientToDb(client);
  if (iClientId == -1)
    return bReturn;

  /* load and initialise the client libraries */
  boost::shared_ptr<CPVRClient> addon = boost::dynamic_pointer_cast<CPVRClient>(client);
  if (addon)
  {
    addon->Create(iClientId);
    if ((bReturn = addon->ReadyToUse()) == true)
    {
      CSingleLock lock(m_critSection);
      m_clientMap.insert(std::make_pair(iClientId, addon));
    }
  }

  if (!bReturn)
    CLog::Log(LOGERROR, "PVR - %s - can't initialise add-on '%s'", __FUNCTION__, client->Name().c_str());

  return bReturn;
}

bool CPVRClients::UpdateAndInitialiseClients(bool bInitialiseAllClients /* = false */)
{
  bool bReturn(true);
  ADDON::VECADDONS map;
  {
    CSingleLock lock(m_critSection);
    map = m_addons;
  }

  for (unsigned iClientPtr = 0; iClientPtr < map.size(); iClientPtr++)
  {
    const AddonPtr clientAddon = map.at(iClientPtr);

    if (!clientAddon->Enabled() && IsKnownClient(clientAddon))
    {
      /* stop the client and remove it from the db */
      bReturn &= StopClient(clientAddon, false) && bReturn;
    }
    else if (clientAddon->Enabled() && (bInitialiseAllClients || !IsKnownClient(clientAddon)))
    {
      /* register the new client and initialise it */
      bReturn &= InitialiseClient(clientAddon) && bReturn;
    }
  }

  /* check whether all clients are (still) connected */
  {
    CSingleLock lock(m_critSection);
    m_bAllClientsConnected = (ConnectedClientAmount() == EnabledClientAmount());
  }

  return bReturn;
}

void CPVRClients::ResetQualityData(PVR_SIGNAL_STATUS &qualityInfo)
{
  if (g_guiSettings.GetBool("pvrplayback.signalquality"))
  {
    strncpy(qualityInfo.strAdapterName, g_localizeStrings.Get(13205).c_str(), 1024);
    strncpy(qualityInfo.strAdapterStatus, g_localizeStrings.Get(13205).c_str(), 1024);
  }
  else
  {
    strncpy(qualityInfo.strAdapterName, g_localizeStrings.Get(13106).c_str(), 1024);
    strncpy(qualityInfo.strAdapterStatus, g_localizeStrings.Get(13106).c_str(), 1024);
  }
  qualityInfo.iSNR          = 0;
  qualityInfo.iSignal       = 0;
  qualityInfo.iSNR          = 0;
  qualityInfo.iUNC          = 0;
  qualityInfo.dVideoBitrate = 0;
  qualityInfo.dAudioBitrate = 0;
  qualityInfo.dDolbyBitrate = 0;
}

int CPVRClients::ReadLiveStream(void* lpBuf, int64_t uiBufSize)
{
  CSingleLock lock(m_critSection);
  return m_bIsPlayingLiveTV ? m_clientMap[m_currentChannel.ClientID()]->ReadLiveStream(lpBuf, uiBufSize) : 0;
}

int CPVRClients::ReadRecordedStream(void* lpBuf, int64_t uiBufSize)
{
  CSingleLock lock(m_critSection);
  return m_bIsPlayingRecording ? m_clientMap[m_currentRecording.m_iClientId]->ReadRecordedStream(lpBuf, uiBufSize) : 0;
}

void CPVRClients::Process(void)
{
  bool bCheckedEnabledClientsOnStartup(false);

  CAddonMgr::Get().RegisterAddonMgrCallback(ADDON_PVRDLL, this);
  CAddonMgr::Get().RegisterObserver(this);

  if (!UpdateAddons())
    return;

  while (!g_application.m_bStop && !m_bStop)
  {
    UpdateAndInitialiseClients();

    if (!bCheckedEnabledClientsOnStartup)
    {
      bCheckedEnabledClientsOnStartup = true;
      if (!HasEnabledClients())
        ShowDialogNoClientsEnabled();
    }
    UpdateCharInfoSignalStatus();
    Sleep(1000);
  }
}

void CPVRClients::ShowDialogNoClientsEnabled(void)
{
  CGUIDialogOK::ShowAndGetInput(19240, 19241, 19242, 19243);

  vector<CStdString> params;
  params.push_back("addons://enabled/xbmc.pvrclient");
  params.push_back("return");
  g_windowManager.ActivateWindow(WINDOW_ADDON_BROWSER, params);
}

void CPVRClients::UpdateCharInfoSignalStatus(void)
{
  CPVRChannel currentChannel;
  boost::shared_ptr<CPVRClient> client;
  PVR_SIGNAL_STATUS qualityInfo;
  ResetQualityData(qualityInfo);

  if (GetPlayingChannel(currentChannel) &&
      g_guiSettings.GetBool("pvrplayback.signalquality") &&
      !currentChannel.IsVirtual() &&
      GetConnectedClient(currentChannel.ClientID(), client))
  {
    client->SignalQuality(qualityInfo);
  }

  CSingleLock lock(m_critSection);
  m_qualityInfo = qualityInfo;
}

void CPVRClients::SaveCurrentChannelSettings(void)
{
  CPVRChannel channel;
  {
    CSingleLock lock(m_critSection);
    if (!GetPlayingChannel(channel) || !m_bIsValidChannelSettings)
      return;
  }

  CPVRDatabase *database = OpenPVRDatabase();
  if (!database)
    return;

  if (g_settings.m_currentVideoSettings != g_settings.m_defaultVideoSettings)
  {
    CLog::Log(LOGDEBUG, "PVR - %s - persisting custom channel settings for channel '%s'",
        __FUNCTION__, channel.ChannelName().c_str());
    database->PersistChannelSettings(channel, g_settings.m_currentVideoSettings);
  }
  else
  {
    CLog::Log(LOGDEBUG, "PVR - %s - no custom channel settings for channel '%s'",
        __FUNCTION__, channel.ChannelName().c_str());
    database->DeleteChannelSettings(channel);
  }

  database->Close();
}

void CPVRClients::LoadCurrentChannelSettings(void)
{
  CPVRChannel channel;
  {
    CSingleLock lock(m_critSection);
    if (!GetPlayingChannel(channel))
      return;
  }

  CPVRDatabase *database = OpenPVRDatabase();
  if (!database)
    return;

  if (g_application.m_pPlayer)
  {
    /* set the default settings first */
    CVideoSettings loadedChannelSettings = g_settings.m_defaultVideoSettings;

    /* try to load the settings from the database */
    database->GetChannelSettings(channel, loadedChannelSettings);
    database->Close();

    g_settings.m_currentVideoSettings = g_settings.m_defaultVideoSettings;
    g_settings.m_currentVideoSettings.m_Brightness          = loadedChannelSettings.m_Brightness;
    g_settings.m_currentVideoSettings.m_Contrast            = loadedChannelSettings.m_Contrast;
    g_settings.m_currentVideoSettings.m_Gamma               = loadedChannelSettings.m_Gamma;
    g_settings.m_currentVideoSettings.m_Crop                = loadedChannelSettings.m_Crop;
    g_settings.m_currentVideoSettings.m_CropLeft            = loadedChannelSettings.m_CropLeft;
    g_settings.m_currentVideoSettings.m_CropRight           = loadedChannelSettings.m_CropRight;
    g_settings.m_currentVideoSettings.m_CropTop             = loadedChannelSettings.m_CropTop;
    g_settings.m_currentVideoSettings.m_CropBottom          = loadedChannelSettings.m_CropBottom;
    g_settings.m_currentVideoSettings.m_CustomPixelRatio    = loadedChannelSettings.m_CustomPixelRatio;
    g_settings.m_currentVideoSettings.m_CustomZoomAmount    = loadedChannelSettings.m_CustomZoomAmount;
    g_settings.m_currentVideoSettings.m_CustomVerticalShift = loadedChannelSettings.m_CustomVerticalShift;
    g_settings.m_currentVideoSettings.m_NoiseReduction      = loadedChannelSettings.m_NoiseReduction;
    g_settings.m_currentVideoSettings.m_Sharpness           = loadedChannelSettings.m_Sharpness;
    g_settings.m_currentVideoSettings.m_InterlaceMethod     = loadedChannelSettings.m_InterlaceMethod;
    g_settings.m_currentVideoSettings.m_OutputToAllSpeakers = loadedChannelSettings.m_OutputToAllSpeakers;
    g_settings.m_currentVideoSettings.m_AudioDelay          = loadedChannelSettings.m_AudioDelay;
    g_settings.m_currentVideoSettings.m_AudioStream         = loadedChannelSettings.m_AudioStream;
    g_settings.m_currentVideoSettings.m_SubtitleOn          = loadedChannelSettings.m_SubtitleOn;
    g_settings.m_currentVideoSettings.m_SubtitleDelay       = loadedChannelSettings.m_SubtitleDelay;
    g_settings.m_currentVideoSettings.m_CustomNonLinStretch = loadedChannelSettings.m_CustomNonLinStretch;
    g_settings.m_currentVideoSettings.m_ScalingMethod       = loadedChannelSettings.m_ScalingMethod;
    g_settings.m_currentVideoSettings.m_PostProcess         = loadedChannelSettings.m_PostProcess;
    g_settings.m_currentVideoSettings.m_DeinterlaceMode     = loadedChannelSettings.m_DeinterlaceMode;

    /* only change the view mode if it's different */
    if (g_settings.m_currentVideoSettings.m_ViewMode != loadedChannelSettings.m_ViewMode)
    {
      g_settings.m_currentVideoSettings.m_ViewMode = loadedChannelSettings.m_ViewMode;

      g_renderManager.SetViewMode(g_settings.m_currentVideoSettings.m_ViewMode);
      g_settings.m_currentVideoSettings.m_CustomZoomAmount = g_settings.m_fZoomAmount;
      g_settings.m_currentVideoSettings.m_CustomPixelRatio = g_settings.m_fPixelRatio;
    }

    /* only change the subtitle stream, if it's different */
    if (g_settings.m_currentVideoSettings.m_SubtitleStream != loadedChannelSettings.m_SubtitleStream)
    {
      g_settings.m_currentVideoSettings.m_SubtitleStream = loadedChannelSettings.m_SubtitleStream;

      g_application.m_pPlayer->SetSubtitle(g_settings.m_currentVideoSettings.m_SubtitleStream);
    }

    /* only change the audio stream if it's different */
    if (g_application.m_pPlayer->GetAudioStream() != g_settings.m_currentVideoSettings.m_AudioStream)
      g_application.m_pPlayer->SetAudioStream(g_settings.m_currentVideoSettings.m_AudioStream);

    g_application.m_pPlayer->SetAVDelay(g_settings.m_currentVideoSettings.m_AudioDelay);
    g_application.m_pPlayer->SetDynamicRangeCompression((long)(g_settings.m_currentVideoSettings.m_VolumeAmplification * 100));
    g_application.m_pPlayer->SetSubtitleVisible(g_settings.m_currentVideoSettings.m_SubtitleOn);
    g_application.m_pPlayer->SetSubTitleDelay(g_settings.m_currentVideoSettings.m_SubtitleDelay);

    /* settings can be saved on next channel switch */
    m_bIsValidChannelSettings = true;
  }
}

bool CPVRClients::UpdateAddons(void)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  if ((bReturn = CAddonMgr::Get().GetAddons(ADDON_PVRDLL, m_addons, true)) == false)
    CLog::Log(LOGERROR, "%s - failed to get add-ons from the add-on manager", __FUNCTION__);

  return bReturn;
}

void CPVRClients::Notify(const Observable &obs, const CStdString& msg)
{
  if (msg.Equals("addons"))
  {
    UpdateAddons();
    UpdateAndInitialiseClients();
  }
}
