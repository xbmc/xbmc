#include "guiradiobuttoncontrol.h"

CGUIRadioButtonControl::CGUIRadioButtonControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strTextureFocus,const CStdString& strTextureNoFocus, const CStdString& strRadioFocus,const CStdString& strRadioNoFocus)
:CGUIButtonControl(dwParentID, dwControlId, dwPosX, dwPosY, dwWidth, dwHeight, strTextureFocus,strTextureNoFocus)
,m_imgRadioFocus(dwParentID, dwControlId, dwPosX, dwPosY,16,16,strTextureFocus)
,m_imgRadioNoFocus(dwParentID, dwControlId, dwPosX, dwPosY,16,16,strTextureNoFocus)
{
}

CGUIRadioButtonControl::~CGUIRadioButtonControl(void)
{
}


void CGUIRadioButtonControl::Render()
{
  if (!IsVisible()) return;
  CGUIButtonControl::Render();
  if ( IsSelected() )
  {
    if ( HasFocus() )
    {
      m_imgRadioFocus.Render();
    }
    else
    {
     m_imgRadioNoFocus.Render();
    }
  }
}

void CGUIRadioButtonControl::OnKey(const CKey& key) 
{
  CGUIButtonControl::OnKey(key);
  if ( key.IsButton() )
  {
    if (key.GetButtonCode() == KEY_BUTTON_A)
    {
      m_bSelected=!m_bSelected;
    }
  }
}

bool CGUIRadioButtonControl::OnMessage(CGUIMessage& message)
{
  return CGUIButtonControl::OnMessage(message);
}

void CGUIRadioButtonControl::AllocResources()
{
  CGUIButtonControl::AllocResources();
  m_imgRadioFocus.AllocResources();
  m_imgRadioNoFocus.AllocResources();

  DWORD dwPosX=(m_dwPosX+m_dwWidth-8) - 16;
  DWORD dwPosY=m_dwPosY+(m_dwHeight-16)/2;
  m_imgRadioFocus.SetPosition(dwPosX,dwPosY);
  m_imgRadioNoFocus.SetPosition(dwPosX,dwPosY);
  

}

void CGUIRadioButtonControl::FreeResources()
{
  CGUIButtonControl::FreeResources();
  m_imgRadioFocus.FreeResources();
  m_imgRadioNoFocus.FreeResources();
}


void  CGUIRadioButtonControl::Update()
{
  CGUIButtonControl::Update();
  
}