
#include "GUIWindowVideoInfo.h"
#include "graphiccontext.h"
#include "localizestrings.h"
#include "utils/http.h"
#include "util.h"
#include "picture.h"
#include "application.h"

#define	CONTROL_TITLE					20
#define	CONTROL_DIRECTOR			21
#define	CONTROL_CREDITS 			22
#define	CONTROL_GENRE					23
#define	CONTROL_YEAR					24
#define	CONTROL_TAGLINE 			25
#define	CONTROL_PLOTOUTLINE		26
#define	CONTROL_RATING				27
#define	CONTROL_VOTES 				28
#define	CONTROL_VOTES 				28
#define	CONTROL_CAST	 				29

#define CONTROL_IMAGE		 			3
#define CONTROL_TEXTAREA 			4

#define CONTROL_BTN_TRACKS		5
#define CONTROL_BTN_REFRESH		6

CGUIWindowVideoInfo::CGUIWindowVideoInfo(void)
:CGUIDialog(0)
{
	m_pTexture=NULL;
}

CGUIWindowVideoInfo::~CGUIWindowVideoInfo(void)
{
}


void CGUIWindowVideoInfo::OnKey(const CKey& key)
{
  if (key.IsButton())
  {
    if ( key.GetButtonCode() == KEY_BUTTON_BACK  || key.GetButtonCode() == KEY_REMOTE_BACK)
    {
      Close();
      return;
    }
  }
  CGUIWindow::OnKey(key);
}

bool CGUIWindowVideoInfo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
		case GUI_MSG_WINDOW_DEINIT:
		{
			m_pMovie=NULL;
			if (m_pTexture)
			{
				m_pTexture->Release();
				m_pTexture=NULL;
				m_pMovie=NULL;
			}
			g_application.EnableOverlay();
		}
		break;

    case GUI_MSG_WINDOW_INIT:
    {
			g_application.DisableOverlay();
			m_pTexture=NULL;
			m_bViewReview=true;
			Refresh();
    }
		break;


    case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();
			if (iControl==CONTROL_BTN_REFRESH)
			{
				if ( m_pMovie->m_strPictureURL.size() )
				{
					CStdString strThumb;
					CStdString strImage=m_pMovie->m_strSearchString;
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

void CGUIWindowVideoInfo::SetMovie(CIMDBMovie& album)
{
		m_pMovie=&album;
}

void CGUIWindowVideoInfo::Update()
{
	if (!m_pMovie) return;
	SetLabel(CONTROL_TITLE, m_pMovie->m_strTitle );
	SetLabel(CONTROL_DIRECTOR, m_pMovie->m_strDirector );
	SetLabel(CONTROL_CREDITS, m_pMovie->m_strWritingCredits );
	SetLabel(CONTROL_GENRE, m_pMovie->m_strGenre );
	SetLabel(CONTROL_TAGLINE, m_pMovie->m_strTagLine);
	SetLabel(CONTROL_PLOTOUTLINE, m_pMovie->m_strPlotOutline );

	CStdString strYear;
	strYear.Format("%i", m_pMovie->m_iYear);
	SetLabel(CONTROL_YEAR, strYear );

	CStdString strRating;
	strRating.Format("%04.2f", m_pMovie->m_fRating);
	SetLabel(CONTROL_RATING, strRating );
	SetLabel(CONTROL_VOTES, m_pMovie->m_strVotes );
	//SetLabel(CONTROL_CAST, m_pMovie->m_strCast );

	if (m_bViewReview)
	{
			SET_CONTROL_LABEL(GetID(), CONTROL_TEXTAREA,m_pMovie->m_strPlot);
			SET_CONTROL_LABEL(GetID(), CONTROL_BTN_TRACKS,206);
	}
	else
	{
			SET_CONTROL_LABEL(GetID(), CONTROL_TEXTAREA,m_pMovie->m_strCast);
			SET_CONTROL_LABEL(GetID(), CONTROL_BTN_TRACKS,207);
		
	}
}

void CGUIWindowVideoInfo::SetLabel(int iControl, const CStdString& strLabel)
{
	if (strLabel.size()==0)	return;
	
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),iControl);
	msg.SetLabel(strLabel);
	OnMessage(msg);

}

void  CGUIWindowVideoInfo::Render()
{
	CGUIDialog::Render();
  if (!m_pTexture) return;

	const CGUIControl* pControl=GetControl(CONTROL_IMAGE);
	DWORD x=pControl->GetXPosition();
	DWORD y=pControl->GetYPosition();
	DWORD width=pControl->GetWidth();
	DWORD height=pControl->GetHeight();

	CPicture picture;
	picture.RenderImage(m_pTexture,x,y,width,height,m_iTextureWidth,m_iTextureHeight);
}


void CGUIWindowVideoInfo::Refresh()
{
	if (m_pTexture)
	{
		m_pTexture->Release();
		m_pTexture=NULL;
	}

	CStdString strThumb;
	CStdString strImage=m_pMovie->m_strPictureURL;
	CUtil::GetThumbnail(m_pMovie->m_strSearchString,strThumb);
	if (!CUtil::FileExists(strThumb) )
	{
		CHTTP http;
		CStdString strExtension;
		CUtil::GetExtension(strImage,strExtension);
		CStdString strTemp="T:\\temp";
		strTemp+=strExtension;
		http.Download(strImage, strTemp);
		
		CPicture picture;
		picture.Convert(strTemp,strThumb);

	}
	CUtil::GetThumbnail(m_pMovie->m_strSearchString,strThumb);
	CStdString strAlbum;
	CUtil::GetIMDBInfo(m_pMovie->m_strSearchString,strAlbum);
	m_pMovie->Save(strAlbum);

	if (CUtil::FileExists(strThumb) )
	{
		CPicture picture;
		m_pTexture=picture.Load(strThumb);
		m_iTextureWidth=picture.GetWidth();
		m_iTextureHeight=picture.GetWidth();
		
	}
	Update();
}