/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "UrlOptions.h"
#include "URL.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

CUrlOptions::CUrlOptions()
{ }

CUrlOptions::CUrlOptions(const std::string &options, const char *strLead /* = "" */)
  : m_strLead(strLead)
{
  AddOptions(options);
}

CUrlOptions::~CUrlOptions()
{ }

std::string CUrlOptions::GetOptionsString(bool withLeadingSeperator /* = false */) const
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

  if (withLeadingSeperator && !options.empty())
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
      CLog::Log(LOGWARNING, "%s: original leading str %s overrided by %c", __FUNCTION__, m_strLead.c_str(), strOptions.at(0));
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
