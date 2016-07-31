/*
 *      Copyright (C) 2005-2015 Team XBMC
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

#include "AddonBuiltins.h"

#include <memory>

#include "addons/AddonManager.h"
#include "addons/AddonInstaller.h"
#include "addons/GUIDialogAddonSettings.h"
#include "addons/GUIWindowAddonBrowser.h"
#include "addons/PluginSource.h"
#include "addons/RepositoryUpdater.h"
#include "FileItem.h"
#include "filesystem/PluginDirectory.h"
#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#if defined(TARGET_DARWIN)
#include "filesystem/SpecialProtocol.h"
#include "platform/darwin/osx/CocoaInterface.h"
#endif

using namespace ADDON;

/*! \brief Install an addon.
 *  \param params The parameters.
 *  \details params[0] = add-on id.
 */
static int InstallAddon(const std::vector<std::string>& params)
{
  const std::string& addonid = params[0];

  AddonPtr addon;
  CAddonInstaller::GetInstance().InstallModal(addonid, addon);

  return 0;
}

/*! \brief Run a plugin.
 *  \param params The parameters.
 *  \details params[0] = plugin:// URL to script.
 */
static int RunPlugin(const std::vector<std::string>& params)
{
  if (params.size())
  {
    CFileItem item(params[0]);
    if (!item.m_bIsFolder)
    {
      item.SetPath(params[0]);
      XFILE::CPluginDirectory::RunScriptWithParams(item.GetPath());
    }
  }
  else
    CLog::Log(LOGERROR, "RunPlugin called with no arguments.");

  return 0;
}

/*! \brief Run a script or plugin add-on.
 *  \param params The parameters.
 *  \details params[0] = add-on id.
 *           params[1] is blank for no add-on parameters
 *           or
 *           params[1] = add-on parameters in url format
 *           or
 *           params[1,...] = additional parameters in format param=value.
 */
static int RunAddon(const std::vector<std::string>& params)
{
  if (params.size())
  {
    const std::string& addonid = params[0];

    AddonPtr addon;
    if (CAddonMgr::GetInstance().GetAddon(addonid, addon, ADDON_PLUGIN))
    {
      PluginPtr plugin = std::dynamic_pointer_cast<CPluginSource>(addon);
      std::string urlParameters;
      std::vector<std::string> parameters;
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
        // Pass the script name (addonid) and all the parameters
        // (params[1] ... params[x]) separated by a comma to RunPlugin
        cmd = StringUtils::Format("RunPlugin(%s)", StringUtils::Join(params, ",").c_str());
      CBuiltins::GetInstance().Execute(cmd);
    }
    else if (CAddonMgr::GetInstance().GetAddon(addonid, addon, ADDON_SCRIPT) ||
        CAddonMgr::GetInstance().GetAddon(addonid, addon, ADDON_SCRIPT_WEATHER) ||
        CAddonMgr::GetInstance().GetAddon(addonid, addon, ADDON_SCRIPT_LYRICS) ||
        CAddonMgr::GetInstance().GetAddon(addonid, addon, ADDON_SCRIPT_LIBRARY))
    {
      // Pass the script name (addonid) and all the parameters
      // (params[1] ... params[x]) separated by a comma to RunScript
      CBuiltins::GetInstance().Execute(StringUtils::Format("RunScript(%s)", StringUtils::Join(params, ",").c_str()));
    }
    else
      CLog::Log(LOGERROR, "RunAddon: unknown add-on id '%s', or unexpected add-on type (not a script or plugin).", addonid.c_str());
  }
  else
  {
    CLog::Log(LOGERROR, "RunAddon called with no arguments.");
  }

  return 0;
}

/*! \brief Run a script add-on or an apple script.
 *  \param params The parameters.
 *  \details params[0] is the URL to the apple script
 *           or
 *           params[0] is the addon-ID to the script add-on
 *           or
 *           params[0] is the URL to the python script.
 *
 *           Set the OnlyApple template parameter to true to only attempt
 *           execution of applescripts.
 */
