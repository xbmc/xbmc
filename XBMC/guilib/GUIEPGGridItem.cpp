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

#include "include.h"
#include "GUIEPGGridItem.h"
#include "guiImage.h"
#include "GUIEPGGridItemLayout.h"

CGUIEPGGridItem::CGUIEPGGridItem(const CGUIEPGGridItem& item)
{
  *this = item;
  SetInvalid();
}

CGUIEPGGridItem::~CGUIEPGGridItem(void)
{
  FreeMemory();
}

void CGUIEPGGridItem::SetLabel(const CStdString& strLabel)
{
  m_strLabel = strLabel;
  SetInvalid();
}

const CStdString& CGUIEPGGridItem::GetLabel() const
{
  return m_strLabel;
}

void CGUIEPGGridItem::SetOverlayImage(GUIEPGGridItemOverlay icon, bool bOnOff)
{
  if (bOnOff)
    m_overlayIcon = GUIEPGGridItemOverlay((int)(icon)+1);
  else
    m_overlayIcon = icon;
  SetInvalid();
}

CStdString CGUIEPGGridItem::GetOverlayImage() const
{
  switch (m_overlayIcon)
  {
  case ICON_OVERLAY_RECORDING:
    return "OverlayRecording.png";
  case ICON_OVERLAY_AUTOWATCH:
    return "OverlayAutoWatch.png";
  default:
    return "";
  }
}

void CGUIEPGGridItem::Select(bool bOnOff)
{
  m_bSelected = bOnOff;
}

bool CGUIEPGGridItem::HasOverlay() const
{
  return (m_overlayIcon != CGUIEPGGridItem::ICON_OVERLAY_NONE);
}

bool CGUIEPGGridItem::IsSelected() const
{
  return m_bSelected;
}

void CGUIEPGGridItem::FreeMemory()
{
  if (m_layout)
  {
    delete m_layout;
    m_layout = NULL;
  }
  if (m_focusedLayout)
  {
    delete m_focusedLayout;
    m_focusedLayout = NULL;
  }
}
void CGUIEPGGridItem::SetLayout(CGUIEPGGridItemLayout *layout)
{
  if (m_layout)
    delete m_layout;
  m_layout = layout;
}

CGUIEPGGridItemLayout *CGUIEPGGridItem::GetLayout()
{
  return m_layout;
}

void CGUIEPGGridItem::SetFocusedLayout(CGUIEPGGridItemLayout *layout)
{
  if (m_focusedLayout)
    delete m_focusedLayout;
  m_focusedLayout = layout;
}

CGUIEPGGridItemLayout *CGUIEPGGridItem::GetFocusedLayout()
{
  return m_focusedLayout;
}

void CGUIEPGGridItem::SetInvalid()
{
  if (m_layout) m_layout->SetInvalid();
  if (m_focusedLayout) m_focusedLayout->SetInvalid();
}

void CGUIEPGGridItem::SetProperty(const CStdString &strKey, const char *strValue)
{
  m_mapProperties[strKey] = strValue;
}

void CGUIEPGGridItem::SetProperty(const CStdString &strKey, const CStdString &strValue)
{
  m_mapProperties[strKey] = strValue;
}

CStdString CGUIEPGGridItem::GetProperty(const CStdString &strKey) const
{
  std::map<CStdString,CStdString,icompare>::const_iterator iter = m_mapProperties.find(strKey);
  if (iter == m_mapProperties.end())
    return "";

  return iter->second;
}

bool CGUIEPGGridItem::HasProperty(const CStdString &strKey) const
{
  std::map<CStdString,CStdString,icompare>::const_iterator iter = m_mapProperties.find(strKey);
  if (iter == m_mapProperties.end())
    return false;

  return true;
}

void CGUIEPGGridItem::ClearProperty(const CStdString &strKey)
{
  std::map<CStdString,CStdString,icompare>::iterator iter = m_mapProperties.find(strKey);
  if (iter != m_mapProperties.end())
    m_mapProperties.erase(iter);
}

void CGUIEPGGridItem::ClearProperties()
{
  m_mapProperties.clear();
}

void CGUIEPGGridItem::SetProperty(const CStdString &strKey, int nVal)
{
  CStdString strVal;
  strVal.Format("%d",nVal);
  SetProperty(strKey, strVal);
}

void CGUIEPGGridItem::SetProperty(const CStdString &strKey, bool bVal)
{
  SetProperty(strKey, bVal?"1":"0");
}

void CGUIEPGGridItem::SetProperty(const CStdString &strKey, double dVal)
{
  CStdString strVal;
  strVal.Format("%f",dVal);
  SetProperty(strKey, strVal);
}

bool CGUIEPGGridItem::GetPropertyBOOL(const CStdString &strKey) const
{
  return GetProperty(strKey) == "1";
}

int CGUIEPGGridItem::GetPropertyInt(const CStdString &strKey) const
{
  return atoi(GetProperty(strKey).c_str()) ;
}

double CGUIEPGGridItem::GetPropertyDouble(const CStdString &strKey) const
{
  return atof(GetProperty(strKey).c_str()) ;
}