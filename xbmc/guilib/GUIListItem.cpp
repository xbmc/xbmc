/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIListItem.h"
#include "GUIListItemLayout.h"
#include "utils/Archive.h"
#include "utils/CharsetConverter.h"
#include "utils/Variant.h"

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
  m_strThumbnailImage = "";
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
  m_strThumbnailImage = "";
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
  g_charsetConverter.utf8ToW(label, m_sortLabel);
  // no need to invalidate - this is never shown in the UI
}

const CStdStringW& CGUIListItem::GetSortLabel() const
{
  return m_sortLabel;
}

void CGUIListItem::SetThumbnailImage(const CStdString& strThumbnail)
{
  if (m_strThumbnailImage == strThumbnail)
    return;
  m_strThumbnailImage = strThumbnail;
  SetInvalid();
}

const CStdString& CGUIListItem::GetThumbnailImage() const
{
  return m_strThumbnailImage;
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


bool CGUIListItem::HasThumbnail() const
{
  return (m_strThumbnailImage.size() != 0);
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
  m_strThumbnailImage = item.m_strThumbnailImage;
  m_overlayIcon = item.m_overlayIcon;
  m_bIsFolder = item.m_bIsFolder;
  m_mapProperties = item.m_mapProperties;
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
    ar << m_strThumbnailImage;
    ar << m_strIcon;
    ar << m_bSelected;
    ar << m_overlayIcon;
    ar << (int)m_mapProperties.size();
    for (std::map<CStdString, CStdString, icompare>::const_iterator it = m_mapProperties.begin(); it != m_mapProperties.end(); it++)
    {
      ar << it->first;
      ar << it->second;
    }
  }
  else
  {
    ar >> m_bIsFolder;
    ar >> m_strLabel;
    ar >> m_strLabel2;
    ar >> m_sortLabel;
    ar >> m_strThumbnailImage;
    ar >> m_strIcon;
    ar >> m_bSelected;

    int overlayIcon;
    ar >> overlayIcon;
    m_overlayIcon = GUIIconOverlay(overlayIcon);

    int mapSize;
    ar >> mapSize;
    for (int i = 0; i < mapSize; i++)
    {
      CStdString key, value;
      ar >> key;
      ar >> value;
      SetProperty(key, value);
    }
  }
}
void CGUIListItem::Serialize(CVariant &value)
{
  value["isFolder"] = m_bIsFolder;
  value["strLabel"] = m_strLabel;
  value["strLabel2"] = m_strLabel2;
  value["sortLabel"] = CStdString(m_sortLabel);
  value["strThumbnailImage"] = m_strThumbnailImage;
  value["strIcon"] = m_strIcon;
  value["selected"] = m_bSelected;

  for (std::map<CStdString, CStdString, icompare>::const_iterator it = m_mapProperties.begin(); it != m_mapProperties.end(); it++)
  {
    value["properties"][it->first] = it->second;
  }
}

void CGUIListItem::FreeIcons()
{
  FreeMemory();
  m_strThumbnailImage = "";
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

void CGUIListItem::SetProperty(const CStdString &strKey, const char *strValue)
{
  m_mapProperties[strKey] = strValue;
}

void CGUIListItem::SetProperty(const CStdString &strKey, const CStdString &strValue)
{
  m_mapProperties[strKey] = strValue;
}

CStdString CGUIListItem::GetProperty(const CStdString &strKey) const
{
  PropertyMap::const_iterator iter = m_mapProperties.find(strKey);
  if (iter == m_mapProperties.end())
    return "";

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

void CGUIListItem::SetProperty(const CStdString &strKey, int nVal)
{
  CStdString strVal;
  strVal.Format("%d",nVal);
  SetProperty(strKey, strVal);
}

void CGUIListItem::IncrementProperty(const CStdString &strKey, int nVal)
{
  int i = GetPropertyInt(strKey);
  i += nVal;
  SetProperty(strKey, i);
}

void CGUIListItem::SetProperty(const CStdString &strKey, bool bVal)
{
  SetProperty(strKey, bVal?"1":"0");
}

void CGUIListItem::SetProperty(const CStdString &strKey, double dVal)
{
  CStdString strVal;
  strVal.Format("%f",dVal);
  SetProperty(strKey, strVal);
}

void CGUIListItem::IncrementProperty(const CStdString &strKey, double dVal)
{
  double d = GetPropertyDouble(strKey);
  d += dVal;
  SetProperty(strKey, d);
}

bool CGUIListItem::GetPropertyBOOL(const CStdString &strKey) const
{
  return GetProperty(strKey) == "1";
}

int CGUIListItem::GetPropertyInt(const CStdString &strKey) const
{
  return atoi(GetProperty(strKey).c_str()) ;
}

double CGUIListItem::GetPropertyDouble(const CStdString &strKey) const
{
  return atof(GetProperty(strKey).c_str()) ;
}

void CGUIListItem::AppendProperties(const CGUIListItem &item)
{
  for (PropertyMap::const_iterator i = item.m_mapProperties.begin(); i != item.m_mapProperties.end(); ++i)
    SetProperty(i->first, i->second);
}
