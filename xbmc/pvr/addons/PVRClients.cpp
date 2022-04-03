/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRClients.h"

#include "ServiceBroker.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/ApplicationMessenger.h"
#include "pvr/PVREventLogJob.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRPlaybackState.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/channels/PVRChannelGroupInternal.h"
#include "pvr/guilib/PVRGUIProgressHandler.h"
#include "utils/JobManager.h"
#include "utils/log.h"

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

using namespace ADDON;
using namespace PVR;

namespace
{
  int ClientIdFromAddonId(const std::string& strID)
  {
    std::hash<std::string> hasher;
    int iClientId = static_cast<int>(hasher(strID));
    if (iClientId < 0)
      iClientId = -iClientId;
    return iClientId;
  }

} // unnamed namespace

CPVRClients::CPVRClients()
{
  CServiceBroker::GetAddonMgr().RegisterAddonMgrCallback(ADDON_PVRDLL, this);
  CServiceBroker::GetAddonMgr().Events().Subscribe(this, &CPVRClients::OnAddonEvent);
}

CPVRClients::~CPVRClients()
{
  CServiceBroker::GetAddonMgr().Events().Unsubscribe(this);
  CServiceBroker::GetAddonMgr().UnregisterAddonMgrCallback(ADDON_PVRDLL);

  for (const auto& client : m_clientMap)
  {
    client.second->Destroy();
  }
}

void CPVRClients::Start()
{
  UpdateAddons();
}

void CPVRClients::Stop()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& client : m_clientMap)
  {
    client.second->Stop();
  }
}

void CPVRClients::Continue()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& client : m_clientMap)
  {
    client.second->Continue();
  }
}

void CPVRClients::UpdateAddons(const std::string& changedAddonId /*= ""*/)
{
  std::vector<AddonInfoPtr> addons;
  CServiceBroker::GetAddonMgr().GetAddonInfos(addons, false, ADDON_PVRDLL);

  if (addons.empty())
    return;

  bool bFoundChangedAddon = changedAddonId.empty();
  std::vector<std::pair<AddonInfoPtr, bool>> addonsWithStatus;
  for (const auto& addon : addons)
  {
    bool bEnabled = !CServiceBroker::GetAddonMgr().IsAddonDisabled(addon->ID());
    addonsWithStatus.emplace_back(std::make_pair(addon, bEnabled));

    if (!bFoundChangedAddon && addon->ID() == changedAddonId)
      bFoundChangedAddon = true;
  }

  if (!bFoundChangedAddon)
    return; // changed addon is not a known pvr client addon, so nothing to update

  addons.clear();

  std::vector<std::pair<std::shared_ptr<CPVRClient>, int>> addonsToCreate;
  std::vector<AddonInfoPtr> addonsToReCreate;
  std::vector<AddonInfoPtr> addonsToDestroy;

  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    for (const auto& addonWithStatus : addonsWithStatus)
    {
      AddonInfoPtr addon = addonWithStatus.first;
      bool bEnabled = addonWithStatus.second;

      if (bEnabled && (!IsKnownClient(addon->ID()) || !IsCreatedClient(addon->ID())))
      {
        int iClientId = ClientIdFromAddonId(addon->ID());

        std::shared_ptr<CPVRClient> client;
        if (IsKnownClient(addon->ID()))
        {
          GetClient(iClientId, client);
        }
        else
        {
          client = std::make_shared<CPVRClient>(addon);
          if (!client)
          {
            CLog::LogF(LOGERROR, "Severe error, incorrect add-on type");
            continue;
          }
        }
        addonsToCreate.emplace_back(std::make_pair(client, iClientId));
      }
      else if (IsCreatedClient(addon->ID()))
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

    auto progressHandler = std::make_unique<CPVRGUIProgressHandler>(
        g_localizeStrings.Get(19239)); // Creating PVR clients

    unsigned int i = 0;
    for (const auto& addon : addonsToCreate)
    {
      progressHandler->UpdateProgress(addon.first->Name(), i++,
                                      addonsToCreate.size() + addonsToReCreate.size());

      ADDON_STATUS status = addon.first->Create(addon.second);

      if (status != ADDON_STATUS_OK)
      {
        CLog::LogF(LOGERROR, "Failed to create add-on {}, status = {}", addon.first->ID(), status);
        if (status == ADDON_STATUS_PERMANENT_FAILURE)
        {
          CServiceBroker::GetAddonMgr().DisableAddon(addon.first->ID(),
                                                     AddonDisabledReason::PERMANENT_FAILURE);
          CServiceBroker::GetJobManager()->AddJob(
              new CPVREventLogJob(true, true, addon.first->Name(), g_localizeStrings.Get(24070),
                                  addon.first->Icon()),
              nullptr);
        }
      }
    }

    for (const auto& addon : addonsToReCreate)
    {
      progressHandler->UpdateProgress(addon->Name(), i++,
                                      addonsToCreate.size() + addonsToReCreate.size());

      // recreate client
      StopClient(addon->ID(), true);
    }

    progressHandler.reset();

    for (const auto& addon : addonsToDestroy)
    {
      // destroy client
      StopClient(addon->ID(), false);
    }

    if (!addonsToCreate.empty())
    {
      // update created clients map
      std::unique_lock<CCriticalSection> lock(m_critSection);
      for (const auto& addon : addonsToCreate)
      {
        if (m_clientMap.find(addon.second) == m_clientMap.end())
        {
          m_clientMap.insert(std::make_pair(addon.second, addon.first));
        }
      }
    }

    CServiceBroker::GetPVRManager().Start();
  }
}

