/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_providers.h"
#include "pvr/PVRCachedImage.h"
#include "threads/CriticalSection.h"
#include "utils/ISerializable.h"
#include "utils/ISortable.h"

#include <memory>
#include <string>
#include <vector>

namespace PVR
{

enum class ProviderUpdateMode
{
  BY_CLIENT,
  BY_DATABASE
};

static constexpr int PVR_PROVIDER_ADDON_UID = -1;
static constexpr int PVR_PROVIDER_INVALID_DB_ID = -1;

class CPVRProvider final : public ISerializable, public ISortable
{
public:
  static const std::string IMAGE_OWNER_PATTERN;

  CPVRProvider(int iUniqueId, int iClientId);
  CPVRProvider(const PVR_PROVIDER& provider, int iClientId);
  CPVRProvider(int iClientId,
               const std::string& addonProviderName,
               const std::string& addonIconPath,
               const std::string& addonThumbPath);

  bool operator==(const CPVRProvider& right) const;
  bool operator!=(const CPVRProvider& right) const;

  void Serialize(CVariant& value) const override;

  // ISortable implementation
  void ToSortable(SortItem& sortable, Field field) const override;

  /*!
   * @brief The database id of this provider
   *
   * A unique identifier for this provider.
   * It can be used to find the same provider on this clients backend
   *
   * @return The database id of this provider
   */
  int GetDatabaseId() const;

  /*!
   * @brief Set the database id of this provider
   * @param iDatabaseId The new ID.
   * @return True if the something changed, false otherwise.
   */
  bool SetDatabaseId(int iDatabaseId);

  /*!
   * @brief A unique identifier for this provider.
   *
   * A unique identifier for this provider.
   * It can be used to find the same provider on this clients backend
   *
   * @return The Unique ID.
   */
  int GetUniqueId() const;

  /*!
   * @return The identifier of the client that supplies this provider.
   */
  int GetClientId() const;

  /*!
   * @return The name of the provider. Can be user provided or the backend name
   */
  std::string GetName() const;

  /*!
   * @brief Set the name of the provider.
   * @param name The new name of the provider.
   * @return True if the something changed, false otherwise.
   */
  bool SetName(const std::string& iName);

  /*!
   * @brief Checks whether this provider has a known type
   * @return True if this provider has a type other than unknown, false otherwise
   */
  bool HasType() const { return m_type != PVR_PROVIDER_TYPE_UNKNOWN; }

  /*!
   * @brief Gets the type of this provider.
   * @return the type of this provider.
   */
  PVR_PROVIDER_TYPE GetType() const;

  /*!
   * @brief Sets the type of this provider.
   * @param type the new provider type.
   * @return True if the something changed, false otherwise.
   */
  bool SetType(PVR_PROVIDER_TYPE type);

  /*!
   * @brief Get the path for this provider's icon
   * @return iconpath for this provider's icon
   */
  std::string GetIconPath() const;

  /*!
   * @brief Set the path for this icon
   * @param strIconPath The new path of the icon.
   * @return true if the icon path was updated successfully
   */
  bool SetIconPath(const std::string& strIconPath);

  /*!
   * @return Get the path to the icon for this provider as given by the client.
   */
  std::string GetClientIconPath() const;

  /*!
   * @brief Get this provider's country codes (ISO 3166).
   * @return This provider's country codes.
   */
  std::vector<std::string> GetCountries() const;

  /*!
   * @brief Set the country codes for this provider
   * @param countries The new ISO 3166 country codes for this provider.
   * @return true if the country codes were updated successfully
   */
  bool SetCountries(const std::vector<std::string>& countries);

  /*!
   * @brief Get this provider's country codes (ISO 3166) as a string.
   * @return This provider's country codes.
   */
  std::string GetCountriesDBString() const;

  /*!
   * @brief Set the country codes for this provider from a string
   * @param strCountries The new ISO 3166 country codes for this provider.
   * @return true if the country codes were updated successfully
   */
  bool SetCountriesFromDBString(const std::string& strCountries);

  /*!
   * @brief Get this provider's language codes (RFC 5646).
   * @return This provider's language codes
   */
  std::vector<std::string> GetLanguages() const;

  /*!
   * @brief Set the language codes for this provider
   * @param languages The new RFC 5646 language codes for this provider.
   * @return true if the language codes were updated successfully
   */
  bool SetLanguages(const std::vector<std::string>& languages);

  /*!
   * @brief Get this provider's language codes (RFC 5646) as a string.
   * @return This provider's language codes.
   */
  std::string GetLanguagesDBString() const;

  /*!
   * @brief Set the language codes for this provider from a string
   * @param strLanguages The new RFC 5646 language codes for this provider.
   * @return true if the language codes were updated successfully
   */
  bool SetLanguagesFromDBString(const std::string& strLanguages);

  /*!
   * @brief Get if this provider has a thumb image path.
   * @return True if this add-on provider has a thumb image path, false otherwise.
   */
  bool HasThumbPath() const;

  /*!
   * @brief Get this provider's thumb image path. Note only PVR add-on providers will set this value.
   * @return This add-on provider's thumb image path.
   */
  std::string GetThumbPath() const;

  /*!
   * @return Get the path to the thumb for this provider as given by the client.
   */
  std::string GetClientThumbPath() const;

  /*!
   * @brief Whether a provider is a default provider of a PVR Client add-on or not
   * @return True if this provider is of a PVR Client add-on, false otherwise.
   */
  bool IsClientProvider() const { return m_bIsClientProvider; }

  /*!
   * @brief updates this provider from the provided entry
   * @param fromProvider A provider containing the data that shall be merged into this provider's data.
   * @param updateMode update as User, Client or DB
   * @return true if the provider was updated successfully
   */
  bool UpdateEntry(const std::shared_ptr<CPVRProvider>& fromProvider,
                   ProviderUpdateMode updateMode);

  /*!
   * @brief Persist this provider in the local database.
   * @param updateRecord True if an existing record should be updated, false for an insert
   * @return True on success, false otherwise.
   */
  bool Persist(bool updateRecord = false);

  /*!
   * @brief Delete this provider from the local database.
   * @return True on success, false otherwise.
   */
  bool DeleteFromDatabase();

private:
  CPVRProvider(const CPVRProvider& provider) = delete;
  CPVRProvider& operator=(const CPVRProvider& orig) = delete;

  int m_iDatabaseId = PVR_PROVIDER_INVALID_DB_ID; /*!< the identifier given to this provider by the TV database */

  int m_iUniqueId = PVR_PROVIDER_ADDON_UID; /*!< @brief unique ID of the provider on the backend */
  int m_iClientId; /*!< @brief ID of the backend */
  std::string m_strName; /*!< @brief name of this provider */
  PVR_PROVIDER_TYPE m_type = PVR_PROVIDER_TYPE_UNKNOWN; /*!< @brief service type for this provider */
  CPVRCachedImage m_iconPath; /*!< @brief the path to the icon for this provider */
  std::string m_strCountries; /*!< @brief the country codes for this provider (empty if undefined) */
  std::string m_strLanguages; /*!< @brief the language codes for this provider (empty if undefined) */
  bool m_bIsClientProvider = false; /*!< the provider is a default provider of a PVR Client add-on */
  CPVRCachedImage m_thumbPath; /*!< a thumb image path for providers that are PVR add-ons */

  mutable CCriticalSection m_critSection;
};
} // namespace PVR
