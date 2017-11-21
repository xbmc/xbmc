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

#include <utility>
#include <functional>

#include "Application.h"
#include "ServiceBroker.h"
#include "addons/BinaryAddonCache.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/ApplicationMessenger.h"
#include "utils/log.h"

#include "pvr/PVRJobs.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupInternal.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimers.h"

using namespace ADDON;
using namespace PVR;
using namespace KODI::MESSAGING;

namespace
{
  int ClientIdFromAddonId(const std::string &strID)
  {
    std::hash<std::string> hasher;
    int iClientId = static_cast<int>(hasher(strID));
    if (iClientId < 0)
      iClientId = -iClientId;
    return iClientId;
  }

} // unnamed namespace

CPVRClients::CPVRClients(void)
: m_playingClientId(-EINVAL),
  m_bIsPlayingLiveTV(false),
  m_bIsPlayingRecording(false),
  m_bIsPlayingEpgTag(false)
{
  CServiceBroker::GetAddonMgr().RegisterAddonMgrCallback(ADDON_PVRDLL, this);
  CServiceBroker::GetAddonMgr().Events().Subscribe(this, &CPVRClients::OnAddonEvent);
}

CPVRClients::~CPVRClients(void)
{
  CServiceBroker::GetAddonMgr().Events().Unsubscribe(this);
  CServiceBroker::GetAddonMgr().UnregisterAddonMgrCallback(ADDON_PVRDLL);

  for (const auto &client : m_clientMap)
  {
    client.second->Destroy();
  }
}

void CPVRClients::Start(void)
{
  UpdateAddons();
}

void CPVRClients::Stop()
{
  CSingleLock lock(m_critSection);
  for (const auto &client : m_clientMap)
  {
    client.second->Stop();
  }
}

void CPVRClients::Continue()
{
  CSingleLock lock(m_critSection);
  for (const auto &client : m_clientMap)
  {
    client.second->Continue();
  }
}

void CPVRClients::UpdateAddons(const std::string &changedAddonId /*= ""*/)
{
  VECADDONS addons;
  CServiceBroker::GetAddonMgr().GetInstalledAddons(addons, ADDON_PVRDLL);

  if (addons.empty())
    return;

  bool bFoundChangedAddon = changedAddonId.empty();
  std::vector<std::pair<AddonPtr, bool>> addonsWithStatus;
  for (const auto &addon : addons)
  {
    bool bEnabled = !CServiceBroker::GetAddonMgr().IsAddonDisabled(addon->ID());
    addonsWithStatus.emplace_back(std::make_pair(addon, bEnabled));

    if (!bFoundChangedAddon && addon->ID() == changedAddonId)
      bFoundChangedAddon = true;
  }

  if (!bFoundChangedAddon)
    return; // changed addon is not a known pvr client addon, so nothing to update

  addons.clear();

  std::vector<std::pair<CPVRClientPtr, int>> addonsToCreate;
  std::vector<AddonPtr> addonsToReCreate;
  std::vector<AddonPtr> addonsToDestroy;

  {
    CSingleLock lock(m_critSection);
    for (const auto &addonWithStatus : addonsWithStatus)
    {
      AddonPtr addon = addonWithStatus.first;
      bool bEnabled = addonWithStatus.second;

      if (bEnabled && (!IsKnownClient(addon) || !IsCreatedClient(addon)))
      {
        int iClientId = ClientIdFromAddonId(addon->ID());

        CPVRClientPtr client;
        if (IsKnownClient(addon))
        {
          GetClient(iClientId, client);
        }
        else
        {
          client = std::dynamic_pointer_cast<CPVRClient>(addon);
          if (!client)
          {
            CLog::Log(LOGERROR, "CPVRClients - %s - severe error, incorrect add-on type", __FUNCTION__);
            continue;
          }
        }
        addonsToCreate.emplace_back(std::make_pair(client, iClientId));
      }
      else if (IsCreatedClient(addon))
      {
        if (bEnabled)
          addonsToReCreate.emplace_back(addon);
        else
          addonsToDestroy.emplace_back(addon);
      }
    }
  }

  if (!addonsToCreate.empty() || !addonsToReCreate.empty() || !addonsToDestroy.empty())
  {
    CServiceBroker::GetPVRManager().Stop();

    for (const auto& addon : addonsToCreate)
    {
      ADDON_STATUS status = addon.first->Create(addon.second);

      if (status != ADDON_STATUS_OK)
      {
        CLog::Log(LOGERROR, "%s - failed to create add-on %s, status = %d", __FUNCTION__, addon.first->Name().c_str(), status);
        if (status == ADDON_STATUS_PERMANENT_FAILURE)
        {
          CServiceBroker::GetAddonMgr().DisableAddon(addon.first->ID());
          CJobManager::GetInstance().AddJob(new CPVREventlogJob(true, true, addon.first->Name(), g_localizeStrings.Get(24070), addon.first->Icon()), nullptr);
        }
      }
    }

    for (const auto& addon : addonsToReCreate)
    {
      // recreate client
      StopClient(addon, true);
    }

    for (const auto& addon : addonsToDestroy)
    {
      // destroy client
      StopClient(addon, false);
    }

    if (!addonsToCreate.empty())
    {
      // update created clients map
      CSingleLock lock(m_critSection);
      for (const auto& addon : addonsToCreate)
      {
        if (m_clientMap.find(addon.second) == m_clientMap.end())
        {
          m_clientMap.insert(std::make_pair(addon.second, addon.first));
          m_addonNameIds.insert(make_pair(addon.first->ID(), addon.second));
        }
      }
    }

    CServiceBroker::GetPVRManager().Start();
  }
}

