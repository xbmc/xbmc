/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "UrlOptions.h"

#include "URL.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

CUrlOptions::CUrlOptions() = default;

CUrlOptions::CUrlOptions(const std::string &options, const char *strLead /* = "" */)
  : m_strLead(strLead)
{
  AddOptions(options);
}

CUrlOptions::~CUrlOptions() = default;

std::string CUrlOptions::GetOptionsString(bool withLeadingSeparator /* = false */) const
{
  std::string options;
  for (const auto &opt : m_options)
  {
    if (!options.empty())
      options += "&";

    options += CURL::Encode(opt.first);
    if (!opt.second.empty())
      options += "=" + CURL::Encode(opt.second.asString());
  }

  if (withLeadingSeparator && !options.empty())
  {
    if (m_strLead.empty())
      options = "?" + options;
    else
      options = m_strLead + options;
  }

  return options;
}

void CUrlOptions::AddOption(const std::string &key, const char *value)
{
  if (key.empty() || value == NULL)
    return;

  return AddOption(key, std::string(value));
}

void CUrlOptions::AddOption(const std::string &key, const std::string &value)
{
  if (key.empty())
    return;

  m_options[key] = value;
}

void CUrlOptions::AddOption(const std::string &key, int value)
{
  if (key.empty())
    return;

  m_options[key] = value;
}

void CUrlOptions::AddOption(const std::string &key, float value)
{
  if (key.empty())
    return;

  m_options[key] = value;
}

void CUrlOptions::AddOption(const std::string &key, double value)
{
  if (key.empty())
    return;

  m_options[key] = value;
}

void CUrlOptions::AddOption(const std::string &key, bool value)
{
  if (key.empty())
    return;

  m_options[key] = value;
}

void CUrlOptions::AddOptions(const std::string &options)
{
  if (options.empty())
    return;

  std::string strOptions = options;

  // if matching the preset leading str, remove from options.
  if (!m_strLead.empty() && strOptions.compare(0, m_strLead.length(), m_strLead) == 0)
    strOptions.erase(0, m_strLead.length());
  else if (strOptions.at(0) == '?' || strOptions.at(0) == '#' || strOptions.at(0) == ';' || strOptions.at(0) == '|')
  {
    // remove leading ?, #, ; or | if present
    if (!m_strLead.empty())
      CLog::Log(LOGWARNING, "%s: original leading str %s overridden by %c", __FUNCTION__, m_strLead.c_str(), strOptions.at(0));
    m_strLead = strOptions.at(0);
    strOptions.erase(0, 1);
  }

  // split the options by & and process them one by one
  for (const auto &option : StringUtils::Split(strOptions, "&"))
  {
    if (option.empty())
      continue;

    std::string key, value;

    size_t pos = option.find('=');
    key = CURL::Decode(option.substr(0, pos));
    if (pos != std::string::npos)
      value = CURL::Decode(option.substr(pos + 1));

    // the key cannot be empty
    if (!key.empty())
      AddOption(key, value);
  }
}

void CUrlOptions::AddOptions(const CUrlOptions &options)
{
  m_options.insert(options.m_options.begin(), options.m_options.end());
}

void CUrlOptions::RemoveOption(const std::string &key)
{
  if (key.empty())
    return;

  auto option = m_options.find(key);
  if (option != m_options.end())
    m_options.erase(option);
}

bool CUrlOptions::HasOption(const std::string &key) const
{
  if (key.empty())
    return false;

  return m_options.find(key) != m_options.end();
}

bool CUrlOptions::GetOption(const std::string &key, CVariant &value) const
{
  if (key.empty())
    return false;

  auto option = m_options.find(key);
  if (option == m_options.end())
    return false;

  value = option->second;
  return true;
}
