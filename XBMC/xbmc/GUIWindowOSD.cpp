#include "localizestrings.h"
#include "GUIWindowOSD.h"
#include "application.h"
#include "GUIProgressControl.h"
#include "GUISliderControl.h"
#include "GUIToggleButtonControl.h"
#include "guilistitem.h"
#include "guilistcontrol.h"
#include "GUIImage.h"
#include "settings.h"
#include "guiFontManager.h"
#include "util.h"
#include "sectionloader.h"
#include "cores/mplayer/mplayer.h"

#define OSD_VIDEOPROGRESS 1
#define OSD_SKIPBWD 210
#define OSD_REWIND 211
#define OSD_STOP 212
#define OSD_PLAY 213
#define OSD_FFWD 214
#define OSD_SKIPFWD 215
#define OSD_MUTE 216
//#define OSD_SYNC 217 - not used
#define OSD_SUBTITLES 218
#define OSD_BOOKMARKS 219
#define OSD_VIDEO 220
#define OSD_AUDIO 221

#define OSD_VOLUMESLIDER 400

#define OSD_AVDELAY 500
#define OSD_AVDELAY_LABEL 550
#define OSD_AUDIOSTREAM_LIST 501

#define OSD_CREATEBOOKMARK 600
#define OSD_BOOKMARKS_LIST 601
#define OSD_BOOKMARKS_LIST_LABEL 650
#define OSD_CLEARBOOKMARKS 602

#define OSD_VIDEOPOS 700
#define OSD_VIDEOPOS_LABEL 750
#define OSD_NONINTERLEAVED 701
#define OSD_NOCACHE 702
#define OSD_ADJFRAMERATE 703

#define OSD_BRIGHTNESS 704
#define OSD_BRIGHTNESSLABEL 752

#define OSD_CONTRAST 705
#define OSD_CONTRASTLABEL 753

#define OSD_GAMMA 706
#define OSD_GAMMALABEL 754

#define OSD_SUBTITLE_DELAY 800
#define OSD_SUBTITLE_DELAY_LABEL 850
#define OSD_SUBTITLE_ONOFF 801
#define OSD_SUBTITLE_LIST 802

#define OSD_TIMEINFO 100
#define OSD_SUBMENU_BG_VOL 300
//#define OSD_SUBMENU_BG_SYNC 301	- not used
#define OSD_SUBMENU_BG_SUBTITLES 302
#define OSD_SUBMENU_BG_BOOKMARKS 303
#define OSD_SUBMENU_BG_VIDEO 304
#define OSD_SUBMENU_BG_AUDIO 305
#define OSD_SUBMENU_NIB 350

CGUIWindowOSD::CGUIWindowOSD(void)
:CGUIWindow(0)
{
	m_bSubMenuOn = false;
	m_iActiveMenu = 0;
	m_iActiveMenuButtonID = 0;
	m_iCurrentBookmark = 0;
}

CGUIWindowOSD::~CGUIWindowOSD(void)
{
}

void CGUIWindowOSD::Render()
{
	SetVideoProgress();			// get the percentage of playback complete so far
	Get_TimeInfo();				// show the time elapsed/total playing time
	CGUIWindow::Render();		// render our controls to the screen
}

void CGUIWindowOSD::OnAction(const CAction &action)
{
	switch (action.wID)
	{
		case ACTION_OSD_HIDESUBMENU:
			if (m_bSubMenuOn)						// is sub menu on?
			{
				SET_CONTROL_FOCUS(GetID(), m_iActiveMenuButtonID, 0);	// set focus to last menu button
				ToggleSubMenu(0, m_iActiveMenu);						// hide the currently active sub-menu
				return;
			}
			break;

		case ACTION_PAUSE:
		{
			// push a message through to this window to handle the remote control button
			CGUIMessage msgSet(GUI_MSG_CLICKED,OSD_PLAY,OSD_PLAY,0,0,NULL);
			OnMessage(msgSet);
		}
		break;

		case ACTION_MUSIC_PLAY:
		{
			// push a message through to this window to handle the remote control button
			CGUIMessage msgSet(GUI_MSG_CLICKED,OSD_PLAY,OSD_PLAY,0,0,NULL);
			OnMessage(msgSet);
		}
		break;

		case ACTION_STOP:
		{
			// push a message through to this window to handle the remote control button
			CGUIMessage msgSet(GUI_MSG_CLICKED,OSD_STOP,OSD_STOP,0,0,NULL);
			OnMessage(msgSet);
		}
		break;

		case ACTION_FORWARD:
		{
			// push a message through to this window to handle the remote control button
			CGUIMessage msgSet(GUI_MSG_CLICKED,OSD_FFWD,OSD_FFWD,0,0,NULL);
			OnMessage(msgSet);
		}
		break;

		case ACTION_REWIND:
		{
			// push a message through to this window to handle the remote control button
			CGUIMessage msgSet(GUI_MSG_CLICKED,OSD_REWIND,OSD_REWIND,0,0,NULL);
			OnMessage(msgSet);
		}
		break;

		case ACTION_OSD_SHOW_VALUE_PLUS:
		{
			// push a message through to this window to handle the remote control button
			CGUIMessage msgSet(GUI_MSG_CLICKED,OSD_SKIPFWD,OSD_SKIPFWD,0,0,NULL);
			OnMessage(msgSet);
		}
		break;

		case ACTION_OSD_SHOW_VALUE_MIN:
		{
			// push a message through to this window to handle the remote control button
			CGUIMessage msgSet(GUI_MSG_CLICKED,OSD_SKIPBWD,OSD_SKIPBWD,0,0,NULL);
			OnMessage(msgSet);
		}
		break;
	}

	CGUIWindow::OnAction(action);
}

