/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIControl.h"

#include "GUIAction.h"
#include "GUIComponent.h"
#include "GUIControlProfiler.h"
#include "GUIInfoManager.h"
#include "GUIMessage.h"
#include "GUITexture.h"
#include "GUIWindowManager.h"
#include "ServiceBroker.h"
#include "input/InputManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "input/mouse/MouseEvent.h"
#include "input/mouse/MouseStat.h"
#include "utils/log.h"

using namespace KODI;
using namespace GUILIB;

CGUIControl::CGUIControl()
{
  m_hasProcessed = false;
  m_bHasFocus = false;
  m_controlID = 0;
  m_parentID = 0;
  m_visible = VISIBLE;
  m_visibleFromSkinCondition = true;
  m_forceHidden = false;
  m_enabled = true;
  m_posX = 0;
  m_posY = 0;
  m_width = 0;
  m_height = 0;
  ControlType = GUICONTROL_UNKNOWN;
  m_bInvalidated = true;
  m_bAllocated=false;
  m_parentControl = NULL;
  m_hasCamera = false;
  m_pushedUpdates = false;
  m_pulseOnSelect = false;
  m_controlDirtyState = DIRTY_STATE_CONTROL;
  m_stereo = 0.0f;
  m_controlStats = nullptr;
}

CGUIControl::CGUIControl(int parentID, int controlID, float posX, float posY, float width, float height)
: m_hitRect(posX, posY, posX + width, posY + height),
  m_diffuseColor(0xffffffff)
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
  m_forceHidden = false;
  m_enabled = true;
  ControlType = GUICONTROL_UNKNOWN;
  m_bInvalidated = true;
  m_bAllocated=false;
  m_hasProcessed = false;
  m_parentControl = NULL;
  m_hasCamera = false;
  m_pushedUpdates = false;
  m_pulseOnSelect = false;
  m_controlDirtyState = DIRTY_STATE_CONTROL;
  m_stereo = 0.0f;
  m_controlStats = nullptr;
}

CGUIControl::CGUIControl(const CGUIControl &) = default;

CGUIControl::~CGUIControl(void) = default;

void CGUIControl::AllocResources()
{
  m_hasProcessed = false;
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
  m_hasProcessed = false;
}

void CGUIControl::DynamicResourceAlloc(bool bOnOff)
{

}

// the main processing routine.
// 1. animate and set animation transform
// 2. if visible, process
// 3. reset the animation transform
void CGUIControl::DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  CRect dirtyRegion = m_renderRegion;

  bool changed = (m_controlDirtyState & DIRTY_STATE_CONTROL) != 0 || (m_bInvalidated && IsVisible());
  m_controlDirtyState = 0;

  if (Animate(currentTime))
    MarkDirtyRegion();

  // if the control changed culling state from true to false, mark it
  const bool culled = m_transform.alpha <= 0.01f;
  if (m_isCulled != culled)
  {
    m_isCulled = false;
    MarkDirtyRegion();
  }
  m_isCulled = culled;

  if (IsVisible())
  {
    m_cachedTransform = CServiceBroker::GetWinSystem()->GetGfxContext().AddTransform(m_transform);
    if (m_hasCamera)
      CServiceBroker::GetWinSystem()->GetGfxContext().SetCameraPosition(m_camera);

    Process(currentTime, dirtyregions);
    m_bInvalidated = false;

    if (dirtyRegion != m_renderRegion)
    {
      dirtyRegion.Union(m_renderRegion);
      changed = true;
    }

    if (m_hasCamera)
      CServiceBroker::GetWinSystem()->GetGfxContext().RestoreCameraPosition();
    CServiceBroker::GetWinSystem()->GetGfxContext().RemoveTransform();
  }

  UpdateControlStats();

  changed |= (m_controlDirtyState & DIRTY_STATE_CONTROL) != 0;

  if (changed)
  {
    dirtyregions.emplace_back(dirtyRegion);
  }
}

void CGUIControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  // update our render region
  m_renderRegion = CServiceBroker::GetWinSystem()->GetGfxContext().GenerateAABB(CalcRenderRegion());
  m_hasProcessed = true;
}

