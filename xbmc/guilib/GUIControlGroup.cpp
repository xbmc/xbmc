/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIControlGroup.h"

#include "GUIMessage.h"
#include "input/mouse/MouseEvent.h"

#include <cassert>
#include <utility>

using namespace KODI;

CGUIControlGroup::CGUIControlGroup()
{
  m_defaultControl = 0;
  m_defaultAlways = false;
  m_focusedControl = 0;
  m_renderFocusedLast = false;
  ControlType = GUICONTROL_GROUP;
}

CGUIControlGroup::CGUIControlGroup(int parentID, int controlID, float posX, float posY, float width, float height)
: CGUIControlLookup(parentID, controlID, posX, posY, width, height)
{
  m_defaultControl = 0;
  m_defaultAlways = false;
  m_focusedControl = 0;
  m_renderFocusedLast = false;
  ControlType = GUICONTROL_GROUP;
}

CGUIControlGroup::CGUIControlGroup(const CGUIControlGroup &from)
: CGUIControlLookup(from)
{
  m_defaultControl = from.m_defaultControl;
  m_defaultAlways = from.m_defaultAlways;
  m_renderFocusedLast = from.m_renderFocusedLast;

  // run through and add our controls
  for (auto *i : from.m_children)
    AddControl(i->Clone());

  // defaults
  m_focusedControl = 0;
  ControlType = GUICONTROL_GROUP;
}

CGUIControlGroup::~CGUIControlGroup(void)
{
  ClearAll();
}

void CGUIControlGroup::AllocResources()
{
  CGUIControl::AllocResources();
  for (auto *control : m_children)
  {
    if (!control->IsDynamicallyAllocated())
      control->AllocResources();
  }
}

void CGUIControlGroup::FreeResources(bool immediately)
{
  CGUIControl::FreeResources(immediately);
  for (auto *control : m_children)
  {
    control->FreeResources(immediately);
  }
}

void CGUIControlGroup::DynamicResourceAlloc(bool bOnOff)
{
  for (auto *control : m_children)
  {
    control->DynamicResourceAlloc(bOnOff);
  }
}

void CGUIControlGroup::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  CPoint pos(GetPosition());
  CServiceBroker::GetWinSystem()->GetGfxContext().SetOrigin(pos.x, pos.y);

  CRect rect;
  for (auto *control : m_children)
  {
    control->UpdateVisibility(nullptr);
    unsigned int oldDirty = dirtyregions.size();
    control->DoProcess(currentTime, dirtyregions);
    if (control->IsVisible() || (oldDirty != dirtyregions.size())) // visible or dirty (was visible?)
      rect.Union(control->GetRenderRegion());
  }

  CServiceBroker::GetWinSystem()->GetGfxContext().RestoreOrigin();
  CGUIControl::Process(currentTime, dirtyregions);
  m_renderRegion = rect;
}

void CGUIControlGroup::Render()
{
  CPoint pos(GetPosition());
  CServiceBroker::GetWinSystem()->GetGfxContext().SetOrigin(pos.x, pos.y);
  CGUIControl *focusedControl = NULL;
  if (CServiceBroker::GetWinSystem()->GetGfxContext().GetRenderOrder() ==
      RENDER_ORDER_FRONT_TO_BACK)
  {
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it)
    {
      if (m_renderFocusedLast && (*it)->HasFocus())
        focusedControl = (*it);
      else
        (*it)->DoRender();
    }
  }
  else
  {
    for (auto* control : m_children)
    {
      if (m_renderFocusedLast && control->HasFocus())
        focusedControl = control;
      else
        control->DoRender();
    }
  }
  if (focusedControl)
    focusedControl->DoRender();
  CGUIControl::Render();
  CServiceBroker::GetWinSystem()->GetGfxContext().RestoreOrigin();
}

void CGUIControlGroup::RenderEx()
{
  for (auto *control : m_children)
    control->RenderEx();
  CGUIControl::RenderEx();
}

