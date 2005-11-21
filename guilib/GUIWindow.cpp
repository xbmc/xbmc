#include "include.h"
#include "GUIWindow.h"
#include "GUIWindowManager.h"
#include "LocalizeStrings.h"
#include "TextureManager.h"
#include "../xbmc/util.h"
#include "GUIControlFactory.h"
#include "GUIButtonControl.h"
#include "GUIConditionalButtonControl.h"
#include "GUIRadioButtonControl.h"
#include "GUISpinControl.h"
#include "GUISpinControlEx.h"
#include "GUIRSSControl.h"
#include "GUIRAMControl.h"
#include "GUIConsoleControl.h"
#include "GUIListControl.h"
#include "GUIListControlEx.h"
#include "GUIImage.h"
#include "GUILabelControl.h"
#include "GUIEditControl.h"
#include "GUIFadeLabelControl.h"
#include "GUICheckMarkControl.h"
#include "GUIThumbnailPanel.h"
#include "GUIToggleButtonControl.h"
#include "GUITextBox.h"
#include "GUIVideoControl.h"
#include "GUIProgressControl.h"
#include "GUISliderControl.h"
#include "GUISelectButtonControl.h"
#include "GUIMoverControl.h"
#include "GUIResizeControl.h"
#include "GUIButtonScroller.h"
#include "GUIMultiImage.h"
#include "SkinInfo.h"
#include "../xbmc/xbox/XKUtils.h"
#include "../xbmc/Application.h"
#include "../xbmc/ApplicationMessenger.h"
#include "../xbmc/utils/GUIInfoManager.h"


CStdString CGUIWindow::CacheFilename = "";
CGUIWindow::VECREFERENCECONTOLS CGUIWindow::ControlsCache;

CGUIWindow::CGUIWindow(DWORD dwID, const CStdString &xmlFile)
{
  m_dwWindowId = dwID;
  m_xmlFile = xmlFile;
  m_dwIDRange = 1;
  m_dwPreviousWindowId = WINDOW_HOME;
  m_dwDefaultFocusControlID = 0;
  m_bRelativeCoords = false;
  m_iPosX = m_iPosY = m_dwWidth = m_dwHeight = 0;
  m_overlayState = OVERLAY_STATE_PARENT_WINDOW;   // Use parent or previous window's state
  m_WindowAllocated = false;
  m_coordsRes = g_guiSettings.m_LookAndFeelResolution;
  m_needsScaling = true;
  m_visibleCondition = 0;
  m_windowLoaded = false;
  m_loadOnDemand = true;
  m_alpha = 255;
  m_renderOrder = 0;
  m_effectType = EFFECT_TYPE_NONE;
  m_effectState = EFFECT_NONE;
  m_effectStart = 0;
  m_effectInTime = 0;
  m_effectOutTime = 0;
  m_dynamicResourceAlloc = true;
}

CGUIWindow::~CGUIWindow(void)
{}

void CGUIWindow::FlushReferenceCache()
{
  CacheFilename.clear();
  for (int i = 0; i < (int)ControlsCache.size();++i)
  {
    struct stReferenceControl stControl = ControlsCache[i];
    delete stControl.m_pControl;
  }
  ControlsCache.clear();
}

