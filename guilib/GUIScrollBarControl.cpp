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

#include "GUIScrollBarControl.h"
#include "Key.h"

#define MIN_NIB_SIZE 4.0f

CGUIScrollBar::CGUIScrollBar(int parentID, int controlID, float posX, float posY, float width, float height, const CTextureInfo& backGroundTexture, const CTextureInfo& barTexture, const CTextureInfo& barTextureFocus, const CTextureInfo& nibTexture, const CTextureInfo& nibTextureFocus, ORIENTATION orientation, bool showOnePage)
    : CGUIControl(parentID, controlID, posX, posY, width, height)
    , m_guiBackground(posX, posY, width, height, backGroundTexture)
    , m_guiBarNoFocus(posX, posY, width, height, barTexture)
    , m_guiBarFocus(posX, posY, width, height, barTextureFocus)
    , m_guiNibNoFocus(posX, posY, width, height, nibTexture)
    , m_guiNibFocus(posX, posY, width, height, nibTextureFocus)
{
  m_guiNibNoFocus.SetAspectRatio(CAspectRatio::AR_CENTER);
  m_guiNibFocus.SetAspectRatio(CAspectRatio::AR_CENTER);
  m_numItems = 100;
  m_offset = 0;
  m_pageSize = 10;
  ControlType = GUICONTROL_SCROLLBAR;
  m_orientation = orientation;
  m_showOnePage = showOnePage;
}

CGUIScrollBar::~CGUIScrollBar(void)
{
}


void CGUIScrollBar::Render()
{
  if (m_bInvalidated)
    UpdateBarSize();

  m_guiBackground.Render();
  if (m_bHasFocus)
  {
    m_guiBarFocus.Render();
    m_guiNibFocus.Render();
  }
  else
  {
    m_guiBarNoFocus.Render();
    m_guiNibNoFocus.Render();
  }

  CGUIControl::Render();
}

bool CGUIScrollBar::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_ITEM_SELECT:
    SetValue(message.GetParam1());
    return true;
  case GUI_MSG_LABEL_RESET:
    SetRange(message.GetParam1(), message.GetParam2());
    return true;
  case GUI_MSG_PAGE_UP:
    Move(-1);
    return true;
  case GUI_MSG_PAGE_DOWN:
    Move(1);
    return true;
  }
  return CGUIControl::OnMessage(message);
}

bool CGUIScrollBar::OnAction(const CAction &action)
{
  switch ( action.GetID() )
  {
  case ACTION_MOVE_LEFT:
    if (m_orientation == HORIZONTAL)
    {
      Move( -1);
      return true;
    }
    break;

  case ACTION_MOVE_RIGHT:
    if (m_orientation == HORIZONTAL)
    {
      Move(1);
      return true;
    }
    break;
  case ACTION_MOVE_UP:
    if (m_orientation == VERTICAL)
    {
      Move(-1);
      return true;
    }
    break;

  case ACTION_MOVE_DOWN:
    if (m_orientation == VERTICAL)
    {
      Move(1);
      return true;
    }
    break;
  }
  return CGUIControl::OnAction(action);
}

void CGUIScrollBar::Move(int numSteps)
{
  m_offset += numSteps * m_pageSize;
  if (m_offset > m_numItems - m_pageSize) m_offset = m_numItems - m_pageSize;
  if (m_offset < 0) m_offset = 0;
  CGUIMessage message(GUI_MSG_NOTIFY_ALL, GetParentID(), GetID(), GUI_MSG_PAGE_CHANGE, m_offset);
  SendWindowMessage(message);
  SetInvalid();
}

void CGUIScrollBar::SetRange(int pageSize, int numItems)
{
  m_pageSize = pageSize;
  m_numItems = numItems;
  m_offset = 0;
  SetInvalid();
}

void CGUIScrollBar::SetValue(int value)
{
  m_offset = value;
  SetInvalid();
}

void CGUIScrollBar::FreeResources(bool immediately)
{
  CGUIControl::FreeResources(immediately);
  m_guiBackground.FreeResources(immediately);
  m_guiBarNoFocus.FreeResources(immediately);
  m_guiBarFocus.FreeResources(immediately);
  m_guiNibNoFocus.FreeResources(immediately);
  m_guiNibFocus.FreeResources(immediately);
}

void CGUIScrollBar::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_guiBackground.DynamicResourceAlloc(bOnOff);
  m_guiBarNoFocus.DynamicResourceAlloc(bOnOff);
  m_guiBarFocus.DynamicResourceAlloc(bOnOff);
  m_guiNibNoFocus.DynamicResourceAlloc(bOnOff);
  m_guiNibFocus.DynamicResourceAlloc(bOnOff);
}

void CGUIScrollBar::AllocResources()
{
  CGUIControl::AllocResources();
  m_guiBackground.AllocResources();
  m_guiBarNoFocus.AllocResources();
  m_guiBarFocus.AllocResources();
  m_guiNibNoFocus.AllocResources();
  m_guiNibFocus.AllocResources();
}

void CGUIScrollBar::SetInvalid()
{
  CGUIControl::SetInvalid();
  m_guiBackground.SetInvalid();
  m_guiBarFocus.SetInvalid();
  m_guiBarFocus.SetInvalid();
  m_guiNibNoFocus.SetInvalid();
  m_guiNibFocus.SetInvalid();
}

