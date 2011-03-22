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

#include "Application.h"
#include "settings/GUISettings.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogSelect.h"
#include "threads/SingleLock.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRDatabase.h"
#include "utils/TimeUtils.h"
#include "guilib/GUIWindowManager.h"
#include "pvr/channels/PVRChannelGroupInternal.h"

using namespace std;
using namespace ADDON;

CPVRClients::CPVRClients(void)
{
  m_bChannelScanRunning = false;
  m_bAllClientsLoaded   = false;
  m_currentChannel      = NULL;
  m_currentRecording    = NULL;
  m_iInfoToggleStart    = 0;
  m_iInfoToggleCurrent  = 0;
  m_scanStart           = 0;
  m_clientsProps.clear();
  m_clientMap.clear();
  ResetQualityData();
}

CPVRClients::~CPVRClients(void)
{
  Unload();
}

void CPVRClients::Unload(void)
{
  CSingleLock lock(m_critSection);

  for (CLIENTMAPITR itr = m_clientMap.begin(); itr != m_clientMap.end(); itr++)
  {
    boost::shared_ptr<CPVRClient> client = m_clientMap[(*itr).first];
    CLog::Log(LOGDEBUG, "PVR - %s - destroying addon '%s' (%s)",
        __FUNCTION__, client->Name().c_str(), client->ID().c_str());

    client->Destroy();
  }

  m_bAllClientsLoaded = false;
  m_currentChannel    = NULL;
  m_currentRecording  = NULL;
  m_clientsProps.clear();
  m_clientMap.clear();
  ResetQualityData();
}

int CPVRClients::GetFirstID(void)
{
  int iReturn = -1;
  CSingleLock lock(m_critSection);

  if (m_clientMap.size() > 0)
  {
    CLIENTMAPITR itr = m_clientMap.begin();
    iReturn = m_clientMap[(*itr).first]->GetID();
  }

  return iReturn;
}