bool CGUIWindow::LoadReference(VECREFERENCECONTOLS& controls)
{
  // load references.xml
  controls.clear();
  TiXmlDocument xmlDoc;
  RESOLUTION res;
  CStdString strReferenceFile = g_SkinInfo.GetSkinPath("references.xml", &res);

  // this takes ages and happens about 20 times per skin load.
  // caching the data speeds up skin loading by a factor of 2. :)
  if (CacheFilename == strReferenceFile)
  {
    for (IVECREFERENCECONTOLS it = ControlsCache.begin(); it != ControlsCache.end(); ++it)
    {
      stReferenceControl stControl;
      strcpy(stControl.m_szType, it->m_szType);
      if (!strcmp(it->m_szType, "label"))
      {
        stControl.m_pControl = new CGUILabelControl(*((CGUILabelControl*)it->m_pControl));
      }
      else if (!strcmp(it->m_szType, "edit"))
      {
        stControl.m_pControl = new CGUIEditControl(*((CGUIEditControl*)it->m_pControl));
      }
      else if (!strcmp(it->m_szType, "videowindow"))
      {
        stControl.m_pControl = new CGUIVideoControl(*((CGUIVideoControl*)it->m_pControl));
      }
      else if (!strcmp(it->m_szType, "fadelabel"))
      {
        stControl.m_pControl = new CGUIFadeLabelControl(*((CGUIFadeLabelControl*)it->m_pControl));
      }
      else if (!strcmp(it->m_szType, "button"))
      {
        stControl.m_pControl = new CGUIButtonControl(*((CGUIButtonControl*)it->m_pControl));
      }
      else if (!strcmp(it->m_szType, "conditionalbutton"))
      {
        stControl.m_pControl = new CGUIConditionalButtonControl(*((CGUIConditionalButtonControl*)it->m_pControl));
      }
      else if (!strcmp(it->m_szType, "rss"))
      {
        stControl.m_pControl = new CGUIRSSControl(*((CGUIRSSControl*)it->m_pControl));
      }
      else if (!strcmp(it->m_szType, "ram"))
      {
        stControl.m_pControl = new CGUIRAMControl(*((CGUIRAMControl*)it->m_pControl));
      }
      else if (!strcmp(it->m_szType, "console"))
      {
        stControl.m_pControl = new CGUIConsoleControl(*((CGUIConsoleControl*)it->m_pControl));
      }
      else if (!strcmp(it->m_szType, "togglebutton"))
      {
        stControl.m_pControl = new CGUIToggleButtonControl(*((CGUIToggleButtonControl*)it->m_pControl));
      }
      else if (!strcmp(it->m_szType, "checkmark"))
      {
        stControl.m_pControl = new CGUICheckMarkControl(*((CGUICheckMarkControl*)it->m_pControl));
      }
      else if (!strcmp(it->m_szType, "radiobutton"))
      {
        stControl.m_pControl = new CGUIRadioButtonControl(*((CGUIRadioButtonControl*)it->m_pControl));
      }
      else if (!strcmp(it->m_szType, "spincontrol"))
      {
        stControl.m_pControl = new CGUISpinControl(*((CGUISpinControl*)it->m_pControl));
      }
      else if (!strcmp(it->m_szType, "slider"))
      {
        stControl.m_pControl = new CGUISliderControl(*((CGUISliderControl*)it->m_pControl));
      }
      else if (!strcmp(it->m_szType, "progress"))
      {
        stControl.m_pControl = new CGUIProgressControl(*((CGUIProgressControl*)it->m_pControl));
      }
      else if (!strcmp(it->m_szType, "image"))
      {
        stControl.m_pControl = new CGUIImage(*((CGUIImage*)it->m_pControl));
      }
      else if (!strcmp(it->m_szType, "multiimage"))
      {
        stControl.m_pControl = new CGUIMultiImage(*((CGUIMultiImage*)it->m_pControl));
      }
      else if (!strcmp(it->m_szType, "listcontrol"))
      {
        stControl.m_pControl = new CGUIListControl(*((CGUIListControl*)it->m_pControl));
      }
      else if (!strcmp(it->m_szType, "listcontrolex"))
      {
        stControl.m_pControl = new CGUIListControlEx(*((CGUIListControlEx*)it->m_pControl));
      }
      else if (!strcmp(it->m_szType, "textbox"))
      {
        stControl.m_pControl = new CGUITextBox(*((CGUITextBox*)it->m_pControl));
      }
      else if (!strcmp(it->m_szType, "thumbnailpanel"))
      {
        stControl.m_pControl = new CGUIThumbnailPanel(*((CGUIThumbnailPanel*)it->m_pControl));
      }
      else if (!strcmp(it->m_szType, "selectbutton"))
      {
        stControl.m_pControl = new CGUISelectButtonControl(*((CGUISelectButtonControl*)it->m_pControl));
      }
      else if (!strcmp(it->m_szType, "mover"))
      {
        stControl.m_pControl = new CGUIMoverControl(*((CGUIMoverControl*)it->m_pControl));
      }
      else if (!strcmp(it->m_szType, "resize"))
      {
        stControl.m_pControl = new CGUIResizeControl(*((CGUIResizeControl*)it->m_pControl));
      }
      else if (!strcmp(it->m_szType, "buttonscroller"))
      {
        stControl.m_pControl = new CGUIButtonScroller(*((CGUIButtonScroller*)it->m_pControl));
      }
      else if (!strcmp(it->m_szType, "spincontrolex"))
      {
        stControl.m_pControl = new CGUISpinControlEx(*((CGUISpinControlEx*)it->m_pControl));
      }
      controls.push_back(stControl);
    }
    return true;
  }

  CLog::Log(LOGINFO, "Loading references file: %s", strReferenceFile.c_str());
  if ( !xmlDoc.LoadFile(strReferenceFile.c_str()) )
  {
    CLog::Log(LOGERROR, "unable to load:%s, Line %d\n%s", strReferenceFile.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  CStdString strValue = pRootElement->Value();
  if (strValue != CStdString("controls"))
  {
    CLog::Log(LOGERROR, "references.xml doesn't contain <controls>");
    return false;
  }
  CGUIControlFactory factory;
  string strType;
  const TiXmlNode *pControl = pRootElement->FirstChild();
  while (pControl)
  {
    TiXmlNode* pNode = pControl->FirstChild("type");
    if (pNode)
    {
      strType = pNode->FirstChild()->Value();
      CGUIControl* pGUIControl = factory.Create(m_dwWindowId, pControl, NULL, res);
      if (pGUIControl)
      {
        struct stReferenceControl stControl;
        strcpy(stControl.m_szType, strType.c_str());
        stControl.m_pControl = pGUIControl;
        controls.push_back(stControl);
      }
    }
    pControl = pControl->NextSibling();
  }
  CacheFilename = strReferenceFile;
  ControlsCache.clear();
  for (IVECREFERENCECONTOLS it = controls.begin(); it != controls.end(); ++it)
  {
    stReferenceControl stControl;
    strcpy(stControl.m_szType, it->m_szType);
    if (!strcmp(it->m_szType, "label"))
    {
      stControl.m_pControl = new CGUILabelControl(*((CGUILabelControl*)it->m_pControl));
    }
    else if (!strcmp(it->m_szType, "edit"))
    {
      stControl.m_pControl = new CGUIEditControl(*((CGUIEditControl*)it->m_pControl));
    }
    else if (!strcmp(it->m_szType, "videowindow"))
    {
      stControl.m_pControl = new CGUIVideoControl(*((CGUIVideoControl*)it->m_pControl));
    }
    else if (!strcmp(it->m_szType, "fadelabel"))
    {
      stControl.m_pControl = new CGUIFadeLabelControl(*((CGUIFadeLabelControl*)it->m_pControl));
    }
    else if (!strcmp(it->m_szType, "button"))
    {
      stControl.m_pControl = new CGUIButtonControl(*((CGUIButtonControl*)it->m_pControl));
    }
    else if (!strcmp(it->m_szType, "conditionalbutton"))
    {
      stControl.m_pControl = new CGUIConditionalButtonControl(*((CGUIConditionalButtonControl*)it->m_pControl));
    }
    else if (!strcmp(it->m_szType, "rss"))
    {
      stControl.m_pControl = new CGUIRSSControl(*((CGUIRSSControl*)it->m_pControl));
    }
    else if (!strcmp(it->m_szType, "ram"))
    {
      stControl.m_pControl = new CGUIRAMControl(*((CGUIRAMControl*)it->m_pControl));
    }
    else if (!strcmp(it->m_szType, "console"))
    {
      stControl.m_pControl = new CGUIConsoleControl(*((CGUIConsoleControl*)it->m_pControl));
    }
    else if (!strcmp(it->m_szType, "togglebutton"))
    {
      stControl.m_pControl = new CGUIToggleButtonControl(*((CGUIToggleButtonControl*)it->m_pControl));
    }
    else if (!strcmp(it->m_szType, "checkmark"))
    {
      stControl.m_pControl = new CGUICheckMarkControl(*((CGUICheckMarkControl*)it->m_pControl));
    }
    else if (!strcmp(it->m_szType, "radiobutton"))
    {
      stControl.m_pControl = new CGUIRadioButtonControl(*((CGUIRadioButtonControl*)it->m_pControl));
    }
    else if (!strcmp(it->m_szType, "spincontrol"))
    {
      stControl.m_pControl = new CGUISpinControl(*((CGUISpinControl*)it->m_pControl));
    }
    else if (!strcmp(it->m_szType, "slider"))
    {
      stControl.m_pControl = new CGUISliderControl(*((CGUISliderControl*)it->m_pControl));
    }
    else if (!strcmp(it->m_szType, "progress"))
    {
      stControl.m_pControl = new CGUIProgressControl(*((CGUIProgressControl*)it->m_pControl));
    }
    else if (!strcmp(it->m_szType, "image"))
    {
      stControl.m_pControl = new CGUIImage(*((CGUIImage*)it->m_pControl));
    }
    else if (!strcmp(it->m_szType, "multiimage"))
    {
      stControl.m_pControl = new CGUIMultiImage(*((CGUIMultiImage*)it->m_pControl));
    }
    else if (!strcmp(it->m_szType, "listcontrol"))
    {
      stControl.m_pControl = new CGUIListControl(*((CGUIListControl*)it->m_pControl));
    }
    else if (!strcmp(it->m_szType, "listcontrolex"))
    {
      stControl.m_pControl = new CGUIListControlEx(*((CGUIListControlEx*)it->m_pControl));
    }
    else if (!strcmp(it->m_szType, "textbox"))
    {
      stControl.m_pControl = new CGUITextBox(*((CGUITextBox*)it->m_pControl));
    }
    else if (!strcmp(it->m_szType, "thumbnailpanel"))
    {
      stControl.m_pControl = new CGUIThumbnailPanel(*((CGUIThumbnailPanel*)it->m_pControl));
    }
    else if (!strcmp(it->m_szType, "selectbutton"))
    {
      stControl.m_pControl = new CGUISelectButtonControl(*((CGUISelectButtonControl*)it->m_pControl));
    }
    else if (!strcmp(it->m_szType, "mover"))
    {
      stControl.m_pControl = new CGUIMoverControl(*((CGUIMoverControl*)it->m_pControl));
    }
    else if (!strcmp(it->m_szType, "resize"))
    {
      stControl.m_pControl = new CGUIResizeControl(*((CGUIResizeControl*)it->m_pControl));
    }
    else if (!strcmp(it->m_szType, "buttonscroller"))
    {
      stControl.m_pControl = new CGUIButtonScroller(*((CGUIButtonScroller*)it->m_pControl));
    }
    else if (!strcmp(it->m_szType, "spincontrolex"))
    {
      stControl.m_pControl = new CGUISpinControlEx(*((CGUISpinControlEx*)it->m_pControl));
    }
    ControlsCache.push_back(stControl);
  }

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
  m_vecGroups.erase(m_vecGroups.begin(), m_vecGroups.end());
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
  bool ret = Load(pRootElement, resToUse);
  LARGE_INTEGER end, freq;
  QueryPerformanceCounter(&end);
  QueryPerformanceFrequency(&freq);
  CLog::DebugLog("Load %s: %.2fms (%.2f ms xml load)", m_xmlFile.c_str(), 1000.f * (end.QuadPart - start.QuadPart) / freq.QuadPart, 1000.f * (lend.QuadPart - start.QuadPart) / freq.QuadPart);
  return ret;
}

bool CGUIWindow::Load(const TiXmlElement* pRootElement, RESOLUTION resToUse)
{
  m_dwDefaultFocusControlID = 0;
  m_bRelativeCoords = false;
  m_iPosX = m_iPosY = m_dwWidth = m_dwHeight = 0;
  m_overlayState = OVERLAY_STATE_PARENT_WINDOW;   // Use parent or previous window's state
  m_coordsRes = g_guiSettings.m_LookAndFeelResolution;
  m_visibleCondition = 0;
  m_effectType = EFFECT_TYPE_NONE;
  m_effectInTime = m_effectOutTime = 0;

  VECREFERENCECONTOLS referencecontrols;
  IVECREFERENCECONTOLS it;
  LoadReference(referencecontrols);
  const TiXmlNode *pChild = pRootElement->FirstChild();
  while (pChild)
  {
    CStdString strValue = pChild->Value();
    if (strValue == "id" && pChild->FirstChild())
    {
      m_dwWindowId = WINDOW_HOME + atoi(pChild->FirstChild()->Value());            // window Id's start at WINDOW_HOME
    }
    else if (strValue == "defaultcontrol" && pChild->FirstChild())
    {
      m_dwDefaultFocusControlID = atoi(pChild->FirstChild()->Value());
    }
    else if (strValue == "visible" && pChild->FirstChild())
    {
      const char *effect = pChild->ToElement()->Attribute("effect");
      if (effect && strcmpi(effect, "fade")) m_effectType = EFFECT_TYPE_FADE;
      pChild->ToElement()->Attribute("time", &m_effectInTime);
      m_effectOutTime = m_effectInTime;
      int fadetime;
      if (pChild->ToElement()->Attribute("intime", &fadetime)) m_effectInTime = fadetime;
      if (pChild->ToElement()->Attribute("outtime", &fadetime)) m_effectOutTime = fadetime;
      m_visibleCondition = g_infoManager.TranslateString(pChild->FirstChild()->Value());
    }
    else if (strValue == "zorder" && pChild->FirstChild())
    {
      m_renderOrder = atoi(pChild->FirstChild()->Value());
    }
    else if (strValue == "coordinates")
    {
      TiXmlNode* pSystem = pChild->FirstChild("system");
      if (pSystem)
      {
        int iCoordinateSystem = atoi(pSystem->FirstChild()->Value());
        m_bRelativeCoords = (iCoordinateSystem == 1);
      }

      TiXmlNode* pPosX = pChild->FirstChild("posX");
      if (pPosX)
      {
        m_iPosX = atoi(pPosX->FirstChild()->Value());
        g_graphicsContext.ScaleXCoord(m_iPosX, resToUse);
      }

      TiXmlNode* pPosY = pChild->FirstChild("posY");
      if (pPosY)
      {
        m_iPosY = atoi(pPosY->FirstChild()->Value());
        g_graphicsContext.ScaleYCoord(m_iPosY, resToUse);
      }
    }
    else if (strValue == "controls")
    {
      const TiXmlNode *pControl = pChild->FirstChild("control");
      while (pControl)
      {
        LoadControl(pControl, -1, referencecontrols, resToUse);
        pControl = pControl->NextSibling("control");
      }

      const TiXmlNode *pControlGroup = pChild->FirstChild("controlgroup");
      int iGroup = 0;
      while (pControlGroup)
      {
        const TiXmlNode *pControl = pControlGroup->FirstChild("control");
        // In this group no focus of the controls is remembered
        m_vecGroups.push_back( -1);
        while (pControl)
        {
          LoadControl(pControl, iGroup, referencecontrols, resToUse);
          pControl = pControl->NextSibling("control");
        }
        pControlGroup = pControlGroup->NextSibling("controlgroup");
        iGroup++;
      }
    }
    else if (strValue == "allowoverlay")
    {
      CStdString strValue = pChild->FirstChild()->Value();
      strValue.MakeLower();

      if (strValue == "yes")
        m_overlayState = OVERLAY_STATE_SHOWN;
      else if (strValue == "true")
        m_overlayState = OVERLAY_STATE_SHOWN;
      else if (strValue == "no")
        m_overlayState = OVERLAY_STATE_HIDDEN;
      else if (strValue == "false")
        m_overlayState = OVERLAY_STATE_HIDDEN;
    }

    pChild = pChild->NextSibling();
  }

  for (int i = 0; i < (int)referencecontrols.size();++i)
  {
    struct stReferenceControl stControl = referencecontrols[i];
    delete stControl.m_pControl;
  }
  m_windowLoaded = true;
  OnWindowLoaded();
  return true;
}

void CGUIWindow::LoadControl(const TiXmlNode* pControl, int iGroup, VECREFERENCECONTOLS& referencecontrols, RESOLUTION& resToUse)
{
  // get control type
  TiXmlNode* pNode = pControl->FirstChild("type");
  if (pNode)
  {
    string strType = pNode->FirstChild()->Value();

    // get reference control
    CGUIControl* pGUIReferenceControl = NULL;
    for (int i = 0; i < (int)referencecontrols.size(); ++i)
    {
      struct stReferenceControl stControl = referencecontrols[i];
      if (strType == stControl.m_szType)
      {
        pGUIReferenceControl = stControl.m_pControl;
        break;
      }
    }
    CGUIControlFactory factory;
    CGUIControl* pGUIControl = factory.Create(m_dwWindowId, pControl, pGUIReferenceControl, resToUse);
    if (pGUIControl)
    {
      pGUIControl->SetGroup(iGroup);
      Add(pGUIControl);

      if (m_bRelativeCoords)
      {
        DWORD dwMaxX = pGUIControl->GetXPosition() + pGUIControl->GetWidth();
        if (dwMaxX > m_dwWidth)
        {
          m_dwWidth = dwMaxX;
        }

        DWORD dwMaxY = pGUIControl->GetYPosition() + pGUIControl->GetHeight();
        if (dwMaxY > m_dwHeight)
        {
          m_dwHeight = dwMaxY;
        }
      }
    }
  }
}

void CGUIWindow::OnWindowLoaded() 
{
  DynamicResourceAlloc(true);
}

void CGUIWindow::SetPosition(int iPosX, int iPosY)
{
  m_iPosX = iPosX;
  m_iPosY = iPosY;
}

void CGUIWindow::CenterWindow()
{
  if (m_bRelativeCoords)
  {
    m_iPosX = (g_graphicsContext.GetWidth() - (int)m_dwWidth) / 2;
    m_iPosY = (g_graphicsContext.GetHeight() - (int)m_dwHeight) / 2;
  }
}

void CGUIWindow::Render()
{
  // If we're rendering from a different thread, then we should wait for the main
  // app thread to finish AllocResources(), as dynamic resources (images in particular)
  // will try and be allocated from 2 different threads, which causes nasty things
  // to occur.
  if (!m_WindowAllocated) return;

  g_graphicsContext.SetScalingResolution(m_coordsRes, m_iPosX, m_iPosY, m_needsScaling);
  g_graphicsContext.SetWindowAlpha(m_alpha);

  for (int i = 0; i < (int)m_vecControls.size(); i++)
  {
    CGUIControl *pControl = m_vecControls[i];
    if (pControl)
    {
      // reset control alpha level
      g_graphicsContext.SetControlAlpha(255);
      pControl->Render();
    }
  }
}

bool CGUIWindow::OnAction(const CAction &action)
{
  static bool PowerButtonDown = false;
  static DWORD PowerButtonCode;
  static DWORD MarkTime;

  if (action.wID == ACTION_TAKE_SCREENSHOT)
  {
    CUtil::TakeScreenshot();
    return true;
  }
  else if (action.wID == ACTION_POWERDOWN)
  {
    // Hold button for 3 secs to power down
    if (!PowerButtonDown)
    {
      MarkTime = GetTickCount();
      PowerButtonDown = true;
      PowerButtonCode = action.m_dwButtonCode;
    }
  }
  if (PowerButtonDown)
  {
    if (g_application.IsButtonDown(PowerButtonCode))
    {
      if (GetTickCount() >= MarkTime + 3000)
      {
        g_applicationMessenger.Shutdown();
        return true;
      }
    }
    else
      PowerButtonDown = false;
  }
  if (action.wID == ACTION_MOUSE)
  {
    OnMouseAction();
    return true;
  }
  for (ivecControls i = m_vecControls.begin();i != m_vecControls.end(); ++i)
  {
    CGUIControl* pControl = *i;
    if (pControl->HasFocus() && pControl->IsVisible())
    {
      return pControl->OnAction(action);
    }
  }

  // no control has focus?
  // set focus to the default control then
  CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), m_dwDefaultFocusControlID);
  OnMessage(msg);
  return false;
}

