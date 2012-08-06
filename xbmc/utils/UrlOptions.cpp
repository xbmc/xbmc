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
{ }

CUrlOptions::~CUrlOptions()
{ }

std::string CUrlOptions::GetOptionsString() const
{
  std::string options;
  for (UrlOptions::const_iterator opt = m_options.begin(); opt != m_options.end(); opt++)
  {
    if (opt != m_options.begin())
      options += "&";

    options += CURL::Encode(opt->first) + "=" + CURL::Encode(opt->second.asString());
  }

  return options;
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

  // remove leading ? if present
  if (strOptions.at(0) == '?')
    strOptions.erase(0, 1);

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