template<bool OnlyApple=false>
static int RunScript(const std::vector<std::string>& params)
{
#if defined(TARGET_DARWIN_OSX)
  std::string execute;
  std::string parameter = params.size() ? params[0] : "";
  if (URIUtils::HasExtension(parameter, ".applescript|.scpt"))
  {
    std::string osxPath = CSpecialProtocol::TranslatePath(parameter);
    Cocoa_DoAppleScriptFile(osxPath.c_str());
  }
  else if (OnlyApple)
    return 1;
  else
#endif
  {
    AddonPtr addon;
    std::string scriptpath;
    // Test to see if the param is an addon ID
    if (CAddonMgr::GetInstance().GetAddon(params[0], addon))
    {
      //Get the correct extension point to run
      if (CAddonMgr::GetInstance().GetAddon(params[0], addon, ADDON_SCRIPT) ||
          CAddonMgr::GetInstance().GetAddon(params[0], addon, ADDON_SCRIPT_WEATHER) ||
          CAddonMgr::GetInstance().GetAddon(params[0], addon, ADDON_SCRIPT_LYRICS) ||
          CAddonMgr::GetInstance().GetAddon(params[0], addon, ADDON_SCRIPT_LIBRARY))
      {
        scriptpath = addon->LibPath();
      }
      else
      {
        //Run a random extension point (old behaviour).
        CAddonMgr::GetInstance().GetAddon(params[0], addon);
        scriptpath = addon->LibPath();
        CLog::Log(LOGWARNING, "RunScript called for a non-script addon '%s'. This behaviour is deprecated.", params[0].c_str());
      }
    }
    else
      scriptpath = params[0];

    // split the path up to find the filename
    std::vector<std::string> argv = params;
    std::string filename = URIUtils::GetFileName(scriptpath);
    if (!filename.empty())
      argv[0] = filename;

    CScriptInvocationManager::GetInstance().ExecuteAsync(scriptpath, addon, argv);
  }

  return 0;
}

/*! \brief Open the settings for the default add-on of a given type.
 *  \param params The parameters.
 *  \details params[0] = The add-on type.
 */
static int OpenDefaultSettings(const std::vector<std::string>& params)
{
  AddonPtr addon;
  ADDON::TYPE type = TranslateType(params[0]);
  if (CAddonMgr::GetInstance().GetDefault(type, addon))
  {
    bool changed = CGUIDialogAddonSettings::ShowAndGetInput(addon);
    if (type == ADDON_VIZ && changed)
      g_windowManager.SendMessage(GUI_MSG_VISUALISATION_RELOAD, 0, 0);
  }

  return 0;
}

/*! \brief Set the default add-on for a given type.
 *  \param params The parameters.
 *  \details params[0] = The add-on type
 */
static int SetDefaultAddon(const std::vector<std::string>& params)
{
  std::string addonID;
  TYPE type = TranslateType(params[0]);
  bool allowNone = false;
  if (type == ADDON_VIZ)
    allowNone = true;

  if (type != ADDON_UNKNOWN && 
      CGUIWindowAddonBrowser::SelectAddonID(type,addonID,allowNone))
  {
    CAddonMgr::GetInstance().SetDefault(type,addonID);
    if (type == ADDON_VIZ)
      g_windowManager.SendMessage(GUI_MSG_VISUALISATION_RELOAD, 0, 0);
  }

  return 0;
}

/*! \brief Open the settings for a given add-on.
 *  \param params The parameters.
 *  \details params[0] = The add-on ID.
 */
static int AddonSettings(const std::vector<std::string>& params)
{
  AddonPtr addon;
  if (CAddonMgr::GetInstance().GetAddon(params[0], addon))
    CGUIDialogAddonSettings::ShowAndGetInput(addon);

  return 0;
}

/*! \brief Stop a running script.
 *  \param params The parameters.
 *  \details params[0] = The add-on ID of the script to stop
 *           or
 *           params[0] = The URL of the script to stop.
 */
static int StopScript(const std::vector<std::string>& params)
{
  //! @todo FIXME: This does not work for addons with multiple extension points!
  //! Are there any use for this? TODO: Fix hack in CScreenSaver::Destroy() and deprecate.
  std::string scriptpath(params[0]);
  // Test to see if the param is an addon ID
  AddonPtr script;
  if (CAddonMgr::GetInstance().GetAddon(params[0], script))
    scriptpath = script->LibPath();
  CScriptInvocationManager::GetInstance().Stop(scriptpath);

  return 0;
}

/*! \brief Check add-on repositories for updates.
 *  \param params (ignored)
 */
static int UpdateRepos(const std::vector<std::string>& params)
{
  CRepositoryUpdater::GetInstance().CheckForUpdates();

  return 0;
}

/*! \brief Check local add-on directories for updates.
 *  \param params (ignored)
 */
static int UpdateLocals(const std::vector<std::string>& params)
{
  CAddonMgr::GetInstance().FindAddons();

  return 0;
}

