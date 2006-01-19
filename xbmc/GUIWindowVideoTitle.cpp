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

#define CONTROL_PLAY_DVD          6
#define CONTROL_STACK             7
#define CONTROL_IMDB              9
#define CONTROL_BTNSHOWMODE       10
#define LABEL_TITLE              100

/*  REMOVED: There was a method here for sort by DVD label, but as this
    doesn't seem to be used anywhere (at least it is not given a value
    anywhere) I've removed it. (JM, 3 Dec 2005) */

//****************************************************************************************************************************
CGUIWindowVideoTitle::CGUIWindowVideoTitle()
: CGUIWindowVideoBase(WINDOW_VIDEO_TITLE, "MyVideoTitle.xml")
{
  m_vecItems.m_strPath = "";
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
  case GUI_MSG_WINDOW_INIT:
    {
      m_iShowMode = g_stSettings.m_iMyVideoTitleShowMode;
    }
    break;
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNSHOWMODE)
	    {
        m_iShowMode++;
		    if (m_iShowMode > VIDEO_SHOW_WATCHED) m_iShowMode = VIDEO_SHOW_ALL;
		    g_stSettings.m_iMyVideoTitleShowMode = m_iShowMode;
        g_settings.Save();
		    Update(m_vecItems.m_strPath);
        return true;
      }
      else
        return CGUIWindowVideoBase::OnMessage(message);
    }
  }
  return CGUIWindowVideoBase::OnMessage(message);
}

bool CGUIWindowVideoTitle::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  items.m_strPath = strDirectory;
  VECMOVIES movies;
  m_database.GetMovies(movies);
  // Display an error message if the database doesn't contain any movies
  DisplayEmptyDatabaseMessage(movies.empty());
  for (int i = 0; i < (int)movies.size(); ++i)
  {
    CIMDBMovie movie = movies[i];
    // add the appropiate movies to m_vecItems based on the showmode
    if (
      (m_iShowMode == VIDEO_SHOW_ALL) ||
      (m_iShowMode == VIDEO_SHOW_WATCHED && movie.m_bWatched == true) ||
      (m_iShowMode == VIDEO_SHOW_UNWATCHED && movie.m_bWatched == false)
      ) 
    {
      // mark watched movies when showing all
      CStdString strTitle = movie.m_strTitle;
      if (m_iShowMode == VIDEO_SHOW_ALL && movie.m_bWatched == true)
        strTitle += " [W]";
      CFileItem *pItem = new CFileItem(strTitle);
      pItem->m_strPath = movie.m_strSearchString;
      pItem->m_strTitle=strTitle;
      pItem->m_bIsFolder = false;
      pItem->m_bIsShareOrDrive = false;

      CStdString strThumb;
      CUtil::GetVideoThumbnail(movie.m_strIMDBNumber, strThumb);
      if (CFile::Exists(strThumb))
        pItem->SetThumbnailImage(strThumb);
      pItem->m_fRating = movie.m_fRating;
      pItem->m_stTime.wYear = movie.m_iYear & 0xFFFF;
      pItem->m_strDVDLabel = movie.m_strDVDLabel;
      items.Add(pItem);
    }
  }

  return true;
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
    if (!pItem->IsParentFolder())
    {
      strSelectedItem = pItem->m_strPath;
      m_history.SetSelectedItem(strSelectedItem, m_vecItems.m_strPath);
    }
  }
  ClearFileItems();

  CStdString strOldDirectory = m_vecItems.m_strPath;

  if (!GetDirectory(strDirectory, m_vecItems))
    return !Update(strOldDirectory); // We assume, we can get the parent 
                                     // directory again, but we have to 
                                     // return false to be able to eg. show 
                                     // an error message.

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
  strSelectedItem = m_history.GetSelectedItem(m_vecItems.m_strPath);

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

void CGUIWindowVideoTitle::UpdateButtons()
{
  CGUIWindowVideoBase::UpdateButtons();
  SET_CONTROL_LABEL(LABEL_TITLE, m_vecItems.m_strPath);
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

  CStdString path;
  m_database.GetFilePath(atol(pItem->m_strPath), path);
  if (path.IsEmpty()) return;
  m_database.DeleteMovie(path);

  Update( m_vecItems.m_strPath );
  m_viewControl.SetSelectedItem(iItem);
  return;
}

void CGUIWindowVideoTitle::OnQueueItem(int iItem)
{
  CGUIWindowVideoBase::OnQueueItem(iItem);
}