int CPVRClients::GetClients(map<long, CStdString> *clients)
{
  CLIENTMAPITR itr;
  int iInitialSize = clients->size();
  CSingleLock lock(m_critSection);

  for (itr = m_clientMap.begin() ; itr != m_clientMap.end(); itr++)
  {
    CStdString strClient = (*itr).second->GetBackendName() + ":" + (*itr).second->GetConnectionString();
    clients->insert(std::make_pair(m_clientMap[(*itr).first]->GetID(), strClient));
  }

  return clients->size() - iInitialSize;
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


bool CPVRClients::HasActiveClients(void)
{
  bool bReturn = false;
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

bool CPVRClients::HasTimerSupport(int iClientId)
{
  CSingleLock lock(m_critSection);

  return m_clientsProps[iClientId].SupportTimers;
}

bool CPVRClients::IsValidClient(int iClientId)
{
  bool bReturn = false;
  CSingleLock lock(m_critSection);

  CLIENTMAPITR itr = m_clientMap.find(iClientId);
  if (itr != m_clientMap.end() && itr->second->ReadyToUse())
    bReturn = true;
  else
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, iClientId);

  return bReturn;
}

bool CPVRClients::IsPlaying(void) const
{
  CSingleLock lock(m_critSection);
  return m_currentRecording != NULL || m_currentChannel != NULL;
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

bool CPVRClients::IsRunningChannelScan(void) const
{
  CSingleLock lock(m_critSection);
  return m_bChannelScanRunning;
}

bool CPVRClients::IsEncrypted(void) const
{
  CSingleLock lock(m_critSection);
  return m_currentChannel != NULL && m_currentChannel->IsEncrypted();
}

const char *CPVRClients::CharInfoEncryption(void) const
{
  static CStdString strReturn = "";
  CSingleLock lock(m_critSection);

  if (m_currentChannel)
    strReturn = m_currentChannel->EncryptionName();

  return strReturn;
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
    if (!CPVRManager::Get()->IsRunning())
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

bool CPVRClients::ClientLoaded(const CStdString &strClientId)
{
  bool bStarted = false;
  CSingleLock lock(m_critSection);

  for (unsigned int iClientPtr = 0; iClientPtr < m_clientMap.size(); iClientPtr++)
  {
    if (m_clientMap[iClientPtr]->ID() == strClientId && m_clientMap[iClientPtr]->ReadyToUse())
    {
      /* already started */
      bStarted = true;
      break;
    }
  }

  return bStarted;
}

int CPVRClients::AddClientToDb(const CStdString &strClientId, const CStdString &strName)
{
  /* add this client to the database if it's not in there yet */
  int iClientDbId = CPVRManager::Get()->GetTVDatabase()->AddClient(strName, strClientId);
  if (iClientDbId == -1)
  {
    CLog::Log(LOGERROR, "PVR - %s - can't add client '%s' to the database",
        __FUNCTION__, strName.c_str());
  }

  return iClientDbId;
}

bool CPVRClients::LoadClients(void)
{
  if (m_bAllClientsLoaded)
    return !m_clientMap.empty();

  CAddonMgr::Get().RegisterAddonMgrCallback(ADDON_PVRDLL, this);

  /* get all PVR addons */
  VECADDONS addons;
  if (!CAddonMgr::Get().GetAddons(ADDON_PVRDLL, addons, true))
    return false;

  /* load and initialise the clients */
  CPVRDatabase *database = CPVRManager::Get()->GetTVDatabase();
  if (!database || !database->Open())
  {
    CLog::Log(LOGERROR, "PVR - %s - cannot open the database", __FUNCTION__);
    return false;
  }

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
    if (ClientLoaded(clientAddon->ID()))
      continue;

    /* load and initialise the client libraries */
    boost::shared_ptr<CPVRClient> addon = boost::dynamic_pointer_cast<CPVRClient>(clientAddon);
    if (addon && addon->Create(iClientId, this))
    {
      /* get the client's properties */
      PVR_SERVERPROPS props;
      if (addon->GetProperties(&props) == PVR_ERROR_NO_ERROR)
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

bool CPVRClients::StopClient(AddonPtr client, bool bRestart)
{
  bool bReturn = false;
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

      CPVRManager::Get()->StopThreads();
      if (bRestart)
        m_clientMap[(*itr).first]->ReCreate();
      else
        m_clientMap[(*itr).first]->Destroy();
      CPVRManager::Get()->StartThreads();

      bReturn = true;
      break;
    }
    itr++;
  }

  return bReturn;
}

const CStdString CPVRClients::GetClientName(int iClientId)
{
  static CStdString strClientName;
  CSingleLock lock(m_critSection);

  if (IsValidClient(iClientId))
  {
    boost::shared_ptr<CPVRClient> client = m_clientMap.find(iClientId)->second;
    strClientName = client->GetBackendName() + ":" + client->GetConnectionString();
  }
  else
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, iClientId);

  return strClientName;
}

void CPVRClients::ResetQualityData(void)
{
  if (g_guiSettings.GetBool("pvrplayback.signalquality"))
  {
    strncpy(m_qualityInfo.frontend_name, g_localizeStrings.Get(13205).c_str(), 1024);
    strncpy(m_qualityInfo.frontend_status, g_localizeStrings.Get(13205).c_str(), 1024);
  }
  else
  {
    strncpy(m_qualityInfo.frontend_name, g_localizeStrings.Get(13106).c_str(), 1024);
    strncpy(m_qualityInfo.frontend_status, g_localizeStrings.Get(13106).c_str(), 1024);
  }
  m_qualityInfo.snr           = 0;
  m_qualityInfo.signal        = 0;
  m_qualityInfo.ber           = 0;
  m_qualityInfo.unc           = 0;
  m_qualityInfo.video_bitrate = 0;
  m_qualityInfo.audio_bitrate = 0;
  m_qualityInfo.dolby_bitrate = 0;
}

void CPVRClients::UpdateSignalQuality(void)
{
  CSingleLock lock(m_critSection);

  if (!m_currentChannel || !g_guiSettings.GetBool("pvrplayback.signalquality"))
    ResetQualityData();
  else if (!m_currentChannel->IsVirtual() && m_currentChannel->ClientID() >= 0 && m_clientMap[m_currentChannel->ClientID()])
    m_clientMap[m_currentChannel->ClientID()]->SignalQuality(m_qualityInfo);
  else
    ResetQualityData();
}

bool CPVRClients::IsReadingLiveStream(void) const
{
  CSingleLock lock(m_critSection);
  return m_currentChannel != NULL;
}

bool CPVRClients::OpenLiveStream(const CPVRChannel &tag)
{
  bool bReturn = false;
  CSingleLock lock(m_critSection);

  if (m_currentChannel)
    delete m_currentChannel;
  if (m_currentRecording)
    delete m_currentRecording;

  ResetQualityData();

  /* try to open the stream on the client */
  if (tag.StreamURL().IsEmpty() &&
      m_clientsProps[tag.ClientID()].HandleInputStream &&
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

int CPVRClients::ReadLiveStream(void* lpBuf, int64_t uiBufSize)
{
  CSingleLock lock(m_critSection);
  return m_currentChannel ? m_clientMap[m_currentChannel->ClientID()]->ReadLiveStream(lpBuf, uiBufSize) : 0;
}

bool CPVRClients::CloseLiveStream(void)
{
  bool bReturn = false;
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

bool CPVRClients::IsPlayingRecording(void) const
{
  CSingleLock lock(m_critSection);
  return m_currentRecording != NULL;
}

bool CPVRClients::OpenRecordedStream(const CPVRRecording &tag)
{
  bool bReturn = false;
  CSingleLock lock(m_critSection);

  if (m_currentChannel)
    delete m_currentChannel;
  if (m_currentRecording)
    delete m_currentRecording;

  /* try to open the recording stream on the client */
  if (m_clientMap[tag.m_clientID]->OpenRecordedStream(tag))
  {
    m_currentRecording = &tag;
    m_strPlayingClientName = GetClientName(tag.m_clientID);
    m_scanStart = CTimeUtils::GetTimeMS();  /* Reset the stream scan timer */
    bReturn = true;
  }

  return bReturn;
}

int CPVRClients::ReadRecordedStream(void* lpBuf, int64_t uiBufSize)
{
  CSingleLock lock(m_critSection);
  return m_currentRecording ? m_clientMap[m_currentRecording->m_clientID]->ReadRecordedStream(lpBuf, uiBufSize) : 0;
}

bool CPVRClients::CloseRecordedStream(void)
{
  bool bReturn = false;
  CSingleLock lock(m_critSection);

  if (!m_currentRecording)
    return bReturn;

  if (m_currentRecording->m_clientID > 0 && m_clientMap[m_currentRecording->m_clientID])
  {
    m_clientMap[m_currentRecording->m_clientID]->CloseRecordedStream();
    bReturn = true;
  }

  return bReturn;
}

void CPVRClients::CloseStream(void)
{
  CSingleLock lock(m_critSection);
  CloseLiveStream() || CloseRecordedStream();
  m_strPlayingClientName = "";
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
    return m_clientMap[m_currentChannel->ClientID()]->ReadRecordedStream(lpBuf, uiBufSize);

  return 0;
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

int64_t CPVRClients::LengthStream(void)
{
  int64_t streamLength = 0;

  if (m_currentChannel)
    streamLength = 0;
  else if (m_currentRecording)
    streamLength = m_clientMap[m_currentRecording->m_clientID]->LengthRecordedStream();

  return streamLength;
}

int64_t CPVRClients::SeekStream(int64_t iFilePosition, int iWhence/* = SEEK_SET*/)
{
  int64_t streamNewPos = 0;

  if (m_currentChannel)
    streamNewPos = 0;
  else if (m_currentRecording)
    streamNewPos = m_clientMap[m_currentRecording->m_clientID]->SeekRecordedStream(iFilePosition, iWhence);

  return streamNewPos;
}

int64_t CPVRClients::GetStreamPosition(void)
{
  int64_t streamPos = 0;

  if (m_currentChannel)
    streamPos = 0;
  else if (m_currentRecording)
    streamPos = m_clientMap[m_currentRecording->m_clientID]->PositionRecordedStream();

  return streamPos;
}

bool CPVRClients::AddTimer(const CPVRTimerInfoTag &timer, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKOWN;
  CSingleLock lock(m_critSection);

  if (IsValidClient(timer.m_iClientID))
  {
    boost::shared_ptr<CPVRClient> client = m_clientMap.find(timer.m_iClientID)->second;
    lock.Leave();
    *error = client->AddTimer(timer);
  }
  else
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, timer.m_iClientID);

  return *error == PVR_ERROR_NO_ERROR;
}

bool CPVRClients::UpdateTimer(const CPVRTimerInfoTag &timer, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKOWN;
  CSingleLock lock(m_critSection);

  if (IsValidClient(timer.m_iClientID))
  {
    boost::shared_ptr<CPVRClient> client = m_clientMap.find(timer.m_iClientID)->second;
    lock.Leave();
    *error = client->UpdateTimer(timer);
  }
  else
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, timer.m_iClientID);

  return *error == PVR_ERROR_NO_ERROR;
}

bool CPVRClients::DeleteTimer(const CPVRTimerInfoTag &timer, bool bForce, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKOWN;
  CSingleLock lock(m_critSection);

  if (IsValidClient(timer.m_iClientID))
  {
    boost::shared_ptr<CPVRClient> client = m_clientMap.find(timer.m_iClientID)->second;
    lock.Leave();
    *error = client->DeleteTimer(timer, bForce);
  }
  else
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, timer.m_iClientID);

  return *error == PVR_ERROR_NO_ERROR;
}

bool CPVRClients::RenameTimer(const CPVRTimerInfoTag &timer, const CStdString &strNewName, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKOWN;
  CSingleLock lock(m_critSection);

  if (IsValidClient(timer.m_iClientID))
  {
    boost::shared_ptr<CPVRClient> client = m_clientMap.find(timer.m_iClientID)->second;
    lock.Leave();
    *error = client->RenameTimer(timer, strNewName);
  }
  else
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, timer.m_iClientID);

  return *error == PVR_ERROR_NO_ERROR;
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
    if (!GetClientProperties((*itrClients).second->GetID())->SupportTimers ||
        (*itrClients).second->GetNumTimers() <= 0)
    {
      ++itrClients;
      continue;
    }

    (*itrClients).second->GetAllTimers(timers);
    ++itrClients;
  }

  return timers->size() - iCurSize;
}

int CPVRClients::GetRecordings(CPVRRecordings *recordings)
{
  int iCurSize = recordings->size();
  CLIENTMAP clients;
  GetActiveClients(&clients);

  CLIENTMAPITR itr = clients.begin();
  while (itr != clients.end())
  {
    /* Load only if the client have Recordings */
    if ((*itr).second->GetNumRecordings() > 0)
    {
      (*itr).second->GetAllRecordings(recordings);
    }
    itr++;
  }

  return recordings->size() - iCurSize;
}

bool CPVRClients::RenameRecording(const CPVRRecording &recording, const CStdString &strNewName, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKOWN;
  CSingleLock lock(m_critSection);

  if (IsValidClient(recording.m_clientID))
  {
    boost::shared_ptr<CPVRClient> client = m_clientMap.find(recording.m_clientID)->second;
    lock.Leave();
    *error = client->RenameRecording(recording, strNewName);
  }
  else
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, recording.m_clientID);

  return *error == PVR_ERROR_NO_ERROR;
}

