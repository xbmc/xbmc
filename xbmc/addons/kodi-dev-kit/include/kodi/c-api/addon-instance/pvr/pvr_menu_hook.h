/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDONINSTANCE_PVR_MENUHOOK_H
#define C_API_ADDONINSTANCE_PVR_MENUHOOK_H

#include "pvr_defines.h"

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C" Definitions group 7 - Menu hook
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_Defs_Menuhook_PVR_MENUHOOK_CAT enum PVR_MENUHOOK_CAT
  /// @ingroup cpp_kodi_addon_pvr_Defs_Menuhook
  /// @brief **PVR context menu hook categories**\n
  /// Possible menu types given to Kodi with @ref kodi::addon::CInstancePVRClient::AddMenuHook().
  ///
  ///@{
  typedef enum PVR_MENUHOOK_CAT
  {
    /// @brief __-1__ : Unknown menu hook.
    PVR_MENUHOOK_UNKNOWN = -1,

    /// @brief __0__ : All categories.
    PVR_MENUHOOK_ALL = 0,

    /// @brief __1__ : For channels.
    PVR_MENUHOOK_CHANNEL = 1,

    /// @brief __2__ : For timers.
    PVR_MENUHOOK_TIMER = 2,

    /// @brief __3__ : For EPG.
    PVR_MENUHOOK_EPG = 3,

    /// @brief __4__ : For recordings.
    PVR_MENUHOOK_RECORDING = 4,

    /// @brief __5__ : For deleted recordings.
    PVR_MENUHOOK_DELETED_RECORDING = 5,

    /// @brief __6__ : For settings.
    PVR_MENUHOOK_SETTING = 6,
  } PVR_MENUHOOK_CAT;
  ///@}
  //----------------------------------------------------------------------------

  /*!
   * @brief "C" PVR add-on menu hook.
   *
   * Structure used to interface in "C" between Kodi and Addon.
   *
   * See @ref kodi::addon::PVRMenuhook for description of values.
   */
  typedef struct PVR_MENUHOOK
  {
    unsigned int iHookId;
    unsigned int iLocalizedStringId;
    enum PVR_MENUHOOK_CAT category;
  } PVR_MENUHOOK;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_PVR_MENUHOOK_H */