// Note: For new Texts with comma add a "\" before!!! Is used for table text.
//
/// \page page_List_of_built_in_functions List of build in functions
/// \section built_in_functions_1 Add-on built-in's
///
/// -----------------------------------------------------------------------------
///
/// \table_start
///   \table_h2_l{
///     Function,
///     Description }
///   \table_row2_l{
///     <b>`Addon.Default.OpenSettings(extensionpoint)`</b>
///     ,
///     Open a settings dialog for the default addon of the given type
///     (extensionpoint)
///     @param[in] extensionpoint        The add-on type
///   }
///   \table_row2_l{
///     <b>`Addon.Default.Set(extensionpoint)`</b>
///     ,
///     Open a select dialog to allow choosing the default addon of the given type
///     (extensionpoint)
///     @param[in] extensionpoint        The add-on type
///   }
///   \table_row2_l{
///     <b>`Addon.OpenSettings(id)`</b>
///     ,
///     Open a settings dialog for the addon of the given id
///     @param[in] id                    The add-on ID
///   }
///   \table_row2_l{
///     <b>`InstallAddon(id)`</b>
///     ,
///     Install the specified plugin/script
///     @param[in] id                    The add-on id
///   }
///   \table_row2_l{
///     <b>`RunAddon(id[\,opt])`</b>
///     ,
///     Runs the specified plugin/script
///     @param[in] id                    The add-on id.
///     @param[in] opt                   is blank for no add-on parameters\n
///     or
///     @param[in] opt                   Add-on parameters in url format\n
///     or
///     @param[in] opt[\,...]            Additional parameters in format param=value.
///   }
///   \table_row2_l{
///     <b>`RunAppleScript(script[\,args]*)`</b>
///     ,
///     Run the specified AppleScript command
///     @param[in] script                Is the URL to the apple script\n
///     or
///     @param[in] script                Is the addon-ID to the script add-on\n
///     or
///     @param[in] script                Is the URL to the python script.
///
///     @note Set the OnlyApple template parameter to true to only attempt
///     execution of applescripts.
///   }
///   \table_row2_l{
///     <b>`RunPlugin(plugin)`</b>
///     ,
///     Runs the plugin. Full path must be specified. Does not work for folder
///     plugins
///     @param[in] plugin                plugin:// URL to script.
///   }
///   \table_row2_l{
///     <b>`RunScript(script[\,args]*)`</b>
///     ,
///     Runs the python script. You must specify the full path to the script. If
///     the script is an add-on\, you can also execute it using its add-on id. As
///     of 2007/02/24\, all extra parameters are passed to the script as arguments
///     and can be accessed by python using sys.argv
///     @param[in] script                Is the addon-ID to the script add-on\n
///     or
///     @param[in] script                Is the URL to the python script.
///   }
///   \table_row2_l{
///     <b>`StopScript(id)`</b>
///     ,
///     Stop the script by ID or path\, if running
///     @param[in] id                    The add-on ID of the script to stop\n
///     or
///     @param[in] id                    The URL of the script to stop.
///   }
///   \table_row2_l{
///     <b>`UpdateAddonRepos`</b>
///     ,
///     Triggers a forced update of enabled add-on repositories.
///   }
///   \table_row2_l{
///     <b>`UpdateLocalAddons`</b>
///     ,
///     Triggers a scan of local add-on directories.
///   }
///  \table_end
///

CBuiltins::CommandMap CAddonBuiltins::GetOperations() const
{
  return {
           {"addon.default.opensettings", {"Open a settings dialog for the default addon of the given type", 1, OpenDefaultSettings}},
           {"addon.default.set",          {"Open a select dialog to allow choosing the default addon of the given type", 1, SetDefaultAddon}},
           {"addon.opensettings",         {"Open a settings dialog for the addon of the given id", 1, AddonSettings}},
           {"installaddon",               {"Install the specified plugin/script", 1, InstallAddon}},
           {"runaddon",                   {"Run the specified plugin/script", 1, RunAddon}},
#ifdef TARGET_DARWIN
           {"runapplescript",             {"Run the specified AppleScript command", 1, RunScript<true>}},
#endif
           {"runplugin",                  {"Run the specified plugin", 1, RunPlugin}},
           {"runscript",                  {"Run the specified script", 1, RunScript}},
           {"stopscript",                 {"Stop the script by ID or path, if running", 1, StopScript}},
           {"updateaddonrepos",           {"Check add-on repositories for updates", 0, UpdateRepos}},
           {"updatelocaladdons",          {"Check for local add-on changes", 0, UpdateLocals}}
         };
}
