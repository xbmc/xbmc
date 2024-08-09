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
#include "messaging/ApplicationMessenger.h"
#include "pvr/PVREventLogJob.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRPlaybackState.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/addons/PVRClientUID.h"
#include "pvr/guilib/PVRGUIProgressHandler.h"
#include "utils/JobManager.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

using namespace ADDON;
using namespace PVR;

CPVRClients::CPVRClients()
{
  CServiceBroker::GetAddonMgr().RegisterAddonMgrCallback(AddonType::PVRDLL, this);
  CServiceBroker::GetAddonMgr().Events().Subscribe(this, &CPVRClients::OnAddonEvent);
}

CPVRClients::~CPVRClients()
{
  CServiceBroker::GetAddonMgr().Events().Unsubscribe(this);
  CServiceBroker::GetAddonMgr().UnregisterAddonMgrCallback(AddonType::PVRDLL);

  for (const auto& client : m_clientMap)
  {
    client.second->Destroy();
  }
}

void CPVRClients::Start()
{
  UpdateClients();
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

void CPVRClients::UpdateClients(
    const std::string& changedAddonId /* = "" */,
    ADDON::AddonInstanceId changedInstanceId /* = ADDON::ADDON_SINGLETON_INSTANCE_ID */)
{
  std::vector<std::pair<AddonInfoPtr, bool>> addonsWithStatus;
  if (!GetAddonsWithStatus(changedAddonId, addonsWithStatus))
    return;

  std::vector<std::shared_ptr<CPVRClient>> clientsToCreate; // client
  std::vector<std::pair<int, std::string>> clientsToReCreate; // client id, addon name
  std::vector<int> clientsToDestroy; // client id

  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    for (const auto& addonWithStatus : addonsWithStatus)
    {
      const AddonInfoPtr addon = addonWithStatus.first;
      const std::vector<std::pair<ADDON::AddonInstanceId, bool>> instanceIdsWithStatus =
          GetInstanceIdsWithStatus(addon, addonWithStatus.second);

      for (const auto& instanceIdWithStatus : instanceIdsWithStatus)
      {
        const ADDON::AddonInstanceId instanceId = instanceIdWithStatus.first;
        bool instanceEnabled = instanceIdWithStatus.second;
        const CPVRClientUID clientUID(addon->ID(), instanceId);
        const int clientId = clientUID.GetUID();

        if (instanceEnabled && (!IsKnownClient(clientId) || !IsCreatedClient(clientId)))
        {
          std::shared_ptr<CPVRClient> client;
          const bool isKnownClient = IsKnownClient(clientId);
          if (isKnownClient)
          {
            client = GetClient(clientId);
          }
          else
          {
            client = std::make_shared<CPVRClient>(addon, instanceId, clientId);
            if (!client)
            {
              CLog::LogF(LOGERROR, "Severe error, incorrect add-on type");
              continue;
            }
          }

          // determine actual enabled state of instance
          if (instanceId != ADDON_SINGLETON_INSTANCE_ID)
            instanceEnabled = client->IsEnabled();

          if (instanceEnabled)
          {
            CLog::LogF(LOGINFO, "Creating PVR client: addonId={}, instanceId={}, clientId={}",
                       addon->ID(), instanceId, clientId);
            clientsToCreate.emplace_back(client);
          }
          else if (isKnownClient)
          {
            CLog::LogF(LOGINFO, "Destroying PVR client: addonId={}, instanceId={}, clientId={}",
                       addon->ID(), instanceId, clientId);
            clientsToDestroy.emplace_back(clientId);
          }
        }
        else if (IsCreatedClient(clientId))
        {
          // determine actual enabled state of instance
          if (instanceEnabled && instanceId != ADDON_SINGLETON_INSTANCE_ID)
          {
            const std::shared_ptr<const CPVRClient> client = GetClient(clientId);
            instanceEnabled = client ? client->IsEnabled() : false;
          }

          if (instanceEnabled)
          {
            CLog::LogF(LOGINFO, "Recreating PVR client: addonId={}, instanceId={}, clientId={}",
                       addon->ID(), instanceId, clientId);
            clientsToReCreate.emplace_back(clientId, addon->Name());
          }
          else
          {
            CLog::LogF(LOGINFO, "Destroying PVR client: addonId={}, instanceId={}, clientId={}",
                       addon->ID(), instanceId, clientId);
            clientsToDestroy.emplace_back(clientId);
          }
        }
      }
    }
  }

  if (!clientsToCreate.empty() || !clientsToReCreate.empty() || !clientsToDestroy.empty())
  {
    CServiceBroker::GetPVRManager().Stop();

    auto progressHandler = std::make_unique<CPVRGUIProgressHandler>(
        g_localizeStrings.Get(19239)); // Creating PVR clients

    unsigned int i = 0;
    for (const auto& client : clientsToCreate)
    {
      progressHandler->UpdateProgress(client->Name(), i++,
                                      clientsToCreate.size() + clientsToReCreate.size());

      const ADDON_STATUS status = client->Create();

      if (status != ADDON_STATUS_OK)
      {
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
    }

    for (const auto& clientInfo : clientsToReCreate)
    {
      progressHandler->UpdateProgress(clientInfo.second, i++,
                                      clientsToCreate.size() + clientsToReCreate.size());

      // stop and recreate client
      StopClient(clientInfo.first, true /* restart */);
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
      std::unique_lock<CCriticalSection> lock(m_critSection);
      for (const auto& client : clientsToCreate)
      {
        if (m_clientMap.find(client->GetID()) == m_clientMap.end())
        {
          m_clientMap.insert({client->GetID(), client});
        }
      }
    }

    CServiceBroker::GetPVRManager().Start();
  }
}

bool CPVRClients::RequestRestart(const std::string& addonId,
                                 ADDON::AddonInstanceId instanceId,
                                 bool bDataChanged)
{
  CServiceBroker::GetJobManager()->Submit([this, addonId, instanceId] {
    UpdateClients(addonId, instanceId);
    return true;
  });
  return true;
}

bool CPVRClients::StopClient(int clientId, bool restart)
{
  // stop playback if needed
  if (CServiceBroker::GetPVRManager().PlaybackState()->IsPlaying())
    CServiceBroker::GetAppMessenger()->SendMsg(TMSG_MEDIA_STOP);

  std::unique_lock<CCriticalSection> lock(m_critSection);

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

void CPVRClients::OnAddonEvent(const AddonEvent& event)
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
    const ADDON::AddonInstanceId instanceId = event.instanceId;
    if (CServiceBroker::GetAddonMgr().HasType(addonId, AddonType::PVRDLL))
    {
      CServiceBroker::GetJobManager()->Submit([this, addonId, instanceId] {
        UpdateClients(addonId, instanceId);
        return true;
      });
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// client access
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<CPVRClient> CPVRClients::GetClient(int clientId) const
{
  if (clientId <= PVR_INVALID_CLIENT_ID)
    return {};

  std::unique_lock<CCriticalSection> lock(m_critSection);
  const auto it = m_clientMap.find(clientId);
  if (it != m_clientMap.end())
    return it->second;

  return {};
}

int CPVRClients::CreatedClientAmount() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return std::count_if(m_clientMap.cbegin(), m_clientMap.cend(),
                       [](const auto& client) { return client.second->ReadyToUse(); });
}

bool CPVRClients::HasCreatedClients() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return std::any_of(m_clientMap.cbegin(), m_clientMap.cend(),
                     [](const auto& client) { return client.second->ReadyToUse(); });
}

bool CPVRClients::IsKnownClient(int clientId) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

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

  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& client : m_clientMap)
  {
    if (client.second->ReadyToUse())
    {
      clients.insert(std::make_pair(client.second->GetID(), client.second));
    }
  }

  return clients;
}

