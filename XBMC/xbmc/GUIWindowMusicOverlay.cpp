#include "GUIWindowMusicOverlay.h"
#include "settings.h"
#include "guiWindowManager.h"
#include "localizestrings.h"
#include "util.h"
#include "stdstring.h"
#include "application.h"
#include "MusicInfoTag.h"
#include "albumdatabase.h"
#include "picture.h"


using namespace DATABASE;
using namespace MUSIC_INFO;

#define CONTROL_LOGO_PIC    1
#define CONTROL_PLAYTIME		2
#define CONTROL_PLAY_LOGO   3
#define CONTROL_PAUSE_LOGO  4
#define CONTROL_INFO			  5
#define CONTROL_BIG_PLAYTIME 6

CGUIWindowMusicOverlay::CGUIWindowMusicOverlay()
:CGUIWindow(0)
{
	m_pTexture=NULL;
}

CGUIWindowMusicOverlay::~CGUIWindowMusicOverlay()
{
}


void CGUIWindowMusicOverlay::OnKey(const CKey& key)
{
  CGUIWindow::OnKey(key);
}

bool CGUIWindowMusicOverlay::OnMessage(CGUIMessage& message)
{
  return CGUIWindow::OnMessage(message);
}


void CGUIWindowMusicOverlay::Render()
{
	if (!g_application.m_pPlayer) return;
	if ( g_application.m_pPlayer->HasVideo()) return;

	__int64 lPTS=g_application.m_pPlayer->GetPTS();
  //int hh = (lPTS / 36000) % 100;
  int mm = (int)((lPTS / 600) % 60);
  int ss = (int)((lPTS /  10) % 60);
  //int f1 = lPTS % 10;
	
	char szTime[32];
	sprintf(szTime,"%02.2i:%02.2i",mm,ss);
	{
		CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_PLAYTIME); 
		msg.SetLabel(szTime); 
		OnMessage(msg); 
	}
	{
		CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_BIG_PLAYTIME); 
		msg.SetLabel(szTime); 
		OnMessage(msg); 
	}

	if (g_application.m_pPlayer->IsPaused() )
	{
		CGUIMessage msg1(GUI_MSG_HIDDEN, GetID(), CONTROL_PLAY_LOGO); 
		OnMessage(msg1);
		CGUIMessage msg2(GUI_MSG_VISIBLE, GetID(), CONTROL_PAUSE_LOGO); 
		OnMessage(msg2); 
	}
	else
	{
		CGUIMessage msg1(GUI_MSG_VISIBLE, GetID(), CONTROL_PLAY_LOGO); 
		OnMessage(msg1);
		CGUIMessage msg2(GUI_MSG_HIDDEN, GetID(), CONTROL_PAUSE_LOGO); 
		OnMessage(msg2); 
	}
	CGUIWindow::Render();

	if (m_pTexture)
	{
		
		const CGUIControl* pControl = GetControl(CONTROL_LOGO_PIC); 
		int iXPos=pControl->GetXPosition();
		int iYPos=pControl->GetYPosition();
		int iWidth=pControl->GetWidth();
		int iHeight=pControl->GetHeight();
		CPicture picture;
		picture.RenderImage(m_pTexture,iXPos,iYPos,iWidth,iHeight,m_iTextureWidth,m_iTextureHeight);
	}
}


	void CGUIWindowMusicOverlay::SetCurrentFile(const CStdString& strFile)
	{
		CStdString strAlbum=strFile;
		if (m_pTexture)
		{
			m_pTexture->Release();
			m_pTexture=NULL;
		}
		CGUIMessage msg(GUI_MSG_VISIBLE, GetID(), CONTROL_LOGO_PIC); 
		OnMessage(msg);
		if ( CUtil::IsAudio(strFile) )
		{
			CGUIMessage msg1(GUI_MSG_LABEL_RESET, GetID(), CONTROL_INFO); 
			OnMessage(msg1);

			CMusicInfoTag tag;
			CStdString strCacheName;
			CUtil::GetSongInfo(strFile, strCacheName);

			if ( tag.Load(strCacheName) ) 
			{
				if (tag.GetTitle().size())
				{
					CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
					msg1.SetLabel(tag.GetTitle() );
					OnMessage(msg1);
					
					if (tag.GetArtist().size())
					{
						CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
						msg1.SetLabel(tag.GetArtist() );
						OnMessage(msg1);
					}
					
					if (tag.GetAlbum().size())
					{
						CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
						msg1.SetLabel(tag.GetAlbum() );
						OnMessage(msg1);
						strAlbum=tag.GetAlbum();
					}
					
					int iTrack=tag.GetTrackNumber();
					if (iTrack >=1)
					{
						CStdString strTrack;
						strTrack.Format("Track %i", iTrack);
						
						CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
						msg1.SetLabel( strTrack );
						OnMessage(msg1);
					}

					SYSTEMTIME dateTime;
					tag.GetReleaseDate(dateTime);
					if (dateTime.wYear >=1900)
					{
						CStdString strYear;
						strYear.Format("%i", dateTime.wYear);
						
						CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
						msg1.SetLabel( strYear );
						OnMessage(msg1);
					}
				}
				else
				{
						CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
						msg1.SetLabel( CUtil::GetFileName(strFile) );
						OnMessage(msg1);
				}
			}
			else
			{
					CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
					msg1.SetLabel( CUtil::GetFileName(strFile) );
					OnMessage(msg1);
			}
		}
	
		CAlbumDatabase albumDatabase(strAlbum);
		if (albumDatabase.Load())
		{
			CStdString strURL=albumDatabase.GetAlbumInfoURL();
			if (CUtil::FileExists(strURL) )
			{
				CMusicAlbumInfo album;
				if ( album.Load(strURL))
				{
					CStdString strThumb;
					CUtil::GetAlbumThumb(album.GetTitle(),strThumb);
					if (CUtil::FileExists(strThumb) )
					{
						CPicture picture;
						m_pTexture=picture.Load(strThumb);
						m_iTextureWidth=picture.GetWidth();
						m_iTextureHeight=picture.GetHeight();
						if (m_pTexture)
						{							
							CGUIMessage msg1(GUI_MSG_HIDDEN, GetID(), CONTROL_LOGO_PIC); 
							OnMessage(msg1);
						}
					}
				}
			}
		}
	}