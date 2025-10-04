/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRClients.h"

#include "ServiceBroker.h"
#include "addons/AddonEvents.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "guilib/LocalizeStrings.h"
#include "jobs/JobManager.h"
#include "messaging/ApplicationMessenger.h"
#include "pvr/PVRConstants.h" // PVR_CLIENT_INVALID_UID
#include "pvr/PVREventLogJob.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRPlaybackState.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/addons/PVRClientUID.h"
#include "pvr/guilib/PVRGUIProgressHandler.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <source_location>
#include <string>
#include <utility>
#include <vector>

using namespace ADDON;
using namespace PVR;

CPVRClients::CPVRClients()
{
  CServiceBroker::GetAddonMgr().RegisterAddonMgrCallback(AddonType::PVRDLL, this);
  CServiceBroker::GetAddonMgr().Events().Subscribe(
      this,
      [this](const AddonEvent& event)
      {
        if (typeid(event) == typeid(AddonEvents::Enabled) || // also called on install,
            typeid(event) == typeid(AddonEvents::Disabled) || // not called on uninstall
            typeid(event) == typeid(AddonEvents::UnInstalled) ||
            typeid(event) == typeid(AddonEvents::ReInstalled) ||
            typeid(event) == typeid(AddonEvents::InstanceAdded) ||
            typeid(event) == typeid(AddonEvents::InstanceRemoved))
        {
          // update addons
          const std::string addonId = event.addonId;
          if (CServiceBroker::GetAddonMgr().HasType(addonId, AddonType::PVRDLL))
          {
            CServiceBroker::GetJobManager()->Submit(
                [this, addonId]
                {
                  UpdateClients(addonId);
                  return true;
                });
          }
        }
      });
}

CPVRClients::~CPVRClients()
{
  CServiceBroker::GetAddonMgr().Events().Unsubscribe(this);
  CServiceBroker::GetAddonMgr().UnregisterAddonMgrCallback(AddonType::PVRDLL);

  DestroyClients();
}

void CPVRClients::Start()
{
  UpdateClients();
}

void CPVRClients::Stop()
{
  std::unique_lock lock(m_critSection);
  for (const auto& [_, client] : m_clientMap)
  {
    client->Stop();
  }

  CPVRClientUID::ClearCache();
}

void CPVRClients::Continue()
{
  std::unique_lock lock(m_critSection);
  for (const auto& [_, client] : m_clientMap)
  {
    client->Continue();
  }
}

void CPVRClients::DestroyClients()
{
  std::unique_lock lock(m_critSection);
  for (const auto& [_, client] : m_clientMap)
  {
    CLog::Log(LOGINFO, "Destroying PVR client: addonId={}, instanceId={}, clientId={}",
              client->ID(), client->InstanceId(), client->GetID());
    client->Destroy();
  }
  m_clientMap.clear();
}

CPVRClients::UpdateClientAction CPVRClients::GetUpdateClientAction(
    const std::shared_ptr<ADDON::CAddonInfo>& addon,
    ADDON::AddonInstanceId instanceId,
    int clientId,
    bool instanceEnabled) const
{
  using enum UpdateClientAction;

  if (instanceEnabled && (!IsKnownClient(clientId) || !IsCreatedClient(clientId)))
  {
    const bool isKnownClient{IsKnownClient(clientId)};

    // determine actual enabled state of instance
    if (instanceId != ADDON_SINGLETON_INSTANCE_ID)
    {
      std::shared_ptr<CPVRClient> client;
      if (isKnownClient)
        client = GetClient(clientId);
      else
        client = std::make_shared<CPVRClient>(addon, instanceId, clientId);

      instanceEnabled = client->IsEnabled();
    }

    if (instanceEnabled)
      return CREATE;
    else if (isKnownClient)
      return DESTROY;
  }
  else if (IsCreatedClient(clientId))
  {
    // determine actual enabled state of instance
    if (instanceEnabled && instanceId != ADDON_SINGLETON_INSTANCE_ID)
    {
      const std::shared_ptr<const CPVRClient> client{GetClient(clientId)};
      instanceEnabled = client ? client->IsEnabled() : false;
    }

    if (instanceEnabled)
      return RECREATE;
    else
      return DESTROY;
  }
  return NONE;
}