bool CPVRClients::DeleteRecording(const CPVRRecording &recording, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKOWN;
  CSingleLock lock(m_critSection);

  if (IsValidClient(recording.m_clientID))
  {
    boost::shared_ptr<CPVRClient> client = m_clientMap.find(recording.m_clientID)->second;
    lock.Leave();
    *error = client->DeleteRecording(recording);
  }
  else
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, recording.m_clientID);

  return *error == PVR_ERROR_NO_ERROR;
}

bool CPVRClients::GetEPGForChannel(const CPVRChannel &channel, CPVREpg *epg, time_t start, time_t end, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKOWN;
  CSingleLock lock(m_critSection);

  if (IsValidClient(channel.ClientID()))
  {
    boost::shared_ptr<CPVRClient> client = m_clientMap.find(channel.ClientID())->second;
    lock.Leave();
    *error = client->GetEPGForChannel(channel, epg, start, end);
  }
  else
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, channel.ClientID());

  return *error == PVR_ERROR_NO_ERROR;
}

int CPVRClients::GetChannels(CPVRChannelGroupInternal *group, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKOWN;
  int iCurSize = group->Size();
  CLIENTMAP clients;
  GetActiveClients(&clients);

  /* get the channel list from each client */
  CLIENTMAPITR itrClients = clients.begin();
  while (itrClients != clients.end())
  {
    if ((*itrClients).second->ReadyToUse() && (*itrClients).second->GetNumChannels() > 0)
      (*itrClients).second->GetChannelList(*group, group->IsRadio());

    itrClients++;
  }

  return group->Size() - iCurSize;
}

