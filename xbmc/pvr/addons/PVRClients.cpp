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
#include "utils/TimeUtils.h"
#include "guilib/GUIWindowManager.h"
#include "settings/AdvancedSettings.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/channels/PVRChannelGroupInternal.h"

using namespace std;
using namespace ADDON;
using namespace PVR;

CPVRClients::CPVRClients(void) :
    m_bChannelScanRunning(false),
    m_bAllClientsLoaded(false),
    m_currentChannel(NULL),
    m_currentRecording(NULL),
    m_scanStart(0),
    m_strPlayingClientName("")
{
  ResetQualityData();
}

CPVRClients::~CPVRClients(void)
{
  Unload();
}

void CPVRClients::Start(void)
{
  Stop();

  Create();
  SetName("XBMC PVR backend info");
  SetPriority(-1);
}

void CPVRClients::Stop(void)
{
  StopThread();
}

bool CPVRClients::IsValidClient(int iClientId)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  CLIENTMAPITR itr = m_clientMap.find(iClientId);
  if (itr != m_clientMap.end() && itr->second->ReadyToUse())
    bReturn = true;

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

bool CPVRClients::TryLoadClients(int iMaxTime /* = 0 */)
{
  CDateTime start = CDateTime::GetCurrentDateTime();
  CSingleLock lock(m_critSection);

  while (!m_bAllClientsLoaded)
  {
    /* try to load clients */
    LoadClients();

    /* always break if the pvrmanager's thread is stopped */
    if (!g_PVRManager.IsRunning())
      break;

    /* check whether iMaxTime has passed */
    if (!m_bAllClientsLoaded && iMaxTime > 0)
    {
      CDateTimeSpan elapsed = CDateTime::GetCurrentDateTime() - start;
      if (elapsed.GetSeconds() >= iMaxTime)
        break;
    }

    /* break if there are no activated clients */
    if (m_clientMap.empty())
      break;

    lock.Leave();
    Sleep(250);
    lock.Enter();
  }

  CLog::Log(LOG_DEBUG, "PVR - %s - %s",
      __FUNCTION__, m_bAllClientsLoaded && !m_clientMap.empty() ? "all clients loaded" : "couldn't load all clients. will keep trying in a separate thread.");
  return m_bAllClientsLoaded;
}

void CPVRClients::Unload(void)
{
  Stop();

  CSingleLock lock(m_critSection);

  /* destroy all clients */
  for (CLIENTMAPITR itr = m_clientMap.begin(); itr != m_clientMap.end(); itr++)
  {
    boost::shared_ptr<CPVRClient> client = m_clientMap[(*itr).first];
    CLog::Log(LOGDEBUG, "PVR - %s - destroying addon '%s' (%s)",
        __FUNCTION__, client->Name().c_str(), client->ID().c_str());

    client->Destroy();
  }

  /* reset class properties */
  m_bChannelScanRunning  = false;
  m_bAllClientsLoaded    = false;
  m_currentChannel       = NULL;
  m_currentRecording     = NULL;
  m_scanStart            = 0;
  m_strPlayingClientName = "";

  m_clientsProps.clear();
  m_clientMap.clear();
  ResetQualityData();
}

int CPVRClients::GetFirstID(void)
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

bool CPVRClients::AllClientsLoaded(void) const
{
  CSingleLock lock(m_critSection);
  return m_bAllClientsLoaded;
}

bool CPVRClients::HasClients(void) const
{
  CSingleLock lock(m_critSection);
  return !m_clientMap.empty();
}

bool CPVRClients::StopClient(AddonPtr client, bool bRestart)
{
  bool bReturn(false);
  if (!client)
    return bReturn;

  CSingleLock lock(m_critSection);
  CLIENTMAPITR itr = m_clientMap.begin();
  while (itr != m_clientMap.end())
  {
    if (m_clientMap[(*itr).first]->ID() == client->ID())
    {
      CLog::Log(LOGINFO, "PVRManager - %s - %s client '%s'",
          __FUNCTION__, bRestart ? "restarting" : "removing", m_clientMap[(*itr).first]->Name().c_str());

      g_PVRManager.StopUpdateThreads();
      if (bRestart)
      {
        m_clientMap[(*itr).first]->ReCreate();
      }
      else
      {
        m_clientMap[(*itr).first]->Destroy();
        m_clientMap.erase((*itr).first);
      }
      g_PVRManager.StartUpdateThreads();

      bReturn = true;
      break;
    }
    itr++;
  }

  return bReturn;
}

