#include "stdafx.h"
#include "guiwindow.h"
#include "LocalizeStrings.h"
#include "texturemanager.h"
#include "tinyxml/tinyxml.h"
#include "../xbmc/utils/log.h"
#include "../xbmc/util.h"
#include "GUIControlFactory.h"
#include "guiButtonControl.h"
#include "guiSpinButtonControl.h"
#include "guiRadioButtonControl.h"
#include "guiSpinControl.h"
#include "guiRSSControl.h"
#include "guiRAMControl.h"
#include "guiListControl.h"
#include "guiListControlEx.h"
#include "guiImage.h"
#include "GUILabelControl.h"
#include "GUIFadeLabelControl.h"
#include "GUICheckMarkControl.h"
#include "GUIThumbnailPanel.h"
#include "GUIMButtonControl.h"
#include "GUIToggleButtonControl.h" 
#include "GUITextBox.h" 
#include "guiVideoControl.h"
#include "GUIProgressControl.h"
#include "GUISliderControl.h"
#include "GUISelectButtonControl.h"
#include "GUIMoverControl.h"
#include "GUIResizeControl.h"
#include "SkinInfo.h"
#include "../xbmc/application.h"
#include "../xbmc/xbox/XKUtils.h"
//#include "../xbmc/util.h"

#include<string>
using namespace std;

CStdString CGUIWindow::CacheFilename = "";
CGUIWindow::VECREFERENCECONTOLS CGUIWindow::ControlsCache;

CGUIWindow::CGUIWindow(DWORD dwID)
{
  m_dwWindowId=dwID;
  m_dwPreviousWindowId=WINDOW_HOME;
  m_dwDefaultFocusControlID=0;
  m_bRelativeCoords = false;
  m_iPosX = m_iPosY = m_dwWidth = m_dwHeight = 0;
}

CGUIWindow::~CGUIWindow(void)
{
}

