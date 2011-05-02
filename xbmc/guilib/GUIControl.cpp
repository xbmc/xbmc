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

#include "GUIControl.h"

#include "GUIInfoManager.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIWindowManager.h"
#include "GUIControlProfiler.h"
#include "input/MouseStat.h"
#include "Key.h"

using namespace std;

CGUIControl::CGUIControl()
{
  m_hasRendered = false;
  m_bHasFocus = false;
  m_controlID = 0;
  m_parentID = 0;
  m_visible = VISIBLE;
  m_visibleFromSkinCondition = true;
  m_forceHidden = false;
  m_visibleCondition = 0;
  m_enableCondition = 0;
  m_enabled = true;
  m_diffuseColor = 0xffffffff;
  m_posX = 0;
  m_posY = 0;
  m_width = 0;
  m_height = 0;
  m_controlLeft = 0;
  m_controlRight = 0;
  m_controlUp = 0;
  m_controlDown = 0;
  m_controlNext = 0;
  m_controlPrev = 0;
  ControlType = GUICONTROL_UNKNOWN;
  m_bInvalidated = true;
  m_bAllocated=false;
  m_parentControl = NULL;
  m_hasCamera = false;
  m_pushedUpdates = false;
  m_pulseOnSelect = false;
}

CGUIControl::CGUIControl(int parentID, int controlID, float posX, float posY, float width, float height)
: m_hitRect(posX, posY, posX + width, posY + height)
{
  m_posX = posX;
  m_posY = posY;
  m_width = width;
  m_height = height;
  m_bHasFocus = false;
  m_controlID = controlID;
  m_parentID = parentID;
  m_visible = VISIBLE;
  m_visibleFromSkinCondition = true;
  m_diffuseColor = 0xffffffff;
  m_forceHidden = false;
  m_visibleCondition = 0;
  m_enableCondition = 0;
  m_enabled = true;
  m_controlLeft = 0;
  m_controlRight = 0;
  m_controlUp = 0;
  m_controlDown = 0;
  m_controlNext = 0;
  m_controlPrev = 0;
  ControlType = GUICONTROL_UNKNOWN;
  m_bInvalidated = true;
  m_bAllocated=false;
  m_hasRendered = false;
  m_parentControl = NULL;
  m_hasCamera = false;
  m_pushedUpdates = false;
  m_pulseOnSelect = false;
}


CGUIControl::~CGUIControl(void)
{

}

void CGUIControl::AllocResources()
{
  m_hasRendered = false;
  m_bInvalidated = true;
  m_bAllocated=true;
}

void CGUIControl::FreeResources(bool immediately)
{
  if (m_bAllocated)
  {
    // Reset our animation states - not conditional anims though.
    // I'm not sure if this is needed for most cases anyway.  I believe it's only here
    // because some windows aren't loaded on demand
    for (unsigned int i = 0; i < m_animations.size(); i++)
    {
      CAnimation &anim = m_animations[i];
      if (anim.GetType() != ANIM_TYPE_CONDITIONAL)
        anim.ResetAnimation();
    }
    m_bAllocated=false;
  }
  m_hasRendered = false;
}

void CGUIControl::DynamicResourceAlloc(bool bOnOff)
{

}

// the main render routine.
// 1. animate and set the animation transform
// 2. if visible, paint
// 3. reset the animation transform
void CGUIControl::DoRender(unsigned int currentTime)
{
  Animate(currentTime);
  if (m_hasCamera)
    g_graphicsContext.SetCameraPosition(m_camera);
  if (IsVisible())
  {
    GUIPROFILER_RENDER_BEGIN(this);
    Render();
    GUIPROFILER_RENDER_END(this);
  }
  if (m_hasCamera)
    g_graphicsContext.RestoreCameraPosition();
  g_graphicsContext.RemoveTransform();
}

void CGUIControl::Render()
{
  m_bInvalidated = false;
  m_hasRendered = true;
}