bool CPVRClients::RequestRestart(AddonPtr addon, bool bDataChanged)
{
  return StopClient(addon, true);
}

bool CPVRClients::StopClient(const AddonPtr &addon, bool bRestart)
{
  /* stop playback if needed */
  if (IsPlaying())
    CApplicationMessenger::GetInstance().SendMsg(TMSG_MEDIA_STOP);

  CSingleLock lock(m_critSection);

  int iId = GetClientId(addon);
  CPVRClientPtr mappedClient;
  if (GetClient(iId, mappedClient))
  {
    if (bRestart)
    {
      mappedClient->ReCreate();
    }
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

void CPVRClients::OnAddonEvent(const AddonEvent& event)
{
  if (typeid(event) == typeid(AddonEvents::Enabled) ||
      typeid(event) == typeid(AddonEvents::Disabled))
  {
    // update addons
    CJobManager::GetInstance().AddJob(new CPVRUpdateAddonsJob(event.id), nullptr);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// client access
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

bool CPVRClients::GetClient(int iClientId, CPVRClientPtr &addon) const
{
  bool bReturn = false;
  if (iClientId <= PVR_INVALID_CLIENT_ID)
    return bReturn;

  CSingleLock lock(m_critSection);
  const auto &itr = m_clientMap.find(iClientId);
  if (itr != m_clientMap.end())
  {
    addon = itr->second;
    bReturn = true;
  }

  return bReturn;
}

int CPVRClients::GetClientId(const AddonPtr &client) const
{
  CSingleLock lock(m_critSection);
  for (const auto &entry : m_clientMap)
  {
    if (entry.second->ID() == client->ID())
      return entry.first;
  }

  return -1;
}

int CPVRClients::GetClientId(const std::string& strId) const
{
  CSingleLock lock(m_critSection);
  const auto& it = m_addonNameIds.find(strId);
  return it != m_addonNameIds.end() ? it->second : -1;
}

int CPVRClients::CreatedClientAmount(void) const
{
  int iReturn = 0;

  CSingleLock lock(m_critSection);
  for (const auto &client : m_clientMap)
  {
    if (client.second->ReadyToUse())
      ++iReturn;
  }

  return iReturn;
}

bool CPVRClients::HasCreatedClients(void) const
{
  CSingleLock lock(m_critSection);
  for (const auto &client : m_clientMap)
  {
    if (client.second->ReadyToUse() && !client.second->IgnoreClient())
      return true;
  }

  return false;
}

bool CPVRClients::IsKnownClient(const AddonPtr &client) const
{
  // valid client IDs start at 1
  return GetClientId(client) > 0;
}

bool CPVRClients::IsCreatedClient(int iClientId) const
{
  CPVRClientPtr client;
  return GetCreatedClient(iClientId, client);
}

bool CPVRClients::IsCreatedClient(const AddonPtr &addon)
{
  CSingleLock lock(m_critSection);
  for (const auto &client : m_clientMap)
  {
    if (client.second->ID() == addon->ID())
      return client.second->ReadyToUse();
  }
  return false;
}

bool CPVRClients::GetCreatedClient(int iClientId, CPVRClientPtr &addon) const
{
  if (GetClient(iClientId, addon))
    return addon->ReadyToUse();

  return false;
}

int CPVRClients::GetCreatedClients(CPVRClientMap &clients) const
{
  int iReturn = 0;

  CSingleLock lock(m_critSection);
  for (const auto &client : m_clientMap)
  {
    if (client.second->ReadyToUse() && !client.second->IgnoreClient())
    {
      clients.insert(std::make_pair(client.second->GetID(), client.second));
      ++iReturn;
    }
  }

  return iReturn;
}

PVR_ERROR CPVRClients::GetCreatedClients(CPVRClientMap &clientsReady, std::vector<int> &clientsNotReady) const
{
  clientsNotReady.clear();

  VECADDONS addons;
  CBinaryAddonCache &addonCache = CServiceBroker::GetBinaryAddonCache();
  addonCache.GetAddons(addons, ADDON::ADDON_PVRDLL);

  for (const auto &addon : addons)
  {
    int iClientId = ClientIdFromAddonId(addon->ID());
    CPVRClientPtr client;
    GetClient(iClientId, client);

    if (client && client->ReadyToUse() && !client->IgnoreClient())
    {
      clientsReady.insert(std::make_pair(iClientId, client));
    }
    else
    {
      clientsNotReady.emplace_back(iClientId);
    }
  }

  return clientsNotReady.empty() ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR;
}

int CPVRClients::GetFirstCreatedClientID(void)
{
  CSingleLock lock(m_critSection);
  for (const auto &client : m_clientMap)
  {
    if (client.second->ReadyToUse())
      return client.second->GetID();
  }

  return -1;
}

int CPVRClients::EnabledClientAmount(void) const
{
  int iReturn = 0;

  CPVRClientMap clientMap;
  {
    CSingleLock lock(m_critSection);
    clientMap = m_clientMap;
  }

  for (const auto &client : clientMap)
  {
    if (!CServiceBroker::GetAddonMgr().IsAddonDisabled(client.second->ID()))
      ++iReturn;
  }

  return iReturn;
}

bool CPVRClients::GetClientFriendlyName(int iClientId, std::string &strName) const
{
  CPVRClientPtr client;
  bool bReturn = GetCreatedClient(iClientId, client);
  if (bReturn)
    strName = client->GetFriendlyName();

  return bReturn;
}

bool CPVRClients::GetClientAddonName(int iClientId, std::string &strName) const
{
  CPVRClientPtr client;
  bool bReturn = GetCreatedClient(iClientId, client);
  if (bReturn)
    strName = client->Name();

  return bReturn;
}

bool CPVRClients::GetClientAddonIcon(int iClientId, std::string &strIcon) const
{
  CPVRClientPtr client;
  bool bReturn = GetCreatedClient(iClientId, client);
  if (bReturn)
    strIcon = client->Icon();

  return bReturn;
}

std::string CPVRClients::GetClientAddonId(int iClientId) const
{
  CPVRClientPtr client;
  return GetClient(iClientId, client) ? client->ID() : "";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// playing client access
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CPVRClients::IsPlaying(void) const
{
  CSingleLock lock(m_critSection);
  return m_bIsPlayingRecording || m_bIsPlayingLiveTV || m_bIsPlayingEpgTag;
}

bool CPVRClients::IsPlayingRadio(void) const
{
  CPVRClientPtr client;
  if (GetPlayingClient(client))
    return client->IsPlayingLiveRadio();

  return false;
}

bool CPVRClients::IsPlayingTV(void) const
{
  CPVRClientPtr client;
  if (GetPlayingClient(client))
    return client->IsPlayingLiveTV();

  return false;
}

bool CPVRClients::IsPlayingRecording(void) const
{
  CSingleLock lock(m_critSection);
  return m_bIsPlayingRecording;
}

bool CPVRClients::IsPlayingEpgTag(void) const
{
  CSingleLock lock(m_critSection);
  return m_bIsPlayingEpgTag;
}

bool CPVRClients::IsPlayingEncryptedChannel(void) const
{
  CPVRClientPtr client;
  if (GetPlayingClient(client))
    return client->IsPlayingEncryptedChannel();

  return false;
}

bool CPVRClients::GetPlayingClient(CPVRClientPtr &client) const
{
  return GetCreatedClient(GetPlayingClientID(), client);
}

int CPVRClients::GetPlayingClientID(void) const
{
  CSingleLock lock(m_critSection);
  if (m_bIsPlayingLiveTV || m_bIsPlayingRecording || m_bIsPlayingEpgTag)
    return m_playingClientId;

  return -EINVAL;
}

const std::string CPVRClients::GetPlayingClientName(void) const
{
  CSingleLock lock(m_critSection);
  return m_strPlayingClientName;
}

void CPVRClients::SetPlayingChannel(const CPVRChannelPtr &channel)
{
  const CPVRChannelPtr playingChannel = GetPlayingChannel();
  if (!playingChannel || *playingChannel != *channel)
  {
    if (playingChannel)
      ClearPlayingChannel();

    CPVRClientPtr client;
    if (GetCreatedClient(channel->ClientID(), client))
    {
      client->SetPlayingChannel(channel);

      CSingleLock lock(m_critSection);
      m_playingClientId = channel->ClientID();
      m_bIsPlayingLiveTV = true;
      m_strPlayingClientName = client->GetFriendlyName();
    }
  }
}

void CPVRClients::ClearPlayingChannel()
{
  CPVRClientPtr playingClient;
  if (GetPlayingClient(playingClient))
    playingClient->ClearPlayingChannel();

  CSingleLock lock(m_critSection);
  m_bIsPlayingLiveTV = false;
  m_playingClientId = PVR_INVALID_CLIENT_ID;
  m_strPlayingClientName.clear();
}

CPVRChannelPtr CPVRClients::GetPlayingChannel() const
{
  CPVRClientPtr client;
  if (GetPlayingClient(client))
    return client->GetPlayingChannel();

  return CPVRChannelPtr();
}

void CPVRClients::SetPlayingRecording(const CPVRRecordingPtr &recording)
{
  const CPVRRecordingPtr playingRecording = GetPlayingRecording();
  if (!playingRecording || *playingRecording != *recording)
  {
    if (playingRecording)
      ClearPlayingRecording();

    CPVRClientPtr client;
    if (GetCreatedClient(recording->ClientID(), client))
    {
      client->SetPlayingRecording(recording);

      CSingleLock lock(m_critSection);
      m_playingClientId = recording->ClientID();
      m_bIsPlayingRecording = true;
      m_strPlayingClientName = client->GetFriendlyName();
    }
  }
}

void CPVRClients::ClearPlayingRecording()
{
  CPVRClientPtr playingClient;
  if (GetPlayingClient(playingClient))
    playingClient->ClearPlayingRecording();

  CSingleLock lock(m_critSection);
  m_bIsPlayingRecording = false;
  m_playingClientId = PVR_INVALID_CLIENT_ID;
  m_strPlayingClientName.clear();
}

CPVRRecordingPtr CPVRClients::GetPlayingRecording(void) const
{
  CPVRClientPtr client;
  return GetPlayingClient(client) ? client->GetPlayingRecording() : CPVRRecordingPtr();
}

void CPVRClients::SetPlayingEpgTag(const CPVREpgInfoTagPtr &epgTag)
{
  const CPVREpgInfoTagPtr playingEpgTag = GetPlayingEpgTag();
  if (!playingEpgTag || *playingEpgTag != *epgTag)
  {
    if (playingEpgTag)
      ClearPlayingEpgTag();

    CPVRClientPtr client;
    if (GetCreatedClient(epgTag->ClientID(), client))
    {
      client->SetPlayingEpgTag(epgTag);

      CSingleLock lock(m_critSection);
      m_playingClientId = epgTag->ClientID();
      m_bIsPlayingEpgTag = true;
      m_strPlayingClientName = client->GetFriendlyName();
    }
  }
}

void CPVRClients::ClearPlayingEpgTag()
{
  CPVRClientPtr playingClient;
  if (GetPlayingClient(playingClient))
    playingClient->ClearPlayingEpgTag();

  CSingleLock lock(m_critSection);
  m_bIsPlayingEpgTag = false;
  m_playingClientId = PVR_INVALID_CLIENT_ID;
  m_strPlayingClientName.clear();
}

CPVREpgInfoTagPtr CPVRClients::GetPlayingEpgTag(void) const
{
  CPVRClientPtr client;
  return GetPlayingClient(client) ? client->GetPlayingEpgTag() : CPVREpgInfoTagPtr();
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// client API calls
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*! @name general methods */
//@{

CPVRClientCapabilities CPVRClients::GetClientCapabilities(int iClientId) const
{
  CPVRClientPtr client;
  if (GetCreatedClient(iClientId, client))
    return client->GetClientCapabilities();

  CLog::Log(LOGERROR, "PVR - %s - cannot find client %d", __FUNCTION__, iClientId);
  return CPVRClientCapabilities();
}

std::vector<SBackend> CPVRClients::GetBackendProperties() const
{
  std::vector<SBackend> backendProperties;

  CPVRClientMap clients;
  GetCreatedClients(clients);

  for (const auto &clientMapEntry : clients)
  {
    const CPVRClientPtr& client = clientMapEntry.second;

    if (!client->ReadyToUse())
      continue;

    SBackend properties;

    if (client->GetDriveSpace(properties.diskTotal, properties.diskUsed) == PVR_ERROR_NO_ERROR)
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

    backendProperties.emplace_back(properties);
  }

  return backendProperties;
}

std::string CPVRClients::GetBackendHostnameByClientId(int iClientId) const
{
  std::string name;

  CPVRClientPtr client;
  if (GetCreatedClient(iClientId, client))
    name = client->GetBackendHostname();

  return name;
}

bool CPVRClients::OpenStream(const CPVRChannelPtr &channel)
{
  bool bReturn = false;
  CloseStream();

  /* try to open the stream on the client */
  CPVRClientPtr client;
  if (GetCreatedClient(channel->ClientID(), client))
    bReturn = client->OpenStream(channel);

  if (bReturn)
    SetPlayingChannel(channel);

  return bReturn;
}

bool CPVRClients::OpenStream(const CPVRRecordingPtr &recording)
{
  bool bReturn = false;

  CloseStream();

  /* try to open the recording stream on the client */
  CPVRClientPtr client;
  if (GetCreatedClient(recording->ClientID(), client))
    bReturn = client->OpenStream(recording);

  if (bReturn)
    SetPlayingRecording(recording);

  return bReturn;
}

void CPVRClients::CloseStream(void)
{
  CPVRClientPtr playingClient;
  if (GetPlayingClient(playingClient))
    playingClient->CloseStream();
}

int CPVRClients::ReadStream(void* lpBuf, int64_t uiBufSize)
{
  CPVRClientPtr client;
  if (GetPlayingClient(client))
    return client->ReadStream(lpBuf, uiBufSize);

  return -EINVAL;
}

int64_t CPVRClients::GetStreamLength(void)
{
  CPVRClientPtr client;
  if (GetPlayingClient(client))
    return client->GetStreamLength();

  return -EINVAL;
}

bool CPVRClients::CanSeekStream(void) const
{
  CPVRClientPtr client;
  if (GetPlayingClient(client))
     return m_bIsPlayingRecording || client->CanSeekStream();

  return false;
}

int64_t CPVRClients::SeekStream(int64_t iFilePosition, int iWhence/* = SEEK_SET*/)
{
  CPVRClientPtr client;
  if (GetPlayingClient(client))
    return client->SeekStream(iFilePosition, iWhence);

  return -EINVAL;
}

bool CPVRClients::CanPauseStream(void) const
{
  CPVRClientPtr client;
  if (GetPlayingClient(client))
    return m_bIsPlayingRecording || client->CanPauseStream();

  return false;
}

void CPVRClients::PauseStream(bool bPaused)
{
  CPVRClientPtr client;
  if (GetPlayingClient(client))
    client->PauseStream(bPaused);
}

std::string CPVRClients::GetCurrentInputFormat(void) const
{
  std::string strReturn;

  const CPVRChannelPtr currentChannel = GetPlayingChannel();
  if (currentChannel)
    strReturn = currentChannel->InputFormat();

  return strReturn;
}

bool CPVRClients::FillChannelStreamFileItem(CFileItem &fileItem)
{
  CPVRClientPtr client;
  if (GetCreatedClient(fileItem.GetPVRChannelInfoTag()->ClientID(), client))
    return client->FillChannelStreamFileItem(fileItem);

  return false;
}

bool CPVRClients::FillRecordingStreamFileItem(CFileItem &fileItem)
{
  CPVRClientPtr client;
  if (GetCreatedClient(fileItem.GetPVRRecordingInfoTag()->ClientID(), client))
    return client->FillRecordingStreamFileItem(fileItem);

  return false;
}

bool CPVRClients::FillEpgTagStreamFileItem(CFileItem &fileItem)
{
  CPVRClientPtr client;
  if (GetCreatedClient(fileItem.GetEPGInfoTag()->ClientID(), client))
    return client->FillEpgTagStreamFileItem(fileItem);

  return false;
}

bool CPVRClients::IsTimeshifting(void) const
{
  CPVRClientPtr client;
  if (GetPlayingClient(client))
    return client->IsTimeshifting();

  return false;
}

time_t CPVRClients::GetPlayingTime() const
{
  CPVRClientPtr client;
  if (GetPlayingClient(client))
    return client->GetPlayingTime();

  return time_t(0);
}

time_t CPVRClients::GetBufferTimeStart() const
{
  CPVRClientPtr client;
  if (GetPlayingClient(client))
    return client->GetBufferTimeStart();

  return time_t(0);
}

time_t CPVRClients::GetBufferTimeEnd() const
{
  CPVRClientPtr client;
  if (GetPlayingClient(client))
    return client->GetBufferTimeEnd();

  return time_t(0);
}

bool CPVRClients::GetStreamTimes(PVR_STREAM_TIMES *times) const
{
  CPVRClientPtr client;
  if (GetPlayingClient(client))
    return client->GetStreamTimes(times);

  return false;
}

bool CPVRClients::IsRealTimeStream(void) const
{
  CPVRClientPtr client;
  if (GetPlayingClient(client))
    return client->IsRealTimeStream();

  return false;
}

bool CPVRClients::SupportsTimers() const
{
  CPVRClientMap clients;
  GetCreatedClients(clients);

  for (const auto &entry : clients)
  {
    if (entry.second->GetClientCapabilities().SupportsTimers())
      return true;
  }

  return false;
}

bool CPVRClients::GetTimers(CPVRTimersContainer *timers, std::vector<int> &failedClients)
{
  CPVRClientMap clients;
  bool bSuccess = GetCreatedClients(clients, failedClients) == PVR_ERROR_NO_ERROR;

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
  PVR_ERROR error = PVR_ERROR_UNKNOWN;

  CPVRClientPtr client;
  if (GetCreatedClient(timer.m_iClientId, client))
    error = client->AddTimer(timer);

  if (error != PVR_ERROR_NO_ERROR)
    CLog::Log(LOGERROR, "PVR - %s - cannot add timer to client '%d': %s",__FUNCTION__, timer.m_iClientId, CPVRClient::ToString(error));

  return error;
}

PVR_ERROR CPVRClients::UpdateTimer(const CPVRTimerInfoTag &timer)
{
  PVR_ERROR error = PVR_ERROR_UNKNOWN;

  CPVRClientPtr client;
  if (GetCreatedClient(timer.m_iClientId, client))
    error = client->UpdateTimer(timer);

  if (error != PVR_ERROR_NO_ERROR)
    CLog::Log(LOGERROR, "PVR - %s - cannot update timer on client '%d': %s",__FUNCTION__, timer.m_iClientId, CPVRClient::ToString(error));

  return error;
}

PVR_ERROR CPVRClients::DeleteTimer(const CPVRTimerInfoTag &timer, bool bForce)
{
  PVR_ERROR error = PVR_ERROR_UNKNOWN;
  CPVRClientPtr client;

  if (GetCreatedClient(timer.m_iClientId, client))
    error = client->DeleteTimer(timer, bForce);

  return error;
}

PVR_ERROR CPVRClients::RenameTimer(const CPVRTimerInfoTag &timer, const std::string &strNewName)
{
  PVR_ERROR error = PVR_ERROR_UNKNOWN;

  CPVRClientPtr client;
  if (GetCreatedClient(timer.m_iClientId, client))
    error = client->RenameTimer(timer, strNewName);

  if (error != PVR_ERROR_NO_ERROR)
    CLog::Log(LOGERROR, "PVR - %s - cannot rename timer on client '%d': %s",__FUNCTION__, timer.m_iClientId, CPVRClient::ToString(error));

  return error;
}

PVR_ERROR CPVRClients::GetTimerTypes(CPVRTimerTypes& results) const
{
  PVR_ERROR error = PVR_ERROR_NO_ERROR;

  CPVRClientMap clients;
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
  PVR_ERROR error = PVR_ERROR_UNKNOWN;

  CPVRClientPtr client;
  if (GetCreatedClient(iClientId, client))
    error = client->GetTimerTypes(results);

  if (error != PVR_ERROR_NO_ERROR)
    CLog::Log(LOGERROR, "PVR - %s - cannot get timer types from client '%d': %s",__FUNCTION__, iClientId, CPVRClient::ToString(error));

  return error;
}

PVR_ERROR CPVRClients::GetRecordings(CPVRRecordings *recordings, bool deleted)
{
  PVR_ERROR error = PVR_ERROR_NO_ERROR;

  CPVRClientMap clients;
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
  PVR_ERROR error = PVR_ERROR_UNKNOWN;

  CPVRClientPtr client;
  if (GetCreatedClient(recording.m_iClientId, client))
    error = client->RenameRecording(recording);

  if (error != PVR_ERROR_NO_ERROR)
    CLog::Log(LOGERROR, "PVR - %s - cannot rename recording on client '%d': %s",__FUNCTION__, recording.m_iClientId, CPVRClient::ToString(error));

  return error;
}

PVR_ERROR CPVRClients::DeleteRecording(const CPVRRecording &recording)
{
  PVR_ERROR error = PVR_ERROR_UNKNOWN;

  CPVRClientPtr client;
  if (GetCreatedClient(recording.m_iClientId, client))
    error = client->DeleteRecording(recording);

  if (error != PVR_ERROR_NO_ERROR)
    CLog::Log(LOGERROR, "PVR - %s - cannot delete recording from client '%d': %s",__FUNCTION__, recording.m_iClientId, CPVRClient::ToString(error));

  return error;
}

PVR_ERROR CPVRClients::UndeleteRecording(const CPVRRecording &recording)
{
  PVR_ERROR error = PVR_ERROR_UNKNOWN;

  if (!recording.IsDeleted())
    return error;

  CPVRClientPtr client;
  if (GetCreatedClient(recording.m_iClientId, client))
    error = client->UndeleteRecording(recording);

  if (error != PVR_ERROR_NO_ERROR)
    CLog::Log(LOGERROR, "PVR - %s - cannot undelete recording from client '%d': %s",__FUNCTION__, recording.m_iClientId, CPVRClient::ToString(error));

  return error;
}

PVR_ERROR CPVRClients::DeleteAllRecordingsFromTrash()
{
  PVR_ERROR error = PVR_ERROR_NO_ERROR;

  CPVRClientMap clients;
  GetCreatedClients(clients);

  for (const auto &client : clients)
  {
    if (client.second->GetClientCapabilities().SupportsRecordingsUndelete() && client.second->GetRecordingsAmount(true) > 0)
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

bool CPVRClients::SetRecordingLifetime(const CPVRRecording &recording, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKNOWN;

  CPVRClientPtr client;
  if (GetCreatedClient(recording.m_iClientId, client) && client->GetClientCapabilities().SupportsRecordingsLifetimeChange())
    *error = client->SetRecordingLifetime(recording);
  else
    CLog::Log(LOGERROR, "PVR - %s - client %d does not support changing recording's lifetime",__FUNCTION__, recording.m_iClientId);

  return *error == PVR_ERROR_NO_ERROR;
}

bool CPVRClients::SetRecordingPlayCount(const CPVRRecording &recording, int count, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKNOWN;

  CPVRClientPtr client;
  if (GetCreatedClient(recording.m_iClientId, client) && client->GetClientCapabilities().SupportsRecordingsPlayCount())
    *error = client->SetRecordingPlayCount(recording, count);
  else
    CLog::Log(LOGERROR, "PVR - %s - client %d does not support setting recording's play count",__FUNCTION__, recording.m_iClientId);

  return *error == PVR_ERROR_NO_ERROR;
}

bool CPVRClients::SetRecordingLastPlayedPosition(const CPVRRecording &recording, int lastplayedposition, PVR_ERROR *error)
{
  *error = PVR_ERROR_UNKNOWN;

  CPVRClientPtr client;
  if (GetCreatedClient(recording.m_iClientId, client) && client->GetClientCapabilities().SupportsRecordings())
    *error = client->SetRecordingLastPlayedPosition(recording, lastplayedposition);
  else
    CLog::Log(LOGERROR, "PVR - %s - client %d does not support recordings",__FUNCTION__, recording.m_iClientId);

  return *error == PVR_ERROR_NO_ERROR;
}

int CPVRClients::GetRecordingLastPlayedPosition(const CPVRRecording &recording)
{
  int iPos = 0;

  CPVRClientPtr client;
  if (GetCreatedClient(recording.m_iClientId, client) && client->GetClientCapabilities().SupportsRecordings())
    iPos = client->GetRecordingLastPlayedPosition(recording);
  else
    CLog::Log(LOGERROR, "PVR - %s - client %d does not support recordings", __FUNCTION__, recording.m_iClientId);

  return iPos;
}

std::vector<PVR_EDL_ENTRY> CPVRClients::GetRecordingEdl(const CPVRRecording &recording)
{
  CPVRClientPtr client;
  if (GetCreatedClient(recording.m_iClientId, client) && client->GetClientCapabilities().SupportsRecordingsEdl())
    return client->GetRecordingEdl(recording);
  else
    CLog::Log(LOGERROR, "PVR - %s - client %d does not support getting Edl", __FUNCTION__, recording.m_iClientId);

  return std::vector<PVR_EDL_ENTRY>();
}

PVR_ERROR CPVRClients::GetEPGForChannel(const CPVRChannelPtr &channel, CPVREpg *epg, time_t start, time_t end)
{
  PVR_ERROR error = PVR_ERROR_UNKNOWN;

  CPVRClientPtr client;
  if (GetCreatedClient(channel->ClientID(), client))
    error = client->GetEPGForChannel(channel, epg, start, end);

  if (error != PVR_ERROR_NO_ERROR)
    CLog::Log(LOGERROR, "PVR - %s - cannot get EPG for channel '%s' from client '%d': %s",__FUNCTION__, channel->ChannelName().c_str(), channel->ClientID(), CPVRClient::ToString(error));

  return error;
}

PVR_ERROR CPVRClients::SetEPGTimeFrame(int iDays)
{
  PVR_ERROR error = PVR_ERROR_NO_ERROR;

  CPVRClientMap clients;
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

PVR_ERROR CPVRClients::IsRecordable(const CConstPVREpgInfoTagPtr& tag, bool &bIsRecordable) const
{
  PVR_ERROR error = PVR_ERROR_UNKNOWN;

  CPVRClientPtr client;
  if (GetCreatedClient(tag->ClientID(), client))
    error = client->IsRecordable(tag, bIsRecordable);

  if (error != PVR_ERROR_NO_ERROR && error != PVR_ERROR_NOT_IMPLEMENTED)
    CLog::Log(LOGERROR, "PVR - %s - unable to obtain 'isRecordable' flag from client '%d': %s", __FUNCTION__, tag->ClientID(), CPVRClient::ToString(error));

  return error;
}

PVR_ERROR CPVRClients::IsPlayable(const CConstPVREpgInfoTagPtr& tag, bool &bIsPlayable) const
{
  PVR_ERROR error(PVR_ERROR_UNKNOWN);

  CPVRClientPtr client;
  if (GetCreatedClient(tag->ClientID(), client))
    error = client->IsPlayable(tag, bIsPlayable);

  if (error != PVR_ERROR_NO_ERROR && error != PVR_ERROR_NOT_IMPLEMENTED)
    CLog::Log(LOGERROR, "PVR - %s - unable to obtain 'isPlayable' flag from client '%d': %s", __FUNCTION__, tag->ClientID(), CPVRClient::ToString(error));

  return error;
}

PVR_ERROR CPVRClients::GetChannels(CPVRChannelGroupInternal *group, std::vector<int> &failedClients)
{
  CPVRClientMap clients;
  PVR_ERROR error = GetCreatedClients(clients, failedClients);

  /* get the channel list from each client */
  for (const auto &client : clients)
  {
    PVR_ERROR currentError = client.second->GetChannels(*group, group->IsRadio());
    if (currentError != PVR_ERROR_NOT_IMPLEMENTED &&
        currentError != PVR_ERROR_NO_ERROR)
    {
      error = currentError;
      CLog::Log(LOGERROR, "PVR - %s - cannot get channels from client '%d': %s",__FUNCTION__, client.first, CPVRClient::ToString(error));
      failedClients.emplace_back(client.first);
    }
  }

  return error;
}

PVR_ERROR CPVRClients::GetChannelGroups(CPVRChannelGroups *groups, std::vector<int> &failedClients)
{
  PVR_ERROR error = PVR_ERROR_NO_ERROR;

  CPVRClientMap clients;
  GetCreatedClients(clients, failedClients);

  for (const auto &client : clients)
  {
    PVR_ERROR currentError = client.second->GetChannelGroups(groups);
    if (currentError != PVR_ERROR_NOT_IMPLEMENTED &&
        currentError != PVR_ERROR_NO_ERROR)
    {
      error = currentError;
      CLog::Log(LOGERROR, "PVR - %s - cannot get groups from client '%d': %s",__FUNCTION__, client.first, CPVRClient::ToString(error));
      failedClients.emplace_back(client.first);
    }
  }

  return error;
}

PVR_ERROR CPVRClients::GetChannelGroupMembers(CPVRChannelGroup *group, std::vector<int> &failedClients)
{
  CPVRClientMap clients;
  PVR_ERROR error = GetCreatedClients(clients, failedClients);

  /* get the member list from each client */
  for (const auto &client : clients)
  {
    PVR_ERROR currentError = client.second->GetChannelGroupMembers(group);
    if (currentError != PVR_ERROR_NOT_IMPLEMENTED &&
        currentError != PVR_ERROR_NO_ERROR)
    {
      error = currentError;
      CLog::Log(LOGERROR, "PVR - %s - cannot get group members from client '%d': %s",__FUNCTION__, client.first, CPVRClient::ToString(error));
      failedClients.emplace_back(client.first);
    }
  }

  return error;
}

PVR_ERROR CPVRClients::DeleteChannel(const CPVRChannelPtr &channel)
{
  PVR_ERROR error = PVR_ERROR_UNKNOWN;

  CPVRClientPtr client;
  if (GetCreatedClient(channel->ClientID(), client))
    error = client->DeleteChannel(channel);
  else
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, channel->ClientID());

  return error;
}

bool CPVRClients::RenameChannel(const CPVRChannelPtr &channel)
{
  PVR_ERROR error = PVR_ERROR_UNKNOWN;

  CPVRClientPtr client;
  if (GetCreatedClient(channel->ClientID(), client))
    error = client->RenameChannel(channel);
  else
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, channel->ClientID());

  return (error == PVR_ERROR_NO_ERROR || error == PVR_ERROR_NOT_IMPLEMENTED);
}

std::vector<CPVRClientPtr> CPVRClients::GetClientsSupportingChannelScan(void) const
{
  std::vector<CPVRClientPtr> possibleScanClients;

  CPVRClientMap clients;
  GetCreatedClients(clients);

  /* get clients that support channel scanning */
  for (const auto &client : clients)
  {
    if (client.second->GetClientCapabilities().SupportsChannelScan())
      possibleScanClients.emplace_back(client.second);
  }

  return possibleScanClients;
}

std::vector<CPVRClientPtr> CPVRClients::GetClientsSupportingChannelSettings(bool bRadio) const
{
  std::vector<CPVRClientPtr> possibleSettingsClients;

  CPVRClientMap clients;
  GetCreatedClients(clients);

  /* get clients that support channel settings */
  for (const auto &client : clients)
  {
    if (client.second->GetClientCapabilities().SupportsChannelSettings() &&
        ((bRadio && client.second->GetClientCapabilities().SupportsRadio()) || (!bRadio && client.second->GetClientCapabilities().SupportsTV())))
      possibleSettingsClients.push_back(client.second);
  }

  return possibleSettingsClients;
}

PVR_ERROR CPVRClients::OpenDialogChannelAdd(const CPVRChannelPtr &channel)
{
  PVR_ERROR error = PVR_ERROR_UNKNOWN;

  CPVRClientPtr client;
  if (GetCreatedClient(channel->ClientID(), client))
    error = client->OpenDialogChannelAdd(channel);
  else
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, channel->ClientID());

  return error;
}

PVR_ERROR CPVRClients::OpenDialogChannelSettings(const CPVRChannelPtr &channel)
{
  PVR_ERROR error = PVR_ERROR_UNKNOWN;

  CPVRClientPtr client;
  if (GetCreatedClient(channel->ClientID(), client))
    error = client->OpenDialogChannelSettings(channel);
  else
    CLog::Log(LOGERROR, "PVR - %s - cannot find client %d",__FUNCTION__, channel->ClientID());

  return error;
}

bool CPVRClients::HasMenuHooks(int iClientID, PVR_MENUHOOK_CAT cat)
{
  if (iClientID < 0)
    iClientID = GetPlayingClientID();

  CPVRClientPtr client;
  return (GetCreatedClient(iClientID, client) && client->HasMenuHooks(cat));
}

void CPVRClients::OnSystemSleep()
{
  CPVRClientMap clients;
  GetCreatedClients(clients);

  for (const auto &client : clients)
  {
    client.second->OnSystemSleep();
  }
}

void CPVRClients::OnSystemWake()
{
  CPVRClientMap clients;
  GetCreatedClients(clients);

  for (const auto &client : clients)
  {
    client.second->OnSystemWake();
  }
}

void CPVRClients::OnPowerSavingActivated()
{
  CPVRClientMap clients;
  GetCreatedClients(clients);

  for (const auto &client : clients)
  {
    client.second->OnPowerSavingActivated();
  }
}

void CPVRClients::OnPowerSavingDeactivated()
{
  CPVRClientMap clients;
  GetCreatedClients(clients);

  for (const auto &client : clients)
  {
    client.second->OnPowerSavingDeactivated();
  }
}

void CPVRClients::ConnectionStateChange(
  CPVRClient *client, std::string &strConnectionString, PVR_CONNECTION_STATE newState, std::string &strMessage)
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

  int iMsg = -1;
  bool bError = true;
  bool bNotify = true;

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
