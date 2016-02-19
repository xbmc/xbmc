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

namespace V2
{
namespace KodiAPI
{

namespace PVR
{

  //============================================================================
  ///
  /// \defgroup CPP_V2_KodiAPI_PVR_Trigger
  /// \ingroup CPP_V2_KodiAPI_PVR
  /// @{
  /// @brief <b>Funtions to trigger PVR related updates on Kodi</b>
  ///
  /// Call of function in this class informs Kodi to do a update of related
  /// parts.
  ///
  /// These are pure static functions them no other initialization need.
  ///
  /// It has the header \ref Trigger.h "#include <kodi/api2/pvr/Trigger.h>" be
  /// included to enjoy it
  ///
  namespace Trigger
  {
    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_PVR_Trigger
    /// @brief Request Kodi to update it's list of timers
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Code Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api2/pvr/Trigger.h>
    ///
    /// ...
    /// KodiAPI::PVR::Trigger::TimerUpdate();
    /// ~~~~~~~~~~~~~
    ///
    void TimerUpdate(void);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_PVR_Trigger
    /// @brief Request Kodi to update it's list of recordings
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Code Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api2/pvr/Trigger.h>
    ///
    /// ...
    /// KodiAPI::PVR::Trigger::RecordingUpdate();
    /// ~~~~~~~~~~~~~
    ///
    void RecordingUpdate(void);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_PVR_Trigger
    /// @brief Request Kodi to update it's list of channels
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Code Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api2/pvr/Trigger.h>
    ///
    /// ...
    /// KodiAPI::PVR::Trigger::ChannelUpdate();
    /// ~~~~~~~~~~~~~
    ///
    void ChannelUpdate(void);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_PVR_Trigger
    /// @brief Request Kodi to update it's list of channel groups
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Code Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api2/pvr/Trigger.h>
    ///
    /// ...
    /// KodiAPI::PVR::Trigger::ChannelGroupsUpdate();
    /// ~~~~~~~~~~~~~
    ///
    void ChannelGroupsUpdate(void);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_PVR_Trigger
    /// @brief Schedule an EPG update for the given channel channel
    ///
    /// @param[in] iChannelUid The unique id of the channel for this add-on
    ///
    void EpgUpdate(unsigned int iChannelUid);
    //--------------------------------------------------------------------------

  };
  /// @}

}; /* namespace PVR */

}; /* namespace KodiAPI */
}; /* namespace V2 */