std::vector<CVariant> CPVRClients::GetClientProviderInfos() const
{
  std::vector<AddonInfoPtr> addonInfos;
  // Get enabled and disabled PVR client addon infos
  CServiceBroker::GetAddonMgr().GetAddonInfos(addonInfos, false, AddonType::PVRDLL);

  std::unique_lock<CCriticalSection> lock(m_critSection);

  std::vector<CVariant> clientProviderInfos;
  for (const auto& addonInfo : addonInfos)
  {
    std::vector<ADDON::AddonInstanceId> instanceIds = addonInfo->GetKnownInstanceIds();
    for (const auto& instanceId : instanceIds)
    {
      CVariant clientProviderInfo(CVariant::VariantTypeObject);
      clientProviderInfo["clientid"] = CPVRClientUID(addonInfo->ID(), instanceId).GetUID();
      clientProviderInfo["addonid"] = addonInfo->ID();
      clientProviderInfo["instanceid"] = instanceId;
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
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const auto it = std::find_if(m_clientMap.cbegin(), m_clientMap.cend(),
                               [](const auto& client) { return client.second->ReadyToUse(); });
  return it != m_clientMap.cend() ? (*it).second->GetID() : -1;
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
        clientsReady.insert(std::make_pair(clientId, client));
      }
      else
      {
        clientsNotReady.emplace_back(clientId);
      }
    }
  }

  return clientsNotReady.empty() ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR;
}