// OnMouseAction - called by OnAction()
void CGUIWindow::OnMouseAction()
{
  // save the mouse coordinates as we will need to change them for
  // relative coordinates or if the window is scaled
  int iPosX = g_Mouse.iPosX;
  int iPosY = g_Mouse.iPosY;
  if (m_coordsRes != g_graphicsContext.GetVideoResolution())
  {
    // calculate necessary scalings
    float fFromWidth = (float)g_settings.m_ResInfo[m_coordsRes].iWidth;
    float fFromHeight = (float)g_settings.m_ResInfo[m_coordsRes].iHeight;
    float fToWidth = (float)g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].iWidth;
    float fToHeight = (float)g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].iHeight;
    float fScaleX = fToWidth / fFromWidth;
    float fScaleY = fToHeight / fFromHeight;
    g_Mouse.iPosX = (int)((float)iPosX / fScaleX + 0.5f);
    g_Mouse.iPosY = (int)((float)iPosY / fScaleY + 0.5f);
  }
  if (m_bRelativeCoords)
  {
    g_Mouse.iPosX = iPosX - m_iPosX;
    g_Mouse.iPosY = iPosY - m_iPosY;
  }
  bool bHandled = false;
  // check if we have exclusive access
  if (g_Mouse.GetExclusiveWindowID() == GetID())
  { // we have exclusive access to the mouse...
    CGUIControl *pControl = (CGUIControl *)GetControl(g_Mouse.GetExclusiveControlID());
    if (pControl)
    { // this control has exclusive access to the mouse
      HandleMouse(pControl);
      goto finished;
    }
  }

  // run through the controls, and find which one is under the pointer
  for (ivecControls i = m_vecControls.begin(); i != m_vecControls.end(); ++i)
  {
    CGUIControl *pControl = *i;
    if (pControl->CanFocus())
    {
      if (!bHandled && pControl->HitTest(g_Mouse.iPosX, g_Mouse.iPosY))
      { // OK, now check which action we should perform
        bHandled = HandleMouse(pControl);
      }
      else
      { // make sure that focus is not set
        pControl->SetFocus(false);
      }
    }
  }
  if (!bHandled)
  { // haven't handled this action - call the window message handlers
    OnMouse();
  }

