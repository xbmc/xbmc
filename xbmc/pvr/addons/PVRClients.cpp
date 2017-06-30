/*
 *      Copyright (C) 2012-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PVRClients.h"

#include <cassert>
#include <utility>
#include <functional>

#include "Application.h"
#include "ServiceBroker.h"
#include "cores/IPlayer.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/ApplicationMessenger.h"
#include "pvr/channels/PVRChannelGroupInternal.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/PVRJobs.h"
#include "pvr/PVRManager.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimers.h"
#include "utils/log.h"

using namespace ADDON;
using namespace PVR;
using namespace KODI::MESSAGING;

/** number of iterations when scanning for add-ons. don't use a timer because the user may block in the dialog */
#define PVR_CLIENT_AVAHI_SCAN_ITERATIONS   (20)
/** sleep time in milliseconds when no auto-configured add-ons were found */
#define PVR_CLIENT_AVAHI_SLEEP_TIME_MS     (250)

CPVRClients::CPVRClients(void) :
    m_bIsSwitchingChannels(false),
    m_playingClientId(-EINVAL),
    m_bIsPlayingLiveTV(false),
    m_bIsPlayingRecording(false)
{
}

CPVRClients::~CPVRClients(void)
{
  CAddonMgr::GetInstance().UnregisterAddonMgrCallback(ADDON_PVRDLL);
  Unload();
}

void CPVRClients::Start(void)
{
  CAddonMgr::GetInstance().RegisterAddonMgrCallback(ADDON_PVRDLL, this);

  UpdateAddons();
}

bool CPVRClients::IsCreatedClient(int iClientId) const
{
  PVR_CLIENT client;
  return GetCreatedClient(iClientId, client);
}

bool CPVRClients::IsCreatedClient(const AddonPtr &addon)
{
  CSingleLock lock(m_critSection);

  for (const auto &client : m_clientMap)
    if (client.second->ID() == addon->ID())
      return client.second->ReadyToUse();
  return false;
}

int CPVRClients::GetClientId(const AddonPtr &client) const
{
  CSingleLock lock(m_critSection);

  for (auto &entry : m_clientMap)
  {
    if (entry.second->ID() == client->ID())
    {
      return entry.first;
    }
  }

  return -1;
}

int CPVRClients::GetClientId(const std::string& strId) const
{
  CSingleLock lock(m_critSection);
  std::map<std::string, int>::const_iterator it = m_addonNameIds.find(strId);
  return it != m_addonNameIds.end() ? it->second : -1;
}

bool CPVRClients::GetClient(int iClientId, PVR_CLIENT &addon) const
{
  bool bReturn(false);
  if (iClientId <= PVR_INVALID_CLIENT_ID)
    return bReturn;

  CSingleLock lock(m_critSection);

  PVR_CLIENTMAP_CITR itr = m_clientMap.find(iClientId);
  if (itr != m_clientMap.end())
  {
    addon = itr->second;
    bReturn = true;
  }

  return bReturn;
}

bool CPVRClients::GetCreatedClient(int iClientId, PVR_CLIENT &addon) const
{
  if (GetClient(iClientId, addon))
    return addon->ReadyToUse();
  return false;
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
  CSingleLock lock(m_critSection);

  /* reset class properties */
  m_bIsPlayingLiveTV     = false;
  m_bIsPlayingRecording  = false;
  m_strPlayingClientName = "";

  for (const auto &client : m_clientMap)
  {
    client.second->Destroy();
  }
  m_clientMap.clear();
}

int CPVRClients::GetFirstConnectedClientID(void)
{
  CSingleLock lock(m_critSection);

  for (const auto &client : m_clientMap)
    if (client.second->ReadyToUse())
      return client.second->GetID();

  return -1;
}

int CPVRClients::EnabledClientAmount(void) const
{
  int iReturn(0);
  PVR_CLIENTMAP clientMap;
  {
    CSingleLock lock(m_critSection);
    clientMap = m_clientMap;
  }

  for (const auto &client : clientMap)
    if (!CAddonMgr::GetInstance().IsAddonDisabled(client.second->ID()))
      ++iReturn;

  return iReturn;
}

bool CPVRClients::HasEnabledClients(void) const
{
  PVR_CLIENTMAP clientMap;
  {
    CSingleLock lock(m_critSection);
    clientMap = m_clientMap;
  }

  for (const auto &client : clientMap)
    if (!CAddonMgr::GetInstance().IsAddonDisabled(client.second->ID()))
      return true;
  return false;
}

bool CPVRClients::StopClient(const AddonPtr &client, bool bRestart)
{
  /* stop playback if needed */
  if (IsPlaying())
    CApplicationMessenger::GetInstance().SendMsg(TMSG_MEDIA_STOP);

  CSingleLock lock(m_critSection);
  int iId = GetClientId(client);
  PVR_CLIENT mappedClient;
  if (GetClient(iId, mappedClient))
  {
    if (bRestart)
      mappedClient->ReCreate();
    else
    {
      const auto it = m_clientMap.find(iId);
      if (it != m_clientMap.end())
        m_clientMap.erase(it);

      mappedClient->Destroy();
    }
    return true;
  }

  return false;
}

int CPVRClients::CreatedClientAmount(void) const
{
  int iReturn(0);
  CSingleLock lock(m_critSection);

  for (const auto &client : m_clientMap)
    if (client.second->ReadyToUse())
      ++iReturn;

  return iReturn;
}

