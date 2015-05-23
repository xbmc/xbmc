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

#include "GUIControlGroup.h"

#include <cassert>

using namespace std;

CGUIControlGroup::CGUIControlGroup()
{
  m_defaultControl = 0;
  m_defaultAlways = false;
  m_focusedControl = 0;
  m_renderFocusedLast = false;
  ControlType = GUICONTROL_GROUP;
}

CGUIControlGroup::CGUIControlGroup(int parentID, int controlID, float posX, float posY, float width, float height)
: CGUIControl(parentID, controlID, posX, posY, width, height)
{
  m_defaultControl = 0;
  m_defaultAlways = false;
  m_focusedControl = 0;
  m_renderFocusedLast = false;
  ControlType = GUICONTROL_GROUP;
}

CGUIControlGroup::CGUIControlGroup(const CGUIControlGroup &from)
: CGUIControl(from)
{
  m_defaultControl = from.m_defaultControl;
  m_defaultAlways = from.m_defaultAlways;
  m_renderFocusedLast = from.m_renderFocusedLast;

  // run through and add our controls
  for (ciControls it = from.m_children.begin(); it != from.m_children.end(); ++it)
    AddControl((*it)->Clone());

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
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *control = *it;
    if (!control->IsDynamicallyAllocated())
      control->AllocResources();
  }
}

void CGUIControlGroup::FreeResources(bool immediately)
{
  CGUIControl::FreeResources(immediately);
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *control = *it;
    control->FreeResources(immediately);
  }
}

void CGUIControlGroup::DynamicResourceAlloc(bool bOnOff)
{
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *control = *it;
    control->DynamicResourceAlloc(bOnOff);
  }
}

void CGUIControlGroup::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  CPoint pos(GetPosition());
  g_graphicsContext.SetOrigin(pos.x, pos.y);

  CRect rect;
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *control = *it;
    control->UpdateVisibility();
    unsigned int oldDirty = dirtyregions.size();
    control->DoProcess(currentTime, dirtyregions);
    if (control->IsVisible() || (oldDirty != dirtyregions.size())) // visible or dirty (was visible?)
      rect.Union(control->GetRenderRegion());
  }

  g_graphicsContext.RestoreOrigin();
  CGUIControl::Process(currentTime, dirtyregions);
  m_renderRegion = rect;
}

void CGUIControlGroup::Render()
{
  CPoint pos(GetPosition());
  g_graphicsContext.SetOrigin(pos.x, pos.y);
  CGUIControl *focusedControl = NULL;
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *control = *it;
    if (m_renderFocusedLast && control->HasFocus())
      focusedControl = control;
    else
      control->DoRender();
  }
  if (focusedControl)
    focusedControl->DoRender();
  CGUIControl::Render();
  g_graphicsContext.RestoreOrigin();
}

void CGUIControlGroup::RenderEx()
{
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
    (*it)->RenderEx();
  CGUIControl::RenderEx();
}

bool CGUIControlGroup::OnAction(const CAction &action)
{
  assert(false);  // unimplemented
  return false;
}

bool CGUIControlGroup::HasFocus() const
{
  for (ciControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *control = *it;
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
      for (iControls it = m_children.begin(); it != m_children.end(); ++it)
        (*it)->SetFocus(false);
      if (!HasID(message.GetParam1()))
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
      for (iControls it = m_children.begin(); it != m_children.end(); ++it)
      {
        CGUIMessage msg(message.GetMessage(), message.GetSenderId(), (*it)->GetID(), message.GetParam1());
        (*it)->OnMessage(msg);
      }
      return true;
    }
    break;
  }
  bool handled(false);
  //not intented for any specific control, send to all childs and our base handler.
  if (message.GetControlId() == 0)
  {
    for (iControls it = m_children.begin();it != m_children.end(); ++it)
    {
      CGUIControl* control = *it;
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
  // see if a child matches, and send to the child control if so
  for (iControls it = m_children.begin();it != m_children.end(); ++it)
  {
    CGUIControl* control = *it;
    if (control->HasVisibleID(message.GetControlId()))
    {
      if (control->OnMessage(message))
        return true;
    }
  }
  // Unhandled - send to all matching invisible controls as well
  bool handled(false);
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl* control = *it;
    if (control->HasID(message.GetControlId()))
    {
      if (control->OnMessage(message))
        handled = true;
    }
  }
  return handled;
}

bool CGUIControlGroup::CanFocus() const
{
  if (!CGUIControl::CanFocus()) return false;
  // see if we have any children that can be focused
  for (ciControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    if ((*it)->CanFocus())
      return true;
  }
  return false;
}

