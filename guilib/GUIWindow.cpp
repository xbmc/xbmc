#include "guiwindow.h"
#include "texturemanager.h"
#include "tinyxml/tinyxml.h"
#include "GUIControlFactory.h"

#include<string>
using namespace std;

CGUIWindow::CGUIWindow(DWORD dwID)
{
  m_dwWindowId=dwID;
  m_dwDefaultFocusControlID=0;
}

CGUIWindow::~CGUIWindow(void)
{
}



bool CGUIWindow::LoadReference(const CStdString& strFileName, map<string,CGUIControl*>& controls)
{
	// load references.xml
	controls.erase(controls.begin(), controls.end());
	TiXmlDocument xmlDoc;
	int iPos = strFileName.ReverseFind('\\');
	CStdString strReferenceFile=strFileName.Left(iPos);
	strReferenceFile += "\\references.xml";
	if ( !xmlDoc.LoadFile(strReferenceFile.c_str()) )
	{
		OutputDebugString("Unable to load:");
		OutputDebugString(strReferenceFile.c_str());
		OutputDebugString("\n");
		return false;
	}

	TiXmlElement* pRootElement =xmlDoc.RootElement();
	CStdString strValue=pRootElement->Value();
	if (strValue!=CStdString("controls")) return false;

	string strType;
	const TiXmlNode *pControl = pRootElement->FirstChild();
	while (pControl)
	{
		CGUIControlFactory factory;
		CGUIControl* pGUIControl = factory.Create(m_dwWindowId,pControl,NULL);
		if (pGUIControl)
		{
			TiXmlNode* pNode=pControl->FirstChild("type");
			strType = pNode->FirstChild()->Value();
			controls[strType]=pGUIControl;
		}
		pControl=pControl->NextSibling();
	}
	return true;
}

bool CGUIWindow::Load(const CStdString& strFileName)
{
  TiXmlDocument xmlDoc;
  if ( !xmlDoc.LoadFile(strFileName.c_str()) )
  {
		OutputDebugString("Unable to load window:");
		OutputDebugString(strFileName.c_str());
		OutputDebugString("\n");
		m_dwWindowId=9999;
    return false;
  }
  TiXmlElement* pRootElement =xmlDoc.RootElement();
  CStdString strValue=pRootElement->Value();
  if (strValue!=CStdString("window")) return false;
  
  m_dwDefaultFocusControlID=0;
	
	map<string,CGUIControl*> referencecontrols;
	map<string,CGUIControl*>::iterator it;
	LoadReference(strFileName, referencecontrols);
 
  const TiXmlNode *pChild = pRootElement->FirstChild();
  while (pChild)
  {
    CStdString strValue=pChild->Value();
    if (strValue=="id")
    {
      m_dwWindowId=atoi(pChild->FirstChild()->Value());
    }
    if (strValue=="defaultcontrol")
    {
      m_dwDefaultFocusControlID=atoi(pChild->FirstChild()->Value());
    }
    if (strValue=="controls")
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
					it = referencecontrols.find(strType);
					if (it != referencecontrols.end() )
					{
						pGUIReferenceControl=it->second;
					}

					CGUIControlFactory factory;
					CGUIControl* pGUIControl = factory.Create(m_dwWindowId,pControl,pGUIReferenceControl);
					if (pGUIControl)
					{
						Add(pGUIControl);
					}
				}
         pControl=pControl->NextSibling();
       }
    }

    pChild=pChild->NextSibling();
  }

	
	it=referencecontrols.begin();
	while (it != referencecontrols.end() )
	{
		CGUIControl* pControl = it->second;
		delete pControl ;
		it = referencecontrols.erase(it);
	}
  return true;
}

void CGUIWindow::Render()
{
  ivecControls i;
  for (i=m_vecControls.begin();i != m_vecControls.end(); ++i)
  {
    CGUIControl* pControl= *i;
    pControl->Render();
  }
}

void CGUIWindow::OnKey(const CKey& key)
{
  ivecControls i;
  for (i=m_vecControls.begin();i != m_vecControls.end(); ++i)
  {
    CGUIControl* pControl= *i;
    if (pControl->HasFocus() )
    {
      pControl->OnKey(key);
      return;
    }
  }
}


DWORD CGUIWindow::GetID(void) const
{
  return m_dwWindowId;
}


bool CGUIWindow::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
    case GUI_MSG_WINDOW_INIT:
      {
        AllocResources();        
        CGUIMessage msg(GUI_MSG_SETFOCUS,GetID(), m_dwDefaultFocusControlID);
        g_graphicsContext.SendMessage(msg);

      }
      return true;
    break;

    
    case GUI_MSG_WINDOW_DEINIT:
      FreeResources();
      return true;
    break;

    case GUI_MSG_SETFOCUS:
    {
      if ( message.GetControlId() )
      {
        ivecControls i;
        for (i=m_vecControls.begin();i != m_vecControls.end(); ++i)
        {
          CGUIControl* pControl= *i;
          if (pControl->HasFocus() ) 
          {
            CGUIMessage msgLostFocus(GUI_MSG_LOSTFOCUS,GetID(),pControl->GetID()  );
            pControl->OnMessage(msgLostFocus);
          }
        }
        for (i=m_vecControls.begin();i != m_vecControls.end(); ++i)
        {
          CGUIControl* pControl= *i;

          if ( message.GetControlId() == pControl->GetID() )
          {
            pControl->OnMessage(message);
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
    if ( message.GetControlId() == pControl->GetID() )
    {
      return pControl->OnMessage(message);
    }
  }
  return false;
}

void CGUIWindow::AllocResources()
{
  ivecControls i;
  for (i=m_vecControls.begin();i != m_vecControls.end(); ++i)
  {
    CGUIControl* pControl= *i;
    pControl->AllocResources();
  }
  //g_TextureManager.Dump();
}

void CGUIWindow::FreeResources()
{
  ivecControls i;
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

int CGUIWindow::GetFocusControl()
{
  for (int i=0; i < (int)m_vecControls.size(); ++i)
  {
    CGUIControl* pControl= m_vecControls[i];
    if (pControl->HasFocus()) return i;
  }
  return -1;
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