#include "include.h"
#include "GUIControl.h"
#include "../xbmc/utils/GUIInfoManager.h"
#include "LocalizeStrings.h"
#include "../xbmc/Util.h"

#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
#include "SkinInfo.h"
#endif

CGUIControl::CGUIControl()
{
  m_hasRendered = false;
  m_bHasFocus = false;
  m_dwControlID = 0;
  m_iGroup = -1;
  m_dwParentID = 0;
  m_bVisible = true;
  m_visibleCondition = 0;
  m_bDisabled = false;
  m_bSelected = false;
  m_bCalibration = true;
  m_colDiffuse = 0xFFFFFFFF;
  m_dwAlpha = 0xFF;
  m_iPosX = 0;
  m_iPosY = 0;
  m_dwControlLeft = 0;
  m_dwControlRight = 0;
  m_dwControlUp = 0;
  m_dwControlDown = 0;
  ControlType = GUICONTROL_UNKNOWN;
  m_bInvalidated = true;
  m_bAllocated=false;
}

CGUIControl::CGUIControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight)
{
  m_colDiffuse = 0xFFFFFFFF;
  m_dwAlpha = 0xFF;
  m_iPosX = iPosX;
  m_iPosY = iPosY;
  m_dwWidth = dwWidth;
  m_dwHeight = dwHeight;
  m_bHasFocus = false;
  m_dwControlID = dwControlId;
  m_iGroup = -1;
  m_dwParentID = dwParentID;
  m_bVisible = true;
  m_lastVisible = true;
  m_visibleCondition = 0;
  m_bDisabled = false;
  m_bSelected = false;
  m_bCalibration = true;
  m_dwControlLeft = 0;
  m_dwControlRight = 0;
  m_dwControlUp = 0;
  m_dwControlDown = 0;
  ControlType = GUICONTROL_UNKNOWN;
  m_bInvalidated = true;
  m_bAllocated=false;
  m_hasRendered = false;
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
    // Reset our animation states
    for (unsigned int i = 0; i < m_animations.size(); i++)
      m_animations[i].ResetAnimation();
    m_bAllocated=false;
  }
  m_hasRendered = false;
}

bool CGUIControl::IsAllocated()
{
  return m_bAllocated;
}

void CGUIControl::DynamicResourceAlloc(bool bOnOff)
{

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
    OnDown();
    return true;
    break;

  case ACTION_MOVE_UP:
    OnUp();
    return true;
    break;

  case ACTION_MOVE_LEFT:
    OnLeft();
    return true;
    break;

  case ACTION_MOVE_RIGHT:
    OnRight();
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
    SetFocus(false);
    CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), m_dwControlUp, ACTION_MOVE_UP);
    g_graphicsContext.SendMessage(msg);
  }
}

void CGUIControl::OnDown()
{
  if (HasFocus())
  {
    SetFocus(false);
    CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), m_dwControlDown, ACTION_MOVE_DOWN);
    g_graphicsContext.SendMessage(msg);
  }
}

void CGUIControl::OnLeft()
{
  if (HasFocus())
  {
    SetFocus(false);
    CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), m_dwControlLeft, ACTION_MOVE_LEFT);
    g_graphicsContext.SendMessage(msg);
  }
}

void CGUIControl::OnRight()
{
  if (HasFocus())
  {
    SetFocus(false);
    CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), m_dwControlRight, ACTION_MOVE_RIGHT);
    g_graphicsContext.SendMessage(msg);
  }
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