int CPVRClients::ActiveClientAmount(void)
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

bool CPVRClients::HasActiveClients(void)
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

const CStdString CPVRClients::GetClientName(int iClientId)
{
  static CStdString strClientName;
  CSingleLock lock(m_critSection);

  if (IsValidClient(iClientId))
    strClientName = m_clientMap.find(iClientId)->second->GetFriendlyName();
  else
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, iClientId);

  return strClientName;
}

int CPVRClients::GetActiveClients(CLIENTMAP *clients)
{
  int iReturn(0);
  CSingleLock lock(m_critSection);

  if (m_clientMap.size() > 0)
  {
    CLIENTMAPITR itr = m_clientMap.begin();
    while (itr != m_clientMap.end())
    {
      if (m_clientMap[(*itr).first]->ReadyToUse())
      {
        clients->insert(std::make_pair(m_clientMap[(*itr).first]->GetID(), m_clientMap[(*itr).first]));
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

  if (m_currentChannel)
    iReturn = m_currentChannel->ClientID();
  else if (m_currentRecording)
    iReturn = m_currentRecording->m_iClientId;

  return iReturn;
}

PVR_ADDON_CAPABILITIES *CPVRClients::GetAddonCapabilities(int iClientId)
{
  PVR_ADDON_CAPABILITIES *props = NULL;
  CSingleLock lock(m_critSection);

  if (IsValidClient(iClientId))
    props = &m_clientsProps[iClientId];

  return props;
}

PVR_ADDON_CAPABILITIES *CPVRClients::GetCurrentAddonCapabilities(void)
{
  PVR_ADDON_CAPABILITIES *props = NULL;
  CSingleLock lock(m_critSection);

  if (m_currentChannel)
    props = &m_clientsProps[m_currentChannel->ClientID()];
  else if (m_currentRecording)
    props = &m_clientsProps[m_currentRecording->m_iClientId];

  return props;
}

bool CPVRClients::IsPlaying(void) const
{
  CSingleLock lock(m_critSection);
  return m_currentRecording != NULL || m_currentChannel != NULL;
}

const CStdString CPVRClients::GetPlayingClientName(void) const
{
  CSingleLock lock(m_critSection);
  return m_strPlayingClientName;
}

int CPVRClients::ReadStream(void* lpBuf, int64_t uiBufSize)
{
  CSingleLock lock(m_critSection);

  /* Check stream for available video or audio data, if after the scantime no stream
     is present playback is canceled and returns to the window */
  if (m_scanStart)
  {
    if (CTimeUtils::GetTimeMS() - m_scanStart > (unsigned int) g_guiSettings.GetInt("pvrplayback.scantime")*1000)
    {
      CLog::Log(LOGERROR,"PVRManager - %s - no video or audio data available after %i seconds, playback stopped",
          __FUNCTION__, g_guiSettings.GetInt("pvrplayback.scantime"));
      {
        return 0;
      }
    }
    else if (g_application.IsPlayingVideo() || g_application.IsPlayingAudio())
      m_scanStart = 0;
  }

  if (m_currentChannel)
    return m_clientMap[m_currentChannel->ClientID()]->ReadLiveStream(lpBuf, uiBufSize);
  else if (m_currentRecording)
    return m_clientMap[m_currentRecording->m_iClientId]->ReadRecordedStream(lpBuf, uiBufSize);

  return 0;
}

int64_t CPVRClients::LengthStream(void)
{
  int64_t streamLength(0);
  CSingleLock lock(m_critSection);

  if (m_currentChannel)
    streamLength = 0;
  else if (m_currentRecording)
    streamLength = m_clientMap[m_currentRecording->m_iClientId]->LengthRecordedStream();

  return streamLength;
}

int64_t CPVRClients::SeekStream(int64_t iFilePosition, int iWhence/* = SEEK_SET*/)
{
  int64_t streamNewPos(0);
  CSingleLock lock(m_critSection);

  if (m_currentChannel)
    streamNewPos = 0;
  else if (m_currentRecording)
    streamNewPos = m_clientMap[m_currentRecording->m_iClientId]->SeekRecordedStream(iFilePosition, iWhence);

  return streamNewPos;
}

int64_t CPVRClients::GetStreamPosition(void)
{
  int64_t streamPos(0);
  CSingleLock lock(m_critSection);

  if (m_currentChannel)
    streamPos = 0;
  else if (m_currentRecording)
    streamPos = m_clientMap[m_currentRecording->m_iClientId]->PositionRecordedStream();

  return streamPos;
}

void CPVRClients::CloseStream(void)
{
  CSingleLock lock(m_critSection);
  CloseLiveStream() || CloseRecordedStream();
  m_strPlayingClientName = "";
}

PVR_STREAM_PROPERTIES *CPVRClients::GetCurrentStreamProperties(void)
{
  PVR_STREAM_PROPERTIES *props = NULL;
  CSingleLock lock(m_critSection);

  if (m_currentChannel)
  {
    int iChannelId = m_currentChannel->ClientID();
    m_clientMap[iChannelId]->GetStreamProperties(&m_streamProps[iChannelId]);

    props = &m_streamProps[iChannelId];
  }

  return props;
}

const char *CPVRClients::GetCurrentInputFormat(void) const
{
  static CStdString strReturn("");
  CSingleLock lock(m_critSection);

  if (m_currentChannel)
    strReturn = m_currentChannel->InputFormat();

  return strReturn.c_str();
}

bool CPVRClients::IsReadingLiveStream(void) const
{
  CSingleLock lock(m_critSection);
  return m_currentChannel != NULL;
}

bool CPVRClients::IsPlayingTV(void) const
{
  CSingleLock lock(m_critSection);
  return m_currentChannel != NULL && !m_currentChannel->IsRadio();
}

bool CPVRClients::IsPlayingRadio(void) const
{
  CSingleLock lock(m_critSection);
  return m_currentChannel != NULL && m_currentChannel->IsRadio();
}

bool CPVRClients::IsEncrypted(void) const
{
  CSingleLock lock(m_critSection);
  return m_currentChannel != NULL && m_currentChannel->IsEncrypted();
}

bool CPVRClients::OpenLiveStream(const CPVRChannel &tag)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  if (m_currentChannel)
    delete m_currentChannel;
  if (m_currentRecording)
    delete m_currentRecording;

  ResetQualityData();

  /* try to open the stream on the client */
  if ((tag.StreamURL().IsEmpty() == false ||
      m_clientsProps[tag.ClientID()].bHandlesInputStream) &&
      m_clientMap[tag.ClientID()]->OpenLiveStream(tag))
  {
    m_currentChannel = &tag;
    if (tag.ClientID() == XBMC_VIRTUAL_CLIENTID)
      m_strPlayingClientName = g_localizeStrings.Get(19209);
    else if (!tag.IsVirtual())
      m_strPlayingClientName = GetClientName(tag.ClientID());
    else
      m_strPlayingClientName = g_localizeStrings.Get(13205);

    m_scanStart = CTimeUtils::GetTimeMS();  /* Reset the stream scan timer */
    bReturn = true;
  }

  return bReturn;
}

bool CPVRClients::CloseLiveStream(void)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);
  ResetQualityData();

  if (!m_currentChannel)
    return bReturn;

  if ((m_currentChannel->StreamURL().IsEmpty()) || (m_currentChannel->StreamURL().compare(0,13, "pvr://stream/") == 0))
  {
    m_clientMap[m_currentChannel->ClientID()]->CloseLiveStream();
    m_currentChannel = NULL;
    bReturn = true;
  }

  return bReturn;
}

