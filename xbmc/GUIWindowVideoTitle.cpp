// Todo:
//  - directory history
//  - if movie does not exists when play movie is called then show dialog asking to insert the correct CD
//  - show if movie has subs

#include "stdafx.h"
#include "GUIWindowVideoTitle.h"
#include "Util.h"
#include "Utils/imdb.h"
#include "GUIWindowVideoInfo.h"
#include "NFOFile.h"
#include "PlayListPlayer.h"
#include "GUIThumbnailPanel.h"
#include "GUIPassword.h"


#define CONTROL_BTNVIEWASICONS   2
#define CONTROL_BTNSORTBY      3
#define CONTROL_BTNSORTASC     4
#define CONTROL_BTNTYPE            5
#define CONTROL_PLAY_DVD           6
#define CONTROL_STACK              7
#define CONTROL_IMDB        9
#define CONTROL_LIST       50
#define CONTROL_THUMBS      51
#define CONTROL_BIGLIST   52
#define CONTROL_LABELFILES        12
#define LABEL_TITLE              100

//****************************************************************************************************************************
struct SSortVideoTitleByTitle
{
  static bool Sort(CFileItem* pStart, CFileItem* pEnd)
  {
    CFileItem& rpStart = *pStart;
    CFileItem& rpEnd = *pEnd;
    if (rpStart.GetLabel() == "..") return true;
    if (rpEnd.GetLabel() == "..") return false;
    bool bGreater = true;
    if (m_bSortAscending) bGreater = false;
    if ( rpStart.m_bIsFolder == rpEnd.m_bIsFolder)
    {
      char szfilename1[1024];
      char szfilename2[1024];
      CStdString strStart, strEnd;

      switch ( m_iSortMethod )
      {
      case 0:  // Sort by name
        strStart = rpStart.GetLabel();
        strEnd = rpEnd.GetLabel();
        if (g_guiSettings.GetBool("MyVideos.IgnoreTheWhenSorting") && strStart.Left(4).Equals("The "))
          strStart = strStart.Mid(4);
        if (g_guiSettings.GetBool("MyVideos.IgnoreTheWhenSorting") && strEnd.Left(4).Equals("The "))
          strEnd = strEnd.Mid(4);
        strcpy(szfilename1, strStart.c_str());
        strcpy(szfilename2, strEnd.c_str());
        break;

      case 1:  // Sort by year
        if ( rpStart.m_stTime.wYear > rpEnd.m_stTime.wYear ) return bGreater;
        if ( rpStart.m_stTime.wYear < rpEnd.m_stTime.wYear ) return !bGreater;

        strcpy(szfilename1, rpStart.GetLabel().c_str());
        strcpy(szfilename2, rpEnd.GetLabel().c_str());
        break;

      case 2:  // sort by rating
        if ( rpStart.m_fRating < rpEnd.m_fRating) return bGreater;
        if ( rpStart.m_fRating > rpEnd.m_fRating) return !bGreater;


        strcpy(szfilename1, rpStart.GetLabel().c_str());
        strcpy(szfilename2, rpEnd.GetLabel().c_str());
        break;

      case 3:  // sort dvdLabel
        {
          int iLabel1 = 0, iLabel2 = 0;
          char szTmp[20];
          int pos = 0;
          strcpy(szTmp, "");
          for (int x = 0; x < (int)rpStart.m_strDVDLabel.size(); x++)
          {
            char k = rpStart.m_strDVDLabel.GetAt(x);
            if (k >= '0' && k <= '9')
            {
              if ( (k == '0' && pos > 0) || (k != '0' ) )
              {
                szTmp[pos++] = k;
                szTmp[pos] = 0;
              }
            }
          }
          sscanf(szTmp, "%i", &iLabel1);
          strcpy(szTmp, "");
          pos = 0;
          for (int x = 0; x < (int)rpEnd.m_strDVDLabel.size(); x++)
          {
            char k = rpEnd.m_strDVDLabel.GetAt(x);
            if (k >= '0' && k <= '9')
            {
              if ( (k == '0' && pos > 0) || (k != '0' ) )
              {
                szTmp[pos++] = k;
                szTmp[pos] = 0;
              }
            }
          }
          sscanf(szTmp, "%i", &iLabel2);

          if ( iLabel1 < iLabel2) return bGreater;
          if ( iLabel1 > iLabel2) return !bGreater;

          strcpy(szfilename1, rpStart.GetLabel().c_str());
          strcpy(szfilename2, rpEnd.GetLabel().c_str());

        }
        break;

      default:  // Sort by Filename by default
        strStart = rpStart.GetLabel();
        strEnd = rpEnd.GetLabel();
        if (g_guiSettings.GetBool("MyVideos.IgnoreTheWhenSorting") && strStart.Left(4).Equals("The "))
          strStart = strStart.Mid(4);
        if (g_guiSettings.GetBool("MyVideos.IgnoreTheWhenSorting") && strEnd.Left(4).Equals("The "))
          strEnd = strEnd.Mid(4);
        strcpy(szfilename1, strStart.c_str());
        strcpy(szfilename2, strEnd.c_str());
        break;
      }


      for (int i = 0; i < (int)strlen(szfilename1); i++)
        szfilename1[i] = tolower((unsigned char)szfilename1[i]);

      for (i = 0; i < (int)strlen(szfilename2); i++)
        szfilename2[i] = tolower((unsigned char)szfilename2[i]);
      //return (rpStart.strPath.compare( rpEnd.strPath )<0);

      if (m_bSortAscending)
        return (strcmp(szfilename1, szfilename2) < 0);
      else
        return (strcmp(szfilename1, szfilename2) >= 0);
    }
    if (!rpStart.m_bIsFolder) return false;
    return true;
  }
  static bool m_bSortAscending;
  static int m_iSortMethod;
};
bool SSortVideoTitleByTitle::m_bSortAscending;
int SSortVideoTitleByTitle::m_iSortMethod;

