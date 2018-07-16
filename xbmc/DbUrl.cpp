/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DbUrl.h"
#include "utils/URIUtils.h"

CDbUrl::CDbUrl()
{
  Reset();
}

CDbUrl::~CDbUrl() = default;

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
