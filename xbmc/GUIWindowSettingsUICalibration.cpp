
#include "stdafx.h"
#include "GUIWindowSettingsUICalibration.h"
#include "settings.h"
#include "stdstring.h"
#include "application.h"

#define CONTROL_LABEL 2

CGUIWindowSettingsUICalibration::CGUIWindowSettingsUICalibration(void)
:CGUIWindow(0)
{
}

CGUIWindowSettingsUICalibration::~CGUIWindowSettingsUICalibration(void)
{
}


void CGUIWindowSettingsUICalibration::OnAction(const CAction &action)
{
	if (timeGetTime()-m_dwLastTime > 500)
	{
		m_iSpeed=1;
		m_iCountU=0;
		m_iCountD=0;
		m_iCountL=0;
		m_iCountR=0;
	}
	m_dwLastTime=timeGetTime();

	int iXOff = g_stSettings.m_iUIOffsetX;
	int iYOff = g_stSettings.m_iUIOffsetY;

	if (action.wID == ACTION_PREVIOUS_MENU)
    {
		m_gWindowManager.PreviousWindow();
		return;
    }
	if (m_iSpeed>10) m_iSpeed=10; // Speed limit for accellerated cursors

	switch (action.wID)
	{
		case ACTION_MOVE_LEFT:
		{
			if (m_iCountL==0) m_iSpeed=1;
			if (iXOff>-128)
			{
				iXOff-=m_iSpeed;
				m_iCountL++;
				if (m_iCountL > 5) {
					m_iSpeed+=1;
					m_iCountL=1;
				}
			}
			m_iCountU=0;
			m_iCountD=0;
			m_iCountR=0;
		}
		break;

		case ACTION_MOVE_RIGHT:
		{
			if (m_iCountR==0) m_iSpeed=1;
			if (iXOff<128)
			{
				iXOff+=m_iSpeed;
				m_iCountR++;
				if (m_iCountR > 5) {
					m_iSpeed+=1;
					m_iCountR=1;
				}
			}
			m_iCountU=0;
			m_iCountD=0;
			m_iCountL=0;
		}
		break;

		case ACTION_MOVE_UP:
		{
			if (m_iCountU==0) m_iSpeed=1;
			if (iYOff>-128)
			{
				iYOff-=m_iSpeed;
				m_iCountU++;
				if (m_iCountU > 5) {
					m_iSpeed+=1;
					m_iCountU=1;
				}
			}
			m_iCountD=0;
			m_iCountL=0;
			m_iCountR=0;
		}
		break;

		case ACTION_MOVE_DOWN:
		{
			if (m_iCountD==0) m_iSpeed=1;
			if ( iYOff< 128 )
			{
				iYOff+=m_iSpeed;
				m_iCountD++;
				if (m_iCountD > 5) {
					m_iSpeed+=1;
					m_iCountD=1;
				}
			}
			m_iCountU=0;
			m_iCountL=0;
			m_iCountR=0;
		}
		break;

		case ACTION_CALIBRATE_RESET:
			iXOff=0;
			iYOff=0;
			m_iSpeed=1;
			m_iCountU=0;
			m_iCountD=0;
			m_iCountL=0;
			m_iCountR=0;
		break;
		case ACTION_ANALOG_MOVE:
			float fX=2*action.fAmount1;
			float fY=2*action.fAmount2;
			if ( fX || fY )
			{
				iXOff += (int)fX;
				if ( iXOff < -128	 ) iXOff=-128;
				if ( iXOff > 128 ) iXOff=128;

				iYOff -= (int)fY;
				if ( iYOff < -128	 ) iYOff=-128;
				if ( iYOff > 128 ) iYOff=128;
			}
		break;
	}
	// do the movement
	if (g_stSettings.m_iUIOffsetX != iXOff || g_stSettings.m_iUIOffsetY != iYOff)
	{
		g_stSettings.m_iUIOffsetX=iXOff ;
		g_stSettings.m_iUIOffsetY=iYOff ;

		CStdString strOffset;
		strOffset.Format("%i,%i", iXOff, iYOff);
		SET_CONTROL_LABEL(GetID(), CONTROL_LABEL,	strOffset);

		g_graphicsContext.SetOffset(g_stSettings.m_iUIOffsetX, g_stSettings.m_iUIOffsetY);
		ResetAllControls();
		g_application.ResetAllControls();
	}
	CGUIWindow::OnAction(action);
}

bool CGUIWindowSettingsUICalibration::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
		case GUI_MSG_WINDOW_DEINIT:
		{
			g_settings.Save();
		}
		break;

		case GUI_MSG_WINDOW_INIT:
		{
			CGUIWindow::OnMessage(message);
			m_iSpeed=1;
			m_iCountU=0;
			m_iCountD=0;
			m_iCountL=0;
			m_iCountR=0;

			CStdString strOffset;
			strOffset.Format("%i,%i", g_stSettings.m_iUIOffsetX, g_stSettings.m_iUIOffsetY);
			SET_CONTROL_LABEL(GetID(), CONTROL_LABEL,	strOffset);
			return true;
		}
		break;
	}
 return CGUIWindow::OnMessage(message);
}