bool CGUIControlGroup::OnAction(const CAction &action)
{
  return false;
}

bool CGUIControlGroup::HasFocus() const
{
  for (auto *control : m_children)
  {
    if (control->HasFocus())
      return true;
  }
  return false;
}

bool CGUIControlGroup::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage() )
  {
  case GUI_MSG_ITEM_SELECT:
    {
      if (message.GetControlId() == GetID())
      {
        m_focusedControl = message.GetParam1();
        return true;
      }
      break;
    }
  case GUI_MSG_ITEM_SELECTED:
    {
      if (message.GetControlId() == GetID())
      {
        message.SetParam1(m_focusedControl);
        return true;
      }
      break;
    }
  case GUI_MSG_FOCUSED:
    { // a control has been focused
      m_focusedControl = message.GetControlId();
      SetFocus(true);
      // tell our parent thatwe have focus
      if (m_parentControl)
        m_parentControl->OnMessage(message);
      return true;
    }
  case GUI_MSG_SETFOCUS:
    {
      // first try our last focused control...
      if (!m_defaultAlways && m_focusedControl)
      {
        CGUIControl *control = GetFirstFocusableControl(m_focusedControl);
        if (control)
        {
          CGUIMessage msg(GUI_MSG_SETFOCUS, GetParentID(), control->GetID());
          return control->OnMessage(msg);
        }
      }
      // ok, no previously focused control, try the default control first
      if (m_defaultControl)
      {
        CGUIControl *control = GetFirstFocusableControl(m_defaultControl);
        if (control)
        {
          CGUIMessage msg(GUI_MSG_SETFOCUS, GetParentID(), control->GetID());
          return control->OnMessage(msg);
        }
      }
      // no success with the default control, so just find one to focus
      CGUIControl *control = GetFirstFocusableControl(0);
      if (control)
      {
        CGUIMessage msg(GUI_MSG_SETFOCUS, GetParentID(), control->GetID());
        return control->OnMessage(msg);
      }
      // unsuccessful
      return false;
      break;
    }
  case GUI_MSG_LOSTFOCUS:
    {
      // set all subcontrols unfocused
      for (auto *control : m_children)
        control->SetFocus(false);
      if (!GetControl(message.GetParam1()))
      { // we don't have the new id, so unfocus
        SetFocus(false);
        if (m_parentControl)
          m_parentControl->OnMessage(message);
      }
      return true;
    }
    break;
  case GUI_MSG_PAGE_CHANGE:
  case GUI_MSG_REFRESH_THUMBS:
  case GUI_MSG_REFRESH_LIST:
  case GUI_MSG_WINDOW_RESIZE:
    { // send to all child controls (make sure the target is the control id)
      for (auto *control : m_children)
      {
        CGUIMessage msg(message.GetMessage(), message.GetSenderId(), control->GetID(), message.GetParam1());
        control->OnMessage(msg);
      }
      return true;
    }
    break;
  case GUI_MSG_REFRESH_TIMER:
    if (!IsVisible() || !IsVisibleFromSkin())
      return true;
    break;
  }
  bool handled(false);
  //not intended for any specific control, send to all childs and our base handler.
  if (message.GetControlId() == 0)
  {
    for (auto *control : m_children)
    {
      handled |= control->OnMessage(message);
    }
    return CGUIControl::OnMessage(message) || handled;
  }
  // if it's intended for us, then so be it
  if (message.GetControlId() == GetID())
    return CGUIControl::OnMessage(message);

  return SendControlMessage(message);
}

bool CGUIControlGroup::SendControlMessage(CGUIMessage &message)
{
  IDCollector collector(m_idCollector);

  CGUIControl *ctrl(GetControl(message.GetControlId(), collector.m_collector));
  // see if a child matches, and send to the child control if so
  if (ctrl && ctrl->OnMessage(message))
    return true;

  // Unhandled - send to all matching invisible controls as well
  bool handled(false);
  for (auto *control : *collector.m_collector)
    if (control->OnMessage(message))
      handled = true;

  return handled;
}