bool CGUIControl::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
  case ACTION_MOVE_DOWN:
    if (!HasFocus()) return false;
    OnDown();
    return true;
    break;

  case ACTION_MOVE_UP:
    if (!HasFocus()) return false;
    OnUp();
    return true;
    break;

  case ACTION_MOVE_LEFT:
    if (!HasFocus()) return false;
    OnLeft();
    return true;
    break;

  case ACTION_MOVE_RIGHT:
    if (!HasFocus()) return false;
    OnRight();
    return true;
    break;

  case ACTION_NEXT_CONTROL:
      if (!HasFocus()) return false;
      OnNextControl();
      return true;
      break;

  case ACTION_PREV_CONTROL:
      if (!HasFocus()) return false;
      OnPrevControl();
      return true;
      break;
  }
  return false;
}

// Movement controls (derived classes can override)
void CGUIControl::OnUp()
{
  if (HasFocus())
  {
    if (m_upActions.size())
      ExecuteActions(m_upActions);
    else if (m_controlID != m_controlUp)
    {
      // Send a message to the window with the sender set as the window
      CGUIMessage msg(GUI_MSG_MOVE, GetParentID(), GetID(), ACTION_MOVE_UP);
      SendWindowMessage(msg);
    }
  }
}

void CGUIControl::OnDown()
{
  if (HasFocus())
  {
    if (m_downActions.size())
      ExecuteActions(m_downActions);
    else if (m_controlID != m_controlDown)
    {
      // Send a message to the window with the sender set as the window
      CGUIMessage msg(GUI_MSG_MOVE, GetParentID(), GetID(), ACTION_MOVE_DOWN);
      SendWindowMessage(msg);
    }
  }
}

void CGUIControl::OnLeft()
{
  if (HasFocus())
  {
    if (m_leftActions.size())
      ExecuteActions(m_leftActions);
    else if (m_controlID != m_controlLeft)
    {
      // Send a message to the window with the sender set as the window
      CGUIMessage msg(GUI_MSG_MOVE, GetParentID(), GetID(), ACTION_MOVE_LEFT);
      SendWindowMessage(msg);
    }
  }
}

void CGUIControl::OnRight()
{
  if (HasFocus())
  {
    if (m_rightActions.size())
      ExecuteActions(m_rightActions);
    else if (m_controlID != m_controlRight)
    {
      // Send a message to the window with the sender set as the window
      CGUIMessage msg(GUI_MSG_MOVE, GetParentID(), GetID(), ACTION_MOVE_RIGHT);
      SendWindowMessage(msg);
    }
  }
}

void CGUIControl::OnNextControl()
{
  if (m_controlID != m_controlNext)
  {
    // Send a message to the window with the sender set as the window
    CGUIMessage msg(GUI_MSG_MOVE, GetParentID(), GetID(), ACTION_NEXT_CONTROL, m_controlNext);
    SendWindowMessage(msg);
  }
}

void CGUIControl::OnPrevControl()
{
  if (m_controlID != m_controlPrev)
  {
    // Send a message to the window with the sender set as the window
    CGUIMessage msg(GUI_MSG_MOVE, GetParentID(), GetID(), ACTION_PREV_CONTROL, m_controlPrev);
    SendWindowMessage(msg);
  }
}

bool CGUIControl::SendWindowMessage(CGUIMessage &message)
{
  CGUIWindow *pWindow = g_windowManager.GetWindow(GetParentID());
  if (pWindow)
    return pWindow->OnMessage(message);
  return g_windowManager.SendMessage(message);
}

int CGUIControl::GetID(void) const
{
  return m_controlID;
}


int CGUIControl::GetParentID(void) const
{
  return m_parentID;
}

bool CGUIControl::HasFocus(void) const
{
  return m_bHasFocus;
}

void CGUIControl::SetFocus(bool focus)
{
  if (m_bHasFocus && !focus)
    QueueAnimation(ANIM_TYPE_UNFOCUS);
  else if (!m_bHasFocus && focus)
    QueueAnimation(ANIM_TYPE_FOCUS);
  m_bHasFocus = focus;
}

