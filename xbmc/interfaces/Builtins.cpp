/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "network/Network.h"
#include "system.h"
#include "utils/AlarmClock.h"
#include "utils/Screenshot.h"
#include "utils/SeekHandler.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "Autorun.h"
#include "Builtins.h"
#include "input/ButtonTranslator.h"
#include "input/InputManager.h"
#include "FileItem.h"
#include "addons/GUIDialogAddonSettings.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "guilib/GUIKeyboardFactory.h"
#include "input/Key.h"
#include "guilib/StereoscopicsManager.h"
#include "guilib/GUIAudioManager.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogYesNo.h"
#include "GUIUserMessages.h"
#include "windows/GUIWindowLoginScreen.h"
#include "video/windows/GUIWindowVideoBase.h"
#include "addons/GUIWindowAddonBrowser.h"
#include "addons/Addon.h" // for TranslateType, TranslateContent
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/PluginSource.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "interfaces/AnnouncementManager.h"
#include "network/NetworkServices.h"
#include "utils/log.h"
#include "storage/MediaManager.h"
#include "utils/RssManager.h"
#include "utils/JSONVariantParser.h"
#include "PartyModeManager.h"
#include "profiles/ProfilesManager.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/MediaSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/SkinSettings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoLibraryQueue.h"
#include "Util.h"
#include "URL.h"
#include "music/MusicDatabase.h"
#include "cores/IPlayer.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/recordings/PVRRecording.h"

#include "filesystem/PluginDirectory.h"
#ifdef HAS_FILESYSTEM_RAR
#include "filesystem/RarManager.h"
#endif
#include "filesystem/ZipManager.h"

#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"

#ifdef HAS_LIRC
#include "input/linux/LIRC.h"
#endif
#ifdef HAS_IRSERVERSUITE

  #include "input/windows/IRServerSuite.h"

#endif

#if defined(TARGET_DARWIN)
#include "filesystem/SpecialProtocol.h"
#include "osx/CocoaInterface.h"
#endif

#ifdef HAS_CDDA_RIPPER
#include "cdrip/CDDARipper.h"
#endif

#include <vector>
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "powermanagement/PowerManager.h"
#include "filesystem/Directory.h"

using namespace std;
using namespace XFILE;
using namespace ADDON;

#ifdef HAS_DVD_DRIVE
using namespace MEDIA_DETECT;
#endif

typedef struct
{
  const char* command;
  bool needsParameters;
  const char* description;
} BUILT_IN;

const BUILT_IN commands[] = {
  { "Help",                       false,  "This help message" },
  { "Reboot",                     false,  "Reboot the system" },
  { "Restart",                    false,  "Restart the system (same as reboot)" },
  { "ShutDown",                   false,  "Shutdown the system" },
  { "Powerdown",                  false,  "Powerdown system" },
  { "Quit",                       false,  "Quit Kodi" },
  { "Hibernate",                  false,  "Hibernates the system" },
  { "Suspend",                    false,  "Suspends the system" },
  { "InhibitIdleShutdown",        false,  "Inhibit idle shutdown" },
  { "AllowIdleShutdown",          false,  "Allow idle shutdown" },
  { "ActivateScreensaver",        false,  "Activate Screensaver" },
  { "RestartApp",                 false,  "Restart Kodi" },
  { "Minimize",                   false,  "Minimize Kodi" },
  { "Reset",                      false,  "Reset the system (same as reboot)" },
  { "Mastermode",                 false,  "Control master mode" },
  { "SetGUILanguage",             true,   "Set GUI Language" },
  { "ActivateWindow",             true,   "Activate the specified window" },
  { "ActivateWindowAndFocus",     true,   "Activate the specified window and sets focus to the specified id" },
  { "ReplaceWindowAndFocus",      true,   "Replaces the current window with the new one and sets focus to the specified id" },
  { "ReplaceWindow",              true,   "Replaces the current window with the new one" },
  { "TakeScreenshot",             false,  "Takes a Screenshot" },
  { "RunScript",                  true,   "Run the specified script" },
  { "StopScript",                 true,   "Stop the script by ID or path, if running" },
#if defined(TARGET_DARWIN)
  { "RunAppleScript",             true,   "Run the specified AppleScript command" },
#endif
  { "RunPlugin",                  true,   "Run the specified plugin" },
  { "RunAddon",                   true,   "Run the specified plugin/script" },
  { "NotifyAll",                  true,   "Notify all connected clients" },
  { "Extract",                    true,   "Extracts the specified archive" },
  { "PlayMedia",                  true,   "Play the specified media file (or playlist)" },
  { "Seek",                       true,   "Performs a seek in seconds on the current playing media file" },
  { "ShowPicture",                true,   "Display a picture by file path" },
  { "SlideShow",                  true,   "Run a slideshow from the specified directory" },
  { "RecursiveSlideShow",         true,   "Run a slideshow from the specified directory, including all subdirs" },
  { "ReloadSkin",                 false,  "Reload Kodi's skin" },
  { "UnloadSkin",                 false,  "Unload Kodi's skin" },
  { "RefreshRSS",                 false,  "Reload RSS feeds from RSSFeeds.xml"},
  { "PlayerControl",              true,   "Control the music or video player" },
  { "Playlist.PlayOffset",        true,   "Start playing from a particular offset in the playlist" },
  { "Playlist.Clear",             false,  "Clear the current playlist" },
  { "EjectTray",                  false,  "Close or open the DVD tray" },
  { "AlarmClock",                 true,   "Prompt for a length of time and start an alarm clock" },
  { "CancelAlarm",                true,   "Cancels an alarm" },
  { "Action",                     true,   "Executes an action for the active window (same as in keymap)" },
  { "Notification",               true,   "Shows a notification on screen, specify header, then message, and optionally time in milliseconds and a icon." },
  { "PlayDVD",                    false,  "Plays the inserted CD or DVD media from the DVD-ROM Drive!" },
  { "RipCD",                      false,  "Rip the currently inserted audio CD"},
  { "Skin.ToggleSetting",         true,   "Toggles a skin setting on or off" },
  { "Skin.SetString",             true,   "Prompts and sets skin string" },
  { "Skin.SetNumeric",            true,   "Prompts and sets numeric input" },
  { "Skin.SetPath",               true,   "Prompts and sets a skin path" },
  { "Skin.Theme",                 true,   "Control skin theme" },
  { "Skin.SetImage",              true,   "Prompts and sets a skin image" },
  { "Skin.SetLargeImage",         true,   "Prompts and sets a large skin images" },
  { "Skin.SetFile",               true,   "Prompts and sets a file" },
  { "Skin.SetAddon",              true,   "Prompts and set an addon" },
  { "Skin.SetBool",               true,   "Sets a skin setting on" },
  { "Skin.Reset",                 true,   "Resets a skin setting to default" },
  { "Skin.ResetSettings",         false,  "Resets all skin settings" },
  { "Mute",                       false,  "Mute the player" },
  { "SetVolume",                  true,   "Set the current volume" },
  { "Dialog.Close",               true,   "Close a dialog" },
  { "System.LogOff",              false,  "Log off current user" },
  { "System.Exec",                true,   "Execute shell commands" },
  { "System.ExecWait",            true,   "Execute shell commands and freezes Kodi until shell is closed" },
  { "Resolution",                 true,   "Change Kodi's Resolution" },
  { "SetFocus",                   true,   "Change current focus to a different control id" },
  { "UpdateLibrary",              true,   "Update the selected library (music or video)" },
  { "CleanLibrary",               true,   "Clean the video/music library" },
  { "ExportLibrary",              true,   "Export the video/music library" },
  { "PageDown",                   true,   "Send a page down event to the pagecontrol with given id" },
  { "PageUp",                     true,   "Send a page up event to the pagecontrol with given id" },
  { "Container.Refresh",          false,  "Refresh current listing" },
  { "Container.Update",           false,  "Update current listing. Send Container.Update(path,replace) to reset the path history" },
  { "Container.NextViewMode",     false,  "Move to the next view type (and refresh the listing)" },
  { "Container.PreviousViewMode", false,  "Move to the previous view type (and refresh the listing)" },
  { "Container.SetViewMode",      true,   "Move to the view with the given id" },
  { "Container.NextSortMethod",   false,  "Change to the next sort method" },
  { "Container.PreviousSortMethod",false, "Change to the previous sort method" },
  { "Container.SetSortMethod",    true,   "Change to the specified sort method" },
  { "Container.SortDirection",    false,  "Toggle the sort direction" },
  { "Control.Move",               true,   "Tells the specified control to 'move' to another entry specified by offset" },
  { "Control.SetFocus",           true,   "Change current focus to a different control id" },
  { "Control.Message",            true,   "Send a given message to a control within a given window" },
  { "SendClick",                  true,   "Send a click message from the given control to the given window" },
  { "LoadProfile",                true,   "Load the specified profile (note; if locks are active it won't work)" },
  { "SetProperty",                true,   "Sets a window property for the current focused window/dialog (key,value)" },
  { "ClearProperty",              true,   "Clears a window property for the current focused window/dialog (key,value)" },
  { "PlayWith",                   true,   "Play the selected item with the specified core" },
  { "WakeOnLan",                  true,   "Sends the wake-up packet to the broadcast address for the specified MAC address" },
  { "Addon.Default.OpenSettings", true,   "Open a settings dialog for the default addon of the given type" },
  { "Addon.Default.Set",          true,   "Open a select dialog to allow choosing the default addon of the given type" },
  { "Addon.OpenSettings",         true,   "Open a settings dialog for the addon of the given id" },
  { "UpdateAddonRepos",           false,  "Check add-on repositories for updates" },
  { "UpdateLocalAddons",          false,  "Check for local add-on changes" },
  { "ToggleDPMS",                 false,  "Toggle DPMS mode manually"},
  { "CECToggleState",             false,  "Toggle state of playing device via a CEC peripheral"},
  { "CECActivateSource",          false,  "Wake up playing device via a CEC peripheral"},
  { "CECStandby",                 false,  "Put playing device on standby via a CEC peripheral"},
  { "Weather.Refresh",            false,  "Force weather data refresh"},
  { "Weather.LocationNext",       false,  "Switch to next weather location"},
  { "Weather.LocationPrevious",   false,  "Switch to previous weather location"},
  { "Weather.LocationSet",        true,   "Switch to given weather location (parameter can be 1-3)"},
#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
  { "LIRC.Stop",                  false,  "Removes Kodi as LIRC client" },
  { "LIRC.Start",                 false,  "Adds Kodi as LIRC client" },
  { "LIRC.Send",                  true,   "Sends a command to LIRC" },
#endif
  { "VideoLibrary.Search",        false,  "Brings up a search dialog which will search the library" },
  { "ToggleDebug",                false,  "Enables/disables debug mode" },
  { "StartPVRManager",            false,  "(Re)Starts the PVR manager" },
  { "StopPVRManager",             false,  "Stops the PVR manager" },
#if defined(TARGET_ANDROID)
  { "StartAndroidActivity",       true,   "Launch an Android native app with the given package name.  Optional parms (in order): intent, dataType, dataURI." },
#endif
  { "SetStereoMode",              true,   "Changes the stereo mode of the GUI. Params can be: toggle, next, previous, select, tomono or any of the supported stereomodes (off, split_vertical, split_horizontal, row_interleaved, hardware_based, anaglyph_cyan_red, anaglyph_green_magenta, anaglyph_yellow_blue, monoscopic)" }
};

