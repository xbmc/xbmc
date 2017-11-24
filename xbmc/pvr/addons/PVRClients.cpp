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

  int iId = GetClientId(addon->ID());
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
  return GetClientId(client->ID()) > 0;
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
  return ForCreatedClient(__FUNCTION__, iClientId, [&strName](const CPVRClientPtr &client) {
    strName = client->GetFriendlyName();
    return PVR_ERROR_NO_ERROR;
  }) == PVR_ERROR_NO_ERROR;
}

bool CPVRClients::GetClientAddonName(int iClientId, std::string &strName) const
{
  return ForCreatedClient(__FUNCTION__, iClientId, [&strName](const CPVRClientPtr &client) {
    strName = client->Name();
    return PVR_ERROR_NO_ERROR;
  }) == PVR_ERROR_NO_ERROR;
}

bool CPVRClients::GetClientAddonIcon(int iClientId, std::string &strIcon) const
{
  return ForCreatedClient(__FUNCTION__, iClientId, [&strIcon](const CPVRClientPtr &client) {
    strIcon = client->Icon();
    return PVR_ERROR_NO_ERROR;
  }) == PVR_ERROR_NO_ERROR;
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
  bool bReturn = false;
  ForPlayingClient(__FUNCTION__, [&bReturn](const CPVRClientPtr &client) {
    bReturn = client->IsPlayingLiveRadio();
    return PVR_ERROR_NO_ERROR;
  });
  return bReturn;
}

bool CPVRClients::IsPlayingTV(void) const
{
  bool bReturn = false;
  ForPlayingClient(__FUNCTION__, [&bReturn](const CPVRClientPtr &client) {
    bReturn = client->IsPlayingLiveTV();
    return PVR_ERROR_NO_ERROR;
  });
  return bReturn;
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
  bool bReturn = false;
  ForPlayingClient(__FUNCTION__, [&bReturn](const CPVRClientPtr &client) {
    bReturn = client->IsPlayingEncryptedChannel();
    return PVR_ERROR_NO_ERROR;
  });
  return bReturn;
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
  ForPlayingClient(__FUNCTION__, [](const CPVRClientPtr &client) {
    client->ClearPlayingChannel();
    return PVR_ERROR_NO_ERROR;
  });

  CSingleLock lock(m_critSection);
  m_bIsPlayingLiveTV = false;
  m_playingClientId = PVR_INVALID_CLIENT_ID;
  m_strPlayingClientName.clear();
}

CPVRChannelPtr CPVRClients::GetPlayingChannel() const
{
  CPVRChannelPtr channel;
  ForPlayingClient(__FUNCTION__, [&channel](const CPVRClientPtr &client) {
    channel = client->GetPlayingChannel();
    return PVR_ERROR_NO_ERROR;
  });
  return channel;
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
  ForPlayingClient(__FUNCTION__, [](const CPVRClientPtr &client) {
    client->ClearPlayingRecording();
    return PVR_ERROR_NO_ERROR;
  });

  CSingleLock lock(m_critSection);
  m_bIsPlayingRecording = false;
  m_playingClientId = PVR_INVALID_CLIENT_ID;
  m_strPlayingClientName.clear();
}

CPVRRecordingPtr CPVRClients::GetPlayingRecording() const
{
  CPVRRecordingPtr recording;
  ForPlayingClient(__FUNCTION__, [&recording](const CPVRClientPtr &client) {
    recording = client->GetPlayingRecording();
    return PVR_ERROR_NO_ERROR;
  });
  return recording;
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
  ForPlayingClient(__FUNCTION__, [](const CPVRClientPtr &client) {
    client->ClearPlayingEpgTag();
    return PVR_ERROR_NO_ERROR;
  });

  CSingleLock lock(m_critSection);
  m_bIsPlayingEpgTag = false;
  m_playingClientId = PVR_INVALID_CLIENT_ID;
  m_strPlayingClientName.clear();
}