bool CPVRClients::RequestRestart(const std::string& id, bool bDataChanged)
{
  return StopClient(id, true);
}

bool CPVRClients::StopClient(const std::string& id, bool bRestart)
{
  // stop playback if needed
  if (CServiceBroker::GetPVRManager().PlaybackState()->IsPlaying())
    CServiceBroker::GetAppMessenger()->SendMsg(TMSG_MEDIA_STOP);

  std::unique_lock<CCriticalSection> lock(m_critSection);

  int iId = GetClientId(id);
  std::shared_ptr<CPVRClient> mappedClient;
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
  if (typeid(event) == typeid(AddonEvents::Enabled) || // also called on install,
      typeid(event) == typeid(AddonEvents::Disabled) || // not called on uninstall
      typeid(event) == typeid(AddonEvents::UnInstalled) ||
      typeid(event) == typeid(AddonEvents::ReInstalled))
  {
    // update addons
    const std::string id = event.id;
    if (CServiceBroker::GetAddonMgr().HasType(id, ADDON_PVRDLL))
    {
      CServiceBroker::GetJobManager()->Submit([this, id] {
        UpdateAddons(id);
        return true;
      });
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// client access
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CPVRClients::GetClient(int iClientId, std::shared_ptr<CPVRClient>& addon) const
{
  bool bReturn = false;
  if (iClientId <= PVR_INVALID_CLIENT_ID)
    return bReturn;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  const auto& itr = m_clientMap.find(iClientId);
  if (itr != m_clientMap.end())
  {
    addon = itr->second;
    bReturn = true;
  }

  return bReturn;
}

int CPVRClients::GetClientId(const std::string& strId) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& entry : m_clientMap)
  {
    if (entry.second->ID() == strId)
    {
      return entry.first;
    }
  }

  return -1;
}

int CPVRClients::CreatedClientAmount() const
{
  int iReturn = 0;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& client : m_clientMap)
  {
    if (client.second->ReadyToUse())
      ++iReturn;
  }

  return iReturn;
}

bool CPVRClients::HasCreatedClients() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& client : m_clientMap)
  {
    if (client.second->ReadyToUse())
      return true;
  }

  return false;
}

