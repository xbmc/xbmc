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
#include "GUIWindow.h"
#include "GUIWindowManager.h"
#include "LocalizeStrings.h"
#include "TextureManager.h"
#include "Settings.h"
#include "GUIControlFactory.h"
#include "GUIControlGroup.h"
#ifdef PRE_SKIN_VERSION_9_10_COMPATIBILITY
#include "GUIEditControl.h"
#endif

#include "SkinInfo.h"
#include "utils/GUIInfoManager.h"
#include "utils/SingleLock.h"
#include "ButtonTranslator.h"
#include "XMLUtils.h"

#ifdef HAS_PERFORMANCE_SAMPLE
#include "utils/PerformanceSample.h"
#endif

using namespace std;

CGUIWindow::CGUIWindow(DWORD dwID, const CStdString &xmlFile)
{
  SetID(dwID);
  m_xmlFile = xmlFile;
  m_dwIDRange = 1;
  m_saveLastControl = false;
  m_lastControlID = 0;
  m_bRelativeCoords = false;
  m_overlayState = OVERLAY_STATE_PARENT_WINDOW;   // Use parent or previous window's state
  m_coordsRes = g_guiSettings.m_LookAndFeelResolution;
  m_isDialog = false;
  m_needsScaling = true;
  m_windowLoaded = false;
  m_loadOnDemand = true;
  m_renderOrder = 0;
  m_dynamicResourceAlloc = true;
  m_previousWindow = WINDOW_INVALID;
  m_animationsEnabled = true;
}

CGUIWindow::~CGUIWindow(void)
{}

bool CGUIWindow::Load(const CStdString& strFileName, bool bContainsPath)
{
#ifdef HAS_PERFORMANCE_SAMPLE
  CPerformanceSample aSample("WindowLoad-" + strFileName, true);
#endif

  if (m_windowLoaded)
    return true;      // no point loading if it's already there

  LARGE_INTEGER start;
  QueryPerformanceCounter(&start);

  RESOLUTION resToUse = INVALID;
  CLog::Log(LOGINFO, "Loading skin file: %s", strFileName.c_str());
  TiXmlDocument xmlDoc;
  // Find appropriate skin folder + resolution to load from
  CStdString strPath;
  CStdString strLowerPath;
  if (bContainsPath)
    strPath = strFileName;
  else
  {
    // FIXME: strLowerPath needs to eventually go since resToUse can get incorrectly overridden
    strLowerPath =  g_SkinInfo.GetSkinPath(CStdString(strFileName).ToLower(), &resToUse);
    strPath = g_SkinInfo.GetSkinPath(strFileName, &resToUse);
  }

  if (!bContainsPath)
    m_coordsRes = resToUse;

  bool ret = LoadXML(strPath.c_str(), strLowerPath.c_str());

  LARGE_INTEGER end, freq;
  QueryPerformanceCounter(&end);
  QueryPerformanceFrequency(&freq);
  CLog::Log(LOGDEBUG,"Load %s: %.2fms", m_xmlFile.c_str(), 1000.f * (end.QuadPart - start.QuadPart) / freq.QuadPart);

  return ret;
}