bool CGUIControl::OnMessage(CGUIMessage& message)
{
  if ( message.GetControlId() == GetID() )
  {
    switch (message.GetMessage() )
    {
    case GUI_MSG_SETFOCUS:
      // if control is disabled then move 2 the next control
      if ( !CanFocus() )
      {
        CLog::Log(LOGERROR, "Control %u in window %u has been asked to focus, "
                            "but it can't",
                  GetID(), GetParentID());
        return false;
      }
      SetFocus(true);
      {
        // inform our parent window that this has happened
        CGUIMessage message(GUI_MSG_FOCUSED, GetParentID(), GetID());
        if (m_parentControl)
          m_parentControl->OnMessage(message);
      }
      return true;
      break;

    case GUI_MSG_LOSTFOCUS:
      {
        SetFocus(false);
        // and tell our parent so it can unfocus
        if (m_parentControl)
          m_parentControl->OnMessage(message);
        return true;
      }
      break;

    case GUI_MSG_VISIBLE:
      if (m_visibleCondition)
        m_visible = g_infoManager.GetBool(m_visibleCondition, m_parentID) ? VISIBLE : HIDDEN;
      else
        m_visible = VISIBLE;
      m_forceHidden = false;
      return true;
      break;

    case GUI_MSG_HIDDEN:
      m_forceHidden = true;
      // reset any visible animations that are in process
      if (IsAnimating(ANIM_TYPE_VISIBLE))
      {
//        CLog::Log(LOGDEBUG, "Resetting visible animation on control %i (we are %s)", m_controlID, m_visible ? "visible" : "hidden");
        CAnimation *visibleAnim = GetAnimation(ANIM_TYPE_VISIBLE);
        if (visibleAnim) visibleAnim->ResetAnimation();
      }
      return true;

      // Note that the skin <enable> tag will override these messages
    case GUI_MSG_ENABLED:
      SetEnabled(true);
      return true;

    case GUI_MSG_DISABLED:
      SetEnabled(false);
      return true;
    }
  }
  return false;
}

bool CGUIControl::CanFocus() const
{
  if (!IsVisible() && !m_allowHiddenFocus) return false;
  if (IsDisabled()) return false;
  return true;
}

bool CGUIControl::IsVisible() const
{
  if (m_forceHidden) return false;
  return m_visible == VISIBLE;
}

bool CGUIControl::IsDisabled() const
{
  return !m_enabled;
}

void CGUIControl::SetEnabled(bool bEnable)
{
  m_enabled = bEnable;
}

void CGUIControl::SetEnableCondition(int condition)
{
  m_enableCondition = condition;
}

void CGUIControl::SetPosition(float posX, float posY)
{
  if ((m_posX != posX) || (m_posY != posY))
  {
    m_hitRect += CPoint(posX - m_posX, posY - m_posY);
    m_posX = posX;
    m_posY = posY;
    SetInvalid();
  }
}

void CGUIControl::SetColorDiffuse(const CGUIInfoColor &color)
{
  m_diffuseColor = color;
}

float CGUIControl::GetXPosition() const
{
  return m_posX;
}

float CGUIControl::GetYPosition() const
{
  return m_posY;
}

float CGUIControl::GetWidth() const
{
  return m_width;
}

float CGUIControl::GetHeight() const
{
  return m_height;
}

void CGUIControl::MarkDirtyRegion()
{
  m_markedLocalRegion.Union(GetRenderRegion());
}

void CGUIControl::SendFinalDirtyRegionToParent(const CRect &dirtyRegion, const CGUIControl *sender)
{
  if (m_parentControl)
    m_parentControl->SendFinalDirtyRegionToParent(dirtyRegion, sender);
  else
    g_windowManager.MarkDirtyRegion(dirtyRegion);
}

void CGUIControl::FlushDirtyRegion()
{
  if (!m_markedLocalRegion.IsEmpty())
  {
    g_graphicsContext.AddTransform(m_transform);
    if (m_hasCamera)
      g_graphicsContext.SetCameraPosition(m_camera);

    CRect AABB = g_graphicsContext.generateAABB(m_markedLocalRegion);

    if (m_hasCamera)
      g_graphicsContext.RestoreCameraPosition();
    g_graphicsContext.RemoveTransform();

    // When we have transformed to screen cordinates we send it to
    // the parent which may choose to ignore it or send it further down.
    SendFinalDirtyRegionToParent(AABB, this);
  }

  m_markedLocalRegion = CRect();
}

CRect CGUIControl::GetRenderRegion()
{
  CPoint tl(GetXPosition(), GetYPosition());
  CPoint br(tl.x + GetWidth(), tl.y + GetHeight());

  return CRect(tl.x, tl.y, br.x, br.y);
}

