#include "GUIWindowSettingsUICalibration.h"
#include "settings.h"
#include "stdstring.h"

#define CONTROL_LABEL 2

CGUIWindowSettingsUICalibration::CGUIWindowSettingsUICalibration(void)
:CGUIWindow(0)
{
}

CGUIWindowSettingsUICalibration::~CGUIWindowSettingsUICalibration(void)
{
}


void CGUIWindowSettingsUICalibration::OnKey(const CKey& key)
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
	if (key.IsButton())
  {
    if ( key.GetButtonCode() == KEY_BUTTON_BACK  || key.GetButtonCode() == KEY_REMOTE_BACK)
    {
      m_gWindowManager.ActivateWindow(9); // back 2 home
      return;
    }
		if (m_iSpeed>10) m_iSpeed=10; // Speed limit for accellerated cursors


		switch (key.GetButtonCode()) 
		{
			case KEY_BUTTON_BLACK:
			case KEY_REMOTE_INFO:
				iXOff=0;
				iYOff=0;
				m_iSpeed=1;
				m_iCountU=0;
				m_iCountD=0;
				m_iCountL=0;
				m_iCountR=0;
			break;

			case KEY_BUTTON_DPAD_LEFT:
			case KEY_REMOTE_LEFT:
			{
				if (iXOff>-128)
				{
					iXOff-=m_iSpeed;
					m_iCountL++;
					if (m_iCountL > 5) {
						m_iSpeed+=1;
						m_iCountL=1;
					}
				}
			}
			break;

			case KEY_BUTTON_DPAD_RIGHT:
			case KEY_REMOTE_RIGHT:
			{
				if (iXOff<128)
				{
					iXOff+=m_iSpeed;
					m_iCountR++;
					if (m_iCountR > 5) {
						m_iSpeed+=1;
						m_iCountR=1;
					}
				}
			}
			break;

			case KEY_BUTTON_DPAD_UP:
			case KEY_REMOTE_UP:
			{
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

			case KEY_BUTTON_DPAD_DOWN:
			case KEY_REMOTE_DOWN:
			{
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

		}
  }

	float fX=2*key.GetThumbX();
	float fY=2*key.GetThumbY();
	if ( (fX<=-1 || fX >=1) || (fY<=-1 || fY >=1) )
	{
		iXOff += (int)fX;
		if ( iXOff < -128	 ) iXOff=-128;
		if ( iXOff > 128 ) iXOff=128;

		iYOff += (int)fY;
		if ( iYOff < -128	 ) iYOff=-128;
		if ( iYOff > 128 ) iYOff=128;

	}
	if (g_stSettings.m_iUIOffsetX != iXOff || g_stSettings.m_iUIOffsetY != iYOff)
	{
		g_stSettings.m_iUIOffsetX=iXOff ;
		g_stSettings.m_iUIOffsetY=iYOff ;

		CStdString strOffset;
		strOffset.Format("%i,%i", iXOff, iYOff);
		SET_CONTROL_LABEL(GetID(), CONTROL_LABEL,	strOffset);


		g_graphicsContext.SetOffset(g_stSettings.m_iUIOffsetX, g_stSettings.m_iUIOffsetY);
		ResetAllControls();
	}
  CGUIWindow::OnKey(key);
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
			m_iSpeed=1;
			m_iCountU=0;
			m_iCountD=0;
			m_iCountL=0;
			m_iCountR=0;

			CStdString strOffset;
			strOffset.Format("%i,%i", g_stSettings.m_iUIOffsetX, g_stSettings.m_iUIOffsetY);
			SET_CONTROL_LABEL(GetID(), CONTROL_LABEL,	strOffset);
		}
		break;
	}
 return CGUIWindow::OnMessage(message);
}
