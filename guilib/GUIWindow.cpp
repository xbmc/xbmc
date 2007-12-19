#include "include.h"
#include "GUIWindow.h"
#include "GUIWindowManager.h"
#include "LocalizeStrings.h"
#include "TextureManager.h"
#include "../xbmc/Util.h"
#include "GuiControlFactory.h"
#include "GUIControlGroup.h"
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
#include "GUIListContainer.h"
#include "GUIPanelContainer.h"
#endif

#include "SkinInfo.h"
#include "../xbmc/utils/GUIInfoManager.h"
#include "../xbmc/utils/SingleLock.h"
#include "../xbmc/ButtonTranslator.h"
#include "XMLUtils.h"

CStdString CGUIWindow::CacheFilename = "";

CGUIWindow::CGUIWindow(DWORD dwID, const CStdString &xmlFile)
{
  m_dwWindowId = dwID;
  m_xmlFile = xmlFile;
  m_dwIDRange = 1;
  m_saveLastControl = false;
  m_dwDefaultFocusControlID = 0;
  m_lastControlID = 0;
  m_focusedControl = 0;
  m_bRelativeCoords = false;
  m_posX = m_posY = m_width = m_height = 0;
  m_overlayState = OVERLAY_STATE_PARENT_WINDOW;   // Use parent or previous window's state
  m_WindowAllocated = false;
  m_coordsRes = g_guiSettings.m_LookAndFeelResolution;
  m_isDialog = false;
  m_needsScaling = true;
  m_visibleCondition = 0;
  m_windowLoaded = false;
  m_loadOnDemand = true;
  m_renderOrder = 0;
  m_dynamicResourceAlloc = true;
  m_hasRendered = false;
  m_hasCamera = false;
  m_previousWindow = WINDOW_INVALID;
}

CGUIWindow::~CGUIWindow(void)
{}

void CGUIWindow::FlushReferenceCache()
{
  CacheFilename.clear();
}