bool CGUIWindowOSD::OnMessage(CGUIMessage& message)
{
	switch ( message.GetMessage() )
	{
		case GUI_MSG_WINDOW_DEINIT:	// fired when OSD is hidden
		{
			if (m_bSubMenuOn)						// is sub menu on?
			{
				SET_CONTROL_FOCUS(GetID(), m_iActiveMenuButtonID, 0);	// set focus to last menu button
				ToggleSubMenu(0, m_iActiveMenu);						// hide the currently active sub-menu
			}
			return true;
		}
		break;

		case GUI_MSG_WINDOW_INIT:	// fired when OSD is shown
		{
			SET_CONTROL_FOCUS(GetID(), OSD_PLAY, 0);	// set focus to play button by default when window is shown
			return true;
		}
		break;

		case GUI_MSG_CLICKED:
		{
			int iControl=message.GetSenderId();		// get the ID of the control sending us a message

			if (iControl >= OSD_VOLUMESLIDER)	// one of the settings (sub menu) controls is sending us a message
			{
				Handle_ControlSetting(iControl);
			}

			if (iControl == OSD_PLAY)
			{
				if (g_application.m_pPlayer)
				{
					if (g_application.GetPlaySpeed() != 1)	// we're in ffwd or rewind mode
					{
						g_application.SetPlaySpeed(1);		// drop back to single speed
						ToggleButton(OSD_REWIND, false);	// pop all the relevant
						ToggleButton(OSD_FFWD, false);		// buttons back to
						ToggleButton(OSD_PLAY, false);		// their up state
					}
					else
					{
						g_application.m_pPlayer->Pause();	// Pause/Un-Pause playback
						if (g_application.m_pPlayer->IsPaused())
							ToggleButton(OSD_PLAY, true);		// make sure play button is down (so it shows the pause symbol)
						else
							ToggleButton(OSD_PLAY, false);		// make sure play button is up (so it shows the play symbol)
					}
				}
			}

			if (iControl == OSD_STOP)
			{
				if (m_bSubMenuOn)	// sub menu currently active ?
				{
					SET_CONTROL_FOCUS(GetID(), m_iActiveMenuButtonID, 0);	// set focus to last menu button
					ToggleSubMenu(0, m_iActiveMenu);						// hide the currently active sub-menu
				}
				g_application.m_guiWindowFullScreen.m_bOSDVisible = false;	// toggle the OSD off so parent window can de-init
				g_application.m_pPlayer->closefile();						// close our media
				m_gWindowManager.PreviousWindow();							// go back to the previous window
			}

			if (iControl == OSD_REWIND)
			{
				g_application.m_guiWindowFullScreen.ChangetheSpeed(ACTION_REWIND);	// start rewinding or speed up rewind
				if (g_application.GetPlaySpeed() != 1)	// are we not playing back at normal speed
					ToggleButton(OSD_REWIND, true);		// make sure out button is in the down position
				else
					ToggleButton(OSD_REWIND, false);	// pop the button back to it's up state
			}

			if (iControl == OSD_FFWD)
			{
				g_application.m_guiWindowFullScreen.ChangetheSpeed(ACTION_FORWARD);	// start ffwd or speed up ffwd
				if (g_application.GetPlaySpeed() != 1)	// are we not playing back at normal speed
					ToggleButton(OSD_FFWD, true);		// make sure out button is in the down position
				else
					ToggleButton(OSD_FFWD, false);		// pop the button back to it's up state
			}

			if (iControl == OSD_SKIPBWD)
			{
				int iPercent=g_application.m_pPlayer->GetPercentage();	// Find out where we are at the moment
				if (iPercent>=10)
					g_application.m_pPlayer->SeekPercentage(iPercent-10);	// provided we're at 10% or greater, go back 10%
				else
					g_application.m_pPlayer->SeekPercentage(0);				// otherwise go back to the start
				ToggleButton(OSD_SKIPBWD, false);		// pop the button back to it's up state
			}

			if (iControl == OSD_SKIPFWD)
			{
				int iPercent=g_application.m_pPlayer->GetPercentage();	// Find out where we are at the moment
				if (iPercent+10<=100) g_application.m_pPlayer->SeekPercentage(iPercent+10);	// provided we're at 90% or less, go forward 10%
				ToggleButton(OSD_SKIPFWD, false);		// pop the button back to it's up state
			}

			if (iControl == OSD_MUTE)
			{
				ToggleSubMenu(iControl, OSD_SUBMENU_BG_VOL);			// hide or show the sub-menu
				if (m_bSubMenuOn)										// is sub menu on?
				{
					SET_CONTROL_VISIBLE(GetID(), OSD_VOLUMESLIDER);		// show the volume control
					SET_CONTROL_FOCUS(GetID(), OSD_VOLUMESLIDER, 0);	// set focus to it
				}
				else													// sub menu is off
				{
					SET_CONTROL_FOCUS(GetID(), OSD_MUTE, 0);			// set focus to the mute button
				}
			}

			/* not used
			if (iControl == OSD_SYNC)
			{
				ToggleSubMenu(iControl, OSD_SUBMENU_BG_SYNC);		// hide or show the sub-menu
			}
			*/

			if (iControl == OSD_SUBTITLES)
			{
				ToggleSubMenu(iControl, OSD_SUBMENU_BG_SUBTITLES);	// hide or show the sub-menu
				if (m_bSubMenuOn)
				{
					// set the controls values
					SetSliderValue(-10.0f, 10.0f, g_application.m_pPlayer->GetSubTitleDelay(), OSD_SUBTITLE_DELAY);

					// show the controls on this sub menu
					SET_CONTROL_VISIBLE(GetID(), OSD_SUBTITLE_DELAY);
					SET_CONTROL_VISIBLE(GetID(), OSD_SUBTITLE_DELAY_LABEL);
					SET_CONTROL_VISIBLE(GetID(), OSD_SUBTITLE_ONOFF);
					SET_CONTROL_VISIBLE(GetID(), OSD_SUBTITLE_LIST);

					SET_CONTROL_FOCUS(GetID(), OSD_SUBTITLE_DELAY, 0);	// set focus to the first control in our group
					PopulateSubTitles();	// populate the list control with subtitles for this video
				}
			}

			if (iControl == OSD_BOOKMARKS)
			{
				ToggleSubMenu(iControl, OSD_SUBMENU_BG_BOOKMARKS);	// hide or show the sub-menu
				if (m_bSubMenuOn)
				{
					// show the controls on this sub menu
					SET_CONTROL_VISIBLE(GetID(), OSD_CREATEBOOKMARK);
					SET_CONTROL_VISIBLE(GetID(), OSD_BOOKMARKS_LIST);
					SET_CONTROL_VISIBLE(GetID(), OSD_BOOKMARKS_LIST_LABEL);
					SET_CONTROL_VISIBLE(GetID(), OSD_CLEARBOOKMARKS);

					SET_CONTROL_FOCUS(GetID(), OSD_CREATEBOOKMARK, 0);	// set focus to the first control in our group
					PopulateBookmarks();	// populate the list control with bookmarks for this video
				}
			}

			if (iControl == OSD_VIDEO)
			{
				ToggleSubMenu(iControl, OSD_SUBMENU_BG_VIDEO);		// hide or show the sub-menu
				if (m_bSubMenuOn)						// is sub menu on?
				{
					// set the controls values
          float fBrightNess=(float)g_settings.m_iBrightness;
          float fContrast=(float)g_settings.m_iContrast;
          float fGamma=(float)g_settings.m_iGamma;
					SetSliderValue(0.0f, 100.0f, (float) g_application.m_pPlayer->GetPercentage(), OSD_VIDEOPOS);
          
          SetSliderValue(0.0f, 100.0f, (float) fBrightNess, OSD_BRIGHTNESS);
          SetSliderValue(0.0f, 100.0f, (float) fContrast, OSD_CONTRAST);
          SetSliderValue(0.0f, 100.0f, (float) fGamma, OSD_GAMMA);

					SetCheckmarkValue(g_stSettings.m_bNonInterleaved, OSD_NONINTERLEAVED);
					SetCheckmarkValue(g_stSettings.m_bNoCache, OSD_NOCACHE);
					SetCheckmarkValue(g_stSettings.m_bFrameRateConversions, OSD_ADJFRAMERATE);

					// show the controls on this sub menu
					SET_CONTROL_VISIBLE(GetID(), OSD_VIDEOPOS);
					SET_CONTROL_VISIBLE(GetID(), OSD_NONINTERLEAVED);
					SET_CONTROL_VISIBLE(GetID(), OSD_NOCACHE);
					SET_CONTROL_VISIBLE(GetID(), OSD_ADJFRAMERATE);
					SET_CONTROL_VISIBLE(GetID(), OSD_VIDEOPOS_LABEL);
					SET_CONTROL_VISIBLE(GetID(), OSD_BRIGHTNESS);
					SET_CONTROL_VISIBLE(GetID(), OSD_BRIGHTNESSLABEL);
					SET_CONTROL_VISIBLE(GetID(), OSD_CONTRAST);
					SET_CONTROL_VISIBLE(GetID(), OSD_CONTRASTLABEL);
					SET_CONTROL_VISIBLE(GetID(), OSD_GAMMA);
					SET_CONTROL_VISIBLE(GetID(), OSD_GAMMALABEL);
					SET_CONTROL_FOCUS(GetID(), OSD_VIDEOPOS, 0);	// set focus to the first control in our group
				}
			}

			if (iControl == OSD_AUDIO)
			{
				ToggleSubMenu( iControl, OSD_SUBMENU_BG_AUDIO);		// hide or show the sub-menu
				if (m_bSubMenuOn)						// is sub menu on?
				{
					// set the controls values
					SetSliderValue(-10.0f, 10.0f, g_application.m_pPlayer->GetAVDelay(), OSD_AVDELAY);
				
					// show the controls on this sub menu
					SET_CONTROL_VISIBLE(GetID(), OSD_AVDELAY);
					SET_CONTROL_VISIBLE(GetID(), OSD_AVDELAY_LABEL);
					SET_CONTROL_VISIBLE(GetID(), OSD_AUDIOSTREAM_LIST);

					SET_CONTROL_FOCUS(GetID(), OSD_AVDELAY, 0);	// set focus to the first control in our group
					PopulateAudioStreams();		// populate the list control with audio streams for this video
				}
			}

			return true;
		}
	}
	return CGUIWindow::OnMessage(message);
}