void CPVRClients::UpdateClients(const std::string& changedAddonId /* = "" */)
{
  std::vector<std::pair<AddonInfoPtr, bool>> addonsWithStatus;
  if (!GetAddonsWithStatus(changedAddonId, addonsWithStatus))
    return;

  std::vector<std::shared_ptr<CPVRClient>> clientsToCreate; // client
  std::vector<std::pair<int, std::string>> clientsToReCreate; // client id, addon name
  std::vector<int> clientsToDestroy; // client id

  {
    std::unique_lock lock(m_critSection);
    for (const auto& [addon, addonStatus] : addonsWithStatus)
    {
      const std::vector<std::pair<ADDON::AddonInstanceId, bool>> instanceIdsWithStatus =
          GetInstanceIdsWithStatus(addon, addonStatus);

      for (const auto& [instanceId, instanceEnabled] : instanceIdsWithStatus)
      {
        const CPVRClientUID clientUID(addon->ID(), instanceId);
        const int clientId = clientUID.GetUID();

        const UpdateClientAction action{
            GetUpdateClientAction(addon, instanceId, clientId, instanceEnabled)};
        switch (action)
        {
          using enum UpdateClientAction;

          case CREATE:
          {
            std::shared_ptr<CPVRClient> client;
            if (IsKnownClient(clientId))
              client = GetClient(clientId);
            else
              client = std::make_shared<CPVRClient>(addon, instanceId, clientId);

            CLog::Log(LOGINFO, "Creating PVR client: addonId={}, instanceId={}, clientId={}",
                      addon->ID(), instanceId, clientId);
            clientsToCreate.emplace_back(client);
            break;
          }
          case RECREATE:
          {
            CLog::Log(LOGINFO, "Recreating PVR client: addonId={}, instanceId={}, clientId={}",
                      addon->ID(), instanceId, clientId);
            clientsToReCreate.emplace_back(clientId, addon->Name());
            break;
          }
          case DESTROY:
          {
            CLog::Log(LOGINFO, "Destroying PVR client: addonId={}, instanceId={}, clientId={}",
                      addon->ID(), instanceId, clientId);
            clientsToDestroy.emplace_back(clientId);
            break;
          }
          case NONE:
          default:
            break;
        }
      }
    }
  }

  if (!clientsToCreate.empty() || !clientsToReCreate.empty() || !clientsToDestroy.empty())
  {
    CServiceBroker::GetPVRManager().Stop();

    auto progressHandler = std::make_unique<CPVRGUIProgressHandler>(
        g_localizeStrings.Get(19239)); // Creating PVR clients

    size_t i = 0;
    for (const auto& client : clientsToCreate)
    {
      progressHandler->UpdateProgress(client->Name(), i,
                                      clientsToCreate.size() + clientsToReCreate.size());
      i++;

      const ADDON_STATUS status = client->Create();

      if (status == ADDON_STATUS_PERMANENT_FAILURE)
      {
        CServiceBroker::GetAddonMgr().DisableAddon(client->ID(),
                                                   AddonDisabledReason::PERMANENT_FAILURE);
        CServiceBroker::GetJobManager()->AddJob(
            new CPVREventLogJob(true, EventLevel::Error, client->Name(),
                                g_localizeStrings.Get(24070), client->Icon()),
            nullptr);
      }
    }

    for (const auto& [clientId, addonName] : clientsToReCreate)
    {
      progressHandler->UpdateProgress(addonName, i,
                                      clientsToCreate.size() + clientsToReCreate.size());
      i++;

      // stop and recreate client
      StopClient(clientId, true /* restart */);
    }

    progressHandler.reset();

    for (const auto& client : clientsToDestroy)
    {
      // destroy client
      StopClient(client, false /* no restart */);
    }

    if (!clientsToCreate.empty())
    {
      // update created clients map
      std::unique_lock lock(m_critSection);
      for (const auto& client : clientsToCreate)
      {
        m_clientMap.try_emplace(client->GetID(), client);
      }
    }

    CServiceBroker::GetPVRManager().Start();
  }
}