bool CPVRClients::IsKnownClient(const std::string& id) const
{
  // valid client IDs start at 1
  return GetClientId(id) > 0;
}

bool CPVRClients::IsCreatedClient(int iClientId) const
{
  std::shared_ptr<CPVRClient> client;
  return GetCreatedClient(iClientId, client);
}

bool CPVRClients::IsCreatedClient(const std::string& id) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& client : m_clientMap)
  {
    if (client.second->ID() == id)
      return client.second->ReadyToUse();
  }
  return false;
}

bool CPVRClients::GetCreatedClient(int iClientId, std::shared_ptr<CPVRClient>& addon) const
{
  if (GetClient(iClientId, addon))
    return addon->ReadyToUse();

  return false;
}

int CPVRClients::GetCreatedClients(CPVRClientMap& clients) const
{
  int iReturn = 0;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& client : m_clientMap)
  {
    if (client.second->ReadyToUse())
    {
      clients.insert(std::make_pair(client.second->GetID(), client.second));
      ++iReturn;
    }
  }

  return iReturn;
}

std::vector<CVariant> CPVRClients::GetClientProviderInfos() const
{
  std::vector<AddonInfoPtr> addonInfos;
  // Get enabled and disabled PVR client addon infos
  CServiceBroker::GetAddonMgr().GetAddonInfos(addonInfos, false, ADDON_PVRDLL);

  std::unique_lock<CCriticalSection> lock(m_critSection);

  std::vector<CVariant> clientProviderInfos;
  for (const auto& addonInfo : addonInfos)
  {
    CVariant clientProviderInfo(CVariant::VariantTypeObject);
    if (IsKnownClient(addonInfo->ID()))
      clientProviderInfo["clientid"] = GetClientId(addonInfo->ID());
    else
      clientProviderInfo["clientid"] = ClientIdFromAddonId(addonInfo->ID());
    clientProviderInfo["addonid"] = addonInfo->ID();
    clientProviderInfo["enabled"] = !CServiceBroker::GetAddonMgr().IsAddonDisabled(addonInfo->ID());
    clientProviderInfo["name"] = addonInfo->Name();
    clientProviderInfo["icon"] = addonInfo->Icon();
    auto& artMap = addonInfo->Art();
    auto thumbEntry = artMap.find("thumb");
    if (thumbEntry != artMap.end())
      clientProviderInfo["thumb"] = thumbEntry->second;

    clientProviderInfos.emplace_back(clientProviderInfo);
  }

  return clientProviderInfos;
}

int CPVRClients::GetFirstCreatedClientID()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& client : m_clientMap)
  {
    if (client.second->ReadyToUse())
      return client.second->GetID();
  }

  return -1;
}