bool CGUIWindow::LoadXML(const CStdString &strPath, const CStdString &strLowerPath)
{
  TiXmlDocument xmlDoc;
  if ( !xmlDoc.LoadFile(strPath) && !xmlDoc.LoadFile(CStdString(strPath).ToLower()) && !xmlDoc.LoadFile(strLowerPath))
  {
    CLog::Log(LOGERROR, "unable to load:%s, Line %d\n%s", strPath.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    SetID(WINDOW_INVALID);
    return false;
  }

  return Load(xmlDoc);
}

bool CGUIWindow::Load(TiXmlDocument &xmlDoc)
{
  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if (strcmpi(pRootElement->Value(), "window"))
  {
    CLog::Log(LOGERROR, "file : XML file doesnt contain <window>");
    return false;
  }

  // set the scaling resolution so that any control creation or initialisation can
  // be done with respect to the correct aspect ratio
  g_graphicsContext.SetScalingResolution(m_coordsRes, 0, 0, m_needsScaling);

  // Resolve any includes that may be present
  g_SkinInfo.ResolveIncludes(pRootElement);
  // now load in the skin file
  SetDefaults();

  TiXmlElement *pChild = pRootElement->FirstChildElement();
  while (pChild)
  {
    CStdString strValue = pChild->Value();
    if (strValue == "type" && pChild->FirstChild())
    {
      // if we have are a window type (ie not a dialog), and we have <type>dialog</type>
      // then make this window act like a dialog
      if (!IsDialog() && strcmpi(pChild->FirstChild()->Value(), "dialog") == 0)
        m_isDialog = true;
    }
    else if (strValue == "previouswindow" && pChild->FirstChild())
    {
      m_previousWindow = g_buttonTranslator.TranslateWindowString(pChild->FirstChild()->Value());
    }
    else if (strValue == "defaultcontrol" && pChild->FirstChild())
    {
      const char *always = pChild->Attribute("always");
      if (always && strcmpi(always, "true") == 0)
        m_saveLastControl = false;
      m_defaultControl = atoi(pChild->FirstChild()->Value());
    }
    else if (strValue == "visible" && pChild->FirstChild())
    {
      CGUIControlFactory::GetConditionalVisibility(pRootElement, m_visibleCondition);
    }
    else if (strValue == "animation" && pChild->FirstChild())
    {
      FRECT rect = { 0, 0, (float)g_settings.m_ResInfo[m_coordsRes].iWidth, (float)g_settings.m_ResInfo[m_coordsRes].iHeight };
      CAnimation anim;
      anim.Create(pChild, rect);
      m_animations.push_back(anim);
    }
    else if (strValue == "zorder" && pChild->FirstChild())
    {
      m_renderOrder = atoi(pChild->FirstChild()->Value());
    }
    else if (strValue == "coordinates")
    {
      // resolve any includes within coordinates tag (such as multiple origin includes)
      g_SkinInfo.ResolveIncludes(pChild);
      TiXmlNode* pSystem = pChild->FirstChild("system");
      if (pSystem)
      {
        int iCoordinateSystem = atoi(pSystem->FirstChild()->Value());
        m_bRelativeCoords = (iCoordinateSystem == 1);
      }

      CGUIControlFactory::GetFloat(pChild, "posx", m_posX);
      CGUIControlFactory::GetFloat(pChild, "posy", m_posY);

      TiXmlElement *originElement = pChild->FirstChildElement("origin");
      while (originElement)
      {
        COrigin origin;
        g_SkinInfo.ResolveConstant(originElement->Attribute("x"), origin.x);
        g_SkinInfo.ResolveConstant(originElement->Attribute("y"), origin.y);
        if (originElement->FirstChild())
          origin.condition = g_infoManager.TranslateString(originElement->FirstChild()->Value());
        m_origins.push_back(origin);
        originElement = originElement->NextSiblingElement("origin");
      }
    }
    else if (strValue == "camera")
    { // z is fixed
      g_SkinInfo.ResolveConstant(pChild->Attribute("x"), m_camera.x);
      g_SkinInfo.ResolveConstant(pChild->Attribute("y"), m_camera.y);
      m_hasCamera = true;
    }
    else if (strValue == "controls")
    {
      // resolve any includes within controls tag (such as whole <control> includes)
      g_SkinInfo.ResolveIncludes(pChild);

      TiXmlElement *pControl = pChild->FirstChildElement();
      while (pControl)
      {
        if (strcmpi(pControl->Value(), "control") == 0)
        {
          LoadControl(pControl, NULL);
        }
        else if (strcmpi(pControl->Value(), "controlgroup") == 0)
        {
          // backward compatibility...
          LoadControl(pControl, NULL);
        }
        pControl = pControl->NextSiblingElement();
      }
    }
    else if (strValue == "allowoverlay")
    {
      bool overlay = false;
      if (XMLUtils::GetBoolean(pRootElement, "allowoverlay", overlay))
        m_overlayState = overlay ? OVERLAY_STATE_SHOWN : OVERLAY_STATE_HIDDEN;
    }

    pChild = pChild->NextSiblingElement();
  }
  LoadAdditionalTags(pRootElement);

  m_windowLoaded = true;
  OnWindowLoaded();
  return true;
}

void CGUIWindow::LoadControl(TiXmlElement* pControl, CGUIControlGroup *pGroup)
{
  // get control type
  CGUIControlFactory factory;

  FRECT rect = { 0, 0, (float)g_settings.m_ResInfo[m_coordsRes].iWidth, (float)g_settings.m_ResInfo[m_coordsRes].iHeight };
  if (pGroup)
  {
    rect.left = pGroup->GetXPosition();
    rect.top = pGroup->GetYPosition();
    rect.right = rect.left + pGroup->GetWidth();
    rect.bottom = rect.top + pGroup->GetHeight();
  }
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
      TiXmlElement *pSubControl = pControl->FirstChildElement("control");
      while (pSubControl)
      {
        LoadControl(pSubControl, (CGUIControlGroup *)pGUIControl);
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
  if (m_bRelativeCoords)
  {
    m_posX = (g_settings.m_ResInfo[m_coordsRes].iWidth - GetWidth()) / 2;
    m_posY = (g_settings.m_ResInfo[m_coordsRes].iHeight - GetHeight()) / 2;
  }
}

void CGUIWindow::Render()
{
  // If we're rendering from a different thread, then we should wait for the main
  // app thread to finish AllocResources(), as dynamic resources (images in particular)
  // will try and be allocated from 2 different threads, which causes nasty things
  // to occur.
  if (!m_bAllocated) return;

  // find our origin point
  float posX = m_posX;
  float posY = m_posY;
  for (unsigned int i = 0; i < m_origins.size(); i++)
  {
    // no condition implies true
    if (!m_origins[i].condition || g_infoManager.GetBool(m_origins[i].condition, GetID()))
    { // found origin
      posX = m_origins[i].x;
      posY = m_origins[i].y;
      break;
    }
  }
  g_graphicsContext.SetRenderingResolution(m_coordsRes, posX, posY, m_needsScaling);
  if (m_hasCamera)
    g_graphicsContext.SetCameraPosition(m_camera);

  DWORD currentTime = timeGetTime();
  // render our window animation - returns false if it needs to stop rendering
  if (!RenderAnimation(currentTime))
    return;

  for (iControls i = m_children.begin(); i != m_children.end(); ++i)
  {
    CGUIControl *pControl = *i;
    if (pControl)
    {
      pControl->UpdateVisibility();
      pControl->DoRender(currentTime);
    }
  }
  m_hasRendered = true;
}

void CGUIWindow::Close(bool forceClose)
{
  CLog::Log(LOGERROR,"%s - should never be called on the base class!", __FUNCTION__);
}

bool CGUIWindow::OnAction(const CAction &action)
{
  if (action.wID == ACTION_MOUSE)
    return OnMouseAction();

  CGUIControl *focusedControl = GetFocusedControl();
  if (focusedControl)
    return focusedControl->OnAction(action);

  // no control has focus?
  // set focus to the default control then
  CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), m_defaultControl);
  OnMessage(msg);
  return false;
}

