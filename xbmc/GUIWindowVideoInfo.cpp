
#include "stdafx.h"
#include "GUIWindowVideoInfo.h"
#include "util.h"
#include "picture.h"
#include "application.h"

#define	CONTROL_TITLE				20
#define	CONTROL_DIRECTOR			21
#define	CONTROL_CREDITS 			22
#define	CONTROL_GENRE				23
#define	CONTROL_YEAR				24
#define	CONTROL_TAGLINE 			25
#define	CONTROL_PLOTOUTLINE			26
#define	CONTROL_RATING				27
#define	CONTROL_VOTES 				28
#define	CONTROL_CAST	 			29
#define CONTROL_RATING_AND_VOTES		30
#define CONTROL_RUNTIME				31


#define CONTROL_IMAGE				3
#define CONTROL_TEXTAREA 			4

#define CONTROL_BTN_TRACKS			5
#define CONTROL_BTN_REFRESH			6
#define CONTROL_DISC          			7

CGUIWindowVideoInfo::CGUIWindowVideoInfo(void)
:CGUIDialog(0)
{
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
	CGUIDialog::OnAction(action);
}

bool CGUIWindowVideoInfo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
		case GUI_MSG_WINDOW_DEINIT:
		{
			m_pMovie=NULL;
		}
		break;

    case GUI_MSG_WINDOW_INIT:
    {
      m_bRefresh=false;
			CGUIDialog::OnMessage(message);
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
      
      SET_CONTROL_HIDDEN(CONTROL_DISC);
      CONTROL_DISABLE(CONTROL_DISC);
      int iItem=0;
			CFileItem movie(m_pMovie->m_strPath, false);
      if ( movie.IsISO9660() || movie.IsDVD() )
      {
        SET_CONTROL_VISIBLE(CONTROL_DISC);
        CONTROL_ENABLE(CONTROL_DISC);
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
            CStdString label;
            dbs.GetDVDLabel(lMovieId, label);
            int iPos = label.Find("DVD#");
            if (iPos >= 0) {
              label.Delete(iPos, label.GetLength());
            }
            label = label.TrimRight(" ");
            label += " ";
            label += strItem;
            dbs.SetDVDLabel( lMovieId,label);
            dbs.Close();
          }
        }
      }
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
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
	strRating.Format("%03.1f", m_pMovie->m_fRating);
	SetLabel(CONTROL_RATING, strRating );

	strTmp=m_pMovie->m_strVotes; strTmp.Trim();
	SetLabel(CONTROL_VOTES, strTmp.c_str() );
	//SetLabel(CONTROL_CAST, m_pMovie->m_strCast );

	CStdString strRating_And_Votes;
	if (strRating.Equals("0.0")) {strRating_And_Votes=m_pMovie->m_strVotes;} else
	// if rating is 0 there are no votes so display not available message already set in Votes string
	{
	strRating_And_Votes.Format("%s (%s votes)", strRating, strTmp);
	SetLabel(CONTROL_RATING_AND_VOTES, strRating_And_Votes);
	}

        strTmp=m_pMovie->m_strRuntime; strTmp.Trim();
        SetLabel(CONTROL_RUNTIME,  strTmp.c_str() );

	if (m_bViewReview)
	{
      strTmp=m_pMovie->m_strPlot; strTmp.Trim();
			SET_CONTROL_LABEL(CONTROL_TEXTAREA,strTmp.c_str() );
			SET_CONTROL_LABEL(CONTROL_BTN_TRACKS,206);
	}
	else
	{
      strTmp=m_pMovie->m_strCast; strTmp.Trim();
			SET_CONTROL_LABEL(CONTROL_TEXTAREA,strTmp.c_str() );
			SET_CONTROL_LABEL(CONTROL_BTN_TRACKS,207);
		
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
}


void CGUIWindowVideoInfo::Refresh()
{
	// quietly return if Internet lookups are disabled
	if (!g_guiSettings.GetBool("Network.EnableInternet"))
	{
		Update();
		return;
	}

  CUtil::ClearCache();
  try
  {
    OutputDebugString("Refresh\n");

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
		    CStdString strTemp="Z:\\temp";
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

	  if (!CUtil::FileExists(strThumb) )
	  {
      strThumb = "";
    }
  		
    const CGUIControl* pControl=GetControl(CONTROL_IMAGE);
    if (pControl)
    {
    	CGUIImage* pImageControl=(CGUIImage*)pControl;
      pImageControl->SetKeepAspectRatio(true);
      pImageControl->SetFileName(strThumb);
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