bool CGUIWindow::LoadReferences()
{
  // load references.xml
  TiXmlDocument xmlDoc;
  RESOLUTION res;
  CStdString strReferenceFile = g_SkinInfo.GetSkinPath("references.xml", &res);
  // check if we've already loaded it previously
  if (CacheFilename == strReferenceFile)
    return true;

  // nope - time to load it in
  if ( !xmlDoc.LoadFile(strReferenceFile.c_str()) )
  {
//    CLog::Log(LOGERROR, "unable to load:%s, Line %d\n%s", strReferenceFile.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }

  CLog::Log(LOGINFO, "Loading references file: %s", strReferenceFile.c_str());
  TiXmlElement* pRootElement = xmlDoc.RootElement();
  CStdString strValue = pRootElement->Value();
  if (strValue != CStdString("controls"))
  {
    CLog::Log(LOGERROR, "references.xml doesn't contain <controls>");
    return false;
  }
  RESOLUTION includeRes;
  g_SkinInfo.GetSkinPath("includes.xml", &includeRes);
  g_SkinInfo.ResolveIncludes(pRootElement);
  CGUIControlFactory factory;
  CStdString strType;
  TiXmlElement *pControl = pRootElement->FirstChildElement();
  TiXmlElement includes("includes");
  while (pControl)
  {
    // ok, this is a <control> block, find the type
    strType = factory.GetType(pControl);
    if (!strType.IsEmpty())
    { // we construct a new <default type="type"> block in our includes document
      TiXmlElement include("default");
      include.SetAttribute("type", strType.c_str());
      // and add the rest of the items under this controlblock to it
      TiXmlElement *child = pControl->FirstChildElement();
      while (child)
      {
        TiXmlElement element(*child);
        // scale element if necessary
        factory.ScaleElement(&element, res, includeRes);
        include.InsertEndChild(element);
        child = child->NextSiblingElement();
      }
      includes.InsertEndChild(include);
    }
    pControl = pControl->NextSiblingElement();
  }
  CacheFilename = strReferenceFile;
  // now load our includes
  g_SkinInfo.LoadIncludes(&includes);
  return true;
}

bool CGUIWindow::Load(const CStdString& strFileName, bool bContainsPath)
{
  if (m_windowLoaded)
    return true;      // no point loading if it's already there
  LARGE_INTEGER start;
  QueryPerformanceCounter(&start);

  RESOLUTION resToUse = INVALID;
  CLog::Log(LOGINFO, "Loading skin file: %s", strFileName.c_str());
  TiXmlDocument xmlDoc;
  // Find appropriate skin folder + resolution to load from
  CStdString strPath;
  if (bContainsPath)
    strPath = strFileName;
  else
    strPath = g_SkinInfo.GetSkinPath(strFileName, &resToUse);

  if ( !xmlDoc.LoadFile(strPath.c_str()) )
  {
    CLog::Log(LOGERROR, "unable to load:%s, Line %d\n%s", strPath.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
    if (g_SkinInfo.GetVersion() < 2.1 && GetID() == WINDOW_VIDEO_NAV && m_xmlFile != "myvideotitle.xml")
    {
      m_xmlFile = "myvideotitle.xml";
      return Load(m_xmlFile);
    }
#endif
    m_dwWindowId = WINDOW_INVALID;
    return false;
  }
  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if (strcmpi(pRootElement->Value(), "window"))
  {
    CLog::Log(LOGERROR, "file :%s doesnt contain <window>", strPath.c_str());
    return false;
  }
  LARGE_INTEGER lend;
  QueryPerformanceCounter(&lend);
  if (!bContainsPath)
    m_coordsRes = resToUse;
  bool ret = Load(pRootElement);
  LARGE_INTEGER end, freq;
  QueryPerformanceCounter(&end);
  QueryPerformanceFrequency(&freq);
  CLog::DebugLog("Load %s: %.2fms (%.2f ms xml load)", m_xmlFile.c_str(), 1000.f * (end.QuadPart - start.QuadPart) / freq.QuadPart, 1000.f * (lend.QuadPart - start.QuadPart) / freq.QuadPart);
  return ret;
}

bool CGUIWindow::Load(TiXmlElement* pRootElement)
{
  // set the scaling resolution so that any control creation or initialisation can
  // be done with respect to the correct aspect ratio
  g_graphicsContext.SetScalingResolution(m_coordsRes, 0, 0, m_needsScaling);

  // Resolve any includes that may be present
  g_SkinInfo.ResolveIncludes(pRootElement);
  // now load in the skin file
  SetDefaults();

  LoadReferences();
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
      m_dwDefaultFocusControlID = atoi(pChild->FirstChild()->Value());
    }
    else if (strValue == "visible" && pChild->FirstChild())
    {
      CGUIControlFactory::GetConditionalVisibility(pRootElement, m_visibleCondition);
    }
    else if (strValue == "animation" && pChild->FirstChild())
    {
      FRECT rect = { 0, 0, (float)g_settings.m_ResInfo[m_coordsRes].iWidth, (float)g_settings.m_ResInfo[m_coordsRes].iHeight };
      if (strcmpi(pChild->FirstChild()->Value(), "windowopen") == 0)
        m_showAnimation.Create(pChild->ToElement(), rect);
      else if (strcmpi(pChild->FirstChild()->Value(), "windowclose") == 0)
        m_closeAnimation.Create(pChild->ToElement(), rect);
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
  CGUIControl* pGUIControl = factory.Create(m_dwWindowId, rect, pControl);
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
    pGUIControl->SetParentControl(pGroup);
    if (pGroup)
      pGroup->AddControl(pGUIControl);
    else
      Add(pGUIControl);
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
    if (pGUIControl->GetControlType() == CGUIControl::GUICONTAINER_LIST)
    {
      CGUIListContainer *list = (CGUIListContainer *)pGUIControl;
      if (list->m_spinControl)
      {
        list->m_spinControl->SetParentControl(pGroup);
        if (pGroup)
          pGroup->AddControl(list->m_spinControl);
        else
          Add(list->m_spinControl);
        list->m_spinControl = NULL;
      }
    }
    if (pGUIControl->GetControlType() == CGUIControl::GUICONTAINER_PANEL)
    {
      CGUIPanelContainer *panel = (CGUIPanelContainer *)pGUIControl;
      if (panel->m_spinControl)
      {
        panel->m_spinControl->SetParentControl(pGroup);
        if (pGroup)
          pGroup->AddControl(panel->m_spinControl);
        else
          Add(panel->m_spinControl);
        panel->m_spinControl = NULL;
      }
      if (panel->m_largePanel)
      {
        panel->m_largePanel->SetParentControl(pGroup);
        if (pGroup)
          pGroup->AddControl(panel->m_largePanel);
        else
          Add(panel->m_largePanel);
        panel->m_largePanel = NULL;
      }
    }
#endif
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
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  // we hook up the controlgroup navigation as desired
  if (g_SkinInfo.GetVersion() < 2.1)
  { // run through controls, and check navigation into a controlgroup
    for (ivecControls i = m_vecControls.begin(); i != m_vecControls.end(); ++i)
    {
      CGUIControl *group = *i;
      if (group->IsGroup())
      {
        // first thing first: We have to have a unique id
        if (!group->GetID())
        {
          DWORD id = 9000;
          while (GetControl(id++) && id < 9100)
            ;
          group->SetID(id);
        }
      }
    }
  }
#endif
}

void CGUIWindow::SetPosition(float posX, float posY)
{
  m_posX = posX;
  m_posY = posY;
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
  if (!m_WindowAllocated) return;

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
  g_graphicsContext.SetScalingResolution(m_coordsRes, posX, posY, m_needsScaling);
  if (m_hasCamera)
    g_graphicsContext.SetCameraPosition(m_camera);

  DWORD currentTime = timeGetTime();
  // render our window animation - returns false if it needs to stop rendering
  if (!RenderAnimation(currentTime))
    return;

  for (int i = 0; i < (int)m_vecControls.size(); i++)
  {
    CGUIControl *pControl = m_vecControls[i];
    if (pControl)
    {
      pControl->UpdateVisibility();
      pControl->DoRender(currentTime);
    }
  }
  m_hasRendered = true;
}

bool CGUIWindow::OnAction(const CAction &action)
{
  if (action.wID == ACTION_MOUSE)
  {
    OnMouseAction();
    return true;
  }
  CGUIControl *focusedControl = GetFocusedControl();
  if (focusedControl)
    return focusedControl->OnAction(action);

  // no control has focus?
  // set focus to the default control then
  CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), m_dwDefaultFocusControlID);
  OnMessage(msg);
  return false;
}