// OnMouseAction - called by OnAction()
bool CGUIWindow::OnMouseAction()
{
  // we need to convert the mouse coordinates to window coordinates
  float posX = m_posX;
  float posY = m_posY;
  for (unsigned int i = 0; i < m_origins.size(); i++)
  {
    // no condition implies true
    if (!m_origins[i].condition || g_infoManager.GetBool(m_origins[i].condition, GetID()))
    { // found origin
      posX = m_origins[i].x;
      posY = m_origins[i].y;
      break;
    }
  }
  g_graphicsContext.SetScalingResolution(m_coordsRes, posX, posY, m_needsScaling);
  CPoint mousePoint(g_Mouse.GetLocation());
  g_graphicsContext.InvertFinalCoords(mousePoint.x, mousePoint.y);
  m_transform.InverseTransformPosition(mousePoint.x, mousePoint.y);

  bool bHandled = false;
  // check if we have exclusive access
  if (g_Mouse.GetExclusiveWindowID() == GetID())
  { // we have exclusive access to the mouse...
    CGUIControl *pControl = (CGUIControl *)GetControl(g_Mouse.GetExclusiveControlID());
    if (pControl)
    { // this control has exclusive access to the mouse
      HandleMouse(pControl, mousePoint + g_Mouse.GetExclusiveOffset());
      return true;
    }
  }

  // run through the controls, and unfocus all those that aren't under the pointer,
  for (iControls i = m_children.begin(); i != m_children.end(); ++i)
  {
    CGUIControl *pControl = *i;
    pControl->UnfocusFromPoint(mousePoint);
  }
  // and find which one is under the pointer
  // go through in reverse order to make sure we start with the ones on top
  bool controlUnderPointer(false);
  for (vector<CGUIControl *>::reverse_iterator i = m_children.rbegin(); i != m_children.rend(); ++i)
  {
    CGUIControl *pControl = *i;
    CGUIControl *focusableControl = NULL;
    CPoint controlPoint;
    if (pControl->CanFocusFromPoint(mousePoint, &focusableControl, controlPoint))
    {
      controlUnderPointer = focusableControl->OnMouseOver(controlPoint);
      bHandled = HandleMouse(focusableControl, controlPoint);
      if (bHandled || controlUnderPointer)
        break;
    }
  }
  if (!bHandled)
  { // haven't handled this action - call the window message handlers
    bHandled = OnMouse(mousePoint);
  }
  // and unfocus everything otherwise
  if (!controlUnderPointer)
    m_focusedControl = 0;
  return bHandled;
}

