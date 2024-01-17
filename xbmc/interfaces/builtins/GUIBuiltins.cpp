/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIBuiltins.h"

#include "ServiceBroker.h"
#include "Util.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPowerHandling.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogNumeric.h"
#include "filesystem/Directory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/StereoscopicsManager.h"
#include "input/WindowTranslator.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "input/actions/ActionTranslator.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/AlarmClock.h"
#include "utils/RssManager.h"
#include "utils/Screenshot.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "windows/GUIMediaWindow.h"

using namespace KODI;

namespace
{
/*! \brief Execute a GUI action.
 *  \param params The parameters.
 *  \details params[0] = Action to execute.
 *           params[1] = Window to send action to (optional).
 */
static int Action(const std::vector<std::string>& params)
{
  // try translating the action from our ActionTranslator
  unsigned int actionID;
  if (ACTION::CActionTranslator::TranslateString(params[0], actionID))
  {
    int windowID = params.size() == 2 ? CWindowTranslator::TranslateWindow(params[1]) : WINDOW_INVALID;
    CServiceBroker::GetAppMessenger()->SendMsg(TMSG_GUI_ACTION, windowID, -1,
                                               static_cast<void*>(new CAction(actionID)));
  }

  return 0;
}

/*! \brief Activate a window.
 *  \param params The parameters.
 *  \details params[0] = The window name.
 *           params[1] = Window starting folder (optional).
 *
 *           Set the Replace template parameter to true to replace current
 *           window in history.
 */
  template<bool Replace>
static int ActivateWindow(const std::vector<std::string>& params2)
{
  std::vector<std::string> params(params2);
  // get the parameters
  std::string strWindow;
  if (params.size())
  {
    strWindow = params[0];
    params.erase(params.begin());
  }

  // confirm the window destination is valid prior to switching
  int iWindow = CWindowTranslator::TranslateWindow(strWindow);
  if (iWindow != WINDOW_INVALID)
  {
    // compare the given directory param with the current active directory
    // if no directory is given, and you switch from a video window to another
    // we retain history, so it makes sense to not switch to the same window in
    // that case
    bool bIsSameStartFolder = true;
    if (!params.empty())
    {
      CGUIWindow *activeWindow = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow());
      if (activeWindow && activeWindow->IsMediaWindow())
        bIsSameStartFolder = static_cast<CGUIMediaWindow*>(activeWindow)->IsSameStartFolder(params[0]);
    }

    // activate window only if window and path differ from the current active window
    if (iWindow != CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() || !bIsSameStartFolder)
    {
      // if the window doesn't change, make sure it knows it's gonna be replaced
      // this ensures setting the start directory if we switch paths
      // if we change windows, that's done anyway
      if (Replace && !params.empty() &&
          iWindow == CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow())
        params.emplace_back("replace");

      auto& components = CServiceBroker::GetAppComponents();
      const auto appPower = components.GetComponent<CApplicationPowerHandling>();
      appPower->WakeUpScreenSaverAndDPMS();
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(iWindow, params, Replace);
      return 0;
    }
  }
  else
  {
    CLog::Log(LOGERROR, "Activate/ReplaceWindow called with invalid destination window: {}",
              strWindow);
    return false;
  }

  return 1;
}

/*! \brief Activate a window and give given controls focus.
 *  \param params The parameters.
 *  \details params[0] = The window name.
 *           params[1,...] = Pair of (container ID, focus item).
 *
 *           Set the Replace template parameter to true to replace current
 *           window in history.
 */
  template<bool Replace>
static int ActivateAndFocus(const std::vector<std::string>& params)
{
  std::string strWindow = params[0];

  // confirm the window destination is valid prior to switching
  int iWindow = CWindowTranslator::TranslateWindow(strWindow);
  if (iWindow != WINDOW_INVALID)
  {
    if (iWindow != CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow())
    {
      // disable the screensaver
      auto& components = CServiceBroker::GetAppComponents();
      const auto appPower = components.GetComponent<CApplicationPowerHandling>();
      appPower->WakeUpScreenSaverAndDPMS();
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(iWindow, {}, Replace);

      unsigned int iPtr = 1;
      while (params.size() > iPtr + 1)
      {
        CGUIMessage msg(GUI_MSG_SETFOCUS, CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog(),
                        atol(params[iPtr].c_str()),
                        (params.size() >= iPtr + 2) ? atol(params[iPtr + 1].c_str())+1 : 0);
        CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
        iPtr += 2;
      }
      return 0;
    }

  }
  else
    CLog::Log(LOGERROR, "Replace/ActivateWindowAndFocus called with invalid destination window: {}",
              strWindow);

  return 1;
}

