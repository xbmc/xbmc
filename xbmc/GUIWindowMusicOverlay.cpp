#include "GUIWindowMusicOverlay.h"
#include "settings.h"
#include "guiWindowManager.h"
#include "localizestrings.h"
#include "util.h"
#include "stdstring.h"
#include "application.h"
#include "MusicInfoTag.h"
#include "picture.h"
#include "musicdatabase.h"
#include "filesystem/CDDADirectory.h"
#include "url.h"
#include "lib/libID3/tag.h"
#include "lib/libID3/misc_support.h"
#include "musicInfoTagLoaderFactory.h"

using namespace MUSIC_INFO;

#define CONTROL_LOGO_PIC    1
#define CONTROL_PLAYTIME		2
#define CONTROL_PLAY_LOGO   3
#define CONTROL_PAUSE_LOGO  4
#define CONTROL_INFO			  5
#define CONTROL_BIG_PLAYTIME 6
#define CONTROL_FF_LOGO  7
#define CONTROL_RW_LOGO  8

CGUIWindowMusicOverlay::CGUIWindowMusicOverlay()
:CGUIWindow(0)
{
  m_iFrames=0;
  m_iPosOrgIcon=0;
	m_pTexture=NULL;
}

CGUIWindowMusicOverlay::~CGUIWindowMusicOverlay()
{
}


void CGUIWindowMusicOverlay::OnAction(const CAction &action)
{
  CGUIWindow::OnAction(action);
}