PVR_ERROR CPVRClients::GetCallableClients(CPVRClientMap& clientsReady,
                                          std::vector<int>& clientsNotReady) const
{
  clientsNotReady.clear();

  std::vector<AddonInfoPtr> addons;
  CServiceBroker::GetAddonMgr().GetAddonInfos(addons, true, ADDON::ADDON_PVRDLL);

  for (const auto& addon : addons)
  {
    int iClientId = ClientIdFromAddonId(addon->ID());
    std::shared_ptr<CPVRClient> client;
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

int CPVRClients::EnabledClientAmount() const
{
  int iReturn = 0;

  CPVRClientMap clientMap;
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    clientMap = m_clientMap;
  }

  for (const auto& client : clientMap)
  {
    if (!CServiceBroker::GetAddonMgr().IsAddonDisabled(client.second->ID()))
      ++iReturn;
  }

  return iReturn;
}

bool CPVRClients::IsEnabledClient(int clientId) const
{
  std::shared_ptr<CPVRClient> client;
  GetClient(clientId, client);
  return client && !CServiceBroker::GetAddonMgr().IsAddonDisabled(client->ID());
}

std::vector<CVariant> CPVRClients::GetEnabledClientInfos() const
{
  std::vector<CVariant> clientInfos;

  CPVRClientMap clientMap;
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    clientMap = m_clientMap;
  }

  for (const auto& client : clientMap)
  {
    const auto& addonInfo = CServiceBroker::GetAddonMgr().GetAddonInfo(client.second->ID());

    if (addonInfo)
    {
      // This will be the same variant structure used in the json api
      CVariant clientInfo(CVariant::VariantTypeObject);
      clientInfo["clientid"] = client.first;
      clientInfo["addonid"] = client.second->ID();
      clientInfo["label"] = addonInfo->Name(); // Note that this is called label instead of name

      const auto& capabilities = client.second->GetClientCapabilities();
      clientInfo["supportstv"] = capabilities.SupportsTV();
      clientInfo["supportsradio"] = capabilities.SupportsRadio();
      clientInfo["supportsepg"] = capabilities.SupportsEPG();
      clientInfo["supportsrecordings"] = capabilities.SupportsRecordings();
      clientInfo["supportstimers"] = capabilities.SupportsTimers();
      clientInfo["supportschannelgroups"] = capabilities.SupportsChannelGroups();
      clientInfo["supportschannelscan"] = capabilities.SupportsChannelScan();
      clientInfo["supportchannelproviders"] = capabilities.SupportsProviders();

      clientInfos.push_back(clientInfo);
    }
  }

  return clientInfos;
}

