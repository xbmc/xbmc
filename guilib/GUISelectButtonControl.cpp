#include "guiselectbuttoncontrol.h"
#include "guifontmanager.h"
#include "guiwindowmanager.h"


CGUISelectButtonControl::CGUISelectButtonControl(DWORD dwParentID, DWORD dwControlId, 
																								 DWORD dwPosX, DWORD dwPosY, 
																								 DWORD dwWidth, DWORD dwHeight, 
																								 const CStdString& strButtonFocus,
																								 const CStdString& strButton,
																								 const CStdString& strSelectBackground,
																								 const CStdString& strSelectArrowLeft,
																								 const CStdString& strSelectArrowLeftFocus,
																								 const CStdString& strSelectArrowRight,
																								 const CStdString& strSelectArrowRightFocus)
:CGUIButtonControl(dwParentID, dwControlId, dwPosX, dwPosY,dwWidth, dwHeight, strButtonFocus, strButton)
,m_imgBackground(dwParentID, dwControlId, dwPosX, dwPosY, dwWidth, dwHeight,strSelectBackground)
,m_imgLeft(dwParentID, dwControlId, dwPosX, dwPosY, 16, 16,strSelectArrowLeft)
,m_imgLeftFocus(dwParentID, dwControlId, dwPosX, dwPosY, 16, 16,strSelectArrowLeftFocus)
,m_imgRight(dwParentID, dwControlId, dwPosX, dwPosY, 16, 16,strSelectArrowRight)
,m_imgRightFocus(dwParentID, dwControlId, dwPosX, dwPosY, 16, 16,strSelectArrowRightFocus)
{
	m_bShowSelect=false;
	m_iCurrentItem=-1;
	m_iDefaultItem=-1;
	m_iStartFrame=0;
	m_bLeftSelected=false;
	m_bRightSelected=false;
	m_dwTicks=0;
}

CGUISelectButtonControl::~CGUISelectButtonControl(void)
{
}

void CGUISelectButtonControl::Render()
{
	if (!IsVisible() ) return;


	//	Are we in selection mode
	if (m_bShowSelect)
	{
		//	Yes, render the select control

		//	render background, left and right arrow

		m_imgBackground.Render();

		D3DCOLOR dwTextColor=m_dwTextColor;

		//	User has moved left...
		if(m_bLeftSelected)
		{
			//	...render focused arrow
			m_iStartFrame++;
			if(m_iStartFrame>=10)
			{
				m_iStartFrame=0;
				m_bLeftSelected=false;
			}
			m_imgLeftFocus.Render();

			//	If we are moving left
			//	render item text as disabled
			dwTextColor=m_dwDisabledColor;
		}
		else
		{
			//	Render none focused arrow
			m_imgLeft.Render();
		}


		//	User has moved right...
		if(m_bRightSelected)
		{
			//	...render focused arrow
			m_iStartFrame++;
			if(m_iStartFrame>=10)
			{
				m_iStartFrame=0;
				m_bRightSelected=false;
			}
			m_imgRightFocus.Render();

			//	If we are moving right
			//	render item text as disabled
			dwTextColor=m_dwDisabledColor;
		}
		else
		{
			//	Render none focused arrow
			m_imgRight.Render();
		}


		//	Render text if a current item is available
		if (m_iCurrentItem>=0 && m_pFont)
			m_pFont->DrawText((float)m_dwPosX+m_imgLeft.GetWidth()+15, (float)2+m_dwPosY, dwTextColor, m_vecItems[m_iCurrentItem].c_str());


		//	Select current item, if user doesn't 
		//	move left or right for 1.5 sec.
		DWORD dwTicksSpan=timeGetTime()-m_dwTicks;
		if ((float)(dwTicksSpan/1000)>1.5f)
		{
			//	User hasn't moved disable selection mode...
			m_bShowSelect=false;

			//	...and send a thread message.
			//	(Sending a message with SendMessage 
			//	can result in a GPF.)
			CGUIMessage message(GUI_MSG_CLICKED,GetID(), GetParentID() );
			m_gWindowManager.SendThreadMessage(message);
		}

	}	//	if (m_bShowSelect)
	else
	{
		//	No, render a normal button
		CGUIButtonControl::Render();
	}

}