void CGUIControl::SetFocus(bool bOnOff)
{
  if (m_bHasFocus && !bOnOff)
    QueueAnimation(ANIM_TYPE_UNFOCUS);
  else if (!m_bHasFocus && bOnOff)
    QueueAnimation(ANIM_TYPE_FOCUS);
  m_bHasFocus = bOnOff;
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
        DWORD dwControl = 0;
        if (message.GetParam1() == ACTION_MOVE_DOWN) dwControl = m_dwControlDown;
        if (message.GetParam1() == ACTION_MOVE_UP) dwControl = m_dwControlUp;
        if (message.GetParam1() == ACTION_MOVE_LEFT) dwControl = m_dwControlLeft;
        if (message.GetParam1() == ACTION_MOVE_RIGHT) dwControl = m_dwControlRight;
        if (GetID() != dwControl)
        {
          CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), dwControl, message.GetParam1());
          g_graphicsContext.SendMessage(msg);
          return true;
        }
        else
        {
          //ok, the control points to itself, it will do a stackoverflow so try to go back
          DWORD dwReverse = 0;
          if (message.GetParam1() == ACTION_MOVE_DOWN) {dwReverse = ACTION_MOVE_UP; dwControl = m_dwControlUp;}
          if (message.GetParam1() == ACTION_MOVE_UP) {dwReverse = ACTION_MOVE_DOWN; dwControl = m_dwControlDown;}
          if (message.GetParam1() == ACTION_MOVE_LEFT) {dwReverse = ACTION_MOVE_RIGHT; dwControl = m_dwControlRight;}
          if (message.GetParam1() == ACTION_MOVE_RIGHT) {dwReverse = ACTION_MOVE_LEFT; dwControl = m_dwControlLeft;}
          // if the other direction also points to itself it will still stackoverflow, so prevent
          // this and indicate in the logs that there is a skin problem
          if (GetID() == dwControl)
          {
            // Note the more likely problem here is that this message has not come from a control
            // on this window - there is no inclusion of the window parameter for messages sent
            // to controls.  Ideally, every message who's destination is a control should have
            // both destination window and destination control included.  If a message has the destination
            // window parameter set, then only that window should be allowed to deal with the message.
            CLog::Log(LOGERROR, "Control %d in window %d has been asked to focus, but it can't, and all directions point to itself", GetID(), GetParentID());
            return true;
          }
          CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), dwControl, dwReverse);
          g_graphicsContext.SendMessage(msg);
          return true;
        }
      }
      SetFocus(true);
      return true;
      break;

    case GUI_MSG_LOSTFOCUS:
      {
        SetFocus(false);
        return true;
      }
      break;

    case GUI_MSG_VISIBLE:
      if (message.GetParam1())  // effect time
      {
//        CLog::DebugLog("Control %i set to visible.  Was %s, effect=%d", m_dwControlID, m_bVisible ? "visible" : "hidden", m_tempAnimation.currentProcess == ANIM_PROCESS_NORMAL ? "fading in" : (m_tempAnimation.currentProcess == ANIM_PROCESS_REVERSE) ? "fading out" : "none");
        if ((!m_bVisible && m_tempAnimation.currentProcess != ANIM_PROCESS_NORMAL) ||
            ( m_bVisible && m_tempAnimation.currentProcess == ANIM_PROCESS_REVERSE))
        {
          m_tempAnimation.type = ANIM_TYPE_VISIBLE;
          m_tempAnimation.effect = EFFECT_TYPE_FADE;
          m_tempAnimation.queuedProcess = ANIM_PROCESS_NORMAL;
          m_tempAnimation.length = message.GetParam1();
        }
        else
          m_tempAnimation.Reset();
      }
      else
        m_bVisible = m_visibleCondition ? g_infoManager.GetBool(m_visibleCondition, m_dwParentID) : true;
      return true;
      break;

    case GUI_MSG_HIDDEN:
      if (message.GetParam1())  // fade time
      {
//        CLog::DebugLog("Control %i set to hidden.  Was %s, effect=%d", m_dwControlID, m_bVisible ? "visible" : "hidden", m_tempAnimation.currentProcess == ANIM_PROCESS_NORMAL ? "fading in" : (m_tempAnimation.currentProcess == ANIM_PROCESS_REVERSE) ? "fading out" : "none");
        if (m_bVisible && m_tempAnimation.currentProcess != ANIM_PROCESS_REVERSE)
        {
          m_tempAnimation.type = ANIM_TYPE_VISIBLE;
          m_tempAnimation.effect = EFFECT_TYPE_FADE;
          m_tempAnimation.queuedProcess = ANIM_PROCESS_REVERSE;
          m_tempAnimation.length = message.GetParam1();
        }
      }
      else
        m_bVisible = false;
      // reset any visible animations that are in process
      if (IsAnimating(ANIM_TYPE_VISIBLE))
      {
//        CLog::DebugLog("Resetting visible animation on control %i (we are %s)", m_dwControlID, m_bVisible ? "visible" : "hidden");
        CAnimation *visibleAnim = GetAnimation(ANIM_TYPE_VISIBLE);
        if (visibleAnim) visibleAnim->ResetAnimation();
      }
      if (m_tempAnimation.type == ANIM_TYPE_VISIBLE && m_tempAnimation.queuedProcess != ANIM_PROCESS_REVERSE && m_tempAnimation.currentProcess != ANIM_PROCESS_REVERSE)
      {
//        CLog::DebugLog("Resetting temp visible animation on control %i (we are %s)", m_dwControlID, m_bVisible ? "visible" : "hidden");
        m_tempAnimation.Reset();
      }
      return true;
      break;

    case GUI_MSG_ENABLED:
      m_bDisabled = false;
      return true;
      break;

    case GUI_MSG_DISABLED:
      m_bDisabled = true;
      return true;
      break;
    case GUI_MSG_SELECTED:
      m_bSelected = true;
      return true;
      break;

    case GUI_MSG_DESELECTED:
      m_bSelected = false;
      return true;
      break;
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
  return m_bVisible;
}

