/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ExecString.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "music/MusicFileItemClassify.h"
#include "music/tags/MusicInfoTag.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoTag.h"

using namespace KODI;

CExecString::CExecString(const std::string& execString)
{
  m_valid = Parse(execString);
}

CExecString::CExecString(const std::string& function, const std::vector<std::string>& params)
  : m_function(function), m_params(params)
{
  m_valid = !m_function.empty();

  if (m_valid)
    SetExecString();
}

CExecString::CExecString(const std::string& function,
                         const CFileItem& target,
                         const std::string& param)
  : m_function(function)
{
  m_valid = !m_function.empty() && !target.GetPath().empty();

  m_params.emplace_back(StringUtils::Paramify(target.GetPath()));

  if (target.m_bIsFolder)
    m_params.emplace_back("isdir");

  if (!param.empty())
    m_params.emplace_back(param);

  if (m_valid)
    SetExecString();
}

CExecString::CExecString(const CFileItem& item, const std::string& contextWindow)
{
  m_valid = Parse(item, contextWindow);
}

namespace
{
void SplitExecFunction(const std::string& execString,
                       std::string& function,
                       std::vector<std::string>& parameters)
{
  std::string paramString;

  size_t iPos = execString.find('(');
  size_t iPos2 = execString.rfind(')');
  if (iPos != std::string::npos && iPos2 != std::string::npos)
  {
    paramString = execString.substr(iPos + 1, iPos2 - iPos - 1);
    function = execString.substr(0, iPos);
  }
  else
    function = execString;

  // remove any whitespace, and the standard prefix (if it exists)
  StringUtils::Trim(function);

  CUtil::SplitParams(paramString, parameters);
}
} // namespace

bool CExecString::Parse(const std::string& execString)
{
  m_execString = execString;
  SplitExecFunction(m_execString, m_function, m_params);

  // Keep original function case in execstring, lowercase it in function
  StringUtils::ToLower(m_function);
  return true;
}

bool CExecString::Parse(const CFileItem& item, const std::string& contextWindow)
{
  if (item.IsFavourite())
  {
    const CURL url(item.GetPath());
    Parse(CURL::Decode(url.GetHostName()));
  }
  else if (item.m_bIsFolder &&
           (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_playlistAsFolders ||
            !(item.IsSmartPlayList() || item.IsPlayList())))
  {
    if (!contextWindow.empty())
      Build("ActivateWindow", {contextWindow, StringUtils::Paramify(item.GetPath()), "return"});
  }
  else if (item.IsScript() && item.GetPath().size() > 9) // script://<foo>
    Build("RunScript", {StringUtils::Paramify(item.GetPath().substr(9))});
  else if (item.IsAddonsPath() && item.GetPath().size() > 9) // addons://<foo>
  {
    const CURL url(item.GetPath());
    if (url.GetHostName() == "install")
      Build("InstallFromZip", {});
    else if (url.GetHostName() == "check_for_updates")
      Build("UpdateAddonRepos", {"showProgress"});
    else
      Build("RunAddon", {StringUtils::Paramify(url.GetFileName())});
  }
  else if (item.IsAndroidApp() && item.GetPath().size() > 26) // androidapp://sources/apps/<foo>
    Build("StartAndroidActivity", {StringUtils::Paramify(item.GetPath().substr(26))});
  else // assume a media file
  {
    if (VIDEO::IsVideoDb(item) && item.HasVideoInfoTag())
      BuildPlayMedia(item, StringUtils::Paramify(item.GetVideoInfoTag()->m_strFileNameAndPath));
    else if (MUSIC::IsMusicDb(item) && item.HasMusicInfoTag())
      BuildPlayMedia(item, StringUtils::Paramify(item.GetMusicInfoTag()->GetURL()));
    else if (item.IsPicture())
      Build("ShowPicture", {StringUtils::Paramify(item.GetPath())});
    else
    {
      // Everything else will be treated as PlayMedia for item's path
      BuildPlayMedia(item, StringUtils::Paramify(item.GetPath()));
    }
  }
  return true;
}

void CExecString::Build(const std::string& function, const std::vector<std::string>& params)
{
  m_function = function;
  m_params = params;
  SetExecString();
}

void CExecString::BuildPlayMedia(const CFileItem& item, const std::string& target)
{
  std::vector<std::string> params{target};

  if (item.HasProperty("playlist_type_hint"))
    params.emplace_back("playlist_type_hint=" + item.GetProperty("playlist_type_hint").asString());

  Build("PlayMedia", params);
}

void CExecString::SetExecString()
{
  if (m_params.empty())
    m_execString = m_function;
  else
    m_execString = StringUtils::Format("{}({})", m_function, StringUtils::Join(m_params, ","));

  // Keep original function case in execstring, lowercase it in function
  StringUtils::ToLower(m_function);
}