bool CPVRClients::HasCreatedClients(void) const
{
  CSingleLock lock(m_critSection);

  for (auto &client : m_clientMap)
  {
    if (client.second->ReadyToUse() && !client.second->IgnoreClient())
    {
      return true;
    }
  }

  return false;
}

bool CPVRClients::GetClientFriendlyName(int iClientId, std::string &strName) const
{
  bool bReturn(false);
  PVR_CLIENT client;
  if ((bReturn = GetCreatedClient(iClientId, client)) == true)
    strName = client->GetFriendlyName();

  return bReturn;
}

bool CPVRClients::GetClientAddonName(int iClientId, std::string &strName) const
{
  bool bReturn(false);
  PVR_CLIENT client;
  if ((bReturn = GetCreatedClient(iClientId, client)) == true)
    strName = client->Name();

  return bReturn;
}

bool CPVRClients::GetClientAddonIcon(int iClientId, std::string &strIcon) const
{
  bool bReturn(false);
  PVR_CLIENT client;
  if ((bReturn = GetCreatedClient(iClientId, client)) == true)
    strIcon = client->Icon();

  return bReturn;
}

std::vector<SBackend> CPVRClients::GetBackendProperties() const
{
  std::vector<SBackend> backendProperties;
  std::vector<PVR_CLIENT> clients;

  {
    CSingleLock lock(m_critSection);

    for (const auto &client : m_clientMap)
    {
      if (client.second)
        clients.push_back(client.second);
    }
  }

  for (const auto &client : clients)
  {
    if (!client->ReadyToUse())
      continue;

    SBackend properties;

    if (client->GetDriveSpace(&properties.diskTotal, &properties.diskUsed) == PVR_ERROR_NO_ERROR)
    {
      properties.diskTotal *= 1024;  
      properties.diskUsed *= 1024;
    }

    properties.numChannels = client->GetChannelsAmount();
    properties.numTimers = client->GetTimersAmount();
    properties.numRecordings = client->GetRecordingsAmount(false);
    properties.numDeletedRecordings = client->GetRecordingsAmount(true);
    properties.name = client->GetBackendName();
    properties.version = client->GetBackendVersion();
    properties.host = client->GetConnectionString();

    backendProperties.push_back(properties);
  }

  return backendProperties;
}

std::string CPVRClients::GetClientAddonId(int iClientId) const
{
  PVR_CLIENT client;
  return GetClient(iClientId, client) ?
      client->ID() :
      "";
}

int CPVRClients::GetCreatedClients(PVR_CLIENTMAP &clients) const
{
  int iReturn(0);
  CSingleLock lock(m_critSection);

  for (const auto &client : m_clientMap)
  {
    if (client.second->ReadyToUse())
    {
      if (client.second->IgnoreClient())
        continue;
      
      clients.insert(std::make_pair(client.second->GetID(), client.second));
      ++iReturn;
    }
  }

  return iReturn;
}

int CPVRClients::GetPlayingClientID(void) const
{
  CSingleLock lock(m_critSection);

  if (m_bIsPlayingLiveTV || m_bIsPlayingRecording)
    return m_playingClientId;
  return -EINVAL;
}

const std::string CPVRClients::GetPlayingClientName(void) const
{
  CSingleLock lock(m_critSection);
  return m_strPlayingClientName;
}

std::string CPVRClients::GetStreamURL(const CPVRChannelPtr &channel)
{
  assert(channel.get());

  std::string strReturn;
  PVR_CLIENT client;
  if (GetCreatedClient(channel->ClientID(), client))
    strReturn = client->GetLiveStreamURL(channel);
  else
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, channel->ClientID());

  return strReturn;
}

bool CPVRClients::SwitchChannel(const CPVRChannelPtr &channel)
{
  assert(channel.get());

  {
    CSingleLock lock(m_critSection);
    if (m_bIsSwitchingChannels)
    {
      CLog::Log(LOGDEBUG, "PVRClients - %s - can't switch to channel '%s'. waiting for the previous switch to complete", __FUNCTION__, channel->ChannelName().c_str());
      return false;
    }
    m_bIsSwitchingChannels = true;
  }

  bool bSwitchSuccessful(false);
  CPVRChannelPtr currentChannel(GetPlayingChannel());
  if (// no channel is currently playing
      !currentChannel ||
      // different backend
      currentChannel->ClientID() != channel->ClientID() ||
      // stream URL should always be opened as a new file
      !channel->StreamURL().empty() || !currentChannel->StreamURL().empty())
  {
    if (channel->StreamURL().empty())
    {
      CloseStream();
      bSwitchSuccessful = OpenStream(channel, true);
    }
    else
    {
      CApplicationMessenger::GetInstance().PostMsg(TMSG_MEDIA_PLAY, 0, 0, static_cast<void*>(new CFileItem(channel)));
      bSwitchSuccessful = true;
    }
  }
  // same channel
  else if (currentChannel && currentChannel == channel)
  {
    bSwitchSuccessful = true;
  }
  else
  {
    PVR_CLIENT client;
    if (GetCreatedClient(channel->ClientID(), client))
      bSwitchSuccessful = client->SwitchChannel(channel);
  }

  {
    CSingleLock lock(m_critSection);
    m_bIsSwitchingChannels = false;
  }

  if (!bSwitchSuccessful)
    CLog::Log(LOGERROR, "PVR - %s - cannot switch to channel '%s' on client '%d'",__FUNCTION__, channel->ChannelName().c_str(), channel->ClientID());

  return bSwitchSuccessful;
}

