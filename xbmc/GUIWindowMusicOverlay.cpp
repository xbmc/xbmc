
#include "stdafx.h"
#include "GUIWindowMusicOverlay.h"
#include "util.h"
#include "application.h"
#include "utils/GUIInfoManager.h"

#define CONTROL_LOGO_PIC_BACK    0
#define CONTROL_LOGO_PIC    1
#define CONTROL_PLAYTIME		2
#define CONTROL_PLAY_LOGO   3
#define CONTROL_PAUSE_LOGO  4
#define CONTROL_INFO			  5
#define CONTROL_BIG_PLAYTIME 6
#define CONTROL_FF_LOGO  7
#define CONTROL_RW_LOGO  8

#define CONTROL_TITLE		51
#define CONTROL_ALBUM		52
#define CONTROL_ARTIST	53
#define CONTROL_YEAR		54

#define STEPS 25

CGUIWindowMusicOverlay::CGUIWindowMusicOverlay()
:CGUIWindow(0)
{
	m_iFrames=0;
	m_dwTimeout=0;
	m_iTopPosition=0;
	m_bShowInfoAlways=false;
	m_bRelativeCoords=true;	// so that we can move things around easily :)
}

CGUIWindowMusicOverlay::~CGUIWindowMusicOverlay()
{
}


void CGUIWindowMusicOverlay::OnAction(const CAction &action)
{
	if (m_gWindowManager.GetActiveWindow() == WINDOW_VISUALISATION)
	{
		switch (action.wID)
		{
			case ACTION_SHOW_INFO:
				//reset timeout
				m_dwTimeout = 0;

				// if we're up permanently, and action is invoked again,
				// remove the info and reset
				if (m_bShowInfoAlways)
				{
					m_bShowInfoAlways = false;
					m_bShowInfo       = false;
					m_dwTimeout       = 0;
					m_iFrames         = STEPS;
					m_iFrameIncrement = -1;
				}

				//figure out which way we are moving
				else if (m_iFrameIncrement == 0)
				{
					// we're not moving, so figure out if we should start moving up or down
					// scroll the info up
					if (m_iFrames <= 0)
					{
						m_bShowInfo=true;
						g_stSettings.m_bMyMusicSongInfoInVis = true;
						g_stSettings.m_bMyMusicSongThumbInVis = true;
						m_iFrameIncrement = 1;
					}
					// info is already on screen in its resting spot. 
					// if INFO is hit again, set bool to make it stay there.
					else if (m_iFrames == STEPS && g_guiSettings.GetInt("MyMusic.OSDTimeout") > 0)
					{
						m_bShowInfoAlways=true;
					}
					// else scroll the info down
					else
					{
						m_bShowInfo=false;
						m_iFrameIncrement = -1;
					}
				}
				else
				{
					//we're moving... reverse
					m_iFrameIncrement*=-1;
				}
				break;

			case ACTION_SHOW_GUI:
				// reset the bool if switching back to the GUI
				m_bShowInfoAlways=false;
				break;
		}
	}
	CGUIWindow::OnAction(action);
}

bool CGUIWindowMusicOverlay::OnMessage(CGUIMessage& message)
{	// check that the mouse wasn't clicked on the thumb...
	if (message.GetMessage() == GUI_MSG_CLICKED)
	{
		if (message.GetControlId() == GetID() && message.GetSenderId() == 0)
		{
			if (message.GetParam1() == ACTION_SELECT_ITEM)
			{	// switch to fullscreen visualisation mode...
				CGUIMessage msg(GUI_MSG_FULLSCREEN, 0, GetID());
				g_graphicsContext.SendMessage(msg);
			}
		}
	}
	if (message.GetMessage() == GUI_MSG_WINDOW_INIT)
	{	// on init, set the correct show vis setting
		m_bShowInfo = g_stSettings.m_bMyMusicSongInfoInVis;
	}
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowMusicOverlay::OnMouse()
{
	CGUIControl *pControl = (CGUIControl *)GetControl(CONTROL_LOGO_PIC);
	if (pControl && pControl->HitTest(g_Mouse.iPosX, g_Mouse.iPosY))
	{
		// send highlight message
		g_Mouse.SetState(MOUSE_STATE_FOCUS);
		if (g_Mouse.bClick[MOUSE_LEFT_BUTTON])
		{	// send mouse message
			CGUIMessage message(GUI_MSG_FULLSCREEN, CONTROL_LOGO_PIC, GetID());
			g_graphicsContext.SendMessage(message);
			// reset the mouse button
			g_Mouse.bClick[MOUSE_LEFT_BUTTON] = false;
		}
		if (g_Mouse.bClick[MOUSE_RIGHT_BUTTON])
		{	// toggle the playlist window
			if (m_gWindowManager.GetActiveWindow() == WINDOW_MUSIC_PLAYLIST)
				m_gWindowManager.PreviousWindow();
			else
				m_gWindowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);
			// reset it so that we don't call other actions
			g_Mouse.bClick[MOUSE_RIGHT_BUTTON] = false;
		}
	}
	else
	{
		CGUIWindow::OnMouse();
	}
}