bool CBuiltins::HasCommand(const std::string& execString)
{
  std::string function;
  vector<string> parameters;
  CUtil::SplitExecFunction(execString, function, parameters);
  for (unsigned int i = 0; i < sizeof(commands)/sizeof(BUILT_IN); i++)
  {
    if (StringUtils::EqualsNoCase(function, commands[i].command) && (!commands[i].needsParameters || parameters.size()))
      return true;
  }
  return false;
}

bool CBuiltins::IsSystemPowerdownCommand(const std::string& execString)
{
  std::string execute;
  vector<string> params;
  CUtil::SplitExecFunction(execString, execute, params);
  StringUtils::ToLower(execute);

  // Check if action is resulting in system powerdown.
  if (execute == "reboot"    ||
      execute == "restart"   ||
      execute == "reset"     ||
      execute == "powerdown" ||
      execute == "hibernate" ||
      execute == "suspend" )
  {
    return true;
  }
  else if (execute == "shutdown")
  {
    switch (CSettings::Get().GetInt("powermanagement.shutdownstate"))
    {
      case POWERSTATE_SHUTDOWN:
      case POWERSTATE_SUSPEND:
      case POWERSTATE_HIBERNATE:
        return true;

      default:
        return false;
    }
  }
  return false;
}

void CBuiltins::GetHelp(std::string &help)
{
  help.clear();
  for (unsigned int i = 0; i < sizeof(commands)/sizeof(BUILT_IN); i++)
  {
    help += commands[i].command;
    help += "\t";
    help += commands[i].description;
    help += "\n";
  }
}

bool CBuiltins::ActivateWindow(int iWindowID, const std::vector<std::string>& params /* = {} */, bool swappingWindows /* = false */)
{
  // disable the screensaver
  g_application.WakeUpScreenSaverAndDPMS();
  g_windowManager.ActivateWindow(iWindowID, params, swappingWindows);
  return true;
}

int CBuiltins::Execute(const std::string& execString)
{
  // Deprecated. Get the text after the "XBMC."
  std::string execute;
  vector<string> params;
  CUtil::SplitExecFunction(execString, execute, params);
  StringUtils::ToLower(execute);
  std::string parameter = params.size() ? params[0] : "";
  std::string paramlow(parameter);
  StringUtils::ToLower(paramlow);

  if (execute == "reboot" || execute == "restart" || execute == "reset")  //Will reboot the system
  {
    CApplicationMessenger::Get().Restart();
  }
  else if (execute == "shutdown")
  {
    CApplicationMessenger::Get().Shutdown();
  }
  else if (execute == "powerdown")
  {
    CApplicationMessenger::Get().Powerdown();
  }
  else if (execute == "restartapp")
  {
    CApplicationMessenger::Get().RestartApp();
  }
  else if (execute == "hibernate")
  {
    CApplicationMessenger::Get().Hibernate();
  }
  else if (execute == "suspend")
  {
    CApplicationMessenger::Get().Suspend();
  }
  else if (execute == "quit")
  {
    CApplicationMessenger::Get().Quit();
  }
  else if (execute == "inhibitidleshutdown")
  {
    bool inhibit = (params.size() == 1 && StringUtils::EqualsNoCase(params[0], "true"));
    CApplicationMessenger::Get().InhibitIdleShutdown(inhibit);
  }
  else if (execute == "activatescreensaver")
  {
    CApplicationMessenger::Get().ActivateScreensaver();
  }
  else if (execute == "minimize")
  {
    CApplicationMessenger::Get().Minimize();
  }
  else if (execute == "loadprofile")
  {
    int index = CProfilesManager::Get().GetProfileIndex(parameter);
    bool prompt = (params.size() == 2 && StringUtils::EqualsNoCase(params[1], "prompt"));
    bool bCanceled;
    if (index >= 0
        && (CProfilesManager::Get().GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE
            || g_passwordManager.IsProfileLockUnlocked(index,bCanceled,prompt)))
    {
      CApplicationMessenger::Get().LoadProfile(index);
    }
  }
  else if (execute == "mastermode")
  {
    if (g_passwordManager.bMasterUser)
    {
      g_passwordManager.bMasterUser = false;
      g_passwordManager.LockSources(true);
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(20052),g_localizeStrings.Get(20053));
    }
    else if (g_passwordManager.IsMasterLockUnlocked(true))
    {
      g_passwordManager.LockSources(false);
      g_passwordManager.bMasterUser = true;
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(20052),g_localizeStrings.Get(20054));
    }

    CUtil::DeleteVideoDatabaseDirectoryCache();
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE);
    g_windowManager.SendMessage(msg);
  }
  else if (execute == "setguilanguage")
  {
    if (params.size())
    {
      CApplicationMessenger::Get().SetGUILanguage(params[0]);
    }
  }
  else if (execute == "takescreenshot")
  {
    if (params.size())
    {
      // get the parameters
      std::string strSaveToPath = params[0];
      bool sync = false;
      if (params.size() >= 2)
        sync = StringUtils::EqualsNoCase(params[1], "sync");

      if (!strSaveToPath.empty())
      {
        if (CDirectory::Exists(strSaveToPath))
        {
          std::string file = CUtil::GetNextFilename(URIUtils::AddFileToFolder(strSaveToPath, "screenshot%03d.png"), 999);

          if (!file.empty())
          {
            CScreenShot::TakeScreenshot(file, sync);
          }
          else
          {
            CLog::Log(LOGWARNING, "Too many screen shots or invalid folder %s", strSaveToPath.c_str());
          }
        }
        else
          CScreenShot::TakeScreenshot(strSaveToPath, sync);
      }
    }
    else
      CScreenShot::TakeScreenshot();
  }
  else if (execute == "activatewindow" || execute == "replacewindow")
  {
    // get the parameters
    std::string strWindow;
    if (params.size())
    {
      strWindow = params[0];
      params.erase(params.begin());
    }

    // confirm the window destination is valid prior to switching
    int iWindow = CButtonTranslator::TranslateWindow(strWindow);
    if (iWindow != WINDOW_INVALID)
    {
      // compate the given directory param with the current active directory
      bool bIsSameStartFolder = true;
      if (!params.empty())
      {
        CGUIWindow *activeWindow = g_windowManager.GetWindow(g_windowManager.GetActiveWindow());
        if (activeWindow && activeWindow->IsMediaWindow())
          bIsSameStartFolder = ((CGUIMediaWindow*) activeWindow)->IsSameStartFolder(params[0]);
      }

      // activate window only if window and path differ from the current active window
      if (iWindow != g_windowManager.GetActiveWindow() || !bIsSameStartFolder)
      {
        return ActivateWindow(iWindow, params, execute != "activatewindow");
      }
    }
    else
    {
      CLog::Log(LOGERROR, "Activate/ReplaceWindow called with invalid destination window: %s", strWindow.c_str());
      return false;
    }
  }
  else if ((execute == "setfocus" || execute == "control.setfocus") && params.size())
  {
    int controlID = atol(params[0].c_str());
    int subItem = (params.size() > 1) ? atol(params[1].c_str())+1 : 0;
    CGUIMessage msg(GUI_MSG_SETFOCUS, g_windowManager.GetFocusedWindow(), controlID, subItem);
    g_windowManager.SendMessage(msg);
  }
  else if ((execute == "activatewindowandfocus" || execute == "replacewindowandfocus") && params.size())
  {
    std::string strWindow = params[0];

    // confirm the window destination is valid prior to switching
    int iWindow = CButtonTranslator::TranslateWindow(strWindow);
    if (iWindow != WINDOW_INVALID)
    {
      if (iWindow != g_windowManager.GetActiveWindow())
      {
        if (!ActivateWindow(iWindow, {}, execute != "activatewindowandfocus"))
          return false;

        unsigned int iPtr = 1;
        while (params.size() > iPtr + 1)
        {
          CGUIMessage msg(GUI_MSG_SETFOCUS, g_windowManager.GetFocusedWindow(),
              atol(params[iPtr].c_str()),
              (params.size() >= iPtr + 2) ? atol(params[iPtr + 1].c_str())+1 : 0);
          g_windowManager.SendMessage(msg);
          iPtr += 2;
        }
      }
    }
    else
    {
      CLog::Log(LOGERROR, "Replace/ActivateWindowAndFocus called with invalid destination window: %s", strWindow.c_str());
      return false;
    }
  }
  else if (execute == "runscript" && params.size())
  {
#if defined(TARGET_DARWIN_OSX)
    if (URIUtils::HasExtension(parameter, ".applescript|.scpt"))
    {
      std::string osxPath = CSpecialProtocol::TranslatePath(parameter);
      Cocoa_DoAppleScriptFile(osxPath.c_str());
    }
    else
#endif
    {
      AddonPtr addon;
      std::string scriptpath;
      // Test to see if the param is an addon ID
      if (CAddonMgr::Get().GetAddon(params[0], addon))
      {
        //Get the correct extension point to run
        if (CAddonMgr::Get().GetAddon(params[0], addon, ADDON_SCRIPT) ||
            CAddonMgr::Get().GetAddon(params[0], addon, ADDON_SCRIPT_WEATHER) ||
            CAddonMgr::Get().GetAddon(params[0], addon, ADDON_SCRIPT_LYRICS) ||
            CAddonMgr::Get().GetAddon(params[0], addon, ADDON_SCRIPT_LIBRARY))
        {
          scriptpath = addon->LibPath();
        }
        else
        {
          //Run a random extension point (old behaviour).
          CAddonMgr::Get().GetAddon(params[0], addon);
          scriptpath = addon->LibPath();
          CLog::Log(LOGWARNING, "RunScript called for a non-script addon '%s'. This behaviour is deprecated.", params[0].c_str());
        }
      }
      else
        scriptpath = params[0];

      // split the path up to find the filename
      vector<string> argv = params;
      std::string filename = URIUtils::GetFileName(scriptpath);
      if (!filename.empty())
        argv[0] = filename;

      CScriptInvocationManager::Get().ExecuteAsync(scriptpath, addon, argv);
    }
  }
