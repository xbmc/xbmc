/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindow.h"

#include "GUIAudioManager.h"
#include "GUIComponent.h"
#include "GUIControlFactory.h"
#include "GUIControlGroup.h"
#include "GUIControlProfiler.h"
#include "GUIInfoManager.h"
#include "GUIWindowManager.h"
#include "ServiceBroker.h"
#include "addons/Skin.h"
#include "input/WindowTranslator.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "input/mouse/MouseEvent.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "threads/SingleLock.h"
#include "utils/ColorUtils.h"
#include "utils/StringUtils.h"
#include "utils/TimeUtils.h"
#include "utils/Variant.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <mutex>

using namespace KODI;

bool CGUIWindow::icompare::operator()(const std::string &s1, const std::string &s2) const
{
  return StringUtils::CompareNoCase(s1, s2) < 0;
}

CGUIWindow::CGUIWindow(int id, const std::string &xmlFile)
{
  CGUIWindow::SetID(id);
  SetProperty("xmlfile", xmlFile);
  m_lastControlID = 0;
  m_needsScaling = true;
  m_windowLoaded = false;
  m_loadType = LOAD_EVERY_TIME;
  m_closing = false;
  m_active = false;
  m_renderOrder = RENDER_ORDER_WINDOW;
  m_dynamicResourceAlloc = true;
  m_previousWindow = WINDOW_INVALID;
  m_animationsEnabled = true;
  m_manualRunActions = false;
  m_exclusiveMouseControl = 0;
  m_clearBackground = 0xff000000; // opaque black -> always clear
  m_menuControlID = 0;
  m_menuLastFocusedControlID = 0;
  m_custom = false;
}

CGUIWindow::~CGUIWindow()
{
}

bool CGUIWindow::Load(const std::string& strFileName, bool bContainsPath)
{
  if (m_windowLoaded || !g_SkinInfo)
    return true;      // no point loading if it's already there

#ifdef _DEBUG
  const auto start = std::chrono::steady_clock::now();
#endif

  const char* strLoadType;
  switch (m_loadType)
  {
  case LOAD_ON_GUI_INIT:
    strLoadType = "LOAD_ON_GUI_INIT";
    break;
  case KEEP_IN_MEMORY:
    strLoadType = "KEEP_IN_MEMORY";
    break;
  case LOAD_EVERY_TIME:
  default:
    strLoadType = "LOAD_EVERY_TIME";
    break;
  }
  CLog::Log(LOGINFO, "Loading skin file: {}, load type: {}", strFileName, strLoadType);

  // Find appropriate skin folder + resolution to load from
  std::string strPath;
  std::string strLowerPath;
  if (bContainsPath)
    strPath = strFileName;
  else
  {
    // FIXME: strLowerPath needs to eventually go since resToUse can get incorrectly overridden
    std::string strFileNameLower = strFileName;
    StringUtils::ToLower(strFileNameLower);
    strLowerPath =  g_SkinInfo->GetSkinPath(strFileNameLower, &m_coordsRes);
    strPath = g_SkinInfo->GetSkinPath(strFileName, &m_coordsRes);
  }

  bool ret = LoadXML(strPath, strLowerPath);
  if (ret)
  {
    m_windowLoaded = true;
    OnWindowLoaded();

#ifdef _DEBUG
    const auto end = std::chrono::steady_clock::now();
    const std::chrono::duration<double, std::milli> duration = end - start;
    CLog::Log(LOGDEBUG, "Skin file {} loaded in {:.2f} ms", strPath, duration.count());
#endif
  }

  return ret;
}

bool CGUIWindow::LoadXML(const std::string &strPath, const std::string &strLowerPath)
{
  // load window xml if we don't have it stored yet
  if (!m_windowXMLRootElement)
  {
    CXBMCTinyXML xmlDoc;
    std::string strPathLower = strPath;
    StringUtils::ToLower(strPathLower);
    if (!xmlDoc.LoadFile(strPath) && !xmlDoc.LoadFile(strPathLower) && !xmlDoc.LoadFile(strLowerPath))
    {
      CLog::Log(LOGERROR, "Unable to load window XML: {}. Line {}\n{}", strPath, xmlDoc.ErrorRow(),
                xmlDoc.ErrorDesc());
      SetID(WINDOW_INVALID);
      return false;
    }

    // xml need a <window> root element
    if (!StringUtils::EqualsNoCase(xmlDoc.RootElement()->Value(), "window"))
    {
      CLog::Log(LOGERROR, "XML file {} does not contain a <window> root element",
                GetProperty("xmlfile").asString());
      return false;
    }

    // store XML for further processing if window's load type is LOAD_EVERY_TIME or a reload is needed
    m_windowXMLRootElement.reset(static_cast<TiXmlElement*>(xmlDoc.RootElement()->Clone()));
  }
  else
    CLog::Log(LOGDEBUG, "Using already stored xml root node for {}", strPath);

  return Load(Prepare(m_windowXMLRootElement).get());
}

