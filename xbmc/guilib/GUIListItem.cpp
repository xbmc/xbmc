/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIListItem.h"
#include "GUIListItemLayout.h"
#include "utils/Archive.h"
#include "utils/CharsetConverter.h"
#include "utils/Variant.h"

using namespace std;

CGUIListItem::CGUIListItem(const CGUIListItem& item)
{
  m_layout = NULL;
  m_focusedLayout = NULL;
  *this = item;
  SetInvalid();
}

CGUIListItem::CGUIListItem(void)
{
  m_bIsFolder = false;
  m_strLabel2 = "";
  m_strLabel = "";
  m_bSelected = false;
  m_strIcon = "";
  m_overlayIcon = ICON_OVERLAY_NONE;
  m_layout = NULL;
  m_focusedLayout = NULL;
}

CGUIListItem::CGUIListItem(const CStdString& strLabel)
{
  m_bIsFolder = false;
  m_strLabel2 = "";
  m_strLabel = strLabel;
  SetSortLabel(strLabel);
  m_bSelected = false;
  m_strIcon = "";
  m_overlayIcon = ICON_OVERLAY_NONE;
  m_layout = NULL;
  m_focusedLayout = NULL;
}

CGUIListItem::~CGUIListItem(void)
{
  FreeMemory();
}

void CGUIListItem::SetLabel(const CStdString& strLabel)
{
  if (m_strLabel == strLabel)
    return;
  m_strLabel = strLabel;
  if (m_sortLabel.IsEmpty())
    SetSortLabel(strLabel);
  SetInvalid();
}

const CStdString& CGUIListItem::GetLabel() const
{
  return m_strLabel;
}


void CGUIListItem::SetLabel2(const CStdString& strLabel2)
{
  if (m_strLabel2 == strLabel2)
    return;
  m_strLabel2 = strLabel2;
  SetInvalid();
}

const CStdString& CGUIListItem::GetLabel2() const
{
  return m_strLabel2;
}

void CGUIListItem::SetSortLabel(const CStdString &label)
{
  g_charsetConverter.utf8ToW(label, m_sortLabel, false);
  // no need to invalidate - this is never shown in the UI
}

void CGUIListItem::SetSortLabel(const CStdStringW &label)
{
  m_sortLabel = label;
}

const CStdStringW& CGUIListItem::GetSortLabel() const
{
  return m_sortLabel;
}

void CGUIListItem::SetArt(const std::string &type, const std::string &url)
{
  ArtMap::iterator i = m_art.find(type);
  if (i == m_art.end() || i->second != url)
  {
    m_art[type] = url;
    SetInvalid();
  }
}

void CGUIListItem::SetArt(const ArtMap &art, bool setFallback /* = true */)
{
  m_art = art;
  // ensure that the fallback "thumb" is available
  if (setFallback && m_art.find("thumb") == m_art.end())
  {
    if (HasArt("poster"))
      m_art["thumb"] = m_art["poster"];
    else if (HasArt("banner"))
      m_art["thumb"] = m_art["banner"];
  }
  
  SetInvalid();
}

void CGUIListItem::AppendArt(const ArtMap &art)
{
  for (ArtMap::const_iterator i = art.begin(); i != art.end(); ++i)
    SetArt(i->first, i->second);
}

std::string CGUIListItem::GetArt(const std::string &type) const
{
  ArtMap::const_iterator i = m_art.find(type);
  if (i != m_art.end())
    return i->second;
  return "";
}

const CGUIListItem::ArtMap &CGUIListItem::GetArt() const
{
  return m_art;
}

bool CGUIListItem::HasArt(const std::string &type) const
{
  ArtMap::const_iterator i = m_art.find(type);
  return (i != m_art.end() && !i->second.empty());
}

void CGUIListItem::SetIconImage(const CStdString& strIcon)
{
  if (m_strIcon == strIcon)
    return;
  m_strIcon = strIcon;
  SetInvalid();
}

const CStdString& CGUIListItem::GetIconImage() const
{
  return m_strIcon;
}

void CGUIListItem::SetOverlayImage(GUIIconOverlay icon, bool bOnOff)
{
  GUIIconOverlay newIcon = (bOnOff) ? GUIIconOverlay((int)(icon)+1) : icon;
  if (m_overlayIcon == newIcon)
    return;
  m_overlayIcon = newIcon;
  SetInvalid();
}

CStdString CGUIListItem::GetOverlayImage() const
{
  switch (m_overlayIcon)
  {
  case ICON_OVERLAY_RAR:
    return "OverlayRAR.png";
  case ICON_OVERLAY_ZIP:
    return "OverlayZIP.png";
  case ICON_OVERLAY_TRAINED:
    return "OverlayTrained.png";
  case ICON_OVERLAY_HAS_TRAINER:
    return "OverlayHasTrainer.png";
  case ICON_OVERLAY_LOCKED:
    return "OverlayLocked.png";
  case ICON_OVERLAY_UNWATCHED:
    return "OverlayUnwatched.png";
  case ICON_OVERLAY_WATCHED:
    return "OverlayWatched.png";
  case ICON_OVERLAY_HD:
    return "OverlayHD.png";
  default:
    return "";
  }
}

void CGUIListItem::Select(bool bOnOff)
{
  m_bSelected = bOnOff;
}

bool CGUIListItem::HasIcon() const
{
  return (m_strIcon.size() != 0);
}

bool CGUIListItem::HasOverlay() const
{
  return (m_overlayIcon != CGUIListItem::ICON_OVERLAY_NONE);
}

bool CGUIListItem::IsSelected() const
{
  return m_bSelected;
}