const char *CPVRClients::GetStreamURL(const CPVRChannel &tag)
{
  static CStdString strReturn("");
  boost::shared_ptr<CPVRClient> client;
  if (GetValidClient(tag.ClientID(), client))
    strReturn = client->GetLiveStreamURL(tag);
  else
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, tag.ClientID());

  return strReturn.c_str();
}

bool CPVRClients::SwitchChannel(const CPVRChannel &channel)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);
  if (m_currentChannel && m_currentChannel->ClientID() != channel.ClientID())
  {
    lock.Leave();
    CloseStream();
    return OpenLiveStream(channel);
  }

  boost::shared_ptr<CPVRClient> client;
  if (GetValidClient(channel.ClientID(), client))
  {
    if (client->SwitchChannel(channel))
    {
      m_currentChannel = &channel;
      m_scanStart = CTimeUtils::GetTimeMS();  /* Reset the stream scan timer */
      ResetQualityData();

      bReturn = true;
    }
    else
    {
      CLog::Log(LOGERROR, "PVR - %s - cannot switch channel on client %d",__FUNCTION__, channel.ClientID());
    }
  }
  else
  {
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, channel.ClientID());
  }

  return bReturn;
}

bool CPVRClients::GetPlayingChannel(CPVRChannel *channel) const
{
  CSingleLock lock(m_critSection);

  if (m_currentChannel != NULL)
    *channel = *m_currentChannel;
  else
    channel = NULL;

  return m_currentChannel != NULL;
}