// Handles any mouse actions that are not handled by a control
// default is to go back a window on a right click.
// This function should be overridden for other windows
bool CGUIWindow::OnMouse(const CPoint &point)
{
  if (g_Mouse.bClick[MOUSE_RIGHT_BUTTON])
  { // no control found to absorb this click - go to previous menu
    CAction action;
    action.wID = ACTION_PREVIOUS_MENU;
    return OnAction(action);
  }
  return false;
}

bool CGUIWindow::HandleMouse(CGUIControl *pControl, const CPoint &point)
{
  if (g_Mouse.bClick[MOUSE_LEFT_BUTTON])
  { // Left click
    return pControl->OnMouseClick(MOUSE_LEFT_BUTTON, point);
  }
  else if (g_Mouse.bClick[MOUSE_RIGHT_BUTTON])
  { // Right click
    return pControl->OnMouseClick(MOUSE_RIGHT_BUTTON, point);
  }
  else if (g_Mouse.bClick[MOUSE_MIDDLE_BUTTON])
  { // Middle click
    return pControl->OnMouseClick(MOUSE_MIDDLE_BUTTON, point);
  }
  else if (g_Mouse.bDoubleClick[MOUSE_LEFT_BUTTON])
  { // Left double click
    return pControl->OnMouseDoubleClick(MOUSE_LEFT_BUTTON, point);
  }
  else if (g_Mouse.bHold[MOUSE_LEFT_BUTTON] && g_Mouse.HasMoved())
  { // Mouse Drag
    return pControl->OnMouseDrag(g_Mouse.GetLastMove(), point);
  }
  else if (g_Mouse.GetWheel())
  { // Mouse wheel
    return pControl->OnMouseWheel(g_Mouse.GetWheel(), point);
  }
  // no mouse stuff done other than movement
  return false;
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
  // set our rendered state
  m_hasRendered = false;
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
  m_gWindowManager.ShowOverlay(m_overlayState);
}

// Called on window close.
//  * Executes the window close animation(s)
//  * Saves control state(s)
// Override this function and call the base class before doing any dynamic memory freeing
void CGUIWindow::OnDeinitWindow(int nextWindowID)
{
  if (nextWindowID != WINDOW_FULLSCREEN_VIDEO)
  {
    // Dialog animations are handled in Close() rather than here
    if (HasAnimation(ANIM_TYPE_WINDOW_CLOSE) && !IsDialog() && IsActive())
    {
      // Perform the window out effect
      QueueAnimation(ANIM_TYPE_WINDOW_CLOSE);
      while (IsAnimating(ANIM_TYPE_WINDOW_CLOSE))
      {
        m_gWindowManager.Process(true);
      }
    }
  }
  SaveControlStates();
}

