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
#include "music/tags/MusicInfoTag.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoInfoTag.h"

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
void SplitParams(const std::string& paramString, std::vector<std::string>& parameters)
{
  bool inQuotes = false;
  bool lastEscaped = false; // only every second character can be escaped
  int inFunction = 0;
  size_t whiteSpacePos = 0;
  std::string parameter;
  parameters.clear();
  for (size_t pos = 0; pos < paramString.size(); pos++)
  {
    char ch = paramString[pos];
    bool escaped = (pos > 0 && paramString[pos - 1] == '\\' && !lastEscaped);
    lastEscaped = escaped;
    if (inQuotes)
    { // if we're in a quote, we accept everything until the closing quote
      if (ch == '"' && !escaped)
      { // finished a quote - no need to add the end quote to our string
        inQuotes = false;
      }
    }
    else
    { // not in a quote, so check if we should be starting one
      if (ch == '"' && !escaped)
      { // start of quote - no need to add the quote to our string
        inQuotes = true;
      }
      if (inFunction && ch == ')')
      { // end of a function
        inFunction--;
      }
      if (ch == '(')
      { // start of function
        inFunction++;
      }
      if (!inFunction && ch == ',')
      { // not in a function, so a comma signifies the end of this parameter
        if (whiteSpacePos)
          parameter.resize(whiteSpacePos);
        // trim off start and end quotes
        if (parameter.length() > 1 && parameter[0] == '"' &&
            parameter[parameter.length() - 1] == '"')
          parameter = parameter.substr(1, parameter.length() - 2);
        else if (parameter.length() > 3 && parameter[parameter.length() - 1] == '"')
        {
          // check name="value" style param.
          size_t quotaPos = parameter.find('"');
          if (quotaPos > 1 && quotaPos < parameter.length() - 1 && parameter[quotaPos - 1] == '=')
          {
            parameter.erase(parameter.length() - 1);
            parameter.erase(quotaPos);
          }
        }
        parameters.push_back(parameter);
        parameter.clear();
        whiteSpacePos = 0;
        continue;
      }
    }
    if ((ch == '"' || ch == '\\') && escaped)
    { // escaped quote or backslash
      parameter[parameter.size() - 1] = ch;
      continue;
    }
    // whitespace handling - we skip any whitespace at the left or right of an unquoted parameter
    if (ch == ' ' && !inQuotes)
    {
      if (parameter.empty()) // skip whitespace on left
        continue;
      if (!whiteSpacePos) // make a note of where whitespace starts on the right
        whiteSpacePos = parameter.size();
    }
    else
      whiteSpacePos = 0;
    parameter += ch;
  }
  if (inFunction || inQuotes)
    CLog::Log(LOGWARNING, "{}({}) - end of string while searching for ) or \"", __FUNCTION__,
              paramString);
  if (whiteSpacePos)
    parameter.erase(whiteSpacePos);
  // trim off start and end quotes
  if (parameter.size() > 1 && parameter[0] == '"' && parameter[parameter.size() - 1] == '"')
    parameter = parameter.substr(1, parameter.size() - 2);
  else if (parameter.size() > 3 && parameter[parameter.size() - 1] == '"')
  {
    // check name="value" style param.
    size_t quotaPos = parameter.find('"');
    if (quotaPos > 1 && quotaPos < parameter.length() - 1 && parameter[quotaPos - 1] == '=')
    {
      parameter.erase(parameter.length() - 1);
      parameter.erase(quotaPos);
    }
  }
  if (!parameter.empty() || parameters.size())
    parameters.push_back(parameter);
}

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

  SplitParams(paramString, parameters);
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
    if (item.IsVideoDb() && item.HasVideoInfoTag())
      BuildPlayMedia(item, StringUtils::Paramify(item.GetVideoInfoTag()->m_strFileNameAndPath));
    else if (item.IsMusicDb() && item.HasMusicInfoTag())
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
