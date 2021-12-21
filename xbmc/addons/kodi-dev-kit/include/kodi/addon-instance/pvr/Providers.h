/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../AddonBase.h"
#include "../../c-api/addon-instance/pvr.h"
#include "../../tools/StringUtils.h"

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C++" Definitions group 2 - PVR provider
#ifdef __cplusplus

namespace kodi
{
namespace addon
{

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_PVRProvider class PVRProvider
/// @ingroup cpp_kodi_addon_pvr_Defs_Provider
/// @brief **Provider data structure**\n
/// Representation of a provider.
///
/// This is used to store all the necessary provider data and can
/// either provide the necessary data from / to Kodi for the associated
/// functions or can also be used in the addon to store its data.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_pvr_Defs_PVRProvider_Help
///
///@{
class PVRProvider : public CStructHdl<PVRProvider, PVR_PROVIDER>
{
  friend class CInstancePVRClient;

public:
  /*! \cond PRIVATE */
  PVRProvider() { memset(m_cStructure, 0, sizeof(PVR_PROVIDER)); }
  PVRProvider(const PVRProvider& provider) : CStructHdl(provider) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_pvr_Defs_PVRProvider_Help Value Help
  /// @ingroup cpp_kodi_addon_pvr_Defs_PVRProvider
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_pvr_Defs_PVRProvider :</b>
  /// | Name | Type | Set call | Get call | Usage
  /// |------|------|----------|----------|-----------
  /// | **Unique id** | `unsigned int` | @ref PVRProvider::SetUniqueId "SetUniqueId" | @ref PVRProvider::GetUniqueId "GetUniqueId" | *required to set*
  /// | **Provider name** | `std::string` | @ref PVRProvider::SetName "SetName" | @ref PVRProvider::GetName "GetName" | *required to set*
  /// | **Provider type** | @ref PVR_PROVIDER_TYPE | @ref PVRProvider::SetType "SetType" | @ref PVRProvider::GetType "GetType" | *optional*
  /// | **Icon path** | `std::string` | @ref PVRProvider::SetIconPath "SetIconPath" | @ref PVRProvider::GetIconPath "GetIconPath" | *optional*
  /// | **Countries** | `std::vector<std::string>` | @ref PVRProvider::SetCountries "SetCountries" | @ref PVRProvider::GetCountries "GetCountries" | *optional*
  /// | **Languages** | `std::vector<std::string>` | @ref PVRProvider::SetLanguages "SetLanguages" | @ref PVRProvider::GetLanguages "GetLanguages" | *optional*
  ///

  /// @addtogroup cpp_kodi_addon_pvr_Defs_PVRProvider
  ///@{

  /// @brief **required**\n
  /// Unique identifier for this provider.
  void SetUniqueId(unsigned int uniqueId) { m_cStructure->iUniqueId = uniqueId; }

  /// @brief To get with @ref SetUniqueId changed values.
  unsigned int GetUniqueId() const { return m_cStructure->iUniqueId; }

  /// @brief **required**\n
  /// Name given to this provider.
  void SetName(const std::string& name)
  {
    strncpy(m_cStructure->strName, name.c_str(), sizeof(m_cStructure->strName) - 1);
  }

  /// @brief To get with @ref SetName changed values.
  std::string GetName() const { return m_cStructure->strName; }

  /// @brief **optional**\n
  /// Provider type.
  ///
  /// Set to @ref PVR_PROVIDER_TYPE_UNKNOWN if the type cannot be
  /// determined.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// Example:
  /// ~~~~~~~~~~~~~{.cpp}
  /// kodi::addon::PVRProvider tag;
  /// tag.SetType(PVR_PROVIDER_TYPE_SATELLITE);
  /// ~~~~~~~~~~~~~
  ///
  void SetType(PVR_PROVIDER_TYPE type) { m_cStructure->type = type; }

  /// @brief To get with @ref SetType changed values
  PVR_PROVIDER_TYPE GetType() const { return m_cStructure->type; }

  /// @brief **optional**\n
  /// Path to the provider icon (if present).
  void SetIconPath(const std::string& iconPath)
  {
    strncpy(m_cStructure->strIconPath, iconPath.c_str(), sizeof(m_cStructure->strIconPath) - 1);
  }

  /// @brief To get with @ref SetIconPath changed values.
  std::string GetIconPath() const { return m_cStructure->strIconPath; }
  ///@}

  /// @brief **optional**\n
  /// The country codes for the provider.
  ///
  /// @note ISO 3166 country codes required (e.g 'GB,IE,CA').
  void SetCountries(const std::vector<std::string>& countries)
  {
    const std::string str = tools::StringUtils::Join(countries, PROVIDER_STRING_TOKEN_SEPARATOR);
    strncpy(m_cStructure->strCountries, str.c_str(), sizeof(m_cStructure->strCountries) - 1);
  }

  /// @brief To get with @ref SetCountries changed values.
  std::vector<std::string> GetCountries() const
  {
    return tools::StringUtils::Split(m_cStructure->strCountries, PROVIDER_STRING_TOKEN_SEPARATOR);
  }
  ///@}

  /// @brief **optional**\n
  /// The language codes for the provider.
  ///
  /// @note RFC 5646 standard codes required (e.g.: 'en_GB,fr_CA').
  void SetLanguages(const std::vector<std::string>& languages)
  {
    const std::string str = tools::StringUtils::Join(languages, PROVIDER_STRING_TOKEN_SEPARATOR);
    strncpy(m_cStructure->strLanguages, str.c_str(), sizeof(m_cStructure->strLanguages) - 1);
  }

  /// @brief To get with @ref SetLanguages changed values.
  std::vector<std::string> GetLanguages() const
  {
    return tools::StringUtils::Split(m_cStructure->strLanguages, PROVIDER_STRING_TOKEN_SEPARATOR);
  }
  ///@}

private:
  PVRProvider(const PVR_PROVIDER* provider) : CStructHdl(provider) {}
  PVRProvider(PVR_PROVIDER* provider) : CStructHdl(provider) {}
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_PVRProvidersResultSet class PVRProvidersResultSet
/// @ingroup cpp_kodi_addon_pvr_Defs_PVRProvider
/// @brief **PVR add-on provider transfer class**\n
/// To transfer the content of @ref kodi::addon::CInstancePVRClient::GetProviders().
///
///@{
class PVRProvidersResultSet
{
public:
  /*! \cond PRIVATE */
  PVRProvidersResultSet() = delete;
  PVRProvidersResultSet(const AddonInstance_PVR* instance, PVR_HANDLE handle)
    : m_instance(instance), m_handle(handle)
  {
  }
  /*! \endcond */

  /// @addtogroup cpp_kodi_addon_pvr_Defs_PVRProvidersResultSet
  ///@{

  /// @brief To add and give content from addon to Kodi on related call.
  ///
  /// @param[in] provider The to transferred data.
  void Add(const kodi::addon::PVRProvider& provider)
  {
    m_instance->toKodi->TransferProviderEntry(m_instance->toKodi->kodiInstance, m_handle, provider);
  }

  ///@}

private:
  const AddonInstance_PVR* m_instance = nullptr;
  const PVR_HANDLE m_handle;
};
///@}
//------------------------------------------------------------------------------

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