void CGUIControl::SetNavigation(int up, int down, int left, int right)
{
  m_controlUp = up;
  m_controlDown = down;
  m_controlLeft = left;
  m_controlRight = right;
}

void CGUIControl::SetTabNavigation(int next, int prev)
{
  m_controlNext = next;
  m_controlPrev = prev;
}

void CGUIControl::SetNavigationActions(const vector<CGUIActionDescriptor> &up, const vector<CGUIActionDescriptor> &down,
                                       const vector<CGUIActionDescriptor> &left, const vector<CGUIActionDescriptor> &right, bool replace)
{
  if (m_leftActions.empty()  || replace) m_leftActions  = left;
  if (m_rightActions.empty() || replace) m_rightActions = right;
  if (m_upActions.empty()    || replace) m_upActions    = up;
  if (m_downActions.empty()  || replace) m_downActions  = down;
}

void CGUIControl::SetWidth(float width)
{
  if (m_width != width)
  {
    m_width = width;
    m_hitRect.x2 = m_hitRect.x1 + width;
    SetInvalid();
  }
}

void CGUIControl::SetHeight(float height)
{
  if (m_height != height)
  {
    m_height = height;
    m_hitRect.y2 = m_hitRect.y1 + height;
    SetInvalid();
  }
}

void CGUIControl::SetVisible(bool bVisible)
{
  // just force to hidden if necessary
  m_forceHidden = !bVisible;
/*
  if (m_visibleCondition)
    bVisible = g_infoManager.GetBool(m_visibleCondition, m_parentID);
  if (m_bVisible != bVisible)
  {
    m_visible = bVisible;
    m_visibleFromSkinCondition = bVisible;
    SetInvalid();
  }*/
}

bool CGUIControl::HitTest(const CPoint &point) const
{
  return m_hitRect.PtInRect(point);
}

EVENT_RESULT CGUIControl::SendMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  CPoint childPoint(point);
  m_transform.InverseTransformPosition(childPoint.x, childPoint.y);
  if (!CanFocusFromPoint(childPoint))
    return EVENT_RESULT_UNHANDLED;

  bool handled = OnMouseOver(childPoint);
  EVENT_RESULT ret = OnMouseEvent(childPoint, event);
  if (ret)
    return ret;
  return (handled && (event.m_id == ACTION_MOUSE_MOVE)) ? EVENT_RESULT_HANDLED : EVENT_RESULT_UNHANDLED;
}

// override this function to implement custom mouse behaviour
bool CGUIControl::OnMouseOver(const CPoint &point)
{
  if (g_Mouse.GetState() != MOUSE_STATE_DRAG)
    g_Mouse.SetState(MOUSE_STATE_FOCUS);
  if (!CanFocus()) return false;
  CGUIMessage msg(GUI_MSG_SETFOCUS, GetParentID(), GetID());
  OnMessage(msg);
  return true;
}

void CGUIControl::UpdateVisibility(const CGUIListItem *item)
{
  if (m_visibleCondition)
  {
    bool bWasVisible = m_visibleFromSkinCondition;
    m_visibleFromSkinCondition = g_infoManager.GetBool(m_visibleCondition, m_parentID, item);
    if (!bWasVisible && m_visibleFromSkinCondition)
    { // automatic change of visibility - queue the in effect
  //    CLog::Log(LOGDEBUG, "Visibility changed to visible for control id %i", m_controlID);
      QueueAnimation(ANIM_TYPE_VISIBLE);
    }
    else if (bWasVisible && !m_visibleFromSkinCondition)
    { // automatic change of visibility - do the out effect
  //    CLog::Log(LOGDEBUG, "Visibility changed to hidden for control id %i", m_controlID);
      QueueAnimation(ANIM_TYPE_HIDDEN);
    }
  }
  // check for conditional animations
  for (unsigned int i = 0; i < m_animations.size(); i++)
  {
    CAnimation &anim = m_animations[i];
    if (anim.GetType() == ANIM_TYPE_CONDITIONAL)
      anim.UpdateCondition(GetParentID(), item);
  }
  // and check for conditional enabling - note this overrides SetEnabled() from the code currently
  // this may need to be reviewed at a later date
  if (m_enableCondition)
    m_enabled = g_infoManager.GetBool(m_enableCondition, m_parentID, item);
  m_allowHiddenFocus.Update(m_parentID, item);
  UpdateColors();
  // and finally, update our control information (if not pushed)
  if (!m_pushedUpdates)
    UpdateInfo(item);
}