void CGUIWindowOSD::SetVideoProgress()
{
	if (g_application.m_pPlayer)
	{
		int iValue=g_application.m_pPlayer->GetPercentage();	// Find out where we are at the moment

		CGUIProgressControl* pControl = (CGUIProgressControl*)GetControl(OSD_VIDEOPROGRESS);
		if (pControl) pControl->SetPercentage(iValue);			// Update our progress bar accordingly ...
	}
}

void CGUIWindowOSD::Get_TimeInfo()
{
	if (!g_application.m_pPlayer) return;
	if (!g_application.m_pPlayer->HasVideo()) return;

	// get the current playing time position
	_int64 lPTS1=10*g_application.m_pPlayer->GetTime();			
	int hh = (int)(lPTS1 / 36000) % 100;
	int mm = (int)((lPTS1 / 600) % 60);
	int ss = (int)((lPTS1 /  10) % 60);

	// get the total play back time
	_int64 lPTS2=10*g_application.m_pPlayer->GetTotalTime();	
	int thh = (int)(lPTS2 / 36000) % 100;
	int tmm = (int)((lPTS2 / 600) % 60);
	int tss = (int)((lPTS2 /  10) % 60);
	
	// format it up for display
	char szTime[128];
	sprintf(szTime,"%02.2i:%02.2i:%02.2i/%02.2i:%02.2i:%02.2i",hh,mm,ss,thh,tmm,tss);	

	CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), OSD_TIMEINFO);
	msg.SetLabel(szTime); 
    OnMessage(msg);			// ask our label to update it's caption
}

