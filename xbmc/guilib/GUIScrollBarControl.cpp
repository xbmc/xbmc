/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIScrollBarControl.h"

#include "GUIMessage.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "input/mouse/MouseEvent.h"
#include "input/mouse/MouseStat.h"
#include "utils/StringUtils.h"

using namespace KODI;

#define MIN_NIB_SIZE 4.0f

GUIScrollBarControl::GUIScrollBarControl(int parentID,
                                         int controlID,
                                         float posX,
                                         float posY,
                                         float width,
                                         float height,
                                         const CTextureInfo& backGroundTexture,
                                         const CTextureInfo& barTexture,
                                         const CTextureInfo& barTextureFocus,
                                         const CTextureInfo& nibTexture,
                                         const CTextureInfo& nibTextureFocus,
                                         ORIENTATION orientation,
                                         bool showOnePage)
  : CGUIControl(parentID, controlID, posX, posY, width, height),
    m_guiBackground(CGUITexture::CreateTexture(posX, posY, width, height, backGroundTexture)),
    m_guiBarNoFocus(CGUITexture::CreateTexture(posX, posY, width, height, barTexture)),
    m_guiBarFocus(CGUITexture::CreateTexture(posX, posY, width, height, barTextureFocus)),
    m_guiNibNoFocus(CGUITexture::CreateTexture(posX, posY, width, height, nibTexture)),
    m_guiNibFocus(CGUITexture::CreateTexture(posX, posY, width, height, nibTextureFocus))
{
  m_guiNibNoFocus->SetAspectRatio(CAspectRatio::AR_CENTER);
  m_guiNibFocus->SetAspectRatio(CAspectRatio::AR_CENTER);
  m_numItems = 100;
  m_offset = 0;
  m_pageSize = 10;
  ControlType = GUICONTROL_SCROLLBAR;
  m_orientation = orientation;
  m_showOnePage = showOnePage;
}

GUIScrollBarControl::GUIScrollBarControl(const GUIScrollBarControl& control)
  : CGUIControl(control),
    m_guiBackground(control.m_guiBackground->Clone()),
    m_guiBarNoFocus(control.m_guiBarNoFocus->Clone()),
    m_guiBarFocus(control.m_guiBarFocus->Clone()),
    m_guiNibNoFocus(control.m_guiNibNoFocus->Clone()),
    m_guiNibFocus(control.m_guiNibFocus->Clone()),
    m_numItems(control.m_numItems),
    m_pageSize(control.m_pageSize),
    m_offset(control.m_offset),
    m_showOnePage(control.m_showOnePage),
    m_orientation(control.m_orientation)
{
}

void GUIScrollBarControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  bool changed = false;

  if (m_bInvalidated)
    changed |= UpdateBarSize();

  changed |= m_guiBackground->Process(currentTime);
  changed |= m_guiBarNoFocus->Process(currentTime);
  changed |= m_guiBarFocus->Process(currentTime);
  changed |= m_guiNibNoFocus->Process(currentTime);
  changed |= m_guiNibFocus->Process(currentTime);

  if (changed)
    MarkDirtyRegion();

  CGUIControl::Process(currentTime, dirtyregions);
}

void GUIScrollBarControl::Render()
{
  m_guiBackground->Render();
  if (m_bHasFocus)
  {
    m_guiBarFocus->Render();
    m_guiNibFocus->Render();
  }
  else
  {
    m_guiBarNoFocus->Render();
    m_guiNibNoFocus->Render();
  }

  CGUIControl::Render();
}

bool GUIScrollBarControl::OnMessage(CGUIMessage& message)
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

bool GUIScrollBarControl::OnAction(const CAction &action)
{
  switch ( action.GetID() )
  {
  case ACTION_MOVE_LEFT:
    if (m_orientation == HORIZONTAL)
    {
      if(Move( -1))
        return true;
    }
    break;

  case ACTION_MOVE_RIGHT:
    if (m_orientation == HORIZONTAL)
    {
      if(Move(1))
        return true;
    }
    break;
  case ACTION_MOVE_UP:
    if (m_orientation == VERTICAL)
    {
      if(Move(-1))
        return true;
    }
    break;

  case ACTION_MOVE_DOWN:
    if (m_orientation == VERTICAL)
    {
      if(Move(1))
        return true;
    }
    break;
  }
  return CGUIControl::OnAction(action);
}

bool GUIScrollBarControl::Move(int numSteps)
{
  if (numSteps < 0 && m_offset == 0) // we are at the beginning - can't scroll up/left anymore
    return false;
  if (numSteps > 0 && m_offset == std::max(m_numItems - m_pageSize, 0)) // we are at the end - we can't scroll down/right anymore
    return false;

  m_offset += numSteps * m_pageSize;
  if (m_offset > m_numItems - m_pageSize) m_offset = m_numItems - m_pageSize;
  if (m_offset < 0) m_offset = 0;
  CGUIMessage message(GUI_MSG_NOTIFY_ALL, GetParentID(), GetID(), GUI_MSG_PAGE_CHANGE, m_offset);
  SendWindowMessage(message);
  SetInvalid();
  return true;
}