std::unique_ptr<TiXmlElement> CGUIWindow::Prepare(const std::unique_ptr<TiXmlElement>& rootElement)
{
  if (!rootElement)
    return nullptr;

  // copy the root element as we will manipulate it
  auto preparedRoot = std::make_unique<TiXmlElement>(*rootElement);

  // Resolve any includes, constants, expressions that may be present
  // and save include's conditions to the given map
  g_SkinInfo->ResolveIncludes(preparedRoot.get(), &m_xmlIncludeConditions);

  return preparedRoot;
}

bool CGUIWindow::Load(TiXmlElement *pRootElement)
{
  if (!pRootElement)
    return false;

  // set the scaling resolution so that any control creation or initialisation can
  // be done with respect to the correct aspect ratio
  CServiceBroker::GetWinSystem()->GetGfxContext().SetScalingResolution(m_coordsRes, m_needsScaling);

  // now load in the skin file
  SetDefaults();

  CGUIControlFactory::GetInfoColor(pRootElement, "backgroundcolor", m_clearBackground, GetID());
  CGUIControlFactory::GetActions(pRootElement, "onload", m_loadActions);
  CGUIControlFactory::GetActions(pRootElement, "onunload", m_unloadActions);
  CRect parentRect(0, 0, static_cast<float>(m_coordsRes.iWidth), static_cast<float>(m_coordsRes.iHeight));
  CGUIControlFactory::GetHitRect(pRootElement, m_hitRect, parentRect);

  TiXmlElement *pChild = pRootElement->FirstChildElement();
  while (pChild)
  {
    std::string strValue = pChild->Value();
    if (strValue == "previouswindow" && pChild->FirstChild())
    {
      m_previousWindow = CWindowTranslator::TranslateWindow(pChild->FirstChild()->Value());
    }
    else if (strValue == "defaultcontrol" && pChild->FirstChild())
    {
      const char *always = pChild->Attribute("always");
      if (always && StringUtils::EqualsNoCase(always, "true"))
        m_defaultAlways = true;
      m_defaultControl = atoi(pChild->FirstChild()->Value());
    }
    else if(strValue == "menucontrol" && pChild->FirstChild())
    {
      m_menuControlID = atoi(pChild->FirstChild()->Value());
    }
    else if (strValue == "visible" && pChild->FirstChild())
    {
      std::string condition;
      CGUIControlFactory::GetConditionalVisibility(pRootElement, condition);
      m_visibleCondition = CServiceBroker::GetGUI()->GetInfoManager().Register(condition, GetID());
    }
    else if (strValue == "animation" && pChild->FirstChild())
    {
      CRect rect(0, 0, static_cast<float>(m_coordsRes.iWidth), static_cast<float>(m_coordsRes.iHeight));
      CAnimation anim;
      anim.Create(pChild, rect, GetID());
      m_animations.push_back(anim);
    }
    else if (strValue == "zorder" && pChild->FirstChild())
    {
      m_renderOrder = atoi(pChild->FirstChild()->Value());
    }
    else if (strValue == "coordinates")
    {
      XMLUtils::GetFloat(pChild, "posx", m_posX);
      XMLUtils::GetFloat(pChild, "posy", m_posY);
      XMLUtils::GetFloat(pChild, "left", m_posX);
      XMLUtils::GetFloat(pChild, "top", m_posY);

      TiXmlElement *originElement = pChild->FirstChildElement("origin");
      while (originElement)
      {
        COrigin origin;
        origin.x = CGUIControlFactory::ParsePosition(originElement->Attribute("x"), static_cast<float>(m_coordsRes.iWidth));
        origin.y = CGUIControlFactory::ParsePosition(originElement->Attribute("y"), static_cast<float>(m_coordsRes.iHeight));
        if (originElement->FirstChild())
          origin.condition = CServiceBroker::GetGUI()->GetInfoManager().Register(originElement->FirstChild()->Value(), GetID());
        m_origins.push_back(origin);
        originElement = originElement->NextSiblingElement("origin");
      }
    }
    else if (strValue == "camera")
    { // z is fixed
      m_camera.x = CGUIControlFactory::ParsePosition(pChild->Attribute("x"), static_cast<float>(m_coordsRes.iWidth));
      m_camera.y = CGUIControlFactory::ParsePosition(pChild->Attribute("y"), static_cast<float>(m_coordsRes.iHeight));
      m_hasCamera = true;
    }
    else if (strValue == "depth" && pChild->FirstChild())
    {
      float stereo = static_cast<float>(atof(pChild->FirstChild()->Value()));
      m_stereo = std::max(-1.f, std::min(1.f, stereo));
    }
    else if (strValue == "controls")
    {
      TiXmlElement *pControl = pChild->FirstChildElement();
      while (pControl)
      {
        if (StringUtils::EqualsNoCase(pControl->Value(), "control"))
        {
          LoadControl(pControl, nullptr, CRect(0, 0, static_cast<float>(m_coordsRes.iWidth), static_cast<float>(m_coordsRes.iHeight)));
        }
        pControl = pControl->NextSiblingElement();
      }
    }

    pChild = pChild->NextSiblingElement();
  }

  return true;
}

