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

#include "GUIListLabel.h"
#include "utils/CharsetConverter.h"
#include <limits>

CGUIListLabel::CGUIListLabel(int parentID, int controlID, float posX, float posY, float width, float height, const CLabelInfo& labelInfo, const CGUIInfoLabel &info, bool alwaysScroll)
    : CGUIControl(parentID, controlID, posX, posY, width, height)
    , m_label(posX, posY, width, height, labelInfo, alwaysScroll ? CGUILabel::OVER_FLOW_SCROLL : CGUILabel::OVER_FLOW_TRUNCATE)
{
  m_info = info;
  m_alwaysScroll = alwaysScroll;
  // TODO: Remove this "correction"
  if (labelInfo.align & XBFONT_RIGHT)
    m_label.SetMaxRect(m_posX - m_width, m_posY, m_width, m_height);
  else if (labelInfo.align & XBFONT_CENTER_X)
    m_label.SetMaxRect(m_posX - m_width*0.5f, m_posY, m_width, m_height);
  if (m_info.IsConstant())
    SetLabel(m_info.GetLabel(m_parentID, true));
  ControlType = GUICONTROL_LISTLABEL;
}

CGUIListLabel::~CGUIListLabel(void)
{
}

void CGUIListLabel::SetScrolling(bool scrolling)
{
  m_label.SetScrolling(scrolling || m_alwaysScroll);
}

void CGUIListLabel::SetSelected(bool selected)
{
  m_label.SetColor(selected ? CGUILabel::COLOR_SELECTED : CGUILabel::COLOR_TEXT);
}

void CGUIListLabel::SetFocus(bool focus)
{
  CGUIControl::SetFocus(focus);
  if (!focus)
    SetScrolling(false);
}

CRect CGUIListLabel::GetRenderRegion() const
{
  return m_label.GetRenderRect();
}

bool CGUIListLabel::UpdateColors()
{
  bool changed = CGUIControl::UpdateColors();
  changed |= m_label.UpdateColors();

  return changed;
}

void CGUIListLabel::Process(unsigned int currentTime)
{
  if (m_label.Process(currentTime))
    MarkDirtyRegion();

  CGUIControl::Process(currentTime);
}

void CGUIListLabel::Render()
{
  m_label.Render();
  CGUIControl::Render();
}

void CGUIListLabel::UpdateInfo(const CGUIListItem *item)
{
  if (m_info.IsConstant() && !m_bInvalidated)
    return; // nothing to do

  if (item)
    SetLabel(m_info.GetItemLabel(item));
  else
    SetLabel(m_info.GetLabel(m_parentID, true));
}

void CGUIListLabel::SetInvalid()
{
  m_label.SetInvalid();
  CGUIControl::SetInvalid();
}

void CGUIListLabel::SetLabel(const CStdString &label)
{
  m_label.SetText(label);
}