bool CPVRClients::RequestRestart(const std::string& addonId,
                                 ADDON::AddonInstanceId instanceId,
                                 bool bDataChanged /* = 0 */)
{
  CServiceBroker::GetJobManager()->Submit(
      [this, addonId]
      {
        UpdateClients(addonId);
        return true;
      });
  return true;
}

bool CPVRClients::StopClient(int clientId, bool restart)
{
  // stop playback if needed
  if (CServiceBroker::GetPVRManager().PlaybackState()->IsPlaying())
    CServiceBroker::GetAppMessenger()->SendMsg(TMSG_MEDIA_STOP);

  std::unique_lock lock(m_critSection);

  const std::shared_ptr<CPVRClient> client = GetClient(clientId);
  if (client)
  {
    if (restart)
    {
      client->ReCreate();
    }
    else
    {
      const auto it = m_clientMap.find(clientId);
      if (it != m_clientMap.end())
        m_clientMap.erase(it);

      client->Destroy();
    }
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// client access
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<CPVRClient> CPVRClients::GetClient(int clientId) const
{
  if (clientId == PVR_CLIENT_INVALID_UID)
    return {};

  std::unique_lock lock(m_critSection);
  const auto it = m_clientMap.find(clientId);
  if (it != m_clientMap.end())
    return it->second;

  return {};
}

size_t CPVRClients::CreatedClientAmount() const
{
  std::unique_lock lock(m_critSection);
  return std::ranges::count_if(m_clientMap,
                               [](const auto& client) { return client.second->ReadyToUse(); });
}

bool CPVRClients::HasCreatedClients() const
{
  std::unique_lock lock(m_critSection);
  return std::ranges::any_of(m_clientMap,
                             [](const auto& client) { return client.second->ReadyToUse(); });
}

bool CPVRClients::IsKnownClient(int clientId) const
{
  std::unique_lock lock(m_critSection);

  // valid client IDs start at 1
  const auto it = m_clientMap.find(clientId);
  return (it != m_clientMap.end() && (*it).second->GetID() > 0);
}

bool CPVRClients::IsCreatedClient(int iClientId) const
{
  return GetCreatedClient(iClientId) != nullptr;
}

std::shared_ptr<CPVRClient> CPVRClients::GetCreatedClient(int clientId) const
{
  std::shared_ptr<CPVRClient> client = GetClient(clientId);
  if (client && client->ReadyToUse())
    return client;

  return {};
}

CPVRClientMap CPVRClients::GetCreatedClients() const
{
  CPVRClientMap clients;

  std::unique_lock lock(m_critSection);
  for (const auto& [clientId, client] : m_clientMap)
  {
    if (client->ReadyToUse())
    {
      clients.try_emplace(clientId, client);
    }
  }

  return clients;
}

std::vector<CVariant> CPVRClients::GetClientProviderInfos() const
{
  std::vector<AddonInfoPtr> addonInfos;
  // Get enabled and disabled PVR client addon infos
  CServiceBroker::GetAddonMgr().GetAddonInfos(addonInfos, false, AddonType::PVRDLL);

  std::unique_lock lock(m_critSection);

  std::vector<CVariant> clientProviderInfos;
  for (const auto& addonInfo : addonInfos)
  {
    std::vector<ADDON::AddonInstanceId> instanceIds = addonInfo->GetKnownInstanceIds();
    for (const auto& instanceId : instanceIds)
    {
      CVariant clientProviderInfo(CVariant::VariantTypeObject);
      const int clientId{CPVRClientUID(addonInfo->ID(), instanceId).GetUID()};
      clientProviderInfo["clientid"] = clientId;
      clientProviderInfo["addonid"] = addonInfo->ID();
      clientProviderInfo["instanceid"] = instanceId;
      std::string fullName;
      const std::shared_ptr<const CPVRClient> client{GetClient(clientId)};
      if (client)
        fullName = client->GetFullClientName();
      else
        fullName = addonInfo->Name();
      clientProviderInfo["fullname"] = fullName;
      clientProviderInfo["enabled"] =
          !CServiceBroker::GetAddonMgr().IsAddonDisabled(addonInfo->ID());
      clientProviderInfo["name"] = addonInfo->Name();
      clientProviderInfo["icon"] = addonInfo->Icon();
      auto& artMap = addonInfo->Art();
      auto thumbEntry = artMap.find("thumb");
      if (thumbEntry != artMap.end())
        clientProviderInfo["thumb"] = thumbEntry->second;

      clientProviderInfos.emplace_back(clientProviderInfo);
    }
  }

  return clientProviderInfos;
}

int CPVRClients::GetFirstCreatedClientID() const
{
  std::unique_lock lock(m_critSection);
  const auto it = std::ranges::find_if(m_clientMap, [](const auto& client)
                                       { return client.second->ReadyToUse(); });
  return it != m_clientMap.cend() ? (*it).second->GetID() : PVR_CLIENT_INVALID_UID;
}

PVR_ERROR CPVRClients::GetCallableClients(CPVRClientMap& clientsReady,
                                          std::vector<int>& clientsNotReady) const
{
  clientsNotReady.clear();

  std::vector<AddonInfoPtr> addons;
  CServiceBroker::GetAddonMgr().GetAddonInfos(addons, true, AddonType::PVRDLL);

  for (const auto& addon : addons)
  {
    std::vector<ADDON::AddonInstanceId> instanceIds = addon->GetKnownInstanceIds();
    for (const auto& instanceId : instanceIds)
    {
      const int clientId = CPVRClientUID(addon->ID(), instanceId).GetUID();
      const std::shared_ptr<CPVRClient> client = GetClient(clientId);

      if (client && client->ReadyToUse() && !client->IgnoreClient())
      {
        clientsReady.try_emplace(clientId, client);
      }
      else
      {
        clientsNotReady.emplace_back(clientId);
      }
    }
  }

  return clientsNotReady.empty() ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR;
}

size_t CPVRClients::EnabledClientAmount() const
{
  CPVRClientMap clientMap;
  {
    std::unique_lock lock(m_critSection);
    clientMap = m_clientMap;
  }

  const ADDON::CAddonMgr& addonMgr = CServiceBroker::GetAddonMgr();
  return std::ranges::count_if(clientMap, [&addonMgr](const auto& client)
                               { return !addonMgr.IsAddonDisabled(client.second->ID()); });
}

bool CPVRClients::IsEnabledClient(int clientId) const
{
  const std::shared_ptr<const CPVRClient> client = GetClient(clientId);
  return client && !CServiceBroker::GetAddonMgr().IsAddonDisabled(client->ID());
}

std::vector<CVariant> CPVRClients::GetEnabledClientInfos() const
{
  std::vector<CVariant> clientInfos;

  CPVRClientMap clientMap;
  {
    std::unique_lock lock(m_critSection);
    clientMap = m_clientMap;
  }

  for (const auto& [clientId, client] : clientMap)
  {
    const auto& addonInfo =
        CServiceBroker::GetAddonMgr().GetAddonInfo(client->ID(), AddonType::PVRDLL);

    if (addonInfo)
    {
      // This will be the same variant structure used in the json api
      CVariant clientInfo(CVariant::VariantTypeObject);
      clientInfo["clientid"] = clientId;
      clientInfo["addonid"] = client->ID();
      clientInfo["instanceid"] = client->InstanceId();
      clientInfo["label"] = addonInfo->Name(); // Note that this is called label instead of name

      const auto& capabilities = client->GetClientCapabilities();
      clientInfo["supportstv"] = capabilities.SupportsTV();
      clientInfo["supportsradio"] = capabilities.SupportsRadio();
      clientInfo["supportsepg"] = capabilities.SupportsEPG();
      clientInfo["supportsrecordings"] = capabilities.SupportsRecordings();
      clientInfo["supportstimers"] = capabilities.SupportsTimers();
      clientInfo["supportschannelgroups"] = capabilities.SupportsChannelGroups();
      clientInfo["supportschannelscan"] = capabilities.SupportsChannelScan();
      clientInfo["supportchannelproviders"] = capabilities.SupportsProviders();

      clientInfos.emplace_back(clientInfo);
    }
  }

  return clientInfos;
}

bool CPVRClients::HasIgnoredClients() const
{
  std::unique_lock lock(m_critSection);
  return std::ranges::any_of(m_clientMap,
                             [](const auto& client) { return client.second->IgnoreClient(); });
}

std::vector<ADDON::AddonInstanceId> CPVRClients::GetKnownInstanceIds(std::string_view addonID) const
{
  std::vector<ADDON::AddonInstanceId> instanceIds;

  std::unique_lock lock(m_critSection);
  for (const auto& [_, client] : m_clientMap)
  {
    if (client->ID() == addonID)
      instanceIds.emplace_back(client->InstanceId());
  }

  return instanceIds;
}

bool CPVRClients::GetAddonsWithStatus(
    std::string_view changedAddonId,
    std::vector<std::pair<AddonInfoPtr, bool>>& addonsWithStatus) const
{
  std::vector<AddonInfoPtr> addons;
  CServiceBroker::GetAddonMgr().GetAddonInfos(addons, false, AddonType::PVRDLL);

  if (addons.empty())
    return false;

  bool foundChangedAddon = changedAddonId.empty();
  for (const auto& addon : addons)
  {
    bool enabled = !CServiceBroker::GetAddonMgr().IsAddonDisabled(addon->ID());
    addonsWithStatus.emplace_back(addon, enabled);

    if (!foundChangedAddon && addon->ID() == changedAddonId)
      foundChangedAddon = true;
  }

  return foundChangedAddon;
}

std::vector<std::pair<ADDON::AddonInstanceId, bool>> CPVRClients::GetInstanceIdsWithStatus(
    const AddonInfoPtr& addon, bool addonIsEnabled) const
{
  std::vector<std::pair<ADDON::AddonInstanceId, bool>> instanceIdsWithStatus;

  std::vector<ADDON::AddonInstanceId> instanceIds = addon->GetKnownInstanceIds();
  std::ranges::transform(instanceIds, std::back_inserter(instanceIdsWithStatus),
                         [addonIsEnabled](const auto& id)
                         { return std::pair<ADDON::AddonInstanceId, bool>(id, addonIsEnabled); });

  // find removed instances
  const std::vector<ADDON::AddonInstanceId> knownInstanceIds = GetKnownInstanceIds(addon->ID());
  for (const auto& knownInstanceId : knownInstanceIds)
  {
    if (std::ranges::find(instanceIds, knownInstanceId) == instanceIds.cend())
    {
      // instance was removed
      instanceIdsWithStatus.emplace_back(knownInstanceId, false);
    }
  }

  return instanceIdsWithStatus;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// client API calls
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<SBackendProperties> CPVRClients::GetBackendProperties() const
{
  std::vector<SBackendProperties> backendProperties;

  ForCreatedClients(std::source_location::current().function_name(),
                    [&backendProperties](const std::shared_ptr<const CPVRClient>& client)
                    {
                      SBackendProperties properties;
                      backendProperties.emplace_back(client->GetBackendProperties());
                      return PVR_ERROR_NO_ERROR;
                    });

  return backendProperties;
}

bool CPVRClients::GetTimers(const std::vector<std::shared_ptr<CPVRClient>>& clients,
                            CPVRTimersContainer* timers,
                            std::vector<int>& failedClients) const
{
  return ForClients(
             std::source_location::current().function_name(), clients,
             [timers](const std::shared_ptr<const CPVRClient>& client)
             { return client->GetTimers(timers); }, failedClients) == PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVRClients::UpdateTimerTypes(const std::vector<std::shared_ptr<CPVRClient>>& clients,
                                        std::vector<int>& failedClients) const
{
  return ForClients(
      std::source_location::current().function_name(), clients,
      [](const std::shared_ptr<CPVRClient>& client) { return client->UpdateTimerTypes(); },
      failedClients);
}

std::vector<std::shared_ptr<CPVRTimerType>> CPVRClients::GetTimerTypes() const
{
  std::vector<std::shared_ptr<CPVRTimerType>> types;

  std::unique_lock lock(m_critSection);
  for (const auto& [_, client] : m_clientMap)
  {
    if (client->ReadyToUse() && !client->IgnoreClient())
    {
      const auto& clientTypes = client->GetTimerTypes();
      types.insert(types.end(), clientTypes.begin(), clientTypes.end());
    }
  }

  return types;
}

PVR_ERROR CPVRClients::GetRecordings(const std::vector<std::shared_ptr<CPVRClient>>& clients,
                                     CPVRRecordings* recordings,
                                     bool deleted,
                                     std::vector<int>& failedClients) const
{
  return ForClients(
      std::source_location::current().function_name(), clients,
      [recordings, deleted](const std::shared_ptr<const CPVRClient>& client)
      { return client->GetRecordings(recordings, deleted); }, failedClients);
}

PVR_ERROR CPVRClients::DeleteAllRecordingsFromTrash() const
{
  return ForCreatedClients(std::source_location::current().function_name(),
                           [](const std::shared_ptr<CPVRClient>& client)
                           { return client->DeleteAllRecordingsFromTrash(); });
}

PVR_ERROR CPVRClients::SetEPGMaxPastDays(int iPastDays) const
{
  return ForCreatedClients(std::source_location::current().function_name(),
                           [iPastDays](const std::shared_ptr<CPVRClient>& client)
                           { return client->SetEPGMaxPastDays(iPastDays); });
}

PVR_ERROR CPVRClients::SetEPGMaxFutureDays(int iFutureDays) const
{
  return ForCreatedClients(std::source_location::current().function_name(),
                           [iFutureDays](const std::shared_ptr<CPVRClient>& client)
                           { return client->SetEPGMaxFutureDays(iFutureDays); });
}

PVR_ERROR CPVRClients::GetChannels(const std::vector<std::shared_ptr<CPVRClient>>& clients,
                                   bool bRadio,
                                   std::vector<std::shared_ptr<CPVRChannel>>& channels,
                                   std::vector<int>& failedClients) const
{
  return ForClients(
      std::source_location::current().function_name(), clients,
      [bRadio, &channels](const std::shared_ptr<CPVRClient>& client)
      { return client->GetChannels(bRadio, channels); }, failedClients);
}

PVR_ERROR CPVRClients::GetProviders(const std::vector<std::shared_ptr<CPVRClient>>& clients,
                                    CPVRProvidersContainer* providers,
                                    std::vector<int>& failedClients) const
{
  return ForClients(
      std::source_location::current().function_name(), clients,
      [providers](const std::shared_ptr<const CPVRClient>& client)
      { return client->GetProviders(*providers); }, failedClients);
}

PVR_ERROR CPVRClients::GetChannelGroups(const std::vector<std::shared_ptr<CPVRClient>>& clients,
                                        CPVRChannelGroups* groups,
                                        std::vector<int>& failedClients) const
{
  return ForClients(
      std::source_location::current().function_name(), clients,
      [groups](const std::shared_ptr<const CPVRClient>& client)
      { return client->GetChannelGroups(groups); }, failedClients);
}

PVR_ERROR CPVRClients::GetChannelGroupMembers(
    const std::vector<std::shared_ptr<CPVRClient>>& clients,
    const CPVRChannelGroup& group,
    std::vector<std::shared_ptr<CPVRChannelGroupMember>>& groupMembers,
    std::vector<int>& failedClients) const
{
  return ForClients(
      std::source_location::current().function_name(), clients,
      [&group, &groupMembers](const std::shared_ptr<const CPVRClient>& client)
      { return client->GetChannelGroupMembers(group, groupMembers); }, failedClients);
}

std::vector<std::shared_ptr<CPVRClient>> CPVRClients::GetClientsSupportingChannelScan() const
{
  std::vector<std::shared_ptr<CPVRClient>> possibleScanClients;

  std::unique_lock lock(m_critSection);
  for (const auto& [_, client] : m_clientMap)
  {
    if (client->ReadyToUse() && !client->IgnoreClient() &&
        client->GetClientCapabilities().SupportsChannelScan())
      possibleScanClients.emplace_back(client);
  }

  return possibleScanClients;
}

std::vector<std::shared_ptr<CPVRClient>> CPVRClients::GetClientsSupportingChannelSettings(
    bool bRadio) const
{
  std::vector<std::shared_ptr<CPVRClient>> possibleSettingsClients;

  std::unique_lock lock(m_critSection);
  for (const auto& [_, client] : m_clientMap)
  {
    if (client->ReadyToUse() && !client->IgnoreClient())
    {
      const CPVRClientCapabilities& caps = client->GetClientCapabilities();
      if (caps.SupportsChannelSettings() &&
          ((bRadio && caps.SupportsRadio()) || (!bRadio && caps.SupportsTV())))
        possibleSettingsClients.emplace_back(client);
    }
  }

  return possibleSettingsClients;
}

bool CPVRClients::AnyClientSupportingRecordingsSize() const
{
  std::unique_lock lock(m_critSection);
  return std::ranges::any_of(m_clientMap,
                             [](const auto& entry)
                             {
                               const auto& client = entry.second;
                               return client->ReadyToUse() && !client->IgnoreClient() &&
                                      client->GetClientCapabilities().SupportsRecordingsSize();
                             });
}

bool CPVRClients::AnyClientSupportingEPG() const
{
  std::unique_lock lock(m_critSection);
  return std::ranges::any_of(m_clientMap,
                             [](const auto& entry)
                             {
                               const auto& client = entry.second;
                               return client->ReadyToUse() && !client->IgnoreClient() &&
                                      client->GetClientCapabilities().SupportsEPG();
                             });
}

bool CPVRClients::AnyClientSupportingRecordings() const
{
  std::unique_lock lock(m_critSection);
  return std::ranges::any_of(m_clientMap,
                             [](const auto& entry)
                             {
                               const auto& client = entry.second;
                               return client->ReadyToUse() && !client->IgnoreClient() &&
                                      client->GetClientCapabilities().SupportsRecordings();
                             });
}

bool CPVRClients::AnyClientSupportingRecordingsDelete() const
{
  std::unique_lock lock(m_critSection);
  return std::ranges::any_of(m_clientMap,
                             [](const auto& entry)
                             {
                               const auto& client = entry.second;
                               return client->ReadyToUse() && !client->IgnoreClient() &&
                                      client->GetClientCapabilities().SupportsRecordingsDelete();
                             });
}

void CPVRClients::OnSleep()
{
  ForCreatedClients(std::source_location::current().function_name(),
                    [](const std::shared_ptr<CPVRClient>& client)
                    {
                      client->OnSystemSleep();
                      return PVR_ERROR_NO_ERROR;
                    });
  CPowerState::OnSleep();
}

void CPVRClients::OnWake()
{
  CPowerState::OnWake();
  ForCreatedClients(std::source_location::current().function_name(),
                    [](const std::shared_ptr<CPVRClient>& client)
                    {
                      client->OnSystemWake();
                      return PVR_ERROR_NO_ERROR;
                    });
}

void CPVRClients::OnPowerSavingActivated() const
{
  ForCreatedClients(std::source_location::current().function_name(),
                    [](const std::shared_ptr<CPVRClient>& client)
                    {
                      client->OnPowerSavingActivated();
                      return PVR_ERROR_NO_ERROR;
                    });
}

void CPVRClients::OnPowerSavingDeactivated() const
{
  ForCreatedClients(std::source_location::current().function_name(),
                    [](const std::shared_ptr<CPVRClient>& client)
                    {
                      client->OnPowerSavingDeactivated();
                      return PVR_ERROR_NO_ERROR;
                    });
}

void CPVRClients::ConnectionStateChange(const CPVRClient* client,
                                        std::string_view strConnectionString,
                                        PVR_CONNECTION_STATE newState,
                                        std::string_view strMessage) const
{
  if (!client)
    return;

  int iMsg = -1;
  EventLevel eLevel = EventLevel::Error;
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
      eLevel = EventLevel::Basic;
      iMsg = 36034; // Connection established
      if (client->GetPreviousConnectionState() == PVR_CONNECTION_STATE_UNKNOWN ||
          client->GetPreviousConnectionState() == PVR_CONNECTION_STATE_CONNECTING)
        bNotify = false;
      break;
    case PVR_CONNECTION_STATE_DISCONNECTED:
      iMsg = 36030; // Connection lost
      break;
    case PVR_CONNECTION_STATE_CONNECTING:
      eLevel = EventLevel::Information;
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

  if (!strConnectionString.empty())
    strMsg = StringUtils::Format("{} ({})", strMsg, strConnectionString);

  // Notify user.
  CServiceBroker::GetJobManager()->AddJob(
      new CPVREventLogJob(bNotify, eLevel, client->GetFullClientName(), strMsg, client->Icon()),
      nullptr);
}

namespace
{

void LogClientWarning(const char* strFunctionName, const std::shared_ptr<const CPVRClient>& client)
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

template<typename F>
PVR_ERROR CPVRClients::ForCreatedClients(const char* strFunctionName, F function) const
{
  std::vector<int> failedClients;
  return ForCreatedClients(strFunctionName, function, failedClients);
}

template<typename F>
PVR_ERROR CPVRClients::ForCreatedClients(const char* strFunctionName,
                                         F function,
                                         std::vector<int>& failedClients) const
{
  PVR_ERROR lastError = PVR_ERROR_NO_ERROR;

  CPVRClientMap clients;
  GetCallableClients(clients, failedClients);

  if (!failedClients.empty())
  {
    std::shared_ptr<CPVRClient> client;
    for (int id : failedClients)
    {
      client = GetClient(id);
      if (client)
        LogClientWarning(strFunctionName, client);
    }
  }

  for (const auto& [clientId, client] : clients)
  {
    //    CLog::LogFC(LOGDEBUG, LOGPVR, "Calling add-on function '{}' on client {}.", strFunctionName,
    //                clientEntry.second->GetID());

    PVR_ERROR currentError = function(client);

    //    CLog::LogFC(LOGDEBUG, LOGPVR, "Called add-on function '{}' on client {}. return={}",
    //                strFunctionName, clientEntry.second->GetID(), currentError);

    if (currentError != PVR_ERROR_NO_ERROR && currentError != PVR_ERROR_NOT_IMPLEMENTED)
    {
      lastError = currentError;
      failedClients.emplace_back(clientId);

      CLog::LogFC(LOGDEBUG, LOGPVR,
                  "Added client {} to failed clients list after call to "
                  "function '{}‘ returned error {}.",
                  client->GetID(), strFunctionName, currentError);
    }
  }
  return lastError;
}

template<typename F>
PVR_ERROR CPVRClients::ForClients(const char* strFunctionName,
                                  const std::vector<std::shared_ptr<CPVRClient>>& clients,
                                  F function,
                                  std::vector<int>& failedClients) const
{
  if (clients.empty())
    return ForCreatedClients(strFunctionName, function, failedClients);

  PVR_ERROR lastError = PVR_ERROR_NO_ERROR;

  failedClients.clear();

  {
    std::unique_lock lock(m_critSection);
    for (const auto& [clientId, client] : m_clientMap)
    {
      if (client->ReadyToUse() && !client->IgnoreClient() &&
          std::ranges::any_of(clients,
                              [clientId](const auto& c) { return c->GetID() == clientId; }))
      {
        // Allow ready to use clients that shall be called
        continue;
      }

      failedClients.emplace_back(clientId);
    }
  }

  for (const auto& client : clients)
  {
    if (std::ranges::none_of(failedClients, [&client](int failedClientId)
                             { return failedClientId == client->GetID(); }))
    {
      //      CLog::LogFC(LOGDEBUG, LOGPVR, "Calling add-on function '{}' on client {}.", strFunctionName,
      //                  client->GetID());

      PVR_ERROR currentError = function(client);

      //      CLog::LogFC(LOGDEBUG, LOGPVR, "Called add-on function '{}' on client {}. return={}",
      //                  strFunctionName, client->GetID(), currentError);

      if (currentError != PVR_ERROR_NO_ERROR && currentError != PVR_ERROR_NOT_IMPLEMENTED)
      {
        lastError = currentError;
        failedClients.emplace_back(client->GetID());

        CLog::LogFC(LOGDEBUG, LOGPVR,
                    "Added client {} to failed clients list after call to "
                    "function '{}‘ returned error {}.",
                    client->GetID(), strFunctionName, currentError);
      }
    }
    else
    {
      LogClientWarning(strFunctionName, client);
    }
  }
  return lastError;
}