void CGUIControlGroup::SetInitialVisibility()
{
  CGUIControl::SetInitialVisibility();
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
    (*it)->SetInitialVisibility();
}

void CGUIControlGroup::QueueAnimation(ANIMATION_TYPE animType)
{
  CGUIControl::QueueAnimation(animType);
  // send window level animations to our children as well
  if (animType == ANIM_TYPE_WINDOW_OPEN || animType == ANIM_TYPE_WINDOW_CLOSE)
  {
    for (iControls it = m_children.begin(); it != m_children.end(); ++it)
      (*it)->QueueAnimation(animType);
  }
}

void CGUIControlGroup::ResetAnimation(ANIMATION_TYPE animType)
{
  CGUIControl::ResetAnimation(animType);
  // send window level animations to our children as well
  if (animType == ANIM_TYPE_WINDOW_OPEN || animType == ANIM_TYPE_WINDOW_CLOSE)
  {
    for (iControls it = m_children.begin(); it != m_children.end(); ++it)
      (*it)->ResetAnimation(animType);
  }
}

void CGUIControlGroup::ResetAnimations()
{ // resets all animations, regardless of condition
  CGUIControl::ResetAnimations();
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
    (*it)->ResetAnimations();
}

bool CGUIControlGroup::IsAnimating(ANIMATION_TYPE animType)
{
  if (CGUIControl::IsAnimating(animType))
    return true;

  if (IsVisible())
  {
    for (iControls it = m_children.begin(); it != m_children.end(); ++it)
    {
      if ((*it)->IsAnimating(animType))
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
    for (iControls it = m_children.begin(); it != m_children.end(); ++it)
    {
      if ((*it)->HasAnimation(animType))
        return true;
    }
  }
  return false;
}

EVENT_RESULT CGUIControlGroup::SendMouseEvent(const CPoint &point, const CMouseEvent &event)
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
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *child = *it;
    child->UnfocusFromPoint(controlCoords);
  }
  CGUIControl::UnfocusFromPoint(point);
}

bool CGUIControlGroup::HasID(int id) const
{
  if (CGUIControl::HasID(id)) return true;
  for (ciControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *child = *it;
    if (child->HasID(id))
      return true;
  }
  return false;
}

bool CGUIControlGroup::HasVisibleID(int id) const
{
  // call base class first as the group may be the requested control
  if (CGUIControl::HasVisibleID(id)) return true;
  // if the group isn't visible, then none of it's children can be
  if (!IsVisible()) return false;
  for (ciControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *child = *it;
    if (child->HasVisibleID(id))
      return true;
  }
  return false;
}

CGUIControl *CGUIControlGroup::GetControl(int iControl)
{
  CGUIControl *pPotential = NULL;
  LookupMap::iterator first = m_lookup.find(iControl);
  if (first != m_lookup.end())
  {
    LookupMap::iterator last = m_lookup.upper_bound(iControl);
    for (LookupMap::iterator i = first; i != last; ++i)
    {
      CGUIControl *control = i->second;
      if (control->IsVisible())
        return control;
      else if (!pPotential)
        pPotential = control;
    }
  }
  return pPotential;
}

const CGUIControl* CGUIControlGroup::GetControl(int iControl) const
{
  const CGUIControl *pPotential = NULL;
  LookupMap::const_iterator first = m_lookup.find(iControl);
  if (first != m_lookup.end())
  {
    LookupMap::const_iterator last = m_lookup.upper_bound(iControl);
    for (LookupMap::const_iterator i = first; i != last; ++i)
    {
      const CGUIControl *control = i->second;
      if (control->IsVisible())
        return control;
      else if (!pPotential)
        pPotential = control;
    }
  }
  return pPotential;
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
    pair<LookupMap::const_iterator, LookupMap::const_iterator> range = m_lookup.equal_range(m_focusedControl);
    for (LookupMap::const_iterator i = range.first; i != range.second; ++i)
    {
      if (i->second->HasFocus())
        return i->second;
    }
  }

  // if lookup didn't find focused control, iterate m_children to find it
  for (ciControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    const CGUIControl* control = *it;
    // Avoid calling HasFocus() on control group as it will (possibly) recursively
    // traverse entire group tree just to check if there is focused control.
    // We are recursively traversing it here so no point in doing it twice.
    if (control->IsGroup())
    {
      CGUIControl* focusedControl = ((CGUIControlGroup *)control)->GetFocusedControl();
      if (focusedControl)
        return (CGUIControl *)focusedControl;
    }
    else if (control->HasFocus())
      return (CGUIControl *)control;
  }
  return NULL;
}