void CGUIControl::UpdateColors()
{
  m_diffuseColor.Update();
}

void CGUIControl::SetInitialVisibility()
{
  if (m_visibleCondition)
  {
    m_visibleFromSkinCondition = g_infoManager.GetBool(m_visibleCondition, m_parentID);
    m_visible = m_visibleFromSkinCondition ? VISIBLE : HIDDEN;
  //  CLog::Log(LOGDEBUG, "Set initial visibility for control %i: %s", m_controlID, m_visible == VISIBLE ? "visible" : "hidden");
    // no need to enquire every frame if we are always visible or always hidden
    if (m_visibleCondition == SYSTEM_ALWAYS_TRUE || m_visibleCondition == SYSTEM_ALWAYS_FALSE)
      m_visibleCondition = 0;
  }
  // and handle animation conditions as well
  for (unsigned int i = 0; i < m_animations.size(); i++)
  {
    CAnimation &anim = m_animations[i];
    if (anim.GetType() == ANIM_TYPE_CONDITIONAL)
      anim.SetInitialCondition(GetParentID());
  }
  // and check for conditional enabling - note this overrides SetEnabled() from the code currently
  // this may need to be reviewed at a later date
  if (m_enableCondition)
    m_enabled = g_infoManager.GetBool(m_enableCondition, m_parentID);
  m_allowHiddenFocus.Update(m_parentID);
  UpdateColors();
}

void CGUIControl::SetVisibleCondition(int visible, const CGUIInfoBool &allowHiddenFocus)
{
  m_visibleCondition = visible;
  m_allowHiddenFocus = allowHiddenFocus;
}

void CGUIControl::SetAnimations(const vector<CAnimation> &animations)
{
  m_animations = animations;
}

void CGUIControl::ResetAnimation(ANIMATION_TYPE type)
{
  for (unsigned int i = 0; i < m_animations.size(); i++)
  {
    if (m_animations[i].GetType() == type)
      m_animations[i].ResetAnimation();
  }
}

void CGUIControl::ResetAnimations()
{
  for (unsigned int i = 0; i < m_animations.size(); i++)
    m_animations[i].ResetAnimation();
}

bool CGUIControl::CheckAnimation(ANIMATION_TYPE animType)
{
  // rule out the animations we shouldn't perform
  if (!IsVisible() || !HasRendered())
  { // hidden or never rendered - don't allow exit or entry animations for this control
    if (animType == ANIM_TYPE_WINDOW_CLOSE)
    { // could be animating a (delayed) window open anim, so reset it
      ResetAnimation(ANIM_TYPE_WINDOW_OPEN);
      return false;
    }
  }
  if (!IsVisible())
  { // hidden - only allow hidden anims if we're animating a visible anim
    if (animType == ANIM_TYPE_HIDDEN && !IsAnimating(ANIM_TYPE_VISIBLE))
    {
      // update states to force it hidden
      UpdateStates(animType, ANIM_PROCESS_NORMAL, ANIM_STATE_APPLIED);
      return false;
    }
    if (animType == ANIM_TYPE_WINDOW_OPEN)
      return false;
  }
  return true;
}

void CGUIControl::QueueAnimation(ANIMATION_TYPE animType)
{
  if (!CheckAnimation(animType))
    return;
  CAnimation *reverseAnim = GetAnimation((ANIMATION_TYPE)-animType, false);
  CAnimation *forwardAnim = GetAnimation(animType);
  // we first check whether the reverse animation is in progress (and reverse it)
  // then we check for the normal animation, and queue it
  if (reverseAnim && reverseAnim->IsReversible() && (reverseAnim->GetState() == ANIM_STATE_IN_PROCESS || reverseAnim->GetState() == ANIM_STATE_DELAYED))
  {
    reverseAnim->QueueAnimation(ANIM_PROCESS_REVERSE);
    if (forwardAnim) forwardAnim->ResetAnimation();
  }
  else if (forwardAnim)
  {
    forwardAnim->QueueAnimation(ANIM_PROCESS_NORMAL);
    if (reverseAnim) reverseAnim->ResetAnimation();
  }
  else
  { // hidden and visible animations delay the change of state.  If there is no animations
    // to perform, then we should just change the state straightaway
    if (reverseAnim) reverseAnim->ResetAnimation();
    UpdateStates(animType, ANIM_PROCESS_NORMAL, ANIM_STATE_APPLIED);
  }
}

