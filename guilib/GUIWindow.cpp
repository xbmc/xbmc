#include "guiwindow.h"
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


bool CGUIWindow::Load(const CStdString& strFileName)
{
  TiXmlDocument xmlDoc;
  if ( !xmlDoc.LoadFile(strFileName.c_str()) )
  {
    return false;
  }
  TiXmlElement* pRootElement =xmlDoc.RootElement();
  CStdString strValue=pRootElement->Value();
  if (strValue!=CStdString("window")) return false;
  
  m_dwDefaultFocusControlID=0;
 
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
         CGUIControlFactory factory;
         CGUIControl* pGUIControl = factory.Create(m_dwWindowId,pControl);
         if (pGUIControl)
         {
           Add(pGUIControl);
         }
         pControl=pControl->NextSibling();
       }
    }

    pChild=pChild->NextSibling();
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
}

void CGUIWindow::FreeResources()
{
  ivecControls i;
  for (i=m_vecControls.begin();i != m_vecControls.end(); ++i)
  {
    CGUIControl* pControl= *i;
    pControl->FreeResources();
  }
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