finished:
  // correct the mouse coordinates back to what they were
  g_Mouse.iPosX = iPosX;
  g_Mouse.iPosY = iPosY;
}

// Handles any mouse actions that are not handled by a control
// default is to go back a window on a right click.
// This function should be overridden for other windows
bool CGUIWindow::OnMouse()
{
  if (g_Mouse.bClick[MOUSE_RIGHT_BUTTON])
  { // no control found to absorb this click - go to previous menu
    CAction action;
    action.wID = ACTION_PREVIOUS_MENU;
    return OnAction(action);
  }
  return false;
}

bool CGUIWindow::HandleMouse(CGUIControl *pControl)
{
  bool bHandled = false;
  // Issue the MouseOver event to highlight the item, and perform any pointer changes
  pControl->OnMouseOver();
  if (g_Mouse.bClick[MOUSE_LEFT_BUTTON])
  { // Left click
    pControl->OnMouseClick(MOUSE_LEFT_BUTTON);
    bHandled = true;
  }
  if (g_Mouse.bClick[MOUSE_RIGHT_BUTTON])
  { // Right click
    pControl->OnMouseClick(MOUSE_RIGHT_BUTTON);
    bHandled = true;
  }
  if (g_Mouse.bClick[MOUSE_MIDDLE_BUTTON])
  { // Middle click
    pControl->OnMouseClick(MOUSE_MIDDLE_BUTTON);
    bHandled = true;
  }
  if (g_Mouse.bDoubleClick[MOUSE_LEFT_BUTTON])
  { // Left double click
    pControl->OnMouseDoubleClick(MOUSE_LEFT_BUTTON);
    bHandled = true;
  }
  if (g_Mouse.bHold[MOUSE_LEFT_BUTTON] && (g_Mouse.cMickeyX || g_Mouse.cMickeyY))
  { // Mouse Drag
    pControl->OnMouseDrag();
  }
  if (g_Mouse.cWheel)
  { // Mouse wheel
    pControl->OnMouseWheel();
    bHandled = true;
  }
  return true; //bHandled;
}

