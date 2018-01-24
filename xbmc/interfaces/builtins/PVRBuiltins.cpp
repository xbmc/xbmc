/*
 *      Copyright (C) 2005-2015 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PVRBuiltins.h"

#include "Application.h"
#include "ServiceBroker.h"
#include "pvr/PVRGUIActions.h"
#include "pvr/PVRManager.h"

/*! \brief Search for missing channel icons
 *   \param params (ignored)
 */
static int SearchMissingIcons(const std::vector<std::string>& params)
{
  CServiceBroker::GetPVRManager().TriggerSearchMissingChannelIcons();
  return 0;
}

/*! \brief will toggle recording of playing channel, if any.
 *   \param params (ignored)
 */
static int ToggleRecordPlayingChannel(const std::vector<std::string>& params)
{
  CServiceBroker::GetPVRManager().GUIActions()->ToggleRecordingOnPlayingChannel();
  return 0;
}

// Note: For new Texts with comma add a "\" before!!! Is used for table text.
//
/// \page page_List_of_built_in_functions
/// \section built_in_functions_10 PVR built-in's
///
/// -----------------------------------------------------------------------------
///
/// \table_start
///   \table_h2_l{
///     Function,
///     Description }
///   \table_row2_l{
///     <b>`PVR.SearchMissingChannelIcons`</b>
///     ,
///     Will start a search for missing channel icons
///   }
///   \table_row2_l{
///     <b>`PVR.ToggleRecordPlayingChannel`</b>
///     ,
///     Will toggle recording on playing channel, if any
///   }
/// \table_end
///

CBuiltins::CommandMap CPVRBuiltins::GetOperations() const
{
  return {
           {"pvr.searchmissingchannelicons",  {"Search for missing channel icons", 0, SearchMissingIcons}},
           {"pvr.togglerecordplayingchannel", {"Toggle recording on playing channel", 0, ToggleRecordPlayingChannel}},
         };
}
