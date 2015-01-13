/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "GUIListLabel.h"
#include <limits>
#include "addons/Skin.h"

CGUIListLabel::CGUIListLabel(int parentID, int controlID, float posX, float posY, float width, float height, const CLabelInfo& labelInfo, const CGUIInfoLabel &info, CGUIControl::GUISCROLLVALUE scroll)
    : CGUIControl(parentID, controlID, posX, posY, width, height)
    , m_label(posX, posY, width, height, labelInfo, (scroll == CGUIControl::ALWAYS) ? CGUILabel::OVER_FLOW_SCROLL : CGUILabel::OVER_FLOW_TRUNCATE)
    , m_info(info)
{
  m_scroll = scroll;
  if (g_SkinInfo && g_SkinInfo->APIVersion() < ADDON::AddonVersion("5.1.0"))
  {
    if (labelInfo.align & XBFONT_RIGHT)
      m_label.SetMaxRect(m_posX - m_width, m_posY, m_width, m_height);
    else if (labelInfo.align & XBFONT_CENTER_X)
      m_label.SetMaxRect(m_posX - m_width*0.5f, m_posY, m_width, m_height);
  }
  if (m_info.IsConstant())
    SetLabel(m_info.GetLabel(m_parentID, true));
  ControlType = GUICONTROL_LISTLABEL;
}

CGUIListLabel::~CGUIListLabel(void)
{
}

void CGUIListLabel::SetScrolling(bool scrolling)
{
  if (m_scroll == CGUIControl::FOCUS)
    m_label.SetScrolling(scrolling);
  else
    m_label.SetScrolling((m_scroll == CGUIControl::ALWAYS) ? true : false);
}

void CGUIListLabel::SetSelected(bool selected)
{
  if(m_label.SetColor(selected ? CGUILabel::COLOR_SELECTED : CGUILabel::COLOR_TEXT))
    SetInvalid();
}

void CGUIListLabel::SetFocus(bool focus)
{
  CGUIControl::SetFocus(focus);
  if (!focus)
    SetScrolling(false);
}

CRect CGUIListLabel::CalcRenderRegion() const
{
  return m_label.GetRenderRect();
}

bool CGUIListLabel::UpdateColors()
{
  bool changed = CGUIControl::UpdateColors();
  changed |= m_label.UpdateColors();

  return changed;
}

void CGUIListLabel::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (m_label.Process(currentTime))
    MarkDirtyRegion();

  CGUIControl::Process(currentTime, dirtyregions);
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

void CGUIListLabel::SetWidth(float width)
{
  m_width = width;
  if (m_label.GetLabelInfo().align & XBFONT_RIGHT)
    m_label.SetMaxRect(m_posX - m_width, m_posY, m_width, m_height);
  else if (m_label.GetLabelInfo().align & XBFONT_CENTER_X)
    m_label.SetMaxRect(m_posX - m_width*0.5f, m_posY, m_width, m_height);
  else
    m_label.SetMaxRect(m_posX, m_posY, m_width, m_height);
  CGUIControl::SetWidth(m_width);
}

void CGUIListLabel::SetLabel(const std::string &label)
{
  m_label.SetText(label);
}
