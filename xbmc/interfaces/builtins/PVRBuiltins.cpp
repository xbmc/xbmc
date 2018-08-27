/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRBuiltins.h"

#include <algorithm>
#include <cstdlib>

#include "Application.h"
#include "guilib/GUIComponent.h"
#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "utils/log.h"

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

/*! \brief seeks to the given percentage in timeshift buffer, if timeshifting is supported.
 *  \param params The parameters
 *  \details params[0] = percentage to seek to in the timeshift buffer.
 */
static int SeekPercentage(const std::vector<std::string>& params)
{
  if (params.empty())
  {
    CLog::Log(LOGERROR,"PVR.SeekPercentage(n) - No argument given");
  }
  else
  {
    const float fTimeshiftPercentage = static_cast<float>(std::atof(params.front().c_str()));
    if (fTimeshiftPercentage < 0 || fTimeshiftPercentage > 100)
    {
      CLog::Log(LOGERROR,"PVR.SeekPercentage(n) - Invalid argument (%f), must be in range 0-100", fTimeshiftPercentage);
    }
    else if (g_application.GetAppPlayer().IsPlaying())
    {
      CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();

      int iTimeshiftProgressDuration = 0;
      infoMgr.GetInt(iTimeshiftProgressDuration, PVR_TIMESHIFT_PROGRESS_DURATION);

      int iTimeshiftBufferStart = 0;
      infoMgr.GetInt(iTimeshiftBufferStart, PVR_TIMESHIFT_PROGRESS_BUFFER_START);

      float fPlayerPercentage = static_cast<float>(iTimeshiftProgressDuration) / g_application.GetTotalTime() * (fTimeshiftPercentage - iTimeshiftBufferStart);
      fPlayerPercentage = std::max(0.0f, std::min(fPlayerPercentage, 100.0f));

      g_application.SeekPercentage(fPlayerPercentage);
    }
  }
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
///     Will toggle recording on playing channel\, if any
///   }
///   \table_row2_l{
///     <b>`PVR.SeekPercentage`</b>
///     ,
///     Performs a seek to the given percentage in timeshift buffer\, if timeshifting is supported
///   }
/// \table_end
///

CBuiltins::CommandMap CPVRBuiltins::GetOperations() const
{
  return {
           {"pvr.searchmissingchannelicons",  {"Search for missing channel icons", 0, SearchMissingIcons}},
           {"pvr.togglerecordplayingchannel", {"Toggle recording on playing channel", 0, ToggleRecordPlayingChannel}},
           {"pvr.seekpercentage",             {"Performs a seek to the given percentage in timeshift buffer", 1, SeekPercentage}},
         };
}