void GUIScrollBarControl::SetRange(int pageSize, int numItems)
{
  if (m_pageSize != pageSize || m_numItems != numItems)
  {
    m_pageSize = pageSize;
    m_numItems = numItems;
    m_offset = 0;
    SetInvalid();
  }
}

void GUIScrollBarControl::SetValue(int value)
{
  if (m_offset != value)
  {
    m_offset = value;
    SetInvalid();
  }
}

void GUIScrollBarControl::FreeResources(bool immediately)
{
  CGUIControl::FreeResources(immediately);
  m_guiBackground->FreeResources(immediately);
  m_guiBarNoFocus->FreeResources(immediately);
  m_guiBarFocus->FreeResources(immediately);
  m_guiNibNoFocus->FreeResources(immediately);
  m_guiNibFocus->FreeResources(immediately);
}

void GUIScrollBarControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_guiBackground->DynamicResourceAlloc(bOnOff);
  m_guiBarNoFocus->DynamicResourceAlloc(bOnOff);
  m_guiBarFocus->DynamicResourceAlloc(bOnOff);
  m_guiNibNoFocus->DynamicResourceAlloc(bOnOff);
  m_guiNibFocus->DynamicResourceAlloc(bOnOff);
}

void GUIScrollBarControl::AllocResources()
{
  CGUIControl::AllocResources();
  m_guiBackground->AllocResources();
  m_guiBarNoFocus->AllocResources();
  m_guiBarFocus->AllocResources();
  m_guiNibNoFocus->AllocResources();
  m_guiNibFocus->AllocResources();
}

void GUIScrollBarControl::SetInvalid()
{
  CGUIControl::SetInvalid();
  m_guiBackground->SetInvalid();
  m_guiBarFocus->SetInvalid();
  m_guiBarFocus->SetInvalid();
  m_guiNibNoFocus->SetInvalid();
  m_guiNibFocus->SetInvalid();
}

bool GUIScrollBarControl::UpdateBarSize()
{
  bool changed = false;

  // scale our textures to suit
  if (m_orientation == VERTICAL)
  {
    // calculate the height to display the nib at
    float percent = (m_numItems == 0) ? 0 : (float)m_pageSize / m_numItems;
    float nibSize = GetHeight() * percent;
    if (nibSize < m_guiNibFocus->GetTextureHeight() + 2 * MIN_NIB_SIZE)
      nibSize = m_guiNibFocus->GetTextureHeight() + 2 * MIN_NIB_SIZE;
    if (nibSize > GetHeight()) nibSize = GetHeight();

    changed |= m_guiBarNoFocus->SetHeight(nibSize);
    changed |= m_guiBarFocus->SetHeight(nibSize);
    changed |= m_guiNibNoFocus->SetHeight(nibSize);
    changed |= m_guiNibFocus->SetHeight(nibSize);
    // nibSize may be altered by the border size of the nib (and bar).
    nibSize = std::max(m_guiBarFocus->GetHeight(), m_guiNibFocus->GetHeight());

    // and the position
    percent = (m_numItems == m_pageSize) ? 0 : (float)m_offset / (m_numItems - m_pageSize);
    float nibPos = (GetHeight() - nibSize) * percent;
    if (nibPos < 0) nibPos = 0;
    if (nibPos > GetHeight() - nibSize) nibPos = GetHeight() - nibSize;

    changed |= m_guiBarNoFocus->SetPosition(GetXPosition(), GetYPosition() + nibPos);
    changed |= m_guiBarFocus->SetPosition(GetXPosition(), GetYPosition() + nibPos);
    changed |= m_guiNibNoFocus->SetPosition(GetXPosition(), GetYPosition() + nibPos);
    changed |= m_guiNibFocus->SetPosition(GetXPosition(), GetYPosition() + nibPos);
  }
  else
  {
    // calculate the height to display the nib at
    float percent = (m_numItems == 0) ? 0 : (float)m_pageSize / m_numItems;
    float nibSize = GetWidth() * percent + 0.5f;
    if (nibSize < m_guiNibFocus->GetTextureWidth() + 2 * MIN_NIB_SIZE)
      nibSize = m_guiNibFocus->GetTextureWidth() + 2 * MIN_NIB_SIZE;
    if (nibSize > GetWidth()) nibSize = GetWidth();

    changed |= m_guiBarNoFocus->SetWidth(nibSize);
    changed |= m_guiBarFocus->SetWidth(nibSize);
    changed |= m_guiNibNoFocus->SetWidth(nibSize);
    changed |= m_guiNibFocus->SetWidth(nibSize);

    // and the position
    percent = (m_numItems == m_pageSize) ? 0 : (float)m_offset / (m_numItems - m_pageSize);
    float nibPos = (GetWidth() - nibSize) * percent;
    if (nibPos < 0) nibPos = 0;
    if (nibPos > GetWidth() - nibSize) nibPos = GetWidth() - nibSize;

    changed |= m_guiBarNoFocus->SetPosition(GetXPosition() + nibPos, GetYPosition());
    changed |= m_guiBarFocus->SetPosition(GetXPosition() + nibPos, GetYPosition());
    changed |= m_guiNibNoFocus->SetPosition(GetXPosition() + nibPos, GetYPosition());
    changed |= m_guiNibFocus->SetPosition(GetXPosition() + nibPos, GetYPosition());
  }

  return changed;
}