void CGUIWindow::LoadControl(TiXmlElement* pControl, CGUIControlGroup *pGroup, const CRect &rect)
{
  // get control type
  CGUIControlFactory factory;

  CGUIControl* pGUIControl = factory.Create(GetID(), rect, pControl);
  if (pGUIControl)
  {
    float maxX = pGUIControl->GetXPosition() + pGUIControl->GetWidth();
    if (maxX > m_width)
    {
      m_width = maxX;
    }

    float maxY = pGUIControl->GetYPosition() + pGUIControl->GetHeight();
    if (maxY > m_height)
    {
      m_height = maxY;
    }
    // if we are in a group, add to the group, else add to our window
    if (pGroup)
      pGroup->AddControl(pGUIControl);
    else
      AddControl(pGUIControl);
    // if the new control is a group, then add it's controls
    if (pGUIControl->IsGroup())
    {
      CGUIControlGroup *grp = static_cast<CGUIControlGroup*>(pGUIControl);
      TiXmlElement *pSubControl = pControl->FirstChildElement("control");
      CRect grpRect(grp->GetXPosition(), grp->GetYPosition(),
                    grp->GetXPosition() + grp->GetWidth(), grp->GetYPosition() + grp->GetHeight());
      while (pSubControl)
      {
        LoadControl(pSubControl, grp, grpRect);
        pSubControl = pSubControl->NextSiblingElement("control");
      }
    }
  }
}

void CGUIWindow::OnWindowLoaded()
{
  DynamicResourceAlloc(true);
}

void CGUIWindow::CenterWindow()
{
  m_posX = (m_coordsRes.iWidth - GetWidth()) / 2;
  m_posY = (m_coordsRes.iHeight - GetHeight()) / 2;
}

void CGUIWindow::DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (!IsControlDirty() && CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_guiSmartRedraw)
    return;

  CServiceBroker::GetWinSystem()->GetGfxContext().SetRenderingResolution(m_coordsRes, m_needsScaling);
  CServiceBroker::GetWinSystem()->GetGfxContext().AddGUITransform();
  CGUIControlGroup::DoProcess(currentTime, dirtyregions);
  CServiceBroker::GetWinSystem()->GetGfxContext().RemoveTransform();

  // check if currently focused control can have it
  // and fallback to default control if not
  CGUIControl* focusedControl = GetFocusedControl();
  if (focusedControl && !focusedControl->CanFocus())
    SET_CONTROL_FOCUS(m_defaultControl, 0);
}

void CGUIWindow::DoRender()
{
  // If we're rendering from a different thread, then we should wait for the main
  // app thread to finish AllocResources(), as dynamic resources (images in particular)
  // will try and be allocated from 2 different threads, which causes nasty things
  // to occur.
  if (!m_bAllocated) return;

  CServiceBroker::GetWinSystem()->GetGfxContext().SetRenderingResolution(m_coordsRes, m_needsScaling);

  CServiceBroker::GetWinSystem()->GetGfxContext().AddGUITransform();
  CGUIControlGroup::DoRender();
  CServiceBroker::GetWinSystem()->GetGfxContext().RemoveTransform();

  if (CGUIControlProfiler::IsRunning()) CGUIControlProfiler::Instance().EndFrame();
}

void CGUIWindow::AfterRender()
{
  // Check to see if we should close at this point
  // We check after the controls have finished rendering, as we may have to close due to
  // the controls rendering after the window has finished it's animation
  // we call the base class instead of this class so that we can find the change
  if (m_closing && !CGUIControlGroup::IsAnimating(ANIM_TYPE_WINDOW_CLOSE))
    Close(true);

}