bool CGUIControl::IsSelected() const
{
  return m_bSelected;
}

bool CGUIControl::IsDisabled() const
{
  return m_bDisabled;
}

void CGUIControl::SetEnabled(bool bEnable)
{
  m_bDisabled = !bEnable;
}

void CGUIControl::SetPosition(int iPosX, int iPosY)
{
  if ((m_iPosX != iPosX) || (m_iPosY != iPosY))
  {
    m_iPosX = iPosX;
    m_iPosY = iPosY;
    Update();
  }
}

void CGUIControl::SetAlpha(DWORD dwAlpha)
{
  m_dwAlpha = dwAlpha;
  D3DCOLOR colour = (dwAlpha << 24) | 0xFFFFFF;
  return SetColourDiffuse(colour);
}

void CGUIControl::SetColourDiffuse(D3DCOLOR colour)
{
  if (colour != m_colDiffuse)
  {
    m_colDiffuse = colour;
    Update();
  }
}

int CGUIControl::GetXPosition() const
{
  return m_iPosX;
}

int CGUIControl::GetYPosition() const
{
  return m_iPosY;
}

DWORD CGUIControl::GetWidth() const
{
  return m_dwWidth;
}

DWORD CGUIControl::GetHeight() const
{
  return m_dwHeight;
}

void CGUIControl::SetNavigation(DWORD dwUp, DWORD dwDown, DWORD dwLeft, DWORD dwRight)
{
  m_dwControlUp = dwUp;
  m_dwControlDown = dwDown;
  m_dwControlLeft = dwLeft;
  m_dwControlRight = dwRight;
}

void CGUIControl::SetWidth(int iWidth)
{
  if (m_dwWidth != iWidth)
  {
    m_dwWidth = iWidth;
    Update();
  }
}

void CGUIControl::SetHeight(int iHeight)
{
  if (m_dwHeight != iHeight)
  {
    m_dwHeight = iHeight;
    Update();
  }
}

void CGUIControl::SetVisible(bool bVisible)
{
  if (m_visibleCondition)
    bVisible = g_infoManager.GetBool(m_visibleCondition, m_dwParentID);
  if (m_bVisible != bVisible)
  {
    m_bVisible = bVisible;
    m_lastVisible = bVisible;
    m_bInvalidated = true;
  }
}