CPVRChannelPtr CPVRClients::GetPlayingChannel() const
{
  PVR_CLIENT client;
  if (GetPlayingClient(client))
    return client->GetPlayingChannel();

  return CPVRChannelPtr();
}

CPVRRecordingPtr CPVRClients::GetPlayingRecording(void) const
{
  PVR_CLIENT client;
  return GetPlayingClient(client) ? client->GetPlayingRecording() : CPVRRecordingPtr();
}

bool CPVRClients::HasTimerSupport(int iClientId)
{
  PVR_CLIENT client;
  if (GetCreatedClient(iClientId, client))
    return client->SupportsTimers();

  return false;
}

bool CPVRClients::GetTimers(CPVRTimersContainer *timers, std::vector<int> &failedClients)
{
  bool bSuccess(true);
  PVR_CLIENTMAP clients;
  GetCreatedClients(clients);

  /* get the timer list from each client */
  for (const auto &client : clients)
  {
    PVR_ERROR currentError = client.second->GetTimers(timers);
    if (currentError != PVR_ERROR_NOT_IMPLEMENTED &&
        currentError != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVR - %s - cannot get timers from client '%d': %s",__FUNCTION__, client.first, CPVRClient::ToString(currentError));
      bSuccess = false;
      failedClients.push_back(client.first);
    }
  }

  return bSuccess;
}

PVR_ERROR CPVRClients::AddTimer(const CPVRTimerInfoTag &timer)
{
  PVR_ERROR error(PVR_ERROR_UNKNOWN);

  PVR_CLIENT client;
  if (GetCreatedClient(timer.m_iClientId, client))
    error = client->AddTimer(timer);

  if (error != PVR_ERROR_NO_ERROR)
    CLog::Log(LOGERROR, "PVR - %s - cannot add timer to client '%d': %s",__FUNCTION__, timer.m_iClientId, CPVRClient::ToString(error));

  return error;
}

PVR_ERROR CPVRClients::UpdateTimer(const CPVRTimerInfoTag &timer)
{
  PVR_ERROR error(PVR_ERROR_UNKNOWN);

  PVR_CLIENT client;
  if (GetCreatedClient(timer.m_iClientId, client))
    error = client->UpdateTimer(timer);

  if (error != PVR_ERROR_NO_ERROR)
    CLog::Log(LOGERROR, "PVR - %s - cannot update timer on client '%d': %s",__FUNCTION__, timer.m_iClientId, CPVRClient::ToString(error));

  return error;
}

PVR_ERROR CPVRClients::DeleteTimer(const CPVRTimerInfoTag &timer, bool bForce)
{
  PVR_ERROR error(PVR_ERROR_UNKNOWN);
  PVR_CLIENT client;

  if (GetCreatedClient(timer.m_iClientId, client))
    error = client->DeleteTimer(timer, bForce);

  return error;
}

PVR_ERROR CPVRClients::RenameTimer(const CPVRTimerInfoTag &timer, const std::string &strNewName)
{
  PVR_ERROR error(PVR_ERROR_UNKNOWN);

  PVR_CLIENT client;
  if (GetCreatedClient(timer.m_iClientId, client))
    error = client->RenameTimer(timer, strNewName);

  if (error != PVR_ERROR_NO_ERROR)
    CLog::Log(LOGERROR, "PVR - %s - cannot rename timer on client '%d': %s",__FUNCTION__, timer.m_iClientId, CPVRClient::ToString(error));

  return error;
}

PVR_ERROR CPVRClients::GetTimerTypes(CPVRTimerTypes& results) const
{
  PVR_ERROR error(PVR_ERROR_NO_ERROR);

  PVR_CLIENTMAP clients;
  GetCreatedClients(clients);

  for (const auto &clientEntry : clients)
  {
    CPVRTimerTypes types;
    PVR_ERROR currentError = clientEntry.second->GetTimerTypes(types);
    if (currentError != PVR_ERROR_NOT_IMPLEMENTED &&
        currentError != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVR - %s - cannot get timer types from client '%d': %s",__FUNCTION__, clientEntry.first, CPVRClient::ToString(currentError));
      error = currentError;
    }
    else
    {
      for (const auto &typesEntry : types)
        results.push_back(typesEntry);
    }
  }

  return error;
}

PVR_ERROR CPVRClients::GetTimerTypes(CPVRTimerTypes& results, int iClientId) const
{
  PVR_ERROR error(PVR_ERROR_UNKNOWN);

  PVR_CLIENT client;
  if (GetCreatedClient(iClientId, client))
    error = client->GetTimerTypes(results);

  if (error != PVR_ERROR_NO_ERROR)
    CLog::Log(LOGERROR, "PVR - %s - cannot get timer types from client '%d': %s",__FUNCTION__, iClientId, CPVRClient::ToString(error));

  return error;
}

PVR_ERROR CPVRClients::GetRecordings(CPVRRecordings *recordings, bool deleted)
{
  PVR_ERROR error(PVR_ERROR_NO_ERROR);
  PVR_CLIENTMAP clients;
  GetCreatedClients(clients);

  for (const auto &client : clients)
  {
    PVR_ERROR currentError = client.second->GetRecordings(recordings, deleted);
    if (currentError != PVR_ERROR_NOT_IMPLEMENTED &&
        currentError != PVR_ERROR_NO_ERROR)
    {
      CLog::Log(LOGERROR, "PVR - %s - cannot get recordings from client '%d': %s",__FUNCTION__, client.first, CPVRClient::ToString(currentError));
      error = currentError;
    }
  }

  return error;
}