void CGUIWindow::Close_Internal(bool forceClose /*= false*/, int nextWindowID /*= 0*/, bool enableSound /*= true*/)
{
  std::unique_lock<CCriticalSection> lock(CServiceBroker::GetWinSystem()->GetGfxContext());

  if (!m_active)
    return;

  forceClose |= (nextWindowID == WINDOW_FULLSCREEN_VIDEO ||
                 nextWindowID == WINDOW_FULLSCREEN_GAME);

  if (!forceClose && HasAnimation(ANIM_TYPE_WINDOW_CLOSE))
  {
    if (!m_closing)
    {
      if (enableSound && IsSoundEnabled())
        CServiceBroker::GetGUI()->GetAudioManager().PlayWindowSound(GetID(), SOUND_DEINIT);

      // Perform the window out effect
      QueueAnimation(ANIM_TYPE_WINDOW_CLOSE);
      m_closing = true;
    }
    return;
  }

  m_closing = false;
  CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0, nextWindowID);
  OnMessage(msg);
}

void CGUIWindow::Close(bool forceClose /*= false*/, int nextWindowID /*= 0*/, bool enableSound /*= true*/, bool bWait /* = true */)
{
  if (!CServiceBroker::GetAppMessenger()->IsProcessThread())
  {
    // make sure graphics lock is not held
    CSingleExit leaveIt(CServiceBroker::GetWinSystem()->GetGfxContext());
    int param2 = (forceClose ? 0x01 : 0) | (enableSound ? 0x02 : 0);
    if (bWait)
      CServiceBroker::GetAppMessenger()->SendMsg(TMSG_GUI_WINDOW_CLOSE, nextWindowID, param2,
                                                 static_cast<void*>(this));
    else
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_GUI_WINDOW_CLOSE, nextWindowID, param2,
                                                 static_cast<void*>(this));
  }
  else
    Close_Internal(forceClose, nextWindowID, enableSound);
}

bool CGUIWindow::OnAction(const CAction &action)
{
  if (action.IsMouse() || action.IsGesture())
    return EVENT_RESULT_UNHANDLED != OnMouseAction(action);

  CGUIControl *focusedControl = GetFocusedControl();
  if (focusedControl)
  {
    while (focusedControl && focusedControl != this)
    {
      if (focusedControl->OnAction(action))
        return true;
      focusedControl = focusedControl->GetParentControl();
    }
  }
  else
  {
    // no control has focus?
    // set focus to the default control then
    CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), m_defaultControl);
    OnMessage(msg);
  }

  // default implementations
  switch(action.GetID())
  {
    case ACTION_NAV_BACK:
    case ACTION_PREVIOUS_MENU:
      return OnBack(action.GetID());
    case ACTION_SHOW_INFO:
      return OnInfo(action.GetID());
    case ACTION_MENU:
      if (m_menuControlID > 0)
      {
        CGUIControl *menu = GetControl(m_menuControlID);
        if (menu)
        {
          int focusControlId;
          if (!menu->HasFocus())
          {
            // focus the menu control
            focusControlId = m_menuControlID;
            // To support a toggle behaviour we store the last focused control id
            // to restore (focus) this control if the menu control has the focus
            // while you press the menu button again.
            m_menuLastFocusedControlID = GetFocusedControlID();
          }
          else
          {
            // restore the last focused control or if not exists use the default control
            focusControlId = m_menuLastFocusedControlID > 0 ? m_menuLastFocusedControlID : m_defaultControl;
          }

          CGUIMessage msg = CGUIMessage(GUI_MSG_SETFOCUS, GetID(), focusControlId);
          return OnMessage(msg);
        }
      }
      break;
  }

  return false;
}

CPoint CGUIWindow::GetPosition() const
{
  for (unsigned int i = 0; i < m_origins.size(); i++)
  {
    // no condition implies true
    if (!m_origins[i].condition || m_origins[i].condition->Get(INFO::DEFAULT_CONTEXT))
    { // found origin
      return CPoint(m_origins[i].x, m_origins[i].y);
    }
  }
  return CGUIControlGroup::GetPosition();
}

// OnMouseAction - called by OnAction()
EVENT_RESULT CGUIWindow::OnMouseAction(const CAction &action)
{
  CServiceBroker::GetWinSystem()->GetGfxContext().SetScalingResolution(m_coordsRes, m_needsScaling);
  CPoint mousePoint(action.GetAmount(0), action.GetAmount(1));
  CServiceBroker::GetWinSystem()->GetGfxContext().InvertFinalCoords(mousePoint.x, mousePoint.y);

  // create the mouse event
  MOUSE::CMouseEvent event(action.GetID(), action.GetHoldTime(), action.GetAmount(2),
                           action.GetAmount(3));
  if (m_exclusiveMouseControl)
  {
    CGUIControl *child = GetControl(m_exclusiveMouseControl);
    if (child)
    {
      CPoint renderPos = child->GetRenderPosition() - CPoint(child->GetXPosition(), child->GetYPosition());
      return child->OnMouseEvent(mousePoint - renderPos, event);
    }
  }

  UnfocusFromPoint(mousePoint);

  return SendMouseEvent(mousePoint, event);
}