void CGUIScrollBar::UpdateBarSize()
{
  // scale our textures to suit
  if (m_orientation == VERTICAL)
  {
    // calculate the height to display the nib at
    float percent = (m_numItems == 0) ? 0 : (float)m_pageSize / m_numItems;
    float nibSize = GetHeight() * percent;
    if (nibSize < m_guiNibFocus.GetTextureHeight() + 2 * MIN_NIB_SIZE) nibSize = m_guiNibFocus.GetTextureHeight() + 2 * MIN_NIB_SIZE;
    if (nibSize > GetHeight()) nibSize = GetHeight();

    m_guiBarNoFocus.SetHeight(nibSize);
    m_guiBarFocus.SetHeight(nibSize);
    m_guiNibNoFocus.SetHeight(nibSize);
    m_guiNibFocus.SetHeight(nibSize);
    // nibSize may be altered by the border size of the nib (and bar).
    nibSize = std::max(m_guiBarFocus.GetHeight(), m_guiNibFocus.GetHeight());

    // and the position
    percent = (m_numItems == m_pageSize) ? 0 : (float)m_offset / (m_numItems - m_pageSize);
    float nibPos = (GetHeight() - nibSize) * percent;
    if (nibPos < 0) nibPos = 0;
    if (nibPos > GetHeight() - nibSize) nibPos = GetHeight() - nibSize;
    m_guiBarNoFocus.SetPosition(GetXPosition(), GetYPosition() + nibPos);
    m_guiBarFocus.SetPosition(GetXPosition(), GetYPosition() + nibPos);
    m_guiNibNoFocus.SetPosition(GetXPosition(), GetYPosition() + nibPos);
    m_guiNibFocus.SetPosition(GetXPosition(), GetYPosition() + nibPos);
  }
  else
  {
    // calculate the height to display the nib at
    float percent = (m_numItems == 0) ? 0 : (float)m_pageSize / m_numItems;
    float nibSize = GetWidth() * percent + 0.5f;
    if (nibSize < m_guiNibFocus.GetTextureWidth() + 2 * MIN_NIB_SIZE) nibSize = m_guiNibFocus.GetTextureWidth() + 2 * MIN_NIB_SIZE;
    if (nibSize > GetWidth()) nibSize = GetWidth();

    m_guiBarNoFocus.SetWidth(nibSize);
    m_guiBarFocus.SetWidth(nibSize);
    m_guiNibNoFocus.SetWidth(nibSize);
    m_guiNibFocus.SetWidth(nibSize);

    // and the position
    percent = (m_numItems == m_pageSize) ? 0 : (float)m_offset / (m_numItems - m_pageSize);
    float nibPos = (GetWidth() - nibSize) * percent;
    if (nibPos < 0) nibPos = 0;
    if (nibPos > GetWidth() - nibSize) nibPos = GetWidth() - nibSize;
    m_guiBarNoFocus.SetPosition(GetXPosition() + nibPos, GetYPosition());
    m_guiBarFocus.SetPosition(GetXPosition() + nibPos, GetYPosition());
    m_guiNibNoFocus.SetPosition(GetXPosition() + nibPos, GetYPosition());
    m_guiNibFocus.SetPosition(GetXPosition() + nibPos, GetYPosition());
  }
}

bool CGUIScrollBar::HitTest(const CPoint &point) const
{
  if (m_guiBackground.HitTest(point)) return true;
  if (m_guiBarNoFocus.HitTest(point)) return true;
  return false;
}

void CGUIScrollBar::SetFromPosition(const CPoint &point)
{
  float fPercent;
  if (m_orientation == VERTICAL)
    fPercent = (point.y - m_guiBackground.GetYPosition() - 0.5f*m_guiBarFocus.GetHeight()) / m_guiBackground.GetHeight();
  else
    fPercent = (point.x - m_guiBackground.GetXPosition() - 0.5f*m_guiBarFocus.GetWidth()) / m_guiBackground.GetWidth();
  if (fPercent < 0) fPercent = 0;
  if (fPercent > 1) fPercent = 1;
  m_offset = (int)(floor(fPercent * m_numItems + 0.5f));
  CGUIMessage message(GUI_MSG_NOTIFY_ALL, GetParentID(), GetID(), GUI_MSG_PAGE_CHANGE, m_offset);
  SendWindowMessage(message);
  SetInvalid();
}

EVENT_RESULT CGUIScrollBar::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  if (event.m_id == ACTION_MOUSE_DRAG)
  {
    if (event.m_state == 1)
    { // we want exclusive access
      CGUIMessage msg(GUI_MSG_EXCLUSIVE_MOUSE, GetID(), GetParentID());
      SendWindowMessage(msg);
    }
    else if (event.m_state == 3)
    { // we're done with exclusive access
      CGUIMessage msg(GUI_MSG_EXCLUSIVE_MOUSE, 0, GetParentID());
      SendWindowMessage(msg);
    }
    SetFromPosition(point);
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_MOUSE_LEFT_CLICK && m_guiBackground.HitTest(point))
  {
    SetFromPosition(point);
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_MOUSE_WHEEL_UP)
  {
    Move(-1);
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_MOUSE_WHEEL_DOWN)
  {
    Move(1);
    return EVENT_RESULT_HANDLED;
  }
  return EVENT_RESULT_UNHANDLED;
}

CStdString CGUIScrollBar::GetDescription() const
{
  CStdString description;
  description.Format("%i/%i", m_offset, m_numItems);
  return description;
}

void CGUIScrollBar::UpdateColors()
{
  CGUIControl::UpdateColors();
  m_guiBackground.SetDiffuseColor(m_diffuseColor);
  m_guiBarNoFocus.SetDiffuseColor(m_diffuseColor);
  m_guiBarFocus.SetDiffuseColor(m_diffuseColor);
  m_guiNibNoFocus.SetDiffuseColor(m_diffuseColor);
  m_guiNibFocus.SetDiffuseColor(m_diffuseColor);
}

bool CGUIScrollBar::IsVisible() const
{
  // page controls can be optionally disabled if the number of pages is 1
  if (m_numItems <= m_pageSize && !m_showOnePage)
    return false;
  return CGUIControl::IsVisible();
}