void CGUIWindowOSD::ToggleButton(DWORD iButtonID, bool bSelected)
{
	CGUIControl* pControl = (CGUIControl*)GetControl(iButtonID);

	if (pControl)
	{
		if (bSelected)	// do we want the button to appear down?
		{
			CGUIMessage msg(GUI_MSG_SELECTED, GetID(), iButtonID);
			OnMessage(msg);
		}
		else			// or appear up?
		{
			CGUIMessage msg(GUI_MSG_DESELECTED, GetID(), iButtonID);
			OnMessage(msg);
		}
	}
}

void CGUIWindowOSD::ToggleSubMenu(DWORD iButtonID, DWORD iBackID)
{
	int iX, iY;

	CGUIImage* pImgNib = (CGUIImage*)GetControl(OSD_SUBMENU_NIB);	// pointer to the nib graphic
	CGUIImage* pImgBG = (CGUIImage*)GetControl(iBackID);			// pointer to the background graphic
	CGUIToggleButtonControl* pButton = (CGUIToggleButtonControl*)GetControl(iButtonID);	// pointer to the OSD menu button

	// check to see if we are currently showing a sub-menu and it's position is different
	if (m_bSubMenuOn && iBackID != m_iActiveMenu)
	{
		m_bSubMenuOn = false;	// toggle it ready for the new menu requested
	}

	// Get button position
	if (pButton)	
	{
		iX = (pButton->GetXPosition() + (pButton->GetWidth() / 2));	// center of button
		iY = pButton->GetYPosition();
	}
	else
	{
		iX = 0;
		iY = 0;
	}

	// Set nib position
	if (pImgNib && pImgBG)
	{		
		pImgNib->SetPosition(iX - (pImgNib->GetTextureWidth() / 2), iY - pImgNib->GetTextureHeight());

		if (!m_bSubMenuOn)	// sub menu not currently showing?
		{
			pImgNib->SetVisible(true);		// make it show
			pImgBG->SetVisible(true);		// make it show
		}
		else
		{
			pImgNib->SetVisible(false);		// hide it
			pImgBG->SetVisible(false);		// hide it
		}
	}

	m_bSubMenuOn = !m_bSubMenuOn;		// toggle sub menu visible status

	// Set all sub menu controls to hidden
	SET_CONTROL_HIDDEN(GetID(), OSD_VOLUMESLIDER);
	SET_CONTROL_HIDDEN(GetID(), OSD_VIDEOPOS);
	SET_CONTROL_HIDDEN(GetID(), OSD_VIDEOPOS_LABEL);
	SET_CONTROL_HIDDEN(GetID(), OSD_AUDIOSTREAM_LIST);
	SET_CONTROL_HIDDEN(GetID(), OSD_AVDELAY);
	SET_CONTROL_HIDDEN(GetID(), OSD_NONINTERLEAVED);
	SET_CONTROL_HIDDEN(GetID(), OSD_NOCACHE);
	SET_CONTROL_HIDDEN(GetID(), OSD_ADJFRAMERATE);
	SET_CONTROL_HIDDEN(GetID(), OSD_AVDELAY_LABEL);
  
	SET_CONTROL_HIDDEN(GetID(), OSD_BRIGHTNESS);
	SET_CONTROL_HIDDEN(GetID(), OSD_BRIGHTNESSLABEL);
  
	SET_CONTROL_HIDDEN(GetID(), OSD_GAMMA);
	SET_CONTROL_HIDDEN(GetID(), OSD_GAMMALABEL);
  
	SET_CONTROL_HIDDEN(GetID(), OSD_CONTRAST);
	SET_CONTROL_HIDDEN(GetID(), OSD_CONTRASTLABEL);

	SET_CONTROL_HIDDEN(GetID(), OSD_CREATEBOOKMARK);
	SET_CONTROL_HIDDEN(GetID(), OSD_BOOKMARKS_LIST);
	SET_CONTROL_HIDDEN(GetID(), OSD_BOOKMARKS_LIST_LABEL);
	SET_CONTROL_HIDDEN(GetID(), OSD_CLEARBOOKMARKS);
	SET_CONTROL_HIDDEN(GetID(), OSD_SUBTITLE_DELAY);
	SET_CONTROL_HIDDEN(GetID(), OSD_SUBTITLE_DELAY_LABEL);
	SET_CONTROL_HIDDEN(GetID(), OSD_SUBTITLE_ONOFF);
	SET_CONTROL_HIDDEN(GetID(), OSD_SUBTITLE_LIST);

	// Reset the other buttons back to up except the one that's active
	if (iButtonID != OSD_MUTE) ToggleButton(OSD_MUTE, false);
	//if (iButtonID != OSD_SYNC) ToggleButton(OSD_SYNC, false); - not used
	if (iButtonID != OSD_SUBTITLES) ToggleButton(OSD_SUBTITLES, false);
	if (iButtonID != OSD_BOOKMARKS) ToggleButton(OSD_BOOKMARKS, false);
	if (iButtonID != OSD_VIDEO) ToggleButton(OSD_VIDEO, false);
	if (iButtonID != OSD_AUDIO) ToggleButton(OSD_AUDIO, false);

	m_iActiveMenu = iBackID;
	m_iActiveMenuButtonID = iButtonID;
}

