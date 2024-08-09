/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SetInfoTag.h"

#include "utils/StringUtils.h"
#include "utils/XMLUtils.h"

void CSetInfoTag::Reset()
{
  m_title.clear();
  m_id = -1;
  m_overview.clear();
  m_updateSetOverview = false;
}

bool CSetInfoTag::Load(const TiXmlElement* element, bool append, bool prioritise)
{
  if (!element)
    return false;
  if (!append)
    Reset();
  ParseNative(element, prioritise);
  return true;
}

void CSetInfoTag::ParseNative(const TiXmlElement* set, bool prioritise)
{
  std::string value;

  if (XMLUtils::GetString(set, "title", value))
    SetTitle(value);
  else if (XMLUtils::GetString(set, "name", value))
    // If no <title> then look for <name> - for compatibility with <set> in movie.nfo which uses <name>
    SetTitle(value);

  if (XMLUtils::GetString(set, "overview", value))
    SetOverview(value);
}

void CSetInfoTag::SetOverview(const std::string& overview)
{
  m_overview = overview;
  m_overview = StringUtils::Trim(m_overview);
  m_updateSetOverview = true;
}

const std::string& CSetInfoTag::GetOverview() const
{
  return m_overview;
}

void CSetInfoTag::SetTitle(const std::string& title)
{
  m_title = title;
  m_title = StringUtils::Trim(m_title);
}

const std::string& CSetInfoTag::GetTitle() const
{
  return m_title;
}

void CSetInfoTag::Merge(const CSetInfoTag& other)
{
  if (!other.GetTitle().empty())
    m_title = other.GetTitle();
  if (!other.GetOverview().empty())
    m_overview = other.GetOverview();
}

void CSetInfoTag::Copy(const CSetInfoTag& other)
{
  m_title = other.GetTitle();
  m_overview = other.GetOverview();
}

bool CSetInfoTag::IsEmpty() const
{
  return (m_title.empty() && m_id == -1 && m_overview.empty());
}