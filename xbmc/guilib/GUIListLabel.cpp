/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIListLabel.h"

#include "addons/Skin.h"

#include <limits>

using namespace KODI::GUILIB;

CGUIListLabel::CGUIListLabel(int parentID, int controlID, float posX, float posY, float width, float height,
                             const CLabelInfo& labelInfo, const GUIINFO::CGUIInfoLabel &info, CGUIControl::GUISCROLLVALUE scroll)
    : CGUIControl(parentID, controlID, posX, posY, width, height)
    , m_label(posX, posY, width, height, labelInfo, (scroll == CGUIControl::ALWAYS) ? CGUILabel::OVER_FLOW_SCROLL : CGUILabel::OVER_FLOW_TRUNCATE)
    , m_info(info)
{
  m_scroll = scroll;
  if (m_info.IsConstant())
    SetLabel(m_info.GetLabel(m_parentID, true));
  m_label.SetScrollLoopCount(2);
  ControlType = GUICONTROL_LISTLABEL;
}

CGUIListLabel::~CGUIListLabel(void) = default;

void CGUIListLabel::SetSelected(bool selected)
{
  if(m_label.SetColor(selected ? CGUILabel::COLOR_SELECTED : CGUILabel::COLOR_TEXT))
    SetInvalid();
}

void CGUIListLabel::SetFocus(bool focus)
{
  CGUIControl::SetFocus(focus);
  if (m_scroll == CGUIControl::FOCUS)
  {
    m_label.SetScrolling(focus);
  }
}

CRect CGUIListLabel::CalcRenderRegion() const
{
  return m_label.GetRenderRect();
}

bool CGUIListLabel::UpdateColors(const CGUIListItem* item)
{
  bool changed = CGUIControl::UpdateColors(nullptr);
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
  if (CServiceBroker::GetWinSystem()->GetGfxContext().GetRenderOrder() ==
      RENDER_ORDER_FRONT_TO_BACK)
    return;
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