const CStdString CPVRClients::GetStreamURL(const CPVRChannel &tag)
{
  static CStdString strReturn;
  CSingleLock lock(m_critSection);

  if (IsValidClient(tag.ClientID()))
  {
    boost::shared_ptr<CPVRClient> client = m_clientMap.find(tag.ClientID())->second;
    lock.Leave();
    strReturn = client->GetLiveStreamURL(tag);
  }
  else
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, tag.ClientID());

  return strReturn;
}

const char *CPVRClients::CharInfoBackendNumber(void)
{
  CSingleLock lock(m_critSection);

  if (m_iInfoToggleStart == 0)
  {
    m_iInfoToggleStart = CTimeUtils::GetTimeMS();
    m_iInfoToggleCurrent = 0;
  }
  else
  {
    if (CTimeUtils::GetTimeMS() - m_iInfoToggleStart > INFO_TOGGLE_TIME)
    {
      if (m_clientMap.size() > 0)
      {
        m_iInfoToggleCurrent++;
        if (m_iInfoToggleCurrent > m_clientMap.size()-1)
          m_iInfoToggleCurrent = 0;

        CLIENTMAPITR itr = m_clientMap.begin();
        for (unsigned int i = 0; i < m_iInfoToggleCurrent; i++)
          itr++;

        long long kBTotal = 0;
        long long kBUsed  = 0;
        if (m_clientMap[(*itr).first]->GetDriveSpace(&kBTotal, &kBUsed) == PVR_ERROR_NO_ERROR)
        {
          kBTotal /= 1024; // Convert to MBytes
          kBUsed /= 1024;  // Convert to MBytes
          m_strBackendDiskspace.Format("%s %.1f GByte - %s: %.1f GByte", g_localizeStrings.Get(20161), (float) kBTotal / 1024, g_localizeStrings.Get(20162), (float) kBUsed / 1024);
        }
        else
        {
          m_strBackendDiskspace = g_localizeStrings.Get(19055);
        }

        int NumChannels = m_clientMap[(*itr).first]->GetNumChannels();
        if (NumChannels >= 0)
          m_strBackendChannels.Format("%i", NumChannels);
        else
          m_strBackendChannels = g_localizeStrings.Get(161);

        int NumTimers = m_clientMap[(*itr).first]->GetNumTimers();
        if (NumTimers >= 0)
          m_strBackendTimers.Format("%i", NumTimers);
        else
          m_strBackendTimers = g_localizeStrings.Get(161);

        int NumRecordings = m_clientMap[(*itr).first]->GetNumRecordings();
        if (NumRecordings >= 0)
          m_strBackendRecordings.Format("%i", NumRecordings);
        else
          m_strBackendRecordings = g_localizeStrings.Get(161);

        m_strBackendName         = m_clientMap[(*itr).first]->GetBackendName();
        m_strBackendVersion      = m_clientMap[(*itr).first]->GetBackendVersion();
        m_strBackendHost         = m_clientMap[(*itr).first]->GetConnectionString();
      }
      else
      {
        m_strBackendName         = "";
        m_strBackendVersion      = "";
        m_strBackendHost         = "";
        m_strBackendDiskspace    = "";
        m_strBackendTimers       = "";
        m_strBackendRecordings   = "";
        m_strBackendChannels     = "";
      }
      m_iInfoToggleStart = CTimeUtils::GetTimeMS();
    }
  }

  static CStdString backendClients;
  if (m_clientMap.size() > 0)
    backendClients.Format("%u %s %u", m_iInfoToggleCurrent+1, g_localizeStrings.Get(20163), m_clientMap.size());
  else
    backendClients = g_localizeStrings.Get(14023);

  return backendClients;
}