// OnMouseAction - called by OnAction()
void CGUIWindow::OnMouseAction()
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

  bool bHandled = false;
  // check if we have exclusive access
  if (g_Mouse.GetExclusiveWindowID() == GetID())
  { // we have exclusive access to the mouse...
    CGUIControl *pControl = (CGUIControl *)GetControl(g_Mouse.GetExclusiveControlID());
    if (pControl)
    { // this control has exclusive access to the mouse
      HandleMouse(pControl, mousePoint + g_Mouse.GetExclusiveOffset());
      return;
    }
  }

  // run through the controls, and unfocus all those that aren't under the pointer,
  for (ivecControls i = m_vecControls.begin(); i != m_vecControls.end(); ++i)
  {
    CGUIControl *pControl = *i;
    pControl->UnfocusFromPoint(mousePoint);
  }
  // and find which one is under the pointer
  // go through in reverse order to make sure we start with the ones on top
  bool controlUnderPointer(false);
  for (vector<CGUIControl *>::reverse_iterator i = m_vecControls.rbegin(); i != m_vecControls.rend(); ++i)
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
    OnMouse(mousePoint);
  }
  // and unfocus everything otherwise
  if (!controlUnderPointer)
    m_focusedControl = 0;
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

DWORD CGUIWindow::GetID(void) const
{
  return m_dwWindowId;
}