#if defined(TARGET_DARWIN_OSX)
  else if (execute == "runapplescript")
  {
    Cocoa_DoAppleScript(parameter.c_str());
  }
#endif
  else if (execute == "stopscript")
  {
    // FIXME: This does not work for addons with multiple extension points!
    // Are there any use for this? TODO: Fix hack in CScreenSaver::Destroy() and deprecate.
    if (!params.empty())
    {
      std::string scriptpath(params[0]);
      // Test to see if the param is an addon ID
      AddonPtr script;
      if (CAddonMgr::Get().GetAddon(params[0], script))
        scriptpath = script->LibPath();
      CScriptInvocationManager::Get().Stop(scriptpath);
    }
  }
  else if (execute == "system.exec")
  {
    CApplicationMessenger::Get().Minimize();
    CApplicationMessenger::Get().ExecOS(parameter, false);
  }
  else if (execute == "system.execwait")
  {
    CApplicationMessenger::Get().Minimize();
    CApplicationMessenger::Get().ExecOS(parameter, true);
  }
  else if (execute == "resolution")
  {
    RESOLUTION res = RES_PAL_4x3;
    if (paramlow == "pal") res = RES_PAL_4x3;
    else if (paramlow == "pal16x9") res = RES_PAL_16x9;
    else if (paramlow == "ntsc") res = RES_NTSC_4x3;
    else if (paramlow == "ntsc16x9") res = RES_NTSC_16x9;
    else if (paramlow == "720p") res = RES_HDTV_720p;
    else if (paramlow == "720psbs") res = RES_HDTV_720pSBS;
    else if (paramlow == "720ptb") res = RES_HDTV_720pTB;
    else if (paramlow == "1080psbs") res = RES_HDTV_1080pSBS;
    else if (paramlow == "1080ptb") res = RES_HDTV_1080pTB;
    else if (paramlow == "1080i") res = RES_HDTV_1080i;
    if (g_graphicsContext.IsValidResolution(res))
    {
      CDisplaySettings::Get().SetCurrentResolution(res, true);
      g_application.ReloadSkin();
    }
  }
  else if (execute == "extract" && params.size())
  {
    // Detects if file is zip or rar then extracts
    std::string strDestDirect;
    if (params.size() < 2)
      strDestDirect = URIUtils::GetDirectory(params[0]);
    else
      strDestDirect = params[1];

    URIUtils::AddSlashAtEnd(strDestDirect);

    if (URIUtils::IsZIP(params[0]))
      g_ZipManager.ExtractArchive(params[0],strDestDirect);
#ifdef HAS_FILESYSTEM_RAR
    else if (URIUtils::IsRAR(params[0]))
      g_RarManager.ExtractArchive(params[0],strDestDirect);
#endif
    else
      CLog::Log(LOGERROR, "Extract, No archive given");
  }
  else if (execute == "runplugin")
  {
    if (params.size())
    {
      CFileItem item(params[0]);
      if (!item.m_bIsFolder)
      {
        item.SetPath(params[0]);
        CPluginDirectory::RunScriptWithParams(item.GetPath());
      }
    }
    else
    {
      CLog::Log(LOGERROR, "RunPlugin called with no arguments.");
    }
  }
  else if (execute == "runaddon")
  {
    if (params.size())
    {
      AddonPtr addon;
      if (CAddonMgr::Get().GetAddon(params[0], addon, ADDON_PLUGIN))
      {
        PluginPtr plugin = std::dynamic_pointer_cast<CPluginSource>(addon);
        std::string addonid = params[0];
        std::string urlParameters;
        vector<string> parameters;
        if (params.size() == 2 &&
           (StringUtils::StartsWith(params[1], "/") || StringUtils::StartsWith(params[1], "?")))
          urlParameters = params[1];
        else if (params.size() > 1)
        {
          parameters.insert(parameters.begin(), params.begin() + 1, params.end());
          urlParameters = "?" + StringUtils::Join(parameters, "&");
        }
        else
        {
          // Add '/' if addon is run without params (will be removed later so it's safe)
          // Otherwise there are 2 entries for the same plugin in ViewModesX.db
          urlParameters = "/";
        }

        std::string cmd;
        if (plugin->Provides(CPluginSource::VIDEO))
          cmd = StringUtils::Format("ActivateWindow(Videos,plugin://%s%s,return)", addonid.c_str(), urlParameters.c_str());
        else if (plugin->Provides(CPluginSource::AUDIO))
          cmd = StringUtils::Format("ActivateWindow(Music,plugin://%s%s,return)", addonid.c_str(), urlParameters.c_str());
        else if (plugin->Provides(CPluginSource::EXECUTABLE))
          cmd = StringUtils::Format("ActivateWindow(Programs,plugin://%s%s,return)", addonid.c_str(), urlParameters.c_str());
        else if (plugin->Provides(CPluginSource::IMAGE))
          cmd = StringUtils::Format("ActivateWindow(Pictures,plugin://%s%s,return)", addonid.c_str(), urlParameters.c_str());
        else
          // Pass the script name (params[0]) and all the parameters
          // (params[1] ... params[x]) separated by a comma to RunPlugin
          cmd = StringUtils::Format("RunPlugin(%s)", StringUtils::Join(params, ",").c_str());
        Execute(cmd);
      }
      else if (CAddonMgr::Get().GetAddon(params[0], addon, ADDON_SCRIPT) ||
               CAddonMgr::Get().GetAddon(params[0], addon, ADDON_SCRIPT_WEATHER) ||
               CAddonMgr::Get().GetAddon(params[0], addon, ADDON_SCRIPT_LYRICS) ||
               CAddonMgr::Get().GetAddon(params[0], addon, ADDON_SCRIPT_LIBRARY))
      {
        // Pass the script name (params[0]) and all the parameters
        // (params[1] ... params[x]) separated by a comma to RunScript
        Execute(StringUtils::Format("RunScript(%s)", StringUtils::Join(params, ",").c_str()));
      }
      else
        CLog::Log(LOGERROR, "RunAddon: unknown add-on id '%s', or unexpected add-on type (not a script or plugin).", params[0].c_str());
    }
    else
    {
      CLog::Log(LOGERROR, "RunAddon called with no arguments.");
    }
  }
  else if (execute == "notifyall")
  {
    if (params.size() > 1)
    {
      CVariant data;
      if (params.size() > 2)
        data = CJSONVariantParser::Parse((const unsigned char *)params[2].c_str(), params[2].size());

      ANNOUNCEMENT::CAnnouncementManager::Get().Announce(ANNOUNCEMENT::Other, params[0].c_str(), params[1].c_str(), data);
    }
    else
      CLog::Log(LOGERROR, "NotifyAll needs two parameters");
  }
  else if (execute == "playmedia")
  {
    if (!params.size())
    {
      CLog::Log(LOGERROR, "PlayMedia called with empty parameter");
      return -3;
    }

    CFileItem item(params[0], false);
    if (URIUtils::HasSlashAtEnd(params[0]))
      item.m_bIsFolder = true;

    // restore to previous window if needed
    if( g_windowManager.GetActiveWindow() == WINDOW_SLIDESHOW ||
        g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO ||
        g_windowManager.GetActiveWindow() == WINDOW_VISUALISATION )
        g_windowManager.PreviousWindow();

    // reset screensaver
    g_application.ResetScreenSaver();
    g_application.WakeUpScreenSaverAndDPMS();

    // ask if we need to check guisettings to resume
    bool askToResume = true;
    int playOffset = 0;
    for (unsigned int i = 1 ; i < params.size() ; i++)
    {
      if (StringUtils::EqualsNoCase(params[i], "isdir"))
        item.m_bIsFolder = true;
      else if (params[i] == "1") // set fullscreen or windowed
        CMediaSettings::Get().SetVideoStartWindowed(true);
      else if (StringUtils::EqualsNoCase(params[i], "resume"))
      {
        // force the item to resume (if applicable) (see CApplication::PlayMedia)
        item.m_lStartOffset = STARTOFFSET_RESUME;
        askToResume = false;
      }
      else if (StringUtils::EqualsNoCase(params[i], "noresume"))
      {
        // force the item to start at the beginning (m_lStartOffset is initialized to 0)
        askToResume = false;
      }
      else if (StringUtils::StartsWithNoCase(params[i], "playoffset=")) {
        playOffset = atoi(params[i].substr(11).c_str()) - 1;
        item.SetProperty("playlist_starting_track", playOffset);
      }
    }

    if (!item.m_bIsFolder && item.IsPlugin())
      item.SetProperty("IsPlayable", true);

    if ( askToResume == true )
    {
      if ( CGUIWindowVideoBase::ShowResumeMenu(item) == false )
        return false;
    }
    if (item.m_bIsFolder)
    {
      CFileItemList items;
      std::string extensions = g_advancedSettings.m_videoExtensions + "|" + g_advancedSettings.GetMusicExtensions();
      CDirectory::GetDirectory(item.GetPath(),items,extensions);

      bool containsMusic = false, containsVideo = false;
      for (int i = 0; i < items.Size(); i++)
      {
        bool isVideo = items[i]->IsVideo();
        containsMusic |= !isVideo;
        containsVideo |= isVideo;
        
        if (containsMusic && containsVideo)
          break;
      }

      unique_ptr<CGUIViewState> state(CGUIViewState::GetViewState(containsVideo ? WINDOW_VIDEO_NAV : WINDOW_MUSIC, items));
      if (state.get())
        items.Sort(state->GetSortMethod());
      else
        items.Sort(SortByLabel, SortOrderAscending);

      int playlist = containsVideo? PLAYLIST_VIDEO : PLAYLIST_MUSIC;;
      if (containsMusic && containsVideo) //mixed content found in the folder
      {
        for (int i = items.Size() - 1; i >= 0; i--) //remove music entries
        {
          if (!items[i]->IsVideo())
            items.Remove(i);
        }
      }
      
      g_playlistPlayer.ClearPlaylist(playlist);
      g_playlistPlayer.Add(playlist, items);
      g_playlistPlayer.SetCurrentPlaylist(playlist);
      g_playlistPlayer.Play(playOffset);
    }
    else
    {
      int playlist = item.IsAudio() ? PLAYLIST_MUSIC : PLAYLIST_VIDEO;
      g_playlistPlayer.ClearPlaylist(playlist);
      g_playlistPlayer.SetCurrentPlaylist(playlist);

      // play media
      if (!g_application.PlayMedia(item, playlist))
      {
        CLog::Log(LOGERROR, "PlayMedia could not play media: %s", params[0].c_str());
        return false;
      }
    }
  }
  else if (execute == "seek")
  {
    if (!params.size())
    {
      CLog::Log(LOGERROR, "Seek called with empty parameter");
      return -3;
    }
    if (g_application.m_pPlayer->IsPlaying())
      CSeekHandler::Get().SeekSeconds(atoi(params[0].c_str()));
  }
  else if (execute == "showpicture")
  {
    if (!params.size())
    {
      CLog::Log(LOGERROR, "ShowPicture called with empty parameter");
      return -2;
    }
    CGUIMessage msg(GUI_MSG_SHOW_PICTURE, 0, 0);
    msg.SetStringParam(params[0]);
    CGUIWindow *pWindow = g_windowManager.GetWindow(WINDOW_SLIDESHOW);
    if (pWindow) pWindow->OnMessage(msg);
  }
  else if (execute == "slideshow" || execute == "recursiveslideshow")
  {
    if (!params.size())
    {
      CLog::Log(LOGERROR, "SlideShow called with empty parameter");
      return -2;
    }
    std::string beginSlidePath;
    // leave RecursiveSlideShow command as-is
    unsigned int flags = 0;
    if (execute == "recursiveslideshow")
      flags |= 1;

    // SlideShow(dir[,recursive][,[not]random][,pause][,beginslide="/path/to/start/slide.jpg"])
    // the beginslide value need be escaped (for '"' or '\' in it, by backslash)
    // and then quoted, or not. See CUtil::SplitParams()
    else
    {
      for (unsigned int i = 1 ; i < params.size() ; i++)
      {
        if (StringUtils::EqualsNoCase(params[i], "recursive"))
          flags |= 1;
        else if (StringUtils::EqualsNoCase(params[i], "random")) // set fullscreen or windowed
          flags |= 2;
        else if (StringUtils::EqualsNoCase(params[i], "notrandom"))
          flags |= 4;
        else if (StringUtils::EqualsNoCase(params[i], "pause"))
          flags |= 8;
        else if (StringUtils::StartsWithNoCase(params[i], "beginslide="))
          beginSlidePath = params[i].substr(11);
      }
    }

    CGUIMessage msg(GUI_MSG_START_SLIDESHOW, 0, 0, flags);
    vector<string> strParams;
    strParams.push_back(params[0]);
    strParams.push_back(beginSlidePath);
    msg.SetStringParams(strParams);
    CGUIWindow *pWindow = g_windowManager.GetWindow(WINDOW_SLIDESHOW);
    if (pWindow) pWindow->OnMessage(msg);
  }
  else if (execute == "reloadskin")
  {
    //  Reload the skin
    g_application.ReloadSkin(!params.empty() && StringUtils::EqualsNoCase(params[0], "confirm"));
  }
  else if (execute == "unloadskin")
  {
    g_application.UnloadSkin(true); // we're reloading the skin after this
  }
  else if (execute == "refreshrss")
  {
    CRssManager::Get().Reload();
  }
  else if (execute == "playercontrol")
  {
    g_application.ResetScreenSaver();
    g_application.WakeUpScreenSaverAndDPMS();
    if (!params.size())
    {
      CLog::Log(LOGERROR, "PlayerControl called with empty parameter");
      return -3;
    }
    if (paramlow ==  "play")
    { // play/pause
      // either resume playing, or pause
      if (g_application.m_pPlayer->IsPlaying())
      {
        if (g_application.m_pPlayer->GetPlaySpeed() != 1)
          g_application.m_pPlayer->SetPlaySpeed(1, g_application.IsMutedInternal());
        else
          g_application.m_pPlayer->Pause();
      }
    }
    else if (paramlow == "stop")
    {
      g_application.StopPlaying();
    }
    else if (paramlow =="rewind" || paramlow == "forward")
    {
      if (g_application.m_pPlayer->IsPlaying() && !g_application.m_pPlayer->IsPaused())
      {
        int iPlaySpeed = g_application.m_pPlayer->GetPlaySpeed();
        if (paramlow == "rewind" && iPlaySpeed == 1) // Enables Rewinding
          iPlaySpeed *= -2;
        else if (paramlow ==  "rewind" && iPlaySpeed > 1) //goes down a notch if you're FFing
          iPlaySpeed /= 2;
        else if (paramlow == "forward" && iPlaySpeed < 1) //goes up a notch if you're RWing
        {
          iPlaySpeed /= 2;
          if (iPlaySpeed == -1) iPlaySpeed = 1;
        }
        else
          iPlaySpeed *= 2;

        if (iPlaySpeed > 32 || iPlaySpeed < -32)
          iPlaySpeed = 1;

        g_application.m_pPlayer->SetPlaySpeed(iPlaySpeed, g_application.IsMutedInternal());
      }
    }
    else if (paramlow == "next")
    {
      g_application.OnAction(CAction(ACTION_NEXT_ITEM));
    }
    else if (paramlow == "previous")
    {
      g_application.OnAction(CAction(ACTION_PREV_ITEM));
    }
    else if (paramlow == "bigskipbackward")
    {
      if (g_application.m_pPlayer->IsPlaying())
        g_application.m_pPlayer->Seek(false, true);
    }
    else if (paramlow == "bigskipforward")
    {
      if (g_application.m_pPlayer->IsPlaying())
        g_application.m_pPlayer->Seek(true, true);
    }
    else if (paramlow == "smallskipbackward")
    {
      if (g_application.m_pPlayer->IsPlaying())
        g_application.m_pPlayer->Seek(false, false);
    }
    else if (paramlow == "smallskipforward")
    {
      if (g_application.m_pPlayer->IsPlaying())
        g_application.m_pPlayer->Seek(true, false);
    }
    else if (StringUtils::StartsWithNoCase(parameter, "seekpercentage"))
    {
      std::string offset;
      if (parameter.size() == 14)
        CLog::Log(LOGERROR,"PlayerControl(seekpercentage(n)) called with no argument");
      else if (parameter.size() < 17) // arg must be at least "(N)"
        CLog::Log(LOGERROR,"PlayerControl(seekpercentage(n)) called with invalid argument: \"%s\"", parameter.substr(14).c_str());
      else
      {
        // Don't bother checking the argument: an invalid arg will do seek(0)
        offset = parameter.substr(15);
        StringUtils::TrimRight(offset, ")");
        float offsetpercent = (float) atof(offset.c_str());
        if (offsetpercent < 0 || offsetpercent > 100)
          CLog::Log(LOGERROR,"PlayerControl(seekpercentage(n)) argument, %f, must be 0-100", offsetpercent);
        else if (g_application.m_pPlayer->IsPlaying())
          g_application.SeekPercentage(offsetpercent);
      }
    }
    else if (paramlow == "showvideomenu")
    {
      if( g_application.m_pPlayer->IsPlaying() )
        g_application.m_pPlayer->OnAction(CAction(ACTION_SHOW_VIDEOMENU));
    }
    else if (paramlow == "record")
    {
      if( g_application.m_pPlayer->IsPlaying() && g_application.m_pPlayer->CanRecord())
        g_application.m_pPlayer->Record(!g_application.m_pPlayer->IsRecording());
    }
    else if (StringUtils::StartsWithNoCase(parameter, "partymode"))
    {
      std::string strXspPath;
      //empty param=music, "music"=music, "video"=video, else xsp path
      PartyModeContext context = PARTYMODECONTEXT_MUSIC;
      if (parameter.size() > 9)
      {
        if (parameter.size() == 16 && StringUtils::EndsWithNoCase(parameter, "video)"))
          context = PARTYMODECONTEXT_VIDEO;
        else if (parameter.size() != 16 || !StringUtils::EndsWithNoCase(parameter, "music)"))
        {
          strXspPath = parameter.substr(10);
          StringUtils::TrimRight(strXspPath, ")");
          context = PARTYMODECONTEXT_UNKNOWN;
        }
      }
      if (g_partyModeManager.IsEnabled())
        g_partyModeManager.Disable();
      else
        g_partyModeManager.Enable(context, strXspPath);
    }
    else if (paramlow == "random" || paramlow == "randomoff" || paramlow == "randomon")
    {
      // get current playlist
      int iPlaylist = g_playlistPlayer.GetCurrentPlaylist();

      // reverse the current setting
      bool shuffled = g_playlistPlayer.IsShuffled(iPlaylist);
      if ((shuffled && paramlow == "randomon") || (!shuffled && paramlow == "randomoff"))
        return 0;

      // check to see if we should notify the user
      bool notify = (params.size() == 2 && StringUtils::EqualsNoCase(params[1], "notify"));
      g_playlistPlayer.SetShuffle(iPlaylist, !shuffled, notify);

      // save settings for now playing windows
      switch (iPlaylist)
      {
      case PLAYLIST_MUSIC:
        CMediaSettings::Get().SetMusicPlaylistShuffled(g_playlistPlayer.IsShuffled(iPlaylist));
        CSettings::Get().Save();
        break;
      case PLAYLIST_VIDEO:
        CMediaSettings::Get().SetVideoPlaylistShuffled(g_playlistPlayer.IsShuffled(iPlaylist));
        CSettings::Get().Save();
      }

      // send message
      CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_RANDOM, 0, 0, iPlaylist, g_playlistPlayer.IsShuffled(iPlaylist));
      g_windowManager.SendThreadMessage(msg);

    }
    else if (StringUtils::StartsWithNoCase(parameter, "repeat"))
    {
      // get current playlist
      int iPlaylist = g_playlistPlayer.GetCurrentPlaylist();
      PLAYLIST::REPEAT_STATE previous_state = g_playlistPlayer.GetRepeat(iPlaylist);

      PLAYLIST::REPEAT_STATE state;
      if (paramlow == "repeatall")
        state = PLAYLIST::REPEAT_ALL;
      else if (paramlow == "repeatone")
        state = PLAYLIST::REPEAT_ONE;
      else if (paramlow == "repeatoff")
        state = PLAYLIST::REPEAT_NONE;
      else if (previous_state == PLAYLIST::REPEAT_NONE)
        state = PLAYLIST::REPEAT_ALL;
      else if (previous_state == PLAYLIST::REPEAT_ALL)
        state = PLAYLIST::REPEAT_ONE;
      else
        state = PLAYLIST::REPEAT_NONE;

      if (state == previous_state)
        return 0;

      // check to see if we should notify the user
      bool notify = (params.size() == 2 && StringUtils::EqualsNoCase(params[1], "notify"));
      g_playlistPlayer.SetRepeat(iPlaylist, state, notify);

      // save settings for now playing windows
      switch (iPlaylist)
      {
      case PLAYLIST_MUSIC:
        CMediaSettings::Get().SetMusicPlaylistRepeat(state == PLAYLIST::REPEAT_ALL);
        CSettings::Get().Save();
        break;
      case PLAYLIST_VIDEO:
        CMediaSettings::Get().SetVideoPlaylistRepeat(state == PLAYLIST::REPEAT_ALL);
        CSettings::Get().Save();
      }

      // send messages so now playing window can get updated
      CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_REPEAT, 0, 0, iPlaylist, (int)state);
      g_windowManager.SendThreadMessage(msg);
    }
    else if (StringUtils::StartsWithNoCase(parameter, "resumelivetv"))
    {
      CFileItem& fileItem(g_application.CurrentFileItem());
      PVR::CPVRChannelPtr channel = fileItem.HasPVRRecordingInfoTag() ? fileItem.GetPVRRecordingInfoTag()->Channel() : PVR::CPVRChannelPtr();

      if (channel)
      {
        CFileItem playItem(channel);
        if (!g_application.PlayMedia(playItem, channel->IsRadio() ? PLAYLIST_MUSIC : PLAYLIST_VIDEO))
        {
          CLog::Log(LOGERROR, "ResumeLiveTv could not play channel: %s", channel->ChannelName().c_str());
          return false;
        }
      }
    }
  }
  else if (execute == "playwith")
  {
    g_application.m_eForcedNextPlayer = CPlayerCoreFactory::Get().GetPlayerCore(parameter);
    g_application.OnAction(CAction(ACTION_PLAYER_PLAY));
  }
  else if (execute == "mute")
  {
    g_application.ToggleMute();
  }
  else if (execute == "setvolume")
  {
    float oldVolume = g_application.GetVolume();
    float volume = (float)strtod(parameter.c_str(), NULL);

    g_application.SetVolume(volume);
    if(oldVolume != volume)
    {
      if(params.size() > 1 && StringUtils::EqualsNoCase(params[1], "showVolumeBar"))
      {
        CApplicationMessenger::Get().ShowVolumeBar(oldVolume < volume);  
      }
    }
  }
  else if (execute == "playlist.playoffset")
  {
    // playlist.playoffset(offset)
    // playlist.playoffset(music|video,offset)
    std::string strPos = parameter;
    if (params.size() > 1)
    {
      // ignore any other parameters if present
      std::string strPlaylist = params[0];
      strPos = params[1];

      int iPlaylist = PLAYLIST_NONE;
      if (paramlow == "music")
        iPlaylist = PLAYLIST_MUSIC;
      else if (paramlow == "video")
        iPlaylist = PLAYLIST_VIDEO;

      // unknown playlist
      if (iPlaylist == PLAYLIST_NONE)
      {
        CLog::Log(LOGERROR,"Playlist.PlayOffset called with unknown playlist: %s", strPlaylist.c_str());
        return false;
      }

      // user wants to play the 'other' playlist
      if (iPlaylist != g_playlistPlayer.GetCurrentPlaylist())
      {
        g_application.StopPlaying();
        g_playlistPlayer.Reset();
        g_playlistPlayer.SetCurrentPlaylist(iPlaylist);
      }
    }
    // play the desired offset
    int pos = atol(strPos.c_str());
    // playlist is already playing
    if (g_application.m_pPlayer->IsPlaying())
      g_playlistPlayer.PlayNext(pos);
    // we start playing the 'other' playlist so we need to use play to initialize the player state
    else
      g_playlistPlayer.Play(pos);
  }
  else if (execute == "playlist.clear")
  {
    g_playlistPlayer.Clear();
  }