bool CGUIWindowMusicOverlay::OnMessage(CGUIMessage& message)
{
  return CGUIWindow::OnMessage(message);
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

void CGUIWindowMusicOverlay::SetPosition(int iControl, int iStep, int iSteps,int iOrgPos)
{
  int iScreenHeight=10+g_graphicsContext.GetHeight();
  float fDiff=(float)iScreenHeight-(float)iOrgPos;
  float fPos = fDiff / ((float)iSteps);
  fPos *= -((float)iStep);
  fPos += (float)iScreenHeight;
  CGUIControl* pControl=(CGUIControl*)GetControl(iControl);
  if (pControl) pControl->SetPosition(pControl->GetXPosition(), (int)fPos);
}
int CGUIWindowMusicOverlay::GetControlYPosition(int iControl)
{
   CGUIControl* pControl=(CGUIControl*)GetControl(iControl);
   if (!pControl) return 0;
   return pControl->GetYPosition();
}

void CGUIWindowMusicOverlay::Render()
{
	if (!g_application.m_pPlayer) return;
	if ( g_application.m_pPlayer->HasVideo()) return;
  if (m_iPosOrgIcon==0)
  {
    m_iPosOrgRectangle=GetControlYPosition(0);
    m_iPosOrgIcon=GetControlYPosition(CONTROL_LOGO_PIC);
    m_iPosOrgPlay=GetControlYPosition(CONTROL_PLAY_LOGO);
    m_iPosOrgPause=GetControlYPosition(CONTROL_PAUSE_LOGO);
    m_iPosOrgInfo=GetControlYPosition(CONTROL_INFO);
    m_iPosOrgPlayTime=GetControlYPosition(CONTROL_PLAYTIME);
    m_iPosOrgBigPlayTime=GetControlYPosition(CONTROL_BIG_PLAYTIME);
  }
  int iSteps=25;
  if ( m_gWindowManager.GetActiveWindow() != WINDOW_VISUALISATION)
  {
    SetPosition(0, 50,50,m_iPosOrgRectangle);
    SetPosition(CONTROL_LOGO_PIC, 50,50,m_iPosOrgIcon);
    SetPosition(CONTROL_PLAY_LOGO, 50,50,m_iPosOrgPlay);
    SetPosition(CONTROL_PAUSE_LOGO, 50,50,m_iPosOrgPause);
    SetPosition(CONTROL_FF_LOGO, 50,50,m_iPosOrgPause);
    SetPosition(CONTROL_RW_LOGO, 50,50,m_iPosOrgPause);
    SetPosition(CONTROL_INFO, 50,50,m_iPosOrgInfo);
    SetPosition(CONTROL_PLAYTIME, 50,50,m_iPosOrgPlayTime);
    SetPosition(CONTROL_BIG_PLAYTIME, 50,50,m_iPosOrgBigPlayTime);
    m_iFrames=0;
  }
  else
  {
    if (m_iFrames < iSteps)
    {
      // scroll up
      SetPosition(0, m_iFrames,iSteps,m_iPosOrgRectangle);
      SetPosition(CONTROL_LOGO_PIC, m_iFrames,iSteps,m_iPosOrgIcon);
      SetPosition(CONTROL_PLAY_LOGO, m_iFrames,iSteps,m_iPosOrgPlay);
      SetPosition(CONTROL_PAUSE_LOGO, m_iFrames,iSteps,m_iPosOrgPause);
      SetPosition(CONTROL_FF_LOGO, m_iFrames,iSteps,m_iPosOrgPause);
      SetPosition(CONTROL_RW_LOGO, m_iFrames,iSteps,m_iPosOrgPause);
      SetPosition(CONTROL_INFO, m_iFrames,iSteps,m_iPosOrgInfo);
      SetPosition(CONTROL_PLAYTIME, m_iFrames,iSteps,m_iPosOrgPlayTime);
      SetPosition(CONTROL_BIG_PLAYTIME, m_iFrames,iSteps,m_iPosOrgBigPlayTime);
      m_iFrames++;
    }
    else if (m_iFrames >=iSteps && m_iFrames<=5*iSteps+iSteps)
    {
      //show
      SetPosition(0, iSteps,iSteps,m_iPosOrgRectangle);
      SetPosition(CONTROL_LOGO_PIC, iSteps,iSteps,m_iPosOrgIcon);
      SetPosition(CONTROL_PLAY_LOGO, iSteps,iSteps,m_iPosOrgPlay);
      SetPosition(CONTROL_PAUSE_LOGO, iSteps,iSteps,m_iPosOrgPause);
      SetPosition(CONTROL_FF_LOGO, iSteps,iSteps,m_iPosOrgPause);
      SetPosition(CONTROL_RW_LOGO, iSteps,iSteps,m_iPosOrgPause);
      SetPosition(CONTROL_INFO, iSteps,iSteps,m_iPosOrgInfo);
      SetPosition(CONTROL_PLAYTIME, iSteps,iSteps,m_iPosOrgPlayTime);
      SetPosition(CONTROL_BIG_PLAYTIME, iSteps,iSteps,m_iPosOrgBigPlayTime);
      m_iFrames++;
    }
    else if (m_iFrames >=5*iSteps+iSteps )
    {
      if (m_iFrames > 5*iSteps+2*iSteps) 
      {
        m_iFrames=5*iSteps+2*iSteps;
      }
      //scroll down
      SetPosition(0,                    5*iSteps+2*iSteps-m_iFrames,iSteps,m_iPosOrgRectangle);
      SetPosition(CONTROL_LOGO_PIC,     5*iSteps+2*iSteps-m_iFrames,iSteps,m_iPosOrgIcon);
      SetPosition(CONTROL_PLAY_LOGO,    5*iSteps+2*iSteps-m_iFrames,iSteps,m_iPosOrgPlay);
      SetPosition(CONTROL_PAUSE_LOGO,   5*iSteps+2*iSteps-m_iFrames,iSteps,m_iPosOrgPause);
      SetPosition(CONTROL_FF_LOGO,   5*iSteps+2*iSteps-m_iFrames,iSteps,m_iPosOrgPause);
      SetPosition(CONTROL_RW_LOGO,   5*iSteps+2*iSteps-m_iFrames,iSteps,m_iPosOrgPause);
      SetPosition(CONTROL_INFO,         5*iSteps+2*iSteps-m_iFrames,iSteps,m_iPosOrgInfo);
      SetPosition(CONTROL_PLAYTIME,     5*iSteps+2*iSteps-m_iFrames,iSteps,m_iPosOrgPlayTime);
      SetPosition(CONTROL_BIG_PLAYTIME, 5*iSteps+2*iSteps-m_iFrames,iSteps,m_iPosOrgBigPlayTime);
      m_iFrames++;
    }
  }
	__int64 lPTS=g_application.m_pPlayer->GetPTS();
  int hh = (int)(lPTS / 36000) % 100;
  int mm = (int)((lPTS / 600) % 60);
  int ss = (int)((lPTS /  10) % 60);
  //int f1 = lPTS % 10;

  int iSpeed=g_application.GetPlaySpeed();
  if (hh==0 && mm==0 && ss<5)
  {
    if (iSpeed < 1)
    {
      iSpeed=1;
      g_application.SetPlaySpeed(iSpeed);
      g_application.m_pPlayer->SeekTime(0);
    }
  }

	char szTime[32];
	if (hh >=1)
	{
		sprintf(szTime,"%02.2i:%02.2i:%02.2i",hh,mm,ss);
	}
	else
	{
		sprintf(szTime,"%02.2i:%02.2i",mm,ss);
	}
	{
		CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_BIG_PLAYTIME); 
		msg.SetLabel(szTime); 
		OnMessage(msg); 
	}

  
  if (iSpeed !=1)
  {
    char szTmp[32];
    sprintf(szTmp,"(%ix)", iSpeed);
    strcat(szTime,szTmp);
    m_iFrames =iSteps ;
  }

  {
		CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_PLAYTIME); 
		msg.SetLabel(szTime); 
		OnMessage(msg); 
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
	CGUIWindow::Render();

	if (m_pTexture)
	{
		
		const CGUIControl* pControl = GetControl(CONTROL_LOGO_PIC); 
    if (pControl)
    {
		  float fXPos=(float)pControl->GetXPosition();
		  float fYPos=(float)pControl->GetYPosition();
		  int iWidth=pControl->GetWidth();
		  int iHeight=pControl->GetHeight();
		  g_graphicsContext.Correct(fXPos,fYPos);

		  CPicture picture;
		  picture.RenderImage(m_pTexture,(int)fXPos,(int)fYPos,iWidth,iHeight,m_iTextureWidth,m_iTextureHeight);
    }
	}
}

