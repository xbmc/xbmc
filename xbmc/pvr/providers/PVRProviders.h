/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

#include <memory>
#include <vector>

namespace PVR
{
class CPVRProvider;

enum class ProviderUpdateMode;

class CPVRProvidersContainer
{
public:
  /*!
     * @brief Add a provider to this container or update the provider if already present in this container.
     * @param The provider
     * @return True, if the update was successful. False, otherwise.
     */
  bool UpdateFromClient(const std::shared_ptr<CPVRProvider>& provider);

  /*!
     * @brief Get the provider denoted by given client id and unique client provider id.
     * @param iClientId The client id.
     * @param iUniqueId The client provider id.
     * @return the provider if found, null otherwise.
     */
  std::shared_ptr<CPVRProvider> GetByClient(int iClientId, int iUniqueId) const;

  /*!
     * Get all providers in this container
     * @return The list of all providers
     */
  std::vector<std::shared_ptr<CPVRProvider>> GetProvidersList() const;

protected:
  void InsertEntry(const std::shared_ptr<CPVRProvider>& newProvider, ProviderUpdateMode updateMode);

  mutable CCriticalSection m_critSection;
  int m_iLastId = 0;
  std::vector<std::shared_ptr<CPVRProvider>> m_providers;
};

class CPVRProviders : public CPVRProvidersContainer
{
public:
  CPVRProviders() = default;
  ~CPVRProviders() = default;

  /**
     * @brief (re)load the providers from the clients.
     * @return True if loaded successfully, false otherwise.
     */
  bool Load();

  /**
     * @brief unload all providers.
     */
  void Unload();

  /**
     * @brief refresh the providers list from the clients.
     */
  bool Update();

  /**
     * @brief load the local providers from database.
     * @return True if loaded successfully, false otherwise.
     */
  bool LoadFromDatabase();

  /*!
     * @brief Check if the entry exists in the container, if it does update it otherwise add it
     * @param newProvider The provider entry to update/add in/to the container
     * @param updateMode update as Client (respect User set values) or DB (update all values)
     * @return The provider if updated or added, otherwise an empty object (nullptr)
     */
  std::shared_ptr<CPVRProvider> CheckAndAddEntry(const std::shared_ptr<CPVRProvider>& newProvider,
                                                 ProviderUpdateMode updateMode);

  /*!
     * @brief Check if the entry exists in the container, if it does update it and persist
     *        it in the DB otherwise add it and persist it in the DB.
     * @param newProvider The provider entry to update/add in/to the container and DB
     * @param updateMode update as Client (respect User set values) or DB (update all values)
     * @return The provider if updated or added, otherwise an empty object (nullptr)
     */
  std::shared_ptr<CPVRProvider> CheckAndPersistEntry(
      const std::shared_ptr<CPVRProvider>& newProvider, ProviderUpdateMode updateMode);

  /**
     * @brief Persist user changes to the current state of the providers in the DB.
     */
  bool PersistUserChanges(const std::vector<std::shared_ptr<CPVRProvider>>& providers);

  /*!
     * @brief Get a provider given it's database ID
     * @param iProviderId The ID to find
     * @return The provider, or an empty one when not found
     */
  std::shared_ptr<CPVRProvider> GetById(int iProviderId) const;

  /*!
    * @brief Erase stale texture db entries and image files.
    * @return number of cleaned up images.
    */
  int CleanupCachedImages();

private:
  void RemoveEntry(const std::shared_ptr<CPVRProvider>& provider);
  bool UpdateDefaultEntries(const CPVRProvidersContainer& newProviders);
  bool UpdateClientEntries(const CPVRProvidersContainer& newProviders,
                           const std::vector<int>& failedClients,
                           const std::vector<int>& disabledClients);

  bool m_bIsUpdating = false;
};
} // namespace PVR