EVENT_RESULT CGUIWindow::OnMouseEvent(const CPoint& point, const MOUSE::CMouseEvent& event)
{
  if (event.m_id == ACTION_MOUSE_RIGHT_CLICK)
  { // no control found to absorb this click - go to previous menu
    return OnAction(CAction(ACTION_PREVIOUS_MENU)) ? EVENT_RESULT_HANDLED : EVENT_RESULT_UNHANDLED;
  }
  return EVENT_RESULT_UNHANDLED;
}

/// \brief Called on window open.
///  * Restores the control state(s)
///  * Sets initial visibility of controls
///  * Queue WindowOpen animation
///  * Set overlay state
/// Override this function and do any window-specific initialisation such
/// as filling control contents and setting control focus before
/// calling the base method.
void CGUIWindow::OnInitWindow()
{
  //  Play the window specific init sound
  if (IsSoundEnabled())
    CServiceBroker::GetGUI()->GetAudioManager().PlayWindowSound(GetID(), SOUND_INIT);

  // set our rendered state
  m_hasProcessed = false;
  m_closing = false;
  m_active = true;
  ResetAnimations();  // we need to reset our animations as those windows that don't dynamically allocate
                      // need their anims reset. An alternative solution is turning off all non-dynamic
                      // allocation (which in some respects may be nicer, but it kills hdd spindown and the like)

  // set our initial control visibility before restoring control state and
  // focusing the default control, and again afterward to make sure that
  // any controls that depend on the state of the focused control (and or on
  // control states) are active.
  SetInitialVisibility();
  RestoreControlStates();
  SetInitialVisibility();
  QueueAnimation(ANIM_TYPE_WINDOW_OPEN);

  if (!m_manualRunActions)
  {
    RunLoadActions();
  }
}

// Called on window close.
//  * Saves control state(s)
// Override this function and call the base class before doing any dynamic memory freeing
void CGUIWindow::OnDeinitWindow(int nextWindowID)
{
  if (!m_manualRunActions)
  {
    RunUnloadActions();
  }

  SaveControlStates();
  m_active = false;
}

bool CGUIWindow::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_LOAD:
    {
      Initialize();
      return true;
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      CLog::Log(LOGDEBUG, "------ Window Init ({}) ------", GetProperty("xmlfile").asString());
      if (m_dynamicResourceAlloc || !m_bAllocated) AllocResources(false);
      OnInitWindow();
      return true;
    }
    break;

  case GUI_MSG_WINDOW_DEINIT:
    {
      CLog::Log(LOGDEBUG, "------ Window Deinit ({}) ------", GetProperty("xmlfile").asString());
      OnDeinitWindow(message.GetParam1());
      // now free the window
      if (m_dynamicResourceAlloc) FreeResources();
      return true;
    }
    break;

  case GUI_MSG_UNFOCUS_ALL:
    {
      //unfocus the current focused control in this window
      CGUIControl *control = GetFocusedControl();
      if(control)
      {
        //tell focused control that it has lost the focus
        CGUIMessage msgLostFocus(GUI_MSG_LOSTFOCUS, GetID(), control->GetID(), control->GetID());
        control->OnMessage(msgLostFocus);
        CLog::Log(LOGDEBUG, "Unfocus WindowID: {}, ControlID: {}", GetID(), control->GetID());
      }
      return true;
    }

  case GUI_MSG_FOCUSED:
    { // a control has been focused
      if (HasID(message.GetSenderId()))
      {
        m_focusedControl = message.GetControlId();
        // We ensure that all others childrens have focus disabled,
        // this can happen when one control overlap the other one in the same
        // coordinates with similar sizes and the mouse pointer is over them
        // in this case only the control in the highest layer will have the focus
        for (CGUIControl* control : m_children)
        {
          if (control->GetID() != m_focusedControl)
            control->SetFocus(false);
        }
        return true;
      }
      break;
    }
  case GUI_MSG_LOSTFOCUS:
    {
      // nothing to do at the window level when we lose focus
      return true;
    }
  case GUI_MSG_MOVE:
    {
      if (HasID(message.GetSenderId()))
        return OnMove(message.GetControlId(), message.GetParam1());
      break;
    }
  case GUI_MSG_SETFOCUS:
    {
      //      CLog::Log(LOGDEBUG,"set focus to control:{} window:{} ({})", message.GetControlId(),message.GetSenderId(), GetID());
      if ( message.GetControlId() )
      {
        // first unfocus the current control
        CGUIControl *control = GetFocusedControl();
        if (control)
        {
          CGUIMessage msgLostFocus(GUI_MSG_LOSTFOCUS, GetID(), control->GetID(), message.GetControlId());
          control->OnMessage(msgLostFocus);
        }

        // get the control to focus
        CGUIControl* pFocusedControl = GetFirstFocusableControl(message.GetControlId());
        if (!pFocusedControl) pFocusedControl = GetControl(message.GetControlId());

        // and focus it
        if (pFocusedControl)
          return pFocusedControl->OnMessage(message);
      }
      return true;
    }
    break;
  case GUI_MSG_EXCLUSIVE_MOUSE:
    {
      m_exclusiveMouseControl = message.GetSenderId();
      return true;
    }
    break;
  case GUI_MSG_GESTURE_NOTIFY:
    {
      CAction action(ACTION_GESTURE_NOTIFY, 0, static_cast<float>(message.GetParam1()), static_cast<float>(message.GetParam2()), 0, 0);
      EVENT_RESULT result = OnMouseAction(action);
      auto res = new int(result);
      message.SetPointer(static_cast<void*>(res));
      return result != EVENT_RESULT_UNHANDLED;
    }
  case GUI_MSG_ADD_CONTROL:
    {
      if (message.GetPointer())
      {
        CGUIControl *control = static_cast<CGUIControl*>(message.GetPointer());
        control->AllocResources();
        AddControl(control);
      }
      return true;
    }
  case GUI_MSG_REMOVE_CONTROL:
    {
      if (message.GetPointer())
      {
        CGUIControl *control = static_cast<CGUIControl*>(message.GetPointer());
        RemoveControl(control);
        control->FreeResources(true);
        delete control;
      }
      return true;
    }
  case GUI_MSG_NOTIFY_ALL:
    {
      // only process those notifications that come from this window, or those intended for every window
      if (HasID(message.GetSenderId()) || !message.GetSenderId())
      {
        if (message.GetParam1() == GUI_MSG_PAGE_CHANGE ||
          message.GetParam1() == GUI_MSG_REFRESH_THUMBS ||
          message.GetParam1() == GUI_MSG_REFRESH_LIST ||
          message.GetParam1() == GUI_MSG_WINDOW_RESIZE)
        { // alter the message accordingly, and send to all controls
          for (iControls it = m_children.begin(); it != m_children.end(); ++it)
          {
            CGUIControl *control = *it;
            CGUIMessage msg(message.GetParam1(), message.GetControlId(), control->GetID(), message.GetParam2());
            control->OnMessage(msg);
          }
        }
        else if (message.GetParam1() == GUI_MSG_STATE_CHANGED)
          MarkDirtyRegion(DIRTY_STATE_CHILD); //Don't force an dirtyRect, we don't know if / what has changed.
      }
    }
    break;
  }

  return CGUIControlGroup::OnMessage(message);
}

