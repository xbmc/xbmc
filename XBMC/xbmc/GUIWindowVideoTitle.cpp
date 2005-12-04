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
#include "SortFileItem.h"

#define CONTROL_BTNVIEWASICONS    2
#define CONTROL_BTNSORTBY         3
#define CONTROL_BTNSORTASC        4
#define CONTROL_BTNTYPE           5
#define CONTROL_PLAY_DVD          6
#define CONTROL_STACK             7
#define CONTROL_IMDB              9
#define CONTROL_BTNSHOWMODE       10
#define CONTROL_LIST              50
#define CONTROL_THUMBS            51
#define CONTROL_BIGLIST           52
#define CONTROL_LABELFILES        12
#define LABEL_TITLE              100

/*  REMOVED: There was a method here for sort by DVD label, but as this
    doesn't seem to be used anywhere (at least it is not given a value
    anywhere) I've removed it. (JM, 3 Dec 2005) */

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
  case GUI_MSG_WINDOW_INIT:
    {
      m_iShowMode = g_stSettings.m_iMyVideoTitleShowMode;
    }
    break;
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNSORTBY) // sort by
      {
        g_stSettings.m_iMyVideoTitleSortMethod++;
        if (g_stSettings.m_iMyVideoTitleSortMethod >= 3 /* JM - this was 4 when we had DVDlabel filtering */)
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
      else if (iControl == CONTROL_BTNSHOWMODE)
	    {
        m_iShowMode++;
		    if (m_iShowMode > VIDEO_SHOW_WATCHED) m_iShowMode = VIDEO_SHOW_ALL;
		    g_stSettings.m_iMyVideoTitleShowMode = m_iShowMode;
        g_settings.Save();
		    Update(m_Directory.m_strPath);
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
  int sortMethod = g_stSettings.m_iMyVideoTitleSortMethod;
  bool sortAscending = g_stSettings.m_bMyVideoTitleSortAscending;
  if (g_stSettings.m_iMyVideoTitleSortMethod == 1)
    sortAscending = !sortAscending;
  switch (sortMethod)
  {
  case 1:
    items.Sort(sortAscending ? SSortFileItem::MovieYearAscending : SSortFileItem::MovieYearDescending); break;
  case 2:
    items.Sort(sortAscending ? SSortFileItem::MovieRatingAscending : SSortFileItem::MovieRatingDescending); break;
  default:
    items.Sort(sortAscending ? SSortFileItem::LabelAscending : SSortFileItem::LabelDescending); break;
  }
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
    PlayMovie(pItem);
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

  CStdString path;
  m_database.GetFilePath(atol(pItem->m_strPath), path);
  if (path.IsEmpty()) return;
  m_database.DeleteMovie(path);

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
  CGUIWindowVideoBase::OnQueueItem(iItem);
}