int CPVRClients::EnabledClientAmount() const
{
  CPVRClientMap clientMap;
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    clientMap = m_clientMap;
  }

  ADDON::CAddonMgr& addonMgr = CServiceBroker::GetAddonMgr();
  return std::count_if(clientMap.cbegin(), clientMap.cend(), [&addonMgr](const auto& client) {
    return !addonMgr.IsAddonDisabled(client.second->ID());
  });
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
    std::unique_lock<CCriticalSection> lock(m_critSection);
    clientMap = m_clientMap;
  }

  for (const auto& client : clientMap)
  {
    const auto& addonInfo =
        CServiceBroker::GetAddonMgr().GetAddonInfo(client.second->ID(), AddonType::PVRDLL);

    if (addonInfo)
    {
      // This will be the same variant structure used in the json api
      CVariant clientInfo(CVariant::VariantTypeObject);
      clientInfo["clientid"] = client.first;
      clientInfo["addonid"] = client.second->ID();
      clientInfo["instanceid"] = client.second->InstanceId();
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
  return std::any_of(m_clientMap.cbegin(), m_clientMap.cend(),
                     [](const auto& client) { return client.second->IgnoreClient(); });
}

std::vector<ADDON::AddonInstanceId> CPVRClients::GetKnownInstanceIds(
    const std::string& addonID) const
{
  std::vector<ADDON::AddonInstanceId> instanceIds;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& entry : m_clientMap)
  {
    if (entry.second->ID() == addonID)
      instanceIds.emplace_back(entry.second->InstanceId());
  }

  return instanceIds;
}

