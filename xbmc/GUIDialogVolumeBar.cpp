
#include "stdafx.h"
#include "GUIDialogVolumeBar.h"
#include "GUISliderControl.h"
#include "guiWindowManager.h"
#include "settings.h"
#include "application.h"
#include "localizeStrings.h"

// May need to change this so that it is "modeless" rather than Modal,
// though it works reasonably well as is...

#define VOLUME_BAR_DISPLAY_TIME 1000L

#define POPUP_VOLUME_SLIDER     401
#define POPUP_VOLUME_LEVEL_TEXT 402

CGUIDialogVolumeBar::CGUIDialogVolumeBar(void)
:CGUIDialog(0)
{
}

CGUIDialogVolumeBar::~CGUIDialogVolumeBar(void)
{
}

void CGUIDialogVolumeBar::OnAction(const CAction &action)
{
	if (action.wID == ACTION_VOLUME_UP || action.wID == ACTION_VOLUME_DOWN)
	{	// reset the timer, as we've changed the volume level
		ResetTimer();
		return;
	}
	if (action.wID == ACTION_CLOSE_DIALOG || action.wID == ACTION_PREVIOUS_MENU)
	{
		Close();
		return;
	}
	CGUIWindow::OnAction(action);
}

bool CGUIDialogVolumeBar::OnMessage(CGUIMessage& message)
{
	switch ( message.GetMessage() )
	{
		case GUI_MSG_WINDOW_INIT:
		{
      //resources are allocated in g_application
			//CGUIDialog::OnMessage(message);		
			// start timer
			m_dwTimer = timeGetTime();
			// levels are set in Render(), so no need to do them here...
			return true;
		}
		break;

 		case GUI_MSG_WINDOW_DEINIT:
		{
      //don't deinit, g_application handles it
			return true;
		}
		break;

		case GUI_MSG_CLICKED:
		{
			if (message.GetSenderId()==POPUP_VOLUME_SLIDER)	// who else is it going to be??
			{
				CGUISliderControl* pControl=(CGUISliderControl*)GetControl(message.GetSenderId());
				if (pControl)
				{
					// Set the global volume setting to the percentage requested
					int iPercentage=pControl->GetPercentage();
					g_application.SetVolume(iPercentage);
					// Label and control will auto-update when Render() is called.
				}
				// reset the timer
				m_dwTimer = timeGetTime();
				return true;
			}
		}
		break;
	}
	return CGUIDialog::OnMessage(message);
}

void CGUIDialogVolumeBar::ResetTimer()
{
	m_dwTimer = timeGetTime();
}

void CGUIDialogVolumeBar::Render()
{
	// set the level on our slider
	int iValue = g_application.GetVolume();
	CGUISliderControl *pSlider = (CGUISliderControl*)GetControl(POPUP_VOLUME_SLIDER);
	if (pSlider) pSlider->SetPercentage(iValue);			// Update our volume bar accordingly
	// and set the level in our text label
	CStdString strLevel;
	strLevel.Format("%2.1f dB",(float)g_stSettings.m_nVolumeLevel/100.0f);
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),POPUP_VOLUME_LEVEL_TEXT);
	msg.SetLabel(strLevel);
	OnMessage(msg);
	// and render the controls
	CGUIDialog::Render();
	// now check if we should exit
	if (timeGetTime() - m_dwTimer > VOLUME_BAR_DISPLAY_TIME)
	{
		Close();
		return;
	}
}