DWORD CGUIWindow::GetID(void) const
{
  return m_dwWindowId;
}

void CGUIWindow::SetID(DWORD dwID)
{
  m_dwWindowId = dwID;
}

DWORD CGUIWindow::GetPreviousWindowID(void) const
{
  return m_dwPreviousWindowId;
}

void CGUIWindow::OnInitWindow()
{
  // set our initial visibility
  SetControlVisibility();

  CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), m_dwDefaultFocusControlID);
  OnMessage(msg);

  if (m_overlayState!=OVERLAY_STATE_PARENT_WINDOW) // True, use our own overlay state
    m_gWindowManager.ShowOverlay(m_overlayState==OVERLAY_STATE_SHOWN ? true : false);
}

bool CGUIWindow::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      CStdString strLine;
      wstring wstrLine;
      wstrLine = g_localizeStrings.Get(10000 + GetID());
      CUtil::Unicode2Ansi(wstrLine, strLine);
      OutputDebugString("------------------- GUI_MSG_WINDOW_INIT ");
      OutputDebugString(strLine.c_str());
      OutputDebugString("------------------- \n");
      SetAlpha(255);  // initial fade state
      if (m_dynamicResourceAlloc || !m_WindowAllocated) AllocResources();
      if (message.GetParam1() != WINDOW_INVALID)
      {
        m_dwPreviousWindowId = message.GetParam1();
      }
      OnInitWindow();
      return true;
    }
    break;

  case GUI_MSG_WINDOW_DEINIT:
    {
      CStdString strLine;
      wstring wstrLine;
      wstrLine = g_localizeStrings.Get(10000 + GetID());
      CUtil::Unicode2Ansi(wstrLine, strLine);
      OutputDebugString("------------------- GUI_MSG_WINDOW_DEINIT ");
      OutputDebugString(strLine.c_str());
      OutputDebugString("------------------- \n");
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

  case GUI_MSG_SETFOCUS:
    {
      CStdString strTmp;
      //strTmp.Format("set focus to control:%i window:%i (%i)\n", message.GetControlId(),message.GetSenderId(), GetID());
      //OutputDebugString(strTmp.c_str());
      if ( message.GetControlId() )
      {
        ivecControls i;
        for (i = m_vecControls.begin();i != m_vecControls.end(); ++i)
        {
          CGUIControl* pControl = *i;
          if (pControl->HasFocus() )
          {
            CGUIMessage msgLostFocus(GUI_MSG_LOSTFOCUS, GetID(), pControl->GetID(), message.GetControlId());
            pControl->OnMessage(msgLostFocus);
          }
        }

        CGUIControl* pFocusedControl = NULL;
        for (i = m_vecControls.begin();i != m_vecControls.end(); ++i)
        {
          CGUIControl* pControl = *i;
          if (pControl->GetID() == message.GetControlId() )
            pFocusedControl = pControl;
        }

        //  Handle control group changes
        if (pFocusedControl)
        {
          int iOldControlGroup = -1;
          int iOldControlId = message.GetSenderId();

          //  Is Message sender a window?
          if (iOldControlId > WINDOW_INVALID)
          {
            iOldControlId = -1;
          }
          else
          {
            const CGUIControl* pOldFocusedControl = GetControl(message.GetSenderId());
            if (pOldFocusedControl)
              iOldControlGroup = pOldFocusedControl->GetGroup();
          }

          //  Save last control of the group if the group changes
          if (iOldControlGroup > -1 && pFocusedControl->GetGroup() != iOldControlGroup)
            m_vecGroups[iOldControlGroup] = iOldControlId;

          //  if the control group changes...
          if (pFocusedControl->GetGroup() > -1 && pFocusedControl->GetGroup() != iOldControlGroup && iOldControlId > -1)
          {
            if (iOldControlId > -1)
            {
              //  ...get the last focused control of the new group...
              int iLastFocusedControl = m_vecGroups[pFocusedControl->GetGroup()];
              for (i = m_vecControls.begin();i != m_vecControls.end(); ++i)
              {
                CGUIControl* pControl = *i;
                if (pControl->GetID() == iLastFocusedControl )
                {
                  //  ...and focus the saved control.

                  //  Redirect our message to the new control and fake it a little
                  //  by saying the new control is sender and target
                  //  at once to prevent a stack overflow.
                  CGUIMessage newFocusMsg(GUI_MSG_SETFOCUS, pControl->GetID(), pControl->GetID(), message.GetParam1());
                  //  Remember new last control of group
                  m_vecGroups[pControl->GetGroup()] = pControl->GetID();
                  //  Send the message to the new focused
                  //  control throu the window.
                  OnMessage(newFocusMsg);
                  return true;
                }
              }
            }
          }

          //  Old and new control have no group, just pass the message
          pFocusedControl->OnMessage(message);
        }
      }
      return true;
    }
    break;
  }

  ivecControls i;
  for (i = m_vecControls.begin();i != m_vecControls.end(); ++i)
  {
    CGUIControl* pControl = *i;
    if (pControl)
    {
      if ( message.GetControlId() == pControl->GetID() )
      {
        return pControl->OnMessage(message);
      }
    }
  }
  return false;
}

