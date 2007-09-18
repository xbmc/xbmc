#include "include.h"
#include "GUIControl.h"

#include "../xbmc/utils/GUIInfoManager.h"
#include "LocalizeStrings.h"
#include "../xbmc/Util.h"
#include "GUIWindowManager.h"

CGUIControl::CGUIControl()
{
  m_hasRendered = false;
  m_bHasFocus = false;
  m_dwControlID = 0;
  m_dwParentID = 0;
  m_visible = VISIBLE;
  m_visibleFromSkinCondition = true;
  m_forceHidden = false;
  m_visibleCondition = 0;
  m_enableCondition = 0;
  m_enabled = true;
  m_diffuseColor = 0xffffffff;
  m_posX = 0;
  m_posY = 0;
  m_dwControlLeft = 0;
  m_dwControlRight = 0;
  m_dwControlUp = 0;
  m_dwControlDown = 0;
  ControlType = GUICONTROL_UNKNOWN;
  m_bInvalidated = true;
  m_bAllocated=false;
  m_parentControl = NULL;
  m_hasCamera = false;
}

CGUIControl::CGUIControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height)
: m_hitRect(posX, posY, posX + width, posY + height)
{
  m_posX = posX;
  m_posY = posY;
  m_width = width;
  m_height = height;
  m_bHasFocus = false;
  m_dwControlID = dwControlId;
  m_dwParentID = dwParentID;
  m_visible = VISIBLE;
  m_visibleFromSkinCondition = true;
  m_diffuseColor = 0xffffffff;
  m_forceHidden = false;
  m_visibleCondition = 0;
  m_enableCondition = 0;
  m_enabled = true;
  m_dwControlLeft = 0;
  m_dwControlRight = 0;
  m_dwControlUp = 0;
  m_dwControlDown = 0;
  ControlType = GUICONTROL_UNKNOWN;
  m_bInvalidated = true;
  m_bAllocated=false;
  m_hasRendered = false;
  m_parentControl = NULL;
  m_hasCamera = false;
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

void CGUIControl::FreeResources()
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

bool CGUIControl::IsAllocated() const
{
  return m_bAllocated;
}

void CGUIControl::DynamicResourceAlloc(bool bOnOff)
{

}

// the main render routine.
// 1. animate and set the animation transform
// 2. if visible, paint
// 3. reset the animation transform
void CGUIControl::DoRender(DWORD currentTime)
{
  Animate(currentTime);
  if (m_hasCamera)
    g_graphicsContext.SetCameraPosition(m_camera);
  if (IsVisible())
    Render();
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
  switch (action.wID)
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
  }
  return false;
}

// Movement controls (derived classes can override)
void CGUIControl::OnUp()
{
  if (HasFocus() && m_dwControlID != m_dwControlUp)
  {
    // Send a message to the window with the sender set as the window
    CGUIMessage msg(GUI_MSG_MOVE, GetParentID(), GetID(), ACTION_MOVE_UP);
    SendWindowMessage(msg);
  }
}

void CGUIControl::OnDown()
{
  if (HasFocus() && m_dwControlID != m_dwControlDown)
  {
    // Send a message to the window with the sender set as the window
    CGUIMessage msg(GUI_MSG_MOVE, GetParentID(), GetID(), ACTION_MOVE_DOWN);
    SendWindowMessage(msg);
  }
}

void CGUIControl::OnLeft()
{
  if (HasFocus() && m_dwControlID != m_dwControlLeft)
  {
    // Send a message to the window with the sender set as the window
    CGUIMessage msg(GUI_MSG_MOVE, GetParentID(), GetID(), ACTION_MOVE_LEFT);
    SendWindowMessage(msg);
  }
}

void CGUIControl::OnRight()
{
  if (HasFocus() && m_dwControlID != m_dwControlRight)
  {
    // Send a message to the window with the sender set as the window
    CGUIMessage msg(GUI_MSG_MOVE, GetParentID(), GetID(), ACTION_MOVE_RIGHT);
    SendWindowMessage(msg);
  }
}

bool CGUIControl::SendWindowMessage(CGUIMessage &message)
{
  CGUIWindow *pWindow = m_gWindowManager.GetWindow(GetParentID());
  if (pWindow)
    return pWindow->OnMessage(message);
  return g_graphicsContext.SendMessage(message);
}

DWORD CGUIControl::GetID(void) const
{
  return m_dwControlID;
}


