
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


void CGUIWindowVideoInfo::OnAction(const CAction &action)
{
	if (action.wID == ACTION_PREVIOUS_MENU)
    {
		Close();
		return;
    }
	CGUIWindow::OnAction(action);
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
      m_bRefresh=false;
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
				if ( m_pMovie->m_strPictureURL.size() )
				{
					CStdString strThumb;
					CStdString strImage=m_pMovie->m_strSearchString;
					CUtil::GetThumbnail(strImage,strThumb);
					DeleteFile(strThumb.c_str());
				}
        m_bRefresh=true;
        Close();
        return true;
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
  CStdString strTmp;
  strTmp=m_pMovie->m_strTitle; strTmp.Trim();
  SetLabel(CONTROL_TITLE, strTmp.c_str() );

  strTmp=m_pMovie->m_strDirector; strTmp.Trim();
  SetLabel(CONTROL_DIRECTOR, strTmp.c_str() );

    strTmp=m_pMovie->m_strWritingCredits; strTmp.Trim();
	SetLabel(CONTROL_CREDITS, strTmp.c_str() );

	strTmp=m_pMovie->m_strGenre; strTmp.Trim();
	SetLabel(CONTROL_GENRE,  strTmp.c_str() );
    
	{
	CGUIMessage msg1(GUI_MSG_LABEL_RESET, GetID(), CONTROL_TAGLINE); 
    OnMessage(msg1);
	}
	{
	strTmp=m_pMovie->m_strTagLine; strTmp.Trim();
	CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_TAGLINE); 
    msg1.SetLabel( strTmp );
    OnMessage(msg1);
	}
	{
	CGUIMessage msg1(GUI_MSG_LABEL_RESET, GetID(), CONTROL_PLOTOUTLINE); 
    OnMessage(msg1);
	}
	{
	strTmp=m_pMovie->m_strPlotOutline; strTmp.Trim();
	CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_PLOTOUTLINE); 
    msg1.SetLabel( strTmp );
    OnMessage(msg1);
	}
	CStdString strYear;
	strYear.Format("%i", m_pMovie->m_iYear);
	SetLabel(CONTROL_YEAR, strYear );

	CStdString strRating;
	strRating.Format("%04.2f", m_pMovie->m_fRating);
	SetLabel(CONTROL_RATING, strRating );

  
	strTmp=m_pMovie->m_strVotes; strTmp.Trim();
	SetLabel(CONTROL_VOTES, strTmp.c_str() );
	//SetLabel(CONTROL_CAST, m_pMovie->m_strCast );

	if (m_bViewReview)
	{
      strTmp=m_pMovie->m_strPlot; strTmp.Trim();
			SET_CONTROL_LABEL(GetID(), CONTROL_TEXTAREA,strTmp.c_str() );
			SET_CONTROL_LABEL(GetID(), CONTROL_BTN_TRACKS,206);
	}
	else
	{
      strTmp=m_pMovie->m_strCast; strTmp.Trim();
			SET_CONTROL_LABEL(GetID(), CONTROL_TEXTAREA,strTmp.c_str() );
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

  //OutputDebugString("render texture\n");
	const CGUIControl* pControl=GetControl(CONTROL_IMAGE);
  if (pControl)
  {
	  float x=(float)pControl->GetXPosition();
	  float y=(float)pControl->GetYPosition();
	  DWORD width=pControl->GetWidth();
	  DWORD height=pControl->GetHeight();
	  g_graphicsContext.Correct(x,y);

    DWORD dwWidth=m_iTextureWidth;
    DWORD dwHeight=m_iTextureHeight;
    float fAspect= ((float)dwWidth) / ((float)dwHeight);
    if (dwWidth > width )
    {
      dwWidth  = width;
      dwHeight = (DWORD)( ( (float)height) / fAspect);
    }

    if (dwHeight > height )
    {
      dwHeight = height;
      dwWidth  = (DWORD)(  fAspect * ( (float)height) );
    }
  	
	  CPicture picture;
	  picture.RenderImage(m_pTexture,(int)x,(int)y,dwWidth,dwHeight,m_iTextureWidth,m_iTextureHeight);
  }
}


void CGUIWindowVideoInfo::Refresh()
{
  try
  {
    OutputDebugString("Refresh\n");
	  if (m_pTexture)
	  {
		  m_pTexture->Release();
		  m_pTexture=NULL;
	  }

	  CStdString strThumb;
	  CStdString strImage=m_pMovie->m_strPictureURL;
    if (strImage.size() >0)
    {
	    CUtil::GetThumbnail(m_pMovie->m_strSearchString,strThumb);
	    if (!CUtil::FileExists(strThumb) )
	    {
		    CHTTP http;
		    CStdString strExtension;
		    CUtil::GetExtension(strImage,strExtension);
		    CStdString strTemp="T:\\temp";
		    strTemp+=strExtension;
		    http.Download(strImage, strTemp);

        try
        {
		      CPicture picture;
		      picture.Convert(strTemp,strThumb);
        }
        catch(...)
        {
          OutputDebugString("...\n");
          ::DeleteFile(strThumb.c_str());
        }
	    }
	    CUtil::GetThumbnail(m_pMovie->m_strSearchString,strThumb);
    }
    //CStdString strAlbum;
	  //CUtil::GetIMDBInfo(m_pMovie->m_strSearchString,strAlbum);
	  //m_pMovie->Save(strAlbum);

	  if (strThumb.size() > 0 && CUtil::FileExists(strThumb) )
	  {
		  CPicture picture;
		  m_pTexture=picture.Load(strThumb);
		  m_iTextureWidth=picture.GetWidth();
      m_iTextureHeight=picture.GetHeight();
  		
	  }
    OutputDebugString("update\n");
	  Update();
    OutputDebugString("updated\n");
  }
  catch(...)
  {
  }
}
bool CGUIWindowVideoInfo::NeedRefresh() const
{
  return m_bRefresh;
}