/*! \brief Start an alarm clock
 *  \param params The parameters.
 *  \details param[0] = name
 *           param[1] = command
 *           param[2] = Length in seconds (optional).
 *           param[3] = "silent" to suppress notifications.
 *           param[3] = "loop" to loop the alarm.
 */
static int AlarmClock(const std::vector<std::string>& params)
{
  // format is alarmclock(name,command[,time,true,false]);
  float seconds = 0;
  if (params.size() > 2)
  {
    if (params[2].find(':') == std::string::npos)
      seconds = static_cast<float>(atoi(params[2].c_str())*60);
    else
      seconds = (float)StringUtils::TimeStringToSeconds(params[2]);
  }
  else
  { // check if shutdown is specified in particular, and get the time for it
    std::string strHeading;
    if (StringUtils::EqualsNoCase(params[0], "shutdowntimer"))
      strHeading = g_localizeStrings.Get(20145);
    else
      strHeading = g_localizeStrings.Get(13209);
    std::string strTime;
    if( CGUIDialogNumeric::ShowAndGetNumber(strTime, strHeading) )
      seconds = static_cast<float>(atoi(strTime.c_str())*60);
    else
      return false;
  }
  bool silent = false;
  bool loop = false;
  for (unsigned int i = 3; i < params.size() ; i++)
  {
    // check "true" for backward comp
    if (StringUtils::EqualsNoCase(params[i], "true") || StringUtils::EqualsNoCase(params[i], "silent"))
      silent = true;
    else if (StringUtils::EqualsNoCase(params[i], "loop"))
      loop = true;
  }

  if( g_alarmClock.IsRunning() )
    g_alarmClock.Stop(params[0],silent);
  // no negative times not allowed, loop must have a positive time
  if (seconds < 0 || (seconds == 0 && loop))
    return false;
  g_alarmClock.Start(params[0], seconds, params[1], silent, loop);

  return 0;
}

/*! \brief Cancel an alarm clock.
 *  \param params The parameters.
 *  \details params[0] = "true" to silently cancel alarm (optional).
 */
static int CancelAlarm(const std::vector<std::string>& params)
{
  bool silent = (params.size() > 1 &&
      (StringUtils::EqualsNoCase(params[1], "true") ||
       StringUtils::EqualsNoCase(params[1], "silent")));
  g_alarmClock.Stop(params[0],silent);

  return 0;
}

/*! \brief Clear a property in a window.
 *  \param params The parameters.
 *  \details params[0] = The property to clear.
 *           params[1] = The window to clear property in (optional).
 */
static int ClearProperty(const std::vector<std::string>& params)
{
  CGUIWindow *window = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(params.size() > 1 ? CWindowTranslator::TranslateWindow(params[1]) : CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog());
  if (window)
    window->SetProperty(params[0],"");

  return 0;
}

/*! \brief Close a dialog.
 *  \param params The parameters.
 *  \details params[0] = "all" to close all dialogs, or dialog name.
 *           params[1] = "true" to force close (skip animations) (optional).
 */
static int CloseDialog(const std::vector<std::string>& params)
{
  bool bForce = false;
  if (params.size() > 1 && StringUtils::EqualsNoCase(params[1], "true"))
    bForce = true;
  if (StringUtils::EqualsNoCase(params[0], "all"))
  {
    CServiceBroker::GetGUI()->GetWindowManager().CloseDialogs(bForce);
  }
  else
  {
    int id = CWindowTranslator::TranslateWindow(params[0]);
    CGUIWindow *window = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(id);
    if (window && window->IsDialog())
      static_cast<CGUIDialog*>(window)->Close(bForce);
  }

  return 0;
}

/*! \brief Send a notification.
 *  \param params The parameters.
 *  \details params[0] = Notification title.
 *           params[1] = Notification text.
 *           params[2] = Display time in milliseconds (optional).
 *           params[3] = Notification icon (optional).
 */
static int Notification(const std::vector<std::string>& params)
{
  if (params.size() < 2)
    return -1;
  if (params.size() == 4)
    CGUIDialogKaiToast::QueueNotification(params[3],params[0],params[1],atoi(params[2].c_str()));
  else if (params.size() == 3)
    CGUIDialogKaiToast::QueueNotification("",params[0],params[1],atoi(params[2].c_str()));
  else
    CGUIDialogKaiToast::QueueNotification(params[0],params[1]);

  return 0;
}