DWORD CGUIControl::GetParentID(void) const
{
  return m_dwParentID;
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
        CLog::Log(LOGERROR, "Control %d in window %d has been asked to focus, but it can't", GetID(), GetParentID());
        return false;
      }
      SetFocus(true);
      {
        // inform our parent window that this has happened
        CGUIMessage message(GUI_MSG_FOCUSED, GetParentID(), GetID());
        if (m_parentControl)
          m_parentControl->OnMessage(message);
        else
        SendWindowMessage(message);
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
        m_visible = g_infoManager.GetBool(m_visibleCondition, m_dwParentID) ? VISIBLE : HIDDEN;
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
//        CLog::DebugLog("Resetting visible animation on control %i (we are %s)", m_dwControlID, m_visible ? "visible" : "hidden");
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
    Update();
  }
}

void CGUIControl::SetColorDiffuse(D3DCOLOR color)
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

void CGUIControl::SetNavigation(DWORD dwUp, DWORD dwDown, DWORD dwLeft, DWORD dwRight)
{
  m_dwControlUp = dwUp;
  m_dwControlDown = dwDown;
  m_dwControlLeft = dwLeft;
  m_dwControlRight = dwRight;
}

void CGUIControl::SetWidth(float width)
{
  if (m_width != width)
  {
    m_width = width;
    m_hitRect.x2 = m_hitRect.x1 + width;
    Update();
  }
}

void CGUIControl::SetHeight(float height)
{
  if (m_height != height)
  {
    m_height = height;
    m_hitRect.y2 = m_hitRect.y1 + height;
    Update();
  }
}

void CGUIControl::SetVisible(bool bVisible)
{
  // just force to hidden if necessary
  m_forceHidden = !bVisible;
/*
  if (m_visibleCondition)
    bVisible = g_infoManager.GetBool(m_visibleCondition, m_dwParentID);
  if (m_bVisible != bVisible)
  {
    m_visible = bVisible;
    m_visibleFromSkinCondition = bVisible;
    m_bInvalidated = true;
  }*/
}