void CGUIWindowMusicOverlay::SetID3Tag(ID3_Tag& id3tag)
{
	auto_ptr<char>pYear  (ID3_GetYear( &id3tag  ));
	auto_ptr<char>pTitle (ID3_GetTitle( &id3tag ));
	auto_ptr<char>pArtist(ID3_GetArtist( &id3tag));
	auto_ptr<char>pAlbum (ID3_GetAlbum( &id3tag ));
	auto_ptr<char>pGenre (ID3_GetGenre( &id3tag ));
	int nTrackNum=ID3_GetTrackNum( &id3tag );
	{
		CGUIMessage msg(GUI_MSG_VISIBLE, GetID(), CONTROL_LOGO_PIC); 
		OnMessage(msg);
	}
	{
		CGUIMessage msg1(GUI_MSG_LABEL_RESET, GetID(), CONTROL_INFO); 
		OnMessage(msg1);
	}
	if (NULL != pTitle.get())
	{
		CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
		msg1.SetLabel(pTitle.get() );
		OnMessage(msg1);
	}

	if (NULL != pArtist.get())
	{
		CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
		msg1.SetLabel( pArtist.get());
		OnMessage(msg1);
	}
		
	if ( NULL != pAlbum.get())
	{
		CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
		msg1.SetLabel( pAlbum.get() );
		OnMessage(msg1);
	}
		
	if (nTrackNum >=1)
	{
		CStdString strText=g_localizeStrings.Get(435);
		if (strText.GetAt(strText.size()-1) != ' ')
			strText+=" ";
		CStdString strTrack;
		strTrack.Format(strText+"%i", nTrackNum);
		
		CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
		msg1.SetLabel( strTrack );
		OnMessage(msg1);
	}
	if (NULL != pYear.get() )
	{
		CStdString strText=g_localizeStrings.Get(436);
		if (strText.GetAt(strText.size()-1) != ' ')
			strText+=" ";
		CStdString strYear;
		strYear.Format(strText+"%s", pYear.get());
		
		CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
		msg1.SetLabel( strYear );
		OnMessage(msg1);
	}
}