//****************************************************************************************************************************
CGUIWindowVideoTitle::CGUIWindowVideoTitle()
: CGUIWindowVideoBase(WINDOW_VIDEO_TITLE, "MyVideoTitle.xml")
{
  m_Directory.m_strPath = "";
}

//****************************************************************************************************************************
CGUIWindowVideoTitle::~CGUIWindowVideoTitle()
{
}

//****************************************************************************************************************************
bool CGUIWindowVideoTitle::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNSORTBY) // sort by
      {
        g_stSettings.m_iMyVideoTitleSortMethod++;
        if (g_stSettings.m_iMyVideoTitleSortMethod >= 4)
          g_stSettings.m_iMyVideoTitleSortMethod = 0;
        g_settings.Save();

        UpdateButtons();
        OnSort();
      }
      else if (iControl == CONTROL_BTNSORTASC) // sort asc
      {
        g_stSettings.m_bMyVideoTitleSortAscending = !g_stSettings.m_bMyVideoTitleSortAscending;
        g_settings.Save();
        UpdateButtons();
        OnSort();
      }
      else if (iControl == CONTROL_BTNVIEWASICONS)
      {
        if (m_Directory.IsVirtualDirectoryRoot())
        {
          m_iViewAsIconsRoot++;
          if (m_iViewAsIconsRoot > VIEW_AS_LARGE_LIST) m_iViewAsIconsRoot = VIEW_AS_LIST;
        }
        else
        {
          m_iViewAsIcons++;
          if (m_iViewAsIcons > VIEW_AS_LARGE_LIST) m_iViewAsIcons = VIEW_AS_LIST;
        }

        SaveViewMode();
        UpdateButtons();
        return true;
      }
      else
        return CGUIWindowVideoBase::OnMessage(message);
    }
  }
  return CGUIWindowVideoBase::OnMessage(message);
}

