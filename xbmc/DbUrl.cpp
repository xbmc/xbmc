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

#include "DbUrl.h"
#include "utils/URIUtils.h"

CDbUrl::CDbUrl()
{
  Reset();
}

CDbUrl::~CDbUrl()
{ }

void CDbUrl::Reset()
{
  m_valid = false;
  m_type.clear();
  m_url.Reset();
  m_options.clear();
}

std::string CDbUrl::ToString() const
{
  if (!m_valid)
    return "";

  return m_url.Get();
}

bool CDbUrl::FromString(const std::string &dbUrl)
{
  Reset();

  m_url.Parse(dbUrl);
  m_valid = parse();

  if (!m_valid)
    Reset();

  return m_valid;
}

void CDbUrl::AppendPath(const std::string &subPath)
{
  if (!m_valid || subPath.empty())
    return;

  m_url.SetFileName(URIUtils::AddFileToFolder(m_url.GetFileName(), subPath));
}

void CDbUrl::AddOption(const std::string &key, const char *value)
{
  if (!validateOption(key, value))
    return;
  
  CUrlOptions::AddOption(key, value);
  updateOptions();
}

void CDbUrl::AddOption(const std::string &key, const std::string &value)
{
  if (!validateOption(key, value))
    return;
  
  CUrlOptions::AddOption(key, value);
  updateOptions();
}

void CDbUrl::AddOption(const std::string &key, int value)
{
  if (!validateOption(key, value))
    return;
  
  CUrlOptions::AddOption(key, value);
  updateOptions();
}

void CDbUrl::AddOption(const std::string &key, float value)
{
  if (!validateOption(key, value))
    return;
  
  CUrlOptions::AddOption(key, value);
  updateOptions();
}

void CDbUrl::AddOption(const std::string &key, double value)
{
  if (!validateOption(key, value))
    return;
  
  CUrlOptions::AddOption(key, value);
  updateOptions();
}

void CDbUrl::AddOption(const std::string &key, bool value)
{
  if (!validateOption(key, value))
    return;
  
  CUrlOptions::AddOption(key, value);
  updateOptions();
}

void CDbUrl::AddOptions(const std::string &options)
{
  CUrlOptions::AddOptions(options);
  updateOptions();  
}

void CDbUrl::RemoveOption(const std::string &key)
{
  CUrlOptions::RemoveOption(key);
  updateOptions(); 
}

bool CDbUrl::validateOption(const std::string &key, const CVariant &value)
{
  if (key.empty())
    return false;

  return true;
}

void CDbUrl::updateOptions()
{
  // Update the options string in the CURL object
  std::string options = GetOptionsString();
  if (!options.empty())
    options = "?" + options;

  m_url.SetOptions(options);
}