bool CGUIWindow::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      OutputDebugString("------------------- GUI_MSG_WINDOW_INIT ");
      OutputDebugString(g_localizeStrings.Get(GetID()).c_str());
      OutputDebugString("------------------- \n");
      if (m_dynamicResourceAlloc || !m_bAllocated) AllocResources();
      OnInitWindow();
      return true;
    }
    break;

  case GUI_MSG_WINDOW_DEINIT:
    {
      OutputDebugString("------------------- GUI_MSG_WINDOW_DEINIT ");
      OutputDebugString(g_localizeStrings.Get(GetID()).c_str());
      OutputDebugString("------------------- \n");
      OnDeinitWindow(message.GetParam1());
      // now free the window
      if (m_dynamicResourceAlloc) FreeResources();
      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      // a specific control was clicked
      CLICK_EVENT clickEvent = m_mapClickEvents[ message.GetSenderId() ];

      // determine if there are any handlers for this event
      if (clickEvent.HasAHandler())
      {
        // fire the message to all handlers
        clickEvent.Fire(message);
      }
      break;
    }

  case GUI_MSG_SELCHANGED:
    {
      // a selection within a specific control has changed
      SELECTED_EVENT selectedEvent = m_mapSelectedEvents[ message.GetSenderId() ];

      // determine if there are any handlers for this event
      if (selectedEvent.HasAHandler())
      {
        // fire the message to all handlers
        selectedEvent.Fire(message);
      }
      break;
    }
  case GUI_MSG_FOCUSED:
    { // a control has been focused
      if (HasID(message.GetSenderId()))
      {
        m_focusedControl = message.GetControlId();
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
//      CLog::Log(LOGDEBUG,"set focus to control:%i window:%i (%i)\n", message.GetControlId(),message.GetSenderId(), GetID());
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
        if (!pFocusedControl) pFocusedControl = (CGUIControl *)GetControl(message.GetControlId());

        // and focus it
        if (pFocusedControl)
          return pFocusedControl->OnMessage(message);
      }
      return true;
    }
    break;
  case GUI_MSG_NOTIFY_ALL:
    {
      // only process those notifications that come from this window, or those intended for every window
      if (HasID(message.GetSenderId()) || !message.GetSenderId())
      {
        if (message.GetParam1() == GUI_MSG_PAGE_CHANGE ||
            message.GetParam1() == GUI_MSG_REFRESH_THUMBS ||
            message.GetParam1() == GUI_MSG_REFRESH_LIST)
        { // alter the message accordingly, and send to all controls
          for (iControls it = m_children.begin(); it != m_children.end(); ++it)
          {
            CGUIControl *control = *it;
            CGUIMessage msg(message.GetParam1(), message.GetControlId(), control->GetID(), message.GetParam2());
            control->OnMessage(msg);
          }
        }
      }
    }
    break;
  }

  return SendControlMessage(message);
}

void CGUIWindow::AllocResources(bool forceLoad /*= FALSE */)
{
  CSingleLock lock(g_graphicsContext);

  LARGE_INTEGER start;
  QueryPerformanceCounter(&start);

  // load skin xml file
  bool bHasPath=false;
  if (m_xmlFile.Find("\\") > -1 || m_xmlFile.Find("/") > -1 )
    bHasPath = true;
  if (m_xmlFile.size() && (forceLoad || m_loadOnDemand || !m_windowLoaded))
    Load(m_xmlFile,bHasPath);

  LARGE_INTEGER slend;
  QueryPerformanceCounter(&slend);

  // and now allocate resources
  g_TextureManager.StartPreLoad();
  CGUIControlGroup::PreAllocResources();
  g_TextureManager.EndPreLoad();

  LARGE_INTEGER plend;
  QueryPerformanceCounter(&plend);

  CGUIControlGroup::AllocResources();

  g_TextureManager.FlushPreLoad();

  LARGE_INTEGER end, freq;
  QueryPerformanceCounter(&end);
  QueryPerformanceFrequency(&freq);
  CLog::Log(LOGDEBUG,"Alloc resources: %.2fms (%.2f ms skin load, %.2f ms preload)", 1000.f * (end.QuadPart - start.QuadPart) / freq.QuadPart, 1000.f * (slend.QuadPart - start.QuadPart) / freq.QuadPart, 1000.f * (plend.QuadPart - slend.QuadPart) / freq.QuadPart);

  m_bAllocated = true;
}