//****************************************************************************************************************************
void CGUIWindowVideoTitle::FormatItemLabels()
{
  for (int i = 0; i < (int)m_vecItems.Size(); i++)
  {
    CFileItem* pItem = m_vecItems[i];
    if (g_stSettings.m_iMyVideoTitleSortMethod == 0 || g_stSettings.m_iMyVideoTitleSortMethod == 2)
    {
      if (pItem->m_bIsFolder) pItem->SetLabel2("");
      else
      {
        CStdString strRating;
        strRating.Format("%2.2f", pItem->m_fRating);
        pItem->SetLabel2(strRating);
      }
    }
    else if (g_stSettings.m_iMyVideoTitleSortMethod == 3)
    {
      pItem->SetLabel2(pItem->m_strDVDLabel);
    }
    else
    {
      if (pItem->m_stTime.wYear )
      {
        CStdString strDateTime;
        strDateTime.Format("%i", pItem->m_stTime.wYear );
        pItem->SetLabel2(strDateTime);
      }
      else
        pItem->SetLabel2("");
    }
  }
}

void CGUIWindowVideoTitle::SortItems(CFileItemList& items)
{
  SSortVideoTitleByTitle::m_iSortMethod = g_stSettings.m_iMyVideoTitleSortMethod;
  SSortVideoTitleByTitle::m_bSortAscending = g_stSettings.m_bMyVideoTitleSortAscending;
  if (g_stSettings.m_iMyVideoTitleSortMethod == 1)
    SSortVideoTitleByTitle::m_bSortAscending = !SSortVideoTitleByTitle::m_bSortAscending;
  items.Sort(SSortVideoTitleByTitle::Sort);
}

//****************************************************************************************************************************
bool CGUIWindowVideoTitle::Update(const CStdString &strDirectory)
{
  // get selected item
  int iItem = m_viewControl.GetSelectedItem();
  CStdString strSelectedItem = "";
  if (iItem >= 0 && iItem < (int)m_vecItems.Size())
  {
    CFileItem* pItem = m_vecItems[iItem];
    if (pItem->GetLabel() != "..")
    {
      strSelectedItem = pItem->m_strPath;
      m_history.Set(strSelectedItem, m_Directory.m_strPath);
    }
  }
  ClearFileItems();
  m_Directory.m_strPath = strDirectory;
  VECMOVIES movies;
  m_database.GetMovies(movies);
  // Display an error message if the database doesn't contain any movies
  DisplayEmptyDatabaseMessage(movies.empty());
  for (int i = 0; i < (int)movies.size(); ++i)
  {
    CIMDBMovie movie = movies[i];
    CFileItem *pItem = new CFileItem(movie.m_strTitle);
    pItem->m_strPath = movie.m_strSearchString;
    pItem->m_bIsFolder = false;
    pItem->m_bIsShareOrDrive = false;

    CStdString strThumb;
    CUtil::GetVideoThumbnail(movie.m_strIMDBNumber, strThumb);
    if (CFile::Exists(strThumb))
      pItem->SetThumbnailImage(strThumb);
    pItem->m_fRating = movie.m_fRating;
    pItem->m_stTime.wYear = movie.m_iYear & 0xFFFF;
    pItem->m_strDVDLabel = movie.m_strDVDLabel;
    m_vecItems.Add(pItem);
  }
  SET_CONTROL_LABEL(LABEL_TITLE, m_Directory.m_strPath);

  m_vecItems.SetThumbs();
  SetIMDBThumbs(m_vecItems);

  // Fill in default icons
  CStdString strPath;
  for (int i = 0; i < (int)m_vecItems.Size(); i++)
  {
    CFileItem* pItem = m_vecItems[i];
    strPath = pItem->m_strPath;
    // Fake a videofile
    pItem->m_strPath = pItem->m_strPath + ".avi";

    pItem->FillInDefaultIcon();
    pItem->m_strPath = strPath;
  }

  OnSort();
  UpdateButtons();
  strSelectedItem = m_history.Get(m_Directory.m_strPath);

  for (int i = 0; i < (int)m_vecItems.Size(); ++i)
  {
    CFileItem* pItem = m_vecItems[i];
    if (pItem->m_strPath == strSelectedItem)
    {
      m_viewControl.SetSelectedItem(i);
      break;
    }
  }

  return true;
}