/*! \brief Refresh RSS feed.
 *  \param params (ignored)
 */
static int RefreshRSS(const std::vector<std::string>& params)
{
  CRssManager::GetInstance().Reload();

  return 0;
}

/*! \brief Take a screenshot.
 *  \param params The parameters.
 *  \details params[0] = URL to save file to. Blank to use default.
 *           params[1] = "sync" to run synchronously (optional).
 */
static int Screenshot(const std::vector<std::string>& params)
{
  if (!params.empty())
  {
    // get the parameters
    std::string strSaveToPath = params[0];
    bool sync = false;
    if (params.size() >= 2)
      sync = StringUtils::EqualsNoCase(params[1], "sync");

    if (!strSaveToPath.empty())
    {
      if (XFILE::CDirectory::Exists(strSaveToPath))
      {
        std::string file = CUtil::GetNextFilename(
            URIUtils::AddFileToFolder(strSaveToPath, "screenshot{:05}.png"), 65535);

        if (!file.empty())
        {
          CScreenShot::TakeScreenshot(file, sync);
        }
        else
        {
          CLog::Log(LOGWARNING, "Too many screen shots or invalid folder {}", strSaveToPath);
        }
      }
      else
        CScreenShot::TakeScreenshot(strSaveToPath, sync);
    }
  }
  else
    CScreenShot::TakeScreenshot();

  return 0;
}

/*! \brief Set GUI language.
 *  \param params The parameters.
 *  \details params[0] = The language to use.
 */
static int SetLanguage(const std::vector<std::string>& params)
{
  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_SETLANGUAGE, -1, -1, nullptr, params[0]);

  return 0;
}

/*! \brief Set a property in a window.
 *  \param params The parameters.
 *  \details params[0] = The property to set.
 *           params[1] = The property value.
 *           params[2] = The window to set property in (optional).
 */
static int SetProperty(const std::vector<std::string>& params)
{
  CGUIWindow *window = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(params.size() > 2 ? CWindowTranslator::TranslateWindow(params[2]) : CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog());
  if (window)
    window->SetProperty(params[0],params[1]);

  return 0;
}

/*! \brief Set GUI stereo mode.
 *  \param params The parameters.
 *  \details param[0] = Stereo mode identifier.
 */
static int SetStereoMode(const std::vector<std::string>& params)
{
  CAction action = CStereoscopicsManager::ConvertActionCommandToAction("SetStereoMode", params[0]);
  if (action.GetID() != ACTION_NONE)
    CServiceBroker::GetAppMessenger()->SendMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1,
                                               static_cast<void*>(new CAction(action)));
  else
  {
    CLog::Log(LOGERROR, "Builtin 'SetStereoMode' called with unknown parameter: {}", params[0]);
    return -2;
  }

  return 0;
}

/*! \brief Toggle visualization of dirty regions.
 *  \param params Ignored.
 */
static int ToggleDirty(const std::vector<std::string>&)
{
  CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->ToggleDirtyRegionVisualization();

  return 0;
}
} // namespace