// the main render routine.
// 1. set the animation transform
// 2. if visible, paint
// 3. reset the animation transform
void CGUIControl::DoRender()
{
  if (IsControlRenderable() &&
      !m_renderRegion.Intersects(CServiceBroker::GetWinSystem()->GetGfxContext().GetScissors()))
    return;

  if (IsVisible() && !m_isCulled)
  {
    bool hasStereo =
        m_stereo != 0.0f &&
        CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoMode() !=
            RENDER_STEREO_MODE_MONO &&
        CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoMode() != RENDER_STEREO_MODE_OFF;

    CServiceBroker::GetWinSystem()->GetGfxContext().SetTransform(m_cachedTransform);
    if (m_hasCamera)
      CServiceBroker::GetWinSystem()->GetGfxContext().SetCameraPosition(m_camera);
    if (hasStereo)
      CServiceBroker::GetWinSystem()->GetGfxContext().SetStereoFactor(m_stereo);

    GUIPROFILER_RENDER_BEGIN(this);

    if (m_hitColor != 0xffffffff)
    {
      UTILS::COLOR::Color color =
          CServiceBroker::GetWinSystem()->GetGfxContext().MergeAlpha(m_hitColor);
      CGUITexture::DrawQuad(CServiceBroker::GetWinSystem()->GetGfxContext().GenerateAABB(m_hitRect), color);
    }

    Render();

    GUIPROFILER_RENDER_END(this);

    if (hasStereo)
      CServiceBroker::GetWinSystem()->GetGfxContext().RestoreStereoFactor();
    if (m_hasCamera)
      CServiceBroker::GetWinSystem()->GetGfxContext().RestoreCameraPosition();
    CServiceBroker::GetWinSystem()->GetGfxContext().RemoveTransform();
  }
}

bool CGUIControl::OnAction(const CAction &action)
{
  if (HasFocus())
  {
    switch (action.GetID())
    {
    case ACTION_MOVE_DOWN:
      OnDown();
      return true;

    case ACTION_MOVE_UP:
      OnUp();
      return true;

    case ACTION_MOVE_LEFT:
      OnLeft();
      return true;

    case ACTION_MOVE_RIGHT:
      OnRight();
      return true;

    case ACTION_SHOW_INFO:
      return OnInfo();

    case ACTION_NAV_BACK:
      return OnBack();

    case ACTION_NEXT_CONTROL:
      OnNextControl();
      return true;

    case ACTION_PREV_CONTROL:
      OnPrevControl();
      return true;
    }
  }
  return false;
}

bool CGUIControl::Navigate(int direction) const
{
  if (HasFocus())
  {
    CGUIMessage msg(GUI_MSG_MOVE, GetParentID(), GetID(), direction);
    return SendWindowMessage(msg);
  }
  return false;
}

// Movement controls (derived classes can override)
void CGUIControl::OnUp()
{
  Navigate(ACTION_MOVE_UP);
}

void CGUIControl::OnDown()
{
  Navigate(ACTION_MOVE_DOWN);
}

void CGUIControl::OnLeft()
{
  Navigate(ACTION_MOVE_LEFT);
}

void CGUIControl::OnRight()
{
  Navigate(ACTION_MOVE_RIGHT);
}

bool CGUIControl::OnBack()
{
  return Navigate(ACTION_NAV_BACK);
}

bool CGUIControl::OnInfo()
{
  CGUIAction action = GetAction(ACTION_SHOW_INFO);
  if (action.HasAnyActions())
    return action.ExecuteActions(GetID(), GetParentID());
  return false;
}

void CGUIControl::OnNextControl()
{
  Navigate(ACTION_NEXT_CONTROL);
}

void CGUIControl::OnPrevControl()
{
  Navigate(ACTION_PREV_CONTROL);
}