PVR_ERROR CPVRClients::RenameRecording(const CPVRRecording &recording)
{
  PVR_ERROR error(PVR_ERROR_UNKNOWN);

  PVR_CLIENT client;
  if (GetCreatedClient(recording.m_iClientId, client))
    error = client->RenameRecording(recording);

  if (error != PVR_ERROR_NO_ERROR)
    CLog::Log(LOGERROR, "PVR - %s - cannot rename recording on client '%d': %s",__FUNCTION__, recording.m_iClientId, CPVRClient::ToString(error));

  return error;
}

PVR_ERROR CPVRClients::DeleteRecording(const CPVRRecording &recording)
{
  PVR_ERROR error(PVR_ERROR_UNKNOWN);

  PVR_CLIENT client;
  if (GetCreatedClient(recording.m_iClientId, client))
    error = client->DeleteRecording(recording);

  if (error != PVR_ERROR_NO_ERROR)
    CLog::Log(LOGERROR, "PVR - %s - cannot delete recording from client '%d': %s",__FUNCTION__, recording.m_iClientId, CPVRClient::ToString(error));

  return error;
}

PVR_ERROR CPVRClients::UndeleteRecording(const CPVRRecording &recording)
{
  PVR_ERROR error(PVR_ERROR_UNKNOWN);

  if (!recording.IsDeleted())
    return error;

  PVR_CLIENT client;
  if (GetCreatedClient(recording.m_iClientId, client))
    error = client->UndeleteRecording(recording);

  if (error != PVR_ERROR_NO_ERROR)
    CLog::Log(LOGERROR, "PVR - %s - cannot undelete recording from client '%d': %s",__FUNCTION__, recording.m_iClientId, CPVRClient::ToString(error));

  return error;
}

PVR_ERROR CPVRClients::DeleteAllRecordingsFromTrash()
{
  PVR_ERROR error(PVR_ERROR_NO_ERROR);
  PVR_CLIENTMAP clients;
  GetCreatedClients(clients);

  for (const auto &client : clients)
  {
    if (client.second->SupportsRecordingsUndelete() && client.second->GetRecordingsAmount(true) > 0)
    {
      PVR_ERROR currentError = client.second->DeleteAllRecordingsFromTrash();
      if (currentError != PVR_ERROR_NO_ERROR)
      {
        CLog::Log(LOGERROR, "PVR - %s - cannot delete all recordings from client '%d': %s",__FUNCTION__, client.second->GetID(), CPVRClient::ToString(currentError));
        error = currentError;
      }
    }
  }

  return error;
}

bool CPVRClients::SetRecordingLastPlayedPosition(const CPVRRecording &recording, int lastplayedposition, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKNOWN;
  PVR_CLIENT client;
  if (GetCreatedClient(recording.m_iClientId, client) && client->SupportsRecordings())
    *error = client->SetRecordingLastPlayedPosition(recording, lastplayedposition);
  else
    CLog::Log(LOGERROR, "PVR - %s - client %d does not support recordings",__FUNCTION__, recording.m_iClientId);

  return *error == PVR_ERROR_NO_ERROR;
}

int CPVRClients::GetRecordingLastPlayedPosition(const CPVRRecording &recording)
{
  int rc = 0;

  PVR_CLIENT client;
  if (GetCreatedClient(recording.m_iClientId, client) && client->SupportsRecordings())
    rc = client->GetRecordingLastPlayedPosition(recording);
  else
    CLog::Log(LOGERROR, "PVR - %s - client %d does not support recordings",__FUNCTION__, recording.m_iClientId);

  return rc;
}

bool CPVRClients::SetRecordingPlayCount(const CPVRRecording &recording, int count, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKNOWN;
  PVR_CLIENT client;
  if (GetCreatedClient(recording.m_iClientId, client) && client->SupportsRecordingPlayCount())
    *error = client->SetRecordingPlayCount(recording, count);
  else
    CLog::Log(LOGERROR, "PVR - %s - client %d does not support setting recording's play count",__FUNCTION__, recording.m_iClientId);

  return *error == PVR_ERROR_NO_ERROR;
}

std::vector<PVR_EDL_ENTRY> CPVRClients::GetRecordingEdl(const CPVRRecording &recording)
{
  PVR_CLIENT client;
  if (GetCreatedClient(recording.m_iClientId, client) && client->SupportsRecordingEdl())
    return client->GetRecordingEdl(recording);
  else
    CLog::Log(LOGERROR, "PVR - %s - client %d does not support getting Edl", __FUNCTION__, recording.m_iClientId);

  return std::vector<PVR_EDL_ENTRY>();
}

bool CPVRClients::IsRecordingOnPlayingChannel(void) const
{
  CPVRChannelPtr currentChannel(GetPlayingChannel());
  return currentChannel && currentChannel->IsRecording();
}

bool CPVRClients::CanRecordInstantly(void)
{
  CPVRChannelPtr currentChannel(GetPlayingChannel());
  return currentChannel && currentChannel->CanRecord();
}