CAnimation *CGUIControl::GetAnimation(ANIMATION_TYPE type, bool checkConditions /* = true */)
{
  for (unsigned int i = 0; i < m_animations.size(); i++)
  {
    CAnimation &anim = m_animations[i];
    if (anim.GetType() == type)
    {
      if (!checkConditions || !anim.GetCondition() || g_infoManager.GetBool(anim.GetCondition()))
        return &anim;
    }
  }
  return NULL;
}

bool CGUIControl::HasAnimation(ANIMATION_TYPE type)
{
  return (NULL != GetAnimation(type, true));
}

void CGUIControl::UpdateStates(ANIMATION_TYPE type, ANIMATION_PROCESS currentProcess, ANIMATION_STATE currentState)
{
  // Make sure control is hidden or visible at the appropriate times
  // while processing a visible or hidden animation it needs to be visible,
  // but when finished a hidden operation it needs to be hidden
  if (type == ANIM_TYPE_VISIBLE)
  {
    if (currentProcess == ANIM_PROCESS_REVERSE)
    {
      if (currentState == ANIM_STATE_APPLIED)
        m_visible = HIDDEN;
    }
    else if (currentProcess == ANIM_PROCESS_NORMAL)
    {
      if (currentState == ANIM_STATE_DELAYED)
        m_visible = DELAYED;
      else
        m_visible = m_visibleFromSkinCondition ? VISIBLE : HIDDEN;
    }
  }
  else if (type == ANIM_TYPE_HIDDEN)
  {
    if (currentProcess == ANIM_PROCESS_NORMAL)  // a hide animation
    {
      if (currentState == ANIM_STATE_APPLIED)
        m_visible = HIDDEN; // finished
      else
        m_visible = VISIBLE; // have to be visible until we are finished
    }
    else if (currentProcess == ANIM_PROCESS_REVERSE)  // a visible animation
    { // no delay involved here - just make sure it's visible
      m_visible = m_visibleFromSkinCondition ? VISIBLE : HIDDEN;
    }
  }
  else if (type == ANIM_TYPE_WINDOW_OPEN)
  {
    if (currentProcess == ANIM_PROCESS_NORMAL)
    {
      if (currentState == ANIM_STATE_DELAYED)
        m_visible = DELAYED; // delayed
      else
        m_visible = m_visibleFromSkinCondition ? VISIBLE : HIDDEN;
    }
  }
  else if (type == ANIM_TYPE_FOCUS)
  {
    // call the focus function if we have finished a focus animation
    // (buttons can "click" on focus)
    if (currentProcess == ANIM_PROCESS_NORMAL && currentState == ANIM_STATE_APPLIED)
      OnFocus();
  }
  else if (type == ANIM_TYPE_UNFOCUS)
  {
    // call the unfocus function if we have finished a focus animation
    // (buttons can "click" on focus)
    if (currentProcess == ANIM_PROCESS_NORMAL && currentState == ANIM_STATE_APPLIED)
      OnUnFocus();
  }
}