// Note: For new Texts with comma add a "\" before!!! Is used for table text.
//
/// \page page_List_of_built_in_functions
/// \section built_in_functions_5 GUI built-in's
///
/// -----------------------------------------------------------------------------
///
/// \table_start
///   \table_h2_l{
///     Function,
///     Description }
///   \table_row2_l{
///     <b>`Action(action[\,window])`</b>
///     ,
///     Executes an action (same as in keymap) for the given window or the
///     active window if the parameter window is omitted. The parameter window
///     can either be the window's id\, or in the case of a standard window\, the
///     window's name. See here for a list of window names\, and their respective
///     ids.
///     @param[in] action                Action to execute.
///     @param[in] window                Window to send action to (optional).
///   }
///   \table_row2_l{
///     <b>`CancelAlarm(name[\,silent])`</b>
///     ,
///     Cancel a running alarm. Set silent to true to hide the alarm notification.
///     @param[in] silent                Send "true" or "silent" to silently cancel alarm (optional).
///   }
///   \table_row2_l{
///     <b>`AlarmClock(name\,command[\,time\,silent\,loop])`</b>
///     ,
///     Pops up a dialog asking for the length of time for the alarm (unless the
///     parameter time is specified)\, and starts a timer. When the timer runs out\,
///     it'll execute the built-in command (the parameter command) if it is
///     specified\, otherwise it'll pop up an alarm notice. Add silent to hide the
///     alarm notification. Add loop for the alarm to execute the command each
///     time the specified time interval expires.
///     @note if using any of the last optional parameters (silent or loop)\, both must
///     be provided for any to take effect.
///     <p>
///     <b>Example:</b>
///     The following example will create an alarmclock named `mytimer` which will silently
///     fire (a single time) and set a property (timerelapsed) with value 1 in the window with
///     id 1109 after 5 seconds have passed.
///     ~~~~~~~~~~~~~
///     AlarmClock(mytimer\,SetProperty(timerelapsed\,1\,1109)\,00:00:05\,silent\,false])
///     ~~~~~~~~~~~~~
///     <p>
///     @param[in] name                  name
///     @param[in] command               command
///     @param[in] time                  [opt] <b>(a)</b> Length in minutes or <b>(b)</b> a timestring in the format `hh:mm:ss` or `mm min`.
///     @param[in] silent                [opt] Send "silent" to suppress notifications.
///     @param[in] loop                  [opt] Send "loop" to loop the alarm.
///   }
///   \table_row2_l{
///     <b>`ActivateWindow(window[\,dir\, return])`</b>
///     ,
///     Opens the given window. The parameter window can either be the window's id\,
///     or in the case of a standard window\, the window's name. See \ref window_ids "here" for a list
///     of window names\, and their respective ids.
///     If\, furthermore\, the window is
///     Music\, Video\, Pictures\, or Program files\, then the optional dir parameter
///     specifies which folder Kodi should default to once the window is opened.
///     This must be a source as specified in sources.xml\, or a subfolder of a
///     valid source. For some windows (MusicLibrary and VideoLibrary)\, a third
///     parameter (return) may be specified\, which indicates that Kodi should use this
///     folder as the "root" of the level\, and thus the "parent directory" action
///     from within this folder will return the user to where they were prior to
///     the window activating.
///     @param[in] window                The window name.
///     @param[in] dir                   Window starting folder (optional).
///     @param[in] return                if dir should be used as the rootfolder of the level
///   }
///   \table_row2_l{
///     <b>`ActivateWindowAndFocus(id1\, id2\,item1\, id3\,item2)`</b>
///     ,
///     Activate window with id1\, first focus control id2 and then focus control
///     id3. if either of the controls is a container\, you can specify which
///     item to focus (else\, set it to 0).
///     @param[in] id1                   The window name.
///     @param[in] params[1\,...]         Pair of (container ID\, focus item).
///   }
///   \table_row2_l{
///     <b>`ClearProperty(key[\,id])`</b>
///     ,
///     Clears a window property for the current focused window/dialog(key)\, or
///     the specified window (key\,id).
///     @param[in] key                   The property to clear.
///     @param[in] id                    The window to clear property in (optional).
///   }
///   \table_row2_l{
///     <b>`Dialog.Close(dialog[\,force])`</b>
///     ,
///     Close a dialog. Set force to true to bypass animations. Use (all\,true)
///     to close all opened dialogs at once.
///     @param[in] dialog                Send "all" to close all dialogs\, or dialog name.
///     @param[in] force                 Send "true" to force close (skip animations) (optional).
///   }
///   \table_row2_l{
///     <b>`Notification(header\,message[\,time\,image])`</b>
///     ,
///     Will display a notification dialog with the specified header and message\,
///     in addition you can set the length of time it displays in milliseconds
///     and a icon image.
///     @param[in] header                Notification title.
///     @param[in] message               Notification text.
///     @param[in] time                  Display time in milliseconds (optional).
///     @param[in] image                 Notification icon (optional).
///   }
///   \table_row2_l{
///     <b>`RefreshRSS`</b>
///     ,
///     Reload RSS feeds from RSSFeeds.xml
///   }
///   \table_row2_l{
///     <b>`ReplaceWindow(window\,dir)`</b>
///     ,
///     Replaces the current window with the given window. This is the same as
///     ActivateWindow() but it doesn't update the window history list\, so when
///     you go back from the new window it will not return to the previous
///     window\, rather will return to the previous window's previous window.
///     @param[in] window                The window name.
///     @param[in] dir                   Window starting folder (optional).
///   }
///   \table_row2_l{
///     <b>`ReplaceWindowAndFocus(id1\, id2\,item1\, id3\,item2)`</b>
///     ,
///     Replace window with id1\, first focus control id2 and then focus control
///     id3. if either of the controls is a container\, you can specify which
///     item to focus (else\, set it to 0).
///     @param[in] id1                   The window name.
///     @param[in] params[1\,...]        Pair of (container ID\, focus item).
///   }
///   \table_row2_l{
///     <b>`Resolution(resIdent)`</b>
///     ,
///     Change Kodi's Resolution (default is 4x3).
///     param[in] resIdent               A resolution identifier.
///     |          | Identifiers |          |
///     |:--------:|:-----------:|:--------:|
///     | pal      | pal16x9     | ntsc     |
///     | ntsc16x9 | 720p        | 720psbs  |
///     | 720ptb   | 1080psbs    | 1080ptb  |
///     | 1080i    |             |          |
///   }
///   \table_row2_l{
///     <b>`SetGUILanguage(lang)`</b>
///     ,
///     Set GUI Language
///     @param[in] lang                  The language to use.
///   }
///   \table_row2_l{
///     <b>`SetProperty(key\,value[\,id])`</b>
///     ,
///     Sets a window property for the current window (key\,value)\, or the
///     specified window (key\,value\,id).
///     @param[in] key                   The property to set.
///     @param[in] value                 The property value.
///     @param[in] id                    The window to set property in (optional).
///   }
///   \table_row2_l{
///     <b>`SetStereoMode(ident)`</b>
///     ,
///     Changes the stereo mode of the GUI.
///     Params can be:
///     toggle\, next\, previous\, select\, tomono or any of the supported stereomodes (off\,
///     split_vertical\, split_horizontal\, row_interleaved\, hardware_based\, anaglyph_cyan_red\, anaglyph_green_magenta\, monoscopic)
///     @param[in] ident                 Stereo mode identifier.
///   }
///   \table_row2_l{
///     <b>`TakeScreenshot(url[\,sync)`</b>
///     ,
///     Takes a Screenshot
///     @param[in] url                   URL to save file to. Blank to use default.
///     @param[in] sync                  Add "sync" to run synchronously (optional).
///   }
///   \table_row2_l{
///     <b>`ToggleDirtyRegionVisualization`</b>
///     ,
///     makes dirty regions visible for debugging proposes.
///   }
///  \table_end
///

