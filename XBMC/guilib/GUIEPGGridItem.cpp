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

CGUIEPGGridItem::CGUIEPGGridItem(void)
{

}

CGUIEPGGridItem::~CGUIEPGGridItem(void)
{
  
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

void CGUIEPGGridItem::SetLabel(const CStdString& strLabel)
{
  m_strLabel = strLabel;
  SetInvalid();
}

const CStdString& CGUIEPGGridItem::GetLabel() const
{
  return m_strLabel;
}

void CGUIEPGGridItem::SetShortDesc(const CStdString& strShortDesc)
{
  m_pShortDesc = strShortDesc;
  SetInvalid();
}

const CStdString& CGUIEPGGridItem::GetShortDesc() const
{
  return m_pShortDesc;
}

void CGUIEPGGridItem::SetStartTime(const CStdString& startTime)
{
  m_pStartTime.SetFromDBDateTime(startTime);
  SetInvalid();
}

const CDateTime& CGUIEPGGridItem::GetStartTime() const
{
  return m_pStartTime;
}

void CGUIEPGGridItem::SetDuration(const int& duration)
{
  m_pDuration = duration;
  SetInvalid();
}

const int& CGUIEPGGridItem::GetDuration() const
{
  return m_pDuration;
}

void CGUIEPGGridItem::SetLayout(CGUIListItemLayout *layout)
{
  if (m_layout)
    delete m_layout;
  m_layout = layout;
}

CGUIListItemLayout *CGUIEPGGridItem::GetLayout()
{
  return m_layout;
}

void CGUIEPGGridItem::SetFocusedLayout(CGUIListItemLayout *layout)
{
  if (m_focusedLayout)
    delete m_focusedLayout;
  m_focusedLayout = layout;
}

CGUIListItemLayout *CGUIEPGGridItem::GetFocusedLayout()
{
  return m_focusedLayout;
}