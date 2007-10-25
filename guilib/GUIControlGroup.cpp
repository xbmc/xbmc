#include "include.h"
#include "GUIControlGroup.h"

CGUIControlGroup::CGUIControlGroup(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height)
: CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
{
  m_defaultControl = 0;
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
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *control = *it;
    control->UpdateVisibility();
    control->DoRender(m_renderTime);
  }
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
      else
        SendWindowMessage(message);
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

  // else, see if a child matches, and send to the child control if so
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
  const CGUIControl* pPotential=NULL;
  for (ciControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    const CGUIControl* pControl = *it;
    if (pControl->IsGroup())
    {
      CGUIControlGroup *group = (CGUIControlGroup *)pControl;
      const CGUIControl *control = group->GetControl(iControl);
      if (control) pControl = control;
    }
    if (pControl->GetID() == iControl) 
    {
      if (pControl->IsVisible())
        return pControl;
      else if (!pPotential)
        pPotential = pControl;
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
  if (id && id == GetID()) return this; // we're focusable and they want us
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl* pControl = *it;
    if (pControl->IsGroup())
    {
      CGUIControlGroup *group = (CGUIControlGroup *)pControl;
      CGUIControl *control = group->GetFirstFocusableControl(id);
      if (control) return control;
    }
    if ((!id || pControl->GetID() == id) && pControl->CanFocus())
      return pControl;
  }
  return NULL;
}

void CGUIControlGroup::AddControl(CGUIControl *control)
{
  if (!control) return;
  m_children.push_back(control);
  control->SetParentControl(this);
}

void CGUIControlGroup::SaveStates(vector<CControlState> &states)
{
  // save our state, and that of our children
  states.push_back(CControlState(GetID(), m_focusedControl));
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
    (*it)->SaveStates(states);
}

// Note: This routine doesn't delete the control.  It just removes it from the control list
bool CGUIControlGroup::RemoveControl(int id)
{
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *control = *it;
    if (control->IsGroup())
    {
      CGUIControlGroup *group = (CGUIControlGroup *)control;
      if (group->RemoveControl(id))
        return true;
    }
    if (control->GetID() == id)
    {
      m_children.erase(it);
      return true;
    }
  }
  return false;
}

void CGUIControlGroup::ClearAll()
{
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *control = *it;
    delete control;
  }
  m_children.clear();
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
  CLog::Log(LOGDEBUG, __FUNCTION__" for controlgroup %i", GetID());
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    (*it)->DumpTextureUse();
  }
}
#endif