bool CGUIControlGroup::CanFocus() const
{
  if (!CGUIControl::CanFocus()) return false;
  // see if we have any children that can be focused
  for (auto *control : m_children)
  {
    if (control->CanFocus())
      return true;
  }
  return false;
}

void CGUIControlGroup::AssignDepth()
{
  CGUIControl* focusedControl = nullptr;
  if (m_children.size())
  {
    for (auto* control : m_children)
    {
      if (m_renderFocusedLast && control->HasFocus())
        focusedControl = control;
      else
        control->AssignDepth();
    }
  }
  if (focusedControl)
    focusedControl->AssignDepth();
}

void CGUIControlGroup::SetInitialVisibility()
{
  CGUIControl::SetInitialVisibility();
  for (auto *control : m_children)
    control->SetInitialVisibility();
}

void CGUIControlGroup::QueueAnimation(ANIMATION_TYPE animType)
{
  CGUIControl::QueueAnimation(animType);
  // send window level animations to our children as well
  if (animType == ANIM_TYPE_WINDOW_OPEN || animType == ANIM_TYPE_WINDOW_CLOSE)
  {
    for (auto *control : m_children)
      control->QueueAnimation(animType);
  }
}

void CGUIControlGroup::ResetAnimation(ANIMATION_TYPE animType)
{
  CGUIControl::ResetAnimation(animType);
  // send window level animations to our children as well
  if (animType == ANIM_TYPE_WINDOW_OPEN || animType == ANIM_TYPE_WINDOW_CLOSE)
  {
    for (auto *control : m_children)
      control->ResetAnimation(animType);
  }
}

void CGUIControlGroup::ResetAnimations()
{ // resets all animations, regardless of condition
  CGUIControl::ResetAnimations();
  for (auto *control : m_children)
    control->ResetAnimations();
}

bool CGUIControlGroup::IsAnimating(ANIMATION_TYPE animType)
{
  if (CGUIControl::IsAnimating(animType))
    return true;

  if (IsVisible())
  {
    for (auto *control : m_children)
    {
      if (control->IsAnimating(animType))
        return true;
    }
  }
  return false;
}

bool CGUIControlGroup::HasAnimation(ANIMATION_TYPE animType)
{
  if (CGUIControl::HasAnimation(animType))
    return true;

  if (IsVisible())
  {
    for (auto *control : m_children)
    {
      if (control->HasAnimation(animType))
        return true;
    }
  }
  return false;
}

EVENT_RESULT CGUIControlGroup::SendMouseEvent(const CPoint& point, const MOUSE::CMouseEvent& event)
{
  // transform our position into child coordinates
  CPoint childPoint(point);
  m_transform.InverseTransformPosition(childPoint.x, childPoint.y);

  if (CGUIControl::CanFocus())
  {
    CPoint pos(GetPosition());
    // run through our controls in reverse order (so that last rendered is checked first)
    for (rControls i = m_children.rbegin(); i != m_children.rend(); ++i)
    {
      CGUIControl *child = *i;
      EVENT_RESULT ret = child->SendMouseEvent(childPoint - pos, event);
      if (ret)
      { // we've handled the action, and/or have focused an item
        return ret;
      }
    }
    // none of our children want the event, but we may want it.
    EVENT_RESULT ret;
    if (HitTest(childPoint) && (ret = OnMouseEvent(childPoint, event)))
      return ret;
  }
  m_focusedControl = 0;
  return EVENT_RESULT_UNHANDLED;
}

void CGUIControlGroup::UnfocusFromPoint(const CPoint &point)
{
  CPoint controlCoords(point);
  m_transform.InverseTransformPosition(controlCoords.x, controlCoords.y);
  controlCoords -= GetPosition();
  for (auto *child : m_children)
  {
    child->UnfocusFromPoint(controlCoords);
  }
  CGUIControl::UnfocusFromPoint(point);
}