void CGUIWindowOSD::SetSliderValue(float fMin, float fMax, float fValue, DWORD iControlID)
{
	CGUISliderControl* pControl=(CGUISliderControl*)GetControl(iControlID);
	
	if (pControl)
	{
		switch(pControl->GetType())
		{
			case SPIN_CONTROL_TYPE_FLOAT:
				pControl->SetFloatRange(fMin, fMax);
				pControl->SetFloatValue(fValue);
				break;
			
			case SPIN_CONTROL_TYPE_INT:
				pControl->SetRange((int) fMin, (int) fMax);
				pControl->SetIntValue((int) fValue);
				break;

			default:
				pControl->SetPercentage((int) fValue);
				break;
		}
	}
}

void CGUIWindowOSD::SetCheckmarkValue(BOOL bValue, DWORD iControlID)
{
	if (bValue)
	{
		CGUIMessage msg(GUI_MSG_SELECTED,GetID(),iControlID,0,0,NULL);
		OnMessage(msg);
	}
	else
	{
		CGUIMessage msg(GUI_MSG_DESELECTED,GetID(),iControlID,0,0,NULL);
		OnMessage(msg);
	}
}

void CGUIWindowOSD::Handle_ControlSetting(DWORD iControlID)
{
	const CStdString& strMovie=g_application.CurrentFile();
	CVideoDatabase dbs;
	VECBOOKMARKS bookmarks;

	switch (iControlID)
	{
		case OSD_VOLUMESLIDER:
		{
			CGUISliderControl* pControl=(CGUISliderControl*)GetControl(iControlID);
			if (pControl)
			{
				// no volume control yet so no code here at the moment
			}
		}
		break;

		case OSD_VIDEOPOS:
		{
			CGUISliderControl* pControl=(CGUISliderControl*)GetControl(iControlID);
			if (pControl)
			{
				// Set mplayer's seek position to the percentage requested by the user
				g_application.m_pPlayer->SeekPercentage(pControl->GetPercentage());
			}
		}
    break;
    case OSD_BRIGHTNESS:
		{
			CGUISliderControl* pControl=(CGUISliderControl*)GetControl(iControlID);
			if (pControl)
			{
				// Set mplayer's seek position to the percentage requested by the user
				g_settings.m_iBrightness=pControl->GetPercentage();
        CUtil::SetBrightnessContrastGammaPercent(g_settings.m_iBrightness, g_settings.m_iContrast, g_settings.m_iGamma, true);
			}
		}
		break;
    case OSD_CONTRAST:
		{
			CGUISliderControl* pControl=(CGUISliderControl*)GetControl(iControlID);
			if (pControl)
			{
				// Set mplayer's seek position to the percentage requested by the user
				g_settings.m_iContrast=pControl->GetPercentage();
        CUtil::SetBrightnessContrastGammaPercent(g_settings.m_iBrightness, g_settings.m_iContrast, g_settings.m_iGamma, true);
			}
		}
		break;
    case OSD_GAMMA:
		{
			CGUISliderControl* pControl=(CGUISliderControl*)GetControl(iControlID);
			if (pControl)
			{
				// Set mplayer's seek position to the percentage requested by the user
				g_settings.m_iGamma=pControl->GetPercentage();
        CUtil::SetBrightnessContrastGammaPercent(g_settings.m_iBrightness, g_settings.m_iContrast, g_settings.m_iGamma, true);
			}
		}
		break;

		case OSD_AUDIOSTREAM_LIST:
		{
			CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),OSD_AUDIOSTREAM_LIST,0,0,NULL);
			OnMessage(msg);
			g_stSettings.m_iAudioStream = msg.GetParam1();				// Set the audio stream to the one selected
			mplayer_getAudioStream(g_stSettings.m_iAudioStream);		// Tell mplayer ...
			m_bSubMenuOn = false;										// hide the sub menu
			g_application.m_guiWindowFullScreen.m_bOSDVisible = false;	// toggle the OSD off so parent window can de-init
			g_application.Restart(true);								// restart to make new audio track active
		}
		break;

		case OSD_AVDELAY:
		{
			CGUISliderControl* pControl=(CGUISliderControl*)GetControl(iControlID);
			if (pControl)
			{
				// Set the AV Delay
				g_application.m_pPlayer->SetAVDelay(pControl->GetFloatValue());
			}
		}
		break;

		case OSD_NONINTERLEAVED:
		{
			g_stSettings.m_bNonInterleaved=!g_stSettings.m_bNonInterleaved;
			m_bSubMenuOn = false;										// hide the sub menu
			g_application.m_guiWindowFullScreen.m_bOSDVisible = false;	// toggle the OSD off so parent window can de-init
			g_application.Restart(true);								// restart to make the new setting active
		}
		break;

		case OSD_NOCACHE:
		{
			g_stSettings.m_bNoCache=!g_stSettings.m_bNoCache;
			m_bSubMenuOn = false;										// hide the sub menu
			g_application.m_guiWindowFullScreen.m_bOSDVisible = false;	// toggle the OSD off so parent window can de-init
			g_application.Restart(true);								// restart to make the new setting active
		}
		break;

		case OSD_ADJFRAMERATE:
		{
			g_stSettings.m_bFrameRateConversions=!g_stSettings.m_bFrameRateConversions;
			m_bSubMenuOn = false;										// hide the sub menu
			g_application.m_guiWindowFullScreen.m_bOSDVisible = false;	// toggle the OSD off so parent window can de-init
		    g_application.Restart(true);								// restart to make the new setting active
		}
		break;

		case OSD_CREATEBOOKMARK:
		{
			_int64 lPTS1=g_application.m_pPlayer->GetTime();			// get the current playing time position

			dbs.Open();													// open the bookmark d/b
			dbs.AddBookMarkToMovie(strMovie, (float)lPTS1);				// add the current timestamp
			dbs.Close();												// close the d/b
			PopulateBookmarks();										// refresh our list control
		}
		break;

		case OSD_BOOKMARKS_LIST:
		{
			CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),OSD_BOOKMARKS_LIST,0,0,NULL);
			OnMessage(msg);
			m_iCurrentBookmark = msg.GetParam1();					// index of bookmark user selected

			dbs.Open();												// open the bookmark d/b
			dbs.GetBookMarksForMovie(strMovie, bookmarks);			// load the stored bookmarks
			dbs.Close();											// close the d/b
			if (bookmarks.size()<=0) return;						// no bookmarks? leave if so ...

			g_application.m_pPlayer->SeekTime((long) bookmarks[m_iCurrentBookmark]);	// set mplayers play position
		}
		break;

		case OSD_CLEARBOOKMARKS:
		{
			dbs.Open();												// open the bookmark d/b
			dbs.ClearBookMarksOfMovie(strMovie);					// empty the bookmarks table for this movie
			dbs.Close();											// close the d/b
			m_iCurrentBookmark=0;									// reset current bookmark
			PopulateBookmarks();									// refresh our list control
		}
		break;

		case OSD_SUBTITLE_DELAY:
		{
			CGUISliderControl* pControl=(CGUISliderControl*)GetControl(iControlID);
			if (pControl)
			{
				// Set the subtitle delay
				g_application.m_pPlayer->SetSubTittleDelay(pControl->GetFloatValue());
			}
		}
		break;

		case OSD_SUBTITLE_ONOFF:
		{
			if (mplayer_SubtitleVisible())
			{
				mplayer_showSubtitle(false);		// Turn off subtitles
			}
			else
			{
				mplayer_showSubtitle(true);			// Turn on subtitles
			}
		}
		break;

		case OSD_SUBTITLE_LIST:
		{
			CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),OSD_SUBTITLE_LIST,0,0,NULL);
			OnMessage(msg);								// retrieve the selected list item
			mplayer_setSubtitle(msg.GetParam1());		// set the current subtitle
		}
		break;
	}
}

