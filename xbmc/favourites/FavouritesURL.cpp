/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FavouritesURL.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "addons/AddonManager.h"
#include "addons/IAddon.h"
#include "guilib/LocalizeStrings.h"
#include "input/WindowTranslator.h"
#include "utils/ExecString.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <vector>

namespace
{
std::string GetActionString(CFavouritesURL::Action action)
{
  switch (action)
  {
    case CFavouritesURL::Action::ACTIVATE_WINDOW:
      return "activatewindow";
    case CFavouritesURL::Action::PLAY_MEDIA:
      return "playmedia";
    case CFavouritesURL::Action::SHOW_PICTURE:
      return "showpicture";
    case CFavouritesURL::Action::RUN_SCRIPT:
      return "runscript";
    case CFavouritesURL::Action::RUN_ADDON:
      return "runaddon";
    case CFavouritesURL::Action::START_ANDROID_ACTIVITY:
      return "startandroidactivity";
    default:
      return {};
  }
}

CFavouritesURL::Action GetActionId(const std::string& actionString)
{
  if (actionString == "activatewindow")
    return CFavouritesURL::Action::ACTIVATE_WINDOW;
  else if (actionString == "playmedia")
    return CFavouritesURL::Action::PLAY_MEDIA;
  else if (actionString == "showpicture")
    return CFavouritesURL::Action::SHOW_PICTURE;
  else if (actionString == "runscript")
    return CFavouritesURL::Action::RUN_SCRIPT;
  else if (actionString == "runaddon")
    return CFavouritesURL::Action::RUN_ADDON;
  else if (actionString == "startandroidactivity")
    return CFavouritesURL::Action::START_ANDROID_ACTIVITY;
  else
    return CFavouritesURL::Action::UNKNOWN;
}
} // namespace

CFavouritesURL::CFavouritesURL(const std::string& favouritesURL)
{
  const CURL url(favouritesURL);
  if (url.GetProtocol() != "favourites")
  {
    CLog::LogF(LOGERROR, "Invalid protocol");
    return;
  }

  m_exec = CExecString(CURL::Decode(url.GetHostName()));
  if (m_exec.IsValid())
    m_valid = Parse(GetActionId(m_exec.GetFunction()), m_exec.GetParams());
}

CFavouritesURL::CFavouritesURL(const CExecString& execString) : m_exec(execString)
{
  if (m_exec.IsValid())
    m_valid = Parse(GetActionId(m_exec.GetFunction()), m_exec.GetParams());
}

CFavouritesURL::CFavouritesURL(Action action, const std::vector<std::string>& params)
  : m_exec(GetActionString(action), params)
{
  if (m_exec.IsValid())
    m_valid = Parse(action, params);
}

CFavouritesURL::CFavouritesURL(const CFileItem& item, int contextWindow)
  : m_exec(item, std::to_string(contextWindow))
{
  if (m_exec.IsValid())
    m_valid = Parse(GetActionId(m_exec.GetFunction()), m_exec.GetParams());
}

bool CFavouritesURL::Parse(CFavouritesURL::Action action, const std::vector<std::string>& params)
{
  m_action = action;

  bool pathIsAddonID = false;

  switch (action)
  {
    case Action::ACTIVATE_WINDOW:
      if (params.empty())
      {
        CLog::LogF(LOGERROR, "Missing parameter");
        return false;
      }

      // mandatory: window name/id
      m_windowId = CWindowTranslator::TranslateWindow(params[0]);

      if (params.size() > 1)
      {
        // optional: target url/path
        m_target = StringUtils::DeParamify(params[1]);
      }
      m_actionLabel =
          StringUtils::Format(g_localizeStrings.Get(15220), // Show content in '<windowname>'
                              g_localizeStrings.Get(m_windowId));
      break;
    case Action::PLAY_MEDIA:
      if (params.empty())
      {
        CLog::LogF(LOGERROR, "Missing parameter");
        return false;
      }
      m_target = StringUtils::DeParamify(params[0]);
      m_actionLabel = g_localizeStrings.Get(15218); // Play media
      break;
    case Action::SHOW_PICTURE:
      if (params.empty())
      {
        CLog::LogF(LOGERROR, "Missing parameter");
        return false;
      }
      m_target = StringUtils::DeParamify(params[0]);
      m_actionLabel = g_localizeStrings.Get(15219); // Show picture
      break;
    case Action::RUN_SCRIPT:
      if (params.empty())
      {
        CLog::LogF(LOGERROR, "Missing parameter");
        return false;
      }
      m_target = StringUtils::DeParamify(params[0]);
      m_actionLabel = g_localizeStrings.Get(15221); // Execute script
      pathIsAddonID = true;
      break;
    case Action::RUN_ADDON:
      if (params.empty())
      {
        CLog::LogF(LOGERROR, "Missing parameter");
        return false;
      }
      m_target = StringUtils::DeParamify(params[0]);
      m_actionLabel = g_localizeStrings.Get(15223); // Execute add-on
      pathIsAddonID = true;
      break;
    case Action::START_ANDROID_ACTIVITY:
      if (params.empty())
      {
        CLog::LogF(LOGERROR, "Missing parameter");
        return false;
      }
      m_target = StringUtils::DeParamify(params[0]);
      m_actionLabel = g_localizeStrings.Get(15222); // Execute Android app
      break;
    default:
      if (params.empty())
      {
        CLog::LogF(LOGERROR, "Missing parameter");
        return false;
      }
      m_action = CFavouritesURL::Action::UNKNOWN;
      m_target = StringUtils::DeParamify(params[0]);
      m_actionLabel = g_localizeStrings.Get(15224); // Other / Unknown
      break;
  }

  m_path = StringUtils::Format("favourites://{}", CURL::Encode(GetExecString()));
  m_isDir = URIUtils::HasSlashAtEnd(m_target, true);

  if (pathIsAddonID || URIUtils::IsPlugin(m_target))
  {
    // get the add-on name
    const std::string plugin = pathIsAddonID ? m_target : CURL(m_target).GetHostName();

    ADDON::AddonPtr addon;
    CServiceBroker::GetAddonMgr().GetAddon(plugin, addon, ADDON::OnlyEnabled::CHOICE_NO);
    if (addon)
      m_providerLabel = addon->Name();
  }
  if (m_providerLabel.empty())
    m_providerLabel = CSysInfo::GetAppName();

  return true;
}
