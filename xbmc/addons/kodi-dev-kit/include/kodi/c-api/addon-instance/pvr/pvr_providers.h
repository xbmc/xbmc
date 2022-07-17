/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDONINSTANCE_PVR_PROVIDERS_H
#define C_API_ADDONINSTANCE_PVR_PROVIDERS_H

#include "pvr_defines.h"

#include <stdbool.h>

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C" Definitions group 2 - PVR providers
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  //============================================================================
  /// @ingroup cpp_kodi_addon_pvr_Defs_Provider
  /// @brief Denotes that no provider uid is available.
  ///
  /// Special @ref kodi::addon::PVRChannel::SetClientProviderUid()
#define PVR_PROVIDER_INVALID_UID -1
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_pvr_Defs_Provider
  /// @brief Separator to use in strings containing different tokens, for example
  /// country and language.
  ///
#define PROVIDER_STRING_TOKEN_SEPARATOR ","

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_Defs_Channel_PVR_PROVIDER_TYPE enum PVR_PROVIDER_TYPE
  /// @ingroup cpp_kodi_addon_pvr_Defs_Channel
  /// @brief **PVR provider types**\n
  /// Used on @ref kodi::addon::PVRProvider:SetProviderType() value to set related
  /// type.
  ///
  ///@{
  typedef enum PVR_PROVIDER_TYPE
  {
    /// @brief __0__ : Unknown type.
    PVR_PROVIDER_TYPE_UNKNOWN = 0,

    /// @brief __1__ : IPTV provider.
    PVR_PROVIDER_TYPE_ADDON = 1,

    /// @brief __2__ : Satellite provider.
    PVR_PROVIDER_TYPE_SATELLITE = 2,

    /// @brief __3__ : Cable provider.
    PVR_PROVIDER_TYPE_CABLE = 3,

    /// @brief __4__ : Aerial provider.
    PVR_PROVIDER_TYPE_AERIAL = 4,

    /// @brief __5__ : IPTV provider.
    PVR_PROVIDER_TYPE_IPTV = 5,

    /// @brief __6__ : Other type of provider.
    PVR_PROVIDER_TYPE_OTHER = 6,
  } PVR_PROVIDER_TYPE;
  ///@}
  //----------------------------------------------------------------------------

  /*!
   * @brief "C" PVR add-on provider.
   *
   * Structure used to interface in "C" between Kodi and Addon.
   *
   * See @ref kodi::addon::PVRProvider for description of values.
   */
  typedef struct PVR_PROVIDER
  {
    unsigned int iUniqueId;
    char strName[PVR_ADDON_NAME_STRING_LENGTH];
    enum PVR_PROVIDER_TYPE type;
    char strIconPath[PVR_ADDON_URL_STRING_LENGTH];
    //! @brief ISO 3166 country codes, separated by PROVIDER_STRING_TOKEN_SEPARATOR
    /// (e.g 'GB,IE,FR,CA'), an empty string means this value is undefined
    char strCountries[PVR_ADDON_COUNTRIES_STRING_LENGTH];
    //! @brief RFC 5646 language codes, separated by PROVIDER_STRING_TOKEN_SEPARATOR
    /// (e.g. 'en_GB,fr_CA'), an empty string means this value is undefined
    char strLanguages[PVR_ADDON_LANGUAGES_STRING_LENGTH];
  } PVR_PROVIDER;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_PVR_PROVIDERS_H */