bool CGUIWindow::NeedLoad() const
{
  return !m_windowLoaded || CServiceBroker::GetGUI()->GetInfoManager().ConditionsChangedValues(m_xmlIncludeConditions);
}

void CGUIWindow::AllocResources(bool forceLoad /*= false */)
{
  std::unique_lock<CCriticalSection> lock(CServiceBroker::GetWinSystem()->GetGfxContext());

#ifdef _DEBUG
  const auto start = std::chrono::steady_clock::now();
#endif
  // use forceLoad to determine if window needs (re)loading
  forceLoad |= NeedLoad() || (m_loadType == LOAD_EVERY_TIME);

  // if window is loaded and load is forced we have to free window resources first
  if (m_windowLoaded && forceLoad)
    FreeResources(true);

  if (forceLoad)
  {
    std::string xmlFile = GetProperty("xmlfile").asString();
    if (xmlFile.size())
    {
      bool bHasPath =
          xmlFile.find('\\') != std::string::npos || xmlFile.find('/') != std::string::npos;
      Load(xmlFile, bHasPath);
    }
  }

#ifdef _DEBUG
  const auto skinLoadEnd = std::chrono::steady_clock::now();
#endif

  // and now allocate resources
  CGUIControlGroup::AllocResources();

#ifdef _DEBUG
  const auto end = std::chrono::steady_clock::now();
  const std::chrono::duration<double, std::milli> skinLoadDuration = skinLoadEnd - start;
  const std::chrono::duration<double, std::milli> duration = end - start;

  if (forceLoad)
    CLog::Log(LOGDEBUG, "Alloc resources: {:.2f} ms ({:.2f} ms skin load)", duration.count(),
              skinLoadDuration.count());
  else
  {
    CLog::Log(LOGDEBUG, "Window {} was already loaded", GetProperty("xmlfile").asString());
    CLog::Log(LOGDEBUG, "Alloc resources: {:.2f} ms", duration.count());
  }
#endif
  m_bAllocated = true;
}

void CGUIWindow::FreeResources(bool forceUnload /*= false */)
{
  m_bAllocated = false;
  CGUIControlGroup::FreeResources();
  //CServiceBroker::GetGUI()->GetTextureManager().Dump();
  // unload the skin
  if (m_loadType == LOAD_EVERY_TIME || forceUnload) ClearAll();
  if (forceUnload)
  {
    m_windowXMLRootElement.reset();
    m_xmlIncludeConditions.clear();
  }
}