bool CPVRClients::IsPlayingRecording(void) const
{
  CSingleLock lock(m_critSection);
  return m_currentRecording != NULL;
}

bool CPVRClients::OpenRecordedStream(const CPVRRecording &tag)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  if (m_currentChannel)
    delete m_currentChannel;
  if (m_currentRecording)
    delete m_currentRecording;

  /* try to open the recording stream on the client */
  if (m_clientMap[tag.m_iClientId]->OpenRecordedStream(tag))
  {
    m_currentRecording = &tag;
    m_strPlayingClientName = GetClientName(tag.m_iClientId);
    m_scanStart = CTimeUtils::GetTimeMS();  /* Reset the stream scan timer */
    bReturn = true;
  }

  return bReturn;
}

bool CPVRClients::CloseRecordedStream(void)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  if (!m_currentRecording)
    return bReturn;

  if (m_currentRecording->m_iClientId > 0 && m_clientMap[m_currentRecording->m_iClientId])
  {
    m_clientMap[m_currentRecording->m_iClientId]->CloseRecordedStream();
    bReturn = true;
  }

  return bReturn;
}

bool CPVRClients::GetPlayingRecording(CPVRRecording *recording) const
{
  CSingleLock lock(m_critSection);
  if (m_currentRecording != NULL)
    *recording = *m_currentRecording;

  return m_currentRecording != NULL;
}

void CPVRClients::DemuxReset(void)
{
  /* don't lock here cause it'll cause a dead lock when the client connection is dropped while playing */
  if (m_currentChannel)
    m_clientMap[m_currentChannel->ClientID()]->DemuxReset();
}

void CPVRClients::DemuxAbort(void)
{
  /* don't lock here cause it'll cause a dead lock when the client connection is dropped while playing */
  if (m_currentChannel)
    m_clientMap[m_currentChannel->ClientID()]->DemuxAbort();
}

void CPVRClients::DemuxFlush(void)
{
  /* don't lock here cause it'll cause a dead lock when the client connection is dropped while playing */
  if (m_currentChannel)
    m_clientMap[m_currentChannel->ClientID()]->DemuxFlush();
}

DemuxPacket* CPVRClients::ReadDemuxStream(void)
{
  /* don't lock here cause it'll cause a dead lock when the client connection is dropped while playing */
  if (m_currentChannel)
    return m_clientMap[m_currentChannel->ClientID()]->DemuxRead();

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

  return IsValidClient(iClientId) && m_clientsProps[iClientId].bSupportsTimers;
}

int CPVRClients::GetTimers(CPVRTimers *timers)
{
  int iCurSize = timers->size();
  CLIENTMAP clients;
  GetActiveClients(&clients);

  /* get the timer list from each client */
  CLIENTMAPITR itrClients = clients.begin();
  while (itrClients != clients.end())
  {
    if (HasTimerSupport((*itrClients).first))
      (*itrClients).second->GetTimers(timers);

    ++itrClients;
  }

  return timers->size() - iCurSize;
}