void CGUIWindow::FreeResources(bool forceUnload /*= FALSE */)
{
  m_bAllocated = false;
  CGUIControlGroup::FreeResources();
  //g_TextureManager.Dump();
  // unload the skin
  if (m_loadOnDemand || forceUnload) ClearAll();
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
}

bool CGUIWindow::Initialize()
{
  return Load(m_xmlFile);
}

void CGUIWindow::SetInitialVisibility()
{
  // reset our info manager caches
  g_infoManager.ResetCache();
  CGUIControlGroup::SetInitialVisibility();
}

bool CGUIWindow::IsActive() const
{
  return m_gWindowManager.IsWindowActive(GetID());
}

bool CGUIWindow::CheckAnimation(ANIMATION_TYPE animType)
{
  // special cases first
  if (animType == ANIM_TYPE_WINDOW_CLOSE)
  {
    if (!m_bAllocated || !m_hasRendered) // can't render an animation if we aren't allocated or haven't rendered
      return false;
    // make sure we update our visibility prior to queuing the window close anim
    for (unsigned int i = 0; i < m_children.size(); i++)
      m_children[i]->UpdateVisibility();
  }
  return true;
}

bool CGUIWindow::IsAnimating(ANIMATION_TYPE animType)
{
  if (!m_animationsEnabled)
    return false;
  return CGUIControlGroup::IsAnimating(animType);
}

bool CGUIWindow::RenderAnimation(DWORD time)
{
  g_graphicsContext.ResetWindowTransform();
  if (m_animationsEnabled)
    CGUIControlGroup::Animate(time);
  else
    m_transform.Reset();
  return true;
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
  if (!group) group = (CGUIControl *)GetControl(groupID);

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
      return (controlID == (int) message.GetParam1());
    }
  }
  return false;
}

void CGUIWindow::SaveControlStates()
{
  ResetControlStates();
  if (m_saveLastControl)
    m_lastControlID = GetFocusedControlID();
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
    (*it)->SaveStates(m_controlStates);
}

void CGUIWindow::RestoreControlStates()
{
  for (vector<CControlState>::iterator it = m_controlStates.begin(); it != m_controlStates.end(); ++it)
  {
    CGUIMessage message(GUI_MSG_ITEM_SELECT, GetID(), (*it).m_id, (*it).m_data);
    OnMessage(message);
  }
  int focusControl = (m_saveLastControl && m_lastControlID) ? m_lastControlID : m_defaultControl;
  SET_CONTROL_FOCUS(focusControl, 0);
}

void CGUIWindow::ResetControlStates()
{
  m_lastControlID = 0;
  m_focusedControl = 0;
  m_controlStates.clear();
}