void CGUIControl::Animate(unsigned int currentTime)
{
  // check visible state outside the loop, as it could change
  GUIVISIBLE visible = m_visible;
  m_transform.Reset();
  CPoint center(m_posX + m_width * 0.5f, m_posY + m_height * 0.5f);
  for (unsigned int i = 0; i < m_animations.size(); i++)
  {
    CAnimation &anim = m_animations[i];
    anim.Animate(currentTime, HasRendered() || visible == DELAYED);
    // Update the control states (such as visibility)
    UpdateStates(anim.GetType(), anim.GetProcess(), anim.GetState());
    // and render the animation effect
    anim.RenderAnimation(m_transform, center);

/*    // debug stuff
    if (anim.currentProcess != ANIM_PROCESS_NONE)
    {
      if (anim.effect == EFFECT_TYPE_ZOOM)
      {
        if (IsVisible())
          CLog::Log(LOGDEBUG, "Animating control %d with a %s zoom effect %s. Amount is %2.1f, visible=%s", m_controlID, anim.type == ANIM_TYPE_CONDITIONAL ? (anim.lastCondition ? "conditional_on" : "conditional_off") : (anim.type == ANIM_TYPE_VISIBLE ? "visible" : "hidden"), anim.currentProcess == ANIM_PROCESS_NORMAL ? "normal" : "reverse", anim.amount, IsVisible() ? "true" : "false");
      }
      else if (anim.effect == EFFECT_TYPE_FADE)
      {
        if (IsVisible())
          CLog::Log(LOGDEBUG, "Animating control %d with a %s fade effect %s. Amount is %2.1f. Visible=%s", m_controlID, anim.type == ANIM_TYPE_CONDITIONAL ? (anim.lastCondition ? "conditional_on" : "conditional_off") : (anim.type == ANIM_TYPE_VISIBLE ? "visible" : "hidden"), anim.currentProcess == ANIM_PROCESS_NORMAL ? "normal" : "reverse", anim.amount, IsVisible() ? "true" : "false");
      }
    }*/
  }
  g_graphicsContext.AddTransform(m_transform);
}

bool CGUIControl::IsAnimating(ANIMATION_TYPE animType)
{
  for (unsigned int i = 0; i < m_animations.size(); i++)
  {
    CAnimation &anim = m_animations[i];
    if (anim.GetType() == animType)
    {
      if (anim.GetQueuedProcess() == ANIM_PROCESS_NORMAL)
        return true;
      if (anim.GetProcess() == ANIM_PROCESS_NORMAL)
        return true;
    }
    else if (anim.GetType() == -animType)
    {
      if (anim.GetQueuedProcess() == ANIM_PROCESS_REVERSE)
        return true;
      if (anim.GetProcess() == ANIM_PROCESS_REVERSE)
        return true;
    }
  }
  return false;
}

int CGUIControl::GetNextControl(int direction) const
{
  switch (direction)
  {
  case ACTION_MOVE_UP:
    return m_controlUp;
  case ACTION_MOVE_DOWN:
    return m_controlDown;
  case ACTION_MOVE_LEFT:
    return m_controlLeft;
  case ACTION_MOVE_RIGHT:
    return m_controlRight;
  default:
    return -1;
  }
}

bool CGUIControl::CanFocusFromPoint(const CPoint &point) const
{
  return CanFocus() && HitTest(point);
}

void CGUIControl::UnfocusFromPoint(const CPoint &point)
{
  CPoint controlPoint(point);
  m_transform.InverseTransformPosition(controlPoint.x, controlPoint.y);
  if (!HitTest(controlPoint))
    SetFocus(false);
}

bool CGUIControl::HasID(int id) const
{
  return GetID() == id;
}

bool CGUIControl::HasVisibleID(int id) const
{
  return GetID() == id && IsVisible();
}

void CGUIControl::SaveStates(vector<CControlState> &states)
{
  // empty for now - do nothing with the majority of controls
}

void CGUIControl::SetHitRect(const CRect &rect)
{
  m_hitRect = rect;
}

void CGUIControl::SetCamera(const CPoint &camera)
{
  m_camera = camera;
  m_hasCamera = true;
}

void CGUIControl::ExecuteActions(const vector<CGUIActionDescriptor> &actions)
{
  // we should really save anything we need, as the action may cause the window to close
  int savedID = GetID();
  int savedParent = GetParentID();
  vector<CGUIActionDescriptor> savedActions = actions;

  for (unsigned int i = 0; i < savedActions.size(); i++)
  {
    CGUIMessage message(GUI_MSG_EXECUTE, savedID, savedParent);
    message.SetAction(savedActions[i]);
    g_windowManager.SendMessage(message);
  }
}

CPoint CGUIControl::GetRenderPosition() const
{
  float z = 0;
  CPoint point(GetPosition());
  m_transform.TransformPosition(point.x, point.y, z);
  if (m_parentControl)
    point += m_parentControl->GetRenderPosition();
  return point;
}