void CGUIWindow::AllocResources(bool forceLoad /*= FALSE */)
{
  LARGE_INTEGER start;
  QueryPerformanceCounter(&start);

  // load skin xml file
  if (m_xmlFile.size() && (forceLoad || m_loadOnDemand || !m_windowLoaded)) Load(m_xmlFile);
  LARGE_INTEGER slend;
  QueryPerformanceCounter(&slend);

  // and now allocate resources
  g_TextureManager.StartPreLoad();
  ivecControls i;
  for (i = m_vecControls.begin();i != m_vecControls.end(); ++i)
  {
    CGUIControl* pControl = *i;
    if (pControl->GetControlType() != CGUIControl::GUICONTROL_IMAGE || !pControl->IsDynamicallyAllocated()) 
      pControl->PreAllocResources();
  }
  g_TextureManager.EndPreLoad();

  LARGE_INTEGER plend;
  QueryPerformanceCounter(&plend);

  for (i = m_vecControls.begin();i != m_vecControls.end(); ++i)
  {
    CGUIControl* pControl = *i;
    if (pControl->GetControlType() != CGUIControl::GUICONTROL_IMAGE || !pControl->IsDynamicallyAllocated()) 
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

void CGUIWindow::Remove(DWORD dwId)
{
  ivecControls i = m_vecControls.begin();
  while (i != m_vecControls.end())
  {
    CGUIControl* pControl = *i;
    if (pControl->GetID() == dwId)
    {
      m_vecControls.erase(i);
      return ;
    }
    ++i;
  }
}

int CGUIWindow::GetFocusControl()
{
  for (int i = 0; i < (int)m_vecControls.size(); ++i)
  {
    CGUIControl* pControl = m_vecControls[i];
    if (pControl->HasFocus()) return i;
  }
  return -1;
}

void CGUIWindow::SelectPreviousControl()
{
  int i = GetFocusControl();
  while (1)
  {
    if ( i < 0 || i >= (int)m_vecControls.size() )
    {
      i = (int)m_vecControls.size();
    }
    if (m_vecControls[i]->CanFocus()) break;
    else i--;
  }
  CGUIMessage msgSetFocus(GUI_MSG_SETFOCUS, GetID(), m_vecControls[i]->GetID() );
  g_graphicsContext.SendMessage(msgSetFocus);
}

void CGUIWindow::SelectNextControl()
{
  int i = GetFocusControl() + 1;
  while (1)
  {
    if ( i < 0 || i >= (int)m_vecControls.size() )
    {
      i = 0;
    }
    if (m_vecControls[i]->CanFocus()) break;
    else i++;
  }
  CGUIMessage msgSetFocus(GUI_MSG_SETFOCUS, GetID(), m_vecControls[i]->GetID() );
  g_graphicsContext.SendMessage(msgSetFocus);
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
  for (int i = 0;i < (int)m_vecControls.size(); ++i)
  {
    const CGUIControl* pControl = m_vecControls[i];
    if (pControl->GetID() == iControl) return pControl;
  }
  return NULL;
}
int CGUIWindow::GetFocusedControl() const
{
  for (int i = 0;i < (int)m_vecControls.size(); ++i)
  {
    const CGUIControl* pControl = m_vecControls[i];
    if (pControl->HasFocus() ) return pControl->GetID();
  }
  return -1;
}

void CGUIWindow::ResetAllControls()
{
  for (int i = 0;i < (int)m_vecControls.size(); ++i)
  {
    CGUIControl* pControl = m_vecControls[i];
    pControl->SetWidth( pControl->GetWidth() );
    pControl->Update();
  }
}

void CGUIWindow::Initialize()
{
  Load(m_xmlFile);
}

void CGUIWindow::SetControlVisibility()
{
  for (unsigned int i=0; i < m_vecControls.size(); i++)
  {
    CGUIControl *pControl = m_vecControls[i];
    if (pControl->GetVisibleCondition())
      pControl->SetInitialVisibility();
  }
}