void CGUIWindowOSD::PopulateBookmarks()
{
	VECBOOKMARKS bookmarks;
	const CStdString& strMovie=g_application.CurrentFile();
	CVideoDatabase dbs;

	// tell the list control not to show the page x/y spin control
	CGUIListControl* pControl=(CGUIListControl*)GetControl(OSD_BOOKMARKS_LIST);
	if (pControl) pControl->SetPageControlVisible(false);

	// open the d/b and retrieve the bookmarks for the current movie
	dbs.Open();
	dbs.GetBookMarksForMovie(strMovie, bookmarks);
	dbs.Close();

	// empty the list ready for population
	CGUIMessage msg(GUI_MSG_LABEL_RESET,GetID(),OSD_BOOKMARKS_LIST,0,0,NULL);
	OnMessage(msg);

	// cycle through each stored bookmark and add it to our list control
	for (int i=0; i < (int)(bookmarks.size()); ++i)
	{
		CStdString strItem;
		_int64 lPTS1 = (_int64)(10 * bookmarks[i]);
		int hh = (int)(lPTS1 / 36000) % 100;
		int mm = (int)((lPTS1 / 600) % 60);
		int	ss = (int)((lPTS1 /  10) % 60);
		strItem.Format("%2i.   %02.2i:%02.2i:%02.2i",i+1,hh,mm,ss);

		// create a list item object to add to the list
		CGUIListItem* pItem = new CGUIListItem();
		pItem->SetLabel(strItem);
		
		// add it ...
		CGUIMessage msg2(GUI_MSG_LABEL_ADD,GetID(),OSD_BOOKMARKS_LIST,0,0,(void*)pItem);
		OnMessage(msg2);    
	}

	// set the currently active bookmark as the selected item in the list control
	CGUIMessage msgSet(GUI_MSG_ITEM_SELECT,GetID(),OSD_BOOKMARKS_LIST,m_iCurrentBookmark,0,NULL);
	OnMessage(msgSet);
}

