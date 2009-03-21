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
#include "GUIControlGroup.h"

using namespace std;

CGUIControlGroup::CGUIControlGroup()
{
  m_defaultControl = 0;
  m_focusedControl = 0;
  m_renderTime = 0;
  m_renderFocusedLast = false;
  ControlType = GUICONTROL_GROUP;
}

CGUIControlGroup::CGUIControlGroup(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height)
: CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
{
  m_defaultControl = 0;
  m_focusedControl = 0;
  m_renderTime = 0;
  m_renderFocusedLast = false;
  ControlType = GUICONTROL_GROUP;
}

CGUIControlGroup::CGUIControlGroup(const CGUIControlGroup &from)
: CGUIControl(from)
{
  m_defaultControl = from.m_defaultControl;
  m_renderFocusedLast = from.m_renderFocusedLast;

  // run through and add our controls
  for (ciControls it = from.m_children.begin(); it != from.m_children.end(); ++it)
    AddControl((*it)->Clone());

  // defaults
  m_focusedControl = 0;
  m_renderTime = 0;
  ControlType = GUICONTROL_GROUP;
}

CGUIControlGroup::~CGUIControlGroup(void)
{
  ClearAll();
}

void CGUIControlGroup::PreAllocResources()
{
  CGUIControl::PreAllocResources();
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *control = *it;
    if (!control->IsDynamicallyAllocated())
      control->PreAllocResources();
  }
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

void CGUIControlGroup::FreeResources()
{
  CGUIControl::FreeResources();
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *control = *it;
    control->FreeResources();
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

void CGUIControlGroup::Render()
{
  g_graphicsContext.SetOrigin(m_posX, m_posY);
  CGUIControl *focusedControl = NULL;
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *control = *it;
    control->UpdateVisibility();
    if (m_renderFocusedLast && control->HasFocus())
      focusedControl = control;
    else
      control->DoRender(m_renderTime);
  }
  if (focusedControl)
    focusedControl->DoRender(m_renderTime);
  CGUIControl::Render();
  g_graphicsContext.RestoreOrigin();
}

bool CGUIControlGroup::OnAction(const CAction &action)
{
  ASSERT(false);  // unimplemented
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
      if (m_focusedControl)
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

void CGUIControlGroup::DoRender(DWORD currentTime)
{
  m_renderTime = currentTime;
  CGUIControl::DoRender(currentTime);
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

bool CGUIControlGroup::HitTest(const CPoint &point) const
{
  for (ciControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *child = *it;
    if (child->HitTest(point - CPoint(m_posX, m_posX)))
      return true;
  }
  return false;
}

bool CGUIControlGroup::CanFocusFromPoint(const CPoint &point, CGUIControl **control, CPoint &controlPoint) const
{
  if (!CGUIControl::CanFocus()) return false;
  CPoint controlCoords(point);
  m_transform.InverseTransformPosition(controlCoords.x, controlCoords.y);
  for (crControls it = m_children.rbegin(); it != m_children.rend(); ++it)
  {
    CGUIControl *child = *it;
    if (child->CanFocusFromPoint(controlCoords - CPoint(m_posX, m_posY), control, controlPoint))
      return true;
  }
  *control = NULL;
  return false;
}

void CGUIControlGroup::UnfocusFromPoint(const CPoint &point)
{
  CPoint controlCoords(point);
  m_transform.InverseTransformPosition(controlCoords.x, controlCoords.y);
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *child = *it;
    child->UnfocusFromPoint(controlCoords - CPoint(m_posX, m_posY));
  }
  CGUIControl::UnfocusFromPoint(point);
}

bool CGUIControlGroup::HasID(DWORD dwID) const
{
  if (CGUIControl::HasID(dwID)) return true;
  for (ciControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *child = *it;
    if (child->HasID(dwID))
      return true;
  }
  return false;
}

bool CGUIControlGroup::HasVisibleID(DWORD dwID) const
{
  // call base class first as the group may be the requested control
  if (CGUIControl::HasVisibleID(dwID)) return true;
  // if the group isn't visible, then none of it's children can be
  if (!IsVisible()) return false;
  for (ciControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *child = *it;
    if (child->HasVisibleID(dwID))
      return true;
  }
  return false;
}

const CGUIControl* CGUIControlGroup::GetControl(int iControl) const
{
  CGUIControl *pPotential = NULL;
  LookupMap::const_iterator first = m_lookup.find(iControl);
  if (first != m_lookup.end())
  {
    LookupMap::const_iterator last = m_lookup.upper_bound(iControl);
    for (LookupMap::const_iterator i = first; i != last; i++)
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

int CGUIControlGroup::GetFocusedControlID() const
{
  if (m_focusedControl) return m_focusedControl;
  CGUIControl *control = GetFocusedControl();
  if (control) return control->GetID();
  return 0;
}

CGUIControl *CGUIControlGroup::GetFocusedControl() const
{
  for (ciControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    const CGUIControl* control = *it;
    if (control->HasFocus())
    {
      if (control->IsGroup())
      {
        CGUIControlGroup *group = (CGUIControlGroup *)control;
        return group->GetFocusedControl();
      }
      return (CGUIControl *)control;
    }
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
  AddLookup(control);
}

void CGUIControlGroup::AddLookup(CGUIControl *control)
{
  if (control->IsGroup())
  { // first add all the subitems of this group (if they exist)
    const LookupMap map = ((CGUIControlGroup *)control)->GetLookup();
    for (LookupMap::const_iterator i = map.begin(); i != map.end(); i++)
      m_lookup.insert(m_lookup.upper_bound(i->first), make_pair(i->first, i->second));
  }
  m_lookup.insert(m_lookup.upper_bound(control->GetID()), make_pair(control->GetID(), control));
  // ensure that our size is what it should be
  if (m_parentControl)
    ((CGUIControlGroup *)m_parentControl)->AddLookup(control);
}

void CGUIControlGroup::RemoveLookup(CGUIControl *control)
{
  if (control->IsGroup())
  { // remove the group's lookup
    const LookupMap map = ((CGUIControlGroup *)control)->GetLookup();
    for (LookupMap::const_iterator i = map.begin(); i != map.end(); i++)
    { // remove this control
      for (LookupMap::iterator it = m_lookup.begin(); it != m_lookup.end(); it++)
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
  for (LookupMap::iterator it = m_lookup.begin(); it != m_lookup.end(); it++)
  {
    if (control == it->second)
    {
      m_lookup.erase(it);
      break;
    }
  }
  if (m_parentControl)
    ((CGUIControlGroup *)m_parentControl)->RemoveLookup(control);
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
    for (iControls it = m_children.begin(); it != m_children.end(); it++)
      ((CGUIControlGroup *)m_parentControl)->RemoveLookup(*it);
  }
  // and delete all our children
  for (iControls it = m_children.begin(); it != m_children.end(); it++)
  {
    CGUIControl *control = *it;
    delete control;
  }
  m_children.clear();
  m_lookup.clear();
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