bool CGUIWindow::OnMove(int fromControl, int moveAction)
{
  const CGUIControl *control = GetFirstFocusableControl(fromControl);
  if (!control) control = GetControl(fromControl);
  if (!control)
  { // no current control??
    CLog::Log(LOGERROR, "Unable to find control %i in window %u",
              fromControl, GetID());
    return false;
  }
  vector<int> moveHistory;
  int nextControl = fromControl;
  while (control)
  { // grab the next control direction
    moveHistory.push_back(nextControl);
    nextControl = control->GetNextControl(moveAction);
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
  m_renderOrder = 0;
  m_saveLastControl = true;
  m_defaultControl = 0;
  m_bRelativeCoords = false;
  m_posX = m_posY = m_width = m_height = 0;
  m_overlayState = OVERLAY_STATE_PARENT_WINDOW;   // Use parent or previous window's state
  m_visibleCondition = 0;
  m_previousWindow = WINDOW_INVALID;
  m_animations.clear();
  m_origins.clear();
  m_hasCamera = false;
  m_animationsEnabled = true;
}

FRECT CGUIWindow::GetScaledBounds() const
{
  CSingleLock lock(g_graphicsContext);
  g_graphicsContext.SetScalingResolution(m_coordsRes, m_posX, m_posY, m_needsScaling);
  FRECT rect = {0, 0, m_width, m_height};
  float z = 0;
  g_graphicsContext.ScaleFinalCoords(rect.left, rect.top, z);
  g_graphicsContext.ScaleFinalCoords(rect.right, rect.bottom, z);
  return rect;
}

void CGUIWindow::OnEditChanged(int id, CStdString &text)
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), id);
  OnMessage(msg);
  text = msg.GetLabel();
}

bool CGUIWindow::SendMessage(DWORD message, DWORD id, DWORD param1 /* = 0*/, DWORD param2 /* = 0*/)
{
  CGUIMessage msg(message, GetID(), id, param1, param2);
  return OnMessage(msg);
}

#ifdef _DEBUG
void CGUIWindow::DumpTextureUse()
{
  CLog::Log(LOGDEBUG, "%s for window %u", __FUNCTION__, GetID());
  CGUIControlGroup::DumpTextureUse();
}
#endif

void CGUIWindow::ChangeButtonToEdit(int id, bool singleLabel /* = false*/)
{
#ifdef PRE_SKIN_VERSION_9_10_COMPATIBILITY
  CGUIControl *name = (CGUIControl *)GetControl(id);
  if (name && name->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
  { // change it to an edit control
    CGUIEditControl *edit = new CGUIEditControl(*(const CGUIButtonControl *)name);
    if (edit)
    {
      if (singleLabel)
        edit->SetLabel("");
      InsertControl(edit, name);
      RemoveControl(name);
      name->FreeResources();
      delete name;
    }
  }
#endif
}

void CGUIWindow::SetProperty(const CStdString &strKey, const char *strValue)
{
  m_mapProperties[strKey] = strValue;
}

void CGUIWindow::SetProperty(const CStdString &strKey, const CStdString &strValue)
{
  m_mapProperties[strKey] = strValue;
}

void CGUIWindow::SetProperty(const CStdString &strKey, int nVal)
{
  CStdString strVal;
  strVal.Format("%d",nVal);
  SetProperty(strKey, strVal);
}

void CGUIWindow::SetProperty(const CStdString &strKey, bool bVal)
{
  SetProperty(strKey, bVal?"1":"0");
}

void CGUIWindow::SetProperty(const CStdString &strKey, double dVal)
{
  CStdString strVal;
  strVal.Format("%f",dVal);
  SetProperty(strKey, strVal);
}

CStdString CGUIWindow::GetProperty(const CStdString &strKey) const
{
  std::map<CStdString,CStdString,icompare>::const_iterator iter = m_mapProperties.find(strKey);
  if (iter == m_mapProperties.end())
    return "";

  return iter->second;
}

int CGUIWindow::GetPropertyInt(const CStdString &strKey) const
{
  return atoi(GetProperty(strKey).c_str()) ;
}

bool CGUIWindow::GetPropertyBOOL(const CStdString &strKey) const
{
  return GetProperty(strKey) == "1";
}

double CGUIWindow::GetPropertyDouble(const CStdString &strKey) const
{
  return atof(GetProperty(strKey).c_str()) ;
}

void CGUIWindow::ClearProperty(const CStdString &strKey)
{
  std::map<CStdString,CStdString,icompare>::iterator iter = m_mapProperties.find(strKey);
  if (iter != m_mapProperties.end())
    m_mapProperties.erase(iter);
}

void CGUIWindow::ClearProperties()
{
  m_mapProperties.clear();
}