const char *CPVRClients::CharInfoTotalDiskSpace(void)
{
  long long kBTotal = 0;
  long long kBUsed  = 0;
  CSingleLock lock(m_critSection);

  CLIENTMAPITR itr = m_clientMap.begin();
  while (itr != m_clientMap.end())
  {
    long long clientKBTotal = 0;
    long long clientKBUsed  = 0;

    if (m_clientMap[(*itr).first]->GetDriveSpace(&clientKBTotal, &clientKBUsed) == PVR_ERROR_NO_ERROR)
    {
      kBTotal += clientKBTotal;
      kBUsed += clientKBUsed;
    }
    itr++;
  }
  kBTotal /= 1024; // Convert to MBytes
  kBUsed /= 1024;  // Convert to MBytes
  m_strTotalDiskspace.Format("%s %0.1f GByte - %s: %0.1f GByte", g_localizeStrings.Get(20161), (float) kBTotal / 1024, g_localizeStrings.Get(20162), (float) kBUsed / 1024);

  return m_strTotalDiskspace;
}


void CPVRClients::StartChannelScan(void)
{
  std::vector<long> clients;
  int scanningClientID = -1;
  CSingleLock lock(m_critSection);
  m_bChannelScanRunning = true;

  /* get clients that support channel scanning */
  CLIENTMAPITR itr = m_clientMap.begin();
  while (itr != m_clientMap.end())
  {
    if (m_clientMap[(*itr).first]->ReadyToUse() && GetClientProperties(m_clientMap[(*itr).first]->GetID())->SupportChannelScan)
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
      pDialog->Add(m_clientMap[clients[i]]->GetBackendName() + ":" + m_clientMap[clients[i]]->GetConnectionString());

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
  CLog::Log(LOGNOTICE,"PVR - %s - starting to scan for channels on client %s:%s",
      __FUNCTION__, m_clientMap[scanningClientID]->GetBackendName().c_str(), m_clientMap[scanningClientID]->GetConnectionString().c_str());
  long perfCnt = CTimeUtils::GetTimeMS();

  /* stop the supervisor thread */
  CPVRManager::Get()->StopThreads();

  /* do the scan */
  if (m_clientMap[scanningClientID]->StartChannelScan() != PVR_ERROR_NO_ERROR)
    /* an error occured */
    CGUIDialogOK::ShowAndGetInput(19111,0,19193,0);

  /* restart the supervisor thread */
  CPVRManager::Get()->StartThreads();

  CLog::Log(LOGNOTICE, "PVRManager - %s - channel scan finished after %li.%li seconds",
      __FUNCTION__, (CTimeUtils::GetTimeMS()-perfCnt)/1000, (CTimeUtils::GetTimeMS()-perfCnt)%1000);
  m_bChannelScanRunning = false;
}

PVR_SERVERPROPS *CPVRClients::GetCurrentClientProperties(void)
{
  PVR_SERVERPROPS * props = NULL;
  CSingleLock lock(m_critSection);

  if (m_currentChannel)
    props = &m_clientsProps[m_currentChannel->ClientID()];
  else if (m_currentRecording)
    props = &m_clientsProps[m_currentRecording->m_clientID];

  return props;
}

PVR_STREAMPROPS *CPVRClients::GetCurrentStreamProperties(void)
{
  PVR_STREAMPROPS *props = NULL;
  CSingleLock lock(m_critSection);

  if (m_currentChannel)
  {
    int iChannelId = m_currentChannel->ClientID();
    m_clientMap[iChannelId]->GetStreamProperties(&m_streamProps[iChannelId]);

    props = &m_streamProps[iChannelId];
  }

  return props;
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

bool CPVRClients::GetPlayingRecording(CPVRRecording *recording) const
{
  CSingleLock lock(m_critSection);
  if (m_currentRecording != NULL)
    *recording = *m_currentRecording;

  return m_currentRecording != NULL;
}

int CPVRClients::GetPlayingClientID(void) const
{
  int iReturn = -1;
  CSingleLock lock(m_critSection);

  if (m_currentChannel)
    iReturn = m_currentChannel->ClientID();
  else if (m_currentRecording)
    iReturn = m_currentRecording->m_clientID;

  return iReturn;
}

bool CPVRClients::HasMenuHooks(int iClientID)
{
  bool bReturn = false;
  CSingleLock lock(m_critSection);

  if (iClientID < 0)
    iClientID = GetPlayingClientID();

  if (IsValidClient(iClientID))
  {
    boost::shared_ptr<CPVRClient> client = m_clientMap.find(iClientID)->second;
    lock.Leave();
    bReturn = client->HaveMenuHooks();
  }
  else
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, iClientID);

  return bReturn;
}

bool CPVRClients::GetMenuHooks(int iClientID, PVR_MENUHOOKS *hooks)
{
  bool bReturn = false;
  CSingleLock lock(m_critSection);

  if (iClientID < 0)
    iClientID = GetPlayingClientID();

  if (IsValidClient(iClientID))
  {
    boost::shared_ptr<CPVRClient> client = m_clientMap.find(iClientID)->second;
    lock.Leave();
    if (client->HaveMenuHooks())
    {
      hooks = client->GetMenuHooks();
      bReturn = true;
    }
  }
  else
  {
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, iClientID);
  }

  return bReturn;
}