void CGUIControl::SetSelected(bool bSelected)
{
  if (m_bSelected != bSelected)
  {
    m_bSelected = bSelected;
    m_bInvalidated = true;
  }
}

bool CGUIControl::HitTest(int iPosX, int iPosY) const
{
  if (iPosX >= (int)m_iPosX && iPosX <= (int)(m_iPosX + m_dwWidth) && iPosY >= (int)m_iPosY && iPosY <= (int)(m_iPosY + m_dwHeight))
    return true;
  return false;
}

// override this function to implement custom mouse behaviour
void CGUIControl::OnMouseOver()
{
  if (g_Mouse.GetState() != MOUSE_STATE_DRAG)
    g_Mouse.SetState(MOUSE_STATE_FOCUS);
  CGUIMessage msg(GUI_MSG_SETFOCUS, GetParentID(), GetID());
  g_graphicsContext.SendMessage(msg);
}

void CGUIControl::SetGroup(int iGroup)
{
  m_iGroup = iGroup;
}

int CGUIControl::GetGroup(void) const
{
  return m_iGroup;
}

void CGUIControl::UpdateVisibility()
{
  bool bWasVisible = m_lastVisible;
  m_lastVisible = g_infoManager.GetBool(m_visibleCondition, m_dwParentID);
  if (!bWasVisible && m_lastVisible)
  { // automatic change of visibility - queue the in effect
//    CLog::DebugLog("Visibility changed to visible for control id %i", m_dwControlID);
    QueueAnimation(ANIM_TYPE_VISIBLE);
  }
  else if (bWasVisible && !m_lastVisible)
  { // automatic change of visibility - do the out effect
//    CLog::DebugLog("Visibility changed to hidden for control id %i", m_dwControlID);
    QueueAnimation(ANIM_TYPE_HIDDEN);
  }
}

void CGUIControl::SetInitialVisibility()
{
  // NOTE: Remove the m_startHidden variable when we switch to 2.0 skin
  if (g_SkinInfo.GetVersion() < 1.90)
  {
    if (m_startHidden)
    {
      m_lastVisible = m_bVisible = false;
    }
    else
    {
      m_lastVisible = m_bVisible = g_infoManager.GetBool(m_visibleCondition, m_dwParentID);
    }
//    CLog::DebugLog("Set initial visibility for control %i: %s", m_dwControlID, m_bVisible ? "visible" : "hidden");
    UpdateVisibility();
    return;
  }
  m_lastVisible = m_bVisible = g_infoManager.GetBool(m_visibleCondition, m_dwParentID);
//  CLog::DebugLog("Set initial visibility for control %i: %s", m_dwControlID, m_bVisible ? "visible" : "hidden");
}

bool CGUIControl::UpdateEffectState()
{
  if (m_visibleCondition)
    UpdateVisibility();
  Animate();
  return IsVisible();
}

void CGUIControl::SetVisibleCondition(int visible)
{
  m_visibleCondition = visible;
}

void CGUIControl::SetVisibleCondition(int visible, bool allowHiddenFocus, bool startHidden)
{
  m_visibleCondition = visible;
  m_allowHiddenFocus = allowHiddenFocus;
  m_startHidden = startHidden;
}

void CGUIControl::SetAnimations(const vector<CAnimation> &animations)
{
  m_animations = animations;
  if (g_SkinInfo.GetVersion() < 1.86)
  { // any slide effects are relative now
    for (unsigned int i = 0; i < m_animations.size(); i++)
    {
      CAnimation &anim = m_animations[i];
      if (anim.effect == EFFECT_TYPE_SLIDE)
      {
        if (anim.type == ANIM_TYPE_VISIBLE)
        {
          anim.startX -= m_iPosX;
          anim.startY -= m_iPosY;
        }
        if (anim.type == ANIM_TYPE_HIDDEN)
        {
          anim.endX -= m_iPosX;
          anim.endY -= m_iPosY;
        }
      }
    }
  }
}