CPVREpgInfoTagPtr CPVRClients::GetPlayingEpgTag() const
{
  CPVREpgInfoTagPtr tag;
  ForPlayingClient(__FUNCTION__, [&tag](const CPVRClientPtr &client) {
    tag = client->GetPlayingEpgTag();
    return PVR_ERROR_NO_ERROR;
  });
  return tag;
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

CPVRClientCapabilities CPVRClients::GetClientCapabilities(int iClientId) const
{
  CPVRClientCapabilities caps;
  ForCreatedClient(__FUNCTION__, iClientId, [&caps](const CPVRClientPtr &client) {
    caps = client->GetClientCapabilities();
    return PVR_ERROR_NO_ERROR;
  });
  return caps;
}

std::vector<SBackend> CPVRClients::GetBackendProperties() const
{
  std::vector<SBackend> backendProperties;

  ForCreatedClients(__FUNCTION__, [&backendProperties](const CPVRClientPtr &client) {
    SBackend properties;

    if (client->GetDriveSpace(properties.diskTotal, properties.diskUsed) == PVR_ERROR_NO_ERROR)
    {
      properties.diskTotal *= 1024;
      properties.diskUsed *= 1024;
    }

    int iAmount = 0;
    if (client->GetChannelsAmount(iAmount) == PVR_ERROR_NO_ERROR)
      properties.numChannels = iAmount;
    if (client->GetTimersAmount(iAmount) == PVR_ERROR_NO_ERROR)
      properties.numTimers = iAmount;
    if (client->GetRecordingsAmount(false, iAmount) == PVR_ERROR_NO_ERROR)
      properties.numRecordings = iAmount;
    if (client->GetRecordingsAmount(true, iAmount) == PVR_ERROR_NO_ERROR)
      properties.numDeletedRecordings = iAmount;
    properties.name = client->GetBackendName();
    properties.version = client->GetBackendVersion();
    properties.host = client->GetConnectionString();

    backendProperties.emplace_back(properties);
    return PVR_ERROR_NO_ERROR;
  });

  return backendProperties;
}

std::string CPVRClients::GetBackendHostnameByClientId(int iClientId) const
{
  std::string name;
  ForCreatedClient(__FUNCTION__, iClientId, [&name](const CPVRClientPtr &client) {
    name = client->GetBackendHostname();
    return PVR_ERROR_NO_ERROR;
  });
  return name;
}

bool CPVRClients::OpenStream(const CPVRChannelPtr &channel)
{
  CloseStream();

  /* try to open the stream on the client */
  bool bReturn = ForCreatedClient(__FUNCTION__, channel->ClientID(), [&channel](const CPVRClientPtr &client) {
    return client->OpenStream(channel);
  }) == PVR_ERROR_NO_ERROR;

  if (bReturn)
    SetPlayingChannel(channel);

  return bReturn;
}

bool CPVRClients::OpenStream(const CPVRRecordingPtr &recording)
{
  CloseStream();

  /* try to open the recording stream on the client */
  bool bReturn = ForCreatedClient(__FUNCTION__, recording->ClientID(), [&recording](const CPVRClientPtr &client) {
    return client->OpenStream(recording);
  }) == PVR_ERROR_NO_ERROR;

  if (bReturn)
    SetPlayingRecording(recording);

  return bReturn;
}

void CPVRClients::CloseStream(void)
{
  ForPlayingClient(__FUNCTION__, [](const CPVRClientPtr &client) {
    return client->CloseStream();
  });
}

int CPVRClients::ReadStream(void* lpBuf, int64_t uiBufSize)
{
  int iRead = -EINVAL;
  ForPlayingClient(__FUNCTION__, [&lpBuf, uiBufSize, &iRead](const CPVRClientPtr &client) {
    return client->ReadStream(lpBuf, uiBufSize, iRead);
  });
  return iRead;
}

int64_t CPVRClients::GetStreamLength(void)
{
  int64_t iLength = -EINVAL;
  ForPlayingClient(__FUNCTION__, [&iLength](const CPVRClientPtr &client) {
    return client->GetStreamLength(iLength);
  });
  return iLength;
}

bool CPVRClients::CanSeekStream(void) const
{
  bool bCanSeek = m_bIsPlayingRecording;
  if (!bCanSeek)
  {
    ForPlayingClient(__FUNCTION__, [&bCanSeek](const CPVRClientPtr &client) {
      return client->CanSeekStream(bCanSeek);
    });
  }
  return bCanSeek;
}

int64_t CPVRClients::SeekStream(int64_t iFilePosition, int iWhence/* = SEEK_SET*/)
{
  int64_t iPos = -EINVAL;
  ForPlayingClient(__FUNCTION__, [iFilePosition, iWhence, &iPos](const CPVRClientPtr &client) {
    return client->SeekStream(iFilePosition, iWhence, iPos);
  });
  return iPos;
}

bool CPVRClients::CanPauseStream(void) const
{
  bool bCanPause = m_bIsPlayingRecording;
  if (!bCanPause)
  {
    ForPlayingClient(__FUNCTION__, [&bCanPause](const CPVRClientPtr &client) {
      return client->CanPauseStream(bCanPause);
    });
  }
  return bCanPause;
}

void CPVRClients::PauseStream(bool bPaused)
{
  ForPlayingClient(__FUNCTION__, [bPaused](const CPVRClientPtr &client) {
    return client->PauseStream(bPaused);
  });
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
  bool bReturn = ForCreatedClient(__FUNCTION__, fileItem.GetPVRChannelInfoTag()->ClientID(), [&fileItem, &bReturn](const CPVRClientPtr &client) {
    bReturn = client->FillChannelStreamFileItem(fileItem);
    return PVR_ERROR_NO_ERROR;
  }) == PVR_ERROR_NO_ERROR;
  return bReturn;
}

bool CPVRClients::FillRecordingStreamFileItem(CFileItem &fileItem)
{
  bool bReturn = ForCreatedClient(__FUNCTION__, fileItem.GetPVRRecordingInfoTag()->ClientID(), [&fileItem, &bReturn](const CPVRClientPtr &client) {
    bReturn = client->FillRecordingStreamFileItem(fileItem);
    return PVR_ERROR_NO_ERROR;
  }) == PVR_ERROR_NO_ERROR;
  return bReturn;
}

bool CPVRClients::FillEpgTagStreamFileItem(CFileItem &fileItem)
{
  bool bReturn = ForCreatedClient(__FUNCTION__, fileItem.GetEPGInfoTag()->ClientID(), [&fileItem, &bReturn](const CPVRClientPtr &client) {
    bReturn = client->FillEpgTagStreamFileItem(fileItem);
    return PVR_ERROR_NO_ERROR;
  }) == PVR_ERROR_NO_ERROR;
  return bReturn;
}

bool CPVRClients::IsTimeshifting(void) const
{
  bool bTimeshifting = false;
  ForPlayingClient(__FUNCTION__, [&bTimeshifting](const CPVRClientPtr &client) {
    return client->IsTimeshifting(bTimeshifting);
  });
  return bTimeshifting;
}

time_t CPVRClients::GetPlayingTime() const
{
  time_t time(0);
  ForPlayingClient(__FUNCTION__, [&time](const CPVRClientPtr &client) {
    return client->GetPlayingTime(time);
  });
  return time;
}

time_t CPVRClients::GetBufferTimeStart() const
{
  time_t time(0);
  ForPlayingClient(__FUNCTION__, [&time](const CPVRClientPtr &client) {
    return client->GetBufferTimeStart(time);
  });
  return time;
}

time_t CPVRClients::GetBufferTimeEnd() const
{
  time_t time(0);
  ForPlayingClient(__FUNCTION__, [&time](const CPVRClientPtr &client) {
    return client->GetBufferTimeEnd(time);
  });
  return time;
}

bool CPVRClients::GetStreamTimes(PVR_STREAM_TIMES *times) const
{
  return ForPlayingClient(__FUNCTION__, [&times](const CPVRClientPtr &client) {
    return client->GetStreamTimes(times);
  }) == PVR_ERROR_NO_ERROR;
}

bool CPVRClients::IsRealTimeStream(void) const
{
  bool bRealTime = false;
  ForPlayingClient(__FUNCTION__, [&bRealTime](const CPVRClientPtr &client) {
    return client->IsRealTimeStream(bRealTime);
  });
  return bRealTime;
}

bool CPVRClients::SupportsTimers() const
{
  bool bReturn = false;
  ForCreatedClients(__FUNCTION__, [&bReturn](const CPVRClientPtr &client) {
    if (!bReturn)
      bReturn = client->GetClientCapabilities().SupportsTimers();
    return PVR_ERROR_NO_ERROR;
  });
  return bReturn;
}

bool CPVRClients::GetTimers(CPVRTimersContainer *timers, std::vector<int> &failedClients)
{
  return ForCreatedClients(__FUNCTION__, [timers](const CPVRClientPtr &client) {
    return client->GetTimers(timers);
  }, failedClients) == PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVRClients::AddTimer(const CPVRTimerInfoTag &timer)
{
  return ForCreatedClient(__FUNCTION__, timer.m_iClientId, [&timer](const CPVRClientPtr &client) {
    return client->AddTimer(timer);
  });
}

PVR_ERROR CPVRClients::UpdateTimer(const CPVRTimerInfoTag &timer)
{
  return ForCreatedClient(__FUNCTION__, timer.m_iClientId, [&timer](const CPVRClientPtr &client) {
    return client->UpdateTimer(timer);
  });
}

PVR_ERROR CPVRClients::DeleteTimer(const CPVRTimerInfoTag &timer, bool bForce)
{
  return ForCreatedClient(__FUNCTION__, timer.m_iClientId, [&timer, bForce](const CPVRClientPtr &client) {
    return client->DeleteTimer(timer, bForce);
  });
}

PVR_ERROR CPVRClients::GetTimerTypes(CPVRTimerTypes& results) const
{
  return ForCreatedClients(__FUNCTION__, [&results](const CPVRClientPtr &client) {
    return client->GetTimerTypes(results);
  });
}

PVR_ERROR CPVRClients::GetTimerTypes(CPVRTimerTypes& results, int iClientId) const
{
  return ForCreatedClient(__FUNCTION__, iClientId, [&results](const CPVRClientPtr &client) {
    return client->GetTimerTypes(results);
  });
}

PVR_ERROR CPVRClients::GetRecordings(CPVRRecordings *recordings, bool deleted)
{
  return ForCreatedClients(__FUNCTION__, [recordings, deleted](const CPVRClientPtr &client) {
    return client->GetRecordings(recordings, deleted);
  });
}

PVR_ERROR CPVRClients::RenameRecording(const CPVRRecording &recording)
{
  return ForCreatedClient(__FUNCTION__, recording.ClientID(), [&recording](const CPVRClientPtr &client) {
    return client->RenameRecording(recording);
  });
}

PVR_ERROR CPVRClients::DeleteRecording(const CPVRRecording &recording)
{
  return ForCreatedClient(__FUNCTION__, recording.ClientID(), [&recording](const CPVRClientPtr &client) {
    return client->DeleteRecording(recording);
  });
}

PVR_ERROR CPVRClients::UndeleteRecording(const CPVRRecording &recording)
{
  if (!recording.IsDeleted())
    return PVR_ERROR_REJECTED;

  return ForCreatedClient(__FUNCTION__, recording.ClientID(), [&recording](const CPVRClientPtr &client) {
    return client->UndeleteRecording(recording);
  });
}

PVR_ERROR CPVRClients::DeleteAllRecordingsFromTrash()
{
  return ForCreatedClients(__FUNCTION__, [](const CPVRClientPtr &client) {
    return client->DeleteAllRecordingsFromTrash();
  });
}

bool CPVRClients::SetRecordingLifetime(const CPVRRecording &recording, PVR_ERROR *error)
{
  *error = ForCreatedClient(__FUNCTION__, recording.ClientID(), [&recording](const CPVRClientPtr &client) {
    return client->SetRecordingLifetime(recording);
  });
  return *error == PVR_ERROR_NO_ERROR;
}

bool CPVRClients::SetRecordingPlayCount(const CPVRRecording &recording, int count, PVR_ERROR *error)
{
  *error = ForCreatedClient(__FUNCTION__, recording.ClientID(), [&recording, count](const CPVRClientPtr &client) {
    return client->SetRecordingPlayCount(recording, count);
  });
  return *error == PVR_ERROR_NO_ERROR;
}

bool CPVRClients::SetRecordingLastPlayedPosition(const CPVRRecording &recording, int lastplayedposition, PVR_ERROR *error)
{
  *error = ForCreatedClient(__FUNCTION__, recording.ClientID(), [&recording, lastplayedposition](const CPVRClientPtr &client) {
    return client->SetRecordingLastPlayedPosition(recording, lastplayedposition);
  });
  return *error == PVR_ERROR_NO_ERROR;
}

int CPVRClients::GetRecordingLastPlayedPosition(const CPVRRecording &recording)
{
  int iPos = 0;
  ForCreatedClient(__FUNCTION__, recording.ClientID(), [&recording, &iPos](const CPVRClientPtr &client) {
    return client->GetRecordingLastPlayedPosition(recording, iPos);
  });
  return iPos;
}

std::vector<PVR_EDL_ENTRY> CPVRClients::GetRecordingEdl(const CPVRRecording &recording)
{
  std::vector<PVR_EDL_ENTRY> edls;
  ForCreatedClient(__FUNCTION__, recording.ClientID(), [&recording, &edls](const CPVRClientPtr &client) {
    return client->GetRecordingEdl(recording, edls);
  });
  return edls;
}

PVR_ERROR CPVRClients::GetEPGForChannel(const CPVRChannelPtr &channel, CPVREpg *epg, time_t start, time_t end)
{
  return ForCreatedClient(__FUNCTION__, channel->ClientID(), [&channel, epg, start, end](const CPVRClientPtr &client) {
    return client->GetEPGForChannel(channel, epg, start, end);
  });
}

PVR_ERROR CPVRClients::SetEPGTimeFrame(int iDays)
{
  return ForCreatedClients(__FUNCTION__, [iDays](const CPVRClientPtr &client) {
    return client->SetEPGTimeFrame(iDays);
  });
}

PVR_ERROR CPVRClients::IsRecordable(const CConstPVREpgInfoTagPtr& tag, bool &bIsRecordable) const
{
  return ForCreatedClient(__FUNCTION__, tag->ClientID(), [&tag, &bIsRecordable](const CPVRClientPtr &client) {
    return client->IsRecordable(tag, bIsRecordable);
  });
}

PVR_ERROR CPVRClients::IsPlayable(const CConstPVREpgInfoTagPtr& tag, bool &bIsPlayable) const
{
  return ForCreatedClient(__FUNCTION__, tag->ClientID(), [&tag, &bIsPlayable](const CPVRClientPtr &client) {
    return client->IsPlayable(tag, bIsPlayable);
  });
}

PVR_ERROR CPVRClients::GetChannels(CPVRChannelGroupInternal *group, std::vector<int> &failedClients)
{
  return ForCreatedClients(__FUNCTION__, [group](const CPVRClientPtr &client) {
    return client->GetChannels(*group, group->IsRadio());
  }, failedClients);
}

PVR_ERROR CPVRClients::GetChannelGroups(CPVRChannelGroups *groups, std::vector<int> &failedClients)
{
  return ForCreatedClients(__FUNCTION__, [groups](const CPVRClientPtr &client) {
    return client->GetChannelGroups(groups);
  }, failedClients);
}

PVR_ERROR CPVRClients::GetChannelGroupMembers(CPVRChannelGroup *group, std::vector<int> &failedClients)
{
  return ForCreatedClients(__FUNCTION__, [group](const CPVRClientPtr &client) {
    return client->GetChannelGroupMembers(group);
  }, failedClients);
}

PVR_ERROR CPVRClients::DeleteChannel(const CPVRChannelPtr &channel)
{
  return ForCreatedClient(__FUNCTION__, channel->ClientID(), [&channel](const CPVRClientPtr &client) {
    return client->DeleteChannel(channel);
  });
}

bool CPVRClients::RenameChannel(const CPVRChannelPtr &channel)
{
  return ForCreatedClient(__FUNCTION__, channel->ClientID(), [&channel](const CPVRClientPtr &client) {
    return client->RenameChannel(channel);
  });
}

std::vector<CPVRClientPtr> CPVRClients::GetClientsSupportingChannelScan(void) const
{
  std::vector<CPVRClientPtr> possibleScanClients;
  ForCreatedClients(__FUNCTION__, [&possibleScanClients](const CPVRClientPtr &client) {
    if (client->GetClientCapabilities().SupportsChannelScan())
      possibleScanClients.emplace_back(client);
    return PVR_ERROR_NO_ERROR;
  });
  return possibleScanClients;
}

std::vector<CPVRClientPtr> CPVRClients::GetClientsSupportingChannelSettings(bool bRadio) const
{
  std::vector<CPVRClientPtr> possibleSettingsClients;
  ForCreatedClients(__FUNCTION__, [bRadio, &possibleSettingsClients](const CPVRClientPtr &client) {
    const CPVRClientCapabilities& caps = client->GetClientCapabilities();
    if (caps.SupportsChannelSettings() &&
        ((bRadio && caps.SupportsRadio()) || (!bRadio && caps.SupportsTV())))
      possibleSettingsClients.emplace_back(client);
    return PVR_ERROR_NO_ERROR;
  });
  return possibleSettingsClients;
}

PVR_ERROR CPVRClients::OpenDialogChannelAdd(const CPVRChannelPtr &channel)
{
  return ForCreatedClient(__FUNCTION__, channel->ClientID(), [&channel](const CPVRClientPtr &client) {
    return client->OpenDialogChannelAdd(channel);
  });
}

PVR_ERROR CPVRClients::OpenDialogChannelSettings(const CPVRChannelPtr &channel)
{
  return ForCreatedClient(__FUNCTION__, channel->ClientID(), [&channel](const CPVRClientPtr &client) {
    return client->OpenDialogChannelSettings(channel);
  });
}

bool CPVRClients::HasMenuHooks(int iClientID, PVR_MENUHOOK_CAT cat)
{
  if (iClientID < 0)
    iClientID = GetPlayingClientID();

  bool bHasMenuHooks = false;
  ForCreatedClient(__FUNCTION__, iClientID, [cat, &bHasMenuHooks](const CPVRClientPtr &client) {
    bHasMenuHooks = client->HasMenuHooks(cat);
    return PVR_ERROR_NO_ERROR;
  });
  return bHasMenuHooks;
}

void CPVRClients::OnSystemSleep()
{
  ForCreatedClients(__FUNCTION__, [](const CPVRClientPtr &client) {
    client->OnSystemSleep();
    return PVR_ERROR_NO_ERROR;
  });
}

void CPVRClients::OnSystemWake()
{
  ForCreatedClients(__FUNCTION__, [](const CPVRClientPtr &client) {
    client->OnSystemWake();
    return PVR_ERROR_NO_ERROR;
  });
}

void CPVRClients::OnPowerSavingActivated()
{
  ForCreatedClients(__FUNCTION__, [](const CPVRClientPtr &client) {
    client->OnPowerSavingActivated();
    return PVR_ERROR_NO_ERROR;
  });
}

void CPVRClients::OnPowerSavingDeactivated()
{
  ForCreatedClients(__FUNCTION__, [](const CPVRClientPtr &client) {
    client->OnPowerSavingDeactivated();
    return PVR_ERROR_NO_ERROR;
  });
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

PVR_ERROR CPVRClients::ForCreatedClients(const char* strFunctionName, PVRClientFunction function) const
{
  std::vector<int> failedClients;
  return ForCreatedClients(strFunctionName, function, failedClients);
}

PVR_ERROR CPVRClients::ForCreatedClients(const char* strFunctionName, PVRClientFunction function, std::vector<int> &failedClients) const
{
  PVR_ERROR lastError = PVR_ERROR_NO_ERROR;

  CPVRClientMap clients;
  GetCreatedClients(clients, failedClients);

  for (const auto &clientEntry : clients)
  {
    PVR_ERROR currentError = function(clientEntry.second);

    if (currentError != PVR_ERROR_NO_ERROR && currentError != PVR_ERROR_NOT_IMPLEMENTED)
    {
      CLog::Log(LOGERROR,
                "CPVRClients - %s - client '%s' returned an error: %s",
                strFunctionName, clientEntry.second->GetFriendlyName().c_str(), CPVRClient::ToString(currentError));
      lastError = currentError;
      failedClients.emplace_back(clientEntry.first);
    }
  }
  return lastError;
}

PVR_ERROR CPVRClients::ForCreatedClient(const char* strFunctionName, int iClientId, PVRClientFunction function) const
{
  PVR_ERROR error = PVR_ERROR_UNKNOWN;
  CPVRClientPtr client;
  if (GetCreatedClient(iClientId, client))
  {
    error = function(client);

    if (error != PVR_ERROR_NO_ERROR && error != PVR_ERROR_NOT_IMPLEMENTED)
      CLog::Log(LOGERROR, "CPVRClients - %s - client '%s' returned an error: %s",
                strFunctionName, client->GetFriendlyName().c_str(), CPVRClient::ToString(error));
  }
  else
  {
    CLog::Log(LOGERROR, "CPVRClients - %s - no created client with id '%d'", strFunctionName, iClientId);
  }
  return error;
}

PVR_ERROR CPVRClients::ForPlayingClient(const char* strFunctionName, PVRClientFunction function) const
{
  PVR_ERROR error = PVR_ERROR_UNKNOWN;

  if (!IsPlaying())
    return PVR_ERROR_REJECTED;

  CPVRClientPtr client;
  if (GetPlayingClient(client))
  {
    error = function(client);

    if (error != PVR_ERROR_NO_ERROR && error != PVR_ERROR_NOT_IMPLEMENTED)
      CLog::Log(LOGERROR, "CPVRClients - %s - playing client '%s' returned an error: %s",
                strFunctionName, client->GetFriendlyName().c_str(), CPVRClient::ToString(error));
  }
  return error;
}