void CGUIWindow::DynamicResourceAlloc(bool bOnOff)
{
  m_dynamicResourceAlloc = bOnOff;
  CGUIControlGroup::DynamicResourceAlloc(bOnOff);
}

void CGUIWindow::ClearAll()
{
  OnWindowUnload();
  CGUIControlGroup::ClearAll();
  m_windowLoaded = false;
  m_dynamicResourceAlloc = true;
  m_visibleCondition.reset();
}

bool CGUIWindow::Initialize()
{
  if (!CServiceBroker::GetGUI()->GetWindowManager().Initialized())
    return false;
  if (!NeedLoad())
    return true;
  if (CServiceBroker::GetAppMessenger()->IsProcessThread())
    AllocResources(false);
  else
  {
    // if not app thread, send gui msg via app messenger
    // and wait for results, so windowLoaded flag would be updated
    CGUIMessage msg(GUI_MSG_WINDOW_LOAD, 0, 0);
    CServiceBroker::GetAppMessenger()->SendGUIMessage(msg, GetID(), true);
  }
  return m_windowLoaded;
}

void CGUIWindow::SetInitialVisibility()
{
  // reset our info manager caches
  CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();
  infoMgr.ResetCache();
  infoMgr.GetInfoProviders().GetGUIControlsInfoProvider().ResetContainerMovingCache();
  CGUIControlGroup::SetInitialVisibility();
}

bool CGUIWindow::IsActive() const
{
  return CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(GetID());
}

bool CGUIWindow::CheckAnimation(ANIMATION_TYPE animType)
{
  // special cases first
  if (animType == ANIM_TYPE_WINDOW_CLOSE)
  {
    if (!m_bAllocated || !HasProcessed()) // can't process an animation if we aren't allocated or haven't processed
      return false;
    // make sure we update our visibility prior to queuing the window close anim
    for (unsigned int i = 0; i < m_children.size(); i++)
      m_children[i]->UpdateVisibility(nullptr);
  }
  return true;
}

bool CGUIWindow::IsAnimating(ANIMATION_TYPE animType)
{
  if (!m_animationsEnabled)
    return false;
  if (animType == ANIM_TYPE_WINDOW_CLOSE)
    return m_closing;
  return CGUIControlGroup::IsAnimating(animType);
}

bool CGUIWindow::Animate(unsigned int currentTime)
{
  if (m_animationsEnabled)
    return CGUIControlGroup::Animate(currentTime);
  else
  {
    m_transform.Reset();
    return false;
  }
}

void CGUIWindow::DisableAnimations()
{
  m_animationsEnabled = false;
}

// returns true if the control group with id groupID has controlID as
// its focused control
bool CGUIWindow::ControlGroupHasFocus(int groupID, int controlID)
{
  // 1.  Run through and get control with groupID (assume unique)
  // 2.  Get it's selected item.
  CGUIControl *group = GetFirstFocusableControl(groupID);
  if (!group) group = GetControl(groupID);

  if (group && group->IsGroup())
  {
    if (controlID == 0)
    { // just want to know if the group is focused
      return group->HasFocus();
    }
    else
    {
      CGUIMessage message(GUI_MSG_ITEM_SELECTED, GetID(), group->GetID());
      group->OnMessage(message);
      return (controlID == message.GetParam1());
    }
  }
  return false;
}

void CGUIWindow::SaveControlStates()
{
  ResetControlStates();
  if (!m_defaultAlways)
    m_lastControlID = GetFocusedControlID();
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
    (*it)->SaveStates(m_controlStates);
}

void CGUIWindow::RestoreControlStates()
{
  for (const auto& it : m_controlStates)
  {
    CGUIMessage message(GUI_MSG_ITEM_SELECT, GetID(), it.m_id, it.m_data);
    OnMessage(message);
  }
  int focusControl = (!m_defaultAlways && m_lastControlID) ? m_lastControlID : m_defaultControl;
  SET_CONTROL_FOCUS(focusControl, 0);
}

void CGUIWindow::ResetControlStates()
{
  m_lastControlID = 0;
  m_focusedControl = 0;
  m_controlStates.clear();
}

bool CGUIWindow::OnBack(int actionID)
{
  CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();
  return true;
}

