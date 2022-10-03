/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRBuiltins.h"

#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "pvr/PVRManager.h"
#include "pvr/guilib/PVRGUIActionsTimers.h"
#include "pvr/windows/GUIWindowPVRGuide.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <cstdlib>
#include <regex>

using namespace PVR;

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
  CServiceBroker::GetPVRManager().Get<PVR::GUI::Timers>().ToggleRecordingOnPlayingChannel();
  return 0;
}

/*! \brief seeks to the given percentage in timeshift buffer, if timeshifting is supported.
 *  \param params The parameters
 *  \details params[0] = percentage to seek to in the timeshift buffer.
 */
static int SeekPercentage(const std::vector<std::string>& params)
{
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  if (params.empty())
  {
    CLog::Log(LOGERROR,"PVR.SeekPercentage(n) - No argument given");
  }
  else
  {
    const float fTimeshiftPercentage = static_cast<float>(std::atof(params.front().c_str()));
    if (fTimeshiftPercentage < 0 || fTimeshiftPercentage > 100)
    {
      CLog::Log(LOGERROR, "PVR.SeekPercentage(n) - Invalid argument ({:f}), must be in range 0-100",
                fTimeshiftPercentage);
    }
    else if (appPlayer->IsPlaying())
    {
      CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();

      int iTimeshiftProgressDuration = 0;
      infoMgr.GetInt(iTimeshiftProgressDuration, PVR_TIMESHIFT_PROGRESS_DURATION,
                     INFO::DEFAULT_CONTEXT);

      int iTimeshiftBufferStart = 0;
      infoMgr.GetInt(iTimeshiftBufferStart, PVR_TIMESHIFT_PROGRESS_BUFFER_START,
                     INFO::DEFAULT_CONTEXT);

      float fPlayerPercentage = static_cast<float>(iTimeshiftProgressDuration) /
                                static_cast<float>(g_application.GetTotalTime()) *
                                (fTimeshiftPercentage - static_cast<float>(iTimeshiftBufferStart));
      fPlayerPercentage = std::max(0.0f, std::min(fPlayerPercentage, 100.0f));

      g_application.SeekPercentage(fPlayerPercentage);
    }
  }
  return 0;
}

namespace
{
/*! \brief Control PVR Guide window's EPG grid.
 *  \param params The parameters
 *  \details params[0] = Control to execute.
 */
int EpgGridControl(const std::vector<std::string>& params)
{
  if (params.empty())
  {
    CLog::Log(LOGERROR, "EpgGridControl(n) - No argument given");
    return 0;
  }

  int activeWindow = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
  if (activeWindow != WINDOW_TV_GUIDE && activeWindow != WINDOW_RADIO_GUIDE)
  {
    CLog::Log(LOGERROR, "EpgGridControl(n) - Guide window not active");
    return 0;
  }

  CGUIWindowPVRGuideBase* guideWindow = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIWindowPVRGuideBase>(activeWindow);
  if (!guideWindow)
  {
    CLog::Log(LOGERROR, "EpgGridControl(n) - Unable to get Guide window instance");
    return 0;
  }

  std::string param(params[0]);
  StringUtils::ToLower(param);

  if (param == "firstprogramme")
  {
    guideWindow->GotoBegin();
  }
  else if (param == "lastprogramme")
  {
    guideWindow->GotoEnd();
  }
  else if (param == "currentprogramme")
  {
    guideWindow->GotoCurrentProgramme();
  }
  else if (param == "selectdate")
  {
    guideWindow->OpenDateSelectionDialog();
  }
  else if (StringUtils::StartsWithNoCase(param, "+") || StringUtils::StartsWithNoCase(param, "-"))
  {
    // jump back/forward n hours
    if (std::regex_match(param, std::regex("[(-|+)|][0-9]+")))
    {
      guideWindow->GotoDate(std::atoi(param.c_str()));
    }
    else
    {
      CLog::Log(LOGERROR, "EpgGridControl(n) - invalid argument");
    }
  }
  else if (param == "firstchannel")
  {
    guideWindow->GotoFirstChannel();
  }
  else if (param == "playingchannel")
  {
    guideWindow->GotoPlayingChannel();
  }
  else if (param == "lastchannel")
  {
    guideWindow->GotoLastChannel();
  }
  else if (param == "previousgroup")
  {
    guideWindow->ActivatePreviousChannelGroup();
  }
  else if (param == "nextgroup")
  {
    guideWindow->ActivateNextChannelGroup();
  }
  else if (param == "selectgroup")
  {
    guideWindow->OpenChannelGroupSelectionDialog();
  }

  return 0;
}

} // unnamed namespace

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
///   \table_row2_l{
///     <b>`PVR.EpgGridControl`</b>
///     ,
///     Control PVR Guide window's EPG grid
///   }
/// \table_end
///

CBuiltins::CommandMap CPVRBuiltins::GetOperations() const
{
  return {
           {"pvr.searchmissingchannelicons",  {"Search for missing channel icons", 0, SearchMissingIcons}},
           {"pvr.togglerecordplayingchannel", {"Toggle recording on playing channel", 0, ToggleRecordPlayingChannel}},
           {"pvr.seekpercentage",             {"Performs a seek to the given percentage in timeshift buffer", 1, SeekPercentage}},
           {"pvr.epggridcontrol",             {"Control PVR Guide window's EPG grid", 1, EpgGridControl}},
         };
}