bool CPVRClients::CanPauseStream(void) const
{
  PVR_CLIENT client;

  if (GetPlayingClient(client))
  {
    return m_bIsPlayingRecording || client->CanPauseStream();
  }

  return false;
}

bool CPVRClients::CanSeekStream(void) const
{
  PVR_CLIENT client;

  if (GetPlayingClient(client))
  {
    return m_bIsPlayingRecording || client->CanSeekStream();
  }

  return false;
}

PVR_ERROR CPVRClients::GetEPGForChannel(const CPVRChannelPtr &channel, CPVREpg *epg, time_t start, time_t end)
{
  assert(channel.get());

  PVR_ERROR error(PVR_ERROR_UNKNOWN);
  PVR_CLIENT client;
  if (GetCreatedClient(channel->ClientID(), client))
    error = client->GetEPGForChannel(channel, epg, start, end);

  if (error != PVR_ERROR_NO_ERROR)
    CLog::Log(LOGERROR, "PVR - %s - cannot get EPG for channel '%s' from client '%d': %s",__FUNCTION__, channel->ChannelName().c_str(), channel->ClientID(), CPVRClient::ToString(error));
  return error;
}

PVR_ERROR CPVRClients::SetEPGTimeFrame(int iDays)
{
  PVR_ERROR error(PVR_ERROR_NO_ERROR);
  PVR_CLIENTMAP clients;
  GetCreatedClients(clients);

  for (const auto &client : clients)
  {
    PVR_ERROR currentError = client.second->SetEPGTimeFrame(iDays);
    if (currentError != PVR_ERROR_NOT_IMPLEMENTED &&
        currentError != PVR_ERROR_NO_ERROR)
    {
      error = currentError;
      CLog::Log(LOGERROR, "PVR - %s - cannot set epg time frame for client '%d': %s",__FUNCTION__, client.first, CPVRClient::ToString(error));
    }
  }

  return error;
}

PVR_ERROR CPVRClients::GetChannels(CPVRChannelGroupInternal *group)
{
  PVR_ERROR error(PVR_ERROR_NO_ERROR);
  PVR_CLIENTMAP clients;
  GetCreatedClients(clients);

  /* get the channel list from each client */
  for (const auto &client : clients)
  {
    PVR_ERROR currentError = client.second->GetChannels(*group, group->IsRadio());
    if (currentError != PVR_ERROR_NOT_IMPLEMENTED &&
        currentError != PVR_ERROR_NO_ERROR)
    {
      error = currentError;
      CLog::Log(LOGERROR, "PVR - %s - cannot get channels from client '%d': %s",__FUNCTION__, client.first, CPVRClient::ToString(error));
    }
  }

  return error;
}

PVR_ERROR CPVRClients::GetChannelGroups(CPVRChannelGroups *groups)
{
  PVR_ERROR error(PVR_ERROR_NO_ERROR);
  PVR_CLIENTMAP clients;
  GetCreatedClients(clients);

  for (const auto &client : clients)
  {
    PVR_ERROR currentError = client.second->GetChannelGroups(groups);
    if (currentError != PVR_ERROR_NOT_IMPLEMENTED &&
        currentError != PVR_ERROR_NO_ERROR)
    {
      error = currentError;
      CLog::Log(LOGERROR, "PVR - %s - cannot get groups from client '%d': %s",__FUNCTION__, client.first, CPVRClient::ToString(error));
    }
  }

  return error;
}

PVR_ERROR CPVRClients::GetChannelGroupMembers(CPVRChannelGroup *group)
{
  PVR_ERROR error(PVR_ERROR_NO_ERROR);
  PVR_CLIENTMAP clients;
  GetCreatedClients(clients);

  /* get the member list from each client */
  for (const auto &client : clients)
  {
    PVR_ERROR currentError = client.second->GetChannelGroupMembers(group);
    if (currentError != PVR_ERROR_NOT_IMPLEMENTED &&
        currentError != PVR_ERROR_NO_ERROR)
    {
      error = currentError;
      CLog::Log(LOGERROR, "PVR - %s - cannot get group members from client '%d': %s",__FUNCTION__, client.first, CPVRClient::ToString(error));
    }
  }

  return error;
}

bool CPVRClients::HasMenuHooks(int iClientID, PVR_MENUHOOK_CAT cat)
{
  if (iClientID < 0)
    iClientID = GetPlayingClientID();

  PVR_CLIENT client;
  return (GetCreatedClient(iClientID, client) && client->HasMenuHooks(cat));
}

std::vector<PVR_CLIENT> CPVRClients::GetClientsSupportingChannelScan(void) const
{
  std::vector<PVR_CLIENT> possibleScanClients;
  CSingleLock lock(m_critSection);

  /* get clients that support channel scanning */
  for (const auto &client : m_clientMap)
  {
    if (client.second->ReadyToUse() && client.second->SupportsChannelScan())
      possibleScanClients.push_back(client.second);
  }

  return possibleScanClients;
}

std::vector<PVR_CLIENT> CPVRClients::GetClientsSupportingChannelSettings(bool bRadio) const
{
  std::vector<PVR_CLIENT> possibleSettingsClients;
  CSingleLock lock(m_critSection);

  /* get clients that support channel settings */
  for (const auto &client : m_clientMap)
  {
    if (client.second->ReadyToUse() && client.second->SupportsChannelSettings() &&
         ((bRadio && client.second->SupportsRadio()) || (!bRadio && client.second->SupportsTV())))
      possibleSettingsClients.push_back(client.second);
  }

  return possibleSettingsClients;
}