bool CPVRClients::AddTimer(const CPVRTimerInfoTag &timer, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKOWN;
  boost::shared_ptr<CPVRClient> client;
  if (HasTimerSupport(timer.m_iClientId) && GetValidClient(timer.m_iClientId, client))
    *error = client->AddTimer(timer);
  else
    CLog::Log(LOGERROR, "PVR - %s - client %d does not support timers",__FUNCTION__, timer.m_iClientId);

  return *error == PVR_ERROR_NO_ERROR;
}

bool CPVRClients::UpdateTimer(const CPVRTimerInfoTag &timer, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKOWN;
  boost::shared_ptr<CPVRClient> client;
  if (HasTimerSupport(timer.m_iClientId) && GetValidClient(timer.m_iClientId, client))
    *error = client->UpdateTimer(timer);
  else
    CLog::Log(LOGERROR, "PVR - %s - client %d doest not support timers",__FUNCTION__, timer.m_iClientId);

  return *error == PVR_ERROR_NO_ERROR;
}

bool CPVRClients::DeleteTimer(const CPVRTimerInfoTag &timer, bool bForce, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKOWN;
  boost::shared_ptr<CPVRClient> client;
  if (HasTimerSupport(timer.m_iClientId) && GetValidClient(timer.m_iClientId, client))
    *error = client->DeleteTimer(timer, bForce);
  else
    CLog::Log(LOGERROR, "PVR - %s - client %d does not support timers",__FUNCTION__, timer.m_iClientId);

  return *error == PVR_ERROR_NO_ERROR;
}

bool CPVRClients::RenameTimer(const CPVRTimerInfoTag &timer, const CStdString &strNewName, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKOWN;
  boost::shared_ptr<CPVRClient> client;
  if (HasTimerSupport(timer.m_iClientId) && GetValidClient(timer.m_iClientId, client))
    *error = client->RenameTimer(timer, strNewName);
  else
    CLog::Log(LOGERROR, "PVR - %s - client %d does not support timers",__FUNCTION__, timer.m_iClientId);

  return *error == PVR_ERROR_NO_ERROR;
}

bool CPVRClients::HasRecordingsSupport(int iClientId)
{
  CSingleLock lock(m_critSection);

  return IsValidClient(iClientId) && m_clientsProps[iClientId].bSupportsRecordings;
}

int CPVRClients::GetRecordings(CPVRRecordings *recordings)
{
  int iCurSize = recordings->size();
  CLIENTMAP clients;
  GetActiveClients(&clients);

  CLIENTMAPITR itrClients = clients.begin();
  while (itrClients != clients.end())
  {
    if (HasRecordingsSupport((*itrClients).first))
      (*itrClients).second->GetRecordings(recordings);

    itrClients++;
  }

  return recordings->size() - iCurSize;
}

bool CPVRClients::RenameRecording(const CPVRRecording &recording, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKOWN;
  boost::shared_ptr<CPVRClient> client;
  if (HasRecordingsSupport(recording.m_iClientId) && GetValidClient(recording.m_iClientId, client))
    *error = client->RenameRecording(recording);
  else
    CLog::Log(LOGERROR, "PVR - %s - client %d does not support recordings",__FUNCTION__, recording.m_iClientId);

  return *error == PVR_ERROR_NO_ERROR;
}

bool CPVRClients::DeleteRecording(const CPVRRecording &recording, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKOWN;
  boost::shared_ptr<CPVRClient> client;
  if (HasRecordingsSupport(recording.m_iClientId) && GetValidClient(recording.m_iClientId, client))
    *error = client->DeleteRecording(recording);
  else
    CLog::Log(LOGERROR, "PVR - %s - client %d does not support recordings",__FUNCTION__, recording.m_iClientId);

  return *error == PVR_ERROR_NO_ERROR;
}

bool CPVRClients::IsRecordingOnPlayingChannel(void) const
{
  return m_currentChannel && m_currentChannel->IsRecording();
}

bool CPVRClients::CanRecordInstantly(void)
{
  return m_currentChannel != NULL && HasRecordingsSupport(m_currentChannel->ClientID());
}

bool CPVRClients::HasEPGSupport(int iClientId)
{
  CSingleLock lock(m_critSection);

  return IsValidClient(iClientId) && m_clientsProps[iClientId].bSupportsEPG;
}