void CGUIWindowOSD::PopulateAudioStreams()
{
	// get the number of audio strams for the current movie
	int iValue=g_application.m_pPlayer->GetAudioStreamCount();

	// tell the list control not to show the page x/y spin control
	CGUIListControl* pControl=(CGUIListControl*)GetControl(OSD_AUDIOSTREAM_LIST);
	if (pControl) pControl->SetPageControlVisible(false);

	// empty the list ready for population
	CGUIMessage msg(GUI_MSG_LABEL_RESET,GetID(),OSD_AUDIOSTREAM_LIST,0,0,NULL);
	OnMessage(msg);

	CStdString strLabel = g_localizeStrings.Get(460).c_str();					// "Audio Stream"
	CStdString strActiveLabel = (WCHAR*)g_localizeStrings.Get(461).c_str();		// "[active]"

	// cycle through each audio stream and add it to our list control
	for (int i=0; i < iValue; ++i)
	{
		CStdString strItem;
		if (g_stSettings.m_iAudioStream == i)
		{
			// formats to 'Audio Stream X [active]'
			strItem.Format(strLabel + " %2i " + strActiveLabel,i+1);	// this audio stream is active, show as such
		}
		else
		{
			// formats to 'Audio Stream X'
			strItem.Format(strLabel + " %2i",i+1);
		}

		// create a list item object to add to the list
		CGUIListItem* pItem = new CGUIListItem();
		pItem->SetLabel(strItem);
		
		// add it ...
		CGUIMessage msg2(GUI_MSG_LABEL_ADD,GetID(),OSD_AUDIOSTREAM_LIST,0,0,(void*)pItem);
		OnMessage(msg2);    
	}

	// set the current active audio stream as the selected item in the list control
	CGUIMessage msgSet(GUI_MSG_ITEM_SELECT,GetID(),OSD_AUDIOSTREAM_LIST,g_stSettings.m_iAudioStream,0,NULL);
	OnMessage(msgSet);
}

