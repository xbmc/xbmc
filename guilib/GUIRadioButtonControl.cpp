#include "stdafx.h"
#include "guiradiobuttoncontrol.h"

CGUIRadioButtonControl::CGUIRadioButtonControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight,
											   const CStdString& strTextureFocus,const CStdString& strTextureNoFocus,
											   DWORD dwTextOffsetX, DWORD dwTextOffsetY,
											   const CStdString& strRadioFocus,const CStdString& strRadioNoFocus
											   )
:CGUIButtonControl(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight, strTextureFocus,strTextureNoFocus, dwTextOffsetX, dwTextOffsetY)
,m_imgRadioFocus(dwParentID, dwControlId, iPosX, iPosY,16,16,strRadioFocus)
,m_imgRadioNoFocus(dwParentID, dwControlId, iPosX, iPosY,16,16,strRadioNoFocus)
{
	ControlType = GUICONTROL_RADIO;
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

void CGUIRadioButtonControl::OnAction(const CAction &action) 
{
	CGUIButtonControl::OnAction(action);
	if (action.wID == ACTION_SELECT_ITEM)
	{
		m_bSelected=!m_bSelected;
	}
}

bool CGUIRadioButtonControl::OnMessage(CGUIMessage& message)
{
  return CGUIButtonControl::OnMessage(message);
}

void CGUIRadioButtonControl::PreAllocResources()
{
	CGUIButtonControl::PreAllocResources();
	m_imgRadioFocus.PreAllocResources();
	m_imgRadioNoFocus.PreAllocResources();
}

void CGUIRadioButtonControl::AllocResources()
{
  CGUIButtonControl::AllocResources();
  m_imgRadioFocus.AllocResources();
  m_imgRadioNoFocus.AllocResources();

  int iPosX=(m_iPosX+m_dwWidth-8) - 16;
  int iPosY=m_iPosY+(m_dwHeight-16)/2;
  m_imgRadioFocus.SetPosition(iPosX,iPosY);
  m_imgRadioNoFocus.SetPosition(iPosX,iPosY);
  

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