// in the case of id == 0, we don't match id
CGUIControl *CGUIControlGroup::GetFirstFocusableControl(int id)
{
  if (!CanFocus()) return NULL;
  if (id && id == (int) GetID()) return this; // we're focusable and they want us
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl* pControl = *it;
    if (pControl->IsGroup())
    {
      CGUIControlGroup *group = (CGUIControlGroup *)pControl;
      CGUIControl *control = group->GetFirstFocusableControl(id);
      if (control) return control;
    }
    if ((!id || (int) pControl->GetID() == id) && pControl->CanFocus())
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
  control->SetPushUpdates(m_pushedUpdates);
  AddLookup(control);
  SetInvalid();
}

void CGUIControlGroup::AddLookup(CGUIControl *control)
{
  if (control->IsGroup())
  { // first add all the subitems of this group (if they exist)
    const LookupMap map = ((CGUIControlGroup *)control)->GetLookup();
    for (LookupMap::const_iterator i = map.begin(); i != map.end(); ++i)
      m_lookup.insert(m_lookup.upper_bound(i->first), make_pair(i->first, i->second));
  }
  if (control->GetID())
    m_lookup.insert(m_lookup.upper_bound(control->GetID()), make_pair(control->GetID(), control));
  // ensure that our size is what it should be
  if (m_parentControl)
    ((CGUIControlGroup *)m_parentControl)->AddLookup(control);
}

void CGUIControlGroup::RemoveLookup(CGUIControl *control)
{
  if (control->IsGroup())
  { // remove the group's lookup
    const LookupMap &map = ((CGUIControlGroup *)control)->GetLookup();
    for (LookupMap::const_iterator i = map.begin(); i != map.end(); ++i)
    { // remove this control
      for (LookupMap::iterator it = m_lookup.begin(); it != m_lookup.end(); ++it)
      {
        if (i->second == it->second)
        {
          m_lookup.erase(it);
          break;
        }
      }
    }
  }
  // remove the actual control
  if (control->GetID())
  {
    for (LookupMap::iterator it = m_lookup.begin(); it != m_lookup.end(); ++it)
    {
      if (control == it->second)
      {
        m_lookup.erase(it);
        break;
      }
    }
  }
  if (m_parentControl)
    ((CGUIControlGroup *)m_parentControl)->RemoveLookup(control);
}

bool CGUIControlGroup::IsValidControl(const CGUIControl *control) const
{
  if (control->GetID())
  {
    for (LookupMap::const_iterator it = m_lookup.begin(); it != m_lookup.end(); ++it)
    {
      if (control == it->second)
        return true;
    }
  }
  return false;
}

bool CGUIControlGroup::InsertControl(CGUIControl *control, const CGUIControl *insertPoint)
{
  // find our position
  for (unsigned int i = 0; i < m_children.size(); i++)
  {
    CGUIControl *child = m_children[i];
    if (child->IsGroup() && ((CGUIControlGroup *)child)->InsertControl(control, insertPoint))
      return true;
    else if (child == insertPoint)
    {
      AddControl(control, i);
      return true;
    }
  }
  return false;
}

void CGUIControlGroup::SaveStates(vector<CControlState> &states)
{
  // save our state, and that of our children
  states.push_back(CControlState(GetID(), m_focusedControl));
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
    (*it)->SaveStates(states);
}

// Note: This routine doesn't delete the control.  It just removes it from the control list
bool CGUIControlGroup::RemoveControl(const CGUIControl *control)
{
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *child = *it;
    if (child->IsGroup() && ((CGUIControlGroup *)child)->RemoveControl(control))
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
  if (m_parentControl)
  {
    for (iControls it = m_children.begin(); it != m_children.end(); ++it)
      ((CGUIControlGroup *)m_parentControl)->RemoveLookup(*it);
  }
  // and delete all our children
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *control = *it;
    delete control;
  }
  m_focusedControl = 0;
  m_children.clear();
  m_lookup.clear();
  SetInvalid();
}

void CGUIControlGroup::GetContainers(vector<CGUIControl *> &containers) const
{
  for (ciControls it = m_children.begin();it != m_children.end(); ++it)
  {
    if ((*it)->IsContainer())
      containers.push_back(*it);
    else if ((*it)->IsGroup())
      ((CGUIControlGroup *)(*it))->GetContainers(containers);
  }
}

#ifdef _DEBUG
void CGUIControlGroup::DumpTextureUse()
{
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
    (*it)->DumpTextureUse();
}
#endif