void CGUIWindowMusicOverlay::SetCurrentFile(const CStdString& strFile)
{
	//	Tries to set the tag information for strFile to window.

	//	Reset frame counter for window fading
  m_iFrames=0;

	//	Release previously shown album 
	//	thumb, if any
	if (m_pTexture)
	{
		m_pTexture->Release();
		m_pTexture=NULL;
	}

	//	Set image visible that is displayed if no thumb is available
	CGUIMessage msg(GUI_MSG_VISIBLE, GetID(), CONTROL_LOGO_PIC); 
	OnMessage(msg);

	//	Reset scrolling text in window
	CGUIMessage msg1(GUI_MSG_LABEL_RESET, GetID(), CONTROL_INFO); 
	OnMessage(msg1);

	//	No audio file, we are finished here
	if (!CUtil::IsAudio(strFile) )
		return;

	CFileItem item;
	item.m_strPath=strFile;
	//	Get a reference to the item's tag
	CMusicInfoTag& tag=item.m_musicInfoTag;

	CURL url(strFile);
	//	if the file is a cdda track, ...
	if (url.GetProtocol()=="cdda" )
	{
		VECFILEITEMS  items;
		CCDDADirectory dir;
		//	... use the directory of the cd to 
		//	get its cddb information...
		if (dir.GetDirectory("D:",items))
		{
			for (int i=0; i < (int)items.size(); ++i)
			{
				CFileItem* pItem=items[i];
				if (pItem->m_strPath==strFile)
				{
					//	...and find current track to use
					//	cddb information for display.
					item=*pItem;
				}
				delete pItem;
			}
		}
	}
	else
	{
		//	we have a audio file.
		//	Look if we have this file in database...
		bool bFound=false;
		CSong song;
		CMusicDatabase dbs;
		if (dbs.Open())
		{
			bFound=dbs.GetSongByFileName(strFile, song);
			dbs.Close();
		}
		if (!bFound && g_stSettings.m_bUseID3)
		{
			//	...no, try to load the tag of the file.
			CMusicInfoTagLoaderFactory factory;
			auto_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(strFile));
			//	Do we have a tag loader for this file type?
			if (NULL != pLoader.get())
			{
				// yes, load its tag
				if ( !pLoader->Load(strFile,tag))
				{
					//	Failed!
					tag.SetLoaded(false);
					//	just to be sure :-)
				}
			}
		}
		else
		{
			//	...yes, this file is found in database
			//	fill the tag of our fileitem
			SYSTEMTIME systime;
			systime.wYear=song.iYear;
			tag.SetReleaseDate(systime);
			tag.SetTrackNumber(song.iTrack);
			tag.SetAlbum(song.strAlbum);
			tag.SetArtist(song.strArtist);
			tag.SetGenre(song.strGenre);
			tag.SetTitle(song.strTitle);
			tag.SetDuration(song.iDuration);
			tag.SetLoaded(true);
		}
	}

	//	If we have tag information, ...
	if (tag.Loaded())
	{
		//	...display them in window

		//	display only, if we have a title
		if (tag.GetTitle().size())
		{
			//	Display available tag information in fade label control

			CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
			msg1.SetLabel(tag.GetTitle());
			OnMessage(msg1);
			
			if (tag.GetArtist().size())
			{
				//	Artist
				CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
				msg1.SetLabel(tag.GetArtist());
				OnMessage(msg1);
			}
			
			if (tag.GetAlbum().size())
			{
				//	Album
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

			SYSTEMTIME systemtime;
			tag.GetReleaseDate(systemtime);
			int iYear=systemtime.wYear;
			if (iYear >=1900)
			{
				//	Year
				CStdString strText=g_localizeStrings.Get(436);	//	"Year:"
				if (strText.GetAt(strText.size()-1) != ' ')
					strText+=" ";
				CStdString strYear;
				strYear.Format(strText+"%i", iYear);
				
				CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
				msg1.SetLabel(strYear);
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
		}	//	if (tag.GetTitle().size())
		else 
		{
			//	No title in tag, show filename only
			CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
			msg1.SetLabel( CUtil::GetFileName(strFile) );
			OnMessage(msg1);
		}
	}	//	if (tag.Loaded())
	else 
	{
		//	If we have a cdda track without cddb information,...
		if (!tag.Loaded() && url.GetProtocol()=="cdda" )
		{
			//	we have the tracknumber...
			int iTrack=tag.GetTrackNumber();
			if (iTrack >=1)
			{
				CStdString strText=g_localizeStrings.Get(435);	//	"Track"
				if (strText.GetAt(strText.size()-1) != ' ')
					strText+=" ";
				CStdString strTrack;
				strTrack.Format(strText+"%i", iTrack);
				
				CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
				msg1.SetLabel(strTrack);
				OnMessage(msg1);
			}

			//	...and its duration for display.
			if (tag.GetDuration() > 0)
			{
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
		}	//	if (!tag.Loaded() && url.GetProtocol()=="cdda" )
		else 
		{
			//	No tag information available for this file, show filename only
			CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
			msg1.SetLabel( CUtil::GetFileName(strFile) );
			OnMessage(msg1);
		}
	}


	//	Find a thumb for this file.
	CUtil::SetMusicThumb(&item);
	if (item.HasThumbnail())
	{
		//	Found, so load it.
		CPicture picture;
		m_pTexture=picture.Load(item.GetThumbnailImage());
		m_iTextureWidth=picture.GetWidth();
		m_iTextureHeight=picture.GetHeight();
		//	 thumb correctly loaded?
		if (m_pTexture)
		{
			//	Hide image that is displayed, if no thumb is available
			CGUIMessage msg1(GUI_MSG_HIDDEN, GetID(), CONTROL_LOGO_PIC); 
			OnMessage(msg1);
		}
	}
}