
#include "GUIWindowVideoInfo.h"
#include "graphiccontext.h"
#include "localizestrings.h"
#include "utils/http.h"
#include "utils/log.h"
#include "util.h"
#include "picture.h"
#include "application.h"
#include "videodatabase.h"

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
#define CONTROL_DISC          7

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
      CGUIMessage msg(GUI_MSG_LABEL_RESET,GetID(),CONTROL_DISC,0,0,NULL);
      g_graphicsContext.SendMessage(msg);         
			for (int i=0; i < 1000; ++i)
			{
				CStdString strItem;
        strItem.Format("DVD#%03i", i);
				CGUIMessage msg2(GUI_MSG_LABEL_ADD,GetID(),CONTROL_DISC,0,0);
				msg2.SetLabel(strItem);
				g_graphicsContext.SendMessage(msg2);    
			}
      
      SET_CONTROL_HIDDEN(GetID(),CONTROL_DISC);
      CONTROL_DISABLE(GetID(),CONTROL_DISC);
      int iItem=0;
      if ( CUtil::IsISO9660(m_pMovie->m_strPath) || CUtil::IsDVD(m_pMovie->m_strPath) )
      {
        SET_CONTROL_VISIBLE(GetID(),CONTROL_DISC);
        CONTROL_ENABLE(GetID(),CONTROL_DISC);
        char szNumber[1024];
        int iPos=0;
        bool bNumber=false;
        for (int i=0; i < (int)m_pMovie->m_strDVDLabel.size();++i)
        {
          char kar=m_pMovie->m_strDVDLabel.GetAt(i);
          if (kar >='0'&& kar <= '9' )
          {
            szNumber[iPos]=kar;
            iPos++;
            szNumber[iPos]=0;
            bNumber=true;
          }
          else
          {
            if (bNumber) break;
          }
        }
        int iDVD=0;
        if (strlen(szNumber))
        {
          int x=0;
          while (szNumber[x]=='0' && x < (int)strlen(szNumber) ) x++;
          if (x < (int)strlen(szNumber))
          {
            sscanf(&szNumber[x],"%i", &iDVD);
            if (iDVD <0 && iDVD >=1000)
              iDVD=-1;
          }
        }
        if (iDVD<=0) iDVD=0;
        iItem=iDVD;
        
        CGUIMessage msgSet(GUI_MSG_ITEM_SELECT,GetID(),CONTROL_DISC,iItem,0,NULL);
			  g_graphicsContext.SendMessage(msgSet);         
      }
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
          CStdString strImage=m_pMovie->m_strIMDBNumber;
          CUtil::GetVideoThumbnail(strImage,strThumb);
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
      if (iControl==CONTROL_DISC)
      {
        int iItem=0;
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),iControl,0,0,NULL);
        g_graphicsContext.SendMessage(msg);         
				wstring wstrHD=msg.GetLabel();
				CStdString strItem;
				CUtil::Unicode2Ansi(wstrHD, strItem);
        if (strItem!="HD" && strItem !="share") 
        {
          long lMovieId;
          sscanf(m_pMovie->m_strSearchString.c_str(),"%i", &lMovieId);
          if (lMovieId>0)
          {
            CVideoDatabase dbs;
            dbs.Open();
            dbs.SetDVDLabel( lMovieId,strItem);
            dbs.Close();
          }
        }
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

	  CStdString strThumb="";
	  CStdString strImage=m_pMovie->m_strPictureURL;
    if (strImage.size() >0 && m_pMovie->m_strSearchString.size() > 0)
    {
      CUtil::GetVideoThumbnail(m_pMovie->m_strIMDBNumber,strThumb);
	    if (!CUtil::FileExists(strThumb) )
	    {
		    CHTTP http;
		    CStdString strExtension;
		    CUtil::GetExtension(strImage,strExtension);
		    CStdString strTemp="T:\\temp";
		    strTemp+=strExtension;
        ::DeleteFile(strTemp.c_str());
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
        ::DeleteFile(strTemp.c_str());
	    }
      CUtil::GetVideoThumbnail(m_pMovie->m_strIMDBNumber,strThumb);
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
    //OutputDebugString("update\n");
	  Update();
    //OutputDebugString("updated\n");
  }
  catch(...)
  {
  }
}
bool CGUIWindowVideoInfo::NeedRefresh() const
{
  return m_bRefresh;
}