void CGUIWindow::FlushReferenceCache()
{
	CacheFilename.clear();
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
			strcpy(stControl.m_szType,it->m_szType);
			if (!strcmp(it->m_szType,"label"))
			{
				stControl.m_pControl = new CGUILabelControl(*((CGUILabelControl*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"videowindow"))
			{
				stControl.m_pControl = new CGUIVideoControl(*((CGUIVideoControl*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"fadelabel"))
			{
				stControl.m_pControl = new CGUIFadeLabelControl(*((CGUIFadeLabelControl*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"spinbutton"))
			{
				stControl.m_pControl = new CGUISpinButtonControl(*((CGUISpinButtonControl*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"button"))
			{
				stControl.m_pControl = new CGUIButtonControl(*((CGUIButtonControl*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"rss"))
			{
				stControl.m_pControl = new CGUIRSSControl(*((CGUIRSSControl*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"ram"))
			{
				stControl.m_pControl = new CGUIRAMControl(*((CGUIRAMControl*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"togglebutton"))
			{
				stControl.m_pControl = new CGUIToggleButtonControl(*((CGUIToggleButtonControl*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"buttonM"))
			{
				stControl.m_pControl = new CGUIMButtonControl(*((CGUIMButtonControl*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"checkmark"))
			{
				stControl.m_pControl = new CGUICheckMarkControl(*((CGUICheckMarkControl*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"radiobutton"))
			{
				stControl.m_pControl = new CGUIRadioButtonControl(*((CGUIRadioButtonControl*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"spincontrol"))
			{
				stControl.m_pControl = new CGUISpinControl(*((CGUISpinControl*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"slider"))
			{
				stControl.m_pControl= new CGUISliderControl(*((CGUISliderControl*)it->m_pControl));
			} 
			else if (!strcmp(it->m_szType,"progress"))
			{
				stControl.m_pControl= new CGUIProgressControl(*((CGUIProgressControl*)it->m_pControl));
			} 
			else if (!strcmp(it->m_szType,"image"))
			{
				stControl.m_pControl = new CGUIImage(*((CGUIImage*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"listcontrol"))
			{
				stControl.m_pControl = new CGUIListControl(*((CGUIListControl*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"listcontrolex"))
			{
				stControl.m_pControl = new CGUIListControlEx(*((CGUIListControlEx*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"textbox"))
			{
				stControl.m_pControl = new CGUITextBox(*((CGUITextBox*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"thumbnailpanel"))
			{
				stControl.m_pControl = new CGUIThumbnailPanel(*((CGUIThumbnailPanel*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"selectbutton"))
			{
				stControl.m_pControl = new CGUISelectButtonControl(*((CGUISelectButtonControl*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"mover"))
			{
				stControl.m_pControl = new CGUIMoverControl(*((CGUIMoverControl*)it->m_pControl));
			}
			else if (!strcmp(it->m_szType,"resize"))
			{
				stControl.m_pControl = new CGUIResizeControl(*((CGUIResizeControl*)it->m_pControl));
			}
			controls.push_back(stControl);
		}
		return true;
	}

	CLog::Log("Loading references file: %s", strReferenceFile.c_str());
	if ( !xmlDoc.LoadFile(strReferenceFile.c_str()) )
	{
    CLog::Log("unable to load:%s", strReferenceFile.c_str());
		return false;
	}

	TiXmlElement* pRootElement =xmlDoc.RootElement();
	CStdString strValue=pRootElement->Value();
	if (strValue!=CStdString("controls")) 
  {
    CLog::Log("references.xml doesnt contain <controls>");
    return false;
  }
	CGUIControlFactory factory;
	string strType;
	const TiXmlNode *pControl = pRootElement->FirstChild();
	while (pControl)
	{
		TiXmlNode* pNode=pControl->FirstChild("type");
		if (pNode)
		{
			strType = pNode->FirstChild()->Value();
			CGUIControl* pGUIControl = factory.Create(m_dwWindowId,pControl,NULL,res);
			if (pGUIControl)
			{	
				struct stReferenceControl stControl;
				strcpy(stControl.m_szType,strType.c_str());
				stControl.m_pControl=pGUIControl;
				controls.push_back(stControl);
			}
		}
		pControl=pControl->NextSibling();
	}
	CacheFilename = strReferenceFile;
	ControlsCache.clear();
	for (IVECREFERENCECONTOLS it = controls.begin(); it != controls.end(); ++it)
	{
		stReferenceControl stControl;
		strcpy(stControl.m_szType,it->m_szType);
		if (!strcmp(it->m_szType,"label"))
		{
			stControl.m_pControl = new CGUILabelControl(*((CGUILabelControl*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"videowindow"))
		{
			stControl.m_pControl = new CGUIVideoControl(*((CGUIVideoControl*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"fadelabel"))
		{
			stControl.m_pControl = new CGUIFadeLabelControl(*((CGUIFadeLabelControl*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"spinbutton"))
		{
			stControl.m_pControl = new CGUISpinButtonControl(*((CGUISpinButtonControl*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"button"))
		{
			stControl.m_pControl = new CGUIButtonControl(*((CGUIButtonControl*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"rss"))
		{
			stControl.m_pControl = new CGUIRSSControl(*((CGUIRSSControl*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"ram"))
		{
			stControl.m_pControl = new CGUIRAMControl(*((CGUIRAMControl*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"togglebutton"))
		{
			stControl.m_pControl = new CGUIToggleButtonControl(*((CGUIToggleButtonControl*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"buttonM"))
		{
			stControl.m_pControl = new CGUIMButtonControl(*((CGUIMButtonControl*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"checkmark"))
		{
			stControl.m_pControl = new CGUICheckMarkControl(*((CGUICheckMarkControl*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"radiobutton"))
		{
			stControl.m_pControl = new CGUIRadioButtonControl(*((CGUIRadioButtonControl*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"spincontrol"))
		{
			stControl.m_pControl = new CGUISpinControl(*((CGUISpinControl*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"slider"))
		{
			stControl.m_pControl= new CGUISliderControl(*((CGUISliderControl*)it->m_pControl));
		} 
		else if (!strcmp(it->m_szType,"progress"))
		{
			stControl.m_pControl= new CGUIProgressControl(*((CGUIProgressControl*)it->m_pControl));
		} 
		else if (!strcmp(it->m_szType,"image"))
		{
			stControl.m_pControl = new CGUIImage(*((CGUIImage*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"listcontrol"))
		{
			stControl.m_pControl = new CGUIListControl(*((CGUIListControl*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"listcontrolex"))
		{
			stControl.m_pControl = new CGUIListControlEx(*((CGUIListControlEx*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"textbox"))
		{
			stControl.m_pControl = new CGUITextBox(*((CGUITextBox*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"thumbnailpanel"))
		{
			stControl.m_pControl = new CGUIThumbnailPanel(*((CGUIThumbnailPanel*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"selectbutton"))
		{
			stControl.m_pControl = new CGUISelectButtonControl(*((CGUISelectButtonControl*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"mover"))
		{
			stControl.m_pControl = new CGUIMoverControl(*((CGUIMoverControl*)it->m_pControl));
		}
		else if (!strcmp(it->m_szType,"resize"))
		{
			stControl.m_pControl = new CGUIResizeControl(*((CGUIResizeControl*)it->m_pControl));
		}
		ControlsCache.push_back(stControl);
	}

	return true;
}

bool CGUIWindow::Load(const CStdString& strFileName, bool bContainsPath)
{
    m_vecPositions.erase(m_vecPositions.begin(),m_vecPositions.end());
	TiXmlDocument xmlDoc;
	// Find appropriate skin folder + resolution to load from
	RESOLUTION resToUse = INVALID;
	CStdString strPath;
	if (bContainsPath)
		strPath = strFileName;
	else
		strPath = g_SkinInfo.GetSkinPath(strFileName, &resToUse);

    if ( !xmlDoc.LoadFile(strPath.c_str()) )
    {
        CLog::Log("unable to load:%s", strPath.c_str());
		m_dwWindowId=WINDOW_INVALID;
        return false;
    }
  TiXmlElement* pRootElement =xmlDoc.RootElement();
  CStdString strValue=pRootElement->Value();
  if (strValue!=CStdString("window")) 
  {
    CLog::Log("file :%s doesnt contain <window>", strPath.c_str());
    return false;
  }
  
  m_dwDefaultFocusControlID=0;
  m_dwWidth=m_dwHeight=0;
	
	VECREFERENCECONTOLS  referencecontrols;
	IVECREFERENCECONTOLS it;
	LoadReference(referencecontrols);
  const TiXmlNode *pChild = pRootElement->FirstChild();
  while (pChild)
  {
    CStdString strValue=pChild->Value();
    if (strValue=="id")
    {
      m_dwWindowId=WINDOW_HOME + atoi(pChild->FirstChild()->Value());		// window Id's start at WINDOW_HOME
    }
    else if (strValue=="defaultcontrol")
    {
      m_dwDefaultFocusControlID=atoi(pChild->FirstChild()->Value());
    }
    else if (strValue=="coordinates")
    {
		TiXmlNode* pSystem=pChild->FirstChild("system");
		if (pSystem)
		{
			int iCoordinateSystem = atoi(pSystem->FirstChild()->Value());
			m_bRelativeCoords = (iCoordinateSystem==1);
		}

		TiXmlNode* pPosX=pChild->FirstChild("posX");
		if (pPosX)
		{
			m_iPosX = atoi(pPosX->FirstChild()->Value());
		}

		TiXmlNode* pPosY=pChild->FirstChild("posY");
		if (pPosY)
		{
			m_iPosY = atoi(pPosY->FirstChild()->Value());
		}
    }
    else if (strValue=="controls")
    {
			
       const TiXmlNode *pControl = pChild->FirstChild();
       while (pControl)
       {
				 // get control type
				 TiXmlNode* pNode=pControl->FirstChild("type");
				 if (pNode)
				 {
					string strType = pNode->FirstChild()->Value();
					 
					// get reference control
					CGUIControl* pGUIReferenceControl=NULL;
					for (int i=0; i < (int)referencecontrols.size(); ++i)
					{
						struct stReferenceControl stControl=referencecontrols[i];
						if (strType==stControl.m_szType)
						{
							pGUIReferenceControl=stControl.m_pControl;
							break;
						}
					}
					CGUIControlFactory factory;
					CGUIControl* pGUIControl = factory.Create(m_dwWindowId,pControl,pGUIReferenceControl, resToUse);
					if (pGUIControl)
					{
						Add(pGUIControl);

						if (m_bRelativeCoords)
						{
							DWORD dwMaxX = pGUIControl->GetXPosition()+pGUIControl->GetWidth();
							if (dwMaxX>m_dwWidth)
							{
								m_dwWidth=dwMaxX;
							}
							
							DWORD dwMaxY = pGUIControl->GetYPosition()+pGUIControl->GetHeight();
							if (dwMaxY>m_dwHeight)
							{
								m_dwHeight=dwMaxY;
							}
						}
					}
				}
         pControl=pControl->NextSibling();
       }
    }

    pChild=pChild->NextSibling();
  }

	
	for (int i=0; i < (int)referencecontrols.size();++i)
	{
		struct stReferenceControl stControl=referencecontrols[i];
		delete stControl.m_pControl;
	}
  OnWindowLoaded();
  return true;
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
		m_iPosX = (g_graphicsContext.GetWidth() - m_dwWidth) / 2;
		m_iPosY = (g_graphicsContext.GetHeight() - m_dwHeight) / 2;
	}
}

void CGUIWindow::Render()
{
	ivecControls i;

	for (i=m_vecControls.begin();i != m_vecControls.end(); ++i)
	{
		CGUIControl* pControl= *i;

		if (m_bRelativeCoords)
		{			
			int iPosX = pControl->GetXPosition();
			int iPosY = pControl->GetYPosition();
			pControl->SetPosition(iPosX+m_iPosX, iPosY+m_iPosY);
			pControl->Render();
			pControl->SetPosition(iPosX, iPosY);
		}
		else
		{
			pControl->Render();
		}
	}
}

void CGUIWindow::OnAction(const CAction &action)
{
	static bool PowerButtonDown = false;
	static DWORD PowerButtonCode;
	static DWORD MarkTime;

	if (action.wID == ACTION_TAKE_SCREENSHOT)
	{
		CUtil::TakeScreenshot();
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
			}
		}
		else
			PowerButtonDown = false;
	}
	if (action.wID == ACTION_MOUSE)
	{
		OnMouseAction();
		return;
	}
	for (ivecControls i=m_vecControls.begin();i != m_vecControls.end(); ++i)
	{
		CGUIControl* pControl= *i;
		if (pControl->HasFocus() )
		{
			pControl->OnAction(action);
			return;
		}
	}

	// no control has focus?
	// set focus to the default control then
	CGUIMessage msg(GUI_MSG_SETFOCUS,GetID(),m_dwDefaultFocusControlID);
	OnMessage(msg);
}

// OnMouseAction - called by OnAction()
void CGUIWindow::OnMouseAction()
{	// correct the mouse coordinates if we are using relative coordinates
	if (m_bRelativeCoords)
	{
		g_Mouse.iPosX -= m_iPosX;
		g_Mouse.iPosY -= m_iPosY;
	}
	bool bHandled = false;
	// check if we have exclusive access
	if (g_Mouse.GetExclusiveWindowID()==GetID())
	{	// we have exclusive access to the mouse...
		CGUIControl *pControl = (CGUIControl *)GetControl(g_Mouse.GetExclusiveControlID());
		if (pControl)
		{	// this control has exclusive access to the mouse
			HandleMouse(pControl);
			// re-correct the mouse coordinates if we are using relative coordinates
			if (m_bRelativeCoords)
			{
				g_Mouse.iPosX += m_iPosX;
				g_Mouse.iPosY += m_iPosY;
			}
			return;
		}
	}

	// run through the controls, and find which one is under the pointer
	for (ivecControls i=m_vecControls.begin(); i!=m_vecControls.end(); ++i)
	{
		CGUIControl *pControl = *i;
		if (pControl->CanFocus())
		{
			if (!bHandled && pControl->HitTest(g_Mouse.iPosX, g_Mouse.iPosY))
			{	// OK, now check which action we should perform
				bHandled = HandleMouse(pControl);
			}
			else
			{	// make sure that focus is not set
				pControl->SetFocus(false);
			}
		}
	}
	if (!bHandled)
	{	// haven't handled this action - call the window message handlers
		OnMouse();
	}
	// correct the mouse coordinates if we are using relative coordinates
	if (m_bRelativeCoords)
	{
		g_Mouse.iPosX += m_iPosX;
		g_Mouse.iPosY += m_iPosY;
	}
}

// Handles any mouse actions that are not handled by a control
// default is to go back a window on a right click.
// This function should be overridden for other windows
void CGUIWindow::OnMouse()
{	
	if (g_Mouse.bClick[MOUSE_RIGHT_BUTTON])
	{	// no control found to absorb this click - go to previous menu
		CAction action;
		action.wID = ACTION_PREVIOUS_MENU;
		OnAction(action);
	}
}

bool CGUIWindow::HandleMouse(CGUIControl *pControl)
{
	bool bHandled = false;
	// Issue the MouseOver event to highlight the item, and perform any pointer changes
	pControl->OnMouseOver();
	if (g_Mouse.bClick[MOUSE_LEFT_BUTTON])
	{	// Left click
		pControl->OnMouseClick(MOUSE_LEFT_BUTTON);
		bHandled = true;
	}
	if (g_Mouse.bClick[MOUSE_RIGHT_BUTTON])
	{	// Right click
		pControl->OnMouseClick(MOUSE_RIGHT_BUTTON);
		bHandled = true;
	}
	if (g_Mouse.bClick[MOUSE_MIDDLE_BUTTON])
	{	// Middle click
		pControl->OnMouseClick(MOUSE_MIDDLE_BUTTON);
		bHandled = true;
	}
	if (g_Mouse.bDoubleClick[MOUSE_LEFT_BUTTON])
	{	// Left double click
		pControl->OnMouseDoubleClick(MOUSE_LEFT_BUTTON);
		bHandled = true;
	}
	if (g_Mouse.bHold[MOUSE_LEFT_BUTTON] && (g_Mouse.cMickeyX || g_Mouse.cMickeyY))
	{	// Mouse Drag
		pControl->OnMouseDrag();
	}
	if (g_Mouse.cWheel)
	{	// Mouse wheel
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
}

bool CGUIWindow::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
    case GUI_MSG_WINDOW_INIT:
      {
        CStdString strLine;
        wstring wstrLine;
        wstrLine=g_localizeStrings.Get(10000+GetID());
        CUtil::Unicode2Ansi(wstrLine,strLine);
        OutputDebugString("------------------- GUI_MSG_WINDOW_INIT ");
        OutputDebugString(strLine.c_str());
        OutputDebugString("------------------- \n");
        AllocResources();
		    if (message.GetParam1()!=WINDOW_INVALID)
		    {
			    m_dwPreviousWindowId = message.GetParam1();
		    }
        CGUIMessage msg(GUI_MSG_SETFOCUS,GetID(), m_dwDefaultFocusControlID);
        g_graphicsContext.SendMessage(msg);
		OnInitWindow();
      }
      return true;
    break;

    
    case GUI_MSG_WINDOW_DEINIT:
    {
      CStdString strLine;
      wstring wstrLine;
      wstrLine=g_localizeStrings.Get(10000+GetID());
      CUtil::Unicode2Ansi(wstrLine,strLine);
      OutputDebugString("------------------- GUI_MSG_WINDOW_DEINIT ");
      OutputDebugString(strLine.c_str());
      OutputDebugString("------------------- \n");
      FreeResources();
      return true;
    }
    break;

    case GUI_MSG_CLICKED:
	{
		CLICK_EVENT clickEvent = m_mapClickEvents[ message.GetSenderId() ];

		if (clickEvent.HasAHandler())
		{
			clickEvent.Fire(message);
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
        for (i=m_vecControls.begin();i != m_vecControls.end(); ++i)
        {
          CGUIControl* pControl= *i;
          if (pControl->HasFocus() ) 
          {
            CGUIMessage msgLostFocus(GUI_MSG_LOSTFOCUS,GetID(),pControl->GetID(),message.GetControlId());
            pControl->OnMessage(msgLostFocus);

          }
        }
        for (i=m_vecControls.begin();i != m_vecControls.end(); ++i)
        {
          CGUIControl* pControl= *i;

          if (pControl)
          {
            if ( message.GetControlId() == pControl->GetID() )
            {
              pControl->OnMessage(message);
            }
          }
        }
      }
      return true;
    }
    break;
  }    

  ivecControls i;
  for (i=m_vecControls.begin();i != m_vecControls.end(); ++i)
  {
    CGUIControl* pControl= *i;
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

void CGUIWindow::AllocResources()
{
	LARGE_INTEGER start;
	QueryPerformanceCounter(&start);

	g_TextureManager.StartPreLoad();
	ivecControls i;
	for (i=m_vecControls.begin();i != m_vecControls.end(); ++i)
	{
		CGUIControl* pControl= *i;
		pControl->PreAllocResources();
	}
	g_TextureManager.EndPreLoad();

	LARGE_INTEGER plend;
	QueryPerformanceCounter(&plend);

  for (i=m_vecControls.begin();i != m_vecControls.end(); ++i)
  {
    CGUIControl* pControl= *i;
    pControl->AllocResources();
  }
	g_TextureManager.FlushPreLoad();

	LARGE_INTEGER end, freq;
	QueryPerformanceCounter(&end);
	QueryPerformanceFrequency(&freq);
	CLog::DebugLog("Alloc resources: %.2fms (%.2f ms preload)", 1000.f * (end.QuadPart - start.QuadPart) / freq.QuadPart, 1000.f * (plend.QuadPart - start.QuadPart) / freq.QuadPart);
}

void CGUIWindow::FreeResources()
{
  ivecControls i;
  //OutputDebugString(" free resources\n");
  for (i=m_vecControls.begin();i != m_vecControls.end(); ++i)
  {
    CGUIControl* pControl= *i;
    pControl->FreeResources();
  }
  //g_TextureManager.Dump();
}

void CGUIWindow::Add(CGUIControl* pControl)
{
  m_vecControls.push_back(pControl);
}

void CGUIWindow::Remove(DWORD dwId)
{
	ivecControls i = m_vecControls.begin();
  while(i != m_vecControls.end())
  {
    CGUIControl* pControl= *i;
		if (pControl->GetID() == dwId)
    {
			m_vecControls.erase(i);
			return;
    }
		++i;
  }
}

int CGUIWindow::GetFocusControl()
{
  for (int i=0; i < (int)m_vecControls.size(); ++i)
  {
    CGUIControl* pControl= m_vecControls[i];
    if (pControl->HasFocus()) return i;
  }
  return -1;
}

void CGUIWindow::SelectPreviousControl()
{
  int i=GetFocusControl()+1;
  while (1)
  {
    if ( i < 0 || i >= (int)m_vecControls.size() )
    {
      i=(int)m_vecControls.size();
    }
    if (m_vecControls[i]->CanFocus()) break;
    else i--;
  }
  CGUIMessage msgSetFocus(GUI_MSG_SETFOCUS,GetID(),m_vecControls[i]->GetID() );
  g_graphicsContext.SendMessage(msgSetFocus);
}

void CGUIWindow::SelectNextControl()
{
  int i=GetFocusControl()+1;
  while (1)
  {
    if ( i < 0 || i >= (int)m_vecControls.size() )
    {
      i=0;
    }
    if (m_vecControls[i]->CanFocus()) break;
    else i++;
  }
  CGUIMessage msgSetFocus(GUI_MSG_SETFOCUS,GetID(),m_vecControls[i]->GetID() );
  g_graphicsContext.SendMessage(msgSetFocus);
}


void CGUIWindow::ClearAll()
{
  for (int i=0; i < (int)m_vecControls.size(); ++i)
  {
    CGUIControl* pControl= m_vecControls[i];
    delete pControl;
  }
	m_vecControls.erase(m_vecControls.begin(),m_vecControls.end());
}

const CGUIControl* 	CGUIWindow::GetControl(int iControl) const
{
	for (int i=0;i < (int)m_vecControls.size(); ++i)
  {
    const CGUIControl* pControl= m_vecControls[i];
		if (pControl->GetID() == iControl) return pControl;
  }
	return NULL;
}
int	CGUIWindow::GetFocusedControl() const
{
	for (int i=0;i < (int)m_vecControls.size(); ++i)
  {
    const CGUIControl* pControl= m_vecControls[i];
		if (pControl->HasFocus() ) return pControl->GetID();
  }
	return -1;
}

void CGUIWindow::ResetAllControls()
{
	for (int i=0;i < (int)m_vecControls.size(); ++i)
  {
    CGUIControl* pControl= m_vecControls[i];
		pControl->SetWidth( pControl->GetWidth() );
	}
}

void CGUIWindow::OnWindowLoaded()
{
  m_vecPositions.erase(m_vecPositions.begin(),m_vecPositions.end());
	for (int i=0;i < (int)m_vecControls.size(); ++i)
  {
    CGUIControl* pControl= m_vecControls[i];
    CPosition pos;
    pos.pControl=pControl;
    pos.x=pControl->GetXPosition();
    pos.y=pControl->GetYPosition();
    m_vecPositions.push_back(pos);
	}
}