#ifdef HAS_DVD_DRIVE
  else if (execute == "ejecttray")
  {
    g_mediaManager.ToggleTray();
  }
#endif
  else if (execute == "alarmclock" && params.size() > 1)
  {
    // format is alarmclock(name,command[,seconds,true]);
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
      if (StringUtils::EqualsNoCase(parameter, "shutdowntimer"))
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
  }
  else if (execute == "notification")
  {
    if (params.size() < 2)
      return -1;
    if (params.size() == 4)
      CGUIDialogKaiToast::QueueNotification(params[3],params[0],params[1],atoi(params[2].c_str()));
    else if (params.size() == 3)
      CGUIDialogKaiToast::QueueNotification("",params[0],params[1],atoi(params[2].c_str()));
    else
      CGUIDialogKaiToast::QueueNotification(params[0],params[1]);
  }
  else if (execute == "cancelalarm")
  {
    bool silent = false;
    if (params.size() > 1 && StringUtils::EqualsNoCase(params[1], "true"))
      silent = true;
    g_alarmClock.Stop(params[0],silent);
  }
  else if (execute == "playdvd")
  {
#ifdef HAS_DVD_DRIVE
    bool restart = false;
    if (params.size() > 0 && StringUtils::EqualsNoCase(params[0], "restart"))
      restart = true;
    CAutorun::PlayDisc(g_mediaManager.GetDiscPath(), true, restart);
#endif
  }
  else if (execute == "ripcd")
  {
#ifdef HAS_CDDA_RIPPER
    CCDDARipper::GetInstance().RipCD();
#endif
  }
  else if (execute == "skin.togglesetting")
  {
    int setting = CSkinSettings::Get().TranslateBool(parameter);
    CSkinSettings::Get().SetBool(setting, !CSkinSettings::Get().GetBool(setting));
    CSettings::Get().Save();
  }
  else if (execute == "skin.setbool" && params.size())
  {
    if (params.size() > 1)
    {
      int string = CSkinSettings::Get().TranslateBool(params[0]);
      CSkinSettings::Get().SetBool(string, StringUtils::EqualsNoCase(params[1], "true"));
      CSettings::Get().Save();
      return 0;
    }
    // default is to set it to true
    int setting = CSkinSettings::Get().TranslateBool(params[0]);
    CSkinSettings::Get().SetBool(setting, true);
    CSettings::Get().Save();
  }
  else if (execute == "skin.reset")
  {
    CSkinSettings::Get().Reset(parameter);
    CSettings::Get().Save();
  }
  else if (execute == "skin.resetsettings")
  {
    CSkinSettings::Get().Reset();
    CSettings::Get().Save();
  }
  else if (execute == "skin.theme")
  {
    // enumerate themes
    vector<std::string> vecTheme;
    CUtil::GetSkinThemes(vecTheme);

    int iTheme = -1;

    // find current theme
    if (!StringUtils::EqualsNoCase(CSettings::Get().GetString("lookandfeel.skintheme"), "SKINDEFAULT"))
    {
      for (unsigned int i=0;i<vecTheme.size();++i)
      {
        std::string strTmpTheme(CSettings::Get().GetString("lookandfeel.skintheme"));
        URIUtils::RemoveExtension(strTmpTheme);
        if (StringUtils::EqualsNoCase(vecTheme[i], strTmpTheme))
        {
          iTheme=i;
          break;
        }
      }
    }

    int iParam = atoi(parameter.c_str());
    if (iParam == 0 || iParam == 1)
      iTheme++;
    else if (iParam == -1)
      iTheme--;
    if (iTheme > (int)vecTheme.size()-1)
      iTheme = -1;
    if (iTheme < -1)
      iTheme = vecTheme.size()-1;

    std::string strSkinTheme = "SKINDEFAULT";
    if (iTheme != -1 && iTheme < (int)vecTheme.size())
      strSkinTheme = vecTheme[iTheme];

    CSettings::Get().SetString("lookandfeel.skintheme", strSkinTheme);
    // also set the default color theme
    std::string colorTheme(URIUtils::ReplaceExtension(strSkinTheme, ".xml"));
    if (StringUtils::EqualsNoCase(colorTheme, "Textures.xml"))
      colorTheme = "defaults.xml";
    CSettings::Get().SetString("lookandfeel.skincolors", colorTheme);
    g_application.ReloadSkin();
  }
  else if (execute == "skin.setstring"  || execute == "skin.setimage" ||
           execute == "skin.setfile"    || execute == "skin.setpath"  ||
           execute == "skin.setnumeric" || execute == "skin.setlargeimage")
  {
    // break the parameter up if necessary
    int string = 0;
    if (params.size() > 1)
    {
      string = CSkinSettings::Get().TranslateString(params[0]);
      if (execute == "skin.setstring")
      {
        CSkinSettings::Get().SetString(string, params[1]);
        CSettings::Get().Save();
        return 0;
      }
    }
    else
      string = CSkinSettings::Get().TranslateString(params[0]);
    std::string value = CSkinSettings::Get().GetString(string);
    VECSOURCES localShares;
    g_mediaManager.GetLocalDrives(localShares);
    if (execute == "skin.setstring")
    {
      if (CGUIKeyboardFactory::ShowAndGetInput(value, g_localizeStrings.Get(1029), true))
        CSkinSettings::Get().SetString(string, value);
    }
    else if (execute == "skin.setnumeric")
    {
      if (CGUIDialogNumeric::ShowAndGetNumber(value, g_localizeStrings.Get(611)))
        CSkinSettings::Get().SetString(string, value);
    }
    else if (execute == "skin.setimage")
    {
      if (params.size() > 2)
      {
        value = params[2];
        URIUtils::AddSlashAtEnd(value);
        bool bIsSource;
        if (CUtil::GetMatchingSource(value,localShares,bIsSource) < 0) // path is outside shares - add it as a separate one
        {
          CMediaSource share;
          share.strName = g_localizeStrings.Get(13278);
          share.strPath = value;
          localShares.push_back(share);
        }
      }
      if (CGUIDialogFileBrowser::ShowAndGetImage(localShares, g_localizeStrings.Get(1030), value))
        CSkinSettings::Get().SetString(string, value);
    }
    else if (execute == "skin.setlargeimage")
    {
      VECSOURCES *shares = CMediaSourceSettings::Get().GetSources("pictures");
      if (!shares) shares = &localShares;
      if (CGUIDialogFileBrowser::ShowAndGetImage(*shares, g_localizeStrings.Get(1030), value))
        CSkinSettings::Get().SetString(string, value);
    }
    else if (execute == "skin.setfile")
    {
      // Note. can only browse one addon type from here
      // if browsing for addons, required param[1] is addontype string, with optional param[2]
      // as contenttype string see IAddon.h & ADDON::TranslateXX
      std::string strMask = (params.size() > 1) ? params[1] : "";
      StringUtils::ToLower(strMask);
      ADDON::TYPE type;
      if ((type = TranslateType(strMask)) != ADDON_UNKNOWN)
      {
        CURL url;
        url.SetProtocol("addons");
        url.SetHostName("enabled");
        url.SetFileName(strMask+"/");
        localShares.clear();
        std::string content = (params.size() > 2) ? params[2] : "";
        StringUtils::ToLower(content);
        url.SetPassword(content);
        std::string strMask;
        if (type == ADDON_SCRIPT)
          strMask = ".py";
        std::string replace;
        if (CGUIDialogFileBrowser::ShowAndGetFile(url.Get(), strMask, TranslateType(type, true), replace, true, true, true))
        {
          if (StringUtils::StartsWithNoCase(replace, "addons://"))
            CSkinSettings::Get().SetString(string, URIUtils::GetFileName(replace));
          else
            CSkinSettings::Get().SetString(string, replace);
        }
      }
      else 
      {
        if (params.size() > 2)
        {
          value = params[2];
          URIUtils::AddSlashAtEnd(value);
          bool bIsSource;
          if (CUtil::GetMatchingSource(value,localShares,bIsSource) < 0) // path is outside shares - add it as a separate one
          {
            CMediaSource share;
            share.strName = g_localizeStrings.Get(13278);
            share.strPath = value;
            localShares.push_back(share);
          }
        }
        if (CGUIDialogFileBrowser::ShowAndGetFile(localShares, strMask, g_localizeStrings.Get(1033), value))
          CSkinSettings::Get().SetString(string, value);
      }
    }
    else // execute == "skin.setpath"))
    {
      g_mediaManager.GetNetworkLocations(localShares);
      if (params.size() > 1)
      {
        value = params[1];
        URIUtils::AddSlashAtEnd(value);
        bool bIsSource;
        if (CUtil::GetMatchingSource(value,localShares,bIsSource) < 0) // path is outside shares - add it as a separate one
        {
          CMediaSource share;
          share.strName = g_localizeStrings.Get(13278);
          share.strPath = value;
          localShares.push_back(share);
        }
      }
      if (CGUIDialogFileBrowser::ShowAndGetDirectory(localShares, g_localizeStrings.Get(1031), value))
        CSkinSettings::Get().SetString(string, value);
    }
    CSettings::Get().Save();
  }
  else if (execute == "skin.setaddon" && params.size() > 1)
  {
    int string = CSkinSettings::Get().TranslateString(params[0]);
    vector<ADDON::TYPE> types;
    for (unsigned int i = 1 ; i < params.size() ; i++)
    {
      ADDON::TYPE type = TranslateType(params[i]);
      if (type != ADDON_UNKNOWN)
        types.push_back(type);
    }
    std::string result;
    if (types.size() > 0 && CGUIWindowAddonBrowser::SelectAddonID(types, result, true) == 1)
    {
      CSkinSettings::Get().SetString(string, result);
      CSettings::Get().Save();
    }
  }
  else if (execute == "dialog.close" && params.size())
  {
    bool bForce = false;
    if (params.size() > 1 && StringUtils::EqualsNoCase(params[1], "true"))
      bForce = true;
    if (StringUtils::EqualsNoCase(params[0], "all"))
    {
      g_windowManager.CloseDialogs(bForce);
    }
    else
    {
      int id = CButtonTranslator::TranslateWindow(params[0]);
      CGUIWindow *window = (CGUIWindow *)g_windowManager.GetWindow(id);
      if (window && window->IsDialog())
        ((CGUIDialog *)window)->Close(bForce);
    }
  }
  else if (execute == "system.logoff")
  {
    // there was a commit from cptspiff here which was reverted
    // for keeping the behaviour from Eden in Frodo - see
    // git rev 9ee5f0047b
    if (g_windowManager.GetActiveWindow() == WINDOW_LOGIN_SCREEN)
      return -1;

    g_application.StopPlaying();
    if (g_application.IsMusicScanning())
      g_application.StopMusicScan();

    if (CVideoLibraryQueue::Get().IsRunning())
      CVideoLibraryQueue::Get().CancelAllJobs();

    ADDON::CAddonMgr::Get().StopServices(true);

    g_application.getNetwork().NetworkMessage(CNetwork::SERVICES_DOWN,1);
    CProfilesManager::Get().LoadMasterProfileForLogin();
    g_passwordManager.bMasterUser = false;

    if (!ActivateWindow(WINDOW_LOGIN_SCREEN))
      return false;

    if (!CNetworkServices::Get().StartEventServer()) // event server could be needed in some situations
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(33102), g_localizeStrings.Get(33100));
  }
  else if (execute == "pagedown")
  {
    int id = atoi(parameter.c_str());
    CGUIMessage message(GUI_MSG_PAGE_DOWN, g_windowManager.GetFocusedWindow(), id);
    g_windowManager.SendMessage(message);
  }
  else if (execute == "pageup")
  {
    int id = atoi(parameter.c_str());
    CGUIMessage message(GUI_MSG_PAGE_UP, g_windowManager.GetFocusedWindow(), id);
    g_windowManager.SendMessage(message);
  }
  else if (execute == "updatelibrary" && !params.empty())
  {
    bool userInitiated = true;
    if (params.size() > 2)
      userInitiated = StringUtils::EqualsNoCase(params[2], "true");
    if (StringUtils::EqualsNoCase(params[0], "music"))
    {
      if (g_application.IsMusicScanning())
        g_application.StopMusicScan();
      else
        g_application.StartMusicScan(params.size() > 1 ? params[1] : "", userInitiated);
    }
    if (StringUtils::EqualsNoCase(params[0], "video"))
    {
      if (g_application.IsVideoScanning())
        g_application.StopVideoScan();
      else
        g_application.StartVideoScan(params.size() > 1 ? params[1] : "", userInitiated);
    }
  }
  else if (execute == "cleanlibrary")
  {
    bool userInitiated = true;
    if (params.size() > 1)
      userInitiated = StringUtils::EqualsNoCase(params[1], "true");
    if (!params.size() || StringUtils::EqualsNoCase(params[0], "video"))
    {
      if (!g_application.IsVideoScanning())
         g_application.StartVideoCleanup(userInitiated);
      else
        CLog::Log(LOGERROR, "CleanLibrary is not possible while scanning or cleaning");
    }
    else if (StringUtils::EqualsNoCase(params[0], "music"))
    {
      if (!g_application.IsMusicScanning())
        g_application.StartMusicCleanup(userInitiated);
      else
        CLog::Log(LOGERROR, "CleanLibrary is not possible while scanning for media info");
    }
  }
  else if (execute == "exportlibrary" && !params.empty())
  {
    int iHeading = 647;
    if (StringUtils::EqualsNoCase(params[0], "music"))
      iHeading = 20196;
    std::string path;
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);
    g_mediaManager.GetNetworkLocations(shares);
    g_mediaManager.GetRemovableDrives(shares);
    bool singleFile;
    bool thumbs=false;
    bool actorThumbs=false;
    bool overwrite=false;
    bool cancelled=false;

    if (params.size() > 1)
      singleFile = StringUtils::EqualsNoCase(params[1], "true");
    else
      singleFile = CGUIDialogYesNo::ShowAndGetInput(iHeading, 20426, cancelled, 20428, 20429);

    if (cancelled)
        return -1;

    if (singleFile)
    {
      if (params.size() > 2)
        thumbs = StringUtils::EqualsNoCase(params[2], "true");
      else
        thumbs = CGUIDialogYesNo::ShowAndGetInput(iHeading, 20430, cancelled);
    }

    if (cancelled)
      return -1;

    if (thumbs && StringUtils::EqualsNoCase(params[0], "video"))
    {
      if (params.size() > 4)
        actorThumbs = StringUtils::EqualsNoCase(params[4], "true");
      else
        actorThumbs = CGUIDialogYesNo::ShowAndGetInput(iHeading, 20436, cancelled);
    }

    if (cancelled)
      return -1;

    if (singleFile)
    {
      if (params.size() > 3)
        overwrite = StringUtils::EqualsNoCase(params[3], "true");
      else
        overwrite = CGUIDialogYesNo::ShowAndGetInput(iHeading, 20431, cancelled);
    }

    if (cancelled)
      return -1;

    if (params.size() > 2)
      path=params[2];
    if (singleFile || !path.empty() ||
        CGUIDialogFileBrowser::ShowAndGetDirectory(shares,
				  g_localizeStrings.Get(661), path, true))
    {
      if (StringUtils::EqualsNoCase(params[0], "video"))
      {
        CVideoDatabase videodatabase;
        videodatabase.Open();
        videodatabase.ExportToXML(path, singleFile, thumbs, actorThumbs, overwrite);
        videodatabase.Close();
      }
      else
      {
        if (URIUtils::HasSlashAtEnd(path))
          path = URIUtils::AddFileToFolder(path, "musicdb.xml");
        CMusicDatabase musicdatabase;
        musicdatabase.Open();
        musicdatabase.ExportToXML(path, singleFile, thumbs, overwrite);
        musicdatabase.Close();
      }
    }
  }
  else if (execute == "control.move" && params.size() > 1)
  {
    CGUIMessage message(GUI_MSG_MOVE_OFFSET, g_windowManager.GetFocusedWindow(), atoi(params[0].c_str()), atoi(params[1].c_str()));
    g_windowManager.SendMessage(message);
  }
  else if (execute == "container.refresh")
  { // NOTE: These messages require a media window, thus they're sent to the current activewindow.
    //       This shouldn't stop a dialog intercepting it though.
    CGUIMessage message(GUI_MSG_NOTIFY_ALL, g_windowManager.GetActiveWindow(), 0, GUI_MSG_UPDATE, 1); // 1 to reset the history
    message.SetStringParam(parameter);
    g_windowManager.SendMessage(message);
  }
  else if (execute == "container.update" && !params.empty())
  {
    CGUIMessage message(GUI_MSG_NOTIFY_ALL, g_windowManager.GetActiveWindow(), 0, GUI_MSG_UPDATE, 0);
    message.SetStringParam(params[0]);
    if (params.size() > 1 && StringUtils::EqualsNoCase(params[1], "replace"))
      message.SetParam2(1); // reset the history
    g_windowManager.SendMessage(message);
  }
  else if (execute == "container.nextviewmode")
  {
    CGUIMessage message(GUI_MSG_CHANGE_VIEW_MODE, g_windowManager.GetActiveWindow(), 0, 0, 1);
    g_windowManager.SendMessage(message);
  }
  else if (execute == "container.previousviewmode")
  {
    CGUIMessage message(GUI_MSG_CHANGE_VIEW_MODE, g_windowManager.GetActiveWindow(), 0, 0, -1);
    g_windowManager.SendMessage(message);
  }
  else if (execute == "container.setviewmode")
  {
    CGUIMessage message(GUI_MSG_CHANGE_VIEW_MODE, g_windowManager.GetActiveWindow(), 0, atoi(parameter.c_str()));
    g_windowManager.SendMessage(message);
  }
  else if (execute == "container.nextsortmethod")
  {
    CGUIMessage message(GUI_MSG_CHANGE_SORT_METHOD, g_windowManager.GetActiveWindow(), 0, 0, 1);
    g_windowManager.SendMessage(message);
  }
  else if (execute == "container.previoussortmethod")
  {
    CGUIMessage message(GUI_MSG_CHANGE_SORT_METHOD, g_windowManager.GetActiveWindow(), 0, 0, -1);
    g_windowManager.SendMessage(message);
  }
  else if (execute == "container.setsortmethod")
  {
    CGUIMessage message(GUI_MSG_CHANGE_SORT_METHOD, g_windowManager.GetActiveWindow(), 0, atoi(parameter.c_str()));
    g_windowManager.SendMessage(message);
  }
  else if (execute == "container.sortdirection")
  {
    CGUIMessage message(GUI_MSG_CHANGE_SORT_DIRECTION, g_windowManager.GetActiveWindow(), 0, 0);
    g_windowManager.SendMessage(message);
  }
  else if (execute == "control.message" && params.size() >= 2)
  {
    int controlID = atoi(params[0].c_str());
    int windowID = (params.size() == 3) ? CButtonTranslator::TranslateWindow(params[2]) : g_windowManager.GetActiveWindow();
    if (params[1] == "moveup")
      g_windowManager.SendMessage(GUI_MSG_MOVE_OFFSET, windowID, controlID, 1);
    else if (params[1] == "movedown")
      g_windowManager.SendMessage(GUI_MSG_MOVE_OFFSET, windowID, controlID, -1);
    else if (params[1] == "pageup")
      g_windowManager.SendMessage(GUI_MSG_PAGE_UP, windowID, controlID);
    else if (params[1] == "pagedown")
      g_windowManager.SendMessage(GUI_MSG_PAGE_DOWN, windowID, controlID);
    else if (params[1] == "click")
      g_windowManager.SendMessage(GUI_MSG_CLICKED, controlID, windowID);
  }
  else if (execute == "sendclick" && !params.empty())
  {
    if (params.size() == 2)
    {
      // have a window - convert it
      int windowID = CButtonTranslator::TranslateWindow(params[0]);
      CGUIMessage message(GUI_MSG_CLICKED, atoi(params[1].c_str()), windowID);
      g_windowManager.SendMessage(message);
    }
    else
    { // single param - assume you meant the active window
      CGUIMessage message(GUI_MSG_CLICKED, atoi(params[0].c_str()), g_windowManager.GetActiveWindow());
      g_windowManager.SendMessage(message);
    }
  }
  else if (execute == "action" && !params.empty())
  {
    // try translating the action from our ButtonTranslator
    int actionID;
    if (CButtonTranslator::TranslateActionString(params[0].c_str(), actionID))
    {
      int windowID = params.size() == 2 ? CButtonTranslator::TranslateWindow(params[1]) : WINDOW_INVALID;
      CApplicationMessenger::Get().SendAction(CAction(actionID), windowID);
    }
  }
  else if (execute == "setproperty" && params.size() >= 2)
  {
    CGUIWindow *window = g_windowManager.GetWindow(params.size() > 2 ? CButtonTranslator::TranslateWindow(params[2]) : g_windowManager.GetFocusedWindow());
    if (window)
      window->SetProperty(params[0],params[1]);
  }
  else if (execute == "clearproperty" && !params.empty())
  {
    CGUIWindow *window = g_windowManager.GetWindow(params.size() > 1 ? CButtonTranslator::TranslateWindow(params[1]) : g_windowManager.GetFocusedWindow());
    if (window)
      window->SetProperty(params[0],"");
  }
  else if (execute == "wakeonlan")
  {
    g_application.getNetwork().WakeOnLan((char*)params[0].c_str());
  }
  else if (execute == "addon.default.opensettings" && params.size() == 1)
  {
    AddonPtr addon;
    ADDON::TYPE type = TranslateType(params[0]);
    if (CAddonMgr::Get().GetDefault(type, addon))
    {
      bool changed = CGUIDialogAddonSettings::ShowAndGetInput(addon);
      if (type == ADDON_VIZ && changed)
        g_windowManager.SendMessage(GUI_MSG_VISUALISATION_RELOAD, 0, 0);
    }
  }
  else if (execute == "addon.default.set" && params.size() == 1)
  {
    std::string addonID;
    TYPE type = TranslateType(params[0]);
    bool allowNone = false;
    if (type == ADDON_VIZ)
      allowNone = true;

    if (type != ADDON_UNKNOWN && 
        CGUIWindowAddonBrowser::SelectAddonID(type,addonID,allowNone))
    {
      CAddonMgr::Get().SetDefault(type,addonID);
      if (type == ADDON_VIZ)
        g_windowManager.SendMessage(GUI_MSG_VISUALISATION_RELOAD, 0, 0);
    }
  }
  else if (execute == "addon.opensettings" && params.size() == 1)
  {
    AddonPtr addon;
    if (CAddonMgr::Get().GetAddon(params[0], addon))
      CGUIDialogAddonSettings::ShowAndGetInput(addon);
  }
  else if (execute == "updateaddonrepos")
  {
    CAddonInstaller::Get().UpdateRepos(true);
  }
  else if (execute == "updatelocaladdons")
  {
    CAddonMgr::Get().FindAddons();
  }
  else if (execute == "toggledpms")
  {
    g_application.ToggleDPMS(true);
  }
  else if (execute == "cectogglestate")
  {
    CApplicationMessenger::Get().CECToggleState();
  }
  else if (execute == "cecactivatesource")
  {
    CApplicationMessenger::Get().CECActivateSource();
  }
  else if (execute == "cecstandby")
  {
    CApplicationMessenger::Get().CECStandby();
  }
  else if (execute == "weather.locationset" && !params.empty())
  {
    int loc = atoi(params[0].c_str());
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, 0, 0, loc);
    g_windowManager.SendMessage(msg, WINDOW_WEATHER);
  }
  else if (execute == "weather.locationnext")
  {
    CGUIMessage msg(GUI_MSG_MOVE_OFFSET, 0, 0, 1);
    g_windowManager.SendMessage(msg, WINDOW_WEATHER);
  }
  else if (execute == "weather.locationprevious")
  {
    CGUIMessage msg(GUI_MSG_MOVE_OFFSET, 0, 0, -1);
    g_windowManager.SendMessage(msg, WINDOW_WEATHER);
  }
  else if (execute == "weather.refresh")
  {
    CGUIMessage msg(GUI_MSG_MOVE_OFFSET, 0, 0, 0);
    g_windowManager.SendMessage(msg, WINDOW_WEATHER);
  }
  else if (execute == "videolibrary.search")
  {
    CGUIMessage msg(GUI_MSG_SEARCH, 0, 0, 0);
    g_windowManager.SendMessage(msg, WINDOW_VIDEO_NAV);
  }
  else if (execute == "toggledebug")
  {
    bool debug = CSettings::Get().GetBool("debug.showloginfo");
    CSettings::Get().SetBool("debug.showloginfo", !debug);
    g_advancedSettings.SetDebugMode(!debug);
  }
  else if (execute == "startpvrmanager")
  {
    g_application.StartPVRManager();
  }
  else if (execute == "stoppvrmanager")
  {
    g_application.StopPVRManager();
  }
  else if (execute == "startandroidactivity" && !params.empty())
  {
    CApplicationMessenger::Get().StartAndroidActivity(params);
  }
  else if (execute == "setstereomode" && !parameter.empty())
  {
    CAction action = CStereoscopicsManager::Get().ConvertActionCommandToAction(execute, parameter);
    if (action.GetID() != ACTION_NONE)
      CApplicationMessenger::Get().SendAction(action);
    else
    {
      CLog::Log(LOGERROR,"Builtin 'SetStereoMode' called with unknown parameter: %s", parameter.c_str());
      return -2;
    }
  }
  else
    return CInputManager::Get().ExecuteBuiltin(execute, params);
  return 0;
}