bool CGUIWindow::OnMove(int fromControl, int moveAction)
{
  const CGUIControl *control = GetFirstFocusableControl(fromControl);
  if (!control) control = GetControl(fromControl);
  if (!control)
  { // no current control??
    CLog::Log(LOGERROR, "Unable to find control {} in window {}", fromControl, GetID());
    return false;
  }
  std::vector<int> moveHistory;
  int nextControl = fromControl;
  while (control)
  { // grab the next control direction
    moveHistory.push_back(nextControl);
    CGUIAction action = control->GetAction(moveAction);
    action.ExecuteActions(nextControl, GetParentID());
    nextControl = action.GetNavigation();
    if (!nextControl) // 0 isn't valid control id
      return false;
    // check our history - if the nextControl is in it, we can't focus it
    for (unsigned int i = 0; i < moveHistory.size(); i++)
    {
      if (nextControl == moveHistory[i])
        return false; // no control to focus so do nothing
    }
    control = GetFirstFocusableControl(nextControl);
    if (control)
      break;  // found a focusable control
    control = GetControl(nextControl); // grab the next control and try again
  }
  if (!control)
    return false;   // no control to focus
  // if we get here we have our new control so focus it (and unfocus the current control)
  SET_CONTROL_FOCUS(nextControl, 0);
  return true;
}

void CGUIWindow::SetDefaults()
{
  m_renderOrder = RENDER_ORDER_WINDOW;
  m_defaultAlways = false;
  m_defaultControl = 0;
  m_posX = m_posY = m_width = m_height = 0;
  m_previousWindow = WINDOW_INVALID;
  m_animations.clear();
  m_origins.clear();
  m_hasCamera = false;
  m_stereo = 0.f;
  m_animationsEnabled = true;
  m_clearBackground = 0xff000000; // opaque black -> clear
  m_hitRect.SetRect(0, 0, static_cast<float>(m_coordsRes.iWidth), static_cast<float>(m_coordsRes.iHeight));
  m_menuControlID = 0;
  m_menuLastFocusedControlID = 0;
}

CRect CGUIWindow::GetScaledBounds() const
{
  std::unique_lock<CCriticalSection> lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  CServiceBroker::GetWinSystem()->GetGfxContext().SetScalingResolution(m_coordsRes, m_needsScaling);
  CPoint pos(GetPosition());
  CRect rect(pos.x, pos.y, pos.x + m_width, pos.y + m_height);
  float z = 0;
  CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalCoords(rect.x1, rect.y1, z);
  CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalCoords(rect.x2, rect.y2, z);
  return rect;
}

void CGUIWindow::OnEditChanged(int id, std::string &text)
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), id);
  OnMessage(msg);
  text = msg.GetLabel();
}

bool CGUIWindow::SendMessage(int message, int id, int param1 /* = 0*/, int param2 /* = 0*/)
{
  CGUIMessage msg(message, GetID(), id, param1, param2);
  return OnMessage(msg);
}
#ifdef _DEBUG
void CGUIWindow::DumpTextureUse()
{
  CLog::Log(LOGDEBUG, "{} for window {}", __FUNCTION__, GetID());
  CGUIControlGroup::DumpTextureUse();
}
#endif

void CGUIWindow::SetProperty(const std::string &strKey, const CVariant &value)
{
  std::unique_lock<CCriticalSection> lock(*this);
  m_mapProperties[strKey] = value;
}

CVariant CGUIWindow::GetProperty(const std::string &strKey) const
{
  std::unique_lock<CCriticalSection> lock(const_cast<CGUIWindow&>(*this));
  std::map<std::string, CVariant, icompare>::const_iterator iter = m_mapProperties.find(strKey);
  if (iter == m_mapProperties.end())
    return CVariant(CVariant::VariantTypeNull);

  return iter->second;
}

void CGUIWindow::ClearProperties()
{
  std::unique_lock<CCriticalSection> lock(*this);
  m_mapProperties.clear();
}

void CGUIWindow::SetRunActionsManually()
{
  m_manualRunActions = true;
}

void CGUIWindow::RunLoadActions() const
{
  m_loadActions.ExecuteActions(GetID(), GetParentID());
}

void CGUIWindow::RunUnloadActions() const
{
  m_unloadActions.ExecuteActions(GetID(), GetParentID());
}

void CGUIWindow::ClearBackground()
{
  m_clearBackground.Update();
  UTILS::COLOR::Color color = m_clearBackground;
  if (color)
    CServiceBroker::GetWinSystem()->GetGfxContext().Clear(color);
  else if (!CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_guiGeometryClear)
    CServiceBroker::GetWinSystem()->GetGfxContext().Clear(0xff000000);
  else
    CServiceBroker::GetWinSystem()->GetGfxContext().Clear();
}

void CGUIWindow::SetID(int id)
{
  CGUIControlGroup::SetID(id);
  m_idRange.clear();
  m_idRange.push_back(id);
}

bool CGUIWindow::HasID(int controlID) const
{
  for (const auto& it : m_idRange)
  {
    if (controlID == it)
      return true;
  }
  return false;
}
