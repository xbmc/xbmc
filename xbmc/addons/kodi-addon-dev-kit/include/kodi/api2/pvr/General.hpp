#pragma once
/*
 *      Copyright (C) 2015-2016 Team KODI
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "../definitions.hpp"
#include "kodi/xbmc_pvr_types.h"

API_NAMESPACE

namespace KodiAPI
{
namespace PVR
{

  //============================================================================
  ///
  /// \defgroup CPP_KodiAPI_PVR_General General
  /// \ingroup CPP_KodiAPI_PVR
  /// @{
  /// @brief <b>Allow use of binary classes and function to use on add-on's</b>
  ///
  /// Permits the use of the required functions of the add-on to Kodi. This class
  /// also contains some functions to the control.
  ///
  /// These are pure static functions them no other initialization need.
  ///
  /// It has the header \ref kodi/api2/pvr/General.hpp "#include <kodi/api2/pvr/General.hpp>" be included
  /// to enjoy it
  ///
  namespace General
  {
    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_PVR_General
    /// @brief Add or replace a menu hook for the context menu for this add-on
    ///
    /// Inserted hooks becomes called from Kodi on PVR system with
    /// <b><tt>PVR_ERROR CallMenuHook(const PVR_MENUHOOK& menuhook, const PVR_MENUHOOK_DATA &item);</tt></b>
    ///
    /// @param[in] hook The hook to add
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// Here structure shows the necessary values the Kodi must be handed over
    /// around a menu by the PVR system to add.
    /// ~~~~~~~~~~~~~{.cpp}
    /// /*
    ///  * Menu hooks that are available in the context menus while playing a
    ///  * stream via this add-on and in the Live TV settings dialog
    ///  *
    ///  * Only as example shown here! See always the original structure on related header.
    ///  */
    /// typedef struct PVR_MENUHOOK
    /// {
    ///   unsigned int     iHookId;              /* This hook's identifier */
    ///   unsigned int     iLocalizedStringId;   /* The id of the label for this hook in g_localizeStrings */
    ///   PVR_MENUHOOK_CAT category;             /* Category of menu hook */
    /// } ATTRIBUTE_PACKED PVR_MENUHOOK;
    /// ~~~~~~~~~~~~~
    ///
    /// ------------------------------------------------------------------------
    ///
    /// <b>Hook category Id's (PVR_MENUHOOK_CAT):</b>
    ///
    /// |  enum code:                    | Id | Description:           |
    /// |-------------------------------:|:--:|:-----------------------|
    /// | PVR_MENUHOOK_ALL               | 0  | All categories         |
    /// | PVR_MENUHOOK_CHANNEL           | 1  | For channels           |
    /// | PVR_MENUHOOK_TIMER             | 2  | For timers             |
    /// | PVR_MENUHOOK_EPG               | 3  | For EPG                |
    /// | PVR_MENUHOOK_RECORDING         | 4  | For recordings         |
    /// | PVR_MENUHOOK_DELETED_RECORDING | 5  | For deleted recordings |
    /// | PVR_MENUHOOK_SETTING           | 6  | For settings           |
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api2/pvr/General.hpp>
    /// ...
    /// PVR_MENUHOOK hook;
    /// hook.iHookId = 1;
    /// hook.category = PVR_MENUHOOK_SETTING;
    /// hook.iLocalizedStringId = 30107;
    /// KodiAPI::PVR::General::AddMenuHook(&hook);
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    void AddMenuHook(
                         PVR_MENUHOOK*      hook);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_PVR_General
    /// @brief Display a notification in Kodi that a recording started or stopped
    ///        on the server
    ///
    /// @param[in] strRecordingName   The name of the recording to display
    /// @param[in] strFileName        The filename of the recording
    /// @param[in] bOn                True when recording started, false when it
    ///                               stopped
    ///
    void Recording(
                         const std::string& strRecordingName,
                         const std::string& strFileName,
                         bool               bOn);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_PVR_General
    /// @brief Notify a state change for a PVR backend connection
    ///
    /// @param strConnectionString    The connection string reported by the
    ///                               backend that can be displayed in the UI.
    /// @param newState               The new state.
    /// @param strMessage             A localized addon-defined string representing
    ///                               the new state, that can be displayed in the
    ///                               UI or NULL if the Kodi-defined default
    ///                               string for the new state shall be displayed.
    ///
    void ConnectionStateChange(
                         const std::string&   strConnectionString,
                         PVR_CONNECTION_STATE newState,
                         const std::string&   strMessage);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_PVR_General
    /// @brief Notify a state change for an EPG event
    ///
    /// @param tag                    The EPG event.
    /// @param iUniqueChannelId       The unique id of the channel for the EPG
    ///                               event
    /// @param newState               The new state. For EPG_EVENT_CREATED and
    ///                               EPG_EVENT_UPDATED, tag must be filled with
    ///                               all available event data, not just a delta.
    ///                               For EPG_EVENT_DELETED, it is sufficient to
    ///                               fill EPG_TAG.iUniqueBroadcastId
    ///
    void EpgEventStateChange(
                         EPG_TAG*             tag,
                         unsigned int         iUniqueChannelId,
                         EPG_EVENT_STATE      newState);
    //--------------------------------------------------------------------------
  };
  /// @}

} /* namespace PVR */
} /* namespace KodiAPI */

END_NAMESPACE()
