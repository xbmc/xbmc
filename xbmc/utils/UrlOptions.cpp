/*
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <sstream>

#include "UrlOptions.h"
#include "URL.h"
#include "utils/StringUtils.h"

using namespace std;

CUrlOptions::CUrlOptions()
  : m_strLead("?")
{ }

CUrlOptions::CUrlOptions(const std::string &options)
  : m_strLead("?")
{
  AddOptions(options);
}

CUrlOptions::~CUrlOptions()
{ }

std::string CUrlOptions::GetOptionsString(bool withLeadingSeperator /* = false */) const
{
  std::string options;
  for (UrlOptions::const_iterator opt = m_options.begin(); opt != m_options.end(); opt++)
  {
    if (opt != m_options.begin())
      options += "&";

    options += CURL::Encode(opt->first) + "=" + CURL::Encode(opt->second.asString());
  }

  if (withLeadingSeperator && !options.empty())
    options = m_strLead + options;

  return options;
}

void CUrlOptions::AddOption(const std::string &key, const char *value)
{
  if (key.empty() || value == NULL)
    return;

  return AddOption(key, string(value));
}

void CUrlOptions::AddOption(const std::string &key, const std::string &value)
{
  if (key.empty())
    return;

  UrlOptions::iterator option = m_options.find(key);
  if (!value.empty())
    m_options[key] = value;
  else if (option != m_options.end())
    m_options.erase(option);
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

  string strOptions = options;

  // remove leading ?, # or ; if present
  if (strOptions.at(0) == '?' || strOptions.at(0) == '#' || strOptions.at(0) == ';')
  {
    m_strLead = strOptions.at(0);
    strOptions.erase(0, 1);
  }
  else
    m_strLead = "?";

  // split the options by & and process them one by one
  vector<string> optionList = StringUtils::Split(strOptions, "&");
  for (vector<string>::const_iterator option = optionList.begin(); option != optionList.end(); option++)
  {
    if (option->empty())
      continue;

    // every option must have the format key=value
    size_t pos = option->find('=');
    if (pos == string::npos || pos == 0 || pos >= option->size() - 1)
      continue;

    string key = CURL::Decode(option->substr(0, pos));
    string value = CURL::Decode(option->substr(pos + 1));

    // the key cannot be empty
    if (!key.empty())
      AddOption(key, value);
  }
}

void CUrlOptions::AddOptions(const CUrlOptions &options)
{
  m_options.insert(options.m_options.begin(), options.m_options.end());
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

  UrlOptions::const_iterator option = m_options.find(key);
  if (option == m_options.end())
    return false;

  value = option->second;
  return true;
}