int CGUIControlGroup::GetFocusedControlID() const
{
  if (m_focusedControl) return m_focusedControl;
  CGUIControl *control = GetFocusedControl();
  if (control) return control->GetID();
  return 0;
}

CGUIControl *CGUIControlGroup::GetFocusedControl() const
{
  // try lookup first
  if (m_focusedControl)
  {
    // we may have multiple controls with same id - we pick first that has focus
    std::pair<LookupMap::const_iterator, LookupMap::const_iterator> range = GetLookupControls(m_focusedControl);

    for (LookupMap::const_iterator i = range.first; i != range.second; ++i)
    {
      if (i->second->HasFocus())
        return i->second;
    }
  }

  // if lookup didn't find focused control, iterate m_children to find it
  for (auto *control : m_children)
  {
    // Avoid calling HasFocus() on control group as it will (possibly) recursively
    // traverse entire group tree just to check if there is focused control.
    // We are recursively traversing it here so no point in doing it twice.
    CGUIControlGroup *groupControl(dynamic_cast<CGUIControlGroup*>(control));
    if (groupControl)
    {
      CGUIControl* focusedControl = groupControl->GetFocusedControl();
      if (focusedControl)
        return focusedControl;
    }
    else if (control->HasFocus())
      return control;
  }
  return NULL;
}

// in the case of id == 0, we don't match id
CGUIControl *CGUIControlGroup::GetFirstFocusableControl(int id)
{
  if (!CanFocus()) return NULL;
  if (id && id == GetID()) return this; // we're focusable and they want us
  for (auto *pControl : m_children)
  {
    CGUIControlGroup *group(dynamic_cast<CGUIControlGroup*>(pControl));
    if (group)
    {
      CGUIControl *control = group->GetFirstFocusableControl(id);
      if (control) return control;
    }
    if ((!id || pControl->GetID() == id) && pControl->CanFocus())
      return pControl;
  }
  return NULL;
}

void CGUIControlGroup::AddControl(CGUIControl *control, int position /* = -1*/)
{
  if (!control) return;
  if (position < 0 || position > (int)m_children.size())
    position = (int)m_children.size();
  m_children.insert(m_children.begin() + position, control);
  control->SetParentControl(this);
  control->SetControlStats(m_controlStats);
  control->SetPushUpdates(m_pushedUpdates);
  AddLookup(control);
  SetInvalid();
}

bool CGUIControlGroup::InsertControl(CGUIControl *control, const CGUIControl *insertPoint)
{
  // find our position
  for (unsigned int i = 0; i < m_children.size(); i++)
  {
    CGUIControl *child = m_children[i];
    CGUIControlGroup *group(dynamic_cast<CGUIControlGroup*>(child));
    if (group && group->InsertControl(control, insertPoint))
      return true;
    else if (child == insertPoint)
    {
      AddControl(control, i);
      return true;
    }
  }
  return false;
}

void CGUIControlGroup::SaveStates(std::vector<CControlState> &states)
{
  // save our state, and that of our children
  states.emplace_back(GetID(), m_focusedControl);
  for (auto *control : m_children)
    control->SaveStates(states);
}

// Note: This routine doesn't delete the control.  It just removes it from the control list
bool CGUIControlGroup::RemoveControl(const CGUIControl *control)
{
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *child = *it;
    CGUIControlGroup *group(dynamic_cast<CGUIControlGroup*>(child));
    if (group && group->RemoveControl(control))
      return true;
    if (control == child)
    {
      m_children.erase(it);
      RemoveLookup(child);
      SetInvalid();
      return true;
    }
  }
  return false;
}

void CGUIControlGroup::ClearAll()
{
  // first remove from the lookup table
  RemoveLookup();

  // and delete all our children
  for (auto *control : m_children)
  {
    delete control;
  }
  m_focusedControl = 0;
  m_children.clear();
  ClearLookup();
  SetInvalid();
}

#ifdef _DEBUG
void CGUIControlGroup::DumpTextureUse()
{
  for (auto *control : m_children)
    control->DumpTextureUse();
}
#endif