bool CPVRClients::GetEPGForChannel(const CPVRChannel &channel, CPVREpg *epg, time_t start, time_t end, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKOWN;
  boost::shared_ptr<CPVRClient> client;
  if (HasEPGSupport(channel.ClientID()) && GetValidClient(channel.ClientID(), client))
    *error = client->GetEPGForChannel(channel, epg, start, end);
  else
    CLog::Log(LOGERROR, "PVR - %s - client %d does not support EPG",__FUNCTION__, channel.ClientID());

  return *error == PVR_ERROR_NO_ERROR;
}

int CPVRClients::GetChannels(CPVRChannelGroupInternal *group, PVR_ERROR *error)
{
  *error = PVR_ERROR_NO_ERROR;
  int iCurSize = group->GetNumChannels();
  CLIENTMAP clients;
  GetActiveClients(&clients);

  /* get the channel list from each client */
  CLIENTMAPITR itrClients = clients.begin();
  while (itrClients != clients.end())
  {
    if ((*itrClients).second->ReadyToUse())
    {
      PVR_ERROR currentError;
      currentError = (*itrClients).second->GetChannels(*group, group->IsRadio());

      if (currentError != PVR_ERROR_NO_ERROR)
        *error = currentError;
    }

    itrClients++;
  }

  return group->GetNumChannels() - iCurSize;
}

bool CPVRClients::HasChannelGroupSupport(int iClientId)
{
  CSingleLock lock(m_critSection);

  return IsValidClient(iClientId) && m_clientsProps[iClientId].bSupportsChannelGroups;
}

int CPVRClients::GetChannelGroups(CPVRChannelGroups *groups, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKOWN;
  int iCurSize = groups->size();
  CLIENTMAP clients;
  GetActiveClients(&clients);

  /* get the channel groups list from each client */
  CLIENTMAPITR itrClients = clients.begin();
  while (itrClients != clients.end())
  {
    if (HasChannelGroupSupport((*itrClients).first))
      (*itrClients).second->GetChannelGroups(groups);

    itrClients++;
  }

  return groups->size() - iCurSize;
}

int CPVRClients::GetChannelGroupMembers(CPVRChannelGroup *group, PVR_ERROR *error)
{
  *error = PVR_ERROR_NO_ERROR;
  int iCurSize = group->GetNumChannels();
  CLIENTMAP clients;
  GetActiveClients(&clients);

  /* get the member list from each client */
  CLIENTMAPITR itrClients = clients.begin();
  while (itrClients != clients.end())
  {
    if (HasChannelGroupSupport((*itrClients).first))
    {
      PVR_ERROR currentError;
      currentError = (*itrClients).second->GetChannelGroupMembers(group);

      if (currentError != PVR_ERROR_NO_ERROR)
        *error = currentError;
    }

    itrClients++;
  }

  return group->GetNumChannels() - iCurSize;
}

bool CPVRClients::HasMenuHooks(int iClientID)
{
  bool bReturn(false);

  if (iClientID < 0)
    iClientID = GetPlayingClientID();

  boost::shared_ptr<CPVRClient> client;
  if (GetValidClient(iClientID, client))
    bReturn = client->HaveMenuHooks();

  return bReturn;
}

bool CPVRClients::GetMenuHooks(int iClientID, PVR_MENUHOOKS *hooks)
{
  bool bReturn(false);

  if (iClientID < 0)
    iClientID = GetPlayingClientID();

  boost::shared_ptr<CPVRClient> client;
  if (GetValidClient(iClientID, client))
  {
    if (client->HaveMenuHooks())
    {
      hooks = client->GetMenuHooks();
      bReturn = true;
    }
  }

  return bReturn;
}

void CPVRClients::ProcessMenuHooks(int iClientID)
{
  PVR_MENUHOOKS *hooks = NULL;

  if (iClientID < 0)
    iClientID = GetPlayingClientID();

  if (GetMenuHooks(iClientID, hooks))
  {
    boost::shared_ptr<CPVRClient> client;
    if (!GetValidClient(iClientID, client))
      return;
    std::vector<int> hookIDs;

    CGUIDialogSelect* pDialog = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
    pDialog->Reset();
    pDialog->SetHeading(19196);
    for (unsigned int i = 0; i < hooks->size(); i++)
      pDialog->Add(client->GetString(hooks->at(i).iLocalizedStringId));
    pDialog->DoModal();

    int selection = pDialog->GetSelectedLabel();
    if (selection >= 0)
    {
      client->CallMenuHook(hooks->at(selection));
    }
  }
  else
  {
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, iClientID);
  }
}