void CPVRClients::ProcessMenuHooks(int iClientID)
{
  PVR_MENUHOOKS *hooks = NULL;
  CSingleLock lock(m_critSection);

  if (iClientID < 0)
    iClientID = GetPlayingClientID();

  if (GetMenuHooks(iClientID, hooks))
  {
    boost::shared_ptr<CPVRClient> client = m_clientMap.find(iClientID)->second;
    lock.Leave();
    std::vector<long> hookIDs;

    CGUIDialogSelect* pDialog = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
    pDialog->Reset();
    pDialog->SetHeading(19196);
    for (unsigned int i = 0; i < hooks->size(); i++)
      pDialog->Add(client->GetString(hooks->at(i).string_id));
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

bool CPVRClients::CanRecordInstantly(void)
{
  return m_currentChannel != NULL &&
      m_clientsProps[m_currentChannel->ClientID()].SupportTimers;
}


bool CPVRClients::IsRecordingOnPlayingChannel(void) const
{
  return m_currentChannel && m_currentChannel->IsRecording();
}

bool CPVRClients::SwitchChannel(const CPVRChannel &channel)
{
  bool bReturn = false;
  CSingleLock lock(m_critSection);
  if (m_currentChannel && m_currentChannel->ClientID() != channel.ClientID())
  {
    lock.Leave();
    CloseStream();
    return OpenLiveStream(channel);
  }

  if (IsValidClient(channel.ClientID()))
  {
    boost::shared_ptr<CPVRClient> client = m_clientMap.find(channel.ClientID())->second;
    lock.Leave();
    if (client->SwitchChannel(channel))
    {
      m_currentChannel = &channel;
      m_scanStart = CTimeUtils::GetTimeMS();
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

CStdString CPVRClients::GetCurrentInputFormat(void) const
{
  static CStdString strReturn = "";
  CSingleLock lock(m_critSection);

  if (m_currentChannel)
    strReturn = m_currentChannel->InputFormat();

  return strReturn;
}

void CPVRClients::OnClientMessage(const int iClientId, const PVR_EVENT clientEvent, const char *strMessage)
{
  /* here the manager reacts to messages sent from any of the clients via the IPVRClientCallback */
  switch (clientEvent)
  {
    case PVR_EVENT_TIMERS_CHANGE:
    {
      CLog::Log(LOGDEBUG, "PVR - %s - timers changed on client '%d'",
          __FUNCTION__, iClientId);
      CPVRManager::Get()->TriggerTimersUpdate();
    }
    break;

    case PVR_EVENT_RECORDINGS_CHANGE:
    {
      CLog::Log(LOGDEBUG, "PVR - %s - recording list changed on client '%d'",
          __FUNCTION__, iClientId);
      CPVRManager::Get()->TriggerRecordingsUpdate();
    }
    break;

    case PVR_EVENT_CHANNELS_CHANGE:
    {
      CLog::Log(LOGDEBUG, "PVR - %s - channel list changed on client '%d'",
          __FUNCTION__, iClientId);
      CPVRManager::Get()->TriggerChannelsUpdate();
    }
    break;

    default:
      CLog::Log(LOGWARNING, "PVR - %s - client '%d' sent unknown event '%s'",
          __FUNCTION__, iClientId, strMessage);
      break;
  }
}

bool CPVRClients::RequestRestart(AddonPtr addon, bool bDataChanged)
{
  return StopClient(addon, true);
}

bool CPVRClients::RequestRemoval(AddonPtr addon)
{
  return StopClient(addon, false);
}

int CPVRClients::GetSignalLevel(void) const
{
  CSingleLock lock(m_critSection);
  return (int) ((float) m_qualityInfo.signal / 0xFFFF * 100);
}

int CPVRClients::GetSNR(void) const
{
  CSingleLock lock(m_critSection);
  return (int) ((float) m_qualityInfo.snr / 0xFFFF * 100);
}

const char *CPVRClients::CharInfoVideoBR(void) const
{
  static CStdString strReturn = "";
  if (m_qualityInfo.video_bitrate > 0)
    strReturn.Format("%.2f Mbit/s", m_qualityInfo.video_bitrate);

  return strReturn;
}

const char *CPVRClients::CharInfoAudioBR(void) const
{
  static CStdString strReturn = "";
  if (m_qualityInfo.audio_bitrate > 0)
    strReturn.Format("%.0f kbit/s", m_qualityInfo.audio_bitrate);

  return strReturn;
}

const char *CPVRClients::CharInfoDolbyBR(void) const
{
  static CStdString strReturn = "";
  if (m_qualityInfo.dolby_bitrate > 0)
    strReturn.Format("%.0f kbit/s", m_qualityInfo.dolby_bitrate);

  return strReturn;
}

const char *CPVRClients::CharInfoSignal(void) const
{
  static CStdString strReturn = "";
  if (m_qualityInfo.signal > 0)
    strReturn.Format("%d %%", m_qualityInfo.signal / 655);

  return strReturn;
}

const char *CPVRClients::CharInfoSNR(void) const
{
  static CStdString strReturn = "";
  if (m_qualityInfo.snr > 0)
    strReturn.Format("%d %%", m_qualityInfo.snr / 655);

  return strReturn;
}

const char *CPVRClients::CharInfoBER(void) const
{
  static CStdString strReturn = "";
  strReturn.Format("%08X", m_qualityInfo.ber);

  return strReturn;
}

const char *CPVRClients::CharInfoUNC(void) const
{
  static CStdString strReturn = "";
  strReturn.Format("%08X", m_qualityInfo.unc);

  return strReturn;
}

const char *CPVRClients::CharInfoFrontendName(void) const
{
  static CStdString strReturn = m_qualityInfo.frontend_name;
  if (strReturn == "")
    strReturn = g_localizeStrings.Get(13205);

  return strReturn;
}

const char *CPVRClients::CharInfoFrontendStatus(void) const
{
  static CStdString strReturn = m_qualityInfo.frontend_status;
  if (strReturn == "")
    strReturn = g_localizeStrings.Get(13205);

  return strReturn;
}

const char *CPVRClients::CharInfoBackendName(void) const
{
  static CStdString strReturn = m_strBackendName;
  if (strReturn == "")
    strReturn = g_localizeStrings.Get(13205);

  return strReturn;
}

const char *CPVRClients::CharInfoBackendVersion(void) const
{
  static CStdString strReturn = m_strBackendVersion;
  if (strReturn == "")
    strReturn = g_localizeStrings.Get(13205);

  return strReturn;
}

const char *CPVRClients::CharInfoBackendHost(void) const
{
  static CStdString strReturn = m_strBackendHost;
  if (strReturn == "")
    strReturn = g_localizeStrings.Get(13205);

  return strReturn;
}

const char *CPVRClients::CharInfoBackendDiskspace(void) const
{
  static CStdString strReturn = m_strBackendDiskspace;
  if (strReturn == "")
    strReturn = g_localizeStrings.Get(13205);

  return strReturn;
}

const char *CPVRClients::CharInfoBackendChannels(void) const
{
  static CStdString strReturn = m_strBackendChannels;
  if (strReturn == "")
    strReturn = g_localizeStrings.Get(13205);

  return strReturn;
}

const char *CPVRClients::CharInfoBackendTimers(void) const
{
  static CStdString strReturn = m_strBackendTimers;
  if (strReturn == "")
    strReturn = g_localizeStrings.Get(13205);

  return strReturn;
}

const char *CPVRClients::CharInfoBackendRecordings(void) const
{
  static CStdString strReturn = m_strBackendRecordings;
  if (strReturn == "")
    strReturn = g_localizeStrings.Get(13205);

  return strReturn;
}

const char *CPVRClients::CharInfoPlayingClientName(void) const
{
  static CStdString strReturn = m_strPlayingClientName;
  if (strReturn == "")
    strReturn = g_localizeStrings.Get(13205);

  return strReturn;
}

int CPVRClients::GetActiveClients(CLIENTMAP *clients)
{
  int iReturn = 0;
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