void GUIScrollBarControl::SetFromPosition(const CPoint &point)
{
  float fPercent;
  if (m_orientation == VERTICAL)
    fPercent = (point.y - m_guiBackground->GetYPosition() - 0.5f * m_guiBarFocus->GetHeight()) /
               (m_guiBackground->GetHeight() - m_guiBarFocus->GetHeight());
  else
    fPercent = (point.x - m_guiBackground->GetXPosition() - 0.5f * m_guiBarFocus->GetWidth()) /
               (m_guiBackground->GetWidth() - m_guiBarFocus->GetWidth());
  if (fPercent < 0) fPercent = 0;
  if (fPercent > 1) fPercent = 1;

  int offset = (int)(floor(fPercent * (m_numItems - m_pageSize) + 0.5f));

  if (m_offset != offset)
  {
    m_offset = offset;
    CGUIMessage message(GUI_MSG_NOTIFY_ALL, GetParentID(), GetID(), GUI_MSG_PAGE_CHANGE, m_offset);
    SendWindowMessage(message);
    SetInvalid();
  }
}

EVENT_RESULT GUIScrollBarControl::OnMouseEvent(const CPoint& point, const MOUSE::CMouseEvent& event)
{
  if (event.m_id == ACTION_MOUSE_DRAG || event.m_id == ACTION_MOUSE_DRAG_END)
  {
    if (static_cast<HoldAction>(event.m_state) == HoldAction::DRAG)
    { // we want exclusive access
      CGUIMessage msg(GUI_MSG_EXCLUSIVE_MOUSE, GetID(), GetParentID());
      SendWindowMessage(msg);
    }
    else if (static_cast<HoldAction>(event.m_state) == HoldAction::DRAG_END)
    { // we're done with exclusive access
      CGUIMessage msg(GUI_MSG_EXCLUSIVE_MOUSE, 0, GetParentID());
      SendWindowMessage(msg);
    }
    SetFromPosition(point);
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_MOUSE_LEFT_CLICK && m_guiBackground->HitTest(point))
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
  else if (event.m_id == ACTION_GESTURE_NOTIFY)
  {
    return (m_orientation == HORIZONTAL) ? EVENT_RESULT_PAN_HORIZONTAL_WITHOUT_INERTIA : EVENT_RESULT_PAN_VERTICAL_WITHOUT_INERTIA;
  }
  else if (event.m_id == ACTION_GESTURE_BEGIN)
  { // grab exclusive access
    CGUIMessage msg(GUI_MSG_EXCLUSIVE_MOUSE, GetID(), GetParentID());
    SendWindowMessage(msg);
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_GESTURE_PAN)
  { // do the drag
    SetFromPosition(point);
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_GESTURE_END || event.m_id == ACTION_GESTURE_ABORT)
  { // release exclusive access
    CGUIMessage msg(GUI_MSG_EXCLUSIVE_MOUSE, 0, GetParentID());
    SendWindowMessage(msg);
    return EVENT_RESULT_HANDLED;
  }

  return EVENT_RESULT_UNHANDLED;
}

std::string GUIScrollBarControl::GetDescription() const
{
  return StringUtils::Format("{}/{}", m_offset, m_numItems);
}

bool GUIScrollBarControl::UpdateColors(const CGUIListItem* item)
{
  bool changed = CGUIControl::UpdateColors(nullptr);
  changed |= m_guiBackground->SetDiffuseColor(m_diffuseColor);
  changed |= m_guiBarNoFocus->SetDiffuseColor(m_diffuseColor);
  changed |= m_guiBarFocus->SetDiffuseColor(m_diffuseColor);
  changed |= m_guiNibNoFocus->SetDiffuseColor(m_diffuseColor);
  changed |= m_guiNibFocus->SetDiffuseColor(m_diffuseColor);

  return changed;
}

bool GUIScrollBarControl::IsVisible() const
{
  // page controls can be optionally disabled if the number of pages is 1
  if (m_numItems <= m_pageSize && !m_showOnePage)
    return false;
  return CGUIControl::IsVisible();
}