bool CPVRClients::IsRunningChannelScan(void) const
{
  CSingleLock lock(m_critSection);
  return m_bChannelScanRunning;
}

void CPVRClients::StartChannelScan(void)
{
  vector<int> clients;
  int scanningClientID = -1;
  CSingleLock lock(m_critSection);
  m_bChannelScanRunning = true;

  /* get clients that support channel scanning */
  CLIENTMAPITR itr = m_clientMap.begin();
  while (itr != m_clientMap.end())
  {
    if (m_clientMap[(*itr).first]->ReadyToUse() && m_clientsProps[m_clientMap[(*itr).first]->GetID()].bSupportsChannelScan)
      clients.push_back(m_clientMap[(*itr).first]->GetID());

    itr++;
  }

  /* multiple clients found */
  if (clients.size() > 1)
  {
    CGUIDialogSelect* pDialog= (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);

    pDialog->Reset();
    pDialog->SetHeading(19119);

    for (unsigned int i = 0; i < clients.size(); i++)
      pDialog->Add(m_clientMap[clients[i]]->GetFriendlyName());

    pDialog->DoModal();

    int selection = pDialog->GetSelectedLabel();
    if (selection >= 0)
      scanningClientID = clients[selection];
  }
  /* one client found */
  else if (clients.size() == 1)
  {
    scanningClientID = clients[0];
  }
  /* no clients found */
  else if (scanningClientID < 0)
  {
    CGUIDialogOK::ShowAndGetInput(19033,0,19192,0);
    return;
  }

  /* start the channel scan */
  CLog::Log(LOGNOTICE,"PVR - %s - starting to scan for channels on client %s",
      __FUNCTION__, m_clientMap[scanningClientID]->GetFriendlyName());
  long perfCnt = CTimeUtils::GetTimeMS();

  /* stop the supervisor thread */
  g_PVRManager.StopUpdateThreads();

  /* do the scan */
  if (m_clientMap[scanningClientID]->StartChannelScan() != PVR_ERROR_NO_ERROR)
    /* an error occured */
    CGUIDialogOK::ShowAndGetInput(19111,0,19193,0);

  /* restart the supervisor thread */
  g_PVRManager.StartUpdateThreads();

  CLog::Log(LOGNOTICE, "PVRManager - %s - channel scan finished after %li.%li seconds",
      __FUNCTION__, (CTimeUtils::GetTimeMS()-perfCnt)/1000, (CTimeUtils::GetTimeMS()-perfCnt)%1000);
  m_bChannelScanRunning = false;
}

bool CPVRClients::GetValidClient(int iClientId, boost::shared_ptr<CPVRClient> &addon)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  CLIENTMAPITR itr = m_clientMap.find(iClientId);
  if (itr != m_clientMap.end() && itr->second->ReadyToUse())
  {
    addon = itr->second;
    bReturn = true;
  }

  return bReturn;
}

int CPVRClients::AddClientToDb(const CStdString &strClientId, const CStdString &strName)
{
  /* add this client to the database if it's not in there yet */
  CPVRDatabase *database = OpenPVRDatabase();
  int iClientDbId = database ? database->AddClient(strName, strClientId) : -1;
  if (iClientDbId == -1)
  {
    CLog::Log(LOGERROR, "PVR - %s - can't add client '%s' to the database",
        __FUNCTION__, strName.c_str());
  }

  return iClientDbId;
}