bool CPVRClients::GetAddonsWithStatus(
    const std::string& changedAddonId,
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
  std::transform(instanceIds.cbegin(), instanceIds.cend(),
                 std::back_inserter(instanceIdsWithStatus), [addonIsEnabled](const auto& id) {
                   return std::pair<ADDON::AddonInstanceId, bool>(id, addonIsEnabled);
                 });

  // find removed instances
  const std::vector<ADDON::AddonInstanceId> knownInstanceIds = GetKnownInstanceIds(addon->ID());
  for (const auto& knownInstanceId : knownInstanceIds)
  {
    if (std::find(instanceIds.begin(), instanceIds.end(), knownInstanceId) == instanceIds.end())
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

std::vector<SBackend> CPVRClients::GetBackendProperties() const
{
  std::vector<SBackend> backendProperties;

  ForCreatedClients(
      __FUNCTION__, [&backendProperties](const std::shared_ptr<const CPVRClient>& client) {
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
                            std::vector<int>& failedClients) const
{
  return ForClients(
             __FUNCTION__, clients,
             [timers](const std::shared_ptr<const CPVRClient>& client) {
               return client->GetTimers(timers);
             },
             failedClients) == PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVRClients::UpdateTimerTypes(const std::vector<std::shared_ptr<CPVRClient>>& clients,
                                        std::vector<int>& failedClients)
{
  return ForClients(
      __FUNCTION__, clients,
      [](const std::shared_ptr<CPVRClient>& client) { return client->UpdateTimerTypes(); },
      failedClients);
}

const std::vector<std::shared_ptr<CPVRTimerType>> CPVRClients::GetTimerTypes() const
{
  std::vector<std::shared_ptr<CPVRTimerType>> types;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& entry : m_clientMap)
  {
    const auto& client = entry.second;
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
      __FUNCTION__, clients,
      [recordings, deleted](const std::shared_ptr<const CPVRClient>& client) {
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
                                   std::vector<int>& failedClients) const
{
  return ForClients(
      __FUNCTION__, clients,
      [bRadio, &channels](const std::shared_ptr<const CPVRClient>& client) {
        return client->GetChannels(bRadio, channels);
      },
      failedClients);
}

PVR_ERROR CPVRClients::GetProviders(const std::vector<std::shared_ptr<CPVRClient>>& clients,
                                    CPVRProvidersContainer* providers,
                                    std::vector<int>& failedClients) const
{
  return ForClients(
      __FUNCTION__, clients,
      [providers](const std::shared_ptr<const CPVRClient>& client) {
        return client->GetProviders(*providers);
      },
      failedClients);
}

PVR_ERROR CPVRClients::GetChannelGroups(const std::vector<std::shared_ptr<CPVRClient>>& clients,
                                        CPVRChannelGroups* groups,
                                        std::vector<int>& failedClients) const
{
  return ForClients(
      __FUNCTION__, clients,
      [groups](const std::shared_ptr<const CPVRClient>& client) {
        return client->GetChannelGroups(groups);
      },
      failedClients);
}

PVR_ERROR CPVRClients::GetChannelGroupMembers(
    const std::vector<std::shared_ptr<CPVRClient>>& clients,
    CPVRChannelGroup* group,
    std::vector<std::shared_ptr<CPVRChannelGroupMember>>& groupMembers,
    std::vector<int>& failedClients) const
{
  return ForClients(
      __FUNCTION__, clients,
      [group, &groupMembers](const std::shared_ptr<const CPVRClient>& client) {
        return client->GetChannelGroupMembers(group, groupMembers);
      },
      failedClients);
}

std::vector<std::shared_ptr<CPVRClient>> CPVRClients::GetClientsSupportingChannelScan() const
{
  std::vector<std::shared_ptr<CPVRClient>> possibleScanClients;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& entry : m_clientMap)
  {
    const auto& client = entry.second;
    if (client->ReadyToUse() && !client->IgnoreClient() &&
        client->GetClientCapabilities().SupportsChannelScan())
      possibleScanClients.emplace_back(client);
  }

  return possibleScanClients;
}

std::vector<std::shared_ptr<CPVRClient>> CPVRClients::GetClientsSupportingChannelSettings(bool bRadio) const
{
  std::vector<std::shared_ptr<CPVRClient>> possibleSettingsClients;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& entry : m_clientMap)
  {
    const auto& client = entry.second;
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
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return std::any_of(m_clientMap.cbegin(), m_clientMap.cend(), [](const auto& entry) {
    const auto& client = entry.second;
    return client->ReadyToUse() && !client->IgnoreClient() &&
           client->GetClientCapabilities().SupportsRecordingsSize();
  });
}

bool CPVRClients::AnyClientSupportingEPG() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return std::any_of(m_clientMap.cbegin(), m_clientMap.cend(), [](const auto& entry) {
    const auto& client = entry.second;
    return client->ReadyToUse() && !client->IgnoreClient() &&
           client->GetClientCapabilities().SupportsEPG();
  });
}

bool CPVRClients::AnyClientSupportingRecordings() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return std::any_of(m_clientMap.cbegin(), m_clientMap.cend(), [](const auto& entry) {
    const auto& client = entry.second;
    return client->ReadyToUse() && !client->IgnoreClient() &&
           client->GetClientCapabilities().SupportsRecordings();
  });
}

bool CPVRClients::AnyClientSupportingRecordingsDelete() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return std::any_of(m_clientMap.cbegin(), m_clientMap.cend(), [](const auto& entry) {
    const auto& client = entry.second;
    return client->ReadyToUse() && !client->IgnoreClient() &&
           client->GetClientCapabilities().SupportsRecordingsDelete();
  });
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
    std::shared_ptr<CPVRClient> client;
    for (int id : failedClients)
    {
      client = GetClient(id);
      if (client)
        LogClientWarning(strFunctionName, client);
    }
  }

  for (const auto& clientEntry : clients)
  {
    //    CLog::LogFC(LOGDEBUG, LOGPVR, "Calling add-on function '{}' on client {}.", strFunctionName,
    //                clientEntry.second->GetID());

    PVR_ERROR currentError = function(clientEntry.second);

    //    CLog::LogFC(LOGDEBUG, LOGPVR, "Called add-on function '{}' on client {}. return={}",
    //                strFunctionName, clientEntry.second->GetID(), currentError);

    if (currentError != PVR_ERROR_NO_ERROR && currentError != PVR_ERROR_NOT_IMPLEMENTED)
    {
      lastError = currentError;
      failedClients.emplace_back(clientEntry.first);

      CLog::LogFC(LOGDEBUG, LOGPVR,
                  "Added client {} to failed clients list after call to "
                  "function '{}‘ returned error {}.",
                  clientEntry.second->GetID(), strFunctionName, currentError);
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
          std::any_of(clients.cbegin(), clients.cend(),
                      [&entry](const auto& client) { return client->GetID() == entry.first; }))
      {
        // Allow ready to use clients that shall be called
        continue;
      }

      failedClients.emplace_back(entry.first);
    }
  }

  for (const auto& client : clients)
  {
    if (std::none_of(failedClients.cbegin(), failedClients.cend(),
                     [&client](int failedClientId) { return failedClientId == client->GetID(); }))
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