PVR_ERROR CPVRClients::OpenDialogChannelAdd(const CPVRChannelPtr &channel)
{
  PVR_ERROR error = PVR_ERROR_UNKNOWN;

  PVR_CLIENT client;
  if (GetCreatedClient(channel->ClientID(), client))
    error = client->OpenDialogChannelAdd(channel);
  else
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, channel->ClientID());

  return error;
}

PVR_ERROR CPVRClients::OpenDialogChannelSettings(const CPVRChannelPtr &channel)
{
  PVR_ERROR error = PVR_ERROR_UNKNOWN;

  PVR_CLIENT client;
  if (GetCreatedClient(channel->ClientID(), client))
    error = client->OpenDialogChannelSettings(channel);
  else
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, channel->ClientID());

  return error;
}

PVR_ERROR CPVRClients::DeleteChannel(const CPVRChannelPtr &channel)
{
  PVR_ERROR error = PVR_ERROR_UNKNOWN;

  PVR_CLIENT client;
  if (GetCreatedClient(channel->ClientID(), client))
    error = client->DeleteChannel(channel);
  else
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, channel->ClientID());

  return error;
}

bool CPVRClients::RenameChannel(const CPVRChannelPtr &channel)
{
  PVR_ERROR error = PVR_ERROR_UNKNOWN;

  PVR_CLIENT client;
  if (GetCreatedClient(channel->ClientID(), client))
    error = client->RenameChannel(channel);
  else
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, channel->ClientID());

  return (error == PVR_ERROR_NO_ERROR || error == PVR_ERROR_NOT_IMPLEMENTED);
}

bool CPVRClients::IsKnownClient(const AddonPtr &client) const
{
  // database IDs start at 1
  return GetClientId(client) > 0;
}

void CPVRClients::UpdateAddons(void)
{
  VECADDONS addons;
  CAddonMgr::GetInstance().GetInstalledAddons(addons, ADDON_PVRDLL);

  if (addons.empty())
    return;

  for (auto &addon : addons)
  {
    bool bEnabled = !CAddonMgr::GetInstance().IsAddonDisabled(addon->ID());

    if (bEnabled && (!IsKnownClient(addon) || !IsCreatedClient(addon)))
    {
      std::hash<std::string> hasher;
      int iClientId = static_cast<int>(hasher(addon->ID()));
      if (iClientId < 0)
        iClientId = -iClientId;

      ADDON_STATUS status;
      if (IsKnownClient(addon))
      {
        PVR_CLIENT client;
        GetClient(iClientId, client);
        status = client->Create(iClientId);
      }
      else
      {
        PVR_CLIENT pvrclient = std::dynamic_pointer_cast<CPVRClient>(addon);
        if (!pvrclient)
        {
          CLog::Log(LOGERROR, "CPVRClients - %s - severe error, incorrect add-on type", __FUNCTION__);
          continue;
        }
        status = pvrclient.get()->Create(iClientId);
        // register the add-on
        if (m_clientMap.find(iClientId) == m_clientMap.end())
        {
          m_clientMap.insert(std::make_pair(iClientId, pvrclient));
          m_addonNameIds.insert(make_pair(addon->ID(), iClientId));
        }
      }

      if (status != ADDON_STATUS_OK)
      {
        CLog::Log(LOGERROR, "%s - failed to create add-on %s, status = %d", __FUNCTION__, addon->Name().c_str(), status);
        if (status == ADDON_STATUS_PERMANENT_FAILURE)
        {
          CAddonMgr::GetInstance().DisableAddon(addon->ID());
          CJobManager::GetInstance().AddJob(new CPVREventlogJob(true, true, addon->Name(), g_localizeStrings.Get(24070), addon->Icon()), nullptr);
        }
      }
    }
    else if (IsCreatedClient(addon))
    {
      // stop add-on if it's no longer enabled, restart add-on if it's still enabled
      StopClient(addon, bEnabled);
    }
  }

  CServiceBroker::GetPVRManager().Start();
}

bool CPVRClients::GetClient(const std::string &strId, AddonPtr &addon) const
{
  CSingleLock lock(m_critSection);
  for (const auto &client : m_clientMap)
  {
    if (client.second->ID() == strId)
    {
      addon = client.second;
      return true;
    }
  }
  return false;
}

bool CPVRClients::SupportsTimers() const
{
  PVR_CLIENTMAP clients;
  GetCreatedClients(clients);

  for (const auto &entry : clients)
  {
    if (entry.second->SupportsTimers())
      return true;
  }
  return false;
}

bool CPVRClients::SupportsChannelGroups(int iClientId) const
{
  PVR_CLIENT client;
  return GetCreatedClient(iClientId, client) && client->SupportsChannelGroups();
}

bool CPVRClients::SupportsChannelScan(int iClientId) const
{
  PVR_CLIENT client;
  return GetCreatedClient(iClientId, client) && client->SupportsChannelScan();
}

bool CPVRClients::SupportsChannelSettings(int iClientId) const
{
  PVR_CLIENT client;
  return GetCreatedClient(iClientId, client) && client->SupportsChannelSettings();
}

bool CPVRClients::SupportsEPG(int iClientId) const
{
  PVR_CLIENT client;
  return GetCreatedClient(iClientId, client) && client->SupportsEPG();
}

