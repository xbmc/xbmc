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
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogHelper.h"
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
#include "GUIUserMessages.h"
#include "windows/GUIWindowLoginScreen.h"
#include "video/windows/GUIWindowVideoBase.h"
#include "addons/GUIWindowAddonBrowser.h"
#include "addons/Addon.h" // for TranslateType, TranslateContent
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/PluginSource.h"
#include "addons/RepositoryUpdater.h"
#include "addons/Skin.h"
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
#include "utils/Variant.h"
#include "video/VideoLibraryQueue.h"
#include "Util.h"
#include "URL.h"
#include "music/MusicDatabase.h"
#include "cores/IPlayer.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/recordings/PVRRecording.h"
#include "pvr/PVRManager.h"

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

#include "AddonBuiltins.h"
#include "CECBuiltins.h"
#include "GUIBuiltins.h"
#include "GUIControlBuiltins.h"
#include "GUIContainerBuiltins.h"
#include "LibraryBuiltins.h"
#include "OpticalBuiltins.h"
#include "PictureBuiltins.h"
#include "PlayerBuiltins.h"
#include "ProfileBuiltins.h"
#include "PVRBuiltins.h"
#include "SkinBuiltins.h"
#include "SystemBuiltins.h"
#include "WeatherBuiltins.h"

using namespace XFILE;
using namespace ADDON;
using namespace KODI::MESSAGING;

#ifdef HAS_DVD_DRIVE
using namespace MEDIA_DETECT;
#endif

using KODI::MESSAGING::HELPERS::DialogResponse;

typedef struct
{
  const char* command;
  bool needsParameters;
  const char* description;
} BUILT_IN;

const BUILT_IN commands[] = {
  { "Help",                       false,  "This help message" },
  { "NotifyAll",                  true,   "Notify all connected clients" },
  { "Extract",                    true,   "Extracts the specified archive" },
  { "Mute",                       false,  "Mute the player" },
  { "SetVolume",                  true,   "Set the current volume" },
  { "WakeOnLan",                  true,   "Sends the wake-up packet to the broadcast address for the specified MAC address" },
  { "ToggleDPMS",                 false,  "Toggle DPMS mode manually"},
  { "ToggleDebug",                false,  "Enables/disables debug mode" },
#if defined(TARGET_ANDROID)
  { "StartAndroidActivity",       true,   "Launch an Android native app with the given package name.  Optional parms (in order): intent, dataType, dataURI." },
#endif
};

CBuiltins::CBuiltins()
{
  RegisterCommands<CAddonBuiltins>();
  RegisterCommands<CGUIBuiltins>();
  RegisterCommands<CGUIContainerBuiltins>();
  RegisterCommands<CGUIControlBuiltins>();
  RegisterCommands<CLibraryBuiltins>();
  RegisterCommands<COpticalBuiltins>();
  RegisterCommands<CPictureBuiltins>();
  RegisterCommands<CPlayerBuiltins>();
  RegisterCommands<CProfileBuiltins>();
  RegisterCommands<CPVRBuiltins>();
  RegisterCommands<CSkinBuiltins>();
  RegisterCommands<CSystemBuiltins>();
  RegisterCommands<CWeatherBuiltins>();
}

CBuiltins::~CBuiltins()
{
}

CBuiltins& CBuiltins::GetInstance()
{
  static CBuiltins sBuiltins;
  return sBuiltins;
}

bool CBuiltins::HasCommand(const std::string& execString)
{
  std::string function;
  std::vector<std::string> parameters;
  CUtil::SplitExecFunction(execString, function, parameters);
  StringUtils::ToLower(function);
  const auto& it = m_command.find(function);
  if (it != m_command.end())
  {
    if (it->second.parameters == 0 || it->second.parameters <= parameters.size())
      return true;
  }
  else
  {
    for (unsigned int i = 0; i < sizeof(commands)/sizeof(BUILT_IN); i++)
    {
      if (StringUtils::EqualsNoCase(function, commands[i].command) && (!commands[i].needsParameters || parameters.size()))
        return true;
    }
  }

  return false;
}

bool CBuiltins::IsSystemPowerdownCommand(const std::string& execString)
{
  std::string execute;
  std::vector<std::string> params;
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
    switch (CSettings::GetInstance().GetInt(CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNSTATE))
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

  for (const auto& it : m_command)
  {
    help += it.first;
    help += "\t";
    help += it.second.description;
    help += "\n";
  }

  for (unsigned int i = 0; i < sizeof(commands)/sizeof(BUILT_IN); i++)
  {
    help += commands[i].command;
    help += "\t";
    help += commands[i].description;
    help += "\n";
  }
}

int CBuiltins::Execute(const std::string& execString)
{
  // Deprecated. Get the text after the "XBMC."
  std::string execute;
  std::vector<std::string> params;
  CUtil::SplitExecFunction(execString, execute, params);
  StringUtils::ToLower(execute);
  std::string parameter = params.size() ? params[0] : "";
  std::string paramlow(parameter);
  StringUtils::ToLower(paramlow);

  const auto& it = m_command.find(execute);
  if (it != m_command.end())
  {
    if (it->second.parameters == 0 || params.size() >= it->second.parameters)
      return it->second.Execute(params);
    else
    {
      CLog::Log(LOGERROR, "%s called with invalid number of parameters (should be: %" PRIdS ", is %" PRIdS")",
                          execute.c_str(), it->second.parameters, params.size());
      return -1;
    }
  }

  if (execute == "extract" && params.size())
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
  else if (execute == "notifyall")
  {
    if (params.size() > 1)
    {
      CVariant data;
      if (params.size() > 2)
        data = CJSONVariantParser::Parse((const unsigned char *)params[2].c_str(), params[2].size());

      ANNOUNCEMENT::CAnnouncementManager::GetInstance().Announce(ANNOUNCEMENT::Other, params[0].c_str(), params[1].c_str(), data);
    }
    else
      CLog::Log(LOGERROR, "NotifyAll needs two parameters");
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
        CApplicationMessenger::GetInstance().PostMsg(TMSG_VOLUME_SHOW, oldVolume < volume ? ACTION_VOLUME_UP : ACTION_VOLUME_DOWN);
      }
    }
  }
  else if (execute == "wakeonlan")
  {
    g_application.getNetwork().WakeOnLan((char*)params[0].c_str());
  }
  else if (execute == "toggledpms")
  {
    g_application.ToggleDPMS(true);
  }
  else if (execute == "toggledebug")
  {
    bool debug = CSettings::GetInstance().GetBool(CSettings::SETTING_DEBUG_SHOWLOGINFO);
    CSettings::GetInstance().SetBool(CSettings::SETTING_DEBUG_SHOWLOGINFO, !debug);
    g_advancedSettings.SetDebugMode(!debug);
  }
  else if (execute == "startandroidactivity" && !params.empty())
  {
    CApplicationMessenger::GetInstance().PostMsg(TMSG_START_ANDROID_ACTIVITY, -1, -1, static_cast<void*>(&params));
  }
  else
    return CInputManager::GetInstance().ExecuteBuiltin(execute, params);
  return 0;
}
