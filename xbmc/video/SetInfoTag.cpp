/*
 *  Copyright (C) 2024-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SetInfoTag.h"

#include "VideoThumbLoader.h"
#include "utils/Archive.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/XMLUtils.h"

void CSetInfoTag::Reset()
{
  m_title.clear();
  m_id = -1;
  m_overview.clear();
  m_updateSetOverview = false;
  m_art.clear();
}

bool CSetInfoTag::Load(const TiXmlElement* element, bool append, bool /*prioritise*/)
{
  if (!element)
    return false;
  if (!append)
    Reset();
  ParseNative(element);
  return true;
}

void CSetInfoTag::ParseNative(const TiXmlElement* set)
{
  std::string value;

  if (XMLUtils::GetString(set, "title", value))
    SetTitle(value);
  if (XMLUtils::GetString(set, "originaltitle", value))
    SetOriginalTitle(value);
  if (XMLUtils::GetString(set, "overview", value))
    SetOverview(value);

  const TiXmlElement* art{set->FirstChildElement("art")};
  if (art)
    for (const TiXmlElement* picture = art->FirstChildElement(); picture;
         picture = picture->NextSiblingElement())
    {
      std::string type{picture->ValueStr()};
      std::string url{picture->GetText()};
      if (!type.empty() && !url.empty())
        m_art.try_emplace(type, url);
    }
}

void CSetInfoTag::SetOverview(std::string_view overview)
{
  m_overview = overview;
  m_overview = StringUtils::Trim(m_overview);
  m_updateSetOverview = true;
}

void CSetInfoTag::SetTitle(std::string_view title)
{
  m_title = title;
  m_title = StringUtils::Trim(m_title);
}

void CSetInfoTag::SetOriginalTitle(std::string_view title)
{
  m_originalTitle = title;
}

void CSetInfoTag::SetArt(const KODI::ART::Artwork& art)
{
  m_art = art;
}

void CSetInfoTag::Merge(const CSetInfoTag& other)
{
  if (other.GetID())
    m_id = other.GetID();
  if (other.HasTitle())
    m_title = other.GetTitle();
  if (other.HasOriginalTitle())
    m_originalTitle = other.GetOriginalTitle();
  if (other.HasOverview())
    m_overview = other.GetOverview();
  if (!other.m_art.empty())
    m_art = other.m_art;
}

void CSetInfoTag::Copy(const CSetInfoTag& other)
{
  m_id = other.GetID();
  m_title = other.GetTitle();
  m_originalTitle = other.GetOriginalTitle();
  m_overview = other.GetOverview();
  m_art = other.m_art;
}

bool CSetInfoTag::Save(TiXmlNode* node,
                       const std::string& tag,
                       const TiXmlElement* additionalNode /* =nullptr */) const
{
  if (!node)
    return false;

  // we start with a <tag> tag
  TiXmlElement setElement(tag.c_str());
  TiXmlNode* set = node->InsertEndChild(setElement);
  if (!set)
    return false;

  XMLUtils::SetString(set, "title", m_title);
  if (!m_originalTitle.empty())
    XMLUtils::SetString(set, "originaltitle", m_originalTitle);
  if (!m_overview.empty())
    XMLUtils::SetString(set, "overview", m_overview);

  if (HasArt())
  {
    TiXmlElement art("art");
    for (const auto& [type, url] : m_art)
    {
      XMLUtils::SetString(&art, type.c_str(), url);
    }
    set->InsertEndChild(art);
  }

  if (additionalNode)
    set->InsertEndChild(*additionalNode);

  return true;
}

void CSetInfoTag::Archive(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar << m_title;
    ar << m_id;
    ar << m_overview;
    ar << m_originalTitle;
  }
  else
  {
    ar >> m_title;
    ar >> m_id;
    ar >> m_overview;
    ar >> m_originalTitle;
  }
}

void CSetInfoTag::Serialize(CVariant& value) const
{
  value["set"] = m_title;
  value["setid"] = m_id;
  value["setoverview"] = m_overview;
  value["originalset"] = m_originalTitle;
}

bool CSetInfoTag::IsEmpty() const
{
  return (m_title.empty() && m_id == -1 && m_overview.empty());
}