bool CPVRClients::SupportsLastPlayedPosition(int iClientId) const
{
  PVR_CLIENT client;
  return GetCreatedClient(iClientId, client) && client->SupportsLastPlayedPosition();
}

bool CPVRClients::SupportsRadio(int iClientId) const
{
  PVR_CLIENT client;
  return GetCreatedClient(iClientId, client) && client->SupportsRadio();
}

bool CPVRClients::SupportsRecordings(int iClientId) const
{
  PVR_CLIENT client;
  return GetCreatedClient(iClientId, client) && client->SupportsRecordings();
}

bool CPVRClients::SupportsRecordingsUndelete(int iClientId) const
{
  PVR_CLIENT client;
  return GetCreatedClient(iClientId, client) && client->SupportsRecordingsUndelete();
}

bool CPVRClients::SupportsRecordingPlayCount(int iClientId) const
{
  PVR_CLIENT client;
  return GetCreatedClient(iClientId, client) && client->SupportsRecordingPlayCount();
}

bool CPVRClients::SupportsRecordingEdl(int iClientId) const
{
  PVR_CLIENT client;
  return GetCreatedClient(iClientId, client) && client->SupportsRecordingEdl();
}

bool CPVRClients::SupportsRecordingsRename(int iClientId) const
{
  PVR_CLIENT client;
  return GetCreatedClient(iClientId, client) && client->SupportsRecordingsRename();
}

bool CPVRClients::SupportsTimers(int iClientId) const
{
  PVR_CLIENT client;
  return GetCreatedClient(iClientId, client) && client->SupportsTimers();
}

bool CPVRClients::SupportsTV(int iClientId) const
{
  PVR_CLIENT client;
  return GetCreatedClient(iClientId, client) && client->SupportsTV();
}

bool CPVRClients::HandlesDemuxing(int iClientId) const
{
  PVR_CLIENT client;
  return GetCreatedClient(iClientId, client) && client->HandlesDemuxing();
}

bool CPVRClients::HandlesInputStream(int iClientId) const
{
  PVR_CLIENT client;
  return GetCreatedClient(iClientId, client) && client->HandlesInputStream();
}

bool CPVRClients::GetPlayingClient(PVR_CLIENT &client) const
{
  return GetCreatedClient(GetPlayingClientID(), client);
}

bool CPVRClients::OpenStream(const CPVRChannelPtr &channel, bool bIsSwitchingChannel)
{
  assert(channel.get());

  bool bReturn(false);
  CloseStream();

  /* try to open the stream on the client */
  PVR_CLIENT client;
  if (GetCreatedClient(channel->ClientID(), client) &&
      client->OpenStream(channel, bIsSwitchingChannel))
  {
    CSingleLock lock(m_critSection);
    m_playingClientId = channel->ClientID();
    m_bIsPlayingLiveTV = true;

    if (client.get())
      m_strPlayingClientName = client->GetFriendlyName();
    else
      m_strPlayingClientName = g_localizeStrings.Get(13205);

    bReturn = true;
  }

  return bReturn;
}

bool CPVRClients::OpenStream(const CPVRRecordingPtr &channel)
{
  assert(channel.get());

  bool bReturn(false);
  CloseStream();

  /* try to open the recording stream on the client */
  PVR_CLIENT client;
  if (GetCreatedClient(channel->m_iClientId, client) &&
      client->OpenStream(channel))
  {
    CSingleLock lock(m_critSection);
    m_playingClientId = channel->m_iClientId;
    m_bIsPlayingRecording = true;
    m_strPlayingClientName = client->GetFriendlyName();
    bReturn = true;
  }

  return bReturn;
}

void CPVRClients::CloseStream(void)
{
  PVR_CLIENT playingClient;
  if (GetPlayingClient(playingClient))
    playingClient->CloseStream();

  CSingleLock lock(m_critSection);
  m_bIsPlayingLiveTV     = false;
  m_bIsPlayingRecording  = false;
  m_playingClientId      = PVR_INVALID_CLIENT_ID;
  m_strPlayingClientName = "";
}

int CPVRClients::ReadStream(void* lpBuf, int64_t uiBufSize)
{
  PVR_CLIENT client;
  if (GetPlayingClient(client))
    return client->ReadStream(lpBuf, uiBufSize);
  return -EINVAL;
}

int64_t CPVRClients::GetStreamLength(void)
{
  PVR_CLIENT client;
  if (GetPlayingClient(client))
    return client->GetStreamLength();
  return -EINVAL;
}

int64_t CPVRClients::SeekStream(int64_t iFilePosition, int iWhence/* = SEEK_SET*/)
{
  PVR_CLIENT client;
  if (GetPlayingClient(client))
    return client->SeekStream(iFilePosition, iWhence);
  return -EINVAL;
}

void CPVRClients::PauseStream(bool bPaused)
{
  PVR_CLIENT client;
  if (GetPlayingClient(client))
    client->PauseStream(bPaused);
}

std::string CPVRClients::GetCurrentInputFormat(void) const
{
  std::string strReturn;
  CPVRChannelPtr currentChannel(GetPlayingChannel());
  if (currentChannel)
    strReturn = currentChannel->InputFormat();

  return strReturn;
}

bool CPVRClients::IsPlaying(void) const
{
  CSingleLock lock(m_critSection);
  return m_bIsPlayingRecording || m_bIsPlayingLiveTV;
}