//****************************************************************************************************************************
void CGUIWindowVideoTitle::OnClick(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];
  CStdString strPath = pItem->m_strPath;

  CStdString strExtension;
  CUtil::GetExtension(pItem->m_strPath, strExtension);

  if (pItem->m_bIsFolder)
  {
    m_iItemSelected = -1;
    if ( pItem->m_bIsShareOrDrive )
    {
      if ( !g_passwordManager.IsItemUnlocked( pItem, "video" ) )
        return ;

      if ( !HaveDiscOrConnection( pItem->m_strPath, pItem->m_iDriveType ) )
        return ;
    }
    Update(strPath);
  }
  else
  {
    m_iItemSelected = m_viewControl.GetSelectedItem();
    int iSelectedFile = 1;
    VECMOVIESFILES movies;
    m_database.GetFiles(atol(pItem->m_strPath), movies);
    PlayMovies(movies, pItem->m_lStartOffset);
  }
}

void CGUIWindowVideoTitle::OnDeleteItem(int iItem)
{
  if (iItem < 0 || iItem >= (int)m_vecItems.Size()) return;

  CFileItem* pItem = m_vecItems[iItem];
  if (pItem->m_bIsFolder) return;

  CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (!pDialog) return;
  pDialog->SetHeading(432);
  pDialog->SetLine(0, 433);
  pDialog->SetLine(1, 434);
  pDialog->SetLine(2, L"");
  pDialog->DoModal(GetID());
  if (!pDialog->IsConfirmed()) return;
  VECMOVIESFILES movies;
  m_database.GetFiles(atol(pItem->m_strPath), movies);
  if (movies.size() <= 0) return;
  m_database.DeleteMovie(movies[0]);

  Update( m_Directory.m_strPath );
  m_viewControl.SetSelectedItem(iItem);
  return;
}

void CGUIWindowVideoTitle::LoadViewMode()
{
  m_iViewAsIconsRoot = g_stSettings.m_iMyVideoTitleRootViewAsIcons;
  m_iViewAsIcons = g_stSettings.m_iMyVideoTitleViewAsIcons;
}

void CGUIWindowVideoTitle::SaveViewMode()
{
  g_stSettings.m_iMyVideoTitleRootViewAsIcons = m_iViewAsIconsRoot;
  g_stSettings.m_iMyVideoTitleViewAsIcons = m_iViewAsIcons;
  g_settings.Save();
}

int CGUIWindowVideoTitle::SortMethod()
{
  if (g_stSettings.m_iMyVideoTitleSortMethod >= 0 && g_stSettings.m_iMyVideoTitleSortMethod < 3)
    return 365 + g_stSettings.m_iMyVideoTitleSortMethod;
  if (g_stSettings.m_iMyVideoTitleSortMethod == 3)
    return 430;
  return -1;
}

bool CGUIWindowVideoTitle::SortAscending()
{
  return g_stSettings.m_bMyVideoTitleSortAscending;
}

void CGUIWindowVideoTitle::OnQueueItem(int iItem)
{
  if (iItem < 0 || iItem >= (int)m_vecItems.Size()) return;

  CFileItem* pItem = m_vecItems[iItem];
  if (pItem->m_bIsFolder) return;

  CLog::Log(LOGDEBUG,"CGUIWindowVideoTitle::OnQueueItem, iItem = %i, movie id = %s", iItem, pItem->m_strPath.c_str());

  VECMOVIESFILES movies;
  m_database.GetFiles(atol(pItem->m_strPath), movies);
  if (movies.size() <= 0) return;
  for (int i = 0; i < (int)movies.size(); ++i)
  {
    CFileItem* pMovieFile = new CFileItem(movies[i], false);
    CStdString strFileNum;
    strFileNum.Format("(%2.2i)",i+1);
    pMovieFile->SetLabel(pItem->GetLabel() + " " + strFileNum);
    CLog::Log(LOGDEBUG,"  file = %s, label = %s", movies[i].c_str(),pMovieFile->GetLabel().c_str());
    AddItemToPlayList(pMovieFile);
  }

  //move to next item
  m_viewControl.SetSelectedItem(iItem + 1);
}