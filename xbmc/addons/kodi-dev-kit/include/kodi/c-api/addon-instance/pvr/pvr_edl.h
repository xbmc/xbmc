/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDONINSTANCE_PVR_EDL_H
#define C_API_ADDONINSTANCE_PVR_EDL_H

#include "pvr_defines.h"

#include <stdint.h>

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C" Definitions group 8 - PVR Edit definition list (EDL)
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_Defs_EDLEntry_PVR_EDL_TYPE enum PVR_EDL_TYPE
  /// @ingroup cpp_kodi_addon_pvr_Defs_EDLEntry
  /// @brief **Edit definition list types**\n
  /// Possible type values for @ref cpp_kodi_addon_pvr_Defs_EDLEntry_PVREDLEntry.
  ///
  ///@{
  typedef enum PVR_EDL_TYPE
  {
    /// @brief __0__  : cut (completely remove content)
    PVR_EDL_TYPE_CUT = 0,

    /// @brief __1__  : mute audio
    PVR_EDL_TYPE_MUTE = 1,

    /// @brief __2__  : scene markers (chapter seeking)
    PVR_EDL_TYPE_SCENE = 2,

    /// @brief __3__  : commercial breaks
    PVR_EDL_TYPE_COMBREAK = 3
  } PVR_EDL_TYPE;
  ///@}
  //----------------------------------------------------------------------------

  /*!
   * @brief "C" Edit definition list entry.
   *
   * Structure used to interface in "C" between Kodi and Addon.
   *
   * See @ref kodi::addon::PVREDLEntry for description of values.
   */
  typedef struct PVR_EDL_ENTRY
  {
    int64_t start;
    int64_t end;
    enum PVR_EDL_TYPE type;
  } PVR_EDL_ENTRY;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_PVR_EDL_H */