bool CPVRClients::IsPlayingRadio(void) const
{
  PVR_CLIENT client;
  if (GetPlayingClient(client))
    return client->IsPlayingLiveRadio();
  return false;
}

bool CPVRClients::IsPlayingTV(void) const
{
  PVR_CLIENT client;
  if (GetPlayingClient(client))
    return client->IsPlayingLiveTV();
  return false;
}

bool CPVRClients::IsPlayingRecording(void) const
{
  CSingleLock lock(m_critSection);
  return m_bIsPlayingRecording;
}

bool CPVRClients::IsEncrypted(void) const
{
  PVR_CLIENT client;
  if (GetPlayingClient(client))
    return client->IsPlayingEncryptedChannel();
  return false;
}

std::string CPVRClients::GetBackendHostnameByClientId(int iClientId) const
{
  PVR_CLIENT client;
  std::string name;

  if (GetCreatedClient(iClientId, client))
  {
    name = client->GetBackendHostname();
  }

  return name;
}

time_t CPVRClients::GetPlayingTime() const
{
  PVR_CLIENT client;
  time_t time = 0;

  if (GetPlayingClient(client))
  {
     time = client->GetPlayingTime();
  }

  return time;
}

bool CPVRClients::IsTimeshifting(void) const
{
  PVR_CLIENT client;
  if (GetPlayingClient(client))
    return client->IsTimeshifting();
  return false;
}

time_t CPVRClients::GetBufferTimeStart() const
{
  PVR_CLIENT client;
  time_t time = 0;

  if (GetPlayingClient(client))
  {
    time = client->GetBufferTimeStart();
  }

  return time;
}

time_t CPVRClients::GetBufferTimeEnd() const
{
  PVR_CLIENT client;
  time_t time = 0;

  if (GetPlayingClient(client))
  {
    time = client->GetBufferTimeEnd();
  }

  return time;
}

bool CPVRClients::IsRealTimeStream(void) const
{
  PVR_CLIENT client;
  if (GetPlayingClient(client))
    return client->IsRealTimeStream();
  return false;
}

void CPVRClients::ConnectionStateChange(CPVRClient *client, std::string &strConnectionString, PVR_CONNECTION_STATE newState,
                                        std::string &strMessage)
{
  if (!client)
  {
    CLog::Log(LOGDEBUG, "PVR - %s - invalid client id", __FUNCTION__);
    return;
  }

  if (strConnectionString.empty())
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  int iMsg(-1);
  bool bError(true);
  bool bNotify(true);

  switch (newState)
  {
    case PVR_CONNECTION_STATE_SERVER_UNREACHABLE:
      iMsg = 35505; // Server is unreachable
      break;
    case PVR_CONNECTION_STATE_SERVER_MISMATCH:
      iMsg = 35506; // Server does not respond properly
      break;
    case PVR_CONNECTION_STATE_VERSION_MISMATCH:
      iMsg = 35507; // Server version is not compatible
      break;
    case PVR_CONNECTION_STATE_ACCESS_DENIED:
      iMsg = 35508; // Access denied
      break;
    case PVR_CONNECTION_STATE_CONNECTED:
      bError = false;
      iMsg = 36034; // Connection established
      if (client->GetPreviousConnectionState() == PVR_CONNECTION_STATE_UNKNOWN ||
          client->GetPreviousConnectionState() == PVR_CONNECTION_STATE_CONNECTING)
        bNotify = false;
      break;
    case PVR_CONNECTION_STATE_DISCONNECTED:
      iMsg = 36030; // Connection lost
      break;
    case PVR_CONNECTION_STATE_CONNECTING:
      bError = false;
      iMsg = 35509; // Connecting
      bNotify = false;
      break;
    default:
      CLog::Log(LOGERROR, "PVR - %s - unknown connection state", __FUNCTION__);
      return;
  }

  // Use addon-supplied message, if present
  std::string strMsg;
  if (!strMessage.empty())
    strMsg = strMessage;
  else
    strMsg = g_localizeStrings.Get(iMsg);

  // Notify user.
  CJobManager::GetInstance().AddJob(new CPVREventlogJob(bNotify, bError, client->Name(), strMsg, client->Icon()), nullptr);

  if (newState == PVR_CONNECTION_STATE_CONNECTED)
  {
    // update properties on connect
    if (!client->GetAddonProperties())
    {
      CLog::Log(LOGERROR, "PVR - %s - error reading properties", __FUNCTION__);
    }
    CServiceBroker::GetPVRManager().Start();
  }
}

void CPVRClients::OnSystemSleep()
{
  PVR_CLIENTMAP clients;
  GetCreatedClients(clients);

  /* propagate event to each client */
  for (auto &client : clients)
    client.second->OnSystemSleep();
}

void CPVRClients::OnSystemWake()
{
  PVR_CLIENTMAP clients;
  GetCreatedClients(clients);

  /* propagate event to each client */
  for (auto &client : clients)
    client.second->OnSystemWake();
}

void CPVRClients::OnPowerSavingActivated()
{
  PVR_CLIENTMAP clients;
  GetCreatedClients(clients);

  /* propagate event to each client */
  for (auto &client : clients)
    client.second->OnPowerSavingActivated();
}

void CPVRClients::OnPowerSavingDeactivated()
{
  PVR_CLIENTMAP clients;
  GetCreatedClients(clients);

  /* propagate event to each client */
  for (auto &client : clients)
    client.second->OnPowerSavingDeactivated();
}
