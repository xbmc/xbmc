
#include "GUIWindowMusicInfo.h"
#include "graphiccontext.h"
#include "localizestrings.h"
#include "utils/http.h"
#include "util.h"
#include "picture.h"
#include "application.h"

#define	CONTROL_ALBUM		20
#define	CONTROL_ARTIST	21
#define	CONTROL_DATE 		22
#define	CONTROL_RATING	23
#define	CONTROL_GENRE		24
#define	CONTROL_TONE 		25
#define	CONTROL_STYLES	26

#define CONTROL_IMAGE		 3
#define CONTROL_TEXTAREA 4

#define CONTROL_BTN_TRACKS	5
#define CONTROL_BTN_REFRESH	6

CGUIWindowMusicInfo::CGUIWindowMusicInfo(void)
:CGUIDialog(0)
{
	m_pTexture=NULL;
}

CGUIWindowMusicInfo::~CGUIWindowMusicInfo(void)
{
}


void CGUIWindowMusicInfo::OnAction(const CAction &action)
{
	if (action.wID == ACTION_PREVIOUS_MENU)
    {
		Close();
		return;
    }
	CGUIWindow::OnAction(action);
}

bool CGUIWindowMusicInfo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
		case GUI_MSG_WINDOW_DEINIT:
		{
			m_pAlbum=NULL;
			if (m_pTexture)
			{
				m_pTexture->Release();
				m_pTexture=NULL;
			}
			g_application.EnableOverlay();
		}
		break;

    case GUI_MSG_WINDOW_INIT:
    {
			CGUIWindow::OnMessage(message);
			g_application.DisableOverlay();
			m_pTexture=NULL;
			m_bViewReview=true;
			Refresh();
			return true;
    }
		break;


    case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();
			if (iControl==CONTROL_BTN_REFRESH)
			{
				if ( m_pAlbum->Load())
				{
					CStdString strThumb;
					CStdString strImage=m_pAlbum->GetImageURL();
					CUtil::GetThumbnail(strImage,strThumb);
					DeleteFile(strThumb.c_str());
				}
				Refresh();
			}

			if (iControl==CONTROL_BTN_TRACKS)
			{
				m_bViewReview=!m_bViewReview;
				Update();
			}
    }
    break;
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIWindowMusicInfo::SetAlbum(CMusicAlbumInfo& album)
{
		m_pAlbum=&album;
}

void CGUIWindowMusicInfo::Update()
{
	if (!m_pAlbum) return;
	SetLabel(CONTROL_ALBUM, m_pAlbum->GetTitle() );
	SetLabel(CONTROL_ARTIST, m_pAlbum->GetArtist() );
	SetLabel(CONTROL_DATE, m_pAlbum->GetDateOfRelease() );

	CStdString strRating;
	if (m_pAlbum->GetRating() > 0)
		strRating.Format("%i/9", m_pAlbum->GetRating());
	SetLabel(CONTROL_RATING, strRating );

	SetLabel(CONTROL_GENRE, m_pAlbum->GetGenre() );
	SetLabel(CONTROL_TONE, m_pAlbum->GetTones() );
	SetLabel(CONTROL_STYLES, m_pAlbum->GetStyles() );

	if (m_bViewReview)
	{
			SET_CONTROL_LABEL(GetID(), CONTROL_TEXTAREA,m_pAlbum->GetReview());
			SET_CONTROL_LABEL(GetID(), CONTROL_BTN_TRACKS,182);
	}
	else
	{
		CStdString strLine;
		for (int i=0; i < m_pAlbum->GetNumberOfSongs();++i)
		{
			const CMusicSong& song=m_pAlbum->GetSong(i);
			CStdString strTmp;
			strTmp.Format("%i. %-30s\n",
							song.GetTrack(), 
							song.GetSongName());
			strLine+=strTmp;
		};

		SET_CONTROL_LABEL(GetID(), CONTROL_TEXTAREA,strLine);

		for (int i=0; i < m_pAlbum->GetNumberOfSongs();++i)
		{
			const CMusicSong& song=m_pAlbum->GetSong(i);
			CStdString strTmp;
			CUtil::SecondsToHMSString(song.GetDuration(), strTmp);
			CGUIMessage msg3(GUI_MSG_LABEL2_SET,GetID(),CONTROL_TEXTAREA,i,0);
			msg3.SetLabel(strTmp);
			g_graphicsContext.SendMessage(msg3);
		}

		SET_CONTROL_LABEL(GetID(), CONTROL_BTN_TRACKS,183);

	}

}

void CGUIWindowMusicInfo::SetLabel(int iControl, const CStdString& strLabel)
{
	CStdString strLabel1=strLabel;
	if (strLabel1.size()==0)
		strLabel1=g_localizeStrings.Get(416);
	
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),iControl);
	msg.SetLabel(strLabel1);
	OnMessage(msg);

}

void  CGUIWindowMusicInfo::Render()
{
	CGUIDialog::Render();
  if (!m_pTexture) return;

	const CGUIControl* pControl=GetControl(CONTROL_IMAGE);
  if (pControl)
  {
	  float x=(float)pControl->GetXPosition();
	  float y=(float)pControl->GetYPosition();
	  DWORD width=pControl->GetWidth();
	  DWORD height=pControl->GetHeight();
	  g_graphicsContext.Correct(x,y);

	  CPicture picture;
	  picture.RenderImage(m_pTexture,(int)x,(int)y,width,height,m_iTextureWidth,m_iTextureHeight);
  }
}


void CGUIWindowMusicInfo::Refresh()
{
	if (m_pTexture)
	{
		m_pTexture->Release();
		m_pTexture=NULL;
	}

	CStdString strThumb;
	CStdString strImage=m_pAlbum->GetImageURL();
	CUtil::GetAlbumThumb(m_pAlbum->GetTitle()+m_pAlbum->GetAlbumPath(),strThumb);
	if (!CUtil::ThumbExists(strThumb) )
	{
		CHTTP http;
		http.Download(strImage,strThumb);
	}


	if (CUtil::FileExists(strThumb) )
	{
		CPicture picture;
		m_pTexture=picture.Load(strThumb);
		m_iTextureWidth=picture.GetWidth();
		m_iTextureHeight=picture.GetWidth();
		CUtil::ThumbCacheAdd(strThumb, true);
	}
	Update();
}