void CGUIWindowMusicOverlay::ShowControl(int iControl)
{
  CGUIMessage msg(GUI_MSG_VISIBLE, GetID(), iControl); 
	OnMessage(msg); 
}

void CGUIWindowMusicOverlay::HideControl(int iControl)
{
  CGUIMessage msg(GUI_MSG_HIDDEN, GetID(), iControl); 
	OnMessage(msg); 
}

void CGUIWindowMusicOverlay::UpdatePosition(int iStep, int iSteps)
{
  int iScreenHeight=10+g_graphicsContext.GetHeight();
  float fDiff=(float)iScreenHeight-(float)m_iTopPosition;
  float fPos = fDiff / ((float)iSteps) * ((float)iSteps-iStep);
	SetPosition(0,(int)fPos);
}

void CGUIWindowMusicOverlay::GetTopControlPosition()
{
	m_iTopPosition = g_graphicsContext.GetHeight();
	for (int i=0; i<(int)m_vecControls.size(); i++)
	{
		CGUIControl* pControl=m_vecControls[i];
		if (pControl)
		{
			if (pControl->GetYPosition() < m_iTopPosition)
				m_iTopPosition = pControl->GetYPosition();
		}
	}
}

void CGUIWindowMusicOverlay::Render()
{
	if (!g_application.m_pPlayer) return;
	if ( g_application.m_pPlayer->HasVideo()) return;
  if (m_iTopPosition==0)
  {
		GetTopControlPosition();
  }
  //int iSteps=25;
  if ( (m_gWindowManager.GetActiveWindow() != WINDOW_VISUALISATION) ||
	  (m_bShowInfoAlways) )
  {
    UpdatePosition(50,50);
    m_iFrames=0;
    m_iFrameIncrement = 1;
    m_dwTimeout = 0;
  }
  else
  {
		UpdatePosition(m_iFrames,STEPS);
		m_iFrames+=m_iFrameIncrement;

		if (!m_bShowInfo && m_iFrames <= 1)
		{
			m_dwTimeout = 0;
			m_iFrames = 0;
			m_iFrameIncrement = 0;
			g_stSettings.m_bMyMusicSongInfoInVis = false;
			g_stSettings.m_bMyMusicSongThumbInVis = false;
    }
		else if (!m_bShowInfo && m_iFrames >= STEPS)
		{
			m_dwTimeout = 0;
			m_iFrames = STEPS;
			m_iFrameIncrement = -1;
		}
		else if (m_iFrames <= 0)
		{
			//we're just sitting at the bottom
			m_dwTimeout =  0;
			m_iFrames = 0;
			m_iFrameIncrement = 0;
			g_stSettings.m_bMyMusicSongInfoInVis = false;
			g_stSettings.m_bMyMusicSongThumbInVis = false;
    }
		else if (m_iFrames >= STEPS)
		{
			//if we just got to the top, start the timer but keep us sitting there until timeout expires
			if (m_dwTimeout <= 0)
			{
				//set timeout to g_guiSettings.GetInt("MyMusic.OSDTimeout") seconds in the future
				//(timeGetTime is in milliseconds, so we multiply by 1000
				if (g_guiSettings.GetInt("MyMusic.OSDTimeout") > 0)
					m_dwTimeout = (g_guiSettings.GetInt("MyMusic.OSDTimeout") * 1000) + timeGetTime();
				else
					m_dwTimeout = 0;
				m_iFrames = STEPS;
				m_iFrameIncrement = 0;
			}
			//if a timeout was set and it has expired, start moving down
			else if (g_guiSettings.GetInt("MyMusic.OSDTimeout") > 0 && timeGetTime() > m_dwTimeout)
			{
				m_dwTimeout = 0;
				m_iFrames = STEPS;
				m_iFrameIncrement = -1;
			}
	  }
  }
	
  if ((m_gWindowManager.GetActiveWindow() != WINDOW_VISUALISATION) || 
      (g_stSettings.m_bMyMusicSongInfoInVis)
  ) {
    ShowControl( CONTROL_PLAYTIME);
    ShowControl( CONTROL_INFO);
    ShowControl( CONTROL_BIG_PLAYTIME);

		// TODO: Move to our application render funtion
    __int64 lPTS=g_application.m_pPlayer->GetTime() - (g_infoManager.GetCurrentSongStart()*(__int64)1000)/75;
    int iSpeed=g_application.GetPlaySpeed();
    if (lPTS<5000 && iSpeed < 1)
    {
      iSpeed=1;
      g_application.SetPlaySpeed(iSpeed);
      g_application.m_pPlayer->SeekTime(0);
    }

		SET_CONTROL_LABEL(CONTROL_BIG_PLAYTIME, g_infoManager.GetLabel("musicplayer.time"));
		SET_CONTROL_LABEL(CONTROL_PLAYTIME, g_infoManager.GetLabel("musicplayer.timespeed"));
    
    if (iSpeed !=1)
    {
      m_iFrames = STEPS ;
    }

	  HideControl( CONTROL_PLAY_LOGO); 
	  HideControl( CONTROL_PAUSE_LOGO);
	  HideControl( CONTROL_FF_LOGO); 
	  HideControl( CONTROL_RW_LOGO);  
	  if (g_application.m_pPlayer->IsPaused() )
	  {
		  ShowControl(CONTROL_PAUSE_LOGO);
	  }
	  else
	  {
      int iSpeed=g_application.GetPlaySpeed();
      if (iSpeed==1)
      {
		    ShowControl( CONTROL_PLAY_LOGO); 
      }
      else if (iSpeed>1)
      {
		    ShowControl(CONTROL_FF_LOGO); 
      }
		  else
      {
        ShowControl(CONTROL_RW_LOGO);  
      }
	  }
  }
  else {
	  HideControl( CONTROL_PLAY_LOGO); 
	  HideControl( CONTROL_PAUSE_LOGO);
	  HideControl( CONTROL_FF_LOGO); 
	  HideControl( CONTROL_RW_LOGO);  
    HideControl( CONTROL_PLAYTIME);
    HideControl( CONTROL_INFO);
    HideControl( CONTROL_BIG_PLAYTIME);
  }

	// Set our label controls
	SET_CONTROL_LABEL( CONTROL_TITLE, g_infoManager.GetLabel("musicplayer.title"));
	SET_CONTROL_LABEL( CONTROL_ALBUM, g_infoManager.GetLabel("musicplayer.album"));
	SET_CONTROL_LABEL( CONTROL_ARTIST, g_infoManager.GetLabel("musicplayer.artist"));
	SET_CONTROL_LABEL( CONTROL_YEAR, g_infoManager.GetLabel("musicplayer.year"));

	// Set our album cover, if necessary
	CGUIImage *pImage = (CGUIImage *)GetControl(CONTROL_LOGO_PIC);
	if (pImage)
	{
		CStdString strNewImage = g_infoManager.GetImage("musicplayer.cover");
		if (pImage->GetFileName() != strNewImage)
			pImage->SetFileName(strNewImage);
	}

	// and now we render everything
	CGUIWindow::Render();
}