void CGUIWindowOSD::PopulateSubTitles()
{
	// get the number of subtitles in the current movie
	int iValue=mplayer_getSubtitleCount();

	// tell the list control not to show the page x/y spin control
	CGUIListControl* pControl=(CGUIListControl*)GetControl(OSD_SUBTITLE_LIST);
	if (pControl) pControl->SetPageControlVisible(false);

	// empty the list ready for population
	CGUIMessage msg(GUI_MSG_LABEL_RESET,GetID(),OSD_SUBTITLE_LIST,0,0,NULL);
	OnMessage(msg);

	CStdString strLabel = g_localizeStrings.Get(462).c_str();					// "Subtitle"
	CStdString strActiveLabel = (WCHAR*)g_localizeStrings.Get(461).c_str();		// "[active]"

	// cycle through each subtitle and add it to our list control
	for (int i=0; i < iValue; ++i)
	{
		CStdString strItem;
		if (mplayer_getSubtitle() == i)		// this subtitle is active, show as such
		{
			// formats to 'Subtitle X [active]'
			strItem.Format(strLabel + " %2i " + strActiveLabel,i+1);	// this audio stream is active, show as such
		}
		else
		{
			// formats to 'Subtitle X'
			strItem.Format(strLabel + " %2i",i+1);
		}

		// create a list item object to add to the list
		CGUIListItem* pItem = new CGUIListItem();
		pItem->SetLabel(strItem);
		
		// add it ...
		CGUIMessage msg2(GUI_MSG_LABEL_ADD,GetID(),OSD_SUBTITLE_LIST,0,0,(void*)pItem);
		OnMessage(msg2);    
	}

	// set the current active subtitle as the selected item in the list control
	CGUIMessage msgSet(GUI_MSG_ITEM_SELECT,GetID(),OSD_SUBTITLE_LIST,g_stSettings.m_iAudioStream,0,NULL);
	OnMessage(msgSet);
}