bool CGUISelectButtonControl::OnMessage(CGUIMessage& message)
{
	if ( message.GetControlId()==GetID() )
	{
		if (message.GetMessage() == GUI_MSG_LABEL_ADD)
		{
			if (m_vecItems.size()<=0)
			{
				m_iCurrentItem=0;
				m_iDefaultItem=0;
			}
			m_vecItems.push_back(message.GetLabel());
		}
		else if (message.GetMessage() == GUI_MSG_LABEL_RESET)
		{
			m_vecItems.erase(m_vecItems.begin(), m_vecItems.end());
			m_iCurrentItem=-1;
			m_iDefaultItem=-1;
		}
		else if (message.GetMessage()==GUI_MSG_ITEM_SELECTED)
		{
			message.SetParam1(m_iCurrentItem);
		}
		else if (message.GetMessage()==GUI_MSG_ITEM_SELECT)
		{
			m_iDefaultItem=m_iCurrentItem=message.GetParam1();
		}
	}

	return CGUIButtonControl::OnMessage(message);
}

void CGUISelectButtonControl::OnAction(const CAction &action) 
{
	if (!m_bShowSelect)
	{
		if (action.wID == ACTION_SELECT_ITEM)
		{
			//	Enter selection mode
			m_bShowSelect=true;

			//	Start timer, if user doesn't select an item
			//	or moves left/right. The control will 
			//	automatically select the current item.
			m_dwTicks=timeGetTime();
		}
		else
			CGUIButtonControl::OnAction(action);
	}
	else
	{
		if (action.wID == ACTION_SELECT_ITEM)
		{
			//	User has selected an item, disable selection mode...
			m_bShowSelect=false;

			// ...and send a message.
			CGUIMessage message(GUI_MSG_CLICKED,GetID(), GetParentID() );
			g_graphicsContext.SendMessage(message);
			return;
		}
		else if (action.wID == ACTION_MOVE_LEFT)
		{
			//	Set for visual feedback
			m_bLeftSelected=true;
			m_iStartFrame=0;

			//	Reset timer for automatically selecting
			//	the current item.
			m_dwTicks=timeGetTime();

			//	Switch to previous item
			if (m_vecItems.size()>0)
			{
				m_iCurrentItem--;
				if (m_iCurrentItem<0) 
					m_iCurrentItem=m_vecItems.size()-1;
			}
			return;
		}
		else if (action.wID == ACTION_MOVE_RIGHT)
		{
			//	Set for visual feedback
			m_bRightSelected=true;
			m_iStartFrame=0;

			//	Reset timer for automatically selecting
			//	the current item.
			m_dwTicks=timeGetTime();

			//	Switch to next item
			if (m_vecItems.size()>0)
			{
				m_iCurrentItem++;
				if (m_iCurrentItem>=(int)m_vecItems.size()) 
					m_iCurrentItem=0;
			}
			return;
		}
		if (action.wID == ACTION_MOVE_UP || action.wID == ACTION_MOVE_DOWN )
		{
			//	Disable selection mode when moving up or down
			m_bShowSelect=false;
			m_iCurrentItem=m_iDefaultItem;
		}

		CGUIButtonControl::OnAction(action);
	}
}

void CGUISelectButtonControl::FreeResources()
{
	CGUIButtonControl::FreeResources();

	m_imgBackground.FreeResources();

	m_imgLeft.FreeResources();
	m_imgLeftFocus.FreeResources();

	m_imgRight.FreeResources();
	m_imgRightFocus.FreeResources();

	m_bShowSelect=false;
}

void CGUISelectButtonControl::PreAllocResources()
{
	CGUIButtonControl::PreAllocResources();

	m_imgBackground.PreAllocResources();

	m_imgLeft.PreAllocResources();
	m_imgLeftFocus.PreAllocResources();

	m_imgRight.PreAllocResources();
	m_imgRightFocus.PreAllocResources();
}

void CGUISelectButtonControl::AllocResources()
{
	CGUIButtonControl::AllocResources();

	m_imgBackground.AllocResources();
	
	m_imgLeft.AllocResources();
	m_imgLeftFocus.AllocResources();

	m_imgRight.AllocResources();
	m_imgRightFocus.AllocResources();

	//	Position right arrow
	DWORD dwPosX=(m_dwPosX+m_dwWidth-8) - 16;
  DWORD dwPosY=m_dwPosY+(m_dwHeight-16)/2;
  m_imgRight.SetPosition(dwPosX,dwPosY);
  m_imgRightFocus.SetPosition(dwPosX,dwPosY);

	//	Position left arrow
	dwPosX=m_dwPosX+8;
	m_imgLeft.SetPosition(dwPosX, dwPosY);
	m_imgLeftFocus.SetPosition(dwPosX, dwPosY);
}

void CGUISelectButtonControl::Update()
{
	CGUIButtonControl::Update();

  m_imgBackground.SetWidth(m_dwWidth);
  m_imgBackground.SetHeight(m_dwHeight);
}