bool CGUIControl::SendWindowMessage(CGUIMessage &message) const
{
  CGUIWindow *pWindow = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(GetParentID());
  if (pWindow)
    return pWindow->OnMessage(message);
  return CServiceBroker::GetGUI()->GetWindowManager().SendMessage(message);
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
        CLog::Log(LOGERROR,
                  "Control {} in window {} has been asked to focus, "
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
      SetVisible(true, true);
      return true;
      break;

    case GUI_MSG_HIDDEN:
      SetVisible(false);
      return true;

      // Note that the skin <enable> tag will override these messages
    case GUI_MSG_ENABLED:
      SetEnabled(true);
      return true;

    case GUI_MSG_DISABLED:
      SetEnabled(false);
      return true;

    case GUI_MSG_WINDOW_RESIZE:
      // invalidate controls to get them to recalculate sizing information
      SetInvalid();
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
  if (m_forceHidden)
    return false;
  return m_visible == VISIBLE;
}

bool CGUIControl::IsDisabled() const
{
  return !m_enabled;
}

void CGUIControl::SetEnabled(bool bEnable)
{
  if (bEnable != m_enabled)
  {
    m_enabled = bEnable;
    SetInvalid();
  }
}

void CGUIControl::SetEnableCondition(const std::string &expression)
{
  if (expression == "true")
    m_enabled = true;
  else if (expression == "false")
    m_enabled = false;
  else
    m_enableCondition = CServiceBroker::GetGUI()->GetInfoManager().Register(expression, GetParentID());
}

void CGUIControl::SetPosition(float posX, float posY)
{
  if ((m_posX != posX) || (m_posY != posY))
  {
    MarkDirtyRegion();

    m_hitRect += CPoint(posX - m_posX, posY - m_posY);
    m_posX = posX;
    m_posY = posY;

    SetInvalid();
  }
}

bool CGUIControl::SetColorDiffuse(const GUIINFO::CGUIInfoColor &color)
{
  bool changed = m_diffuseColor != color;
  m_diffuseColor = color;
  return changed;
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

void CGUIControl::AssignDepth()
{
  m_cachedTransform.depth = CServiceBroker::GetWinSystem()->GetGfxContext().GetDepth();
}

void CGUIControl::MarkDirtyRegion(const unsigned int dirtyState)
{
  // if the control is culled, bail
  if (dirtyState == DIRTY_STATE_CONTROL && m_isCulled)
    return;
  if (!m_controlDirtyState && m_parentControl)
    m_parentControl->MarkDirtyRegion(DIRTY_STATE_CHILD);

  m_controlDirtyState |= dirtyState;
}

CRect CGUIControl::CalcRenderRegion() const
{
  CPoint tl(GetXPosition(), GetYPosition());
  CPoint br(tl.x + GetWidth(), tl.y + GetHeight());

  return CRect(tl.x, tl.y, br.x, br.y);
}

void CGUIControl::SetActions(const ActionMap &actions)
{
  m_actions = actions;
}

void CGUIControl::SetAction(int actionID, const CGUIAction &action, bool replace /*= true*/)
{
  ActionMap::iterator i = m_actions.find(actionID);
  if (i == m_actions.end() || !i->second.HasAnyActions() || replace)
    m_actions[actionID] = action;
}

void CGUIControl::SetWidth(float width)
{
  if (m_width != width)
  {
    MarkDirtyRegion();
    m_width = width;
    m_hitRect.x2 = m_hitRect.x1 + width;
    SetInvalid();
  }
}

void CGUIControl::SetHeight(float height)
{
  if (m_height != height)
  {
    MarkDirtyRegion();
    m_height = height;
    m_hitRect.y2 = m_hitRect.y1 + height;
    SetInvalid();
  }
}

void CGUIControl::SetVisible(bool bVisible, bool setVisState)
{
  if (bVisible && setVisState)
  {  //! @todo currently we only update m_visible from GUI_MSG_VISIBLE (SET_CONTROL_VISIBLE)
     //!       otherwise we just set m_forceHidden
    GUIVISIBLE visible;
    if (m_visibleCondition)
      visible = m_visibleCondition->Get(INFO::DEFAULT_CONTEXT) ? VISIBLE : HIDDEN;
    else
      visible = VISIBLE;
    if (visible != m_visible)
    {
      m_visible = visible;
      SetInvalid();
    }
  }
  if (m_forceHidden == bVisible)
  {
    m_forceHidden = !bVisible;
    SetInvalid();
    if (m_forceHidden)
      MarkDirtyRegion();
  }
  if (m_forceHidden)
  { // reset any visible animations that are in process
    if (IsAnimating(ANIM_TYPE_VISIBLE))
    {
      //        CLog::Log(LOGDEBUG, "Resetting visible animation on control {} (we are {})", m_controlID, m_visible ? "visible" : "hidden");
      CAnimation *visibleAnim = GetAnimation(ANIM_TYPE_VISIBLE);
      if (visibleAnim) visibleAnim->ResetAnimation();
    }
  }
}

bool CGUIControl::HitTest(const CPoint &point) const
{
  return m_hitRect.PtInRect(point);
}

EVENT_RESULT CGUIControl::SendMouseEvent(const CPoint& point, const MOUSE::CMouseEvent& event)
{
  CPoint childPoint(point);
  m_transform.InverseTransformPosition(childPoint.x, childPoint.y);
  if (!CanFocusFromPoint(childPoint))
    return EVENT_RESULT_UNHANDLED;

  bool handled = event.m_id != ACTION_MOUSE_MOVE || OnMouseOver(childPoint);
  EVENT_RESULT ret = OnMouseEvent(childPoint, event);
  if (ret)
    return ret;
  return (handled && (event.m_id == ACTION_MOUSE_MOVE)) ? EVENT_RESULT_HANDLED : EVENT_RESULT_UNHANDLED;
}

// override this function to implement custom mouse behaviour
bool CGUIControl::OnMouseOver(const CPoint &point)
{
  if (CServiceBroker::GetInputManager().GetMouseState() != MOUSE_STATE_DRAG)
    CServiceBroker::GetInputManager().SetMouseState(MOUSE_STATE_FOCUS);
  if (!CanFocus()) return false;
  if (!HasFocus())
  {
    CGUIMessage msg(GUI_MSG_SETFOCUS, GetParentID(), GetID());
    OnMessage(msg);
  }
  return true;
}

void CGUIControl::UpdateVisibility(const CGUIListItem *item)
{
  if (m_visibleCondition)
  {
    bool bWasVisible = m_visibleFromSkinCondition;
    m_visibleFromSkinCondition = m_visibleCondition->Get(INFO::DEFAULT_CONTEXT, item);
    if (!bWasVisible && m_visibleFromSkinCondition)
    { // automatic change of visibility - queue the in effect
      //    CLog::Log(LOGDEBUG, "Visibility changed to visible for control id {}", m_controlID);
      QueueAnimation(ANIM_TYPE_VISIBLE);
    }
    else if (bWasVisible && !m_visibleFromSkinCondition)
    { // automatic change of visibility - do the out effect
      //    CLog::Log(LOGDEBUG, "Visibility changed to hidden for control id {}", m_controlID);
      QueueAnimation(ANIM_TYPE_HIDDEN);
    }
  }
  // check for conditional animations
  for (unsigned int i = 0; i < m_animations.size(); i++)
  {
    CAnimation &anim = m_animations[i];
    if (anim.GetType() == ANIM_TYPE_CONDITIONAL)
      anim.UpdateCondition(item);
  }
  // and check for conditional enabling - note this overrides SetEnabled() from the code currently
  // this may need to be reviewed at a later date
  bool enabled = m_enabled;
  if (m_enableCondition)
    m_enabled = m_enableCondition->Get(INFO::DEFAULT_CONTEXT, item);

  if (m_enabled != enabled)
    MarkDirtyRegion();

  m_allowHiddenFocus.Update(INFO::DEFAULT_CONTEXT, item);
  if (UpdateColors(item))
    MarkDirtyRegion();
  // and finally, update our control information (if not pushed)
  if (!m_pushedUpdates)
    UpdateInfo(item);
}

bool CGUIControl::UpdateColors(const CGUIListItem* item)
{
  return m_diffuseColor.Update(item);
}

void CGUIControl::SetInitialVisibility()
{
  if (m_visibleCondition)
  {
    m_visibleFromSkinCondition = m_visibleCondition->Get(INFO::DEFAULT_CONTEXT);
    m_visible = m_visibleFromSkinCondition ? VISIBLE : HIDDEN;
    //  CLog::Log(LOGDEBUG, "Set initial visibility for control {}: {}", m_controlID, m_visible == VISIBLE ? "visible" : "hidden");
  }
  else if (m_visible == DELAYED)
    m_visible = VISIBLE;
  // and handle animation conditions as well
  for (unsigned int i = 0; i < m_animations.size(); i++)
  {
    CAnimation &anim = m_animations[i];
    if (anim.GetType() == ANIM_TYPE_CONDITIONAL)
      anim.SetInitialCondition();
  }
  // and check for conditional enabling - note this overrides SetEnabled() from the code currently
  // this may need to be reviewed at a later date
  if (m_enableCondition)
    m_enabled = m_enableCondition->Get(INFO::DEFAULT_CONTEXT);
  m_allowHiddenFocus.Update(INFO::DEFAULT_CONTEXT);
  UpdateColors(nullptr);

  MarkDirtyRegion();
}

void CGUIControl::SetVisibleCondition(const std::string &expression, const std::string &allowHiddenFocus)
{
  if (expression == "true")
    m_visible = VISIBLE;
  else if (expression == "false")
    m_visible = HIDDEN;
  else  // register with the infomanager for updates
    m_visibleCondition = CServiceBroker::GetGUI()->GetInfoManager().Register(expression, GetParentID());
  m_allowHiddenFocus.Parse(allowHiddenFocus, GetParentID());
}

void CGUIControl::SetAnimations(const std::vector<CAnimation> &animations)
{
  m_animations = animations;
  MarkDirtyRegion();
}

void CGUIControl::ResetAnimation(ANIMATION_TYPE type)
{
  MarkDirtyRegion();

  for (unsigned int i = 0; i < m_animations.size(); i++)
  {
    if (m_animations[i].GetType() == type)
      m_animations[i].ResetAnimation();
  }
}

void CGUIControl::ResetAnimations()
{
  MarkDirtyRegion();

  for (unsigned int i = 0; i < m_animations.size(); i++)
    m_animations[i].ResetAnimation();

  MarkDirtyRegion();
}

bool CGUIControl::CheckAnimation(ANIMATION_TYPE animType)
{
  // rule out the animations we shouldn't perform
  if (!IsVisible() || !HasProcessed())
  { // hidden or never processed - don't allow exit or entry animations for this control
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

  MarkDirtyRegion();

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
      if (!checkConditions || anim.CheckCondition())
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

bool CGUIControl::Animate(unsigned int currentTime)
{
  // check visible state outside the loop, as it could change
  GUIVISIBLE visible = m_visible;

  m_transform.Reset();
  bool changed = false;

  CPoint center(GetXPosition() + GetWidth() * 0.5f, GetYPosition() + GetHeight() * 0.5f);
  for (unsigned int i = 0; i < m_animations.size(); i++)
  {
    CAnimation &anim = m_animations[i];
    anim.Animate(currentTime, HasProcessed() || visible == DELAYED);
    // Update the control states (such as visibility)
    UpdateStates(anim.GetType(), anim.GetProcess(), anim.GetState());
    // and render the animation effect
    changed |= (anim.GetProcess() != ANIM_PROCESS_NONE);
    anim.RenderAnimation(m_transform, center);

    // debug stuff
    //if (anim.GetProcess() != ANIM_PROCESS_NONE && IsVisible())
    //{
    //  CLog::Log(LOGDEBUG, "Animating control {}", m_controlID);
    //}
  }

  return changed;
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

CGUIAction CGUIControl::GetAction(int actionID) const
{
  ActionMap::const_iterator i = m_actions.find(actionID);
  if (i != m_actions.end())
    return i->second;
  return CGUIAction();
}

bool CGUIControl::CanFocusFromPoint(const CPoint &point) const
{
  return CanFocus() && HitTest(point);
}

void CGUIControl::UnfocusFromPoint(const CPoint &point)
{
  if (HasFocus())
  {
    CPoint controlPoint(point);
    m_transform.InverseTransformPosition(controlPoint.x, controlPoint.y);
    if (!HitTest(controlPoint))
    {
      SetFocus(false);

      // and tell our parent so it can unfocus
      if (m_parentControl)
      {
        CGUIMessage msgLostFocus(GUI_MSG_LOSTFOCUS, GetID(), GetID());
        m_parentControl->OnMessage(msgLostFocus);
      }
    }
  }
}

void CGUIControl::SaveStates(std::vector<CControlState> &states)
{
  // empty for now - do nothing with the majority of controls
}

CGUIControl *CGUIControl::GetControl(int iControl, std::vector<CGUIControl*> *idCollector)
{
  return (iControl == m_controlID) ? this : nullptr;
}


void CGUIControl::UpdateControlStats()
{
  if (m_controlStats)
  {
    ++m_controlStats->nCountTotal;
    if (IsVisible() && IsVisibleFromSkin())
      ++m_controlStats->nCountVisible;
  }
}

bool CGUIControl::IsControlRenderable()
{
  switch (ControlType)
  {
    case GUICONTAINER_EPGGRID:
    case GUICONTAINER_FIXEDLIST:
    case GUICONTAINER_LIST:
    case GUICONTAINER_PANEL:
    case GUICONTAINER_WRAPLIST:
    case GUICONTROL_GROUP:
    case GUICONTROL_GROUPLIST:
    case GUICONTROL_LISTGROUP:
      return false;
    default:
      return true;
  }
}

void CGUIControl::SetHitRect(const CRect& rect, const UTILS::COLOR::Color& color)
{
  m_hitRect = rect;
  m_hitColor = color;
}

void CGUIControl::SetCamera(const CPoint &camera)
{
  m_camera = camera;
  m_hasCamera = true;
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

void CGUIControl::SetStereoFactor(const float &factor)
{
  m_stereo = factor;
}