void CGUIControl::QueueAnimation(ANIMATION_TYPE animType)
{
  // rule out the animations we shouldn't perform
  if (!m_bVisible || !HasRendered()) 
  { // hidden or never rendered - don't allow exit animations for this control
    if (animType == ANIM_TYPE_WINDOW_CLOSE)
      return;
  }
  if (!m_bVisible)
  { // hidden - only allow hidden anims if we're animating a visible anim
    if (animType == ANIM_TYPE_HIDDEN && !IsAnimating(ANIM_TYPE_VISIBLE))
      return;
  }
  CAnimation *reverseAnim = GetAnimation((ANIMATION_TYPE)-animType);
  CAnimation *forwardAnim = GetAnimation(animType);
  // we first check whether the reverse animation is in progress (and reverse it)
  // then we check for the normal animation, and queue it
  if (reverseAnim && (reverseAnim->currentState == ANIM_STATE_IN_PROCESS || reverseAnim->currentState == ANIM_STATE_DELAYED))
  {
    reverseAnim->queuedProcess = ANIM_PROCESS_REVERSE;
    if (forwardAnim) forwardAnim->ResetAnimation();
  }
  else if (forwardAnim)
  {
    forwardAnim->queuedProcess = ANIM_PROCESS_NORMAL;
    if (reverseAnim) reverseAnim->ResetAnimation();
  }
  else
  { // hidden and visible animations delay the change of state.  If there is no animations
    // to perform, then we should just change the state straightaway
//    if (m_dwParentID == WINDOW_VISUALISATION) CLog::DebugLog("No forward animation found for control %i, anim %i", m_dwControlID, animType);
    if (reverseAnim) reverseAnim->ResetAnimation();
//    else if (m_dwParentID == WINDOW_VISUALISATION) CLog::DebugLog("Also no reverse animation found");
    UpdateStates(animType, ANIM_PROCESS_NORMAL, ANIM_STATE_APPLIED);
  }
}

CAnimation *CGUIControl::GetAnimation(ANIMATION_TYPE type)
{
  for (unsigned int i = 0; i < m_animations.size(); i++)
  {
    if (m_animations[i].type == type)
      return &m_animations[i];
  }
  return NULL;
}

void CGUIControl::UpdateStates(ANIMATION_TYPE type, ANIMATION_PROCESS currentProcess, ANIMATION_STATE currentState)
{
  bool visible = m_bVisible;
  // Make sure control is hidden or visible at the appropriate times
  // while processing a visible or hidden animation it needs to be visible,
  // but when finished a hidden operation it needs to be hidden
  if (type == ANIM_TYPE_VISIBLE)
  {
    if (currentProcess == ANIM_PROCESS_REVERSE)
    {
      if (currentState == ANIM_STATE_APPLIED)
        m_bVisible = false;
    }
    else if (currentProcess == ANIM_PROCESS_NORMAL)
    {
      if (currentState == ANIM_STATE_DELAYED)
        m_bVisible = false;
      else
        m_bVisible = m_lastVisible; // true ??
    }
  }
  else if (type == ANIM_TYPE_HIDDEN)
  {
    if (currentProcess == ANIM_PROCESS_NORMAL)  // a hide animation
    {
      if (currentState == ANIM_STATE_APPLIED)
        m_bVisible = false; // finished
      else
        m_bVisible = true; // have to be visible until we are finished
    }
    else if (currentProcess == ANIM_PROCESS_REVERSE)  // a visible animation
    { // no delay involved here - just make sure it's visible
      m_bVisible = m_lastVisible;
    }
  }
  else if (type == ANIM_TYPE_WINDOW_OPEN)
  {
    if (currentProcess == ANIM_PROCESS_NORMAL)
    {
      if (currentState == ANIM_STATE_DELAYED)
        m_bVisible = false; // delayed
      else
        m_bVisible = m_lastVisible;
    }
  }
//  if (visible != m_bVisible)
//    CLog::DebugLog("UpdateControlState of control id %i - now %s (type=%d, process=%d, state=%d)", m_dwControlID, m_bVisible ? "visible" : "hidden", type, currentProcess, currentState);
}