bool CPVRClients::HasIgnoredClients() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& client : m_clientMap)
  {
    if (client.second->IgnoreClient())
      return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// client API calls
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<SBackend> CPVRClients::GetBackendProperties() const
{
  std::vector<SBackend> backendProperties;

  ForCreatedClients(__FUNCTION__, [&backendProperties](const std::shared_ptr<CPVRClient>& client) {
    SBackend properties;

    if (client->GetDriveSpace(properties.diskTotal, properties.diskUsed) == PVR_ERROR_NO_ERROR)
    {
      properties.diskTotal *= 1024;
      properties.diskUsed *= 1024;
    }

    int iAmount = 0;
    if (client->GetProvidersAmount(iAmount) == PVR_ERROR_NO_ERROR)
      properties.numProviders = iAmount;
    if (client->GetChannelGroupsAmount(iAmount) == PVR_ERROR_NO_ERROR)
      properties.numChannelGroups = iAmount;
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

bool CPVRClients::GetTimers(const std::vector<std::shared_ptr<CPVRClient>>& clients,
                            CPVRTimersContainer* timers,
                            std::vector<int>& failedClients)
{
  return ForClients(__FUNCTION__, clients,
                    [timers](const std::shared_ptr<CPVRClient>& client) {
                      return client->GetTimers(timers);
                    },
                    failedClients) == PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVRClients::GetTimerTypes(std::vector<std::shared_ptr<CPVRTimerType>>& results) const
{
  return ForCreatedClients(__FUNCTION__, [&results](const std::shared_ptr<CPVRClient>& client) {
    std::vector<std::shared_ptr<CPVRTimerType>> types;
    PVR_ERROR ret = client->GetTimerTypes(types);
    if (ret == PVR_ERROR_NO_ERROR)
      results.insert(results.end(), types.begin(), types.end());
    return ret;
  });
}

PVR_ERROR CPVRClients::GetRecordings(const std::vector<std::shared_ptr<CPVRClient>>& clients,
                                     CPVRRecordings* recordings,
                                     bool deleted,
                                     std::vector<int>& failedClients)
{
  return ForClients(__FUNCTION__, clients,
                    [recordings, deleted](const std::shared_ptr<CPVRClient>& client) {
                      return client->GetRecordings(recordings, deleted);
                    },
                    failedClients);
}

PVR_ERROR CPVRClients::DeleteAllRecordingsFromTrash()
{
  return ForCreatedClients(__FUNCTION__, [](const std::shared_ptr<CPVRClient>& client) {
    return client->DeleteAllRecordingsFromTrash();
  });
}

PVR_ERROR CPVRClients::SetEPGMaxPastDays(int iPastDays)
{
  return ForCreatedClients(__FUNCTION__, [iPastDays](const std::shared_ptr<CPVRClient>& client) {
    return client->SetEPGMaxPastDays(iPastDays);
  });
}

PVR_ERROR CPVRClients::SetEPGMaxFutureDays(int iFutureDays)
{
  return ForCreatedClients(__FUNCTION__, [iFutureDays](const std::shared_ptr<CPVRClient>& client) {
    return client->SetEPGMaxFutureDays(iFutureDays);
  });
}

PVR_ERROR CPVRClients::GetChannels(const std::vector<std::shared_ptr<CPVRClient>>& clients,
                                   bool bRadio,
                                   std::vector<std::shared_ptr<CPVRChannel>>& channels,
                                   std::vector<int>& failedClients)
{
  return ForClients(__FUNCTION__, clients,
                    [bRadio, &channels](const std::shared_ptr<CPVRClient>& client) {
                      return client->GetChannels(bRadio, channels);
                    },
                    failedClients);
}

PVR_ERROR CPVRClients::GetProviders(const std::vector<std::shared_ptr<CPVRClient>>& clients,
                                    CPVRProvidersContainer* providers,
                                    std::vector<int>& failedClients)
{
  return ForClients(__FUNCTION__, clients,
                    [providers](const std::shared_ptr<CPVRClient>& client) {
                      return client->GetProviders(*providers);
                    },
                    failedClients);
}

PVR_ERROR CPVRClients::GetChannelGroups(const std::vector<std::shared_ptr<CPVRClient>>& clients,
                                        CPVRChannelGroups* groups,
                                        std::vector<int>& failedClients)
{
  return ForClients(__FUNCTION__, clients,
                    [groups](const std::shared_ptr<CPVRClient>& client) {
                      return client->GetChannelGroups(groups);
                    },
                    failedClients);
}

PVR_ERROR CPVRClients::GetChannelGroupMembers(
    const std::vector<std::shared_ptr<CPVRClient>>& clients,
    CPVRChannelGroup* group,
    std::vector<std::shared_ptr<CPVRChannelGroupMember>>& groupMembers,
    std::vector<int>& failedClients)
{
  return ForClients(__FUNCTION__, clients,
                    [group, &groupMembers](const std::shared_ptr<CPVRClient>& client) {
                      return client->GetChannelGroupMembers(group, groupMembers);
                    },
                    failedClients);
}

std::vector<std::shared_ptr<CPVRClient>> CPVRClients::GetClientsSupportingChannelScan() const
{
  std::vector<std::shared_ptr<CPVRClient>> possibleScanClients;
  ForCreatedClients(__FUNCTION__, [&possibleScanClients](const std::shared_ptr<CPVRClient>& client) {
    if (client->GetClientCapabilities().SupportsChannelScan())
      possibleScanClients.emplace_back(client);
    return PVR_ERROR_NO_ERROR;
  });
  return possibleScanClients;
}

std::vector<std::shared_ptr<CPVRClient>> CPVRClients::GetClientsSupportingChannelSettings(bool bRadio) const
{
  std::vector<std::shared_ptr<CPVRClient>> possibleSettingsClients;
  ForCreatedClients(__FUNCTION__, [bRadio, &possibleSettingsClients](const std::shared_ptr<CPVRClient>& client) {
    const CPVRClientCapabilities& caps = client->GetClientCapabilities();
    if (caps.SupportsChannelSettings() &&
        ((bRadio && caps.SupportsRadio()) || (!bRadio && caps.SupportsTV())))
      possibleSettingsClients.emplace_back(client);
    return PVR_ERROR_NO_ERROR;
  });
  return possibleSettingsClients;
}

bool CPVRClients::AnyClientSupportingRecordingsSize() const
{
  std::vector<std::shared_ptr<CPVRClient>> recordingSizeClients;
  ForCreatedClients(__FUNCTION__, [&recordingSizeClients](const std::shared_ptr<CPVRClient>& client) {
    if (client->GetClientCapabilities().SupportsRecordingsSize())
      recordingSizeClients.emplace_back(client);
    return PVR_ERROR_NO_ERROR;
  });
  return recordingSizeClients.size() != 0;
}

bool CPVRClients::AnyClientSupportingEPG() const
{
  bool bHaveSupportingClient = false;
  ForCreatedClients(__FUNCTION__,
                    [&bHaveSupportingClient](const std::shared_ptr<CPVRClient>& client) {
                      if (client->GetClientCapabilities().SupportsEPG())
                        bHaveSupportingClient = true;
                      return PVR_ERROR_NO_ERROR;
                    });
  return bHaveSupportingClient;
}

bool CPVRClients::AnyClientSupportingRecordings() const
{
  bool bHaveSupportingClient = false;
  ForCreatedClients(__FUNCTION__,
                    [&bHaveSupportingClient](const std::shared_ptr<CPVRClient>& client) {
                      if (client->GetClientCapabilities().SupportsRecordings())
                        bHaveSupportingClient = true;
                      return PVR_ERROR_NO_ERROR;
                    });
  return bHaveSupportingClient;
}

bool CPVRClients::AnyClientSupportingRecordingsDelete() const
{
  bool bHaveSupportingClient = false;
  ForCreatedClients(__FUNCTION__,
                    [&bHaveSupportingClient](const std::shared_ptr<CPVRClient>& client) {
                      if (client->GetClientCapabilities().SupportsRecordingsDelete())
                        bHaveSupportingClient = true;
                      return PVR_ERROR_NO_ERROR;
                    });
  return bHaveSupportingClient;
}

void CPVRClients::OnSystemSleep()
{
  ForCreatedClients(__FUNCTION__, [](const std::shared_ptr<CPVRClient>& client) {
    client->OnSystemSleep();
    return PVR_ERROR_NO_ERROR;
  });
}

void CPVRClients::OnSystemWake()
{
  ForCreatedClients(__FUNCTION__, [](const std::shared_ptr<CPVRClient>& client) {
    client->OnSystemWake();
    return PVR_ERROR_NO_ERROR;
  });
}

void CPVRClients::OnPowerSavingActivated()
{
  ForCreatedClients(__FUNCTION__, [](const std::shared_ptr<CPVRClient>& client) {
    client->OnPowerSavingActivated();
    return PVR_ERROR_NO_ERROR;
  });
}

void CPVRClients::OnPowerSavingDeactivated()
{
  ForCreatedClients(__FUNCTION__, [](const std::shared_ptr<CPVRClient>& client) {
    client->OnPowerSavingDeactivated();
    return PVR_ERROR_NO_ERROR;
  });
}

void CPVRClients::ConnectionStateChange(CPVRClient* client,
                                        const std::string& strConnectionString,
                                        PVR_CONNECTION_STATE newState,
                                        const std::string& strMessage)
{
  if (!client)
    return;

  int iMsg = -1;
  bool bError = true;
  bool bNotify = true;

  switch (newState)
  {
    case PVR_CONNECTION_STATE_SERVER_UNREACHABLE:
      iMsg = 35505; // Server is unreachable
      if (client->GetPreviousConnectionState() == PVR_CONNECTION_STATE_UNKNOWN ||
          client->GetPreviousConnectionState() == PVR_CONNECTION_STATE_CONNECTING)
      {
        // Make our users happy. There were so many complaints about this notification because their TV backend
        // was not up quick enough after Kodi start. So, ignore the very first 'server not reachable' notification.
        bNotify = false;
      }
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
      CLog::LogF(LOGERROR, "Unknown connection state");
      return;
  }

  // Use addon-supplied message, if present
  std::string strMsg;
  if (!strMessage.empty())
    strMsg = strMessage;
  else
    strMsg = g_localizeStrings.Get(iMsg);

  // Notify user.
  CServiceBroker::GetJobManager()->AddJob(
      new CPVREventLogJob(bNotify, bError, client->Name(), strMsg, client->Icon()), nullptr);
}

namespace
{

void LogClientWarning(const char* strFunctionName, const std::shared_ptr<CPVRClient>& client)
{
  if (client->IgnoreClient())
    CLog::Log(LOGWARNING, "{}: Not calling add-on '{}'. Add-on not (yet) connected.",
              strFunctionName, client->ID());
  else if (!client->ReadyToUse())
    CLog::Log(LOGWARNING, "{}: Not calling add-on '{}'. Add-on not ready to use.", strFunctionName,
              client->ID());
  else
    CLog::Log(LOGERROR, "{}: Not calling add-on '{}' for unexpected reason.", strFunctionName,
              client->ID());
}

} // unnamed namespace

PVR_ERROR CPVRClients::ForCreatedClients(const char* strFunctionName,
                                         const PVRClientFunction& function) const
{
  std::vector<int> failedClients;
  return ForCreatedClients(strFunctionName, function, failedClients);
}

PVR_ERROR CPVRClients::ForCreatedClients(const char* strFunctionName,
                                         const PVRClientFunction& function,
                                         std::vector<int>& failedClients) const
{
  PVR_ERROR lastError = PVR_ERROR_NO_ERROR;

  CPVRClientMap clients;
  GetCallableClients(clients, failedClients);

  if (!failedClients.empty())
  {
    for (int id : failedClients)
    {
      std::shared_ptr<CPVRClient> client;
      GetClient(id, client);

      if (client)
        LogClientWarning(strFunctionName, client);
    }
  }

  for (const auto& clientEntry : clients)
  {
    PVR_ERROR currentError = function(clientEntry.second);

    if (currentError != PVR_ERROR_NO_ERROR && currentError != PVR_ERROR_NOT_IMPLEMENTED)
    {
      lastError = currentError;
      failedClients.emplace_back(clientEntry.first);
    }
  }
  return lastError;
}

PVR_ERROR CPVRClients::ForClients(const char* strFunctionName,
                                  const std::vector<std::shared_ptr<CPVRClient>>& clients,
                                  const PVRClientFunction& function,
                                  std::vector<int>& failedClients) const
{
  if (clients.empty())
    return ForCreatedClients(strFunctionName, function, failedClients);

  PVR_ERROR lastError = PVR_ERROR_NO_ERROR;

  failedClients.clear();

  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    for (const auto& entry : m_clientMap)
    {
      if (entry.second->ReadyToUse() && !entry.second->IgnoreClient() &&
          std::find_if(clients.cbegin(), clients.cend(),
                       [&entry](const std::shared_ptr<CPVRClient>& client) {
                         return entry.first == client->GetID();
                       }) != clients.cend())
      {
        // Allow ready to use clients that shall be called
        continue;
      }

      failedClients.emplace_back(entry.first);
    }
  }

  for (const auto& client : clients)
  {
    if (std::find_if(failedClients.cbegin(), failedClients.cend(), [&client](int failedClientId) {
          return client->GetID() == failedClientId;
        }) == failedClients.cend())
    {
      PVR_ERROR currentError = function(client);

      if (currentError != PVR_ERROR_NO_ERROR && currentError != PVR_ERROR_NOT_IMPLEMENTED)
      {
        lastError = currentError;
        failedClients.emplace_back(client->GetID());
      }
    }
    else
    {
      LogClientWarning(strFunctionName, client);
    }
  }
  return lastError;
}