CBuiltins::CommandMap CGUIBuiltins::GetOperations() const
{
  return {
           {"action",                         {"Executes an action for the active window (same as in keymap)", 1, Action}},
           {"cancelalarm",                    {"Cancels an alarm", 1, CancelAlarm}},
           {"alarmclock",                     {"Prompt for a length of time and start an alarm clock", 2, AlarmClock}},
           {"activatewindow",                 {"Activate the specified window", 1, ActivateWindow<false>}},
           {"activatewindowandfocus",         {"Activate the specified window and sets focus to the specified id", 1, ActivateAndFocus<false>}},
           {"clearproperty",                  {"Clears a window property for the current focused window/dialog (key,value)", 1, ClearProperty}},
           {"dialog.close",                   {"Close a dialog", 1, CloseDialog}},
           {"notification",                   {"Shows a notification on screen, specify header, then message, and optionally time in milliseconds and a icon.", 2, Notification}},
           {"refreshrss",                     {"Reload RSS feeds from RSSFeeds.xml", 0, RefreshRSS}},
           {"replacewindow",                  {"Replaces the current window with the new one", 1, ActivateWindow<true>}},
           {"replacewindowandfocus",          {"Replaces the current window with the new one and sets focus to the specified id", 1, ActivateAndFocus<true>}},
           {"setguilanguage",                 {"Set GUI Language", 1, SetLanguage}},
           {"setproperty",                    {"Sets a window property for the current focused window/dialog (key,value)", 2, SetProperty}},
           {"setstereomode",                  {"Changes the stereo mode of the GUI. Params can be: toggle, next, previous, select, tomono or any of the supported stereomodes (off, split_vertical, split_horizontal, row_interleaved, hardware_based, anaglyph_cyan_red, anaglyph_green_magenta, anaglyph_yellow_blue, monoscopic)", 1, SetStereoMode}},
           {"takescreenshot",                 {"Takes a Screenshot", 0, Screenshot}},
           {"toggledirtyregionvisualization", {"Enables/disables dirty-region visualization", 0, ToggleDirty}}
         };
}
