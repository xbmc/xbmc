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

			CSong song;
			CMusicDatabase dbs;
			if (dbs.Open())
			{
				bool bContinue(false);
				CURL url(strFile);
				if (url.GetProtocol()=="cdda" )
				{
					VECFILEITEMS  items;
					CCDDADirectory dir;
					if (dir.GetDirectory("D:",items))
					{
						for (int i=0; i < (int)items.size(); ++i)
						{
							CFileItem* pItem=items[i];
							if (pItem->m_strPath==strFile)
							{
								SYSTEMTIME systime;
								song.iTrack		=pItem->m_musicInfoTag.GetTrackNumber();
								song.strAlbum =pItem->m_musicInfoTag.GetAlbum();
								song.strArtist=pItem->m_musicInfoTag.GetArtist();
								song.strFileName=strFile;
								song.strGenre==pItem->m_musicInfoTag.GetGenre();
								song.strTitle=pItem->m_musicInfoTag.GetTitle();
								song.iDuration=pItem->m_musicInfoTag.GetDuration();
								pItem->m_musicInfoTag.GetReleaseDate(systime);
								song.iYear=systime.wYear;
								bContinue=true;
							}
							delete pItem;
						}
					}
				}
				else
				{
					bContinue=dbs.GetSongByFileName(strFile, song);
				}
				if (bContinue)
				{
					if (song.strTitle.size())
					{
						CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
						msg1.SetLabel(song.strTitle );
						OnMessage(msg1);
						
						if (song.strArtist.size())
						{
							CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
							msg1.SetLabel( song.strArtist );
							OnMessage(msg1);
						}
						
						if ( song.strAlbum.size())
						{
							CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
							msg1.SetLabel( song.strAlbum );
							OnMessage(msg1);
							strAlbum=song.strAlbum;
						}
						
						int iTrack=song.iTrack;
						if (iTrack >=1)
						{
							CStdString strTrack;
							strTrack.Format("Track %i", iTrack);
							
							CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
							msg1.SetLabel( strTrack );
							OnMessage(msg1);
						}

						if (song.iYear >=1900)
						{
							CStdString strYear;
							strYear.Format("Year:%i", song.iYear);
							
							CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
							msg1.SetLabel( strYear );
							OnMessage(msg1);
						}
						if (song.iDuration > 0)
						{
							CStdString strYear;
							strYear.Format("Duration: %i:%02.2i", song.iDuration/60, song.iDuration%60);
							
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
				dbs.Close();
			}
			else
			{
					CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
					msg1.SetLabel( CUtil::GetFileName(strFile) );
					OnMessage(msg1);
			}
		
			if ( dbs.Open() )
			{
				CAlbum album;
				if ( dbs.GetAlbumInfo(strAlbum, album) )
				{
					CStdString strThumb;
					CUtil::GetAlbumThumb(album.strAlbum,strThumb);
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
				dbs.Close();
			}
		}
	}