const CGUIListItem& CGUIListItem::operator =(const CGUIListItem& item)
{
  if (&item == this) return * this;
  m_strLabel2 = item.m_strLabel2;
  m_strLabel = item.m_strLabel;
  m_sortLabel = item.m_sortLabel;
  FreeMemory();
  m_bSelected = item.m_bSelected;
  m_strIcon = item.m_strIcon;
  m_overlayIcon = item.m_overlayIcon;
  m_bIsFolder = item.m_bIsFolder;
  m_mapProperties = item.m_mapProperties;
  m_art = item.m_art;
  SetInvalid();
  return *this;
}

void CGUIListItem::Archive(CArchive &ar)
{
  if (ar.IsStoring())
  {
    ar << m_bIsFolder;
    ar << m_strLabel;
    ar << m_strLabel2;
    ar << m_sortLabel;
    ar << m_strIcon;
    ar << m_bSelected;
    ar << m_overlayIcon;
    ar << (int)m_mapProperties.size();
    for (PropertyMap::const_iterator it = m_mapProperties.begin(); it != m_mapProperties.end(); it++)
    {
      ar << it->first;
      ar << it->second;
    }
    ar << (int)m_art.size();
    for (ArtMap::const_iterator i = m_art.begin(); i != m_art.end(); i++)
    {
      ar << i->first;
      ar << i->second;
    }
  }
  else
  {
    ar >> m_bIsFolder;
    ar >> m_strLabel;
    ar >> m_strLabel2;
    ar >> m_sortLabel;
    ar >> m_strIcon;
    ar >> m_bSelected;

    int overlayIcon;
    ar >> overlayIcon;
    m_overlayIcon = GUIIconOverlay(overlayIcon);

    int mapSize;
    ar >> mapSize;
    for (int i = 0; i < mapSize; i++)
    {
      std::string key;
      CVariant value;
      ar >> key;
      ar >> value;
      SetProperty(key, value);
    }
    ar >> mapSize;
    for (int i = 0; i < mapSize; i++)
    {
      std::string key, value;
      ar >> key;
      ar >> value;
      m_art.insert(make_pair(key, value));
    }
  }
}
void CGUIListItem::Serialize(CVariant &value)
{
  value["isFolder"] = m_bIsFolder;
  value["strLabel"] = m_strLabel;
  value["strLabel2"] = m_strLabel2;
  value["sortLabel"] = m_sortLabel;
  value["strIcon"] = m_strIcon;
  value["selected"] = m_bSelected;

  for (PropertyMap::const_iterator it = m_mapProperties.begin(); it != m_mapProperties.end(); it++)
  {
    value["properties"][it->first] = it->second;
  }
  for (ArtMap::const_iterator it = m_art.begin(); it != m_art.end(); it++)
    value["art"][it->first] = it->second;
}

void CGUIListItem::FreeIcons()
{
  FreeMemory();
  m_art.clear();
  m_strIcon = "";
  SetInvalid();
}

void CGUIListItem::FreeMemory(bool immediately)
{
  if (m_layout)
  {
    m_layout->FreeResources(immediately);
    delete m_layout;
    m_layout = NULL;
  }
  if (m_focusedLayout)
  {
    m_focusedLayout->FreeResources(immediately);
    delete m_focusedLayout;
    m_focusedLayout = NULL;
  }
}

void CGUIListItem::SetLayout(CGUIListItemLayout *layout)
{
  delete m_layout;
  m_layout = layout;
}

CGUIListItemLayout *CGUIListItem::GetLayout()
{
  return m_layout;
}

void CGUIListItem::SetFocusedLayout(CGUIListItemLayout *layout)
{
  delete m_focusedLayout;
  m_focusedLayout = layout;
}

CGUIListItemLayout *CGUIListItem::GetFocusedLayout()
{
  return m_focusedLayout;
}

void CGUIListItem::SetInvalid()
{
  if (m_layout) m_layout->SetInvalid();
  if (m_focusedLayout) m_focusedLayout->SetInvalid();
}

void CGUIListItem::SetProperty(const CStdString &strKey, const CVariant &value)
{
  m_mapProperties[strKey] = value;
}

CVariant CGUIListItem::GetProperty(const CStdString &strKey) const
{
  PropertyMap::const_iterator iter = m_mapProperties.find(strKey);
  if (iter == m_mapProperties.end())
    return CVariant(CVariant::VariantTypeNull);

  return iter->second;
}

bool CGUIListItem::HasProperty(const CStdString &strKey) const
{
  PropertyMap::const_iterator iter = m_mapProperties.find(strKey);
  if (iter == m_mapProperties.end())
    return false;

  return true;
}

void CGUIListItem::ClearProperty(const CStdString &strKey)
{
  PropertyMap::iterator iter = m_mapProperties.find(strKey);
  if (iter != m_mapProperties.end())
    m_mapProperties.erase(iter);
}

void CGUIListItem::ClearProperties()
{
  m_mapProperties.clear();
}

void CGUIListItem::IncrementProperty(const CStdString &strKey, int nVal)
{
  int64_t i = GetProperty(strKey).asInteger();
  i += nVal;
  SetProperty(strKey, i);
}

void CGUIListItem::IncrementProperty(const CStdString &strKey, double dVal)
{
  double d = GetProperty(strKey).asDouble();
  d += dVal;
  SetProperty(strKey, d);
}

void CGUIListItem::AppendProperties(const CGUIListItem &item)
{
  for (PropertyMap::const_iterator i = item.m_mapProperties.begin(); i != item.m_mapProperties.end(); ++i)
    SetProperty(i->first, i->second);
}