void CGUIWindow::SetID(DWORD dwID)
{
  m_dwWindowId = dwID;
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
  // set our initial control visibility before restoring control state and
  // focusing the default control, and again afterward to make sure that
  // any controls that depend on the state of the focused control (and or on
  // control states) are active.
  SetControlVisibility();
  RestoreControlStates();
  SetControlVisibility();
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
    if (HasAnimation(ANIM_TYPE_WINDOW_CLOSE) && !IsDialog())
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
      if (m_dynamicResourceAlloc || !m_WindowAllocated) AllocResources();
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
  case GUI_MSG_MOVE:
    {
      if (HasID(message.GetSenderId()))
        return OnMove(message.GetControlId(), message.GetParam1());
      break;
    }
  case GUI_MSG_SETFOCUS:
    {
//      CLog::DebugLog("set focus to control:%i window:%i (%i)\n", message.GetControlId(),message.GetSenderId(), GetID());
      if ( message.GetControlId() )
      {
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
        if (g_SkinInfo.GetVersion() < 2.1)
        { // to support the best backwards compatibility, we reproduce the old method here
          const CGUIControl *oldGroup = NULL;
          // first unfocus the current control
          CGUIControl *control = GetFocusedControl();
          if (control)
          {
            CGUIMessage msgLostFocus(GUI_MSG_LOSTFOCUS, GetID(), control->GetID(), message.GetControlId());
            control->OnMessage(msgLostFocus);
            oldGroup = control->GetParentControl();
          }
          // get the control to focus
          CGUIControl* pFocusedControl = GetFirstFocusableControl(message.GetControlId());
          if (!pFocusedControl) pFocusedControl = (CGUIControl *)GetControl(message.GetControlId());

          // and focus it
          if (pFocusedControl)
          {
            // check for group changes
            if (pFocusedControl->GetParentControl() && pFocusedControl->GetParentControl() != oldGroup)
            { // going to a different group, focus the group instead
              CGUIControlGroup *group = (CGUIControlGroup *)pFocusedControl->GetParentControl();
              if (group->GetFocusedControlID())
                pFocusedControl = group;
            }
            return pFocusedControl->OnMessage(message);
          }
        }
        else
          {
#endif
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
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
        }
#endif
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
          for (ivecControls it = m_vecControls.begin(); it != m_vecControls.end(); ++it)
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

  ivecControls i;
  // Send to the visible matching control first
  for (i = m_vecControls.begin();i != m_vecControls.end(); ++i)
  {
    CGUIControl* pControl = *i;
    if (pControl->HasVisibleID(message.GetControlId()))
    {
      if (pControl->OnMessage(message))
        return true;
    }
  }
  // Unhandled - send to all matching invisible controls as well
  bool handled(false);
  for (i = m_vecControls.begin();i != m_vecControls.end(); ++i)
  {
    CGUIControl* pControl = *i;
    if (pControl->HasID(message.GetControlId()))
    {
      if (pControl->OnMessage(message))
        handled = true;
    }
  }
  return handled;
}

void CGUIWindow::AllocResources(bool forceLoad /*= FALSE */)
{
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
  ivecControls i;
  for (i = m_vecControls.begin();i != m_vecControls.end(); ++i)
  {
    CGUIControl* pControl = *i;
    if (!pControl->IsDynamicallyAllocated()) 
      pControl->PreAllocResources();
  }
  g_TextureManager.EndPreLoad();

  LARGE_INTEGER plend;
  QueryPerformanceCounter(&plend);

  for (i = m_vecControls.begin();i != m_vecControls.end(); ++i)
  {
    CGUIControl* pControl = *i;
    if (!pControl->IsDynamicallyAllocated()) 
      pControl->AllocResources();
  }
  g_TextureManager.FlushPreLoad();

  LARGE_INTEGER end, freq;
  QueryPerformanceCounter(&end);
  QueryPerformanceFrequency(&freq);
  m_WindowAllocated = true;
  CLog::DebugLog("Alloc resources: %.2fms (%.2f ms skin load, %.2f ms preload)", 1000.f * (end.QuadPart - start.QuadPart) / freq.QuadPart, 1000.f * (slend.QuadPart - start.QuadPart) / freq.QuadPart, 1000.f * (plend.QuadPart - slend.QuadPart) / freq.QuadPart);
}

void CGUIWindow::FreeResources(bool forceUnload /*= FALSE */)
{
  m_WindowAllocated = false;
  ivecControls i;
  for (i = m_vecControls.begin();i != m_vecControls.end(); ++i)
  {
    CGUIControl* pControl = *i;
    pControl->FreeResources();
  }
  //g_TextureManager.Dump();
  // unload the skin
  if (m_loadOnDemand || forceUnload) ClearAll();
}

void CGUIWindow::DynamicResourceAlloc(bool bOnOff)
{
  m_dynamicResourceAlloc = bOnOff;
  for (ivecControls i = m_vecControls.begin();i != m_vecControls.end(); ++i)
  {
    CGUIControl* pControl = *i;
    pControl->DynamicResourceAlloc(bOnOff);
  }
}

void CGUIWindow::Add(CGUIControl* pControl)
{
  m_vecControls.push_back(pControl);
}

void CGUIWindow::Insert(CGUIControl *control, const CGUIControl *insertPoint)
{
  // get the insertion point
  ivecControls i = m_vecControls.begin();
  while (i != m_vecControls.end())
  {
    if (*i == insertPoint)
      break;
    i++;
  }
  m_vecControls.insert(i, control);
}

// Note: This routine doesn't delete the control.  It just removes it from the control list
bool CGUIWindow::Remove(DWORD dwId)
{
  ivecControls i = m_vecControls.begin();
  while (i != m_vecControls.end())
  {
    CGUIControl* pControl = *i;
    if (pControl->IsGroup())
    {
      CGUIControlGroup *group = (CGUIControlGroup *)pControl;
      if (group->RemoveControl(dwId))
        return true;
    }
    if (pControl->GetID() == dwId)
    {
      m_vecControls.erase(i);
      return true;
    }
    ++i;
  }
  return false;
}

void CGUIWindow::ClearAll()
{
  OnWindowUnload();

  for (int i = 0; i < (int)m_vecControls.size(); ++i)
  {
    CGUIControl* pControl = m_vecControls[i];
    delete pControl;
  }
  m_vecControls.erase(m_vecControls.begin(), m_vecControls.end());
  m_windowLoaded = false;
  m_dynamicResourceAlloc = true;
}

const CGUIControl* CGUIWindow::GetControl(int iControl) const
{
  const CGUIControl* pPotential=NULL;
  for (int i = 0;i < (int)m_vecControls.size(); ++i)
  {
    const CGUIControl* pControl = m_vecControls[i];
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

int CGUIWindow::GetFocusedControlID() const
{
  if (m_focusedControl) return m_focusedControl;
  CGUIControl *control = GetFocusedControl();
  if (control) return control->GetID();
  return -1;
}

CGUIControl *CGUIWindow::GetFocusedControl() const
{
  for (vector<CGUIControl *>::const_iterator it = m_vecControls.begin(); it != m_vecControls.end(); ++it)
  {
    const CGUIControl* pControl = *it;
    if (pControl->HasFocus())
    {
      if (pControl->IsGroup())
      {
        CGUIControlGroup *group = (CGUIControlGroup *)pControl;
        return group->GetFocusedControl();
      }
      return (CGUIControl *)pControl;
    }
  }
  return NULL;
}

bool CGUIWindow::Initialize()
{
  return Load(m_xmlFile);
}

void CGUIWindow::SetControlVisibility()
{
  // reset our info manager caches
  g_infoManager.ResetCache();
  for (unsigned int i=0; i < m_vecControls.size(); i++)
  {
    CGUIControl *pControl = m_vecControls[i];
    pControl->SetInitialVisibility();
  }
}

bool CGUIWindow::IsActive() const
{
  return m_gWindowManager.IsWindowActive(GetID());
}

void CGUIWindow::QueueAnimation(ANIMATION_TYPE animType)
{
  if (animType == ANIM_TYPE_WINDOW_OPEN)
  {
    if (m_closeAnimation.GetProcess() == ANIM_PROCESS_NORMAL && m_closeAnimation.IsReversible())
    {
      m_closeAnimation.QueueAnimation(ANIM_PROCESS_REVERSE);
      m_showAnimation.ResetAnimation();
    }
    else
    {
      if (!m_showAnimation.GetCondition() || g_infoManager.GetBool(m_showAnimation.GetCondition(), GetID()))
        m_showAnimation.QueueAnimation(ANIM_PROCESS_NORMAL);
      m_closeAnimation.ResetAnimation();
    }
  }
  if (animType == ANIM_TYPE_WINDOW_CLOSE)
  {
    if (!m_WindowAllocated || !m_hasRendered) // can't render an animation if we aren't allocated or haven't rendered
      return;
    if (m_showAnimation.GetProcess() == ANIM_PROCESS_NORMAL && m_showAnimation.IsReversible())
    {
      m_showAnimation.QueueAnimation(ANIM_PROCESS_REVERSE);
      m_closeAnimation.ResetAnimation();
    }
    else
    {
      if (!m_closeAnimation.GetCondition() || g_infoManager.GetBool(m_closeAnimation.GetCondition(), GetID()))
        m_closeAnimation.QueueAnimation(ANIM_PROCESS_NORMAL);
      m_showAnimation.ResetAnimation();
    }
  }
  for (unsigned int i = 0; i < m_vecControls.size(); i++)
  {
    CGUIControl *pControl = m_vecControls[i];
    pControl->QueueAnimation(animType);
  }
}

bool CGUIWindow::IsAnimating(ANIMATION_TYPE animType)
{
  if (animType == ANIM_TYPE_WINDOW_OPEN)
  {
    if (m_showAnimation.GetQueuedProcess() == ANIM_PROCESS_NORMAL) return true;
    if (m_showAnimation.GetProcess() == ANIM_PROCESS_NORMAL) return true;
    if (m_closeAnimation.GetQueuedProcess() == ANIM_PROCESS_REVERSE) return true;
    if (m_closeAnimation.GetProcess() == ANIM_PROCESS_REVERSE) return true;
  }
  else if (animType == ANIM_TYPE_WINDOW_CLOSE)
  {
    if (m_closeAnimation.GetQueuedProcess() == ANIM_PROCESS_NORMAL) return true;
    if (m_closeAnimation.GetProcess() == ANIM_PROCESS_NORMAL) return true;
    if (m_showAnimation.GetQueuedProcess() == ANIM_PROCESS_REVERSE) return true;
    if (m_showAnimation.GetProcess() == ANIM_PROCESS_REVERSE) return true;
  }
  for (unsigned int i = 0; i < m_vecControls.size(); i++)
  {
    CGUIControl *pControl = m_vecControls[i];
    if (pControl->IsAnimating(animType)) return true;
  }
  return false;
}

bool CGUIWindow::RenderAnimation(DWORD time)
{
  TransformMatrix transform;
  CPoint center(m_posX + m_width * 0.5f, m_posY + m_height * 0.5f);
  // show animation
  m_showAnimation.Animate(time, true);
  UpdateStates(m_showAnimation.GetType(), m_showAnimation.GetProcess(), m_showAnimation.GetState());
  m_showAnimation.RenderAnimation(transform, center);
  // close animation
  m_closeAnimation.Animate(time, true);
  UpdateStates(m_closeAnimation.GetType(), m_closeAnimation.GetProcess(), m_closeAnimation.GetState());
  m_closeAnimation.RenderAnimation(transform, center);
  g_graphicsContext.SetWindowTransform(transform);
  return true;
}

void CGUIWindow::UpdateStates(ANIMATION_TYPE type, ANIMATION_PROCESS currentProcess, ANIMATION_STATE currentState)
{
}

bool CGUIWindow::HasAnimation(ANIMATION_TYPE animType)
{
  if (m_showAnimation.GetType() == animType && (!m_showAnimation.GetCondition() || g_infoManager.GetBool(m_showAnimation.GetCondition())))
    return true;
  else if (m_closeAnimation.GetType() == animType && (!m_closeAnimation.GetCondition() || g_infoManager.GetBool(m_closeAnimation.GetCondition())))
    return true;
  // Now check the controls to see if we have this animation
  for (unsigned int i = 0; i < m_vecControls.size(); i++)
    if (m_vecControls[i]->GetAnimation(animType)) return true;
  return false;
}

// returns true if the control group with id groupID has controlID as
// its focused control
bool CGUIWindow::ControlGroupHasFocus(int groupID, int controlID)
{
  // 1.  Run through and get control with groupID (assume unique)
  // 2.  Get it's selected item.
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  if (g_SkinInfo.GetVersion() < 2.1)
    groupID += 9000;
#endif
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
      return (controlID == message.GetParam1());
    }
  }
  return false;
}

void CGUIWindow::SaveControlStates()
{
  ResetControlStates();
  if (m_saveLastControl)
    m_lastControlID = GetFocusedControlID();
  for (ivecControls it = m_vecControls.begin(); it != m_vecControls.end(); ++it)
    (*it)->SaveStates(m_controlStates);
}

void CGUIWindow::RestoreControlStates()
{
  for (vector<CControlState>::iterator it = m_controlStates.begin(); it != m_controlStates.end(); ++it)
  {
    CGUIMessage message(GUI_MSG_ITEM_SELECT, GetID(), (*it).m_id, (*it).m_data);
    OnMessage(message);
  }
  int focusControl = (m_saveLastControl && m_lastControlID) ? m_lastControlID : m_dwDefaultFocusControlID;
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  if (g_SkinInfo.GetVersion() < 2.1)
  { // skins such as mc360 focus a control in a group by default.
    // In 2.1 they should set the focus to the group, rather than the control in the group
    CGUIControl *control = GetFirstFocusableControl(focusControl);
    if (!control) control = (CGUIControl *)GetControl(focusControl);
    if (control && control->GetParentControl())
    {
      CGUIControlGroup *group = (CGUIControlGroup *)control->GetParentControl();
      if (group->GetFocusedControlID())
        focusControl = group->GetID();
    }
  }
#endif
  SET_CONTROL_FOCUS(focusControl, 0);
}

void CGUIWindow::ResetControlStates()
{
  m_lastControlID = 0;
  m_focusedControl = 0;
  m_controlStates.clear();
}

// find the first focusable control with this id.
// if no focusable control exists with this id, return NULL
CGUIControl *CGUIWindow::GetFirstFocusableControl(int id)
{
  for (ivecControls i = m_vecControls.begin();i != m_vecControls.end(); ++i)
  {
    CGUIControl* pControl = *i;
    if (pControl->IsGroup())
    {
      CGUIControlGroup *group = (CGUIControlGroup *)pControl;
      CGUIControl *control = group->GetFirstFocusableControl(id);
      if (control) return control;
    }
    if (pControl->GetID() == id && pControl->CanFocus())
      return pControl;
  }
  return NULL;
}

bool CGUIWindow::OnMove(int fromControl, int moveAction)
{
  const CGUIControl *control = GetFirstFocusableControl(fromControl);
  if (!control) control = GetControl(fromControl);
  if (!control)
  { // no current control??
    CLog::Log(LOGERROR, "Unable to find control %i in window %i", fromControl, GetID());
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
  m_dwDefaultFocusControlID = 0;
  m_bRelativeCoords = false;
  m_posX = m_posY = m_width = m_height = 0;
  m_overlayState = OVERLAY_STATE_PARENT_WINDOW;   // Use parent or previous window's state
  m_visibleCondition = 0;
  m_previousWindow = WINDOW_INVALID;
  m_showAnimation.Reset();
  m_closeAnimation.Reset();
  m_origins.clear();
  m_hasCamera = false;
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

void CGUIWindow::GetContainers(vector<CGUIControl *> &containers) const
{
  for (ciControls it = m_vecControls.begin();it != m_vecControls.end(); ++it)
  {
    if ((*it)->IsContainer())
      containers.push_back(*it);
    else if ((*it)->IsGroup())
      ((CGUIControlGroup *)(*it))->GetContainers(containers);
  }
}

#ifdef _DEBUG
void CGUIWindow::DumpTextureUse()
{
  CLog::Log(LOGDEBUG, __FUNCTION__" for window %i", GetID());
  for (ivecControls it = m_vecControls.begin();it != m_vecControls.end(); ++it)
  {
    (*it)->DumpTextureUse();
  }
}
#endif