bool CPVRClients::LoadClients(void)
{
  CSingleLock lock(m_critSection);
  if (m_bAllClientsLoaded)
    return !m_clientMap.empty();

  CAddonMgr::Get().RegisterAddonMgrCallback(ADDON_PVRDLL, this);

  /* get all PVR addons */
  VECADDONS addons;
  if (!CAddonMgr::Get().GetAddons(ADDON_PVRDLL, addons, true))
    return false;

  /* load and initialise the clients */
  CPVRDatabase *database = OpenPVRDatabase();
  if (!database)
    return false;

  m_bAllClientsLoaded = true;
  for (unsigned iClientPtr = 0; iClientPtr < addons.size(); iClientPtr++)
  {
    const AddonPtr clientAddon = addons.at(iClientPtr);
    if (!clientAddon->Enabled())
      continue;

    int iClientId = AddClientToDb(clientAddon->ID(), clientAddon->Name());
    if (iClientId == -1)
      continue; // don't set "m_bAllClientsLoaded = false;" here because this will enter a neverending loop

    /* check if this client isn't active already */
    if (IsValidClient(iClientId))
      continue;

    /* load and initialise the client libraries */
    boost::shared_ptr<CPVRClient> addon = boost::dynamic_pointer_cast<CPVRClient>(clientAddon);
    if (addon && addon->Create(iClientId))
    {
      /* get the client's properties */
      PVR_ADDON_CAPABILITIES props;
      if (addon->GetAddonCapabilities(&props) == PVR_ERROR_NO_ERROR)
      {
        m_clientMap.insert(std::make_pair(iClientId, addon));
        m_clientsProps.insert(std::make_pair(iClientId, props));
      }
      else
      {
        CLog::Log(LOGERROR, "PVR - %s - can't get client properties from addon '%s'",
            __FUNCTION__, clientAddon->Name().c_str());
        m_bAllClientsLoaded = false;
      }
    }
    else
    {
      CLog::Log(LOGERROR, "PVR - %s - can't initialise client '%s'",
          __FUNCTION__, clientAddon->Name().c_str());
      m_bAllClientsLoaded = false;
    }
  }

  database->Close();

  return !m_clientMap.empty();
}

void CPVRClients::ResetQualityData(void)
{
  if (g_guiSettings.GetBool("pvrplayback.signalquality"))
  {
    strncpy(m_qualityInfo.strAdapterName, g_localizeStrings.Get(13205).c_str(), 1024);
    strncpy(m_qualityInfo.strAdapterStatus, g_localizeStrings.Get(13205).c_str(), 1024);
  }
  else
  {
    strncpy(m_qualityInfo.strAdapterName, g_localizeStrings.Get(13106).c_str(), 1024);
    strncpy(m_qualityInfo.strAdapterStatus, g_localizeStrings.Get(13106).c_str(), 1024);
  }
  m_qualityInfo.iSNR          = 0;
  m_qualityInfo.iSignal       = 0;
  m_qualityInfo.iSNR          = 0;
  m_qualityInfo.iUNC          = 0;
  m_qualityInfo.dVideoBitrate = 0;
  m_qualityInfo.dAudioBitrate = 0;
  m_qualityInfo.dDolbyBitrate = 0;
}

int CPVRClients::ReadLiveStream(void* lpBuf, int64_t uiBufSize)
{
  CSingleLock lock(m_critSection);
  return m_currentChannel ? m_clientMap[m_currentChannel->ClientID()]->ReadLiveStream(lpBuf, uiBufSize) : 0;
}

int CPVRClients::ReadRecordedStream(void* lpBuf, int64_t uiBufSize)
{
  CSingleLock lock(m_critSection);
  return m_currentRecording ? m_clientMap[m_currentRecording->m_iClientId]->ReadRecordedStream(lpBuf, uiBufSize) : 0;
}

void CPVRClients::Process(void)
{
  while (!m_bStop)
  {
    UpdateCharInfoSignalStatus();
    Sleep(1000);
  }
}

void CPVRClients::UpdateCharInfoSignalStatus(void)
{
  CSingleLock lock(m_critSection);

  if (m_currentChannel && g_guiSettings.GetBool("pvrplayback.signalquality") &&
      !m_currentChannel->IsVirtual() && m_currentChannel->ClientID() >= 0 &&
      m_clientMap[m_currentChannel->ClientID()])
  {
    m_clientMap[m_currentChannel->ClientID()]->SignalQuality(m_qualityInfo);
  }
  else
  {
    ResetQualityData();
  }
}