void CGUIControl::Animate()
{
  DWORD currentTime = timeGetTime();
  for (unsigned int i = 0; i < m_animations.size(); i++)
  {
    CAnimation &anim = m_animations[i];
    anim.Animate(currentTime, HasRendered());
    // Update the control states (such as visibility)
    UpdateStates(anim.type, anim.currentProcess, anim.currentState);
    // and render the animation effect
    g_graphicsContext.AddControlAnimation(anim.RenderAnimation());
/*
    // debug stuff
    if (anim.currentProcess != ANIM_PROCESS_NONE)
    {
      if (anim.effect == EFFECT_TYPE_SLIDE)
      {
        if (IsVisible() && m_dwParentID == WINDOW_VISUALISATION)
          CLog::DebugLog("Animating control %d with a %s slide effect %s. Amount is %2.1f, visible=%s", m_dwControlID, anim.type == ANIM_TYPE_VISIBLE ? "visible" : "hidden", anim.currentProcess == ANIM_PROCESS_NORMAL ? "normal" : "reverse", anim.amount, IsVisible() ? "true" : "false");
      }
      else if (anim.effect == EFFECT_TYPE_FADE)
      {
        if (IsVisible() && m_dwParentID == WINDOW_VISUALISATION)
          CLog::DebugLog("Animating control %d with a %s fade effect %s. Amount is %2.1f. Visible=%s", m_dwControlID, anim.type == ANIM_TYPE_VISIBLE ? "visible" : "hidden", anim.currentProcess == ANIM_PROCESS_NORMAL ? "normal" : "reverse", anim.amount, IsVisible() ? "true" : "false");
      }
    }
  */  
  }
#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
  // do the temporary fade effect as well
  m_tempAnimation.Animate(currentTime, HasRendered());
  UpdateStates(m_tempAnimation.type, m_tempAnimation.currentProcess, m_tempAnimation.currentState);
  g_graphicsContext.AddControlAnimation(m_tempAnimation.RenderAnimation());
  CAnimation anim = m_tempAnimation;
/*    // debug stuff
    if (anim.currentProcess != ANIM_PROCESS_NONE)
    {
      if (anim.effect == EFFECT_TYPE_SLIDE)
      {
        if (IsVisible())
          CLog::DebugLog("Animating control %d with a %s slide effect %s. Amount is %2.1f, visible=%s", m_dwControlID, anim.type == ANIM_TYPE_VISIBLE ? "visible" : "hidden", anim.currentProcess == ANIM_PROCESS_NORMAL ? "normal" : "reverse", anim.amount, IsVisible() ? "true" : "false");
      }
      else if (anim.effect == EFFECT_TYPE_FADE)
      {
        if (IsVisible())
          CLog::DebugLog("Animating control %d with a %s fade effect %s. Amount is %2.1f. Visible=%s", m_dwControlID, anim.type == ANIM_TYPE_VISIBLE ? "visible" : "hidden", anim.currentProcess == ANIM_PROCESS_NORMAL ? "normal" : "reverse", anim.amount, IsVisible() ? "true" : "false");
      }
    }*/
#endif
}

bool CGUIControl::IsAnimating(ANIMATION_TYPE animType)
{
  for (unsigned int i = 0; i < m_animations.size(); i++)
  {
    CAnimation &anim = m_animations[i];
    if (anim.type == animType)
    {
      if (anim.queuedProcess == ANIM_PROCESS_NORMAL)
        return true;
      if (anim.currentProcess == ANIM_PROCESS_NORMAL)
        return true;
    }
    else if (anim.type == -animType)
    {
      if (anim.queuedProcess == ANIM_PROCESS_REVERSE)
        return true;
      if (anim.currentProcess == ANIM_PROCESS_REVERSE)
        return true;
    }
  }
  return false;
}
