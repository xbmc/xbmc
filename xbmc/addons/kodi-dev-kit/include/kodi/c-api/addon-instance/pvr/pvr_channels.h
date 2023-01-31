/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDONINSTANCE_PVR_CHANNELS_H
#define C_API_ADDONINSTANCE_PVR_CHANNELS_H

#include "pvr_defines.h"

#include <stdbool.h>

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C" Definitions group 2 - PVR channel
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  //============================================================================
  /// @ingroup cpp_kodi_addon_pvr_Defs_Channel
  /// @brief Denotes that no channel uid is available.
  ///
  /// Special @ref kodi::addon::PVRTimer::SetClientChannelUid() and
  /// @ref kodi::addon::PVRRecording::SetChannelUid() value to indicate that no
  /// channel uid is available.
#define PVR_CHANNEL_INVALID_UID -1
  //----------------------------------------------------------------------------

  /*!
   * @brief "C" PVR add-on channel.
   *
   * Structure used to interface in "C" between Kodi and Addon.
   *
   * See @ref kodi::addon::PVRChannel for description of values.
   */
  typedef struct PVR_CHANNEL
  {
    unsigned int iUniqueId;
    bool bIsRadio;
    unsigned int iChannelNumber;
    unsigned int iSubChannelNumber;
    char strChannelName[PVR_ADDON_NAME_STRING_LENGTH];
    char strMimeType[PVR_ADDON_INPUT_FORMAT_STRING_LENGTH];
    unsigned int iEncryptionSystem;
    char strIconPath[PVR_ADDON_URL_STRING_LENGTH];
    bool bIsHidden;
    bool bHasArchive;
    int iOrder;
    int iClientProviderUid;
  } PVR_CHANNEL;

  /*!
   * @brief "C" PVR add-on signal status information.
   *
   * Structure used to interface in "C" between Kodi and Addon.
   *
   * See @ref kodi::addon::PVRSignalStatus for description of values.
   */
  typedef struct PVR_SIGNAL_STATUS
  {
    char strAdapterName[PVR_ADDON_NAME_STRING_LENGTH];
    char strAdapterStatus[PVR_ADDON_NAME_STRING_LENGTH];
    char strServiceName[PVR_ADDON_NAME_STRING_LENGTH];
    char strProviderName[PVR_ADDON_NAME_STRING_LENGTH];
    char strMuxName[PVR_ADDON_NAME_STRING_LENGTH];
    int iSNR;
    int iSignal;
    long iBER;
    long iUNC;
  } PVR_SIGNAL_STATUS;

  //============================================================================
  /// @ingroup cpp_kodi_addon_pvr_Defs_Channel_PVRDescrambleInfo
  /// @brief Special @ref cpp_kodi_addon_pvr_Defs_Channel_PVRDescrambleInfo
  /// value to indicate that a struct member's value is not available
  ///
#define PVR_DESCRAMBLE_INFO_NOT_AVAILABLE -1
  //----------------------------------------------------------------------------

  /*!
   * @brief "C" PVR add-on descramble information.
   *
   * Structure used to interface in "C" between Kodi and Addon.
   *
   * See @ref kodi::addon::PVRDescrambleInfo for description of values.
   */
  typedef struct PVR_DESCRAMBLE_INFO
  {
    int iPid;
    int iCaid;
    int iProvid;
    int iEcmTime;
    int iHops;
    char strCardSystem[PVR_ADDON_DESCRAMBLE_INFO_STRING_LENGTH];
    char strReader[PVR_ADDON_DESCRAMBLE_INFO_STRING_LENGTH];
    char strFrom[PVR_ADDON_DESCRAMBLE_INFO_STRING_LENGTH];
    char strProtocol[PVR_ADDON_DESCRAMBLE_INFO_STRING_LENGTH];
  } PVR_DESCRAMBLE_INFO;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_PVR_CHANNELS_H */