bool CGUIControl::HitTest(const CPoint &point) const
{
  return m_hitRect.PtInRect(point);
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

void CGUIControl::UpdateVisibility()
{
  if (m_visibleCondition)
  {
    bool bWasVisible = m_visibleFromSkinCondition;
    m_visibleFromSkinCondition = g_infoManager.GetBool(m_visibleCondition, m_dwParentID);
    if (!bWasVisible && m_visibleFromSkinCondition)
    { // automatic change of visibility - queue the in effect
  //    CLog::DebugLog("Visibility changed to visible for control id %i", m_dwControlID);
      QueueAnimation(ANIM_TYPE_VISIBLE);
    }
    else if (bWasVisible && !m_visibleFromSkinCondition)
    { // automatic change of visibility - do the out effect
  //    CLog::DebugLog("Visibility changed to hidden for control id %i", m_dwControlID);
      QueueAnimation(ANIM_TYPE_HIDDEN);
    }
  }
  // check for conditional animations
  for (unsigned int i = 0; i < m_animations.size(); i++)
  {
    CAnimation &anim = m_animations[i];
    if (anim.GetType() == ANIM_TYPE_CONDITIONAL)
      anim.UpdateCondition(GetParentID());
  }
  // and check for conditional enabling - note this overrides SetEnabled() from the code currently
  // this may need to be reviewed at a later date
  if (m_enableCondition)
    m_enabled = g_infoManager.GetBool(m_enableCondition, m_dwParentID);
}

void CGUIControl::SetInitialVisibility()
{
  if (m_visibleCondition)
  {
    m_visibleFromSkinCondition = g_infoManager.GetBool(m_visibleCondition, m_dwParentID);
    m_visible = m_visibleFromSkinCondition ? VISIBLE : HIDDEN;
  //  CLog::DebugLog("Set initial visibility for control %i: %s", m_dwControlID, m_visible == VISIBLE ? "visible" : "hidden");
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
}

void CGUIControl::SetVisibleCondition(int visible, bool allowHiddenFocus)
{
  m_visibleCondition = visible;
  m_allowHiddenFocus = allowHiddenFocus;
}

void CGUIControl::SetAnimations(const vector<CAnimation> &animations)
{
  m_animations = animations;
}

void CGUIControl::QueueAnimation(ANIMATION_TYPE animType)
{
  // rule out the animations we shouldn't perform
  if (!IsVisible() || !HasRendered()) 
  { // hidden or never rendered - don't allow exit or entry animations for this control
    if (animType == ANIM_TYPE_WINDOW_CLOSE && !IsAnimating(ANIM_TYPE_WINDOW_OPEN))
      return;
  }
  if (!IsVisible())
  { // hidden - only allow hidden anims if we're animating a visible anim
    if (animType == ANIM_TYPE_HIDDEN && !IsAnimating(ANIM_TYPE_VISIBLE))
    {
      // update states to force it hidden
      UpdateStates(animType, ANIM_PROCESS_NORMAL, ANIM_STATE_APPLIED);
      return;
    }
    if (animType == ANIM_TYPE_WINDOW_OPEN)
      return;
  }
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
    if (m_animations[i].GetType() == type)
    {
      if (!checkConditions || !m_animations[i].GetCondition() || g_infoManager.GetBool(m_animations[i].GetCondition()))
        return &m_animations[i];
    }
  }
  return NULL;
}

void CGUIControl::UpdateStates(ANIMATION_TYPE type, ANIMATION_PROCESS currentProcess, ANIMATION_STATE currentState)
{
  GUIVISIBLE visible = m_visible;
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
//  if (visible != m_visible)
//    CLog::DebugLog("UpdateControlState of control id %i - now %s (type=%d, process=%d, state=%d)", m_dwControlID, m_visible == VISIBLE ? "visible" : (m_visible == DELAYED ? "delayed" : "hidden"), type, currentProcess, currentState);
}

void CGUIControl::Animate(DWORD currentTime)
{
  // check visible state outside the loop, as it could change
  GUIVISIBLE visible = m_visible;
  m_transform.Reset();
  for (unsigned int i = 0; i < m_animations.size(); i++)
  {
    CAnimation &anim = m_animations[i];
    anim.Animate(currentTime, HasRendered() || visible == DELAYED);
    // Update the control states (such as visibility)
    UpdateStates(anim.GetType(), anim.GetProcess(), anim.GetState());
    // and render the animation effect
    anim.RenderAnimation(m_transform);

/*    // debug stuff
    if (anim.currentProcess != ANIM_PROCESS_NONE)
    {
      if (anim.effect == EFFECT_TYPE_ZOOM)
      {
        if (IsVisible())
          CLog::DebugLog("Animating control %d with a %s zoom effect %s. Amount is %2.1f, visible=%s", m_dwControlID, anim.type == ANIM_TYPE_CONDITIONAL ? (anim.lastCondition ? "conditional_on" : "conditional_off") : (anim.type == ANIM_TYPE_VISIBLE ? "visible" : "hidden"), anim.currentProcess == ANIM_PROCESS_NORMAL ? "normal" : "reverse", anim.amount, IsVisible() ? "true" : "false");
      }
      else if (anim.effect == EFFECT_TYPE_FADE)
      {
        if (IsVisible())
          CLog::DebugLog("Animating control %d with a %s fade effect %s. Amount is %2.1f. Visible=%s", m_dwControlID, anim.type == ANIM_TYPE_CONDITIONAL ? (anim.lastCondition ? "conditional_on" : "conditional_off") : (anim.type == ANIM_TYPE_VISIBLE ? "visible" : "hidden"), anim.currentProcess == ANIM_PROCESS_NORMAL ? "normal" : "reverse", anim.amount, IsVisible() ? "true" : "false");
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

DWORD CGUIControl::GetNextControl(int direction) const
{
  switch (direction)
  {
  case ACTION_MOVE_UP:
    return m_dwControlUp;
  case ACTION_MOVE_DOWN:
    return m_dwControlDown;
  case ACTION_MOVE_LEFT:
    return m_dwControlLeft;
  case ACTION_MOVE_RIGHT:
    return m_dwControlRight;
  default:
    return -1;
  }
}

// input the point with respect to this control to hit, and return
// the control and the point with respect to his control if we have a hit
bool CGUIControl::CanFocusFromPoint(const CPoint &point, CGUIControl **control, CPoint &controlPoint) const
{
  controlPoint = point;
  m_transform.InverseTransformPosition(controlPoint.x, controlPoint.y);
  if (CanFocus() && HitTest(controlPoint))
  {
    *control = (CGUIControl *)this;
    return true;
  }
  *control = NULL;
  return false;
}

void CGUIControl::UnfocusFromPoint(const CPoint &point)
{
  CPoint controlPoint(point);
  m_transform.InverseTransformPosition(controlPoint.x, controlPoint.y);
  if (!HitTest(controlPoint))
    SetFocus(false);
}

bool CGUIControl::HasID(DWORD dwID) const
{
  return GetID() == dwID;
}

bool CGUIControl::HasVisibleID(DWORD dwID) const
{
  return GetID() == dwID && IsVisible();
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