/// \brief Updates the music information on screen from the \e tag info.
/// \param tag Tag data to set
void CGUIWindowMusicOverlay::UpdateInfo(const CMusicInfoTag &tag)
{
	// Display available tag information in fade label control and
	// individual label controls

	// reset the fadelabel control
	{
		CGUIMessage msg1(GUI_MSG_LABEL_RESET, GetID(), CONTROL_INFO); 
		OnMessage(msg1);
	}

	if (tag.GetTitle().size())
	{	// Title
		CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
		msg1.SetLabel(tag.GetTitle());
		OnMessage(msg1);
	}

	if (tag.GetArtist().size())
	{	//	Artist
		CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
		msg1.SetLabel(tag.GetArtist());
		OnMessage(msg1);
	}

	if (tag.GetAlbum().size())
	{	//	Album
		CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
		msg1.SetLabel(tag.GetAlbum());
		OnMessage(msg1);
	}

	int iTrack=tag.GetTrackNumber();
	if (iTrack >=1)
	{
		//	Tracknumber
		CStdString strText=g_localizeStrings.Get(435);	//	"Track"
		if (strText.GetAt(strText.size()-1) != ' ')
			strText+=" ";
		CStdString strTrack;
		strTrack.Format(strText+"%i", iTrack);
		
		CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
		msg1.SetLabel(strTrack);
		OnMessage(msg1);
	}

	if (tag.GetYear().size())
	{
		//	Year
		CStdString strText=g_localizeStrings.Get(436);	//	"Year:"
		if (strText.GetAt(strText.size()-1) != ' ')
			strText+=" ";
		CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
		msg1.SetLabel(strText + tag.GetYear());
		OnMessage(msg1);
	}

	if (tag.GetDuration() > 0)
	{
		//	Duration
		CStdString strDuration, strTime;

		CStdString strText=g_localizeStrings.Get(437);
		if (strText.GetAt(strText.size()-1) != ' ')
			strText+=" ";

		CUtil::SecondsToHMSString(tag.GetDuration(), strTime);

		strDuration=strText+strTime;
		
		CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
		msg1.SetLabel( strDuration );
		OnMessage(msg1);
	}
}

/// \brief Tries to set the music tag information for \e item to window.
/// \param item Audiofile to set.
void CGUIWindowMusicOverlay::Update()
{
	// Get our tag info from the infoManager
	UpdateInfo(g_infoManager.GetCurrentSongTag());

	if (m_bShowInfo && m_gWindowManager.GetActiveWindow()==WINDOW_VISUALISATION)
	{
		//reset timeout
		m_dwTimeout = 0;

		//if we're not at the top, start moving up
		if (m_iFrames < STEPS)
		{
			m_iFrameIncrement = 1;
		}
	}
}

void CGUIWindowMusicOverlay::FreeResources()
{
	CGUIWindow::FreeResources();
	m_iTopPosition=0;
}
